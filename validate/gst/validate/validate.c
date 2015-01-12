/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thiago Sousa Santos <thiago.sousa.santos@collabora.com>
 *
 * validate.c - Validate generic functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:validate
 * @short_description: Initialize GstValidate
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "validate.h"
#include "gst-validate-internal.h"

GST_DEBUG_CATEGORY (gstvalidate_debug);

static GMutex _gst_validate_registry_mutex;
static GstRegistry *_gst_validate_registry_default = NULL;

gboolean
gst_validate_scan_plugin_path (GstRegistry * registry)
{
  const gchar *plugin_path = g_getenv ("GST_VALIDATE_PLUGIN_PATH");

  if (plugin_path) {
    gint i;

    gchar **list = g_strsplit (plugin_path, G_SEARCHPATH_SEPARATOR_S, 0);

    for (i = 0; list[i]; i++) {
      GST_INFO ("scanning path %s", list[i]);
      gst_registry_scan_path (registry, list[i]);
    }
  }

  return TRUE;
}

GstRegistry *
gst_validate_registry_get (void)
{
  GstRegistry *registry;

  g_mutex_lock (&_gst_validate_registry_mutex);
  if (G_UNLIKELY (!_gst_validate_registry_default)) {
    _gst_validate_registry_default = g_object_newv (GST_TYPE_REGISTRY, 0, NULL);
    gst_object_ref_sink (GST_OBJECT_CAST (_gst_validate_registry_default));
  }
  registry = _gst_validate_registry_default;
  g_mutex_unlock (&_gst_validate_registry_mutex);

  return registry;
}

/**
 * gst_validate_init:
 *
 * Initializes GstValidate, call that before any usage of GstValidate.
 * You should take care of initilizing GStreamer before calling this
 * function.
 */
void
gst_validate_init (void)
{
  GST_DEBUG_CATEGORY_INIT (gstvalidate_debug, "validate", 0,
      "Validation library");

  /* init the report system (can be called multiple times) */
  gst_validate_report_init ();

  /* Init the scenario system */
  init_scenarios ();

  gst_registry_fork_set_enabled (FALSE);
  gst_validate_registry_get ();
  gst_validate_scan_plugin_path (_gst_validate_registry_default);
  gst_registry_fork_set_enabled (TRUE);

  /* Ensure we load overrides before any use of a monitor */
  gst_validate_override_parser_preload ();
}
