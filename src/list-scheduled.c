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


extern HbKvData CYA_ARC_UNIT[];


/* = = = = = = = = = = = = = = = = */


#if MYDEBUG == 1
static void
ui_arc_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DSPUPC_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	



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
lst_sch_cell_data_func_date (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;
gchar buffer[256];

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
		-1);

	if( arc != NULL )
	{
	GDate *date = g_date_new_julian (arc->nextdate);
		g_date_strftime (buffer, 256-1, PREFS->date_format, date);
		g_date_free(date);
		text = buffer;
	}
	g_object_set(renderer, "text", text, NULL);
}


static void 
lst_sch_cell_data_func_payee (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
		-1);

	if( arc != NULL )
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

	gtk_tree_model_get(model, iter,
		LST_DSPUPC_DATAS, &arc,
		-1);
		
	if( arc != NULL )
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
ui_arc_listview_cell_data_function_memo (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DSPUPC_DATAS, &arc, -1);
	if(arc)
	{
		text = arc->memo;
	}
	g_object_set(renderer, "text", text, NULL);
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
ui_arc_listview_cell_data_function_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gdouble amount;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
Account *acc;
gchar *color;
gint weight;

	gtk_tree_model_get(model, iter, 
		LST_DSPUPC_DATAS, &arc,
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


static void 
ui_arc_listview_cell_data_func_info (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *iconname = NULL;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, 
		LST_DSPUPC_DATAS, &arc,
	    -1);


	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			if( arc != NULL )
				iconname = (arc->flags & OF_INTXFER) ? ICONNAME_HB_PM_INTXFER : (gchar *)get_paymode_icon_name(arc->paymode);
			g_object_set(renderer, "icon-name", iconname, NULL);
			break;
		case 2:
			//list_txn_eval_future(renderer, ope);
			if( arc != NULL )
				text = arc->number;
			#if MYDEBUG
			if( arc != NULL )
			{
				gchar *ds = g_strdup_printf ("%s kx[%d] f[%d]", text == NULL ? "" : text, arc->kxferacc, arc->flags );
				g_object_set(renderer, "text", ds, NULL);
				g_free(ds);
			}
			#else
				g_object_set(renderer, "text", text, NULL);
			#endif
			break;
	}
}


static void 
ui_arc_listview_cell_data_func_clr (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *iconname = NULL;
//const gchar *c = "";

	gtk_tree_model_get(model, iter, 
		LST_DSPUPC_DATAS, &arc,
		 -1);

	if( arc != NULL )
	{
		switch(GPOINTER_TO_INT(user_data))
		{
			case 2:
			{
				switch(arc->status)
				{
					/*case TXN_STATUS_CLEARED: c = "c"; break;
					case TXN_STATUS_RECONCILED: c = "R"; break;
					case TXN_STATUS_REMIND: c = "!"; break;*/
					case TXN_STATUS_CLEARED:	iconname = ICONNAME_HB_OPE_CLEARED; break;
					case TXN_STATUS_RECONCILED: iconname = ICONNAME_HB_OPE_RECONCILED; break;
					case TXN_STATUS_REMIND:     iconname = ICONNAME_HB_OPE_REMIND; break;
					case TXN_STATUS_VOID:       iconname = ICONNAME_HB_OPE_VOID; break;		
				}
				break;
			}
		}
	}

	//TODO 5.6 after switch to on the change prevent do not display, maybe gtk bug
	//DB( g_print("\n[list_txn] clr lockrecon=%d, icon=%s", data->lockreconciled, iconname) );

	//g_object_set(renderer, "text", c, NULL);
	g_object_set(renderer, "icon-name", iconname, NULL);
}


static void 
lst_sch_cell_data_func_account (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *arc;
gchar *text = NULL;

	gtk_tree_model_get(model, iter, LST_DSPUPC_DATAS, &arc, -1);
	if( arc != NULL )
	{
	Account *acc = da_acc_get(arc->kacc);

		if(acc != NULL)
			text = acc->name;
	}
	g_object_set(renderer, "text", text, NULL);
}


/*
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

	gtk_tree_model_get(model, a, LST_DSPUPC_DATAS, &item1, -1);
	gtk_tree_model_get(model, b, LST_DSPUPC_DATAS, &item2, -1);

    switch (sortcol)
    {
		case COL_SCH_UID_NEXTDATE:
			retval = item1->nextdate - item2->nextdate;
			//#2024956 default to next date
			if(!retval)
				goto domemo;
			break;

		case COL_SCH_UID_MEMO:
domemo:		retval = (item1->flags & GF_INCOME) - (item2->flags & GF_INCOME);
			if(!retval)
			{
				retval = hb_string_utf8_compare(item1->memo, item2->memo);
			}
			break;
			
		case COL_SCH_UID_PAYNUMBER:
			if(!(retval = item1->paymode - item2->paymode))
			{
				retval = hb_string_utf8_compare(item1->number, item2->number);
			}
			break;

		case COL_SCH_UID_PAYEE:
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

		case COL_SCH_UID_CATEGORY:
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

		case COL_SCH_UID_CLR:
			retval = item1->status - item2->status;
			break;

		case COL_SCH_UID_AMOUNT:
			tmpval = item1->amount - item2->amount;
			retval = tmpval > 0 ? 1 : -1;
			break;
		//#1928147 sort on account as well
		case COL_SCH_UID_ACCOUNT:
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
ui_arc_listview_cell_data_func_icons (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *item;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, LST_DSPUPC_DATAS, &item, -1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			iconname = ( item->flags & OF_PREFILLED  ) ? ICONNAME_HB_OPE_PREFILLED : NULL;
			break;
	}

	g_object_set(renderer, "icon-name", iconname, NULL);
}


static void 
ui_arc_listview_cell_data_function_auto (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *item;
gchar *info, *iconname;

	// get the transaction
	gtk_tree_model_get(model, iter, LST_DSPUPC_DATAS, &item, -1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			iconname = ( item->flags & OF_AUTO ) ? ICONNAME_HB_OPE_AUTO : NULL;
			g_object_set(renderer, "icon-name", iconname, NULL);
			break;
		case 2:
			info = NULL;
			//#1898294 not translated
			if( ( item->flags & OF_AUTO ) )
			   info = g_strdup_printf("%d %s", item->every, hbtk_get_label(CYA_ARC_UNIT, item->unit));

			g_object_set(renderer, "text", info, NULL);

			g_free(info);
			break;
	}
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static gboolean
ui_arc_listview_func_column_drop (GtkTreeView *tree_view,
                              GtkTreeViewColumn *column,
                              GtkTreeViewColumn *prev_column,
                              GtkTreeViewColumn *next_column,
                              gpointer data)
{
gboolean retval = FALSE;

	//DB( g_print ("\n[lst_sch] column drop %p %p\n", prev_column, next_column) );
	if( prev_column != NULL )
	{
	gint coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(prev_column), "uid"));
		retval = coluid < COL_SCH_UID_PAYNUMBER ? FALSE : TRUE;
		//DB( g_print (" coluid=%d > %d\n", coluid, retval) );
	}
	return retval;
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
lst_sch_cb_button_pressed_event (GtkWidget *widget, GdkEvent *event, GtkWidget *menu)
{
GdkEventType type;
guint button = 0;

	type = gdk_event_get_event_type(event);
	gdk_event_get_button(event, &button);

	DB( g_print ("\n[lst_sch] popmenu pop\n") );
	
	if (type == GDK_BUTTON_PRESS && button == 3)
	{

		// check we ARE in the header but in bin window
		if (gdk_event_get_window(event) != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)))
		{
			#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
				gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
			#else
				gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, button, gdk_event_get_time(event));
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
GtkWidget *menu, *menuitem;
guint i;

	DB( g_print ("\n[lst_sch] create popmenu\n") );

	menu = gtk_menu_new();
	//data->ME_popmenu = menu;

	for(i=0 ; i < gtk_tree_view_get_n_columns(GTK_TREE_VIEW(treeview)) - 1 ; i++ )
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), i);

		if( column != NULL )
		{
		gint uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));


			if( (uid == COL_SCH_UID_PAYEE)
			 || (uid == COL_SCH_UID_CATEGORY)
			 || (uid == COL_SCH_UID_MEMO) )
			{
				menuitem = gtk_check_menu_item_new_with_label ( gtk_tree_view_column_get_title (column) );
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), gtk_tree_view_column_get_visible (column) );
				g_signal_connect (menuitem, "activate", G_CALLBACK (lst_sch_popmenu_cb_activate), column);

				DB( g_print (" [%02d] uid=%02d add\n", i, uid) );
			}
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

	for(i=COL_SCH_UID_PAYEE;i<gtk_tree_view_get_n_columns(GTK_TREE_VIEW(data->treeview));i++)
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(data->treeview), i);

		if(column)
		{
		gint coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			switch(coluid)
			{
				case COL_SCH_UID_PAYEE:
					gtk_tree_view_column_set_visible(column, PREFS->pnl_upc_col_pay_show);
					gtk_tree_view_column_set_fixed_width(column, PREFS->pnl_upc_col_pay_width);
					break;
				case COL_SCH_UID_CATEGORY:
					gtk_tree_view_column_set_visible(column, PREFS->pnl_upc_col_cat_show);
					gtk_tree_view_column_set_fixed_width(column, PREFS->pnl_upc_col_cat_width);
					break;
				case COL_SCH_UID_MEMO:
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

	for(i=COL_SCH_UID_PAYEE;i<gtk_tree_view_get_n_columns(GTK_TREE_VIEW(data->treeview));i++)
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(data->treeview), i);

		if(column)
		{
		gint coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			//#1830656 use xxx_get_fixed_width instead of width, as if not visible will save 0 otherwise
			switch(coluid)
			{
				case COL_SCH_UID_PAYEE:
					PREFS->pnl_upc_col_pay_show  = gtk_tree_view_column_get_visible(column);
					PREFS->pnl_upc_col_pay_width = gtk_tree_view_column_get_fixed_width(column);
					break;
				case COL_SCH_UID_CATEGORY:
					PREFS->pnl_upc_col_cat_show  = gtk_tree_view_column_get_visible(column);
					PREFS->pnl_upc_col_cat_width = gtk_tree_view_column_get_fixed_width(column);
					break;
				case COL_SCH_UID_MEMO:
					PREFS->pnl_upc_col_mem_show  = gtk_tree_view_column_get_visible(column);
					PREFS->pnl_upc_col_mem_width = gtk_tree_view_column_get_fixed_width(column);
					break;
			}
		}
	}
}


// test 5.8
static GtkTreeViewColumn *
ui_arc_listview_widget_get_column_by_uid(GtkTreeView *treeview, gint uid)
{
GtkTreeViewColumn *column;
guint i;
gint coluid;

	for(i=0;i<gtk_tree_view_get_n_columns(treeview)-1;i++)	//-1 to avoid last empty column
	{
		column = gtk_tree_view_get_column(treeview, i);
		coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));
		if( coluid == uid )
			return column;

	}
	return NULL;
}


void
ui_arc_listview_widget_columns_order_load(GtkTreeView *treeview)
{
GtkTreeViewColumn *base_column;
gint i, *colpos_ptr = PREFS->lst_sch_columns;

	DB( g_print ("\n[lst_sch] columns set order\n") );

	base_column = ui_arc_listview_widget_get_column_by_uid(treeview, COL_SCH_UID_NEXTDATE);
	for(i=0;i<NUM_COL_SCH_UID;i++)
	{
	GtkTreeViewColumn *column;
	
		if( colpos_ptr[i] <= 0 )
			break;
	
		column = ui_arc_listview_widget_get_column_by_uid(treeview, colpos_ptr[i]);
		if( column != NULL )
		{
			DB( g_print (" >move %d '%s' after %d '%s'\n", 
				colpos_ptr[i],
				gtk_tree_view_column_get_title(column),
				GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(base_column), "uid")),
				gtk_tree_view_column_get_title(base_column)
				) );
		
			gtk_tree_view_move_column_after(treeview, column, base_column);
			base_column = column;
		}
	}
}


void
ui_arc_listview_widget_columns_order_save(GtkTreeView *treeview)
{
guint i;
gint *colpos_ptr = PREFS->lst_sch_columns;

	DB( g_print ("\n[lst_arc] columns get order\n") );
	
	for(i=0;i<gtk_tree_view_get_n_columns(treeview)-1;i++)	//-1 to avoid empty column
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column(treeview, i);

		if(column)
		{
		gint coluid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(column), "uid"));

			DB( g_print (" [%02d] uid=%02d %s\n", i, coluid, gtk_tree_view_column_get_title(column)) );

			if( coluid >= COL_SCH_UID_PAYNUMBER )
			{
				DB( g_print(" > store %d\n", coluid) );
				*colpos_ptr++ = coluid;
				//as amount is split into upcoming, fill in exp/inc after
				if( coluid == COL_SCH_UID_AMOUNT )
				{
					*colpos_ptr++ = COL_SCH_UID_EXPENSE;
					*colpos_ptr++ = COL_SCH_UID_INCOME;
				}
			}
		}
	}

	*colpos_ptr = 0;
}


static GtkTreeViewColumn *
ui_arc_listview_column_info_create(gint sortcolumnid)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Pay./Number"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_info, GINT_TO_POINTER(1), NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_info, GINT_TO_POINTER(2), NULL);

	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);

	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_PAYNUMBER));
	if( sortcolumnid > 0 )	
		gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_PAYNUMBER);

	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);

	return column;
}



static GtkTreeViewColumn *
lst_sch_listview_column_text_create(gchar *title, gint uid, GtkTreeCellDataFunc func, gpointer user_data)
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

	if( uid > 0 )
		g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(uid));

	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	//gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	//gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, func, user_data, NULL);

	return column;
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
lst_sch_widget_new(gint listtype)
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


	/* column: Next Date */
	renderer = gtk_cell_renderer_text_new ();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Next date"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_date, NULL, NULL);
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_NEXTDATE));
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// info column
	column = ui_arc_listview_column_info_create(-1);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Payee */
	column = lst_sch_listview_column_text_create(_("Payee"), COL_SCH_UID_PAYEE, lst_sch_cell_data_func_payee, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Category */
	column = lst_sch_listview_column_text_create(_("Category"), COL_SCH_UID_CATEGORY, lst_sch_cell_data_func_category, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column: Memo */
	column = lst_sch_listview_column_text_create(_("Memo"), COL_SCH_UID_MEMO, lst_sch_cell_data_func_memo, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// status column
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Status"));

	//renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_clr, GINT_TO_POINTER(1), NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_clr, GINT_TO_POINTER(2), NULL);

	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_CLR));
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
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_EXPENSE));
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
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_INCOME));
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Account */
	column = lst_sch_listview_column_text_create(_("Account"), COL_SCH_UID_ACCOUNT, lst_sch_cell_data_func_account, NULL);
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


GtkWidget *ui_arc_listview_widget_new(void)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	//store
	store = gtk_list_store_new (
		1,
		G_TYPE_POINTER /* scheduled */
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

	// column: icons
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_icons, GINT_TO_POINTER(1), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


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

	/* column: Date Next on */
	renderer = gtk_cell_renderer_text_new ();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Next date"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_sch_cell_data_func_date, NULL, NULL);
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_NEXTDATE));
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_NEXTDATE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// info column
	column = ui_arc_listview_column_info_create(COL_SCH_UID_PAYNUMBER);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Payee */
	column = lst_sch_listview_column_text_create(_("Payee"), COL_SCH_UID_PAYEE, lst_sch_cell_data_func_payee, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_PAYEE);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Category */
	column = lst_sch_listview_column_text_create(_("Category"), COL_SCH_UID_CATEGORY, lst_sch_cell_data_func_category, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_CATEGORY);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Memo */
	column = lst_sch_listview_column_text_create(_("Memo"), COL_SCH_UID_MEMO, ui_arc_listview_cell_data_function_memo, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_MEMO);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// status column
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Status"));

	//renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_clr, GINT_TO_POINTER(1), NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_func_clr, GINT_TO_POINTER(2), NULL);

	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_CLR));
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_CLR);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column : amount */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Amount"));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_cell_data_function_amount, NULL, NULL);
	g_object_set_data(G_OBJECT(column), "uid", GUINT_TO_POINTER(COL_SCH_UID_AMOUNT));
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_AMOUNT);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	/* column : Account */
	column = lst_sch_listview_column_text_create(_("Account"), COL_SCH_UID_ACCOUNT, lst_sch_cell_data_func_account, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_SCH_UID_ACCOUNT);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  	/* column : empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//sortable
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_NEXTDATE, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_NEXTDATE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_PAYNUMBER, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_PAYNUMBER), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_PAYEE, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_PAYEE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_CATEGORY, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_CATEGORY), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_CLR, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_CLR), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_AMOUNT, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_AMOUNT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_MEMO, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_MEMO), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), COL_SCH_UID_ACCOUNT, ui_arc_listview_compare_func, GINT_TO_POINTER(COL_SCH_UID_ACCOUNT), NULL);

	//#2024956 default to next date
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), COL_SCH_UID_MEMO, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), COL_SCH_UID_NEXTDATE, GTK_SORT_ASCENDING);

	//gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(view), FALSE);
	//gtk_tree_view_set_reorderable (GTK_TREE_VIEW(view), TRUE);

	gtk_tree_view_set_column_drag_function(GTK_TREE_VIEW(treeview), ui_arc_listview_func_column_drop, NULL, NULL);

	return(treeview);
}

