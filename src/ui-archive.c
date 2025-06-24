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


#include "homebank.h"

#include "ui-archive.h"
#include "ui-account.h"
#include "ui-category.h"
#include "ui-payee.h"
#include "ui-txn-split.h"
#include "ui-tag.h"

#include "ui-dialogs.h"
#include "ui-widgets.h"

#include "list-scheduled.h"
#include "gtk-dateentry.h"
#include "hbtk-switcher.h"

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

extern char *CYA_ARC_FREQ[];
extern HbKvData CYA_ARC_ORDINAL[];
extern HbKvData CYA_ARC_WEEKDAY[];
extern HbKvData CYA_ARC_WEEKEND[];


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void ui_arc_listview_select_by_pointer(GtkTreeView *treeview, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter	iter;
GtkTreeSelection *selection;
gboolean valid;
Archive *arc = user_data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Archive *tmp_arc;

		gtk_tree_model_get (model, &iter, LST_DSPUPC_DATAS, &tmp_arc, -1);
		if( arc == tmp_arc )
		{
			gtk_tree_selection_select_iter (selection, &iter);
			break;
		}

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static void
ui_arc_manage_update(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
Archive *arc;
gboolean selected, recurring, sensitive;

	DB( g_print("\n[ui-scheduled] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);

	DB( g_print(" toolbutton sensitive\n") );

	sensitive = selected;
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_rem, sensitive);
	gtk_widget_set_sensitive(data->BT_dup, sensitive);
	gtk_widget_set_sensitive(data->BT_schedule, sensitive);
	gtk_widget_set_sensitive(data->IM_wrnwe, sensitive);

	gtk_widget_set_sensitive(data->SW_recurrent, sensitive);

	DB( g_print(" scheduled popover sensitive\n") );

	recurring = FALSE;
	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arc, -1);

		if( arc->rec_flags & TF_RECUR )
			recurring = TRUE;
	}

	gtk_widget_set_sensitive(data->GR_recurrent, recurring);
	gtk_widget_set_sensitive(data->SW_recurrent, TRUE);
	gtk_widget_set_sensitive(data->LB_next, recurring);
	gtk_widget_set_sensitive(data->PO_next, recurring);

	if(recurring == TRUE)
	{
		sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_relative));
		DB( g_print(" relative = %d (recu=%d cm=%d)\n", sensitive, recurring, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_relative))) );
		gtk_widget_set_sensitive(data->LB_relative, sensitive);
		gtk_widget_set_sensitive(data->CY_ordinal, sensitive);
		gtk_widget_set_sensitive(data->CY_weekday, sensitive);

		//gint freq = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_rec_freq));
		gint freq = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_rec_freq));
		sensitive = (freq == AUTO_FREQ_MONTH) ? TRUE : FALSE;
		hb_widget_visible(data->CM_relative, sensitive);
		hb_widget_visible(data->LB_relative, sensitive);
		hb_widget_visible(data->CY_ordinal, sensitive);
		hb_widget_visible(data->CY_weekday, sensitive);

		//update freq label
		//#2108847 not translated
		gchar *txt = _(ui_arc_listview_get_freq_label(freq));
		gtk_label_set_text(GTK_LABEL(data->LB_rec_every2), txt);

		sensitive = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_relative));
		gtk_widget_set_sensitive(data->LB_weekend, sensitive);
		gtk_widget_set_sensitive(data->CY_weekend, sensitive);

		sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_limit));
		gtk_widget_set_sensitive(data->NB_limit, sensitive);
		gtk_widget_set_sensitive(data->LB_posts, sensitive);
	}

	DB( g_print(" row changed\n") );

	if(selected)
	{
	GtkTreePath *path;

		// redraw the row to display/hide the icon
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free (path);

		//	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_arc));
		//gtk_widget_queue_draw (GTK_WIDGET(data->LV_arc));
	}
}


static void
ui_arc_manage_cb_check_weekend(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
gboolean visible;
gint tmpwe;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//update weekend info
	tmpwe = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_weekend));
	visible = FALSE;
	if( tmpwe==ARC_WEEKEND_BEFORE || tmpwe==ARC_WEEKEND_AFTER )
	{
	guint32 tmpdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_next));

		if( tmpdate != scheduled_get_txn_real_postdate(tmpdate, tmpwe) )
			visible = TRUE;
	}
	DB( g_print(" warn weekend %d\n", visible) );
	hb_widget_visible(data->IM_wrnwe, visible);
}


static void
ui_arc_manage_cb_relative_changed(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
gint ordinal, nextwd, every;
GDate date;
guint32 nextdate = GLOBALS->today;

	DB( g_print("\n[ui-scheduled] cb relative changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ordinal = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_ordinal));
	nextwd  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_weekday));
	every = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_rec_every));

	g_date_set_julian(&date, GLOBALS->today);
	nextdate = scheduled_date_get_next_relative(&date, ordinal, nextwd, every);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_next), nextdate);

	ui_arc_manage_update(widget, user_data);
}


static void
ui_arc_manage_cb_schedule_changed(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
Archive *arcitem;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
gboolean selected, sensitive;

	DB( g_print("\n[ui-scheduled] cb schedule changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = FALSE;

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);
	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arcitem, -1);

		arcitem->rec_flags &= ~(TF_RECUR);
		sensitive = gtk_switch_get_active(GTK_SWITCH(data->SW_recurrent)) ? TRUE : FALSE;
		if(sensitive)
			arcitem->rec_flags |= TF_RECUR;
	}

	ui_arc_manage_update(widget, user_data);

}


static void
ui_arc_manage_populate_listview(struct ui_arc_manage_data *data)
{
GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
gchar *needle;
gboolean hastext;
gint i, typsch, typtpl;

	DB( g_print("\n[ui-scheduled] populate listview\n") );

	typsch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_typsch));
	typtpl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_typtpl));

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_arc));
	hastext = (gtk_entry_get_text_length (GTK_ENTRY(data->ST_search)) >= 2) ? TRUE : FALSE;
	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));

	DB( g_print(" typsch=%d / typtpl=%d\n", typsch, typtpl) );

	gtk_list_store_clear (GTK_LIST_STORE(model));

	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_arc), NULL); /* Detach model from view */

	i=0;
	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *item = list->data;
	gboolean insert = FALSE;

		if( (typsch) && (item->rec_flags & TF_RECUR) )
			insert = TRUE;

		if( (typtpl) && !(item->rec_flags & TF_RECUR) )
			insert = TRUE;

		if( insert )
		{
		gboolean qinsert = TRUE;

			if(hastext)
			{
				qinsert = filter_tpl_search_match(needle, item);
			}

			if( qinsert )
			{
				gtk_list_store_insert_with_values (GTK_LIST_STORE(model), &iter, -1,
					LST_DSPUPC_DATAS, item,	//data struct
//					LST_DEFARC_OLDPOS, i,		//oldpos
					-1);
			}
		}
		//DB( g_print(" populate_treeview: %d %08x\n", i, list->data) );

		i++; list = g_list_next(list);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_arc), model); /* Re-attach model to view */
	g_object_unref(model);

//	gtk_tree_view_expand_all (GTK_TREE_VIEW(data->LV_arc));
}


static void
ui_arc_manage_cb_add_clicked(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel *model;
GtkTreeIter  iter;
Archive *item;
gint typsch, typtpl;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-scheduled] cb add\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_arc));



GtkWidget *dialog;
Transaction *new_txn = da_transaction_malloc();
gboolean result;

	dialog = create_deftransaction_window(GTK_WINDOW(data->dialog), TXN_DLG_ACTION_ADD, TXN_DLG_TYPE_TPL, 0);
	deftransaction_set_transaction(dialog, new_txn);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	if(result == HB_RESPONSE_ADD)
	{
		deftransaction_get(dialog, NULL);

		item = da_archive_malloc();
		//item->memo = g_strdup_printf(_("(template %d)"), g_list_length(GLOBALS->arc_list) + 1);

		da_archive_init_from_transaction(item, new_txn, FALSE);

		typsch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_typsch));
		typtpl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_typtpl));

		if( typsch && !typtpl )
			item->rec_flags |= TF_RECUR;

		item->rec_every = 1;
		item->rec_freq = AUTO_FREQ_MONTH;
		item->nextdate = GLOBALS->today;

		//GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
		da_archive_append_new(item);

		DB( g_print(" kacc: '%d'\n", item->kacc) );


		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DSPUPC_DATAS, item,
//			LST_DEFARC_OLDPOS, 0,
			-1);

		gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &iter);

		data->change++;
	}

	deftransaction_dispose(dialog, NULL);
	gtk_window_destroy (GTK_WINDOW(dialog));

	da_transaction_free(new_txn);

}


static void
ui_arc_manage_cb_edit_clicked(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
gboolean selected;
Archive *arcitem;
GtkWidget *dialog;
Transaction *new_txn = da_transaction_malloc();
gboolean result;

	DB( g_print("\n[ui-scheduled] cb edit\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);
	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arcitem, -1);

		dialog = create_deftransaction_window(GTK_WINDOW(data->dialog), TXN_DLG_ACTION_EDIT, TXN_DLG_TYPE_TPL, 0);

		da_transaction_init_from_template(new_txn, arcitem);
		deftransaction_set_transaction(dialog, new_txn);

		result = gtk_dialog_run (GTK_DIALOG (dialog));
		if(result == GTK_RESPONSE_ACCEPT)
		{
			deftransaction_get(dialog, NULL);

			da_archive_init_from_transaction(arcitem, new_txn, FALSE);

			//this redraw the row
			gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &iter);

			data->change++;
		}

		deftransaction_dispose(dialog, NULL);
		gtk_window_destroy (GTK_WINDOW(dialog));

		da_transaction_free(new_txn);
	}
}


static void
ui_arc_manage_cb_dup_clicked(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
Archive *arcitem, *newitem;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-scheduled] dup (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arcitem, -1);

		newitem = da_archive_clone(arcitem);
		if( newitem )
		{
			if( da_archive_append(newitem) )
			{
				gtk_list_store_append (GTK_LIST_STORE(model), &iter);
				gtk_list_store_set (GTK_LIST_STORE(model), &iter,
					LST_DSPUPC_DATAS, newitem,
		//			LST_DEFARC_OLDPOS, 0,
					-1);

				gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &iter);
			}

		}
	}
}


static void
ui_arc_manage_cb_delete_clicked(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
Archive *item;
gint result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-scheduled] cb delete (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	gchar *title;
	gchar *secondtext;

		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &item, -1);

		//5.7.4 check if template is used
		if( !(item->rec_flags & TF_RECUR) )
		{
			if( template_is_account_used(item) == TRUE )
			{
				ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_INFO,
					_("Template delete"),
					_("This template is used as an account template and cannot be deleted.")
				);
				return;
			}
		}

		//#1940103 as memo can be null, use (no memo) instead
		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), item->memo != NULL ? item->memo : _("(no memo)") );

		secondtext = _("If you delete a scheduled/template, it will be permanently lost.");

		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->dialog),
				title,
				secondtext,
				_("_Delete"),
				TRUE
			);

		g_free(title);

		if( result == GTK_RESPONSE_OK )
		{
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

			GLOBALS->arc_list = g_list_remove(GLOBALS->arc_list, item);

			data->change++;

		}
		//DB( g_print(" delete =%08x (pos=%d)\n", entry, g_list_index(data->tmp_list, entry) ) );
	}
}


static void
ui_arc_manage_set(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
Archive *item;

	DB( g_print("\n[ui-scheduled] set popover\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &item, -1);

		g_signal_handlers_block_by_func (G_OBJECT (data->PO_next), G_CALLBACK (ui_arc_manage_cb_check_weekend), NULL);
		g_signal_handlers_block_by_func (G_OBJECT (data->CY_weekend), G_CALLBACK (ui_arc_manage_cb_check_weekend), NULL);
		g_signal_handlers_block_by_func (G_OBJECT (data->SW_recurrent ), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
		g_signal_handlers_block_by_func (G_OBJECT (data->CM_limit), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);

		gtk_switch_set_active(GTK_SWITCH(data->SW_recurrent), (item->rec_flags & TF_RECUR) ? 1 : 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_rec_every), item->rec_every);
		//hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_rec_freq), item->rec_freq);
		hbtk_switcher_set_active(HBTK_SWITCHER(data->RA_rec_freq), item->rec_freq);
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_next), item->nextdate);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_limit), (item->rec_flags & TF_LIMIT) ? 1 : 0);
		DB( g_print(" nb_limit = %d %g\n", item->limit, (gdouble)item->limit) );
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_limit), (gdouble)item->limit);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_weekend), item->weekend);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_relative), (item->rec_flags & TF_RELATIVE) ? 1 : 0);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_ordinal), item->rec_ordinal);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_weekday), item->rec_weekday);

		g_signal_handlers_unblock_by_func (G_OBJECT (data->CM_limit), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
		g_signal_handlers_unblock_by_func (G_OBJECT (data->SW_recurrent ), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
		g_signal_handlers_unblock_by_func (G_OBJECT (data->CY_weekend ), G_CALLBACK (ui_arc_manage_cb_check_weekend), NULL);
		g_signal_handlers_unblock_by_func (G_OBJECT (data->PO_next ), G_CALLBACK (ui_arc_manage_cb_check_weekend), NULL);
	}
}


static void
ui_arc_manage_getlast(struct ui_arc_manage_data *data)
{
GtkTreeModel *model;
GtkTreeIter iter;
Archive *item;
gboolean active;

	DB( g_print("\n[ui-scheduled] getlast\n") );

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &item, -1);

		//#1863484: reset flag to enable remove auto and limit :)
		item->rec_flags &= ~(TF_RECUR|TF_LIMIT|TF_RELATIVE);

		active = gtk_switch_get_active(GTK_SWITCH(data->SW_recurrent));
		if(active == 1) item->rec_flags |= TF_RECUR;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->NB_rec_every));
		item->rec_every   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_rec_every));
		//item->rec_freq    = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_rec_freq));
		item->rec_freq    = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_rec_freq));
		item->nextdate	= gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_next));

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_limit));
		if(active == 1) item->rec_flags |= TF_LIMIT;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->NB_limit));
		item->limit   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_limit));

		item->weekend = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_weekend));

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_relative));
		if(active == 1) item->rec_flags |= TF_RELATIVE;

		item->rec_ordinal = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_ordinal));
		item->rec_weekday = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_weekday));

		//#1906953 add skip weekend
		scheduled_nextdate_weekend_adjust(item);

		data->change++;
	}
}


static void
ui_arc_manage_cb_popover_closed(GtkWidget *popover, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
GtkTreePath			*path;
gboolean selected;

	DB( g_print("\n[ui-scheduled] cb popover closed\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(popover, GTK_TYPE_WINDOW)), "inst_data");
	data = user_data;

	/* redraw the row to display/hide the icon */
	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);
	if(selected)
	{
		ui_arc_manage_getlast(data);

		path = gtk_tree_model_get_path(model, &iter);
		#if MYDEBUG == 1
			gchar *spath = gtk_tree_path_to_string(path);
			g_print(" selected '%s'\n", spath);
			g_free(spath);
		#endif
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free (path);
	}
}



static gboolean ui_arc_manage_cb_on_key_press(GtkWidget *source, GdkEvent *event, gpointer user_data)
{
struct ui_arc_manage_data *data = user_data;
GdkModifierType state;
guint keyval;

	gdk_event_get_state (event, &state);
	gdk_event_get_keyval(event, &keyval);

	// On Control-f enable search entry
	if (state & GDK_CONTROL_MASK && keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->ST_search);
	}
	else
	if (keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
	{
		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), NULL);
		gtk_widget_grab_focus(data->LV_arc);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}


static void
ui_arc_manage_cb_selection_changed(GtkTreeSelection *treeselection, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkWidget *treeview;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
gboolean selected;
Archive *arcitem;

	DB( g_print("\n[ui-scheduled] selection\n") );

	treeview = (GtkWidget *)gtk_tree_selection_get_tree_view (treeselection);

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(treeview, GTK_TYPE_WINDOW)), "inst_data");

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);

	DB( g_print(" a row is selected = %d\n", selected) );

	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arcitem, -1);

		ui_arc_manage_set(treeview, NULL);
	}

	ui_arc_manage_cb_check_weekend(GTK_WIDGET(treeview), NULL);
	ui_arc_manage_update(GTK_WIDGET(treeview), NULL);
}


static void
ui_arc_manage_cb_row_activated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
	ui_arc_manage_cb_edit_clicked(GTK_WIDGET(treeview), userdata);
}


static void
ui_arc_manage_cb_flttype_changed (GtkToggleButton *button, gpointer user_data)
{
	ui_arc_manage_populate_listview(user_data);
	//g_print(" toggle type=%d\n", gtk_toggle_button_get_active(button));
}


static gboolean
ui_arc_manage_cleanup(struct ui_arc_manage_data *data, gint result)
{
gboolean doupdate = FALSE;

	DB( g_print("\n[ui-scheduled] cleanup\n") );

	da_archive_glist_sorted(HB_GLIST_SORT_NAME);

	GLOBALS->changes_count += data->change;

	return doupdate;
}


static void
ui_arc_manage_setup(struct ui_arc_manage_data *data)
{

	DB( g_print("\n[ui-scheduled] setup\n") );

	DB( g_print(" init data\n") );

	//init GList
	data->tmp_list = NULL; //hb-glist_clone_list(GLOBALS->arc_list, sizeof(struct _Archive));
	data->change = 0;

	DB( g_print(" set widgets default\n") );

	//hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_rec_freq), AUTO_FREQ_MONTH);
	hbtk_switcher_set_active(HBTK_SWITCHER(data->RA_rec_freq), AUTO_FREQ_MONTH);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->BT_typsch), TRUE);


	DB( g_print(" populate\n") );
	ui_arc_manage_populate_listview(data);


	DB( g_print(" connect widgets signals\n") );

	g_signal_connect (data->BT_typsch, "toggled", G_CALLBACK (ui_arc_manage_cb_flttype_changed), data);
	g_signal_connect (data->BT_typtpl, "toggled", G_CALLBACK (ui_arc_manage_cb_flttype_changed), data);

	//#1999297 seems to crash because of internal gtk search
	//gtk_tree_view_set_search_entry(GTK_TREE_VIEW(data->LV_arc), GTK_ENTRY(data->ST_search));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(data->LV_arc), FALSE);

	g_signal_connect (data->ST_search, "search-changed", G_CALLBACK (ui_arc_manage_cb_flttype_changed), data);

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), "changed", G_CALLBACK (ui_arc_manage_cb_selection_changed), NULL);
	g_signal_connect (data->LV_arc, "row-activated", G_CALLBACK (ui_arc_manage_cb_row_activated), NULL);

	g_signal_connect (data->BT_add , "clicked", G_CALLBACK (ui_arc_manage_cb_add_clicked), NULL);
	g_signal_connect (data->BT_rem , "clicked", G_CALLBACK (ui_arc_manage_cb_delete_clicked), NULL);

	g_signal_connect (data->BT_edit, "clicked", G_CALLBACK (ui_arc_manage_cb_edit_clicked), NULL);
	g_signal_connect (data->BT_dup , "clicked", G_CALLBACK (ui_arc_manage_cb_dup_clicked), NULL);

	//popover
	g_signal_connect (data->PO_recurrent, "closed", G_CALLBACK (ui_arc_manage_cb_popover_closed), data);

	g_signal_connect (data->SW_recurrent,  "notify::active", G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
	g_signal_connect (data->PO_next, "changed", G_CALLBACK (ui_arc_manage_cb_check_weekend), NULL);

	g_signal_connect (data->RA_rec_freq, "changed", G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);

	g_signal_connect (data->CM_relative, "toggled", G_CALLBACK (ui_arc_manage_cb_relative_changed), NULL);
	g_signal_connect (data->CY_ordinal, "changed", G_CALLBACK (ui_arc_manage_cb_relative_changed), NULL);
	g_signal_connect (data->CY_weekday, "changed", G_CALLBACK (ui_arc_manage_cb_relative_changed), NULL);

	g_signal_connect (data->CY_weekend, "changed", G_CALLBACK (ui_arc_manage_cb_check_weekend), NULL);
	g_signal_connect (data->CM_limit, "toggled", G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);

	if(data->ext_arc != NULL)
		ui_arc_listview_select_by_pointer(GTK_TREE_VIEW(data->LV_arc), data->ext_arc);

}


static gboolean
ui_arc_manage_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct ui_arc_manage_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[ui-scheduled] mapped\n") );

	ui_arc_manage_setup(data);
	ui_arc_manage_update(data->LV_arc, NULL);
	//gtk_widget_grab_focus(GTK_WIDGET(data->LV_arc));

	data->mapped_done = TRUE;

	return FALSE;
}


static GtkWidget *
ui_arc_manage_create_scheduling(struct ui_arc_manage_data *data)
{
GtkWidget *content, *group_grid, *hbox, *vbox, *expander, *label, *widget;
gint row;

	content = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (content), hbox);

		//on/off switch
		widget = gtk_switch_new();
		data->SW_recurrent = widget;
		gtk_widget_set_hexpand(widget, TRUE);
		gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = gtk_date_entry_new(NULL);
		data->PO_next = widget;
		gtk_box_append (GTK_BOX (hbox), widget);

		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_WARNING);
		gtk_widget_set_tooltip_text(widget, _("The post date will be shifted outside of the weekend"));
		data->IM_wrnwe = widget;
		gtk_box_append (GTK_BOX (hbox), widget);

		label = gtk_label_new_with_mnemonic (_("Next _date:"));
		data->LB_next = label;
		gtk_box_append (GTK_BOX (hbox), label);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	data->GR_recurrent = vbox;
	gtk_box_prepend (GTK_BOX (content), vbox);

	label = make_label_group(_("Recurrence pattern"));
	gtk_box_prepend (GTK_BOX (vbox), label);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_box_prepend (GTK_BOX (vbox), hbox);

	widget = hbtk_switcher_new(GTK_ORIENTATION_VERTICAL);
	data->RA_rec_freq = widget;
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_ARC_FREQ, FALSE);
	gtk_box_prepend (GTK_BOX (hbox), widget);

	widget = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_box_prepend (GTK_BOX (hbox), widget);

	// group :: Scheduled insertion
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (hbox), group_grid);

	row = 0;
	label = make_label_widget(_("Ever_y:"));
	data->LB_rec_every = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 1, 1);
	widget = make_numeric(label, 1, 100);
	data->NB_rec_every = widget;
    gtk_box_prepend (GTK_BOX (hbox), widget);
	label = gtk_label_new(NULL);
	data->LB_rec_every2 = label;
	//gtk_widget_set_hexpand(label, TRUE);
    gtk_box_prepend (GTK_BOX (hbox), label);

	row++;
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_widget_set_halign(hbox, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 0, row, 1, 1);

		widget = gtk_check_button_new();
		data->CM_relative = widget;
	    gtk_box_prepend (GTK_BOX (hbox), widget);

		label = make_label_widget(_("The"));
		data->LB_relative = label;
	    gtk_box_prepend (GTK_BOX (hbox), label);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_widget_set_halign(hbox, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 1, 1);

		widget = hbtk_combo_box_new_with_data(label, CYA_ARC_ORDINAL);
		data->CY_ordinal = widget;
	    gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = hbtk_combo_box_new_with_data(label, CYA_ARC_WEEKDAY);
	    data->CY_weekday = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);


	row++;
	expander = gtk_expander_new_with_mnemonic(_("More options"));
	data->EX_options = expander;
	gtk_grid_attach (GTK_GRID (group_grid), expander, 0, row, 2, 1);
	//gtk_box_prepend (GTK_BOX (content), expander);

	// group :: Scheduled insertion
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	hb_widget_set_margin(GTK_WIDGET(group_grid), SPACING_SMALL);
	gtk_expander_set_child (GTK_EXPANDER(expander), group_grid);

	row++;
	label = make_label_widget(_("Week end:"));
	data->LB_weekend = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_ARC_WEEKEND);
	gtk_widget_set_hexpand(widget, FALSE);
	data->CY_weekend = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("_Stop after:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 1, 1);

		widget = gtk_check_button_new();
		data->CM_limit = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = make_numeric(label, 1, 366);
		data->NB_limit = widget;
	    hbtk_box_prepend (GTK_BOX (hbox), widget);

		label = gtk_label_new_with_mnemonic (_("posts"));
		data->LB_posts = label;
	    gtk_box_prepend (GTK_BOX (hbox), label);

	gtk_widget_show_all(content);

	return content;
}


GtkWidget *
ui_arc_manage_dialog (Archive *ext_arc)
{
struct ui_arc_manage_data *data;
GtkWidget *dialog, *content_area, *bbox, *hbox, *vbox, *tbar;
GtkWidget *box, *treeview, *scrollwin;
GtkWidget *widget, *content, *menubutton, *image, *label;
gint w, h, dw, dh;

	DB( g_print("\n[ui-scheduled] dialog\n") );

	data = g_malloc0(sizeof(struct ui_arc_manage_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Manage scheduled/template transactions"),
				GTK_WINDOW(GLOBALS->mainwindow),
				0,
		    	_("_Close"),
			    GTK_RESPONSE_ACCEPT,
			    NULL);

	data->dialog = dialog;
	data->ext_arc = ext_arc;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 3:2
	dw = (dh * 3) / 2;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);

	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);

	//dialog content
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));	 	// return a vbox

	content = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	hb_widget_set_margin(GTK_WIDGET(content), SPACING_LARGE);
	hbtk_box_prepend (GTK_BOX (content_area), content);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (content), hbox);

		box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
		hbtk_box_prepend (GTK_BOX (hbox), box);

		widget = gtk_toggle_button_new_with_label(_("Scheduled"));
		data->BT_typsch = widget;
		gtk_box_prepend (GTK_BOX (box), widget);

		widget = gtk_toggle_button_new_with_label(_("Template"));
		data->BT_typtpl = widget;
		gtk_box_prepend (GTK_BOX (box), widget);

	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_box_prepend (GTK_BOX (hbox), widget);

	// list + toolbar
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hbtk_box_prepend (GTK_BOX (content), vbox);

	// listview
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	//#1970509 enable hscrollbar
	//gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	treeview = (GtkWidget *)ui_arc_listview_widget_new();
	data->LV_arc = treeview;
	gtk_widget_set_size_request(treeview, HB_MINWIDTH_LIST, -1);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	hbtk_box_prepend (GTK_BOX (vbox), scrollwin);

	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_prepend (GTK_BOX (vbox), tbar);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

		widget = make_image_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_add = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_rem = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

		//widget = gtk_button_new_with_mnemonic(_("_Edit"));
		widget = make_image_button(ICONNAME_LIST_EDIT, _("Edit"));
		data->BT_edit = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

		widget = make_image_button(ICONNAME_LIST_DUPLICATE, _("Duplicate"));
		data->BT_dup = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

		//schedule button
		menubutton = gtk_menu_button_new ();
		data->BT_schedule = menubutton;
		gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_DOWN );
		gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
		//gtk_widget_set_hexpand (menubutton, TRUE);
		gtk_widget_show_all(menubutton);
		gtk_box_prepend(GTK_BOX(bbox), menubutton);

		box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
		label = gtk_label_new_with_mnemonic (_("_Schedule"));
		gtk_box_prepend (GTK_BOX(box), label);
		image = hbtk_image_new_from_icon_name_16 ("pan-down-symbolic");
		gtk_box_prepend (GTK_BOX(box), image);
		gtk_container_add(GTK_CONTAINER(menubutton), box);
		GtkWidget *template = ui_arc_manage_create_scheduling(data);
		GtkWidget *popover = create_popover (menubutton, template, GTK_POS_TOP);
		data->PO_recurrent = popover;
		gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);


	// connect dialog signals
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_arc_manage_mapped), &dialog);
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_arc_manage_cb_on_key_press), (gpointer)data);

	ui_arc_listview_widget_columns_order_load(GTK_TREE_VIEW(data->LV_arc));

	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all(content);
	gtk_widget_show (dialog);

	// wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	ui_arc_listview_widget_columns_order_save(GTK_TREE_VIEW(data->LV_arc));

	// cleanup and destroy
	ui_arc_manage_cleanup(data, result);
	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);

	return NULL;
}


