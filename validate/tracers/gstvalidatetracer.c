#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gsttracer.h>
#include "gstvalidatetracer.h"

GST_DEBUG_CATEGORY_STATIC (gst_validate_tracer_debug);
#define GST_CAT_DEFAULT gst_validate_tracer_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_validate_tracer_debug, "validatetracer", 0, "base validate tracer");

#define gst_validate_tracer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstValidateTracer, gst_validate_tracer,
    GST_TYPE_TRACER, _do_init);

static void
gst_validate_tracer_class_init (GstValidateTracerClass * klass)
{
}

static void
gst_validate_tracer_init (GstValidateTracer * self)
{
  GST_DEBUG_OBJECT (self, "initializing base validate tracer");
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_tracer_register (plugin, "validatetracer",
          gst_validate_tracer_get_type ()))
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, validatetracers,
    "GStreamer validate tracers", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
