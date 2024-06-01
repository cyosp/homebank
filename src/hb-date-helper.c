#include "hb-date-helper.h"

#include "gtk-dateentry.h"

void moveToEndOfMonth(GDate * date) {
    while(!g_date_is_last_of_month(date)) {
        g_date_add_days(date, 1);
    }
}

guint32 getJulianDateEndOfMonth(GtkWidget * gtkDateWidget) {
    GDate * date = g_date_new_julian(gtk_date_entry_get_date(GTK_DATE_ENTRY(gtkDateWidget)));
    moveToEndOfMonth(date);
    guint32 dateEndOfMonth = g_date_get_julian(date);
    g_date_free(date);
    return dateEndOfMonth;
}
