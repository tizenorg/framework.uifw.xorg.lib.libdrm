AM_CFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/libkms/ \
	-I$(top_srcdir) \
	$(CAIRO_CFLAGS)

noinst_PROGRAMS = \
	planetest

planetest_SOURCES = \
	planetest.c
planetest_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la \
	$(CAIRO_LIBS)
