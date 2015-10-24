AM_CFLAGS = \
	-I $(top_srcdir)/include/drm \
	-I $(top_srcdir)/sdp \
	-I $(top_srcdir)

noinst_PROGRAMS = \
	sdp_test

sdp_test_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/sdp/libdrm_sdp.la

sdp_test_SOURCES = \
	sdp_test.c buffers.c buffers.h

run: sdp_test
	./sdp_test
