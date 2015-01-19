#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gsttracer.h>
#include <gst/validate/gst-validate-utils.h>
#include <gst/validate/validate.h>
#include "gstvalidator.h"
#include "gstvalidatetracer.h"
#include "padmonitor.h"
#include "elementmonitor.h"

GST_DEBUG_CATEGORY_STATIC (gst_validator_debug);
#define GST_CAT_DEFAULT gst_validator_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_validator_debug, "validator", 0, "validate runner");

#define gst_validator_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstValidator, gst_validator,
    GST_TYPE_TRACER, _do_init);

typedef enum
{
  OK,
  WRONG_OVERRIDES,
  WRONG_FILE
} ParseResult;

static gboolean
_parse_checker_structure (GstValidator * self, GstStructure * structure)
{
  GstValidateTracer *tracer;
  const gchar *tracer_name;

  tracer_name = gst_structure_get_name (structure);
  tracer = g_hash_table_lookup (self->tracers, tracer_name);

  if (tracer == NULL) {
    tracer = (GstValidateTracer *) gst_tracer_factory_make (tracer_name, NULL);
    gst_validate_tracer_set_runner (tracer, self->runner);
    g_hash_table_insert (self->tracers, g_strdup (tracer_name), tracer);
  }
  if (!tracer) {
    GST_ERROR ("No factory for tracer named %s", tracer_name);

    return FALSE;
  }

  return gst_validate_tracer_parse_structure (tracer, structure);
}

static ParseResult
_parse_config (GstValidator * self, const gchar * path)
{
  ParseResult ret = OK;
  GList *structs = gst_validate_utils_structs_parse_from_filename (path);

  if (structs) {
    GList *tmp;

    for (tmp = structs; tmp; tmp = tmp->next) {
      if (!_parse_checker_structure (self, tmp->data)) {
        GST_ERROR ("Wrong overrides %" GST_PTR_FORMAT, tmp->data);
        ret = WRONG_OVERRIDES;
      }
    }

    return ret;
  }

  return WRONG_FILE;
}

static void
_constructed (GObject * object)
{
  gchar *params;

  g_object_get (object, "params", &params, NULL);
  ((GObjectClass *) parent_class)->constructed (object);

  _parse_config (GST_VALIDATOR (object), "default.txt");
}

static void
_dispose (GObject * object)
{
  GstValidator *self = GST_VALIDATOR (object);
  gst_validate_runner_printf (self->runner);
}

static void
gst_validator_class_init (GstValidatorClass * klass)
{
  G_OBJECT_CLASS (klass)->constructed = _constructed;
  G_OBJECT_CLASS (klass)->dispose = _dispose;
}

static void
do_add_pad_post (GstTracer * self, G_GNUC_UNUSED guint64 ts,
    GstElement * elem, GstPad * pad, gboolean result)
{
  PadMonitor *padmonitor;

  if (!result) {
    GST_ERROR ("fuck");
    return;
  }

  padmonitor = g_object_new (pad_monitor_get_type (), "object", pad, NULL);
  monitor_register (MONITOR (padmonitor), self, G_OBJECT (pad));
}

static void
do_add_element_pre (GstTracer * self, G_GNUC_UNUSED guint64 ts,
    G_GNUC_UNUSED GstBin * bin, GstElement * element)
{
  ElementMonitor *element_monitor =
      g_object_new (element_monitor_get_type (), "object", element, NULL);
  element_monitor_inspect (element_monitor);
  monitor_register (MONITOR (element_monitor), self, G_OBJECT (element));
}

static void
gst_validator_init (GstValidator * self)
{
  self->tracers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->runner = gst_validate_runner_new ();

  gst_validate_init ();
  /* Register our default hooks, that will create monitors before anyone has a
   * chance to ask for them */
  gst_tracing_register_hook (GST_TRACER (self), "element-add-pad-post",
      G_CALLBACK (do_add_pad_post));
  gst_tracing_register_hook (GST_TRACER (self), "bin-add-element-pre",
      G_CALLBACK (do_add_element_pre));
}
