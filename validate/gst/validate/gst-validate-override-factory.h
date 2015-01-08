/* GStreamer
 * Copyright (C) 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_VALIDATE_OVERRIDE_FACTORY_H__
#define __GST_VALIDATE_OVERRIDE_FACTORY_H__

#include <gst/validate/validate.h>
#include <gst/gstplugin.h>
#include <gst/gstpluginfeature.h>

G_BEGIN_DECLS
#define GST_TYPE_VALIDATE_OVERRIDE_FACTORY                (gst_validate_override_factory_get_type())
#define GST_VALIDATE_OVERRIDE_FACTORY(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
      GST_TYPE_VALIDATE_OVERRIDE_FACTORY,\
      GstValidateOverrideFactory))
#define GST_VALIDATE_OVERRIDE_FACTORY_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
      GST_TYPE_VALIDATE_OVERRIDE_FACTORY,\
      GstValidateOverrideFactoryClass))
#define GST_IS_VALIDATE_OVERRIDE_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
      GST_TYPE_VALIDATE_OVERRIDE_FACTORY))
#define GST_IS_VALIDATE_OVERRIDE_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE((klass),\
      GST_TYPE_VALIDATE_OVERRIDE_FACTORY))
#define GST_VALIDATE_OVERRIDE_FACTORY_CAST(obj)           ((GstValidateOverrideFactory *)(obj))
/**
 * GstValidateOverrideFactory:
 *
 * The opaque #GstValidateOverrideFactory data structure.
 */
typedef struct _GstValidateOverrideFactory GstValidateOverrideFactory;
typedef struct _GstValidateOverrideFactoryClass GstValidateOverrideFactoryClass;

GType gst_validate_override_factory_get_type (void);
gboolean gst_validate_override_factory_register (GstPlugin * plugin,
    const gchar * name, guint rank, GType type);
GstValidateOverride *gst_validate_override_factory_make (const gchar *
    factoryname, const gchar * name);

#endif /* __GST_VALIDATE_OVERRIDE_FACTORY_H__ */
