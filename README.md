# 🌟 DuckDB Astro Extension

Comprehensive astronomical calculations and coordinate transformations for DuckDB with modern integrations.

[![Build Status](https://github.com/bjoernbethge/astro-duck/workflows/CI/badge.svg)](https://github.com/bjoernbethge/astro-duck/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![DuckDB](https://img.shields.io/badge/DuckDB-v1.4.3-blue.svg)](https://duckdb.org/)

## 🚀 Quick Start

### Installation from Community Extensions (Recommended)
```sql
INSTALL astro FROM community;
LOAD astro;

-- Calculate angular separation between two stars
SELECT astro_angular_separation(45.0, 30.0, 46.0, 31.0) as separation_degrees;

-- Get physical properties of a Sun-like star
SELECT astro_body_star_ms(1.0);
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

## 📊 Functions (48 total)

### Coordinate Transformations

| Function | Description | Example |
|----------|-------------|---------|
| `astro_angular_separation(ra1, dec1, ra2, dec2)` | Angular distance (Haversine) | `SELECT astro_angular_separation(45.0, 30.0, 46.0, 31.0);` |
| `astro_radec_to_xyz(ra, dec, dist)` | RA/Dec to Cartesian STRUCT | `SELECT astro_radec_to_xyz(45.0, 30.0, 10.0);` |
| `astro_frame_transform_pos(pos, from, to)` | Transform position between frames | `SELECT astro_frame_transform_pos(pos, 'icrs', 'galactic');` |
| `astro_frame_transform_vel(vel, from, to)` | Transform velocity between frames | `SELECT astro_frame_transform_vel(vel, 'icrs', 'galactic');` |

### Photometry

| Function | Description | Example |
|----------|-------------|---------|
| `astro_mag_to_flux(mag, zp)` | Magnitude to flux | `SELECT astro_mag_to_flux(15.5, 25.0);` |
| `astro_flux_to_mag(flux, zp)` | Flux to magnitude | `SELECT astro_flux_to_mag(1000.0, 25.0);` |
| `astro_absolute_mag(app_mag, dist_pc)` | Absolute magnitude | `SELECT astro_absolute_mag(10.0, 100.0);` |
| `astro_distance_modulus(dist_pc)` | Distance modulus | `SELECT astro_distance_modulus(1000.0);` |

### Cosmology

| Function | Description | Example |
|----------|-------------|---------|
| `astro_luminosity_distance(z, h0)` | Luminosity distance (Mpc) | `SELECT astro_luminosity_distance(0.1, 70.0);` |
| `astro_comoving_distance(z, h0)` | Comoving distance | `SELECT astro_comoving_distance(1.0, 70.0);` |

### Physical Constants

| Function | Value | Description |
|----------|-------|-------------|
| `astro_const_c()` | 299792458 m/s | Speed of light |
| `astro_const_G()` | 6.67430e-11 m³/(kg·s²) | Gravitational constant |
| `astro_const_AU()` | 149597870700 m | Astronomical unit |
| `astro_const_pc()` | 3.0857e16 m | Parsec |
| `astro_const_ly()` | 9.4607e15 m | Light year |
| `astro_const_M_sun()` | 1.989e30 kg | Solar mass |
| `astro_const_R_sun()` | 6.957e8 m | Solar radius |
| `astro_const_L_sun()` | 3.828e26 W | Solar luminosity |
| `astro_const_M_earth()` | 5.972e24 kg | Earth mass |
| `astro_const_R_earth()` | 6.371e6 m | Earth radius |
| `astro_const_sigma_sb()` | 5.670374e-8 W/(m²·K⁴) | Stefan-Boltzmann constant |

### Unit Conversions

| Function | Description | Example |
|----------|-------------|---------|
| `astro_unit_AU(n)` | n AU in meters | `SELECT astro_unit_AU(1.0);` |
| `astro_unit_pc(n)` | n parsec in meters | `SELECT astro_unit_pc(1.0);` |
| `astro_unit_ly(n)` | n light-years in meters | `SELECT astro_unit_ly(1.0);` |
| `astro_unit_M_sun(n)` | n solar masses in kg | `SELECT astro_unit_M_sun(1.0);` |
| `astro_unit_M_earth(n)` | n Earth masses in kg | `SELECT astro_unit_M_earth(1.0);` |
| `astro_unit_length_to_m(val, unit)` | Convert length to meters | `SELECT astro_unit_length_to_m(1.0, 'pc');` |
| `astro_unit_mass_to_kg(val, unit)` | Convert mass to kg | `SELECT astro_unit_mass_to_kg(1.0, 'M_sun');` |
| `astro_unit_time_to_s(val, unit)` | Convert time to seconds | `SELECT astro_unit_time_to_s(1.0, 'yr');` |

### Celestial Body Models

Returns STRUCT with: `mass_kg`, `radius_m`, `temperature_K`, `luminosity_W`, `density_kg_m3`, `body_type`

| Function | Description | Example |
|----------|-------------|---------|
| `astro_body_star_ms(mass_M_sun)` | Main sequence star | `SELECT astro_body_star_ms(1.0);` |
| `astro_body_star_white_dwarf(mass_M_sun)` | White dwarf (Chandrasekhar) | `SELECT astro_body_star_white_dwarf(0.6);` |
| `astro_body_star_neutron(mass_M_sun)` | Neutron star (~11km radius) | `SELECT astro_body_star_neutron(1.4);` |
| `astro_body_brown_dwarf(mass_M_jup)` | Brown dwarf (13-80 M_jup) | `SELECT astro_body_brown_dwarf(50.0);` |
| `astro_body_black_hole(mass_M_sun)` | Black hole (Schwarzschild) | `SELECT astro_body_black_hole(10.0);` |
| `astro_body_planet_rocky(mass_M_earth)` | Rocky planet | `SELECT astro_body_planet_rocky(1.0);` |
| `astro_body_planet_gas_giant(mass_M_jup)` | Gas giant (Jupiter-like) | `SELECT astro_body_planet_gas_giant(1.0);` |
| `astro_body_planet_ice_giant(mass_M_earth)` | Ice giant (Neptune-like) | `SELECT astro_body_planet_ice_giant(17.0);` |
| `astro_body_asteroid(radius_km, density)` | Asteroid from size/density | `SELECT astro_body_asteroid(500, 2000);` |

### Orbital Mechanics

| Function | Description | Example |
|----------|-------------|---------|
| `astro_orbit_period(a_m, M_kg)` | Kepler orbital period (s) | `SELECT astro_orbit_period(1.496e11, 1.989e30);` |
| `astro_orbit_mean_motion(a_m, M_kg)` | Mean motion (rad/s) | `SELECT astro_orbit_mean_motion(1.496e11, 1.989e30);` |
| `astro_orbit_make(a, e, i, omega, w, M0, epoch, M, frame)` | Create orbit STRUCT | See examples below |
| `astro_orbit_position(orbit, t)` | Position at time t | `SELECT astro_orbit_position(orbit, 2451545.0);` |
| `astro_orbit_velocity(orbit, t)` | Velocity at time t | `SELECT astro_orbit_velocity(orbit, 2451545.0);` |

### Spatial Sectors (3D Octree)

| Function | Description | Example |
|----------|-------------|---------|
| `astro_sector_id(x, y, z, level)` | Get sector ID for position | `SELECT astro_sector_id(1.0, 0.0, 0.0, 3);` |
| `astro_sector_center(sector)` | Get sector center position | `SELECT astro_sector_center(sector);` |
| `astro_sector_bounds(sector)` | Get sector bounding box | `SELECT astro_sector_bounds(sector);` |
| `astro_sector_parent(sector)` | Get parent sector | `SELECT astro_sector_parent(sector);` |

### Dynamics

| Function | Description | Example |
|----------|-------------|---------|
| `astro_dyn_gravity_accel(m1, pos1, m2, pos2)` | Gravitational acceleration | See examples below |

### Function Details

#### Coordinate Functions

**`astro_radec_to_xyz(ra, dec, distance)`** returns a STRUCT:
```sql
SELECT astro_radec_to_xyz(45.0, 30.0, 10.0);
-- {'x': 6.123724356957945, 'y': 6.123724356957946, 'z': 4.999999999999999}
```

#### Body Models

Each body function returns a STRUCT with physical properties:
```sql
SELECT astro_body_star_ms(1.0);  -- Sun-like star
-- {'mass_kg': 1.989e+30, 'radius_m': 6.957e+08, 'temperature_K': 5778.0,
--  'luminosity_W': 3.828e+26, 'density_kg_m3': 1408.0, 'body_type': 'star_main_sequence'}

SELECT astro_body_black_hole(10.0);  -- 10 solar mass black hole
-- {'mass_kg': 1.989e+31, 'radius_m': 29541.0, 'temperature_K': 0.0,
--  'luminosity_W': 0.0, 'density_kg_m3': 1.83e+18, 'body_type': 'black_hole'}
```

#### Unit Conversions

Flexible unit conversion with string identifiers:
```sql
-- Length: 'AU', 'pc', 'ly', 'km', 'm'
SELECT astro_unit_length_to_m(1.0, 'pc');  -- 3.0857e16

-- Mass: 'M_sun', 'M_earth', 'M_jup', 'kg'
SELECT astro_unit_mass_to_kg(1.0, 'M_sun');  -- 1.989e30

-- Time: 'yr', 'day', 'hr', 's'
SELECT astro_unit_time_to_s(1.0, 'yr');  -- 3.1557e7
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

### Coordinate Transformations
```sql
LOAD astro;

-- Convert catalog coordinates to Cartesian
SELECT
    name,
    astro_radec_to_xyz(ra, dec, astro_parallax_to_distance(parallax)) as xyz
FROM stars;

-- Query objects within angular distance
SELECT name, ra, dec
FROM stars
WHERE astro_angular_separation(ra, dec, 180.0, 0.0) < 5.0;

-- Convert between coordinate systems
SELECT
    astro_equatorial_to_galactic(ra, dec) as galactic,
    astro_galactic_to_equatorial(l, b) as equatorial
FROM coordinates;
```

### Photometry Pipeline
```sql
LOAD astro;

-- Complete photometric analysis
SELECT
    object_id,
    astro_mag_to_flux(mag_g, 25.0) as flux_g,
    astro_abs_mag(mag_g, astro_parallax_to_distance(parallax)) as abs_mag,
    astro_color_index(mag_b, mag_v) as bv_color,
    astro_extinction_correction(mag_v, av, 3.1) as mag_v_corrected
FROM photometry;
```

### Cosmological Calculations
```sql
-- Calculate universe properties at different redshifts
SELECT
    z as redshift,
    astro_luminosity_distance(z, 70.0) as lum_dist_mpc,
    astro_comoving_distance(z, 70.0) as comoving_mpc,
    astro_lookback_time(z) as lookback_gyr,
    astro_redshift_to_age(z) as age_gyr
FROM generate_series(0.1, 2.0, 0.1) as t(z);
```

### Body Model Analysis
```sql
-- Compare different stellar remnants
SELECT
    mass,
    (astro_body_star_white_dwarf(mass)).radius_m / 1000 as wd_radius_km,
    (astro_body_star_neutron(mass)).radius_m / 1000 as ns_radius_km,
    (astro_body_black_hole(mass)).radius_m / 1000 as bh_radius_km
FROM generate_series(0.5, 2.0, 0.1) as t(mass);

-- Exoplanet characterization
SELECT
    name,
    astro_body_planet_rocky(mass_earth) as rocky_model,
    astro_body_planet_gas_giant(mass_earth / 318.0) as gas_model
FROM exoplanets
WHERE mass_earth < 10;
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