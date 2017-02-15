#ifndef EDGESFILTER_H
#define EDGESFILTER_H

#include "ImageFilter.h"

class FILTER_SYMBOLS EdgesFilter : public ImageFilter {
	public:
		EdgesFilter();
		BBitmap *Run(BBitmap *input);
		~EdgesFilter();
};

extern "C" FILTER_SYMBOLS EdgesFilter *instantiate_filter();

#endif
