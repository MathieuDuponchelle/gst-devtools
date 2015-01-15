#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gsttracer.h>
#include "gstvalidateframecomparator.h"

GST_DEBUG_CATEGORY_STATIC (gst_validate_frame_comparator_debug);
#define GST_CAT_DEFAULT gst_validate_frame_comparator_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_validate_frame_comparator_debug, "framecomparator", 0, "validate frame comparator");

#define gst_validate_frame_comparator_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstFrameComparator, gst_validate_frame_comparator,
    GST_TYPE_VALIDATE_TRACER, _do_init);

static void
_constructed (GObject * object)
{
  gchar *params;

  GST_ERROR ("constructed");
  g_object_get (object, "params", &params, NULL);
  GST_ERROR_OBJECT (object, "constructing, params : %s", params);
  ((GObjectClass *) parent_class)->constructed (object);
}

static void
gst_validate_frame_comparator_class_init (GstFrameComparatorClass * klass)
{
  G_OBJECT_CLASS (klass)->constructed = _constructed;
}

static void
do_push_buffer_pre (GstTracer * self, guint64 ts, GstPad * pad,
    GstBuffer * buffer)
{
  GST_ERROR_OBJECT (self, "about to push a buffer %p on %p", buffer, pad);
}

static void
gst_validate_frame_comparator_init (GstFrameComparator * self)
{
  GstTracer *tracer = GST_TRACER (self);


  gst_tracing_register_hook (tracer, "pad-push-pre",
      G_CALLBACK (do_push_buffer_pre));
}
