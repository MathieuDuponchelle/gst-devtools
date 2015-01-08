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

#include <glib.h>
#include <gst/gstregistrychunks.h>
#include "gst-validate-override.h"
#include "gst-validate-override-factory.h"


struct _GstValidateOverrideFactoryClass
{
  GstPluginFeatureClass parent_class;
  gpointer _gst_reserved[GST_PADDING];
};

struct _GstValidateOverrideFactory
{
  GstPluginFeature parent;
  GType type;
  gpointer _gst_reserved[GST_PADDING];
};

typedef struct _GstRegistryChunkValidateOverrideFactory
{
  GstRegistryChunkPluginFeature plugin_feature;
} GstRegistryChunkValidateOverrideFactory;

G_DEFINE_TYPE (GstValidateOverrideFactory, gst_validate_override_factory,
    GST_TYPE_PLUGIN_FEATURE);

static void
gst_validate_override_factory_class_init (GstValidateOverrideFactoryClass *
    klass)
{
}

static void
gst_validate_override_factory_init (GstValidateOverrideFactory * factory)
{
}

/**
 * gst_validate_override_factory_register:
 * @plugin: (allow-none): #GstPlugin to register the override with, or %NULL for
 *     a static override.
 * @name: name of overrides of this type
 * @type: GType of override to register
 *
 * Create a new overridefactory capable of instantiating objects of the
 * @type and add the factory to @plugin.
 *
 * Returns: %TRUE, if the registering succeeded, %FALSE on error
 */
gboolean
gst_validate_override_factory_register (GstPlugin * plugin, const gchar * name,
    guint rank, GType type)
{
  GstPluginFeature *existing_feature;
  GstRegistry *registry;
  GstValidateOverrideFactory *factory;

  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (g_type_is_a (type, GST_TYPE_VALIDATE_OVERRIDE), FALSE);

  registry = gst_validate_registry_get ();

  /* check if feature already exists, if it exists there is no need to update it
   * when the registry is getting updated, outdated plugins and all their
   * features are removed and readded.
   */
  existing_feature = gst_registry_lookup_feature (registry, name);
  if (existing_feature) {
    GST_DEBUG_OBJECT (registry, "update existing feature %p (%s)",
        existing_feature, name);
    factory = GST_VALIDATE_OVERRIDE_FACTORY_CAST (existing_feature);
    factory->type = type;
    gst_plugin_feature_set_loaded (existing_feature, TRUE);
    gst_object_unref (existing_feature);
    return TRUE;
  }

  factory =
      GST_VALIDATE_OVERRIDE_FACTORY_CAST (g_object_newv
      (GST_TYPE_VALIDATE_OVERRIDE_FACTORY, 0, NULL));

  gst_plugin_feature_set_name (GST_PLUGIN_FEATURE_CAST (factory), name);
  GST_LOG_OBJECT (factory, "Created new overridefactory for type %s",
      g_type_name (type));

  factory->type = type;

  if (plugin && gst_plugin_get_name (plugin)) {
    gst_plugin_feature_set_plugin (GST_PLUGIN_FEATURE_CAST (factory), plugin);
  } else {
    gst_plugin_feature_set_plugin (GST_PLUGIN_FEATURE_CAST (factory), NULL);
  }
  gst_plugin_feature_set_rank (GST_PLUGIN_FEATURE_CAST (factory), rank);
  gst_plugin_feature_set_loaded (GST_PLUGIN_FEATURE_CAST (factory), TRUE);

  return gst_registry_add_feature (registry, GST_PLUGIN_FEATURE_CAST (factory));
}

static GstValidateOverride *
gst_validate_override_factory_create (GstValidateOverrideFactory * factory,
    const gchar * name)
{
  GstValidateOverride *override;
  GstValidateOverrideFactory *newfactory;

  g_return_val_if_fail (factory != NULL, NULL);

  newfactory =
      GST_VALIDATE_OVERRIDE_FACTORY (gst_plugin_feature_load (GST_PLUGIN_FEATURE
          (factory)));

  if (newfactory == NULL)
    goto load_failed;

  factory = newfactory;

  if (name)
    GST_INFO ("creating override \"%s\" named \"%s\"",
        GST_OBJECT_NAME (factory), GST_STR_NULL (name));
  else
    GST_INFO ("creating override \"%s\"", GST_OBJECT_NAME (factory));

  if (factory->type == 0)
    goto no_type;

  override =
      GST_VALIDATE_OVERRIDE_CAST (g_object_newv (factory->type, 0, NULL));

  if (G_UNLIKELY (override == NULL))
    goto no_element;

  GST_DEBUG ("created override \"%s\"", GST_OBJECT_NAME (factory));

  return override;

  /* ERRORS */
load_failed:
  {
    GST_WARNING_OBJECT (factory,
        "loading plugin containing feature %s returned NULL!", name);
    return NULL;
  }
no_type:
  {
    GST_WARNING_OBJECT (factory, "factory has no type");
    gst_object_unref (factory);
    return NULL;
  }
no_element:
  {
    GST_WARNING_OBJECT (factory, "could not create override");
    gst_object_unref (factory);
    return NULL;
  }
}

static GstValidateOverrideFactory *
gst_validate_override_factory_find (const gchar * name)
{
  GstPluginFeature *feature;

  g_return_val_if_fail (name != NULL, NULL);

  feature = gst_registry_find_feature (gst_validate_registry_get (), name,
      GST_TYPE_VALIDATE_OVERRIDE_FACTORY);

  if (feature)
    return GST_VALIDATE_OVERRIDE_FACTORY (feature);

  /* this isn't an error, for instance when you query if an element factory is
   * present */
  GST_ERROR ("no such element factory \"%s\"", name);
  return NULL;
}

GstValidateOverride *
gst_validate_override_factory_make (const gchar * factoryname,
    const gchar * name)
{
  GstValidateOverrideFactory *factory;
  GstValidateOverride *override;

  g_return_val_if_fail (factoryname != NULL, NULL);
  g_return_val_if_fail (gst_is_initialized (), NULL);

  factory = gst_validate_override_factory_find (factoryname);
  if (factory == NULL)
    goto no_factory;

  GST_LOG_OBJECT (factory, "found factory %p", factory);
  override = gst_validate_override_factory_create (factory, name);
  if (override == NULL)
    goto create_failed;

  gst_object_unref (factory);
  return override;

  /* ERRORS */
no_factory:
  {
    GST_ERROR ("no such override factory \"%s\"!", factoryname);
    return NULL;
  }
create_failed:
  {
    GST_ERROR_OBJECT (factory, "couldn't create instance!");
    gst_object_unref (factory);
    return NULL;
  }
}
