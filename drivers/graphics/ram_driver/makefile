dirs := driver accelerant

default .DEFAULT:
	@cwd=`pwd`; for dir in $(dirs); do \
		if [ -d "$$dir" ]; then \
			cd $$dir && $(MAKE) $@; cd $$cwd; \
		fi \
	done
