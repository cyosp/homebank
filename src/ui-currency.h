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

#ifndef __HB_CURRENCY_GTK_H__
#define __HB_CURRENCY_GTK_H__

enum
{
	LST_DEFCUR_TOGGLE,
	LST_DEFCUR_DATAS,
	NUM_LST_DEFCUR
};

enum
{
	LST_SELCUR_NAME,
	LST_SELCUR_DATA,
	NUM_LST_SELCUR
};


struct ui_cur_manage_dialog_data
{
	GtkWidget	*dialog;
	gboolean	mapped_done;

	GtkWidget	*LV_cur;
//	GtkWidget	*CY_curr;
//	GtkWidget	*BT_curr;
	GtkWidget	*TB_log;
	
	GtkWidget   *BB_update;
	GtkWidget	*BT_add;
	GtkWidget	*BT_del;
	GtkWidget	*BT_edit;
	GtkWidget	*BT_base;

	gint	change;
};


struct ui_cur_edit_dialog_data
{
	GtkWidget	*dialog;

	GtkWidget	*LB_name;
	GtkWidget	*LB_rate;
	GtkWidget	*NB_rate;

	GtkWidget	*LB_sample;
	
	GtkWidget	*ST_symbol;
	GtkWidget   *CM_symisprefix;
	GtkWidget	*ST_decimalchar;	
	GtkWidget	*ST_groupingchar;	
	GtkWidget	*NB_fracdigits;

};

enum {
	CUR_SELECT_MODE_BASE,
	CUR_SELECT_MODE_NOCUSTOM,
	CUR_SELECT_MODE_CUSTOM,
};


struct ui_cur_select_dialog_data
{
	GtkWidget	*dialog;

	GtkTreeModel *modelfilter;
	GtkTreeModel *sortmodel;
	GtkTreeModel *model;
	GtkWidget	*ST_search;
	GtkWidget	*LV_cur;
	GtkWidget	*CM_custom;
	GtkWidget	*LB_custiso, *ST_custiso;
	GtkWidget	*LB_custname, *ST_custname;
};


struct curPopContext
{
	GtkTreeModel *model;
	guint	except_key;
};


struct curSelectContext
{
	Currency4217	*cur_4217;
	gchar			*cur_name;
	gchar			*cur_iso;
};



gchar *ui_cur_combobox_get_name(GtkComboBox *entry_box);
guint32 ui_cur_combobox_get_key(GtkComboBox *entry_box);
guint32 ui_cur_combobox_get_key_add_new(GtkComboBox *entry_box);
gboolean ui_cur_combobox_set_active(GtkComboBox *entry_box, guint32 key);
void ui_cur_combobox_add(GtkComboBox *entry_box, Currency *cur);
void ui_cur_combobox_populate(GtkComboBox *entry_box, GHashTable *hash);
void ui_cur_combobox_populate_except(GtkComboBox *entry_box, GHashTable *hash, guint except_key);
GtkWidget *ui_cur_combobox_new(GtkWidget *label);

/* = = = = = = = = = = */

void ui_cur_listview_add(GtkTreeView *treeview, Currency *item);
guint32 ui_cur_listview_get_selected_key(GtkTreeView *treeview);
void ui_cur_listview_remove_selected(GtkTreeView *treeview);
void ui_cur_listview_populate(GtkWidget *view);
GtkWidget *ui_cur_listview_new(gboolean withtoggle);

gint ui_cur_manage_dialog_update_currencies(GtkWindow *parent, GString *node);

GtkWidget *ui_cur_manage_dialog (void);

gint ui_cur_select_dialog_new(GtkWindow *parent, gint select_mode, struct curSelectContext *ctx);

void ui_cur_edit_dialog_new(GtkWindow *parent, Currency *cur);

#endif

