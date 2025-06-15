# 🌟 DuckDB Astro Extension

Comprehensive astronomical calculations and coordinate transformations for DuckDB with modern integrations.

[![Build Status](https://github.com/bjoernbethge/astro-duck/workflows/CI/badge.svg)](https://github.com/bjoernbethge/astro-duck/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![DuckDB](https://img.shields.io/badge/DuckDB-v1.2.1-blue.svg)](https://duckdb.org/)

## 🚀 Quick Start

### Installation from Community Extensions (Recommended)
```sql
INSTALL astro FROM community;
LOAD astro;

-- Calculate angular separation between two stars
SELECT angular_separation(45.0, 30.0, 46.0, 31.0) as separation_degrees;
```

### Manual Installation
1. Download the latest release from [Releases](https://github.com/bjoernbethge/astro-duck/releases)
2. Load in DuckDB:
```sql
LOAD '/path/to/astro.duckdb_extension';
```

## ✨ Features

- **🌐 Coordinate Transformations**: RA/Dec ↔ Cartesian with full metadata
- **📐 Angular Calculations**: Precise angular separation using Haversine formula
- **💫 Photometric Functions**: Magnitude/flux conversions with zero-point support
- **🌌 Cosmological Calculations**: Luminosity distance, redshift to age
- **🔗 Modern Integrations**: Arrow, Spatial, and Catalog compatibility

## 📊 Functions

| Function | Description | Example |
|----------|-------------|---------|
| `angular_separation(ra1, dec1, ra2, dec2)` | Angular distance between celestial objects | `SELECT angular_separation(45.0, 30.0, 46.0, 31.0);` |
| `radec_to_cartesian(ra, dec, distance)` | Coordinate conversion with metadata | `SELECT radec_to_cartesian(45.0, 30.0, 10.0);` |
| `mag_to_flux(magnitude, zero_point)` | Photometric conversions | `SELECT mag_to_flux(15.5, 25.0);` |
| `distance_modulus(distance_pc)` | Distance modulus calculations | `SELECT distance_modulus(1000.0);` |
| `luminosity_distance(z, h0, om, ol)` | Cosmological distances | `SELECT luminosity_distance(0.1, 70, 0.3, 0.7);` |
| `redshift_to_age(z, h0, om, ol)` | Universe age calculations | `SELECT redshift_to_age(1.0, 70, 0.3, 0.7);` |
| `celestial_point(ra, dec, distance)` | WKT geometry creation | `SELECT celestial_point(45.0, 30.0, 10.0);` |
| `catalog_info()` | Extension metadata | `SELECT catalog_info();` |

## 🏗️ Building from Source

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler
- DuckDB development headers

### Build Steps
```bash
# Clone repository
git clone https://github.com/bjoernbethge/astro-duck.git
cd astro-duck

# Build extension
make clean && make release

# Run tests
python test_astro.py
```

## 📈 Performance

- **10,000 objects** processed in **0.004 seconds**
- Vectorized execution for optimal performance
- Memory-efficient batch processing

## 🔧 Integration Examples

### With Spatial Extension
```sql
LOAD spatial;
LOAD astro;

-- Create spatial points from astronomical coordinates
SELECT 
    name,
    celestial_point(ra, dec, distance) as geometry,
    angular_separation(ra, dec, 0, 0) as distance_from_origin
FROM stars;
```

### With Arrow Format
```sql
LOAD astro;

-- Export astronomical calculations to Arrow
COPY (
    SELECT 
        object_id,
        radec_to_cartesian(ra, dec, distance) as coordinates,
        mag_to_flux(magnitude, 25.0) as flux
    FROM catalog
) TO 'astronomical_data.arrow' (FORMAT ARROW);
```

## 📚 Documentation

- [Function Reference](docs/functions.md)
- [Integration Guide](docs/integration.md)
- [Performance Benchmarks](docs/performance.md)
- [Examples](examples/)
- [Deployment Scripts](scripts/README.md)

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

- **Issues**: [GitHub Issues](https://github.com/bjoernbethge/astro-duck/issues)
- **Discussions**: [GitHub Discussions](https://github.com/bjoernbethge/astro-duck/discussions)
- **DuckDB Discord**: [Join the community](https://discord.duckdb.org)

---

**Made with ❤️ for the astronomical and data science communities** 