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

#include <gst/validate/validate.h>
#include <gst/validate/gst-validate-pad-monitor.h>
#include <gst/check/gstcheck.h>
#include "test-utils.h"

static gboolean
_check_reports_refcount (GstPad * pad, gint refcount)
{
  GList *tmp, *reports;
  gboolean result = TRUE;
  GstValidateReporter *reporter =
      (GstValidateReporter *) g_object_get_data (G_OBJECT (pad),
      "validate-monitor");

  reports = gst_validate_reporter_get_reports (reporter);
  /* We take a ref here */
  refcount += 1;

  for (tmp = reports; tmp; tmp = tmp->next) {
    if (((GstValidateReport *) tmp->data)->refcount != refcount)
      result = FALSE;
  }

  g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  return result;
}

GST_START_TEST (buffer_before_segment)
{
  GstPad *srcpad;
  GstElement *src, *sink;
  GstValidateRunner *runner;
  GstValidateReport *report;
  GstValidateMonitor *monitor;
  GList *reports;

  /* getting an existing element class is cheating, but easier */
  src = gst_element_factory_make ("fakesrc", "fakesrc");
  sink = gst_element_factory_make ("fakesink", "fakesink");

  fail_unless (gst_element_link (src, sink));

  fail_unless (g_setenv ("GST_VALIDATE_REPORT_LEVEL", "all", TRUE));
  runner = gst_validate_runner_new ();
  monitor =
      gst_validate_monitor_factory_create (GST_OBJECT (src), runner, NULL);
  gst_validate_reporter_set_handle_g_logs (GST_VALIDATE_REPORTER (monitor));
  fail_unless (GST_IS_VALIDATE_ELEMENT_MONITOR (monitor));

  srcpad = gst_element_get_static_pad (src, "src");

  /* We want to handle the src behaviour ourself */
  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, TRUE));
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);

  /* Send a buffer before pushing any segment (FAILS) */
  {
    _gst_check_expecting_log = TRUE;
    fail_unless_equals_int (gst_pad_push (srcpad, gst_buffer_new ()),
        GST_FLOW_OK);

    reports = gst_validate_runner_get_reports (runner);
    assert_equals_int (g_list_length (reports), 1);
    report = reports->data;
    fail_unless_equals_int (report->level, GST_VALIDATE_REPORT_LEVEL_WARNING);
    fail_unless_equals_int (report->issue->issue_id,
        GST_VALIDATE_ISSUE_ID_BUFFER_BEFORE_SEGMENT);
    g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  }

  /* Setup all needed event and push a new buffer (WORKS) */
  {
    _gst_check_expecting_log = FALSE;
    gst_check_setup_events (srcpad, src, NULL, GST_FORMAT_TIME);
    fail_unless_equals_int (gst_pad_push (srcpad, gst_buffer_new ()),
        GST_FLOW_OK);
    reports = gst_validate_runner_get_reports (runner);
    assert_equals_int (g_list_length (reports), 1);
    g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  }

  /* clean up */
  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, FALSE));
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  fail_unless (_check_reports_refcount (srcpad, 2));
  gst_object_unref (srcpad);
  check_destroyed (src, srcpad, NULL);
  check_destroyed (sink, NULL, NULL);
  check_destroyed (runner, NULL, NULL);
}

GST_END_TEST;

GST_START_TEST (buffer_outside_segment)
{
  GstPad *srcpad;
  GstBuffer *buffer;
  GstSegment segment;
  GstElement *src, *sink;
  const gchar *fakesrc_klass;
  GstValidateReport *report;
  GstValidateRunner *runner;
  GstValidateMonitor *monitor;
  GList *reports;

  /* getting an existing element class is cheating, but easier */
  src = gst_element_factory_make ("fakesrc", "fakesrc");
  sink = gst_element_factory_make ("fakesink", "fakesink");

  fakesrc_klass =
      gst_element_class_get_metadata (GST_ELEMENT_GET_CLASS (src), "klass");

  /* Testing if a buffer is outside a segment is only done for buffer outputed
   * from decoders for the moment, fake a Decoder so that the test is properly
   * executed */
  gst_element_class_add_metadata (GST_ELEMENT_GET_CLASS (src), "klass",
      "Decoder");

  fail_unless (g_setenv ("GST_VALIDATE_REPORT_LEVEL", "all", TRUE));
  runner = gst_validate_runner_new ();
  monitor =
      gst_validate_monitor_factory_create (GST_OBJECT (src), runner, NULL);
  gst_validate_reporter_set_handle_g_logs (GST_VALIDATE_REPORTER (monitor));

  srcpad = gst_element_get_static_pad (src, "src");
  fail_unless (GST_IS_VALIDATE_PAD_MONITOR (g_object_get_data ((GObject *)
              srcpad, "validate-monitor")));

  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, TRUE));
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  segment.start = 0;
  segment.stop = GST_SECOND;
  fail_unless (gst_pad_push_event (srcpad,
          gst_event_new_stream_start ("the-stream")));
  fail_unless (gst_pad_push_event (srcpad, gst_event_new_segment (&segment)));


  /*  Pushing a buffer that is outside the segment */
  {
    buffer = gst_buffer_new ();
    GST_BUFFER_PTS (buffer) = 10 * GST_SECOND;
    GST_BUFFER_DURATION (buffer) = GST_SECOND;
    fail_unless (gst_pad_push (srcpad, buffer));

    reports = gst_validate_runner_get_reports (runner);
    assert_equals_int (g_list_length (reports), 1);
    report = reports->data;
    fail_unless_equals_int (report->level, GST_VALIDATE_REPORT_LEVEL_ISSUE);
    fail_unless_equals_int (report->issue->issue_id,
        GST_VALIDATE_ISSUE_ID_BUFFER_IS_OUT_OF_SEGMENT);
    g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  }

  /* Pushing a buffer inside the segment */
  {
    fail_unless (gst_pad_push (srcpad, gst_buffer_new ()));
    reports = gst_validate_runner_get_reports (runner);
    assert_equals_int (g_list_length (reports), 1);
    g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  }


  /* clean up */
  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, FALSE));
  gst_object_unref (srcpad);

  gst_element_class_add_metadata (GST_ELEMENT_GET_CLASS (src), "klass",
      fakesrc_klass);
  gst_object_unref (src);
  gst_object_unref (runner);

  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);
  gst_object_unref (sink);
}

GST_END_TEST;

static void
_first_buffer_running_time (gboolean failing)
{
  GstPad *srcpad;
  GstBuffer *buffer;
  GstElement *src, *sink;
  GstValidateReport *report;
  GstValidateRunner *runner;
  GstValidateMonitor *monitor;
  GList *reports;

  /* getting an existing element class is cheating, but easier */
  src = gst_element_factory_make ("fakesrc", "fakesrc");
  sink = gst_element_factory_make ("fakesink", "fakesink");

  fail_unless (g_setenv ("GST_VALIDATE_REPORT_LEVEL", "all", TRUE));
  runner = gst_validate_runner_new ();
  monitor =
      gst_validate_monitor_factory_create (GST_OBJECT (src), runner, NULL);
  gst_validate_reporter_set_handle_g_logs (GST_VALIDATE_REPORTER (monitor));

  srcpad = gst_element_get_static_pad (src, "src");
  fail_unless (GST_IS_VALIDATE_PAD_MONITOR (g_object_get_data ((GObject *)
              srcpad, "validate-monitor")));

  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, TRUE));
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);

  gst_check_setup_events (srcpad, src, NULL, GST_FORMAT_TIME);

  /*  Pushing a first buffer that as a wrong running time */
  {
    buffer = gst_buffer_new ();

    if (failing)
      GST_BUFFER_PTS (buffer) = 23;

    GST_BUFFER_DURATION (buffer) = GST_SECOND;
    fail_unless (gst_pad_push (srcpad, buffer));

    reports = gst_validate_runner_get_reports (runner);
    if (failing) {
      assert_equals_int (g_list_length (reports), 1);
      report = reports->data;
      fail_unless_equals_int (report->level, GST_VALIDATE_REPORT_LEVEL_WARNING);
      fail_unless_equals_int (report->issue->issue_id,
          GST_VALIDATE_ISSUE_ID_FIRST_BUFFER_RUNNING_TIME_IS_NOT_ZERO);
    } else {
      assert_equals_int (g_list_length (reports), 0);
    }
    g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  }

  /* clean up */
  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, FALSE));
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (srcpad);
  check_destroyed (src, srcpad, NULL);
  check_destroyed (sink, NULL, NULL);
  check_destroyed (runner, NULL, NULL);
  check_destroyed (monitor, NULL, NULL);
}

GST_START_TEST (first_buffer_running_time)
{
  /*  First run the test with a first buffer timestamp != 0 */
  _first_buffer_running_time (TRUE);

  /*  First run the test with a first buffer timestamp == 0 */
  _first_buffer_running_time (FALSE);
}

GST_END_TEST;

static void
fake_demuxer_prepare_pads (GstBin * pipeline, GstElement * demux,
    GstValidateRunner * runner)
{
  gint i = 0;
  GList *tmp;

  fail_unless (g_list_length (demux->srcpads), 3);

  for (tmp = demux->srcpads; tmp; tmp = tmp->next) {
    GstPad *new_peer;
    gchar *name = g_strdup_printf ("sink-%d", i++);
    GstElement *sink = gst_element_factory_make ("fakesink", name);

    gst_bin_add (pipeline, sink);

    new_peer = sink->sinkpads->data;
    gst_pad_link (tmp->data, new_peer);
    gst_element_set_state (sink, GST_STATE_PLAYING);
    gst_pad_activate_mode (tmp->data, GST_PAD_MODE_PUSH, TRUE);

    g_free (name);
  }

  fail_unless (gst_pad_activate_mode (demux->sinkpads->data, GST_PAD_MODE_PUSH,
          TRUE));
}

static GstValidatePadMonitor *
_get_pad_monitor (GstPad * pad)
{
  GstValidatePadMonitor *m = get_pad_monitor (pad);

  gst_object_unref (pad);

  return m;
}

static void
_test_flow_aggregation (GstFlowReturn flow, GstFlowReturn flow1,
    GstFlowReturn flow2, GstFlowReturn demux_flow, gboolean should_fail)
{
  GstPad *srcpad;
  GstValidateReport *report;
  GstValidatePadMonitor *pmonitor, *pmonitor1, *pmonitor2;
  GstElement *demuxer = fake_demuxer_new ();
  GstBin *pipeline = GST_BIN (gst_pipeline_new ("validate-pipeline"));
  GList *reports;
  GstValidateRunner *runner;
  GstValidateMonitor *monitor;

  fail_unless (g_setenv ("GST_VALIDATE_REPORT_LEVEL", "all", TRUE));
  runner = gst_validate_runner_new ();
  monitor = gst_validate_monitor_factory_create (GST_OBJECT (pipeline),
      runner, NULL);
  gst_validate_reporter_set_handle_g_logs (GST_VALIDATE_REPORTER (monitor));

  gst_bin_add (pipeline, demuxer);
  fake_demuxer_prepare_pads (pipeline, demuxer, runner);

  srcpad = gst_pad_new ("srcpad1", GST_PAD_SRC);
  gst_pad_link (srcpad, demuxer->sinkpads->data);
  fail_unless (gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, TRUE));
  gst_check_setup_events_with_stream_id (srcpad, demuxer, NULL,
      GST_FORMAT_TIME, "the-stream");

  pmonitor = _get_pad_monitor (gst_pad_get_peer (demuxer->srcpads->data));
  pmonitor1 =
      _get_pad_monitor (gst_pad_get_peer (demuxer->srcpads->next->data));
  pmonitor2 =
      _get_pad_monitor (gst_pad_get_peer (demuxer->srcpads->next->next->data));

  pmonitor->last_flow_return = flow;
  pmonitor1->last_flow_return = flow1;
  pmonitor2->last_flow_return = flow2;
  FAKE_DEMUXER (demuxer)->return_value = demux_flow;

  fail_unless_equals_int (gst_pad_push (srcpad, gst_buffer_new ()), demux_flow);

  reports = gst_validate_runner_get_reports (runner);
  if (should_fail) {
    assert_equals_int (g_list_length (reports), 1);
    report = reports->data;
    fail_unless_equals_int (report->level, GST_VALIDATE_REPORT_LEVEL_CRITICAL);
    fail_unless_equals_int (report->issue->issue_id,
        GST_VALIDATE_ISSUE_ID_WRONG_FLOW_RETURN);
  } else {
    assert_equals_int (g_list_length (reports), 0);

  }

  g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);
  clean_bus (GST_ELEMENT (pipeline));

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
  ASSERT_OBJECT_REFCOUNT (pipeline, "ours", 1);
  check_destroyed (pipeline, demuxer, NULL);
  check_destroyed (monitor, pmonitor, NULL);
}

GST_START_TEST (flow_aggregation)
{
  /* Check the GstFlowCombiner to find the rules */

  /* Failling cases: */
  _test_flow_aggregation (GST_FLOW_OK, GST_FLOW_OK,
      GST_FLOW_ERROR, GST_FLOW_OK, TRUE);
  _test_flow_aggregation (GST_FLOW_EOS, GST_FLOW_EOS,
      GST_FLOW_EOS, GST_FLOW_OK, TRUE);
  _test_flow_aggregation (GST_FLOW_FLUSHING, GST_FLOW_OK,
      GST_FLOW_OK, GST_FLOW_OK, TRUE);
  _test_flow_aggregation (GST_FLOW_NOT_NEGOTIATED, GST_FLOW_OK,
      GST_FLOW_OK, GST_FLOW_OK, TRUE);

  /* Passing cases: */
  _test_flow_aggregation (GST_FLOW_EOS, GST_FLOW_EOS,
      GST_FLOW_EOS, GST_FLOW_EOS, FALSE);
  _test_flow_aggregation (GST_FLOW_EOS, GST_FLOW_EOS,
      GST_FLOW_OK, GST_FLOW_OK, FALSE);
  _test_flow_aggregation (GST_FLOW_OK, GST_FLOW_OK,
      GST_FLOW_OK, GST_FLOW_EOS, FALSE);
  _test_flow_aggregation (GST_FLOW_NOT_NEGOTIATED, GST_FLOW_OK,
      GST_FLOW_OK, GST_FLOW_NOT_NEGOTIATED, FALSE);
}

GST_END_TEST;

static GstPadProbeReturn
drop_buffers (GstPad * pad, GstPadProbeInfo * info, gpointer unused)
{
  return GST_PAD_PROBE_DROP;
}

GST_START_TEST (issue_concatenation)
{
  GstPad *srcpad1, *srcpad2, *sinkpad, *funnel_sink1, *funnel_sink2;
  GstElement *src1, *src2, *sink, *funnel;
  GstValidateRunner *runner;
  GstValidatePadMonitor *srcpad_monitor1, *srcpad_monitor2, *sinkpad_monitor;
  GstValidatePadMonitor *funnel_sink_monitor1, *funnel_sink_monitor2;
  GstSegment segment;
  GList *reports;
  gint n_reports;
  gulong probe_id1, probe_id2;

  fail_unless (g_setenv ("GST_VALIDATE_REPORT_LEVEL", "subchain", TRUE));
  runner = gst_validate_runner_new ();

  src1 = create_and_monitor_element ("fakesrc", "fakesrc1", runner);
  src2 = create_and_monitor_element ("fakesrc", "fakesrc2", runner);
  funnel = create_and_monitor_element ("funnel", "funnel", runner);
  sink = create_and_monitor_element ("fakesink", "fakesink", runner);

  srcpad1 = gst_element_get_static_pad (src1, "src");
  srcpad_monitor1 = g_object_get_data (G_OBJECT (srcpad1), "validate-monitor");
  srcpad2 = gst_element_get_static_pad (src2, "src");
  srcpad_monitor2 = g_object_get_data (G_OBJECT (srcpad2), "validate-monitor");
  funnel_sink1 = gst_element_get_request_pad (funnel, "sink_%u");
  funnel_sink_monitor1 =
      g_object_get_data (G_OBJECT (funnel_sink1), "validate-monitor");
  funnel_sink2 = gst_element_get_request_pad (funnel, "sink_%u");
  funnel_sink_monitor2 =
      g_object_get_data (G_OBJECT (funnel_sink2), "validate-monitor");
  sinkpad = gst_element_get_static_pad (sink, "sink");
  sinkpad_monitor = g_object_get_data (G_OBJECT (sinkpad), "validate-monitor");

  fail_unless (gst_element_link (funnel, sink));
  fail_unless (gst_pad_link (srcpad1, funnel_sink1) == GST_PAD_LINK_OK);
  fail_unless (gst_pad_link (srcpad2, funnel_sink2) == GST_PAD_LINK_OK);

  /* There's gonna be some clunkiness in here because of funnel */
  probe_id1 = gst_pad_add_probe (srcpad1,
      GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST,
      (GstPadProbeCallback) drop_buffers, NULL, NULL);
  probe_id2 = gst_pad_add_probe (srcpad2,
      GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST,
      (GstPadProbeCallback) drop_buffers, NULL, NULL);

  /* We want to handle the src behaviour ourselves */
  fail_unless (gst_pad_activate_mode (srcpad1, GST_PAD_MODE_PUSH, TRUE));
  fail_unless (gst_pad_activate_mode (srcpad2, GST_PAD_MODE_PUSH, TRUE));

  /* Setup all needed events */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  segment.start = 0;
  segment.stop = GST_SECOND;

  fail_unless (gst_pad_push_event (srcpad1,
          gst_event_new_stream_start ("the-stream")));
  fail_unless (gst_pad_push_event (srcpad1, gst_event_new_segment (&segment)));

  fail_unless (gst_pad_push_event (srcpad2,
          gst_event_new_stream_start ("the-stream")));
  fail_unless (gst_pad_push_event (srcpad2, gst_event_new_segment (&segment)));

  fail_unless_equals_int (gst_element_set_state (funnel, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);


  /* Send an unexpected flush stop */
  _gst_check_expecting_log = TRUE;
  fail_unless (gst_pad_push_event (srcpad1, gst_event_new_flush_stop (TRUE)));

  /* The runner only sees one report */
  reports = gst_validate_runner_get_reports (runner);
  assert_equals_int (g_list_length (reports), 1);
  g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);

  /* Each pad monitor on the way actually holds a report */
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      srcpad_monitor1);
  fail_unless_equals_int (n_reports, 1);
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      sinkpad_monitor);
  fail_unless_equals_int (n_reports, 1);
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      funnel_sink_monitor1);
  fail_unless_equals_int (n_reports, 1);

  /* But not the pad monitor of the other funnel sink */
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      funnel_sink_monitor2);
  fail_unless_equals_int (n_reports, 0);
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      srcpad_monitor2);
  fail_unless_equals_int (n_reports, 0);

  /* Once again but on the other funnel sink */
  fail_unless (gst_pad_push_event (srcpad2, gst_event_new_flush_stop (TRUE)));

  /* The runner now sees two reports */
  reports = gst_validate_runner_get_reports (runner);
  assert_equals_int (g_list_length (reports), 2);
  g_list_free_full (reports, (GDestroyNotify) gst_validate_report_unref);

  /* These monitors already saw that issue */
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      srcpad_monitor1);
  fail_unless_equals_int (n_reports, 1);
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      sinkpad_monitor);
  fail_unless_equals_int (n_reports, 1);
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      funnel_sink_monitor1);
  fail_unless_equals_int (n_reports, 1);

  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      funnel_sink_monitor2);
  fail_unless_equals_int (n_reports, 1);
  n_reports =
      gst_validate_reporter_get_reports_count ((GstValidateReporter *)
      srcpad_monitor2);
  fail_unless_equals_int (n_reports, 1);

  /* clean up */
  fail_unless (gst_pad_activate_mode (srcpad1, GST_PAD_MODE_PUSH, FALSE));
  fail_unless (gst_pad_activate_mode (srcpad2, GST_PAD_MODE_PUSH, FALSE));
  fail_unless_equals_int (gst_element_set_state (funnel, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);
  fail_unless_equals_int (gst_element_set_state (sink, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  gst_pad_remove_probe (srcpad1, probe_id1);
  gst_pad_remove_probe (srcpad2, probe_id2);

  /* The reporter, the runner */
  fail_unless (_check_reports_refcount (srcpad1, 2));
  /* The reporter, the master report */
  fail_unless (_check_reports_refcount (funnel_sink1, 2));
  free_element_monitor (src1);
  free_element_monitor (src2);
  free_element_monitor (funnel);
  free_element_monitor (sink);
  gst_object_unref (srcpad1);
  gst_object_unref (srcpad2);
  gst_object_unref (sinkpad);
  gst_object_unref (funnel_sink1);
  gst_object_unref (funnel_sink2);
  check_destroyed (funnel, funnel_sink1, funnel_sink2, NULL);
  check_destroyed (src1, srcpad1, NULL);
  check_destroyed (src2, srcpad2, NULL);
  check_destroyed (sink, sinkpad, NULL);
  check_destroyed (runner, NULL, NULL);
}

GST_END_TEST;

static Suite *
gst_validate_suite (void)
{
  Suite *s = suite_create ("padmonitor");
  TCase *tc_chain = tcase_create ("padmonitor");
  suite_add_tcase (s, tc_chain);

  gst_validate_init ();

  tcase_add_test (tc_chain, buffer_before_segment);
  tcase_add_test (tc_chain, buffer_outside_segment);
  tcase_add_test (tc_chain, first_buffer_running_time);
  tcase_add_test (tc_chain, flow_aggregation);
  tcase_add_test (tc_chain, issue_concatenation);

  return s;
}

GST_CHECK_MAIN (gst_validate);
