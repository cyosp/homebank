/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2022 Maxime DOYEN
 *
 *	This file is part of HomeBank.
 *
 *	HomeBank is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	HomeBank is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */


#include "homebank.h"

#include "dsp-account.h"
#include "dsp-mainwindow.h"

#include "list-operation.h"
#include "hub-account.h"

#include "ui-widgets.h"
#include "ui-filter.h"
#include "ui-transaction.h"
#include "ui-txn-multi.h"

/****************************************************************************/
/* Debug macros											                    */
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

extern gchar *CYA_FLT_TYPE[];
extern gchar *CYA_FLT_STATUS[];


static void register_panel_export_csv(GtkWidget *widget, gpointer user_data);
static void register_panel_edit_multiple(GtkWidget *widget, Transaction *txn, gint column_id, gpointer user_data);
static void register_panel_make_assignment(GtkWidget *widget, gpointer user_data);
static void register_panel_make_archive(GtkWidget *widget, gpointer user_data);
static void register_panel_collect_filtered_txn(GtkWidget *view, gboolean emptysearch);
static void register_panel_listview_populate(GtkWidget *view);
static void register_panel_add_after_propagate(struct register_panel_data *data, Transaction *add_txn);
static void register_panel_add_single_transaction(GtkWindow *window, Transaction *txn);
static void register_panel_action(GtkWidget *widget, gpointer user_data);
static void register_panel_selection(GtkTreeSelection *treeselection, gpointer user_data);
static void register_panel_update(GtkWidget *widget, gpointer user_data);


/* account action functions -------------------- */

static void register_panel_action_editfilter(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_FILTER));
}


static void register_panel_action_add(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_ADD));
}

static void register_panel_action_inherit(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_INHERIT));
}

static void register_panel_action_edit(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_EDIT));
}

static void register_panel_action_multiedit(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_edit_multiple(data->window, NULL, 0, user_data);
}

static void register_panel_action_reconcile(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_RECONCILE));
}

static void register_panel_action_clear(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_CLEAR));
}

static void register_panel_action_none(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_NONE));
}

static void register_panel_action_delete(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_DELETE));
}


static void register_panel_action_find(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	gtk_widget_grab_focus(data->ST_search);
}



static void register_panel_action_createtemplate(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_make_archive(data->window, NULL);
}

static void register_panel_action_createassignment(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_make_assignment(data->window, NULL);
}


static void register_panel_action_exportcsv(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	register_panel_export_csv(data->window, NULL);
}


/* = = = = = = = = future version = = = = = = = = */

static void register_panel_action_exportpdf(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
gchar *name, *filepath;

	if(data->showall == FALSE)
	{
		name = g_strdup_printf("%s.pdf", data->acc->name);
		filepath = g_build_filename(PREFS->path_export, name, NULL);
		g_free(name);

		if( ui_dialog_export_pdf(GTK_WINDOW(data->window), &filepath) == GTK_RESPONSE_ACCEPT )
		{
			DB( g_printf(" filename is'%s'\n", filepath) );
	
			
			hb_export_pdf_listview(GTK_TREE_VIEW(data->LV_ope), filepath, data->acc->name);
		}

		g_free(filepath);
		
	}
}


static void register_panel_action_duplicate_mark(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
	
	DB( g_print("\n[register] check duplicate\n\n") );

	// open dialog to select date tolerance in days
	//  with info message
	//  with check/fix button and progress bar
	// parse listview txn, clear/mark duplicate
	// apply filter

	if(data->showall == FALSE)
	{
	gint daygap;

		daygap = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_txn_daygap));
		data->similar = transaction_similar_mark (data->acc, daygap);
		if( data->similar > 0 )
		{
		gchar *text = g_strdup_printf(_("There is %d group of similar transactions"), data->similar);
			gtk_label_set_text(GTK_LABEL(data->LB_duplicate), text);
			g_free(text);
		}
		else
			gtk_label_set_text(GTK_LABEL(data->LB_duplicate), _("No similar transaction were found !"));

		gtk_widget_show(data->IB_duplicate);
		//#GTK+710888: hack waiting a fix
		gtk_widget_queue_resize (data->IB_duplicate);

		gtk_widget_queue_draw (data->LV_ope);
	}


}


static void register_panel_action_duplicate_unmark(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	DB( g_print("\n[register] uncheck duplicate\n\n") );

	if(data->showall == FALSE)
	{   
		data->similar = 0;
		gtk_widget_hide(data->IB_duplicate);
		transaction_similar_unmark(data->acc);
		gtk_widget_queue_draw (data->LV_ope);
	}
}


static void register_panel_action_check_internal_xfer(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
GtkTreeModel *model;
GtkTreeIter iter;
GList *badxferlist;
gboolean valid, lockrecon;
gint count;

	DB( g_print("\n[register] check intenal xfer\n") );

	lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));

	badxferlist = NULL;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Transaction *txn;

		gtk_tree_model_get (model, &iter,
			MODEL_TXN_POINTER, &txn,
			-1);

		//#1909749 skip reconciled if lock is ON
		if( lockrecon && txn->status == TXN_STATUS_RECONCILED )
			goto next;

		if( txn->flags & OF_INTXFER )
		{
			if( transaction_xfer_child_strong_get(txn) == NULL )
			{
				DB( g_print(" - invalid xfer: '%s'\n", txn->memo) );
				
				//test unrecoverable (kxferacc = 0)
				if( txn->kxferacc <= 0 )
				{
					DB( g_print(" - unrecoverable, revert to normal xfer\n") );
					txn->flags |= OF_CHANGED;
					txn->paymode = PAYMODE_XFER;
					txn->kxfer = 0;
					txn->kxferacc = 0;
				}
				else
				{
					//perf must use preprend, see glib doc
					badxferlist = g_list_prepend(badxferlist, txn);
				}
			}
		}
next:
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	count = g_list_length (badxferlist);
	DB( g_print(" - found %d invalid int xfer\n", count) );

	if(count <= 0)
	{
		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
		_("Check internal transfer result"),
		_("No inconsistency found !")
		);
	}
	else
	{
	gboolean do_fix;
	
		do_fix = ui_dialog_msg_question(
			GTK_WINDOW(data->window),
			_("Check internal transfer result"),
			_("Inconsistency were found: %d\n"
			  "do you want to review and fix?"),
			count
			);

		if(do_fix == GTK_RESPONSE_YES)
		{	
		GList *tmplist = g_list_first(badxferlist);

			while (tmplist != NULL)
			{
			Transaction *stxn = tmplist->data;

				//future (open dialog to select date tolerance in days)
				transaction_xfer_search_or_add_child(GTK_WINDOW(data->window), stxn, 0);

				tmplist = g_list_next(tmplist);
			}	
		}
	}

	g_list_free (badxferlist);

}


//#1983995 copy raw amount
static void register_panel_action_copyrawamount(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
Transaction *ope;

	ope = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
	if(ope != NULL)
	{
	Currency *cur = da_cur_get(ope->kcur);

		if( cur != NULL )
		{
		GtkClipboard *clipboard = gtk_clipboard_get_default(gdk_display_get_default());
		gdouble monval = hb_amount_round(ABS(ope->amount), cur->frac_digits);
		gchar *text;

			text = g_strdup_printf ("%.*f", cur->frac_digits, monval);
			DB( g_print(" raw amount is '%s'\n", text) );
			gtk_clipboard_set_text(clipboard, text, strlen(text));
			g_free(text);
		}
	}
}


static void register_panel_action_viewsplit(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
Transaction *ope;

	ope = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
	if(ope != NULL)
		ui_split_view_dialog(data->window, ope);
}


/* = = = = = = = = end future version = = = = = = = = */


static void register_panel_action_exportqif(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
gchar *filename;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[register] export QIF\n") );

	if( ui_file_chooser_qif(GTK_WINDOW(data->window), &filename) == TRUE )
	{
		hb_export_qif_account_single(filename, data->acc);
		g_free( filename );
	}
}


static void register_panel_action_converttoeuro(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
gchar *msg;
gint result;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[register] convert euro\n") );

	msg = g_strdup_printf(_("Every transaction amount will be divided by %.6f."), PREFS->euro_value);

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			_("Are you sure you want to convert this account\nto Euro as Major currency?"),
			msg,
			_("_Convert"),
			FALSE
		);

	g_free(msg);

	if(result == GTK_RESPONSE_OK)
	{
		account_convert_euro(data->acc);
		register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_BALANCE));
	}
}


//#1818052 wish: copy/paste one/multiple transaction(s)
static void register_panel_action_edit_copy(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
gint count;

	DB( g_print("\n[register] copy\n") );
	
	//struct register_panel_data *data2;
	//data2 = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(menuitem), GTK_TYPE_WINDOW)), "inst_data");
	//DB( g_print("%p = %p\n", data, data2) );

	g_queue_free_full(data->q_txn_clip, (GDestroyNotify)da_transaction_free);
	data->q_txn_clip = g_queue_new();
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

	count = 0;
	list = g_list_first(selection);
	while(list != NULL)
	{
	Transaction *ope, *newope;
	GtkTreeIter iter;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

		newope = da_transaction_clone(ope);
		DB( g_print(" copy txn %p - '%.2f' '%s'\n", newope, newope->amount, newope->memo) );
		
		g_queue_push_tail(data->q_txn_clip, newope);
		count++;
		list = g_list_next(list);
	}

	g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selection);

	if(count > 0 )
		register_panel_update(data->window, GINT_TO_POINTER(FLG_REG_SENSITIVE));
	
}


static void register_panel_action_edit_paste(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
GList *list;
gboolean istoday = FALSE;

	DB( g_print("\n[register] paste normal\n") );

	istoday = ( (gpointer)menuitem == (gpointer)data->MI_pastet ) ? TRUE : FALSE;

	DB( g_print(" paste %d - as today=%d\n", (gint)g_queue_get_length(data->q_txn_clip), istoday) );
	
	list = g_queue_peek_head_link(data->q_txn_clip);
	while (list != NULL)
	{
	Transaction *item = list->data;
	Transaction *add_txn;
		
		DB( g_print(" paste txn %p - '%.2f' '%s'\n", item, item->amount, item->memo) );

		if( istoday == TRUE )
			item->date = GLOBALS->today;

		add_txn = transaction_add(GTK_WINDOW(data->window), item);
		add_txn->flags |= OF_ADDED;

		register_panel_add_after_propagate(data, add_txn);
		
		list = g_list_next(list);
	}

	register_panel_update(data->window, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));
	
}


static void register_panel_action_assign(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;
gint count;
gboolean usermode = TRUE;
gboolean lockrecon;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[register] assign\n") );

	lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));

	count = transaction_auto_assign(g_queue_peek_head_link(data->acc->txn_queue), data->acc->key, lockrecon);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_ope));
	GLOBALS->changes_count += count;

	//inform the user
	if(usermode == TRUE)
	{
	gchar *txt;

		if(count == 0)
			txt = _("No transaction changed");
		else
			txt = _("transaction changed: %d");

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
			_("Automatic assignment result"),
			txt,
			count);
	}

	//refresh main
	if( count > 0 )
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void register_panel_action_close(GtkMenuItem *menuitem, gpointer     user_data)
{
struct register_panel_data *data = user_data;

	DB( g_print("\n[register] close\n") );

	DB( g_print("window %p\n", data->window) );

	gtk_widget_destroy (GTK_WIDGET (data->window));

	//g_signal_emit_by_name(data->window, "delete-event", NULL, &result);

}


/* these 5 functions are independant from account window */

/* account functions -------------------- */

static void register_panel_export_csv(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gchar *name, *filepath;
GString *node;
GIOChannel *io;
gboolean hassplit, hasstatus;

	DB( g_print("\n[register] export csv\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	name = g_strdup_printf("%s.csv", (data->showall == TRUE) ? GLOBALS->owner :data->acc->name);

	filepath = g_build_filename(PREFS->path_export, name, NULL);

	if( ui_dialog_export_csv(GTK_WINDOW(data->window), &filepath, &hassplit, &hasstatus, data->showall) == GTK_RESPONSE_ACCEPT )
	{
		DB( g_printf(" filename is '%s'\n", filepath) );

		io = g_io_channel_new_file(filepath, "w", NULL);
		if(io != NULL)
		{
			//TODO: add showall & detailsplit here
			//+ handle it
			node = list_txn_to_string(GTK_TREE_VIEW(data->LV_ope), FALSE, hassplit, hasstatus, data->showall);

			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
	}

	g_free(filepath);
	g_free(name);

}


static void register_panel_edit_multiple(GtkWidget *widget, Transaction *txn, gint column_id, gpointer user_data)
{
struct register_panel_data *data;
GtkWidget *dialog;

	DB( g_print("\n[register] edit multiple\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" - txn:%p, column: %d\n", txn, column_id) );

	dialog = ui_multipleedit_dialog_new(GTK_WINDOW(data->window), GTK_TREE_VIEW(data->LV_ope));

	if(txn != NULL && column_id != 0)
	{
		ui_multipleedit_dialog_prefill(dialog, txn, column_id);
	}

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if( result == GTK_RESPONSE_ACCEPT )
	{
	gboolean do_sort;
	gint changes;

		//#1792808: sort if date changed 
		changes = ui_multipleedit_dialog_apply (dialog, &do_sort);
		data->do_sort = do_sort;
		if( changes > 0 )
		{
			//#1782749 update account status
			if( data->acc != NULL )
				data->acc->flags |= AF_CHANGED;

			ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
		}
	}

	gtk_widget_destroy (dialog);
	
	register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));
}


static void register_panel_make_assignment(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
GtkTreeModel *model;
GList *selection, *list;
gint result, count;

	DB( g_print("\n[register] make assignment\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");


	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	if( count > 0 )
	{

		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->window),
				NULL,
				_("Do you want to create an assignment with\neach of the selected transaction?"),
				_("_Create"),
				FALSE
			);

	/*
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("%d archives will be created"),
			GLOBALS->changes_count
			);
	*/

		if(result == GTK_RESPONSE_OK)
		{
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

			list = g_list_first(selection);
			while(list != NULL)
			{
			Assign *item;
			Transaction *ope;
			GtkTreeIter iter;

				gtk_tree_model_get_iter(model, &iter, list->data);
				gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

				DB( g_print(" create assignment %s %.2f\n", ope->memo, ope->amount) );

				//TODO: migrate to hb-assign
				item = da_asg_malloc();

				da_asg_init_from_transaction(item, ope);
				
				da_asg_append(item);

				GLOBALS->changes_count++;

				list = g_list_next(list);
			}

			g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
			g_list_free(selection);
		}
	}
}


static void register_panel_make_archive(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
GtkTreeModel *model;
GList *selection, *list;
gint result, count;

	DB( g_print("\n[register] make archive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");


	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	if( count > 0 )
	{

		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->window),
				NULL,
				_("Do you want to create a template with\neach of the selected transaction?"),
				_("_Create"),
				FALSE
			);

	/*
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("%d archives will be created"),
			GLOBALS->changes_count
			);
	*/

		if(result == GTK_RESPONSE_OK)
		{
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

			list = g_list_first(selection);
			while(list != NULL)
			{
			Archive *item;
			Transaction *ope;
			GtkTreeIter iter;

				gtk_tree_model_get_iter(model, &iter, list->data);
				gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

				DB( g_print(" create archive %s %.2f\n", ope->memo, ope->amount) );

				item = da_archive_malloc();

				da_archive_init_from_transaction(item, ope, TRUE);

				//GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
				da_archive_append_new(item);
				GLOBALS->changes_count++;

				list = g_list_next(list);
			}

			g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
			g_list_free(selection);
		}
	}
}



static void register_panel_move_up(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data = user_data;
Transaction *txn = NULL;
Transaction *prevtxn = NULL;
gint count = 0;
	
	DB( g_print("\n[register] move up\n\n") );

	txn = list_txn_get_surround_transaction(GTK_TREE_VIEW(data->LV_ope), &prevtxn, NULL);
	if( txn && prevtxn )
	{
		if( txn->date == prevtxn->date )
		{
			DB( g_print(" swapping, as txn are same date\n") );
			//swap position
			gint savedpos = txn->pos;
			txn->pos = prevtxn->pos;
			prevtxn->pos = savedpos;
			GLOBALS->changes_count++;
			count++;
		}
	}

	if( count > 0 )
	{
		data->do_sort = TRUE;
		register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));
	}		
}


static void register_panel_move_down(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data = user_data;
Transaction *txn = NULL;
Transaction *nexttxn = NULL;
gint count = 0;

	DB( g_print("\n[register] move down\n\n") );

	txn = list_txn_get_surround_transaction(GTK_TREE_VIEW(data->LV_ope), NULL, &nexttxn);
	if( txn && nexttxn )
	{
		if( txn->date == nexttxn->date )
		{
			DB( g_print(" swapping, as txn are same date\n") );
			//swap position
			gint savedpos = txn->pos;
			txn->pos = nexttxn->pos;
			nexttxn->pos = savedpos;
			GLOBALS->changes_count++;
			count++;
		}
	}

	if( count > 0 )
	{
		data->do_sort = TRUE;
		register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));
	}		
}


static void register_panel_cb_bar_duplicate_response(GtkWidget *info_bar, gint response_id, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[register] bar_duplicate_response\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(info_bar, GTK_TYPE_WINDOW)), "inst_data");

	switch( response_id )
	{
		case HB_RESPONSE_REFRESH:
			register_panel_action_duplicate_mark(NULL, data);
			break;
		case GTK_RESPONSE_CLOSE:
			register_panel_action_duplicate_unmark(NULL, data);
			gtk_widget_hide (GTK_WIDGET (info_bar));	
			break;
	}
}


static gboolean register_panel_cb_recon_change (GtkWidget *widget, gboolean state, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[register] cb recon change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	
	DB( g_print(" state=%d switch=%d\n", state, gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled)) ) );

	register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE));

	return FALSE;
}



static void register_panel_cb_filter_daterange(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gboolean future;
gint range;

	DB( g_print("\n[register] filter_daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_range));
	future = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_future));

	data->filter->nbdaysfuture = 0;

	//in 5.6 no longer custom open the filter
	//if(range != FLT_RANGE_OTHER)
	//{
		filter_preset_daterange_set(data->filter, range, (data->showall == FALSE) ? data->acc->key : 0);
		// add eventual x days into future display
		if( future && (PREFS->date_future_nbdays > 0) )
			filter_preset_daterange_add_futuregap(data->filter, PREFS->date_future_nbdays);
		
		register_panel_collect_filtered_txn(data->LV_ope, FALSE);
		register_panel_listview_populate(data->LV_ope);
	/*}
	else
	{
		if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, data->showall, TRUE) != GTK_RESPONSE_REJECT)
		{
			register_panel_collect_filtered_txn(data->LV_ope, FALSE);
			register_panel_listview_populate(data->LV_ope);
			register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));
		}
	}*/
}


static void register_panel_cb_filter_type(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint type;

	DB( g_print("\n[register] filter_type\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_type));

	filter_preset_type_set(data->filter, type);

	register_panel_collect_filtered_txn(data->LV_ope, FALSE);
	register_panel_listview_populate(data->LV_ope);
}


static void register_panel_cb_filter_status(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint status;

	DB( g_print("\n[register] filter_status\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	status = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_status));

	filter_preset_status_set(data->filter, status);

	register_panel_collect_filtered_txn(data->LV_ope, FALSE);
	register_panel_listview_populate(data->LV_ope);
}


//#1960755 add refresh button
static void register_panel_cb_filter_refresh(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[register] filter_refresh\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	register_panel_collect_filtered_txn(data->LV_ope, FALSE);
	register_panel_listview_populate(data->LV_ope);
}


static void register_panel_cb_filter_reset(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint dspstatus;

	DB( g_print("\n[register] filter_reset\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	filter_reset(data->filter);

	filter_preset_daterange_set (data->filter, PREFS->date_range_txn, (data->showall == FALSE) ? data->acc->key : 0);

	if(PREFS->hidereconciled)
		filter_preset_status_set (data->filter, FLT_STATUS_UNRECONCILED);

	// add eventual x days into future display
	if( PREFS->date_future_nbdays > 0 )
		filter_preset_daterange_add_futuregap(data->filter, PREFS->date_future_nbdays);

	register_panel_collect_filtered_txn(data->LV_ope, TRUE);
	register_panel_listview_populate(data->LV_ope);
	
	g_signal_handler_block(data->CY_range, data->handler_id[HID_RANGE]);
	g_signal_handler_block(data->CY_type, data->handler_id[HID_TYPE]);
	g_signal_handler_block(data->CY_status, data->handler_id[HID_STATUS]);

	DB( g_print(" - set range : %d\n", data->filter->range) );
	DB( g_print(" - set type  : %d\n", data->filter->type) );
	DB( g_print(" - set status: %d\n", data->filter->status) );

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), data->filter->range);
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_type), data->filter->type);
	//#1873324 register status quick filter do not reset 
	//gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_status), data->filter->rawstatus);
	//#1878483 status with hidereconciled shows reconciled (due to filter !reconciled internal
	dspstatus = data->filter->status;
	if( (dspstatus == FLT_STATUS_RECONCILED) && (data->filter->option[FLT_GRP_STATUS] == 2) )
		dspstatus = FLT_STATUS_UNRECONCILED;
	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_status), dspstatus);

	g_signal_handler_unblock(data->CY_status, data->handler_id[HID_STATUS]);
	g_signal_handler_unblock(data->CY_type, data->handler_id[HID_TYPE]);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_RANGE]);
	
}


static void register_panel_balance_refresh(GtkWidget *view)
{
struct register_panel_data *data;
Transaction *minbalope;
GList *list;
gdouble balance;
GtkTreeModel *model;
gdouble lbalance = 0;
guint32 ldate = 0;
gushort lpos = 1;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(view, GTK_TYPE_WINDOW)), "inst_data");

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[register] balance refresh kacc=%d\n", data->acc != NULL ? (gint)data->acc->key : -1) );

	balance = data->acc->initial;

	//#1270687: sort if date changed
	if(data->do_sort)
	{
		DB( g_print(" - complete txn sort\n") );
		
		da_transaction_queue_sort(data->acc->txn_queue);
		data->do_sort = FALSE;
	}

	minbalope = NULL;
	list = g_queue_peek_head_link(data->acc->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;
	gdouble value;

		//#1267344 maybe no remind in running balance
		if( transaction_is_balanceable(ope) )
			balance += ope->amount;

		ope->balance = balance;

		// clear mark flags
		ope->dspflags &= ~(TXN_DSPFLG_OVER|TXN_DSPFLG_LOWBAL);

		//#1661806 add show overdraft
		//#1672209 added round like for #400483
		value = hb_amount_round(balance, 2);
		if( (value != 0.0) && (value < data->acc->minimum) )
		{
			ope->dspflags |= TXN_DSPFLG_OVER;
		}
	
		//# mark lowest balance for future
		if( (ope->date > GLOBALS->today) )
		{
			if( balance < lbalance )
				minbalope = ope;
		}
		
		if(ope->date == ldate)
		{
			ope->pos = ++lpos;	
		}
		else
		{
			ope->pos = lpos = 1;
		}

		ldate = ope->date;
		lbalance = balance;

		list = g_list_next(list);
	}

	if( minbalope != NULL )
		minbalope->dspflags |= TXN_DSPFLG_LOWBAL;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	list_txn_sort_force(GTK_TREE_SORTABLE(model), NULL);
	
}


static void register_panel_collect_filtered_txn(GtkWidget *view, gboolean emptysearch)
{
struct register_panel_data *data;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;

	DB( g_print("\n[register] collect_filtered_txn\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(view, GTK_TYPE_WINDOW)), "inst_data");

	if(data->gpatxn != NULL)
		g_ptr_array_free (data->gpatxn, TRUE);

	//TODO: why this ?
	data->gpatxn = g_ptr_array_sized_new(64);

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		// skip closed in showall mode
		//#1861337 users want them
		//if( data->showall == TRUE && (acc->flags & AF_CLOSED) )
		//	goto next_acc;

		// skip other than current in normal mode
		if( (data->showall == FALSE) && (data->acc != NULL) && (acc->key != data->acc->key) )
			goto next_acc;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *ope = lnk_txn->data;

			if(filter_txn_match(data->filter, ope) == 1)
			{
				//add to the list
				g_ptr_array_add(data->gpatxn, (gpointer)ope);
			}
			lnk_txn = g_list_next(lnk_txn);
		}
	
	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	//#1789698 not always empty
	if( emptysearch == TRUE )
	{
		g_signal_handler_block(data->ST_search, data->handler_id[HID_SEARCH]);
		gtk_entry_set_text (GTK_ENTRY(data->ST_search), "");
		g_signal_handler_unblock(data->ST_search, data->handler_id[HID_SEARCH]);
	}	
}




static void register_panel_listview_populate(GtkWidget *widget)
{
struct register_panel_data *data;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean hastext;
gchar *needle;
gint sort_column_id;
GtkSortType order;
guint i, qs_flag;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[register] listview_populate\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));

	// ref model to keep it
	//g_object_ref(model);
	//gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_ope), NULL);
	gtk_tree_store_clear (GTK_TREE_STORE(model));


	// perf: if you leave the sort, insert is damned slow
	gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), &sort_column_id, &order);
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, PREFS->lst_ope_sort_order);

	hastext = (gtk_entry_get_text_length (GTK_ENTRY(data->ST_search)) >= 2) ? TRUE : FALSE;
	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));

	//build the mask flag for quick search
	qs_flag = 0;
	if(hastext)
	{
		qs_flag = list_txn_get_quicksearch_column_mask(GTK_TREE_VIEW(data->LV_ope));
	}
	
	data->total = 0;
	data->totalsum = 0.0;

	for(i=0;i<data->gpatxn->len;i++)
	{
	Transaction *txn = g_ptr_array_index(data->gpatxn, i);
	gboolean insert = TRUE;
		
		if(hastext)
		{
			insert = filter_txn_search_match(needle, txn, qs_flag);
		}

		if(insert)
		{
			//gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	 		//gtk_list_store_set (GTK_LIST_STORE(model), &iter,
	 		gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
				MODEL_TXN_POINTER, txn,
				-1);

			if( data->showall == FALSE )
				data->totalsum += txn->amount;
			else
				data->totalsum += hb_amount_base (txn->amount, txn->kcur);

			data->total++;
		}
	}
	
	//gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_ope), model); /* Re-attach model to view */
	//g_object_unref(model);

	// push back the sort id
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), sort_column_id, order);

	/* update info range text */
	{
	gchar *daterange;
		
		daterange = filter_daterange_text_get(data->filter);
		gtk_widget_set_tooltip_markup(GTK_WIDGET(data->CY_range), daterange);
		gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
		g_free(daterange);
	}
	
	register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));

}

static gint list_txn_get_count_reconciled(GtkTreeView *treeview)
{
GtkTreeModel *model;
GList *lselection, *list;
gint count = 0;
	
	model = gtk_tree_view_get_model(treeview);
	lselection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(treeview), &model);

	list = g_list_last(lselection);
	while(list != NULL)
	{
	GtkTreeIter iter;
	Transaction *txn;


		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &txn, -1);
		if(txn->status == TXN_STATUS_RECONCILED)
			count++;
		
		list = g_list_previous(list);
	}

	g_list_foreach(lselection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(lselection);

	return count;
}


static void list_txn_add_by_value(GtkTreeView *treeview, Transaction *ope)
{
GtkTreeModel *model;
GtkTreeIter  iter;
//GtkTreePath *path;
//GtkTreeSelection *sel;

	if( ope == NULL )
		return;
	
	DB( g_print("\n[transaction] add_treeview\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
		MODEL_TXN_POINTER, ope,
		-1);

	//activate that new line
	//path = gtk_tree_model_get_path(model, &iter);
	//gtk_tree_view_expand_to_path(GTK_TREE_VIEW(treeview), path);

	//sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	//gtk_tree_selection_select_iter(sel, &iter);

	//gtk_tree_path_free(path);

}


/* used to remove a intxfer child from a treeview */
static void list_txn_remove_by_value(GtkTreeModel *model, Transaction *txn)
{
GtkTreeIter iter;
gboolean valid;

	if( txn == NULL )
		return;

	DB( g_print("remove by value %p\n\n", txn) );

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Transaction *tmp;

		gtk_tree_model_get (model, &iter,
			MODEL_TXN_POINTER, &tmp,
			-1);

		if( txn == tmp )
		{
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
			break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


// this func to some toggle
static void list_txn_status_selected_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
gint targetstatus = GPOINTER_TO_INT(userdata);
Transaction *txn;
gboolean saverecondate = FALSE;

	gtk_tree_model_get(model, iter, MODEL_TXN_POINTER, &txn, -1);

	account_balances_sub(txn);
	
	switch(targetstatus)
	{
		case TXN_STATUS_NONE:
			switch(txn->status)
			{
				case TXN_STATUS_CLEARED:
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_NONE;
					txn->flags |= OF_CHANGED;
					break;
			}
			break;

		case TXN_STATUS_CLEARED:
			switch(txn->status)
			{
				case TXN_STATUS_NONE:
					txn->status = TXN_STATUS_CLEARED;
					txn->flags |= OF_CHANGED;
					break;
				case TXN_STATUS_CLEARED:
					txn->status = TXN_STATUS_NONE;
					txn->flags |= OF_CHANGED;
					break;
			}
			break;
			
		case TXN_STATUS_RECONCILED:
			switch(txn->status)
			{
				case TXN_STATUS_NONE:
				case TXN_STATUS_CLEARED:
					txn->status = TXN_STATUS_RECONCILED;
					txn->flags |= OF_CHANGED;
					saverecondate = TRUE;
					break;
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_CLEARED;
					txn->flags |= OF_CHANGED;
					break;
			}
			break;

	}

	transaction_changed(txn, saverecondate);
	
	account_balances_add(txn);
	
	/* #492755 let the child transfer unchanged */

}


static void register_panel_add_after_propagate(struct register_panel_data *data, Transaction *add_txn)
{

	if((data->showall == TRUE) || ( (data->acc != NULL) && (add_txn->kacc == data->acc->key) ) )
	{
		list_txn_add_by_value(GTK_TREE_VIEW(data->LV_ope), add_txn);
		//#1716181 also add to the ptr_array (quickfilter)
		g_ptr_array_add(data->gpatxn, (gpointer)add_txn);

		//#1840100 updates when use multiple account window
		if( (add_txn->flags & OF_INTXFER) )
		{
		GtkWindow *accwin = account_window(add_txn->kxferacc);

			if(accwin)
			{
			Transaction *child = transaction_xfer_child_strong_get(add_txn);

				if( child )
				{
					register_panel_add_single_transaction(accwin, child);
					register_panel_update(GTK_WIDGET(accwin), GINT_TO_POINTER(FLG_REG_BALANCE));
				}
			}
		}
	}
}


static void lst_txn_remove_active_transaction(GtkTreeView *treeview)
{
GtkTreeModel *model;
GList *list;

	model = gtk_tree_view_get_model(treeview);
	list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(treeview), &model);

	if(list != NULL)
	{
	GtkTreeIter iter;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);


}


static void register_panel_add_single_transaction(GtkWindow *window, Transaction *txn)
{
struct register_panel_data *data;

	if(txn == NULL)
		return;

	DB( g_print("\n[register] add single txn\n") );
	
	data = g_object_get_data(G_OBJECT(window), "inst_data");

	list_txn_add_by_value(GTK_TREE_VIEW(data->LV_ope), txn);
}


static void register_panel_action(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint action = GPOINTER_TO_INT(user_data);
guint changes = GLOBALS->changes_count;
gboolean result;

	DB( g_print("\n[register] action\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	DB( g_print(" - action=%d\n", action) );

	switch(action)
	{
		case ACTION_ACCOUNT_ADD:
		case ACTION_ACCOUNT_INHERIT:
		{
		GtkWidget *dialog;
		gint kacc, type = 0;

			homebank_app_date_get_julian();
			
			if(action == ACTION_ACCOUNT_ADD)
			{
				DB( g_print("(transaction) add multiple\n") );
				data->cur_ope = da_transaction_malloc();
				// miss from 5.2.8 ??
				//da_transaction_set_default_template(src_txn);
				type = TXN_DLG_ACTION_ADD;
				result = HB_RESPONSE_ADD;
			}
			else
			{
				DB( g_print("(transaction) inherit multiple\n") );
				data->cur_ope = da_transaction_clone(list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope)));
				//#1873311 inherit+kepplastdate=OFF = today
				if( PREFS->heritdate == FALSE ) 
					data->cur_ope->date = GLOBALS->today;

				//#1432204 inherit => status none
				data->cur_ope->status = TXN_STATUS_NONE;
				type = TXN_DLG_ACTION_INHERIT;
				result = HB_RESPONSE_ADDKEEP;
			}

			kacc = (data->acc != NULL) ? data->acc->key : 0;
			dialog = create_deftransaction_window(GTK_WINDOW(data->window), type, TXN_DLG_TYPE_TXN, kacc );
			while(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP)
			{
				if( result == HB_RESPONSE_ADD )
				{
					da_transaction_init(data->cur_ope, kacc);
				}

				deftransaction_set_transaction(dialog, data->cur_ope);

				result = gtk_dialog_run (GTK_DIALOG (dialog));

				DB( g_print(" - dialog result is %d\n", result) );

				if(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP || result == GTK_RESPONSE_ACCEPT)
				{
				Transaction *add_txn;
				
					deftransaction_get(dialog, NULL);

					add_txn = transaction_add(GTK_WINDOW(data->window), data->cur_ope);
					//#1831975
					if(PREFS->txn_showconfirm)
						deftransaction_external_confirm(dialog, add_txn);

					DB( g_print(" - added 1 transaction to %d\n", add_txn->kacc) );

					register_panel_add_after_propagate(data, add_txn);

					register_panel_update(widget, GINT_TO_POINTER(FLG_REG_BALANCE));
					//#1667201 already done into transaction_add
					//data->acc->flags |= AF_ADDED;
					GLOBALS->changes_count++;
				}
			}

			da_transaction_free (data->cur_ope);

			deftransaction_dispose(dialog, NULL);
			gtk_widget_destroy (dialog);
		}
		break;

		case ACTION_ACCOUNT_EDIT:
			{
		Transaction *active_txn;

			DB( g_print(" - edit\n") );
				
			active_txn = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
					
			if(active_txn)
			{
			Transaction *old_txn, *new_txn;

				old_txn = da_transaction_clone (active_txn);
				new_txn = active_txn;
				
				result = deftransaction_external_edit(GTK_WINDOW(data->window), old_txn, new_txn);

				if(result == GTK_RESPONSE_ACCEPT)
				{
					//manage current window display stuff
					
					//#1270687: sort if date changed
					//if(old_txn->date != new_txn->date)
					//	data->do_sort = TRUE;
					//#1931816: sort is already done in deftransaction_external_edit
					// but still to be done if showall
					if(data->showall == FALSE)
					{
					GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
						list_txn_sort_force(GTK_TREE_SORTABLE(model), NULL);
					}

					// txn changed of account
					//TODO: maybe this should move to deftransaction_external_edit
					if( data->acc != NULL && (new_txn->kacc != data->acc->key) )
					{
					GtkWindow *accwin = account_window(new_txn->kacc);
						
						lst_txn_remove_active_transaction(GTK_TREE_VIEW(data->LV_ope));
						//#1667501 update target account window if open
						if(accwin)
						{
							register_panel_add_single_transaction(accwin, new_txn);
							register_panel_update(GTK_WIDGET(accwin), GINT_TO_POINTER(FLG_REG_BALANCE));
						}
					}

					//#1812470 txn is xfer update target account window if open
					if( (old_txn->flags & OF_INTXFER) && (old_txn->amount != new_txn->amount) )
					{
					GtkWindow *accwin = account_window(new_txn->kxferacc);

						if(accwin)
						{
							register_panel_update(GTK_WIDGET(accwin), GINT_TO_POINTER(FLG_REG_BALANCE));
						}
					}

					//da_transaction_copy(new_txn, old_txn);

					register_panel_update(widget, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));

					//TODO: saverecondate is handled in external edit already
					transaction_changed(new_txn, FALSE);

					GLOBALS->changes_count++;
				}

				da_transaction_free (old_txn);
			}
		}
		break;

		case ACTION_ACCOUNT_DELETE:
		{
		GtkTreeModel *model;
		GList *selection, *list;
		gint result;

			DB( g_print(" - delete\n") );

			result = ui_dialog_msg_confirm_alert(
					GTK_WINDOW(data->window),
					NULL,
					_("Do you want to delete\neach of the selected transaction?"),
					_("_Delete"),
					TRUE
				);

			if(result == GTK_RESPONSE_OK)
			{
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
				selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

				//block selection change to avoid refresh and call to update
				g_signal_handlers_block_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope))), G_CALLBACK (register_panel_selection), NULL);
				
				list = g_list_last(selection);
				while(list != NULL)
				{
				Transaction *rem_txn;
				GtkTreeIter iter;

					//#1860232 crash here if no test when reach a txn already removed
					if( gtk_tree_model_get_iter(model, &iter, list->data) == TRUE )
					{
						gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &rem_txn, -1);

						DB( g_print(" delete %s %.2f\n", rem_txn->memo, rem_txn->amount) );

						//#1716181 also remove from the ptr_array (quickfilter)
						g_ptr_array_remove(data->gpatxn, (gpointer)rem_txn);

						// 1) remove visible current and potential xfer
						gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);

						//manage target open window as well
						if((rem_txn->flags & OF_INTXFER))
						{
						Transaction *child = transaction_xfer_child_strong_get(rem_txn);
							if( child )
							{
								if( data->showall )
								{
									list_txn_remove_by_value(model, child);
									//#1716181 also remove from the ptr_array (quickfilter)				
									g_ptr_array_remove(data->gpatxn, (gpointer)child);
									data->total--;
									GLOBALS->changes_count++;
								}
								//TODO: else
								
							}
						}

						// 2) remove datamodel
						transaction_remove(rem_txn);
						data->total--;
						GLOBALS->changes_count++;
					}

					list = g_list_previous(list);
				}

				g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
				g_list_free(selection);

				g_signal_handlers_unblock_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope))), G_CALLBACK (register_panel_selection), NULL);
				
				register_panel_update(widget, GINT_TO_POINTER(FLG_REG_BALANCE));
			}
		}
		break;

		case ACTION_ACCOUNT_NONE:
		{
		GtkTreeSelection *selection;
		gint count, result;
			
			count = list_txn_get_count_reconciled(GTK_TREE_VIEW(data->LV_ope));

			if(count > 0 )
			{
			
			result = ui_dialog_msg_confirm_alert(
					GTK_WINDOW(data->window),
					_("Are you sure you want to change the status to None?"),
					_("Some transaction in your selection are already Reconciled."),
					_("_Change"),
					FALSE
				);
			}
			else
				result = GTK_RESPONSE_OK;
				
			if( result == GTK_RESPONSE_OK )
			{
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
				gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)list_txn_status_selected_foreach_func, 
					GINT_TO_POINTER(TXN_STATUS_NONE));

				DB( g_print(" - none\n") );

				gtk_widget_queue_draw (data->LV_ope);
				//gtk_widget_queue_resize (data->LV_acc);

				register_panel_update(widget, GINT_TO_POINTER(FLG_REG_BALANCE));

				GLOBALS->changes_count++;
			}

		}
		break;

		case ACTION_ACCOUNT_CLEAR:
		{
			GtkTreeSelection *selection;
			
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
			gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)list_txn_status_selected_foreach_func, 
				GINT_TO_POINTER(TXN_STATUS_CLEARED));

			DB( g_print(" - clear\n") );

			gtk_widget_queue_draw (data->LV_ope);
			//gtk_widget_queue_resize (data->LV_acc);

			register_panel_update(widget, GINT_TO_POINTER(FLG_REG_BALANCE));

			GLOBALS->changes_count++;
		}
		break;

		case ACTION_ACCOUNT_RECONCILE:
		{
		GtkTreeSelection *selection;
		gint count, result;
			
			count = list_txn_get_count_reconciled(GTK_TREE_VIEW(data->LV_ope));

			if(count > 0 )
			{
			
			result = ui_dialog_msg_confirm_alert(
					GTK_WINDOW(data->window),
					_("Are you sure you want to toggle the status Reconciled?"),
					_("Some transaction in your selection are already Reconciled."),
					_("_Toggle"),
					FALSE
				);
			}
			else
				result = GTK_RESPONSE_OK;
				
			if( result == GTK_RESPONSE_OK )
			{
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
				gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)list_txn_status_selected_foreach_func, 
					GINT_TO_POINTER(TXN_STATUS_RECONCILED));

				DB( g_print(" - reconcile\n") );

				gtk_widget_queue_draw (data->LV_ope);
				//gtk_widget_queue_resize (data->LV_acc);

				register_panel_update(widget, GINT_TO_POINTER(FLG_REG_BALANCE));

				GLOBALS->changes_count++;
			}

		}
		break;

		case ACTION_ACCOUNT_FILTER:
		{

			if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, data->showall, TRUE) != GTK_RESPONSE_REJECT)
			{
				register_panel_collect_filtered_txn(data->LV_ope, TRUE);
				register_panel_listview_populate(data->LV_ope);
				register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));

				g_signal_handler_block(data->CY_range, data->handler_id[HID_RANGE]);
				hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), FLT_RANGE_MISC_CUSTOM);
				g_signal_handler_unblock(data->CY_range, data->handler_id[HID_RANGE]);
			}

		}
		break;

	}

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}



static void register_panel_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[register] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_BALANCE));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_ope));
}


static gboolean register_panel_cb_on_key_press(GtkWidget *source, GdkEventKey *event, gpointer user_data)
{
struct register_panel_data *data = user_data;

	// On Control-f enable search entry
	//already bind to menu Txn > Find ctrl+F
	/*if (event->state & GDK_CONTROL_MASK
		&& event->keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->ST_search);
	}
	else*/
	if (event->keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
	{
		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), NULL);
		gtk_widget_grab_focus(data->LV_ope);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}



static void register_panel_selection(GtkTreeSelection *treeselection, gpointer user_data)
{

	DB( g_print("\n[register] selection changed cb\n") );


	register_panel_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(FLG_REG_SENSITIVE));

}


static void register_panel_update(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
GtkTreeSelection *selection;
gint flags = GPOINTER_TO_INT(user_data);
gboolean lockrecon, visible;
gint count = 0;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	DB( g_print("\n[register] update kacc=%d\n", data->acc != NULL ? (gint)data->acc->key : -1) );

	
	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	/* set window title */
	if(flags & FLG_REG_TITLE)
	{
		DB( g_print(" FLG_REG_TITLE\n") );

	}

	/* update toolbar & list */
	if(flags & FLG_REG_VISUAL)
	{
	//gboolean visible;
		
		DB( g_print(" FLG_REG_VISUAL\n") );

		if(PREFS->toolbar_style == 0)
			gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
		else
			gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);

		//minor ?
		hb_widget_visible (data->CM_minor, PREFS->euro_active);
	}

	/* update balances */
	if(flags & FLG_REG_BALANCE)
	{
		DB( g_print(" FLG_REG_BALANCE\n") );

		if(data->showall == FALSE)
		{
		Account *acc = data->acc;

			register_panel_balance_refresh(widget);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[0]), acc->bal_recon, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[1]), acc->bal_clear, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[2]), acc->bal_today, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[3]), acc->bal_future, acc->kcur, GLOBALS->minor);
		}
		else
		{
		GList *lst_acc, *lnk_acc;
		gdouble recon, clear, today, future;
	
			recon = clear = today = future = 0.0;
			lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
			lnk_acc = g_list_first(lst_acc);
			while (lnk_acc != NULL)
			{
			Account *acc = lnk_acc->data;

				recon += hb_amount_base(acc->bal_recon, acc->kcur);
				clear += hb_amount_base(acc->bal_clear, acc->kcur);
				today += hb_amount_base(acc->bal_today, acc->kcur);
				future += hb_amount_base(acc->bal_future, acc->kcur);
				
				lnk_acc = g_list_next(lnk_acc);
			}
			g_list_free(lst_acc);
		
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[0]), recon, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[1]), clear, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[2]), today, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[3]), future, GLOBALS->kcur, GLOBALS->minor);
		}
		ui_hub_account_compute(GLOBALS->mainwindow, NULL);
	}

	/* update disabled things */
	if(flags & FLG_REG_SENSITIVE)
	{
	gboolean sensitive, psensitive, nsensitive;
	GtkTreeModel *model;
	gint sort_column_id;
	GtkSortType order;
	Transaction *ope;

		DB( g_print(" FLG_REG_SENSITIVE\n") );

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
		count = gtk_tree_selection_count_selected_rows(selection);
		DB( g_print(" - count = %d\n", count) );

		ope = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));

		//showall part
		visible = !data->showall;
		hb_widget_visible(data->MI_exportpdf, visible);
		hb_widget_visible(data->MI_print, visible);
		hb_widget_visible(data->MI_exportqif, visible);
		//5.4.3 enabled 
		//hb_widget_visible(data->MI_exportcsv, visible);
		//tools
		hb_widget_visible(data->MI_markdup, visible);
		hb_widget_visible(data->MI_chkintxfer, visible);
		//#1873248 Auto. assignments faulty visible on 'All transactions' window
		hb_widget_visible(data->MI_autoassign, visible);

		//1909749 lock/unlock reconciled
		lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));
		if( ope != NULL )
		{
			if( (ope->status != TXN_STATUS_RECONCILED) )
				lockrecon = FALSE;

			DB( g_print(" - lockrecon = %d (%d %d)\n", lockrecon, ope->status != TXN_STATUS_RECONCILED, gtk_switch_get_state (GTK_SWITCH(data->SW_lockreconciled)) ) );
		}


		//5.3.1 if closed account : disable any change
		sensitive =  TRUE;
		if( (data->showall == FALSE) && (data->acc->flags & AF_CLOSED) )
			sensitive = FALSE;

		gtk_widget_set_sensitive(data->TB_bar, sensitive);
		gtk_widget_set_sensitive(data->ME_menuedit, sensitive);
		gtk_widget_set_sensitive(data->ME_menutxn, sensitive);
		gtk_widget_set_sensitive(data->ME_menutools, sensitive);
		gtk_widget_set_sensitive(data->ME_popmenu, sensitive);

		// multiple: disable inherit, edit
		sensitive = (count != 1 ) ? FALSE : TRUE;
		gtk_widget_set_sensitive(data->MI_herit, sensitive);
		gtk_widget_set_sensitive(data->MI_popherit, sensitive);
		gtk_widget_set_sensitive(data->MI_edit, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->MI_popedit, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->BT_herit, sensitive);
		gtk_widget_set_sensitive(data->BT_edit, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->MI_popcopyamount, sensitive);

		//txn have split
		sensitive = (count == 1) && (ope != NULL) && (ope->flags & OF_SPLIT) ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_popviewsplit, sensitive);
		
		// single: disable multiedit
		sensitive = (count <= 1 ) ? FALSE : TRUE;
		//1909749 lock/unlock reconciled
		if( (list_txn_get_count_reconciled(GTK_TREE_VIEW(data->LV_ope)) > 0) && (gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled)) == TRUE) )
			sensitive = FALSE;
		gtk_widget_set_sensitive(data->MI_multiedit   , sensitive);
		gtk_widget_set_sensitive(data->MI_popmultiedit, sensitive);
		gtk_widget_set_sensitive(data->BT_multiedit   , sensitive);

		// no selection: disable reconcile, delete
		sensitive = (count > 0 ) ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_copy, sensitive);
		gtk_widget_set_sensitive(data->ME_menustatus, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->ME_popmenustatus, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->MI_delete, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->MI_popdelete, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->MI_assign, sensitive);
		gtk_widget_set_sensitive(data->MI_popassign, sensitive);
		gtk_widget_set_sensitive(data->MI_template, sensitive);
		gtk_widget_set_sensitive(data->MI_poptemplate, sensitive);
		gtk_widget_set_sensitive(data->BT_delete, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->BT_clear, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->BT_reconcile, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->BT_template, sensitive);

		//edit menu
		sensitive = g_queue_get_length(data->q_txn_clip) > 0 ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_pasten, sensitive);
		gtk_widget_set_sensitive(data->MI_pastet, sensitive);

		// euro convert
		visible = (data->showall == TRUE) ? FALSE : PREFS->euro_active;
		if( (data->acc != NULL) && currency_is_euro(data->acc->kcur) )
			visible = FALSE;
		hb_widget_visible(data->MI_conveuro, visible);

		//move up/down button : not when showall and when sort is not date
		visible = FALSE;
		psensitive = FALSE;
		nsensitive = FALSE;
		if( count == 1 && data->showall == FALSE )
		{
		Transaction *prevtxn, *nexttxn;

			visible = TRUE;
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), &sort_column_id, &order);
			if( (data->showall == TRUE) || (sort_column_id != LST_DSPOPE_DATE) )
				visible = FALSE;

			prevtxn = NULL; nexttxn = NULL;
			Transaction *txn = list_txn_get_surround_transaction(GTK_TREE_VIEW(data->LV_ope), &prevtxn, &nexttxn);

			if( prevtxn && txn )
			{
				psensitive = (prevtxn->date == txn->date) ? TRUE : FALSE;
			}

			if( nexttxn && txn )
			{
				nsensitive = (nexttxn->date == txn->date) ? TRUE : FALSE;
			}
		}

		hb_widget_visible(data->SP_updown, visible);
		hb_widget_visible(data->BT_up, visible);
		hb_widget_visible(data->BT_down, visible);
		hb_widget_visible(data->MI_poptxnup, visible);
		hb_widget_visible(data->MI_poptxndown, visible);
		gtk_widget_set_sensitive(data->MI_poptxnup, psensitive);
		gtk_widget_set_sensitive(data->BT_up, psensitive);
		gtk_widget_set_sensitive(data->MI_poptxndown, nsensitive);
		gtk_widget_set_sensitive(data->BT_down, nsensitive);
	}

	//#1835588
	visible = PREFS->date_future_nbdays > 0 ? TRUE : FALSE;
	if( !(filter_preset_daterange_future_enable( hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_range)) )) )
		visible = FALSE;
	hb_widget_visible(data->CM_future, visible);
	DB( g_print(" - show future=%d\n", visible) );

	
	/* update fltinfo */
	DB( g_print(" - FLG_REG_INFOBAR\n") );


	DB( g_print(" - statusbar\n") );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
	count = gtk_tree_selection_count_selected_rows(selection);
	DB( g_print(" - nb selected = %d\n", count) );

	/* if more than one ope selected, we make a sum to display to the user */
	gdouble opeexp = 0.0;
	gdouble opeinc = 0.0;
	gchar buf1[64];
	gchar buf2[64];
	gchar buf3[64];
	gchar fbufavg[64];
	guint32 kcur;

	kcur = (data->showall == TRUE) ? GLOBALS->kcur : data->acc->kcur;


	if( count >= 1 )
	{
	GList *list, *tmplist;
	GtkTreeModel *model;
	GtkTreeIter iter;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
		list = gtk_tree_selection_get_selected_rows(selection, &model);
		tmplist = g_list_first(list);
		while (tmplist != NULL)
		{
		Transaction *item;

			gtk_tree_model_get_iter(model, &iter, tmplist->data);
			gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &item, -1);

			if( data->showall == FALSE )
			{
				if( item->flags & OF_INCOME )
					opeinc += item->amount;
				else
					opeexp += item->amount;
			}
			else
			{
				if( item->flags & OF_INCOME )
					opeinc += hb_amount_base(item->amount, item->kcur);
				else
					opeexp += hb_amount_base(item->amount, item->kcur);
			}

			DB( g_print(" - %s, %.2f\n", item->memo, item->amount ) );

			tmplist = g_list_next(tmplist);
		}
		g_list_free(list);

		DB( g_print(" %f - %f = %f\n", opeinc, opeexp, opeinc + opeexp) );


		hb_strfmon(buf1, 64-1, opeinc, kcur, GLOBALS->minor);
		hb_strfmon(buf2, 64-1, -opeexp, kcur, GLOBALS->minor);
		hb_strfmon(buf3, 64-1, opeinc + opeexp, kcur, GLOBALS->minor);
		hb_strfmon(fbufavg, 64-1, (opeinc + opeexp) / count, kcur, GLOBALS->minor);
	}

	gchar *msg;

	if( count <= 1 )
	{
		msg = g_strdup_printf(_("%d transactions"), data->total);
	}
	else
		msg = g_strdup_printf(_("%d transactions, %d selected, avg: %s, sum: %s (%s - %s)"), data->total, count, fbufavg, buf3, buf1, buf2);

	gtk_label_set_markup(GTK_LABEL(data->TX_selection), msg);
	g_free (msg);

	//5.6 update lock/unlock
	lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));
	DB( g_print(" lockrecon=%d\n", lockrecon) );

	list_txn_set_lockreconciled(GTK_TREE_VIEW(data->LV_ope), lockrecon);
	gtk_image_set_from_icon_name(GTK_IMAGE(data->IM_lockreconciled), lockrecon == TRUE ? ICONNAME_CHANGES_PREVENT : ICONNAME_CHANGES_ALLOW, GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text (data->SW_lockreconciled, 
		lockrecon == TRUE ? _("Locked. Click to unlock") : _("Unlocked. Click to lock"));
	gtk_widget_queue_draw (data->LV_ope);

}


static void register_panel_onRowActivated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
struct register_panel_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
gint col_id, count;
Transaction *ope;
gboolean lockrecon;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	//5.3.1 if closed account : disable any change
	if( (data->showall == FALSE) && (data->acc->flags & AF_CLOSED) )
		return;

	col_id = gtk_tree_view_column_get_sort_column_id (col);
	count  = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(treeview));
	model  = gtk_tree_view_get_model(treeview);

	//get transaction double clicked to initiate the widget
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

	DB( g_print ("%d rows been double-clicked on column=%d! ope=%s\n", count, col_id, ope->memo) );

	if( count == 1)
	{
		//1909749 lock/unlock reconciled	
		lockrecon = FALSE;
		if( (ope->status == TXN_STATUS_RECONCILED) && (gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled)) == TRUE) )
			lockrecon = TRUE;

		if( lockrecon == FALSE )
		{
			register_panel_action(GTK_WIDGET(treeview), GINT_TO_POINTER(ACTION_ACCOUNT_EDIT));
		}
	}
	else
	{
		if( data->showall == FALSE )
		{
			if(col_id >= LST_DSPOPE_DATE && col_id != LST_DSPOPE_BALANCE)
			{
				register_panel_edit_multiple (data->window, ope, col_id, data);
			}
		}
	}
}


/*
** populate the account window
*/
void register_panel_window_init(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[register] init window\n") );

	if( data->showall == TRUE )
	{
		//gtk_label_set_text (GTK_LABEL(data->LB_name), _("All transactions"));
		//hb_widget_visible (data->IM_closed, FALSE);
	}
	else
	{
		//gtk_label_set_text (GTK_LABEL(data->LB_name), data->acc->name);
		//hb_widget_visible (data->IM_closed, (data->acc->flags & AF_CLOSED) ? TRUE : FALSE);

		DB( g_print(" - sort transactions\n") );
		da_transaction_queue_sort(data->acc->txn_queue);
	}

	list_txn_set_column_acc_visible(GTK_TREE_VIEW(data->LV_ope), data->showall);

	if( (data->showall == FALSE) && !(data->acc->flags & AF_NOBUDGET) )
		list_txn_set_warn_nocategory(GTK_TREE_VIEW(data->LV_ope), TRUE);

	//DB( g_print(" mindate=%d, maxdate=%d %x\n", data->filter->mindate,data->filter->maxdate) );

	DB( g_print(" - set range or populate+update sensitive+balance\n") );
	
	register_panel_cb_filter_reset(widget, user_data);

	DB( g_print(" - call update visual\n") );
	register_panel_update(widget, GINT_TO_POINTER(FLG_REG_VISUAL|FLG_REG_SENSITIVE));

}

/*
**
*/
static gboolean 
register_panel_getgeometry(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
//struct register_panel_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[register] get geometry\n") );

	//store position and size
	wg = &PREFS->acc_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(widget));
	GdkWindowState state = gdk_window_get_state(gdk_window);
	wg->s = (state & GDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
	
	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d s=%d, state=%d\n", wg->l, wg->t, wg->w, wg->h, wg->s, state & GDK_WINDOW_STATE_MAXIMIZED) );

	return FALSE;
}


static gboolean register_panel_cb_search_focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[register] search focus-in event\n") );

	
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	return FALSE;
}


//beta
static gboolean register_panel_cb_focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[register] focus-in event\n") );


	if( gtk_widget_is_visible (data->window) )
	{
		DB( g_print("- window is visible\n") );

		//register_panel_collect_filtered_txn(data->LV_ope);
		//register_panel_listview_populate(data->LV_ope);
		//register_panel_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));
	}


	return FALSE;
}


/*
**
*/
static gboolean register_panel_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct register_panel_data *data = user_data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print("\n[register] delete-event\n") );

	register_panel_getgeometry(data->window, NULL, data);

	return FALSE;
}


/* Another callback */
static gboolean register_panel_destroy( GtkWidget *widget,
										 gpointer	 user_data )
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");


	DB( g_print ("\n[register] destroy event occurred\n") );



	//enable define windows
	GLOBALS->define_off--;

	/* unset transaction edit mutex */
	if(data->showall == FALSE)
		data->acc->window = NULL;
	else
		GLOBALS->alltxnwindow = NULL;

	/* free title and filter */
	DB( g_print(" user_data=%p to be free\n", user_data) );
	g_free(data->wintitle);

	if(data->gpatxn != NULL)
		g_ptr_array_free (data->gpatxn, TRUE);

	g_queue_free_full(data->q_txn_clip, (GDestroyNotify)da_transaction_free);
	da_flt_free(data->filter);

	g_free(data);


	//our global list has changed, so update the treeview
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));

	return FALSE;
}


static void quick_search_text_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_listview_populate (data->window);
}


static gint listview_context_cb (GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#1993088 register closed account popmenu should be disabled
	if( data->acc != NULL )
	{
		if( data->acc->flags & AF_CLOSED )
			goto end;
	}
	
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{

		// check we are not in the header but in bin window
		if (event->window == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)))
		{
			#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
				gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
			#else
				gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
			#endif
				// On indique à l'appelant que l'on a géré cet événement.

				return TRUE;
		}

		// On indique à l'appelant que l'on n'a pas géré cet événement.
	}
end:
	return FALSE;
}


static GtkWidget *
register_panel_popmenu_create(struct register_panel_data *data)
{
GtkWidget *menu, *submenu;
GtkWidget *menuitem;
//GtkAccelGroup *accel_group = NULL;

	//accel_group = gtk_accel_group_new();
	//gtk_window_add_accel_group(GTK_WINDOW(data->window), accel_group);
	
	menu = gtk_menu_new();
	data->ME_popmenu = menu;

	menuitem = hbtk_menu_add_menuitem (menu,  _("_Add...") );
	data->MI_popadd = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("_Inherit..."));
	data->MI_popherit = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("_Edit...") );
	data->MI_popedit = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("_Multiple Edit..."));
	data->MI_popmultiedit = menuitem;
	
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
	
	submenu = hbtk_menubar_add_menu(menu, _("_Status"), &data->ME_popmenustatus);
	
	menuitem = hbtk_menu_add_menuitem (submenu, _("_None"));
	data->MI_popstatnone = menuitem;
	menuitem = hbtk_menu_add_menuitem (submenu, _("_Cleared"));
	data->MI_popstatclear = menuitem;
	menuitem = hbtk_menu_add_menuitem (submenu, _("_Reconciled"));
	data->MI_popstatrecon = menuitem;

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());

	menuitem = hbtk_menu_add_menuitem (menu, _("View _Split"));
	data->MI_popviewsplit = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("Copy raw amount"));
	data->MI_popcopyamount = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("Create template..."));
	data->MI_poptemplate = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("Create assignment..."));
	data->MI_popassign = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("_Delete...")	);
	data->MI_popdelete = menuitem;

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());

	menuitem = hbtk_menu_add_menuitem (menu, _("_Up"));
	data->MI_poptxnup = menuitem;
	menuitem = hbtk_menu_add_menuitem (menu, _("_Down"));
	data->MI_poptxndown = menuitem;

	gtk_widget_show_all(menu);

	g_signal_connect (data->MI_popadd       , "activate", G_CALLBACK (register_panel_action_add), (gpointer)data);
	g_signal_connect (data->MI_popherit     , "activate", G_CALLBACK (register_panel_action_inherit), (gpointer)data);
	g_signal_connect (data->MI_popedit      , "activate", G_CALLBACK (register_panel_action_edit), (gpointer)data);
	g_signal_connect (data->MI_popmultiedit , "activate", G_CALLBACK (register_panel_action_multiedit), (gpointer)data);

	g_signal_connect (data->MI_popstatnone  , "activate", G_CALLBACK (register_panel_action_none), (gpointer)data);
	g_signal_connect (data->MI_popstatclear , "activate", G_CALLBACK (register_panel_action_clear), (gpointer)data);
	g_signal_connect (data->MI_popstatrecon , "activate", G_CALLBACK (register_panel_action_reconcile), (gpointer)data);

	g_signal_connect (data->MI_popviewsplit , "activate", G_CALLBACK (register_panel_action_viewsplit), (gpointer)data);
	g_signal_connect (data->MI_popcopyamount, "activate", G_CALLBACK (register_panel_action_copyrawamount), (gpointer)data);
	g_signal_connect (data->MI_poptemplate  , "activate", G_CALLBACK (register_panel_action_createtemplate), (gpointer)data);
	g_signal_connect (data->MI_popassign    , "activate", G_CALLBACK (register_panel_action_createassignment), (gpointer)data);
	g_signal_connect (data->MI_popdelete    , "activate", G_CALLBACK (register_panel_action_delete), (gpointer)data);

	g_signal_connect (data->MI_poptxnup     , "activate", G_CALLBACK (register_panel_move_up), (gpointer)data);
	g_signal_connect (data->MI_poptxndown   , "activate", G_CALLBACK (register_panel_move_down), (gpointer)data);

	return menu;
}


static GtkWidget *
register_panel_menubar_create(struct register_panel_data *data)
{
GtkWidget *menubar;
GtkWidget *menu, *submenu;
GtkWidget *menuitem;
GtkAccelGroup *accel_group = NULL;

	menubar = gtk_menu_bar_new ();

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(data->window), accel_group);

	menu = hbtk_menubar_add_menu(menubar, _("A_ccount"), &data->ME_menuacc);

		data->MI_exportqif = hbtk_menu_add_menuitem(menu, _("Export QIF...") );
		data->MI_exportcsv = hbtk_menu_add_menuitem(menu, _("Export CSV...") );
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_exportpdf = hbtk_menu_add_menuitem(menu, _("Export as PDF...") );
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_print = menuitem = hbtk_menu_add_menuitem(menu, _("Print...") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		data->MI_close = menuitem = hbtk_menu_add_menuitem(menu, _("_Close") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 

	//#1818052 wish: copy/paste one/multiple transaction(s)
	menu = hbtk_menubar_add_menu(menubar, _("_Edit"), &data->ME_menuedit);

		data->MI_copy = menuitem = hbtk_menu_add_menuitem(menu, _("Copy") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
		data->MI_pasten = menuitem = hbtk_menu_add_menuitem(menu, _("Paste") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
		data->MI_pastet = menuitem = hbtk_menu_add_menuitem(menu, _("Paste (today)") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_v, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE); 
	
	menu = hbtk_menubar_add_menu(menubar, _("Transacti_on"), &data->ME_menutxn);

		data->MI_add   = menuitem = hbtk_menu_add_menuitem(menu, _("_Add...") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		data->MI_herit = menuitem = hbtk_menu_add_menuitem(menu, _("_Inherit...") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_u, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		data->MI_edit  = menuitem = hbtk_menu_add_menuitem(menu,  _("_Edit...") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());

		submenu = hbtk_menubar_add_menu(menu, _("_Status"), &data->ME_menustatus);

			data->MI_statnone  = menuitem = hbtk_menu_add_menuitem(submenu, _("_None") );
			gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_c, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
			data->MI_statclear = menuitem = hbtk_menu_add_menuitem(submenu, _("_Cleared") );
			gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_r, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
			data->MI_statrecon = menuitem = hbtk_menu_add_menuitem(submenu, _("_Reconciled") );
			gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

		data->MI_multiedit = hbtk_menu_add_menuitem(menu, _("_Multiple Edit...") );
		data->MI_assign    = hbtk_menu_add_menuitem(menu, _("Create assignment..."));
		data->MI_template  = hbtk_menu_add_menuitem(menu, _("Create template...") );
		data->MI_delete    = menuitem = hbtk_menu_add_menuitem(menu, _("_Delete...") );
		gtk_widget_add_accelerator (menuitem, "activate", accel_group, GDK_KEY_Delete, (GdkModifierType)0, GTK_ACCEL_VISIBLE);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());

		data->MI_find    = menuitem = hbtk_menu_add_menuitem(menu, _("_Find") );
		gtk_widget_add_accelerator (menuitem, "activate", accel_group, GDK_KEY_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	
	menu = hbtk_menubar_add_menu(menubar, _("_Tools"), &data->ME_menutools);

		data->MI_markdup    = hbtk_menu_add_menuitem(menu, _("Mark duplicate...") );
		data->MI_chkintxfer = hbtk_menu_add_menuitem(menu, _("Check internal transfer") );
		data->MI_autoassign = hbtk_menu_add_menuitem(menu, _("Auto. assignments") );
		data->MI_filter     = hbtk_menu_add_menuitem(menu, _("_Filter...") );
		data->MI_conveuro   = hbtk_menu_add_menuitem(menu, _("Convert to Euro...") );

	return menubar;
}


static GtkWidget *
register_panel_toolbar_create(struct register_panel_data *data)
{
GtkWidget *toolbar, *button, *hbox, *widget, *label;
GtkToolItem *toolitem;

	toolbar = gtk_toolbar_new();
	
	data->BT_add   = button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_ADD, _("Add"), _("Add a new transaction"));
	g_object_set(button, "is-important", TRUE, NULL);
	data->BT_herit = button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_HERIT, _("Inherit"), _("Inherit from the active transaction"));
	g_object_set(button, "is-important", TRUE, NULL);
	data->BT_edit  = button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_EDIT,  _("Edit"), _("Edit the active transaction"));
	g_object_set(button, "is-important", TRUE, NULL);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	
	data->BT_clear     = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_CLEARED, _("Cleared"), _("Toggle cleared for selected transaction(s)"));
	data->BT_reconcile = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_RECONCILED, _("Reconciled"), _("Toggle reconciled for selected transaction(s)"));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	
	data->BT_multiedit = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_MULTIEDIT, _("Edit Multiple"), _("Edit multiple transaction"));
	data->BT_template  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_CONVERT, _("Create template"), _("Create template"));
	data->BT_delete    = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_DELETE, _("Delete"),  _("Delete selected transaction(s)"));

	toolitem = gtk_separator_tool_item_new();
	data->SP_updown = GTK_WIDGET(toolitem);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	data->BT_up   = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_MOVUP, _("Up"), _("Move transaction up"));
	data->BT_down = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_MOVDW, _("Down"), _("Move transaction down"));


	//#1909749 lock/unlock reconciled
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_widget_set_margin_start (hbox, SPACING_LARGE);

	label = gtk_label_new (_("Reconciled changes is"));
	data->LB_lockreconciled = label;
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	widget = gtk_image_new_from_icon_name (ICONNAME_CHANGES_PREVENT, GTK_ICON_SIZE_BUTTON);
	data->IM_lockreconciled = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = gtk_switch_new();
	data->SW_lockreconciled = widget;
	gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), hbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	return toolbar;
}


/*
 * if accnum = 0 or acc is null : show all account 
 */
GtkWidget *register_panel_window_new(Account *acc)
{
struct register_panel_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainbox, *intbox, *menubar, *table, *sw, *bar;
GtkWidget *treeview, *label, *widget, *image;
gint row;

	DB( g_print("\n[register] create_register_panel_window\n") );

	data = g_malloc0(sizeof(struct register_panel_data));
	if(!data) return NULL;

	//disable define windows
	GLOBALS->define_off++;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	/* setup TODO: to moove */
	data->filter = da_flt_malloc();
	data->q_txn_clip = g_queue_new ();

	
	/* create window, etc */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->window = window;

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	data->acc = acc;
	data->showall = (acc != NULL) ? FALSE : TRUE;
	
	if(data->showall == FALSE)
	{
		data->acc->window = GTK_WINDOW(window);
		if( data->acc->flags & AF_CLOSED )
			data->wintitle = g_strdup_printf("%s %s - HomeBank", data->acc->name, _("(closed)"));
		else
			data->wintitle = g_strdup_printf("%s - HomeBank", data->acc->name);
	}
	else
	{
		GLOBALS->alltxnwindow = window;
		data->wintitle = g_strdup_printf(_("%s - HomeBank"), _("All transactions"));
	}

	gtk_window_set_title (GTK_WINDOW (window), data->wintitle);

	// connect our dispose function
	g_signal_connect (window, "delete-event",
		G_CALLBACK (register_panel_dispose), (gpointer)data);

	// connect our dispose function
	g_signal_connect (window, "destroy",
		G_CALLBACK (register_panel_destroy), (gpointer)data);
		
	g_signal_connect (window, "key-press-event", G_CALLBACK (register_panel_cb_on_key_press), (gpointer)data);

	// connect our dispose function
	//g_signal_connect (window, "configure-event",
	//	G_CALLBACK (register_panel_getgeometry), (gpointer)data);

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	//gtk_container_set_border_width(GTK_CONTAINER(mainbox), SPACING_SMALL);
	gtk_container_add (GTK_CONTAINER (window), mainbox);

	// new menubar
	menubar = register_panel_menubar_create(data);
	gtk_box_pack_start (GTK_BOX (mainbox), menubar, FALSE, FALSE, 0);	


	
	// info bar for duplicate
	bar = gtk_info_bar_new_with_buttons (_("_Refresh"), HB_RESPONSE_REFRESH, NULL);
	data->IB_duplicate = bar;
	gtk_box_pack_start (GTK_BOX (mainbox), bar, FALSE, FALSE, 0);

	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (bar), TRUE);
	label = gtk_label_new (NULL);
	data->LB_duplicate = label;
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0);
      gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

		widget = make_numeric(NULL, 0, HB_DATE_MAX_GAP);
		data->NB_txn_daygap = widget;
		gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), widget, FALSE, FALSE, 0);

	// windows interior
	intbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(intbox), SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (mainbox), intbox, TRUE, TRUE, 0);

	table = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	gtk_widget_set_margin_top(table, SPACING_SMALL);
	gtk_widget_set_margin_bottom(table, SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (intbox), table, FALSE, FALSE, 0);

	row = 0;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID(table), label, row, 0, 1, 1);
	row++;
	data->CY_range = make_daterange(label, DATE_RANGE_CUSTOM_SHOW);
	gtk_grid_attach (GTK_GRID(table), data->CY_range, row, 0, 1, 1);

	row++;
	widget = gtk_toggle_button_new();
	image = gtk_image_new_from_icon_name (ICONNAME_HB_OPE_FUTURE, GTK_ICON_SIZE_MENU);
	g_object_set (widget, "image", image,  NULL);
	gtk_widget_set_tooltip_text (widget, _("Toggle show future transaction"));
	data->CM_future = widget;
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);

	row++;
	label = make_label_widget(_("_Type:"));
	gtk_grid_attach (GTK_GRID(table), label, row, 0, 1, 1);
	row++;
	data->CY_type = make_cycle(label, CYA_FLT_TYPE);
	gtk_grid_attach (GTK_GRID(table), data->CY_type, row, 0, 1, 1);

	row++;
	label = make_label_widget(_("_Status:"));
	gtk_grid_attach (GTK_GRID(table), label, row, 0, 1, 1);
	row++;
	data->CY_status = make_cycle(label, CYA_FLT_STATUS);
	gtk_grid_attach (GTK_GRID(table), data->CY_status, row, 0, 1, 1);

	row++;
	widget = make_image_button(ICONNAME_HB_FILTER, _("Open the list filter"));
	data->BT_filter = widget;
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);

	row++;
	widget = make_image_button(ICONNAME_HB_REFRESH, _("Refresh results"));
	data->BT_refresh = widget;
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);

	row++;
	//widget = gtk_button_new_with_mnemonic (_("Reset _filters"));
	widget = gtk_button_new_with_mnemonic (_("_Reset"));
	data->BT_reset = widget;
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);

	row++;
	//TRANSLATORS: this is for Euro specific users, a toggle to display in 'Minor' currency
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);

	// account name (+ balance)
	row++;
	//space
	label = gtk_label_new(NULL);
	gtk_widget_set_hexpand (label, TRUE);
	gtk_grid_attach (GTK_GRID(table), label, row, 0, 1, 1);

	
	//test menubutton
	/*
	widget = gtk_menu_button_new();
	image = gtk_image_new_from_icon_name (ICONNAME_HB_BUTTON_MENU, GTK_ICON_SIZE_MENU);
	g_object_set (widget, "image", image,  NULL);	
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);
	*/

	row++;
	//quick search
	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_grid_attach (GTK_GRID(table), widget, row, 0, 1, 1);


	/* test */
	table = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	//gtk_style_context_add_class (gtk_widget_get_style_context (table), GTK_STYLE_CLASS_FRAME);
	//gtk_style_context_add_class (gtk_widget_get_style_context (table), GTK_STYLE_CLASS_VIEW);
	gtk_box_pack_start (GTK_BOX (intbox), table, FALSE, FALSE, 0);

	// text range
	label = gtk_label_new(NULL);
	data->TX_daterange = label;
	gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL, -1);
	//gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
	//gtk_widget_set_hexpand (label, TRUE);
	gtk_grid_attach (GTK_GRID(table), label, 0, 0, 1, 1);

	// text total/selection
	label = make_label(NULL, 0.0, 0.5);
	//#1930395 text selectable for copy/paste 
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	//gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
	gtk_widget_set_hexpand (label, TRUE);
	data->TX_selection = label;
	g_object_set(label, "margin", SPACING_TINY, NULL);
	gtk_grid_attach (GTK_GRID(table), label, 1, 0, 1, 1);


	// account name

	label = gtk_label_new(_("Reconciled:"));
	//data->LB_recon = label;
	gtk_grid_attach (GTK_GRID(table), label, 3, 0, 1, 1);
	widget = gtk_label_new(NULL);
	data->TX_balance[0] = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 4, 0, 1, 1);

	label = gtk_label_new(_("Cleared:"));
	//data->LB_clear = label;
	gtk_grid_attach (GTK_GRID(table), label, 5, 0, 1, 1);
	widget = gtk_label_new(NULL);
	data->TX_balance[1] = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 6, 0, 1, 1);

	label = gtk_label_new(_("Today:"));
	//data->LB_today = label;
	gtk_grid_attach (GTK_GRID(table), label, 7, 0, 1, 1);
	widget = gtk_label_new(NULL);
	data->TX_balance[2] = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 8, 0, 1, 1);

	label = gtk_label_new(_("Future:"));
	//data->LB_futur = label;
	gtk_grid_attach (GTK_GRID(table), label, 9, 0, 1, 1);
	widget = gtk_label_new(NULL);
	data->TX_balance[3] = widget;
	g_object_set(widget, "margin", SPACING_TINY, NULL);
	gtk_grid_attach (GTK_GRID(table), widget, 10, 0, 1, 1);


	
	//list
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	treeview = (GtkWidget *)create_list_transaction(LIST_TXN_TYPE_BOOK, PREFS->lst_ope_columns);
	data->LV_ope = treeview;
	gtk_container_add (GTK_CONTAINER (sw), treeview);
	gtk_box_pack_start (GTK_BOX (intbox), sw, TRUE, TRUE, 0);

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);
	

	
	/* toolbars */
	table = gtk_grid_new();
	gtk_box_pack_start (GTK_BOX (intbox), table, FALSE, FALSE, 0);
	
	//test new toolbar
	bar = register_panel_toolbar_create(data);
	data->TB_bar = bar;
	gtk_box_pack_start (GTK_BOX (intbox), bar, FALSE, FALSE, 0);

	
    #ifdef G_OS_WIN32
    if(PREFS->toolbar_style == 0)
    {
        gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
    }
    else
    {
        gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);
    }
    #endif

	//TODO should move this
	//setup
	//TODO minor data seems no more used
	gtk_switch_set_active(GTK_SWITCH(data->SW_lockreconciled), PREFS->lockreconciled);
	list_txn_set_lockreconciled(GTK_TREE_VIEW(data->LV_ope), PREFS->lockreconciled);
	
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope))), "minor", data->CM_minor);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_future), (PREFS->date_future_nbdays > 0) ? TRUE : FALSE );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor), GLOBALS->minor);
	gtk_widget_grab_focus(GTK_WIDGET(data->LV_ope));

	// connect signals
	//beta
	g_signal_connect (data->window, "focus-in-event", G_CALLBACK (register_panel_cb_focus_in_event), NULL);

	g_signal_connect (data->IB_duplicate    , "response", G_CALLBACK (register_panel_cb_bar_duplicate_response), NULL);

	g_signal_connect (data->MI_exportqif , "activate", G_CALLBACK (register_panel_action_exportqif), (gpointer)data);
	g_signal_connect (data->MI_exportcsv , "activate", G_CALLBACK (register_panel_action_exportcsv), (gpointer)data);
	g_signal_connect (data->MI_exportpdf , "activate", G_CALLBACK (register_panel_action_exportpdf), (gpointer)data);
	g_signal_connect (data->MI_print     , "activate", G_CALLBACK (register_panel_action_exportpdf), (gpointer)data);
	g_signal_connect (data->MI_close     , "activate", G_CALLBACK (register_panel_action_close), (gpointer)data);

	g_signal_connect (data->MI_copy      , "activate", G_CALLBACK (register_panel_action_edit_copy), (gpointer)data);
	g_signal_connect (data->MI_pasten    , "activate", G_CALLBACK (register_panel_action_edit_paste), (gpointer)data);
	g_signal_connect (data->MI_pastet    , "activate", G_CALLBACK (register_panel_action_edit_paste), (gpointer)data);
	
	g_signal_connect (data->MI_add       , "activate", G_CALLBACK (register_panel_action_add), (gpointer)data);
	g_signal_connect (data->MI_herit     , "activate", G_CALLBACK (register_panel_action_inherit), (gpointer)data);
	g_signal_connect (data->MI_edit      , "activate", G_CALLBACK (register_panel_action_edit), (gpointer)data);
	g_signal_connect (data->MI_statnone  , "activate", G_CALLBACK (register_panel_action_none), (gpointer)data);
	g_signal_connect (data->MI_statclear , "activate", G_CALLBACK (register_panel_action_clear), (gpointer)data);
	g_signal_connect (data->MI_statrecon , "activate", G_CALLBACK (register_panel_action_reconcile), (gpointer)data);
	g_signal_connect (data->MI_multiedit , "activate", G_CALLBACK (register_panel_action_multiedit), (gpointer)data);
	g_signal_connect (data->MI_template  , "activate", G_CALLBACK (register_panel_action_createtemplate), (gpointer)data);
	g_signal_connect (data->MI_delete    , "activate", G_CALLBACK (register_panel_action_delete), (gpointer)data);
	g_signal_connect (data->MI_find      , "activate", G_CALLBACK (register_panel_action_find), (gpointer)data);

	g_signal_connect (data->MI_markdup   , "activate", G_CALLBACK (register_panel_action_duplicate_mark), (gpointer)data);
	g_signal_connect (data->MI_chkintxfer, "activate", G_CALLBACK (register_panel_action_check_internal_xfer), (gpointer)data);
	g_signal_connect (data->MI_autoassign, "activate", G_CALLBACK (register_panel_action_assign), (gpointer)data);
	g_signal_connect (data->MI_filter    , "activate", G_CALLBACK (register_panel_action_editfilter), (gpointer)data);
	g_signal_connect (data->MI_conveuro  , "activate", G_CALLBACK (register_panel_action_converttoeuro), (gpointer)data);

	g_signal_connect (data->SW_lockreconciled, "state-set", G_CALLBACK (register_panel_cb_recon_change), NULL);

	data->handler_id[HID_RANGE]	 = g_signal_connect (data->CY_range , "changed", G_CALLBACK (register_panel_cb_filter_daterange), NULL);
	data->handler_id[HID_TYPE]	 = g_signal_connect (data->CY_type	, "changed", G_CALLBACK (register_panel_cb_filter_type), NULL);
	data->handler_id[HID_STATUS] = g_signal_connect (data->CY_status, "changed", G_CALLBACK (register_panel_cb_filter_status), NULL);

	g_signal_connect (data->CM_future, "toggled", G_CALLBACK (register_panel_cb_filter_daterange), NULL);

	g_signal_connect (data->BT_reset  , "clicked", G_CALLBACK (register_panel_cb_filter_reset), NULL);
	g_signal_connect (data->BT_refresh, "clicked", G_CALLBACK (register_panel_cb_filter_refresh), NULL);
	g_signal_connect (data->BT_filter , "clicked", G_CALLBACK (register_panel_action_editfilter), (gpointer)data);
	
	g_signal_connect (data->CM_minor , "toggled", G_CALLBACK (register_panel_toggle_minor), NULL);
	
	data->handler_id[HID_SEARCH] = g_signal_connect (data->ST_search, "search-changed", G_CALLBACK (quick_search_text_changed_cb), data);
	//#1879451 deselect all when quicksearch has the focus (to prevent delete)
	g_signal_connect (data->ST_search, "focus-in-event", G_CALLBACK (register_panel_cb_search_focus_in_event), NULL);
	
	g_signal_connect (G_OBJECT (data->BT_add   ), "clicked", G_CALLBACK (register_panel_action_add), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_herit ), "clicked", G_CALLBACK (register_panel_action_inherit), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_edit  ), "clicked", G_CALLBACK (register_panel_action_edit), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_clear ), "clicked", G_CALLBACK (register_panel_action_clear), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_reconcile), "clicked", G_CALLBACK (register_panel_action_reconcile), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_multiedit), "clicked", G_CALLBACK (register_panel_action_multiedit), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_template ), "clicked", G_CALLBACK (register_panel_action_createtemplate), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_delete), "clicked", G_CALLBACK (register_panel_action_delete), (gpointer)data);

	g_signal_connect (G_OBJECT (data->BT_up), "clicked", G_CALLBACK (register_panel_move_up), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_down), "clicked", G_CALLBACK (register_panel_move_down), (gpointer)data);
	
	//g_signal_connect (GTK_TREE_VIEW(treeview), "cursor-changed", G_CALLBACK (register_panel_update), (gpointer)2);
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK (register_panel_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(treeview), "row-activated", G_CALLBACK (register_panel_onRowActivated), GINT_TO_POINTER(2));

	GtkWidget *popmenu = register_panel_popmenu_create(data);
	
	//todo: debug test
	g_signal_connect (treeview, "button-press-event", G_CALLBACK (listview_context_cb),
		// todo: here is not a GtkMenu but GtkImageMenuItem...
		popmenu
		//gtk_ui_manager_get_widget (ui, "/MenuBar")
	);

	//setup, init and show window
	wg = &PREFS->acc_wg;
	if(wg->s == 0)
	{
		if( wg->l && wg->t )
			gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
		gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);
	}
	else
		gtk_window_maximize(GTK_WINDOW(window));
	
	gtk_widget_show_all (window);
	
	gtk_widget_hide(data->IB_duplicate);


	return window;
}

