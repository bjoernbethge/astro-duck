/* Minimal unistd.h stub for MSVC — cfitsio uses getcwd/access which are
   available as _getcwd/_access in <direct.h> and <io.h>. cfitsio guards
   these with its own WIN32 checks, so an empty stub is sufficient. */
