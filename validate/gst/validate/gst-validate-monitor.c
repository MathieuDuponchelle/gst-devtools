/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thiago Sousa Santos <thiago.sousa.santos@collabora.com>
 *
 * gst-validate-monitor.c - Validate Monitor class
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gst-validate-internal.h"
#include "gst-validate-monitor.h"

/**
 * SECTION:gst-validate-monitor
 * @short_description: Base class that wraps a #GObject for Validate checks
 *
 * TODO
 */

enum
{
  PROP_0,
  PROP_OBJECT,
  PROP_RUNNER,
  PROP_VALIDATE_PARENT,
  PROP_LAST
};

static gboolean gst_validate_monitor_do_setup (GstValidateMonitor * monitor);
static void
gst_validate_monitor_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void
gst_validate_monitor_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static GObject *gst_validate_monitor_constructor (GType type,
    guint n_construct_params, GObjectConstructParam * construct_params);

gboolean gst_validate_monitor_setup (GstValidateMonitor * monitor);

static void gst_validate_monitor_intercept_report (GstValidateReporter *
    reporter, GstValidateReport * report);

#define _do_init \
  G_IMPLEMENT_INTERFACE (GST_TYPE_VALIDATE_REPORTER, _reporter_iface_init)

static void
_reporter_iface_init (GstValidateReporterInterface * iface)
{
  iface->intercept_report = gst_validate_monitor_intercept_report;
}

#define gst_validate_monitor_parent_class parent_class
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GstValidateMonitor, gst_validate_monitor,
    G_TYPE_OBJECT, _do_init);

static void
_target_freed_cb (GstValidateMonitor * monitor, GObject * where_the_object_was)
{
  GST_DEBUG_OBJECT (monitor, "Target was freed");
  monitor->target = NULL;
}

static void
gst_validate_monitor_dispose (GObject * object)
{
  GstValidateMonitor *monitor = GST_VALIDATE_MONITOR_CAST (object);

  g_mutex_clear (&monitor->mutex);
  g_mutex_clear (&monitor->overrides_mutex);
  g_queue_clear (&monitor->overrides);

  if (monitor->target)
    g_object_weak_unref (G_OBJECT (monitor->target),
        (GWeakNotify) _target_freed_cb, monitor);

  if (monitor->media_descriptor)
    gst_object_unref (monitor->media_descriptor);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_validate_monitor_finalize (GObject * object)
{
  gst_validate_reporter_set_name (GST_VALIDATE_REPORTER (object), NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_validate_monitor_class_init (GstValidateMonitorClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = gst_validate_monitor_get_property;
  gobject_class->set_property = gst_validate_monitor_set_property;
  gobject_class->dispose = gst_validate_monitor_dispose;
  gobject_class->finalize = gst_validate_monitor_finalize;
  gobject_class->constructor = gst_validate_monitor_constructor;

  klass->setup = gst_validate_monitor_do_setup;

  g_object_class_install_property (gobject_class, PROP_OBJECT,
      g_param_spec_object ("object", "Object", "The object to be monitored",
          GST_TYPE_OBJECT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RUNNER,
      g_param_spec_object ("validate-runner", "VALIDATE Runner",
          "The Validate runner to " "report errors to",
          GST_TYPE_VALIDATE_RUNNER,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_VALIDATE_PARENT,
      g_param_spec_object ("validate-parent", "VALIDATE parent monitor",
          "The Validate monitor " "that is the parent of this one",
          GST_TYPE_VALIDATE_MONITOR,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static GObject *
gst_validate_monitor_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  GstValidateMonitor *monitor =
      GST_VALIDATE_MONITOR_CAST (G_OBJECT_CLASS (parent_class)->constructor
      (type,
          n_construct_params,
          construct_params));

  if (monitor->parent) {
    gst_validate_monitor_set_media_descriptor (monitor,
        monitor->parent->media_descriptor);
  }


  gst_validate_monitor_setup (monitor);
  return (GObject *) monitor;
}

static void
gst_validate_monitor_init (GstValidateMonitor * monitor)
{
  g_mutex_init (&monitor->mutex);

  g_mutex_init (&monitor->overrides_mutex);
  g_queue_init (&monitor->overrides);
}

#if 0
/* This shouldn't be used. it's a base class */
/**
 * gst_validate_monitor_new:
 * @element: (transfer-none): a #GObject to run Validate on
 */
GstValidateMonitor *
gst_validate_monitor_new (GObject * object)
{
  GstValidateMonitor *monitor =
      g_object_new (GST_TYPE_VALIDATE_MONITOR, "object",
      G_TYPE_OBJECT, object, NULL);

  if (GST_VALIDATE_MONITOR_GET_OBJECT (monitor) == NULL) {
    /* setup failed, no use on returning this monitor */
    g_object_unref (monitor);
    return NULL;
  }

  return monitor;
}
#endif

static gboolean
gst_validate_monitor_do_setup (GstValidateMonitor * monitor)
{
  /* NOP */
  return TRUE;
}

gboolean
gst_validate_monitor_setup (GstValidateMonitor * monitor)
{
  GST_DEBUG_OBJECT (monitor, "Starting monitor setup");
  return GST_VALIDATE_MONITOR_GET_CLASS (monitor)->setup (monitor);
}

GstElement *
gst_validate_monitor_get_element (GstValidateMonitor * monitor)
{
  GstValidateMonitorClass *klass = GST_VALIDATE_MONITOR_GET_CLASS (monitor);
  GstElement *element = NULL;

  if (klass->get_element)
    element = klass->get_element (monitor);

  return element;
}

const gchar *
gst_validate_monitor_get_element_name (GstValidateMonitor * monitor)
{
  GstElement *element;

  element = gst_validate_monitor_get_element (monitor);
  if (element)
    return GST_ELEMENT_NAME (element);
  return NULL;
}

/* Check if any of our overrides wants to change the report severity */
static void
gst_validate_monitor_intercept_report (GstValidateReporter * reporter,
    GstValidateReport * report)
{
  GList *iter;
  GstValidateMonitor *monitor = GST_VALIDATE_MONITOR_CAST (reporter);

  GST_VALIDATE_MONITOR_OVERRIDES_LOCK (monitor);
  for (iter = monitor->overrides.head; iter; iter = g_list_next (iter)) {
    report->level =
        gst_validate_override_get_severity (iter->data,
        gst_validate_issue_get_id (report->issue), report->level);
  }
  GST_VALIDATE_MONITOR_OVERRIDES_UNLOCK (monitor);
}

void
gst_validate_monitor_attach_override (GstValidateMonitor * monitor,
    GstValidateOverride * override)
{
  GST_VALIDATE_MONITOR_OVERRIDES_LOCK (monitor);
  g_queue_push_tail (&monitor->overrides, override);
  GST_VALIDATE_MONITOR_OVERRIDES_UNLOCK (monitor);
}

static void
gst_validate_monitor_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstValidateMonitor *monitor;

  monitor = GST_VALIDATE_MONITOR_CAST (object);

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
    case PROP_VALIDATE_PARENT:
      monitor->parent = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_validate_monitor_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstValidateMonitor *monitor;

  monitor = GST_VALIDATE_MONITOR_CAST (object);

  switch (prop_id) {
    case PROP_OBJECT:
      g_value_set_object (value, GST_VALIDATE_MONITOR_GET_OBJECT (monitor));
      break;
    case PROP_RUNNER:
      g_value_set_object (value, GST_VALIDATE_MONITOR_GET_RUNNER (monitor));
      break;
    case PROP_VALIDATE_PARENT:
      g_value_set_object (value, GST_VALIDATE_MONITOR_GET_PARENT (monitor));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

void
gst_validate_monitor_set_media_descriptor (GstValidateMonitor * monitor,
    GstMediaDescriptor * media_descriptor)
{
  GstValidateMonitorClass *klass = GST_VALIDATE_MONITOR_GET_CLASS (monitor);

  GST_DEBUG_OBJECT (monitor->target, "Set media desc: %" GST_PTR_FORMAT,
      media_descriptor);
  if (monitor->media_descriptor)
    gst_object_unref (monitor->media_descriptor);

  if (media_descriptor)
    gst_object_ref (media_descriptor);

  monitor->media_descriptor = media_descriptor;
  if (klass->set_media_descriptor)
    klass->set_media_descriptor (monitor, media_descriptor);
}
