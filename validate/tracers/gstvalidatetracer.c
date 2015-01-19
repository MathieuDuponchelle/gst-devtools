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
do_add_pad_post (GstTracer * tracer, G_GNUC_UNUSED guint64 ts,
    G_GNUC_UNUSED GstElement * elem, GstPad * pad, gboolean result)
{
  GstValidateTracer *self = GST_VALIDATE_TRACER (tracer);
  GstValidateTracerClass *tracer_class =
      GST_VALIDATE_TRACER_CLASS (G_OBJECT_GET_CLASS (self));

  if (!result)
    return;

  if (tracer_class->monitor_pad) {
    PadMonitor *pad_monitor = tracer_class->monitor_pad (tracer, pad);
    monitor_register (MONITOR (pad_monitor), tracer, G_OBJECT (pad));
  }
}

gboolean
gst_validate_tracer_parse_structure (GstValidateTracer * self,
    GstStructure * structure)
{
  return TRUE;
}

void
gst_validate_tracer_set_runner (GstValidateTracer * self,
    GstValidateRunner * runner)
{
  self->runner = runner;
}

static void
gst_validate_tracer_init (GstValidateTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);

  GST_DEBUG_OBJECT (self, "initializing base validate tracer");

  gst_tracing_register_hook (tracer, "element-add-pad-post",
      G_CALLBACK (do_add_pad_post));
}
