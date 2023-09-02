# makefile for colorPickerModule

LEAFDIRS = \
	colorPickerClient \
	crayonColorPicker \
	simpleColorPicker

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
