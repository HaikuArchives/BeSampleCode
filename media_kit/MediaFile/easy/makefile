# makefile for easy-media-reader

ifeq ($(BE_HOST_CPU), ppc)
    OPTIMIZER = -O7
    LFLAGS = -lbe -lmedia -ltracker
else
    OPTIMIZER = -O1 -no-fpic
    LFLAGS = -lbe -lmedia -ltracker
endif

CFLAGS += $(OPTIMIZER)

.PHONY: default
default: easy-media-reader

easy-media-reader : easy-media-reader.cpp
	cc $(CFLAGS) -o easy-media-reader easy-media-reader.cpp $(LFLAGS)

clean:
	rm -f *.o doc *.xSYM *.xMAP
