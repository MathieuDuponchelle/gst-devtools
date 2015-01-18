#include <gst/gst.h>
#include <string.h>

#include "elementmonitor.h"


#define element_monitor_parent_class parent_class
G_DEFINE_TYPE (ElementMonitor, element_monitor, TYPE_MONITOR);

static void
element_monitor_class_init (ElementMonitorClass * klass)
{
}

static void
element_monitor_init (ElementMonitor * element_monitor)
{
}

void
element_monitor_inspect (ElementMonitor * monitor)
{
  GstElement *element;
  GstElementClass *klass;
  const gchar *klassname;

  element = GST_ELEMENT (MONITOR (monitor)->target);
  klass = GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS (element));

  klassname =
      gst_element_class_get_metadata (klass, GST_ELEMENT_METADATA_KLASS);
  if (klassname) {
    monitor->is_decoder = strstr (klassname, "Decoder") != NULL;
    monitor->is_encoder = strstr (klassname, "Encoder") != NULL;
    monitor->is_demuxer = strstr (klassname, "Demuxer") != NULL;
    monitor->is_converter = strstr (klassname, "Converter") != NULL;
  } else
    GST_ERROR_OBJECT (element, "no klassname");
}
