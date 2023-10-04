# makefile for all "add-ons" sample code

SUBMAKES = \
	LoadAddon

LEAFDIRS = \
	MailMarker \
	dosfs \
	iso9660

default .DEFAULT:
	-@for f in $(SUBMAKES); do \
		$(MAKE) -C $$f -f makefile.all $@; \
	done
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
