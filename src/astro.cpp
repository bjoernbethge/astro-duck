#define DUCKDB_EXTENSION_MAIN

#include "astro.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include "duckdb/common/vector_operations/ternary_executor.hpp"
#include "duckdb/common/arrow/arrow_util.hpp"
#include "duckdb/common/arrow/arrow_appender.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/catalog/catalog.hpp"
#include "duckdb/catalog/catalog_entry/scalar_function_catalog_entry.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
#include "duckdb/main/client_context.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Define M_PI for Windows compatibility
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace duckdb {

// ===== ASTRONOMICAL CONSTANTS =====
const double SPEED_OF_LIGHT = 299792458.0; // m/s
const double PARSEC_TO_METERS = 3.0857e16; // meters
const double SOLAR_MASS = 1.989e30;        // kg
const double EARTH_RADIUS = 6.371e6;       // meters
const double AU = 1.496e11;                // meters (Astronomical Unit)
const double HUBBLE_CONSTANT = 70.0;       // km/s/Mpc (approximate)

// ===== FORWARD DECLARATIONS =====
std::vector<double> RADecToCartesian(double ra_deg, double dec_deg, double distance = 1.0);
double AngularSeparation(double ra1_deg, double dec1_deg, double ra2_deg, double dec2_deg);

// ===== ASTRO GEOMETRY INTEGRATION =====
class AstroGeometryProcessor {
private:
	ClientContext &context;
	unique_ptr<ExpressionExecutor> executor;

public:
	AstroGeometryProcessor(ClientContext &ctx) : context(ctx) {
		executor = make_uniq<ExpressionExecutor>(context);
		InitializeGeometryFunctions();
	}

	void InitializeGeometryFunctions() {
		// Check if spatial extension is loaded for geometry operations
		try {
			auto &catalog = Catalog::GetSystemCatalog(context);
			auto &geom_func_set = catalog.GetEntry<ScalarFunctionCatalogEntry>(context, DEFAULT_SCHEMA, "st_point");
			// Spatial extension is available
		} catch (...) {
			// Spatial extension not loaded, use basic geometry
		}
	}

	// Create celestial geometry points - uses standalone function
	string CreateCelestialPoint(double ra, double dec, double distance = 1.0) {
		auto coords = RADecToCartesian(ra, dec, distance);

		std::ostringstream wkt;
		wkt << "POINT Z(" << std::fixed << std::setprecision(8) << coords[0] << " " << coords[1] << " " << coords[2]
		    << ")";
		return wkt.str();
	}

	// Calculate spherical distance on celestial sphere - uses standalone function
	double CelestialDistance(double ra1, double dec1, double ra2, double dec2) {
		return AngularSeparation(ra1, dec1, ra2, dec2);
	}
};

// ===== COORDINATE CONVERSION FUNCTIONS =====
std::vector<double> RADecToCartesian(double ra_deg, double dec_deg, double distance) {
	double ra_rad = ra_deg * M_PI / 180.0;
	double dec_rad = dec_deg * M_PI / 180.0;

	double x = distance * cos(dec_rad) * cos(ra_rad);
	double y = distance * cos(dec_rad) * sin(ra_rad);
	double z = distance * sin(dec_rad);

	return {x, y, z};
}

double AngularSeparation(double ra1_deg, double dec1_deg, double ra2_deg, double dec2_deg) {
	double ra1 = ra1_deg * M_PI / 180.0;
	double dec1 = dec1_deg * M_PI / 180.0;
	double ra2 = ra2_deg * M_PI / 180.0;
	double dec2 = dec2_deg * M_PI / 180.0;

	double dra = ra2 - ra1;
	double ddec = dec2 - dec1;

	double a = sin(ddec / 2) * sin(ddec / 2) + cos(dec1) * cos(dec2) * sin(dra / 2) * sin(dra / 2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));

	return c * 180.0 / M_PI;
}

// ===== PHOTOMETRIC FUNCTIONS =====
double MagnitudeToFlux(double magnitude, double zero_point = 0.0) {
	return pow(10.0, (zero_point - magnitude) / 2.5);
}

double FluxToMagnitude(double flux, double zero_point = 0.0) {
	if (flux <= 0)
		return std::numeric_limits<double>::quiet_NaN();
	return -2.5 * log10(flux) + zero_point;
}

double DistanceModulus(double distance_pc) {
	if (distance_pc <= 0)
		return std::numeric_limits<double>::quiet_NaN();
	return 5.0 * log10(distance_pc) - 5.0;
}

// ===== COSMOLOGICAL FUNCTIONS =====
double LuminosityDistance(double redshift, double h0 = HUBBLE_CONSTANT) {
	double c_km_s = SPEED_OF_LIGHT / 1000.0;
	return (c_km_s * redshift) / h0;
}

double ComovingDistance(double redshift, double h0 = HUBBLE_CONSTANT) {
	return LuminosityDistance(redshift, h0) / (1.0 + redshift);
}

// ===== ENHANCED ASTRO FUNCTIONS WITH INTEGRATION =====

// Enhanced coordinate conversion with geometry support
static void AstroRADecToCartesianFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &ra_vector = args.data[0];
	auto &dec_vector = args.data[1];
	auto &distance_vector = args.data[2];

	TernaryExecutor::Execute<double, double, double, string_t>(
	    ra_vector, dec_vector, distance_vector, result, args.size(), [&](double ra, double dec, double distance) {
		    auto coords = RADecToCartesian(ra, dec, distance);

		    // Enhanced JSON output with additional metadata
		    std::ostringstream json_result;
		    json_result << "{\"x\":" << std::fixed << std::setprecision(8) << coords[0] << ",\"y\":" << coords[1]
		                << ",\"z\":" << coords[2] << ",\"ra\":" << ra << ",\"dec\":" << dec
		                << ",\"distance\":" << distance << ",\"coordinate_system\":\"ICRS\"" << ",\"epoch\":2000.0}";

		    return StringVector::AddString(result, json_result.str());
	    });
}

// Enhanced angular separation - optimized implementation
static void AstroAngularSeparationFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &ra1_vector = args.data[0];
	auto &dec1_vector = args.data[1];
	auto &ra2_vector = args.data[2];
	auto &dec2_vector = args.data[3];

	UnifiedVectorFormat ra1_data, dec1_data, ra2_data, dec2_data;
	ra1_vector.ToUnifiedFormat(args.size(), ra1_data);
	dec1_vector.ToUnifiedFormat(args.size(), dec1_data);
	ra2_vector.ToUnifiedFormat(args.size(), ra2_data);
	dec2_vector.ToUnifiedFormat(args.size(), dec2_data);

	auto ra1_ptr = UnifiedVectorFormat::GetData<double>(ra1_data);
	auto dec1_ptr = UnifiedVectorFormat::GetData<double>(dec1_data);
	auto ra2_ptr = UnifiedVectorFormat::GetData<double>(ra2_data);
	auto dec2_ptr = UnifiedVectorFormat::GetData<double>(dec2_data);
	auto result_data = FlatVector::GetData<double>(result);
	auto &result_validity = FlatVector::Validity(result);

	for (idx_t i = 0; i < args.size(); i++) {
		auto ra1_idx = ra1_data.sel->get_index(i);
		auto dec1_idx = dec1_data.sel->get_index(i);
		auto ra2_idx = ra2_data.sel->get_index(i);
		auto dec2_idx = dec2_data.sel->get_index(i);

		if (!ra1_data.validity.RowIsValid(ra1_idx) || !dec1_data.validity.RowIsValid(dec1_idx) ||
		    !ra2_data.validity.RowIsValid(ra2_idx) || !dec2_data.validity.RowIsValid(dec2_idx)) {
			result_validity.SetInvalid(i);
			continue;
		}

		result_data[i] = AngularSeparation(ra1_ptr[ra1_idx], dec1_ptr[dec1_idx], ra2_ptr[ra2_idx], dec2_ptr[dec2_idx]);
	}
}

// Create celestial geometry point
static void AstroCelestialPointFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &ra_vector = args.data[0];
	auto &dec_vector = args.data[1];
	auto &distance_vector = args.data[2];

	TernaryExecutor::Execute<double, double, double, string_t>(
	    ra_vector, dec_vector, distance_vector, result, args.size(), [&](double ra, double dec, double distance) {
		    auto coords = RADecToCartesian(ra, dec, distance);

		    std::ostringstream wkt;
		    wkt << "POINT Z(" << std::fixed << std::setprecision(8) << coords[0] << " " << coords[1] << " " << coords[2]
		        << ")";

		    return StringVector::AddString(result, wkt.str());
	    });
}

// Catalog metadata function
static void AstroCatalogInfoFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &catalog_name_vector = args.data[0];

	UnaryExecutor::Execute<string_t, string_t>(catalog_name_vector, result, args.size(), [&](string_t catalog_name) {
		string catalog_str = catalog_name.GetString();

		std::ostringstream info;
		info << "{\"catalog\":\"" << catalog_str << "\"" << ",\"version\":\"1.0.0\""
		     << ",\"coordinate_system\":\"ICRS\"" << ",\"epoch\":2000.0"
		     << ",\"supported_functions\":[\"angular_separation\",\"radec_to_cartesian\",\"mag_to_flux\",\"distance_"
		        "modulus\",\"luminosity_distance\",\"redshift_to_age\"]"
		     << ",\"extensions\":[\"arrow\",\"spatial\",\"parquet\"]}";

		return StringVector::AddString(result, info.str());
	});
}

// Enhanced photometric functions
static void AstroMagToFluxFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &mag_vector = args.data[0];
	auto &zp_vector = args.data[1];

	BinaryExecutor::Execute<double, double, double>(
	    mag_vector, zp_vector, result, args.size(),
	    [&](double magnitude, double zero_point) { return MagnitudeToFlux(magnitude, zero_point); });
}

static void AstroDistanceModulusFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &distance_vector = args.data[0];

	UnifiedVectorFormat distance_data;
	distance_vector.ToUnifiedFormat(args.size(), distance_data);

	auto distance_ptr = UnifiedVectorFormat::GetData<double>(distance_data);
	auto result_data = FlatVector::GetData<double>(result);
	auto &result_validity = FlatVector::Validity(result);

	for (idx_t i = 0; i < args.size(); i++) {
		auto distance_idx = distance_data.sel->get_index(i);

		if (!distance_data.validity.RowIsValid(distance_idx)) {
			result_validity.SetInvalid(i);
			continue;
		}

		double distance_pc = distance_ptr[distance_idx];
		if (distance_pc <= 0) {
			result_validity.SetInvalid(i);
		} else {
			result_data[i] = DistanceModulus(distance_pc);
		}
	}
}

static void AstroLuminosityDistanceFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &redshift_vector = args.data[0];
	auto &h0_vector = args.data[1];

	BinaryExecutor::Execute<double, double, double>(
	    redshift_vector, h0_vector, result, args.size(),
	    [&](double redshift, double h0) { return LuminosityDistance(redshift, h0); });
}

static void AstroRedshiftToAgeFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &redshift_vector = args.data[0];

	UnaryExecutor::Execute<double, double>(redshift_vector, result, args.size(), [&](double redshift) {
		double h0_s = HUBBLE_CONSTANT * 1000.0 / (1e6 * PARSEC_TO_METERS);
		double hubble_time = 1.0 / h0_s / (365.25 * 24 * 3600 * 1e9);
		return hubble_time / (1.0 + redshift);
	});
}

// ===== EXTENSION REGISTRATION =====
static void LoadInternal(ExtensionLoader &loader) {
	// Enhanced coordinate conversion with geometry support
	auto radec_to_cartesian =
	    ScalarFunction("radec_to_cartesian", {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE},
	                   LogicalType::VARCHAR, AstroRADecToCartesianFunction);
	loader.RegisterFunction( radec_to_cartesian);

	// Angular separation
	auto angular_separation = ScalarFunction(
	    "angular_separation", {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE},
	    LogicalType::DOUBLE, AstroAngularSeparationFunction);
	loader.RegisterFunction( angular_separation);

	// Celestial geometry point creation
	auto celestial_point =
	    ScalarFunction("celestial_point", {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE},
	                   LogicalType::VARCHAR, AstroCelestialPointFunction);
	loader.RegisterFunction( celestial_point);

	// Catalog metadata
	auto catalog_info =
	    ScalarFunction("catalog_info", {LogicalType::VARCHAR}, LogicalType::VARCHAR, AstroCatalogInfoFunction);
	loader.RegisterFunction( catalog_info);

	// Photometric functions
	auto mag_to_flux = ScalarFunction("mag_to_flux", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE,
	                                  AstroMagToFluxFunction);
	loader.RegisterFunction( mag_to_flux);

	auto distance_modulus =
	    ScalarFunction("distance_modulus", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroDistanceModulusFunction);
	loader.RegisterFunction( distance_modulus);

	// Cosmological functions
	auto luminosity_distance = ScalarFunction("luminosity_distance", {LogicalType::DOUBLE, LogicalType::DOUBLE},
	                                          LogicalType::DOUBLE, AstroLuminosityDistanceFunction);
	loader.RegisterFunction( luminosity_distance);

	auto redshift_to_age =
	    ScalarFunction("redshift_to_age", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroRedshiftToAgeFunction);
	loader.RegisterFunction( redshift_to_age);
}

void AstroExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

std::string AstroExtension::Name() {
	return "astro";
}

std::string AstroExtension::Version() const {
#ifdef EXT_VERSION_ASTRO
	return EXT_VERSION_ASTRO;
#else
	return "2.0.0";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(astro, loader) {
	duckdb::LoadInternal(loader);
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
