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

#include "hub-account.h"
#include "dsp-mainwindow.h"
#include "list-account.h"


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


static void da_accgrp_free(PnlAccGrp *item)
{
	DB( g_print("da_accgrp_free\n") );
	DB( g_print(" free '%s'\n", item->name) );

	if(item->name)
		g_free(item->name);
	if(item->acclist)
		g_ptr_array_free (item->acclist, TRUE);
	g_free(item);
}


static PnlAccGrp *da_accgrp_malloc(void)
{
	DB( g_print("da_accgrp_malloc\n") );
	return g_malloc0(sizeof(PnlAccGrp));
}
                      

static void da_accgrp_destroy(GHashTable *h_group)
{
GHashTableIter grp_iter;
gpointer key, value;

	DB( g_print("\n[hub-account] groups free\n") );

	if(h_group == NULL)
		return;
	
	g_hash_table_iter_init (&grp_iter, h_group);
	while (g_hash_table_iter_next (&grp_iter, &key, &value))
	{
	PnlAccGrp *group = value;
		da_accgrp_free(group);
	}

	g_hash_table_destroy (h_group);  
}


static GHashTable *da_accgrp_new(void)
{
	DB( g_print("\n[hub-account] groups new\n") );
	return g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static GHashTable *ui_hub_account_groups_get(GtkTreeView *treeview, gint groupby, gboolean showall)
{
GHashTable *h_group;
GList *lacc, *elt;
gchar *groupname;
gint nballoc;
	
	DB( g_print("\n[hub-account] groups get\n") );

	nballoc = da_acc_length ();
	DB( g_print(" %d accounts\n", nballoc) );

	h_group = da_accgrp_new();

	lacc = g_hash_table_get_values(GLOBALS->h_acc);
	elt = g_list_first(lacc);
	while (elt != NULL)
	{
	Account *acc = elt->data;
	PnlAccGrp *group;
	
		//#1674045 ony rely on nosummary
		//if( showall || !(acc->flags & (AF_CLOSED|AF_NOSUMMARY)) )
		if( showall || !(acc->flags & AF_NOSUMMARY) )
		{
			switch( groupby )
			{
				case DSPACC_GROUP_BY_BANK:
				{
					groupname = _("(no institution)");
					if( (acc->bankname != NULL) && strlen(acc->bankname) > 0 ) 
						groupname = acc->bankname;
				}
				break;

				case DSPACC_GROUP_BY_GROUP:
				{
				Group *grp = da_grp_get(acc->kgrp);

					groupname = _("(no group)");
					if( grp != NULL && grp->key > 0 )
						groupname = grp->name;
				}
				break;

				default:
					//pre 5.1.3 historical by type display
					groupname = hbtk_get_label(CYA_ACC_TYPE, acc->type);
				break;
			}

			//#1820853 groupname could be NULL
			if( groupname != NULL )
			{
				if( g_hash_table_contains(h_group, groupname) == FALSE )
				{
					group = da_accgrp_malloc();
					group->name = g_strdup(groupname);
					group->acclist = g_ptr_array_sized_new(nballoc);
					g_hash_table_insert(h_group, g_strdup(groupname), group );
				}

				group = g_hash_table_lookup(h_group, groupname);
				if( group != NULL )
				{
					g_ptr_array_add(group->acclist, (gpointer)acc);
				}
			}
		}
		elt = g_list_next(elt);
	}

	g_list_free(lacc);
	
	return h_group;
}


static void ui_hub_account_groups_compute(GHashTable *h_accgrp, PnlAccGrp *totaccgrp)
{
GHashTableIter grp_iter;
gpointer key, value;
PnlAccGrp *gt;
guint j;

	DB( g_print("\n[hub-account] groups compute\n") );


	if( !h_accgrp || !totaccgrp)
		return;
	
	gt = totaccgrp;
	gt->bal_recon = 0;
	gt->bal_clear = 0;
	gt->bal_today = 0;
	gt->bal_future = 0;

	g_hash_table_iter_init (&grp_iter, h_accgrp);
	while (g_hash_table_iter_next (&grp_iter, &key, &value))
	{
	PnlAccGrp *g = value;

		if(!g)
			continue;

		DB( g_print(" g '%s'\n", g->name) );

		g->showtotal = TRUE;
		g->bal_recon = 0;
		g->bal_clear = 0;
		g->bal_today = 0;
		g->bal_future = 0;
		for(j=0;j<g->acclist->len;j++)
		{
		Account *acc = g_ptr_array_index(g->acclist, j);

			if(acc)
			{
				//#1896441 outflow summary
				if( (acc->flags & AF_OUTFLOWSUM) == FALSE )
				{
					g->bal_recon += hb_amount_base(acc->bal_recon, acc->kcur);
					g->bal_clear += hb_amount_base(acc->bal_clear, acc->kcur);
					g->bal_today += hb_amount_base(acc->bal_today, acc->kcur);
					g->bal_future += hb_amount_base(acc->bal_future, acc->kcur);

					//#1950234 show subtotal when single with foreign currency
					if( (g->acclist->len == 1) && (acc->kcur == GLOBALS->kcur) )
					{
						g->showtotal = FALSE;
					}
				}
				else
				{
					DB( g_print("  '%s' is outflow\n", acc->name) );
				}
			}
		}
		
		DB( g_print("  + total :: %.2f %.2f %.2f %.2f - showsub:%d\n", g->bal_recon, g->bal_clear, g->bal_today, g->bal_future, g->showtotal) );

		//sum for grand total		
		gt->bal_recon += g->bal_recon;
		gt->bal_clear += g->bal_clear;
		gt->bal_today += g->bal_today;
		gt->bal_future += g->bal_future;
	}
}


void ui_hub_account_populate(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter  iter1, child_iter;
Account *acc;
guint j, nbtype;
GHashTable *h_group;
GHashTableIter grp_iter;
gpointer key, value;

	DB( g_print("\n[hub-account] populate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	// clear previous
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc));
	gtk_tree_store_clear (GTK_TREE_STORE(model));
	if(data->totaccgrp != NULL)
		da_accgrp_free(data->totaccgrp);
	if(data->h_accgrp != NULL)
		da_accgrp_destroy(data->h_accgrp);

	h_group = ui_hub_account_groups_get(GTK_TREE_VIEW(data->LV_acc), PREFS->pnl_acc_show_by, data->showall);
	data->h_accgrp = h_group;
	data->totaccgrp = da_accgrp_malloc();

	ui_hub_account_groups_compute(data->h_accgrp, data->totaccgrp);
	
	DB( g_print("\n\n populate listview, %d group(s)\n", g_hash_table_size(h_group)) );

	nbtype = 0;

	g_hash_table_iter_init (&grp_iter, h_group);
	while (g_hash_table_iter_next (&grp_iter, &key, &value))
	{
	PnlAccGrp *group = value;
	gint position;

		if(group != NULL)
		{
			nbtype++;
			//1: Header: Bank, Cash, ...
			DB( g_print(" g '%s'\n", (gchar *)key) );

			//#1663399 keep group type position like in dropdown
			position = 0;
			if( PREFS->pnl_acc_show_by == DSPACC_GROUP_BY_TYPE )
			{
			gint t = 0;	

				while(CYA_ACC_TYPE[t].name != NULL && t < 32)
				{
					if( !strcmp(CYA_ACC_TYPE[t].name, key) )
						break;
					t++;
				}

				position = t;
			}

			gtk_tree_store_append (GTK_TREE_STORE(model), &iter1, NULL);
			gtk_tree_store_set (GTK_TREE_STORE(model), &iter1,
					  LST_DSPACC_POS, position,
					  LST_DSPACC_DATATYPE, DSPACC_TYPE_HEADER,
			          LST_DSPACC_DATAS, group,
					  -1);

			//2: Accounts for real
			for(j=0;j<group->acclist->len;j++)
			{
				acc = g_ptr_array_index(group->acclist, j);

				DB( g_print("  + '%s' :: %.2f %.2f %.2f %.2f\n", acc->name, acc->bal_recon, acc->bal_clear, acc->bal_today, acc->bal_future) );

				gtk_tree_store_append (GTK_TREE_STORE(model), &child_iter, &iter1);
				gtk_tree_store_set (GTK_TREE_STORE(model), &child_iter,
						LST_DSPACC_DATATYPE, DSPACC_TYPE_NORMAL,
						LST_DSPACC_DATAS, acc,
					  -1);
			}

			// insert group total line
			//if(group->acclist->len > 1)
			//#1950234 show subtotal when single with foreign currency
			if( group->showtotal == TRUE )
			{
				gtk_tree_store_append (GTK_TREE_STORE(model), &child_iter, &iter1);
				gtk_tree_store_set (GTK_TREE_STORE(model), &child_iter,
						LST_DSPACC_DATATYPE, DSPACC_TYPE_SUBTOTAL,
				        LST_DSPACC_DATAS, group,
						  -1);
			}
		}
	}

	// Grand total
	if( nbtype > 1 )
	{
		gtk_tree_store_append (GTK_TREE_STORE(model), &iter1, NULL);
		gtk_tree_store_set (GTK_TREE_STORE(model), &iter1,
					LST_DSPACC_DATATYPE, DSPACC_TYPE_TOTAL,
		            LST_DSPACC_DATAS, data->totaccgrp,
				  -1);
	}

	ui_hub_account_groups_compute(data->h_accgrp, data->totaccgrp);
	
	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_acc));
	
}


//this func should only recompute balance of acc groups
void ui_hub_account_compute(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;

	DB( g_print("\n[hub-account] compute\n") );
	
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ui_hub_account_groups_compute(data->h_accgrp, data->totaccgrp);
	gtk_widget_queue_draw (data->LV_acc);
}


static void ui_hub_account_browse(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( account_has_website(data->acc) )
	{
		homebank_util_url_show(data->acc->website);
	}
}


static void ui_hub_account_expand_all(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_acc));
}


static void ui_hub_account_collapse_all(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data->LV_acc));
}



/* Callback function for the undo action */
/*static void
activate_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));
}*/


static void
ui_hub_account_clipboard (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hbfile_data *data = user_data;
GtkClipboard *clipboard;
GString *node;

	//g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));

	node = lst_accview_to_string(GTK_TREE_VIEW(data->LV_acc), TRUE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void
ui_hub_account_print (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hbfile_data *data = user_data;
GString *node;

	//g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));

	node = lst_accview_to_string(GTK_TREE_VIEW(data->LV_acc), HB_STRING_PRINT);
	hb_print_listview(GTK_WINDOW(data->window), node->str, NULL, _("Your accounts"), NULL, FALSE);

	g_string_free(node, TRUE);
}


static void
ui_hub_account_activate_toggle (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hbfile_data *data = user_data;
  GVariant *old_state, *new_state;

  old_state = g_action_get_state (G_ACTION (action));
  new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));

  DB( g_print ("Toggle action %s activated, state changes from %d to %d\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_boolean (old_state),
           g_variant_get_boolean (new_state)) );

	data->showall = g_variant_get_boolean (new_state);
	ui_hub_account_populate(GLOBALS->mainwindow, NULL);

  g_simple_action_set_state (action, new_state);
  g_variant_unref (old_state);
}

static void
ui_hub_account_activate_radio (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
//struct hbfile_data *data = user_data;
GVariant *old_state, *new_state;

  old_state = g_action_get_state (G_ACTION (action));
  new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));

  DB( g_print ("Radio action %s activated, state changes from %s to %s\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_string (old_state, NULL),
           g_variant_get_string (new_state, NULL)) );

	PREFS->pnl_acc_show_by = DSPACC_GROUP_BY_TYPE;
	if( !strcmp("bank", g_variant_get_string(new_state, NULL)) )
		PREFS->pnl_acc_show_by = DSPACC_GROUP_BY_BANK;
	else
		if( !strcmp("group", g_variant_get_string(new_state, NULL)) )
			PREFS->pnl_acc_show_by = DSPACC_GROUP_BY_GROUP;

	g_simple_action_set_state (action, new_state);
	g_variant_unref (old_state);

	ui_hub_account_populate(GLOBALS->mainwindow, NULL);

}


static const GActionEntry actions[] = {
//	name, function(), type, state, 
	{ "groupby"		, ui_hub_account_activate_radio		,  "s", "'type'", NULL, {0,0,0} },
	{ "showall"		, ui_hub_account_activate_toggle	, NULL, "false" , NULL, {0,0,0} },
	{ "clipboard"	, ui_hub_account_clipboard			, NULL, NULL , NULL, {0,0,0} },
	{ "print"		, ui_hub_account_print				, NULL, NULL , NULL, {0,0,0} },
//  { "paste", activate_action, NULL, NULL,      NULL, {0,0,0} },
};


void ui_hub_account_setup(struct hbfile_data *data)
{
GAction *action;
GVariant *new_state;

	if( !G_IS_SIMPLE_ACTION_GROUP(data->action_group_acc) )
		return;

	action = g_action_map_lookup_action (G_ACTION_MAP (data->action_group_acc), "showall");
	if( action )
	{
		new_state = g_variant_new_boolean (data->showall);
		g_simple_action_set_state (G_SIMPLE_ACTION(action), new_state);
	}
	
	action = g_action_map_lookup_action (G_ACTION_MAP (data->action_group_acc), "groupby");
	if( action )
	{
		const gchar *value = "type";
		if( PREFS->pnl_acc_show_by == DSPACC_GROUP_BY_BANK )
			value = "bank";
		else
			if( PREFS->pnl_acc_show_by == DSPACC_GROUP_BY_GROUP )
			value = "group";
		
		new_state = g_variant_new_string (value);
		g_simple_action_set_state (G_SIMPLE_ACTION (action), new_state);
	}

}


void ui_hub_account_dispose(struct hbfile_data *data)
{

	DB( g_print("\n[hub-account] dispose\n") );

	if(data->h_accgrp != NULL)
		da_accgrp_destroy(data->h_accgrp);
	if(data->totaccgrp != NULL)
		da_accgrp_free(data->totaccgrp);
	data->h_accgrp = NULL;
	data->totaccgrp = NULL;
}


GtkWidget *ui_hub_account_create(struct hbfile_data *data)
{
GtkWidget *hub, *label, *widget, *scrollwin, *treeview, *tbar, *bbox, *image;

	DB( g_print("\n[hub-account] create\n") );


	hub = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hb_widget_set_margins(GTK_WIDGET(hub), 0, SPACING_SMALL, SPACING_SMALL, SPACING_SMALL);

	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (hub), scrollwin, TRUE, TRUE, 0);
	treeview = (GtkWidget *)lst_accview_new();
	data->LV_acc = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	//list toolbar
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (hub), tbar, FALSE, FALSE, 0);

	label = make_label_group(_("Your accounts"));
	gtk_box_pack_start (GTK_BOX (tbar), label, FALSE, FALSE, 0);

	//gmenu test (see test folder into gtk)
GMenu *menu, *section;

	menu = g_menu_new ();

	section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("Copy to clipboard"), "actions.clipboard");
	g_menu_append (section, _("Print..."), "actions.print");
	g_object_unref (section);

	section = g_menu_new ();
	g_menu_append_section (menu, _("Group by"), G_MENU_MODEL(section));
	g_menu_append (section, _("type")       , "actions.groupby::type");
	g_menu_append (section, _("group")      , "actions.groupby::group");
	g_menu_append (section, _("institution"), "actions.groupby::bank");
	g_object_unref (section);

	section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("Show all"), "actions.showall");
	g_object_unref (section);

	GSimpleActionGroup *group = g_simple_action_group_new ();
	data->action_group_acc = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS (actions), data);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = gtk_menu_button_new();
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);
		gtk_menu_button_set_direction (GTK_MENU_BUTTON(widget), GTK_ARROW_UP);
		gtk_widget_set_halign (widget, GTK_ALIGN_END);
		image = gtk_image_new_from_icon_name (ICONNAME_EMBLEM_SYSTEM, GTK_ICON_SIZE_MENU);
		g_object_set (widget, "image", image,  NULL);

		gtk_widget_insert_action_group (widget, "actions", G_ACTION_GROUP(group));
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (widget), G_MENU_MODEL (menu));

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);
	
		widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
		data->BT_expandall = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
		data->BT_collapseall = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);
	
		widget = make_image_button(ICONNAME_HB_BUTTON_BROWSER, _("Browse Website"));
		data->BT_browse = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (data->BT_expandall  ), "clicked"      , G_CALLBACK (ui_hub_account_expand_all), NULL);
	g_signal_connect (G_OBJECT (data->BT_collapseall), "clicked"      , G_CALLBACK (ui_hub_account_collapse_all), NULL);
	g_signal_connect (G_OBJECT (data->BT_browse     ), "clicked"      , G_CALLBACK (ui_hub_account_browse), NULL);

	return hub;
}

