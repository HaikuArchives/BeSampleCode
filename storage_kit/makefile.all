# makefile for all Storage Kit sample code

LEAFDIRS = \
	DriveDaemon \
	FolderWatcher \
	Indexer \
	LiveQueryApp \
	People \
	QueryApp

default .DEFAULT:
	-@for f in $(LEAFDIRS); do \
		$(MAKE) -C $$f $@; \
	done
