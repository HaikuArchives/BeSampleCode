#ifndef BLURFILTER_H
#define BLURFILTER_H

#include "ImageFilter.h"

class FILTER_SYMBOLS BlurFilter : public ImageFilter {
	public:
		BlurFilter();
		BBitmap *Run(BBitmap *input);
		~BlurFilter();
};

extern "C" FILTER_SYMBOLS BlurFilter *instantiate_filter();

#endif
