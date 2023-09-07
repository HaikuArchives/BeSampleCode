# makefile for all graphics sample code

LEAFDIRS = \
	FrameRate \
	Mandelbrot \
	QuickPaint

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
