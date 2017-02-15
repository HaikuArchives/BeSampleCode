# Makefile template 
# you may edit the original in
#	~/config/settings/Eddie/TemplateMakefile

# This template works with the generic makefile in
#	/boot/develop/etc/makefile-engine
# and is a subset of
#	/boot/develop/etc/makefile

# you may add stuff here 


# name of your binary
NAME= CrayonColorPicker

# type of binary (one of APP, SHARED, STATIC, DRIVER)
TYPE= APP

# @src->@ 

ORIGIN := /boot/home/src/colorPickerModule/crayonColorPicker/ 

crayonPicker_src := \
	CrayonPickerApp.cpp \
	CrayonColorPicker.cpp \
	CrayonPicker.cpp \
#

Resources_rsrc := \
	CrayonPicker.rsrc \
#

SRCS := \
	$(crayonPicker_src) \
#

RSRCS := \
	$(Resources_rsrc) \
#

# @<-src@ 


# list the libraries your binary needs to link against
# (lbe, ltracker, etc.)
LIBS= be 

# you may add stuff here 


include /boot/develop/etc/makefile-engine
