/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thiago Sousa Santos <thiago.sousa.santos@collabora.com>
 *
 * gst-validate-override.c - Validate Override that allows customizing Validate behavior
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

#include "gst-validate-internal.h"
#include "gst-validate-override.h"

static void gst_validate_override_dispose (GObject * object);


G_DEFINE_TYPE (GstValidateOverride, gst_validate_override, G_TYPE_OBJECT);

static void
gst_validate_override_class_init (GstValidateOverrideClass * klass)
{
  GObjectClass *g_object_class;

  g_object_class = (GObjectClass *) klass;
  g_object_class->dispose = gst_validate_override_dispose;
}

static void
gst_validate_override_dispose (GObject * object)
{
  g_hash_table_unref (GST_VALIDATE_OVERRIDE (object)->level_override);
}

static void
gst_validate_override_init (GstValidateOverride * override)
{
  override->level_override = g_hash_table_new (g_direct_hash, g_direct_equal);
}

GstValidateOverride *
gst_validate_override_new (void)
{
  return g_object_new (GST_TYPE_VALIDATE_OVERRIDE, NULL);
}

void
gst_validate_override_set_event_handler (GstValidateOverride * override,
    GstValidateOverrideEventHandler handler)
{
  override->event_handler = handler;
}

void
gst_validate_override_set_buffer_handler (GstValidateOverride * override,
    GstValidateOverrideBufferHandler handler)
{
  override->buffer_handler = handler;
}

void
gst_validate_override_set_query_handler (GstValidateOverride * override,
    GstValidateOverrideQueryHandler handler)
{
  override->query_handler = handler;
}

void
gst_validate_override_set_buffer_probe_handler (GstValidateOverride * override,
    GstValidateOverrideBufferHandler handler)
{
  override->buffer_probe_handler = handler;
}

void
gst_validate_override_set_getcaps_handler (GstValidateOverride * override,
    GstValidateOverrideGetCapsHandler handler)
{
  override->getcaps_handler = handler;
}

void
gst_validate_override_set_setcaps_handler (GstValidateOverride * override,
    GstValidateOverrideSetCapsHandler handler)
{
  override->setcaps_handler = handler;
}

void
gst_validate_override_set_report_handler (GstValidateOverride * override,
    GstValidateOverrideReportHandler handler)
{
  override->report_handler = handler;
}

void
gst_validate_override_event_handler (GstValidateOverride * override,
    GstValidateMonitor * monitor, GstEvent * event)
{
  if (override->event_handler)
    override->event_handler (override, monitor, event);
}

void
gst_validate_override_buffer_handler (GstValidateOverride * override,
    GstValidateMonitor * monitor, GstBuffer * buffer)
{
  if (override->buffer_handler)
    override->buffer_handler (override, monitor, buffer);
}

void
gst_validate_override_query_handler (GstValidateOverride * override,
    GstValidateMonitor * monitor, GstQuery * query)
{
  if (override->query_handler)
    override->query_handler (override, monitor, query);
}

void
gst_validate_override_buffer_probe_handler (GstValidateOverride * override,
    GstValidateMonitor * monitor, GstBuffer * buffer)
{
  if (override->buffer_probe_handler)
    override->buffer_probe_handler (override, monitor, buffer);
}

void
gst_validate_override_getcaps_handler (GstValidateOverride * override,
    GstValidateMonitor * monitor, GstCaps * caps)
{
  if (override->getcaps_handler)
    override->getcaps_handler (override, monitor, caps);
}

void
gst_validate_override_setcaps_handler (GstValidateOverride * override,
    GstValidateMonitor * monitor, GstCaps * caps)
{
  if (override->setcaps_handler)
    override->setcaps_handler (override, monitor, caps);
}

void
gst_validate_override_report_handler (GstValidateOverride * override,
    GstValidateReporter * reporter, GstValidateReport * report)
{
  if (override->report_handler)
    override->report_handler (override, reporter, report);
}
