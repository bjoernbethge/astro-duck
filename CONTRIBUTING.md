# Contributing to DuckDB Astro Extension

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## 🚀 Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/astro-duck.git
   cd astro-duck
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