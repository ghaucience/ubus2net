ROOTDIR=$(shell pwd)
WORKDIR=$(ROOTDIR)/build

targets	 += ubus2net

.PHONY: targets

all : $(targets)


srcs							:= $(ROOTDIR)/main.c
srcs							+= $(ROOTDIR)/src/ayla/log.c
srcs							+= $(ROOTDIR)/src/ayla/lookup_by_name.c
srcs							+= $(ROOTDIR)/src/ayla/timer.c
srcs							+= $(ROOTDIR)/src/ayla/time_utils.c
srcs							+= $(ROOTDIR)/src/ayla/assert.c
srcs							+= $(ROOTDIR)/src/ayla/file_event.c
srcs							+= $(ROOTDIR)/src/ayla/json_parser.c
srcs							+= $(ROOTDIR)/src/lockqueue.c
srcs							+= $(ROOTDIR)/src/mutex.c
srcs							+= $(ROOTDIR)/src/cond.c
srcs							+= $(ROOTDIR)/src/list.c
srcs							+= $(ROOTDIR)/src/tcp.c

objs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(srcs)))

-include $(ROOTDIR)/make/arch.mk
-include $(ROOTDIR)/make/rules.mk

$(eval $(call LinkApp,ubus2net,$(objs)))


run : 
	sudo ./build/ubus2net
