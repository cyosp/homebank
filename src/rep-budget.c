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


#include "homebank.h"

#include "rep-budget.h"

#include "list-operation.h"
#include "gtk-chart-progress.h"
#include "gtk-dateentry.h"

#include "dsp-mainwindow.h"
#include "ui-transaction.h"


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


/* prototypes */
static void repbudget_compute(GtkWidget *widget, gpointer user_data);
static void repbudget_sensitive(GtkWidget *widget, gpointer user_data);
static void repbudget_update_daterange(GtkWidget *widget, gpointer user_data);
static void repbudget_update_chart(GtkWidget *widget, gpointer user_data);
static void repbudget_update_detail(GtkWidget *widget, gpointer user_data);
static void repbudget_detail(GtkWidget *widget, gpointer user_data);

static GtkWidget *lst_repbud_create(void);
static GString *lst_repbud_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard);


extern gchar *CYA_CATSUBCAT[];
extern gchar *CYA_KIND[];


/* action functions -------------------- */
static void repbudget_action_viewlist(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbudget_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	repbudget_sensitive(data->window, NULL);
}

static void repbudget_action_viewstack(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbudget_data *data = user_data;

	//#1860905 we redraw chart in case a sort changed
	repbudget_update_chart(data->window, NULL);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	repbudget_sensitive(data->window, NULL);
}

static void repbudget_action_print(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbudget_data *data = user_data;
gint tmpfor;
gchar *name;

	tmpfor  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));
	name = g_strdup_printf("hb-repbudget_%s", CYA_CATSUBCAT[tmpfor]);

	gtk_chart_progress_print(GTK_CHARTPROGRESS(data->RE_progress), GTK_WINDOW(data->window), PREFS->path_export, name);

	g_free(name);
}


/* ======================== */


static gint getmonth(guint date)
{
GDate *date1;
gint month;

	date1 = g_date_new_julian(date);
	month = g_date_get_month(date1);

	/*#if MYDEBUG == 1
		g_print("\n[repbudget] getmonth\n");
		gchar buffer1[128];
		g_date_strftime (buffer1, 128-1, "%x", date1);
		g_print("  date is '%s', month=%d\n", buffer1, month);
	#endif*/

	g_date_free(date1);

	return(month);
}

static gint countmonth(guint32 mindate, guint32 maxdate)
{
GDate *date1, *date2;
gint nbmonth;

	date1 = g_date_new_julian(mindate);
	date2 = g_date_new_julian(maxdate);

	nbmonth = 0;
	while(g_date_compare(date1, date2) < 0)
	{
		nbmonth++;
		g_date_add_months(date1, 1);
	}

	g_date_free(date2);
	g_date_free(date1);

	return(nbmonth);
}

static gdouble budget_compute_result(gdouble budget, gdouble spent)
{
gdouble retval;

	//original formula
	//result = ABS(budget) - ABS(spent);

	retval = spent - budget;

	return retval;
}





static void repbudget_date_change(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	DB( g_print("\n[repbudget] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	gtk_date_entry_set_maxdate(GTK_DATE_ENTRY(data->PO_mindate), data->filter->maxdate);
	gtk_date_entry_set_mindate(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->mindate);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPBUDGET_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPBUDGET_RANGE]);


	repbudget_compute(widget, NULL);
	repbudget_update_daterange(widget, NULL);

}


static void repbudget_range_change(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gint range;

	DB( g_print("\n[repbudget] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_range));
	

	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);

		g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPBUDGET_MINDATE]);
		g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPBUDGET_MAXDATE]);
		
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);
		
		g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPBUDGET_MINDATE]);
		g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPBUDGET_MAXDATE]);

		repbudget_compute(widget, NULL);
		repbudget_update_daterange(widget, NULL);
	}
}


static void repbudget_update_daterange(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gchar *daterange;

	DB( g_print("\n[repbudget] update daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	daterange = filter_daterange_text_get(data->filter);
	gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
	g_free(daterange);
}


static void repbudget_toggle_detail(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] toggle detail\n") );

	data->detail ^= 1;

	repbudget_update_detail(widget, user_data);

}


static void repbudget_detail_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct repbudget_data *data;
Transaction *active_txn;
gboolean result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print ("\n[repbudget] A detail row has been double-clicked!\n") );

	active_txn = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_detail));
	if(active_txn)
	{
	Transaction *old_txn, *new_txn;

		//#1909749 skip reconciled if lock is ON
		if( PREFS->lockreconciled == TRUE && active_txn->status == TXN_STATUS_RECONCILED )
			return;

		old_txn = da_transaction_clone (active_txn);
		new_txn = active_txn;
		result = deftransaction_external_edit(GTK_WINDOW(data->window), old_txn, new_txn);

		if(result == GTK_RESPONSE_ACCEPT)
		{
		GtkTreeSelection *treeselection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreePath *path = NULL;

			//1936806 keep the selection
			treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->LV_report));
			if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
			{
				path = gtk_tree_model_get_path(model, &iter);
			}

			//#1640885
			GLOBALS->changes_count++;
			repbudget_compute(data->window, NULL);

			if( path != NULL )
			{
				gtk_tree_selection_select_path(treeselection, path);
				gtk_tree_path_free(path);
			}

		}

		da_transaction_free (old_txn);
	}
}


static void repbudget_update_detail(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] update detail\n") );

	if(GTK_IS_TREE_VIEW(data->LV_report))
	{
		if(data->detail)
		{
		GtkTreeSelection *treeselection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		guint key;

			treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->LV_report));

			if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
			{
				gtk_tree_model_get(model, &iter, LST_BUDGET_KEY, &key, -1);

				//DB( g_print(" - active is %d\n", pos) );

				repbudget_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
			}


			gtk_widget_show(data->GR_detail);
		}
		else
			gtk_widget_hide(data->GR_detail);

		
	}
}


static void repbudget_export_result_clipboard(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
GtkClipboard *clipboard;
GString *node;
gint tmpfor;
gchar *title;

	DB( g_print("\n[repbudget] export result clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpfor     = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));
	title = _(CYA_CATSUBCAT[tmpfor]);
	
	node = lst_repbud_to_string(GTK_TREE_VIEW(data->LV_report), title, TRUE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void repbudget_export_result_csv(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gchar *filename = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpfor;
gchar *title;

	DB( g_print("\n[repbudget] export result csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpfor  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));
	name = g_strdup_printf("hb-repbudget_%s.csv", CYA_CATSUBCAT[tmpfor]);

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );
		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{
			title = _(CYA_CATSUBCAT[tmpfor]);
			node = lst_repbud_to_string(GTK_TREE_VIEW(data->LV_report), title, FALSE);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
		g_free( filename );
	}
	g_free(name);
}


static void repbudget_export_detail_clipboard(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
GtkClipboard *clipboard;
GString *node;

	DB( g_print("\n[repbudget] export detail clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), TRUE, FALSE, TRUE, FALSE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void repbudget_export_detail_csv(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gchar *filename = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpfor;

	DB( g_print("\n[repbudget] export detail csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpfor  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));
	name = g_strdup_printf("hb-repbudget-detail_%s.csv", CYA_CATSUBCAT[tmpfor]);

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{
			node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), FALSE, FALSE, TRUE, FALSE);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);

			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}

		g_free( filename );
	}

	g_free(name);
}




static void repbudget_detail(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
guint active = GPOINTER_TO_INT(user_data);
GList *list;
guint tmpfor;
GtkTreeModel *model;
GtkTreeIter  iter, child;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] detail\n") );

	if(data->detail)
	{
		/* clear and detach our model */
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
		gtk_tree_store_clear (GTK_TREE_STORE(model));
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), NULL); /* Detach model from view */

		tmpfor  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));

		/* fill in the model */
		list = g_queue_peek_head_link(data->txn_queue);
		while (list != NULL)
		{
		Account *acc;
		Transaction *ope = list->data;
		gdouble dtlamt = ope->amount;
		guint i, pos = 0;
		gboolean match = FALSE;

			//DB( g_print(" get %s\n", ope->ope_Word) );

			acc = da_acc_get(ope->kacc);
			if(acc != NULL)
			{
				if((acc->flags & AF_NOBUDGET)) goto next1;
			}

			//filter here
			if( ope->flags & OF_SPLIT )
			{
			guint nbsplit = da_splits_length(ope->splits);
			Split *split;
			guint i;

				dtlamt = 0.0;
				for(i=0;i<nbsplit;i++)
				{
					split = da_splits_get(ope->splits, i);
					pos = category_report_id(split->kcat, (tmpfor == BUDG_SUBCATEGORY) ? TRUE : FALSE);
					if( pos == active )
					{
						match = TRUE; 
						dtlamt += split->amount;
						// no more break here as we need to compute split 4 cat
						//break;
					}
				}
			}
			else
			{
				pos = category_report_id(ope->kcat, (tmpfor == BUDG_SUBCATEGORY) ? TRUE : FALSE);
				if( pos == active )
					match = TRUE;
			}

			//insert
			if( match == TRUE )
			{
		    	gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);
	     		gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
					MODEL_TXN_POINTER, ope,
			        MODEL_TXN_SPLITAMT, dtlamt,
					-1);

				//#1875801 show split detail
				if( ope->flags & OF_SPLIT )
				{
				guint nbsplit = da_splits_length(ope->splits);
				Split *split;

					for(i=0;i<nbsplit;i++)
					{
						split = da_splits_get(ope->splits, i);
						pos = category_report_id(split->kcat, (tmpfor == BUDG_SUBCATEGORY) ? TRUE : FALSE);
						if( pos == active )
						{
							gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &child, &iter, -1,
								MODEL_TXN_POINTER, ope,
								MODEL_TXN_SPLITPTR, split,
								-1);
						}
					}
				}


			}

			
next1:
			list = g_list_next(list);
		}

		/* Re-attach model to view */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), model);
		g_object_unref(model);

		gtk_tree_view_columns_autosize( GTK_TREE_VIEW(data->LV_detail) );

	}

}


static void repbudget_compute_cat_spent(guint32 key, gdouble amount, gdouble *tmp_spent, gdouble *tmp_budget)
{
Category *cat;

	cat = da_cat_get(key);
	if(cat)
	{
		DB( g_print(" cat %02d:%02d (sub=%d), bud=%.2f\n", cat->parent, cat->key, (cat->flags & GF_SUB), tmp_budget[cat->key]) );

		if( (cat->flags & GF_FORCED) || (cat->flags & GF_BUDGET) )
		{
			DB( g_print("  + spend %.2f to cat %d\n", amount, cat->key) );
			tmp_spent[cat->key] += amount;
		}

		//#1825653 subcat without budget must be computed
		if( (cat->flags & GF_SUB) )
		{
		Category *pcat = da_cat_get(cat->parent);

			if(pcat)
			{
				if( (cat->flags & GF_FORCED) || (cat->flags & GF_BUDGET) || (pcat->flags & GF_FORCED) || (pcat->flags & GF_BUDGET) )
				{
					DB( g_print("  + spend %.2f to parent %d\n", amount, cat->parent) );
					tmp_spent[pcat->key] += amount;
				}

			}
		}
	}				
}


static void repbudget_update_total(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	DB( g_print("\n[repbudget] update total\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	hb_label_set_colvalue(GTK_LABEL(data->TX_total[0]), data->total_spent, GLOBALS->kcur, GLOBALS->minor);
	hb_label_set_colvalue(GTK_LABEL(data->TX_total[1]), data->total_budget, GLOBALS->kcur, GLOBALS->minor);
	hb_label_set_colvalue(GTK_LABEL(data->TX_total[2]), budget_compute_result(data->total_budget, data->total_spent), GLOBALS->kcur, GLOBALS->minor);
	

}


static void repbudget_update_chart(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
GtkTreeModel *model;
gint tmpfor;
gchar *title;
	
	DB( g_print("\n[repbudget] update chart\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpfor = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	
	/* update stack chart */
	title = g_strdup_printf(_("Budget for %s"), _(CYA_CATSUBCAT[tmpfor]) );

	ui_chart_progress_set_currency(GTK_CHARTPROGRESS(data->RE_progress), GLOBALS->kcur);

	/* set chart color scheme */
	ui_chart_progress_set_color_scheme(GTK_CHARTPROGRESS(data->RE_progress), PREFS->report_color_scheme);
	ui_chart_progress_set_dualdatas(GTK_CHARTPROGRESS(data->RE_progress), model, _("Budget"), _("Result"), title, NULL);

	g_free(title);
}


static void repbudget_compute(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

gint tmpfor, tmpkind, tmponlyout;
guint32 mindate, maxdate;

GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
guint n_result, id;
gdouble *tmp_spent, *tmp_budget;
gboolean *tmp_hassub;
gint nbmonth = 1;


	DB( g_print("\n[repbudget] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpfor     = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_for));
	tmpkind    = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_kind));
	tmponlyout = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_onlyout));

	mindate = data->filter->mindate;
	maxdate = data->filter->maxdate;
	if(maxdate < mindate) return;

	g_queue_free (data->txn_queue);
	data->txn_queue = hbfile_transaction_get_partial_budget(data->filter->mindate, data->filter->maxdate);

	DB( g_print(" for=%d, kind=%d\n", tmpfor, tmpkind) );

	nbmonth = countmonth(mindate, maxdate);
	DB( g_print(" date: min=%d max=%d nbmonth=%d\n", mindate, maxdate, nbmonth) );

	n_result = da_cat_get_max_key();
	DB( g_print(" nbcat=%d\n", n_result) );

	/* allocate some memory */
	tmp_spent  = g_malloc0((n_result+1) * sizeof(gdouble));
	tmp_budget = g_malloc0((n_result+1) * sizeof(gdouble));
	tmp_hassub = g_malloc0((n_result+1) * sizeof(gboolean));

	if(tmp_spent && tmp_budget && tmp_hassub)
	{
	guint i = 0;
		/* compute the results */
		data->total_spent = 0.0;
		data->total_budget = 0.0;

		/* compute budget for each category */
		DB( g_print("\n+ compute budget\n") );

		//fixed #328034: here <=n_result
		for(i=1; i<=n_result; i++)
		{
		Category *entry;

			entry = da_cat_get(i);
			if( entry == NULL)
				continue;

			DB( g_print(" %d:'%s' issub=%d hasbudget=%d custom=%d\n", 
				entry->key, entry->name, (entry->flags & GF_SUB), (entry->flags & GF_BUDGET), (entry->flags & GF_CUSTOM)) );

			//debug
			/*#if MYDEBUG == 1
			gint k;
			g_print("    budget vector: ");
			for(k=0;k<13;k++)
				g_print( " %d:[%.2f]", k, entry->budget[k]);
			g_print("\n");
			#endif*/

			// same value each month ?
			if(!(entry->flags & GF_CUSTOM))
			{
				DB( g_print("    - monthly %.2f\n", entry->budget[0]) );
				tmp_budget[entry->key] += entry->budget[0]*nbmonth;
				if( entry->flags & GF_SUB )
				{
					tmp_budget[entry->parent] += entry->budget[0]*nbmonth;
					tmp_hassub[entry->parent] = TRUE;
				}
			}
			//otherwise	sum each month from mindate month
			else
			{
			gint month = getmonth(mindate);
			gint j;

				DB( g_print("    - custom each month for %d months\n", nbmonth) );
				for(j=0;j<nbmonth;j++) {
					DB( g_print("      j=%d month=%d budg=%.2f\n", j, month, entry->budget[month]) );
					tmp_budget[entry->key] += entry->budget[month];
					if( entry->flags & GF_SUB )
					{
						tmp_budget[entry->parent] += entry->budget[month];
						tmp_hassub[entry->parent] = TRUE;
					}
					month++;
					if(month > 12) { month = 1; }
				}
			}
		}


		// compute spent for each transaction */
		DB( g_print("\n+ compute spent from transactions\n") );

		list = g_queue_peek_head_link(data->txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;

			DB( g_print("ope: %s :: acc=%d, cat=%d, mnt=%.2f\n", ope->memo, ope->kacc, ope->kcat, ope->amount) );

			if( ope->flags & OF_SPLIT )
			{
			guint nbsplit = da_splits_length(ope->splits);
			Split *split;
			
				for(i=0;i<nbsplit;i++)
				{
					split = da_splits_get(ope->splits, i);
					repbudget_compute_cat_spent(split->kcat, hb_amount_base(split->amount, ope->kcur), tmp_spent, tmp_budget);
				}
			}
			else
			{
				repbudget_compute_cat_spent(ope->kcat, hb_amount_base(ope->amount, ope->kcur), tmp_spent, tmp_budget);
			}

			list = g_list_next(list);
			i++;
		}

		DB( g_print("\nclear and detach model\n") );

		/* clear and detach our model */
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
		gtk_list_store_clear (GTK_LIST_STORE(model));
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL); /* Detach model from view */

		for(i=1, id=0; i<=n_result; i++)
		{
		gchar *name;
		gboolean outofbudget;
		Category *entry;

			entry = da_cat_get(i);
			if( entry == NULL)
				continue;

			name = entry->key == 0 ? "(None)" : entry->fullname;

			//#1553862
			//if( (tmpfor == BUDG_CATEGORY && !(entry->flags & GF_SUB)) || (tmpfor == BUDG_SUBCATEGORY) )
			if( (tmpfor == BUDG_CATEGORY && !(entry->flags & GF_SUB)) || 
			   (tmpfor == BUDG_SUBCATEGORY && ((entry->flags & GF_SUB) || !tmp_hassub[i]) ) )
			{
			guint pos;

				pos = 0;
				switch(tmpfor)
				{
					case BUDG_CATEGORY:
						{
						Category *catentry = da_cat_get(i);
							if(catentry)
								pos = (catentry->flags & GF_SUB) ? catentry->parent : catentry->key;
						}
						break;
					case BUDG_SUBCATEGORY:
						pos = i;
						break;
				}

				// display expense or income (filter on amount and not category hypothetical flag
				if( tmpkind == 1 && tmp_budget[pos] > 0)
					continue;

				if( tmpkind == 2 && tmp_budget[pos] < 0)
					continue;

				//DB( g_print(" eval %d '%s' : spen=%.2f bud=%.2f \n", i, name, tmp_spent[pos], tmp_budget[pos] ) );

				if((entry->flags & (GF_BUDGET|GF_FORCED)) || tmp_budget[pos] /*|| tmp_spent[pos]*/)
				{
				gdouble result, rawrate;
				gchar *status;

					result = budget_compute_result(tmp_budget[pos], tmp_spent[pos]);
					rawrate = 0.0;
					if(ABS(tmp_budget[pos]) > 0)
					{
						rawrate = tmp_spent[pos] / tmp_budget[pos];
					}
					else if(tmp_budget[pos] == 0.0)
						rawrate = ABS(tmp_spent[pos]);

					status = "";
					outofbudget = FALSE;
					if( result )
					{
						if(rawrate > 1.0)
						{
							status = _(" over");
							outofbudget = TRUE;
						}
						else
						{
							if(tmp_budget[pos] < 0.0)
								status = _(" left");
							else if(tmp_budget[pos] > 0.0)
							{
								status = _(" under");
								outofbudget = TRUE;
							}
						}
					}

					if(tmponlyout == TRUE && outofbudget == FALSE)
						goto nextins;

					DB( g_print(" => insert '%s' s:%.2f b:%.2f r:%.2f (%%%.2f) '%s' '%d'\n\n", name, tmp_spent[pos], tmp_budget[pos], result, rawrate, status, outofbudget ) );

					gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			 		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
						LST_BUDGET_POS, id++,
						LST_BUDGET_KEY, pos,
						LST_BUDGET_NAME, name,
						LST_BUDGET_SPENT, tmp_spent[pos],
						LST_BUDGET_BUDGET, tmp_budget[pos],
						LST_BUDGET_RESULT, result,
						LST_BUDGET_STATUS, status,
						-1);

				nextins:
					data->total_spent  += tmp_spent[pos];
					data->total_budget += tmp_budget[pos];
				}
			}
		}

		/* update column 0 title */
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report), 0);
		gtk_tree_view_column_set_title(column, _(CYA_CATSUBCAT[tmpfor]));

		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

		/* Re-attach model to view */
  		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
		g_object_unref(model);


		repbudget_update_total(widget, NULL);

		repbudget_update_chart(widget, NULL);
	}

	//DB( g_print(" inserting %i, %f %f\n", i, total_expense, total_income) );

	/* free our memory */
	g_free(tmp_hassub);
	g_free(tmp_spent);
	g_free(tmp_budget);

}


/*
** update sensitivity
*/
static void repbudget_sensitive(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gboolean visible, sensitive;
gint page;

	DB( g_print("\n[repbudget] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	visible = (page == 0) ? TRUE : FALSE;
	hb_widget_visible (data->BT_detail, visible);
	hb_widget_visible (data->BT_export, visible);
	hb_widget_visible (data->BT_print, !visible);

	page = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail)), NULL);

	sensitive = ((page > 0) && data->detail) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->MI_detailtoclip, sensitive);
	gtk_widget_set_sensitive(data->MI_detailtocsv, sensitive);

}


static void repbudget_toggle(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gboolean minor;

	DB( g_print("\n[repbudget] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	repbudget_update_total(widget, NULL);

	//hbfile_update(data->LV_acc, (gpointer)4);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));
	ui_chart_progress_show_minor(GTK_CHARTPROGRESS(data->RE_progress), minor);

}


static void repbudget_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint key = -1;

	DB( g_print("\n[repbudget] selection\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_BUDGET_KEY, &key, -1);
	}

	DB( g_print(" - active is %d\n", key) );

	repbudget_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	repbudget_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static GtkWidget *
repbudget_toolbar_create(struct repbudget_data *data)
{
GtkWidget *toolbar, *button;

	toolbar = gtk_toolbar_new();

	button = (GtkWidget *)gtk_radio_tool_button_new(NULL);
	data->BT_list = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_LIST, "label", _("List"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as list"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_progress = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_PROGRESS, "label", _("Stack"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as stack bars"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);
	
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	button = gtk_widget_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_OPE_SHOW,
	    "label", _("Detail"),
		"tooltip-text", _("Toggle detail"),
	    "active", PREFS->budg_showdetail,
		NULL);
	data->BT_detail = button;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REFRESH, _("Refresh"), _("Refresh results"));
	data->BT_refresh = button;
	
	GtkWidget *menu, *menuitem;

	menu = gtk_menu_new ();
	//gtk_widget_set_halign (menu, GTK_ALIGN_END);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Result to clipboard"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repbudget_export_result_clipboard), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Result to CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repbudget_export_result_csv), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Detail to clipboard"));
	data->MI_detailtoclip = menuitem;
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repbudget_export_detail_clipboard), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Detail to CSV"));
	data->MI_detailtocsv = menuitem;
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repbudget_export_detail_csv), data);

	gtk_widget_show_all (menu);


	
	button = gtk_menu_button_new();
	data->BT_export = button;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)), GTK_STYLE_CLASS_FLAT);

	//gtk_menu_button_set_direction (GTK_MENU_BUTTON(widget), GTK_ARROW_DOWN);
	//gtk_widget_set_halign (widget, GTK_ALIGN_END);
	GtkWidget *image = gtk_image_new_from_icon_name (ICONNAME_HB_FILE_EXPORT, GTK_ICON_SIZE_LARGE_TOOLBAR);
	g_object_set (button, "image", image, "popup", GTK_MENU(menu),  NULL);

	GtkToolItem *toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), button);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_PRINT, _("Print"), _("Print"));
	data->BT_print = button;

	return toolbar;
}


//reset the filter
static void repbudget_filter_setup(struct repbudget_data *data)
{
	DB( g_print("\n[repbudget] reset filter\n") );

	filter_reset(data->filter);

	/* 3.4 : make int transfer out of stats */
	data->filter->option[FLT_GRP_TYPE] = 2;
	data->filter->type = FLT_TYPE_INTXFER;

	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, 0);
	
	//g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPBUDGET_MINDATE]);
	//g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPBUDGET_MAXDATE]);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);

	//g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPBUDGET_MINDATE]);
	//g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPBUDGET_MAXDATE]);

}


// setup default for our object/memory
static void repbudget_window_setup(struct repbudget_data *data)
{
	DB( g_print("\n[repbudget] setup\n") );

	DB( g_print(" init data\n") );
	
	repbudget_filter_setup(data);


	DB( g_print(" set widgets default\n") );
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor),GLOBALS->minor);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), PREFS->date_range_rep);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail))), "minor", (gpointer)data->CM_minor);


	DB( g_print(" connect widgets signals\n") );


	g_signal_connect (data->CM_onlyout, "toggled", G_CALLBACK (repbudget_compute), NULL);
	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (repbudget_toggle), NULL);


	data->handler_id[HID_REPBUDGET_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repbudget_range_change), NULL);

    g_signal_connect (data->CY_for , "changed", G_CALLBACK (repbudget_compute), (gpointer)data);
    g_signal_connect (data->CY_kind, "changed", G_CALLBACK (repbudget_compute), (gpointer)data);

    data->handler_id[HID_REPBUDGET_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repbudget_date_change), (gpointer)data);
    data->handler_id[HID_REPBUDGET_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repbudget_date_change), (gpointer)data);

	g_signal_connect (G_OBJECT (data->BT_list), "clicked", G_CALLBACK (repbudget_action_viewlist), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_progress), "clicked", G_CALLBACK (repbudget_action_viewstack), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_detail), "clicked", G_CALLBACK (repbudget_toggle_detail), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_refresh), "clicked", G_CALLBACK (repbudget_compute), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_print), "clicked", G_CALLBACK (repbudget_action_print), (gpointer)data);
	//export is a menu


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), "changed", G_CALLBACK (repbudget_selection), NULL);

	g_signal_connect (GTK_TREE_VIEW(data->LV_detail), "row-activated", G_CALLBACK (repbudget_detail_onRowActivated), NULL);

}


static gboolean repbudget_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] window mapped\n") );

	//hb_window_run_pending();
	//hb_window_set_busy(GTK_WINDOW(data->window), TRUE);

	//g_usleep( G_USEC_PER_SEC * 10 );

	//setup, init and show window
	repbudget_window_setup(data);	
	repbudget_compute(data->window, NULL);
	repbudget_update_daterange(data->window, NULL);

	//hb_window_set_busy(GTK_WINDOW(data->window), FALSE);



	//check for any account included into the budget or warn
	{
	guint count =0;
	GList *lacc, *list;

		lacc = list = g_hash_table_get_values(GLOBALS->h_acc);

		while (list != NULL)
		{
		Account *acc;
			acc = list->data;
			//#1674045 ony rely on nosummary
			//if((acc->flags & (AF_CLOSED|AF_NOREPORT))) goto next1;
			if((acc->flags & (AF_NOREPORT))) goto next1;
			if(!(acc->flags & AF_NOBUDGET))
				count++;
		next1:
			list = g_list_next(list);
		}
		g_list_free(lacc);

		if(count <= 0)
		{
			ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_WARNING,
				_("No account is defined to be part of the budget."),
				_("You should include some accounts from the account dialog.")
				);
		}

	
	
	}


	return FALSE;
}


static gboolean repbudget_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudget_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[repbudget] start dispose\n") );

	g_queue_free (data->txn_queue);

	da_flt_free(data->filter);
	
	g_free(data);

	//store position and size
	wg = &PREFS->bud_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

	//enable define windows
	GLOBALS->define_off--;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	//unref window to our open window list
	GLOBALS->openwindows = g_slist_remove(GLOBALS->openwindows, widget);

	DB( g_print("\n[repbudget] end dispose\n") );

	return FALSE;
}


//allocate our object/memory
static void repbudget_window_acquire(struct repbudget_data *data)
{
	DB( g_print("\n[repbudget] acquire\n") );

	data->txn_queue = g_queue_new ();
	data->filter = da_flt_malloc();
	data->detail = PREFS->budg_showdetail;
	data->legend = 1;
}


// the window creation
GtkWidget *repbudget_window_new(void)
{
struct repbudget_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainvbox, *hbox, *vbox, *notebook, *treeview;
GtkWidget *label, *widget, *table, *entry;
gint row;

	DB( g_print("\n[repbudget] new\n") );


	data = g_malloc0(sizeof(struct repbudget_data));
	if(!data) return NULL;

	repbudget_window_acquire(data);

	//disable define windows
	GLOBALS->define_off++;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

    /* create window, etc */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->window = window;
	//ref window to our open window list
	GLOBALS->openwindows = g_slist_prepend(GLOBALS->openwindows, window);

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	gtk_window_set_title (GTK_WINDOW (window), _("Budget report"));

	//window contents
	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (window), mainvbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (mainvbox), hbox, TRUE, TRUE, 0);

	//control part
	table = gtk_grid_new ();
	//			gtk_alignment_new(xalign, yalign, xscale, yscale)
	//alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	//gtk_container_add(GTK_CONTAINER(alignment), table);
    gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (table), SPACING_SMALL);
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0;
	label = make_label_group(_("Display"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	label = make_label_widget(_("_View by:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = make_cycle(label, CYA_CATSUBCAT);
	data->CY_for = widget;
	gtk_grid_attach (GTK_GRID (table), data->CY_for, 2, row, 1, 1);


	row++;
	label = make_label_widget(_("_Type:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = make_cycle(label, CYA_KIND);
	data->CY_kind = widget;
	gtk_grid_attach (GTK_GRID (table), data->CY_kind, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Only out of budget"));
	data->CM_onlyout = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	row++;
	label = make_label_group(_("Date filter"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->CY_range = make_daterange(label, DATE_RANGE_CUSTOM_DISABLE);
	gtk_grid_attach (GTK_GRID (table), data->CY_range, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_From:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->PO_mindate = gtk_date_entry_new(label);
	gtk_grid_attach (GTK_GRID (table), data->PO_mindate, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_To:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->PO_maxdate = gtk_date_entry_new(label);
	gtk_grid_attach (GTK_GRID (table), data->PO_maxdate, 2, row, 1, 1);


	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	//toolbar
	widget = repbudget_toolbar_create(data);
	data->TB_bar = widget;
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	//infos
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER(hbox), SPACING_SMALL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	widget = make_label(NULL, 0.5, 0.5);
	gimp_label_set_attributes (GTK_LABEL (widget), PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL, -1);
	data->TX_daterange = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);


	entry = gtk_label_new(NULL);
	data->TX_total[2] = entry;
	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	label = gtk_label_new(_("Result:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	entry = gtk_label_new(NULL);
	data->TX_total[1] = entry;
	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	label = gtk_label_new(_("Budget:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	entry = gtk_label_new(NULL);
	data->TX_total[0] = entry;
	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	label = gtk_label_new(_("Spent:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);


	notebook = gtk_notebook_new();
	data->GR_result = notebook;
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

	//page: list

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, NULL);

	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = lst_repbud_create();
	data->LV_report = treeview;
	gtk_container_add (GTK_CONTAINER(widget), treeview);
    gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	//detail
	widget = gtk_scrolled_window_new (NULL, NULL);
	data->GR_detail = widget;
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = create_list_transaction(LIST_TXN_TYPE_DETAIL, PREFS->lst_det_columns);
	data->LV_detail = treeview;
	gtk_container_add (GTK_CONTAINER(widget), treeview);

    gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);

	//page: 2d bar
	//widget = gtk_chart_new(CHART_TYPE_COL);
	widget = ui_chart_progress_new();
	data->RE_progress = widget;
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);


	// connect dialog signals
    g_signal_connect (window, "delete-event", G_CALLBACK (repbudget_window_dispose), (gpointer)data);
	g_signal_connect (window, "map-event"   , G_CALLBACK (repbudget_window_mapped), NULL);

	// setup, init and show window
	wg = &PREFS->bud_wg;
	if( wg->l && wg->t )
		gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
	gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);

	// toolbar
	if(PREFS->toolbar_style == 0)
		gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
	else
		gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);

	gtk_widget_show_all (window);


	//minor ?
	hb_widget_visible(data->CM_minor, PREFS->euro_active);
	repbudget_sensitive(window, NULL);
	repbudget_update_detail(window, NULL);

	return(window);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static GString *lst_repbud_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
const gchar *format;

	node = g_string_new(NULL);

	// header
	format = (clipboard == TRUE) ? "%s\t%s\t%s\t%s\t\n" : "%s;%s;%s;%s;\n";
	g_string_append_printf(node, format, (title == NULL) ? _("Category") : title, _("Spent"), _("Budget"), _("Result"));

	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	gchar *name, *status;
	gdouble spent, budget, result;

		gtk_tree_model_get (model, &iter,
			//LST_REPDIST_KEY, i,
			LST_BUDGET_NAME, &name,
			LST_BUDGET_SPENT, &spent,
			LST_BUDGET_BUDGET, &budget,
			LST_BUDGET_RESULT, &result,
		    LST_BUDGET_STATUS, &status,
			-1);

		format = (clipboard == TRUE) ? "%s\t%.2f\t%.2f\t%.2f\t%s\n" : "%s;%.2f;%.2f;%.2f;%s\n";
		g_string_append_printf(node, format, name, spent, budget, result, status);
		
		//leak
		g_free(name);
		g_free(status);
		
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}




/*
**
** The function should return:
** a negative integer if the first value comes before the second,
** 0 if they are equal,
** or a positive integer if the first value comes after the second.
*/
/*
static gint budget_listview_compare_funct (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint retval = 0;
//Category *entry1, *entry2;
gchar *entry1, *entry2;

	gtk_tree_model_get(model, a, LST_BUDGET_NAME, &entry1, -1);
	gtk_tree_model_get(model, b, LST_BUDGET_NAME, &entry2, -1);

	retval = hb_string_utf8_compare(entry1, entry2);

	//leak
	g_free(entry2);
	g_free(entry1);

    return retval;
}*/


static void lst_repbud_cell_data_function_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
   {
gdouble  value;
gchar *color;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter, column_id, &value, -1);

	if( value )
	{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);

		if( column_id == LST_BUDGET_RESULT)
			color = get_minimum_color_amount (value, 0.0);
		else
			color = get_normal_color_amount(value);

		g_object_set(renderer,
			"foreground",  color,
			"text", buf,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
}



static void lst_repbud_cell_data_function_result (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
   {
gdouble  value;
gchar *color;
gchar *status;
gint column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter, 
		column_id, &value,
		LST_BUDGET_STATUS, &status,
		-1);

	if( value )
	{
		color = get_minimum_color_amount (value, 0.0);
		g_object_set(renderer,
			"foreground",  color,
			"text", status,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
	
	//leak
	g_free(status);
}



static GtkTreeViewColumn *lst_repbud_column_create_amount(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbud_cell_data_function_amount, GINT_TO_POINTER(id), NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}

/*
** create our statistic list
*/
static GtkWidget *lst_repbud_create(void)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	DB( g_print("\n[repbudget] create list\n") );


	/* create list store */
	store = gtk_list_store_new(
	  	NUM_LST_BUDGET,
		G_TYPE_INT,
		G_TYPE_INT,
		G_TYPE_STRING,
		G_TYPE_DOUBLE,     //spent
		G_TYPE_DOUBLE,     //budget
		G_TYPE_DOUBLE,     //result
		G_TYPE_STRING	   //status
		);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	/* column: Name */
	renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Category"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_set_cell_data_func(column, renderer, ope_result_cell_data_function, NULL, NULL);
	gtk_tree_view_column_add_attribute(column, renderer, "text", LST_BUDGET_NAME);
	gtk_tree_view_column_set_sort_column_id (column, LST_BUDGET_NAME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Expense */
	column = lst_repbud_column_create_amount(_("Spent"), LST_BUDGET_SPENT);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Income */
	column = lst_repbud_column_create_amount(_("Budget"), LST_BUDGET_BUDGET);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Result */
	column = lst_repbud_column_create_amount(_("Result"), LST_BUDGET_RESULT);
	renderer = gtk_cell_renderer_text_new ();
	//g_object_set(renderer, "xalign", 0.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbud_cell_data_function_result, GINT_TO_POINTER(LST_BUDGET_RESULT), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* sort */
	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), budget_listview_compare_funct, NULL, NULL);
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_BUDGET_NAME, GTK_SORT_ASCENDING);

/*
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_NAME  , stat_list_compare_func, GINT_TO_POINTER(LST_BUDGET_POS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_SPENT , stat_list_compare_func, GINT_TO_POINTER(LST_BUDGET_SPENT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_BUDGET, stat_list_compare_func, GINT_TO_POINTER(LST_BUDGET_BUDGET), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_RESULT, stat_list_compare_func, GINT_TO_POINTER(LST_BUDGET_RESULT), NULL);
*/

	return(view);
}

