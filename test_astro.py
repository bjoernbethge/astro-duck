#!/usr/bin/env python3
"""
Test Suite for Astro DuckDB Extension
Tests all 48 astronomical functions
"""

import subprocess
import sys
from pathlib import Path
import platform

def find_duckdb():
    """Find DuckDB binary for current platform"""
    if platform.system() == "Windows":
        paths = [
            Path("build/release/Release/duckdb.exe"),
            Path("build/release/duckdb.exe"),
        ]
    else:
        paths = [
            Path("build/release/duckdb"),
        ]

    for p in paths:
        if p.exists():
            return p

    raise FileNotFoundError(f"DuckDB binary not found. Tried: {[str(p) for p in paths]}")

def run_query(query):
    """Run a DuckDB query and return the output"""
    duckdb = find_duckdb()
    result = subprocess.run(
        [str(duckdb), "-c", query],
        capture_output=True,
        text=True,
        encoding='utf-8',
        errors='replace'
    )
    if result.returncode != 0:
        raise Exception(f"Query failed: {result.stderr}")
    return result.stdout.strip() if result.stdout else ""

def run_query_value(query):
    """Run query and extract single value from table output"""
    output = run_query(query)
    if not output:
        return ""
    lines = output.strip().split('\n')
    # Find the data line (after header and separator)
    for line in lines:
        if '│' in line and '───' not in line and '┌' not in line and '└' not in line:
            # Extract value between │ symbols
            parts = line.split('│')
            if len(parts) >= 2:
                return parts[1].strip()
    return output

def test_group(name, tests):
    """Run a group of tests and return results"""
    print(f"\n{'='*50}")
    print(f" {name}")
    print('='*50)

    passed = 0
    failed = 0

    for test_name, query in tests:
        try:
            result = run_query_value(query)
            print(f"  [OK] {test_name}: {result[:60]}{'...' if len(result) > 60 else ''}")
            passed += 1
        except Exception as e:
            err_msg = str(e)[:80]
            print(f"  [FAIL] {test_name}: {err_msg}")
            failed += 1

    return passed, failed

def main():
    print("Astro DuckDB Extension Test Suite")
    print("="*50)

    # Check DuckDB exists
    try:
        duckdb = find_duckdb()
        print(f"Using DuckDB: {duckdb}")
    except FileNotFoundError as e:
        print(f"ERROR: {e}")
        return 1

    # Check extension is loaded
    try:
        query = "SELECT COUNT(*) FROM duckdb_functions() WHERE function_name LIKE 'astro%';"
        count = run_query_value(query)
        print(f"Astro functions available: {count}")
    except Exception as e:
        print(f"ERROR: Extension not available: {e}")
        return 1

    total_passed = 0
    total_failed = 0

    # Physical Constants
    p, f = test_group("Physical Constants", [
        ("Speed of light", "SELECT astro_const_c();"),
        ("Gravitational constant", "SELECT astro_const_G();"),
        ("Stefan-Boltzmann", "SELECT astro_const_sigma_sb();"),
        ("AU in meters", "SELECT astro_const_AU();"),
        ("Parsec in meters", "SELECT astro_const_pc();"),
        ("Light year in meters", "SELECT astro_const_ly();"),
        ("Solar mass", "SELECT astro_const_M_sun();"),
        ("Solar radius", "SELECT astro_const_R_sun();"),
        ("Solar luminosity", "SELECT astro_const_L_sun();"),
        ("Earth mass", "SELECT astro_const_M_earth();"),
        ("Earth radius", "SELECT astro_const_R_earth();"),
    ])
    total_passed += p
    total_failed += f

    # Unit Conversions
    p, f = test_group("Unit Conversions", [
        ("Unit AU (1.0)", "SELECT astro_unit_AU(1.0);"),
        ("Unit parsec (1.0)", "SELECT astro_unit_pc(1.0);"),
        ("Unit light year (1.0)", "SELECT astro_unit_ly(1.0);"),
        ("Unit solar mass (1.0)", "SELECT astro_unit_M_sun(1.0);"),
        ("Unit Earth mass (1.0)", "SELECT astro_unit_M_earth(1.0);"),
        ("Length to meters (pc)", "SELECT astro_unit_length_to_m(1.0, 'pc');"),
        ("Length to meters (AU)", "SELECT astro_unit_length_to_m(1.0, 'AU');"),
        ("Mass to kg (M_sun)", "SELECT astro_unit_mass_to_kg(1.0, 'M_sun');"),
        ("Time to seconds (yr)", "SELECT astro_unit_time_to_s(1.0, 'yr');"),
    ])
    total_passed += p
    total_failed += f

    # Coordinate Functions
    p, f = test_group("Coordinate Transformations", [
        ("Angular separation", "SELECT astro_angular_separation(0.0, 0.0, 1.0, 1.0);"),
        ("RA/Dec to XYZ", "SELECT astro_radec_to_xyz(45.0, 30.0, 10.0);"),
    ])
    total_passed += p
    total_failed += f

    # Photometry
    p, f = test_group("Photometry", [
        ("Magnitude to flux", "SELECT astro_mag_to_flux(15.0, 25.0);"),
        ("Flux to magnitude", "SELECT astro_flux_to_mag(1000.0, 25.0);"),
        ("Absolute magnitude", "SELECT astro_absolute_mag(10.0, 100.0);"),
        ("Distance modulus", "SELECT astro_distance_modulus(1000.0);"),
    ])
    total_passed += p
    total_failed += f

    # Cosmology
    p, f = test_group("Cosmology", [
        ("Luminosity distance", "SELECT astro_luminosity_distance(0.1, 70.0);"),
        ("Comoving distance", "SELECT astro_comoving_distance(1.0, 70.0);"),
    ])
    total_passed += p
    total_failed += f

    # Body Models
    p, f = test_group("Celestial Body Models", [
        ("Main sequence star (1 M_sun)", "SELECT astro_body_star_ms(1.0);"),
        ("White dwarf (0.6 M_sun)", "SELECT astro_body_star_white_dwarf(0.6);"),
        ("Neutron star (1.4 M_sun)", "SELECT astro_body_star_neutron(1.4);"),
        ("Brown dwarf (50 M_jup)", "SELECT astro_body_brown_dwarf(50.0);"),
        ("Black hole (10 M_sun)", "SELECT astro_body_black_hole(10.0);"),
        ("Rocky planet (1 M_earth)", "SELECT astro_body_planet_rocky(1.0);"),
        ("Gas giant (1 M_jup)", "SELECT astro_body_planet_gas_giant(1.0);"),
        ("Ice giant (17 M_earth)", "SELECT astro_body_planet_ice_giant(17.0);"),
        ("Asteroid (500km, 2000 kg/m3)", "SELECT astro_body_asteroid(500.0, 2000.0);"),
    ])
    total_passed += p
    total_failed += f

    # Orbital Mechanics
    p, f = test_group("Orbital Mechanics", [
        ("Orbit period", "SELECT astro_orbit_period(1.496e11, 1.989e30);"),
        ("Orbit mean motion", "SELECT astro_orbit_mean_motion(1.496e11, 1.989e30);"),
        # Complex STRUCT-based functions:
        ("Orbit make", "SELECT astro_orbit_make(1.496e11, 0.0167, 0.0, 0.0, 0.0, 0.0, 2451545.0, 1.989e30, 'icrs');"),
    ])
    total_passed += p
    total_failed += f

    # Spatial Sectors (3D octree, not HEALPix)
    p, f = test_group("Spatial Sectors (Octree)", [
        ("Sector ID from XYZ", "SELECT astro_sector_id(1.0, 0.0, 0.0, 3);"),
    ])
    total_passed += p
    total_failed += f

    # Summary
    print("\n" + "="*50)
    print(" TEST SUMMARY")
    print("="*50)
    print(f"  Passed: {total_passed}")
    print(f"  Failed: {total_failed}")
    print(f"  Total:  {total_passed + total_failed}")

    if total_failed == 0:
        print("\n  ALL TESTS PASSED!")
        return 0
    else:
        print(f"\n  {total_failed} TESTS FAILED")
        return 1

if __name__ == "__main__":
    sys.exit(main())
