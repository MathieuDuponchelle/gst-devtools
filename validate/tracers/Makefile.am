lib_LTLIBRARIES = libgstvalidatetracers.la

libgstvalidatetracers_la_SOURCES = \
				   gstvalidatetracers.c \
				   gstvalidatetracer.c \
				   gstvalidateframecomparator.c \
				   gstvalidator.c \
				   monitor.c \
				   padmonitor.c \
				   elementmonitor.c

libgstvalidatetracers_la_CFLAGS = $(GST_ALL_CFLAGS) \
  -DGST_USE_UNSTABLE_API
libgstvalidatetracers_la_LDFLAGS = $(GST_LIB_LDFLAGS) $(GST_ALL_LDFLAGS) \
				   $(top_builddir)/gst/validate/libgstvalidate-1.0.la
libgstvalidatetracers_la_LIBADD = \
	$(GST_BASE_LIBS) $(GST_ALL_LIBS)

noinst_HEADERS = \
	gstvalidatetracer.h \
	gstvalidateframecomparator.h \
	gstvalidator.h \
	monitor.h \
	padmonitor.h \
	elementmonitor.h
