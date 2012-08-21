AM_CFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/libkms/ \
	-I$(top_srcdir)/exynos/ \
	-I$(top_srcdir) \
	$(CAIRO_CFLAGS)

noinst_PROGRAMS = \
	g2dtest

g2dtest_SOURCES = \
	g2dtest.c

g2dtest_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la \
	$(top_builddir)/exynos/libdrm_exynos.la \
	$(CAIRO_LIBS)
