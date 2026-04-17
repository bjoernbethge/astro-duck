#include "fits_types.hpp"

namespace duckdb {

static int CountHdus(fitsfile *fptr) {
	int hdunum, fits_status = 0;
	fits_get_num_hdus(fptr, &hdunum, &fits_status);
	return hdunum;
}

static int ResolveHduIndex(fitsfile *fptr, Value &hdu_val) {
	if (hdu_val.IsNull()) {
		return -1;
	}
	if (hdu_val.type().id() == LogicalTypeId::INTEGER ||
	    hdu_val.type().id() == LogicalTypeId::BIGINT) {
		return hdu_val.GetValue<int>();
	}
	auto name = hdu_val.GetValue<string>();
	int hdunum, fits_status = 0;
	if (fits_movnam_hdu(fptr, ANY_HDU, const_cast<char *>(name.c_str()), 0, &fits_status)) {
		throw InternalException("HDU '%s' not found.", name);
	}
	fits_get_hdu_num(fptr, &hdunum);
	return hdunum - 1;
}

static char TformCode(const char *tform) {
	size_t len = strlen(tform);
	while (len > 0 && tform[len - 1] == ' ') len--;
	return len > 0 ? tform[len - 1] : 'D';
}

// ============================================================================
// Table Function: fits_hdus(path)
// ============================================================================

static unique_ptr<FunctionData> FitsHdusBind(ClientContext &, TableFunctionBindInput &input,
                                              vector<LogicalType> &return_types, vector<string> &names) {
	auto path = input.inputs[0].GetValue<string>();
	auto result = make_uniq<FitsHdusData>();
	result->path = path;
	return_types = {LogicalType::INTEGER, LogicalType::VARCHAR, LogicalType::VARCHAR};
	names = {"hdu_index", "hdu_name", "hdu_type"};
	return std::move(result);
}

static void FitsHdusFunction(ClientContext &, TableFunctionInput &input, DataChunk &output) {
	auto &data = const_cast<FitsHdusData &>(input.bind_data->Cast<FitsHdusData>());
	if (data.done) {
		output.SetCardinality(0);
		return;
	}
	data.done = true;

	int fits_status = 0;
	fitsfile *fptr = nullptr;

	if (fits_open_file(&fptr, data.path.c_str(), READONLY, &fits_status)) {
		throw InternalException("FITS file not found: %s", data.path);
	}

	int total = CountHdus(fptr);
	idx_t out_idx = 0;

	for (int i = 0; i < total && out_idx < STANDARD_VECTOR_SIZE; i++) {
		int hdutype = 0;
		char extname[FLEN_KEYWORD] = {0};

		if (i > 0) {
			fits_movabs_hdu(fptr, i + 1, &hdutype, &fits_status);
		} else {
			fits_get_hdu_type(fptr, &hdutype, &fits_status);
		}
		int name_status = 0;
		fits_read_key_str(fptr, "EXTNAME", extname, nullptr, &name_status);

		string type_str;
		switch (hdutype) {
		case IMAGE_HDU:  type_str = "IMAGE"; break;
		case BINARY_TBL: type_str = "BINTABLE"; break;
		case ASCII_TBL:  type_str = "ASCII_TABLE"; break;
		default:         type_str = "UNKNOWN"; break;
		}

		output.data[0].SetValue(out_idx, Value::INTEGER(i));
		output.data[1].SetValue(out_idx, Value(extname[0] ? string(extname) : string()));
		output.data[2].SetValue(out_idx, Value(type_str));
		out_idx++;
	}

	output.SetCardinality(out_idx);
	fits_close_file(fptr, &fits_status);
}

// ============================================================================
// Table Function: fits_header(path, hdu := 0)
// ============================================================================

static unique_ptr<FunctionData> FitsHeaderBind(ClientContext &, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names) {
	auto path = input.inputs[0].GetValue<string>();
	auto it = input.named_parameters.find("hdu");
	int hdu_index = (it != input.named_parameters.end() && !it->second.IsNull())
	                ? it->second.GetValue<int>() : 0;

	auto result = make_uniq<FitsHeaderData>();
	result->path = path;
	result->hdu_index = hdu_index;
	return_types = {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR};
	names = {"keyword", "value", "comment"};
	return std::move(result);
}

static void FitsHeaderFunction(ClientContext &, TableFunctionInput &input, DataChunk &output) {
	auto &data = const_cast<FitsHeaderData &>(input.bind_data->Cast<FitsHeaderData>());
	if (data.done) {
		output.SetCardinality(0);
		return;
	}
	data.done = true;
	int fits_status = 0;
	fitsfile *fptr = nullptr;

	if (fits_open_file(&fptr, data.path.c_str(), READONLY, &fits_status)) {
		throw InternalException("FITS file not found: %s", data.path);
	}

	int max_hdu = CountHdus(fptr) - 1;
	if (data.hdu_index < 0 || data.hdu_index > max_hdu) {
		fits_close_file(fptr, &fits_status);
		throw InternalException("HDU index %d out of range [0, %d]", data.hdu_index, max_hdu);
	}

	if (data.hdu_index > 0) {
		fits_movabs_hdu(fptr, data.hdu_index + 1, nullptr, &fits_status);
	}

	int nkeys = 0, morekeys = 0;
	fits_get_hdrspace(fptr, &nkeys, &morekeys, &fits_status);

	idx_t out_idx = 0;
	for (int k = 1; k <= nkeys && out_idx < STANDARD_VECTOR_SIZE; k++) {
		char keyname[FLEN_KEYWORD] = {0};
		char keyval[FLEN_VALUE] = {0};
		char comm[FLEN_COMMENT] = {0};

		int kstatus = 0;
		fits_read_keyn(fptr, k, keyname, keyval, comm, &kstatus);
		if (kstatus != 0 || strcmp(keyname, "END") == 0) {
			continue;
		}

		output.data[0].SetValue(out_idx, Value(string(keyname)));
		output.data[1].SetValue(out_idx, Value(string(keyval)));
		output.data[2].SetValue(out_idx, Value(string(comm)));
		out_idx++;
	}

	output.SetCardinality(out_idx);
	fits_close_file(fptr, &fits_status);
}

// ============================================================================
// Table Function: read_fits(path, hdu := NULL, header := false, flatten := true)
// ============================================================================

static unique_ptr<FunctionData> ReadFitsBind(ClientContext &, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	auto path = input.inputs[0].GetValue<string>();
	auto hdu_it = input.named_parameters.find("hdu");
	Value hdu_val = (hdu_it != input.named_parameters.end()) ? hdu_it->second : Value();
	auto hdr_it = input.named_parameters.find("header");
	bool header = (hdr_it != input.named_parameters.end() && !hdr_it->second.IsNull())
	              ? hdr_it->second.GetValue<bool>() : false;
	auto flat_it = input.named_parameters.find("flatten");
	bool flatten = (flat_it != input.named_parameters.end() && !flat_it->second.IsNull())
	               ? flat_it->second.GetValue<bool>() : true;

	auto result = make_uniq<FitsReadData>();
	result->path = path;
	result->header = header;
	result->flatten = flatten;

	int fits_status = 0;
	fitsfile *fptr = nullptr;
	if (fits_open_file(&fptr, path.c_str(), READONLY, &fits_status)) {
		throw InternalException("FITS file not found: %s", path);
	}

	int total_hdus = CountHdus(fptr);

	if (hdu_val.IsNull()) {
		names = {"hdu", "note"};
		return_types = {LogicalType::INTEGER, LogicalType::VARCHAR};
		result->num_rows = 0;
	} else {
		int hdu_idx = ResolveHduIndex(fptr, hdu_val);
		if (hdu_idx < 0 || hdu_idx >= total_hdus) {
			fits_close_file(fptr, &fits_status);
			throw InternalException("HDU index %d out of range [0, %d]", hdu_idx, total_hdus - 1);
		}

		fits_movabs_hdu(fptr, hdu_idx + 1, nullptr, &fits_status);
		int hdutype = 0;
		fits_get_hdu_type(fptr, &hdutype, &fits_status);

		if (hdutype == ASCII_TBL) {
			fits_close_file(fptr, &fits_status);
			throw InternalException("ASCII TABLE not yet supported (HDU %d)", hdu_idx);
		}

		if (hdutype == BINARY_TBL) {
			int tfields = 0;
			LONGLONG nrows = 0;
			fits_get_num_rowsll(fptr, &nrows, &fits_status);
			fits_get_num_cols(fptr, &tfields, &fits_status);

			for (int c = 0; c < tfields; c++) {
				int colnum = c + 1;
				char keyname[16];
				char tform_val[FLEN_VALUE] = {0};
				char ttype_val[FLEN_KEYWORD] = {0};

				snprintf(keyname, sizeof(keyname), "TFORM%d", colnum);
				fits_read_key_str(fptr, keyname, tform_val, nullptr, &fits_status);
				if (fits_status == KEY_OUT_BOUNDS) {
					fits_status = 0;
					tform_val[0] = 'D';
					tform_val[1] = '\0';
				}

				int name_status = 0;
				snprintf(keyname, sizeof(keyname), "TTYPE%d", colnum);
				fits_read_key_str(fptr, keyname, ttype_val, nullptr, &name_status);

				string col_name;
				if (name_status == 0 && ttype_val[0] != '\0') {
					col_name = string(ttype_val);
					col_name.erase(col_name.find_last_not_of(' ') + 1);
				} else {
					col_name = "COLUMN_" + to_string(colnum);
				}
				names.push_back(col_name);

				char code = TformCode(tform_val);
				LogicalType lt;
				switch (code) {
				case 'D': lt = LogicalType::DOUBLE; break;
				case 'E': lt = LogicalType::FLOAT; break;
				case 'J': lt = LogicalType::INTEGER; break;
				case 'I': lt = LogicalType::SMALLINT; break;
				case 'K': lt = LogicalType::BIGINT; break;
				case 'B': lt = LogicalType::UTINYINT; break;
				case 'L': lt = LogicalType::BOOLEAN; break;
				case 'A': lt = LogicalType::VARCHAR; break;
				default:  lt = LogicalType::SQLNULL; break;
				}
				return_types.push_back(lt);
			}

			result->num_rows = static_cast<int>(nrows);
			result->hdu_index = hdu_idx;

		} else if (hdutype == IMAGE_HDU) {
			int naxis = 0, bitpix = 0;
			LONGLONG naxes[3] = {0, 0, 0};
			fits_get_img_paramll(fptr, 3, &bitpix, &naxis, naxes, &fits_status);

			if (flatten) {
				LONGLONG total_pixels = 1;
				for (int a = 0; a < naxis; a++) {
					total_pixels *= naxes[a];
				}
				names = {"x", "y", "value"};
				return_types = {LogicalType::INTEGER, LogicalType::INTEGER, FitsImageToDuckDBType(bitpix)};
				result->num_rows = static_cast<int>(total_pixels);
			} else {
				names = {"pixels", "width", "height"};
				return_types = {LogicalType::LIST(FitsImageToDuckDBType(bitpix)), LogicalType::INTEGER, LogicalType::INTEGER};
				result->num_rows = 1;
			}
			result->hdu_index = hdu_idx;
		}
	}

	if (header) {
		int nkeys = 0, morekeys = 0, fits_status2 = 0;
		fitsfile *fptr2 = nullptr;
		if (fits_open_file(&fptr2, path.c_str(), READONLY, &fits_status2) == 0) {
			if (result->hdu_index > 0) {
				fits_movabs_hdu(fptr2, result->hdu_index + 1, nullptr, &fits_status2);
			}
			fits_get_hdrspace(fptr2, &nkeys, &morekeys, &fits_status2);
			for (int k = 1; k <= nkeys; k++) {
				char keyname[FLEN_KEYWORD] = {0};
				char keyval[FLEN_VALUE] = {0};
				int kstatus = 0;
				fits_read_keyn(fptr2, k, keyname, keyval, nullptr, &kstatus);
				if (kstatus != 0 || strcmp(keyname, "END") == 0 || keyname[0] == '\0') {
					continue;
				}
				names.push_back("_hdr_" + string(keyname));
				return_types.push_back(LogicalType::VARCHAR);
			}
			fits_close_file(fptr2, &fits_status2);
		}
	}

	fits_close_file(fptr, &fits_status);
	return std::move(result);
}

static void ReadFitsFunction(ClientContext &, TableFunctionInput &input, DataChunk &output) {
	auto &data = const_cast<FitsReadData &>(input.bind_data->Cast<FitsReadData>());

	if (data.num_rows == 0 || data.offset >= data.num_rows) {
		output.SetCardinality(0);
		return;
	}

	int fits_status = 0;
	fitsfile *fptr = nullptr;

	if (fits_open_file(&fptr, data.path.c_str(), READONLY, &fits_status)) {
		throw InternalException("FITS file not found: %s", data.path);
	}

	idx_t out_idx = 0;

	fits_movabs_hdu(fptr, data.hdu_index + 1, nullptr, &fits_status);
	int hdutype = 0;
	fits_get_hdu_type(fptr, &hdutype, &fits_status);

	if (hdutype == BINARY_TBL) {
		int tfields = 0;
		fits_get_num_cols(fptr, &tfields, &fits_status);

		idx_t start = data.offset;
		for (; out_idx < STANDARD_VECTOR_SIZE && (start + out_idx) < data.num_rows; out_idx++) {
			for (int col = 0; col < tfields; col++) {
				int colnum = col + 1;
				char keyname[16], tform[FLEN_VALUE] = {0};
				snprintf(keyname, sizeof(keyname), "TFORM%d", colnum);
				fits_read_key_str(fptr, keyname, tform, nullptr, &fits_status);
				if (fits_status == KEY_OUT_BOUNDS) {
					fits_status = 0;
					tform[0] = 'D';
					tform[1] = '\0';
				}
				char code = TformCode(tform);
				auto &out_vec = output.data[col];
				LONGLONG firstrow = static_cast<LONGLONG>(start + out_idx + 1);

				switch (code) {
				case 'D': {
					double val = 0;
					fits_read_col_dbl(fptr, colnum, firstrow, 1, 1, 0.0, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val));
					break;
				}
				case 'E': {
					float val = 0;
					fits_read_col_flt(fptr, colnum, firstrow, 1, 1, 0.0f, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val));
					break;
				}
				case 'J': {
					int val = 0;
					fits_read_col_int(fptr, colnum, firstrow, 1, 1, 0, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val));
					break;
				}
				case 'I': {
					short val = 0;
					fits_read_col_sht(fptr, colnum, firstrow, 1, 1, 0, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val));
					break;
				}
				case 'K': {
					long long val = 0;
					fits_read_col_lnglng(fptr, colnum, firstrow, 1, 1, 0, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val));
					break;
				}
				case 'B': {
					unsigned char val = 0;
					fits_read_col_byt(fptr, colnum, firstrow, 1, 1, 0, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val));
					break;
				}
				case 'L': {
					char val = 0;
					fits_read_col_log(fptr, colnum, firstrow, 1, 1, 0, &val, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(val != 0));
					break;
				}
				case 'A': {
					char buf[FLEN_VALUE] = {0};
					char *vals[1] = {buf};
					fits_read_col_str(fptr, colnum, firstrow, 1, 1, nullptr, vals, nullptr, &fits_status);
					out_vec.SetValue(out_idx, Value(string(buf)));
					break;
				}
				default:
					out_vec.SetValue(out_idx, Value());
					break;
				}
			}
		}
	} else if (hdutype == IMAGE_HDU) {
		int naxis = 0, bitpix = 0;
		LONGLONG naxes[3] = {0, 0, 0};
		fits_get_img_paramll(fptr, 3, &bitpix, &naxis, naxes, &fits_status);
		LogicalType pix_type = FitsImageToDuckDBType(bitpix);

		if (data.flatten) {
			LONGLONG total = naxes[0] * (naxis > 1 ? naxes[1] : 1);
			idx_t start = data.offset;
			for (; out_idx < STANDARD_VECTOR_SIZE && (start + out_idx) < static_cast<idx_t>(total); out_idx++) {
				idx_t abs_pix = start + out_idx;
				idx_t y = naxis > 1 ? abs_pix / static_cast<idx_t>(naxes[0]) : 0;
				idx_t x = abs_pix % static_cast<idx_t>(naxes[0]);
				output.data[0].SetValue(out_idx, Value::INTEGER(static_cast<int>(x)));
				output.data[1].SetValue(out_idx, Value::INTEGER(static_cast<int>(y)));

				LONGLONG firstelem = static_cast<LONGLONG>(abs_pix + 1);
				if (pix_type.id() == LogicalTypeId::DOUBLE) {
					double val = 0;
					fits_read_img_dbl(fptr, 0L, firstelem, 1, 0.0, &val, nullptr, &fits_status);
					output.data[2].SetValue(out_idx, Value(val));
				} else if (pix_type.id() == LogicalTypeId::FLOAT) {
					float val = 0;
					fits_read_img_flt(fptr, 0L, firstelem, 1, 0.0f, &val, nullptr, &fits_status);
					output.data[2].SetValue(out_idx, Value(val));
				} else if (pix_type.id() == LogicalTypeId::INTEGER) {
					int val = 0;
					fits_read_img_int(fptr, 0L, firstelem, 1, 0, &val, nullptr, &fits_status);
					output.data[2].SetValue(out_idx, Value(val));
				} else if (pix_type.id() == LogicalTypeId::SMALLINT) {
					short val = 0;
					fits_read_img_sht(fptr, 0L, firstelem, 1, 0, &val, nullptr, &fits_status);
					output.data[2].SetValue(out_idx, Value(val));
				} else if (pix_type.id() == LogicalTypeId::UTINYINT) {
					unsigned char val = 0;
					fits_read_img_byt(fptr, 0L, firstelem, 1, 0, &val, nullptr, &fits_status);
					output.data[2].SetValue(out_idx, Value(val));
				} else {
					double val = 0;
					fits_read_img_dbl(fptr, 0L, firstelem, 1, 0.0, &val, nullptr, &fits_status);
					output.data[2].SetValue(out_idx, Value(val));
				}
			}
		} else {
			LONGLONG total = naxes[0] * (naxis > 1 ? naxes[1] : 1);

			auto buf_to_list = [&](auto *buf, LONGLONG n) -> vector<Value> {
				vector<Value> vals;
				vals.reserve(static_cast<size_t>(n));
				for (LONGLONG i = 0; i < n; i++) {
					vals.push_back(Value(static_cast<double>(buf[i])));
				}
				return vals;
			};

			if (bitpix == -64) {
				auto *buf = new double[static_cast<size_t>(total)];
				fits_read_img_dbl(fptr, 0L, 1, total, 0.0, buf, nullptr, &fits_status);
				output.data[0].SetValue(out_idx, Value::LIST(buf_to_list(buf, total)));
				delete[] buf;
			} else if (bitpix == -32) {
				auto *buf = new float[static_cast<size_t>(total)];
				fits_read_img_flt(fptr, 0L, 1, total, 0.0f, buf, nullptr, &fits_status);
				output.data[0].SetValue(out_idx, Value::LIST(buf_to_list(buf, total)));
				delete[] buf;
			} else if (bitpix == 32) {
				auto *buf = new int[static_cast<size_t>(total)];
				fits_read_img_int(fptr, 0L, 1, total, 0, buf, nullptr, &fits_status);
				output.data[0].SetValue(out_idx, Value::LIST(buf_to_list(buf, total)));
				delete[] buf;
			} else if (bitpix == 16) {
				auto *buf = new short[static_cast<size_t>(total)];
				fits_read_img_sht(fptr, 0L, 1, total, 0, buf, nullptr, &fits_status);
				output.data[0].SetValue(out_idx, Value::LIST(buf_to_list(buf, total)));
				delete[] buf;
			} else {
				auto *buf = new unsigned char[static_cast<size_t>(total)];
				fits_read_img_byt(fptr, 0L, 1, total, 0, buf, nullptr, &fits_status);
				output.data[0].SetValue(out_idx, Value::LIST(buf_to_list(buf, total)));
				delete[] buf;
			}

			output.data[1].SetValue(out_idx, Value::INTEGER(static_cast<int>(naxes[0])));
			output.data[2].SetValue(out_idx, Value::INTEGER(static_cast<int>(naxis > 1 ? naxes[1] : 1)));
			out_idx++;
		}
	}

	data.offset += out_idx;
	output.SetCardinality(out_idx);
	fits_close_file(fptr, &fits_status);
}

// ============================================================================
// RegisterFitsFunctions
// ============================================================================

void RegisterFitsFunctions(ExtensionLoader &loader) {
	auto fits_hdus = TableFunction(
		"fits_hdus",
		{LogicalType::VARCHAR},
		FitsHdusFunction,
		FitsHdusBind
	);
	loader.RegisterFunction(fits_hdus);

	auto fits_header = TableFunction(
		"fits_header",
		{LogicalType::VARCHAR},
		FitsHeaderFunction,
		FitsHeaderBind
	);
	fits_header.named_parameters["hdu"] = LogicalType::INTEGER;
	loader.RegisterFunction(fits_header);

	auto read_fits = TableFunction(
		"read_fits",
		{LogicalType::VARCHAR},
		ReadFitsFunction,
		ReadFitsBind
	);
	read_fits.named_parameters["hdu"] = LogicalType::INTEGER;
	read_fits.named_parameters["header"] = LogicalType::BOOLEAN;
	read_fits.named_parameters["flatten"] = LogicalType::BOOLEAN;
#ifndef __EMSCRIPTEN__
	loader.RegisterFunction(read_fits);
#endif
}

} // namespace duckdb
