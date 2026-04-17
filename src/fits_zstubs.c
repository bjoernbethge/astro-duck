/* Stubs for zlib-based FITS gzip functions (zcompress.c).
   These allow linking without zlib. Gzip-compressed FITS files (.fits.gz)
   will return FILE_NOT_OPENED; plain .fits files are unaffected. */
#include <stdio.h>
#include <stdlib.h>

#define FILE_NOT_OPENED 104  /* cfitsio error code */

int uncompress2mem(char *filename, FILE *diskfile,
             char **buffptr, size_t *buffsize,
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize, int *status) {
    (void)filename; (void)diskfile; (void)buffptr; (void)buffsize;
    (void)mem_realloc; (void)filesize;
    *status = FILE_NOT_OPENED;
    return *status;
}

int uncompress2mem_from_mem(char *inmemptr, size_t inmemsize,
             char **buffptr, size_t *buffsize,
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize, int *status) {
    (void)inmemptr; (void)inmemsize; (void)buffptr; (void)buffsize;
    (void)mem_realloc; (void)filesize;
    *status = FILE_NOT_OPENED;
    return *status;
}

int uncompress2file(char *filename, FILE *indiskfile,
             FILE *outdiskfile, int *status) {
    (void)filename; (void)indiskfile; (void)outdiskfile;
    *status = FILE_NOT_OPENED;
    return *status;
}

int compress2mem_from_mem(char *inmemptr, size_t inmemsize,
             char **buffptr, size_t *buffsize,
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize, int *status) {
    (void)inmemptr; (void)inmemsize; (void)buffptr; (void)buffsize;
    (void)mem_realloc; (void)filesize;
    *status = FILE_NOT_OPENED;
    return *status;
}

int compress2file_from_mem(char *inmemptr, size_t inmemsize,
             FILE *outdiskfile, size_t *filesize, int *status) {
    (void)inmemptr; (void)inmemsize; (void)outdiskfile; (void)filesize;
    *status = FILE_NOT_OPENED;
    return *status;
}
