AM_CFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir) \
	$(CAIRO_CFLAGS)

noinst_PROGRAMS = \
	gemtest

gemtest_SOURCES = \
	gemtest.c
gemtest_LDADD = \
	$(top_builddir)/libdrm.la \
	$(CAIRO_LIBS)
