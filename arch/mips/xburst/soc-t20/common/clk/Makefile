obj-y += clk_sources.o clk.o clk_cgu.o  clk_cpccr.o  clk_gate.o  clk_pll.o cpm_pwc.o

# XXX cpm_pwc_srcs is empty array, gcc 4.9 doesn't like it.
#  Seems like a bug but kernel works.
CFLAGS_cpm_pwc.o += -Wno-unused-function -Wno-array-bounds
