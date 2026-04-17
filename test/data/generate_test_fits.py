#!/usr/bin/env python3
"""Generate test FITS file for astro-duck test suite."""

from pathlib import Path
import sys

try:
    from astropy.io import fits
except ImportError:
    print("astropy not installed — run: uv pip install astropy", file=sys.stderr)
    sys.exit(1)

OUT = Path(__file__).parent / "test.fits"

# HDU 0: empty PRIMARY
hdu0 = fits.PrimaryHDU()

# HDU 1: BINTABLE with all scalar types + one array column
# Columns: D (double), E (float), J (int32), I (int16), B (uint8), L (bool), A (string), 7E (float[7])
import numpy as np

nrows = 10
data = np.zeros(
    nrows,
    dtype=[
        ("D_COL", ">f8"),      # D: double
        ("E_COL", ">f4"),      # E: float
        ("J_COL", ">i4"),      # J: int32
        ("I_COL", ">i2"),      # I: int16
        ("B_COL", "u1"),      # B: uint8
        ("L_COL", "u1"),      # L: boolean
        ("A_COL", "S8"),      # A: string
        ("ARR7",  ">f4", (7,)),  # 7E: float array
    ],
)

for i in range(nrows):
    data["D_COL"][i] = i * 1.5
    data["E_COL"][i] = i * 2.5
    data["J_COL"][i] = i * 10
    data["I_COL"][i] = i * 100
    data["B_COL"][i] = i + 1
    data["L_COL"][i] = i % 2
    data["A_COL"][i] = f"ROW{i:02d}"
    data["ARR7"][i] = np.arange(7) * (i + 1)

hdu1 = fits.BinTableHDU(data, name="TEST_TABLE")

# HDU 2: IMAGE (4x3 float)
img_data = np.arange(12, dtype=np.float32).reshape((4, 3))  # width=4, height=3
hdu2 = fits.ImageHDU(data=img_data, name="TEST_IMAGE")
hdu2.header["COMMENT"] = "Test 4x3 float image"

hdulist = fits.HDUList([hdu0, hdu1, hdu2])
hdulist.writeto(OUT, overwrite=True)
print(f"Written: {OUT}")
