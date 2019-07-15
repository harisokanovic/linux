obj-y += clk_sources.o clk.o clk_cgu.o  clk_cpccr.o  clk_gate.o  clk_pll.o cpm_pwc.o

# XXX Workaround bugs in cpm_pwc.c
CFLAGS_cpm_pwc.o += -Wno-unused-function -Wno-array-bounds
