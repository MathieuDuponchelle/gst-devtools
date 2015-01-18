#ifndef __GST_VALIDATOR_H__
#define __GST_VALIDATOR_H__

#include <gst/gst.h>
#include <gst/gsttracer.h>

G_BEGIN_DECLS

#define GST_TYPE_VALIDATOR \
  (gst_validator_get_type())
#define GST_VALIDATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VALIDATOR,GstValidator))
#define GST_VALIDATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VALIDATOR,GstValidatorClass))
#define GST_IS_VALIDATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VALIDATOR))
#define GST_IS_VALIDATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VALIDATOR))
#define GST_VALIDATOR_CAST(obj) ((GstValidator *)(obj))

typedef struct _GstValidator GstValidator;
typedef struct _GstValidatorClass GstValidatorClass;

/**
 * GstValidator:
 *
 * Opaque #GstValidator data structure
 */
struct _GstValidator {
  GstTracer 	 parent;

  /*< private >*/
  GHashTable *tracers;
};

struct _GstValidatorClass {
  GstTracerClass parent_class;
  /* signals */
};

G_GNUC_INTERNAL GType gst_validator_get_type (void);

G_END_DECLS

#endif /* __GST_VALIDATOR_H__ */
