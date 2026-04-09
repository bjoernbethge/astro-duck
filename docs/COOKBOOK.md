# Astro Extension Cookbook

Real-world recipes for doing astronomy in SQL with DuckDB.

Every recipe below is self-contained — the minimal example uses `VALUES` or
`generate_series` so you can paste it into a DuckDB shell without downloading
anything. Each recipe then shows the same query pattern against a real
catalog file so you can swap in your own data.

---

## Prerequisites

```sql
INSTALL astro FROM community;
LOAD astro;
```

### Conventions

- **Right ascension / declination (`ra`, `dec`)**: degrees
- **`astro_angular_separation` output**: degrees
- **`astro_radec_to_xyz` output**: a `STRUCT(x_m, y_m, z_m, frame)`. The
  coordinate unit is whatever you pass as the distance argument — the `_m`
  suffix is a convention, not a forced conversion. If you want real meters,
  wrap the distance with `astro_unit_pc(dist_pc)`, `astro_unit_ly(dist_ly)`,
  or `astro_unit_AU(dist_au)`.
- **Body model STRUCTs** (`astro_body_*`) use SI units with lowercase
  suffixes: `{mass_kg, radius_m, luminosity_w, temperature_k, density_kg_m3,
  body_type}`.
- **Cosmology inputs**: `H0` is in km/s/Mpc (typical values 67 – 74). Results
  are in Mpc unless noted.

---

## Data Sources

Good public catalogs to point DuckDB at:

| Catalog | What it is | How to get it |
|---|---|---|
| Gaia DR3 | 1.8B stars, 5-parameter astrometry + photometry | Parquet releases on HuggingFace (search `gaia-dr3`), or the ESA archive via TAP |
| SDSS DR17 | Galaxies, quasars, photometry, spectra | SDSS SciServer; parquet exports available |
| NASA Exoplanet Archive | ~5k confirmed exoplanets + host stars | https://exoplanetarchive.ipac.caltech.edu (CSV download) |
| 2MASS | NIR all-sky photometry (J, H, K) | IPAC / IRSA bulk downloads |
| SFD / Planck dust maps | E(B-V) per sightline | Healpix FITS; can be preprocessed into parquet |
| IllustrisTNG / EAGLE | Cosmological simulation snapshots | Parquet / HDF5 on the project sites |

DuckDB reads parquet directly from S3/HTTPS, so you can run most of these
queries without materializing files locally:

```sql
SELECT COUNT(*)
FROM read_parquet('https://example.com/catalog.parquet');
```

---

## Recipe 1 — Cone Search Around a Target

**Goal**: find all objects within a given angular radius of a sky position.
This is the bread-and-butter query for crossmatching transients, identifying
cluster members, or building local samples around a target.

**Functions**: `astro_angular_separation`

### Minimal example

```sql
WITH stars(name, ra, dec) AS (
    VALUES
        ('M31 core',       10.6847,  41.2687),   -- Andromeda Galaxy
        ('M32',            10.6742,  40.8653),   -- M31 satellite
        ('M110',           10.0919,  41.6853),   -- M31 satellite
        ('random field',   15.0000,  42.0000),
        ('south pole',      0.0000, -90.0000)
)
SELECT
    name,
    astro_angular_separation(ra, dec, 10.6847, 41.2687) AS deg_from_m31
FROM stars
WHERE astro_angular_separation(ra, dec, 10.6847, 41.2687) < 1.5
ORDER BY deg_from_m31;
```

Expected result — M31, M32 and M110 all within 1.5° of the M31 core; the
random field and south pole are filtered out.

### Scale-up on Gaia

```sql
SELECT source_id, ra, dec, phot_g_mean_mag
FROM read_parquet('gaia_dr3/*.parquet')
WHERE astro_angular_separation(ra, dec, 10.6847, 41.2687) < 0.5
  AND phot_g_mean_mag < 18
ORDER BY phot_g_mean_mag;
```

DuckDB's vectorized execution means the `angular_separation` filter runs at
memory-bandwidth speed — 100M+ rows per second on a modern laptop.

---

## Recipe 2 — Hertzsprung–Russell Diagram from Parallax

**Goal**: turn a parallax catalog into absolute magnitudes so you can plot an
HR diagram (color vs absolute magnitude). Reveals the main sequence, white
dwarfs, giants, and any peculiar populations in your sample.

**Functions**: `astro_absolute_mag`

### Minimal example

```sql
WITH sample(name, g_mag, bp_rp, parallax_mas) AS (
    VALUES
        ('Sun-like',    4.83,  0.82,  1000.0),   -- 1 pc (synthetic)
        ('Red giant',  -0.30,  1.45,   100.0),   -- 10 pc
        ('White dwarf', 10.0, -0.20,   200.0),   -- 5 pc
        ('M dwarf',     12.5,  2.80,  1000.0)    -- 1 pc
)
SELECT
    name,
    bp_rp                               AS color_bp_rp,
    g_mag                               AS apparent_g,
    1000.0 / parallax_mas               AS distance_pc,
    astro_absolute_mag(g_mag, 1000.0 / parallax_mas) AS abs_g
FROM sample;
```

### Scale-up on Gaia

```sql
SELECT
    bp_rp,
    astro_absolute_mag(phot_g_mean_mag, 1000.0 / parallax) AS abs_g
FROM read_parquet('gaia_dr3_subset.parquet')
WHERE parallax > 0
  AND parallax_over_error > 10
  AND bp_rp IS NOT NULL;
```

Pipe the result into matplotlib / plotly / marimo and you have the classic
HR diagram. Add a `WHERE abs_g > 10 AND bp_rp > 2` filter to isolate the
M-dwarf cloud.

---

## Recipe 3 — Photometric Dereddening with CCM89

**Goal**: correct observed magnitudes and fluxes for interstellar dust
extinction using the Cardelli, Clayton & Mathis (1989) law with the
O'Donnell (1994) optical coefficients. This is where the v1.1.1 dust
functions earn their keep.

**Functions**: `astro_extinction_av`, `astro_extinction_band`,
`astro_extinction_alambda`, `astro_mag_deredden`, `astro_flux_deredden`,
`astro_color_excess`

### Minimal example

```sql
WITH obs(object_id, mag_b, mag_v, mag_k, ebv) AS (
    VALUES
        ('obj_001', 15.32, 14.85, 12.10, 0.15),
        ('obj_002', 18.90, 17.20, 13.40, 0.82),
        ('obj_003', 12.10, 11.95, 10.50, 0.04)
)
SELECT
    object_id,

    -- A_V from the reddening E(B-V) with the default R_V = 3.1
    astro_extinction_av(ebv)                              AS a_v,

    -- Extinction in each observed band
    astro_extinction_band('B', astro_extinction_av(ebv))  AS a_b,
    astro_extinction_band('V', astro_extinction_av(ebv))  AS a_v_band,
    astro_extinction_band('K', astro_extinction_av(ebv))  AS a_k,

    -- Dereddened magnitudes (A_band is subtracted)
    astro_mag_deredden(mag_b,
        astro_extinction_band('B', astro_extinction_av(ebv))) AS mag_b_0,
    astro_mag_deredden(mag_v,
        astro_extinction_band('V', astro_extinction_av(ebv))) AS mag_v_0,
    astro_mag_deredden(mag_k,
        astro_extinction_band('K', astro_extinction_av(ebv))) AS mag_k_0
FROM obs;
```

Expected behaviour: K-band extinction is always much smaller than B-band
extinction (the classic 1/λ wavelength dependence), and the dereddened
magnitudes are brighter (numerically smaller) than the observed ones.

### Extinction at an arbitrary wavelength

Useful for spectroscopy or narrow-band filters:

```sql
-- Extinction at H-alpha (6563 Å) for A_V = 1.0
SELECT astro_extinction_alambda(6563.0, 1.0) AS a_halpha;
```

### Scale-up on a photometric survey with a dust map

Assuming you have joined your photometric catalog with an SFD or Planck
E(B-V) dustmap by healpix cell:

```sql
SELECT
    object_id,
    mag_v,
    mag_b,
    astro_mag_deredden(mag_v,
        astro_extinction_band('V', astro_extinction_av(ebv))) AS mag_v_0,
    astro_mag_deredden(mag_b,
        astro_extinction_band('B', astro_extinction_av(ebv))) AS mag_b_0,
    -- Intrinsic B-V colour
    astro_mag_deredden(mag_b,
        astro_extinction_band('B', astro_extinction_av(ebv)))
    - astro_mag_deredden(mag_v,
        astro_extinction_band('V', astro_extinction_av(ebv))) AS bv_0
FROM read_parquet('observations.parquet')
JOIN read_parquet('dustmap.parquet') USING (healpix_id);
```

### Going the other way — measuring reddening

If you already know the intrinsic colour of a class of objects (say 0.65
for a K0V star) you can back out E(B-V) from the observed colour:

```sql
SELECT
    object_id,
    astro_color_excess(mag_b, mag_v, 0.65) AS ebv_observed
FROM k0v_candidates;
```

---

## Recipe 4 — Cosmological Hubble Diagram

**Goal**: for a sample of galaxies with spectroscopic redshifts, compute
luminosity and comoving distances, plus the distance modulus so you can
convert apparent to absolute magnitudes.

**Functions**: `astro_luminosity_distance`, `astro_comoving_distance`,
`astro_distance_modulus`

### Minimal example — a smooth z grid

```sql
SELECT
    z,
    astro_luminosity_distance(z, 70.0)      AS d_l_mpc,
    astro_comoving_distance(z, 70.0)        AS d_c_mpc,
    -- Distance modulus wants distance in parsecs
    astro_distance_modulus(astro_luminosity_distance(z, 70.0) * 1.0e6) AS mu
FROM generate_series(0.01, 2.0, 0.01) AS t(z);
```

At low z, `d_l ≈ d_c ≈ c·z/H0`. At high z, `d_l` grows faster than `d_c`
because of the `(1+z)²` factor — this is the shape that falsified a
matter-only universe in the late 1990s.

### Scale-up on a spectroscopic survey

```sql
WITH cosmo AS (
    SELECT
        object_id,
        z_spec,
        mag_r,
        astro_luminosity_distance(z_spec, 70.0) AS d_l_mpc,
        astro_distance_modulus(astro_luminosity_distance(z_spec, 70.0) * 1.0e6) AS mu
    FROM read_parquet('galaxies.parquet')
    WHERE z_spec BETWEEN 0.01 AND 1.5
)
SELECT
    object_id,
    z_spec,
    d_l_mpc,
    mag_r - mu AS abs_mag_r
FROM cosmo;
```

The `mag_r - mu` column is the absolute r-band magnitude — the observable
that goes on the y-axis of a Hubble diagram.

---

## Recipe 5 — Exoplanet Characterization

**Goal**: for every confirmed exoplanet in a catalog, attach a physical
model of both the host star and the planet, plus the orbital period you
would predict from Kepler's 3rd law.

**Functions**: `astro_body_star_ms`, `astro_body_planet_rocky`,
`astro_body_planet_gas_giant`, `astro_orbit_period`, `astro_unit_AU`,
`astro_unit_M_sun`

### Minimal example

```sql
WITH planets(pl_name, st_mass_msun, pl_mass_mearth, pl_orbsmax_au) AS (
    VALUES
        ('Earth analog',  1.00,    1.00,  1.00),
        ('Hot Jupiter',   1.10,  318.00,  0.05),
        ('Super-Earth',   0.80,    5.00,  0.12),
        ('Mars analog',   1.00,    0.11,  1.52)
)
SELECT
    pl_name,

    -- Host star model (main sequence)
    (astro_body_star_ms(st_mass_msun)).luminosity_w AS host_luminosity_w,
    (astro_body_star_ms(st_mass_msun)).temperature_k AS host_temp_k,

    -- Planet model: rocky for < 10 Earth masses, else gas giant scaled to Jupiter
    CASE
        WHEN pl_mass_mearth < 10
            THEN (astro_body_planet_rocky(pl_mass_mearth)).radius_m / 1000
        ELSE (astro_body_planet_gas_giant(pl_mass_mearth / 318.0)).radius_m / 1000
    END AS predicted_radius_km,

    -- Kepler's 3rd law: period in days
    astro_orbit_period(
        astro_unit_AU(pl_orbsmax_au),
        astro_unit_M_sun(st_mass_msun)
    ) / 86400.0 AS period_days
FROM planets;
```

Earth comes out at ~365 days, the hot Jupiter at about 4 days, the
super-Earth at roughly 40 days, and a Mars analog at ~686 days.

### Scale-up on the NASA Exoplanet Archive

```sql
SELECT
    pl_name,
    hostname,
    st_spectype,
    astro_orbit_period(
        astro_unit_AU(pl_orbsmax),
        astro_unit_M_sun(st_mass)
    ) / 86400.0 AS period_days_kepler,
    pl_orbper AS period_days_observed
FROM read_csv('PS_exoplanets.csv', header=true)
WHERE pl_orbsmax IS NOT NULL
  AND st_mass  IS NOT NULL
  AND pl_orbper IS NOT NULL;
```

Good sanity check: the Kepler-predicted period should match the observed
period to within a few percent; large disagreements usually indicate bad
catalog rows.

---

## Recipe 6 — 3D Spatial Binning for N-body Data

**Goal**: take a point cloud of particles (stars, DM particles, galaxies)
and bin them into a 3D octree for density estimation, neighbor lookup, or
down-sampling. The `astro_sector_*` functions implement a simple level-N
cartesian octree — each level subdivides the bounding box into 8 children.

**Functions**: `astro_sector_id`, `astro_sector_center`, `astro_sector_bounds`

### Minimal example

```sql
WITH particles(id, x, y, z, mass) AS (
    VALUES
        (1,  1.0,  0.0,  0.0,  2.0),
        (2,  1.1,  0.0,  0.1,  1.8),
        (3, -1.0,  0.0,  0.0,  2.5),
        (4,  0.0,  5.0,  0.0,  1.0),
        (5,  0.0,  5.1,  0.0,  1.2)
)
SELECT
    astro_sector_id(x, y, z, 3) AS sector,
    COUNT(*)                    AS n_particles,
    SUM(mass)                   AS total_mass,
    astro_sector_center(astro_sector_id(x, y, z, 3)) AS center
FROM particles
GROUP BY sector
ORDER BY total_mass DESC;
```

Particles 1 and 2 land in the same sector (they're ~0.14 apart), particles
4 and 5 share another, and particle 3 is alone in a third.

### Scale-up on a simulation snapshot

```sql
-- Build a density grid at level 5 (32³ = 32 768 cells per octant)
WITH grid AS (
    SELECT
        astro_sector_id(x, y, z, 5) AS sector,
        SUM(mass)  AS total_mass,
        COUNT(*)   AS n_particles
    FROM read_parquet('snapshot_099/*.parquet')
    GROUP BY sector
)
SELECT
    sector,
    astro_sector_center(sector) AS center,
    total_mass,
    n_particles
FROM grid
WHERE n_particles > 50
ORDER BY total_mass DESC
LIMIT 100;
```

The result is a halo finder sketch: the densest sectors are where your
halos live.

---

## Recipe 7 — ICRS ↔ Galactic Frame Transforms

**Goal**: rotate a position vector from equatorial (ICRS) to galactic
coordinates. Useful for splitting a catalog into disk / halo / bulge
populations, or for comparing Milky Way structure with extragalactic
samples.

**Functions**: `astro_radec_to_xyz`, `astro_frame_transform_pos`

### Minimal example

```sql
WITH bright_stars(name, ra, dec, dist_pc) AS (
    VALUES
        ('Sirius',    101.2875, -16.7161, 2.64),
        ('Vega',      279.2347,  38.7837, 7.68),
        ('Betelgeuse', 88.7929,   7.4071, 168.0),
        ('Polaris',    37.9547,  89.2642, 132.0)
)
SELECT
    name,
    astro_radec_to_xyz(ra, dec, astro_unit_pc(dist_pc))            AS xyz_icrs_m,
    astro_frame_transform_pos(
        astro_radec_to_xyz(ra, dec, astro_unit_pc(dist_pc)),
        'icrs', 'galactic'
    ) AS xyz_galactic_m
FROM bright_stars;
```

The resulting STRUCT has fields `x_m`, `y_m`, `z_m`, `frame`. Because we
passed the distance through `astro_unit_pc` the components really are in
meters; the `frame` field flips from `'icrs'` to `'galactic'` after the
transform.

### Scale-up — selecting stars in the galactic disk

```sql
WITH gal AS (
    SELECT
        source_id,
        astro_frame_transform_pos(
            astro_radec_to_xyz(ra, dec, astro_unit_pc(1000.0 / parallax)),
            'icrs', 'galactic'
        ) AS pos
    FROM read_parquet('gaia_dr3_subset.parquet')
    WHERE parallax > 0
      AND parallax_over_error > 5
)
SELECT source_id, pos
FROM gal
WHERE abs(pos.z_m) < astro_unit_pc(100.0);  -- |z| < 100 pc → galactic disk
```

---

## Recipe 8 — What Is Above the Horizon Right Now?

**Goal**: for an observer at a given latitude/longitude and a moment in time,
turn a catalog of equatorial RA/Dec coordinates into horizontal alt/az
coordinates and select everything that is currently above the horizon. This
is the foundation of any observation-planning pipeline.

**Functions**: `astro_jd_from_timestamp`, `astro_lmst`, `astro_altaz_from_radec`,
optionally `astro_hour_angle`, `astro_gmst`

### Minimal example

```sql
WITH
    -- Observer location: Hamburg, Germany
    observer(lat, lon) AS (VALUES (53.55, 10.00)),
    -- Compute Local Mean Sidereal Time for "now"
    obs AS (
        SELECT
            (SELECT lat FROM observer) AS lat,
            astro_lmst(astro_jd_from_timestamp(now()),
                       (SELECT lon FROM observer)) AS lmst_h
    ),
    -- A few well-known bright stars (RA/Dec in degrees, ICRS)
    stars(name, ra, dec) AS (
        VALUES
            ('Polaris',     37.9547,  89.2642),
            ('Vega',       279.2347,  38.7837),
            ('Sirius',     101.2875, -16.7161),
            ('Betelgeuse',  88.7929,   7.4071),
            ('Antares',    247.3519, -26.4320),
            ('Arcturus',   213.9153,  19.1825)
    )
SELECT
    name,
    (astro_altaz_from_radec(ra, dec, (SELECT lmst_h FROM obs),
                             (SELECT lat  FROM obs))).alt_deg AS alt_deg,
    (astro_altaz_from_radec(ra, dec, (SELECT lmst_h FROM obs),
                             (SELECT lat  FROM obs))).az_deg  AS az_deg
FROM stars
ORDER BY alt_deg DESC;
```

Polaris will always sit at altitude ≈ observer latitude (≈ 53.55° in
Hamburg) regardless of time. Stars below the horizon get negative altitudes
and would normally be filtered out with `WHERE alt_deg > 0`.

### Reference values for sanity-checking

| Test                                                   | Expected            |
|--------------------------------------------------------|---------------------|
| `astro_jd_from_timestamp('1970-01-01 00:00:00')`       | 2440587.5           |
| `astro_jd_from_timestamp('2000-01-01 12:00:00')`       | 2451545.0           |
| `astro_gmst(2451545.0)`                                | 18.6974 h           |
| `astro_lmst(jd, 0)` − `astro_gmst(jd)`                 | 0                   |
| `astro_lmst(jd, 15)` − `astro_gmst(jd)`                | 1.0 h               |
| `astro_altaz_from_radec(0, 90, *, 53.55).alt_deg`      | 53.55 (= latitude)  |
| `astro_altaz_from_radec(0, 0, 0, 45).az_deg`           | 180 (due south)     |
| `astro_altaz_from_radec(90, 0, 0, 45).az_deg`          | 90 (due east)       |

### Scale-up — bright stars visible from your observatory

Combined with a real catalog (Gaia DR3 bright sample, Hipparcos, Yale BSC):

```sql
WITH
    observer(lat, lon) AS (VALUES (53.55, 10.0)),
    obs AS (
        SELECT
            (SELECT lat FROM observer) AS lat,
            astro_lmst(astro_jd_from_timestamp(now() + INTERVAL 4 HOURS),
                       (SELECT lon FROM observer)) AS lmst_h
    )
SELECT
    source_id,
    ra,
    dec,
    phot_g_mean_mag,
    (astro_altaz_from_radec(ra, dec,
        (SELECT lmst_h FROM obs),
        (SELECT lat FROM obs))).alt_deg AS alt_deg
FROM read_parquet('gaia_bright.parquet')
WHERE phot_g_mean_mag < 6           -- naked-eye limit
  AND (astro_altaz_from_radec(ra, dec,
        (SELECT lmst_h FROM obs),
        (SELECT lat FROM obs))).alt_deg > 30   -- avoid horizon haze
ORDER BY alt_deg DESC
LIMIT 50;
```

The `now() + INTERVAL 4 HOURS` lets you plan ahead — substitute any
`TIMESTAMP` literal to plan an entire observing run in advance.

### Conventions

- `ra`, `dec`, `lon_deg`, `lat_deg` are in **degrees**
- `lmst_h` is in **hours** (in the [0, 24) range from `astro_lmst`)
- Hour angle is in **hours**, signed: negative = east of meridian (rising),
  zero = on the meridian (transit / culmination), positive = west of
  meridian (setting)
- Altitude is in **degrees**, signed: negative = below the horizon
- Azimuth is measured **from North through East** (0° = N, 90° = E,
  180° = S, 270° = W)
- Eastern longitudes are positive (`+10` for Hamburg, `-122` for Berkeley)
- The sidereal time formula is the Meeus / IAU 1982 expression — accurate
  to a few seconds for any reasonable input date

---

## Notes on Units and Frames

- **Distances**: all body / orbit functions expect SI (meters, kg). The
  helper functions `astro_unit_AU`, `astro_unit_pc`, `astro_unit_ly`,
  `astro_unit_M_sun`, `astro_unit_M_earth`, `astro_unit_length_to_m`,
  `astro_unit_mass_to_kg`, and `astro_unit_time_to_s` convert for you.
- **Angles**: all coordinate inputs are in **degrees** unless a function
  is explicitly named `_rad`. Orbital elements inside the orbit STRUCT use
  radians (`i_rad`, `omega_rad`, `w_rad`, `M0_rad`).
- **Frames supported by `astro_frame_transform_pos` / `_vel`**: `icrs`,
  `galactic`. The `astro_radec_to_xyz` function tags its output with
  `frame = 'icrs'`.
- **Cosmology**: `astro_luminosity_distance` / `astro_comoving_distance`
  use a flat ΛCDM with Ω_m ≈ 0.3 and Ω_Λ ≈ 0.7. Only `H0` is exposed as a
  parameter.
- **Dust extinction**: `astro_extinction_*` uses CCM89 with O'Donnell 1994
  optical coefficients. Default `R_V = 3.1` (diffuse Milky Way ISM).
  Supported bands: `U` (3650 Å), `B` (4400 Å), `V` (5494.5 Å), `R` (6580 Å),
  `I` (8060 Å), `J` (12350 Å), `H` (16620 Å), `K` (21590 Å). Band lookups
  are case-insensitive.

---

## See Also

- [Function reference (README)](../README.md#-functions-48)
- [DuckDB Community Extensions](https://github.com/duckdb/community-extensions)
- [duckdb/astro-duck on GitHub](https://github.com/synapticore-io/astro-duck)
