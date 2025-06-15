#!/usr/bin/env python3
"""
Comprehensive Test Suite for Refactored Astro Extension
Tests all astronomical functions with integrated Arrow, GeoParquet, and catalog features
"""

import duckdb
import json
import time
import sys
from pathlib import Path

def test_extension_loading():
    """Test loading the refactored astro extension"""
    print("🚀 Testing Extension Loading...")
    
    try:
        # Create connection with unsigned extensions allowed
        conn = duckdb.connect(config={'allow_unsigned_extensions': 'true'})
        
        # Load the refactored extension
        extension_path = Path("build/astro_release/extension/astro/astro.duckdb_extension")
        if not extension_path.exists():
            raise FileNotFoundError(f"Extension not found: {extension_path}")
        
        conn.execute(f"LOAD '{extension_path.absolute()}'")
        print("   ✅ Refactored Astro extension loaded successfully")
        
        return conn
        
    except Exception as e:
        print(f"   ❌ Failed to load extension: {e}")
        return None

def test_basic_functions(conn):
    """Test basic astronomical functions"""
    print("\n📐 Testing Basic Astronomical Functions...")
    
    tests = [
        {
            'name': 'Angular Separation',
            'query': "SELECT angular_separation(0.0, 0.0, 1.0, 1.0) as separation",
            'expected_type': float
        },
        {
            'name': 'RA/Dec to Cartesian (Enhanced)',
            'query': "SELECT radec_to_cartesian(45.0, 30.0, 10.0) as coords",
            'expected_type': str
        },
        {
            'name': 'Magnitude to Flux',
            'query': "SELECT mag_to_flux(15.0, 25.0) as flux",
            'expected_type': float
        },
        {
            'name': 'Distance Modulus',
            'query': "SELECT distance_modulus(100.0) as dm",
            'expected_type': float
        },
        {
            'name': 'Luminosity Distance',
            'query': "SELECT luminosity_distance(0.1, 70.0) as dl",
            'expected_type': float
        },
        {
            'name': 'Redshift to Age',
            'query': "SELECT redshift_to_age(1.0) as age",
            'expected_type': float
        }
    ]
    
    results = {}
    for test in tests:
        try:
            result = conn.execute(test['query']).fetchone()[0]
            if isinstance(result, test['expected_type']):
                print(f"   ✅ {test['name']}: {result}")
                results[test['name']] = result
            else:
                print(f"   ⚠️  {test['name']}: Unexpected type {type(result)}")
                results[test['name']] = result
        except Exception as e:
            print(f"   ❌ {test['name']}: {e}")
            results[test['name']] = None
    
    return results

def test_enhanced_functions(conn):
    """Test enhanced functions with metadata"""
    print("\n🔬 Testing Enhanced Functions with Metadata...")
    
    try:
        # Test enhanced coordinate conversion
        result = conn.execute("""
            SELECT radec_to_cartesian(45.0, 30.0, 10.0) as enhanced_coords
        """).fetchone()[0]
        
        # Parse JSON result
        coords_data = json.loads(result)
        print(f"   ✅ Enhanced Coordinates: {coords_data}")
        
        # Verify enhanced metadata
        expected_keys = ['x', 'y', 'z', 'ra', 'dec', 'distance', 'coordinate_system', 'epoch']
        missing_keys = [key for key in expected_keys if key not in coords_data]
        
        if not missing_keys:
            print("   ✅ All metadata fields present")
        else:
            print(f"   ⚠️  Missing metadata fields: {missing_keys}")
        
        return coords_data
        
    except Exception as e:
        print(f"   ❌ Enhanced functions test failed: {e}")
        return None

def test_new_functions(conn):
    """Test new functions added in refactored version"""
    print("\n🆕 Testing New Functions...")
    
    try:
        # Test celestial point creation
        result = conn.execute("""
            SELECT celestial_point(45.0, 30.0, 10.0) as wkt_point
        """).fetchone()[0]
        print(f"   ✅ Celestial Point (WKT): {result}")
        
        # Test catalog info
        result = conn.execute("""
            SELECT catalog_info('test_catalog') as info
        """).fetchone()[0]
        
        catalog_info = json.loads(result)
        print(f"   ✅ Catalog Info: {catalog_info}")
        
        return {'celestial_point': result, 'catalog_info': catalog_info}
        
    except Exception as e:
        print(f"   ❌ New functions test failed: {e}")
        return None

def test_batch_processing(conn):
    """Test batch processing capabilities"""
    print("\n⚡ Testing Batch Processing Performance...")
    
    try:
        # Create test data
        conn.execute("""
            CREATE TEMPORARY TABLE test_stars AS
            SELECT 
                i as star_id,
                random() * 360 as ra,
                (random() - 0.5) * 180 as dec,
                random() * 1000 + 10 as distance,
                random() * 5 + 10 as magnitude
            FROM range(10000) t(i)
        """)
        
        # Test batch coordinate conversion
        start_time = time.time()
        result = conn.execute("""
            SELECT 
                COUNT(*) as total_stars,
                AVG(angular_separation(ra, dec, 0.0, 0.0)) as avg_separation,
                AVG(mag_to_flux(magnitude, 25.0)) as avg_flux
            FROM test_stars
        """).fetchone()
        
        processing_time = time.time() - start_time
        
        print(f"   ✅ Processed {result[0]} stars in {processing_time:.3f}s")
        print(f"   📊 Average separation: {result[1]:.3f}°")
        print(f"   📊 Average flux: {result[2]:.6f}")
        
        return {
            'stars_processed': result[0],
            'processing_time': processing_time,
            'avg_separation': result[1],
            'avg_flux': result[2]
        }
        
    except Exception as e:
        print(f"   ❌ Batch processing test failed: {e}")
        return None

def test_integration_features(conn):
    """Test integration features (Arrow, Catalog, etc.)"""
    print("\n🔗 Testing Integration Features...")
    
    try:
        # Test catalog metadata integration
        result = conn.execute("""
            SELECT catalog_info('astro_survey') as survey_info
        """).fetchone()[0]
        
        survey_data = json.loads(result)
        
        # Verify integration metadata
        expected_extensions = ['arrow', 'spatial', 'parquet']
        available_extensions = survey_data.get('extensions', [])
        
        print(f"   📋 Available extensions: {available_extensions}")
        
        for ext in expected_extensions:
            if ext in available_extensions:
                print(f"   ✅ {ext.capitalize()} integration available")
            else:
                print(f"   ⚠️  {ext.capitalize()} integration not listed")
        
        # Test coordinate system support
        coord_system = survey_data.get('coordinate_system', 'Unknown')
        epoch = survey_data.get('epoch', 'Unknown')
        
        print(f"   🌐 Coordinate System: {coord_system}")
        print(f"   📅 Epoch: {epoch}")
        
        return survey_data
        
    except Exception as e:
        print(f"   ❌ Integration features test failed: {e}")
        return None

def test_error_handling(conn):
    """Test error handling and edge cases"""
    print("\n🛡️  Testing Error Handling...")
    
    error_tests = [
        {
            'name': 'Invalid coordinates',
            'query': "SELECT angular_separation(NULL, 0.0, 1.0, 1.0)",
            'expect_null': True
        },
        {
            'name': 'Negative distance',
            'query': "SELECT distance_modulus(-10.0)",
            'expect_null': True
        },
        {
            'name': 'Zero flux',
            'query': "SELECT mag_to_flux(0.0, 0.0)",
            'expect_null': False
        }
    ]
    
    for test in error_tests:
        try:
            result = conn.execute(test['query']).fetchone()[0]
            if test['expect_null'] and result is None:
                print(f"   ✅ {test['name']}: Correctly returned NULL")
            elif not test['expect_null'] and result is not None:
                print(f"   ✅ {test['name']}: Returned {result}")
            else:
                print(f"   ⚠️  {test['name']}: Unexpected result {result}")
        except Exception as e:
            print(f"   ❌ {test['name']}: {e}")

def run_comprehensive_test():
    """Run all tests"""
    print("🌟 Comprehensive Astro Extension Test Suite")
    print("=" * 50)
    
    # Load extension
    conn = test_extension_loading()
    if not conn:
        print("❌ Cannot proceed without extension")
        return False
    
    # Run all test suites
    basic_results = test_basic_functions(conn)
    enhanced_results = test_enhanced_functions(conn)
    new_functions_results = test_new_functions(conn)
    batch_results = test_batch_processing(conn)
    integration_results = test_integration_features(conn)
    test_error_handling(conn)
    
    # Summary
    print("\n📊 Test Summary")
    print("=" * 30)
    
    total_tests = len(basic_results) if basic_results else 0
    passed_tests = sum(1 for v in basic_results.values() if v is not None) if basic_results else 0
    
    print(f"Basic Functions: {passed_tests}/{total_tests} passed")
    print(f"Enhanced Features: {'✅' if enhanced_results else '❌'}")
    print(f"New Functions: {'✅' if new_functions_results else '❌'}")
    print(f"Batch Processing: {'✅' if batch_results else '❌'}")
    print(f"Integration Features: {'✅' if integration_results else '❌'}")
    
    if batch_results:
        print(f"Performance: {batch_results['stars_processed']} objects in {batch_results['processing_time']:.3f}s")
    
    success_rate = (passed_tests / total_tests * 100) if total_tests > 0 else 0
    print(f"\n🎯 Overall Success Rate: {success_rate:.1f}%")
    
    conn.close()
    return success_rate > 80

if __name__ == "__main__":
    success = run_comprehensive_test()
    sys.exit(0 if success else 1) 