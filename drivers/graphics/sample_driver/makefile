# Build the sample graphics driver

SUBDIRS = driver accelerant

.DEFAULT:
	@for f in $(SUBDIRS); do \
		$(MAKE) -C $$f $@; \
	done
