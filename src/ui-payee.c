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

#include "ui-payee.h"
#include "ui-category.h"

#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static GtkWidget *
container_get_nth(GtkBox *container, gint nth)
{
GList *lchild, *list;
GtkWidget *child;

	if(!GTK_IS_CONTAINER(container))
		return NULL;

	lchild = list = gtk_container_get_children (GTK_CONTAINER(container));
	child = g_list_nth_data (list, nth);
	g_list_free(lchild);
	
	return child;
}


GtkWidget *
ui_pay_entry_popover_get_entry(GtkBox *box)
{
	return container_get_nth(box, 0);
}


Payee
*ui_pay_entry_popover_get(GtkBox *box)
{
GtkWidget *entry;
gchar *name;
Payee *item = NULL;

	DB( g_print ("ui_pay_entry_popover_get()\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		name = (gchar *)gtk_entry_get_text(GTK_ENTRY (entry));
		item = da_pay_get_by_name(name);
	}
	return item;
}


guint32
ui_pay_entry_popover_get_key_add_new(GtkBox *box)
{
Payee *item = ui_pay_entry_popover_get(box);
GtkWidget *entry;
GtkTreeModel *store;
	
	if( item == NULL )
	{
		/* automatic add */
		//todo: check prefs + ask the user here 1st time
		entry = container_get_nth(box, 0);
		if( entry != NULL && GTK_IS_ENTRY(entry) )
		{
			item = da_pay_malloc();
			item->name = g_strdup(gtk_entry_get_text(GTK_ENTRY (entry)));
			da_pay_append(item);

			store = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));
			if( store )
				gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
					0, item->name, -1);
		}
	}
	return item->key;
}


guint32
ui_pay_entry_popover_get_key(GtkBox *box)
{
Payee *item = ui_pay_entry_popover_get(box);

	return ((item != NULL) ? item->key : 0);
}


void
ui_pay_entry_popover_set_active(GtkBox *box, guint32 key)
{
GtkWidget *entry;

	DB( g_print ("ui_pay_comboboxentry_set_active()\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
	Payee *item = da_pay_get(key);

		hbtk_entry_set_text(GTK_ENTRY(entry), item != NULL ? item->name : "");
	}
}


static void 
ui_pay_entry_popover_cb_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
GtkTreeSelection *treeselection;
GtkTreeModel *model;
GtkTreeIter iter;
GtkEntry *entry = user_data;

	if( GTK_IS_ENTRY(entry) )
	{
		treeselection = gtk_tree_view_get_selection(tree_view);
		if( gtk_tree_selection_get_selected(treeselection, &model, &iter) )
		{
		gchar *item;

			gtk_tree_model_get(model, &iter, 0, &item, -1);
			gtk_entry_set_text(GTK_ENTRY(user_data), item);
			g_free(item);			
		}
	}
}


static void
ui_pay_entry_popover_populate(GtkListStore *store)
{
GHashTableIter hiter;
gpointer key, value;

	g_hash_table_iter_init (&hiter, GLOBALS->h_pay);
	while (g_hash_table_iter_next (&hiter, &key, &value))
	{
	Payee *item = value;

		//#1826360 wish: archive payee/category to lighten the lists 
		if( !(item->flags & PF_HIDDEN) )
		{

			gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
				0, item->name, -1);
		}
	}
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

}


static void
ui_pay_entry_popover_function (GtkEditable *editable, gpointer user_data)
{

	DB( g_print("text changed to %s\n", gtk_entry_get_text(GTK_ENTRY(editable)) ) );
	


}


static void
ui_pay_entry_popover_cb_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
GtkWidget *entry = user_data;
GtkAllocation allocation;
GtkPopover *popover;

	if(GTK_IS_ENTRY(entry))
	{
		gtk_widget_get_allocation (entry, &allocation);
		popover = gtk_menu_button_get_popover(GTK_MENU_BUTTON(togglebutton));
		if(GTK_IS_POPOVER(popover))
		{
			gtk_widget_set_size_request (GTK_WIDGET(popover), allocation.width + (2*SPACING_POPOVER), -1);	
			DB( g_print("should set width to %d\n", allocation.width + (2*SPACING_POPOVER)) );	
		}
	}
}


static gint
ui_pay_entry_popover_compare_func (GtkTreeModel *model, GtkTreeIter  *a, GtkTreeIter  *b, gpointer      userdata)
{
gint retval = 0;
gchar *name1, *name2;

    gtk_tree_model_get(model, a, 0, &name1, -1);
    gtk_tree_model_get(model, b, 0, &name2, -1);

	retval = hb_string_utf8_compare(name1, name2);

    g_free(name2);
    g_free(name1);

  	return retval;
}


static gboolean
ui_pay_entry_popover_completion_func (GtkEntryCompletion *completion,
                                              const gchar        *key,
                                              GtkTreeIter        *iter,
                                              gpointer            user_data)
{
gchar *name = NULL;
gchar *normalized_string;
gchar *case_normalized_string;
gboolean ret = FALSE;
GtkTreeModel *model;

	model = gtk_entry_completion_get_model (completion);

	gtk_tree_model_get (model, iter,
		0, &name,
		-1);

  if (name != NULL)
    {
      normalized_string = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);

      if (normalized_string != NULL)
        {
          case_normalized_string = g_utf8_casefold (normalized_string, -1);

			//g_print("match '%s' for '%s' ?\n", key, case_normalized_string);
		//if (!strncmp (key, case_normalized_string, strlen (key)))
          if (g_strstr_len (case_normalized_string, strlen (case_normalized_string), key ))
			{
        		ret = TRUE;
				//	g_print(" ==> yes !\n");
				
			}
				
          g_free (case_normalized_string);
        }
      g_free (normalized_string);
    }

  return ret;
}



static void 
ui_pay_entry_popover_destroy( GtkWidget *widget, gpointer user_data )
{

    DB( g_print ("[pay entry popover] destroy\n") );

}


GtkWidget *
ui_pay_entry_popover_new(GtkWidget *label)
{
GtkWidget *mainbox, *box, *entry, *menubutton, *image, *popover, *scrollwin, *treeview;
GtkListStore *store;
GtkEntryCompletion *completion;

    DB( g_print ("[pay entry popover] new\n") );

	mainbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(mainbox)), GTK_STYLE_CLASS_LINKED);
	
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(mainbox), entry, TRUE, TRUE, 0);

	menubutton = gtk_menu_button_new ();
	//data->MB_template = menubutton;
	image = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(menubutton), image);
	gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_LEFT );
	//gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(mainbox), menubutton, FALSE, FALSE, 0);
	
    completion = gtk_entry_completion_new ();

	gtk_entry_set_completion (GTK_ENTRY (entry), completion);
	g_object_unref(completion);
	
	store = gtk_list_store_new (1,
		G_TYPE_STRING
		);

	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_pay_entry_popover_compare_func, NULL, NULL);

	ui_pay_entry_popover_populate(store);

	
    gtk_entry_completion_set_model (completion, GTK_TREE_MODEL(store));
    gtk_entry_completion_set_match_func(completion, ui_pay_entry_popover_completion_func, NULL, NULL);
	g_object_unref(store);

	gtk_entry_completion_set_text_column (completion, 0);

	gtk_widget_show_all(mainbox);


	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_box_pack_start(GTK_BOX(box), scrollwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	//gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), HB_MINHEIGHT_LIST);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_widget_show_all(box);

	//gtk_widget_set_can_focus(GTK_WIDGET(treeview), FALSE);

GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
	
  renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     0,
                                                     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(treeview), TRUE);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);
	
	//gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_BROWSE);
	
	
	//popover = create_popover (menubutton, box, GTK_POS_BOTTOM);
	popover = create_popover (menubutton, box, GTK_POS_LEFT);
	//gtk_widget_set_size_request (popover, HB_MINWIDTH_LIST, HB_MINHEIGHT_LIST);
	gtk_widget_set_vexpand(popover, TRUE);
	
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);
	
	
	// connect our dispose function
	g_signal_connect (entry, "destroy", G_CALLBACK (ui_pay_entry_popover_destroy), NULL);

	g_signal_connect_after (entry  , "changed", G_CALLBACK (ui_pay_entry_popover_function), NULL);

	g_signal_connect (menubutton, "toggled", G_CALLBACK (ui_pay_entry_popover_cb_toggled), entry);
	
	g_signal_connect (treeview, "row-activated", G_CALLBACK (ui_pay_entry_popover_cb_row_activated), entry);

	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_popover_popdown), popover);
	#else
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_widget_hide), popover);
	#endif

	//g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK (ui_pay_entry_popover_cb_selection), entry);
	//g_signal_connect_swapped(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK(gtk_popover_popdown), popover);
	
	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);

	//gtk_widget_set_size_request(comboboxentry, HB_MINWIDTH_LIST, -1);

	return mainbox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void ui_pay_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gboolean toggled;

	DB( g_print("(ui_pay_listview) toggle_to_filter\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_pay));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Payee *payitem;

		gtk_tree_model_get (model, &iter,
			LST_DEFPAY_TOGGLE, &toggled,
			LST_DEFPAY_DATAS, &payitem,
			-1);

		DB( g_print(" payee k:%3d = %d (%s)\n", payitem->key, toggled, payitem->name) );
		da_flt_status_pay_set(filter, payitem->key, toggled);
		
		//payitem->flt_select = toggled;

		/* Make iter point to the next row in the list store */
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static void
ui_pay_listview_toggled_cb (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, LST_DEFPAY_TOGGLE, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFPAY_TOGGLE, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
  
  
  //g_signal_emit_by_name(gtk_tree_view_get_selection(treeview), "changed", NULL);
}


void
ui_pay_listview_quick_select(GtkTreeView *treeview, const gchar *uri)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gboolean toggle;
gint qselect = hb_clicklabel_to_int(uri);

	DB( g_print("(ui_acc_listview) quick select\n") );


	DB( g_print(" comboboxlink '%s' %d\n", uri, qselect) );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		switch(qselect)
		{
			case HB_LIST_QUICK_SELECT_ALL:
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFPAY_TOGGLE, TRUE, -1);
				break;
			case HB_LIST_QUICK_SELECT_NONE:
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFPAY_TOGGLE, FALSE, -1);
				break;
			case HB_LIST_QUICK_SELECT_INVERT:
					gtk_tree_model_get (model, &iter, LST_DEFPAY_TOGGLE, &toggle, -1);
					toggle ^= 1;
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFPAY_TOGGLE, toggle, -1);
				break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static gint
ui_pay_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
Payee *entry1, *entry2;
gint retval = 0;

	gtk_tree_model_get(model, a, LST_DEFPAY_DATAS, &entry1, -1);
	gtk_tree_model_get(model, b, LST_DEFPAY_DATAS, &entry2, -1);

    switch (sortcol)
    {
		case LST_DEFPAY_SORT_NAME:
			retval = hb_string_utf8_compare(entry1->name, entry2->name);
			break;
		case LST_DEFPAY_SORT_USETXN:
			retval = entry1->nb_use_all - entry2->nb_use_all;
			break;
		case LST_DEFPAY_SORT_USECFG:
			retval = (entry1->nb_use_all - entry1->nb_use_txn) - (entry2->nb_use_all - entry2->nb_use_txn);
			break;
		case LST_DEFPAY_SORT_DEFPAY:
			retval = entry1->paymode - entry2->paymode;
			break;
		case LST_DEFPAY_SORT_DEFCAT:
			{
			Category *c1, *c2;

				c1 = da_cat_get(entry1->kcat);
				c2 = da_cat_get(entry2->kcat);
				if( c1 != NULL && c2 != NULL )
				{
					retval = hb_string_utf8_compare(c1->fullname, c2->fullname);
				}
			}
			break;
		default:
			g_return_val_if_reached(0);
	}
		
    return retval;
}


static void
ui_pay_listview_count_txn_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Payee *entry;
gchar buffer[256];

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &entry, -1);
	if(entry->nb_use_txn > 0)
	{
		g_snprintf(buffer, 256-1, "%d", entry->nb_use_txn);
		g_object_set(renderer, "text", buffer, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);
}


static void
ui_pay_listview_count_cfg_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Payee *entry;
gchar buffer[256];
guint use;

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &entry, -1);
	use = entry->nb_use_all - entry->nb_use_txn;
	if(use > 0)
	{
		g_snprintf(buffer, 256-1, "%d", use);
		g_object_set(renderer, "text", buffer, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);
}


static void
ui_pay_listview_defcat_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Payee *entry;
Category *cat;

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &entry, -1);

	cat = da_cat_get(entry->kcat);
	if( cat != NULL )
	{
		g_object_set(renderer, "text", cat->fullname, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);
}


static void
ui_pay_listview_info_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Payee *entry;

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &entry, -1);

	g_object_set(renderer, "icon-name", get_paymode_icon_name(entry->paymode), NULL);

}


static void
ui_pay_listview_name_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Payee *entry;
gchar *name;

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &entry, -1);

	if(entry->key == 0)
		name = _("(no payee)");
	else
		name = entry->name;

	g_object_set(renderer, "text", name, NULL);
}


static void 
ui_pay_listview_icon_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Payee *entry;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &entry, -1);
	if( entry->flags & PF_HIDDEN )
		iconname = ICONNAME_HB_BUTTON_HIDE;

	g_object_set(renderer, "icon-name", iconname, NULL);
}


#if MYDEBUG == 1
static void
ui_pay_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Payee *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	


/* = = = = = = = = = = = = = = = = */


void
ui_pay_listview_add(GtkTreeView *treeview, Payee *item)
{
GtkTreeModel *model;
GtkTreeIter	iter;

	if( item->name != NULL )
	{
		model = gtk_tree_view_get_model(treeview);

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFPAY_TOGGLE, FALSE,
			LST_DEFPAY_DATAS, item,
			-1);
	}
}

guint32
ui_pay_listview_get_selected_key(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Payee *item;

		gtk_tree_model_get(model, &iter, LST_DEFPAY_DATAS, &item, -1);

		if( item!= NULL	 )
			return item->key;
	}
	return 0;
}

void
ui_pay_listview_remove_selected(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("ui_pay_listview_remove_selected() \n") );

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}
}


struct PayListContext {
	GtkTreeModel	*model;
	gchar			*needle;
	gboolean		showhidden;
};


static void ui_pay_listview_populate_ghfunc(gpointer key, gpointer value, struct PayListContext *context)
{
GtkTreeIter	iter;
Payee *item = value;
gboolean hastext = FALSE;
gboolean insert = TRUE;

	//DB( g_print(" populate: %p\n", key) );
	if( context->needle != NULL )
		hastext = (strlen(context->needle) >= 2) ? TRUE : FALSE;

	if(hastext)
		insert = hb_string_utf8_strstr(item->name, context->needle, FALSE);

	if(context->showhidden == FALSE)
	{
		if( item->flags & PF_HIDDEN )
			insert = FALSE;
	}

	if( insert == TRUE )
	{
		gtk_list_store_insert_with_values(GTK_LIST_STORE(context->model), &iter, -1,
			LST_DEFPAY_TOGGLE	, FALSE,
			LST_DEFPAY_DATAS, item,
			-1);
	}
}


void ui_pay_listview_populate(GtkWidget *treeview, gchar *needle, gboolean showhidden)
{
struct PayListContext context;
	
	DB( g_print("ui_pay_listview_populate \n") );

	context.model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	context.needle = needle;
	context.showhidden = showhidden;

	gtk_list_store_clear (GTK_LIST_STORE(context.model));

	//g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	//gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view */

	/* populate */
	g_hash_table_foreach(GLOBALS->h_pay, (GHFunc)ui_pay_listview_populate_ghfunc, &context);

	//gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view */
	//g_object_unref(model);
}


static gboolean ui_pay_listview_search_equal_func (GtkTreeModel *model,
                               gint column,
                               const gchar *key,
                               GtkTreeIter *iter,
                               gpointer search_data)
{
  gboolean retval = TRUE;
  gchar *normalized_string;
  gchar *normalized_key;
  gchar *case_normalized_string = NULL;
  gchar *case_normalized_key = NULL;
  Payee *item;
	
  //gtk_tree_model_get_value (model, iter, column, &value);
  gtk_tree_model_get(model, iter, LST_DEFPAY_DATAS, &item, -1);

  if(item !=  NULL)
  {
	  normalized_string = g_utf8_normalize (item->name, -1, G_NORMALIZE_ALL);
	  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

	  if (normalized_string && normalized_key)
		{
		  case_normalized_string = g_utf8_casefold (normalized_string, -1);
		  case_normalized_key = g_utf8_casefold (normalized_key, -1);

		  if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
		    retval = FALSE;
		}

	  g_free (normalized_key);
	  g_free (normalized_string);
	  g_free (case_normalized_key);
	  g_free (case_normalized_string);
  }
  return retval;
}


GtkWidget *
ui_pay_listview_new(gboolean withtoggle, gboolean withcount)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	DB( g_print("ui_pay_listview_new() \n") );

	store = gtk_list_store_new(
		NUM_LST_DEFPAY,
		G_TYPE_BOOLEAN,
		G_TYPE_POINTER
		);

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);


	#if MYDEBUG == 1
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_cell_data_function_debugkey, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	#endif

	// column: hide icon
	//#1826360 wish: archive payee/category to lighten the lists
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_icon_cell_data_function, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column: toggle
	if( withtoggle == TRUE )
	{
		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Visible"),
						renderer, "active", LST_DEFPAY_TOGGLE, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_NONE);

		g_signal_connect (G_OBJECT(renderer), "toggled",
			    G_CALLBACK (ui_pay_listview_toggled_cb), store);

		g_object_set_data(G_OBJECT(treeview), "togrdr_data", renderer);
	}

	// column: usage
	if( withcount == TRUE )
	{
		renderer = gtk_cell_renderer_text_new ();
		g_object_set(renderer, "xalign", 0.5, NULL);
		
		column = gtk_tree_view_column_new();
		//TRANSLATORS: 'txn' is abbrevation for transaction
		gtk_tree_view_column_set_title(column, _("# txn"));
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_count_txn_cell_data_function, GINT_TO_POINTER(LST_DEFPAY_DATAS), NULL);
		gtk_tree_view_column_set_alignment (column, 0.5);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFPAY_SORT_USETXN);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
		//by degfault hide this column
		gtk_tree_view_column_set_visible(column, FALSE);

		renderer = gtk_cell_renderer_text_new ();
		g_object_set(renderer, "xalign", 0.5, NULL);
		
		column = gtk_tree_view_column_new();
		//TRANSLATORS: 'txn' is abbrevation for configuration
		gtk_tree_view_column_set_title(column, _("# cfg"));
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_count_cfg_cell_data_function, GINT_TO_POINTER(LST_DEFPAY_DATAS), NULL);
		gtk_tree_view_column_set_alignment (column, 0.5);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFPAY_SORT_USECFG);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
		//by degfault hide this column
		gtk_tree_view_column_set_visible(column, FALSE);
	}

	// column: name
	renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Payee"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_name_cell_data_function, GINT_TO_POINTER(LST_DEFPAY_DATAS), NULL);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST);
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFPAY_SORT_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column: category
	if( withtoggle == FALSE )
	{
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
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_defcat_cell_data_function, GINT_TO_POINTER(LST_DEFPAY_DATAS), NULL);
		//#2004631 date and column title alignement
		//gtk_tree_view_column_set_alignment (column, 0.5);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST);
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFPAY_SORT_DEFCAT);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}

	// column: payment
	if( withtoggle == FALSE )
	{
		column = gtk_tree_view_column_new();
		//TRANSLATORS: this is abbreviation for Payment
		gtk_tree_view_column_set_title(column, _("Payment"));
		renderer = gtk_cell_renderer_pixbuf_new ();
		g_object_set(renderer, "xalign", 0.0, NULL);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_pay_listview_info_cell_data_function, NULL, NULL);
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFPAY_SORT_DEFPAY);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}	

	/* empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	if( withtoggle == TRUE )
		gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(treeview), ui_pay_listview_search_equal_func, NULL, NULL);

	// treeview attribute
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), withcount);

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFPAY_SORT_NAME, ui_pay_listview_compare_func, GINT_TO_POINTER(LST_DEFPAY_SORT_NAME), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFPAY_SORT_USETXN, ui_pay_listview_compare_func, GINT_TO_POINTER(LST_DEFPAY_SORT_USETXN), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFPAY_SORT_USECFG, ui_pay_listview_compare_func, GINT_TO_POINTER(LST_DEFPAY_SORT_USECFG), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFPAY_SORT_DEFPAY, ui_pay_listview_compare_func, GINT_TO_POINTER(LST_DEFPAY_SORT_DEFPAY), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFPAY_SORT_DEFCAT, ui_pay_listview_compare_func, GINT_TO_POINTER(LST_DEFPAY_SORT_DEFCAT), NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_DEFPAY_SORT_NAME, GTK_SORT_ASCENDING);

	return treeview;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void
ui_pay_manage_dialog_refilter(struct ui_pay_manage_dialog_data *data)
{
gboolean showhidden;
gchar *needle;

	DB( g_print("(ui_pay_manage_dialog) refilter\n") );

	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));
	showhidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_showhidden));
	ui_pay_listview_populate(data->LV_pay, needle, showhidden);
}



static void
ui_pay_manage_dialog_delete_unused( GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data = user_data;
gboolean result;

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("(ui_pay_manage_dialog) delete unused - data %p\n", data) );

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->dialog),
			_("Delete unused payee"),
			_("Are you sure you want to\npermanently delete unused payee?"),
			_("_Delete"),
			TRUE
		);

	if( result == GTK_RESPONSE_OK )
	{
	GtkTreeModel *model;	

		//#1996275 fill usage before delete !
		if( data->usagefilled == FALSE )
		{
			payee_fill_usage();
			data->usagefilled = TRUE;
		}

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_pay));
		gtk_list_store_clear (GTK_LIST_STORE(model));
		
		//#1917075
		data->change += payee_delete_unused();
	
		ui_pay_manage_dialog_refilter(data);
	}
}


/**
 * ui_pay_manage_dialog_load_csv:
 *
 */
static void
ui_pay_manage_dialog_load_csv( GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data = user_data;
gchar *filename = NULL;
gchar *error;

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("(ui_pay_manage_dialog) load csv - data %p\n", data) );

	if( ui_file_chooser_csv(GTK_WINDOW(data->dialog), GTK_FILE_CHOOSER_ACTION_OPEN, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		if( !payee_load_csv(filename, &error) )
		{
			ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
					_("File format error"),
					_("The CSV file must contains the exact numbers of column,\nseparated by a semi-colon, please see the help for more details.")
					);
		}

		g_free( filename );
		ui_pay_manage_dialog_refilter(data);
	}
}

/**
 * ui_pay_manage_dialog_save_csv:
 *
 */
static void
ui_pay_manage_dialog_save_csv( GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data = user_data;
gchar *filename = NULL;

	DB( g_print("(ui_pay_manage_dialog) save csv\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( ui_file_chooser_csv(GTK_WINDOW(data->dialog), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		payee_save_csv(filename);
		g_free( filename );
	}
}


static void
ui_pay_manage_dialog_cb_show_usage (GtkToggleButton *button, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
gboolean showusage;
GtkTreeViewColumn *column;

	DB( g_print("(ui_pay_manage_dialog) show usage\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW)), "inst_data");

	if( data->usagefilled == FALSE )
	{
		payee_fill_usage();
		data->usagefilled = TRUE;
	}

	showusage = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_showusage));

	column = hbtk_treeview_get_column_by_id(GTK_TREE_VIEW(data->LV_pay), LST_DEFPAY_SORT_USETXN);
	if(column != NULL)
	{
		gtk_tree_view_column_set_visible(column, showusage);
	}
	column = hbtk_treeview_get_column_by_id(GTK_TREE_VIEW(data->LV_pay), LST_DEFPAY_SORT_USECFG);
	if(column != NULL)
	{
		gtk_tree_view_column_set_visible(column, showusage);
	}

}




static void
ui_pay_manage_dialog_cb_show_hidden (GtkToggleButton *button, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW)), "inst_data");
	ui_pay_manage_dialog_refilter(data);
}


/**
 * ui_pay_manage_dialog_add:
 *
 */
static void
ui_pay_manage_dialog_add(GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
Payee *item;
gchar *name;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(defayee) add (data=%p)\n", data) );

	name = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_name));

	item = da_pay_malloc ();
	item->name = g_strdup(name);

	g_strstrip(item->name);
	
	if( strlen(item->name) > 0 )
	{
		if( da_pay_append(item) )
		{
			ui_pay_listview_add(GTK_TREE_VIEW(data->LV_pay), item);
			data->change++;
		}
	}
	else
		da_pay_free (item);
		
	gtk_entry_set_text(GTK_ENTRY(data->ST_name), "");
}


static void ui_pay_manage_dialog_edit_entry_cb(GtkEditable *editable, gpointer user_data)
{
GtkDialog *window = user_data;
const gchar *buffer;

	buffer = gtk_entry_get_text(GTK_ENTRY(editable));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(window), GTK_RESPONSE_ACCEPT, strlen(buffer) > 0 ? TRUE : FALSE);
}


static void ui_pay_manage_dialog_edit(GtkWidget *dowidget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
GtkWidget *dialog, *content_area, *grid;
GtkWidget *label, *widget;
GtkWidget *ST_name, *PO_cat, *NU_mode, *TB_notes, *scrollwin;
gint row;
guint32 key;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(dowidget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(defayee) modify %p\n", data) );

	key = ui_pay_listview_get_selected_key(GTK_TREE_VIEW(data->LV_pay));
	if( key > 0 )
	{
	Payee *item;

		item = da_pay_get( key );

		dialog = gtk_dialog_new_with_buttons (_("Edit Payee"),
						    GTK_WINDOW (data->dialog),
						    0,
						    _("_Cancel"),
						    GTK_RESPONSE_REJECT,
						    _("_OK"),
						    GTK_RESPONSE_ACCEPT,
						    NULL);

		content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

		grid = gtk_grid_new();
		gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
		gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
		hb_widget_set_margin(GTK_WIDGET(grid), SPACING_LARGE);
		gtk_box_pack_start (GTK_BOX (content_area), grid, TRUE, TRUE, 0);

		// group :: General
		//label = make_label_group(_("Payee"));
		//gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

		row = 0;
		label = make_label_widget(_("_Name:"));
		gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
		widget = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(widget), 24);
		ST_name = widget;
		gtk_widget_set_hexpand(widget, TRUE);
		gtk_grid_attach (GTK_GRID (grid), widget, 2, row, 1, 1);

		//#1932193 add notes for payee
		row++;
		label = make_label(_("Notes:"), 1.0, 0.0);
		gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
		widget = gtk_text_view_new ();
		//#1697171 add wrap
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widget), GTK_WRAP_WORD);
		scrollwin = gtk_scrolled_window_new (NULL, NULL);
		gtk_widget_set_size_request (scrollwin, -1, 24);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);
		gtk_widget_set_hexpand (scrollwin, TRUE);
		gtk_widget_set_vexpand (scrollwin, TRUE);
		TB_notes = widget;
		gtk_grid_attach (GTK_GRID (grid), scrollwin, 2, row, 1, 1);


		// group :: Default
		row++;
		label = make_label_group(_("Default Fill"));
		gtk_widget_set_margin_top(label, SPACING_LARGE);
		gtk_grid_attach (GTK_GRID (grid), label, 0, row, 3, 1);

		row++;
		label = make_label_widget(_("_Category:"));
		gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
		//widget = ui_cat_comboboxentry_new(label);
		widget = ui_cat_entry_popover_new(label);
		PO_cat = widget;
		gtk_widget_set_hexpand (widget, TRUE);
		gtk_grid_attach (GTK_GRID (grid), widget, 2, row, 1, 1);

		row++;
		label = make_label_widget(_("Pa_yment:"));
		gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
		//5.2.4 we drop internal xfer here as it will disapear 
		//widget = make_paymode_nointxfer(label);
		widget = make_paymode_nointxfer(label);
		NU_mode = widget;
		gtk_grid_attach (GTK_GRID (grid), widget, 2, row, 1, 1);


		//setup
		gtk_entry_set_text(GTK_ENTRY(ST_name), item->name);
		gtk_widget_grab_focus (ST_name);
		gtk_entry_set_activates_default (GTK_ENTRY(ST_name), TRUE);

		//5.5 done in popover		
		//ui_cat_comboboxentry_populate(GTK_COMBO_BOX(PO_cat), GLOBALS->h_cat);
		//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(PO_cat), item->kcat);
		ui_cat_entry_popover_set_active(GTK_BOX(PO_cat), item->kcat);

		paymode_combo_box_set_active(GTK_COMBO_BOX(NU_mode), item->paymode);

		//#1932193 add notes for payee
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (TB_notes));
		GtkTextIter iter;

		gtk_text_buffer_set_text (buffer, "", 0);
		gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
		if(item->notes != NULL)
			gtk_text_buffer_insert (buffer, &iter, item->notes, -1);
		
		g_signal_connect (G_OBJECT (ST_name), "changed", G_CALLBACK (ui_pay_manage_dialog_edit_entry_cb), dialog);

		gtk_widget_show_all(grid);


		gtk_dialog_set_default_response(GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT);

		//wait for the user
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		if(result == GTK_RESPONSE_ACCEPT)
		{
		const gchar *name;

			// 1: manage renaming
			name = gtk_entry_get_text(GTK_ENTRY(ST_name));
			// ignore if item is empty
			if (name && *name)
			{
				if( payee_rename(item, name) )
				{
					//to redraw the active entry
					gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_pay));
					data->change++;
				}
				else
				{
					ui_dialog_msg_infoerror(GTK_WINDOW(dialog), GTK_MESSAGE_ERROR,
						_("Error"),
						_("Cannot rename this Payee,\n"
						"from '%s' to '%s',\n"
						"this name already exists."),
						item->name,
						name
						);

				}
			}

			//item->kcat    = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(PO_cat));
			item->kcat    = ui_cat_entry_popover_get_key_add_new(GTK_BOX(PO_cat));
			item->paymode = paymode_combo_box_get_active(GTK_COMBO_BOX(NU_mode));

			//#1932193 add notes for payee
			GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (TB_notes));
			GtkTextIter siter, eiter;
			gchar *notes;
			
			gtk_text_buffer_get_iter_at_offset (buffer, &siter, 0);
			gtk_text_buffer_get_end_iter(buffer, &eiter);
			notes = gtk_text_buffer_get_text(buffer, &siter, &eiter, FALSE);
			if(notes != NULL)
				item->notes = g_strdup(notes);
			
			//TODO: missing sort here
	    }

		// cleanup and destroy
		gtk_window_destroy (GTK_WINDOW(dialog));
	}

}


static void ui_pay_manage_dialog_merge_entry_cb(GtkComboBox *widget, gpointer user_data)
{
GtkDialog *window = user_data;
gchar *buffer;

	//buffer = (gchar *)gtk_entry_get_text(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (widget))));
	buffer = (gchar *)gtk_entry_get_text(GTK_ENTRY (widget));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(window), GTK_RESPONSE_OK, strlen(buffer) > 0 ? TRUE : FALSE);
}


static void ui_pay_manage_dialog_merge(GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
GtkWidget *dialog, *content, *mainvbox;
GtkWidget *getwidget, *togglebutton;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(defayee) merge %p\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_pay));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Payee *srcpay;
	gchar *title;
	gchar *secondtext;

		gtk_tree_model_get(model, &iter, LST_DEFPAY_DATAS, &srcpay, -1);

		title = g_strdup_printf (
			_("Merge payee '%s'"), srcpay->name);
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
			                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			                              GTK_MESSAGE_WARNING,
			                              GTK_BUTTONS_NONE,
			                              title,
		                             	  NULL
			                              );

		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
				_("_Cancel"), GTK_RESPONSE_CANCEL,
				_("Merge"), GTK_RESPONSE_OK,
				NULL);

		gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

		content = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG (dialog));
		mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
		gtk_box_pack_start (GTK_BOX (content), mainvbox, TRUE, TRUE, 0);

		secondtext = _("Transactions assigned to this payee,\n"
			  "will be moved to the payee selected below.");

		g_object_set(GTK_MESSAGE_DIALOG (dialog), "secondary-text", secondtext, NULL);
		g_free(title);

		//getwidget = ui_pay_comboboxentry_new(NULL);
		getwidget = ui_pay_entry_popover_new(NULL);
		gtk_box_pack_start (GTK_BOX (mainvbox), getwidget, FALSE, FALSE, 0);

		secondtext = g_strdup_printf (
			_("_Delete the payee '%s'"), srcpay->name);
		togglebutton = gtk_check_button_new_with_mnemonic(secondtext);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), TRUE);
		g_free(secondtext);
		gtk_box_pack_start (GTK_BOX (mainvbox), togglebutton, FALSE, FALSE, 0);

		//setup 
		//gtk_combo_box_set_active(GTK_COMBO_BOX(getwidget), oldpos);
		g_signal_connect (G_OBJECT (ui_pay_entry_popover_get_entry(GTK_BOX(getwidget))), "changed", G_CALLBACK (ui_pay_manage_dialog_merge_entry_cb), dialog);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);

		//5.5 done in popover, and keep src as well
		//ui_pay_comboboxentry_populate_except(GTK_COMBO_BOX(getwidget), GLOBALS->h_pay, srcpay->key);
		gtk_widget_grab_focus (getwidget);

		gtk_widget_show_all(mainvbox);
		
		//wait for the user
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		if(result == GTK_RESPONSE_OK)
		{
		GtkTreeModel *model;
		Payee *newpay;
		guint dstpaykey;

			//dstpaykey = ui_pay_comboboxentry_get_key_add_new(GTK_COMBO_BOX(getwidget));
			dstpaykey = ui_pay_entry_popover_get_key_add_new(GTK_BOX(getwidget));

			//do nothing if src = dst...
			if( srcpay->key != dstpaykey )
			{

				DB( g_print(" -> move pay to %d)\n", dstpaykey) );

				model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_pay));
				gtk_list_store_clear (GTK_LIST_STORE(model));

				payee_move(srcpay->key, dstpaykey);

				newpay = da_pay_get(dstpaykey);

				//#1771720: update count
				newpay->nb_use_all += srcpay->nb_use_all;
				newpay->nb_use_txn += srcpay->nb_use_txn;
				srcpay->nb_use_all = 0;
				srcpay->nb_use_txn = 0;

				// add the new payee to listview
				if(newpay)
					ui_pay_listview_add(GTK_TREE_VIEW(data->LV_pay), newpay);

				// delete the old payee
				if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)) )
				{
					DB( g_print(" -> delete %d '%s'\n", srcpay->key, srcpay->name ) );

					ui_pay_listview_remove_selected(GTK_TREE_VIEW(data->LV_pay));
					da_pay_delete(srcpay->key);
				}

				data->change++;

				//#1958767 if searchbar is active get the text
				ui_pay_manage_dialog_refilter(data);
			}
		}

		// cleanup and destroy
		gtk_window_destroy (GTK_WINDOW(dialog));
	}

}


/*
** delete the selected payee to our treeview and temp GList
*/
static void ui_pay_manage_dialog_delete(GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
Payee *item;
guint32 key;
gint result;


	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(ui_pay_manage_dialog) delete (data=%p)\n", data) );

	key = ui_pay_listview_get_selected_key(GTK_TREE_VIEW(data->LV_pay));
	if( key > 0 )
	{
	gchar *title;
	gchar *secondtext = NULL;

		item = da_pay_get(key);

		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), item->name);

		if( item->nb_use_all > 0 )
		{
			secondtext = _("This payee is used.\n"
			    "Any transaction using that payee will be set to (no payee)");
		}

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
			payee_move(key, 0);
			ui_pay_listview_remove_selected(GTK_TREE_VIEW(data->LV_pay));
			da_pay_delete(key);
			data->change++;
		}

	}
}


//#1826360 wish: archive payee/category to lighten the lists
static void ui_pay_manage_dialog_hide(GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
Payee *item;
guint32 key;
gboolean showhidden;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(ui_pay_manage_dialog) hide (data=%p)\n", data) );

	key = ui_pay_listview_get_selected_key(GTK_TREE_VIEW(data->LV_pay));
	if( key > 0 )
	{
		item = da_pay_get(key);
		item->flags ^= PF_HIDDEN;
		data->change++;
	}
	
	//todo remove row
	showhidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_showhidden));
	if( showhidden == FALSE )
		ui_pay_listview_remove_selected(GTK_TREE_VIEW(data->LV_pay));
	else
		hbtk_listview_redraw_selected_row (GTK_TREE_VIEW(data->LV_pay));
}


static void ui_pay_manage_dialog_update(GtkWidget *treeview, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;
gboolean sensitive;
guint32 key;

	DB( g_print("\n(ui_pay_manage_dialog) cursor changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	key = ui_pay_listview_get_selected_key(GTK_TREE_VIEW(data->LV_pay));

	sensitive = (key > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_merge, sensitive);
	gtk_widget_set_sensitive(data->BT_delete, sensitive);
	gtk_widget_set_sensitive(data->BT_hide, sensitive);
}


static gboolean ui_pay_manage_dialog_cb_on_key_press(GtkWidget *source, GdkEventKey *event, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data = user_data;

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
		gtk_widget_grab_focus(data->LV_pay);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}



static void ui_pay_manage_dialog_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_pay_manage_dialog_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}

static void ui_pay_manage_dialog_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            user_data)
{
//GtkTreeModel		 *model;
//GtkTreeIter			 iter;

	DB( g_print("ui_pay_manage_dialog_onRowActivated()\n") );

	//#1960743 double click payee not working after search
	// no need to check is is not none here, done into edit
	//model = gtk_tree_view_get_model(treeview);
	//gtk_tree_model_get_iter_first(model, &iter);
	//gtk_tree_model_get_iter(model, &iter, path);
	//if(gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter) == FALSE)
	//{
		ui_pay_manage_dialog_edit(GTK_WIDGET(treeview), NULL);
	//}
}


static void
ui_pay_manage_search_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data = user_data;

	DB( g_printf("\n[ui_pay_manage_dialog] search_changed_cb\n") );

	ui_pay_manage_dialog_refilter(data);
}


static void ui_pay_manage_setup(struct ui_pay_manage_dialog_data *data)
{

	DB( g_print("\n[ui-budget] setup\n") );

	DB( g_print(" init data\n") );
	data->change = 0;
	data->usagefilled = FALSE;

	DB( g_print(" populate\n") );
	
	ui_pay_listview_populate(data->LV_pay, NULL, FALSE);

	//DB( g_print(" set widgets default\n") );


	DB( g_print(" connect widgets signals\n") );
    
    g_signal_connect (G_OBJECT (data->BT_showhidden), "toggled", G_CALLBACK (ui_pay_manage_dialog_cb_show_hidden), NULL);
    g_signal_connect (G_OBJECT (data->BT_showusage) , "toggled", G_CALLBACK (ui_pay_manage_dialog_cb_show_usage), NULL);
    
	g_object_bind_property (data->BT_add, "active", data->RE_addreveal, "reveal-child", G_BINDING_BIDIRECTIONAL);

	gtk_tree_view_set_search_entry(GTK_TREE_VIEW(data->LV_pay), GTK_ENTRY(data->ST_search));

	g_signal_connect (G_OBJECT (data->ST_search), "search-changed", G_CALLBACK (ui_pay_manage_search_changed_cb), data);

	g_signal_connect (G_OBJECT (data->ST_name), "activate", G_CALLBACK (ui_pay_manage_dialog_add), NULL);

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_pay)), "changed", G_CALLBACK (ui_pay_manage_dialog_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_pay), "row-activated", G_CALLBACK (ui_pay_manage_dialog_onRowActivated), NULL);

	g_signal_connect (G_OBJECT (data->BT_edit)  , "clicked", G_CALLBACK (ui_pay_manage_dialog_edit), NULL);
	g_signal_connect (G_OBJECT (data->BT_merge) , "clicked", G_CALLBACK (ui_pay_manage_dialog_merge), NULL);
	g_signal_connect (G_OBJECT (data->BT_delete), "clicked", G_CALLBACK (ui_pay_manage_dialog_delete), NULL);
	
	g_signal_connect (G_OBJECT (data->BT_hide), "clicked", G_CALLBACK (ui_pay_manage_dialog_hide), NULL);

}


static gboolean ui_pay_manage_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct ui_pay_manage_dialog_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n(ui_pay_manage_mapped)\n") );

	ui_pay_manage_setup(data);
	ui_pay_manage_dialog_update(data->LV_pay, NULL);

	data->mapped_done = TRUE;

	return FALSE;
}


GtkWidget *ui_pay_manage_dialog (void)
{
struct ui_pay_manage_dialog_data *data;
GtkWidget *dialog, *content, *mainvbox, *vbox, *bbox, *treeview, *scrollwin, *table;
GtkWidget *menu, *menuitem, *widget, *image, *revealer, *tbar;
gint w, h, dw, dh, row;

	data = g_malloc0(sizeof(struct ui_pay_manage_dialog_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Manage Payees"),
					    GTK_WINDOW(GLOBALS->mainwindow),
						0,
					    _("_Close"), GTK_RESPONSE_ACCEPT,
					    NULL);

	/*
	dialog = g_object_new (GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Manage Payees"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW(GLOBALS->mainwindow));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	*/

	//gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	
	data->dialog = dialog;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 2:3
	dw = (dh * 2) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);
	
	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print("(ui_pay_manage_dialog) dialog=%p, inst_data=%p\n", dialog, data) );

	//dialog contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (content), mainvbox, TRUE, TRUE, 0);
	hb_widget_set_margin(GTK_WIDGET(mainvbox), SPACING_LARGE);

    //our table
	table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (mainvbox), table, TRUE, TRUE, 0);

	//filter part
	row = 0;
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), bbox, 0, row, 2, 1);
	//test headerbar
	//content = gtk_dialog_get_header_bar(GTK_DIALOG (dialog));

	widget = make_image_toggle_button(ICONNAME_HB_BUTTON_HIDE, _("Show Hidden") );
	data->BT_showhidden = widget;
	gtk_box_pack_start(GTK_BOX (bbox), widget, FALSE, FALSE, 0);

	widget = make_image_toggle_button(ICONNAME_HB_BUTTON_USAGE, _("Show Usage") );
	data->BT_showusage = widget;
	gtk_box_pack_start(GTK_BOX (bbox), widget, FALSE, FALSE, 0);	

		
	menu = gtk_menu_new ();
	gtk_widget_set_halign (menu, GTK_ALIGN_END);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Import CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (ui_pay_manage_dialog_load_csv), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("E_xport CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (ui_pay_manage_dialog_save_csv), data);
	
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	
	menuitem = gtk_menu_item_new_with_mnemonic (_("_Delete unused"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (ui_pay_manage_dialog_delete_unused), data);
	
	gtk_widget_show_all (menu);


	widget = gtk_menu_button_new();
	image = gtk_image_new_from_icon_name (ICONNAME_HB_BUTTON_MENU, GTK_ICON_SIZE_MENU);
	g_object_set (widget, "image", image, "popup", GTK_MENU(menu),  NULL);
	gtk_widget_set_halign (widget, GTK_ALIGN_END);
	gtk_box_pack_end(GTK_BOX (bbox), widget, FALSE, FALSE, 0);
	//gtk_header_bar_pack_end(GTK_HEADER_BAR (content), widget);

	widget = make_search();
	data->ST_search = widget;
	gtk_box_pack_end(GTK_BOX (bbox), widget, FALSE, FALSE, 0);

	
	// list + toolbar
	row++;
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_grid_attach (GTK_GRID (table), vbox, 0, row, 2, 1);

	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_box_pack_start(GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), HB_MINHEIGHT_LIST);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	treeview = ui_pay_listview_new(FALSE, TRUE);
	data->LV_pay = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);
		data->BT_add = widget = make_image_toggle_button(ICONNAME_LIST_ADD, _("Add"));
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);
		data->BT_delete = widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete")); 
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);
		data->BT_edit = widget = make_image_button(ICONNAME_LIST_EDIT, _("Edit"));
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);
		data->BT_merge = widget = make_image_button(ICONNAME_HB_LIST_MERGE, _("Move/Merge"));
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);
	
		data->BT_hide = widget = make_image_button(ICONNAME_HB_BUTTON_HIDE, _("Show/Hide"));
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	row++;
	revealer = gtk_revealer_new ();
	data->RE_addreveal = revealer;
	gtk_grid_attach (GTK_GRID (table), revealer, 0, row, 2, 1);
	data->ST_name = gtk_entry_new ();
	gtk_entry_set_placeholder_text(GTK_ENTRY(data->ST_name), _("new payee") );
	gtk_widget_set_hexpand (data->ST_name, TRUE);
	gtk_revealer_set_child (GTK_REVEALER(revealer), data->ST_name);
	
	
	// connect dialog signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_pay_manage_mapped), &dialog);
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_pay_manage_dialog_cb_on_key_press), (gpointer)data);
	

	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all (dialog);

	// wait for the user
	gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	GLOBALS->changes_count += data->change;

	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);
	
	return NULL;
}

