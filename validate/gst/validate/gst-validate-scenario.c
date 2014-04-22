/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thibault Saunier <thibault.saunier@collabora.com>
 *
 * gst-validate-scenario.c - Validate Scenario class
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
#include "config.h"
#endif

#ifdef linux
#include <glib/gstdio.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <gst/gst.h>
#include <gio/gio.h>
#include <string.h>

#include "gst-validate-internal.h"
#include "gst-validate-scenario.h"
#include "gst-validate-reporter.h"
#include "gst-validate-report.h"
#include "gst-validate-utils.h"

#define GST_VALIDATE_SCENARIO_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_VALIDATE_SCENARIO, GstValidateScenarioPrivate))

#define GST_VALIDATE_SCENARIO_SUFFIX ".scenario"
#define GST_VALIDATE_SCENARIO_DIRECTORY "validate-scenario"

#define DEFAULT_SEEK_TOLERANCE (0.1 * GST_SECOND)       /* tolerance seek interval
                                                           TODO make it overridable  */
enum
{
  PROP_0,
  PROP_RUNNER,
  PROP_LAST
};

static GHashTable *action_types_table;
static void gst_validate_scenario_dispose (GObject * object);
static void gst_validate_scenario_finalize (GObject * object);
static GRegex *clean_action_str;

G_DEFINE_TYPE_WITH_CODE (GstValidateScenario, gst_validate_scenario,
    G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (GST_TYPE_VALIDATE_REPORTER, NULL));

typedef struct _GstValidateActionType
{
  GstValidateExecuteAction execute;
  gchar **mandatory_fields;
  gchar *description;
  gboolean is_config;
} GstValidateActionType;

struct _GstValidateScenarioPrivate
{
  GstValidateRunner *runner;

  GList *actions;
  /*  List of action that need parsing when reaching ASYNC_DONE
   *  most probably to be able to query duration */
  GList *needs_parsing;

  GstEvent *last_seek;
  GstSeekFlags seek_flags;
  GstClockTime segment_start;
  GstClockTime segment_stop;
  GstClockTime seek_pos_tol;

  guint num_actions;

  guint get_pos_id;
};

typedef struct KeyFileGroupName
{
  GKeyFile *kf;
  gchar *group_name;
} KeyFileGroupName;

GType _gst_validate_action_type;
static GType gst_validate_action_get_type (void);

GST_DEFINE_MINI_OBJECT_TYPE (GstValidateAction, gst_validate_action);
static GstValidateAction *gst_validate_action_new (void);

static GstValidateAction *
_action_copy (GstValidateAction * act)
{
  GstValidateAction *copy = gst_validate_action_new ();

  if (act->structure) {
    copy->structure = gst_structure_copy (act->structure);
    copy->type = gst_structure_get_name (copy->structure);
    if (!(act->name = gst_structure_get_string (copy->structure, "name")))
      act->name = "";
  }

  copy->action_number = act->action_number;
  copy->playback_time = act->playback_time;

  return copy;
}

static void
_action_free (GstValidateAction * action)
{
  if (action->structure)
    gst_structure_free (action->structure);
}

static void
gst_validate_action_init (GstValidateAction * action)
{
  gst_mini_object_init (((GstMiniObject *) action), 0,
      _gst_validate_action_type, (GstMiniObjectCopyFunction) _action_copy, NULL,
      (GstMiniObjectFreeFunction) _action_free);

}

static GstValidateAction *
gst_validate_action_new (void)
{
  GstValidateAction *action = g_slice_new0 (GstValidateAction);

  gst_validate_action_init (action);

  return action;
}

static void
gst_validate_action_print (GstValidateAction * action, const gchar * format,
    ...)
{
  va_list var_args;
  GString *string = g_string_new (NULL);

  g_string_printf (string, "(Executing action: %s, number: %u at position: %"
      GST_TIME_FORMAT " repeat: %i) | ", g_strcmp0 (action->name, "") == 0 ?
      "Unnamed" : action->name,
      action->action_number, GST_TIME_ARGS (action->playback_time),
      action->repeat);

  va_start (var_args, format);
  g_string_append_vprintf (string, format, var_args);
  va_end (var_args);

  g_print ("%s\n", string->str);

  g_string_free (string, TRUE);
}

static gboolean
_set_variable_func (const gchar * name, double *value, gpointer user_data)
{
  GstValidateScenario *scenario = GST_VALIDATE_SCENARIO (user_data);

  if (!g_strcmp0 (name, "duration")) {
    gint64 duration;

    if (!gst_element_query_duration (scenario->pipeline,
            GST_FORMAT_TIME, &duration)) {
      GST_WARNING_OBJECT (scenario, "Could not query duration");
      return FALSE;
    }
    *value = ((double) (duration / GST_SECOND));

    return TRUE;
  } else if (!g_strcmp0 (name, "position")) {
    gint64 position;

    if (!gst_element_query_position (scenario->pipeline,
            GST_FORMAT_TIME, &position)) {
      GST_WARNING_OBJECT (scenario, "Could not query position");
      return FALSE;
    }
    *value = ((double) position / GST_SECOND);

    return TRUE;
  }

  return FALSE;
}

gboolean
gst_validate_action_get_clocktime (GstValidateScenario * scenario,
    GstValidateAction * action, const gchar * name, GstClockTime * retval)
{
  gdouble val;
  const gchar *strval;

  if (!gst_structure_get_double (action->structure, name, &val)) {
    gchar *error = NULL;

    if (!(strval = gst_structure_get_string (action->structure, name))) {
      GST_WARNING_OBJECT (scenario, "Could not find %s", name);
      return FALSE;
    }
    val =
        gst_validate_utils_parse_expression (strval, _set_variable_func,
        scenario, &error);

    if (error) {
      GST_WARNING ("Error while parsing %s: %s", strval, error);
      g_free (error);

      return FALSE;
    }
  }

  if (val == -1.0)
    *retval = GST_CLOCK_TIME_NONE;
  else
    *retval = val * GST_SECOND;

  return TRUE;
}

static gboolean
_execute_seek (GstValidateScenario * scenario, GstValidateAction * action)
{
  GstValidateScenarioPrivate *priv = scenario->priv;
  const char *str_format, *str_flags, *str_start_type, *str_stop_type;
  gboolean ret = TRUE;

  gdouble rate = 1.0;
  GstFormat format = GST_FORMAT_TIME;
  GstSeekFlags flags = 0;
  GstSeekType start_type = GST_SEEK_TYPE_SET;
  GstClockTime start;
  GstSeekType stop_type = GST_SEEK_TYPE_SET;
  GstClockTime stop = GST_CLOCK_TIME_NONE;
  GstEvent *seek;

  if (!gst_validate_action_get_clocktime (scenario, action, "start", &start))
    return FALSE;

  gst_structure_get_double (action->structure, "rate", &rate);
  if ((str_format = gst_structure_get_string (action->structure, "format")))
    gst_validate_utils_enum_from_str (GST_TYPE_FORMAT, str_format, &format);

  if ((str_start_type =
          gst_structure_get_string (action->structure, "start_type")))
    gst_validate_utils_enum_from_str (GST_TYPE_SEEK_TYPE, str_start_type,
        &start_type);

  if ((str_stop_type =
          gst_structure_get_string (action->structure, "stop_type")))
    gst_validate_utils_enum_from_str (GST_TYPE_SEEK_TYPE, str_stop_type,
        &stop_type);

  if ((str_flags = gst_structure_get_string (action->structure, "flags")))
    flags = gst_validate_utils_flags_from_str (GST_TYPE_SEEK_FLAGS, str_flags);

  gst_validate_action_get_clocktime (scenario, action, "stop", &stop);

  gst_validate_action_print (action, "seeking to: %" GST_TIME_FORMAT
      " stop: %" GST_TIME_FORMAT " Rate %lf",
      GST_TIME_ARGS (start), GST_TIME_ARGS (stop), rate);

  seek = gst_event_new_seek (rate, format, flags, start_type, start,
      stop_type, stop);
  gst_event_ref (seek);
  if (gst_element_send_event (scenario->pipeline, seek)) {
    gst_event_replace (&priv->last_seek, seek);
    priv->seek_flags = flags;
  } else {
    GST_VALIDATE_REPORT (scenario, EVENT_SEEK_NOT_HANDLED,
        "Could not execute seek: '(position %" GST_TIME_FORMAT
        "), %s (num %u, missing repeat: %i), seeking to: %" GST_TIME_FORMAT
        " stop: %" GST_TIME_FORMAT " Rate %lf'",
        GST_TIME_ARGS (action->playback_time), action->name,
        action->action_number, action->repeat, GST_TIME_ARGS (start),
        GST_TIME_ARGS (stop), rate);
    ret = FALSE;
  }
  gst_event_unref (seek);

  return ret;
}

static gboolean
_pause_action_restore_playing (GstValidateScenario * scenario)
{
  GstElement *pipeline = scenario->pipeline;


  g_print ("\n==== Back to playing ===\n");

  if (gst_element_set_state (pipeline, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_FAILURE) {
    GST_VALIDATE_REPORT (scenario, STATE_CHANGE_FAILURE,
        "Failed to set state to playing");
  }

  return FALSE;
}


static gboolean
_execute_pause (GstValidateScenario * scenario, GstValidateAction * action)
{
  gdouble duration = 0;

  gst_structure_get_double (action->structure, "duration", &duration);
  gst_validate_action_print (action, "pausing for %" GST_TIME_FORMAT,
      GST_TIME_ARGS (duration * GST_SECOND));

  GST_DEBUG ("Pausing for %" GST_TIME_FORMAT,
      GST_TIME_ARGS (duration * GST_SECOND));

  if (gst_element_set_state (scenario->pipeline, GST_STATE_PAUSED) ==
      GST_STATE_CHANGE_FAILURE) {
    GST_VALIDATE_REPORT (scenario, STATE_CHANGE_FAILURE,
        "Failed to set state to paused");

    return FALSE;
  }
  if (duration)
    g_timeout_add (duration * 1000,
        (GSourceFunc) _pause_action_restore_playing, scenario);

  return TRUE;
}

static gboolean
_execute_play (GstValidateScenario * scenario, GstValidateAction * action)
{
  gst_validate_action_print (action, "Playing back");

  GST_DEBUG ("Playing back");

  if (gst_element_set_state (scenario->pipeline, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_FAILURE) {
    GST_VALIDATE_REPORT (scenario, STATE_CHANGE_FAILURE,
        "Failed to set state to playing");

    return FALSE;
  }
  gst_element_get_state (scenario->pipeline, NULL, NULL, -1);
  return TRUE;
}

static gboolean
_execute_eos (GstValidateScenario * scenario, GstValidateAction * action)
{
  gst_validate_action_print (action, "sending EOS at %" GST_TIME_FORMAT,
      GST_TIME_ARGS (action->playback_time));

  GST_DEBUG ("Sending eos to pipeline at %" GST_TIME_FORMAT,
      GST_TIME_ARGS (action->playback_time));

  return gst_element_send_event (scenario->pipeline, gst_event_new_eos ());
}

static int
find_input_selector (GValue * velement, const gchar * type)
{
  GstElement *element = g_value_get_object (velement);

  if (G_OBJECT_TYPE (element) == g_type_from_name ("GstInputSelector")) {
    GstPad *srcpad = gst_element_get_static_pad (element, "src");

    if (srcpad) {
      GstCaps *caps = gst_pad_query_caps (srcpad, NULL);

      if (caps) {
        const char *mime =
            gst_structure_get_name (gst_caps_get_structure (caps, 0));
        gboolean found = FALSE;

        if (g_strcmp0 (type, "audio") == 0)
          found = g_str_has_prefix (mime, "audio/");
        else if (g_strcmp0 (type, "video") == 0)
          found = g_str_has_prefix (mime, "video/")
              && !g_str_has_prefix (mime, "video/x-dvd-subpicture");
        else if (g_strcmp0 (type, "text") == 0)
          found = g_str_has_prefix (mime, "text/")
              || g_str_has_prefix (mime, "subtitle/")
              || g_str_has_prefix (mime, "video/x-dvd-subpicture");

        gst_object_unref (srcpad);
        if (found)
          return 0;
      }
    }
  }
  return !0;
}

static GstElement *
find_input_selector_with_type (GstBin * bin, const gchar * type)
{
  GValue result = { 0, };
  GstElement *input_selector = NULL;
  GstIterator *iterator = gst_bin_iterate_recurse (bin);

  if (gst_iterator_find_custom (iterator,
          (GCompareFunc) find_input_selector, &result, (gpointer) type)) {
    input_selector = g_value_get_object (&result);
  }
  gst_iterator_free (iterator);

  return input_selector;
}

static GstPad *
find_nth_sink_pad (GstElement * element, int index)
{
  GstIterator *iterator;
  gboolean done = FALSE;
  GstPad *pad = NULL;
  int dec_index = index;
  GValue data = { 0, };

  iterator = gst_element_iterate_sink_pads (element);
  while (!done) {
    switch (gst_iterator_next (iterator, &data)) {
      case GST_ITERATOR_OK:
        if (!dec_index--) {
          done = TRUE;
          pad = g_value_get_object (&data);
          break;
        }
        g_value_reset (&data);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iterator);
        dec_index = index;
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iterator);
  return pad;
}

static int
find_sink_pad_index (GstElement * element, GstPad * pad)
{
  GstIterator *iterator;
  gboolean done = FALSE;
  int index = 0;
  GValue data = { 0, };

  iterator = gst_element_iterate_sink_pads (element);
  while (!done) {
    switch (gst_iterator_next (iterator, &data)) {
      case GST_ITERATOR_OK:
        if (pad == g_value_get_object (&data)) {
          done = TRUE;
        } else {
          index++;
        }
        g_value_reset (&data);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iterator);
        index = 0;
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iterator);
  return index;
}

static gboolean
_execute_switch_track (GstValidateScenario * scenario,
    GstValidateAction * action)
{
  guint index;
  gboolean relative = FALSE;
  const gchar *type, *str_index;
  GstElement *input_selector;

  if (!(type = gst_structure_get_string (action->structure, "type")))
    type = "audio";

  /* First find an input selector that has the right type */
  input_selector =
      find_input_selector_with_type (GST_BIN (scenario->pipeline), type);
  if (input_selector) {
    GstPad *pad;

    if ((str_index = gst_structure_get_string (action->structure, "index"))) {
      if (!gst_structure_get_uint (action->structure, "index", &index)) {
        GST_WARNING ("No index given, defaulting to +1");
        index = 1;
        relative = TRUE;
      }
    } else {
      relative = strchr ("+-", str_index[0]) != NULL;
      index = g_ascii_strtoll (str_index, NULL, 10);
    }

    if (relative) {             /* We are changing track relatively to current track */
      int npads;

      g_object_get (input_selector, "active-pad", &pad, "n-pads", &npads, NULL);
      if (pad) {
        int current_index = find_sink_pad_index (input_selector, pad);

        index = (current_index + index) % npads;
        gst_object_unref (pad);
      }
    }

    gst_validate_action_print (action, "Switching to track number: %i", index);
    pad = find_nth_sink_pad (input_selector, index);
    g_object_set (input_selector, "active-pad", pad, NULL);
    gst_object_unref (pad);
    gst_object_unref (input_selector);

    return TRUE;
  }

  /* No selector found -> Failed */
  return FALSE;
}

static gboolean
_set_rank (GstValidateScenario * scenario, GstValidateAction * action)
{
  guint rank;
  GstPluginFeature *feature;
  const gchar *feature_name;

  if (!(feature_name =
          gst_structure_get_string (action->structure, "feature-name"))) {
    GST_ERROR ("Could not find the name of the feature to tweak");

    return FALSE;
  }

  if (!(gst_structure_get_uint (action->structure, "rank", &rank) ||
          gst_structure_get_int (action->structure, "rank", (gint *) & rank))) {
    GST_ERROR ("Could not get rank to set on %s", feature_name);

    return FALSE;
  }

  feature = gst_registry_lookup_feature (gst_registry_get (), feature_name);
  if (!feature) {
    GST_ERROR ("Could not find feaure %s", feature_name);

    return FALSE;
  }

  gst_plugin_feature_set_rank (feature, rank);
  gst_object_unref (feature);

  return TRUE;
}

#ifdef linux
typedef struct {
      unsigned long size,resident,share,text,lib,data,dt;
} statm_t;
#endif

static unsigned long
get_resident_memory_usage(void) {
#ifdef linux
  FILE *proc_file;
  statm_t result;
  const gchar *statm_path = "/proc/self/statm";
  proc_file = g_fopen(statm_path, "r");

  if (!proc_file)
    return 0;
  if (7 != fscanf(proc_file,"%ld %ld %ld %ld %ld %ld %ld",
                  &result.size,
                  &result.resident,
                  &result.share,
                  &result.text,
                  &result.lib,
                  &result.data,
                  &result.dt)) {
    fclose(proc_file);
    return 0;
  } else {
    fclose(proc_file);
    return result.resident;
  }

#else
  struct rusage* memory = g_malloc(sizeof(struct rusage));
  errno = 0;
  getrusage(RUSAGE_SELF, memory);
  if(errno == EFAULT)
    return 0;
  else if(errno == EINVAL)
    return 0;

  return (memory->ru_idrss);
#endif
}

static gboolean
get_position (GstValidateScenario * scenario)
{
  GList *tmp;
  GstQuery *query;
  gdouble rate = 1.0;
  GstValidateAction *act = NULL;
  gint64 start_with_tolerance, stop_with_tolerance;
  gint64 position, duration;
  GstFormat format = GST_FORMAT_TIME;
  GstValidateScenarioPrivate *priv = scenario->priv;
  GstElement *pipeline = scenario->pipeline;

  query = gst_query_new_segment (GST_FORMAT_DEFAULT);
  if (gst_element_query (GST_ELEMENT (scenario->pipeline), query))
    gst_query_parse_segment (query, &rate, NULL, NULL, NULL);

  gst_query_unref (query);
  if (scenario->priv->actions)
    act = scenario->priv->actions->data;
  gst_element_query_position (pipeline, format, &position);

  format = GST_FORMAT_TIME;
  gst_element_query_duration (pipeline, format, &duration);

  if (position > duration) {
    GST_VALIDATE_REPORT (scenario,
        QUERY_POSITION_SUPERIOR_DURATION,
        "Reported position %" GST_TIME_FORMAT " > reported duration %"
        GST_TIME_FORMAT, GST_TIME_ARGS (position), GST_TIME_ARGS (duration));

    return TRUE;
  }

  GST_INFO("memory usage is : %ld", get_resident_memory_usage());
  GST_LOG ("Current position: %" GST_TIME_FORMAT, GST_TIME_ARGS (position));

  /* Check if playback is within seek segment */
  start_with_tolerance =
      MAX (0, (gint64) (priv->segment_start - priv->seek_pos_tol));
  stop_with_tolerance =
      priv->segment_stop != -1 ? priv->segment_stop + priv->seek_pos_tol : -1;
  if ((GST_CLOCK_TIME_IS_VALID (stop_with_tolerance) && position > stop_with_tolerance)
      || (priv->seek_flags & GST_SEEK_FLAG_ACCURATE && position < start_with_tolerance)) {

    GST_VALIDATE_REPORT (scenario, QUERY_POSITION_OUT_OF_SEGMENT,
        "Current position %" GST_TIME_FORMAT " not in the expected range [%"
        GST_TIME_FORMAT " -- %" GST_TIME_FORMAT, GST_TIME_ARGS (position),
        GST_TIME_ARGS (start_with_tolerance),
        GST_TIME_ARGS (stop_with_tolerance));
  }

  if (act && ((rate > 0 && (GstClockTime) position >= act->playback_time) ||
          (rate < 0 && (GstClockTime) position <= act->playback_time))) {
    GstValidateActionType *type;

    /* TODO what about non flushing seeks? */
    /* TODO why is this inside the action time if ? */
    if (priv->last_seek)
      return TRUE;

    type = g_hash_table_lookup (action_types_table, act->type);

    if (act->repeat == -1 &&
        !gst_structure_get_int (act->structure, "repeat", &act->repeat)) {
      gchar *error = NULL;
      const gchar *repeat_expr = gst_structure_get_string (act->structure,
          "repeat");

      if (repeat_expr) {
        act->repeat =
            gst_validate_utils_parse_expression (repeat_expr,
            _set_variable_func, scenario, &error);
      }
    }

    GST_DEBUG_OBJECT (scenario, "Executing %" GST_PTR_FORMAT
        " at %" GST_TIME_FORMAT, act->structure, GST_TIME_ARGS (position));
    if (!type->execute (scenario, act))
      GST_WARNING_OBJECT (scenario, "Could not execute %" GST_PTR_FORMAT,
          act->structure);

    if (act->repeat > 0) {
      act->repeat--;
    } else {
      tmp = priv->actions;
      priv->actions = g_list_remove_link (priv->actions, tmp);
      gst_mini_object_unref (GST_MINI_OBJECT (act));
      g_list_free (tmp);
    }
  }

  return TRUE;
}

static void
gst_validate_scenario_update_segment_from_seek (GstValidateScenario * scenario,
    GstEvent * seek)
{
  GstValidateScenarioPrivate *priv = scenario->priv;
  gint64 start, stop;
  GstSeekType start_type, stop_type;

  gst_event_parse_seek (seek, NULL, NULL, NULL, &start_type, &start,
      &stop_type, &stop);

  if (start_type == GST_SEEK_TYPE_SET) {
    priv->segment_start = start;
  } else if (start_type == GST_SEEK_TYPE_END) {
    /* TODO fill me */
  }

  if (stop_type == GST_SEEK_TYPE_SET) {
    priv->segment_stop = stop;
  } else if (start_type == GST_SEEK_TYPE_END) {
    /* TODO fill me */
  }
}

static gint
_compare_actions (GstValidateAction * a, GstValidateAction * b)
{
  if (a->action_number < b->action_number)
    return -1;
  else if (a->action_number == b->action_number)
    return 0;

  return 1;
}

static gboolean
message_cb (GstBus * bus, GstMessage * message, GstValidateScenario * scenario)
{
  GstValidateScenarioPrivate *priv = scenario->priv;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ASYNC_DONE:
      if (priv->last_seek) {
        gst_validate_scenario_update_segment_from_seek (scenario,
            priv->last_seek);
        gst_event_replace (&priv->last_seek, NULL);
      }

      if (priv->needs_parsing) {
        GList *tmp;

        for (tmp = priv->needs_parsing; tmp; tmp = tmp->next) {
          GstValidateAction *action = tmp->data;

          if (!gst_validate_action_get_clocktime (scenario, action,
                  "playback_time", &action->playback_time)) {
            gchar *str = gst_structure_to_string (action->structure);

            g_error ("Could not parse playback_time on structure: %s", str);
            g_free (str);

            return FALSE;
          }

          priv->actions = g_list_insert_sorted (priv->actions, action,
              (GCompareFunc) _compare_actions);
        }

        g_list_free (priv->needs_parsing);
        priv->needs_parsing = NULL;
      }

      if (priv->get_pos_id == 0) {
        get_position (scenario);
        priv->get_pos_id =
            g_timeout_add (50, (GSourceFunc) get_position, scenario);
      }
      break;
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_EOS:
    {
      if (scenario->priv->actions) {
        GList *tmp;
        guint nb_actions = 0;
        gchar *actions = g_strdup (""), *tmpconcat;

        for (tmp = scenario->priv->actions; tmp; tmp = tmp->next) {
          GstValidateAction *action = ((GstValidateAction *) tmp->data);
          tmpconcat = actions;

          if (g_strcmp0 (action->name, "eos"))
            continue;

          nb_actions++;
          actions = g_strdup_printf ("%s\n%*s%s",
              actions, 20, "", gst_structure_to_string (action->structure));
          g_free (tmpconcat);

        }

        if (nb_actions > 0)
          GST_VALIDATE_REPORT (scenario, SCENARIO_NOT_ENDED,
              "%i actions were not executed: %s", nb_actions, actions);
        g_free (actions);
      }
    }
    default:
      break;
  }

  return TRUE;
}

static void
_pipeline_freed_cb (GstValidateScenario * scenario,
    GObject * where_the_object_was)
{
  GstValidateScenarioPrivate *priv = scenario->priv;

  if (priv->get_pos_id) {
    g_source_remove (priv->get_pos_id);
    priv->get_pos_id = 0;
  }
  scenario->pipeline = NULL;

  GST_DEBUG_OBJECT (scenario, "pipeline was freed");
}

static gchar **
_scenario_file_get_lines (GFile * file)
{
  gsize size;

  GError *err = NULL;
  gchar *content = NULL, *escaped_content = NULL, **lines = NULL;

  /* TODO Handle GCancellable */
  if (!g_file_load_contents (file, NULL, &content, &size, NULL, &err))
    goto failed;

  if (g_strcmp0 (content, "") == 0)
    goto failed;

  escaped_content = g_regex_replace (clean_action_str, content, -1, 0, "", 0,
      NULL);
  g_free (content);

  lines = g_strsplit (escaped_content, "\n", 0);
  g_free (escaped_content);

done:

  return lines;

failed:
  if (err) {
    GST_WARNING ("Failed to load contents: %d %s", err->code, err->message);
    g_error_free (err);
  }

  if (content)
    g_free (content);
  content = NULL;

  if (escaped_content)
    g_free (escaped_content);
  escaped_content = NULL;

  if (lines)
    g_strfreev (lines);
  lines = NULL;

  goto done;
}

static gchar **
_scenario_get_lines (const gchar * scenario_file)
{
  GFile *file = NULL;
  gchar **lines = NULL;

  GST_DEBUG ("Trying to load %s", scenario_file);
  if ((file = g_file_new_for_path (scenario_file)) == NULL) {
    GST_WARNING ("%s wrong uri", scenario_file);
    return NULL;
  }

  lines = _scenario_file_get_lines (file);

  g_object_unref (file);

  return lines;
}

static GList *
_scenario_lines_get_strutures (gchar ** lines)
{
  gint i;
  GList *structures = NULL;

  for (i = 0; lines[i]; i++) {
    GstStructure *structure;

    if (g_strcmp0 (lines[i], "") == 0)
      continue;

    structure = gst_structure_from_string (lines[i], NULL);
    if (structure == NULL) {
      GST_ERROR ("Could not parse action %s", lines[i]);
      goto failed;
    }

    structures = g_list_append (structures, structure);
  }

done:
  if (lines)
    g_strfreev (lines);

  return structures;

failed:
  if (structures)
    g_list_free_full (structures, (GDestroyNotify) gst_structure_free);
  structures = NULL;

  goto done;
}

static GList *
_scenario_get_structures (const gchar * scenario_file)
{
  gchar **lines;

  lines = _scenario_get_lines (scenario_file);

  if (lines == NULL)
    return NULL;

  return _scenario_lines_get_strutures (lines);
}

static GList *
_scenario_file_get_structures (GFile * scenario_file)
{
  gchar **lines;

  lines = _scenario_file_get_lines (scenario_file);

  if (lines == NULL)
    return NULL;

  return _scenario_lines_get_strutures (lines);
}

static gboolean
_load_scenario_file (GstValidateScenario * scenario,
    const gchar * scenario_file, gboolean * is_config)
{
  gboolean ret = TRUE;
  GList *structures, *tmp;
  GstValidateScenarioPrivate *priv = scenario->priv;

  *is_config = FALSE;

  structures = _scenario_get_structures (scenario_file);
  if (structures == NULL)
    goto failed;

  for (tmp = structures; tmp; tmp = tmp->next) {
    gdouble playback_time;
    GstValidateAction *action;
    GstValidateActionType *action_type;
    const gchar *type, *str_playback_time = NULL;

    GstStructure *structure = tmp->data;


    type = gst_structure_get_name (structure);
    if (!g_strcmp0 (type, "description")) {
      gst_structure_get_boolean (structure, "is-config", is_config);
      continue;
    } else if (!(action_type = g_hash_table_lookup (action_types_table, type))) {
      GST_ERROR_OBJECT (scenario, "We do not handle action types %s", type);
      goto failed;
    }

    action = gst_validate_action_new ();
    action->type = type;
    action->repeat = -1;
    if (gst_structure_get_double (structure, "playback_time", &playback_time)) {
      action->playback_time = playback_time * GST_SECOND;
    } else if ((str_playback_time =
            gst_structure_get_string (structure, "playback_time"))) {
      priv->needs_parsing = g_list_append (priv->needs_parsing, action);
    } else
      GST_WARNING_OBJECT (scenario,
          "No playback time for action %" GST_PTR_FORMAT, structure);

    if (!(action->name = gst_structure_get_string (structure, "name")))
      action->name = "";

    action->structure = structure;

    if (action_type->is_config) {
      ret = action_type->execute (scenario, action);
      gst_mini_object_unref (GST_MINI_OBJECT (action));

      if (ret == FALSE)
        goto failed;

      continue;
    }

    action->action_number = priv->num_actions++;
    if (str_playback_time == NULL)
      priv->actions = g_list_append (priv->actions, action);
  }

done:
  if (structures)
    g_list_free (structures);

  return ret;

failed:
  ret = FALSE;
  if (structures)
    g_list_free_full (structures, (GDestroyNotify) gst_structure_free);
  structures = NULL;

  goto done;
}

static gboolean
gst_validate_scenario_load (GstValidateScenario * scenario,
    const gchar * scenario_name)
{
  gchar **scenarios;
  guint i;
  gchar *lfilename = NULL, *tldir = NULL;
  gboolean found_actions = FALSE, is_config, ret = TRUE;

  gchar **env_scenariodir =
      g_strsplit (g_getenv ("GST_VALIDATE_SCENARIOS_PATH"), ":",
      0);

  if (!scenario_name)
    goto invalid_name;

  scenarios = g_strsplit (scenario_name, ":", -1);

  for (i = 0; scenarios[i]; i++) {

    /* First check if the scenario name is not a full path to the
     * actual scenario */
    if (g_file_test (scenarios[i], G_FILE_TEST_IS_REGULAR)) {
      GST_DEBUG_OBJECT (scenario, "Scenario: %s is a full path to a scenario "
          "trying to load it", scenarios[i]);
      if ((ret = _load_scenario_file (scenario, scenario_name, &is_config)))
        goto check_scenario;
    }

    lfilename =
        g_strdup_printf ("%s" GST_VALIDATE_SCENARIO_SUFFIX, scenarios[i]);

    tldir = g_build_filename ("data/", lfilename, NULL);

    if ((ret = _load_scenario_file (scenario, tldir, &is_config)))
      goto check_scenario;

    g_free (tldir);

    if (env_scenariodir) {
      guint i;

      for (i = 0; env_scenariodir[i]; i++) {
        tldir = g_build_filename (env_scenariodir[i], lfilename, NULL);
        if ((ret = _load_scenario_file (scenario, tldir, &is_config)))
          goto check_scenario;
        g_free (tldir);
      }
    }

    /* Try from local profiles */
    tldir =
        g_build_filename (g_get_user_data_dir (),
        "gstreamer-" GST_API_VERSION, GST_VALIDATE_SCENARIO_DIRECTORY,
        lfilename, NULL);


    if (!(ret = _load_scenario_file (scenario, tldir, &is_config))) {
      g_free (tldir);
      /* Try from system-wide profiles */
      tldir = g_build_filename (GST_DATADIR, "gstreamer-" GST_API_VERSION,
          GST_VALIDATE_SCENARIO_DIRECTORY, lfilename, NULL);

      if (!(ret = _load_scenario_file (scenario, tldir, &is_config))) {
        goto invalid_name;
      }
    }
    /* else check scenario */
  check_scenario:
    if (tldir)
      g_free (tldir);
    if (lfilename)
      g_free (lfilename);

    if (!is_config) {
      if (found_actions == TRUE)
        goto one_actions_scenario_max;
      else
        found_actions = TRUE;
    }
  }

done:

  if (env_scenariodir)
    g_strfreev (env_scenariodir);

  if (ret == FALSE)
    g_error ("Could not set scenario %s => EXIT\n", scenario_name);

  return ret;

invalid_name:
  {
    GST_ERROR ("Invalid name for scenario '%s'", scenario_name);
    ret = FALSE;
    goto done;
  }
one_actions_scenario_max:
  {
    GST_ERROR ("You can set at most only one action scenario. "
        "You can have several config scenarios though (a config scenario's "
        "file must have is-config=true, and all its actions must be executable "
        "at parsing time).");
    ret = FALSE;
    goto done;

  }
}


static void
gst_validate_scenario_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_RUNNER:
      /* we assume the runner is valid as long as this scenario is,
       * no ref taken */
      gst_validate_reporter_set_runner (GST_VALIDATE_REPORTER (object),
          g_value_get_object (value));
      break;
    default:
      break;
  }
}

static void
gst_validate_scenario_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_RUNNER:
      /* we assume the runner is valid as long as this scenario is,
       * no ref taken */
      g_value_set_object (value,
          gst_validate_reporter_get_runner (GST_VALIDATE_REPORTER (object)));
      break;
    default:
      break;
  }
}

static void
gst_validate_scenario_class_init (GstValidateScenarioClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GstValidateScenarioPrivate));

  object_class->dispose = gst_validate_scenario_dispose;
  object_class->finalize = gst_validate_scenario_finalize;

  object_class->get_property = gst_validate_scenario_get_property;
  object_class->set_property = gst_validate_scenario_set_property;

  g_object_class_install_property (object_class, PROP_RUNNER,
      g_param_spec_object ("validate-runner", "VALIDATE Runner",
          "The Validate runner to " "report errors to",
          GST_TYPE_VALIDATE_RUNNER,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
gst_validate_scenario_init (GstValidateScenario * scenario)
{
  GstValidateScenarioPrivate *priv = scenario->priv =
      GST_VALIDATE_SCENARIO_GET_PRIVATE (scenario);

  priv->seek_pos_tol = DEFAULT_SEEK_TOLERANCE;
  priv->segment_start = 0;
  priv->segment_stop = GST_CLOCK_TIME_NONE;
}

static void
gst_validate_scenario_dispose (GObject * object)
{
  GstValidateScenarioPrivate *priv = GST_VALIDATE_SCENARIO (object)->priv;

  if (priv->last_seek)
    gst_event_unref (priv->last_seek);
  if (GST_VALIDATE_SCENARIO (object)->pipeline)
    gst_object_unref (GST_VALIDATE_SCENARIO (object)->pipeline);
  g_list_free_full (priv->actions, (GDestroyNotify) gst_mini_object_unref);


  G_OBJECT_CLASS (gst_validate_scenario_parent_class)->dispose (object);
}

static void
gst_validate_scenario_finalize (GObject * object)
{
  G_OBJECT_CLASS (gst_validate_scenario_parent_class)->finalize (object);
}

GstValidateScenario *
gst_validate_scenario_factory_create (GstValidateRunner *
    runner, GstElement * pipeline, const gchar * scenario_name)
{
  GstBus *bus;
  GstValidateScenario *scenario =
      g_object_new (GST_TYPE_VALIDATE_SCENARIO, "validate-runner",
      runner, NULL);

  GST_LOG ("Creating scenario %s", scenario_name);
  if (!gst_validate_scenario_load (scenario, scenario_name)) {
    g_object_unref (scenario);

    return NULL;
  }

  scenario->pipeline = pipeline;
  g_object_weak_ref (G_OBJECT (pipeline),
      (GWeakNotify) _pipeline_freed_cb, scenario);
  gst_validate_reporter_set_name (GST_VALIDATE_REPORTER (scenario),
      g_strdup (scenario_name));

  bus = gst_element_get_bus (pipeline);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", (GCallback) message_cb, scenario);
  gst_object_unref (bus);

  g_print ("\n=========================================\n"
      "Running scenario %s on pipeline %s"
      "\n=========================================\n", scenario_name,
      GST_OBJECT_NAME (pipeline));

  return scenario;
}

static gboolean
_add_description (GQuark field_id, const GValue * value, KeyFileGroupName * kfg)
{
  gchar *tmp = gst_value_serialize (value);

  g_key_file_set_string (kfg->kf, kfg->group_name,
      g_quark_to_string (field_id), g_strcompress (tmp));

  g_free (tmp);

  return TRUE;
}


static void
_list_scenarios_in_dir (GFile * dir, GKeyFile * kf)
{
  GFileEnumerator *fenum;
  GFileInfo *info;

  fenum = g_file_enumerate_children (dir, G_FILE_ATTRIBUTE_STANDARD_NAME,
      G_FILE_QUERY_INFO_NONE, NULL, NULL);

  if (fenum == NULL)
    return;

  for (info = g_file_enumerator_next_file (fenum, NULL, NULL);
      info; info = g_file_enumerator_next_file (fenum, NULL, NULL)) {
    if (g_str_has_suffix (g_file_info_get_name (info),
            GST_VALIDATE_SCENARIO_SUFFIX)) {
      gchar **name = g_strsplit (g_file_info_get_name (info),
          GST_VALIDATE_SCENARIO_SUFFIX, 0);


      GstStructure *desc = NULL;
      GFile *f = g_file_enumerator_get_child (fenum, info);
      GList *tmp, *structures = _scenario_file_get_structures (f);

      gst_object_unref (f);
      for (tmp = structures; tmp; tmp = tmp->next) {
        if (gst_structure_has_name (tmp->data, "description")) {
          desc = tmp->data;
          break;
        }
      }

      if (desc) {
        KeyFileGroupName kfg;

        kfg.group_name = name[0];
        kfg.kf = kf;

        gst_structure_foreach (desc,
            (GstStructureForeachFunc) _add_description, &kfg);
      } else {
        g_key_file_set_string (kf, name[0], "noinfo", "nothing");
      }
      g_list_free_full (structures, (GDestroyNotify) gst_structure_free);
      g_strfreev (name);
    }
  }
}

gboolean
gst_validate_list_scenarios (gchar * output_file)
{
  gchar *result;

  gsize datalength;
  GError *err = NULL;
  GKeyFile *kf = NULL;
  gchar **env_scenariodir =
      g_strsplit (g_getenv ("GST_VALIDATE_SCENARIOS_PATH"), ":",
      0);
  gchar *tldir = g_build_filename (g_get_user_data_dir (),
      "gstreamer-" GST_API_VERSION, GST_VALIDATE_SCENARIO_DIRECTORY,
      NULL);
  GFile *dir = g_file_new_for_path (tldir);

  kf = g_key_file_new ();
  _list_scenarios_in_dir (dir, kf);
  g_object_unref (dir);
  g_free (tldir);

  tldir = g_build_filename (GST_DATADIR, "gstreamer-" GST_API_VERSION,
      GST_VALIDATE_SCENARIO_DIRECTORY, NULL);
  dir = g_file_new_for_path (tldir);
  _list_scenarios_in_dir (dir, kf);
  g_object_unref (dir);
  g_free (tldir);

  if (env_scenariodir) {
    guint i;

    for (i = 0; env_scenariodir[i]; i++) {
      dir = g_file_new_for_path (env_scenariodir[i]);
      _list_scenarios_in_dir (dir, kf);
      g_object_unref (dir);
    }
  }

  /* Hack to make it work uninstalled */
  dir = g_file_new_for_path ("data/");
  _list_scenarios_in_dir (dir, kf);
  g_object_unref (dir);

  result = g_key_file_to_data (kf, &datalength, &err);
  g_print ("All scenarios avalaible:\n%s", result);

  if (output_file && !err)
    g_file_set_contents (output_file, result, datalength, &err);

  if (env_scenariodir)
    g_strfreev (env_scenariodir);

  if (err) {
    GST_WARNING ("Got error '%s' listing scenarios", err->message);
    g_clear_error (&err);

    return FALSE;
  }

  return TRUE;
}

static void
_free_action_type (GstValidateActionType * type)
{
  g_free (type->description);

  if (type->mandatory_fields)
    g_strfreev (type->mandatory_fields);
  g_free (type->description);

  g_slice_free (GstValidateActionType, type);
}

void
gst_validate_add_action_type (const gchar * type_name,
    GstValidateExecuteAction function, const gchar * const *mandatory_fields,
    const gchar * description, gboolean is_config)
{
  GstValidateActionType *type = g_slice_new0 (GstValidateActionType);

  if (action_types_table == NULL)
    action_types_table = g_hash_table_new_full (g_str_hash, g_str_equal,
        (GDestroyNotify) _free_action_type, NULL);

  type->execute = function;
  type->mandatory_fields = g_strdupv ((gchar **) mandatory_fields);
  type->description = g_strdup (description);
  type->is_config = is_config;

  g_hash_table_insert (action_types_table, g_strdup (type_name), type);
}

void
init_scenarios (void)
{
  const gchar *seek_mandatory_fields[] = { "start", NULL };

  _gst_validate_action_type = gst_validate_action_get_type ();

  clean_action_str = g_regex_new ("\\\\\n", G_REGEX_CASELESS, 0, NULL);
  gst_validate_add_action_type ("seek", _execute_seek, seek_mandatory_fields,
      "Allows to seek into the files", FALSE);
  gst_validate_add_action_type ("pause", _execute_pause, NULL,
      "Make it possible to set pipeline to PAUSED, you can add a duration"
      " parametter so the pipeline goaes back to playing after that duration"
      " (in second)", FALSE);
  gst_validate_add_action_type ("play", _execute_play, NULL,
      "Make it possible to set the pipeline state to PLAYING", FALSE);
  gst_validate_add_action_type ("eos", _execute_eos, NULL,
      "Make it possible to send an EOS to the pipeline", FALSE);
  gst_validate_add_action_type ("switch-track", _execute_switch_track, NULL,
      "The 'switch-track' command can be used to switch tracks.\n"
      "The 'type' argument selects which track type to change (can be 'audio', 'video',"
      " or 'text'). The 'index' argument selects which track of this type"
      " to use: it can be either a number, which will be the Nth track of"
      " the given type, or a number with a '+' or '-' prefix, which means"
      " a relative change (eg, '+1' means 'next track', '-1' means 'previous"
      " track'), note that you need to state that it is a string in the scenario file"
      " prefixing it with (string).", FALSE);
  gst_validate_add_action_type ("set-feature-rank", _set_rank, NULL,
      "Allows you to change the ranking of a particular plugin feature", TRUE);
}
