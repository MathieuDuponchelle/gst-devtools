/* GStreamer
 *
 * Copyright (C) 2013 Thibault Saunier <thibault.saunier@collabora.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _GST_VALIDATE_REPORTER_
#define _GST_VALIDATE_REPORTER_

typedef struct _GstValidateReporter GstValidateReporter;
typedef struct _GstValidateReporterInterface GstValidateReporterInterface;

#include <glib-object.h>
#include <gst/validate/gst-validate-report.h>
#include <gst/validate/gst-validate-runner.h>

G_BEGIN_DECLS

/* GstValidateReporter interface declarations */
#define GST_TYPE_VALIDATE_REPORTER                (gst_validate_reporter_get_type ())
#define GST_VALIDATE_REPORTER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_VALIDATE_REPORTER, GstValidateReporter))
#define GST_IS_VALIDATE_REPORTER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_VALIDATE_REPORTER))
#define GST_VALIDATE_REPORTER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GST_TYPE_VALIDATE_REPORTER, GstValidateReporterInterface))
#define GST_VALIDATE_REPORTER_CAST(obj)           ((GstValidateReporter *) obj)

#ifdef G_HAVE_ISO_VARARGS
#define GST_VALIDATE_REPORT(m, issue_id, ...)				\
  G_STMT_START {							\
    gst_validate_report (GST_VALIDATE_REPORTER (m),			\
			 issue_id,		\
			 __VA_ARGS__ );					\
  } G_STMT_END

#else /* G_HAVE_GNUC_VARARGS */
#ifdef G_HAVE_GNUC_VARARGS
#define GST_VALIDATE_REPORT(m, issue_id, args...)			\
  G_STMT_START {							\
    gst_validate_report (GST_VALIDATE_REPORTER (m),			\
			 issue_id, ##args );	\
  } G_STMT_END

#endif /* G_HAVE_ISO_VARARGS */
#endif /* G_HAVE_GNUC_VARARGS */

GType gst_validate_reporter_get_type (void);

/**
 * GstValidateReporter:
 */
struct _GstValidateReporterInterface
{
  GTypeInterface parent;

  void (*intercept_report)(GstValidateReporter * reporter, GstValidateReport * report);
};

void gst_validate_reporter_set_name            (GstValidateReporter * reporter,
                                          gchar * name);
const gchar * gst_validate_reporter_get_name            (GstValidateReporter * reporter);
GstValidateRunner * gst_validate_reporter_get_runner (GstValidateReporter *reporter);
void gst_validate_reporter_init                (GstValidateReporter * reporter, const gchar *name);
void gst_validate_report                       (GstValidateReporter * reporter, GstValidateIssueId issue_id,
                                          const gchar * format, ...);
void gst_validate_report_valist                (GstValidateReporter * reporter, GstValidateIssueId issue_id,
                                          const gchar * format, va_list var_args);

void gst_validate_reporter_set_runner          (GstValidateReporter * reporter, GstValidateRunner *runner);
void gst_validate_reporter_set_handle_g_logs   (GstValidateReporter * reporter);

G_END_DECLS
#endif /* _GST_VALIDATE_REPORTER_ */
