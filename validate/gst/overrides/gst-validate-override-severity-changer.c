/*
 * gst-validate severity changer override
 *
 * Copyright 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>.
 *
 * gst-validate-severity-changer.c: Override that changes severity of reports
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

#include "gst-validate-override-severity-changer.h"

G_DEFINE_TYPE (GstValidateOverrideSeverityChanger,
    gst_validate_override_severity_changer, GST_TYPE_VALIDATE_OVERRIDE);

static void
    gst_validate_override_severity_changer_class_init
    (GstValidateOverrideSeverityChangerClass * klass)
{
}

static void
_change_severity_report_handler (GstValidateOverride * override,
    GstValidateReporter * reporter, GstValidateReport * report)
{
  GstValidateReportLevel level;
  GQuark issue_id;
  const gchar *str_issue_id, *str_new_severity;

  str_issue_id = gst_structure_get_string (override->parameters, "issue-id");
  if (!str_issue_id) {
    return;
  }

  issue_id = g_quark_from_string (str_issue_id);

  str_new_severity =
      gst_structure_get_string (override->parameters, "new-severity");
  if (!str_new_severity) {
    return;
  }

  level = gst_validate_report_level_from_name (str_new_severity);
  if (report->issue->issue_id == issue_id) {
    report->level = level;
  }
}

static void
gst_validate_override_severity_changer_init (GstValidateOverrideSeverityChanger
    * soverride)
{
  GstValidateOverride *override = (GstValidateOverride *) soverride;

  override->report_handler = _change_severity_report_handler;
}
