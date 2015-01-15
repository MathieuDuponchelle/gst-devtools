/*
 * gst-validate factory checker override
 *
 * Copyright 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>.
 *
 * gst-validate-factory-checker.c: Override that checks whether an
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gst-validate-override-factory-checker.h"
#include <gst/validate/gst-validate-element-monitor.h>

#define WRONG_FACTORY g_quark_from_static_string ("factory-check::wrong-factory")

G_DEFINE_TYPE (GstValidateOverrideFactoryChecker,
    gst_validate_override_factory_checker, GST_TYPE_VALIDATE_OVERRIDE);

static void
    gst_validate_override_factory_checker_class_init
    (G_GNUC_UNUSED GstValidateOverrideFactoryCheckerClass * klass)
{
}

static void
_element_added_factory_checker (GstValidateOverride * override,
    G_GNUC_UNUSED GstValidateMonitor * monitor, GstElement * element)
{
  const gchar *element_type, *str_input_caps, *factory_name,
      *actual_factory_name;
  GList *templates, *tmp;
  GstElementClass *klass;
  GstCaps *input_caps, *template_caps;
  gboolean match = FALSE;

  element_type =
      gst_structure_get_string (override->parameters, "element-type");
  str_input_caps =
      gst_structure_get_string (override->parameters, "input-caps");
  factory_name =
      gst_structure_get_string (override->parameters, "factory-name");

  klass = GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS (element));

  if (!element_type || !str_input_caps || !factory_name)
    return;

  input_caps = gst_caps_from_string (str_input_caps);

  if (!input_caps)
    return;

  if (!g_strcmp0 (element_type, "decoder")) {
    if (!GST_VALIDATE_ELEMENT_MONITOR_ELEMENT_IS_DECODER (monitor))
      return;
  } else {
    return;
  }

  templates = gst_element_class_get_pad_template_list (klass);

  for (tmp = templates; tmp; tmp = tmp->next) {
    if (GST_PAD_TEMPLATE_DIRECTION (tmp->data) == GST_PAD_SRC)
      continue;
    template_caps = gst_pad_template_get_caps (tmp->data);
    if (gst_caps_is_subset (template_caps, input_caps))
      match = TRUE;
  }
  if (!match)
    return;

  actual_factory_name =
      g_type_name (gst_element_factory_get_element_type (gst_element_get_factory
          (element)));
  if (g_strcmp0 (factory_name, actual_factory_name))
    GST_VALIDATE_REPORT (GST_VALIDATE_REPORTER (monitor), WRONG_FACTORY,
        "%s was instantiated from %s, where %s is required",
        gst_element_get_name (element), actual_factory_name, factory_name);
}

static void
gst_validate_override_factory_checker_init (GstValidateOverrideFactoryChecker
    * soverride)
{
  GstValidateOverride *override = (GstValidateOverride *) soverride;

  override->element_added_handler = _element_added_factory_checker;
  gst_validate_issue_register (gst_validate_issue_new (WRONG_FACTORY,
          "wrong factory",
          "The factory this element was instantiated from is not the required one",
          GST_VALIDATE_REPORT_LEVEL_CRITICAL));
}
