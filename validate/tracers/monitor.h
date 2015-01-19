#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <glib-object.h>
#include <gst/gsttracer.h>

typedef struct _Monitor Monitor;
typedef struct _MonitorClass MonitorClass;

G_BEGIN_DECLS

#define TYPE_MONITOR			(monitor_get_type ())
#define IS_MONITOR(obj)		        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MONITOR))
#define IS_MONITOR_CLASS(klass)	        (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MONITOR))
#define MONITOR_GET_CLASS(obj)	        (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MONITOR, MonitorClass))
#define MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MONITOR, Monitor))
#define MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MONITOR, MonitorClass))
#define MONITOR_CAST(obj)                ((Monitor*)(obj))
#define MONITOR_CLASS_CAST(klass)        ((MonitorClass*)( klass))

struct _Monitor {
  GObject 	 object;
  GObject    *target;
  GstTracer *tracer;
};

struct _MonitorClass {
  GObjectClass	parent_class;
};

GType		monitor_get_type		(void);

void    monitor_register    (Monitor *, GstTracer *, GObject *);
Monitor * get_monitor_for_type_name (GObject *object, const gchar *);

G_END_DECLS

#endif /* __MONITOR_H__ */

