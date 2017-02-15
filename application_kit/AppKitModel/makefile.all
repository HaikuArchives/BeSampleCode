# Makefile for the AppKitModel samples

SUBDIRS = \
	HandlerAsDataObject \
	HandlerAsOperation

default .DEFAULT:
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f $@; \
	done
