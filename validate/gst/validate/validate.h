/* GStreamer
 * Copyright (C) 2013 Thiago Santos <thiago.sousa.santos@collabora.com>
 */

#ifndef _GST_VALIDATE_H
#define _GST_VALIDATE_H

#include <gst/validate/gst-validate-runner.h>
#include <gst/validate/gst-validate-monitor-factory.h>
#include <gst/validate/gst-validate-override-registry.h>
#include <gst/validate/gst-validate-report.h>
#include <gst/validate/gst-validate-reporter.h>
#include <gst/validate/gst-validate-media-info.h>

void gst_validate_init (void);
GstRegistry * gst_validate_registry_get (void);
gboolean gst_validate_scan_plugin_path (GstRegistry *registry);

#endif /* _GST_VALIDATE_H */
