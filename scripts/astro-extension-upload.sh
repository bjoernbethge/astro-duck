#!/bin/bash

# Astro Extension Upload Script
# Customized for DuckDB Astro Extension deployment

# Usage: ./astro-extension-upload.sh <extension_version> <duckdb_version> <architecture> [upload_target]
# <extension_version>   : Version (commit / version tag) of the Astro extension
# <duckdb_version>      : Version (commit / version tag) of DuckDB
# <architecture>        : Architecture target (linux_amd64, osx_arm64, windows_amd64, wasm_mvp, etc.)
# [upload_target]       : Where to upload ("github", "s3", "community", default: "github")

set -e

EXTENSION_NAME="astro"
EXTENSION_VERSION=${1:-"1.0.0"}
DUCKDB_VERSION=${2:-"v1.2.1"}
ARCHITECTURE=${3:-"linux_amd64"}
UPLOAD_TARGET=${4:-"github"}

echo "🚀 Deploying Astro Extension"
echo "   Extension: $EXTENSION_NAME v$EXTENSION_VERSION"
echo "   DuckDB: $DUCKDB_VERSION"
echo "   Architecture: $ARCHITECTURE"
echo "   Target: $UPLOAD_TARGET"

# Determine extension file path and name
if [[ $ARCHITECTURE == wasm* ]]; then
  EXT_FILE="build/astro_release/extension/astro/$EXTENSION_NAME.duckdb_extension.wasm"
  EXT_COMPRESSED="$EXTENSION_NAME.duckdb_extension.wasm"
else
  EXT_FILE="build/astro_release/extension/astro/$EXTENSION_NAME.duckdb_extension"
  EXT_COMPRESSED="$EXTENSION_NAME.duckdb_extension.gz"
fi

# Check if extension file exists
if [ ! -f "$EXT_FILE" ]; then
    echo "❌ Extension file not found: $EXT_FILE"
    echo "   Please build the extension first with: make release"
    exit 1
fi

echo "✅ Found extension file: $EXT_FILE"

# Create deployment directory
DEPLOY_DIR="deploy/$EXTENSION_VERSION/$ARCHITECTURE"
mkdir -p "$DEPLOY_DIR"

# Copy extension to deployment directory
cp "$EXT_FILE" "$DEPLOY_DIR/"

# Create working copy for processing
WORK_FILE="/tmp/$EXTENSION_NAME.duckdb_extension"
cp "$EXT_FILE" "$WORK_FILE"

echo "📝 Processing extension binary..."

# Calculate SHA256 hash
cat "$WORK_FILE" > "$WORK_FILE.append"

# Add WebAssembly custom section for WASM builds
if [[ $ARCHITECTURE == wasm* ]]; then
  echo "🌐 Adding WebAssembly signature section..."
  # Custom section header for signature
  echo -n -e '\x00' >> "$WORK_FILE.append"           # Custom section ID
  echo -n -e '\x93\x02' >> "$WORK_FILE.append"       # Section length
  echo -n -e '\x10' >> "$WORK_FILE.append"           # Name length (16)
  echo -n -e 'duckdb_signature' >> "$WORK_FILE.append" # Section name
  echo -n -e '\x80\x02' >> "$WORK_FILE.append"       # Signature length (256)
fi

# Sign extension if signing key is available
if [ "$DUCKDB_EXTENSION_SIGNING_PK" != "" ]; then
    echo "🔐 Signing extension with provided key..."
    echo "$DUCKDB_EXTENSION_SIGNING_PK" > private.pem
    
    # Compute hash
    script_dir="$(dirname "$(readlink -f "$0")")"
    if [ -f "$script_dir/../duckdb/scripts/compute-extension-hash.sh" ]; then
        $script_dir/../duckdb/scripts/compute-extension-hash.sh "$WORK_FILE.append" > "$WORK_FILE.hash"
    else
        # Fallback: compute SHA256 directly
        sha256sum "$WORK_FILE.append" | cut -d' ' -f1 > "$WORK_FILE.hash"
    fi
    
    # Sign the hash
    openssl pkeyutl -sign -in "$WORK_FILE.hash" -inkey private.pem -pkeyopt digest:sha256 -out "$WORK_FILE.sign"
    rm -f private.pem
    echo "✅ Extension signed successfully"
else
    echo "⚠️  No signing key provided - creating unsigned extension"
    # Create empty signature (256 zeros)
    dd if=/dev/zero of="$WORK_FILE.sign" bs=256 count=1 2>/dev/null
fi

# Ensure signature is exactly 256 bytes
truncate -s 256 "$WORK_FILE.sign"

# Append signature to extension
cat "$WORK_FILE.sign" >> "$WORK_FILE.append"

# Compress extension
echo "🗜️  Compressing extension..."
if [[ $ARCHITECTURE == wasm* ]]; then
    # Use Brotli for WebAssembly
    if command -v brotli &> /dev/null; then
        brotli < "$WORK_FILE.append" > "$DEPLOY_DIR/$EXT_COMPRESSED"
    else
        echo "❌ Brotli not found - required for WebAssembly builds"
        exit 1
    fi
else
    # Use gzip for other architectures
    gzip < "$WORK_FILE.append" > "$DEPLOY_DIR/$EXT_COMPRESSED"
fi

# Calculate final file sizes
ORIGINAL_SIZE=$(stat -c%s "$EXT_FILE")
COMPRESSED_SIZE=$(stat -c%s "$DEPLOY_DIR/$EXT_COMPRESSED")
COMPRESSION_RATIO=$(echo "scale=1; $COMPRESSED_SIZE * 100 / $ORIGINAL_SIZE" | bc)

echo "📊 Compression Results:"
echo "   Original: $(numfmt --to=iec $ORIGINAL_SIZE)"
echo "   Compressed: $(numfmt --to=iec $COMPRESSED_SIZE)"
echo "   Ratio: ${COMPRESSION_RATIO}%"

# Upload based on target
case $UPLOAD_TARGET in
    "github")
        echo "📤 Preparing GitHub Release artifacts..."
        
        # Create release info
        cat > "$DEPLOY_DIR/release-info.json" << EOF
{
  "extension_name": "$EXTENSION_NAME",
  "extension_version": "$EXTENSION_VERSION",
  "duckdb_version": "$DUCKDB_VERSION",
  "architecture": "$ARCHITECTURE",
  "original_size": $ORIGINAL_SIZE,
  "compressed_size": $COMPRESSED_SIZE,
  "compression_ratio": "$COMPRESSION_RATIO%",
  "build_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "build_host": "$(hostname)",
  "git_commit": "$(git rev-parse HEAD 2>/dev/null || echo 'unknown')"
}
EOF
        
        echo "✅ GitHub artifacts ready in: $DEPLOY_DIR"
        echo "   Files:"
        ls -la "$DEPLOY_DIR"
        ;;
        
    "s3")
        if [ -z "$AWS_ACCESS_KEY_ID" ] || [ -z "$S3_BUCKET" ]; then
            echo "❌ AWS credentials or S3_BUCKET not set"
            echo "   Required: AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, S3_BUCKET"
            exit 1
        fi
        
        echo "☁️  Uploading to S3: s3://$S3_BUCKET"
        
        # Upload versioned version
        S3_PATH="$S3_BUCKET/$EXTENSION_NAME/$EXTENSION_VERSION/$DUCKDB_VERSION/$ARCHITECTURE"
        
        if [[ $ARCHITECTURE == wasm* ]]; then
            aws s3 cp "$DEPLOY_DIR/$EXT_COMPRESSED" "s3://$S3_PATH/$EXT_COMPRESSED" \
                --acl public-read --content-encoding br --content-type="application/wasm"
        else
            aws s3 cp "$DEPLOY_DIR/$EXT_COMPRESSED" "s3://$S3_PATH/$EXT_COMPRESSED" \
                --acl public-read
        fi
        
        # Also upload as latest if specified
        if [ "$UPLOAD_AS_LATEST" = "true" ]; then
            LATEST_PATH="$S3_BUCKET/$DUCKDB_VERSION/$ARCHITECTURE"
            if [[ $ARCHITECTURE == wasm* ]]; then
                aws s3 cp "$DEPLOY_DIR/$EXT_COMPRESSED" "s3://$LATEST_PATH/$EXT_COMPRESSED" \
                    --acl public-read --content-encoding br --content-type="application/wasm"
            else
                aws s3 cp "$DEPLOY_DIR/$EXT_COMPRESSED" "s3://$LATEST_PATH/$EXT_COMPRESSED" \
                    --acl public-read
            fi
        fi
        
        echo "✅ S3 upload completed"
        ;;
        
    "community")
        echo "🏘️  Preparing for DuckDB Community Extensions..."
        
        # Create community extension structure
        COMMUNITY_DIR="deploy/community-extension"
        mkdir -p "$COMMUNITY_DIR"
        
        # Copy extension
        cp "$DEPLOY_DIR/$EXT_COMPRESSED" "$COMMUNITY_DIR/"
        
        # Create description.yml for community repository
        cat > "$COMMUNITY_DIR/description.yml" << EOF
extension:
  name: $EXTENSION_NAME
  description: "Comprehensive astronomical calculations and coordinate transformations for DuckDB with Arrow, Spatial, and Catalog integration"
  version: $EXTENSION_VERSION
  language: C++
  build: cmake
  license: MIT
  maintainers:
    - bjoern

repo:
  github: bjoern/astropy-extension
  ref: main

docs:
  hello_world: |
    -- Load the extension
    LOAD astro;
    
    -- Calculate angular separation between two stars
    SELECT angular_separation(45.0, 30.0, 46.0, 31.0) as separation_degrees;
    
    -- Convert coordinates with enhanced metadata
    SELECT radec_to_cartesian(45.0, 30.0, 10.0) as coords_json;
    
    -- Create celestial geometry point
    SELECT celestial_point(45.0, 30.0, 10.0) as wkt_geometry;

  extended_description: |
    The Astro extension provides a comprehensive suite of astronomical 
    calculations for DuckDB, including:
    
    **Coordinate Transformations:**
    - RA/Dec to Cartesian conversion with full metadata
    - Angular separation calculations
    - Celestial geometry point creation (WKT format)
    
    **Photometric Functions:**
    - Magnitude to flux conversion
    - Distance modulus calculations
    
    **Cosmological Functions:**
    - Luminosity distance calculations
    - Redshift to age conversion
    
    **Integration Features:**
    - Arrow format support for efficient data exchange
    - Spatial extension compatibility
    - Catalog metadata management
    - Enhanced JSON output with coordinate system information
    
    Perfect for astronomical data analysis, sky surveys, and 
    scientific computing workflows.
EOF
        
        echo "✅ Community extension files ready in: $COMMUNITY_DIR"
        echo "   Next steps:"
        echo "   1. Fork https://github.com/duckdb/community-extensions"
        echo "   2. Copy description.yml to extensions/astro/"
        echo "   3. Create Pull Request"
        ;;
        
    *)
        echo "❌ Unknown upload target: $UPLOAD_TARGET"
        echo "   Valid targets: github, s3, community"
        exit 1
        ;;
esac

# Cleanup temporary files
rm -f "$WORK_FILE"* 2>/dev/null || true

echo ""
echo "🎉 Astro Extension deployment completed successfully!"
echo "   Version: $EXTENSION_VERSION"
echo "   Architecture: $ARCHITECTURE"
echo "   Target: $UPLOAD_TARGET"
echo "   Deployment directory: $DEPLOY_DIR" 