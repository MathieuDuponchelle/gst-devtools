#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gsttracer.h>
#include "gstvalidateframecomparator.h"
#include "gstvalidator.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_tracer_register (plugin, "framecomparator",
          gst_validate_frame_comparator_get_type ()))
    return FALSE;
  if (!gst_tracer_register (plugin, "validator", gst_validator_get_type ()))
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, validatetracers,
    "GStreamer validate tracers", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
