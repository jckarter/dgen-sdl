# DGen/SDL v1.20+

include $(top_srcdir)/Makefile.asm.am

DIST_SUBDIRS = star musa mz80 sdl
SUBDIRS = @CPUDIRS@ @INTERFACE@

INCLUDES += -I$(top_srcdir)/@INTERFACE@

bin_PROGRAMS = dgen tobin

man_MANS = dgen.1 dgenrc.5 tobin.1

EXTRA_DIST = sample.dgenrc $(man_MANS) Makefile.asm.am

# dgen

dgen_SOURCES = rc.cpp romload.c md.cpp mdfr.cpp md-joe.cpp decode.c vdp.cpp \
	       save.cpp graph.cpp myfm.cpp fm.c sn76496.c ras.cpp main.cpp \
	       rc.h md-phil.h rc-vars.h md.h mz80.h fm.h decode.h mem.cpp \
	       ras-drawplane.h pd.h driver.h sn76496.h \
	       mz80.h starcpu.h m68k.h m68kconf.h

dgen_LDADD = -L@INTERFACE@ -lpd @STAR@ @MUSA@ @MZ80@ @SDL_LIBS@ @EXTRAOBJS@

dgen_DEPENDENCIES = @EXTRAOBJS@

EXTRA_dgen_SOURCES = asm_tiles.asmu ctv.asmu asm_memcpy.asmu mmx_memcpy.asmu \
		     sn76496u.c zz80.c md-phil.cpp

# tobin

tobin_SOURCES = tobin.c romload.c

