#define DUCKDB_EXTENSION_MAIN

#include "astro_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/common/vector_operations/ternary_executor.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/common/types/vector.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

// Windows MSVC doesn't define M_PI by default
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace duckdb {

// ============================================================================
// CONSTANTS
// ============================================================================
constexpr double DEG_TO_RAD = 0.017453292519943295;  // M_PI / 180
constexpr double RAD_TO_DEG = 57.29577951308232;     // 180 / M_PI

// Physical constants (IAU 2015 nominal values where applicable)
constexpr double CONST_C = 299792458.0;              // m/s
constexpr double CONST_G = 6.67430e-11;              // m^3/(kg*s^2)
constexpr double CONST_M_SUN = 1.98892e30;           // kg
constexpr double CONST_R_SUN = 6.96340e8;            // m
constexpr double CONST_L_SUN = 3.828e26;             // W
constexpr double CONST_M_EARTH = 5.9722e24;          // kg
constexpr double CONST_R_EARTH = 6.371e6;            // m
constexpr double CONST_AU = 1.495978707e11;          // m
constexpr double CONST_PC = 3.0856775814913673e16;   // m
constexpr double CONST_LY = 9.4607304725808e15;      // m
constexpr double CONST_SIGMA_SB = 5.670374419e-8;    // W/(m^2*K^4)
constexpr double CONST_M_JUPITER = 1.898e27;         // kg
constexpr double CONST_R_JUPITER = 6.9911e7;         // m
constexpr double JULIAN_DAY_SECONDS = 86400.0;
constexpr double SECTOR_BASE_SIZE_M = 1e12;          // 1 trillion meters at level 0

// Galactic coordinate transformation (ICRS J2000 pole and center)
// North Galactic Pole: RA=192.85948°, Dec=27.12825° (ICRS)
// Galactic center: l=0, b=0 at RA=266.40510°, Dec=-28.93617°
constexpr double NGP_RA_RAD = 3.3660332687500043;    // 192.85948° in rad
constexpr double NGP_DEC_RAD = 0.4734773249531265;   // 27.12825° in rad
constexpr double GAL_LON_NCP = 2.1455716467163547;   // 122.932° in rad

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================
static LogicalType GetAstroPosType() {
    static LogicalType type = LogicalType::STRUCT({
        {"x_m", LogicalType::DOUBLE},
        {"y_m", LogicalType::DOUBLE},
        {"z_m", LogicalType::DOUBLE},
        {"frame", LogicalType::VARCHAR}
    });
    return type;
}

static LogicalType GetAstroVelType() {
    static LogicalType type = LogicalType::STRUCT({
        {"vx_ms", LogicalType::DOUBLE},
        {"vy_ms", LogicalType::DOUBLE},
        {"vz_ms", LogicalType::DOUBLE},
        {"frame", LogicalType::VARCHAR}
    });
    return type;
}

static LogicalType GetAstroOrbitType() {
    static LogicalType type = LogicalType::STRUCT({
        {"a_m", LogicalType::DOUBLE},
        {"e", LogicalType::DOUBLE},
        {"i_rad", LogicalType::DOUBLE},
        {"omega_rad", LogicalType::DOUBLE},
        {"w_rad", LogicalType::DOUBLE},
        {"M0_rad", LogicalType::DOUBLE},
        {"epoch_jd", LogicalType::DOUBLE},
        {"central_mass_kg", LogicalType::DOUBLE},
        {"frame", LogicalType::VARCHAR}
    });
    return type;
}

static LogicalType GetAstroSectorIdType() {
    static LogicalType type = LogicalType::STRUCT({
        {"x", LogicalType::BIGINT},
        {"y", LogicalType::BIGINT},
        {"z", LogicalType::BIGINT},
        {"level", LogicalType::INTEGER}
    });
    return type;
}

static LogicalType GetBodyType() {
    static LogicalType type = LogicalType::STRUCT({
        {"mass_kg", LogicalType::DOUBLE},
        {"radius_m", LogicalType::DOUBLE},
        {"luminosity_w", LogicalType::DOUBLE},
        {"temperature_k", LogicalType::DOUBLE},
        {"density_kg_m3", LogicalType::DOUBLE},
        {"body_type", LogicalType::VARCHAR}
    });
    return type;
}

static LogicalType GetSectorBoundsType() {
    static LogicalType type = LogicalType::STRUCT({
        {"min_x_m", LogicalType::DOUBLE},
        {"max_x_m", LogicalType::DOUBLE},
        {"min_y_m", LogicalType::DOUBLE},
        {"max_y_m", LogicalType::DOUBLE},
        {"min_z_m", LogicalType::DOUBLE},
        {"max_z_m", LogicalType::DOUBLE}
    });
    return type;
}

// ============================================================================
// MATH HELPERS
// ============================================================================
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    double dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
    double length() const { return sqrt(x * x + y * y + z * z); }
    double length2() const { return x * x + y * y + z * z; }
};

// 3x3 rotation matrix
struct Mat3 {
    double m[9];
    Vec3 apply(const Vec3 &v) const {
        return {
            m[0] * v.x + m[1] * v.y + m[2] * v.z,
            m[3] * v.x + m[4] * v.y + m[5] * v.z,
            m[6] * v.x + m[7] * v.y + m[8] * v.z
        };
    }
    Mat3 transpose() const {
        return {{m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]}};
    }
};

// ICRS <-> Galactic rotation matrix
static Mat3 GetICRSToGalacticMatrix() {
    // Precomputed rotation matrix from ICRS to Galactic coordinates
    // Based on IAU 1958 definition with J2000 pole
    static Mat3 mat = {{
        -0.0548755604162154, -0.8734370902348850, -0.4838350155487132,
        +0.4941094278755837, -0.4448296299600112, +0.7469822444972812,
        -0.8676661490190047, -0.1980763734312015, +0.4559837761750669
    }};
    return mat;
}

// Spherical to Cartesian (unit sphere)
static Vec3 SphericalToCartesian(double lon_rad, double lat_rad) {
    double cos_lat = cos(lat_rad);
    return {cos_lat * cos(lon_rad), cos_lat * sin(lon_rad), sin(lat_rad)};
}

// Cartesian to Spherical
static void CartesianToSpherical(const Vec3 &v, double &lon_rad, double &lat_rad) {
    lat_rad = atan2(v.z, sqrt(v.x * v.x + v.y * v.y));
    lon_rad = atan2(v.y, v.x);
    if (lon_rad < 0) lon_rad += 2.0 * M_PI;
}

// Kepler equation solver (Newton-Raphson)
static double SolveKeplerEquation(double M, double e) {
    double E = M;
    for (int i = 0; i < 50; i++) {
        double delta = (E - e * sin(E) - M) / (1.0 - e * cos(E));
        E -= delta;
        if (std::abs(delta) < 1e-12) break;
    }
    return E;
}

// True anomaly from eccentric anomaly
static double TrueAnomalyFromEccentric(double E, double e) {
    return 2.0 * atan2(sqrt(1.0 + e) * sin(E / 2.0), sqrt(1.0 - e) * cos(E / 2.0));
}

// Compute orbital state (position and velocity) from Keplerian elements
struct OrbitalState {
    Vec3 pos;
    Vec3 vel;
};

static OrbitalState ComputeOrbitalState(double a, double e, double i, double omega,
                                         double w, double M0, double epoch_jd,
                                         double central_mass, double t_jd) {
    // Mean motion
    double n = sqrt(CONST_G * central_mass / (a * a * a));

    // Mean anomaly at time t
    double dt_s = (t_jd - epoch_jd) * JULIAN_DAY_SECONDS;
    double M = fmod(M0 + n * dt_s, 2.0 * M_PI);
    if (M < 0) M += 2.0 * M_PI;

    // Solve Kepler's equation
    double E = SolveKeplerEquation(M, e);
    double nu = TrueAnomalyFromEccentric(E, e);

    // Distance and position in orbital plane
    double r = a * (1.0 - e * cos(E));
    double x_orb = r * cos(nu);
    double y_orb = r * sin(nu);

    // Velocity in orbital plane
    double h = sqrt(CONST_G * central_mass * a * (1.0 - e * e));
    double vx_orb = -h / r * sin(nu);
    double vy_orb = h / r * (e + cos(nu));

    // Rotation matrix elements
    double cO = cos(omega), sO = sin(omega);
    double ci = cos(i), si = sin(i);
    double cw = cos(w), sw = sin(w);

    // Combined rotation matrix (3-1-3: Omega, i, w)
    double r11 = cO * cw - sO * sw * ci;
    double r12 = -cO * sw - sO * cw * ci;
    double r21 = sO * cw + cO * sw * ci;
    double r22 = -sO * sw + cO * cw * ci;
    double r31 = sw * si;
    double r32 = cw * si;

    OrbitalState state;
    state.pos = {r11 * x_orb + r12 * y_orb, r21 * x_orb + r22 * y_orb, r31 * x_orb + r32 * y_orb};
    state.vel = {r11 * vx_orb + r12 * vy_orb, r21 * vx_orb + r22 * vy_orb, r31 * vx_orb + r32 * vy_orb};
    return state;
}

// Sector size helper
static double GetSectorSize(int32_t level) {
    return SECTOR_BASE_SIZE_M / static_cast<double>(1LL << level);
}

// ============================================================================
// CONSTANT FUNCTIONS
// ============================================================================
#define DEFINE_CONST_FUNC(Name, Value) \
static void AstroConst##Name(DataChunk &args, ExpressionState &state, Vector &result) { \
    result.SetVectorType(VectorType::CONSTANT_VECTOR); \
    ConstantVector::GetData<double>(result)[0] = Value; \
}

DEFINE_CONST_FUNC(C, CONST_C)
DEFINE_CONST_FUNC(G, CONST_G)
DEFINE_CONST_FUNC(MSun, CONST_M_SUN)
DEFINE_CONST_FUNC(RSun, CONST_R_SUN)
DEFINE_CONST_FUNC(LSun, CONST_L_SUN)
DEFINE_CONST_FUNC(MEarth, CONST_M_EARTH)
DEFINE_CONST_FUNC(REarth, CONST_R_EARTH)
DEFINE_CONST_FUNC(AU, CONST_AU)
DEFINE_CONST_FUNC(Pc, CONST_PC)
DEFINE_CONST_FUNC(Ly, CONST_LY)
DEFINE_CONST_FUNC(SigmaSB, CONST_SIGMA_SB)

#undef DEFINE_CONST_FUNC

// ============================================================================
// UNIT CONVERSION FUNCTIONS
// ============================================================================
static void AstroUnitLengthToM(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, string_t, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double value, string_t unit_str) {
            string unit = StringUtil::Lower(unit_str.GetString());
            if (unit == "m") return value;
            if (unit == "km") return value * 1000.0;
            if (unit == "au") return value * CONST_AU;
            if (unit == "ly") return value * CONST_LY;
            if (unit == "pc") return value * CONST_PC;
            throw InvalidInputException("Unknown length unit: '%s'. Valid: m, km, AU, ly, pc", unit.c_str());
        });
}

static void AstroUnitMassToKg(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, string_t, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double value, string_t unit_str) {
            string unit = StringUtil::Lower(unit_str.GetString());
            if (unit == "kg") return value;
            if (unit == "m_sun" || unit == "msun") return value * CONST_M_SUN;
            if (unit == "m_earth" || unit == "mearth") return value * CONST_M_EARTH;
            if (unit == "m_jupiter" || unit == "mjup") return value * CONST_M_JUPITER;
            throw InvalidInputException("Unknown mass unit: '%s'. Valid: kg, M_sun, M_earth, M_jupiter", unit.c_str());
        });
}

static void AstroUnitTimeToS(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, string_t, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double value, string_t unit_str) {
            string unit = StringUtil::Lower(unit_str.GetString());
            if (unit == "s") return value;
            if (unit == "min") return value * 60.0;
            if (unit == "h") return value * 3600.0;
            if (unit == "d") return value * 86400.0;
            if (unit == "yr") return value * 31557600.0;  // Julian year
            if (unit == "myr") return value * 31557600.0e6;
            if (unit == "gyr") return value * 31557600.0e9;
            throw InvalidInputException("Unknown time unit: '%s'. Valid: s, min, h, d, yr, Myr, Gyr", unit.c_str());
        });
}

// Unit shortcuts
#define DEFINE_UNIT_SHORTCUT(Name, Factor) \
static void AstroUnit##Name(DataChunk &args, ExpressionState &state, Vector &result) { \
    UnaryExecutor::Execute<double, double>(args.data[0], result, args.size(), \
        [](double v) { return v * Factor; }); \
}

DEFINE_UNIT_SHORTCUT(AU, CONST_AU)
DEFINE_UNIT_SHORTCUT(pc, CONST_PC)
DEFINE_UNIT_SHORTCUT(ly, CONST_LY)
DEFINE_UNIT_SHORTCUT(M_sun, CONST_M_SUN)
DEFINE_UNIT_SHORTCUT(M_earth, CONST_M_EARTH)

#undef DEFINE_UNIT_SHORTCUT

// ============================================================================
// BODY MODEL FUNCTIONS
// ============================================================================
struct BodyProperties {
    double mass_kg;
    double radius_m;
    double luminosity_w;
    double temperature_k;
    double density_kg_m3;
    const char *body_type;
};

static void WriteBodyToResult(Vector &result, idx_t row, const BodyProperties &props) {
    auto &children = StructVector::GetEntries(result);
    FlatVector::GetData<double>(*children[0])[row] = props.mass_kg;
    FlatVector::GetData<double>(*children[1])[row] = props.radius_m;
    FlatVector::GetData<double>(*children[2])[row] = props.luminosity_w;
    FlatVector::GetData<double>(*children[3])[row] = props.temperature_k;
    FlatVector::GetData<double>(*children[4])[row] = props.density_kg_m3;
    FlatVector::GetData<string_t>(*children[5])[row] = StringVector::AddString(*children[5], props.body_type);
}

static double ComputeDensity(double mass_kg, double radius_m) {
    double volume = (4.0 / 3.0) * M_PI * radius_m * radius_m * radius_m;
    return mass_kg / volume;
}

static void AstroBodyMakeStarMs(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_msun = mass_ptr[input.sel->get_index(i)];

        // Main-sequence relations (simplified but physically motivated)
        double L_lsun = pow(mass_msun, 3.5);
        double R_rsun = pow(mass_msun, 0.8);
        double L_w = L_lsun * CONST_L_SUN;
        double R_m = R_rsun * CONST_R_SUN;
        double T_k = pow(L_w / (4.0 * M_PI * R_m * R_m * CONST_SIGMA_SB), 0.25);
        double M_kg = mass_msun * CONST_M_SUN;

        WriteBodyToResult(result, i, {M_kg, R_m, L_w, T_k, ComputeDensity(M_kg, R_m), "star_main_sequence"});
    }
}

static void AstroBodyMakePlanetRocky(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_mearth = mass_ptr[input.sel->get_index(i)];

        // Chen & Kipping 2017 mass-radius relation for rocky planets
        double R_rearth = mass_mearth < 1.0 ? pow(mass_mearth, 0.27) : pow(mass_mearth, 0.55);
        double M_kg = mass_mearth * CONST_M_EARTH;
        double R_m = R_rearth * CONST_R_EARTH;

        WriteBodyToResult(result, i, {M_kg, R_m, 0.0, 0.0, ComputeDensity(M_kg, R_m), "planet_rocky"});
    }
}

static void AstroBodyMakePlanetGasGiant(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_mjup = mass_ptr[input.sel->get_index(i)];

        // Gas giants have roughly constant radius (degeneracy pressure)
        double R_rjup = mass_mjup < 0.3 ? pow(mass_mjup / 0.3, 0.6) : pow(mass_mjup, -0.04);
        double M_kg = mass_mjup * CONST_M_JUPITER;
        double R_m = R_rjup * CONST_R_JUPITER;

        WriteBodyToResult(result, i, {M_kg, R_m, 0.0, 0.0, ComputeDensity(M_kg, R_m), "planet_gas_giant"});
    }
}

static void AstroBodyMakePlanetIceGiant(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    // Neptune mass/radius for scaling
    constexpr double M_NEPTUNE = 1.024e26;  // kg
    constexpr double R_NEPTUNE = 2.4622e7;  // m

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_mnep = mass_ptr[input.sel->get_index(i)];

        // Ice giant mass-radius relation (Neptune-like, higher density than gas giants)
        double R_rnep = pow(mass_mnep, 0.55);
        double M_kg = mass_mnep * M_NEPTUNE;
        double R_m = R_rnep * R_NEPTUNE;

        WriteBodyToResult(result, i, {M_kg, R_m, 0.0, 0.0, ComputeDensity(M_kg, R_m), "planet_ice_giant"});
    }
}

static void AstroBodyMakeStarWhiteDwarf(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_msun = mass_ptr[input.sel->get_index(i)];

        // White dwarf: Mass-radius relation (Chandrasekhar) - radius decreases with mass
        // R ∝ M^(-1/3) normalized to ~0.01 R_sun at 0.6 M_sun
        double R_rsun = 0.01 * pow(0.6 / mass_msun, 1.0/3.0);
        double M_kg = mass_msun * CONST_M_SUN;
        double R_m = R_rsun * CONST_R_SUN;

        // Luminosity from cooling curve approximation (L ∝ T^4, typical T ~ 10000-20000 K)
        double T_k = 15000.0 * pow(mass_msun / 0.6, 0.1);  // simplified
        double L_w = 4.0 * M_PI * R_m * R_m * CONST_SIGMA_SB * pow(T_k, 4);

        WriteBodyToResult(result, i, {M_kg, R_m, L_w, T_k, ComputeDensity(M_kg, R_m), "star_white_dwarf"});
    }
}

static void AstroBodyMakeStarNeutron(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_msun = mass_ptr[input.sel->get_index(i)];

        // Neutron star: ~10-12 km radius, roughly constant
        double R_m = 1.1e4;  // ~11 km typical radius
        double M_kg = mass_msun * CONST_M_SUN;

        // Young neutron stars are hot; simplify to ~1e6 K
        double T_k = 1.0e6;
        double L_w = 4.0 * M_PI * R_m * R_m * CONST_SIGMA_SB * pow(T_k, 4);

        WriteBodyToResult(result, i, {M_kg, R_m, L_w, T_k, ComputeDensity(M_kg, R_m), "star_neutron"});
    }
}

static void AstroBodyMakeBrownDwarf(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_mjup = mass_ptr[input.sel->get_index(i)];

        // Brown dwarfs: 13-80 M_jup, roughly Jupiter-sized due to degeneracy
        // Radius nearly constant at ~0.1 R_sun for most of the range
        double R_m = 0.1 * CONST_R_SUN;
        double M_kg = mass_mjup * CONST_M_JUPITER;

        // Temperature: ~500-2500 K depending on mass and age
        double T_k = 1000.0 + 1500.0 * (mass_mjup / 80.0);
        double L_w = 4.0 * M_PI * R_m * R_m * CONST_SIGMA_SB * pow(T_k, 4);

        WriteBodyToResult(result, i, {M_kg, R_m, L_w, T_k, ComputeDensity(M_kg, R_m), "brown_dwarf"});
    }
}

static void AstroBodyMakeBlackHole(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat input;
    args.data[0].ToUnifiedFormat(args.size(), input);
    auto mass_ptr = UnifiedVectorFormat::GetData<double>(input);

    for (idx_t i = 0; i < args.size(); i++) {
        double mass_msun = mass_ptr[input.sel->get_index(i)];

        double M_kg = mass_msun * CONST_M_SUN;
        // Schwarzschild radius: r_s = 2GM/c^2
        double R_m = 2.0 * CONST_G * M_kg / (CONST_C * CONST_C);

        // Black holes have no luminosity (except Hawking radiation, negligible for stellar mass)
        // Temperature set to 0 for classical treatment
        WriteBodyToResult(result, i, {M_kg, R_m, 0.0, 0.0, ComputeDensity(M_kg, R_m), "black_hole"});
    }
}

// Asteroid: inputs are radius_km and density_kg_m3
static void AstroBodyMakeAsteroid(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat r_fmt, d_fmt;
    args.data[0].ToUnifiedFormat(args.size(), r_fmt);
    args.data[1].ToUnifiedFormat(args.size(), d_fmt);
    auto r_ptr = UnifiedVectorFormat::GetData<double>(r_fmt);
    auto d_ptr = UnifiedVectorFormat::GetData<double>(d_fmt);

    for (idx_t i = 0; i < args.size(); i++) {
        double radius_m = r_ptr[r_fmt.sel->get_index(i)] * 1000.0;  // km to m
        double density = d_ptr[d_fmt.sel->get_index(i)];

        double volume = (4.0 / 3.0) * M_PI * radius_m * radius_m * radius_m;
        double M_kg = density * volume;

        WriteBodyToResult(result, i, {M_kg, radius_m, 0.0, 0.0, density, "asteroid"});
    }
}

// ============================================================================
// ORBIT FUNCTIONS
// ============================================================================
static void AstroOrbitMake(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &children = StructVector::GetEntries(result);

    // Copy numeric columns
    for (idx_t col = 0; col < 8; col++) {
        UnifiedVectorFormat fmt;
        args.data[col].ToUnifiedFormat(args.size(), fmt);
        auto ptr = UnifiedVectorFormat::GetData<double>(fmt);
        auto out = FlatVector::GetData<double>(*children[col]);
        for (idx_t i = 0; i < args.size(); i++) {
            out[i] = ptr[fmt.sel->get_index(i)];
        }
    }

    // Copy frame string
    UnifiedVectorFormat fmt;
    args.data[8].ToUnifiedFormat(args.size(), fmt);
    auto ptr = UnifiedVectorFormat::GetData<string_t>(fmt);
    for (idx_t i = 0; i < args.size(); i++) {
        FlatVector::GetData<string_t>(*children[8])[i] =
            StringVector::AddString(*children[8], ptr[fmt.sel->get_index(i)].GetString());
    }
}

static void AstroOrbitPeriod(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double a_m, double M_kg) {
            if (a_m <= 0 || M_kg <= 0) return std::numeric_limits<double>::quiet_NaN();
            return 2.0 * M_PI * sqrt(a_m * a_m * a_m / (CONST_G * M_kg));
        });
}

static void AstroOrbitMeanMotion(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double a_m, double M_kg) {
            if (a_m <= 0 || M_kg <= 0) return std::numeric_limits<double>::quiet_NaN();
            return sqrt(CONST_G * M_kg / (a_m * a_m * a_m));
        });
}

static void AstroOrbitPosition(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &orbit_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto a = FlatVector::GetData<double>(*orbit_children[0]);
    auto e = FlatVector::GetData<double>(*orbit_children[1]);
    auto inc = FlatVector::GetData<double>(*orbit_children[2]);
    auto omega = FlatVector::GetData<double>(*orbit_children[3]);
    auto w = FlatVector::GetData<double>(*orbit_children[4]);
    auto M0 = FlatVector::GetData<double>(*orbit_children[5]);
    auto epoch = FlatVector::GetData<double>(*orbit_children[6]);
    auto mass = FlatVector::GetData<double>(*orbit_children[7]);
    auto frame = FlatVector::GetData<string_t>(*orbit_children[8]);

    auto x_out = FlatVector::GetData<double>(*result_children[0]);
    auto y_out = FlatVector::GetData<double>(*result_children[1]);
    auto z_out = FlatVector::GetData<double>(*result_children[2]);

    UnifiedVectorFormat t_fmt;
    args.data[1].ToUnifiedFormat(args.size(), t_fmt);
    auto t_ptr = UnifiedVectorFormat::GetData<double>(t_fmt);

    for (idx_t i = 0; i < args.size(); i++) {
        double t_jd = t_ptr[t_fmt.sel->get_index(i)];
        auto state = ComputeOrbitalState(a[i], e[i], inc[i], omega[i], w[i], M0[i], epoch[i], mass[i], t_jd);
        x_out[i] = state.pos.x;
        y_out[i] = state.pos.y;
        z_out[i] = state.pos.z;
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], frame[i].GetString());
    }
}

static void AstroOrbitVelocity(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &orbit_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto a = FlatVector::GetData<double>(*orbit_children[0]);
    auto e = FlatVector::GetData<double>(*orbit_children[1]);
    auto inc = FlatVector::GetData<double>(*orbit_children[2]);
    auto omega = FlatVector::GetData<double>(*orbit_children[3]);
    auto w = FlatVector::GetData<double>(*orbit_children[4]);
    auto M0 = FlatVector::GetData<double>(*orbit_children[5]);
    auto epoch = FlatVector::GetData<double>(*orbit_children[6]);
    auto mass = FlatVector::GetData<double>(*orbit_children[7]);
    auto frame = FlatVector::GetData<string_t>(*orbit_children[8]);

    auto vx_out = FlatVector::GetData<double>(*result_children[0]);
    auto vy_out = FlatVector::GetData<double>(*result_children[1]);
    auto vz_out = FlatVector::GetData<double>(*result_children[2]);

    UnifiedVectorFormat t_fmt;
    args.data[1].ToUnifiedFormat(args.size(), t_fmt);
    auto t_ptr = UnifiedVectorFormat::GetData<double>(t_fmt);

    for (idx_t i = 0; i < args.size(); i++) {
        double t_jd = t_ptr[t_fmt.sel->get_index(i)];
        auto state = ComputeOrbitalState(a[i], e[i], inc[i], omega[i], w[i], M0[i], epoch[i], mass[i], t_jd);
        vx_out[i] = state.vel.x;
        vy_out[i] = state.vel.y;
        vz_out[i] = state.vel.z;
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], frame[i].GetString());
    }
}

// ============================================================================
// DYNAMICS FUNCTIONS
// ============================================================================
static void AstroDynGravityAccel(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &pos1_children = StructVector::GetEntries(args.data[1]);
    auto &pos2_children = StructVector::GetEntries(args.data[3]);
    auto &result_children = StructVector::GetEntries(result);

    auto x1 = FlatVector::GetData<double>(*pos1_children[0]);
    auto y1 = FlatVector::GetData<double>(*pos1_children[1]);
    auto z1 = FlatVector::GetData<double>(*pos1_children[2]);
    auto frame1 = FlatVector::GetData<string_t>(*pos1_children[3]);

    auto x2 = FlatVector::GetData<double>(*pos2_children[0]);
    auto y2 = FlatVector::GetData<double>(*pos2_children[1]);
    auto z2 = FlatVector::GetData<double>(*pos2_children[2]);

    auto ax_out = FlatVector::GetData<double>(*result_children[0]);
    auto ay_out = FlatVector::GetData<double>(*result_children[1]);
    auto az_out = FlatVector::GetData<double>(*result_children[2]);

    UnifiedVectorFormat m2_fmt;
    args.data[2].ToUnifiedFormat(args.size(), m2_fmt);
    auto m2_ptr = UnifiedVectorFormat::GetData<double>(m2_fmt);

    for (idx_t i = 0; i < args.size(); i++) {
        double m2 = m2_ptr[m2_fmt.sel->get_index(i)];
        Vec3 r = {x2[i] - x1[i], y2[i] - y1[i], z2[i] - z1[i]};
        double r3 = r.length2() * r.length();

        if (r3 < 1e-30) {
            ax_out[i] = ay_out[i] = az_out[i] = 0.0;
        } else {
            double factor = CONST_G * m2 / r3;
            ax_out[i] = factor * r.x;
            ay_out[i] = factor * r.y;
            az_out[i] = factor * r.z;
        }
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], frame1[i].GetString());
    }
}

// ============================================================================
// FRAME TRANSFORMATION FUNCTIONS
// ============================================================================
// Supported: icrs <-> galactic (fixed rotation)
// barycentric = icrs (synonym for solar system context)
static void AstroFrameTransformPos(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &pos_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto x_in = FlatVector::GetData<double>(*pos_children[0]);
    auto y_in = FlatVector::GetData<double>(*pos_children[1]);
    auto z_in = FlatVector::GetData<double>(*pos_children[2]);

    auto x_out = FlatVector::GetData<double>(*result_children[0]);
    auto y_out = FlatVector::GetData<double>(*result_children[1]);
    auto z_out = FlatVector::GetData<double>(*result_children[2]);

    UnifiedVectorFormat from_fmt, to_fmt;
    args.data[1].ToUnifiedFormat(args.size(), from_fmt);
    args.data[2].ToUnifiedFormat(args.size(), to_fmt);
    auto from_ptr = UnifiedVectorFormat::GetData<string_t>(from_fmt);
    auto to_ptr = UnifiedVectorFormat::GetData<string_t>(to_fmt);

    Mat3 icrs_to_gal = GetICRSToGalacticMatrix();
    Mat3 gal_to_icrs = icrs_to_gal.transpose();

    for (idx_t i = 0; i < args.size(); i++) {
        string from_frame = StringUtil::Lower(from_ptr[from_fmt.sel->get_index(i)].GetString());
        string to_frame = StringUtil::Lower(to_ptr[to_fmt.sel->get_index(i)].GetString());

        // Normalize frame names
        if (from_frame == "barycentric" || from_frame == "icrs") from_frame = "icrs";
        if (to_frame == "barycentric" || to_frame == "icrs") to_frame = "icrs";

        Vec3 pos = {x_in[i], y_in[i], z_in[i]};
        Vec3 result_pos;

        if (from_frame == to_frame) {
            result_pos = pos;
        } else if (from_frame == "icrs" && to_frame == "galactic") {
            result_pos = icrs_to_gal.apply(pos);
        } else if (from_frame == "galactic" && to_frame == "icrs") {
            result_pos = gal_to_icrs.apply(pos);
        } else {
            throw InvalidInputException(
                "Frame transform '%s' -> '%s' not supported. Supported: icrs/barycentric <-> galactic",
                from_frame.c_str(), to_frame.c_str());
        }

        x_out[i] = result_pos.x;
        y_out[i] = result_pos.y;
        z_out[i] = result_pos.z;
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], to_frame);
    }
}

static void AstroFrameTransformVel(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &vel_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto vx_in = FlatVector::GetData<double>(*vel_children[0]);
    auto vy_in = FlatVector::GetData<double>(*vel_children[1]);
    auto vz_in = FlatVector::GetData<double>(*vel_children[2]);

    auto vx_out = FlatVector::GetData<double>(*result_children[0]);
    auto vy_out = FlatVector::GetData<double>(*result_children[1]);
    auto vz_out = FlatVector::GetData<double>(*result_children[2]);

    UnifiedVectorFormat from_fmt, to_fmt;
    args.data[1].ToUnifiedFormat(args.size(), from_fmt);
    args.data[2].ToUnifiedFormat(args.size(), to_fmt);
    auto from_ptr = UnifiedVectorFormat::GetData<string_t>(from_fmt);
    auto to_ptr = UnifiedVectorFormat::GetData<string_t>(to_fmt);

    Mat3 icrs_to_gal = GetICRSToGalacticMatrix();
    Mat3 gal_to_icrs = icrs_to_gal.transpose();

    for (idx_t i = 0; i < args.size(); i++) {
        string from_frame = StringUtil::Lower(from_ptr[from_fmt.sel->get_index(i)].GetString());
        string to_frame = StringUtil::Lower(to_ptr[to_fmt.sel->get_index(i)].GetString());

        if (from_frame == "barycentric" || from_frame == "icrs") from_frame = "icrs";
        if (to_frame == "barycentric" || to_frame == "icrs") to_frame = "icrs";

        Vec3 vel = {vx_in[i], vy_in[i], vz_in[i]};
        Vec3 result_vel;

        if (from_frame == to_frame) {
            result_vel = vel;
        } else if (from_frame == "icrs" && to_frame == "galactic") {
            result_vel = icrs_to_gal.apply(vel);
        } else if (from_frame == "galactic" && to_frame == "icrs") {
            result_vel = gal_to_icrs.apply(vel);
        } else {
            throw InvalidInputException(
                "Frame transform '%s' -> '%s' not supported. Supported: icrs/barycentric <-> galactic",
                from_frame.c_str(), to_frame.c_str());
        }

        vx_out[i] = result_vel.x;
        vy_out[i] = result_vel.y;
        vz_out[i] = result_vel.z;
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], to_frame);
    }
}

// ============================================================================
// SECTOR FUNCTIONS
// ============================================================================
static void AstroSectorId(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &result_children = StructVector::GetEntries(result);

    auto sx_out = FlatVector::GetData<int64_t>(*result_children[0]);
    auto sy_out = FlatVector::GetData<int64_t>(*result_children[1]);
    auto sz_out = FlatVector::GetData<int64_t>(*result_children[2]);
    auto level_out = FlatVector::GetData<int32_t>(*result_children[3]);

    UnifiedVectorFormat x_fmt, y_fmt, z_fmt, level_fmt;
    args.data[0].ToUnifiedFormat(args.size(), x_fmt);
    args.data[1].ToUnifiedFormat(args.size(), y_fmt);
    args.data[2].ToUnifiedFormat(args.size(), z_fmt);
    args.data[3].ToUnifiedFormat(args.size(), level_fmt);

    auto x_ptr = UnifiedVectorFormat::GetData<double>(x_fmt);
    auto y_ptr = UnifiedVectorFormat::GetData<double>(y_fmt);
    auto z_ptr = UnifiedVectorFormat::GetData<double>(z_fmt);
    auto level_ptr = UnifiedVectorFormat::GetData<int32_t>(level_fmt);

    for (idx_t i = 0; i < args.size(); i++) {
        int32_t level = level_ptr[level_fmt.sel->get_index(i)];
        if (level < 0) {
            throw InvalidInputException("Sector level must be >= 0, got %d", level);
        }

        double size = GetSectorSize(level);
        sx_out[i] = static_cast<int64_t>(floor(x_ptr[x_fmt.sel->get_index(i)] / size));
        sy_out[i] = static_cast<int64_t>(floor(y_ptr[y_fmt.sel->get_index(i)] / size));
        sz_out[i] = static_cast<int64_t>(floor(z_ptr[z_fmt.sel->get_index(i)] / size));
        level_out[i] = level;
    }
}

static void AstroSectorCenter(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &sector_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto sx = FlatVector::GetData<int64_t>(*sector_children[0]);
    auto sy = FlatVector::GetData<int64_t>(*sector_children[1]);
    auto sz = FlatVector::GetData<int64_t>(*sector_children[2]);
    auto level = FlatVector::GetData<int32_t>(*sector_children[3]);

    auto x_out = FlatVector::GetData<double>(*result_children[0]);
    auto y_out = FlatVector::GetData<double>(*result_children[1]);
    auto z_out = FlatVector::GetData<double>(*result_children[2]);

    for (idx_t i = 0; i < args.size(); i++) {
        double size = GetSectorSize(level[i]);
        x_out[i] = (sx[i] + 0.5) * size;
        y_out[i] = (sy[i] + 0.5) * size;
        z_out[i] = (sz[i] + 0.5) * size;
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], "barycentric");
    }
}

static void AstroSectorBounds(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &sector_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto sx = FlatVector::GetData<int64_t>(*sector_children[0]);
    auto sy = FlatVector::GetData<int64_t>(*sector_children[1]);
    auto sz = FlatVector::GetData<int64_t>(*sector_children[2]);
    auto level = FlatVector::GetData<int32_t>(*sector_children[3]);

    auto min_x = FlatVector::GetData<double>(*result_children[0]);
    auto max_x = FlatVector::GetData<double>(*result_children[1]);
    auto min_y = FlatVector::GetData<double>(*result_children[2]);
    auto max_y = FlatVector::GetData<double>(*result_children[3]);
    auto min_z = FlatVector::GetData<double>(*result_children[4]);
    auto max_z = FlatVector::GetData<double>(*result_children[5]);

    for (idx_t i = 0; i < args.size(); i++) {
        double size = GetSectorSize(level[i]);
        min_x[i] = sx[i] * size;
        max_x[i] = (sx[i] + 1) * size;
        min_y[i] = sy[i] * size;
        max_y[i] = (sy[i] + 1) * size;
        min_z[i] = sz[i] * size;
        max_z[i] = (sz[i] + 1) * size;
    }
}

static void AstroSectorParent(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &sector_children = StructVector::GetEntries(args.data[0]);
    auto &result_children = StructVector::GetEntries(result);

    auto sx = FlatVector::GetData<int64_t>(*sector_children[0]);
    auto sy = FlatVector::GetData<int64_t>(*sector_children[1]);
    auto sz = FlatVector::GetData<int64_t>(*sector_children[2]);
    auto level = FlatVector::GetData<int32_t>(*sector_children[3]);

    auto sx_out = FlatVector::GetData<int64_t>(*result_children[0]);
    auto sy_out = FlatVector::GetData<int64_t>(*result_children[1]);
    auto sz_out = FlatVector::GetData<int64_t>(*result_children[2]);
    auto level_out = FlatVector::GetData<int32_t>(*result_children[3]);

    for (idx_t i = 0; i < args.size(); i++) {
        if (level[i] <= 0) {
            sx_out[i] = sx[i];
            sy_out[i] = sy[i];
            sz_out[i] = sz[i];
            level_out[i] = 0;
        } else {
            // Use arithmetic right shift for proper floor division
            sx_out[i] = sx[i] >> 1;
            sy_out[i] = sy[i] >> 1;
            sz_out[i] = sz[i] >> 1;
            level_out[i] = level[i] - 1;
        }
    }
}

// ============================================================================
// COORDINATE CONVERSION (RA/Dec <-> Cartesian)
// ============================================================================
static void AstroRadecToXyz(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &result_children = StructVector::GetEntries(result);

    auto x_out = FlatVector::GetData<double>(*result_children[0]);
    auto y_out = FlatVector::GetData<double>(*result_children[1]);
    auto z_out = FlatVector::GetData<double>(*result_children[2]);

    UnifiedVectorFormat ra_fmt, dec_fmt, dist_fmt;
    args.data[0].ToUnifiedFormat(args.size(), ra_fmt);
    args.data[1].ToUnifiedFormat(args.size(), dec_fmt);
    args.data[2].ToUnifiedFormat(args.size(), dist_fmt);

    auto ra_ptr = UnifiedVectorFormat::GetData<double>(ra_fmt);
    auto dec_ptr = UnifiedVectorFormat::GetData<double>(dec_fmt);
    auto dist_ptr = UnifiedVectorFormat::GetData<double>(dist_fmt);

    for (idx_t i = 0; i < args.size(); i++) {
        double ra = ra_ptr[ra_fmt.sel->get_index(i)] * DEG_TO_RAD;
        double dec = dec_ptr[dec_fmt.sel->get_index(i)] * DEG_TO_RAD;
        double dist = dist_ptr[dist_fmt.sel->get_index(i)];

        Vec3 v = SphericalToCartesian(ra, dec) * dist;
        x_out[i] = v.x;
        y_out[i] = v.y;
        z_out[i] = v.z;
        FlatVector::GetData<string_t>(*result_children[3])[i] =
            StringVector::AddString(*result_children[3], "icrs");
    }
}

static void AstroAngularSeparation(DataChunk &args, ExpressionState &state, Vector &result) {
    UnifiedVectorFormat ra1_fmt, dec1_fmt, ra2_fmt, dec2_fmt;
    args.data[0].ToUnifiedFormat(args.size(), ra1_fmt);
    args.data[1].ToUnifiedFormat(args.size(), dec1_fmt);
    args.data[2].ToUnifiedFormat(args.size(), ra2_fmt);
    args.data[3].ToUnifiedFormat(args.size(), dec2_fmt);

    auto ra1 = UnifiedVectorFormat::GetData<double>(ra1_fmt);
    auto dec1 = UnifiedVectorFormat::GetData<double>(dec1_fmt);
    auto ra2 = UnifiedVectorFormat::GetData<double>(ra2_fmt);
    auto dec2 = UnifiedVectorFormat::GetData<double>(dec2_fmt);
    auto out = FlatVector::GetData<double>(result);
    auto &validity = FlatVector::Validity(result);

    for (idx_t i = 0; i < args.size(); i++) {
        auto i1 = ra1_fmt.sel->get_index(i);
        auto i2 = dec1_fmt.sel->get_index(i);
        auto i3 = ra2_fmt.sel->get_index(i);
        auto i4 = dec2_fmt.sel->get_index(i);

        if (!ra1_fmt.validity.RowIsValid(i1) || !dec1_fmt.validity.RowIsValid(i2) ||
            !ra2_fmt.validity.RowIsValid(i3) || !dec2_fmt.validity.RowIsValid(i4)) {
            validity.SetInvalid(i);
            continue;
        }

        // Haversine formula
        double r1 = ra1[i1] * DEG_TO_RAD, d1 = dec1[i2] * DEG_TO_RAD;
        double r2 = ra2[i3] * DEG_TO_RAD, d2 = dec2[i4] * DEG_TO_RAD;
        double sdec = sin((d2 - d1) / 2);
        double sra = sin((r2 - r1) / 2);
        double a = sdec * sdec + cos(d1) * cos(d2) * sra * sra;
        out[i] = 2.0 * atan2(sqrt(a), sqrt(1.0 - a)) * RAD_TO_DEG;
    }
}

// ============================================================================
// PHOTOMETRY FUNCTIONS
// ============================================================================
static void AstroMagToFlux(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double mag, double zp) { return pow(10.0, (zp - mag) / 2.5); });
}

static void AstroFluxToMag(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double flux, double zp) {
            return flux > 0 ? -2.5 * log10(flux) + zp : std::numeric_limits<double>::quiet_NaN();
        });
}

static void AstroDistanceModulus(DataChunk &args, ExpressionState &state, Vector &result) {
    UnaryExecutor::Execute<double, double>(args.data[0], result, args.size(),
        [](double dist_pc) {
            return dist_pc > 0 ? 5.0 * log10(dist_pc) - 5.0 : std::numeric_limits<double>::quiet_NaN();
        });
}

static void AstroAbsoluteMag(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double app_mag, double dist_pc) {
            return dist_pc > 0 ? app_mag - 5.0 * log10(dist_pc) + 5.0 : std::numeric_limits<double>::quiet_NaN();
        });
}

// ============================================================================
// COSMOLOGY FUNCTIONS
// ============================================================================
static void AstroLuminosityDistance(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double z, double H0) {
            // Simple Hubble law approximation (valid for z << 1)
            return (CONST_C / 1000.0) * z / H0;  // Mpc
        });
}

static void AstroComovingDistance(DataChunk &args, ExpressionState &state, Vector &result) {
    BinaryExecutor::Execute<double, double, double>(
        args.data[0], args.data[1], result, args.size(),
        [](double z, double H0) {
            return (CONST_C / 1000.0) * z / H0 / (1.0 + z);  // Mpc
        });
}

// ============================================================================
// EXTENSION REGISTRATION
// ============================================================================
// Note: DuckDB provides radians(), degrees(), pi() - no need to duplicate
static void LoadInternal(ExtensionLoader &loader) {
    auto pos_type = GetAstroPosType();
    auto vel_type = GetAstroVelType();
    auto orbit_type = GetAstroOrbitType();
    auto sector_type = GetAstroSectorIdType();
    auto body_type = GetBodyType();
    auto bounds_type = GetSectorBoundsType();

    // Constants
    loader.RegisterFunction(ScalarFunction("astro_const_c", {}, LogicalType::DOUBLE, AstroConstC));
    loader.RegisterFunction(ScalarFunction("astro_const_G", {}, LogicalType::DOUBLE, AstroConstG));
    loader.RegisterFunction(ScalarFunction("astro_const_M_sun", {}, LogicalType::DOUBLE, AstroConstMSun));
    loader.RegisterFunction(ScalarFunction("astro_const_R_sun", {}, LogicalType::DOUBLE, AstroConstRSun));
    loader.RegisterFunction(ScalarFunction("astro_const_L_sun", {}, LogicalType::DOUBLE, AstroConstLSun));
    loader.RegisterFunction(ScalarFunction("astro_const_M_earth", {}, LogicalType::DOUBLE, AstroConstMEarth));
    loader.RegisterFunction(ScalarFunction("astro_const_R_earth", {}, LogicalType::DOUBLE, AstroConstREarth));
    loader.RegisterFunction(ScalarFunction("astro_const_AU", {}, LogicalType::DOUBLE, AstroConstAU));
    loader.RegisterFunction(ScalarFunction("astro_const_pc", {}, LogicalType::DOUBLE, AstroConstPc));
    loader.RegisterFunction(ScalarFunction("astro_const_ly", {}, LogicalType::DOUBLE, AstroConstLy));
    loader.RegisterFunction(ScalarFunction("astro_const_sigma_sb", {}, LogicalType::DOUBLE, AstroConstSigmaSB));

    // Unit conversions
    loader.RegisterFunction(ScalarFunction("astro_unit_length_to_m", {LogicalType::DOUBLE, LogicalType::VARCHAR}, LogicalType::DOUBLE, AstroUnitLengthToM));
    loader.RegisterFunction(ScalarFunction("astro_unit_mass_to_kg", {LogicalType::DOUBLE, LogicalType::VARCHAR}, LogicalType::DOUBLE, AstroUnitMassToKg));
    loader.RegisterFunction(ScalarFunction("astro_unit_time_to_s", {LogicalType::DOUBLE, LogicalType::VARCHAR}, LogicalType::DOUBLE, AstroUnitTimeToS));
    loader.RegisterFunction(ScalarFunction("astro_unit_AU", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroUnitAU));
    loader.RegisterFunction(ScalarFunction("astro_unit_pc", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroUnitpc));
    loader.RegisterFunction(ScalarFunction("astro_unit_ly", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroUnitly));
    loader.RegisterFunction(ScalarFunction("astro_unit_M_sun", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroUnitM_sun));
    loader.RegisterFunction(ScalarFunction("astro_unit_M_earth", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroUnitM_earth));

    // Body models - Stars
    loader.RegisterFunction(ScalarFunction("astro_body_star_ms", {LogicalType::DOUBLE}, body_type, AstroBodyMakeStarMs));
    loader.RegisterFunction(ScalarFunction("astro_body_star_white_dwarf", {LogicalType::DOUBLE}, body_type, AstroBodyMakeStarWhiteDwarf));
    loader.RegisterFunction(ScalarFunction("astro_body_star_neutron", {LogicalType::DOUBLE}, body_type, AstroBodyMakeStarNeutron));
    loader.RegisterFunction(ScalarFunction("astro_body_brown_dwarf", {LogicalType::DOUBLE}, body_type, AstroBodyMakeBrownDwarf));
    loader.RegisterFunction(ScalarFunction("astro_body_black_hole", {LogicalType::DOUBLE}, body_type, AstroBodyMakeBlackHole));
    // Body models - Planets
    loader.RegisterFunction(ScalarFunction("astro_body_planet_rocky", {LogicalType::DOUBLE}, body_type, AstroBodyMakePlanetRocky));
    loader.RegisterFunction(ScalarFunction("astro_body_planet_gas_giant", {LogicalType::DOUBLE}, body_type, AstroBodyMakePlanetGasGiant));
    loader.RegisterFunction(ScalarFunction("astro_body_planet_ice_giant", {LogicalType::DOUBLE}, body_type, AstroBodyMakePlanetIceGiant));
    // Body models - Small bodies
    loader.RegisterFunction(ScalarFunction("astro_body_asteroid", {LogicalType::DOUBLE, LogicalType::DOUBLE}, body_type, AstroBodyMakeAsteroid));

    // Orbit functions
    loader.RegisterFunction(ScalarFunction("astro_orbit_make",
        {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE,
         LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::VARCHAR},
        orbit_type, AstroOrbitMake));
    loader.RegisterFunction(ScalarFunction("astro_orbit_period", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroOrbitPeriod));
    loader.RegisterFunction(ScalarFunction("astro_orbit_mean_motion", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroOrbitMeanMotion));
    loader.RegisterFunction(ScalarFunction("astro_orbit_position", {orbit_type, LogicalType::DOUBLE}, pos_type, AstroOrbitPosition));
    loader.RegisterFunction(ScalarFunction("astro_orbit_velocity", {orbit_type, LogicalType::DOUBLE}, vel_type, AstroOrbitVelocity));

    // Dynamics
    loader.RegisterFunction(ScalarFunction("astro_dyn_gravity_accel",
        {LogicalType::DOUBLE, pos_type, LogicalType::DOUBLE, pos_type}, vel_type, AstroDynGravityAccel));

    // Frame transforms
    loader.RegisterFunction(ScalarFunction("astro_frame_transform_pos", {pos_type, LogicalType::VARCHAR, LogicalType::VARCHAR}, pos_type, AstroFrameTransformPos));
    loader.RegisterFunction(ScalarFunction("astro_frame_transform_vel", {vel_type, LogicalType::VARCHAR, LogicalType::VARCHAR}, vel_type, AstroFrameTransformVel));

    // Sectors
    loader.RegisterFunction(ScalarFunction("astro_sector_id", {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::INTEGER}, sector_type, AstroSectorId));
    loader.RegisterFunction(ScalarFunction("astro_sector_center", {sector_type}, pos_type, AstroSectorCenter));
    loader.RegisterFunction(ScalarFunction("astro_sector_bounds", {sector_type}, bounds_type, AstroSectorBounds));
    loader.RegisterFunction(ScalarFunction("astro_sector_parent", {sector_type}, sector_type, AstroSectorParent));

    // Coordinates
    loader.RegisterFunction(ScalarFunction("astro_radec_to_xyz", {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE}, pos_type, AstroRadecToXyz));
    loader.RegisterFunction(ScalarFunction("astro_angular_separation", {LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroAngularSeparation));

    // Photometry
    loader.RegisterFunction(ScalarFunction("astro_mag_to_flux", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroMagToFlux));
    loader.RegisterFunction(ScalarFunction("astro_flux_to_mag", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroFluxToMag));
    loader.RegisterFunction(ScalarFunction("astro_distance_modulus", {LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroDistanceModulus));
    loader.RegisterFunction(ScalarFunction("astro_absolute_mag", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroAbsoluteMag));

    // Cosmology
    loader.RegisterFunction(ScalarFunction("astro_luminosity_distance", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroLuminosityDistance));
    loader.RegisterFunction(ScalarFunction("astro_comoving_distance", {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, AstroComovingDistance));
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
    return "3.1.0";
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
