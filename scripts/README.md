# 🚀 Deployment Scripts

This directory contains scripts for deploying the DuckDB Astro Extension to various platforms and repositories.

## 📁 Scripts Overview

### `astro-extension-upload.sh`
Main deployment script that handles:
- Extension binary processing and signing
- Compression (gzip for native, brotli for WebAssembly)
- Multi-target deployment (GitHub, S3, Community)
- Metadata generation

**Usage:**
```bash
./scripts/astro-extension-upload.sh <version> <duckdb_version> <architecture> [target]
```

**Examples:**
```bash
# Deploy to GitHub (default)
./scripts/astro-extension-upload.sh 1.0.0 v1.2.1 linux_amd64 github

# Deploy to S3
./scripts/astro-extension-upload.sh 1.0.0 v1.2.1 linux_amd64 s3

# Prepare for Community Extensions
./scripts/astro-extension-upload.sh 1.0.0 v1.2.1 linux_amd64 community
```

### `deploy-local.sh`
Local testing script that:
- Builds extension if needed
- Tests all deployment targets
- Shows file sizes and compression ratios
- Provides next steps guidance

**Usage:**
```bash
./scripts/deploy-local.sh [version] [duckdb_version] [architecture]
```

### `setup-github-repo.sh`
Repository setup helper that:
- Initializes Git repository
- Creates standard files (README, LICENSE, etc.)
- Provides GitHub setup instructions

### `submit-to-community.sh`
Community extension submission helper that:
- Clones DuckDB community-extensions repository
- Customizes description.yml
- Prepares Pull Request

### `extension-upload.sh`
Original DuckDB extension upload script (reference implementation).

## 🎯 Deployment Targets

### 1. GitHub Releases (`github`)
- Creates compressed extension binaries
- Generates release metadata JSON
- Prepares artifacts for GitHub Actions

### 2. AWS S3 (`s3`)
- Uploads to versioned S3 paths
- Supports latest version tagging
- Handles WebAssembly content encoding

### 3. Community Extensions (`community`)
- Generates `description.yml` for DuckDB Community Extensions
- Includes comprehensive documentation
- Ready for Pull Request submission

## 🏗️ Architecture Support

| Architecture | Platform | Status |
|-------------|----------|---------|
| `linux_amd64` | Linux x86_64 | ✅ |
| `linux_arm64` | Linux ARM64 | ✅ |
| `osx_amd64` | macOS Intel | ✅ |
| `osx_arm64` | macOS Apple Silicon | ✅ |
| `windows_amd64` | Windows x86_64 | ✅ |
| `wasm_mvp` | WebAssembly | ✅ |

## 🔧 Quick Start

1. **Test deployment locally**:
   ```bash
   ./scripts/deploy-local.sh
   ```

2. **Deploy to community**:
   ```bash
   ./scripts/astro-extension-upload.sh 1.0.0 v1.2.1 linux_amd64 community
   ```

3. **Submit to DuckDB Community Extensions**:
   ```bash
   ./scripts/submit-to-community.sh
   ```

## 📚 Related Documentation

- [Main README](../README.md)
- [Community Sharing Guide](../COMMUNITY_SHARING_GUIDE.md)
- [Community Submission Guide](../COMMUNITY_SUBMISSION_GUIDE.md) 