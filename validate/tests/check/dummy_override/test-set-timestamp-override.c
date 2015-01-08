/*
 * gst-validate set timestamp override
 *
 * Copyright 2014 Mathieu Duponchelle <mathieu.duponchelle@collabora.com>.
 *
 * test-set-timestamp-override.c: Test override that sets timestamps on buffers.
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

#include "test-set-timestamp-override.h"

G_DEFINE_TYPE (TestSetTimestampOverride, test_set_timestamp_override,
    GST_TYPE_VALIDATE_OVERRIDE);

static void
my_override_buffer_handler (GstValidateOverride * override,
    GstValidateMonitor * pad_monitor, GstBuffer * buffer)
{
  GST_BUFFER_PTS (buffer) = 42 * GST_SECOND;
}

static void
test_set_timestamp_override_class_init (TestSetTimestampOverrideClass * klass)
{
}

static void
test_set_timestamp_override_init (TestSetTimestampOverride * override)
{
  gst_validate_override_set_buffer_handler ((GstValidateOverride *) override,
      my_override_buffer_handler);
}
