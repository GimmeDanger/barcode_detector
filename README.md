# Barcode reader

Purpose: find EAN-13 barcode on image with opencv lib and decode it

Required libs: opencv

Compilation: g++ main.cpp barcode.cpp -O3 -std=c++14 -o barcode_reader `pkg-config --cflags --libs opencv`

Run: ./barcode_reader <path_to_image>

References: 
1) https://ru.wikipedia.org/wiki/Universal_Product_Code
2) https://ru.wikipedia.org/wiki/European_Article_Number
3) https://www.pyimagesearch.com/2014/11/24/detecting-barcodes-images-python-opencv/


