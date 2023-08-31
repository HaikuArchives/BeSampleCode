# makefile for mplay

OBJS = DrawingTidbits.o \
       MediaSlider.o \
       MediaView.o \
       TransportButton.o \
       AudioOutput.o \
       draw.o \
       draw_window.o

INCLUDES := -I.

ifeq ($(BE_HOST_CPU), ppc)
    OPTIMIZER = -O4
    LFLAGS = 
	XMAP = -map MediaPlayer.xMAP
else
    OPTIMIZER = -O1 
    LFLAGS = -lbe -lmedia -ltracker -lstdc++
	XMAP =
endif

CFLAGS += $(OPTIMIZER) $(INCLUDES)

.PHONY: default
default: mplay

mplay : $(OBJS)
	cc $(XMAP) $(OPTIMIZER) -o mplay $(OBJS) $(LFLAGS)
	xres -o $@ MediaPlayer.rsrc
	mimeset $@

DrawingTidbits.o : DrawingTidbits.cpp
	cc -c $(CFLAGS) $<
MediaSlider.o : MediaSlider.cpp
	cc -c $(CFLAGS) $<
MediaView.o : MediaView.cpp
	cc -c $(CFLAGS) $<
AudioOutput.o : AudioOutput.cpp
	cc -c $(CFLAGS) $<
TransportButton.o : TransportButton.cpp	
	cc -c $(CFLAGS) $<
draw.o : draw.cpp
	cc -c $(CFLAGS) $<
draw_window.o : draw_window.cpp
	cc -c $(CFLAGS) $<

clean:
	rm -f *.o mplay *.xSYM *.xMAP
