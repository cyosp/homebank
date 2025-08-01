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

#ifndef __HB_ACCOUNT_GTK_H__
#define __HB_ACCOUNT_GTK_H__


enum {
	LST_DEFACC_SORT_POS = 1,
	LST_DEFACC_SORT_NAME
};


enum
{
	LST_DEFACC_TOGGLE,
	LST_DEFACC_DATAS,
	NUM_LST_DEFACC
};

enum
{
	ACC_LST_INSERT_NORMAL,
	ACC_LST_INSERT_REPORT
};


/* = = = = = = = = = = */

enum
{
	ACTION_NEW,
	ACTION_MODIFY,
	ACTION_REMOVE,
};

enum
{
	FIELD_NAME,
	//todo: for stock account	
	//FIELD_TYPE,
	FIELD_BANK,
	FIELD_NUMBER,
	FIELD_BUDGET,
	FIELD_CLOSED,
	FIELD_INITIAL,
	FIELD_MINIMUM,
	FIELD_CHEQUE1,
	FIELD_CHEQUE2,
	MAX_ACC_FIELD
};


struct ui_acc_manage_data
{
	GList	*tmp_list;
	gint	change;
	gint	action;
	guint32	lastkey;

	GtkWidget	*dialog;
	gboolean	mapped_done;

	GtkWidget	*ST_search;
	GtkWidget	*LV_acc;
	GtkWidget	*BT_add, *BT_edit, *BT_rem;
	GtkWidget	*BT_up, *BT_down;

	GtkWidget   *notebook;

	GtkWidget	*CY_type;
	GtkWidget	*CY_curr;
	GtkWidget	*ST_institution;
	GtkWidget	*ST_number;
	GtkWidget	*ST_group;
	GtkWidget	*ST_website;
	GtkWidget	*TB_notes;
	GtkWidget	*CM_closed;

	GtkWidget	*ST_initial;
	//GtkWidget	*ST_warning;
	GtkWidget	*ST_minimum;
	GtkWidget	*ST_maximum;

	GtkWidget	*CY_template;
	GtkWidget	*CM_nosummary;
	GtkWidget	*CM_nobudget;
	GtkWidget	*CM_noreport;
	GtkWidget	*CM_outflowsum;
	
	GtkWidget	*ST_cheque1;
	GtkWidget	*ST_cheque2;
};


struct accPopContext
{
	GtkTreeModel *model;
	guint32	except_key;
	//guint32 kcur;
	gint	insert_type;
};


GtkWidget *ui_acc_manage_dialog (void);

/* = = = = = = = = = = */

void ui_acc_entry_popover_populate(GtkBox *box, GHashTable *hash, gint insert_type);
void ui_acc_entry_popover_populate_except(GtkBox *box, GHashTable *hash, guint except_key, gint insert_type);
GtkTreeModel *ui_acc_entry_popover_get_model(GtkBox *box);
GtkWidget *ui_acc_entry_popover_get_entry(GtkBox *box);
Account *ui_acc_entry_popover_get(GtkBox *box);
guint32 ui_acc_entry_popover_get_key_add_new(GtkBox *box);
guint32 ui_acc_entry_popover_get_key(GtkBox *box);
void ui_acc_entry_popover_set_single(GtkBox *box);
void ui_acc_entry_popover_set_active(GtkBox *box, guint32 key);
GtkWidget *ui_acc_entry_popover_new(GtkWidget *label);


/* = = = = = = = = = = */

guint ui_acc_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter);

void ui_acc_listview_quick_select(GtkTreeView *treeview, const gchar *uri);


void ui_acc_listview_set_active(GtkTreeView *treeview, guint32 key);
void ui_acc_listview_add(GtkTreeView *treeview, Account *item);
guint32 ui_acc_listview_get_selected_key(GtkTreeView *treeview);
void ui_acc_listview_remove_selected(GtkTreeView *treeview);
void ui_acc_listview_populate(GtkWidget *view, gint insert_type, gchar *needle);
GtkWidget *ui_acc_listview_new(gboolean withtoggle);

#endif

