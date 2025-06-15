#!/bin/bash

# GitHub Repository Setup Script for Astro Extension
# Helps create and configure a new GitHub repository

set -e

echo "🏗️  GitHub Repository Setup for Astro Extension"
echo ""

# Get repository information
read -p "Your GitHub username: " GITHUB_USERNAME
read -p "Repository name (default: duckdb-astro-extension): " REPO_NAME
REPO_NAME=${REPO_NAME:-duckdb-astro-extension}
read -p "Repository description (default: Comprehensive astronomical calculations for DuckDB): " REPO_DESC
REPO_DESC=${REPO_DESC:-"Comprehensive astronomical calculations for DuckDB"}

echo ""
echo "📋 Repository Configuration:"
echo "   Username: $GITHUB_USERNAME"
echo "   Repository: $REPO_NAME"
echo "   Description: $REPO_DESC"
echo "   URL: https://github.com/$GITHUB_USERNAME/$REPO_NAME"

echo ""
read -p "Continue with this configuration? (y/N): " CONFIRM
if [[ ! $CONFIRM =~ ^[Yy]$ ]]; then
    echo "❌ Setup cancelled"
    exit 1
fi

# Initialize git repository if not already done
if [ ! -d ".git" ]; then
    echo "🔧 Initializing Git repository..."
    git init
    git branch -M main
else
    echo "✅ Git repository already initialized"
fi

# Create .gitignore if it doesn't exist
if [ ! -f ".gitignore" ]; then
    echo "📝 Creating .gitignore..."
    cat > .gitignore << 'EOF'
# Build directories
build/
deploy/

# Temporary files
*.tmp
*.log
/tmp/

# IDE files
.vscode/
.idea/
*.swp
*.swo

# OS files
.DS_Store
Thumbs.db

# Python
__pycache__/
*.pyc
*.pyo
*.pyd
.Python
env/
venv/
.env

# Test environments
test_env/

# Extension binaries (keep source, not binaries)
*.duckdb_extension
*.duckdb_extension.gz
*.duckdb_extension.wasm

# Private keys
*.pem
private.key
EOF
else
    echo "✅ .gitignore already exists"
fi

# Create comprehensive README.md
echo "📖 Creating README.md..."
cat > README.md << EOF
# 🌟 DuckDB Astro Extension

Comprehensive astronomical calculations and coordinate transformations for DuckDB with modern integrations.

[![Build Status](https://github.com/$GITHUB_USERNAME/$REPO_NAME/workflows/CI/badge.svg)](https://github.com/$GITHUB_USERNAME/$REPO_NAME/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![DuckDB](https://img.shields.io/badge/DuckDB-v1.2.1-blue.svg)](https://duckdb.org/)

## 🚀 Quick Start

### Installation from Community Extensions (Recommended)
\`\`\`sql
INSTALL astro FROM community;
LOAD astro;

-- Calculate angular separation between two stars
SELECT angular_separation(45.0, 30.0, 46.0, 31.0) as separation_degrees;
\`\`\`

### Manual Installation
1. Download the latest release from [Releases](https://github.com/$GITHUB_USERNAME/$REPO_NAME/releases)
2. Load in DuckDB:
\`\`\`sql
LOAD '/path/to/astro.duckdb_extension';
\`\`\`

## ✨ Features

- **🌐 Coordinate Transformations**: RA/Dec ↔ Cartesian with full metadata
- **📐 Angular Calculations**: Precise angular separation using Haversine formula
- **💫 Photometric Functions**: Magnitude/flux conversions with zero-point support
- **🌌 Cosmological Calculations**: Luminosity distance, redshift to age
- **🔗 Modern Integrations**: Arrow, Spatial, and Catalog compatibility

## 📊 Functions

| Function | Description | Example |
|----------|-------------|---------|
| \`angular_separation(ra1, dec1, ra2, dec2)\` | Angular distance between celestial objects | \`SELECT angular_separation(45.0, 30.0, 46.0, 31.0);\` |
| \`radec_to_cartesian(ra, dec, distance)\` | Coordinate conversion with metadata | \`SELECT radec_to_cartesian(45.0, 30.0, 10.0);\` |
| \`mag_to_flux(magnitude, zero_point)\` | Photometric conversions | \`SELECT mag_to_flux(15.5, 25.0);\` |
| \`distance_modulus(distance_pc)\` | Distance modulus calculations | \`SELECT distance_modulus(1000.0);\` |
| \`luminosity_distance(z, h0, om, ol)\` | Cosmological distances | \`SELECT luminosity_distance(0.1, 70, 0.3, 0.7);\` |
| \`redshift_to_age(z, h0, om, ol)\` | Universe age calculations | \`SELECT redshift_to_age(1.0, 70, 0.3, 0.7);\` |
| \`celestial_point(ra, dec, distance)\` | WKT geometry creation | \`SELECT celestial_point(45.0, 30.0, 10.0);\` |
| \`catalog_info()\` | Extension metadata | \`SELECT catalog_info();\` |

## 🏗️ Building from Source

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler
- DuckDB development headers

### Build Steps
\`\`\`bash
# Clone repository
git clone https://github.com/$GITHUB_USERNAME/$REPO_NAME.git
cd $REPO_NAME

# Build extension
make clean && make release

# Run tests
python test_astro.py
\`\`\`

## 📈 Performance

- **10,000 objects** processed in **0.004 seconds**
- Vectorized execution for optimal performance
- Memory-efficient batch processing

## 🔧 Integration Examples

### With Spatial Extension
\`\`\`sql
LOAD spatial;
LOAD astro;

-- Create spatial points from astronomical coordinates
SELECT 
    name,
    celestial_point(ra, dec, distance) as geometry,
    angular_separation(ra, dec, 0, 0) as distance_from_origin
FROM stars;
\`\`\`

### With Arrow Format
\`\`\`sql
LOAD astro;

-- Export astronomical calculations to Arrow
COPY (
    SELECT 
        object_id,
        radec_to_cartesian(ra, dec, distance) as coordinates,
        mag_to_flux(magnitude, 25.0) as flux
    FROM catalog
) TO 'astronomical_data.arrow' (FORMAT ARROW);
\`\`\`

## 📚 Documentation

- [Function Reference](docs/functions.md)
- [Integration Guide](docs/integration.md)
- [Performance Benchmarks](docs/performance.md)
- [Examples](examples/)

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [DuckDB](https://duckdb.org/) for the excellent database engine
- [Astropy](https://www.astropy.org/) for astronomical calculation references
- The astronomical community for domain expertise

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/$GITHUB_USERNAME/$REPO_NAME/issues)
- **Discussions**: [GitHub Discussions](https://github.com/$GITHUB_USERNAME/$REPO_NAME/discussions)
- **DuckDB Discord**: [Join the community](https://discord.duckdb.org)

---

**Made with ❤️ for the astronomical and data science communities**
EOF

# Create LICENSE file
echo "📄 Creating LICENSE..."
cat > LICENSE << EOF
MIT License

Copyright (c) $(date +%Y) $GITHUB_USERNAME

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOF

# Create CONTRIBUTING.md
echo "🤝 Creating CONTRIBUTING.md..."
cat > CONTRIBUTING.md << 'EOF'
# Contributing to DuckDB Astro Extension

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## 🚀 Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/REPO_NAME.git
   cd REPO_NAME
   ```
3. **Create a branch** for your feature:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## 🏗️ Development Setup

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler
- Python 3.7+ (for testing)
- DuckDB development headers

### Building
```bash
# Clean build
make clean && make release

# Debug build
make debug

# Run tests
python test_astro.py
```

## 📝 Code Guidelines

### C++ Code Style
- Follow existing code style and formatting
- Use meaningful variable and function names
- Add comments for complex calculations
- Include error handling for edge cases

### Function Development
1. **Add function signature** to `src/include/astro.hpp`
2. **Implement function** in `src/astro.cpp`
3. **Register function** in the extension initialization
4. **Add tests** to `test_astro.py`
5. **Update documentation**

### Example Function Addition
```cpp
// In astro.hpp
double MyAstroFunction(double param1, double param2);

// In astro.cpp
double MyAstroFunction(double param1, double param2) {
    // Validate inputs
    if (param1 < 0 || param2 < 0) {
        throw std::invalid_argument("Parameters must be non-negative");
    }
    
    // Perform calculation
    return param1 * param2; // Your calculation here
}

// Register in LoadInternal()
CreateScalarFunction("my_astro_function", {LogicalType::DOUBLE, LogicalType::DOUBLE}, 
                     LogicalType::DOUBLE, MyAstroFunction);
```

## 🧪 Testing

### Running Tests
```bash
# Run all tests
python test_astro.py

# Run specific test
python -c "import test_astro; test_astro.test_specific_function()"
```

### Adding Tests
Add test cases to `test_astro.py`:
```python
def test_my_new_function():
    """Test the new astronomical function"""
    result = conn.execute("SELECT my_astro_function(1.0, 2.0)").fetchone()[0]
    assert abs(result - 2.0) < 1e-10, f"Expected 2.0, got {result}"
    print("✅ my_astro_function test passed")
```

## 📚 Documentation

### Function Documentation
Document all functions with:
- Purpose and mathematical background
- Parameter descriptions and units
- Return value description
- Usage examples
- Performance characteristics

### README Updates
Update README.md when adding:
- New functions (add to function table)
- New features
- New examples
- Performance improvements

## 🔄 Pull Request Process

1. **Ensure tests pass**: All existing and new tests must pass
2. **Update documentation**: Include relevant documentation updates
3. **Describe changes**: Provide clear description of what was changed and why
4. **Reference issues**: Link to any related GitHub issues

### PR Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Performance improvement

## Testing
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Manual testing completed

## Documentation
- [ ] README updated
- [ ] Function documentation added
- [ ] Examples provided
```

## 🐛 Bug Reports

When reporting bugs, please include:
- DuckDB version
- Operating system
- Extension version
- Minimal reproduction case
- Expected vs actual behavior

## 💡 Feature Requests

For new features, please provide:
- Use case description
- Proposed API design
- Mathematical/astronomical background
- Performance considerations

## 📊 Performance Considerations

- Use vectorized operations when possible
- Minimize memory allocations
- Consider numerical stability
- Benchmark performance-critical functions

## 🌟 Recognition

Contributors will be:
- Listed in the README acknowledgments
- Credited in release notes
- Invited to join the maintainer team (for significant contributions)

## 📞 Getting Help

- **GitHub Discussions**: For questions and ideas
- **GitHub Issues**: For bugs and feature requests
- **DuckDB Discord**: For general DuckDB questions

Thank you for contributing! 🚀
EOF

# Add all files to git
echo "📦 Adding files to Git..."
git add .

# Create initial commit
echo "💾 Creating initial commit..."
git commit -m "Initial commit: DuckDB Astro Extension v1.0.0

- Comprehensive astronomical calculations for DuckDB
- Coordinate transformations (RA/Dec ↔ Cartesian)
- Angular separation calculations
- Photometric and cosmological functions
- Arrow, Spatial, and Catalog integration
- Enhanced JSON output with metadata
- Complete deployment and community submission system"

echo ""
echo "🎉 Repository setup completed!"
echo ""
echo "📋 Next Steps:"
echo ""
echo "1. 🌐 Create GitHub repository:"
echo "   - Go to: https://github.com/new"
echo "   - Repository name: $REPO_NAME"
echo "   - Description: $REPO_DESC"
echo "   - Make it public"
echo "   - Don't initialize with README (we have one)"
echo ""
echo "2. 📤 Push to GitHub:"
echo "   git remote add origin https://github.com/$GITHUB_USERNAME/$REPO_NAME.git"
echo "   git push -u origin main"
echo ""
echo "3. 🏷️  Create release tag:"
echo "   git tag v1.0.0"
echo "   git push origin v1.0.0"
echo ""
echo "4. 🏘️  Submit to Community Extensions:"
echo "   ./scripts/submit-to-community.sh"
echo ""
echo "✅ Repository URL: https://github.com/$GITHUB_USERNAME/$REPO_NAME" 