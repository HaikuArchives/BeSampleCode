#include "GreyscaleFilter.h"
#include <stdlib.h>
#include <stdio.h>

GreyscaleFilter *instantiate_filter() {
	return new GreyscaleFilter();
}

GreyscaleFilter::GreyscaleFilter() : ImageFilter() {
	id = 0xfeedface;
	strcpy(name, "Greyscale");
}

BBitmap *GreyscaleFilter::Run(BBitmap *input) {
	if (input == NULL) return NULL;
	BRect bounds = input->Bounds();
	color_space cs = input->ColorSpace();
	if (cs != B_RGB32 && cs != B_RGBA32) {
		printf("GreyscaleFilter: unknown colorspace\n");
		return NULL;
	}
	
	BBitmap *output = new BBitmap(bounds, cs);
	if (output == NULL) {
		printf("GreyscaleFilter: could not allocate bitmap\n");
		return NULL;
	}
	
	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;
	unsigned char *in_bits = (unsigned char *)input->Bits();
	unsigned char *out_bits = (unsigned char *)output->Bits();
	if (in_bits == NULL || out_bits == NULL) {
		printf("GreyscaleFilter: bits was NULL\n");
		delete output;
		return NULL;
	}
	
	unsigned char temp;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			temp = (in_bits[0] + in_bits[1] + in_bits[1] + in_bits[2]) >> 2;
			out_bits[0] = temp;
			out_bits[1] = temp;
			out_bits[2] = temp;
			out_bits[3] = 255;
			in_bits += 4;
			out_bits += 4;
		}
	}
	
	return output;
}

GreyscaleFilter::~GreyscaleFilter() {

}