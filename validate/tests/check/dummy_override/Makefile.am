lib_LTLIBRARIES = libdummy_override.la
libdummy_override_la_SOURCES = \
    dummy_override.c \
    test-set-timestamp-override.c \
    test-set-timestamp-override.h

libdummy_override_la_CFLAGS = $(GST_ALL_CFLAGS) $(GIO_CFLAGS)
libdummy_override_la_LDFLAGS = $(GST_LIB_LDFLAGS) $(GST_ALL_LDFLAGS) \
	$(GST_LT_LDFLAGS) $(GIO_LDFLAGS) $(top_builddir)/gst/validate/libgstvalidate-1.0.la
libdummy_override_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) \
	$(GST_ALL_LIBS) $(GIO_LIBS)
libdummy_overrideincludedir = $(includedir)/gstreamer-@GST_API_VERSION@/gst/validate
libdummy_overrideinclude_HEADERS =

CLEANFILES =
