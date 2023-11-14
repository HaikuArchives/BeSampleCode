
OBJ_DIR = objects.generated
PREFIX=/boot/home/config/non-packaged

all:
	$(MAKE) -C srcs/InputRecorder OBJ_DIR=$(OBJ_DIR)
	$(MAKE) -C srcs/InputRecorderDevice OBJ_DIR=$(OBJ_DIR)
	$(MAKE) -C srcs/InputRecorderFilter OBJ_DIR=$(OBJ_DIR)

clean:
	$(MAKE) -C srcs/InputRecorder clean OBJ_DIR=$(OBJ_DIR)
	$(MAKE) -C srcs/InputRecorderDevice clean OBJ_DIR=$(OBJ_DIR)
	$(MAKE) -C srcs/InputRecorderFilter clean OBJ_DIR=$(OBJ_DIR)

install: all
	mkdir -pv $(PREFIX)/apps
	mkdir -pv $(PREFIX)/add-ons/input_server/devices
	mkdir -pv $(PREFIX)/add-ons/input_server/filters
	copyattr -v -d srcs/InputRecorder/$(OBJ_DIR)/InputRecorder \
		$(PREFIX)/apps/InputRecorder
	copyattr -v -d srcs/InputRecorderDevice/$(OBJ_DIR)/InputRecorderDevice \
		$(PREFIX)/add-ons/input_server/devices/InputRecorderDevice
	copyattr -v -d srcs/InputRecorderFilter/$(OBJ_DIR)/InputRecorderFilter \
		$(PREFIX)/add-ons/input_server/filters/InputRecorderFilter
	@echo "***"
	@echo "*** Please restart Haiku for the input_server to load the new add-ons!"
	@echo "***"
