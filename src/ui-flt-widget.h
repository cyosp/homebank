/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2025 Maxime DOYEN
 *
 *  This file is part of HomeBank.
 *
 *  HomeBank is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  HomeBank is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HB_FILTER_WIDGET_GTK_H__
#define __HB_FILTER_WIDGET_GTK_H__

/* list display account */
enum
{
	LST_FAVFLT_KEY,
	LST_FAVFLT_NAME,
	NUM_LST_FAVFLT
};


struct ui_flt_popover_data
{
	Filter		*filter;

	GSimpleActionGroup *actions;

	GtkWindow	*parent;
	GtkWidget	*box;
	GtkWidget	*combobox;
	GtkWidget	*menubutton;

};



/* = = = = = = = = = = */

GtkListStore *lst_lst_favfilter_model_new(void);

void
ui_flt_popover_hub_save (GtkWidget *widget, gpointer user_data);
void ui_flt_manage_header_sensitive(GtkWidget *widget, gpointer user_data);
GtkWidget *ui_flt_popover_hub_get_combobox(GtkBox *box, gpointer user_data);
Filter *ui_flt_popover_hub_get(GtkBox *box, gpointer user_data);

GtkWidget *create_popover_widget(GtkWindow *parent, Filter *filter);


#endif