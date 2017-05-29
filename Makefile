ROOTDIR=$(shell pwd)
WORKDIR=$(ROOTDIR)/build

#targets	 += ubus2net_svr
targets	 += ubus2net_cli

.PHONY: targets

all : $(targets)


svrsrcs							:= $(ROOTDIR)/main_svr.c
svrsrcs							+= $(ROOTDIR)/src/ayla/log.c
svrsrcs							+= $(ROOTDIR)/src/ayla/lookup_by_name.c
svrsrcs							+= $(ROOTDIR)/src/ayla/timer.c
svrsrcs							+= $(ROOTDIR)/src/ayla/time_utils.c
svrsrcs							+= $(ROOTDIR)/src/ayla/assert.c
svrsrcs							+= $(ROOTDIR)/src/ayla/file_event.c
svrsrcs							+= $(ROOTDIR)/src/ayla/json_parser.c
svrsrcs							+= $(ROOTDIR)/src/lockqueue.c
svrsrcs							+= $(ROOTDIR)/src/mutex.c
svrsrcs							+= $(ROOTDIR)/src/cond.c
svrsrcs							+= $(ROOTDIR)/src/list.c
svrsrcs							+= $(ROOTDIR)/src/tcp.c

clisrcs							:= $(ROOTDIR)/main_cli.c
clisrcs							+= $(ROOTDIR)/src/ayla/log.c
clisrcs							+= $(ROOTDIR)/src/ayla/lookup_by_name.c
clisrcs							+= $(ROOTDIR)/src/ayla/timer.c
clisrcs							+= $(ROOTDIR)/src/ayla/time_utils.c
clisrcs							+= $(ROOTDIR)/src/ayla/assert.c
clisrcs							+= $(ROOTDIR)/src/ayla/file_event.c
clisrcs							+= $(ROOTDIR)/src/ayla/json_parser.c
clisrcs							+= $(ROOTDIR)/src/lockqueue.c
clisrcs							+= $(ROOTDIR)/src/mutex.c
clisrcs							+= $(ROOTDIR)/src/cond.c
clisrcs							+= $(ROOTDIR)/src/list.c
clisrcs							+= $(ROOTDIR)/src/tcp.c


svrobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(svrsrcs)))
cliobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(clisrcs)))

-include $(ROOTDIR)/make/arch.mk
-include $(ROOTDIR)/make/rules.mk

$(eval $(call LinkApp,ubus2net_svr,$(svrobjs)))
$(eval $(call LinkApp,ubus2net_cli,$(cliobjs)))


run : 
	sudo ./build/ubus2net
