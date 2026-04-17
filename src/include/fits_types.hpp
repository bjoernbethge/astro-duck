#pragma once

#include "duckdb.hpp"
#include "fitsio.h"

namespace duckdb {

// ============================================================================
// FITS → DuckDB Type Mapping
// ============================================================================

inline LogicalType FitsToDuckDBType(int datatype) {
	switch (datatype) {
	case TDOUBLE:  return LogicalType::DOUBLE;
	case TFLOAT:   return LogicalType::FLOAT;
	case TINT:     return LogicalType::INTEGER;
	case TSHORT:   return LogicalType::SMALLINT;
	case TLONGLONG: return LogicalType::BIGINT;
	case TBYTE:    return LogicalType::UTINYINT;
	case TLOGICAL: return LogicalType::BOOLEAN;
	case TSTRING:  return LogicalType::VARCHAR;
	default:       return LogicalType::SQLNULL;
	}
}

inline LogicalType FitsImageToDuckDBType(int bitpix) {
	switch (bitpix) {
	case 8:   return LogicalType::UTINYINT;
	case 16:  return LogicalType::SMALLINT;
	case 32:  return LogicalType::INTEGER;
	case -32: return LogicalType::FLOAT;
	case -64: return LogicalType::DOUBLE;
	default:  return LogicalType::DOUBLE;
	}
}

// ============================================================================
// FITS Function Data (stored as FunctionData per query)
// ============================================================================

struct FitsReadData : TableFunctionData {
	string path;
	int hdu_index = 0;
	bool header = false;
	bool flatten = true;
	idx_t num_rows = 0;
	idx_t offset = 0;   // rows already returned
};

struct FitsHdusData : TableFunctionData {
	string path;
	bool done = false;
};

struct FitsHeaderData : TableFunctionData {
	string path;
	int hdu_index = 0;
	bool done = false;
};

// ============================================================================
// RegisterFitsFunctions — called from LoadInternal()
// ============================================================================

void RegisterFitsFunctions(ExtensionLoader &loader);

} // namespace duckdb
