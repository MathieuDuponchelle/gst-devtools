/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Vincent Penquerc'h <vincent.penquerch@collabora.co.uk>
 *
 * gst-validate-default-overrides.c - Test overrides
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

#include <gst/gst.h>
#include <gst/validate/validate.h>
#include <gst/validate/gst-validate-override-factory.h>

#include "test-set-timestamp-override.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_validate_override_factory_register (plugin, "set-timestamp",
          GST_RANK_NONE, test_set_timestamp_override_get_type ()))
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, coreoverrides,
    "gst-validate dummy overrides", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
