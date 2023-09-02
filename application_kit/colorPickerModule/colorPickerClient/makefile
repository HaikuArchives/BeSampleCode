# Makefile template 
# you may edit the original in
#	~/config/settings/Eddie/TemplateMakefile

# This template works with the generic makefile in
#	/boot/system/develop/etc/makefile-engine
# and is a subset of
#	/boot/system/develop/etc/makefile

# you may add stuff here 


# name of your binary
NAME= SimpleApp

# type of binary (one of APP, SHARED, STATIC, DRIVER)
TYPE= APP

# @src->@ 

ORIGIN := /boot/home/src/colorPickerModule/colorPickerClient/ 

colorPickerClient_src := \
	main.cpp \
	SimpleWindow.cpp \
	ColorLabel.cpp \
	ModuleProxy.cpp \
#

SRCS := \
	$(colorPickerClient_src) \
#

# @<-src@ 


# list the libraries your binary needs to link against
# (lbe, ltracker, etc.)
LIBS= be 

# you may add stuff here 


include /boot/system/develop/etc/makefile-engine
