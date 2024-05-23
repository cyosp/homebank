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

#include "ui-filter.h"
#include "ui-flt-widget.h"


/****************************************************************************/
/* Debug macros										 */
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



gboolean
ui_flt_popover_hub_set (GtkBox *box, gint active_key);

/* = = = = = = = = = = = = = = = = */

/*
**
** The function should return:
** a negative integer if the first value comes before the second,
** 0 if they are equal,
** or a positive integer if the first value comes after the second.
*/
static gint
lst_favfilter_model_compare_func (GtkTreeModel *model, GtkTreeIter  *a, GtkTreeIter  *b, gpointer      userdata)
{
gint retval = 0;
guint32 key1, key2;
gchar *name1, *name2;

    gtk_tree_model_get(model, a, LST_FAVFLT_KEY, &key1, -1);
    gtk_tree_model_get(model, b, LST_FAVFLT_KEY, &key2, -1);

	if( !key1 )
	{
		retval = -1;
	}
	else
	if( !key2 )
	{
		retval = 1;
	}
	else
	{
		gtk_tree_model_get(model, a, LST_FAVFLT_NAME, &name1, -1);
		gtk_tree_model_get(model, b, LST_FAVFLT_NAME, &name2, -1);

		retval = hb_string_utf8_compare(name1, name2);

		g_free(name2);
		g_free(name1);
	}
  	return retval;
}


static gboolean
lst_favfilter_get_iter(GtkTreeModel *model, gint key, GtkTreeIter *return_iter)
{
GtkTreeIter iter;
gboolean match = FALSE;

	if (gtk_tree_model_get_iter_first (model, &iter))
		do {
			gint tmpkey;

			gtk_tree_model_get (model, &iter, LST_FAVFLT_KEY, &tmpkey, -1);
			match = (tmpkey == key) ? TRUE : FALSE;

			if (match)
			{
				*return_iter = iter;
				break;
			}
		} while (gtk_tree_model_iter_next (model, &iter));

	return match;
}



GtkListStore *
lst_lst_favfilter_model_new(void)
{
GtkListStore *store;
GtkTreeIter iter;

	store = gtk_list_store_new (2,
		G_TYPE_INT,
		G_TYPE_STRING);

	// insert none
	gtk_list_store_insert_with_values(store, &iter, 0,
		LST_FAVFLT_KEY, 0,
		LST_FAVFLT_NAME, _("(default)"),
		-1);
		
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), lst_favfilter_model_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	return store;
}


/* = = = = = = = = = = = = = = = = */


void ui_flt_manage_header_sensitive(GtkWidget *widget, gpointer user_data)
{
struct ui_flt_popover_data *data;
gboolean sensitive = FALSE;
Filter *curflt;

	DB( g_print("\n[Fav Filters] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_BOX)), "inst_data");

	//curflt = ui_flt_popover_hub_get(GTK_BOX(data->box), NULL);
	curflt = data->filter;

	DB( g_print(" key: %d '%s', changes:%d\n", curflt->key, curflt->name, curflt->nbchanges) );	

	gtk_widget_set_tooltip_text(data->combobox, curflt->name);

	sensitive = (curflt->nbchanges > 0) ? TRUE : FALSE;
	//key 0 disable all
	sensitive = curflt->key == 0 ? FALSE : sensitive;
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "save")), sensitive);
	//gtk_widget_set_sensitive(data->MI_sav, sensitive);
	//gtk_widget_set_sensitive(data->BT_sav, sensitive);
	//we can always saveas

	sensitive = (curflt->key == 0) ? FALSE : TRUE;
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "rename")), sensitive);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "delete")), sensitive);
	//gtk_widget_set_sensitive(data->MI_ren, sensitive);
	//gtk_widget_set_sensitive(data->MI_del, sensitive);

}


static void ui_flt_manage_header_action_del(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_flt_popover_data *data = user_data;
Filter *curflt;
GtkTreeModel *model;
GtkTreeIter iter;

	DB( g_print("\n[Fav Filters] action del\n") );

	curflt = ui_flt_popover_hub_get(GTK_BOX(data->box), NULL);
	if( curflt )
	{
		DB( g_print(" flt:%d '%s'\n", curflt->key, curflt->name) );

		//TODO: add refcount test + confirmation here

		//TODO: remove from model
		model = gtk_combo_box_get_model (GTK_COMBO_BOX(data->combobox));
		lst_favfilter_get_iter(model, curflt->key, &iter);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		da_flt_remove(curflt->key);

		//set to none
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->combobox), 0);

		GLOBALS->changes_count++;
	}
}



static void
ui_flt_manage_header_action_ren (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_flt_popover_data *data = user_data;
GtkTreeIter iter;
Filter *curflt;

	DB( g_print("\n[Fav Filters] action rename\n") );

	if( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(data->combobox), &iter) )
	{
	gint key;
	
		gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->fltmodel), &iter,
			LST_FAVFLT_KEY, &key,
			-1);

		curflt = da_flt_get(key);

		if( curflt && curflt->key > 0 )
		{
			DB( g_print(" flt:%d '%s'\n", curflt->key, curflt->name) );

			gchar *rawname = dialog_get_name(_("Filter rename"), curflt->name, data->parent);
			if(rawname != NULL)
			{
				//TODO: maybe, no unique label for now
				if( g_strcmp0(curflt->name, rawname) )
				{
				GValue gvalue = G_VALUE_INIT;

					g_free(curflt->name);
					curflt->name = rawname;

					g_value_init (&gvalue, G_TYPE_STRING);
					g_value_set_string (&gvalue, (const gchar*)rawname);
					gtk_list_store_set_value(GLOBALS->fltmodel, &iter,
						LST_FAVFLT_NAME, &gvalue);

					GLOBALS->changes_count++;
				}
			}
		}

	}
}


static void
ui_flt_manage_header_action_new (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_flt_popover_data *data = user_data;
Filter *curflt, *newflt;
gchar *newname;

	DB( g_print("\n[Fav Filters] action save as\n") );

	newname = g_strdup_printf(_("New filter %d"), da_flt_get_max_key());
	gchar *rawname = dialog_get_name(_("Filter name"), newname, data->parent);
	g_free(newname);

	if( rawname != NULL )
	{
		curflt = ui_flt_popover_hub_get(GTK_BOX(data->box), NULL);
		newflt = da_flt_malloc();

		if(curflt == NULL)
			curflt = data->filter;

		if(curflt && newflt)
		{
			da_flt_copy(curflt, newflt);
			g_free(newflt->name);
			newflt->name = rawname;
			da_flt_append(newflt);
			GLOBALS->changes_count++;
		}
		
		//set combo to new filter
		ui_flt_popover_hub_set(GTK_BOX(data->box), newflt->key);
	}
}


static void
ui_flt_manage_header_action_sav (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_flt_popover_data *data = user_data;
Filter *curflt;

	DB( g_print("\n[Fav Filters] action save\n") );

	curflt = ui_flt_popover_hub_get(GTK_BOX(data->box), NULL);
	if( curflt && data->filter )
	{
		data->filter->nbchanges = 0;
		da_flt_copy(data->filter, curflt);
		
		ui_flt_manage_header_sensitive(data->box, NULL);
		
		GLOBALS->changes_count++;
	}
}


gboolean
ui_flt_popover_hub_set (GtkBox *box, gint active_key)
{
struct ui_flt_popover_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
gboolean match = FALSE;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(box), GTK_TYPE_BOX)), "inst_data");

	model = gtk_combo_box_get_model (GTK_COMBO_BOX(data->combobox));

	if (gtk_tree_model_get_iter_first (model, &iter))
		do {
			gint key;

			gtk_tree_model_get (model, &iter, LST_FAVFLT_KEY, &key, -1);
			match = (key == active_key) ? TRUE : FALSE;

			if (match)
			{
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX(data->combobox), &iter);
				break;
			}
		} while (gtk_tree_model_iter_next (model, &iter));

	return match;
}



Filter *
ui_flt_popover_hub_get (GtkBox *box, gpointer user_data)
{
struct ui_flt_popover_data *data;
Filter *item = NULL;
GtkTreeIter iter;

	//DB( g_print("\n[Fav Filters] get key\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(box), GTK_TYPE_BOX)), "inst_data");

	if( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(data->combobox), &iter) )
	{
	gint key;
	
		gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->fltmodel), &iter,
			0, &key,
			-1);
		
		item = da_flt_get(key);
		//DB( g_print(" found: %p for k:%d\n", item, key) );	
	}

	return item;
}


void
ui_flt_popover_hub_save (GtkWidget *widget, gpointer user_data)
{
struct ui_flt_popover_data *data;
GAction *action;

	DB( g_print("\n[Fav Filters] external save\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_BOX)), "inst_data");

	//we need to call sensitive here to enable save
	ui_flt_manage_header_sensitive(data->box, NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (data->actions), "save");
	g_action_activate(action, NULL);

}


GtkWidget *
ui_flt_popover_hub_get_combobox (GtkBox *box, gpointer user_data)
{
struct ui_flt_popover_data *data;

	DB( g_print("\n[Fav Filters] get combobox\n") );

	if(!GTK_IS_BOX(box))
		return NULL;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(box), GTK_TYPE_BOX)), "inst_data");

	return data->combobox;
}


static GtkWidget *
hbtk_kvcombobox_new ()
{
GtkWidget *p_combo;
GtkCellRenderer *p_cell = NULL;

	p_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(GLOBALS->fltmodel));

	p_cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (p_combo), p_cell, FALSE);
	gtk_cell_layout_set_attributes (
		GTK_CELL_LAYOUT (p_combo),
		p_cell, "text", 1,
		NULL
	);

	/*g_object_set(p_cell, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 20,
	    NULL);
	*/

	gtk_combo_box_set_active(GTK_COMBO_BOX(p_combo), 0);

	return p_combo;
}


static const GActionEntry favflt_actions[] = {
//	name, function(), type, state, 
	{ "save"		, ui_flt_manage_header_action_sav				, NULL, NULL , NULL, {0,0,0} },
	{ "new"			, ui_flt_manage_header_action_new				, NULL, NULL , NULL, {0,0,0} },
	{ "rename"		, ui_flt_manage_header_action_ren				, NULL, NULL , NULL, {0,0,0} },
	{ "delete"		, ui_flt_manage_header_action_del				, NULL, NULL , NULL, {0,0,0} },
};


static GtkWidget *
ui_flt_dialog_create_menubutton (struct ui_flt_popover_data *data)
{
GtkWidget *button, *image;
GMenu *menu, *section;

	menu = g_menu_new ();

	section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("_Save")      , "favfltactions.save");
	g_menu_append (section, _("_Save as..."), "favfltactions.new");
	g_menu_append (section, _("_Rename...") , "favfltactions.rename");
	g_menu_append (section, _("_Delete...") , "favfltactions.delete");
	g_object_unref (section);

	GSimpleActionGroup *group = g_simple_action_group_new ();
	data->actions = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), favflt_actions, G_N_ELEMENTS (favflt_actions), data);

	button = gtk_menu_button_new();
	gtk_menu_button_set_direction (GTK_MENU_BUTTON(button), GTK_ARROW_DOWN);
	gtk_widget_set_halign (button, GTK_ALIGN_END);
	image = gtk_image_new_from_icon_name (ICONNAME_HB_BUTTON_MENU, GTK_ICON_SIZE_MENU);
	g_object_set (button, "image", image,  NULL);

	gtk_widget_insert_action_group (button, "favfltactions", G_ACTION_GROUP(group));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), G_MENU_MODEL (menu));

	return button;
}



static void
ui_flt_popover_hub_destroy ( GtkWidget *widget, gpointer user_data )
{
struct ui_flt_popover_data *data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print ("\n\n[Fav Filters] destroy event occurred\n") );

		DB( g_print(" - box=%p, inst_data=%p\n", widget, data) );
	g_free(data);

}

//only used in stats
GtkWidget *
create_popover_widget (GtkWindow *parent, Filter *filter)
{
struct ui_flt_popover_data *data;
GtkWidget *hbox, *widget;
GtkWidget *prtbox;

	DB( g_print ("\n\n[Fav Filters] new\n") );

	data = g_malloc0(sizeof(struct ui_flt_popover_data));
	if(!data) return NULL;

	data->filter = filter;
	data->parent = parent;

	prtbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	data->box = prtbox;
	//hb_widget_set_margin(prtbox, SPACING_SMALL);

	g_object_set_data(G_OBJECT(prtbox), "inst_data", (gpointer)data);
	DB( g_print(" - box=%p, inst_data=%p\n", prtbox, data) );

	// connect our dispose function
	g_signal_connect (prtbox, "destroy", G_CALLBACK (ui_flt_popover_hub_destroy), (gpointer)data);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (prtbox), hbox, FALSE, FALSE, 0);

	//label = make_label_widget(_("Preset:"));
	//gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	widget = hbtk_kvcombobox_new();
	data->combobox = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	widget = ui_flt_dialog_create_menubutton(data);
	data->menubutton = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);


	/*hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_widget_set_halign(hbox, GTK_ALIGN_END);
	gtk_box_pack_start (GTK_BOX (prtbox), hbox, FALSE, FALSE, 0);

		//save as / rename / delete
		widget = make_clicklabel("1", _("Save as"));
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		widget = make_clicklabel("2", _("Rename"));
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		widget = make_clicklabel("3", _("Delete"));
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		//widget = make_clicklabel("4", _("Save"));
		widget = gtk_button_new_with_label(_("Save"));
		data->BT_sav = widget;
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	*/

	ui_flt_manage_header_sensitive(prtbox, data);

	//g_signal_connect (data->BT_sav, "clicked" , G_CALLBACK (ui_flt_manage_header_action_sav), (gpointer)data);
/*	g_signal_connect (data->MI_sav, "activate", G_CALLBACK (ui_flt_manage_header_action_sav), (gpointer)data);

	g_signal_connect (data->MI_new, "activate", G_CALLBACK (ui_flt_manage_header_action_new), (gpointer)data);
	g_signal_connect (data->MI_ren, "activate", G_CALLBACK (ui_flt_manage_header_action_ren), (gpointer)data);
	g_signal_connect (data->MI_del, "activate", G_CALLBACK (ui_flt_manage_header_action_del), (gpointer)data);	
*/
	//TODO: combobox
//	g_signal_connect_after(data->combobox, "changed", G_CALLBACK (ui_flt_manage_header_change), (gpointer)data);	

	return prtbox;
}

