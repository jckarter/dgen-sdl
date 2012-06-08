# DGen/SDL v1.20+
# Automakefile for mz80

include $(top_srcdir)/Makefile.asm.am
# Since the chop_ script flakes on mz80, we use mz80's builtin facility
if NO_USCORE
 plain = -l
else
 plain =
endif

INCLUDES += -I..

EXTRA_DIST = mz80.txt

noinst_LIBRARIES = libmz80.a

makez80: makez80.c
	$(CC) -o makez80 makez80.c

mz80.asm: makez80
	./makez80 $(plain) -s -x86 mz80.asm

libmz80_a_SOURCES =

libmz80_a_LIBADD = mz80.o

libmz80_a_DEPENDENCIES = mz80.o

EXTRA_libmz80_a_SOURCES = makez80.c

DISTCLEANFILES = mz80.asm makez80
