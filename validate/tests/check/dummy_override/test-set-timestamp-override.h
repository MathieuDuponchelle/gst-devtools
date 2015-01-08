/*
 * gst-validate test override add metadata
 *
 * Copyright 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>.
 *
 * test-set-timestamp-override.h: Test override that adds metadata to buffers
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


#ifndef __TEST_SET_TIMESTAMP_OVERRIDE_H__
#define __TEST_SET_TIMESTAMP_OVERRIDE_H__

#include <gst/validate/gst-validate-override.h>

G_BEGIN_DECLS

#define GST_TYPE_TEST_SET_TIMESTAMP_OVERRIDE \
  (gst_validate_override_severity_changer_get_type ())
#define GST_TEST_SET_TIMESTAMP_OVERRIDE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TEST_SET_TIMESTAMP_OVERRIDE,TestSetTimestampOverride))
#define GST_TEST_SET_TIMESTAMP_OVERRIDE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TEST_SET_TIMESTAMP_OVERRIDE,TestSetTimestampOverrideClass))
#define GST_IS_TEST_SET_TIMESTAMP_OVERRIDE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TEST_SET_TIMESTAMP_OVERRIDE))
#define GST_IS_TEST_SET_TIMESTAMP_OVERRIDE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TEST_SET_TIMESTAMP_OVERRIDE))

typedef struct _TestSetTimestampOverride          TestSetTimestampOverride;
typedef struct _TestSetTimestampOverrideClass     TestSetTimestampOverrideClass;

/**
 * TestSetTimestampOverride:
 *
 * Opaque #TestSetTimestampOverride data structure.
 */
struct _TestSetTimestampOverride {
  GstValidateOverride      element;
};

struct _TestSetTimestampOverrideClass {
  GstValidateOverrideClass parent_class;
};

G_GNUC_INTERNAL GType   test_set_timestamp_override_get_type        (void);

G_END_DECLS

#endif /* __TEST_SET_TIMESTAMP_OVERRIDE_H__ */
