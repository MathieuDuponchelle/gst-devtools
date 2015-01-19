#include <gst/validate/validate.h>
#include "padmonitor.h"
#include "elementmonitor.h"

static GstPad *
_get_actual_pad (GstPad * pad)
{
  GstPad *tmp_pad;

  gst_object_ref (pad);

  /* We don't monitor ghost pads */
  while (GST_IS_GHOST_PAD (pad)) {
    tmp_pad = pad;
    pad = gst_ghost_pad_get_target ((GstGhostPad *) pad);
    gst_object_unref (tmp_pad);
  }

  while (GST_IS_PROXY_PAD (pad)) {
    tmp_pad = pad;
    pad = gst_pad_get_peer (pad);
    gst_object_unref (tmp_pad);
  }

  return pad;
}

static gboolean
_find_master_report_on_pad (GstPad * pad, GstValidateReport * report,
    const gchar * type_name)
{
  PadMonitor *pad_monitor;
  GstValidateReport *prev_report;
  gboolean result = FALSE;
  GstPad *tmppad = pad;

  pad = _get_actual_pad (pad);
  if (pad == NULL) {
    GST_ERROR_OBJECT (tmppad, "Does not have a target yet");

    return FALSE;
  }

  pad_monitor =
      PAD_MONITOR (get_monitor_for_type_name (G_OBJECT (pad), type_name));

  /* For some reason this pad isn't monitored */
  if (pad_monitor == NULL)
    goto done;

  prev_report = gst_validate_reporter_get_report ((GstValidateReporter *)
      pad_monitor, report->issue->issue_id);

  if (prev_report) {
    if (prev_report->master_report)
      result = gst_validate_report_set_master_report (report,
          prev_report->master_report);
    else
      result = gst_validate_report_set_master_report (report, prev_report);
  }

done:
  gst_object_unref (pad);

  return result;
}

static gboolean
_find_master_report_for_sink_pad (PadMonitor * pad_monitor,
    GstValidateReport * report)
{
  GstPad *peerpad;
  gboolean result = FALSE;

  peerpad = gst_pad_get_peer (PAD_MONITOR_GET_PAD (pad_monitor));

  /* If the peer src pad already has a similar report no need to look
   * any further */
  if (peerpad
      && _find_master_report_on_pad (peerpad, report,
          g_type_name (G_OBJECT_TYPE (MONITOR (pad_monitor)->tracer))))
    result = TRUE;

  if (peerpad)
    gst_object_unref (peerpad);

  return result;
}

static gboolean
_find_master_report_for_src_pad (PadMonitor * pad_monitor,
    GstValidateReport * report)
{
  GstIterator *iter;
  gboolean done;
  GstPad *pad;
  gboolean result = FALSE;

  iter = gst_pad_iterate_internal_links (PAD_MONITOR_GET_PAD (pad_monitor));
  done = FALSE;
  while (!done) {
    GValue value = { 0, };
    switch (gst_iterator_next (iter, &value)) {
      case GST_ITERATOR_OK:
        pad = g_value_get_object (&value);

        if (_find_master_report_on_pad (pad, report,
                g_type_name (G_OBJECT_TYPE (MONITOR (pad_monitor)->tracer)))) {
          result = TRUE;
          done = TRUE;
        }

        g_value_reset (&value);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING_OBJECT (PAD_MONITOR_GET_PAD (pad_monitor),
            "Internal links pad iteration error");
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iter);

  return result;
}

static GstValidateInterceptionReturn
_concatenate_issues (PadMonitor * pad_monitor, GstValidateReport * report)
{
  if (GST_PAD_IS_SINK (PAD_MONITOR_GET_PAD (pad_monitor))
      && _find_master_report_for_sink_pad (pad_monitor, report))
    return GST_VALIDATE_REPORTER_KEEP;
  else if (GST_PAD_IS_SRC (PAD_MONITOR_GET_PAD (pad_monitor))
      && _find_master_report_for_src_pad (pad_monitor, report))
    return GST_VALIDATE_REPORTER_KEEP;

  return GST_VALIDATE_REPORTER_REPORT;
}

static GstValidateInterceptionReturn
gst_validate_pad_monitor_intercept_report (GstValidateReporter *
    reporter, GstValidateReport * report)
{
  GstValidateReporterInterface *iface_class, *old_iface_class;
  PadMonitor *pad_monitor = PAD_MONITOR (reporter);
  GstValidateInterceptionReturn ret;

  iface_class =
      G_TYPE_INSTANCE_GET_INTERFACE (reporter, GST_TYPE_VALIDATE_REPORTER,
      GstValidateReporterInterface);
  old_iface_class = g_type_interface_peek_parent (iface_class);

  old_iface_class->intercept_report (reporter, report);

  ret = _concatenate_issues (pad_monitor, report);
  return ret;
}

static void
_reporter_iface_init (GstValidateReporterInterface * iface)
{
  iface->intercept_report = gst_validate_pad_monitor_intercept_report;
}

#define _do_init \
  G_IMPLEMENT_INTERFACE (GST_TYPE_VALIDATE_REPORTER, _reporter_iface_init)

#define pad_monitor_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (PadMonitor, pad_monitor, TYPE_MONITOR, _do_init);

static void
_parent_name_changed_cb (GObject * object, GParamSpec * pspec,
    PadMonitor * pmonitor)
{
  gchar *name = g_strdup_printf ("%s:%s",
      GST_DEBUG_PAD_NAME (PAD_MONITOR_GET_PAD (pmonitor)));
  gst_validate_reporter_set_name (GST_VALIDATE_REPORTER (pmonitor), name);
}

static void
_constructed (GObject * object)
{
  PadMonitor *monitor = PAD_MONITOR (object);
  gchar *name = g_strdup_printf ("%s:%s",
      GST_DEBUG_PAD_NAME (PAD_MONITOR_GET_PAD (monitor)));
  GstElement *element =
      GST_ELEMENT (gst_pad_get_parent (PAD_MONITOR_GET_PAD (monitor)));

  g_signal_connect (element, "notify::name",
      G_CALLBACK (_parent_name_changed_cb), object);
  gst_validate_reporter_set_name (GST_VALIDATE_REPORTER (monitor), name);
  ((GObjectClass *) parent_class)->constructed (object);
}

static void
pad_monitor_class_init (PadMonitorClass * klass)
{
  G_OBJECT_CLASS (klass)->constructed = _constructed;
}


static void
pad_monitor_init (PadMonitor * pad_monitor)
{
}
