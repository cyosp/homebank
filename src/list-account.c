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

#include "list-account.h"
#include "hub-account.h"

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




static void lst_accview_to_string_row(GString *node, gboolean clipboard, GtkTreeModel *model, GtkTreeIter *iter, gchar *sub)
{
const gchar *format;
gpointer p;
gint type;
gchar *text = "";
gdouble clear, recon, today, future;

	clear = recon = today = future = 0.0;

	gtk_tree_model_get (model, iter,
		LST_DSPACC_DATATYPE, &type,
		LST_DSPACC_DATAS, &p,
		-1);

	if( type == DSPACC_TYPE_HEADER )
	{
	PnlAccGrp *g = p;
		text = g->name;	
		g_string_append_printf(node, "%s\t\t\t\t\n", text);
	}
	else
	{
		if( type == DSPACC_TYPE_NORMAL )
		{
		Account *acc = p;
			text = acc->name;
			recon  = acc->bal_recon;
			clear  = acc->bal_clear;
			today  = acc->bal_today;
			future = acc->bal_future;
		}
		else
		{
		PnlAccGrp *g = p;
			recon  = g->bal_recon;
			clear  = g->bal_clear;
			today  = g->bal_today;
			future = g->bal_future;
		}

		if( type ==  DSPACC_TYPE_SUBTOTAL )
			text = _("Total");
		else
		if( type == DSPACC_TYPE_TOTAL )
			text = _("Grand total");

		format = (clipboard == TRUE) ? "%s%s\t%.2f\t%.2f\t%.2f\t%.2f\n" : "%s%s;%.2f;%.2f;%.2f;%.2f\n";
		g_string_append_printf(node, format, sub, text, recon, clear, today, future);
	}
}


GString *lst_accview_to_string(GtkTreeView *treeview, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter, child;
gboolean valid;
guint32 nbcols, i;
gchar sep;

	DB( g_print("\n[lst_accview] to string\n") );

	node = g_string_new(NULL);

	sep = (clipboard == TRUE) ? '\t' : ';';

	// header (nbcols-2 for icon column)
	nbcols = gtk_tree_view_get_n_columns (treeview) - 1;
	for( i=1 ; i < nbcols ; i++ )
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column (treeview, i);
	
		//todo: ? restrict to visibility
		if( GTK_IS_TREE_VIEW_COLUMN(column) )
		{
			g_string_append(node, gtk_tree_view_column_get_title (column));
			if( i < nbcols-1 )
			{
				g_string_append_c(node, sep);
			}
		}
	}
	g_string_append_c(node, '\n');



	//lines
	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		lst_accview_to_string_row(node, clipboard, model, &iter, "");
		if( gtk_tree_model_iter_has_child(model, &iter) )
		{
			valid = gtk_tree_model_iter_children(model, &child, &iter);
			while (valid)
			{
				lst_accview_to_string_row(node, clipboard, model, &child, "- ");
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
			}		
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}





/*
** draw some icons according to the stored data structure
*/
static void lst_accview_cell_data_func_status (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
Account *acc;
gchar *iconname = NULL;
gint dt;

	gtk_tree_model_get(model, iter, 
		LST_DSPACC_DATATYPE, &dt,
		LST_DSPACC_DATAS, &acc,
		-1);

	if( dt == DSPACC_TYPE_NORMAL )
	{
	
		switch(GPOINTER_TO_INT(user_data))
		{
			case 1:
				iconname = (acc->flags & AF_ADDED) ? ICONNAME_HB_OPE_NEW : NULL;
				// override if closed account
				if( acc->flags & AF_CLOSED )
					iconname = ICONNAME_CHANGES_PREVENT;
				break;
			case 2:
				iconname = (acc->flags & AF_CHANGED) ? ICONNAME_HB_OPE_EDIT : NULL;
				break;
		}
	}

	g_object_set(renderer, "icon-name", iconname, NULL);
}


/*
** draw some text from the stored data structure
*/
static void lst_accview_cell_data_func_text (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
gpointer p;
gint dt;
gint weight = PANGO_WEIGHT_BOLD;
gchar *text = NULL;
//gchar *groupname;

	gtk_tree_model_get(model, iter,
		LST_DSPACC_DATATYPE, &dt,
		LST_DSPACC_DATAS, &p,
		//LST_DSPACC_NAME, &groupname,
		-1);

	if( p == NULL )
		goto end;

	switch( dt )
	{
		case DSPACC_TYPE_NORMAL:
		{
		Account *acc = p;
			weight = PANGO_WEIGHT_NORMAL;
			text = acc->name;
		}
		break;

		case DSPACC_TYPE_HEADER:
		{
		PnlAccGrp *g = p;
			text = g->name;
		}
		break;

		case DSPACC_TYPE_SUBTOTAL:
			text = _("Total");
		break;

		case DSPACC_TYPE_TOTAL:
			text = _("Grand total");
		break;
	}

end:
	g_object_set(renderer, "weight", weight, "text", text, NULL);

}


static void
lst_accview_cell_data_func_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gpointer p;
gint dt;
gint colid = GPOINTER_TO_INT(user_data); //LST_DSPACC_(BANK/TODAY/FUTURE)
gdouble amount = -1;
gint weight = PANGO_WEIGHT_NORMAL;
gchar *color = NULL;
gchar *text = NULL;
	
	gtk_tree_model_get(model, iter,
		LST_DSPACC_DATATYPE, &dt,
		LST_DSPACC_DATAS, &p,
		-1);

	if( p == NULL )
		goto end;		

	if( dt == DSPACC_TYPE_NORMAL )
	{
	Account *acc = p;

		switch(colid)
		{
			case COL_DSPACC_CLEAR: amount = acc->bal_clear; break;
			case COL_DSPACC_RECON: amount = acc->bal_recon; break;
			case COL_DSPACC_TODAY: amount = acc->bal_today; break;
			case COL_DSPACC_FUTURE: amount = acc->bal_future; break;
		}

		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, acc->kcur, GLOBALS->minor);
		color = get_minimum_color_amount(amount, acc->minimum);
		text = buf;
	}
	else
	{
	gboolean expanded = TRUE;

		if( dt == DSPACC_TYPE_HEADER )
		{
		GtkTreePath* tp = gtk_tree_model_get_path(model, iter);
			expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(gtk_tree_view_column_get_tree_view(col)), tp);
			gtk_tree_path_free(tp);
		}
			
		if( dt != DSPACC_TYPE_HEADER || (expanded == FALSE) )
		{
		PnlAccGrp *g = p;

			switch(colid)
			{
				case COL_DSPACC_CLEAR: amount = g->bal_clear; break;
				case COL_DSPACC_RECON: amount = g->bal_recon; break;
				case COL_DSPACC_TODAY: amount = g->bal_today; break;
				case COL_DSPACC_FUTURE: amount = g->bal_future; break;
			}

			//nota: header is always in base currency
			hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, GLOBALS->kcur, GLOBALS->minor);
			if(expanded)
				weight = PANGO_WEIGHT_BOLD;
			color = get_normal_color_amount(amount);
			text = buf;
		}
	}

end:
	g_object_set(renderer, 
		"weight", weight,
		"foreground",  color,
		"text", text,
		NULL);	
}




static gint
lst_accview_func_compare (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint retval = 0;
gint dt1, dt2;
gpointer p1, p2;
gint pos1, pos2;

    gtk_tree_model_get(model, a, 
		LST_DSPACC_POS, &pos1,
    	LST_DSPACC_DATATYPE, &dt1, 
    	LST_DSPACC_DATAS, &p1,
    	-1);
    gtk_tree_model_get(model, b, 
		LST_DSPACC_POS, &pos2,
    	LST_DSPACC_DATATYPE, &dt2, 
    	LST_DSPACC_DATAS, &p2,
    	-1);

	if( p1 == NULL || p2 == NULL )
		return 0;
	
	if( dt1 == DSPACC_TYPE_NORMAL && dt2 == DSPACC_TYPE_NORMAL )
	{
	Account *entry1 = p1;
	Account *entry2 = p2;

		retval = entry1->pos - entry2->pos;
	}
	else
	if( dt1 == DSPACC_TYPE_HEADER && dt2 == DSPACC_TYPE_HEADER )
	{
	PnlAccGrp *elt1 = p1;
	PnlAccGrp *elt2 = p2;

		retval = pos1 - pos2;
		if( !retval )
			retval = hb_string_utf8_compare(elt1->name, elt2->name);
	}

    return retval;
}


static gboolean
lst_accview_func_selection(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
GtkTreeIter iter;

	DB( g_print ("\n[lst_accview] selection\n") );

	
	if( gtk_tree_path_get_depth( path ) < 2 )
		return FALSE;

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
	gint dt;

		gtk_tree_model_get(model, &iter,
			LST_DSPACC_DATATYPE, &dt,
			-1);	

		if( dt != DSPACC_TYPE_NORMAL )
			return FALSE;
	}

	return TRUE;
}


static void
lst_accview_columns_prefs_get(GtkTreeView *treeview)
{
GtkTreeViewColumn *column;
gint i, uid;

	DB( g_print ("\n[lst_accview] columns prefs get\n") );

	DB( g_print(" nbcol=%d, nbsortid=%d\n", NUM_LST_COL_DSPACC, gtk_tree_view_get_n_columns (treeview)) );
	
	for(i=0 ; i < NUM_LST_COL_DSPACC ; i++ )
	{
		column = gtk_tree_view_get_column(treeview, i);
		if(column != NULL)
		{
			uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));
			if( uid >= 0 )
			{
			gboolean visible;

				visible = gtk_tree_view_column_get_visible (column);
				PREFS->lst_acc_columns[i] = (visible == TRUE) ? uid : -uid;
				DB( g_print(" col-%2d => %2d '%s' w=%d\n", i, uid, gtk_tree_view_column_get_title(column), PREFS->lst_ope_col_width[uid-1] ) );

				/* save width for accounts */
				if( uid == COL_DSPACC_ACCOUNTS )
				{
					PREFS->pnl_acc_col_acc_width = gtk_tree_view_column_get_width(column);
				}
			}
			else	//should not occurs
				PREFS->lst_ope_columns[i] = 0;
		}
	}
}




static void
lst_accview_columns_prefs_set(GtkTreeView *treeview, gint *cols_id)
{
GtkTreeViewColumn *column;
gboolean visible;
gint uid, i = 0;
gint tmpval;

	DB( g_print ("\n[lst_accview] columns prefs set\n") );
	
	for(i=0; i < NUM_LST_COL_DSPACC ; i++ )
	{
		column = gtk_tree_view_get_column(treeview, i);
		if( column != NULL )
		{
			uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));
			tmpval = homebank_pref_list_column_get(cols_id, uid, NUM_LST_COL_DSPACC);
			
			DB( g_print(" - pos:%2d uid:%d sortid:%2d (%s)\n", i, uid, tmpval, gtk_tree_view_column_get_title(column)) );
			visible = tmpval < 0 ? FALSE : TRUE;
			gtk_tree_view_column_set_visible (column, visible);
		}
	}
}


static GtkTreeViewColumn *
lst_accview_column_amount_new(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(id));
	gtk_tree_view_column_set_title(column, name);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_accview_cell_data_func_amount, GINT_TO_POINTER(id), NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_spacing( column, 16 );
	//gtk_tree_view_column_set_sort_column_id (column, LST_DSPACC_BANK);

	return column;
}


static void
lst_accview_popmenu_cb_activate (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
GtkTreeViewColumn *column = user_data;
//GtkWidget *treeview;

	DB( g_print ("\n[lst_accview] menuitem activated\n") );

	if( !GTK_IS_TREE_VIEW_COLUMN(column) )
		return;

	//TDOO: useless until we link dsp_accoutn balance to this list
	//treeview = gtk_tree_view_column_get_tree_view(GTK_TREE_VIEW_COLUMN(column));
	gtk_tree_view_column_set_visible(column, gtk_check_menu_item_get_active(checkmenuitem) );
	//lst_accview_columns_prefs_get(GTK_TREE_VIEW(treeview));
}


static gint
lst_accview_cb_button_pressed_event (GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{

	DB( g_print ("\n[lst_accview] popmenu\n") );
	
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
lst_accview_popmenu_destroy(GtkTreeView *treeview, gpointer user_data)
{
	DB( g_print ("\n[lst_accview] menu destroy\n") );

}


static GtkWidget *
lst_accview_popmenu_new(GtkTreeView *treeview)
{
GtkTreeViewColumn *column;
GtkWidget *menu;
GtkWidget *menuitem;
gint i, uid;
//GtkAccelGroup *accel_group = NULL;

	//accel_group = gtk_accel_group_new();
	//gtk_window_add_accel_group(GTK_WINDOW(data->window), accel_group);

	DB( g_print ("\n[lst_accview] create popmenu\n") );
	
	menu = gtk_menu_new();
	//data->ME_popmenu = menu;

	for(i=0 ; i < NUM_LST_COL_DSPACC ; i++ )
	{
		column = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), i);
		if( column != NULL )
		{
			//uid = gtk_tree_view_column_get_sort_column_id (column);
			uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			DB( g_print (" col %d\n", uid) );
			
			if( (uid == -1) 
				|| (uid == COL_DSPACC_STATUS) 
				|| (uid == COL_DSPACC_ACCOUNTS) 
			)
				continue;

			menuitem = gtk_check_menu_item_new_with_label ( gtk_tree_view_column_get_title (column) );
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), gtk_tree_view_column_get_visible (column) );

			g_signal_connect (menuitem, "activate",
								G_CALLBACK (lst_accview_popmenu_cb_activate), column);

		}
	}
	
	g_signal_connect (menu, "destroy", G_CALLBACK (lst_accview_popmenu_destroy), NULL);
	
	gtk_widget_show_all(menu);

	return menu;
}


static gboolean
gtk_tree_view_set_tooltip_query_cb (GtkWidget  *widget,
				    gint        x,
				    gint        y,
				    gboolean    keyboard_tip,
				    GtkTooltip *tooltip,
				    gpointer    data)
{
GtkTreeIter iter;
GtkTreePath *path;
GtkTreeModel *model;
GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
GString *node;
gint dt;
Account *acc;
gboolean hastext = FALSE;
gchar *escapedname;

	if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (widget),
					  &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
	return FALSE;

	gtk_tree_model_get(model, &iter,
		LST_DSPACC_DATATYPE, &dt,
		LST_DSPACC_DATAS, &acc,
		-1);

	// no tooltip if not an account
	if( (dt != DSPACC_TYPE_NORMAL) )
	{
		gtk_tree_path_free (path);
		return FALSE;
	}

	//#1993599 tooltip fail when & into acc name
	escapedname = g_markup_escape_text(acc->name, -1);
	node = g_string_sized_new(255);
	g_string_append(node, "<b>");
	g_string_append(node, escapedname);
	g_string_append(node, "</b>\n");
	g_free(escapedname);

	if( acc->rdate > 0 )
	{
	gchar buffer[256];
	GDate *date;

		//format date
		date = g_date_new_julian (acc->rdate);
		g_date_strftime (buffer, 256-1, PREFS->date_format, date);
		g_date_free(date);
		
		g_string_append(node, "\n");
		g_string_append(node, _("last reconciled"));
		g_string_append(node, ": ");
		g_string_append(node, buffer);
		hastext = TRUE;
	}

	if( (acc->minimum < 0) && (acc->bal_today >= acc->minimum) )
	{
	gchar formatd_buf[G_ASCII_DTOSTR_BUF_SIZE];

		g_string_append(node, "\n");
		hb_strfmon(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, acc->bal_today - acc->minimum, acc->kcur, FALSE);
		g_string_append(node, formatd_buf);
		g_string_append(node, " ");
		g_string_append(node, _("until overdraft"));
		g_string_append(node, " (");
		hb_strfmon(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, acc->minimum, acc->kcur, FALSE);
		g_string_append(node, formatd_buf);
		g_string_append(node, ")");
		hastext = TRUE;
	}

	if( (acc->maximum > 0) && (acc->maximum >= acc->bal_today) )
	{
	gchar formatd_buf[G_ASCII_DTOSTR_BUF_SIZE];

		g_string_append(node, "\n");
		hb_strfmon(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, acc->maximum - acc->bal_today, acc->kcur, FALSE);
		g_string_append(node, formatd_buf);
		g_string_append(node, " ");
		g_string_append(node, _("before maximum"));
		g_string_append(node, " (");
		hb_strfmon(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, acc->maximum, acc->kcur, FALSE);
		g_string_append(node, formatd_buf);
		g_string_append(node, ")");
		hastext = TRUE;
	}

	if( hastext == FALSE )
	{
		gtk_tree_path_free (path);
		g_string_free(node, TRUE);
		return FALSE;
	}

	gtk_tooltip_set_markup (tooltip, node->str);
	gtk_tree_view_set_tooltip_row (tree_view, tooltip, path);

	gtk_tree_path_free (path);
	g_string_free(node, TRUE);

	return TRUE;
}


static void
lst_accview_destroy(GtkWidget *widget, gpointer user_data)
{
struct lst_accview_data *data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print ("\n[lst_accview] destroy\n") );

	lst_accview_columns_prefs_get(GTK_TREE_VIEW(data->treeview));

	gtk_widget_destroy (data->menu);
}


GtkWidget *lst_accview_new(void)
{
struct lst_accview_data *data;
GtkTreeStore *store;
GtkWidget *treeview;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	DB( g_print ("\n[lst_accview] create\n") );

	data = g_malloc0(sizeof(struct lst_accview_data));
	if(!data) return NULL;

	
	/* create list store */
	store = gtk_tree_store_new(
	  	NUM_LST_DSPACC,
		G_TYPE_INT,		/* datatype */
		G_TYPE_INT,		/* pos */
		G_TYPE_POINTER  /* datas*/
		);

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	data->treeview = treeview;
	g_object_unref(store);

	//store our window private data
	g_object_set_data(G_OBJECT(treeview), "inst_data", (gpointer)data);
	DB( g_print(" - treeview=%p, inst_data=%p\n", treeview, data) );

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);
	//gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
	//			       COLUMN_DESCRIPTION);

	//gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW (view), TRUE);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_SINGLE);

    /* Status */
	column = gtk_tree_view_column_new();
    //gtk_tree_view_column_set_title(column, _("Status"));

    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, lst_accview_cell_data_func_status, GINT_TO_POINTER(1), NULL);

    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, lst_accview_cell_data_func_status, GINT_TO_POINTER(2), NULL);

	gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//add system icon to 1st column
	gtk_tree_view_column_set_clickable(column, TRUE);
	GtkWidget *img = gtk_image_new_from_icon_name (ICONNAME_EMBLEM_SYSTEM, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(img);
	gtk_tree_view_column_set_widget(column, img);

	/* Account */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
		"ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
		NULL);

	column = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_DSPACC_ACCOUNTS));
	gtk_tree_view_column_set_title(column, _("Accounts"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_accview_cell_data_func_text, NULL, NULL);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST/2);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	gtk_tree_view_column_set_fixed_width(column, PREFS->pnl_acc_col_acc_width);
	gtk_tree_view_set_expander_column(GTK_TREE_VIEW (treeview), column);

    /* Reconciled */
	column = lst_accview_column_amount_new(_("Reconciled"), COL_DSPACC_RECON);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

    /* Cleared */
	column = lst_accview_column_amount_new(_("Cleared"), COL_DSPACC_CLEAR);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

    /* Today */
	column = lst_accview_column_amount_new(_("Today"), COL_DSPACC_TODAY);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

    /* Future */
	column = lst_accview_column_amount_new(_("Future"), COL_DSPACC_FUTURE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

    /* column 7: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	/* disable selection for level 1 of the tree */
	
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), lst_accview_func_selection, NULL, NULL);
	
	//sort etc
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), lst_accview_func_compare, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	lst_accview_columns_prefs_set(GTK_TREE_VIEW(treeview), PREFS->lst_acc_columns);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	GtkWidget *popmenu = lst_accview_popmenu_new(GTK_TREE_VIEW(treeview));
	data->menu = popmenu;

	//add tooltip
	gtk_widget_set_has_tooltip (GTK_WIDGET (treeview), TRUE);

	g_signal_connect (treeview, "destroy", G_CALLBACK (lst_accview_destroy), NULL);

	g_signal_connect (treeview, "button-press-event", G_CALLBACK (lst_accview_cb_button_pressed_event), popmenu);

	g_signal_connect (treeview, "query-tooltip",
		            G_CALLBACK (gtk_tree_view_set_tooltip_query_cb), NULL);


	return(treeview);
}

