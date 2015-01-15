/*
 * gst-validate factory checker override
 *
 * Copyright 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>.
 *
 * gst-validate-factory-checker.h: Override that checks whether an
 * element added to the pipeline, for instance a decoder, actually
 * matches the required factory, for example if one wants to make
 * sure hardware decoders are used.
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


#ifndef __GST_VALIDATE_OVERRIDE_FACTORY_CHECKER_H__
#define __GST_VALIDATE_OVERRIDE_FACTORY_CHECKER_H__

#include <gst/validate/gst-validate-override.h>

G_BEGIN_DECLS

#define GST_TYPE_VALIDATE_OVERRIDE_FACTORY_CHECKER \
  (gst_validate_override_factory_checker_get_type ())
#define GST_VALIDATE_OVERRIDE_FACTORY_CHECKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VALIDATE_OVERRIDE_FACTORY_CHECKER,GstValidateOverrideFactoryChecker))
#define GST_VALIDATE_OVERRIDE_FACTORY_CHECKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VALIDATE_OVERRIDE_FACTORY_CHECKER,GstValidateOverrideFactoryCheckerClass))
#define GST_IS_VALIDATE_OVERRIDE_FACTORY_CHECKER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VALIDATE_OVERRIDE_FACTORY_CHECKER))
#define GST_IS_VALIDATE_OVERRIDE_FACTORY_CHECKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VALIDATE_OVERRIDE_FACTORY_CHECKER))

typedef struct _GstValidateOverrideFactoryChecker          GstValidateOverrideFactoryChecker;
typedef struct _GstValidateOverrideFactoryCheckerClass     GstValidateOverrideFactoryCheckerClass;

/**
 * GstValidateOverrideFactoryChecker:
 *
 * Opaque #GstValidateOverrideFactoryChecker data structure.
 */
struct _GstValidateOverrideFactoryChecker {
  GstValidateOverride      element;
};

struct _GstValidateOverrideFactoryCheckerClass {
  GstValidateOverrideClass parent_class;
};

G_GNUC_INTERNAL GType   gst_validate_override_factory_checker_get_type        (void);

G_END_DECLS

#endif /* __GST_VALIDATE_OVERRIDE_FACTORY_CHECKER_H__ */
