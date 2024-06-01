#ifndef __HB_DATE_HELPER_H__
#define __HB_DATE_HELPER_H__

#include <glib.h>
#include <gtk/gtk.h>

void moveToEndOfMonth(GDate * date);
guint32 getJulianDateEndOfMonth(GtkWidget * gtkDateWidget);

#endif
