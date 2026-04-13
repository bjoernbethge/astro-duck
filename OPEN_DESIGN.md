# astro-duck — open design questions

## Phase Z: erfa / IAU SOFA integration

**Status:** not started. Today astro-duck ships its own hand-written stellar
/ orbital math. For IAU-conformant coordinate transforms, time scales,
precession / nutation / aberration we want to lean on the official
reference: the ERFA C library (github.com/liberfa/erfa), which is a clean
MIT fork of the IAU SOFA routines.

**Why erfa, not SOFA directly:** SOFA's license is restrictive about
distribution and requires renaming of derived code. ERFA is the same
numerics under an MIT license and is what every modern astronomy project
(Astropy, Skyfield, …) uses when they want SOFA-level correctness without
the licensing headache.

**Minimum shape:**
- Vendor ERFA sources under `third_party/erfa/` (pure C, ~75 .c files,
  ~500 KB). License drop-in, no CMake surgery — compile alongside the
  extension sources the same way spz's `src/cc/*.cc` are compiled.
- Expose a first slice: `astro_era(jd_ut1)` (Earth Rotation Angle),
  `astro_eors(jd_tt, jd_ut1)` (equation of origins), and the big one
  `astro_bpn(jd_tt) -> STRUCT(9 doubles)` returning the IAU 2006/2000A
  precession-nutation matrix. That unlocks celestial-to-terrestrial
  transforms in SQL without hand-maths.
- Add a small vocab layer mapping DuckDB TIMESTAMP to julian date pairs
  (`jd_high`, `jd_low`) because ERFA uses two doubles to preserve
  femtosecond-scale time precision.

**Open questions:**
1. Do we also ship the planetary ephemerides (JPL DE440) alongside ERFA?
   That's a separate ~100 MB data blob; probably no, we keep DE440 as a
   separate Parquet-or-BLOB input and let users pass it in.
2. How much of the ERFA surface do we expose? Start with ~20 routines (the
   "common astrometry" set in `astropy.coordinates`), not all 240.
3. Does astro-duck's existing hand-written math become a thin compatibility
   layer over ERFA, or do we keep both and let users pick?

**Size:** ~1500 LOC SQL wrappers + ~5 MB vendored C source. CMake changes
are minor. The hard part is the vocab layer, not the numerics.

## CI build on a toolchain machine

Same open item as in sdf-duck / gaussian-duck: the distribution pipeline has
not been exercised on a clean runner yet. First run will surface whatever
Windows / Linux / macOS fallout exists in the current extension config.
