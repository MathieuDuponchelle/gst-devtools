#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gsttracer.h>
#include <gst/validate/validate.h>
#include "gstvalidateseqnumchecker.h"
#include "padmonitor.h"

#undef EOS_HAS_WRONG_SEQNUM
#define EOS_HAS_WRONG_SEQNUM g_quark_from_static_string ("seqnum-checker::eos-has-wrong-seqnum")

typedef struct _SeqnumPadMonitor SeqnumPadMonitor;
typedef struct _SeqnumPadMonitorClass SeqnumPadMonitorClass;

#define TYPE_SEQNUM_PAD_MONITOR			(seqnum_pad_monitor_get_type ())
#define SEQNUM_PAD_MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SEQNUM_PAD_MONITOR, SeqnumPadMonitor))

struct _SeqnumPadMonitor
{
  PadMonitor parent;

  guint pending_newsegment_seqnum;
  guint pending_eos_seqnum;
};

struct _SeqnumPadMonitorClass
{
  PadMonitorClass parent_class;
};

GType seqnum_pad_monitor_get_type (void);

G_DEFINE_TYPE (SeqnumPadMonitor, seqnum_pad_monitor, TYPE_PAD_MONITOR);

static void
seqnum_pad_monitor_class_init (SeqnumPadMonitorClass * klass)
{
}

static void
seqnum_pad_monitor_init (SeqnumPadMonitor * pad_monitor)
{
}

GST_DEBUG_CATEGORY_STATIC (gst_validate_seqnum_checker_debug);
#define GST_CAT_DEFAULT gst_validate_seqnum_checker_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_validate_seqnum_checker_debug, "seqnumchecker", 0, "validate seqnum checker");

#define gst_validate_seqnum_checker_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstValidateSeqnumChecker, gst_validate_seqnum_checker,
    GST_TYPE_VALIDATE_TRACER, _do_init);

static PadMonitor *
_monitor_pad (GstTracer * tracer, GstPad * pad)
{
  return g_object_new (seqnum_pad_monitor_get_type (), "object", pad,
      "validate-runner", GST_VALIDATE_TRACER (tracer)->runner, NULL);
}

static void
gst_validate_seqnum_checker_class_init (GstValidateSeqnumCheckerClass * klass)
{
  GstValidateTracerClass *tracer_class = GST_VALIDATE_TRACER_CLASS (klass);

  tracer_class->monitor_pad = _monitor_pad;
}

static void
do_pad_push_event_pre (GstTracer * tracer, guint64 ts, GstPad * pad,
    GstEvent * event)
{
  SeqnumPadMonitor *spmonitor =
      SEQNUM_PAD_MONITOR (get_monitor_for_type_name (G_OBJECT (pad),
          g_type_name (G_OBJECT_TYPE (tracer))));
  guint32 seqnum = gst_event_get_seqnum (event);

  if (spmonitor == NULL)
    return;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
      if (spmonitor->pending_newsegment_seqnum) {
        if (spmonitor->pending_newsegment_seqnum == seqnum) {
          spmonitor->pending_newsegment_seqnum = 0;
        } else {
          GST_ERROR ("something's fucky");
        }
      }

      spmonitor->pending_eos_seqnum = seqnum;
      break;
    case GST_EVENT_EOS:
      if (spmonitor->pending_eos_seqnum == 0) {
        GST_ERROR ("double lol");
      } else if (spmonitor->pending_eos_seqnum != seqnum) {
        GST_VALIDATE_REPORT (spmonitor, EOS_HAS_WRONG_SEQNUM,
            "Got : %u. Expected: %u", seqnum, spmonitor->pending_eos_seqnum);
        GST_ERROR_OBJECT (pad, "cool that's an error");
      }
      break;
    case GST_EVENT_FLUSH_STOP:
      spmonitor->pending_eos_seqnum = seqnum;
      spmonitor->pending_newsegment_seqnum = seqnum;
    default:
      break;
  }
}

static void
do_pad_push_event_post (GstTracer * tracer, guint64 ts, GstPad * pad,
    GstEvent * event, GstFlowReturn result)
{
  SeqnumPadMonitor *spmonitor =
      SEQNUM_PAD_MONITOR (get_monitor_for_type_name (G_OBJECT (pad),
          g_type_name (G_OBJECT_TYPE (tracer))));

  if (spmonitor == NULL)
    return;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_QOS:
    case GST_EVENT_SEEK:
    {
      if (result == FALSE) {
        spmonitor->pending_newsegment_seqnum = 0;
        spmonitor->pending_eos_seqnum = 0;
      }
    }
      break;
    case GST_EVENT_EOS:
      break;
    default:
      break;
  }
}

static void
gst_validate_seqnum_checker_init (GstValidateSeqnumChecker * self)
{
  GstTracer *tracer = GST_TRACER (self);
  (void) tracer;

  gst_validate_issue_register (gst_validate_issue_new (EOS_HAS_WRONG_SEQNUM,
          "EOS has wrong seqnum",
          "Got an EOS with a wrong seqnum", GST_VALIDATE_REPORT_LEVEL_ISSUE));

  gst_tracing_register_hook (tracer, "pad-push-event-pre",
      G_CALLBACK (do_pad_push_event_pre));
  gst_tracing_register_hook (tracer, "pad-send-event-pre",
      G_CALLBACK (do_pad_push_event_pre));
  gst_tracing_register_hook (tracer, "pad-push-event-post",
      G_CALLBACK (do_pad_push_event_post));
}
