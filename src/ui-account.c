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
#include "hb-account.h"
#include "ui-account.h"
#include "ui-currency.h"
#include "ui-dialogs.h"
#include "ui-group.h"

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


extern HbKvData CYA_ACC_TYPE[];


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


GtkTreeModel *
ui_acc_entry_popover_get_model(GtkBox *box)
{
GtkWidget *entry = container_get_nth(box, 0);

	if( GTK_IS_ENTRY(entry) )
	{
		return gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));
	}
	return NULL;
}


GtkWidget *
ui_acc_entry_popover_get_entry(GtkBox *box)
{
	return container_get_nth(box, 0);
}


Account
*ui_acc_entry_popover_get(GtkBox *box)
{
GtkWidget *entry;
gchar *name;
Account *item = NULL;

	DB( g_print ("ui_acc_entry_popover_get()\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		name = (gchar *)gtk_entry_get_text(GTK_ENTRY (entry));
		item = da_acc_get_by_name(name);
	}
	return item;
}


guint32
ui_acc_entry_popover_get_key(GtkBox *box)
{
Account *item = ui_acc_entry_popover_get(box);

	return ((item != NULL) ? item->key : 0);
}


//#1859077 preset account if single 
void
ui_acc_entry_popover_set_single(GtkBox *box)
{
GtkWidget *entry;
GtkTreeModel *model;
GtkTreeIter iter;

	DB( g_print ("ui_acc_popover_set_single()\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		model = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));
		if(model != NULL && gtk_tree_model_iter_n_children(model, NULL) == 1)
		{
			if( gtk_tree_model_get_iter_first(model, &iter) == TRUE )
			{
			gchar *item;

				gtk_tree_model_get(model, &iter, 0, &item, -1);
				hbtk_entry_set_text(GTK_ENTRY(entry), item != NULL ? item : "");
				g_free(item);	
			}
		}
	}
}


void
ui_acc_entry_popover_set_active(GtkBox *box, guint32 key)
{
GtkWidget *entry;

	DB( g_print ("ui_acc_popover_set_active()\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
	Account *item = da_acc_get(key);
	gchar *txt = "";
	
		//#1972078 forbid set a closed account
		if( (item != NULL) && !(item->flags & AF_CLOSED) )
		{
			txt = item->name;
		}
		hbtk_entry_set_text(GTK_ENTRY(entry), txt);
	}
}


static gboolean ui_acc_manage_cb_on_key_press(GtkWidget *source, GdkEvent *event, gpointer user_data)
{
struct ui_acc_manage_data *data = user_data;
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
	if(keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
	{
		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), NULL);
		gtk_widget_grab_focus(data->LV_acc);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}


static void 
ui_acc_entry_popover_cb_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
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
ui_acc_entry_popover_populate_ghfunc(gpointer key, gpointer value, struct accPopContext *ctx)
{
GtkTreeIter  iter;
Account *acc = value;

	if( (acc->flags & AF_CLOSED) ) return;
	if( (ctx->insert_type == ACC_LST_INSERT_REPORT) && (acc->flags & AF_NOREPORT) ) return;
	if( (acc->key == ctx->except_key) ) return;


	//#1673260 this was for xfer with same currency only
	//if( (ctx->kcur > 0 ) && (acc->kcur != ctx->kcur) ) return;
	
    DB( g_print (" -> append model '%s'\n", acc->name) );
	
	gtk_list_store_append (GTK_LIST_STORE(ctx->model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(ctx->model), &iter,
			0, acc->name,
			1, acc->pos,
			-1);
}


void
ui_acc_entry_popover_populate(GtkBox *box, GHashTable *hash, gint insert_type)
{
	ui_acc_entry_popover_populate_except(box, hash, 0, insert_type);
}


void
ui_acc_entry_popover_populate_except(GtkBox *box, GHashTable *hash, guint except_key, gint insert_type)
{
GtkTreeModel *model;
struct accPopContext ctx;

    DB( g_print ("ui_acc_entry_popover_populate\n") );

    DB( g_print (" -> except is %d\n", except_key) );

	model = ui_acc_entry_popover_get_model(GTK_BOX(box));
	
	/* keep our model alive and detach from popover and completion */
	g_object_ref(model);

	/* clear and populate */
	ctx.model = model;
	ctx.except_key = except_key;
	ctx.insert_type = insert_type;
	//#1673260 xfer same currency
	//ctx.kcur = 0;
	//Account *acc = da_acc_get(except_key);
	//if(acc != NULL)
		//ctx.kcur = acc->kcur;
	
	gtk_list_store_clear (GTK_LIST_STORE(model));
	//5.3 empty the entry
	gtk_entry_set_text(GTK_ENTRY (ui_acc_entry_popover_get_entry(GTK_BOX (box))), "");
	g_hash_table_foreach(hash, (GHFunc)ui_acc_entry_popover_populate_ghfunc, &ctx);

	/* reatach our model */
	g_object_unref(model);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
}


static void
ui_acc_entry_popover_function (GtkEditable *editable, gpointer user_data)
{

	DB( g_print("text changed to %s\n", gtk_entry_get_text(GTK_ENTRY(editable)) ) );
	


}


static void
ui_acc_entry_popover_cb_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
GtkWidget *entry = user_data;
GtkAllocation allocation;
GtkPopover *popover;

    DB( g_print ("[acc entry popover] open\n") );

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
ui_acc_entry_popover_compare_func (GtkTreeModel *model, GtkTreeIter  *a, GtkTreeIter  *b, gpointer      userdata)
{
gint pos1, pos2;
  
    gtk_tree_model_get(model, a, 1, &pos1, -1);
    gtk_tree_model_get(model, b, 1, &pos2, -1);
  	return (pos1 - pos2);
}


static gboolean
ui_acc_entry_popover_completion_func (GtkEntryCompletion *completion,
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
ui_acc_entry_popover_destroy( GtkWidget *widget, gpointer user_data )
{

    DB( g_print ("[acc entry popover] destroy\n") );

}


GtkWidget *
ui_acc_entry_popover_new(GtkWidget *label)
{
GtkWidget *mainbox, *box, *entry, *menubutton, *image, *popover, *scrollwin, *treeview;
GtkListStore *store;
GtkEntryCompletion *completion;

    DB( g_print ("[acc entry popover] new\n") );

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
	
	store = gtk_list_store_new (2,
		G_TYPE_STRING,
		G_TYPE_INT
		);

	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_acc_entry_popover_compare_func, NULL, NULL);

    gtk_entry_completion_set_model (completion, GTK_TREE_MODEL(store));
    gtk_entry_completion_set_match_func(completion, ui_acc_entry_popover_completion_func, NULL, NULL);
	g_object_unref(store);

	gtk_entry_completion_set_text_column (completion, 0);

	gtk_widget_show_all(mainbox);


	//popover
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(box), scrollwin, TRUE, TRUE, 0);
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
	g_signal_connect (entry, "destroy", G_CALLBACK (ui_acc_entry_popover_destroy), NULL);

	g_signal_connect_after (entry  , "changed", G_CALLBACK (ui_acc_entry_popover_function), NULL);

	g_signal_connect (menubutton, "toggled", G_CALLBACK (ui_acc_entry_popover_cb_toggled), entry);
	
	g_signal_connect (treeview, "row-activated", G_CALLBACK (ui_acc_entry_popover_cb_row_activated), entry);

	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_popover_popdown), popover);
	#else
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_widget_hide), popover);
	#endif

	//g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK (ui_acc_entry_popover_cb_selection), entry);
	//g_signal_connect_swapped(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK(gtk_popover_popdown), popover);
	
	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);

	return mainbox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

guint ui_acc_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter)
{
GtkTreeModel *model;
GtkTreeIter iter;
gboolean valid;
guint change = 0;

	DB( g_print("(ui_acc_listview) toggle_to_filter\n") );

	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Account *item;
	gboolean toggled;

		gtk_tree_model_get (model, &iter,
			LST_DEFACC_TOGGLE, &toggled,
			LST_DEFACC_DATAS , &item,
			-1);

		DB( g_print(" acc k:%3d = %d (%s)\n", item->key, toggled, item->name) );
		change += da_flt_status_acc_set(filter, item->key, toggled);
	
		/* Make iter point to the next row in the list store */
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
	return change;
}






static void
ui_acc_listview_toggled_cb (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
GtkTreeView *treeview = (GtkTreeView *)data; 
GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
GtkTreeIter  iter;
GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
gboolean fixed;

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, LST_DEFACC_TOGGLE, &fixed, -1);

	/* do something with the value */
	fixed ^= 1;

	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFACC_TOGGLE, fixed, -1);

	/* clean up */
	gtk_tree_path_free (path);
}


static void ui_acc_listview_sort_force(GtkTreeSortable *sortable, gpointer user_data)
{
gint sort_column_id;
GtkSortType order;

	DB( g_print("\n[ui-cacc-listview] sort force\n") );

	gtk_tree_sortable_get_sort_column_id(sortable, &sort_column_id, &order);
	DB( g_print(" id %d\n order %d\n", sort_column_id, order) );

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, order);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), sort_column_id, order);
}


static gint
ui_acc_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
Account *accitem1, *accitem2;

    gtk_tree_model_get(model, a, LST_DEFACC_DATAS, &accitem1, -1);
    gtk_tree_model_get(model, b, LST_DEFACC_DATAS, &accitem2, -1);

	switch (sortcol)
	{
		case LST_DEFACC_SORT_POS:
			retval = accitem1->pos - accitem2->pos;
		break;
		case LST_DEFACC_SORT_NAME:
			retval = hb_string_utf8_compare(accitem1->name, accitem2->name);
		break;
	}

    return retval;
}


static void
ui_acc_listview_icon_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Account *entry;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, LST_DEFACC_DATAS, &entry, -1);
	if( entry->flags & AF_CLOSED )
		iconname = ICONNAME_CHANGES_PREVENT;
	else
	{
		if( !(entry->flags & AF_NOBUDGET) )
			iconname = ICONNAME_HB_OPE_BUDGET;
	}
	g_object_set(renderer, "icon-name", iconname, NULL);
}

static void
ui_acc_listview_cell_data_function_pos (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Account *accitem;
gchar buffer[256];

	gtk_tree_model_get(model, iter, LST_DEFACC_DATAS, &accitem, -1);
	g_snprintf(buffer, 256-1, "%d", accitem->pos);
	g_object_set(renderer, "text", buffer, NULL);
}


static void
ui_acc_listview_name_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Account *entry;
gchar *name;

	gtk_tree_model_get(model, iter, LST_DEFACC_DATAS, &entry, -1);
	if(entry->name == NULL)
		name = _("(none)");		// can never occurs !
	else
		name = entry->name;

	g_object_set(renderer, "text", name, NULL);
}


#if MYDEBUG == 1
static void
ui_acc_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Account *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DEFACC_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	


/* = = = = = = = = = = = = = = = = */


guint 
ui_acc_listview_fill_keys(GtkTreeView *treeview, guint32 *keys, guint32 *kcur)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid, ismulti;
guint count, lastcurr = GLOBALS->kcur;

	count = 0;
	ismulti = FALSE;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Account *accitem;
	gboolean toggled;

		gtk_tree_model_get (model, &iter,
			LST_DEFACC_TOGGLE, &toggled,
			LST_DEFACC_DATAS, &accitem,
			-1);

		keys[accitem->key] = toggled;

		DB( g_print(" store acc:%d, curr:%d, sel:%d multi:%d @%p\n", accitem->key, accitem->kcur, toggled, ismulti, &keys[accitem->key]) );
		
		//check if multiple currencies
		if( toggled == TRUE )
		{
			if( count > 0 )
			{
				if( lastcurr != accitem->kcur )
					ismulti = TRUE;
			}
			lastcurr = accitem->kcur;
			count++;
		}

		/* Make iter point to the next row in the list store */
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	*kcur = (ismulti == TRUE) ? GLOBALS->kcur : lastcurr;
	return count;
}


void
ui_acc_listview_set_active(GtkTreeView *treeview, guint32 key)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Account *accitem;

		gtk_tree_model_get (model, &iter,
			LST_DEFACC_DATAS, &accitem,
			-1);

		if( accitem->key == key )
		{
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				LST_DEFACC_TOGGLE, TRUE, -1);
			break;
		}
		/* Make iter point to the next row in the list store */
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


/**
 * acc_list_add:
 * 
 * Add a single element (useful for dynamics add)
 * 
 * Return value: --
 *
 */
void
ui_acc_listview_add(GtkTreeView *treeview, Account *item)
{
	if( item->name != NULL )
	{
	GtkTreeModel *model;
	GtkTreeIter	iter;

		model = gtk_tree_view_get_model(treeview);

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFACC_TOGGLE, FALSE,
			LST_DEFACC_DATAS, item,
			-1);

		gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter);

	}
}

guint32
ui_acc_listview_get_selected_key(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Account *item;

		gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &item, -1);
		
		if( item!= NULL	 )
			return item->key;
	}
	return 0;
}

void
ui_acc_listview_remove_selected(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}
}


void
ui_acc_listview_quick_select(GtkTreeView *treeview, const gchar *uri)
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
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFACC_TOGGLE, TRUE, -1);
				break;
			case HB_LIST_QUICK_SELECT_NONE:
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFACC_TOGGLE, FALSE, -1);
				break;
			case HB_LIST_QUICK_SELECT_INVERT:
					gtk_tree_model_get (model, &iter, LST_DEFACC_TOGGLE, &toggle, -1);
					toggle ^= 1;
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFACC_TOGGLE, toggle, -1);
				break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


void ui_acc_listview_populate(GtkWidget *view, gint insert_type, gchar *needle)
{
GtkTreeModel *model;
GtkTreeIter	iter;
GList *lacc, *list;
gboolean hastext = FALSE;
gboolean insert = TRUE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	gtk_list_store_clear (GTK_LIST_STORE(model));

	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view */

	if( needle != NULL )
		hastext = (strlen(needle) >= 2) ? TRUE : FALSE;

	/* populate */
	//g_hash_table_foreach(GLOBALS->h_acc, (GHFunc)ui_acc_listview_populate_ghfunc, model);
	list = g_hash_table_get_values(GLOBALS->h_acc);
	
	//lacc = list = g_list_sort(list, (GCompareFunc)ui_acc_glist_compare_func);
	lacc = list = account_glist_sorted(HB_GLIST_SORT_POS);
	while (list != NULL)
	{
	Account *item = list->data;
	
		if( insert_type == ACC_LST_INSERT_REPORT )
		{
			//#1674045 ony rely on nosummary
			//if( (item->flags & AF_CLOSED) ) goto next1;
			if( (item->flags & AF_NOREPORT) ) goto next1;
		}

		if(hastext)
			insert = hb_string_utf8_strstr(item->name, needle, FALSE);
		else
			insert = TRUE;


		if( insert == TRUE )
		{

			DB( g_print(" populate acc k=%d\n", item->key) );

			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				LST_DEFACC_TOGGLE	, FALSE,
				LST_DEFACC_DATAS, item,
				-1);
		}

next1:
		list = g_list_next(list);
	}
	g_list_free(lacc);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view */
	g_object_unref(model);
}


GtkWidget *
ui_acc_listview_new(gboolean withtoggle)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	// create list store
	store = gtk_list_store_new(NUM_LST_DEFACC,
		G_TYPE_BOOLEAN,
		G_TYPE_POINTER
		);

	// treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);


	#if MYDEBUG == 1
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_acc_listview_cell_data_function_debugkey, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	#endif

	// column 1: toggle
	if( withtoggle == TRUE )
	{
		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Visible"),
							     renderer,
							     "active", LST_DEFACC_TOGGLE,
							     NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_NONE);

		g_signal_connect (renderer, "toggled",
			    G_CALLBACK (ui_acc_listview_toggled_cb), treeview);

		g_object_set_data(G_OBJECT(treeview), "togrdr_data", renderer);

	}

	//2028464 sort by pos/name
	if( withtoggle == FALSE )
	{
		// column: position
		renderer = gtk_cell_renderer_text_new ();
		//#2004631 date and column title alignement
		g_object_set(renderer, "xalign", 1.0, NULL);
		
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(column, "#");
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFACC_SORT_POS);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_acc_listview_cell_data_function_pos, GINT_TO_POINTER(LST_DEFACC_DATAS), NULL);
		//#2004631 date and column title alignement
		gtk_tree_view_column_set_alignment (column, 1.0);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}

	// column 2: name
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Account"));
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFACC_SORT_NAME);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);

	if( withtoggle == FALSE )
	{
		g_object_set(renderer, 
			//taken from nemo, not exactly a resize to content, but good compromise
			"width-chars", 40,
			NULL);
	}

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_acc_listview_name_cell_data_function, GINT_TO_POINTER(LST_DEFACC_DATAS), NULL);

	if( withtoggle == FALSE )
	{
		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_acc_listview_icon_cell_data_function, GINT_TO_POINTER(LST_DEFACC_DATAS), NULL);
	}

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	// treeviewattribute
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), !withtoggle);
	//5.7 no more dragndrop
	//gtk_tree_view_set_reorderable (GTK_TREE_VIEW(treeview), TRUE);
	
	//sortable
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFACC_SORT_POS, ui_acc_listview_compare_func, GINT_TO_POINTER(LST_DEFACC_SORT_POS), NULL);
	if( withtoggle == FALSE )
	{
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFACC_SORT_NAME, ui_acc_listview_compare_func, GINT_TO_POINTER(LST_DEFACC_SORT_NAME), NULL);
	}

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_DEFACC_SORT_POS, GTK_SORT_ASCENDING);

	//gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(treeview), ui_acc_listview_search_equal_func, NULL, NULL);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	return treeview;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
ui_acc_manage_dialog_refilter(struct ui_acc_manage_data *data)
{
gchar *needle;

	DB( g_print("(ui_acc_manage_dialog) refilter\n") );

	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));
	ui_acc_listview_populate(data->LV_acc, ACC_LST_INSERT_NORMAL, needle);
}


static void ui_acc_manage_getlast(struct ui_acc_manage_data *data)
{
gboolean active;
gdouble value;
Account *item;

	DB( g_print("\n(ui_acc_manage_getlast)\n") );

	DB( g_print(" -> for account id=%d\n", data->lastkey) );

	item = da_acc_get(data->lastkey);
	if(item != NULL)
	{	
		data->change++;

		item->type = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));

		account_set_currency(item, ui_cur_combobox_get_key(GTK_COMBO_BOX(data->CY_curr)) );

		g_free(item->bankname);
		item->bankname = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->ST_institution)));

		g_free(item->number);
		item->number = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->ST_number)));

		item->kgrp = ui_grp_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->ST_group));

		g_free(item->website);
		item->website = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->ST_website)));

		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->TB_notes));
		GtkTextIter siter, eiter;
		gchar *notes;
		
		gtk_text_buffer_get_iter_at_offset (buffer, &siter, 0);
		gtk_text_buffer_get_end_iter(buffer, &eiter);
		notes = gtk_text_buffer_get_text(buffer, &siter, &eiter, FALSE);
		if(notes != NULL)
			item->notes = g_strdup(notes);
		
		item->flags &= ~(AF_CLOSED);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_closed));
		if(active) item->flags |= AF_CLOSED;

		item->flags &= ~(AF_NOBUDGET);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_nobudget));
		if(active) item->flags |= AF_NOBUDGET;

		item->flags &= ~(AF_NOSUMMARY);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_nosummary));
		if(active) item->flags |= AF_NOSUMMARY;

		item->flags &= ~(AF_NOREPORT);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_noreport));
		if(active) item->flags |= AF_NOREPORT;

		//#1896441 outflow summary
		item->flags &= ~(AF_OUTFLOWSUM);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_outflowsum));
		if(active) item->flags |= AF_OUTFLOWSUM;
		
		
		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_initial));
		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_initial));
		item->initial = value;

		//gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_warning));
		//value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_warning));
		//item->warning = value;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_minimum));
		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_minimum));
		item->minimum = value;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_maximum));
		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_maximum));
		item->maximum = value;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_cheque1));
		item->cheque1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->ST_cheque1));

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_cheque2));
		item->cheque2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->ST_cheque2));

		item->karc= hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_template));
		//active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_template));
		//item->karc = atoi(active_id);
	}

}


//#1743254 set frac digits as well
static void ui_acc_manage_changed_curr_cb(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
guint32 key;
Currency *cur;

	DB( g_print("\n(ui_acc_manage changed_curr_cb)\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	key = ui_cur_combobox_get_key(GTK_COMBO_BOX(data->CY_curr));
	cur = da_cur_get (key);
	if( cur != NULL )
	{
		DB( g_print("- set digits to '%s' %d\n", cur->name, cur->frac_digits) );
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON(data->ST_initial), cur->frac_digits);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON(data->ST_minimum), cur->frac_digits);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON(data->ST_maximum), cur->frac_digits);
	}

}


/*
** set widgets contents from the selected account
*/
static void ui_acc_manage_set(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
Account *item;
//gchar idbuffer[12];

	DB( g_print("\n(ui_acc_manage_set)\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &item, -1);

		DB( g_print(" -> set acc id=%d\n", item->key) );

		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_type), item->type );

		ui_cur_combobox_set_active(GTK_COMBO_BOX(data->CY_curr), item->kcur);
		
		if(item->bankname != NULL)
			gtk_entry_set_text(GTK_ENTRY(data->ST_institution), item->bankname);
		else
			gtk_entry_set_text(GTK_ENTRY(data->ST_institution), "");

		if(item->number != NULL)
			gtk_entry_set_text(GTK_ENTRY(data->ST_number), item->number);
		else
			gtk_entry_set_text(GTK_ENTRY(data->ST_number), "");

		ui_grp_comboboxentry_set_active(GTK_COMBO_BOX(data->ST_group), item->kgrp);

		if(item->website != NULL)
			gtk_entry_set_text(GTK_ENTRY(data->ST_website), item->website);
		else
			gtk_entry_set_text(GTK_ENTRY(data->ST_website), "");

		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->TB_notes));
		GtkTextIter iter;

		gtk_text_buffer_set_text (buffer, "", 0);
		gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
		if(item->notes != NULL)
			gtk_text_buffer_insert (buffer, &iter, item->notes, -1);	

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_nobudget), item->flags & AF_NOBUDGET);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_nosummary), item->flags & AF_NOSUMMARY);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_noreport), item->flags & AF_NOREPORT);
		//#1896441 outflow summary
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_outflowsum), item->flags & AF_OUTFLOWSUM);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_closed), item->flags & AF_CLOSED);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_initial), item->initial);
		//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_warning), item->warning);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_minimum), item->minimum);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_maximum), item->maximum);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_cheque1), item->cheque1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_cheque2), item->cheque2);

		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_template), item->karc);
		//g_snprintf(idbuffer, 11, "%d", item->karc);
		//gtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_template), idbuffer);

	}

}

/*
static gboolean ui_acc_manage_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
	ui_acc_manage_get(widget, user_data);
	return FALSE;
}
*/

/*
** update the widgets status and contents from action/selection value
*/
static void ui_acc_manage_update(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
gboolean selected, sensitive, canup, candw;
guint32 key;
gint sort_column_id;
GtkSortType sort_order;

	DB( g_print("\n(ui_acc_manage_update)\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//window = gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW);
	//DB( g_print("(defpayee) widget=%08lx, window=%08lx, inst_data=%08lx\n", treeview, window, data) );

	//if true there is a selected node
	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), &model, &iter);
	key = ui_acc_listview_get_selected_key(GTK_TREE_VIEW(data->LV_acc));

	DB( g_print(" -> selected = %d  action = %d key = %d\n", selected, data->action, key) );


	sensitive = (selected == TRUE) ? TRUE : FALSE;

	gtk_widget_set_sensitive(data->notebook, sensitive);

	sensitive = (selected == TRUE && data->action == 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_rem, sensitive);

	canup = candw = selected;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc));
	sort_column_id = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
	sort_order = GTK_SORT_DESCENDING;
	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &sort_column_id, &sort_order);
	DB( g_print(" sort is %d colid=%d order=%d (ok is %d %d)\n", valid, sort_column_id, sort_order, LST_DEFACC_SORT_POS, GTK_SORT_ASCENDING) );

	if( !((sort_column_id == LST_DEFACC_SORT_POS) && (sort_order == GTK_SORT_ASCENDING)) )
	{
		canup = candw = FALSE;
		DB( g_print(" sort is not by position ASC\n") );
		goto next;
	}

	if( selected == TRUE )
	{
	Account *item;

		gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &item, -1);
		
		DB( g_print(" item pos is %d\n", item->pos) );

		canup = (item->pos <= 1) ? FALSE : TRUE;
		candw = (item->pos >= da_acc_length()) ? FALSE : TRUE;
	}

next:
	DB( g_print(" can up=%d dw=%d\n", canup, candw) );
	gtk_widget_set_sensitive(data->BT_up  , canup);
	gtk_widget_set_sensitive(data->BT_down, candw);


	if(selected)
	{
		if(key != data->lastkey)
		{
			DB( g_print(" -> should first do a get for account %d\n", data->lastkey) );
			ui_acc_manage_getlast(data);
		}

		ui_acc_manage_set(widget, NULL);
	}

	data->lastkey = key;

}


static void ui_acc_manage_cb_move_updown(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
GtkDirectionType direction = GPOINTER_TO_INT(user_data);
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
gboolean hasprvnxt;
Account *curitem, *prvnxtitem;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-acc-manage] up/down (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &curitem, -1);
		hasprvnxt = FALSE;
		if( direction == GTK_DIR_UP )
			hasprvnxt = gtk_tree_model_iter_previous(model, &iter);
		else if( direction == GTK_DIR_DOWN )
			hasprvnxt = gtk_tree_model_iter_next(model, &iter);
		
		if( hasprvnxt == TRUE )
		{
		gushort tmp = curitem->pos;
		
			gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &prvnxtitem, -1);
			//swap position
			curitem->pos = prvnxtitem->pos;
			prvnxtitem->pos = tmp;
			//#1999243 add change
			data->change++;
			
			ui_acc_listview_sort_force(GTK_TREE_SORTABLE(model), NULL);
		}
	}
}



/*
** add an empty new account to our temp GList and treeview
*/
static void ui_acc_manage_add(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
Account *item;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_acc_manage_add) data=%p\n", data) );

	gchar *name = dialog_get_name(_("Account name"), NULL, GTK_WINDOW(data->dialog));
	if(name != NULL)
	{
		if(account_exists(name))
		{
			ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
				_("Error"),
				_("Cannot add an account '%s',\n"
				"this name already exists."),
				name
				);
		}
		else
		{
			item = da_acc_malloc();
			item->name = name; //g_strdup_printf( _("(account %d)"), da_acc_length()+1);
			item->kcur = GLOBALS->kcur;

			g_strstrip(item->name);
			
			if( strlen(item->name) > 0 )
			{
				if( da_acc_append(item) )
				{
					ui_acc_listview_add(GTK_TREE_VIEW(data->LV_acc), item);
					data->change++;
				}
			}
		}
	}
}

/*
** delete the selected account to our treeview and temp GList
*/
static void ui_acc_manage_delete(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
guint32 key;
gint result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_acc_manage_remove) data=%p\n", data) );

	key = ui_acc_listview_get_selected_key(GTK_TREE_VIEW(data->LV_acc));
	if( key > 0 )
	{
	Account *item = da_acc_get(key);
	guint usagecode = account_is_used(key);
		
		if( usagecode != ACC_USAGE_NONE )
		{
		gchar *title, *reason;

			title = g_strdup_printf (
				_("Cannot delete account '%s'"), item->name);

			switch(usagecode)
			{
				case ACC_USAGE_TXN     : reason = _("It has transaction"); break;
				case ACC_USAGE_TXN_XFER: reason = _("It is target of xfer transaction"); break;
				case ACC_USAGE_ARC     : reason = _("It has scheduled/template"); break;
				case ACC_USAGE_ARC_XFER: reason = _("It is target of xfer scheduled/template"); break;
				default: reason = ""; break;
			}

			ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
				title,
				reason
				//_("This account contains transactions and/or is part of internal transfers.")
			);

			g_free(title);
		}
		else
		{
		gchar *title;
		gchar *secondtext;

			title = g_strdup_printf (
				_("Are you sure you want to permanently delete '%s'?"), item->name);

			secondtext = _("If you delete an account, it will be permanently lost.");
			
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
				da_acc_delete(key);
				ui_acc_listview_remove_selected(GTK_TREE_VIEW(data->LV_acc));
				data->change++;
			}
		
		}
	}
}


/*
** rename the selected account to our treeview and temp GList
*/
static void ui_acc_manage_rename(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
Account *item;
guint32 key;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_acc_manage_rename) data=%p\n", data) );

	key = ui_acc_listview_get_selected_key(GTK_TREE_VIEW(data->LV_acc));
	if( key > 0 )
	{
		item = da_acc_get(key);

		gchar *name = dialog_get_name(_("Account name"), item->name, GTK_WINDOW(data->dialog));
		if(name != NULL)
		{
			if(account_rename(item, name))
			{
				gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_acc));
				data->change++;
			}
			else
			{
				ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
					_("Error"),
					_("Cannot rename this Account,\n"
					"from '%s' to '%s',\n"
					"this name already exists."),
					item->name,
					name
				    );
			}
		}
	}
}


static void ui_acc_manage_toggled_nobudget(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
GtkTreePath			*path;
Account 	*accitem;
gboolean selected, active;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_acc_manage_toggled_nobudget) data=%p\n", data) );

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), &model, &iter);

	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &accitem, -1);
		accitem->flags &= ~(AF_NOBUDGET);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_nobudget));
		if(active) accitem->flags |= AF_NOBUDGET;

		/* redraw the row to display/hide the icon */
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free (path);

		//	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_arc));
		//gtk_widget_queue_draw (GTK_WIDGET(data->LV_arc));
	}	

}


static void ui_acc_manage_toggled_closed(GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
GtkTreePath			*path;
Account 	*accitem;
gboolean selected, active;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_acc_manage_toggled_closed) data=%p\n", data) );

	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), &model, &iter);

	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DEFACC_DATAS, &accitem, -1);
		accitem->flags &= ~(AF_CLOSED);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_closed));
		if(active) accitem->flags |= AF_CLOSED;

		/* redraw the row to display/hide the icon */
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free (path);

		//	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_arc));
		//gtk_widget_queue_draw (GTK_WIDGET(data->LV_arc));
	}	

}


static void ui_acc_manage_rowactivated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
//struct account_data *data;

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	ui_acc_manage_rename(GTK_WIDGET(treeview), NULL);
	
}




/*
**
*/
static void ui_acc_manage_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_acc_manage_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static void
ui_acc_manage_sort_changed(GtkTreeSortable *sortable, gpointer user_data)
{
struct ui_acc_manage_data *data = user_data;

	DB( g_printf("\n[ui-acc-manage] sort changed\n") );

	ui_acc_manage_update(data->dialog, NULL);
}



//gint ui_acc_manage_list_sort(struct _Account *a, struct _Account *b) { return( a->acc_Id - b->acc_Id); }

/*
**
*/
static gboolean ui_acc_manage_cleanup(struct ui_acc_manage_data *data, gint result)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
guint32 i;
guint32 key;
gboolean doupdate = FALSE;

	DB( g_print("\n(ui_acc_manage_cleanup) %p\n", data) );

		key = ui_acc_listview_get_selected_key(GTK_TREE_VIEW(data->LV_acc));
		if(key > 0)
		{
			data->lastkey = key;
			DB( g_print(" -> should first do a get for account %d\n", data->lastkey) );
			ui_acc_manage_getlast(data);
		}

		// test for change & store new position
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc));
		i=1; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
		while (valid)
		{
		Account *item;

			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
				LST_DEFACC_DATAS, &item,
				-1);

			DB( g_print(" -> check acc %d, pos is %d, %s\n", i, item->pos, item->name) );

			if(item->pos != i)
				data->change++;

			item->pos = i;

			// Make iter point to the next row in the list store 
			i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
		}

	GLOBALS->changes_count += data->change;

	group_delete_unused();
	
	return doupdate;
}


static void
ui_acc_manage_search_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct ui_acc_manage_data *data = user_data;

	DB( g_printf("\n[ui_acc_manage_dialog] search_changed_cb\n") );

	ui_acc_manage_dialog_refilter(data);
}


/*
**
*/
static void ui_acc_manage_setup(struct ui_acc_manage_data *data)
{
GList *tmplist;
GString *tpltitle;

	DB( g_print("\n(ui_acc_manage_setup)\n") );

	DB( g_print(" init data\n") );

	//init GList
	data->tmp_list = NULL; //hb-glist_clone_list(GLOBALS->acc_list, sizeof(struct _Account));
	data->action = 0;
	data->change = 0;
	data->lastkey = 0;

	DB( g_print(" populate\n") );

	ui_acc_listview_populate(data->LV_acc, ACC_LST_INSERT_NORMAL, NULL);
	ui_cur_combobox_populate(GTK_COMBO_BOX(data->CY_curr), GLOBALS->h_cur);
	//populate_view_acc(data->LV_acc, GLOBALS->acc_list, TRUE);

	ui_grp_comboboxentry_populate(GTK_COMBO_BOX(data->ST_group), GLOBALS->h_grp);

	//TODO: move to ui-archive
	DB( g_print(" populate tpl start\n") );

	//setting wrap causes O(n^2) performance regression because after
	//every insert the drop-down list size is re-computed.
	//gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(data->CY_template), 0);

	//populate template
	hbtk_combo_box_text_append(GTK_COMBO_BOX(data->CY_template), 0, _("(none)"));

	tpltitle = g_string_sized_new(255);

	tmplist = g_list_first(GLOBALS->arc_list);
	while (tmplist != NULL)
	{
	Archive *item = tmplist->data;
		if( !(item->flags & OF_AUTO) )
		{
			da_archive_get_display_label(tpltitle, item);
			hbtk_combo_box_text_append(GTK_COMBO_BOX(data->CY_template), item->key, tpltitle->str);
		}
		tmplist = g_list_next(tmplist);
	}
	
	g_string_free(tpltitle, TRUE);

	//gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_template), 0);
	//gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(data->CY_template), 1);

	DB( g_print(" populate tpl end\n") );

	//DB( g_print(" set widgets default\n") );

	DB( g_print(" connect widgets signals\n") );

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), "changed", G_CALLBACK (ui_acc_manage_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_acc), "row-activated", G_CALLBACK (ui_acc_manage_rowactivated), GINT_TO_POINTER(2));
	g_signal_connect (gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc)), "sort-column-changed", G_CALLBACK (ui_acc_manage_sort_changed), data);

	gtk_tree_view_set_search_entry(GTK_TREE_VIEW(data->LV_acc), GTK_ENTRY(data->ST_search));
	g_signal_connect (G_OBJECT (data->ST_search), "search-changed", G_CALLBACK (ui_acc_manage_search_changed_cb), data);

	g_signal_connect (data->CY_curr  , "changed", G_CALLBACK (ui_acc_manage_changed_curr_cb), NULL);
	g_signal_connect (data->CM_closed, "toggled", G_CALLBACK (ui_acc_manage_toggled_closed), NULL);
	g_signal_connect (data->CM_nobudget, "toggled", G_CALLBACK (ui_acc_manage_toggled_nobudget), NULL);
	
	g_signal_connect (G_OBJECT (data->BT_add) , "clicked", G_CALLBACK (ui_acc_manage_add), NULL);
	g_signal_connect (G_OBJECT (data->BT_edit), "clicked", G_CALLBACK (ui_acc_manage_rename), NULL);
	g_signal_connect (G_OBJECT (data->BT_rem) , "clicked", G_CALLBACK (ui_acc_manage_delete), NULL);

	g_signal_connect (G_OBJECT (data->BT_up  ), "clicked", G_CALLBACK (ui_acc_manage_cb_move_updown), GUINT_TO_POINTER(GTK_DIR_UP));
	g_signal_connect (G_OBJECT (data->BT_down), "clicked", G_CALLBACK (ui_acc_manage_cb_move_updown), GUINT_TO_POINTER(GTK_DIR_DOWN));

}


static gboolean
ui_acc_manage_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct ui_acc_manage_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n(ui_acc_manage_mapped)\n") );

	ui_acc_manage_setup(data);
	ui_acc_manage_update(data->LV_acc, NULL);

	data->mapped_done = TRUE;

	return FALSE;
}


/*
**
*/
GtkWidget *ui_acc_manage_dialog (void)
{
struct ui_acc_manage_data *data;
GtkWidget *dialog, *content, *mainbox, *vbox, *scrollwin, *notebook;
GtkWidget *content_grid, *group_grid, *tbar, *bbox;
GtkWidget *label, *widget, *treeview, *hpaned;
gint w, h, dw, dh, row;

	data = g_malloc0(sizeof(struct ui_acc_manage_data));
	if(!data) return NULL;
	
	dialog = gtk_dialog_new_with_buttons (_("Manage Accounts"),
				GTK_WINDOW(GLOBALS->mainwindow),
				0,
			    _("_Close"),
			    GTK_RESPONSE_ACCEPT,
				NULL);

	data->dialog = dialog;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 4:3
	dw = (dh * 4) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);
	
	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print("(ui_acc_manage_) dialog=%p, inst_data=%p\n", dialog, data) );

	//window contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (content), mainbox, TRUE, TRUE, 0);
	hb_widget_set_margin(GTK_WIDGET(mainbox), SPACING_LARGE);

	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start (GTK_BOX (mainbox), hpaned, TRUE, TRUE, 0);

	// set paned position
	//w = w/PHI;
	//gtk_paned_set_position(GTK_PANED(hpaned), w - (w/PHI));

	/* left area */
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_end(vbox, SPACING_SMALL);
	gtk_paned_pack1 (GTK_PANED(hpaned), vbox, FALSE, FALSE);

	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	hb_widget_set_margins(GTK_WIDGET(widget), 0, 0, SPACING_MEDIUM, 0);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);


 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	treeview = ui_acc_listview_new(FALSE);
	data->LV_acc = treeview;
	gtk_widget_set_size_request(treeview, HB_MINWIDTH_LIST, -1);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);

	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);
	
		widget = make_image_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_add = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_rem = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_EDIT, _("Rename"));
		data->BT_edit = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);
	
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_MOVE_UP, _("Move up"));
		data->BT_up = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_MOVE_DOWN, _("Move down"));
		data->BT_down = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);


	
	/* right area */
	notebook = gtk_notebook_new();
	data->notebook = notebook;
	gtk_widget_set_margin_start(notebook, SPACING_SMALL);
	gtk_paned_pack2 (GTK_PANED(hpaned), notebook, FALSE, FALSE);

	/* page :: General */
	content_grid = gtk_grid_new();
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	hb_widget_set_margin(GTK_WIDGET(content_grid), SPACING_MEDIUM);
	label = gtk_label_new(_("General"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content_grid, label);

	// group :: Account
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, 0, 1, 1);

	//label = make_label_group(_("Account"));
	//gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 0;
	label = make_label_widget(_("_Type:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_ACC_TYPE);
	data->CY_type = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Group:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	//widget = make_string(label);
	widget = ui_grp_comboboxentry_new(label);
	data->ST_group = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	row++;
	label = make_label_widget(_("_Institution:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_institution = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	row++;
	label = make_label_widget(_("N_umber:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_number = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);
	
	row++;
	label = make_label_widget(_("Start _balance:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_amount(label);
	data->ST_initial = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Currency:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_cur_combobox_new(label);
	data->CY_curr = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);
	
	row++;
	widget = gtk_check_button_new_with_mnemonic (_("this account was _closed"));
	data->CM_closed = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	//5.7
	row++;
	label = make_label_widget(_("Website:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_website = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	row++;
	label = make_label_widget(_("Notes:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = gtk_text_view_new ();
	//#1697171 add wrap
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widget), GTK_WRAP_WORD);
    scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (scrollwin, -1, 48);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	data->TB_notes = widget;
	gtk_grid_attach (GTK_GRID (group_grid), scrollwin, 2, row, 2, 1);
	
	
	/* page :: Behaviour */
	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	hb_widget_set_margin(GTK_WIDGET(content_grid), SPACING_MEDIUM);
	label = gtk_label_new(_("Behaviour"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content_grid, label);
	
	// group :: miscelleaneous
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, 1, 1, 1);

	label = make_label_group(_("Automation"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 2, 1);
	
	row = 1;
	label = make_label_widget(_("Default _Template:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new(label);
	data->CY_template = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

/* test not working
GValue gvalue = G_VALUE_INIT;

	g_value_init (&gvalue, G_TYPE_BOOLEAN);
	g_value_set_boolean (&gvalue, TRUE);
	g_object_set_property(data->CY_template, "appears-as-list", &gvalue);
*/


	// group :: Report exclusion
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, 2, 1, 1);

	label = make_label_group(_("Report exclusion"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 2, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("exclude from account _summary"));
	data->CM_nosummary = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	//#1896441 outflow summary
	row++;
	widget = gtk_check_button_new_with_mnemonic (_("outflow into summary"));
	data->CM_outflowsum= widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	
	row++;
	widget = gtk_check_button_new_with_mnemonic (_("exclude from the _budget"));
	data->CM_nobudget = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("exclude from any _reports"));
	data->CM_noreport = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);


	/* page :: Misc. */
	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	hb_widget_set_margin(GTK_WIDGET(content_grid), SPACING_MEDIUM);
	label = gtk_label_new(_("Misc."));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content_grid, label);

	// group :: Current check number
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, 3, 1, 1);

	label = make_label_group(_("Current check number"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_widget(_("Checkbook _1:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_long (label);
	data->ST_cheque1 = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Checkbook _2:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_long (label);
	data->ST_cheque2 = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	// group :: Institution
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, 0, 1, 1);

	label = make_label_group(_("Balance limits"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row++;
	label = make_label_widget(_("_Overdraft at:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_amount(label);
	data->ST_minimum = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Max_imum:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_amount(label);
	data->ST_maximum = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//TODO: warning/absolute minimum balance
	/*
	label = make_label_widget(_("_Warning at:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_amount(label);
	data->ST_warning = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);
	*/


	// connect dialog signals
	g_signal_connect (dialog, "destroy"  , G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_acc_manage_mapped), &dialog);
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_acc_manage_cb_on_key_press), (gpointer)data);

	// setup, init and show dialog
	//moved to mapped-event
	//g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), "changed", G_CALLBACK (ui_acc_manage_selection), NULL);
	//g_signal_connect (GTK_TREE_VIEW(data->LV_acc), "row-activated", G_CALLBACK (ui_acc_manage_rowactivated), GINT_TO_POINTER(2));
	//g_signal_connect (data->CY_curr  , "changed", G_CALLBACK (ui_acc_manage_changed_curr_cb), NULL);
	//g_signal_connect (data->CM_closed, "toggled", G_CALLBACK (ui_acc_manage_toggled_closed), NULL);
	//g_signal_connect (data->CM_nobudget, "toggled", G_CALLBACK (ui_acc_manage_toggled_nobudget), NULL);
	//g_signal_connect (G_OBJECT (data->BT_add) , "clicked", G_CALLBACK (ui_acc_manage_add), NULL);
	//g_signal_connect (G_OBJECT (data->BT_edit), "clicked", G_CALLBACK (ui_acc_manage_rename), NULL);
	//g_signal_connect (G_OBJECT (data->BT_rem) , "clicked", G_CALLBACK (ui_acc_manage_delete), NULL);
	//setup, init and show dialog
	//ui_acc_manage_setup(data);
	//ui_acc_manage_update(data->LV_acc, NULL);

	//avoid focus on search
	gtk_window_set_focus(GTK_WINDOW(dialog), data->BT_add);

	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all (dialog);

	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup & destroy
	ui_acc_manage_cleanup(data, result);
	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);
	
	return NULL;
}


