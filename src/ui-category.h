/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2023 Maxime DOYEN
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

#ifndef __HB_CATEGORY_GTK_H__
#define __HB_CATEGORY_GTK_H__

enum
{
	LST_DEFCAT_TOGGLE,
	LST_DEFCAT_DATAS,
	NUM_LST_DEFCAT
};

#define LST_DEFCAT_SORT_NAME	1
#define LST_DEFCAT_SORT_USETXN	2
#define LST_DEFCAT_SORT_USECFG	3


//liststore
enum
{
	STO_CAT_DATA,
	STO_CAT_FULLNAME,
	NUM_STO_CAT
};

enum
{
	CAT_TYPE_ALL,
	CAT_TYPE_EXPENSE,
	CAT_TYPE_INCOME
};


/*
enum
{
	LST_CMBCAT_DATAS,
	LST_CMBCAT_FULLNAME,
	LST_CMBCAT_SORTNAME,
	LST_CMBCAT_NAME,
	LST_CMBCAT_SUBCAT,
	NUM_LST_CMBCAT
};
*/


//TODO: only used into vehicle
void ui_cat_entry_popover_clear(GtkBox *box);
void ui_cat_entry_popover_sort_type(GtkBox *box, guint type);
void ui_cat_entry_popover_add(GtkBox *box, Category *item);

GtkWidget *ui_cat_entry_popover_get_entry(GtkBox *box);
Category *ui_cat_entry_popover_get(GtkBox *box);
guint32 ui_cat_entry_popover_get_key_add_new(GtkBox *box);
guint32 ui_cat_entry_popover_get_key(GtkBox *box);
void ui_cat_entry_popover_set_active(GtkBox *box, guint32 key);
GtkWidget *ui_cat_entry_popover_new(GtkWidget *label);


/* = = = = = = = = = = */

void ui_cat_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter);
void ui_cat_listview_quick_select(GtkTreeView *treeview, const gchar *uri);

void ui_cat_listview_add(GtkTreeView *treeview, Category *item, GtkTreeIter	*parent);
Category *ui_cat_listview_get_selected(GtkTreeView *treeview);
Category *ui_cat_listview_get_selected_parent(GtkTreeView *treeview, GtkTreeIter *parent);
void ui_cat_listview_remove_selected(GtkTreeView *treeview);
void ui_cat_listview_populate(GtkWidget *view, gint type, gchar *needle, gboolean showhidden);
GtkWidget *ui_cat_listview_new(gboolean withtoggle, gboolean withcount);

/* = = = = = = = = = = */

struct ui_cat_manage_dialog_data
{
	GList	*tmp_list;
	gboolean	usagefilled;
	gint	change;

	GtkWidget	*dialog;
	gboolean	mapped_done;

	GtkWidget	*BT_showhidden;
	GtkWidget	*BT_showusage;
	GtkWidget	*ST_search;

	GtkWidget	*LV_cat;
	GtkWidget	*RE_addreveal;
	GtkWidget	*ST_name1, *ST_name2;


	//GtkWidget	*CM_type;
	GtkWidget	*RA_type;

	GtkWidget	*BT_add;
	GtkWidget	*BT_edit;
	GtkWidget	*BT_merge;
	GtkWidget	*BT_delete;
	GtkWidget	*BT_hide;
	
	GtkWidget	*BT_expand;
	GtkWidget	*BT_collapse;

	GtkWidget	*LA_category;

};

struct catPopContext
{
	GtkTreeModel *model;
	guint	except_key;
	gint	type;
};

GtkWidget *ui_cat_manage_dialog (void);

#endif

