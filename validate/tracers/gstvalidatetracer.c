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

typedef struct
{
  gint lol;
} DummyData;

static gpointer
_get_dummy_data (GstTracer * self, GstObject * object)
{
  return (g_malloc0 (sizeof (DummyData)));
}

static void
gst_validate_tracer_class_init (GstValidateTracerClass * klass)
{
  klass->create_monitor_data = _get_dummy_data;
}

static GQuark
_get_quark_for_instance (GstTracer * self)
{
  gchar *quark_string;
  GQuark quark;

  quark_string =
      g_strdup_printf ("%s-%p", g_type_name (G_OBJECT_TYPE (self)), self);
  quark = g_quark_from_string (quark_string);
  g_free (quark_string);
  return quark;
}

static void
do_add_pad_pre (GstTracer * self, G_GNUC_UNUSED guint64 ts,
    G_GNUC_UNUSED GstElement * elem, GstPad * pad)
{
  GstValidateTracerClass *tracer_class =
      GST_VALIDATE_TRACER_CLASS (G_OBJECT_GET_CLASS (self));

  if (tracer_class->create_monitor_data) {
    gpointer qdata = tracer_class->create_monitor_data (self, GST_OBJECT (pad));
    g_object_set_qdata (G_OBJECT (pad), _get_quark_for_instance (self), qdata);
  }
}

gpointer
gst_validate_tracer_get_monitor_data (GstTracer * self, GstObject * object)
{
  return g_object_get_qdata (G_OBJECT (object), _get_quark_for_instance (self));
}

static void
gst_validate_tracer_init (GstValidateTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);

  GST_DEBUG_OBJECT (self, "initializing base validate tracer");

  gst_tracing_register_hook (tracer, "element-add-pad-pre",
      G_CALLBACK (do_add_pad_pre));
}
