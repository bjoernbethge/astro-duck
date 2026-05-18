cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME astro)
set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/include)

# cfitsio: compile sources directly in this project so they stay out of the
# DuckDB export set and inherit the parent's compiler environment.

# Windows: cfitsio's flex-generated eval_l.c includes <unistd.h> which MSVC
# doesn't provide; supply a stub from src/compat/ before the cfitsio includes.
if(MSVC)
    include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/src/compat")
endif()

include_directories(third_party/cfitsio)
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)

# Minimal cfitsio source set (no Fortran wrappers, no network driver,
# no shared-memory driver — local file I/O only)
set(CFITSIO_SOURCES
    third_party/cfitsio/buffers.c
    third_party/cfitsio/cfileio.c
    third_party/cfitsio/checksum.c
    third_party/cfitsio/drvrfile.c
    third_party/cfitsio/drvrmem.c
    third_party/cfitsio/editcol.c
    third_party/cfitsio/edithdu.c
    third_party/cfitsio/eval_f.c
    third_party/cfitsio/eval_l.c
    third_party/cfitsio/eval_y.c
    third_party/cfitsio/fits_hcompress.c
    third_party/cfitsio/fits_hdecompress.c
    third_party/cfitsio/fitscore.c
    third_party/cfitsio/getcol.c
    third_party/cfitsio/getcolb.c
    third_party/cfitsio/getcold.c
    third_party/cfitsio/getcole.c
    third_party/cfitsio/getcoli.c
    third_party/cfitsio/getcolj.c
    third_party/cfitsio/getcolk.c
    third_party/cfitsio/getcoll.c
    third_party/cfitsio/getcols.c
    third_party/cfitsio/getcolsb.c
    third_party/cfitsio/getcolui.c
    third_party/cfitsio/getcoluj.c
    third_party/cfitsio/getcoluk.c
    third_party/cfitsio/getkey.c
    third_party/cfitsio/group.c
    third_party/cfitsio/grparser.c
    third_party/cfitsio/histo.c
    third_party/cfitsio/imcompress.c
    third_party/cfitsio/iraffits.c
    third_party/cfitsio/modkey.c
    third_party/cfitsio/pliocomp.c
    third_party/cfitsio/putcol.c
    third_party/cfitsio/putcolb.c
    third_party/cfitsio/putcold.c
    third_party/cfitsio/putcole.c
    third_party/cfitsio/putcoli.c
    third_party/cfitsio/putcolj.c
    third_party/cfitsio/putcolk.c
    third_party/cfitsio/putcoll.c
    third_party/cfitsio/putcols.c
    third_party/cfitsio/putcolsb.c
    third_party/cfitsio/putcolu.c
    third_party/cfitsio/putcolui.c
    third_party/cfitsio/putcoluj.c
    third_party/cfitsio/putcoluk.c
    third_party/cfitsio/putkey.c
    third_party/cfitsio/quantize.c
    third_party/cfitsio/region.c
    third_party/cfitsio/ricecomp.c
    third_party/cfitsio/scalnull.c
    third_party/cfitsio/simplerng.c
    third_party/cfitsio/swapproc.c
    third_party/cfitsio/wcssub.c
    third_party/cfitsio/wcsutil.c
    third_party/cfitsio/zuncompress.c
    src/fits_zstubs.c
)
# zcompress.c requires zlib (gzip .fits.gz support); replaced with stubs in fits_zstubs.c.
# drvrnet.c requires network stack; excluded for local-file-only builds.

# Per-source compile flags for the vendored cfitsio C files:
#   * macOS Clang promotes -Wimplicit-function-declaration to an error
#     (group.c uses getcwd without an explicit unistd.h include).
#   * Newer gcc (>=15) defaults to C23, where `nullptr` is a reserved
#     keyword. cfitsio's putcol.c uses `nullptr` as an ordinary variable
#     name (`char *nullptr`), so pin the C standard to gnu11 on GCC/Clang.
foreach(cfitsio_src ${CFITSIO_SOURCES})
    set(_cfitsio_flags "")
    if(APPLE)
        set(_cfitsio_flags "${_cfitsio_flags} -Wno-implicit-function-declaration")
    endif()
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
       OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
        set(_cfitsio_flags "${_cfitsio_flags} -std=gnu11")
    endif()
    if(_cfitsio_flags)
        set_source_files_properties(${cfitsio_src} PROPERTIES
            COMPILE_FLAGS "${_cfitsio_flags}")
    endif()
endforeach()

set(EXTENSION_SOURCES src/astro.cpp src/fits_read.cpp ${CFITSIO_SOURCES})

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
