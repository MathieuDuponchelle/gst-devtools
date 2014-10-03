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

#include "test-utils.h"

typedef struct _DestroyedObjectStruct
{
  GObject *object;
  gboolean destroyed;
} DestroyedObjectStruct;

static void
weak_notify (DestroyedObjectStruct * destroyed, GObject ** object)
{
  destroyed->destroyed = TRUE;
}

void
check_destroyed (gpointer object_to_unref, gpointer first_object, ...)
{
  gint i = 0;
  GObject *object;
  GList *objs = NULL, *tmp;
  DestroyedObjectStruct *destroyed = g_slice_new0 (DestroyedObjectStruct);

  destroyed->object = G_OBJECT (object_to_unref);
  g_object_weak_ref (G_OBJECT (object_to_unref), (GWeakNotify) weak_notify,
      destroyed);
  objs = g_list_prepend (objs, destroyed);

  if (first_object) {
    va_list varargs;

    object = G_OBJECT (first_object);

    va_start (varargs, first_object);
    while (object) {
      destroyed = g_slice_new0 (DestroyedObjectStruct);
      destroyed->object = object;
      g_object_weak_ref (object, (GWeakNotify) weak_notify, destroyed);
      objs = g_list_append (objs, destroyed);
      object = va_arg (varargs, GObject *);
    }
    va_end (varargs);
  }
  gst_object_unref (object_to_unref);

  for (tmp = objs; tmp; tmp = tmp->next) {
    fail_unless (((DestroyedObjectStruct *) tmp->data)->destroyed == TRUE,
        "%p is not destroyed (object nb %i)",
        ((DestroyedObjectStruct *) tmp->data)->object, i);
    g_slice_free (DestroyedObjectStruct, tmp->data);
    i++;
  }
  g_list_free (objs);

}

void
clean_bus (GstElement * element)
{
  GstBus *bus;

  bus = gst_element_get_bus (element);
  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);
}

GstValidatePadMonitor *
get_pad_monitor (GstPad * pad)
{
  return g_object_get_data ((GObject *) pad, "validate-monitor");
}

static GstStaticPadTemplate fake_demuxer_src_template =
GST_STATIC_PAD_TEMPLATE ("src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("something")
    );

static GstStaticPadTemplate fake_demuxer_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("something")
    );

static void
fake_demuxer_dispose (FakeDemuxer * self)
{
}

static void
fake_demuxer_finalize (FakeDemuxer * self)
{
}

static GstFlowReturn
_chain (GstPad * pad, GstObject * self, GstBuffer * buffer)
{
  gst_buffer_unref (buffer);

  return FAKE_DEMUXER (self)->return_value;
}

static void
fake_demuxer_init (FakeDemuxer * self, gpointer * g_class)
{
  GstPad *pad;
  GstElement *element = GST_ELEMENT (self);
  GstPadTemplate *pad_template;

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (g_class), "src%u");

  pad = gst_pad_new_from_template (pad_template, "src0");
  gst_element_add_pad (element, pad);

  pad = gst_pad_new_from_template (pad_template, "src1");
  gst_element_add_pad (element, pad);

  pad = gst_pad_new_from_template (pad_template, "src2");
  gst_element_add_pad (element, pad);

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (g_class), "sink");
  pad = gst_pad_new_from_template (pad_template, "sink");
  gst_element_add_pad (element, pad);

  self->return_value = GST_FLOW_OK;

  gst_pad_set_chain_function (pad, _chain);
}

static void
fake_demuxer_class_init (FakeDemuxerClass * self_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (self_class);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (self_class);

  object_class->dispose = (void (*)(GObject * object)) fake_demuxer_dispose;
  object_class->finalize = (void (*)(GObject * object)) fake_demuxer_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fake_demuxer_src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&fake_demuxer_sink_template));
  gst_element_class_set_static_metadata (gstelement_class,
      "Fake Demuxer", "Demuxer", "Some demuxer", "Thibault Saunier");
}

GType
fake_demuxer_get_type (void)
{
  static volatile gsize type = 0;

  if (g_once_init_enter (&type)) {
    GType _type;
    static const GTypeInfo info = {
      sizeof (FakeDemuxerClass),
      NULL,
      NULL,
      (GClassInitFunc) fake_demuxer_class_init,
      NULL,
      NULL,
      sizeof (FakeDemuxer),
      0,
      (GInstanceInitFunc) fake_demuxer_init,
    };

    _type = g_type_register_static (GST_TYPE_ELEMENT, "FakeDemuxer", &info, 0);
    g_once_init_leave (&type, _type);
  }
  return type;
}

GstElement *
fake_demuxer_new (void)
{
  return GST_ELEMENT (g_object_new (FAKE_DEMUXER_TYPE, NULL));
}

GstElement * create_and_monitor_element (const gchar *factoryname, const gchar *name,
    GstValidateRunner *runner)
{
  GstElement *element;
  GstValidateMonitor *monitor;

  element = gst_element_factory_make (factoryname, name);
  if (runner) {
    monitor =
        gst_validate_monitor_factory_create (GST_OBJECT (element), runner, NULL);
    gst_validate_reporter_set_handle_g_logs (GST_VALIDATE_REPORTER (monitor));
    fail_unless (GST_IS_VALIDATE_ELEMENT_MONITOR (monitor));
  }

 return element;
}
