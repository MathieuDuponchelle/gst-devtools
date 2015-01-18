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
  klass->monitor_pad = NULL;
}

static void
do_add_pad_pre (GstTracer * self, G_GNUC_UNUSED guint64 ts,
    G_GNUC_UNUSED GstElement * elem, GstPad * pad)
{
  GstValidateTracerClass *tracer_class =
      GST_VALIDATE_TRACER_CLASS (G_OBJECT_GET_CLASS (self));

  if (tracer_class->monitor_pad) {
    PadMonitor *pad_monitor = tracer_class->monitor_pad (self, pad);
    monitor_register (MONITOR (pad_monitor), self, G_OBJECT (pad));
  }
}

gboolean
gst_validate_tracer_parse_structure (GstValidateTracer * self,
    GstStructure * structure)
{
  return TRUE;
}

static void
gst_validate_tracer_init (GstValidateTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);

  GST_DEBUG_OBJECT (self, "initializing base validate tracer");

  gst_tracing_register_hook (tracer, "element-add-pad-pre",
      G_CALLBACK (do_add_pad_pre));
}
