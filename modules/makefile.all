# makefile for all driver sample code

LEAFDIRS = \
	xyz5038

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
