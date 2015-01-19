#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/validate/validate.h>
#include <gst/validate/gst-validate-reporter.h>
#include "monitor.h"

enum
{
  PROP_0,
  PROP_RUNNER,
  PROP_OBJECT,
  PROP_LAST
};

static GstValidateInterceptionReturn
monitor_intercept_report (GstValidateReporter * reporter,
    GstValidateReport * report)
{
  return GST_VALIDATE_REPORTER_REPORT;
}

static void
_reporter_iface_init (GstValidateReporterInterface * iface)
{
  iface->intercept_report = monitor_intercept_report;
}

#define _do_init \
  G_IMPLEMENT_INTERFACE (GST_TYPE_VALIDATE_REPORTER, _reporter_iface_init)

#define monitor_parent_class parent_class

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (Monitor, monitor, G_TYPE_OBJECT, _do_init);

static void
_target_freed_cb (Monitor * monitor, GObject * where_the_object_was)
{
  GST_DEBUG_OBJECT (monitor, "Target was freed");
  monitor->target = NULL;
}

static void
monitor_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Monitor *monitor = MONITOR (object);

  switch (prop_id) {
    case PROP_OBJECT:
      g_assert (monitor->target == NULL);
      monitor->target = g_value_get_object (value);
      g_object_weak_ref (G_OBJECT (monitor->target),
          (GWeakNotify) _target_freed_cb, monitor);

      if (monitor->target)
        gst_validate_reporter_set_name (GST_VALIDATE_REPORTER (monitor),
            g_strdup (GST_OBJECT_NAME (monitor->target)));
      break;
    case PROP_RUNNER:
      gst_validate_reporter_set_runner (GST_VALIDATE_REPORTER (monitor),
          g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
monitor_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Monitor *monitor = MONITOR (object);

  switch (prop_id) {
    case PROP_OBJECT:
      g_value_set_object (value, GST_VALIDATE_MONITOR_GET_OBJECT (monitor));
      break;
    case PROP_RUNNER:
      g_value_set_object (value,
          gst_validate_reporter_get_runner (GST_VALIDATE_REPORTER (monitor)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
monitor_class_init (MonitorClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = monitor_get_property;
  gobject_class->set_property = monitor_set_property;

  g_object_class_install_property (gobject_class, PROP_RUNNER,
      g_param_spec_object ("validate-runner", "VALIDATE Runner",
          "The Validate runner to " "report errors to",
          GST_TYPE_VALIDATE_RUNNER,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_OBJECT,
      g_param_spec_object ("object", "Object", "The object to be monitored",
          GST_TYPE_OBJECT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

void
monitor_register (Monitor * monitor, GstTracer * tracer, GObject * object)
{
  GQuark quark;

  quark = g_quark_from_string (g_type_name (G_OBJECT_TYPE (tracer)));
  g_object_set_qdata (object, quark, monitor);
  monitor->tracer = tracer;
}

/* FIXME move to some utils file */
Monitor *
get_monitor_for_type_name (GObject * object, const gchar * type_name)
{
  return g_object_get_qdata (object, g_quark_from_string (type_name));
}

static void
monitor_init (Monitor * monitor)
{
}
