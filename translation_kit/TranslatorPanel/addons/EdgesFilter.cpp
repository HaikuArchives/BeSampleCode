#include "EdgesFilter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

EdgesFilter *instantiate_filter() {
	return new EdgesFilter();
}

EdgesFilter::EdgesFilter() : ImageFilter() {
	id = 0xfacedeed;
	strcpy(name, "Edges");
}

BBitmap *EdgesFilter::Run(BBitmap *input) {
	if (input == NULL) return NULL;
	BRect bounds = input->Bounds();
	color_space cs = input->ColorSpace();
	if (cs != B_RGB32 && cs != B_RGBA32) {
		printf("EdgesFilter: unknown colorspace\n");
		return NULL;
	}
	
	BBitmap *output = new BBitmap(bounds, cs);
	if (output == NULL) {
		printf("EdgesFilter: could not allocate bitmap\n");
		return NULL;
	}
	
	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;
	unsigned char *in_bits = (unsigned char *)input->Bits();
	unsigned char *out_bits = (unsigned char *)output->Bits();
	if (in_bits == NULL || out_bits == NULL) {
		printf("EdgesFilter: bits was NULL\n");
		delete output;
		return NULL;
	}
	
	memset(out_bits, 0, width * height * 4);
	
	int left, right, top, bottom, edge;
	out_bits += (width * 4) + 4;
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int pos = (y * width + x - 1) * 4;
			left = (in_bits[pos] + in_bits[pos + 1] * 2 + in_bits[pos + 2]) >> 2;
			pos += 8;
			right = (in_bits[pos] + in_bits[pos + 1] * 2 + in_bits[pos + 2]) >> 2;
			pos = ((y - 1) * width + x) * 4;
			top = (in_bits[pos] + in_bits[pos + 1] * 2 + in_bits[pos + 2]) >> 2;
			pos += (width * 4 * 2);
			bottom = (in_bits[pos] + in_bits[pos + 1] * 2 + in_bits[pos + 2]) >> 2;
			edge = (abs(left - right) + abs(top - bottom)) >> 1;
			
			out_bits[0] = edge;
			out_bits[1] = edge;
			out_bits[2] = edge;
			out_bits[3] = 255;
			out_bits += 4;
		}
		out_bits += 8;
	}
	
	return output;
}

EdgesFilter::~EdgesFilter() {

}