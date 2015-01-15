#ifndef __GST_VALIDATE_TRACER_H__
#define __GST_VALIDATE_TRACER_H__

#include <gst/gst.h>
#include <gst/gsttracer.h>

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
};

typedef gpointer (*GstValidateTracerCreateMonitorData) (GstTracer *, GstObject *);

struct _GstValidateTracerClass {
  GstTracerClass parent_class;

  GstValidateTracerCreateMonitorData create_monitor_data;
  /* signals */
};

G_GNUC_INTERNAL GType gst_validate_tracer_get_type (void);

gpointer gst_validate_tracer_get_monitor_data (GstTracer *self, GstObject *object);

G_END_DECLS

#endif /* __GST_VALIDATE_TRACER_H__ */
