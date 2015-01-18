#ifndef __ELEMENT_MONITOR_H__
#define __ELEMENT_MONITOR_H__

#include <glib-object.h>

typedef struct _ElementMonitor ElementMonitor;
typedef struct _ElementMonitorClass ElementMonitorClass;

#include "monitor.h"

G_BEGIN_DECLS

#define TYPE_ELEMENT_MONITOR			(element_monitor_get_type ())
#define IS_ELEMENT_MONITOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_ELEMENT_MONITOR))
#define IS_ELEMENT_MONITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_ELEMENT_MONITOR))
#define ELEMENT_MONITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_ELEMENT_MONITOR, ElementMonitorClass))
#define ELEMENT_MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_ELEMENT_MONITOR, ElementMonitor))
#define ELEMENT_MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_ELEMENT_MONITOR, ElementMonitorClass))
#define ELEMENT_MONITOR_CAST(obj)            ((ElementMonitor*)(obj))
#define ELEMENT_MONITOR_CLASS_CAST(klass)    ((ElementMonitorClass*)(klass))

struct _ElementMonitor {
  Monitor 	 parent;

  gboolean is_decoder;
  gboolean is_encoder;
  gboolean is_demuxer;
  gboolean is_converter;
};

/**
 * ElementMonitorClass:
 * @parent_class: parent
 *
 * GStreamer Validate ElementMonitor object class.
 */
struct _ElementMonitorClass {
  MonitorClass	parent_class;
};

/* normal GObject stuff */
GType		element_monitor_get_type		(void);
void element_monitor_inspect (ElementMonitor *monitor);

G_END_DECLS

#endif /* __ELEMENT_MONITOR_H__ */
