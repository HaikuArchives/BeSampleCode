Digit -- README.txt
last edited 15-Jun-1999

Digit is a simple driver that can be used as a starting point for your own drivers. It creates a simple device in /dev/misc/digit that by default presents a reader with 16 bytes of zeros. The value it returns can be changed for a particular open instance via either ioctl or write, as demonstrated by the test program included in the archive.

		Digit Contents

Here are the contents of this sample code archive:
driver/		The code for the digit driver
test/		A simple test app that works with the digit device
article.txt	The newsletter article associated with this sample code archive
README.txt	This file, as if you couldn't guess.

		Building Digit

To build the driver and test app, simply use the makefile or the project files that are located in each subdirectory.

After you build the driver, place the binary in ~/config/add-ons/kernel/drivers/bin. Then create a link to it in ~/config/add-ons/kernel/drivers/dev/misc, i.e.:

mkdir -p ~/config/add-ons/kernel/drivers/dev/misc
cd ~/config/add-ons/kernel/drivers/dev/misc
ln -s ../../bin/digit .
