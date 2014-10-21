/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thiago Sousa Santos <thiago.sousa.santos@collabora.com>
 *
 * gst-validate-element-monitor.c - Validate ElementMonitor class
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
#include "gst-validate-element-monitor.h"
#include "gst-validate-pad-monitor.h"
#include "gst-validate-monitor-factory.h"
#include <string.h>

/**
 * SECTION:gst-validate-element-monitor
 * @short_description: Class that wraps a #GstElement for Validate checks
 *
 * TODO
 */

#define gst_validate_element_monitor_parent_class parent_class
G_DEFINE_TYPE (GstValidateElementMonitor, gst_validate_element_monitor,
    GST_TYPE_VALIDATE_MONITOR);

static void
gst_validate_element_monitor_wrap_pad (GstValidateElementMonitor * monitor,
    GstPad * pad);
static gboolean gst_validate_element_monitor_do_setup (GstValidateMonitor *
    monitor);
static GstElement *gst_validate_element_monitor_get_element (GstValidateMonitor
    * monitor);

static void
_validate_element_pad_added (GstElement * element, GstPad * pad,
    GstValidateElementMonitor * monitor);

static void
gst_validate_element_set_media_descriptor (GstValidateMonitor * monitor,
    GstMediaDescriptor * media_descriptor)
{
  gboolean done;
  GstPad *pad;
  GstValidateMonitor *pmonitor;
  GstIterator *iterator;

  iterator =
      gst_element_iterate_pads (GST_ELEMENT (GST_VALIDATE_MONITOR_GET_OBJECT
          (monitor)));
  done = FALSE;
  while (!done) {
    GValue value = { 0, };

    switch (gst_iterator_next (iterator, &value)) {
      case GST_ITERATOR_OK:

        pad = g_value_get_object (&value);

        pmonitor = g_object_get_data ((GObject *) pad, "validate-monitor");
        if (pmonitor)
          gst_validate_monitor_set_media_descriptor (pmonitor,
              media_descriptor);
        g_value_reset (&value);
        break;
      case GST_ITERATOR_RESYNC:
        /* TODO how to handle this? */
        gst_iterator_resync (iterator);
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iterator);
}


static void
gst_validate_element_monitor_dispose (GObject * object)
{
  GstValidateElementMonitor *monitor =
      GST_VALIDATE_ELEMENT_MONITOR_CAST (object);

  if (GST_VALIDATE_MONITOR_GET_OBJECT (monitor) && monitor->pad_added_id)
    g_signal_handler_disconnect (GST_VALIDATE_MONITOR_GET_OBJECT (monitor),
        monitor->pad_added_id);

  g_list_free_full (monitor->pad_monitors, g_object_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
gst_validate_element_monitor_class_init (GstValidateElementMonitorClass * klass)
{
  GObjectClass *gobject_class;
  GstValidateMonitorClass *monitor_klass;

  gobject_class = G_OBJECT_CLASS (klass);
  monitor_klass = GST_VALIDATE_MONITOR_CLASS (klass);

  gobject_class->dispose = gst_validate_element_monitor_dispose;

  monitor_klass->setup = gst_validate_element_monitor_do_setup;
  monitor_klass->get_element = gst_validate_element_monitor_get_element;
  monitor_klass->set_media_descriptor =
      gst_validate_element_set_media_descriptor;
}

static void
gst_validate_element_monitor_init (GstValidateElementMonitor * element_monitor)
{
}

/**
 * gst_validate_element_monitor_new:
 * @element: (transfer-none): a #GstElement to run Validate on
 */
GstValidateElementMonitor *
gst_validate_element_monitor_new (GstElement * element,
    GstValidateRunner * runner, GstValidateMonitor * parent)
{
  GstValidateElementMonitor *monitor;

  g_return_val_if_fail (element != NULL, NULL);

  monitor = g_object_new (GST_TYPE_VALIDATE_ELEMENT_MONITOR, "object", element,
      "validate-runner", runner, "validate-parent", parent, NULL);

  if (GST_VALIDATE_ELEMENT_MONITOR_GET_ELEMENT (monitor) == NULL) {
    g_object_unref (monitor);
    return NULL;
  }

  return monitor;
}

static GstElement *
gst_validate_element_monitor_get_element (GstValidateMonitor * monitor)
{
  return GST_VALIDATE_ELEMENT_MONITOR_GET_ELEMENT (monitor);
}

static void
gst_validate_element_monitor_inspect (GstValidateElementMonitor * monitor)
{
  GstElement *element;
  GstElementClass *klass;
  const gchar *klassname;

  element = GST_VALIDATE_ELEMENT_MONITOR_GET_ELEMENT (monitor);
  klass = GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS (element));


  klassname =
      gst_element_class_get_metadata (klass, GST_ELEMENT_METADATA_KLASS);
  if (klassname) {
    monitor->is_decoder = strstr (klassname, "Decoder") != NULL;
    monitor->is_encoder = strstr (klassname, "Encoder") != NULL;
    monitor->is_demuxer = strstr (klassname, "Demuxer") != NULL;
  } else
    GST_ERROR_OBJECT (element, "no klassname");
}

static gboolean
gst_validate_element_monitor_do_setup (GstValidateMonitor * monitor)
{
  GstIterator *iterator;
  gboolean done;
  GstPad *pad;
  GstValidateElementMonitor *elem_monitor;
  GstElement *element;

  if (!GST_IS_ELEMENT (GST_VALIDATE_MONITOR_GET_OBJECT (monitor))) {
    GST_WARNING_OBJECT (monitor, "Trying to create element monitor with other "
        "type of object");
    return FALSE;
  }

  elem_monitor = GST_VALIDATE_ELEMENT_MONITOR_CAST (monitor);

  GST_DEBUG_OBJECT (monitor, "Setting up monitor for element %" GST_PTR_FORMAT,
      GST_VALIDATE_MONITOR_GET_OBJECT (monitor));
  element = GST_VALIDATE_ELEMENT_MONITOR_GET_ELEMENT (monitor);

  gst_validate_element_monitor_inspect (elem_monitor);

  elem_monitor->pad_added_id = g_signal_connect (element, "pad-added",
      G_CALLBACK (_validate_element_pad_added), monitor);

  iterator = gst_element_iterate_pads (element);
  done = FALSE;
  while (!done) {
    GValue value = { 0, };

    switch (gst_iterator_next (iterator, &value)) {
      case GST_ITERATOR_OK:
        pad = g_value_get_object (&value);
        gst_validate_element_monitor_wrap_pad (elem_monitor, pad);
        g_value_reset (&value);
        break;
      case GST_ITERATOR_RESYNC:
        /* TODO how to handle this? */
        gst_iterator_resync (iterator);
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iterator);
  return TRUE;
}

static void
gst_validate_element_monitor_wrap_pad (GstValidateElementMonitor * monitor,
    GstPad * pad)
{
  GstValidatePadMonitor *pad_monitor;
  GST_DEBUG_OBJECT (monitor, "Wrapping pad %s:%s", GST_DEBUG_PAD_NAME (pad));

  pad_monitor =
      GST_VALIDATE_PAD_MONITOR (gst_validate_monitor_factory_create (GST_OBJECT
          (pad), GST_VALIDATE_MONITOR_GET_RUNNER (monitor),
          GST_VALIDATE_MONITOR (monitor)));
  g_return_if_fail (pad_monitor != NULL);

  GST_VALIDATE_MONITOR_LOCK (monitor);
  monitor->pad_monitors = g_list_prepend (monitor->pad_monitors, pad_monitor);
  GST_VALIDATE_MONITOR_UNLOCK (monitor);
}

static void
_validate_element_pad_added (GstElement * element, GstPad * pad,
    GstValidateElementMonitor * monitor)
{
  g_return_if_fail (GST_VALIDATE_ELEMENT_MONITOR_GET_ELEMENT (monitor) ==
      element);
  gst_validate_element_monitor_wrap_pad (monitor, pad);
}
