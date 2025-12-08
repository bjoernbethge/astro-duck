# 🌟 DuckDB Astro Extension

Comprehensive astronomical calculations and coordinate transformations for DuckDB with modern integrations.

[![Build Status](https://github.com/bjoernbethge/astro-duck/workflows/CI/badge.svg)](https://github.com/bjoernbethge/astro-duck/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![DuckDB](https://img.shields.io/badge/DuckDB-v1.3.0-blue.svg)](https://duckdb.org/)

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

| Function | Description | Parameters | Output | Example |
|----------|-------------|------------|--------|---------|
| `angular_separation(ra1, dec1, ra2, dec2)` | Angular distance between celestial objects | 4 DOUBLE | DOUBLE (degrees) | `SELECT angular_separation(45.0, 30.0, 46.0, 31.0);` |
| `radec_to_cartesian(ra, dec, distance)` | Coordinate conversion with metadata | 3 DOUBLE | JSON VARCHAR | `SELECT radec_to_cartesian(45.0, 30.0, 10.0);` |
| `mag_to_flux(magnitude, zero_point)` | Photometric conversions | 2 DOUBLE | DOUBLE | `SELECT mag_to_flux(15.5, 25.0);` |
| `distance_modulus(distance_pc)` | Distance modulus calculations | 1 DOUBLE | DOUBLE | `SELECT distance_modulus(1000.0);` |
| `luminosity_distance(redshift, h0)` | Cosmological distances | 2 DOUBLE | DOUBLE (Mpc) | `SELECT luminosity_distance(0.1, 70.0);` |
| `redshift_to_age(redshift)` | Universe age calculations | 1 DOUBLE | DOUBLE (Gyr) | `SELECT redshift_to_age(1.0);` |
| `celestial_point(ra, dec, distance)` | WKT geometry creation | 3 DOUBLE | WKT VARCHAR | `SELECT celestial_point(45.0, 30.0, 10.0);` |
| `catalog_info(catalog_name)` | Extension metadata | 1 VARCHAR | JSON VARCHAR | `SELECT catalog_info('my_catalog');` |

### Function Details

#### `angular_separation(ra1, dec1, ra2, dec2)`
Calculates angular separation between two celestial objects using the Haversine formula.
- **Parameters**: ra1, dec1, ra2, dec2 (all in degrees)
- **Returns**: Angular separation in degrees
- **Example**: `1.414177660952114` degrees

#### `radec_to_cartesian(ra, dec, distance)`
Converts RA/Dec coordinates to Cartesian with full metadata.
- **Parameters**: ra, dec (degrees), distance (any unit)
- **Returns**: JSON with x, y, z coordinates and metadata
- **Example**: 
```json
{
  "x": 6.12372436,
  "y": 6.12372436, 
  "z": 5.00000000,
  "ra": 45.00000000,
  "dec": 30.00000000,
  "distance": 10.00000000,
  "coordinate_system": "ICRS",
  "epoch": 2000.0
}
```

#### `mag_to_flux(magnitude, zero_point)`
Converts astronomical magnitude to flux.
- **Parameters**: magnitude, zero_point
- **Returns**: Flux value
- **Formula**: `10^((zero_point - magnitude) / 2.5)`

#### `distance_modulus(distance_pc)`
Calculates distance modulus from distance in parsecs.
- **Parameters**: distance_pc (distance in parsecs)
- **Returns**: Distance modulus in magnitudes
- **Formula**: `5 * log10(distance_pc) - 5`

#### `luminosity_distance(redshift, h0)`
Calculates luminosity distance from redshift.
- **Parameters**: redshift, h0 (Hubble constant in km/s/Mpc)
- **Returns**: Luminosity distance in Mpc
- **Note**: Uses simplified cosmology (matter-dominated universe)

#### `redshift_to_age(redshift)`
Calculates universe age at given redshift.
- **Parameters**: redshift
- **Returns**: Age in Gyr (billion years)
- **Note**: Uses Hubble constant of 70 km/s/Mpc

#### `celestial_point(ra, dec, distance)`
Creates WKT POINT Z geometry from celestial coordinates.
- **Parameters**: ra, dec (degrees), distance
- **Returns**: WKT string `POINT Z(x y z)`
- **Example**: `POINT Z(6.12372436 6.12372436 5.00000000)`

#### `catalog_info(catalog_name)`
Returns metadata about astronomical catalog.
- **Parameters**: catalog_name (string)
- **Returns**: JSON with catalog metadata
- **Example**:
```json
{
  "catalog": "my_catalog",
  "version": "1.0.0",
  "coordinate_system": "ICRS",
  "epoch": 2000.0,
  "supported_functions": ["angular_separation", "radec_to_cartesian", "mag_to_flux", "distance_modulus", "luminosity_distance", "redshift_to_age"],
  "extensions": ["arrow", "spatial", "parquet"]
}
```

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

- **10,000 objects** processed in **0.003 seconds** (actual benchmark)
- **Average angular separation**: 90.281° for random sky positions
- **Average flux calculation**: 214,753.94 for magnitude range 10-20
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

-- Query objects within angular distance
SELECT name, ra, dec
FROM stars 
WHERE angular_separation(ra, dec, 180.0, 0.0) < 5.0;
```

### With Arrow Format
```sql
LOAD astro;

-- Export astronomical calculations to Arrow with enhanced metadata
COPY (
    SELECT 
        object_id,
        radec_to_cartesian(ra, dec, distance) as coordinates,
        mag_to_flux(magnitude, 25.0) as flux,
        distance_modulus(distance_pc) as dist_mod
    FROM catalog
) TO 'astronomical_data.arrow' (FORMAT ARROW);
```

### Cosmological Calculations
```sql
-- Calculate universe properties at different redshifts
SELECT 
    redshift,
    luminosity_distance(redshift, 70.0) as lum_dist_mpc,
    redshift_to_age(redshift) as age_gyr
FROM generate_series(0.1, 2.0, 0.1) as t(redshift);
```

### Catalog Management
```sql
-- Get catalog metadata and supported features
SELECT catalog_info('gaia_dr3') as metadata;

-- Batch coordinate conversion with metadata
SELECT 
    source_id,
    radec_to_cartesian(ra, dec, parallax_distance) as enhanced_coords
FROM gaia_catalog 
LIMIT 1000;
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