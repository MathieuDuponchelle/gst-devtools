/* GstValidate
 * Copyright (C) 2014 Thibault Saunier <thibault.saunier@collabora.com>
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

#include <gst/check/gstcheck.h>
#include <glib/gstdio.h>
#include <gst/validate/validate.h>
#include <gst/validate/gst-validate-override-registry.h>

static const gchar *some_overrides =
    "change-severity, issue-id=buffer::not-expected-one, new-severity=critical\n"
    "change-severity, issue-id=buffer::not-expected-one, new-severity=warning, element-factory-name=queue";

static void
_check_message_level (const gchar * factoryname, GstValidateReportLevel level,
    const gchar * message_id)
{
  GList *reports;
  GstElement *element;
  GstValidateRunner *runner;
  GstValidateMonitor *monitor;

  element = gst_element_factory_make (factoryname, NULL);
  runner = gst_validate_runner_new ();
  monitor =
      gst_validate_monitor_factory_create (GST_OBJECT (element), runner, NULL);

  GST_VALIDATE_REPORT (monitor, g_quark_from_string (message_id),
      "Just some fakery");

  reports = gst_validate_runner_get_reports (runner);
  fail_unless_equals_int (g_list_length (reports), 1);
  fail_unless_equals_int (((GstValidateReport *) reports->data)->level, level);
  g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  gst_object_unref (element);
  gst_object_unref (monitor);

}

GST_START_TEST (check_text_overrides)
{
  GstValidateIssue *issue;
  gchar *override_filename =
      g_strdup_printf ("%s%c%s", g_get_tmp_dir (), G_DIR_SEPARATOR,
      "some_overrides");

  fail_unless (g_file_set_contents (override_filename,
          some_overrides, -1, NULL));

  issue =
      gst_validate_issue_from_id (g_quark_from_string
      ("buffer::not-expected-one"));
  fail_unless (issue != NULL);

  assert_equals_int (issue->default_level, GST_VALIDATE_REPORT_LEVEL_WARNING);

  g_setenv ("GST_VALIDATE_OVERRIDE", override_filename, TRUE);
  gst_validate_override_registry_preload ();
  assert_equals_int (issue->default_level, GST_VALIDATE_REPORT_LEVEL_CRITICAL);

  /* Check that with a queue, the level of a
   * buffer::not-expected-one is WARNING */
  _check_message_level ("queue", GST_VALIDATE_REPORT_LEVEL_WARNING,
      "buffer::not-expected-one");

  /* Check that with an identity, the level of a
   * buffer::not-expected-one is CRITICAL */
  _check_message_level ("identity", GST_VALIDATE_REPORT_LEVEL_CRITICAL,
      "buffer::not-expected-one");

  g_remove (override_filename);
  g_free (override_filename);
}

GST_END_TEST;


static Suite *
gst_validate_suite (void)
{
  Suite *s = suite_create ("registry");
  TCase *tc_chain = tcase_create ("registry");
  suite_add_tcase (s, tc_chain);

  gst_validate_init ();

  tcase_add_test (tc_chain, check_text_overrides);

  return s;
}

GST_CHECK_MAIN (gst_validate);
