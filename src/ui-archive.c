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

#include "ui-archive.h"
#include "ui-account.h"
#include "ui-category.h"
#include "ui-payee.h"
#include "ui-split.h"
#include "ui-tag.h"

#include "gtk-dateentry.h"

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


extern gchar *RA_ARC_TYPE[];
extern gchar *CYA_ARC_UNIT[];
extern gchar *RA_ARC_WEEKEND[];

extern HbKvData CYA_TXN_STATUS[];

extern gchar *CYA_TXN_TYPE[];


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */






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

		gtk_tree_model_get (model, &iter, LST_DEFARC_DATAS, &tmp_arc, -1);
		if( arc == tmp_arc )
		{
			gtk_tree_selection_select_iter (selection, &iter);
			break;
		}
	
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


/*
**
** The function should return:
** a negative integer if the first value comes before the second,
** 0 if they are equal,
** or a positive integer if the first value comes after the second.
*/
static gint ui_arc_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
Archive *item1, *item2;
gdouble tmpval;
gint retval = 0;

	gtk_tree_model_get(model, a, LST_DEFARC_DATAS, &item1, -1);
	gtk_tree_model_get(model, b, LST_DEFARC_DATAS, &item2, -1);

    switch (sortcol)
    {
		case LST_DEFARC_SORT_DATE:
			retval = item1->nextdate - item2->nextdate;
			break;
		case LST_DEFARC_SORT_MEMO:
			retval = (item1->flags & GF_INCOME) - (item2->flags & GF_INCOME);
			if(!retval)
			{
				retval = hb_string_utf8_compare(item1->memo, item2->memo);
			}
			break;
		case LST_DEFARC_SORT_PAYEE:
			{
			Payee *p1, *p2;

				p1 = da_pay_get(item1->kpay);
				p2 = da_pay_get(item2->kpay);
				if( p1 != NULL && p2 != NULL )
				{
					retval = hb_string_utf8_compare(p1->name, p2->name);
				}
			}
			break;

		case LST_DEFARC_SORT_CATEGORY:
			{
			Category *c1, *c2;

				c1 = da_cat_get(item1->kcat);
				c2 = da_cat_get(item2->kcat);
				if( c1 != NULL && c2 != NULL )
				{
					retval = hb_string_utf8_compare(c1->fullname, c2->fullname);
				}
			}
			break;

		case LST_DEFARC_SORT_AMOUNT:
			tmpval = item1->amount - item2->amount;
			retval = tmpval > 0 ? 1 : -1;
			break;
		//#1928147 sort on account as well
		case LST_DEFARC_SORT_ACCOUNT:
			{
			Account *a1, *a2;

				a1 = da_acc_get(item1->kacc);
				a2 = da_acc_get(item2->kacc);
				if( a1 != NULL && a2 != NULL )
				{
					retval = hb_string_utf8_compare(a1->name, a2->name);
				}
			}
			break;
			
		default:
			g_return_val_if_reached(0);
	}
    return retval;
}


static void 
ui_arc_listview_cell_data_function_auto (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *item;
gchar *info, *iconname;

	// get the transaction
	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &item, -1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			iconname = ( item->flags & OF_AUTO ) ? ICONNAME_HB_OPE_AUTO : NULL;
			g_object_set(renderer, "icon-name", iconname, NULL);
			break;
		case 2:
			info = NULL;
			//TODO: this is crappy/unsafe to call CYA_ARC_UNIT here
			//#1898294 not translated
			if( ( item->flags & OF_AUTO ) )
			   info = g_strdup_printf("%d %s", item->every, _(CYA_ARC_UNIT[item->unit]));

			g_object_set(renderer, "text", info, NULL);

			g_free(info);
			break;
	}
}


static void
ui_arc_listview_cell_data_function_date (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar buffer[256];
GDate *date;

	gtk_tree_model_get(model, iter,
		LST_DEFARC_DATAS, &arc,
		-1);

	if(arc && (arc->flags & OF_AUTO) )
	{
		date = g_date_new_julian (arc->nextdate);
		g_date_strftime (buffer, 256-1, PREFS->date_format, date);
		g_date_free(date);

		//g_snprintf(buf, sizeof(buf), "%d", ope->ope_Date);

		g_object_set(renderer, "text", buffer, NULL);

	}
	else
		g_object_set(renderer, "text", NULL, NULL);

}


static void 
ui_arc_listview_cell_data_function_memo (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &arc, -1);
	if(arc)
	{
		text = arc->memo;
	}
	g_object_set(renderer, "text", text, NULL);
}


static void
ui_arc_listview_cell_data_function_payee (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &arc, -1);
	if(arc)
	{
	Payee *pay = da_pay_get(arc->kpay);

		if(pay != NULL)
			text = pay->name;
	}
	g_object_set(renderer, "text", text, NULL);
}


static void
ui_arc_listview_cell_data_function_category (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &arc, -1);
	if(arc)
	{
		if(arc->flags & OF_SPLIT)
		{
			text = _("- split -");
		}
		else
		{
		Category *cat = da_cat_get(arc->kcat);

			text = (cat != NULL) ? cat->fullname : "";
		}
	}
	g_object_set(renderer, "text", text, NULL);
}


static void 
ui_arc_listview_cell_data_function_account (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &arc, -1);
	if(arc)
	{
	Account *acc = da_acc_get(arc->kacc);

		if(acc != NULL)
			text = acc->name;
	}
	g_object_set(renderer, "text", text, NULL);
}


static void
ui_arc_listview_cell_data_function_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gdouble amount;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
Account *acc;
gchar *color;
gint weight;

	gtk_tree_model_get(model, iter, 
		LST_DEFARC_DATAS, &arc,
		-1);

	amount = arc->amount;

	if( amount != 0.0)
	{
	acc = da_acc_get(arc->kacc);

		if( acc != NULL )
			hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, acc->kcur, GLOBALS->minor);
		else
			hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, GLOBALS->kcur, GLOBALS->minor);

		color = get_normal_color_amount(amount);

		weight = arc == NULL ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;

		g_object_set(renderer,
			"weight", weight,
			"foreground", color,
			"text", buf,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", NULL, NULL);
	}
	
}


#if MYDEBUG == 1
static void
ui_arc_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	


static GtkTreeViewColumn *
ui_arc_listview_column_text_create(gchar *title, gint sortcolumnid, GtkTreeCellDataFunc func, gpointer user_data)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	
	column = gtk_tree_view_column_new_with_attributes(title, renderer, NULL);

	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);

	gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	//gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	//gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	//gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, func, user_data, NULL);

	return column;
}


//TODO: we could maybe merge this list with the scheduled list
static GtkWidget *ui_arc_listview_new(void)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	//store
	store = gtk_list_store_new (
		NUM_LST_DEFARC,
		G_TYPE_POINTER /* scheduled */
//		G_TYPE_UINT,
//		G_TYPE_BOOLEAN
		);

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);


	#if MYDEBUG == 1
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_function_debugkey, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	#endif

	/* column: Scheduled icon */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_function_auto, GINT_TO_POINTER(1), NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_function_auto, GINT_TO_POINTER(2), NULL);
	gtk_tree_view_column_set_spacing(column, SPACING_TINY);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Next on */
	renderer = gtk_cell_renderer_text_new ();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Next date"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_function_date, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFARC_SORT_DATE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column: Payee */
	column = ui_arc_listview_column_text_create(_("Payee"), LST_DEFARC_SORT_PAYEE, ui_arc_listview_cell_data_function_payee, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Category */
	column = ui_arc_listview_column_text_create(_("Category"), LST_DEFARC_SORT_CATEGORY, ui_arc_listview_cell_data_function_category, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column: Memo */
	column = ui_arc_listview_column_text_create(_("Memo"), LST_DEFARC_SORT_MEMO, ui_arc_listview_cell_data_function_memo, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column : amount */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Amount"));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_function_amount, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFARC_SORT_AMOUNT);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column : Account */
	column = ui_arc_listview_column_text_create(_("Account"), LST_DEFARC_SORT_ACCOUNT, ui_arc_listview_cell_data_function_account, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  	/* column : empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//sortable
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_DATE, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_SORT_DATE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_MEMO, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_SORT_MEMO), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_PAYEE, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_SORT_PAYEE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_CATEGORY, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_SORT_CATEGORY), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_AMOUNT, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_SORT_AMOUNT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_ACCOUNT, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_SORT_ACCOUNT), NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_DEFARC_SORT_MEMO, GTK_SORT_ASCENDING);

	//gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(view), FALSE);
	//gtk_tree_view_set_reorderable (GTK_TREE_VIEW(view), TRUE);

	return(treeview);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
ui_arc_manage_update(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
Archive *arc;
gboolean selected, sensitive;

	DB( g_print("\n[ui-scheduled] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);

	DB( g_print(" toolbutton sensitive\n") );

	sensitive = (selected == TRUE) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_rem, sensitive);
	gtk_widget_set_sensitive(data->MB_schedule, sensitive);
	gtk_widget_set_sensitive(data->CM_auto, sensitive);

	DB( g_print(" scheduled popover sensitive\n") );

	sensitive = FALSE;
	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &arc, -1);

		if( arc->flags & OF_AUTO )
			sensitive = TRUE;
	}
	
	gtk_widget_set_sensitive(data->LB_next, sensitive);
	gtk_widget_set_sensitive(data->PO_next, sensitive);

	gtk_widget_set_sensitive(data->LB_every, sensitive);
	gtk_widget_set_sensitive(data->NB_every, sensitive);

	gtk_widget_set_sensitive(data->LB_weekend, sensitive);
	gtk_widget_set_sensitive(data->CY_weekend, sensitive);

	gtk_widget_set_sensitive(data->EX_options, sensitive);
	
	gtk_widget_set_sensitive(data->CY_unit, sensitive);
	gtk_widget_set_sensitive(data->CM_limit, sensitive);

	gtk_widget_set_sensitive(data->LB_posts, sensitive);
	
	sensitive = (sensitive == TRUE) ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_limit)) : sensitive;
	gtk_widget_set_sensitive(data->NB_limit, sensitive);

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
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &arcitem, -1);

		arcitem->flags &= ~(OF_AUTO);
		sensitive = gtk_switch_get_active(GTK_SWITCH(data->CM_auto)) ? TRUE : FALSE;
		if(sensitive)
			arcitem->flags |= OF_AUTO;
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

		if( (typsch) && (item->flags & OF_AUTO) )
			insert = TRUE;

		if( (typtpl) && !(item->flags & OF_AUTO) )
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
					LST_DEFARC_DATAS, item,	//data struct
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
			item->flags |= OF_AUTO;

		item->every = 1;
		item->unit = 2;
		item->nextdate = GLOBALS->today;
		
		//GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
		da_archive_append_new(item);

		DB( g_print(" kacc: '%d'\n", item->kacc) );

		
		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFARC_DATAS, item,
//			LST_DEFARC_OLDPOS, 0,
			-1);

		gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &iter);

		data->change++;
	}

	deftransaction_dispose(dialog, NULL);
	gtk_widget_destroy (dialog);
	
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
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &arcitem, -1);

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
		gtk_widget_destroy (dialog);
		
		da_transaction_free(new_txn);
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
		
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &item, -1);

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
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &item, -1);

		g_signal_handlers_block_by_func (G_OBJECT (data->CM_auto ), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
		g_signal_handlers_block_by_func (G_OBJECT (data->CM_limit), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);

		gtk_switch_set_active(GTK_SWITCH(data->CM_auto), (item->flags & OF_AUTO) ? 1 : 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_every), item->every);
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_unit), item->unit);
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_next), item->nextdate);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_limit), (item->flags & OF_LIMIT) ? 1 : 0);
		DB( g_print(" nb_limit = %d %g\n", item->limit, (gdouble)item->limit) );
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_limit), (gdouble)item->limit);
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_weekend), item->weekend);
		
		g_signal_handlers_unblock_by_func (G_OBJECT (data->CM_limit), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
		g_signal_handlers_unblock_by_func (G_OBJECT (data->CM_auto ), G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
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
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &item, -1);

		//#1863484: reset flag to enable remove auto and limit :)
		item->flags &= ~(OF_AUTO|OF_LIMIT);
		
		active = gtk_switch_get_active(GTK_SWITCH(data->CM_auto));
		if(active == 1) item->flags |= OF_AUTO;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->NB_every));
		item->every   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_every));
		item->unit    = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_unit));
		item->nextdate	= gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_next));

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_limit));
		if(active == 1) item->flags |= OF_LIMIT;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->NB_limit));
		item->limit   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_limit));

		item->weekend = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_weekend));

		//#1906953 add ignore weekend
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

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(popover, GTK_TYPE_WINDOW)), "inst_data");

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



static gboolean ui_arc_manage_cb_on_key_press(GtkWidget *source, GdkEventKey *event, gpointer user_data)
{
struct ui_arc_manage_data *data = user_data;

	// On Control-f enable search entry
	if (event->state & GDK_CONTROL_MASK
		&& event->keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->ST_search);
	}
	else
	if (event->keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
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
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &arcitem, -1);

		ui_arc_manage_set(treeview, NULL);
	}

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

	da_archive_glist_sorted(1);

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

	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_unit), 2);
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
	g_signal_connect (data->BT_edit, "clicked", G_CALLBACK (ui_arc_manage_cb_edit_clicked), NULL);
	g_signal_connect (data->BT_rem , "clicked", G_CALLBACK (ui_arc_manage_cb_delete_clicked), NULL);

	g_signal_connect (data->PO_schedule, "closed", G_CALLBACK (ui_arc_manage_cb_popover_closed), NULL);
	
	g_signal_connect (data->CM_auto,  "notify::active", G_CALLBACK (ui_arc_manage_cb_schedule_changed), NULL);
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
GtkWidget *content, *group_grid, *hbox, *expander, *label, *widget;
gint row;

	content = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	// group :: Scheduled insertion
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content), group_grid, FALSE, FALSE, 0);

	row = 0;
	widget = gtk_switch_new();
	data->CM_auto = widget;
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	
	row++;
	label = gtk_label_new_with_mnemonic (_("Next _date:"));
	data->LB_next = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = gtk_date_entry_new(label);
	data->PO_next = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("Ever_y:"));
	data->LB_every = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 1, 1);
	widget = make_numeric(label, 1, 100);
	data->NB_every = widget;
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	//label = gtk_label_new_with_mnemonic (_("_Unit:"));
    //gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	widget = make_cycle(label, CYA_ARC_UNIT);
	data->CY_unit = widget;
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	expander = gtk_expander_new_with_mnemonic(_("More options"));
	data->EX_options = expander;
	gtk_box_pack_start (GTK_BOX (content), expander, FALSE, FALSE, 0);

	// group :: Scheduled insertion
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	g_object_set(group_grid, "margin", SPACING_SMALL, NULL);
	gtk_container_add(GTK_CONTAINER(expander), group_grid);
	
	row++;
	label = make_label_widget(_("Week end:"));
	data->LB_weekend = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_cycle(label, RA_ARC_WEEKEND);
	data->CY_weekend = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("_Stop after:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 1, 1);

		widget = gtk_check_button_new();
		data->CM_limit = widget;
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

		widget = make_numeric(label, 1, 366);
		data->NB_limit = widget;
	    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

		label = gtk_label_new_with_mnemonic (_("posts"));
		data->LB_posts = label;
	    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

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
GtkToolItem *toolitem;
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
	g_object_set(content, "margin", SPACING_LARGE, NULL);
	gtk_box_pack_start (GTK_BOX (content_area), content, TRUE, TRUE, 0);
	
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (content), hbox, FALSE, FALSE, 0);

		box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, TRUE, 0);

		gtk_style_context_add_class (gtk_widget_get_style_context (box), GTK_STYLE_CLASS_LINKED);
		gtk_style_context_add_class (gtk_widget_get_style_context (box), GTK_STYLE_CLASS_RAISED);
		gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	
		widget = gtk_toggle_button_new_with_label(_("Scheduled"));
		data->BT_typsch = widget;
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
		
		widget = gtk_toggle_button_new_with_label(_("Template"));
		data->BT_typtpl = widget;
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
	
	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	// list + toolbar
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (content), vbox, TRUE, TRUE, 0);
	
	// listview
	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	//#1970509 enable hscrollbar
	//gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = (GtkWidget *)ui_arc_listview_new();
	data->LV_arc = treeview;
	gtk_widget_set_size_request(treeview, HB_MINWIDTH_LIST, -1);
	gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
	gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);

	tbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size (GTK_TOOLBAR(tbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(tbar), GTK_TOOLBAR_ICONS);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), bbox);
	gtk_toolbar_insert(GTK_TOOLBAR(tbar), GTK_TOOL_ITEM(toolitem), -1);

		widget = make_image_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_add = widget;
		gtk_container_add (GTK_CONTAINER(bbox), widget);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_rem = widget;
		gtk_container_add (GTK_CONTAINER(bbox), widget);

	toolitem = gtk_separator_tool_item_new ();
	//gtk_tool_item_set_expand (toolitem, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolitem), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(tbar), GTK_TOOL_ITEM(toolitem), -1);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), bbox);
	gtk_toolbar_insert(GTK_TOOLBAR(tbar), GTK_TOOL_ITEM(toolitem), -1);

	//widget = gtk_button_new_with_mnemonic(_("_Edit"));
	widget = make_image_button(ICONNAME_LIST_EDIT, _("Edit"));
	data->BT_edit = widget;
	gtk_container_add (GTK_CONTAINER(bbox), widget);
	
	//schedule button
	menubutton = gtk_menu_button_new ();
	data->MB_schedule = menubutton;
	gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_DOWN );
	gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
	//gtk_widget_set_hexpand (menubutton, TRUE);
	gtk_widget_show_all(menubutton);
	gtk_container_add (GTK_CONTAINER(bbox), menubutton);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	label = gtk_label_new_with_mnemonic (_("_Schedule"));
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);
	image = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX(box), image, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(menubutton), box);
	GtkWidget *template = ui_arc_manage_create_scheduling(data);
	GtkWidget *popover = create_popover (menubutton, template, GTK_POS_TOP);
	data->PO_schedule = popover;
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);
	

	// connect dialog signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_arc_manage_mapped), &dialog);
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_arc_manage_cb_on_key_press), (gpointer)data);


	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all(content);
	gtk_widget_show (dialog);

	// wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	ui_arc_manage_cleanup(data, result);
	gtk_widget_destroy (dialog);

	g_free(data);
	
	return NULL;
}


