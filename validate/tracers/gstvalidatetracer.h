#ifndef __GST_VALIDATE_TRACER_H__
#define __GST_VALIDATE_TRACER_H__

#include <gst/gst.h>
#include <gst/gsttracer.h>
#include <gst/validate/gst-validate-runner.h>
#include "padmonitor.h"

G_BEGIN_DECLS

#define GST_TYPE_VALIDATE_TRACER \
  (gst_validate_tracer_get_type())
#define GST_VALIDATE_TRACER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VALIDATE_TRACER,GstValidateTracer))
#define GST_VALIDATE_TRACER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VALIDATE_TRACER,GstValidateTracerClass))
#define GST_IS_VALIDATE_TRACER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VALIDATE_TRACER))
#define GST_IS_VALIDATE_TRACER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VALIDATE_TRACER))
#define GST_VALIDATE_TRACER_CAST(obj) ((GstValidateTracer *)(obj))

typedef struct _GstValidateTracer GstValidateTracer;
typedef struct _GstValidateTracerClass GstValidateTracerClass;

/**
 * GstValidateTracer:
 *
 * Opaque #GstValidateTracer data structure
 */
struct _GstValidateTracer {
  GstTracer 	 parent;

  /*< private >*/
  GstValidateRunner *runner;
};

typedef PadMonitor * (*GstValidateTracerMonitorPadFunc) (GstTracer *, GstPad *);

struct _GstValidateTracerClass {
  GstTracerClass parent_class;

  GstValidateTracerMonitorPadFunc monitor_pad;
  /* signals */
};

G_GNUC_INTERNAL GType gst_validate_tracer_get_type (void);

gboolean gst_validate_tracer_parse_structure (GstValidateTracer *self, GstStructure * structure);
void     gst_validate_tracer_set_runner (GstValidateTracer *self, GstValidateRunner *runner);

G_END_DECLS

#endif /* __GST_VALIDATE_TRACER_H__ */
