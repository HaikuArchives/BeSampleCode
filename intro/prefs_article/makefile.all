# makefile for the prefslib sample code

LEAFDIRS = \
	prefslib \
	sample

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
