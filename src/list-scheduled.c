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

#include "list-scheduled.h"

/****************************************************************************/
/* Debug macros																*/
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


static void
lst_sch_cell_data_func_lateicon (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar *iconname = NULL;
gint nblate;

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_NB_LATE, &nblate,
		-1);

	iconname = ( nblate > 0 ) ? ICONNAME_WARNING : NULL;

	g_object_set(renderer, "icon-name", iconname, NULL);
}


static void 
lst_sch_cell_data_func_latetext (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *markuptxt;
gchar *color;
gint nblate;
//gint weight;

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
	  LST_DSPUPC_NB_LATE, &nblate,
		-1);

	if(arc && nblate > 0)
	{
		markuptxt = g_strdup_printf(nblate < 10 ? "%d" : "+10", nblate);

		color = NULL;
		//weight = PANGO_WEIGHT_NORMAL;

		if(nblate > 0 && PREFS->custom_colors == TRUE)
		{
			color = PREFS->color_warn;
		}

		g_object_set(renderer,
			//"weight", weight,
			"foreground", color,
			"text", markuptxt,
			NULL);

		g_free(markuptxt);
	}
	else
		g_object_set(renderer, "text", NULL, NULL);

}


static void
lst_sch_cell_data_func_date (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar buffer[256];
GDate *date;

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
		-1);

	if(arc)
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
lst_sch_cell_data_func_still (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *markuptxt;

	gtk_tree_model_get(model, iter,
	  LST_DSPUPC_DATAS, &arc,
		-1);

	if(arc && (arc->flags & OF_LIMIT) )
	{
		markuptxt = g_strdup_printf("%d", arc->limit);
		g_object_set(renderer, "markup", markuptxt, NULL);
		g_free(markuptxt);
	}
	else
		g_object_set(renderer, "text", NULL, NULL);
}


static void 
lst_sch_cell_data_func_payee (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
		-1);

	if(arc)
	{
		//#bugfix 5.6.3
		if(arc->flags & OF_INTXFER)
		{
		Account *acc = da_acc_get(arc->kxferacc);

			//5.6 use acc strings for 5.3 add > < for internal xfer
			if( acc )
				text = ( arc->flags & OF_INCOME ) ? acc->xferincname : acc->xferexpname;
			
		}
		else
		{
		Payee *pay = da_pay_get(arc->kpay);
			
			text = (pay != NULL) ? pay->name : NULL;
		}
	}

	g_object_set(renderer, "text", text, NULL);

}


static void
lst_sch_cell_data_func_category (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DSPUPC_DATAS, &arc, -1);
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
lst_sch_cell_data_func_memo (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *memo;
gint weight;

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
		LST_DSPUPC_MEMO, &memo,
		-1);

	//to display total
	weight = arc == NULL ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;

	g_object_set(renderer, "weight", weight, "text", memo, NULL);

	//leak
	g_free(memo);

}


static void 
lst_sch_cell_data_func_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gdouble expense, income, amount;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint column = GPOINTER_TO_INT(user_data);
gchar *color;
gint weight;

	gtk_tree_model_get(model, iter, 
		LST_DSPUPC_DATAS, &arc,
		LST_DSPUPC_EXPENSE, &expense,
		LST_DSPUPC_INCOME , &income,
		-1);

	amount = (column == -1) ? expense : income;
		
	if( amount != 0.0 )
	{
	Account *acc = NULL;
	guint32 kcur = GLOBALS->kcur;

		if( arc != NULL ) /* display total */
		{
			if(arc->flags & OF_INTXFER)
			{
				if( column == -1)
					acc = da_acc_get(arc->kacc);
				else
					acc = da_acc_get(arc->kxferacc);
			}
			else
			{
				acc = da_acc_get(arc->kacc);
			}
		}

		if(acc != NULL)
			kcur = acc->kcur;

		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, kcur, GLOBALS->minor);

		color = get_normal_color_amount(amount);

		weight = (arc == NULL) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;

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


static void 
lst_sch_cell_data_func_account (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
Account *acc;
gchar *name = NULL;

	gtk_tree_model_get(model, iter, 
		LST_DSPUPC_DATAS, &arc,
		-1);

	/* 1378836 display acc or dstacc */
	if( arc != NULL )
	{
		acc = da_acc_get(arc->kacc);
		if( acc != NULL )
		{
			name = acc->name;
		}
	}

	g_object_set(renderer, "text", name, NULL);

}


static gboolean 
lst_sch_cb_selection_func(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
GtkTreeIter iter;
Archive *arc;

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter,
			LST_DSPUPC_DATAS, &arc,
			-1);

		if( arc == NULL )
			return FALSE;
	}

	return TRUE;
}


static void
lst_sch_popmenu_cb_activate (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
GtkTreeViewColumn *column = user_data;
//GtkWidget *treeview;

	DB( g_print ("\n[lst_sch] menuitem activated\n") );

	if( !GTK_IS_TREE_VIEW_COLUMN(column) )
		return;

	//TDOO: useless until we link dsp_accoutn balance to this list
	//treeview = gtk_tree_view_column_get_tree_view(GTK_TREE_VIEW_COLUMN(column));
	gtk_tree_view_column_set_visible(column, gtk_check_menu_item_get_active(checkmenuitem) );
	//lst_sch_columns_prefs_get(GTK_TREE_VIEW(treeview));
}


static gint
lst_sch_cb_button_pressed_event (GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{

	DB( g_print ("\n[lst_sch] popmenu\n") );
	
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{

		// check we ARE in the header but in bin window
		if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)))
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
		return FALSE;
}


static void
lst_sch_popmenu_destroy(GtkTreeView *treeview, gpointer user_data)
{
	DB( g_print ("\n[lst_sch] menu destroy\n") );

}


static GtkWidget *
lst_sch_popmenu_new(GtkTreeView *treeview)
{
GtkTreeViewColumn *column;
GtkWidget *menu;
GtkWidget *menuitem;
gint i, uid;
//GtkAccelGroup *accel_group = NULL;

	//accel_group = gtk_accel_group_new();
	//gtk_window_add_accel_group(GTK_WINDOW(data->window), accel_group);

	DB( g_print ("\n[lst_sch] create popmenu\n") );
	
	menu = gtk_menu_new();
	//data->ME_popmenu = menu;

	for(i=0 ; i < NUM_LST_COL_DSPUPC ; i++ )
	{
		column = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), i);
		if( column != NULL )
		{
			//uid = gtk_tree_view_column_get_sort_column_id (column);
			uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			DB( g_print (" col %d\n", uid) );
			
			if( (uid == -1) || (uid == 0)
				|| (uid == COL_DSPUPC_ACCOUNT) 
			)
				continue;

			menuitem = gtk_check_menu_item_new_with_label ( gtk_tree_view_column_get_title (column) );
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), gtk_tree_view_column_get_visible (column) );

			g_signal_connect (menuitem, "activate",
								G_CALLBACK (lst_sch_popmenu_cb_activate), column);

		}
	}
	
	g_signal_connect (menu, "destroy", G_CALLBACK (lst_sch_popmenu_destroy), NULL);
	
	gtk_widget_show_all(menu);

	return menu;
}


static void
lst_sch_widget_columns_prefs_set(struct lst_sch_data *data)
{
guint i;

	DB( g_print ("\n[lst_sch] colums prefs set\n") );

	for(i=COL_DSPUPC_PAYEE;i<gtk_tree_view_get_n_columns(GTK_TREE_VIEW(data->treeview));i++)
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(data->treeview), i);

		if(column)
		{
		gint coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			switch(coluid)
			{
				case COL_DSPUPC_PAYEE:
					gtk_tree_view_column_set_visible(column, PREFS->pnl_upc_col_pay_show);
					gtk_tree_view_column_set_fixed_width(column, PREFS->pnl_upc_col_pay_width);
					break;
				case COL_DSPUPC_CATEGORY:
					gtk_tree_view_column_set_visible(column, PREFS->pnl_upc_col_cat_show);
					gtk_tree_view_column_set_fixed_width(column, PREFS->pnl_upc_col_cat_width);
					break;
				case COL_DSPUPC_MEMO:
					gtk_tree_view_column_set_visible(column, PREFS->pnl_upc_col_mem_show);
					gtk_tree_view_column_set_fixed_width(column, PREFS->pnl_upc_col_mem_width);
					break;
			}
		}
	}

}


static void
lst_sch_widget_columns_prefs_get(struct lst_sch_data *data)
{
guint i;

	DB( g_print ("\n[lst_sch] colums prefs get\n") );

	for(i=COL_DSPUPC_PAYEE;i<gtk_tree_view_get_n_columns(GTK_TREE_VIEW(data->treeview));i++)
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(data->treeview), i);

		if(column)
		{
		gint coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			//#1830656 use xxx_get_fixed_width instead of width, as if not visible will save 0 otherwise
			switch(coluid)
			{
				case COL_DSPUPC_PAYEE:
					PREFS->pnl_upc_col_pay_show  = gtk_tree_view_column_get_visible(column);
					PREFS->pnl_upc_col_pay_width = gtk_tree_view_column_get_fixed_width(column);
					break;
				case COL_DSPUPC_CATEGORY:
					PREFS->pnl_upc_col_cat_show  = gtk_tree_view_column_get_visible(column);
					PREFS->pnl_upc_col_cat_width = gtk_tree_view_column_get_fixed_width(column);
					break;
				case COL_DSPUPC_MEMO:
					PREFS->pnl_upc_col_mem_show  = gtk_tree_view_column_get_visible(column);
					PREFS->pnl_upc_col_mem_width = gtk_tree_view_column_get_fixed_width(column);
					break;
			}
		}
	}
}


static void 
lst_sch_widget_destroy(GtkTreeView *treeview, gpointer user_data)
{
struct lst_sch_data *data;

	data = g_object_get_data(G_OBJECT(treeview), "inst_data");

	DB( g_print ("\n[lst_sch] destroy\n") );

	lst_sch_widget_columns_prefs_get(data);

	gtk_widget_destroy (data->menu);
}


GtkWidget *
lst_sch_widget_new(void)
{
struct lst_sch_data *data;
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer  *renderer;
GtkTreeViewColumn *column;

	DB( g_print ("\n[lst_sch] create\n") );

	data = g_malloc0(sizeof(struct lst_sch_data));
	if(!data) return NULL;

	/* create list store */
	store = gtk_list_store_new(
	 	NUM_LST_DSPUPC,
		G_TYPE_POINTER,	/* scheduled */
		G_TYPE_INT,		/* next (sort) */
		G_TYPE_STRING,	/* memo/total */
		G_TYPE_DOUBLE,	/* expense */
		G_TYPE_DOUBLE,	/* income */
		G_TYPE_INT		/* nb late */
		);

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	data->treeview = treeview;
	g_object_unref(store);

	//store our window private data
	g_object_set_data(G_OBJECT(treeview), "inst_data", (gpointer)data);
	DB( g_print(" - treeview=%p, inst_data=%p\n", treeview, data) );

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_MULTIPLE);

	/* column: system */
	column = gtk_tree_view_column_new();
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//add system icon to 1st column
	gtk_tree_view_column_set_clickable(column, TRUE);
	GtkWidget *img = gtk_image_new_from_icon_name (ICONNAME_EMBLEM_SYSTEM, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(img);
	gtk_tree_view_column_set_widget(column, img);

	/* column : Late */
	column = gtk_tree_view_column_new();
	//TRANSLATORS: title of list column to inform the scheduled transaction is Late
	gtk_tree_view_column_set_title(column, _("Late"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_lateicon, NULL, NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_latetext, NULL, NULL);

	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPUPC_NB_LATE);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column : Still (for limited scheduled) */
	column = gtk_tree_view_column_new();
	//TRANSLATORS: title of list column to inform how many occurence remain to post for limited scheduled txn
	gtk_tree_view_column_set_title(column, _("Still"));

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_still, NULL, NULL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPUPC_REMAINING);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column: Next on */
	renderer = gtk_cell_renderer_text_new ();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Next date"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_date, NULL, NULL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPUPC_DATE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Payee */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
		"ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
		"width-chars", 40,
		NULL);
	column = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_DSPUPC_PAYEE));
	gtk_tree_view_column_set_title(column, _("Payee"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_payee, NULL, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", 1);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPACC_NAME);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST/2);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	/* column: Category */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
		"ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
		"width-chars", 40,
		NULL);
	column = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_DSPUPC_CATEGORY));
	gtk_tree_view_column_set_title(column, _("Category"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_category, NULL, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", 1);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPACC_NAME);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST/2);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column: Memo */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
		"ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
		"width-chars", 40,
		NULL);

	column = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_DSPUPC_MEMO));
	gtk_tree_view_column_set_title(column, _("Memo"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_memo, NULL, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", 2);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPACC_NAME);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST/2);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Amount */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Expense"));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_amount, GINT_TO_POINTER(-1), NULL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPACC_NAME);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Amount */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Income"));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_amount, GINT_TO_POINTER(1), NULL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPACC_NAME);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Account */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
		"ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
		"width-chars", 40,
		NULL);

	column = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_DSPUPC_ACCOUNT));
	gtk_tree_view_column_set_title(column, _("Account"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_account, NULL, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPOPE_DATE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// treeview func
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), lst_sch_cb_selection_func, NULL, NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_DSPUPC_NEXT, GTK_SORT_ASCENDING);

	lst_sch_widget_columns_prefs_set(data);
	
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	GtkWidget *popmenu = lst_sch_popmenu_new(GTK_TREE_VIEW(treeview));
	data->menu = popmenu;

	g_signal_connect (treeview, "destroy", G_CALLBACK (lst_sch_widget_destroy), NULL);

	g_signal_connect (treeview, "button-press-event", G_CALLBACK (lst_sch_cb_button_pressed_event), popmenu);


	return(treeview);
}

