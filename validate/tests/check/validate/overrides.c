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
#include <gst/validate/gst-validate-override-parser.h>

static const gchar *some_overrides =
    "change-severity, issue-id=buffer::not-expected-one, new-severity=critical\n"
    "change-severity, issue-id=buffer::not-expected-one, new-severity=issue, element-factory-name=queue";

static const gchar *some_other_overrides =
    "set-timestamp, element-factory-name=fakesink";

static void
_check_message_level (const gchar * factoryname, GstValidateReportLevel level,
    const gchar * message_id)
{
  GList *reports;
  GstElement *element;
  GstValidateRunner *runner;
  GstValidateMonitor *monitor;

  element = gst_element_factory_make (factoryname, NULL);
  fail_unless (g_setenv ("GST_VALIDATE_REPORTING_DETAILS", "all", TRUE));
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

  g_setenv ("GST_VALIDATE_PLUGIN_PATH",
      "/home/meh/devel/pitivi-git/gst-devtools/validate/gst/overrides/.libs",
      TRUE);
  g_setenv ("GST_VALIDATE_OVERRIDE", override_filename, TRUE);
  gst_validate_init ();

  /* Check that with a queue, the level of a
   * buffer::not-expected-one is WARNING */
  _check_message_level ("queue", GST_VALIDATE_REPORT_LEVEL_ISSUE,
      "buffer::not-expected-one");

  /* Check that with an identity, the level of a
   * buffer::not-expected-one is CRITICAL */
  _check_message_level ("identity", GST_VALIDATE_REPORT_LEVEL_CRITICAL,
      "buffer::not-expected-one");

  g_remove (override_filename);
  g_free (override_filename);
}

GST_END_TEST;


GST_START_TEST (test_override)
{
  GstElement *fakesink = gst_element_factory_make ("fakesink", "fakesink");
  GstPad *pad = gst_element_get_static_pad (fakesink, "sink");
  GstPad *srcpad = gst_pad_new ("src", GST_PAD_SRC);
  GstBuffer *buffer = gst_buffer_new ();
  GstSegment segment;
  GstValidateRunner *runner;
  GstValidateMonitor *monitor;
  gchar *override_filename;

  /* Load our dummy override plugins */
  g_setenv ("GST_VALIDATE_PLUGIN_PATH",
      "/home/meh/devel/pitivi-git/gst-devtools/validate/tests/check/dummy_override/.libs/",
      TRUE);

  /* Define where to instantiate an override, in this case on the fakesink */
  override_filename =
      g_strdup_printf ("%s%c%s", g_get_tmp_dir (), G_DIR_SEPARATOR,
      "some_other_overrides");

  fail_unless (g_file_set_contents (override_filename,
          some_other_overrides, -1, NULL));
  g_setenv ("GST_VALIDATE_OVERRIDE", override_filename, TRUE);

  gst_validate_init ();

  runner = gst_validate_runner_new ();
  monitor =
      gst_validate_monitor_factory_create (GST_OBJECT (fakesink), runner, NULL);

  gst_segment_init (&segment, GST_FORMAT_TIME);

  gst_pad_link (srcpad, pad);

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_push_event (srcpad, gst_event_new_stream_start ("dummy"));
  gst_pad_push_event (srcpad, gst_event_new_segment (&segment));
  gst_element_set_state (fakesink, GST_STATE_PLAYING);
  gst_pad_push (srcpad, buffer);

  /* Our dummy-override has set the PTS of the buffer to 42 seconds */
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 42 * GST_SECOND);

  gst_element_set_state (fakesink, GST_STATE_NULL);
  gst_object_unref (fakesink);
  gst_object_unref (monitor);
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
  tcase_add_test (tc_chain, test_override);

  return s;
}

GST_CHECK_MAIN (gst_validate);
