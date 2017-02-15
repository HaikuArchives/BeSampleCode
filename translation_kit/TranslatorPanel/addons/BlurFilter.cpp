#include "BlurFilter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

BlurFilter *instantiate_filter() {
	return new BlurFilter();
}

BlurFilter::BlurFilter() : ImageFilter() {
	id = 0xdeedace;
	strcpy(name, "Blur");
}

BBitmap *BlurFilter::Run(BBitmap *input) {
	if (input == NULL) return NULL;
	BRect bounds = input->Bounds();
	color_space cs = input->ColorSpace();
	if (cs != B_RGB32 && cs != B_RGBA32) {
		printf("BlurFilter: unknown colorspace\n");
		return NULL;
	}
	
	BBitmap *output = new BBitmap(bounds, cs);
	if (output == NULL) {
		printf("BlurFilter: could not allocate bitmap\n");
		return NULL;
	}
	
	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;
	unsigned char *in_bits = (unsigned char *)input->Bits();
	unsigned char *out_bits = (unsigned char *)output->Bits();
	if (in_bits == NULL || out_bits == NULL) {
		printf("BlurFilter: bits was NULL\n");
		delete output;
		return NULL;
	}
	
	unsigned char *rows[] = { NULL, NULL, NULL };
	for (int x = 0; x < 3; x++) {
		rows[x] = (unsigned char *)malloc((width + 2) * 4);
		if (rows[x] == NULL) {
			printf("BlurFilter: could not allocate rows\n");
			if (rows[0] != NULL) free(rows[0]);
			if (rows[1] != NULL) free(rows[1]);
			delete output;
			return NULL;
		}
		if (x == 0) memset(rows[x], 0, (width + 2) * 4);
	}
	
	int temp;
	for (int y = 0; y < height; y++) {
		if (y != 0) memcpy(rows[0] + 4, in_bits + ((y - 1) * width * 4), width * 4);
		memcpy(rows[1] + 4, in_bits + (y * width * 4), width * 4);
		if (y != height - 1) memcpy(rows[2] + 4, in_bits + ((y + 1) * width * 4), width * 4);
		else memset(rows[2], 0, (width + 2) * 4);
		
		for (int x = 0; x < width; x++) {
			temp = rows[0][x * 4] + rows[0][(x + 1) * 4] + rows[0][(x + 2) * 4] +
				rows[1][x * 4] + rows[1][(x + 1) * 4] + rows[1][(x + 2) * 4] +
				rows[2][x * 4] + rows[2][(x + 1) * 4] + rows[2][(x + 2) * 4];
			temp /= 9;
			out_bits[0] = temp;
			temp = rows[0][x * 4 + 1] + rows[0][(x + 1) * 4 + 1] + rows[0][(x + 2) * 4 + 1] +
				rows[1][x * 4 + 1] + rows[1][(x + 1) * 4 + 1] + rows[1][(x + 2) * 4 + 1] +
				rows[2][x * 4 + 1] + rows[2][(x + 1) * 4 + 1] + rows[2][(x + 2) * 4 + 1];
			temp /= 9;
			out_bits[1] = temp;
			temp = rows[0][x * 4 + 2] + rows[0][(x + 1) * 4 + 2] + rows[0][(x + 2) * 4 + 2] +
				rows[1][x * 4 + 2] + rows[1][(x + 1) * 4 + 2] + rows[1][(x + 2) * 4 + 2] +
				rows[2][x * 4 + 2] + rows[2][(x + 1) * 4 + 2] + rows[2][(x + 2) * 4 + 2];
			temp /= 9;
			out_bits[2] = temp;
			out_bits[3] = 255;
			out_bits += 4;
		}
	}
	
	free(rows[0]);
	free(rows[1]);
	free(rows[2]);
	return output;
}

BlurFilter::~BlurFilter() {

}