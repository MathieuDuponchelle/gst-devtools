/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thiago Sousa Santos <thiago.sousa.santos@collabora.com>
 *
 * gst-validate-override-parser.c - Validate Override Parser
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <gmodule.h>

#include "gst-validate-utils.h"
#include "gst-validate-internal.h"
#include "gst-validate-override-parser.h"
#include "gst-validate-override-factory.h"


typedef struct
{
  gchar *name;
  GstValidateOverride *override;
} GstValidateOverrideParserNameEntry;

typedef struct
{
  GType gtype;
  GstValidateOverride *override;
} GstValidateOverrideParserGTypeEntry;

static GMutex _gst_validate_override_parser_mutex;
static GstValidateOverrideParser *_parser_default;

#define GST_VALIDATE_OVERRIDE_PARSER_LOCK(r) g_mutex_lock (&r->mutex)
#define GST_VALIDATE_OVERRIDE_PARSER_UNLOCK(r) g_mutex_unlock (&r->mutex)

#define GST_VALIDATE_OVERRIDE_INIT_SYMBOL "gst_validate_create_overrides"
typedef int (*GstValidateCreateOverride) (void);

static GstValidateOverrideParser *
gst_validate_override_parser_new (void)
{
  GstValidateOverrideParser *reg = g_slice_new0 (GstValidateOverrideParser);

  g_mutex_init (&reg->mutex);
  g_queue_init (&reg->global_overrides);
  g_queue_init (&reg->name_overrides);
  g_queue_init (&reg->gtype_overrides);
  g_queue_init (&reg->klass_overrides);

  return reg;
}

GstValidateOverrideParser *
gst_validate_override_parser_get (void)
{
  g_mutex_lock (&_gst_validate_override_parser_mutex);
  if (G_UNLIKELY (!_parser_default)) {
    _parser_default = gst_validate_override_parser_new ();
  }
  g_mutex_unlock (&_gst_validate_override_parser_mutex);

  return _parser_default;
}

void
gst_validate_override_add_by_name (const gchar * name,
    GstValidateOverride * override)
{
  GstValidateOverrideParser *parser = gst_validate_override_parser_get ();
  GstValidateOverrideParserNameEntry *entry =
      g_slice_new (GstValidateOverrideParserNameEntry);

  GST_VALIDATE_OVERRIDE_PARSER_LOCK (parser);
  entry->name = g_strdup (name);
  entry->override = override;
  g_queue_push_tail (&parser->name_overrides, entry);
  GST_VALIDATE_OVERRIDE_PARSER_UNLOCK (parser);
}

void
gst_validate_override_add_by_type (GType gtype, GstValidateOverride * override)
{
  GstValidateOverrideParser *parser = gst_validate_override_parser_get ();
  GstValidateOverrideParserGTypeEntry *entry =
      g_slice_new (GstValidateOverrideParserGTypeEntry);

  GST_VALIDATE_OVERRIDE_PARSER_LOCK (parser);
  entry->gtype = gtype;
  entry->override = override;
  g_queue_push_tail (&parser->gtype_overrides, entry);
  GST_VALIDATE_OVERRIDE_PARSER_UNLOCK (parser);
}

void
gst_validate_override_add_by_klass (const gchar * klass,
    GstValidateOverride * override)
{
  GstValidateOverrideParser *parser = gst_validate_override_parser_get ();
  GstValidateOverrideParserNameEntry *entry =
      g_slice_new (GstValidateOverrideParserNameEntry);

  GST_VALIDATE_OVERRIDE_PARSER_LOCK (parser);
  entry->name = g_strdup (klass);
  entry->override = override;
  g_queue_push_tail (&parser->klass_overrides, entry);
  GST_VALIDATE_OVERRIDE_PARSER_UNLOCK (parser);
}

static void
gst_validate_override_add_global (GstValidateOverride * override)
{
  GstValidateOverrideParser *parser = gst_validate_override_parser_get ();

  GST_VALIDATE_OVERRIDE_PARSER_LOCK (parser);
  g_queue_push_tail (&parser->global_overrides, override);
  GST_VALIDATE_OVERRIDE_PARSER_UNLOCK (parser);
}

static void
    gst_validate_override_parser_attach_name_overrides_unlocked
    (GstValidateOverrideParser * parser, GstValidateMonitor * monitor)
{
  GstValidateOverrideParserNameEntry *entry;
  GList *iter;
  const gchar *name;

  name = gst_validate_monitor_get_element_name (monitor);
  for (iter = parser->name_overrides.head; iter; iter = g_list_next (iter)) {
    entry = iter->data;
    if (g_strcmp0 (name, entry->name) == 0) {
      gst_validate_monitor_attach_override (monitor, entry->override);
    }
  }
}

static void
    gst_validate_override_parser_attach_gtype_overrides_unlocked
    (GstValidateOverrideParser * parser, GstValidateMonitor * monitor)
{
  GstValidateOverrideParserGTypeEntry *entry;
  GstElement *element;
  GList *iter;

  element = gst_validate_monitor_get_element (monitor);
  if (!element)
    return;

  for (iter = parser->gtype_overrides.head; iter; iter = g_list_next (iter)) {
    entry = iter->data;
    if (G_TYPE_CHECK_INSTANCE_TYPE (element, entry->gtype)) {
      gst_validate_monitor_attach_override (monitor, entry->override);
    }
  }
}

static void
    gst_validate_override_parser_attach_global_overrides_unlocked
    (GstValidateOverrideParser * parser, GstValidateMonitor * monitor)
{
  GList *iter;

  for (iter = parser->global_overrides.head; iter; iter = g_list_next (iter)) {
    GstValidateOverride *override;

    override = iter->data;

    gst_validate_monitor_attach_override (monitor, override);
  }
}

static void
    gst_validate_override_parser_attach_klass_overrides_unlocked
    (GstValidateOverrideParser * parser, GstValidateMonitor * monitor)
{
  GstValidateOverrideParserNameEntry *entry;
  GList *iter;
  GstElement *element;
  GstElementClass *klass;
  const gchar *klassname;

  element = gst_validate_monitor_get_element (monitor);
  if (!element)
    return;

  klass = GST_ELEMENT_GET_CLASS (element);
  klassname =
      gst_element_class_get_metadata (klass, GST_ELEMENT_METADATA_KLASS);

  for (iter = parser->name_overrides.head; iter; iter = g_list_next (iter)) {

    entry = iter->data;

    /* TODO It would be more correct to split it before comparing */
    if (strstr (klassname, entry->name) != NULL) {
      gst_validate_monitor_attach_override (monitor, entry->override);
    }
  }
}

void
gst_validate_override_parser_attach_overrides (GstValidateMonitor * monitor)
{
  GstValidateOverrideParser *reg = gst_validate_override_parser_get ();

  GST_VALIDATE_OVERRIDE_PARSER_LOCK (reg);
  gst_validate_override_parser_attach_global_overrides_unlocked (reg, monitor);
  gst_validate_override_parser_attach_name_overrides_unlocked (reg, monitor);
  gst_validate_override_parser_attach_gtype_overrides_unlocked (reg, monitor);
  gst_validate_override_parser_attach_klass_overrides_unlocked (reg, monitor);
  GST_VALIDATE_OVERRIDE_PARSER_UNLOCK (reg);
}

enum
{
  WRONG_FILE,
  WRONG_OVERRIDES,
  OK
};

static gboolean
_add_override_from_struct (GstStructure * soverride)
{
  GstValidateOverride *override;
  const gchar *factory_name = NULL, *name = NULL, *klass = NULL;

  gboolean added = FALSE;

  override =
      gst_validate_override_factory_make (gst_structure_get_name (soverride),
      NULL);
  if (!override) {
    GST_ERROR ("No factory for override named %s",
        gst_structure_get_name (soverride));

    return FALSE;
  }

  name = gst_structure_get_string (soverride, "element-name");
  klass = gst_structure_get_string (soverride, "element-classification");
  factory_name = gst_structure_get_string (soverride, "element-factory-name");

  if (factory_name) {
    GType type;
    GstElement *element = gst_element_factory_make (factory_name, NULL);

    if (element == NULL) {
      GST_ERROR ("Unknown element factory name: %s (gst is %sinitialized)",
          factory_name, gst_is_initialized ()? "" : "NOT ");

      if (!name && !klass)
        return FALSE;
    } else {
      type = G_OBJECT_TYPE (element);
      gst_validate_override_add_by_type (type, override);
    }

    added = TRUE;
  }

  if (name) {
    gst_validate_override_add_by_name (name, override);
    added = TRUE;
  }

  if (klass) {
    gst_validate_override_add_by_klass (klass, override);
    added = TRUE;
  }

  /* Applies everywhere */
  if (!klass && !name && !factory_name) {
    gst_validate_override_add_global (override);
    added = TRUE;
  }

  override->parameters = soverride;
  return added;
}

static gboolean
_load_text_override_file (const gchar * filename)
{
  gint ret = OK;
  GList *structs = gst_validate_utils_structs_parse_from_filename (filename);

  if (structs) {
    GList *tmp;

    for (tmp = structs; tmp; tmp = tmp->next) {
      if (!_add_override_from_struct (tmp->data)) {
        GST_ERROR ("Wrong overrides %" GST_PTR_FORMAT, tmp->data);
        ret = WRONG_OVERRIDES;
      }
    }

    return ret;
  }

  return WRONG_FILE;
}

int
gst_validate_override_parser_preload (void)
{
  gchar **filelist, *const *filename;
  const char *sos, *loaderr;
  int nloaded = 0;

  sos = g_getenv ("GST_VALIDATE_OVERRIDE");
  if (!sos) {
    GST_INFO ("No GST_VALIDATE_OVERRIDE found, no overrides to load");
    return 0;
  }

  filelist = g_strsplit (sos, ",", 0);
  for (filename = filelist; *filename; ++filename) {
    if (_load_text_override_file (*filename) == WRONG_FILE) {
      loaderr = g_module_error ();
      GST_ERROR ("Failed to load %s %s", *filename,
          loaderr ? loaderr : "no idea why");
    } else {
      nloaded++;
    }
  }

  g_strfreev (filelist);
  GST_INFO ("%d overrides loaded", nloaded);
  return nloaded;
}

GList *gst_validate_override_parser_get_override_for_names
    (GstValidateOverrideParser * reg, const gchar * name, ...)
{
  GList *iter;
  GList *ret = NULL;

  if (name) {
    va_list varargs;
    GstValidateOverrideParserNameEntry *entry;

    va_start (varargs, name);

    GST_VALIDATE_OVERRIDE_PARSER_LOCK (reg);
    while (name) {
      for (iter = reg->name_overrides.head; iter; iter = g_list_next (iter)) {
        entry = iter->data;
        if ((g_strcmp0 (name, entry->name)) == 0) {
          ret = g_list_prepend (ret, entry->override);
        }
      }
      name = va_arg (varargs, gchar *);
    }
    GST_VALIDATE_OVERRIDE_PARSER_UNLOCK (reg);

    va_end (varargs);
  }

  return ret;
}
