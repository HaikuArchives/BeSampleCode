# Makefile for the transcode tools

INCLUDES := -I.

ifeq ($(BE_HOST_CPU), ppc)
    OPTIMIZER = -O4
    LFLAGS = 
else
    OPTIMIZER = -O1 -no-fpic
    LFLAGS = -lbe -lmedia -ltracker
endif

CFLAGS += $(OPTIMIZER) $(INCLUDES)

.PHONY: default
default: encode tcode

encode : encode.cpp
	cc $(OPTIMIZER) $(INCLUDES) -o encode encode.cpp $(LFLAGS) -ltranslation   

tcode : tcode.cpp
	cc $(OPTIMIZER) $(INCLUDES) -o tcode tcode.cpp $(LFLAGS)

clean:
	rm -f *.o tcode encode *.xSYM *.xMAP 
