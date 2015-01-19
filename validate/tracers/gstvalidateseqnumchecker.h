#ifndef __GST_VALIDATE_SEQNUM_CHECKER_H__
#define __GST_VALIDATE_SEQNUM_CHECKER_H__

#include <gst/gst.h>
#include <gst/gsttracer.h>

#include "gstvalidatetracer.h"

G_BEGIN_DECLS

#define GST_TYPE_VALIDATE_SEQNUM_CHECKER \
  (gst_validate_seqnum_checker_get_type())
#define GST_VALIDATE_SEQNUM_CHECKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VALIDATE_SEQNUM_CHECKER,GstValidateSeqnumChecker))
#define GST_VALIDATE_SEQNUM_CHECKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VALIDATE_SEQNUM_CHECKER,GstValidateSeqnumCheckerClass))
#define GST_IS_VALIDATE_SEQNUM_CHECKER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VALIDATE_SEQNUM_CHECKER))
#define GST_IS_VALIDATE_SEQNUM_CHECKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VALIDATE_SEQNUM_CHECKER))
#define GST_VALIDATE_SEQNUM_CHECKER_CAST(obj) ((GstValidateSeqnumChecker *)(obj))

typedef struct _GstValidateSeqnumChecker GstValidateSeqnumChecker;
typedef struct _GstValidateSeqnumCheckerClass GstValidateSeqnumCheckerClass;

/**
 * GstValidateSeqnumChecker:
 *
 * Opaque #GstValidateSeqnumChecker data structure
 */
struct _GstValidateSeqnumChecker {
  GstValidateTracer 	 parent;

  /*< private >*/
};

struct _GstValidateSeqnumCheckerClass {
  GstValidateTracerClass parent_class;
  /* signals */
};

G_GNUC_INTERNAL GType gst_validate_seqnum_checker_get_type (void);


G_END_DECLS

#endif /* __GST_VALIDATE_SEQNUM_CHECKER_H__ */
