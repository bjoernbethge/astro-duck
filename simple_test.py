#!/usr/bin/env python3
"""
Simple Test Script for Astro Extension
Tests basic functionality using local DuckDB binary
"""

import subprocess
import sys
from pathlib import Path

def run_duckdb_query(query):
    """Run a query using local DuckDB binary"""
    duckdb_path = Path("./build/release/duckdb")
    extension_path = Path("build/release/extension/astro/astro.duckdb_extension")

    if not duckdb_path.exists():
        print(f"❌ DuckDB binary not found: {duckdb_path}")
        return None

    if not extension_path.exists():
        print(f"❌ Extension not found: {extension_path}")
        return None

    try:
        # Load extension and run query
        full_query = f"LOAD '{extension_path.absolute()}'; {query}"
        cmd = [str(duckdb_path), "-unsigned", "-c", full_query]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=Path.cwd())

        if result.returncode != 0:
            print(f"❌ DuckDB error: {result.stderr}")
            return None

        return result.stdout.strip()
    except Exception as e:
        print(f"❌ Failed to run query: {e}")
        return None

def main():
    print("🌟 Simple Astro Extension Test")
    print("=" * 40)

    # Test 1: Load extension
    print("🚀 Testing Extension Loading...")
    result = run_duckdb_query("SELECT 'Extension loaded successfully' as status")
    if result and "successfully" in result:
        print("   ✅ Extension loaded successfully")
    else:
        print("   ❌ Failed to load extension")
        return False

    # Test 2: Angular separation
    print("\n📐 Testing Angular Separation...")
    result = run_duckdb_query("SELECT angular_separation(0.0, 0.0, 1.0, 1.0) as separation")
    if result:
        print(f"   ✅ Angular separation: {result}")
    else:
        print("   ❌ Angular separation failed")

    # Test 3: Coordinate conversion
    print("\n🔄 Testing Coordinate Conversion...")
    result = run_duckdb_query("SELECT radec_to_cartesian(45.0, 30.0, 10.0) as coords")
    if result:
        print(f"   ✅ Coordinate conversion: {result[:100]}...")
    else:
        print("   ❌ Coordinate conversion failed")

    # Test 4: Magnitude to flux
    print("\n💫 Testing Magnitude to Flux...")
    result = run_duckdb_query("SELECT mag_to_flux(15.5, 25.0) as flux")
    if result:
        print(f"   ✅ Magnitude to flux: {result}")
    else:
        print("   ❌ Magnitude to flux failed")

    # Test 5: Distance modulus
    print("\n📏 Testing Distance Modulus...")
    result = run_duckdb_query("SELECT distance_modulus(1000.0) as dist_mod")
    if result:
        print(f"   ✅ Distance modulus: {result}")
    else:
        print("   ❌ Distance modulus failed")

    print("\n🎉 Basic tests completed!")
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
