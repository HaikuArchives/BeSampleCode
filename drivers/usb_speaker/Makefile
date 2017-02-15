CC		:= gcc
CFLAGS	:= -O2

DRIVER	:= usb_speaker
OBJS	:= usb_speaker.o

BINDIR	:= /boot/home/config/add-ons/kernel/drivers/bin
LINKDIR	:= /boot/home/config/add-ons/kernel/drivers/dev/misc
KERNEL	:= /system/kernel_intel


default: $(DRIVER)

$(DRIVER): $(OBJS)
	@echo "Linking $(DRIVER)"
	@$(CC) -nostdlib $(OBJS) $(KERNEL)

%.o: %.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

install:
	@echo "Installing $(DRIVER)"
	@mkdir -p $(BINDIR) $(LINKDIR)
	@cp $(DRIVER) $(BINDIR)/$(DRIVER)
	@ln -sf $(BINDIR)/$(DRIVER) $(LINKDIR)/$(DRIVER)

uninstall:
	@echo "Uninstalling $(DRIVER)"
	@rm -f $(BINDIR)/$(DRIVER)
	@rm -f $(LINKDIR)/$(DRIVER)

clean:
	@echo "Cleaning"
	@rm -f $(DRIVER) $(OBJS)
