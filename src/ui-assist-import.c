/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2025 Maxime DOYEN
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

#include "hub-account.h"
#include "hb-import.h"
#include "ui-assist-import.h"
#include "dsp-mainwindow.h"
#include "list-operation.h"

#include "ui-widgets.h"

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


extern HbKvData CYA_IMPORT_DATEORDER[];
extern HbKvData CYA_IMPORT_OFXNAME[];
extern HbKvData CYA_IMPORT_OFXMEMO[];


static void ui_import_page_filechooser_eval(GtkWidget *widget, gpointer user_data);


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


#if MYDEBUG == 1
static void list_txn_cell_data_function_debug (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenTxn *gentxn;
gchar *text;

	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &gentxn, 
		-1);

	text = g_strdup_printf("%d %d > %d", gentxn->is_imp_similar, gentxn->is_dst_similar, gentxn->to_import);
	
	g_object_set(renderer, 
		"text", text, 
		NULL);
	
	g_free(text);
}
#endif


static void list_txn_cell_data_function_toggle (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenTxn *gentxn;

	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &gentxn, 
		-1);

	g_object_set(renderer, "active", gentxn->to_import, NULL);
}


static void list_txn_cell_data_function_warning (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenTxn *gentxn;
gchar *iconname = NULL;

	// get the transaction
	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &gentxn, 
		-1);

	//iconname = ( gentxn->julian == 0 ) ? ICONNAME_WARNING : NULL;
	//if(iconname == NULL)
	iconname = ( gentxn->is_dst_similar || gentxn->is_imp_similar ) ? ICONNAME_HB_ITEM_SIMILAR : NULL;

	g_object_set(renderer, "icon-name", iconname, NULL);
}


static void list_txn_cell_data_function_error (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenTxn *gentxn;
gchar *iconname = NULL;

	// get the transaction
	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &gentxn, 
		-1);

	iconname = ( gentxn->julian == 0 ) ? ICONNAME_ERROR : NULL;

	g_object_set(renderer, "icon-name", iconname, NULL);
}


static void list_txn_cell_data_function_text (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gint colid = GPOINTER_TO_INT(user_data);
gchar buf[12];
GDate date;
gchar *text = "";
GenTxn *item;

	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &item, 
		-1);

	switch(colid)
	{
		case LST_DSPOPE_DATE: //date
			{
			gchar *color = NULL;
			
				if(item->julian > 0)
				{	
					g_date_set_julian(&date, item->julian);
					//#1794170 %F is ignored under ms windows
					//g_date_strftime (buf, 12-1, "%F", &date);
					g_date_strftime (buf, 12-1, "%Y-%m-%d", &date);
					text = buf;
				}
				else
				{
					text = item->date;
					color = PREFS->color_warn;
				}

				g_object_set(renderer, 
					"foreground", color,
					NULL);
			}
			//g_object_set(renderer, "text", item->date, NULL);
			break;
		case LST_DSPOPE_MEMO: //memo
			text = item->memo;
			break;
		case LST_DSPOPE_PAYEE: //payee
			text = item->payee;
			break;
		case LST_DSPOPE_CATEGORY: //category
			text = item->category;
			break;
	}

	g_object_set(renderer, 
		"text", text, 
		//"scale-set", TRUE,
		//"scale", item->to_import ? 1.0 : 0.8,
		"strikethrough-set", TRUE,
		"strikethrough", item->to_import ? FALSE : TRUE,
		NULL);

}


/*
** amount cell function
*/
static void list_txn_cell_data_function_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenTxn *item;
gchar formatd_buf[G_ASCII_DTOSTR_BUF_SIZE];
gdouble amount;
gchar *color;

	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &item, 
		-1);

	//#1866456
	amount = (item->togamount == TRUE) ? -item->amount : item->amount;

	//todo: we could use digit and currency of target account
	//hb_strfnum(buf, G_ASCII_DTOSTR_BUF_SIZE-1, item->amount, GLOBALS->kcur, FALSE);
	//hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, ope->amount, GLOBALS->minor);
	g_ascii_formatd(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, "%.2f", amount);

	color = get_normal_color_amount(amount);

	g_object_set(renderer,
			"foreground",  color,
			"text", formatd_buf,
			NULL);

}


static void list_txn_cell_data_function_info (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenTxn *item;

	gtk_tree_model_get(model, iter, 
		LST_GENTXN_POINTER, &item, 
		-1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			g_object_set(renderer, "icon-name", get_paymode_icon_name(item->paymode), NULL);
			break;
		case 2:
		    g_object_set(renderer, "text", item->number, NULL);
			break;
	}
}


static void list_txn_importfixed_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer userdata)
{
GtkTreeView *treeview;
GtkTreeModel *model;
GtkTreeIter  iter;
GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
GenTxn *gentxn;

	g_return_if_fail(GTK_IS_TREE_VIEW(userdata));

	treeview = userdata;
	model = gtk_tree_view_get_model(treeview);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, LST_GENTXN_POINTER, &gentxn, -1);
	gentxn->to_import ^= 1;
	gtk_tree_path_free (path);

	//#1993727 no update after toggle
	//fake a treeview changed signal
	g_signal_emit_by_name(gtk_tree_view_get_selection(treeview), "changed", NULL);
}


static gint list_txn_import_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
GenTxn *gentxn1, *gentxn2;

    gtk_tree_model_get(model, a, LST_GENTXN_POINTER, &gentxn1, -1);
    gtk_tree_model_get(model, b, LST_GENTXN_POINTER, &gentxn2, -1);

	switch(sortcol)
	{
		case LST_DSPOPE_MEMO:
			retval = hb_string_utf8_compare(gentxn1->memo, gentxn2->memo);
			break;
		case LST_DSPOPE_AMOUNT:
			retval = (gentxn1->amount - gentxn2->amount) > 0 ? 1 : -1;
			break;
		case LST_DSPOPE_PAYEE:
			retval = hb_string_utf8_compare(gentxn1->payee, gentxn2->payee);
			break;
		case LST_DSPOPE_CATEGORY:
			retval = hb_string_utf8_compare(gentxn1->category, gentxn2->category);
			break;
		case LST_DSPOPE_DATE:
		default:
			retval = gentxn1->julian - gentxn2->julian;
			break;
	}

    return retval;
}


static GtkTreeViewColumn *
list_txn_import_column_text_create(gchar *title, gint sortcolumnid, gpointer user_data)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	renderer = gtk_cell_renderer_text_new ();
	/*g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);*/
	if( sortcolumnid == LST_DSPOPE_AMOUNT )
	   g_object_set(renderer, "xalign", 1.0, NULL);
	
	column = gtk_tree_view_column_new_with_attributes(title, renderer, NULL);
	//#2004631 date and column title alignement
	if( sortcolumnid == LST_DSPOPE_AMOUNT )
		gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);

	if(sortcolumnid == LST_DSPOPE_AMOUNT )
		gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_amount, GINT_TO_POINTER(sortcolumnid), NULL);
	else
		gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_text, GINT_TO_POINTER(sortcolumnid), NULL);

	return column;
}



static GtkWidget *list_txn_import_create(void)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	/* create list store */
	store = gtk_list_store_new(
	  	NUM_LST_GENTXN,
		G_TYPE_POINTER
		);

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines|GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	// debug/import checkbox
	column = gtk_tree_view_column_new();
	#if MYDEBUG  == 1
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_debug, NULL, NULL);
	#endif
	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_toggle, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	g_signal_connect (renderer, "toggled", G_CALLBACK (list_txn_importfixed_toggled), treeview);

	// icons
	column = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(column, _("Import ?"));
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, 16, -1);
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_warning, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// date	
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_error, NULL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_text, GINT_TO_POINTER(LST_DSPOPE_DATE), NULL);
	gtk_tree_view_column_set_title (column, _("Date"));
	gtk_tree_view_column_set_sort_column_id (column, LST_DSPOPE_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// memo
	column = list_txn_import_column_text_create(_("Memo"), LST_DSPOPE_MEMO, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// amount
	column = list_txn_import_column_text_create(_("Amount"), LST_DSPOPE_AMOUNT, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// info
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Pay./Number"));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_info, GINT_TO_POINTER(1), NULL);
	renderer = gtk_cell_renderer_text_new ();
	/*g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);*/
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_cell_data_function_info, GINT_TO_POINTER(2), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// payee
	column = list_txn_import_column_text_create(_("Payee"), LST_DSPOPE_PAYEE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// category
	column = list_txn_import_column_text_create(_("Category"), LST_DSPOPE_CATEGORY, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// empty 
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), list_txn_import_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_DATE    , list_txn_import_compare_func, GINT_TO_POINTER(LST_DSPOPE_DATE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_MEMO    , list_txn_import_compare_func, GINT_TO_POINTER(LST_DSPOPE_MEMO), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_AMOUNT  , list_txn_import_compare_func, GINT_TO_POINTER(LST_DSPOPE_AMOUNT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_PAYEE   , list_txn_import_compare_func, GINT_TO_POINTER(LST_DSPOPE_PAYEE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_CATEGORY, list_txn_import_compare_func, GINT_TO_POINTER(LST_DSPOPE_CATEGORY), NULL);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	return(treeview);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static gint ui_genacc_comboboxtext_get_active(GtkWidget *widget)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gint key = -1;

	g_return_val_if_fail(GTK_IS_COMBO_BOX(widget), key);

	if( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
	{
		model = gtk_combo_box_get_model (GTK_COMBO_BOX(widget));

		gtk_tree_model_get(model, &iter,
			LST_GENACC_KEY, &key,
			-1);
	}
	return key;
}


static void ui_genacc_comboboxtext_set_active(GtkWidget *widget, gint active_key)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gint key;

	g_return_if_fail(GTK_IS_COMBO_BOX(widget));

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		gtk_tree_model_get(model, &iter,
			LST_GENACC_KEY, &key,
			-1);
		if(key == active_key)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX(widget), &iter);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static GtkWidget *ui_genacc_comboboxtext_new(struct import_data *data, GtkWidget *label)
{
GtkListStore *store;
GtkCellRenderer *renderer;
GtkWidget *combobox;
GtkTreeIter  iter;
GList *lacc, *list;

	store = gtk_list_store_new (NUM_LST_GENACC, G_TYPE_STRING, G_TYPE_INT);
	combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL(store));
	
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combobox), renderer, "text", LST_GENACC_NAME);

	g_object_unref(store);

	gtk_list_store_insert_with_values (GTK_LIST_STORE(store), &iter, -1,
			LST_GENACC_NAME, _("<New account (global)>"),
			LST_GENACC_KEY, DST_ACC_GLOBAL,
			-1);

	gtk_list_store_insert_with_values (GTK_LIST_STORE(store), &iter, -1,
			LST_GENACC_NAME, _("<New account>"),
			LST_GENACC_KEY, DST_ACC_NEW,
			-1);
	
	//#2030333 5.7 sort by pos
	lacc = list = account_glist_sorted(HB_GLIST_SORT_POS);
	while (list != NULL)
	{
	Account *item = list->data;
	
		if( !(item->flags & AF_CLOSED) )
		{
			gtk_list_store_insert_with_values (GTK_LIST_STORE(store), &iter, -1,
					LST_GENACC_NAME, item->name,
					LST_GENACC_KEY, item->key,
					-1);
		}
		list = g_list_next(list);
	}
	g_list_free(lacc);

	gtk_list_store_insert_with_values (GTK_LIST_STORE(store), &iter, -1,
			LST_GENACC_NAME, _("<Skip this account>"),
			LST_GENACC_KEY, DST_ACC_SKIP,
			-1);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), combobox);


	return combobox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


enum
{
	TARGET_URI_LIST
};

static GtkTargetEntry drop_types[] =
{
	{"text/uri-list", 0, TARGET_URI_LIST}
};


static void
list_file_add(GtkWidget *treeview, GenFile *genfile)
{
char *basename;
GtkTreeModel *model;
GtkTreeIter	iter;

	basename = g_path_get_basename(genfile->filepath);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter,
		LST_GENFILE_POINTER, genfile,
		LST_GENFILE_NAME, g_strdup(basename),
		-1);

	g_free(basename);
}


static void list_file_drag_data_received (GtkWidget *widget,
			GdkDragContext *context,
			gint x, gint y,
			GtkSelectionData *selection_data,
			guint info, guint time, GtkWindow *window)
{
struct import_data *data;
	gchar **uris, **str;
	gchar *newseldata;
	gint slen;

	if (info != TARGET_URI_LIST)
		return;

	DB( g_print("\n[ui-treeview] drag_data_received\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	/* On MS-Windows, it looks like `selection_data->data' is not NULL terminated. */
	slen = gtk_selection_data_get_length(selection_data);
	newseldata = g_new (gchar, slen + 1);
	memcpy (newseldata, gtk_selection_data_get_data(selection_data), slen);
	newseldata[slen] = 0;

	uris = g_uri_list_extract_uris (newseldata);

	ImportContext *ictx = &data->ictx;

	str = uris;
	for (str = uris; *str; str++)
	//if( *str )
	{
		GError *error = NULL;
		gchar *path = g_filename_from_uri (*str, NULL, &error);

		if (path)
		{
		GenFile *genfile;
		
			genfile = da_gen_file_append_from_filename(ictx, path);
			if(genfile)
				list_file_add(data->LV_file, genfile);
		}
		else
		{
			g_warning ("Could not convert uri to local path: %s", error->message);
			g_error_free (error);
		}
		g_free (path);
	}
	g_strfreev (uris);
	
	g_free(newseldata);
	
	ui_import_page_filechooser_eval(widget,  NULL);
}


static void
list_file_valid_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GenFile *genfile;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, 
		LST_GENFILE_POINTER, &genfile,
		-1);

	iconname = (genfile->filetype == FILETYPE_UNKNOWN) ? ICONNAME_HB_FILE_INVALID : ICONNAME_HB_FILE_VALID;
	
	g_object_set(renderer, "icon-name", iconname, NULL);
}


static GtkWidget *
list_file_new(void)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	// create list store
	store = gtk_list_store_new(NUM_LST_FILE,
		G_TYPE_POINTER,
		G_TYPE_STRING
		);

	// treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	//column: valid
	column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Valid"));
    renderer = gtk_cell_renderer_pixbuf_new ();
    //gtk_cell_renderer_set_fixed_size(renderer, 16, -1);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, list_file_valid_cell_data_function, NULL, NULL);
    //#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	g_object_set(renderer, "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
 
	// column: name
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                     renderer,
                                                     "text",
                                                     LST_GENFILE_NAME,
                                                     NULL);
	gtk_tree_view_column_set_sort_column_id (column, LST_GENFILE_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	
	// treeviewattribute
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), TRUE);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_acc_listview_compare_func, NULL, NULL);
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	gtk_drag_dest_set (GTK_WIDGET (treeview),
			   GTK_DEST_DEFAULT_ALL,
			   drop_types,
	           G_N_ELEMENTS (drop_types),
			   GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (treeview), "drag-data-received",
			  G_CALLBACK (list_file_drag_data_received), treeview);


	return treeview;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void ui_import_page_filechooser_delete_action(GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
ImportContext *ictx;
GtkTreeModel *model;
GtkTreeIter iter;
GtkTreeSelection *selection;

	DB( g_print("\n[ui-import] page_filechooser_delete_action\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	ictx = &data->ictx;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_file));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	GenFile *genfile;

		gtk_tree_model_get(model, &iter, LST_GENFILE_POINTER, &genfile, -1);

		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

		ictx->gen_lst_file = g_list_remove(ictx->gen_lst_file, genfile);
		da_gen_file_free(genfile);
	}

	ui_import_page_filechooser_eval(widget,  NULL);
}


static void ui_import_page_filechooser_add_action(GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
ImportContext *ictx;
GtkWidget *dialog;
GtkFileFilter *filter;
gint res;

	DB( g_print("\n[ui-import] page_filechooser_add_action\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	ictx = &data->ictx;

	dialog = gtk_file_chooser_dialog_new ("Open File",
		                                  GTK_WINDOW(data->assistant),
		                                  GTK_FILE_CHOOSER_ACTION_OPEN,
		                                  _("_Cancel"),
		                                  GTK_RESPONSE_CANCEL,
		                                  _("_Open"),
		                                  GTK_RESPONSE_ACCEPT,
		                                  NULL);

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), PREFS->path_import);
	DB( g_print(" set current folder '%s'\n", PREFS->path_import) );

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Known files"));
	gtk_file_filter_add_pattern (filter, "*.[Qq][Ii][Ff]");
	#ifndef NOOFX
	gtk_file_filter_add_pattern (filter, "*.[OoQq][Ff][Xx]");
	#endif
	gtk_file_filter_add_pattern (filter, "*.[Cc][Ss][Vv]");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);
	//if(data->filetype == FILETYPE_UNKNOWN)
	//	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("QIF files"));
	gtk_file_filter_add_pattern (filter, "*.[Qq][Ii][Ff]");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);
	//if(data->filetype == FILETYPE_QIF)
	//	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(dialog), filter);
	
	#ifndef NOOFX
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("OFX/QFX files"));
	gtk_file_filter_add_pattern (filter, "*.[OoQq][Ff][Xx]");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);
	//if(data->filetype == FILETYPE_OFX)
	//	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(dialog), filter);
	#endif

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("CSV files"));
	gtk_file_filter_add_pattern (filter, "*.[Cc][Ss][Vv]");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);
	//if(data->filetype == FILETYPE_CSV_HB)
	//	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(dialog), filter);
	
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);


	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
	GSList *list;

		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		list = gtk_file_chooser_get_filenames(chooser);
		while(list)
		{
		GenFile *genfile;

			DB( g_print("  selected '%p'\n", list->data) );

			genfile = da_gen_file_append_from_filename(ictx, list->data);
			if(genfile)
			list_file_add(data->LV_file, genfile);

			list = g_slist_next(list);
		}
		g_slist_free_full (list, g_free);

		/* remind folder to preference */
		gchar *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
		DB( g_print(" store folder '%s'\n", folder) );
		g_free(PREFS->path_import);
		PREFS->path_import = folder;
	}
	
	gtk_window_destroy (GTK_WINDOW(dialog));
	
	ui_import_page_filechooser_eval(widget,  NULL);
	
}


static void
ui_import_page_filechooser_visible (GtkWidget *widget, gpointer   user_data)
{
struct import_data *data;

	DB( g_print("\n[ui-import] page_filechooser_visible\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	
	//open the file add if no file
	if( gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_file)), NULL) == 0 )
	{
		//g_signal_emit_by_name(data->BT_file_add, "clicked", NULL);
		ui_import_page_filechooser_add_action(data->BT_file_add, NULL);
	}

}


static void ui_import_page_confirmation_fill(struct import_data *data)
{
ImportContext *ictx = &data->ictx;
GList *list;
GString *node;

	DB( g_print("\n[ui-import] page_confirmation_fill\n") );

	node = g_string_sized_new(255);

	list = g_list_first(ictx->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;
	gchar *targetname = NULL;

		switch( genacc->kacc )
		{
			case DST_ACC_GLOBAL:
				targetname = _("new global account");
				break;
			case DST_ACC_NEW:
				targetname = _("new account");
				break;
			case DST_ACC_SKIP:
				targetname = _("skipped");
				break;
			default:
			{
			Account *acc = da_acc_get (genacc->kacc);

				if(acc)
					targetname = acc->name;
			}
			break;
		}
				
		//line1: title
		g_string_append_printf(node, "<b>'%s'</b>\n => '%s'", genacc->name, targetname);

		//line2: count	
		if( genacc->kacc != DST_ACC_SKIP)
		{
			hb_import_gen_acc_count_txn(ictx, genacc);
			g_string_append_printf(node, _(", %d of %d transactions"), genacc->n_txnimp, genacc->n_txnall);
		}

		g_string_append(node, "\n\n");

		list = g_list_next(list);
	}

	gtk_label_set_markup (GTK_LABEL(data->TX_summary), node->str);

	g_string_free(node, TRUE);
}


static gboolean ui_import_page_import_eval(GtkWidget *widget, gpointer user_data)
{
//struct import_data *data;
//ImportContext *ictx;
//gint count;

	DB( g_print("\n[ui-import] page_import_eval\n") );


	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//ictx = &data->ictx;

	//count = g_list_length (ictx->gen_lst_acc);

	//DB( g_print(" count=%d (max=%d)\n", count, TXN_MAX_ACCOUNT) );

	//if( count <= TXN_MAX_ACCOUNT )
		return TRUE;

	//return FALSE;
}


static void ui_import_page_filechooser_eval(GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
ImportContext *ictx;
GList *list;
gint count = 0;

	DB( g_print("\n[ui-import] page_filechooser_eval\n") );


	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	ictx = &data->ictx;

	list = g_list_first(ictx->gen_lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;

		if(genfile->filetype != FILETYPE_UNKNOWN)
			count++;

		list = g_list_next(list);
	}	

	gint index = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	GtkWidget *current_page = gtk_assistant_get_nth_page (GTK_ASSISTANT(data->assistant), index);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), current_page, (count > 0) ? TRUE : FALSE);

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static ImpTxnData *ui_import_page_transaction_data_get(GArray *txndata, guint32 idx)
{
//gint i;
	
	/*g_print(" array @%p, len is %d\n", txndata, txndata->len);
	for(i=0;i<txndata->len;i++)
		g_print(" %d %p\n", i, &g_array_index(txndata, ImpTxnData, i) );

	g_print(" get idx=%d - %p\n", idx, &g_array_index (txndata, ImpTxnData, idx) );
	*/

	if( idx <= txndata->len )
		return &g_array_index (txndata, ImpTxnData, idx);
	return NULL;
}


//#1993727 no update after toggle
static void ui_import_page_transaction_update_count(struct import_data *data)
{
ImpTxnData *txndata;
ImportContext *ictx;
gchar *label = NULL;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gint count;

	DB( g_print("\n[ui-import] txn update count\n") );

	ictx = &data->ictx;

	//get the account, it will be the account into the glist
	gint acckey = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant)) - (PAGE_IMPORT);
	GenAcc *genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, acckey);
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);

	//count gentxn total and toimport
	count = 0;
	genacc->n_txnimp = 0;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(txndata->LV_gentxn));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	GenTxn *gentxn;

		gtk_tree_model_get(model, &iter, 
			LST_GENTXN_POINTER, &gentxn, 
			-1);

		count++;
		if(gentxn->to_import)
			genacc->n_txnimp++;

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	if(genacc->n_txnsimimp || genacc->n_txnsimdst)
		label = g_strdup_printf(_("%d transaction(s), %d similar, %d existing, %d selected"), count, genacc->n_txnsimimp, genacc->n_txnsimdst, genacc->n_txnimp);	
	else
		label = g_strdup_printf(_("%d transaction(s), %d selected"), count, genacc->n_txnimp);
	gtk_label_set_markup (GTK_LABEL(txndata->LB_txn_title), label);
	g_free(label);
}


static void ui_import_page_transaction_cb_fill_same(GtkTreeSelection *treeselection, gpointer user_data)
{
struct import_data *data;
ImpTxnData *txndata;
//ImportContext *ictx;
GtkTreeSelection *selection;
GtkTreeModel		 *model, *dupmodel;
GtkTreeIter			 iter, newiter;
GList *tmplist;
GtkWidget *widget;
guint count = 0;

	DB( g_print("\n[ui-import] page_transaction_cb_fill_same\n") );

	widget = GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection));

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//ictx = &data->ictx;

	gint pageidx = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	gint acckey =  pageidx - (PAGE_IMPORT);
	//GenAcc *genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, acckey);

	//txndata = &data->txndata[acckey];
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);

	ui_import_page_transaction_update_count(data);

	//update same
	dupmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(txndata->LV_duptxn));
	gtk_tree_store_clear (GTK_TREE_STORE(dupmodel));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(txndata->LV_gentxn));

	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	GenTxn *gentxn;

		gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &gentxn, -1);

		if( gentxn->lst_existing != NULL )
		{
			tmplist = g_list_first(gentxn->lst_existing);
			while (tmplist != NULL)
			{
			Transaction *tmp = tmplist->data;

				/* append to our treeview */
				//gtk_list_store_append (GTK_LIST_STORE(dupmodel), &newiter);
				//gtk_list_store_set (GTK_LIST_STORE(dupmodel), &newiter,
				//#1830523/#1840393
				count++;
				gtk_tree_store_insert_with_values(GTK_TREE_STORE(dupmodel), &newiter, NULL, -1,
					MODEL_TXN_POINTER, tmp,
				    MODEL_TXN_SPLITAMT, tmp->amount,
					-1);

				//DB( g_print(" - fill: %s %.2f %x\n", item->memo, item->amount, (unsigned int)item->same) );

				tmplist = g_list_next(tmplist);
			}
		}
	}

	gtk_expander_set_expanded (GTK_EXPANDER(txndata->EX_duptxn), (count > 0) ? TRUE : FALSE);

}


static void ui_import_page_transaction_options_get(struct import_data *data)
{
ImpTxnData *txndata;
ImportContext *ictx;


	DB( g_print("\n[ui-import] options_get\n") );

	ictx = &data->ictx;

	gint pageidx = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	gint accidx =  pageidx - (PAGE_IMPORT);
	//GenAcc *genacc = g_list_nth_data(ictx->gen_lst_acc, accidx);

	//txndata = &data->txndata[accidx];
	txndata = ui_import_page_transaction_data_get(data->txndata, accidx);

	ictx->opt_dateorder = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(txndata->CY_txn_dateorder));
	ictx->opt_daygap    = gtk_spin_button_get_value(GTK_SPIN_BUTTON(txndata->NB_txn_daygap));
	
	ictx->opt_ucfirst   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(txndata->CM_txn_ucfirst));
	ictx->opt_togamount = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(txndata->CM_txn_togamount));
	
	ictx->opt_ofxname   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(txndata->CY_txn_ofxname));
	ictx->opt_ofxmemo   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(txndata->CY_txn_ofxmemo));

	ictx->opt_qifmemo   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(txndata->CM_txn_qifmemo));
	ictx->opt_qifswap   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(txndata->CM_txn_qifswap));

	DB( g_print(" datefmt  = '%s' (%d)\n", CYA_IMPORT_DATEORDER[ictx->opt_dateorder], ictx->opt_dateorder) );
}


static void ui_import_page_transaction_update(struct import_data *data)
{
ImpTxnData *txndata;
ImportContext *ictx;
gboolean sensitive, visible;
gboolean iscomplete;
GtkTreeModel *model;

	DB( g_print("\n[ui-import] page_transaction_update\n") );
	
	ictx = &data->ictx;

	gint pageidx = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	gint acckey =  pageidx - (PAGE_IMPORT);
	//GenAcc *genacc = g_list_nth_data(ictx->gen_lst_acc, acckey);
	GenAcc *genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, acckey);

	//txndata = &data->txndata[acckey];
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);

	DB( g_print(" page idx:%d, genacckey:%d genacc:%p, txndata:%p\n", pageidx, acckey, genacc, txndata) );

	if(genacc)
	{
		DB( g_print(" genacc id=%d name='%s'\n dstacc=%d\n", acckey, genacc->name, genacc->kacc ) );

		visible = (genacc->is_unamed == TRUE) && (genacc->filetype != FILETYPE_CSV_HB) ? TRUE: FALSE;
		hb_widget_visible (txndata->IM_unamed, visible);

		sensitive = (genacc->kacc == DST_ACC_SKIP) ? FALSE : TRUE;
		DB( g_print(" sensitive=%d\n", sensitive) );

		gtk_widget_set_sensitive(txndata->LV_gentxn, sensitive);
		gtk_widget_set_sensitive(txndata->EX_duptxn, sensitive);
		//todo: disable option button
		gtk_widget_set_sensitive(txndata->GR_misc, sensitive);
		gtk_widget_set_sensitive(txndata->GR_date, sensitive);
		gtk_widget_set_sensitive(txndata->GR_ofx, sensitive);
		gtk_widget_set_sensitive(txndata->GR_qif, sensitive);
		gtk_widget_set_sensitive(txndata->GR_select, sensitive);
		
		//todo: display a warning if incorrect date
		gchar *msg_icon = NULL, *msg_label = NULL;

		iscomplete = (genacc->n_txnbaddate > 0) ? FALSE : TRUE;
		iscomplete = (genacc->kacc == DST_ACC_SKIP) ? TRUE : iscomplete;

		DB( g_print(" nbbaddates=%d, dstacc=%d\n", genacc->n_txnbaddate, genacc->kacc) );
		DB( g_print(" iscomplete=%d\n", iscomplete) );
		
		//show/hide invalid date group
		visible = FALSE;
		if(genacc->n_txnbaddate > 0)
		{
			visible = TRUE;
			DB( g_print(" invalid date detected\n" ) );
			msg_icon = ICONNAME_ERROR;
			msg_label = 
				_("Some date cannot be converted. Please try to change the date order to continue.");
		}
		gtk_image_set_from_icon_name(GTK_IMAGE(txndata->IM_txn), msg_icon, GTK_ICON_SIZE_BUTTON);
		gtk_label_set_text(GTK_LABEL(txndata->LB_txn), msg_label);
		hb_widget_visible (txndata->GR_msg, visible);

		//show/hide select valid button
		visible = (!genacc->n_txnsimimp && !genacc->n_txnsimdst) ? FALSE : TRUE;
		hb_widget_visible (txndata->BT_val, visible);

		//show/hide bottom duplicate panel
		visible = TRUE;
		if( genacc->kacc==DST_ACC_GLOBAL || genacc->kacc==DST_ACC_NEW || genacc->kacc==DST_ACC_SKIP)
			visible = FALSE;
		hb_widget_visible (txndata->EX_duptxn, visible);

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(txndata->LV_gentxn));

		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), LST_DSPOPE_DATE, GTK_SORT_ASCENDING);
		//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
		//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
		
		GtkWidget *page = gtk_assistant_get_nth_page (GTK_ASSISTANT(data->assistant), pageidx);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page, iscomplete);
	}
	
}


static void ui_import_page_transaction_cb_account_changed(GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
ImpTxnData *txndata;
ImportContext *ictx;
gint dstacc;

	DB( g_print("\n[ui-import] cb_account_changed\n") );
	
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ictx = &data->ictx;

	gint pageidx = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	gint acckey =  pageidx - (PAGE_IMPORT);
	//GenAcc *genacc = g_list_nth_data(ictx->gen_lst_acc, accidx);
	GenAcc *genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, acckey);
	
	//txndata = &data->txndata[acckey];
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);

	//set target acc id
	dstacc = ui_genacc_comboboxtext_get_active (txndata->CY_acc);
	genacc->kacc = dstacc;

	ui_import_page_transaction_options_get(data);
	hb_import_option_apply(ictx, genacc);

	hb_import_gen_txn_check_duplicate(ictx, genacc);
	hb_import_gen_txn_check_target_similar(ictx, genacc);
	genacc->is_dupcheck = TRUE;

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(txndata->LV_gentxn));
	
	ui_import_page_transaction_update(data);
}


static void ui_import_page_transaction_cb_option_changed(GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
ImpTxnData *txndata;
ImportContext *ictx;

	DB( g_print("\n[ui-import] cb_option_changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ictx = &data->ictx;

	gint pageidx = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	gint acckey =  pageidx - (PAGE_IMPORT);
	//GenAcc *genacc = g_list_nth_data(ictx->gen_lst_acc, accidx);
	GenAcc *genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, acckey);

	//txndata = &data->txndata[acckey];
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);

	ui_import_page_transaction_options_get(data);
	hb_import_option_apply(ictx, genacc);

	//#1866456 check also target similar
	if( txndata->CM_txn_togamount == widget )
	{
		DB( g_print(" should check target similar\n") );
		hb_import_gen_txn_check_duplicate(ictx, genacc);
		hb_import_gen_txn_check_target_similar(ictx, genacc);
	}

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(txndata->LV_gentxn));

	ui_import_page_transaction_update(data);
}


static void ui_import_page_transaction_fill(struct import_data *data)
{
ImpTxnData *txndata;
ImportContext *ictx = &data->ictx;
GtkWidget *view;
GtkTreeModel *model;
GtkTreeIter	iter;
GList *tmplist;
gchar *label = NULL;
gboolean visible;
//gint nbacc;

	DB( g_print("\n[ui-import] page_transaction_fill\n") );

	//get the account, it will be the account into the glist
	//of pagenum - PAGE_IMPORT
	//gint pageidx = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant));
	gint acckey = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant)) - (PAGE_IMPORT);
	//GenAcc *genacc = g_list_nth_data(ictx->gen_lst_acc, acckey);
	GenAcc *genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, acckey);
	//nbacc = g_list_length(ictx->gen_lst_acc);
	
	//txndata = &data->txndata[acckey];
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);
	
	DB( g_print(" genacckey:%d genacc:%p, txndata:%p\n", acckey, genacc, txndata) );
	
	if(genacc)
	{
	gint count;

		DB( g_print(" genacc id=%d name='%s'\n dstacc=%d\n", acckey, genacc->name, genacc->kacc ) );

		g_signal_handlers_block_by_func(txndata->CY_acc, G_CALLBACK(ui_import_page_transaction_cb_account_changed), NULL);
		ui_genacc_comboboxtext_set_active(txndata->CY_acc, genacc->kacc);
		g_signal_handlers_unblock_by_func(txndata->CY_acc, G_CALLBACK(ui_import_page_transaction_cb_account_changed), NULL);

		g_signal_handlers_block_by_func(txndata->NB_txn_daygap, G_CALLBACK(ui_import_page_transaction_cb_account_changed), NULL);
		ictx->opt_daygap = PREFS->dtex_daygap;

		DB( g_print(" init %d to dategap\n", ictx->opt_daygap) );

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(txndata->NB_txn_daygap), ictx->opt_daygap);
		g_signal_handlers_unblock_by_func(txndata->NB_txn_daygap, G_CALLBACK(ui_import_page_transaction_cb_account_changed), NULL);


		ui_import_page_transaction_options_get(data);
		hb_import_option_apply(ictx, genacc);
		if( genacc->is_dupcheck == FALSE )
		{
			hb_import_gen_txn_check_duplicate(ictx, genacc);
			hb_import_gen_txn_check_target_similar(ictx, genacc);
			genacc->is_dupcheck = TRUE;
		}

		view = txndata->LV_gentxn;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		gtk_list_store_clear (GTK_LIST_STORE(model));

		count = 0;
		genacc->n_txnimp = 0;
		tmplist = g_list_first(ictx->gen_lst_txn);
		while (tmplist != NULL)
		{
		GenTxn *item = tmplist->data;

			//todo: change this, this should be account
			if(item->kacc == genacc->key)
			{
				// append to our treeview
				//gtk_list_store_append (GTK_LIST_STORE(model), &iter);
				//gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				gtk_list_store_insert_with_values(GTK_LIST_STORE(model), &iter, -1,
					LST_GENTXN_POINTER, item,
					-1);

				DB( g_print(" fill: %s, %d, %s %.2f\n", item->account, item->julian, item->memo, item->amount) );
				count++;
				if(item->to_import)
					genacc->n_txnimp++;
			}
			tmplist = g_list_next(tmplist);
		}

		//label = g_strdup_printf(_("'%s' - %s"), genacc->name, hb_import_filetype_char_get(genacc));
		label = g_strdup_printf(_("Import <b>%s</b> in_to:"), genacc->is_unamed ? _("this file") : _("this account") );
		gtk_label_set_markup_with_mnemonic (GTK_LABEL(txndata->LB_acc_title), label);
		g_free(label);

		//build tooltip
		GenFile *genfile = da_gen_file_get (ictx->gen_lst_file, genacc->kfile);

		label = g_strdup_printf(_("Name: %s\nNumber: %s\nFile: %s\nEncoding: %s"), genacc->name, genacc->number, genfile->filepath, genfile->encoding);
		gtk_widget_set_tooltip_text (GTK_WIDGET(txndata->LB_acc_title), label);
		gtk_widget_set_tooltip_text (GTK_WIDGET(txndata->LB_acc_info), label);
		g_free(label);
		
		//label = g_strdup_printf(_("Account %d of %d"), acckey+1, nbacc);
		//gtk_label_set_markup (GTK_LABEL(txndata->LB_acc_count), label);
		//g_free(label);

		//#1993727 no update after toggle
		ui_import_page_transaction_update_count(data);

		visible = (genacc->filetype == FILETYPE_OFX) ? FALSE : TRUE;
		hb_widget_visible(GTK_WIDGET(txndata->GR_date), visible);
		
		visible = (genacc->filetype == FILETYPE_OFX) ? TRUE : FALSE;
		hb_widget_visible(GTK_WIDGET(txndata->GR_ofx), visible);
		
		visible = (genacc->filetype == FILETYPE_QIF) ? TRUE : FALSE;
		hb_widget_visible(GTK_WIDGET(txndata->GR_qif), visible);

		gtk_stack_set_visible_child_name(GTK_STACK(txndata->ST_stack), visible ? "QIF" : "OFX");
		
	}

}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
ui_import_page_intro_cb_dontshow(GtkWidget *widget, gpointer user_data)
{
	PREFS->dtex_nointro = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
}


static GtkWidget *
ui_import_page_intro_create(GtkWidget *assistant, struct import_data *data)
{
GtkWidget *mainbox, *label, *widget;


	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	gtk_widget_set_halign(mainbox, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(mainbox, GTK_ALIGN_CENTER);


	label = make_label(_("Import transactions from bank or credit card"), 0, 0);
	gimp_label_set_attributes(GTK_LABEL(label), 
		PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, 
		PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE, 
		-1);
	gtk_box_prepend (GTK_BOX (mainbox), label);
	//SPACING_SMALL

	label = make_label(
		_("With this assistant you will be guided through the process of importing one or several\n" \
		  "downloaded statements from your bank or credit card, in the following formats:"), 0, 0);
	gtk_box_prepend (GTK_BOX (mainbox), label);
	//SPACING_SMALL

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), 
		_("<b>Recommended:</b> .OFX or .QFX\n" \
		"<i>(Sometimes named Money™ or Quicken™)</i>\n" \
		"<b>Supported:</b> .QIF\n" \
		"<i>(Common Quicken™ file)</i>\n" \
		"<b>Advanced users only:</b> .CSV\n"
		"<i>(format is specific to HomeBank, see the documentation)</i>"));


	/* supported format */
	/*label = make_label(
	    _("HomeBank can import files in the following formats:\n" \
		"- QIF\n" \
		"- OFX/QFX (optional at compilation time)\n" \
		"- CSV (format is specific to HomeBank, see the documentation)\n" \
	), 0.0, 0.0);*/

	gtk_box_prepend (GTK_BOX (mainbox), label);
	//SPACING_SMALL

	label = make_label(
	    _("No changes will be made until you click \"Apply\" at the end of this assistant."), 0., 0.0);
	gtk_box_prepend (GTK_BOX (mainbox), label);
	//SPACING_SMALL

	widget = gtk_check_button_new_with_mnemonic (_("Don't show this again"));
	data->CM_dsta = widget;
	gtk_box_append (GTK_BOX (mainbox), widget);
	//SPACING_SMALL

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(data->CM_dsta), PREFS->dtex_nointro);


	gtk_widget_show_all (mainbox);

	g_signal_connect (data->CM_dsta, "toggled", G_CALLBACK (ui_import_page_intro_cb_dontshow), data);


	return mainbox;
}


static void ui_import_page_filechooser_update(GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
GtkTreeSelection *selection;
gboolean sensitive;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_file));

	gint count = gtk_tree_selection_count_selected_rows(selection);

	sensitive = (count > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->BT_file_delete, sensitive);
	//gtk_widget_set_sensitive(data->BT_merge, sensitive);
	//gtk_widget_set_sensitive(data->BT_delete, sensitive);

}


static void ui_import_page_filechooser_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_import_page_filechooser_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static GtkWidget *
ui_import_page_filechooser_create (GtkWidget *assistant, struct import_data *data)
{
GtkWidget *mainbox, *vbox, *hbox, *bbox;
GtkWidget *widget, *label, *scrollwin, *treeview, *tbar;

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (mainbox), hbox);

	widget = hbtk_image_new_from_icon_name_24 (ICONNAME_HB_QUICKTIPS);
	gtk_box_prepend (GTK_BOX (hbox), widget);
	//SPACING_SMALL

	label = make_label(
	    _("Drag&Drop one or several files to import.\n" \
	    "You can also use the add/delete buttons of the list.")
			, 0., 0.0);
	gtk_box_prepend (GTK_BOX (hbox), label);
	//SPACING_SMALL

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hbtk_box_prepend (GTK_BOX (mainbox), vbox);

	//list
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_hexpand(scrollwin, TRUE);
	gtk_widget_set_vexpand(scrollwin, TRUE);
	treeview = list_file_new();
	data->LV_file = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	hbtk_box_prepend (GTK_BOX (vbox), scrollwin);

	//list toolbar
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_prepend (GTK_BOX (vbox), tbar);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (tbar), bbox);
	
		widget = make_image_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_file_add = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_file_delete = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

	gtk_widget_show_all (mainbox);
	
	ui_import_page_filechooser_update(assistant, NULL);


	g_signal_connect (G_OBJECT (data->BT_file_add), "clicked", G_CALLBACK (ui_import_page_filechooser_add_action), data);
	g_signal_connect (G_OBJECT (data->BT_file_delete), "clicked", G_CALLBACK (ui_import_page_filechooser_delete_action), data);


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_file)), "changed", G_CALLBACK (ui_import_page_filechooser_selection), NULL);


	return mainbox;
}


static GtkWidget *
ui_import_page_import_create (GtkWidget *assistant, struct import_data *data)
{
GtkWidget *mainbox;
GtkWidget *label, *widget;
gchar *txt;

	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	//gtk_widget_set_halign(mainbox, GTK_ALIGN_CENTER);
	//gtk_widget_set_valign(mainbox, GTK_ALIGN_CENTER);

	widget = hbtk_image_new_from_icon_name_32(ICONNAME_ERROR);
	gtk_box_prepend (GTK_BOX (mainbox), widget);
	
	txt = _("There is too much account in the files you choose,\n" \
			"please use the back button to select less files.");
	label = gtk_label_new(txt);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_box_prepend (GTK_BOX (mainbox), label);

	gtk_widget_show_all (mainbox);
	
	return mainbox;
}


static gboolean
ui_import_page_transaction_cb_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
struct import_data *data;
ImpTxnData *txndata;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
guint action;
	
	//g_return_val_if_fail(GTK_IS_TREE_VIEW(user_data), TRUE);

	data = user_data;

	gint acckey = gtk_assistant_get_current_page(GTK_ASSISTANT(data->assistant)) - (PAGE_IMPORT);
	txndata = ui_import_page_transaction_data_get(data->txndata, acckey);	
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(txndata->LV_gentxn));

	DB( g_print(" comboboxlink '%s' \n", uri) );

	//define the action
	action = LST_SELECT_UNSET;
	if (g_strcmp0 (uri, "all") == 0)	
		action = LST_SELECT_ALL;
	else if (g_strcmp0 (uri, "non") == 0)	
		action = LST_SELECT_NONE;
	else if (g_strcmp0 (uri, "inv") == 0)	
		action = LST_SELECT_INVERT;
	else if (g_strcmp0 (uri, "val") == 0)	
		action = LST_SELECT_VALID;

	//apply the action
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	GenTxn *gentxn;

		gtk_tree_model_get(model, &iter, 
			LST_GENTXN_POINTER, &gentxn, 
			-1);

		switch(action)
		{ 
			case LST_SELECT_ALL : gentxn->to_import = TRUE; break;
			case LST_SELECT_NONE: gentxn->to_import = FALSE; break;
			case LST_SELECT_INVERT: gentxn->to_import ^= TRUE; break;
			case LST_SELECT_VALID:
				gentxn->to_import = (!gentxn->is_imp_similar && !gentxn->is_dst_similar) ? TRUE : FALSE;		
				break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	gtk_widget_queue_draw(GTK_WIDGET(txndata->LV_gentxn));

	ui_import_page_transaction_update_count(data);

    return TRUE;
}


static GtkWidget *
ui_import_page_transaction_create (GtkWidget *assistant, gint idx, struct import_data *data)
{
ImpTxnData *txndata;
GtkWidget *table, *box, *group, *stack;
GtkWidget *label, *scrollwin, *treeview, *expander, *widget;
ImpTxnData tmp;
gint row;

	//txndata = &data->txndata[idx];
	memset(&tmp, 0, sizeof(ImpTxnData));
	g_array_insert_vals(data->txndata, idx, &tmp, 1);

	txndata = ui_import_page_transaction_data_get(data->txndata, idx);

	DB( g_print(" txndat=%p\n", txndata) );

	if(!txndata)
		return NULL;
	
	table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0;
	//line 1 left
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	//gtk_widget_set_hexpand(box, TRUE);
	gtk_grid_attach (GTK_GRID(table), box, 0, row, 1, 1);

		//5.6 info icon
		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
		txndata->LB_acc_info = widget;
		gtk_box_prepend (GTK_BOX (box), widget);

		// XXX (type) + accname
		label = make_label(NULL, 0.0, 0.5);
		txndata->LB_acc_title = label;
		//gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_LARGE, -1);
		gtk_box_prepend (GTK_BOX (box), label);

		widget = ui_genacc_comboboxtext_new(data, label);
		//gtk_widget_set_hexpand(widget, TRUE);
		txndata->CY_acc = widget;
		gtk_box_prepend (GTK_BOX (box), widget);

		widget = hbtk_image_new_from_icon_name_16(ICONNAME_WARNING);
		txndata->IM_unamed = widget;
		gtk_widget_set_tooltip_text (widget, _("Target account identification by name or number failed."));
		gtk_box_prepend (GTK_BOX (box), widget);

	//line 1 right
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	//gtk_widget_set_hexpand(box, TRUE);
	gtk_grid_attach (GTK_GRID(table), box, 1, row, 1, 1);
	
	//csv options
	group = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	txndata->GR_date = group;
	gtk_box_prepend (GTK_BOX(box), group);

		label = make_label(_("Date order:"), 0, 0.5);
		gtk_box_prepend (GTK_BOX(group), label);
		widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_DATEORDER);
		txndata->CY_txn_dateorder = widget;
		gtk_box_prepend (GTK_BOX(group), widget);

	stack = gtk_stack_new();
	gtk_box_prepend (GTK_BOX(box), stack);
	txndata->ST_stack= stack;
	
	//qif options
	group = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	txndata->GR_qif = group;
	gtk_stack_add_named(GTK_STACK(stack), group, "QIF");
	
		widget = gtk_check_button_new_with_mnemonic (_("_Import memos"));
		txndata->CM_txn_qifmemo = widget;
		gtk_box_prepend (GTK_BOX(group), widget);

		widget = gtk_check_button_new_with_mnemonic (_("_Swap memos with payees"));
		txndata->CM_txn_qifswap = widget;
		gtk_box_prepend (GTK_BOX(group), widget);

	//ofx options
	group = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	txndata->GR_ofx = group;
	gtk_stack_add_named(GTK_STACK(stack), group, "OFX");

		label = make_label(_("OFX _Name:"), 0, 0.5);
		gtk_box_prepend (GTK_BOX(group), label);
		widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_OFXNAME);
		txndata->CY_txn_ofxname = widget;
		gtk_box_prepend (GTK_BOX(group), widget);

		label = make_label(_("OFX _Memo:"), 0, 0.5);
		gtk_box_prepend (GTK_BOX(group), label);
		widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_OFXMEMO);
		txndata->CY_txn_ofxmemo = widget;
		gtk_box_prepend (GTK_BOX(group), widget);

	// n transaction ...
	row++;
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	//gtk_widget_set_hexpand(box, TRUE);
	gtk_grid_attach (GTK_GRID(table), box, 0, row, 1, 1);
	
		group = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
		txndata->GR_select = group;
		gtk_box_prepend (GTK_BOX (box), group);

			label = make_label (_("Select:"), 0, 0.5);
			gtk_box_prepend (GTK_BOX (group), label);

			label = make_clicklabel("all", _("All"));
			txndata->BT_all= label;
			gtk_box_prepend (GTK_BOX (group), label);
			
			label = make_clicklabel("non", _("None"));
			txndata->BT_non = label;
			gtk_box_prepend (GTK_BOX (group), label);

			label = make_clicklabel("inv", _("Invert"));
			txndata->BT_inv = label;
			gtk_box_prepend (GTK_BOX (group), label);

			label = make_clicklabel("val", _("Valid"));
			txndata->BT_val = label;
			gtk_box_prepend (GTK_BOX (group), label);

	
		label = make_label(NULL, 0.5, 0.5);
		txndata->LB_txn_title = label;
		hbtk_box_prepend (GTK_BOX (box), label);

	// import into
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID(table), box, 1, row, 1, 1);

		group = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
		txndata->GR_misc = group;
		gtk_box_prepend (GTK_BOX (box), group);

			widget = gtk_check_button_new_with_mnemonic (_("Sentence _case memo/payee"));
			txndata->CM_txn_ucfirst = widget;
			gtk_box_prepend (GTK_BOX(group), widget);

			widget = gtk_check_button_new_with_mnemonic (_("_Toggle amount"));
			txndata->CM_txn_togamount = widget;
			gtk_box_prepend (GTK_BOX(group), widget);

	
	// error messages
	row++;
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	txndata->GR_msg = box;
	//gtk_widget_set_hexpand(box, TRUE);
	gtk_grid_attach (GTK_GRID(table), box, 0, row, 2, 1);

		widget = gtk_image_new ();
		txndata->IM_txn = widget;
		gtk_widget_set_valign(widget, GTK_ALIGN_START);
		gtk_box_prepend (GTK_BOX (box), widget);
		label = make_label(NULL, 0.0, 0.5);
		txndata->LB_txn = label;
		gtk_box_prepend (GTK_BOX (box), label);

	row++;
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = list_txn_import_create();
	txndata->LV_gentxn = treeview;
	gtk_widget_set_hexpand(scrollwin, TRUE);
	gtk_widget_set_vexpand(scrollwin, TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_grid_attach (GTK_GRID(table), scrollwin, 0, row, 2, 1);
	

	//duplicate
	row++;
	expander = gtk_expander_new (_("Similar transaction in target account (possible duplicate)"));
	txndata->EX_duptxn = expander;
	//gtk_widget_set_hexpand(expander, TRUE);
	gtk_grid_attach (GTK_GRID(table), expander, 0, row, 2, 1);


	group = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group), SPACING_SMALL);
	gtk_expander_set_child (GTK_EXPANDER(expander), group);

		row = 0;
		scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_hexpand(scrollwin, TRUE);
		treeview = create_list_transaction(LIST_TXN_TYPE_OTHER, PREFS->lst_impope_columns);
		txndata->LV_duptxn = treeview;
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_widget_set_size_request(scrollwin, -1, HB_MINWIDTH_LIST/2);
		gtk_grid_attach (GTK_GRID (group), scrollwin, 0, row, 5, 1);

		row++;
		label = make_label(_("Date _gap:"), 0, 0.5);
		gtk_grid_attach (GTK_GRID (group), label, 0, row, 1, 1);

		widget = make_numeric(label, 0.0, HB_DATE_MAX_GAP);
		txndata->NB_txn_daygap = widget;
		gtk_grid_attach (GTK_GRID (group), widget, 1, row, 1, 1);

		//TRANSLATORS: there is a spinner on the left of this label, and so you have 0....x days of date tolerance
		label = make_label(_("days"), 0, 0.5);
		gtk_grid_attach (GTK_GRID (group), label, 2, row, 1, 1);

		widget = hbtk_image_new_from_icon_name_16(ICONNAME_HB_QUICKTIPS );
		gtk_widget_set_hexpand(widget, FALSE);
		gtk_grid_attach (GTK_GRID (group), widget, 3, row, 1, 1);
	
		label = make_label (_(
			"The match is done in order: by account, amount and date.\n" \
			"A date tolerance of 0 day means an exact match"), 0, 0.5);
		gimp_label_set_attributes (GTK_LABEL (label),
				                 PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
				                 -1);
		gtk_widget_set_hexpand(label, TRUE);
		gtk_grid_attach (GTK_GRID (group), label, 4, row, 1, 1);


	// init ofx/qfx option to move
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(txndata->CY_txn_dateorder), PREFS->dtex_datefmt);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(txndata->CM_txn_ucfirst), PREFS->dtex_ucfirst);

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(txndata->CY_txn_ofxname), PREFS->dtex_ofxname);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(txndata->CY_txn_ofxmemo), PREFS->dtex_ofxmemo);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(txndata->CM_txn_qifmemo), PREFS->dtex_qifmemo);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(txndata->CM_txn_qifswap), PREFS->dtex_qifswap);

	gtk_widget_show_all (table);
	gtk_widget_hide(txndata->GR_qif);
	gtk_widget_hide(txndata->GR_ofx);

	//#1993727 no update after toggle
	g_signal_connect (txndata->BT_all, "activate-link", G_CALLBACK (ui_import_page_transaction_cb_activate_link), data);
	g_signal_connect (txndata->BT_non, "activate-link", G_CALLBACK (ui_import_page_transaction_cb_activate_link), data);
	g_signal_connect (txndata->BT_inv, "activate-link", G_CALLBACK (ui_import_page_transaction_cb_activate_link), data);
	g_signal_connect (txndata->BT_val, "activate-link", G_CALLBACK (ui_import_page_transaction_cb_activate_link), data);

	g_signal_connect (txndata->CY_acc          , "changed", G_CALLBACK (ui_import_page_transaction_cb_account_changed), data);
	g_signal_connect (txndata->CY_txn_dateorder, "changed", G_CALLBACK (ui_import_page_transaction_cb_account_changed), data);
	g_signal_connect (txndata->NB_txn_daygap   , "value-changed", G_CALLBACK (ui_import_page_transaction_cb_account_changed), data);

	g_signal_connect (txndata->CY_txn_ofxname  , "changed", G_CALLBACK (ui_import_page_transaction_cb_option_changed), data);
	g_signal_connect (txndata->CY_txn_ofxmemo  , "changed", G_CALLBACK (ui_import_page_transaction_cb_option_changed), data);
	g_signal_connect (txndata->CM_txn_qifmemo, "toggled", G_CALLBACK (ui_import_page_transaction_cb_option_changed), data);
	g_signal_connect (txndata->CM_txn_qifswap, "toggled", G_CALLBACK (ui_import_page_transaction_cb_option_changed), data);
	g_signal_connect (txndata->CM_txn_ucfirst, "toggled", G_CALLBACK (ui_import_page_transaction_cb_option_changed), data);
	g_signal_connect (txndata->CM_txn_togamount, "toggled", G_CALLBACK (ui_import_page_transaction_cb_option_changed), data);

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(txndata->LV_gentxn)), "changed",
		G_CALLBACK (ui_import_page_transaction_cb_fill_same), NULL);

	return table;
}


static GtkWidget *
ui_import_page_confirmation_create(GtkWidget *assistant, struct import_data *data)
{
GtkWidget *group_grid, *label, *widget, *scrollwin;
gint row = 0;
	
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_NONE);
	gtk_widget_set_hexpand(scrollwin, TRUE);
	gtk_widget_set_vexpand(scrollwin, TRUE);
	widget = gtk_label_new (NULL);
	data->TX_summary = widget;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);
	gtk_grid_attach (GTK_GRID (group_grid), scrollwin, 0, row, 4, 1);

	row++;
	label = make_label_group(_("Option"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic(_("Import as pending"));
	gtk_widget_set_margin_start(widget, SPACING_MEDIUM);
	data->CM_set_pending = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 3, row, 1, 1);

	row++;
	label = make_label_group(_("Run automation"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic(_("1) Enrich with _payee default"));
	gtk_widget_set_margin_start(widget, SPACING_MEDIUM);
	data->CM_do_auto_payee = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 3, row, 1, 1);
	
	row++;
	widget = gtk_check_button_new_with_mnemonic(_("2) Run automatic _assigment rules"));
	gtk_widget_set_margin_start(widget, SPACING_MEDIUM);
	data->CM_do_auto_assign = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 3, row, 1, 1);

	row++;
	label = make_label(_("Click \"Apply\" to update your accounts."), 0, 0.5);
	gtk_widget_set_margin_top(label, SPACING_LARGE);
	gtk_widget_set_margin_bottom(label, SPACING_LARGE);
	gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, -1);
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(data->CM_do_auto_payee), PREFS->dtex_dodefpayee);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(data->CM_do_auto_assign), PREFS->dtex_doautoassign);

	gtk_widget_show_all (group_grid);

	return group_grid;
}


static void
ui_import_assistant_prepare (GtkWidget *widget, GtkWidget *page, gpointer user_data)
{
struct import_data *data;
ImportContext *ictx;
gint current_page, n_pages;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ictx = &data->ictx;
	
	current_page = gtk_assistant_get_current_page (GTK_ASSISTANT(data->assistant));
	n_pages = gtk_assistant_get_n_pages (GTK_ASSISTANT(data->assistant));

	DB( g_print("\n--------\n[ui-import] prepare \n page %d of %d\n", current_page, n_pages) );

	switch( current_page )
	{
		case PAGE_WELCOME:
			DB( g_print("\n #1 intro\n") );
			gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), page, TRUE);
			break;

		case PAGE_FILES:
			DB( g_print("\n #2 file choose\n") );
			gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), page, FALSE);

			// the page complete is contextual in ui_import_page_filechooser_selection_changed
				// check is something valid :: count total rows
			ui_import_page_filechooser_eval(widget, user_data);
			break;

		case PAGE_IMPORT:
			DB( g_print("\n #3 real import\n") );
			gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), page, FALSE);

			//todo: more test needed here
			//clean any previous txn page
			/*for(i=(n_pages-1);i>=PAGE_IMPORT+1;i--)
			{
			GtkWidget *page = gtk_assistant_get_nth_page (GTK_ASSISTANT(data->assistant), i);
			gboolean isacc;
				
				if( page != NULL )
				{
					isacc = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "pgacc"));

					DB( g_print(" %d is acc: %d\n", i, isacc) );

					if( isacc )
					{
						gtk_assistant_remove_page(GTK_ASSISTANT(data->assistant), i);
						gtk_widget_destroy (page);
					}
				}
			}*/
			
			hb_import_load_all(&data->ictx);

			//add 1 page per account
			gint key, nbacc;
			nbacc = g_list_length (ictx->gen_lst_acc);

			if(data->txndata)
			{
				g_array_free(data->txndata, TRUE);
				data->txndata = NULL;
			}
			data->txndata = g_array_sized_new(FALSE, TRUE, sizeof(ImpTxnData), nbacc);

			//#1820618 patch for glib < 2.58 https://gitlab.gnome.org/GNOME/glib/issues/1374
			if( glib_minor_version < 58 )
			{
				g_array_set_size(data->txndata, nbacc);
			}
			
			DB( g_print(" accnb=%d @%p\n", nbacc, data->txndata) );
			
			//debug
			//_import_context_debug_acc_list(&data->ictx);
			
			//if(nbacc < TXN_MAX_ACCOUNT)
			//{
				for(key=1;key<nbacc+1;key++)
				{
				GtkWidget *page;
				GenAcc *genacc;
				gchar *title;

					genacc = da_gen_acc_get_by_key(ictx->gen_lst_acc, key);

					DB( g_print(" create page txn for '%s' '%s' at page %d\n", genacc->name, genacc->number, PAGE_IMPORT + key) );

					page = ui_import_page_transaction_create (data->assistant, key, data);
					//g_object_set_data(G_OBJECT(page), "pgacc", (gpointer)TRUE);
					gtk_widget_show_all (page);
					gtk_assistant_insert_page (GTK_ASSISTANT (data->assistant), page, PAGE_IMPORT + key);
					//gtk_assistant_set_page_title (GTK_ASSISTANT (data->assistant), page, _("Transaction"));
					//gtk_assistant_set_page_title (GTK_ASSISTANT (data->assistant), page, genacc->name);
					
					title = g_strdup_printf("%s %d", (!genacc->is_unamed) ? _("Account") : _("Unknown"), key );
					gtk_assistant_set_page_title (GTK_ASSISTANT (data->assistant), page, title);
					g_free(title);
				}
			//}
			
			// obsolete ??
			if( ui_import_page_import_eval (widget, NULL) )
			{   
				/*if(ictx->nb_new_acc == 0)
				{
					DB( g_print(" -> jump to Transaction page\n") );
					//gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), data->pages[PAGE_ACCOUNT], TRUE);
					gtk_assistant_next_page(GTK_ASSISTANT(data->assistant));
					gtk_assistant_next_page(GTK_ASSISTANT(data->assistant));
					//gtk_assistant_set_current_page (GTK_ASSISTANT(data->assistant), PAGE_TRANSACTION);
				}
				else
				{
					DB( g_print(" -> jump to Account page\n") );
					//gtk_assistant_set_current_page (GTK_ASSISTANT(data->assistant), PAGE_ACCOUNT);
					gtk_assistant_next_page(GTK_ASSISTANT(data->assistant));
				}*/

				gtk_assistant_next_page(GTK_ASSISTANT(data->assistant));
				gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), page, TRUE);
			}
			break;		

		default:
			if(current_page != (n_pages - 1))
			{
				DB( g_print("\n #4 transaction\n") );

				if( current_page == PAGE_IMPORT + 1)
					//hack to get rid of back button
					gtk_assistant_set_page_type (GTK_ASSISTANT(data->assistant), page, GTK_ASSISTANT_PAGE_INTRO);
				
				ui_import_page_transaction_fill(data);
				ui_import_page_transaction_update(data);
			}
			else
			{	
				DB( g_print("\n #5 confirmation\n") );

				ui_import_page_confirmation_fill(data);
				gtk_assistant_set_page_complete (GTK_ASSISTANT(data->assistant), page, TRUE);
			}
	}
}


static void
ui_import_assistant_apply (GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
ImportContext *ictx;

	DB( g_print("\n[ui-import] apply\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ictx = &data->ictx;

	ictx->set_pending    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_set_pending));
	ictx->do_auto_payee  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_do_auto_payee));
	ictx->do_auto_assign = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_do_auto_assign));

	//#1845388 persist options for lazy people
	PREFS->dtex_dodefpayee = ictx->do_auto_payee;
	PREFS->dtex_doautoassign = ictx->do_auto_assign;

	PREFS->dtex_daygap = ictx->opt_daygap;
	DB( g_print(" store %d to daygap\n", PREFS->dtex_daygap) );


	hb_import_apply(&data->ictx);
}


static gboolean
ui_import_assistant_dispose(GtkWidget *widget, gpointer user_data)
{
struct import_data *data = user_data;

	DB( g_print("\n[ui-import] dispose\n") );

#if MYDEBUG == 1
	gpointer data2 = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	g_print(" user_data=%p to be free, data2=%p\n", user_data, data2);
#endif

	da_import_context_destroy(&data->ictx);

	if(data->txndata)
	{
		g_array_free(data->txndata, TRUE);
	}


	// todo: optimize this
	//if(data->imp_cnt_trn > 0)
	//{
		//GLOBALS->changes_count += data->imp_cnt_trn;

		//our global list has changed, so update the treeview
		account_compute_balances (FALSE);
		ui_hub_account_populate(GLOBALS->mainwindow, NULL);
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
	//}

	g_free(user_data);

	//delete-event TRUE abort/FALSE destroy
	return FALSE;
}


static void
ui_import_assistant_close_cancel (GtkWidget *widget, gpointer user_data)
{
struct import_data *data;
GtkWidget *assistant = (GtkWidget *) user_data;

	DB( g_print("\n[ui-import] close\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ui_import_assistant_dispose(widget, data);
	gtk_window_destroy (GTK_WINDOW(assistant));
}


/* starting point of import */
GtkWidget *ui_import_assistant_new (gchar **paths)
{
struct import_data *data;
GtkWidget *assistant, *page, *page_file;
gint w, h;

	DB( g_print("\n[ui-import] new\n") );

	data = g_malloc0(sizeof(struct import_data));
	if(!data) return NULL;

	assistant = gtk_assistant_new ();
	data->assistant = assistant;

	//store our window private data
	g_object_set_data(G_OBJECT(assistant), "inst_data", (gpointer)data);
	//DB( g_print("** \n[ui-import] window=%x, inst_data=%x\n", assistant, data) );

	gtk_window_set_modal(GTK_WINDOW (assistant), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(assistant), GTK_WINDOW(GLOBALS->mainwindow));

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	gtk_window_set_default_size (GTK_WINDOW(assistant), w * 0.8, h * 0.8);
	//gtk_window_set_default_size (GTK_WINDOW(assistant), w - 24, h - 24);

	page = ui_import_page_intro_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Welcome"));
	gtk_assistant_set_page_complete (GTK_ASSISTANT(assistant), page, TRUE );

	page = ui_import_page_filechooser_create (assistant, data);
	page_file = page;
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Select file(s)"));

	page = ui_import_page_import_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	//gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Import"));

	//3...x transaction page will be added automatically

	//page = ui_import_page_transaction_create (assistant, 0, data);
	//gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	//hack to hide the back button here
	//gtk_assistant_set_page_type (GTK_ASSISTANT(assistant), page, GTK_ASSISTANT_PAGE_INTRO);
	//gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Transaction"));
	
	page = ui_import_page_confirmation_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Confirmation"));
	
	//gtk_assistant_set_forward_page_func(GTK_ASSISTANT(assistant), ui_import_assistant_forward_page_func, data, NULL);

	//setup
	//ui_import_page_filechooser_selection_changed(assistant, data);
	DB( g_printf(" check list of paths '%p'\n", paths) );
	if( paths != NULL )
	{
	ImportContext *ictx = &data->ictx;
	GenFile *genfile;
	gchar **str = paths;

		while(*str != NULL)
		{
			DB( g_printf(" try to append '%s'\n", *str) );

			genfile = da_gen_file_append_from_filename(ictx, *str);
			if(genfile)
			{
				list_file_add(data->LV_file, genfile);
			}
			str++;
		}
		g_strfreev(paths);
	}

	//connect all our signals
	//g_signal_connect (window, "delete-event", G_CALLBACK (hbfile_dispose), (gpointer)data);
	g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (ui_import_assistant_prepare), NULL);

	g_signal_connect (G_OBJECT (page_file), "map", G_CALLBACK (ui_import_page_filechooser_visible), NULL);


	g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (ui_import_assistant_close_cancel), assistant);
	g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (ui_import_assistant_close_cancel), assistant);
	g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (ui_import_assistant_apply), NULL);

#ifdef G_OS_WIN32
	hbtk_assistant_hack_button_order(GTK_ASSISTANT(assistant));
#endif

	gtk_widget_show (assistant);

	if(PREFS->dtex_nointro)
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), PAGE_FILES);

	return assistant;
}

