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

#ifndef __HB_PAYEE_GTK_H__
#define __HB_PAYEE_GTK_H__

enum
{
	LST_DEFPAY_TOGGLE,
	LST_DEFPAY_DATAS,
	NUM_LST_DEFPAY
};


#define LST_DEFPAY_SORT_USETXN	 2
#define LST_DEFPAY_SORT_USECFG	 3
#define LST_DEFPAY_SORT_NAME	 4
#define LST_DEFPAY_SORT_DEFPAY   5
#define LST_DEFPAY_SORT_DEFCAT   6


struct ui_pay_manage_dialog_data
{
	GtkWidget	*dialog;
	GActionGroup * actions;

	gboolean	mapped_done;

	GtkWidget	*BT_showhidden;
	GtkWidget	*BT_showusage;
	GtkWidget	*ST_search;
	
	GtkWidget	*RE_addreveal;
	GtkWidget	*ST_name;
	GtkWidget	*LV_pay;

	GtkWidget	*BT_add;
	GtkWidget	*BT_edit;
	GtkWidget	*BT_merge;
	GtkWidget	*BT_delete;
	GtkWidget	*BT_hide;

	gboolean	usagefilled;
	gint		change;
};


struct payPopContext
{
	GtkTreeModel *model;
	guint	except_key;
};



/* = = = = = = = = = = */

GtkWidget *ui_pay_entry_popover_get_entry(GtkBox *box);
Payee *ui_pay_entry_popover_get(GtkBox *box);
guint32 ui_pay_entry_popover_get_key_add_new(GtkBox *box);
guint32 ui_pay_entry_popover_get_key(GtkBox *box);
void ui_pay_entry_popover_set_active(GtkBox *box, guint32 key);
GtkWidget *ui_pay_entry_popover_new(GtkWidget *label);

/* = = = = = = = = = = */

guint ui_pay_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter);
void ui_pay_listview_quick_select(GtkTreeView *treeview, const gchar *uri);

void ui_pay_listview_add(GtkTreeView *treeview, Payee *item);
guint32 ui_pay_listview_get_selected_key(GtkTreeView *treeview);
void ui_pay_listview_remove_selected(GtkTreeView *treeview);
void ui_pay_listview_populate(GtkWidget *treeview, gchar *needle, gboolean showhidden);
GtkWidget *ui_pay_listview_new(gboolean withtoggle, gboolean withcount);

GtkWidget *ui_pay_manage_dialog (void);

#endif

