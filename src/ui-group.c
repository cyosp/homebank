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

#include "ui-group.h"

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


/**
 * ui_grp_comboboxentry_get_key_add_new:
 *
 * get the key of the active group
 * and create the group if it do not exists
 *
 * Return value: the key or 0
 *
 */
guint32
ui_grp_comboboxentry_get_key_add_new(GtkComboBox *entry_box)
{
guint32 retval;
gchar *name;
Group *item;

	retval = 0;
	
	name = (gchar *)gtk_entry_get_text(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (entry_box))));
	item = da_grp_get_by_name(name);
	if( item == NULL )
	{
	gchar *stripname;

		stripname = g_strdup(name);
		g_strstrip(stripname);
		if( strlen(stripname) > 0 )
		{
			/* automatic add */
			//todo: check prefs + ask the user here 1st time
			item = da_grp_malloc();
			item->name = g_strdup(name);
			da_grp_append(item);
			ui_grp_comboboxentry_add(entry_box, item);
			retval = item->key;
		}
		g_free(stripname);
	}
	else
		retval = item->key;
	
	return retval;
}

/**
 * ui_grp_comboboxentry_get_key:
 *
 * get the key of the active group
 *
 * Return value: the key or 0
 *
 */
guint32
ui_grp_comboboxentry_get_key(GtkComboBox *entry_box)
{
gchar *name;
Group *item;

	name = (gchar *)gtk_entry_get_text(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (entry_box))));
	item = da_grp_get_by_name(name);

	if( item != NULL )
		return item->key;

	return 0;
}


Group
*ui_grp_comboboxentry_get(GtkComboBox *entry_box)
{
gchar *name;
Group *item = NULL;

	DB( g_print ("ui_grp_comboboxentry_get()\n") );

	name = (gchar *)gtk_entry_get_text(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (entry_box))));
	item = da_grp_get_by_name(name);

	return item;
}


gboolean
ui_grp_comboboxentry_set_active(GtkComboBox *entry_box, guint32 key)
{
Group *item;

	DB( g_print ("ui_grp_comboboxentry_set_active()\n") );

	DB( g_print("- key:%d\n", key) );
	
	if( key > 0 )
	{
		item = da_grp_get(key);
		if( item != NULL )
		{
			DB( g_print("- set combo to '%s'\n", item->name) );

			gtk_entry_set_text(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (entry_box))), item->name);
			return TRUE;
		}
	}

	DB( g_print("- set combo to ''\n") );
	
	gtk_entry_set_text(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (entry_box))), "");
	return FALSE;
}

/**
 * ui_grp_comboboxentry_add:
 *
 * Add a single element (useful for dynamics add)
 *
 * Return value: --
 *
 */
void
ui_grp_comboboxentry_add(GtkComboBox *entry_box, Group *grp)
{
	if( grp->name != NULL )
	{
	GtkTreeModel *model;
	GtkTreeIter  iter;

		model = gtk_combo_box_get_model(GTK_COMBO_BOX(entry_box));

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, grp->name, -1);
	}
}

static void
ui_grp_comboboxentry_populate_ghfunc(gpointer key, gpointer value, struct grpPopContext *ctx)
{
GtkTreeIter  iter;
Group *grp = value;

	if( ( grp->key != ctx->except_key ) )
	{
		//gtk_list_store_append (GTK_LIST_STORE(ctx->model), &iter);
		//gtk_list_store_set (GTK_LIST_STORE(ctx->model), &iter, 0, grp->name, -1);
		gtk_list_store_insert_with_values(GTK_LIST_STORE(ctx->model), &iter, -1,
			0, grp->name, -1);
	}
}

/**
 * ui_grp_comboboxentry_populate:
 *
 * Populate the list and completion
 *
 * Return value: --
 *
 */
void
ui_grp_comboboxentry_populate(GtkComboBox *entry_box, GHashTable *hash)
{
	ui_grp_comboboxentry_populate_except(entry_box, hash, -1);
}

void
ui_grp_comboboxentry_populate_except(GtkComboBox *entry_box, GHashTable *hash, guint except_key)
{
GtkTreeModel *model;
//GtkEntryCompletion *completion;
struct grpPopContext ctx;

    DB( g_print ("ui_grp_comboboxentry_populate\n") );

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(entry_box));
	//completion = gtk_entry_get_completion(GTK_ENTRY (gtk_bin_get_child(GTK_BIN (entry_box))));

	/* keep our model alive and detach from comboboxentry and completion */
	//g_object_ref(model);
	//gtk_combo_box_set_model(GTK_COMBO_BOX(entry_box), NULL);
	//gtk_entry_completion_set_model (completion, NULL);

	/* clear and populate */
	ctx.model = model;
	ctx.except_key = except_key;
	gtk_list_store_clear (GTK_LIST_STORE(model));

	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_LIST_STORE(model)), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	g_hash_table_foreach(hash, (GHFunc)ui_grp_comboboxentry_populate_ghfunc, &ctx);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	/* reatach our model */
	//g_print("reattach\n");
	//gtk_combo_box_set_model(GTK_COMBO_BOX(entry_box), model);
	//gtk_entry_completion_set_model (completion, model);
	//g_object_unref(model);
	
}


static gint
ui_grp_comboboxentry_compare_func (GtkTreeModel *model, GtkTreeIter  *a, GtkTreeIter  *b, gpointer      userdata)
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


static void
ui_grp_comboboxentry_test (GtkCellLayout   *cell_layout,
		   GtkCellRenderer *cell,
		   GtkTreeModel    *tree_model,
		   GtkTreeIter     *iter,
		   gpointer         data)
{
gchar *name;

	gtk_tree_model_get(tree_model, iter,
		0, &name,
		-1);

	if( !name )
		g_object_set(cell, "text", _("(no group)"), NULL);
	else
		g_object_set(cell, "text", name, NULL);

	//leak
	g_free(name);

}

/**
 * ui_grp_comboboxentry_new:
 *
 * Create a new group comboboxentry
 *
 * Return value: the new widget
 *
 */
GtkWidget *
ui_grp_comboboxentry_new(GtkWidget *label)
{
GtkListStore *store;
GtkWidget *comboboxentry;
GtkEntryCompletion *completion;
GtkCellRenderer    *renderer;

    DB( g_print ("ui_grp_comboboxentry_new()\n") );

	store = gtk_list_store_new (1,
		G_TYPE_STRING
		);
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_grp_comboboxentry_compare_func, NULL, NULL);

    completion = gtk_entry_completion_new ();
    gtk_entry_completion_set_model (completion, GTK_TREE_MODEL(store));
	g_object_set(completion, "text-column", 0, NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), renderer, "text", 0, NULL);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (completion),
					    renderer,
					    ui_grp_comboboxentry_test,
					    NULL, NULL);

	// dothe same for combobox

	comboboxentry = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(comboboxentry), 0);

	gtk_cell_layout_clear(GTK_CELL_LAYOUT (comboboxentry));

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comboboxentry), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (comboboxentry), renderer, "text", 0, NULL);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (comboboxentry),
					    renderer,
					    ui_grp_comboboxentry_test,
					    NULL, NULL);



	gtk_entry_set_completion (GTK_ENTRY (gtk_bin_get_child(GTK_BIN (comboboxentry))), completion);

	g_object_unref(store);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), comboboxentry);

	gtk_widget_set_size_request(comboboxentry, HB_MINWIDTH_LIST, -1);

	return comboboxentry;
}
