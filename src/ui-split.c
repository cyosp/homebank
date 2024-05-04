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
 *  but WITHOUT ANY WARRANTY; without even the implied warranty ofdeftransaction_amountchanged
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "homebank.h"

#include "ui-split.h"
#include "ui-transaction.h"
#include "ui-archive.h"
#include "gtk-dateentry.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-account.h"
#include "hb-split.h"


/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


#define GTK_RESPONSE_SPLIT_REM 10888


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void list_split_number_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GtkTreePath *path;
gint *indices;
gchar num[16];

	path = gtk_tree_model_get_path(model, iter);
	indices = gtk_tree_path_get_indices(path);
	//num = gtk_tree_path_to_string(path);
	g_snprintf(num, 15, "%d", 1 + *indices);
	gtk_tree_path_free(path);

    g_object_set(renderer, "text", num, NULL);

}


static void list_split_amount_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Split *split;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gdouble amount;
gchar *color, *format;

	gtk_tree_model_get(model, iter, 0, &split, -1);

	//hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, ope->kcur, GLOBALS->minor);
	format = g_object_get_data(G_OBJECT(col), "format_data");
	
	amount = split->amount;
	g_snprintf(buf, G_ASCII_DTOSTR_BUF_SIZE-1, format, amount);

	color = get_normal_color_amount(amount);
	g_object_set(renderer,
		"foreground",  color,
		"text", buf,
		NULL);
	
}


static void list_split_memo_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Split *split;

	gtk_tree_model_get(model, iter, 0, &split, -1);
	
    g_object_set(renderer, "text", split->memo, NULL);
}


static void list_split_category_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Split *split;
Category *cat;

	gtk_tree_model_get(model, iter, 0, &split, -1);

	cat = da_cat_get(split->kcat);
	if( cat != NULL )
	{
		g_object_set(renderer, "text", cat->fullname, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);
}


static void list_split_populate(GtkWidget *treeview, GPtrArray *splits)
{
GtkTreeModel *model;
GtkTreeIter	iter;
Split *split;
gint count, i;

	DB( g_print("\n[list_split] populate\n") );

	count = da_splits_length (splits);

	if( count <= 0 )
		return;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	gtk_list_store_clear (GTK_LIST_STORE(model));

	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), NULL); /* Detach model from view */

	/* populate */
	for(i=0 ; i < count ; i++)
	{
		split = da_splits_get(splits, i);

		DB( g_print(" append split %d : %d, %.2f, '%s'\n", i, split->kcat, split->amount, split->memo) );

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			0, split,
			-1);

	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model); /* Re-attach model to view */
	g_object_unref(model);
}


static GtkWidget *
list_split_new(gchar *format)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	DB( g_print("\n[ui_split_listview] new\n") );


	// create list store
	store = gtk_list_store_new(1,
		G_TYPE_POINTER
		);

	// treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);

	//column 0: line number
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new_with_attributes("#", renderer, NULL);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_split_number_cell_data_function, NULL, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	// column 1: category
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, NULL);

	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);

	//gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	//gtk_tree_view_column_set_fixed_width( column, HB_MINWIDTH_LIST);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_split_category_cell_data_function, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	// column 2: memo
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	
	column = gtk_tree_view_column_new_with_attributes(_("Memo"), renderer, NULL);

	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);

	//gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	//gtk_tree_view_column_set_fixed_width( column, HB_MINWIDTH_LIST);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_split_memo_cell_data_function, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	// column 3: amount
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);

	column = gtk_tree_view_column_new_with_attributes(_("Amount"), renderer, NULL);
	g_object_set_data(G_OBJECT(column), "format_data", format);
	
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	gtk_tree_view_column_set_fixed_width( column, HB_MINWIDTH_LIST);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_split_amount_cell_data_function, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	// column empty
	//column = gtk_tree_view_column_new();
	//gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	// treeviewattribute
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW(treeview), TRUE);
	
	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_acc_listview_compare_func, NULL, NULL);
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	return treeview;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


/*static gboolean ui_split_dialog_cb_output_amount(GtkSpinButton *spin, gpointer data)
{
GtkAdjustment *adjustment;
gchar *text;
gdouble value;
//gpointer position;

	DB( g_print("\n[ui_split_dialog] amount output\n") );


	adjustment = gtk_spin_button_get_adjustment (spin);
	value = gtk_adjustment_get_value (adjustment);

	if( value == 0.0 )
	{
		gtk_entry_set_text (GTK_ENTRY (spin), "");
	}
	else
	{
		text = g_strdup_printf ("%+.*f", gtk_spin_button_get_digits(spin), value);
		
		DB( g_print(" output '%s'\n", text) );
		
		gtk_entry_set_text (GTK_ENTRY (spin), text);
		//gtk_editable_delete_text (GTK_EDITABLE(spin), 0, -1);
		//gtk_editable_insert_text (GTK_EDITABLE(spin), text, -1, &position);
		
		g_free (text);
	}
	return TRUE;
}*/


static void ui_split_dialog_filter_text_handler (GtkEntry    *entry,
                          const gchar *text,
                          gint         length,
                          gint        *position,
                          gpointer     data)
{
GtkEditable *editable = GTK_EDITABLE(entry);
gint i, count=0;
gchar *result = g_new0 (gchar, length+1);

  for (i=0; i < length; i++)
  {
    if (text[i]=='|')
      continue;
    result[count++] = text[i];
  }


  if (count > 0) {
    g_signal_handlers_block_by_func (G_OBJECT (editable),
                                     G_CALLBACK (ui_split_dialog_filter_text_handler),
                                     data);
    gtk_editable_insert_text (editable, result, count, position);
    g_signal_handlers_unblock_by_func (G_OBJECT (editable),
                                       G_CALLBACK (ui_split_dialog_filter_text_handler),
                                       data);
  }
  g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");

  g_free (result);
}


static void ui_split_dialog_cb_eval_order(struct ui_split_dialog_data *data)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
guint i;

	DB( g_print("\n[ui_split_dialog] eval order\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_split));
	i=1; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Split *split;

		gtk_tree_model_get (model, &iter, 0, &split, -1);
		split->pos = i;

		DB( g_print(" split pos: %d '%s' %.2f\n", i, split->memo, split->amount) );
		
		i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	da_splits_sort(data->tmp_splits);
}


static gboolean ui_split_dialog_cb_amount_focus_out (GtkEditable *spin_button, GdkEvent  *event, gpointer user_data)
{
struct ui_split_dialog_data *data;
const gchar *txt;

	DB( g_print("\n[ui_split_dialog] cb amount focus out\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(spin_button), GTK_TYPE_WINDOW)), "inst_data");

	txt = gtk_entry_get_text(GTK_ENTRY(spin_button));

	data->isopposite = FALSE;
	if( ((data->txntype == TXN_TYPE_EXPENSE) && (*txt == '+')) || ((data->txntype == TXN_TYPE_INCOME) && (*txt == '-')) )
	{
		data->isopposite = TRUE;
	}

	DB( g_print(" txt='%s'\n amt=%.8f\n opp=%d\n", txt, gtk_spin_button_get_value (GTK_SPIN_BUTTON(spin_button)), data->isopposite) );

	return FALSE;
}


static void ui_split_dialog_update(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;
gboolean tmpval;
guint count;

	DB( g_print("\n[ui_split_dialog] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	count = da_splits_length (data->tmp_splits);

	//btn: edit/rem
	tmpval = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_split)), NULL, NULL);
	gtk_widget_set_sensitive (data->BT_edit, (data->isedited) ? FALSE : tmpval);
	gtk_widget_set_sensitive (data->BT_rem, (data->isedited) ? FALSE : tmpval);

	//btn: remall
	tmpval = (count > 1) ? TRUE : FALSE;
	gtk_widget_set_sensitive (data->BT_remall, (data->isedited) ? FALSE : tmpval);

	//btn: add/apply
	/*amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	tmpval = hb_amount_round(amount, 2) != 0.0 ? TRUE : FALSE;
	gtk_widget_set_sensitive (data->BT_apply, tmpval);

	if( count >= TXN_MAX_SPLIT )
		tmpval = FALSE;
	gtk_widget_set_sensitive (data->BT_add, tmpval);
	*/
	
	//btn: show/hide
	gtk_widget_set_sensitive (data->LV_split, !data->isedited);

	hb_widget_visible (data->BT_add, !data->isedited);
	
	hb_widget_visible (data->IM_edit, data->isedited);
	hb_widget_visible (data->BT_apply, data->isedited);
	hb_widget_visible (data->BT_cancel, data->isedited);
}


static void ui_split_dialog_edit_end(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;

	DB( g_print("\n[ui_split_dialog] edit_end\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), 0);
	ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), 0);
	if( data->mode == SPLIT_MODE_EMPTY )
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), 0.0);
	gtk_entry_set_text(GTK_ENTRY(data->ST_memo), "");

	gtk_widget_grab_focus(data->ST_amount);
	
	data->isedited = FALSE;
}


static void ui_split_dialog_edit_start(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("\n[ui_split_dialog] edit_start\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_split));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_split));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Split *split;
	gchar *txt;

		gtk_tree_model_get(model, &iter, 0, &split, -1);

		//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), split->kcat);
		ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), split->kcat);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), split->amount);
		txt = (split->memo != NULL) ? split->memo : "";
		gtk_entry_set_text(GTK_ENTRY(data->ST_memo), txt);
		
		data->isedited = TRUE;

		ui_split_dialog_update (data->dialog, user_data);
	}
}


static void ui_split_dialog_cancel_cb(GtkWidget *widget, gpointer user_data)
{
//struct ui_split_dialog_data *data;

	DB( g_print("\n[ui_split_dialog] cancel\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	ui_split_dialog_edit_end(widget, user_data);
	ui_split_dialog_update (widget, user_data);
}


static void ui_split_dialog_apply_cb(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("--------\n[ui_split_dialog] apply\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_split));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Split *split;
	gdouble amount;

		gtk_tree_model_get(model, &iter, 0, &split, -1);
		DB( g_print(" update spin\n") );
		gtk_spin_button_update (GTK_SPIN_BUTTON(data->ST_amount));
		amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		if(amount)
		{
			//split->kcat = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_cat));
			split->kcat = ui_cat_entry_popover_get_key_add_new(GTK_BOX(data->PO_cat));
			g_free(split->memo);
			split->memo = g_strdup((gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_memo)));
			//split->amount = amount;
			//#1910819 must round frac digit
			split->amount = hb_amount_round(amount, data->cur->frac_digits);
		}
	}

	ui_split_dialog_edit_end(widget, user_data);
	ui_split_dialog_compute (widget, data);
	ui_split_dialog_update (widget, user_data);
}


static void ui_split_dialog_deleteall_cb(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;

	DB( g_print("\n[ui_split_dialog] deleteall_cb\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	gtk_list_store_clear (GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(data->LV_split))));
	da_split_destroy(data->tmp_splits);
	data->tmp_splits = da_split_new ();
	
	ui_split_dialog_compute (widget, data);
	ui_split_dialog_update (widget, user_data);
}


static void ui_split_dialog_delete_cb(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("\n[ui_split_dialog] delete_cb\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_split));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Split *split;

		gtk_tree_model_get(model, &iter, 0, &split, -1);
		//todo: not implemented yet
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		da_splits_delete(data->tmp_splits, split);
	}
	
	ui_split_dialog_compute (widget, data);
	ui_split_dialog_update (widget, user_data);
}


static void ui_split_dialog_add_cb(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;
GtkTreeModel *model;
GtkTreeIter	iter;
Split *split;
guint count;
gdouble amount;

	DB( g_print("--------\n[ui_split_dialog] add\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	count = da_splits_length (data->tmp_splits);
	DB( g_print(" n_split: %d (of %d)\n", count, TXN_MAX_SPLIT) );
	
	if( count <= TXN_MAX_SPLIT )
	{
		split = da_split_malloc ();
		//5.4.4
		DB( g_print(" update spin\n") );
		gtk_spin_button_update (GTK_SPIN_BUTTON(data->ST_amount));
		amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		if(amount)
		{
			//by default affect txntype sign
			if( data->txntype == TXN_TYPE_EXPENSE && amount > 0 )
			{
				DB( g_print(" force expense\n") );
				amount *= -1;
			}
			//but take opposite into account
			if( data->isopposite == TRUE )
			{
				DB( g_print(" force opposite\n") );
				amount *= -1;
			}
			//split->amount = amount;
			//#1910819 must round frac digit
			split->amount = hb_amount_round(amount, data->cur->frac_digits);

			//split->kcat = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_cat));
			split->kcat = ui_cat_entry_popover_get_key_add_new(GTK_BOX(data->PO_cat));
			split->memo = g_strdup((gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_memo)));

			//#1977686 add into memo autocomplete
			if( da_transaction_insert_memo(split->memo, data->date) )
			{
			GtkEntryCompletion *completion;
			GtkTreeModel *model;
			GtkTreeIter  iter;

				completion = gtk_entry_get_completion (GTK_ENTRY(data->ST_memo));
				model = gtk_entry_completion_get_model (completion);
				gtk_list_store_insert_with_values(GTK_LIST_STORE(model), &iter, -1,
					0, split->memo, 
					-1);
			}

			DB( g_print(" append split : %d, %.2f, %s\n", split->kcat, split->amount, split->memo) );

			da_splits_append (data->tmp_splits, split);

			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_split));
			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				0, split,
				-1);

			ui_split_dialog_compute (widget, data);
		}
		// 0 amount not allowed into splits
		else
		{
			da_split_free(split);
		}
	}
	else
	{
		g_warning("split error: limit of %d reached", TXN_MAX_SPLIT);
	}
	
	ui_split_dialog_edit_end(widget, user_data);
	ui_split_dialog_update (widget, user_data);
}


static void ui_split_dialog_cb_activate_split(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;

	DB( g_print("\n[ui_split_dialog] cb activate split\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	//we trigger the focus-out-event on spinbutton, with grab the add button
	//because we also do things before the legacy spinbutton fucntion
	gtk_widget_grab_focus(data->BT_add);

	if( data->isedited == TRUE )
		ui_split_dialog_apply_cb(widget, NULL);
	else
		ui_split_dialog_add_cb(widget, NULL);
}


static void ui_split_rowactivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
	DB( g_print("\n[ui_split_dialog] rowactivated\n") );

	ui_split_dialog_edit_start(GTK_WIDGET(treeview), NULL);
}


static void ui_split_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	DB( g_print("\n[ui_split_dialog] selection\n") );

	ui_split_dialog_update (GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), user_data);
}


void ui_split_dialog_compute(GtkWidget *widget, gpointer user_data)
{
struct ui_split_dialog_data *data;
gint i, count, nbvalid;
gchar buf[48];
gboolean sensitive;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;

	DB( g_print("\n[ui_split_dialog] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	nbvalid = 0;
	data->sumsplit = 0.0;
	data->remsplit = 0.0;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_split));
	i=0; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Split *split;

		gtk_tree_model_get (model, &iter,
			0, &split,
			-1);

		data->sumsplit += split->amount;
		if( hb_amount_round(split->amount, data->cur->frac_digits) != 0.0 )
			 nbvalid++;

		/* Make iter point to the next row in the list store */
		i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
	count = i;

	DB( g_print(" n_count=%d, n_valid=%d\n", count, nbvalid ) );
	
	data->remsplit = data->amount - data->sumsplit;

	//validation: 2 split min, if 0 split
	//Accept button is not disabled to enable empty the splits
	sensitive = FALSE;
	if( (count == 0) || nbvalid >= 2 )
		sensitive = TRUE;

	gtk_widget_hide(data->IB_wrnsum);
	gtk_widget_hide(data->IB_errtype);

	if( data->mode == SPLIT_MODE_AMOUNT )
	{
		if( hb_amount_round(data->remsplit, data->cur->frac_digits) == 0.0 )
		{
			g_sprintf(buf, "----");
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), 0);
		}
		else
		{
			g_snprintf(buf, 48, data->cur->format, data->remsplit);
			//#1845841 bring back checkpoint with initial amount + init remainder
			//revert, because block any edition/inherit
			//sensitive = (count > 1) ? FALSE : sensitive;
			//but keep prefill remainder
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), data->remsplit);

			gtk_widget_show_all(data->IB_wrnsum);
			//#GTK+710888: hack waiting a GTK fix 
			gtk_widget_queue_resize (data->IB_wrnsum);
		}

		gtk_label_set_label(GTK_LABEL(data->LB_remain), buf);

		g_snprintf(buf, 48, data->cur->format, data->amount);
		gtk_label_set_label(GTK_LABEL(data->LB_txnamount), buf);
	}

	g_snprintf(buf, 48, data->cur->format, data->sumsplit);
	gtk_label_set_text(GTK_LABEL(data->LB_sumsplit), buf);

	//if split sum sign do not match
	if( hb_amount_type_match(data->sumsplit, data->txntype) == FALSE )
	{
		//#1885413 enable sign invert from split dialog
		//sensitive = FALSE;
		gtk_widget_show_all(data->IB_errtype);
		//#GTK+710888: hack waiting a GTK fix 
		gtk_widget_queue_resize (data->IB_errtype);
	}

	gtk_dialog_set_response_sensitive(GTK_DIALOG(data->dialog), GTK_RESPONSE_ACCEPT, sensitive);
}


static void ui_split_dialog_setup(struct ui_split_dialog_data *data)
{
guint count;

	DB( g_print("\n[ui_split_dialog] set\n") );

	count = da_splits_length(data->tmp_splits);
	data->nbsplit = count > 1 ? count-1 : 0;

	DB( g_print(" n_count = %d\n", count) );
	list_split_populate (data->LV_split, data->tmp_splits);

	data->isedited = FALSE;


	gtk_spin_button_set_digits (GTK_SPIN_BUTTON(data->ST_amount), data->cur->frac_digits);

	//5.5 done in popover
	//ui_cat_comboboxentry_populate(GTK_COMBO_BOX(data->PO_cat), GLOBALS->h_cat);

	ui_split_dialog_compute(data->dialog, data);
	ui_split_dialog_update (data->dialog, data);
}


GtkWidget *ui_split_view_dialog (GtkWidget *parent, Transaction *ope)
{
GtkWidget *dialog, *content, *table, *scrollwin, *treeview;
gint w, h, dw, dh, row;
Currency *cur;

	DB( g_print("\n[ui_split_dialog] new view only\n") );

	if( ope->splits == NULL )
		return NULL;

	cur = da_cur_get(ope->kcur);

	dialog = gtk_dialog_new_with_buttons (_("Transaction splits"),
					    GTK_WINDOW(parent),
					    0,
					    _("_Close"),
					    GTK_RESPONSE_ACCEPT,
					    NULL);

	//store our dialog private data
	DB( g_print(" window=%p\n", dialog) );

    g_signal_connect (dialog, "destroy",
			G_CALLBACK (gtk_widget_destroyed), &dialog);

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(parent), &w, &h);
	dh = (h/PHI);
	//ratio 3:2
	dw = (dh * 3) / 2;
	DB( g_print(" parent w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);

	
	//dialog contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	table = gtk_grid_new ();
	gtk_container_set_border_width (GTK_CONTAINER (table), SPACING_SMALL);
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_TINY);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_TINY);
	gtk_box_pack_start (GTK_BOX (content), table, TRUE, TRUE, 0);

	row = 0;

	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(scrollwin, HB_MINWIDTH_LIST, HB_MINHEIGHT_LIST);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	treeview = list_split_new(cur->format);
	gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
	gtk_grid_attach (GTK_GRID (table), scrollwin, 0, row, 4, 1);

	//setup
	list_split_populate(treeview, ope->splits);
	
	gtk_widget_show_all (dialog);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
	
	return NULL;
	
}


GtkWidget *ui_split_dialog (GtkWidget *parent, GPtrArray **src_splits, gint txntype, guint32 date, gdouble amount, guint32 kcur, void (update_callbackFunction(GtkWidget*, gdouble)))
{
struct ui_split_dialog_data *data;
GtkWidget *dialog, *content, *table, *box, *scrollwin, *bar;
GtkWidget *label, *widget;
gint row;

	DB( g_print("\n[ui_split_dialog] new\n") );

	data = g_malloc0(sizeof(struct ui_split_dialog_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Transaction splits"),
					    GTK_WINDOW(parent),
					    0,
					    _("_Cancel"),
					    GTK_RESPONSE_CANCEL,
					    NULL);

	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" window=%p, inst_data=%p\n", dialog, data) );

    g_signal_connect (dialog, "destroy",
			G_CALLBACK (gtk_widget_destroyed), &dialog);

	data->dialog = dialog;

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("_OK"), GTK_RESPONSE_ACCEPT);

	data->date = date;
	data->cur  = da_cur_get (kcur);

	DB( g_print(" kcur: %d %d %s\n", data->cur->key, data->cur->frac_digits, data->cur->format) );


	//todo: init should move 
	//clone splits or create new
	data->src_splits = *src_splits;
	data->txntype = txntype;
	data->mode = (hb_amount_round(amount, data->cur->frac_digits) != 0.0) ? SPLIT_MODE_AMOUNT : SPLIT_MODE_EMPTY;
	data->amount = amount;
	data->sumsplit = amount;

	DB( g_print(" amount : %f\n", data->amount) );
	DB( g_print(" txntype: %s\n", data->txntype == TXN_TYPE_EXPENSE ? "expense" : "income" ));
	DB( g_print(" mode   : %s\n", data->mode == SPLIT_MODE_AMOUNT ? "amount" : "empty" ));

	if( *src_splits != NULL )
		data->tmp_splits = da_splits_clone(*src_splits); 
	else
		data->tmp_splits = da_split_new();

	//dialog contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	table = gtk_grid_new ();
	gtk_container_set_border_width (GTK_CONTAINER (table), SPACING_LARGE);
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_TINY);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_TINY);
	gtk_box_pack_start (GTK_BOX (content), table, TRUE, TRUE, 0);

	row = 0;

	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(scrollwin, HB_MINWIDTH_LIST, HB_MINHEIGHT_LIST);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	data->LV_split = list_split_new(data->cur->format);
	gtk_container_add(GTK_CONTAINER(scrollwin), data->LV_split);
	gtk_grid_attach (GTK_GRID (table), scrollwin, 0, row, 4, 1);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_TINY);
	gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (table), box, 4, row, 1, 1);

		widget = make_image_button(ICONNAME_LIST_DELETE_ALL, _("Delete all"));
		data->BT_remall = widget;
		gtk_box_pack_end (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_rem = widget;
		gtk_box_pack_end (GTK_BOX(box), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_OPE_EDIT, _("Edit"));
		data->BT_edit = widget;
		gtk_box_pack_end (GTK_BOX(box), widget, FALSE, FALSE, 0);

	row++;
	label = gtk_label_new(_("Category"));
	gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

	label = gtk_label_new(_("Memo"));
	gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);

	label = gtk_label_new(_("Amount"));
	gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
	gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);


	row++;
	//widget = ui_cat_comboboxentry_new(NULL);
	widget = ui_cat_entry_popover_new(NULL);
	data->PO_cat = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 1, 1);

	//1977686
	//widget = make_string(NULL);
	widget = make_memo_entry(NULL);
	data->ST_memo= widget;
	gtk_grid_attach (GTK_GRID (table), widget, 1, row, 1, 1);

	widget = make_amount(NULL);
	data->ST_amount = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_TINY);
	gtk_grid_attach (GTK_GRID (table), box, 3, row, 1, 1);

		widget = gtk_image_new_from_icon_name (ICONNAME_HB_OPE_EDIT, GTK_ICON_SIZE_BUTTON);
		data->IM_edit = widget;
		gtk_box_pack_start (GTK_BOX(box), widget, TRUE, TRUE, 0);

		widget = make_image_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_add = widget;
		gtk_box_pack_start (GTK_BOX(box), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_EMBLEM_OK, _("Apply"));
		data->BT_apply = widget;
		gtk_box_pack_start (GTK_BOX(box), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_WINDOW_CLOSE, _("Cancel"));
		data->BT_cancel = widget;
		gtk_box_pack_start (GTK_BOX(box), widget, FALSE, FALSE, 0);


	if( data->mode == SPLIT_MODE_AMOUNT )
	{
		row++;
		label = gtk_label_new(_("Transaction amount:"));
		gtk_widget_set_halign (label, GTK_ALIGN_END);
		gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
		widget = gtk_label_new(NULL);
		gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
		data->LB_txnamount = widget;
		gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

		row++;
		label = gtk_label_new(_("Unassigned:"));
		gtk_widget_set_halign (label, GTK_ALIGN_END);
		gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
		widget = gtk_label_new(NULL);
		gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
		data->LB_remain = widget;
		gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

		row++;
		widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach (GTK_GRID (table), widget, 1, row, 2, 1);

	}

	row++;
	label = gtk_label_new(_("Sum of splits:"));
	gtk_widget_set_halign (label, GTK_ALIGN_END);
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = gtk_label_new(NULL);
	gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
	data->LB_sumsplit = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	bar = gtk_info_bar_new ();
	data->IB_wrnsum = bar;
	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_ERROR);
	label = gtk_label_new (_("Warning: sum of splits and transaction amount don't match"));
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, TRUE, TRUE, 0);
	gtk_grid_attach (GTK_GRID (table), bar, 0, row, 4, 1);

	row++;
	bar = gtk_info_bar_new ();
	data->IB_errtype = bar;
	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	label = gtk_label_new (_("Warning: sum of splits and transaction type don't match"));
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, TRUE, TRUE, 0);
	gtk_grid_attach (GTK_GRID (table), bar, 0, row, 4, 1);
	
	//connect all our signals
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_split)), "changed", G_CALLBACK (ui_split_selection), data);
	g_signal_connect (GTK_TREE_VIEW(data->LV_split), "row-activated", G_CALLBACK (ui_split_rowactivated), data);

	g_signal_connect (data->BT_edit  , "clicked", G_CALLBACK (ui_split_dialog_edit_start), NULL);
	g_signal_connect (data->BT_rem   , "clicked", G_CALLBACK (ui_split_dialog_delete_cb), NULL);
	g_signal_connect (data->BT_remall, "clicked", G_CALLBACK (ui_split_dialog_deleteall_cb), NULL);

	g_signal_connect (data->ST_memo  , "insert-text", G_CALLBACK(ui_split_dialog_filter_text_handler), data);
	g_signal_connect (data->ST_amount, "focus-out-event", G_CALLBACK (ui_split_dialog_cb_amount_focus_out), data);
	g_signal_connect (data->ST_amount, "activate", G_CALLBACK (ui_split_dialog_cb_activate_split), NULL);

	g_signal_connect (data->BT_add   , "clicked", G_CALLBACK (ui_split_dialog_add_cb), NULL);
	g_signal_connect (data->BT_apply , "clicked", G_CALLBACK (ui_split_dialog_apply_cb), NULL);
	g_signal_connect (data->BT_cancel, "clicked", G_CALLBACK (ui_split_dialog_cancel_cb), NULL);

	//gtk_window_set_default_size(GTK_WINDOW(dialog), 480, -1);
	gtk_widget_show_all (dialog);

	//setup, init and show dialog
	ui_split_dialog_setup(data);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result)
    {
	// sum split and alter txn amount   	
	case GTK_RESPONSE_ACCEPT:
		if( da_splits_length(data->tmp_splits) )
		{
			ui_split_dialog_cb_eval_order(data);
			// here we swap src_splits <> tmp_splits
			*src_splits = data->tmp_splits;
			data->tmp_splits = data->src_splits;
			update_callbackFunction(parent, data->sumsplit);
		}
		else
		{
			//delete split and revert back original amount
			da_split_destroy(*src_splits);
			*src_splits = NULL;
			update_callbackFunction(parent, data->amount);
		}
		break;
	/*case GTK_RESPONSE_SPLIT_REM:
		da_split_destroy(*src_splits);
		*src_splits = NULL;
		update_callbackFunction(parent, data->sumsplit);
		break;
		*/
	default:
	   //do_nothing_since_dialog_was_cancelled ();
	   break;
    }

	// debug
	/*#if MYDEBUG == 1
	{
	guint i;

		for(i=0;i<TXN_MAX_SPLIT;i++)
		{
		Split *split = data->ope_splits[i];
			if(data->ope_splits[i] == NULL)
				break;
			g_print(" split %d : %d, %.2f, %s\n", i, split->kcat, split->amount, split->memo);
		}
	}
	#endif*/

	// cleanup and destroy
	//GLOBALS->changes_count += data->change;
	gtk_widget_destroy (dialog);
	
	da_split_destroy (data->tmp_splits);

	g_free(data);

	return NULL;
}

