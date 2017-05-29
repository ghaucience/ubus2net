CROSS 	?= 
ARCH		?=

GCC 		?= $(CROSS)gcc
CXX			?= $(CROSS)g++
AR			?= $(CROSS)ar
AS			?= $(CROSS)gcc
RANLIB	?= $(CROSS)ranlib
STRIP 	?= $(CROSS)strip
OBJCOPY	?= $(CROSS)objcopy
OBJDUMP ?= $(CROSS)objdump
SIZE		?= $(CROSS)size
LD			?= $(CROSS)ld
MKDIR		?= mkdir -p


TARGET_CFLAGS 		+= -Wall -g -O2 -I$(ROOTDIR)/inc -I$(ROOTDIR)/inc/ayla -I$(ROOTDIR)/platform

TARGET_CXXFLAGS 	+= $(TARGET_CFLAGS) -std=c++0x

TARGET_LDFLAGS 		+= -L$(ROOTDIR)/lib -lm -lrt -ldl -lpthread
TARGET_LDFLAGS 	+= -L/usr/lib/ -ljansson -lubus -lblobmsg_json -lubox
#TARGET_LDFLAGS		+= -lstdc++

