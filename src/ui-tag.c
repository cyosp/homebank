/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2022 Maxime DOYEN
 *
 *  This file is part of HomeBank.
 *
 *  HomeBank is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  HomeBank is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty ofdeftransaction_amountchanged
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "homebank.h"

#include "hb-tag.h"
#include "ui-tag.h"


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


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

//TODO: ui_tag_combobox still used in rep_time


guint32
ui_tag_combobox_get_key(GtkComboBoxText *combobox)
{
	return hbtk_combo_box_get_active_id(combobox);
}


void
ui_tag_combobox_populate_except(GtkComboBoxText *combobox, guint except_key)
{
GList *ltag, *list;
	
	//populate template
	//hbtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combobox), 0, "----");
	//gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	ltag = list = tag_glist_sorted(1);
	while (list != NULL)
	{
	Tag *item = list->data;

		if( item->key != except_key )
		{

			DB( g_print(" populate: %d\n", item->key) );

			hbtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combobox), item->key, item->name);
		}

		list = g_list_next(list);
	}
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	g_list_free(ltag);
}


void
ui_tag_combobox_populate(GtkComboBoxText *combobox)
{
	ui_tag_combobox_populate_except(combobox, -1);
}


GtkWidget *
ui_tag_combobox_new(GtkWidget *label)
{
GtkWidget *combobox;

	combobox = hbtk_combo_box_new(label);
	return combobox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
ui_tag_popover_cb_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
GtkWidget *entry = user_data;
GtkAllocation allocation;
GtkPopover *popover;

    DB( g_print ("[tag popover] open\n") );

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


static void ui_tag_popover_cb_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
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
		Tag *item;

			gtk_tree_model_get(model, &iter, LST_DEFTAG_DATAS, &item, -1);

			hbtk_entry_tag_name_append(GTK_ENTRY(user_data), item->name);
		}
	}
}


GtkWidget *
ui_tag_popover_list(GtkWidget *entry)
{
GtkWidget *box, *menubutton, *image, *scrollwin, *treeview;

	menubutton = gtk_menu_button_new ();
	image = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(menubutton), image);

	gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_LEFT );
	//gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
	gtk_widget_show_all(menubutton);

	//GtkWidget *template = ui_popover_tpl_create(data);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_box_pack_start(GTK_BOX(box), scrollwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	treeview = ui_tag_listview_new(FALSE);
	//data.LV_tag = treeview;
	gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
	gtk_widget_show_all(box);

	gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(treeview), TRUE);

	
	GtkWidget *popover = create_popover (menubutton, box, GTK_POS_LEFT);
	//gtk_widget_set_size_request (popover, HB_MINWIDTH_LIST, HB_MINHEIGHT_LIST);
	gtk_widget_set_vexpand(popover, TRUE);

	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);

	ui_tag_listview_populate(treeview, 0);

	g_signal_connect (menubutton, "toggled", G_CALLBACK (ui_tag_popover_cb_toggled), entry);

	g_signal_connect (treeview, "row-activated", G_CALLBACK (ui_tag_popover_cb_row_activated), entry);
	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_popover_popdown), popover);
	#else
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_widget_hide), popover);
	#endif

	return menubutton;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void ui_tag_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gboolean toggled;

	DB( g_print("(ui_tag_listview) toggle_to_filter\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_tag));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Tag *tagitem;

		gtk_tree_model_get (model, &iter,
			LST_DEFTAG_TOGGLE, &toggled,
			LST_DEFTAG_DATAS, &tagitem,
			-1);

		DB( g_print(" get tag k:%3d = %d (%s)\n", tagitem->key, toggled, tagitem->name) );
		da_flt_status_tag_set(filter, tagitem->key, toggled);
		
		//tagitem->flt_select = toggled;

		/* Make iter point to the next row in the list store */
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static void
ui_tag_listview_toggled_cb (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, LST_DEFTAG_TOGGLE, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFTAG_TOGGLE, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}

void
ui_tag_listview_quick_select(GtkTreeView *treeview, const gchar *uri)
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
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFTAG_TOGGLE, TRUE, -1);
				break;
			case HB_LIST_QUICK_SELECT_NONE:
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFTAG_TOGGLE, FALSE, -1);
				break;
			case HB_LIST_QUICK_SELECT_INVERT:
					gtk_tree_model_get (model, &iter, LST_DEFTAG_TOGGLE, &toggle, -1);
					toggle ^= 1;
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFTAG_TOGGLE, toggle, -1);
				break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static gint
ui_tag_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint retval = 0;
Tag *entry1, *entry2;
//gchar *name1, *name2;

    gtk_tree_model_get(model, a, LST_DEFTAG_DATAS, &entry1, -1);
    gtk_tree_model_get(model, b, LST_DEFTAG_DATAS, &entry2, -1);

	retval = hb_string_utf8_compare(entry1->name, entry2->name);

    return retval;
}


static void
ui_tag_listview_name_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Tag *entry;
gchar *name;

	gtk_tree_model_get(model, iter, LST_DEFTAG_DATAS, &entry, -1);
	if(entry->key == 0)
		name = _("(no tag)");
	else
		name = entry->name;

	g_object_set(renderer, "text", name, NULL);
}


#if MYDEBUG == 1
static void
ui_tag_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Tag *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DEFTAG_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	



/* = = = = = = = = = = = = = = = = */

/**
 * tag_list_add:
 * 
 * Add a single element (useful for dynamics add)
 * 
 * Return value: --
 *
 */
void
ui_tag_listview_add(GtkTreeView *treeview, Tag *item)
{
	if( item->name != NULL )
	{
	GtkTreeModel *model;
	GtkTreeIter	iter;

		model = gtk_tree_view_get_model(treeview);

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFTAG_TOGGLE, FALSE,
			LST_DEFTAG_DATAS, item,
			-1);

		gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter);

	}
}

guint32
ui_tag_listview_get_selected_key(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Tag *item;

		gtk_tree_model_get(model, &iter, LST_DEFTAG_DATAS, &item, -1);
		
		if( item!= NULL	 )
			return item->key;
	}
	return 0;
}

void
ui_tag_listview_remove_selected(GtkTreeView *treeview)
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


void ui_tag_listview_populate(GtkWidget *view, gint insert_type)
{
GtkTreeModel *model;
GtkTreeIter	iter;
GList *ltag, *list;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	gtk_list_store_clear (GTK_LIST_STORE(model));

	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view */

	/* populate */
	//g_hash_table_foreach(GLOBALS->h_tag, (GHFunc)ui_tag_listview_populate_ghfunc, model);
	ltag = list = g_hash_table_get_values(GLOBALS->h_tag);
	while (list != NULL)
	{
	Tag *item = list->data;
	
		DB( g_print(" populate: %d\n", item->key) );

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFTAG_TOGGLE	, FALSE,
			LST_DEFTAG_DATAS, item,
			-1);

		list = g_list_next(list);
	}
	g_list_free(ltag);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view */
	g_object_unref(model);
}


GtkWidget *
ui_tag_listview_new(gboolean withtoggle)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	// create list store
	store = gtk_list_store_new(NUM_LST_DEFTAG,
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
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_tag_listview_cell_data_function_debugkey, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	#endif


	// column 1: toggle
	if( withtoggle == TRUE )
	{
		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Visible"),
							     renderer,
							     "active", LST_DEFTAG_TOGGLE,
							     NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_NONE);

		g_signal_connect (renderer, "toggled",
			    G_CALLBACK (ui_tag_listview_toggled_cb), store);

		g_object_set_data(G_OBJECT(treeview), "togrdr_data", renderer);
	}

	// column 2: name
	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_tag_listview_name_cell_data_function, GINT_TO_POINTER(LST_DEFTAG_DATAS), NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// treeviewattribute
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW(treeview), TRUE);
	
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_tag_listview_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	return treeview;
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void ui_tag_manage_filter_text_handler (GtkEntry    *entry,
                          const gchar *text,
                          gint         length,
                          gint        *position,
                          gpointer     data)
{
GtkEditable *editable = GTK_EDITABLE(entry);
gint i, count=0;
gchar *result = g_new0 (gchar, length+1);

  for (i=0; i < length; i++)
  {
    if (text[i]==' ')
      continue;
    result[count++] = text[i];
  }


  if (count > 0) {
    g_signal_handlers_block_by_func (G_OBJECT (editable),
                                     G_CALLBACK (ui_tag_manage_filter_text_handler),
                                     data);
    gtk_editable_insert_text (editable, result, count, position);
    g_signal_handlers_unblock_by_func (G_OBJECT (editable),
                                       G_CALLBACK (ui_tag_manage_filter_text_handler),
                                       data);
  }
  g_signal_stop_emission_by_name (G_OBJECT (editable), "insert_text");

  g_free (result);
}


static void
ui_tag_manage_dialog_load_csv( GtkWidget *widget, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data = user_data;
gchar *filename = NULL;
gchar *error;

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("(ui_tag_manage_dialog) load csv - data %p\n", data) );

	if( ui_file_chooser_csv(GTK_WINDOW(data->dialog), GTK_FILE_CHOOSER_ACTION_OPEN, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		if( !tag_load_csv(filename, &error) )
		{
			ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
					_("File format error"),
					_("The CSV file must contains the exact numbers of column,\nseparated by a semi-colon, please see the help for more details.")
					);
		}

		g_free( filename );
		ui_tag_listview_populate(data->LV_tag, 0);
	}
}


static void
ui_tag_manage_dialog_save_csv( GtkWidget *widget, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data = user_data;
gchar *filename = NULL;

	DB( g_print("(ui_tag_manage_dialog) save csv\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( ui_file_chooser_csv(GTK_WINDOW(data->dialog), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		tag_save_csv(filename);
		g_free( filename );
	}
}



/**
 * ui_tag_manage_dialog_add:
 *
 */
static void
ui_tag_manage_dialog_add(GtkWidget *widget, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data;
Tag *item;
gchar *name;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(defayee) add (data=%p)\n", data) );

	name = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_name));

	item = da_tag_malloc ();
	item->name = g_strdup(name);

	g_strstrip(item->name);
	
	if( strlen(item->name) > 0 )
	{
		if( da_tag_append(item) )
		{
			ui_tag_listview_add(GTK_TREE_VIEW(data->LV_tag), item);
			data->change++;
		}
	}
	else
		da_tag_free (item);
		
	gtk_entry_set_text(GTK_ENTRY(data->ST_name), "");
}


static void ui_tag_manage_dialog_edit_entry_cb(GtkEditable *editable, gpointer user_data)
{
GtkDialog *window = user_data;
const gchar *buffer;

	buffer = gtk_entry_get_text(GTK_ENTRY(editable));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(window), GTK_RESPONSE_ACCEPT, strlen(buffer) > 0 ? TRUE : FALSE);
}


static void ui_tag_manage_dialog_edit(GtkWidget *dowidget, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data;
GtkWidget *dialog, *content_area, *content_grid, *group_grid;
GtkWidget *label, *widget;
GtkWidget *ST_name;
gint crow, row;
guint32 key;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(dowidget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(defayee) modify %p\n", data) );

	key = ui_tag_listview_get_selected_key(GTK_TREE_VIEW(data->LV_tag));
	if( key > 0 )
	{
	Tag *item;

		item = da_tag_get( key );

		dialog = gtk_dialog_new_with_buttons (_("Edit Tag"),
						    GTK_WINDOW (data->dialog),
						    0,
						    _("_Cancel"),
						    GTK_RESPONSE_REJECT,
						    _("_OK"),
						    GTK_RESPONSE_ACCEPT,
						    NULL);

		content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

		content_grid = gtk_grid_new();
		gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
		gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
		gtk_container_set_border_width (GTK_CONTAINER(content_grid), SPACING_MEDIUM);
		gtk_box_pack_start (GTK_BOX (content_area), content_grid, TRUE, TRUE, 0);

		crow = 0;
		// group :: General
		group_grid = gtk_grid_new ();
		gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
		gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
		gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);
	
		//label = make_label_group(_("General"));
		//gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

		row = 1;
		label = make_label_widget(_("_Name:"));
		gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
		widget = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(widget), 24);
		ST_name = widget;
		gtk_widget_set_hexpand(widget, TRUE);
		gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

		//setup
		//#1992284 manage tag edit input is empty
		gtk_entry_set_text(GTK_ENTRY(ST_name), item->name);
		gtk_widget_grab_focus (ST_name);

		gtk_entry_set_activates_default (GTK_ENTRY(ST_name), TRUE);

		gtk_dialog_set_default_response(GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT);


		g_signal_connect (G_OBJECT (ST_name), "changed", G_CALLBACK (ui_tag_manage_dialog_edit_entry_cb), dialog);

		gtk_widget_show_all(content_grid);

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
				if( tag_rename(item, name) )
				{
					//to redraw the active entry
					gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_tag));
					data->change++;
				}
				else
				{
					ui_dialog_msg_infoerror(GTK_WINDOW(dialog), GTK_MESSAGE_ERROR,
						_("Error"),
						_("Cannot rename this Tag,\n"
						"from '%s' to '%s',\n"
						"this name already exists."),
						item->name,
						name
						);

				}
			}


	    }

		// cleanup and destroy
		gtk_widget_destroy (dialog);
	}

}


static void ui_tag_manage_dialog_merge(GtkWidget *widget, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data;
GtkWidget *dialog, *content, *mainvbox;
GtkWidget *getwidget, *togglebutton;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(ui_tag_manage_dialog) merge %p\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_tag));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Tag *srctag;
	gchar *title;
	gchar *secondtext;

		gtk_tree_model_get(model, &iter, LST_DEFTAG_DATAS, &srctag, -1);

		title = g_strdup_printf (
			_("Merge tag '%s'"), srctag->name);
		
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

		secondtext = _("Transactions assigned to this tag,\n"
			  "will be moved to the tag selected below.");

		g_object_set(GTK_MESSAGE_DIALOG (dialog), "secondary-text", secondtext, NULL);
		g_free(title);

		getwidget = ui_tag_combobox_new(NULL);
		gtk_box_pack_start (GTK_BOX (mainvbox), getwidget, FALSE, FALSE, 0);

		secondtext = g_strdup_printf (
			_("_Delete the tag '%s'"), srctag->name);
		togglebutton = gtk_check_button_new_with_mnemonic(secondtext);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), TRUE);
		g_free(secondtext);
		gtk_box_pack_start (GTK_BOX (mainvbox), togglebutton, FALSE, FALSE, 0);

		//setup 

		ui_tag_combobox_populate_except(GTK_COMBO_BOX_TEXT(getwidget), srctag->key);
		gtk_widget_grab_focus (getwidget);

		gtk_widget_show_all(mainvbox);
		
		//wait for the user
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		if(result == GTK_RESPONSE_OK)
		{
		GtkTreeModel *model;
		Tag *newtag;
		guint dsttagkey;

			dsttagkey = ui_tag_combobox_get_key(GTK_COMBO_BOX_TEXT(getwidget));

			//do nothing if src = dst...
			if( srctag->key != dsttagkey )
			{
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_tag));
				gtk_list_store_clear (GTK_LIST_STORE(model));

				DB( g_print(" -> move cat to %d (subcat=%d)\n", dstcatkey, dosubcat) );

				tag_move(srctag->key, dsttagkey);

				newtag = da_tag_get(dsttagkey);

				//#1771720: update count
				newtag->usage_count += srctag->usage_count;
				srctag->usage_count = 0;

				// add the new tag to listview
				if(newtag)
					ui_tag_listview_add(GTK_TREE_VIEW(data->LV_tag), newtag);

				// delete the old tag
				if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)) )
				{
					DB( g_print(" -> delete %d '%s'\n", srctag->key, srctag->name ) );

					ui_tag_listview_remove_selected(GTK_TREE_VIEW(data->LV_tag));
					da_tag_delete(srctag->key);
				}

				data->change++;

				ui_tag_listview_populate(data->LV_tag, 0);
			}
		}

		// cleanup and destroy
		gtk_widget_destroy (dialog);
	}
}


/*
** delete the selected tag to our treeview and temp GList
*/
static void ui_tag_manage_dialog_delete(GtkWidget *widget, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data;
Tag *item;
guint32 key;
gint result;


	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(ui_tag_manage_dialog) delete (data=%p)\n", data) );

	key = ui_tag_listview_get_selected_key(GTK_TREE_VIEW(data->LV_tag));
	if( key > 0 )
	{
	gchar *title;
	gchar *secondtext = NULL;

		item = da_tag_get(key);

		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), item->name);

		if( item->usage_count > 0 )
		{
			secondtext = _("This tag is used.\n"
			    "That tag will be deleted from any transaction using it.");
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
			//#1915729 no need to move to 0 (crash), just delete is safe
			//tag_move(key, 0);
			ui_tag_listview_remove_selected(GTK_TREE_VIEW(data->LV_tag));
			da_tag_delete(key);
			data->change++;
		}

	}
}


static void ui_tag_manage_dialog_update(GtkWidget *treeview, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data;
gboolean sensitive;
guint32 key;

	DB( g_print("\n(ui_tag_manage_dialog) cursor changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	key = ui_tag_listview_get_selected_key(GTK_TREE_VIEW(data->LV_tag));

	sensitive = (key > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_merge, sensitive);
	gtk_widget_set_sensitive(data->BT_delete, sensitive);

}


/*
**
*/
static void ui_tag_manage_dialog_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_tag_manage_dialog_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}

static void ui_tag_manage_dialog_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            user_data)
{
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("ui_tag_manage_dialog_onRowActivated()\n") );


	model = gtk_tree_view_get_model(treeview);
	gtk_tree_model_get_iter_first(model, &iter);
	if(gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter) == FALSE)
	{
		ui_tag_manage_dialog_edit(GTK_WIDGET(treeview), NULL);
	}
}


static void ui_tag_manage_setup(struct ui_tag_manage_dialog_data *data)
{

	DB( g_print("\n[ui-tag] setup\n") );

	DB( g_print(" init data\n") );
	data->change = 0;

	DB( g_print(" populate\n") );
	//tag_fill_usage();
	ui_tag_listview_populate(data->LV_tag, 0);


	//DB( g_print(" set widgets default\n") );

	DB( g_print(" connect widgets signals\n") );

	g_object_bind_property (data->BT_add, "active", data->RE_addreveal, "reveal-child", G_BINDING_BIDIRECTIONAL);

	g_signal_connect (G_OBJECT (data->ST_name), "activate", G_CALLBACK (ui_tag_manage_dialog_add), NULL);
	g_signal_connect(G_OBJECT(data->ST_name), "insert-text", G_CALLBACK(ui_tag_manage_filter_text_handler), NULL);
	
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_tag)), "changed", G_CALLBACK (ui_tag_manage_dialog_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_tag), "row-activated", G_CALLBACK (ui_tag_manage_dialog_onRowActivated), NULL);

	g_signal_connect (G_OBJECT (data->BT_edit), "clicked", G_CALLBACK (ui_tag_manage_dialog_edit), NULL);
	g_signal_connect (G_OBJECT (data->BT_merge), "clicked", G_CALLBACK (ui_tag_manage_dialog_merge), NULL);
	g_signal_connect (G_OBJECT (data->BT_delete), "clicked", G_CALLBACK (ui_tag_manage_dialog_delete), NULL);

}


static gboolean
ui_tag_manage_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct ui_tag_manage_dialog_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n(ui_tag_manage_mapped)\n") );

	ui_tag_manage_setup(data);
	ui_tag_manage_dialog_update(data->LV_tag, NULL);

	return FALSE;
}


GtkWidget *ui_tag_manage_dialog (void)
{
struct ui_tag_manage_dialog_data *data;
GtkWidget *dialog, *content, *mainvbox, *box, *bbox, *tbar, *treeview, *scrollwin, *table, *addreveal;
GtkWidget *menu, *menuitem, *widget, *image;
GtkToolItem *toolitem;
gint w, h, dw, dh, row;

	data = g_malloc0(sizeof(struct ui_tag_manage_dialog_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Manage Tags"),
					    GTK_WINDOW(GLOBALS->mainwindow),
						0,
					    _("_Close"), GTK_RESPONSE_ACCEPT,
					    NULL);

	/*dialog = g_object_new (GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Manage Tags"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW(GLOBALS->mainwindow));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	*/
	//gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	
	data->dialog = dialog;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 3:2
	dw = (dh * 2) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);


	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print("(ui_tag_manage_dialog) dialog=%p, inst_data=%p\n", dialog, data) );


	//dialog contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (content), mainvbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(mainvbox), SPACING_LARGE);

    //our table
	table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (mainvbox), table, TRUE, TRUE, 0);

	row = 0;
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), bbox, 0, row, 2, 1);
	//test headerbar
	//content = gtk_dialog_get_header_bar(GTK_DIALOG (dialog));

	menu = gtk_menu_new ();
	gtk_widget_set_halign (menu, GTK_ALIGN_END);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Import CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (ui_tag_manage_dialog_load_csv), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("E_xport CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (ui_tag_manage_dialog_save_csv), data);
	
	//menuitem = gtk_separator_menu_item_new();
	//gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	
	//menuitem = gtk_menu_item_new_with_mnemonic (_("_Delete unused"));
	//gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	//g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (ui_pay_manage_dialog_delete_unused), data);
	
	gtk_widget_show_all (menu);

	widget = gtk_menu_button_new();
	image = gtk_image_new_from_icon_name (ICONNAME_HB_BUTTON_MENU, GTK_ICON_SIZE_MENU);
	g_object_set (widget, "image", image, "popup", GTK_MENU(menu),  NULL);
	gtk_widget_set_halign (widget, GTK_ALIGN_END);
	gtk_box_pack_end(GTK_BOX (bbox), widget, FALSE, FALSE, 0);
	//gtk_header_bar_pack_end(GTK_HEADER_BAR (content), widget);
	
	// list
	row++;
	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_grid_attach (GTK_GRID (table), box, 0, row, 2, 1);
	
	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_add(GTK_CONTAINER(box), scrollwin);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), HB_MINHEIGHT_LIST);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	treeview = ui_tag_listview_new(FALSE);
	data->LV_tag = treeview;
	gtk_container_add(GTK_CONTAINER(scrollwin), treeview);

	tbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size (GTK_TOOLBAR(tbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(tbar), GTK_TOOLBAR_ICONS);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (box), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), bbox);
	gtk_toolbar_insert(GTK_TOOLBAR(tbar), GTK_TOOL_ITEM(toolitem), -1);

		data->BT_add = make_image_toggle_button(ICONNAME_LIST_ADD, _("Add"));
		gtk_container_add (GTK_CONTAINER (bbox), data->BT_add);

		data->BT_delete = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		gtk_container_add (GTK_CONTAINER (bbox), data->BT_delete);

	toolitem = gtk_separator_tool_item_new ();
	//gtk_tool_item_set_expand (toolitem, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolitem), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(tbar), GTK_TOOL_ITEM(toolitem), -1);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), bbox);
	gtk_toolbar_insert(GTK_TOOLBAR(tbar), GTK_TOOL_ITEM(toolitem), -1);

		data->BT_edit = make_image_button(ICONNAME_LIST_EDIT, _("Edit"));
		gtk_container_add (GTK_CONTAINER (bbox), data->BT_edit);

		data->BT_merge = make_image_button(ICONNAME_HB_LIST_MERGE, _("Move/Merge"));
		gtk_container_add (GTK_CONTAINER (bbox), data->BT_merge);

	row++;
	addreveal = gtk_revealer_new ();
	data->RE_addreveal = addreveal;
	gtk_grid_attach (GTK_GRID (table), addreveal, 0, row, 2, 1);
	data->ST_name = gtk_entry_new ();
	gtk_entry_set_placeholder_text(GTK_ENTRY(data->ST_name), _("new tag") );
	gtk_widget_set_hexpand (data->ST_name, TRUE);
	gtk_container_add(GTK_CONTAINER(addreveal), data->ST_name);
	
	
	// connect dialog signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_tag_manage_mapped), &dialog);

	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all (dialog);

	// wait for the user
	gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	GLOBALS->changes_count += data->change;
	
	gtk_widget_destroy (dialog);

	g_free(data);
	
	return NULL;
}

