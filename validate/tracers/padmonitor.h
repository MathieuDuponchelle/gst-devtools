#ifndef __PAD_MONITOR_H__
#define __PAD_MONITOR_H__

#include <glib-object.h>

typedef struct _PadMonitor PadMonitor;
typedef struct _PadMonitorClass PadMonitorClass;

#include "monitor.h"

G_BEGIN_DECLS

#define TYPE_PAD_MONITOR			(pad_monitor_get_type ())
#define IS_PAD_MONITOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PAD_MONITOR))
#define IS_PAD_MONITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PAD_MONITOR))
#define PAD_MONITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PAD_MONITOR, PadMonitorClass))
#define PAD_MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PAD_MONITOR, PadMonitor))
#define PAD_MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PAD_MONITOR, PadMonitorClass))
#define PAD_MONITOR_CAST(obj)            ((PadMonitor*)(obj))
#define PAD_MONITOR_CLASS_CAST(klass)    ((PadMonitorClass*)(klass))

struct _PadMonitor {
  Monitor 	 parent;
};

/**
 * PadMonitorClass:
 * @parent_class: parent
 *
 * GStreamer Validate PadMonitor object class.
 */
struct _PadMonitorClass {
  MonitorClass	parent_class;
};

/* normal GObject stuff */
GType		pad_monitor_get_type		(void);

G_END_DECLS

#endif /* __PAD_MONITOR_H__ */

