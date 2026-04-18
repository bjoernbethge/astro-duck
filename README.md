<p align="center">
  <img src="assets/hero.jpg" alt="astro-duck" width="420">
</p>

# 🌟 DuckDB Astro Extension
Comprehensive astronomical calculations and coordinate transformations for DuckDB with modern integrations.

[![Build Status](https://github.com/synapticore-io/astro-duck/actions/workflows/MainDistributionPipeline.yml/badge.svg?branch=main)](https://github.com/synapticore-io/astro-duck/actions/workflows/MainDistributionPipeline.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![DuckDB](https://img.shields.io/badge/DuckDB-v1.5.1-blue.svg)](https://duckdb.org/)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.19482715.svg)](https://doi.org/10.5281/zenodo.19482715)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/synapticore-io/astro-duck)

## 🚀 Quick Start

### Installation from Community Extensions (Recommended)
```sql
INSTALL astro FROM community;
LOAD astro;

-- Calculate angular separation between two stars
SELECT astro_angular_separation(45.0, 30.0, 46.0, 31.0) as separation_degrees;

-- Get physical properties of a Sun-like star
SELECT astro_body_star_main_sequence(1.0);
```

### Manual Installation
1. Download the latest release from [Releases](https://github.com/synapticore-io/astro-duck/releases)
2. Load in DuckDB:
```sql
LOAD '/path/to/astro.duckdb_extension';
```

## ✨ Features

- **🌐 Coordinate Transformations**: RA/Dec ↔ Cartesian, ICRS ↔ galactic frame
- **📐 Angular Calculations**: Precise angular separation using Haversine formula
- **💫 Photometric Functions**: Magnitude/flux conversions with zero-point support
- **🌫️ Dust Extinction**: CCM89 + O'Donnell (1994) with UBVRIJHK bands
- **🌌 Cosmological Calculations**: Luminosity and comoving distance
- **🔭 Sidereal Time & Observing**: GMST/LMST, hour angle, equatorial → horizontal (alt/az)
- **🪐 Celestial Body Models**: Stars, planets, brown dwarfs, black holes, asteroids
- **🛰️ Orbital Mechanics**: Keplerian orbits, mean motion, position/velocity
- **🧊 3D Spatial Octree**: Sector-based spatial indexing for N-body data
- **🔭 FITS File I/O**: Read FITS tables and images directly into DuckDB (`fits_hdus`, `fits_header`, `read_fits`)

## 📊 Functions (62: 59 scalar + 3 table)

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

### Dust Extinction (CCM89)

Interstellar dust extinction using the Cardelli, Clayton & Mathis (1989) extinction law with O'Donnell (1994) optical coefficients.

| Function | Description | Example |
|----------|-------------|---------|
| `astro_extinction_av(ebv, [rv=3.1])` | A_V from E(B-V) and R_V | `SELECT astro_extinction_av(0.5);` |
| `astro_extinction_alambda(λ_Å, av, [rv=3.1])` | Extinction at wavelength | `SELECT astro_extinction_alambda(5494.5, 1.0);` |
| `astro_extinction_band(band, av, [rv=3.1])` | Extinction in photometric band | `SELECT astro_extinction_band('V', 1.0);` |
| `astro_mag_deredden(mag, extinction)` | Correct magnitude for extinction | `SELECT astro_mag_deredden(15.5, 1.25);` |
| `astro_flux_deredden(flux, extinction)` | Correct flux for extinction | `SELECT astro_flux_deredden(1000.0, 1.0);` |
| `astro_color_excess(mag1, mag2, intrinsic)` | Calculate color excess | `SELECT astro_color_excess(12.5, 11.8, 0.3);` |

**Default R_V=3.1** corresponds to the average for the Milky Way diffuse ISM.

**Supported photometric bands**: U (3650Å), B (4400Å), V (5494.5Å), R (6580Å), I (8060Å), J (12350Å), H (16620Å), K (21590Å)

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

Returns STRUCT with: `mass_kg`, `radius_m`, `luminosity_w`, `temperature_k`, `density_kg_m3`, `body_type`

| Function | Description | Example |
|----------|-------------|---------|
| `astro_body_star_main_sequence(mass_M_sun)` | Main sequence star | `SELECT astro_body_star_main_sequence(1.0);` |
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

### FITS File I/O

| Function | Description | Example |
|----------|-------------|---------|
| `fits_hdus(path)` | List all HDUs (index, name, type) | `SELECT * FROM fits_hdus('file.fits');` |
| `fits_header(path, hdu:=0)` | Read header keywords from an HDU | `SELECT * FROM fits_header('file.fits', hdu:=1);` |
| `read_fits(path, hdu:=NULL, header:=false, flatten:=true)` | Read BINTABLE or IMAGE HDU as rows | `SELECT * FROM read_fits('file.fits', hdu:=1);` |

`read_fits` supports:
- **BINTABLE**: each row in the table becomes a DuckDB row; columns are typed to the FITS column types
- **IMAGE**: with `flatten:=true` (default) each pixel becomes a row with coordinates; with `flatten:=false` returns one row with a nested LIST column
- **`header:=true`**: prepend `_hdr_<keyword>` columns from the HDU header

### Sidereal Time & Observing

| Function | Description | Example |
|----------|-------------|---------|
| `astro_jd_from_timestamp(ts)` | Julian Date from TIMESTAMP or TIMESTAMPTZ | `SELECT astro_jd_from_timestamp(now());` |
| `astro_gmst(jd)` | Greenwich Mean Sidereal Time in hours [0,24) | `SELECT astro_gmst(2451545.0);` |
| `astro_lmst(jd, lon_deg)` | Local Mean Sidereal Time in hours [0,24) | `SELECT astro_lmst(2451545.0, 10.0);` |
| `astro_hour_angle(ra_deg, lmst_h)` | Hour angle in hours, in (-12, 12] | `SELECT astro_hour_angle(180.0, 10.0);` |
| `astro_altaz_from_radec(ra, dec, lmst_h, lat_deg)` | STRUCT{`alt_deg`, `az_deg`} — equatorial → horizontal | See examples below |

`astro_altaz_from_radec` returns a STRUCT with two fields: `alt_deg` (altitude
above horizon, [-90, 90]) and `az_deg` (azimuth measured from North through
East, [0, 360)). Use it together with `astro_lmst` to ask "what is currently
above the horizon at my observatory":

```sql
WITH obs AS (
    SELECT astro_lmst(astro_jd_from_timestamp(now()), 10.0) AS lmst_h  -- lon ~Germany
)
SELECT name, ra, dec,
       (astro_altaz_from_radec(ra, dec, (SELECT lmst_h FROM obs), 53.55)).alt_deg AS alt
FROM bright_stars
WHERE (astro_altaz_from_radec(ra, dec, (SELECT lmst_h FROM obs), 53.55)).alt_deg > 30
ORDER BY alt DESC;
```

### Function Details

#### Coordinate Functions

**`astro_radec_to_xyz(ra, dec, distance)`** returns a STRUCT with the position
in the ICRS frame. The distance unit is whatever you pass in — wrap it with
`astro_unit_pc(...)` / `astro_unit_AU(...)` / `astro_unit_ly(...)` if you
want SI meters.
```sql
SELECT astro_radec_to_xyz(45.0, 30.0, 10.0);
-- {'x_m': 6.123724..., 'y_m': 6.123724..., 'z_m': 5.0, 'frame': 'icrs'}
```

#### Body Models

Each body function returns a STRUCT with physical properties in SI units:
```sql
SELECT astro_body_star_main_sequence(1.0);  -- Sun-like star
-- {'mass_kg': 1.989e+30, 'radius_m': 6.957e+08, 'luminosity_w': 3.828e+26,
--  'temperature_k': 5778.0, 'density_kg_m3': 1408.0, 'body_type': 'star_main_sequence'}

SELECT astro_body_black_hole(10.0);  -- 10 solar mass black hole
-- {'mass_kg': 1.989e+31, 'radius_m': 29541.0, 'luminosity_w': 0.0,
--  'temperature_k': 0.0, 'density_kg_m3': 1.83e+18, 'body_type': 'black_hole'}
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
- CMake 3.5+
- C++17 compatible compiler
- DuckDB development headers

### Build Steps
```bash
# Clone repository with submodules (duckdb, extension-ci-tools, cfitsio)
git clone --recursive https://github.com/synapticore-io/astro-duck.git
cd astro-duck

# Build extension
make clean && make release

# Run tests
python test_astro.py
```

If you already cloned without `--recursive`, initialize the submodules with
`git submodule update --init --recursive`.

## 📈 Performance

- **10,000 objects** processed in **0.003 seconds** (actual benchmark)
- **Average angular separation**: 90.281° for random sky positions
- **Average flux calculation**: 214,753.94 for magnitude range 10-20
- Vectorized execution for optimal performance
- Memory-efficient batch processing

## 🔧 Integration Examples

> The snippets below are minimal patterns. For end-to-end recipes against
> real catalogs (Gaia, NASA Exoplanet Archive, simulation snapshots,
> dustmaps) see the **[Cookbook](docs/COOKBOOK.md)**.

### Coordinate Transformations
```sql
LOAD astro;

-- Convert catalog coordinates to Cartesian
SELECT
    name,
    astro_radec_to_xyz(ra, dec, distance_pc) as xyz
FROM stars;

-- Query objects within angular distance
SELECT name, ra, dec
FROM stars
WHERE astro_angular_separation(ra, dec, 180.0, 0.0) < 5.0;

-- Transform positions between reference frames (icrs, galactic, ...)
SELECT
    astro_frame_transform_pos(astro_radec_to_xyz(ra, dec, distance_pc), 'icrs', 'galactic') as galactic_pos
FROM stars;
```

### Photometry Pipeline
```sql
LOAD astro;

-- Complete photometric analysis with dust extinction correction
SELECT
    object_id,
    mag_v,
    -- Calculate A_V from E(B-V) reddening
    astro_extinction_av(ebv, 3.1) as A_V,
    -- Get extinction in each band
    astro_extinction_band('V', astro_extinction_av(ebv, 3.1), 3.1) as A_V_band,
    astro_extinction_band('B', astro_extinction_av(ebv, 3.1), 3.1) as A_B_band,
    -- Deredden the observed magnitude
    astro_mag_deredden(mag_v, astro_extinction_band('V', astro_extinction_av(ebv, 3.1), 3.1)) as mag_v_dereddened,
    -- Convert to flux and deredden
    astro_flux_deredden(astro_mag_to_flux(mag_v, 25.0), astro_extinction_band('V', astro_extinction_av(ebv, 3.1), 3.1)) as flux_v_dereddened
FROM photometry;

-- Extinction at specific wavelength (H-alpha = 6563 Å)
SELECT astro_extinction_alambda(6563, 1.55, 3.1) as A_Halpha;
```

### Cosmological Calculations
```sql
-- Calculate cosmological distances at different redshifts (H0 in km/s/Mpc)
SELECT
    z as redshift,
    astro_luminosity_distance(z, 70.0) as lum_dist_mpc,
    astro_comoving_distance(z, 70.0) as comoving_mpc
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

- **[Cookbook](docs/COOKBOOK.md)** — real-world recipes: Gaia cone search,
  HR diagrams, dust dereddening with CCM89, Hubble diagrams, exoplanet
  characterization, N-body spatial binning, ICRS↔galactic transforms,
  observation planning with sidereal time and alt/az. Each recipe is
  self-contained and runs without external data.
- Function reference: see the tables above
- [Updating guide](docs/UPDATING.md)
- [Deployment scripts](scripts/README.md)

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## 📖 How to Cite

If you use astro-duck in research that leads to a publication, please cite
the software via its Zenodo DOI. The badge above resolves to the latest
version; for a specific release use the version DOI.

**Concept DOI (always-latest):** [10.5281/zenodo.19482715](https://doi.org/10.5281/zenodo.19482715)

**Latest version DOI (v1.4.0):** [10.5281/zenodo.19645339](https://doi.org/10.5281/zenodo.19645339)

GitHub also reads `CITATION.cff` and shows a "Cite this repository" button
in the right sidebar with auto-generated BibTeX / APA / RIS / EndNote
formats.

### BibTeX

```bibtex
@software{bethge_astro_duck,
  author       = {Bethge, Björn},
  title        = {{astro-duck: A DuckDB extension for astronomical calculations}},
  year         = {2026},
  version      = {1.4.0},
  publisher    = {Zenodo},
  doi          = {10.5281/zenodo.19482715},
  url          = {https://doi.org/10.5281/zenodo.19482715}
}
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [DuckDB](https://duckdb.org/) for the excellent database engine
- [Astropy](https://www.astropy.org/) for astronomical calculation references
- The astronomical community for domain expertise

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/synapticore-io/astro-duck/issues)
- **Discussions**: [GitHub Discussions](https://github.com/synapticore-io/astro-duck/discussions)
- **DuckDB Discord**: [Join the community](https://discord.duckdb.org)

---

**Made with ❤️ for the astronomical and data science communities** 
