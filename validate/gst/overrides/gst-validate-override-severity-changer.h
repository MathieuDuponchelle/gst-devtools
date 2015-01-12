/*
 * gst-validate severity changer override
 *
 * Copyright 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>.
 *
 * gst-validate-severity-changer.h: Override that changes severity of reports
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */


#ifndef __GST_VALIDATE_OVERRIDE_SEVERITY_CHANGER_H__
#define __GST_VALIDATE_OVERRIDE_SEVERITY_CHANGER_H__

#include <gst/validate/gst-validate-override.h>

G_BEGIN_DECLS

#define GST_TYPE_VALIDATE_OVERRIDE_SEVERITY_CHANGER \
  (gst_validate_override_severity_changer_get_type ())
#define GST_VALIDATE_OVERRIDE_SEVERITY_CHANGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VALIDATE_OVERRIDE_SEVERITY_CHANGER,GstValidateOverrideSeverityChanger))
#define GST_VALIDATE_OVERRIDE_SEVERITY_CHANGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VALIDATE_OVERRIDE_SEVERITY_CHANGER,GstValidateOverrideSeverityChangerClass))
#define GST_IS_VALIDATE_OVERRIDE_SEVERITY_CHANGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VALIDATE_OVERRIDE_SEVERITY_CHANGER))
#define GST_IS_VALIDATE_OVERRIDE_SEVERITY_CHANGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VALIDATE_OVERRIDE_SEVERITY_CHANGER))

typedef struct _GstValidateOverrideSeverityChanger          GstValidateOverrideSeverityChanger;
typedef struct _GstValidateOverrideSeverityChangerClass     GstValidateOverrideSeverityChangerClass;

/**
 * GstValidateOverrideSeverityChanger:
 *
 * Opaque #GstValidateOverrideSeverityChanger data structure.
 */
struct _GstValidateOverrideSeverityChanger {
  GstValidateOverride      element;
};

struct _GstValidateOverrideSeverityChangerClass {
  GstValidateOverrideClass parent_class;
};

G_GNUC_INTERNAL GType   gst_validate_override_severity_changer_get_type        (void);

G_END_DECLS

#endif /* __GST_VALIDATE_OVERRIDE_SEVERITY_CHANGER_H__ */
