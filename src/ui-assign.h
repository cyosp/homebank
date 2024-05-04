/*  HomeBank -- Free, easy, personal rulounting for everyone.
 *  Copyright (C) 1995-2024 Maxime DOYEN
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

#ifndef __HB_ASSIGN_GTK_H__
#define __HB_ASSIGN_GTK_H__


enum {
	LST_DEFASG_SORT_POS = 1,
	LST_DEFASG_SORT_SEARCH,
	LST_DEFASG_SORT_PAYEE,
	LST_DEFASG_SORT_CATEGORY,
	LST_DEFASG_SORT_NOTES
};


enum
{
	LST_DEFASG_TOGGLE,
	LST_DEFASG_DATAS,
	NUM_LST_DEFASG
};


struct ui_asg_dialog_data
{
	//GList	*tmp_list;
	Assign  *asgitem;
	gint	change;

	GtkWidget	*dialog;

	GtkWidget   *GR_condition;
	GtkWidget   *CY_field;
	GtkWidget	*ST_search;
	GtkWidget	*GR_wrntxt, *LB_wrntxt;
	GtkWidget	*CM_exact;
	GtkWidget	*CM_re;
	GtkWidget	*CM_amount, *LB_amount, *ST_amount;

	GtkWidget   *GR_assignment;
	GtkWidget	*RA_pay;
	GtkWidget   *LB_pay;
	GtkWidget   *PO_pay;

	GtkWidget	*RA_cat;
	GtkWidget   *LB_cat;
	GtkWidget	*PO_cat;

	GtkWidget	*RA_mod;
	GtkWidget   *LB_mod;
	GtkWidget	*NU_mod;

	GtkWidget   *GR_misc;
	GtkWidget	*ST_notes;

};


struct ui_asg_manage_data
{
	GList	*tmp_list;
	gint	change;

	GtkWidget	*dialog;
	gboolean	mapped_done;

	GtkWidget	*ST_search;
	GtkWidget	*LV_rul;
	GtkWidget	*BT_add, *BT_rem, *BT_edit, *BT_dup, *BT_up, *BT_down, *BT_move;

	GtkWidget	*MB_moveto, *ST_poppos, *BT_popmove;

};


struct rulPopContext
{
	GtkTreeModel *model;
	guint	except_key;
};


gchar *ui_asg_comboboxentry_get_name(GtkComboBox *entry_box);
guint32 ui_asg_comboboxentry_get_key(GtkComboBox *entry_box);
gboolean ui_asg_comboboxentry_set_active(GtkComboBox *entry_box, guint32 key);
void ui_asg_comboboxentry_add(GtkComboBox *entry_box, Assign *asg);
void ui_asg_comboboxentry_populate(GtkComboBox *entry_box, GHashTable *hash);
void ui_asg_comboboxentry_populate_except(GtkComboBox *entry_box, GHashTable *hash, guint except_key);
GtkWidget *ui_asg_comboboxentry_new(GtkWidget *label);

/* = = = = = = = = = = */

void ui_asg_listview_add(GtkTreeView *treeview, Assign *item);
guint32 ui_asg_listview_get_selected_key(GtkTreeView *treeview);
void ui_asg_listview_remove_selected(GtkTreeView *treeview);
void ui_asg_listview_populate(GtkWidget *view, gchar *needle);
GtkWidget *ui_asg_listview_new(gboolean withtoggle);

/* = = = = = = = = = = */

GtkWidget *ui_asg_manage_dialog (void);


#endif

