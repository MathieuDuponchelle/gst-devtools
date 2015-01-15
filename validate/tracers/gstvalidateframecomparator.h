#ifndef __GST_VALIDATE_FRAME_COMPARATOR_H__
#define __GST_VALIDATE_FRAME_COMPARATOR_H__

#include <gst/gst.h>
#include <gst/gsttracer.h>

#include "gstvalidatetracer.h"

G_BEGIN_DECLS

#define GST_TYPE_VALIDATE_FRAME_COMPARATOR \
  (gst_validate_frame_comparator_get_type())
#define GST_VALIDATE_FRAME_COMPARATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VALIDATE_FRAME_COMPARATOR,GstFrameComparator))
#define GST_VALIDATE_FRAME_COMPARATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VALIDATE_FRAME_COMPARATOR,GstFrameComparatorClass))
#define GST_IS_VALIDATE_FRAME_COMPARATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VALIDATE_FRAME_COMPARATOR))
#define GST_IS_VALIDATE_FRAME_COMPARATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VALIDATE_FRAME_COMPARATOR))
#define GST_VALIDATE_FRAME_COMPARATOR_CAST(obj) ((GstFrameComparator *)(obj))

typedef struct _GstFrameComparator GstFrameComparator;
typedef struct _GstFrameComparatorClass GstFrameComparatorClass;

/**
 * GstFrameComparator:
 *
 * Opaque #GstFrameComparator data structure
 */
struct _GstFrameComparator {
  GstValidateTracer 	 parent;

  /*< private >*/
};

struct _GstFrameComparatorClass {
  GstValidateTracerClass parent_class;
  /* signals */
};

G_GNUC_INTERNAL GType gst_validate_frame_comparator_get_type (void);


G_END_DECLS

#endif /* __GST_VALIDATE_FRAME_COMPARATOR_H__ */
