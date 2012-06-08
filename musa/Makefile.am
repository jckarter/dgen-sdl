# DGen/SDL v1.20+
# Automakefile for Musashi

# hack it so that musa doesn't get affected by memcpy defines
CFLAGS = @CFLAGS@ -Umemcpy

noinst_PROGRAMS = m68kmake

noinst_LIBRARIES = libmusa68.a

m68kmake_SOURCES = m68kmake.c

m68kops.c: m68kmake
	./m68kmake
m68kopac.c: m68kmake
	./m68kmake
m68kopdm.c: m68kmake
	./m68kmake
m68kopnz.c: m68kmake
	./m68kmake
m68kops.h: m68kmake
	./m68kmake

EXTRA_libmusa68_a_SOURCES = m68k_in.c m68kmake.c m68kops.h

libmusa68_a_SOURCES = m68kcpu.c m68kcpu.h m68k.h m68kconf.h

libmusa68_a_LIBADD = m68kops.o m68kopac.o m68kopdm.o m68kopnz.o

libmusa68_a_DEPENDENCIES = m68kmake m68kops.o m68kopac.o m68kopdm.o m68kopnz.o

DISTCLEANFILES = m68kops.c m68kopac.c m68kopdm.c m68kopnz.c m68kmake
