#ifndef GREYSCALEFILTER_H
#define GREYSCALEFILTER_H

#include "ImageFilter.h"

class FILTER_SYMBOLS GreyscaleFilter : public ImageFilter {
	public:
		GreyscaleFilter();
		BBitmap *Run(BBitmap *input);
		~GreyscaleFilter();
};

extern "C" FILTER_SYMBOLS GreyscaleFilter *instantiate_filter();

#endif
