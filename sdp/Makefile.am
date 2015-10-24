AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/sdp \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_sdp_la_LTLIBRARIES = libdrm_sdp.la
libdrm_sdp_ladir = $(libdir)
libdrm_sdp_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_sdp_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_sdp_la_SOURCES = \
	sdp_drm.c

libdrm_sdpincludedir = ${includedir}/libdrm

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_sdp.pc
