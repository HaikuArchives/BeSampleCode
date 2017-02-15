# makefile for Scripter

LEAFDIRS = \
	FIR \
	ScriptWorld

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
