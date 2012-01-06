AM_CFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/libkms/ \
	-I$(top_srcdir)/slp/ \
	-I$(top_srcdir) \
	$(CAIRO_CFLAGS)

noinst_PROGRAMS = \
	modetest

modetest_SOURCES = \
	modetest.c
modetest_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la \
	$(top_builddir)/slp/libdrm_slp.la \
	$(CAIRO_LIBS)
