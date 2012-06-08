# DGen/SDL v1.20+
# SDL Interface

EXTRA_DIST = dgenfont.pbm chartfont.c

INCLUDES += -I..

noinst_PROGRAMS = pbm2df

noinst_LIBRARIES = libpd.a

# built sources

BUILT_SOURCES = dgenfont.cpp

dgenfont.cpp: pbm2df dgenfont.pbm
	./pbm2df dgenfont.pbm dgenfont.cpp

# pd.a

libpd_a_SOURCES = font.cpp dgenfont.cpp sdl.cpp font.h pd-defs.h ogl_fonts.h
