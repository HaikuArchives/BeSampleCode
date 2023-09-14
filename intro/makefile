# makefile for all introductory sample code

SUBMAKES = \
	prefs_article

LEAFDIRS = \
	BasicButton \
	ButtonWorld \
	DoubleClick \
	DropWorld \
	HelloWorld \
	QSort \
	TextEditor \
	doodle \
	messageworld \
	xmas

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
	-@for f in $(SUBMAKES); do \
		$(MAKE) -C $$f -f makefile.all $@; \
	done
