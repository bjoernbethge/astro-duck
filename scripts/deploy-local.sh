#!/bin/bash

# Local Deployment Test Script
# Tests the deployment process without actually uploading

set -e

echo "🧪 Testing Astro Extension Deployment Locally"

# Default values
EXTENSION_VERSION=${1:-"1.1.0"}
DUCKDB_VERSION=${2:-"v1.5.1"}
ARCHITECTURE=${3:-"linux_amd64"}

echo "   Extension Version: $EXTENSION_VERSION"
echo "   DuckDB Version: $DUCKDB_VERSION"
echo "   Architecture: $ARCHITECTURE"

# Check if extension is built
EXT_FILE="build/astro_release/extension/astro/astro.duckdb_extension"

if [ ! -f "$EXT_FILE" ]; then
    echo "❌ Extension not found: $EXT_FILE"
    echo "   Building extension first..."
    make clean && make release
    
    if [ ! -f "$EXT_FILE" ]; then
        echo "❌ Build failed - extension still not found"
        exit 1
    fi
fi

echo "✅ Extension found: $EXT_FILE"

# Test all deployment targets
echo ""
echo "🎯 Testing GitHub deployment..."
./scripts/astro-extension-upload.sh "$EXTENSION_VERSION" "$DUCKDB_VERSION" "$ARCHITECTURE" "github"

echo ""
echo "🏘️  Testing Community deployment..."
./scripts/astro-extension-upload.sh "$EXTENSION_VERSION" "$DUCKDB_VERSION" "$ARCHITECTURE" "community"

echo ""
echo "📊 Deployment Summary:"
echo "   GitHub artifacts: deploy/$EXTENSION_VERSION/$ARCHITECTURE/"
echo "   Community files: deploy/community-extension/"

# Show file sizes and structure
echo ""
echo "📁 File Structure:"
find deploy -type f -exec ls -lh {} \; | while read line; do
    echo "   $line"
done

echo ""
echo "✅ Local deployment test completed successfully!"
echo ""
echo "🚀 Next Steps:"
echo "   1. Commit and push changes"
echo "   2. Create version tag: git tag v$EXTENSION_VERSION"
echo "   3. Push tag: git push origin v$EXTENSION_VERSION"
echo "   4. GitHub Actions will automatically deploy" 