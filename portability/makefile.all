# makefile for all portability sample code

LEAFDIRS = \
	condvar

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
