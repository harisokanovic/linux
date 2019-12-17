obj-y += prom.o mxu-v2-ex.o

# XXX Define rule to build mxu-v2-ex.o because there is
#  no source file. Copy mxu-v2-ex.obj --> mxu-v2-ex.o .
ifeq ($(KBUILD_SRC),)
SOURCE_PATH = $(src)
else
SOURCE_PATH = $(KBUILD_SRC)/$(src)
endif
$(obj)/mxu-v2-ex.o: $(SOURCE_PATH)/mxu-v2-ex.obj FORCE
	cp "$<" "$@"

obj-$(CONFIG_JZ_CPU_SCACHE)	+= sc-jz.o
obj-$(CONFIG_TRAPS_USE_TCSM) += tcsm.o
obj-$(CONFIG_SMP)	+= smp.o smp_secondary.o ltimer.o
obj-$(CONFIG_PMON_DEBUG)	+= perf_proc_jz.o perf_event_jz.o
