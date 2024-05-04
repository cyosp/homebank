/*  HomeBank -- Free, easy, personal accounting for everyone.
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

#ifndef __LIST_OPERATION__H__
#define __LIST_OPERATION__H__


enum {
	LIST_TXN_TYPE_BOOK = 0,
	LIST_TXN_TYPE_DETAIL,
	LIST_TXN_TYPE_OTHER,
	LIST_TXN_TYPE_XFERSOURCE,
	LIST_TXN_TYPE_XFERTARGET
};


enum {
	MODEL_TXN_POINTER = 0,
	MODEL_TXN_SPLITAMT,
	MODEL_TXN_SPLITPTR
};


enum {
	LST_TXN_EXP_ACC = 1 << 0,	//detail/print
	LST_TXN_EXP_PMT = 1 << 1,	//!print
	LST_TXN_EXP_CLR = 1 << 2,	
	LST_TXN_EXP_CAT = 1 << 3,
	LST_TXN_EXP_TAG = 1 << 4,
	LST_TXN_EXP_BAL = 1 << 5
};


struct list_txn_data
{
	GtkWidget			*treeview;
	GtkTreeViewColumn   *tvc_balance;
	
	gint				list_type;
	gboolean			showall;
	gboolean			lockreconciled;
	gboolean			warnnocategory;
	gboolean			tvc_is_visible;
	gboolean			save_column_width;
};


GtkWidget *create_list_transaction(gint type, gboolean *pref_columns);
GtkWidget *create_list_import_transaction(gboolean enable_choose);

void list_txn_set_warn_nocategory(GtkTreeView *treeview, gboolean warn);

gboolean list_txn_column_id_isvisible(GtkTreeView *treeview, gint sort_id);
void list_txn_set_column_acc_visible(GtkTreeView *treeview, gboolean visible);

Transaction *list_txn_get_surround_transaction(GtkTreeView *treeview, Transaction **prev, Transaction **next);
Transaction *list_txn_get_active_transaction(GtkTreeView *treeview);

GString *list_txn_to_string(GtkTreeView *treeview, gboolean isclipboard, gboolean hassplit, guint flags);

void list_txn_set_lockreconciled(GtkTreeView *treeview, gboolean lockreconciled);
void list_txn_set_save_column_width(GtkTreeView *treeview, gboolean save_column_width);
void list_txn_sort_force(GtkTreeSortable *sortable, gpointer user_data);
guint list_txn_get_quicksearch_column_mask(GtkTreeView *treeview);

#endif
