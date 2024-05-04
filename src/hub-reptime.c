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

#include "dsp-mainwindow.h"

#include "hub-reptime.h"
#include "gtk-chart.h"
#include "list-report.h"


/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else


#define DB(x);
#endif

#define SHOW_TREE_VIEW	0


/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


static gint list_reptime_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
//gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
DataRow *dr1, *dr2;
	
	gtk_tree_model_get(model, a,
		LST_REPTIME_ROW, &dr1,
		-1);
	gtk_tree_model_get(model, b,
		LST_REPTIME_ROW, &dr2,
		-1);

	//total always at bottom
	if( dr1->pos == LST_REPORT_POS_TOTAL )
	{
		retval = -1;
	}
	else
	{
		if( dr2->pos == LST_REPORT_POS_TOTAL )
		{
			retval = 1;
		}
		else
		{
			retval = dr2->pos - dr1->pos;
		}
	}
	
	//DB( g_print(" sort %d=%d or %.2f=%.2f :: %d\n", pos1,pos2, val1, val2, ret) );

	return retval;
}


static GtkWidget *create_list_reptime(void)
{
GtkTreeStore *store;
GtkWidget *view;

	/* create list store */
	store = lst_rep_time_new();

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	//5.7
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPTIME_POS    , list_reptime_compare_func, GINT_TO_POINTER(LST_REPTIME_POS), NULL);
	//gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPTIME_AMOUNT    , list_reptime_compare_func, GINT_TO_POINTER(LST_REPTIME_AMOUNT), NULL);
	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), list_reptime_compare_func, NULL, NULL);

	return(view);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


void ui_hub_reptime_update(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gchar *title = "";
gboolean showmono = TRUE;

	DB( g_print("\n[hub-time] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	switch( PREFS->hub_tim_view )
	{
		case HUB_TIM_VIEW_SPENDING: 
			title = _("Spending by Month");
			break;
		case HUB_TIM_VIEW_ACCBALANCE:
			title = _("Account Balance");
			showmono = FALSE;
			break;
		case HUB_TIM_VIEW_ALLBALANCE:
			title = _("Global Balance");
			break;
	}

	//time chart
	gtk_chart_set_showmono(GTK_CHART(data->RE_hubtim_chart), showmono);
	gtk_chart_show_legend(GTK_CHART(data->RE_hubtim_chart), FALSE, FALSE);
	gtk_chart_set_color_scheme(GTK_CHART(data->RE_hubtim_chart), PREFS->report_color_scheme);
	gtk_chart_set_smallfont (GTK_CHART(data->RE_hubtim_chart), PREFS->rep_smallfont);
	gtk_chart_set_currency(GTK_CHART(data->RE_hubtim_chart), GLOBALS->kcur);
	gtk_chart_show_xval(GTK_CHART(data->RE_hubtim_chart), TRUE);
	//5.7 trendrow is unused, we pass the treeview to get the column labels
	gtk_chart_set_datas_time(GTK_CHART(data->RE_hubtim_chart), GTK_TREE_VIEW(data->LV_hubtim), data->hubtim_dt, data->hubtim_rows , data->hubtim_cols, title, NULL);

}


void ui_hub_reptime_clear(GtkWidget *widget, gpointer user_data)
{

}


void ui_hub_reptime_populate(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
DataTable *dt;
gint range;
gint tmpview, tmpsrc, tmpintvl;
guint i, n_inserted;

	DB( g_print("\n[hub-time] populate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpview = PREFS->hub_tim_view;
	tmpsrc = REPORT_SRC_ACCOUNT;
	//tmptype = REPORT_TYPE_EXPENSE;
	tmpintvl = REPORT_INTVL_MONTH;

	switch( tmpview )
	{
		case HUB_TIM_VIEW_SPENDING:
			tmpsrc = REPORT_SRC_NONE;
			break;
		case HUB_TIM_VIEW_ACCBALANCE:
			tmpsrc = REPORT_SRC_ACCOUNT;
			break;
		case HUB_TIM_VIEW_ALLBALANCE:
			tmpsrc = REPORT_SRC_NONE;
			break;
	}

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_hubtim_range));
	PREFS->hub_tim_range = range;

	DB( g_print(" - range=%d\n", range) );
	
	filter_preset_daterange_set(data->hubtim_filter, range, 0);
	filter_preset_type_set(data->hubtim_filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);


	gtk_chart_set_datas_none(GTK_CHART(data->RE_hubtim_chart));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_hubtim));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	if(data->hubtim_dt)
	{
		da_datatable_free (data->hubtim_dt);
		data->hubtim_dt = NULL;
	}

	GQueue *txn_queue = hbfile_transaction_get_partial(data->hubtim_filter->mindate, data->hubtim_filter->maxdate);
	if( tmpview == HUB_TIM_VIEW_SPENDING )
	{
		filter_preset_type_set(data->hubtim_filter, FLT_TYPE_EXPENSE, FLT_INCLUDE);
		dt = report_compute(tmpsrc, tmpintvl, data->hubtim_filter, txn_queue, PREFS->rep_forcast, FALSE);
	}
	else
	{
		filter_preset_type_set(data->hubtim_filter, FLT_TYPE_ALL, FLT_OFF);
		dt = report_compute(tmpsrc, tmpintvl, data->hubtim_filter, txn_queue, PREFS->rep_forcast, TRUE);
		//dt = report_compute_balance(tmpsrc, tmpintvl, data->hubtim_filter);
	}
	g_queue_free (txn_queue);


	if(dt)
	{
		data->hubtim_dt = dt;

		DB( g_print("\n -- populate time listview : %d rows --\n", dt->nbrows) );

		// clear and detach our model
		g_object_ref(model); // Make sure the model stays with us after the tree view unrefs it
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_hubtim), NULL); // Detach model from view

		lst_rep_time_renewcol(GTK_TREE_VIEW(data->LV_hubtim), dt, dt->nbcols, data->hubtim_filter->mindate, tmpintvl, TRUE);

		n_inserted = 0;
		for(i=0; i<dt->nbrows; i++)
		{
		DataRow *dr;
		guint32 reskey;

			reskey = dt->keylist[i];
			dr = report_data_get_row(dt, reskey);

			DB( g_printf(" time eval item %4d:'%s' %.2f\n", i, dr->label, dr->rowexp + dr->rowinc ) );

			//#2024940 test on exp/inc individually
			if( hb_amount_equal(dr->rowexp, 0.0) && hb_amount_equal(dr->rowinc, 0.0))
			{
				DB( g_printf("  hide because no data\n") );
				continue;
			}

			//if( tmpsrc == REPORT_SRC_ACCOUNT && (i == 0) )
			//	continue;

			n_inserted++;

			DB( g_printf(" --> insert\n") );

			gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
				LST_REPTIME_POS, n_inserted,
				LST_REPTIME_KEY, reskey,
				LST_REPTIME_LABEL, dr->label,
				LST_REPTIME_ROW, dr,
				-1);
		}

		data->hubtim_rows = n_inserted;
		data->hubtim_cols = dt->nbcols;

		//add fake total row as chart will retrieve 1 !!
		if( n_inserted > 1 )
		{
		DataRow *dr = dt->totrow;

			//TODO: crappy here		
			dr->pos = LST_REPORT_POS_TOTAL;

			gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
				LST_REPTIME_POS, LST_REPORT_POS_TOTAL,
				LST_REPTIME_KEY, -1,
				LST_REPTIME_LABEL, _("Total"),
				LST_REPTIME_ROW, dr,
				-1);
		}
		// Re-attach model to view
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_hubtim), model);
		g_object_unref(model);

		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), LST_REPTIME_POS, GTK_SORT_DESCENDING);
	}

	//dont forget to dispose dt !!
	ui_hub_reptime_update(widget, data);

}


static void
ui_hub_reptime_activate_radio (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
GVariant *old_state, *new_state;

	old_state = g_action_get_state (G_ACTION (action));
	new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));

	DB( g_print ("Radio action %s activated, state changes from %s to %s\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_string (old_state, NULL),
           g_variant_get_string (new_state, NULL)) );

	PREFS->hub_tim_view = HUB_TIM_VIEW_NONE;

	if( !strcmp("expmon", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tim_view = HUB_TIM_VIEW_SPENDING;
	else	
	if( !strcmp("accbal", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tim_view = HUB_TIM_VIEW_ACCBALANCE;
	else
	if( !strcmp("allbal", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tim_view = HUB_TIM_VIEW_ALLBALANCE;

	g_simple_action_set_state (action, new_state);
	g_variant_unref (old_state);

	ui_hub_reptime_populate(GLOBALS->mainwindow, NULL);
}


static const GActionEntry actions[] = {
//	name, function(), type, state, 
	{ "view", ui_hub_reptime_activate_radio ,  "s", "'cat'", NULL, {0,0,0} }
};


void ui_hub_reptime_setup(struct hbfile_data *data)
{
GAction *action;
GVariant *new_state;

	data->hubtim_filter = da_flt_malloc();
	filter_reset(data->hubtim_filter);

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_hubtim_range), PREFS->hub_tim_range);

	if( !G_IS_SIMPLE_ACTION_GROUP(data->hubtim_action_group) )
		return;

	action = g_action_map_lookup_action (G_ACTION_MAP (data->hubtim_action_group), "view");
	if( action )
	{
	const gchar *value = "expmon";

		switch( PREFS->hub_tim_view )
		{
			case HUB_TIM_VIEW_SPENDING  : value = "expmon"; break;
			case HUB_TIM_VIEW_ACCBALANCE: value = "accbal"; break;
			case HUB_TIM_VIEW_ALLBALANCE: value = "allbal"; break;
		}
		
		new_state = g_variant_new_string (value);
		g_simple_action_set_state (G_SIMPLE_ACTION (action), new_state);
	}
}


void ui_hub_reptime_dispose(struct hbfile_data *data)
{
	DB( g_print("\n[hub-time] dispose\n") );

	DB( g_print(" set chart to none\n") );
	gtk_chart_set_datas_none(GTK_CHART(data->RE_hubtim_chart));
	da_flt_free(data->hubtim_filter);
	data->hubtim_filter = NULL;
	
	DB( g_print(" free dt %p\n", data->hubtim_dt) );
	if(data->hubtim_dt)
	{
		da_datatable_free (data->hubtim_dt);
		data->hubtim_dt = NULL;
	}

}


GtkWidget *ui_hub_reptime_create(struct hbfile_data *data)
{
GtkWidget *hub, *hbox, *bbox, *tbar;
GtkWidget *label, *widget, *image;

	DB( g_print("\n[hub-time] create\n") );

	// /!\ this widget has to be freed
	widget = (GtkWidget *)create_list_reptime();
	data->LV_hubtim = widget;

	hub = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hb_widget_set_margins(GTK_WIDGET(hub), 0, SPACING_SMALL, SPACING_SMALL, SPACING_SMALL);
	data->GR_hubtim = hub;

#if SHOW_TREE_VIEW == 1
	GtkWidget *scrollwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX(hub), scrollwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), data->LV_hubtim);
	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (data->LV_hubtim), GTK_TREE_VIEW_GRID_LINES_BOTH);
#endif

	/* chart + listview */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (hub), hbox, TRUE, TRUE, 0);

	widget = gtk_chart_new(CHART_TYPE_STACK);
	data->RE_hubtim_chart = widget;
	gtk_chart_set_minor_prefs(GTK_CHART(widget), PREFS->euro_value, PREFS->minor_cur.symbol);
	gtk_chart_set_currency(GTK_CHART(widget), GLOBALS->kcur);
	gtk_chart_show_legend(GTK_CHART(widget), TRUE, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	//list toolbar
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (hub), tbar, FALSE, FALSE, 0);

	label = make_label_group(_("Time chart"));
	data->LB_hubtim = label;
	gtk_box_pack_start (GTK_BOX (tbar), label, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = gtk_menu_button_new();
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);
		gtk_menu_button_set_direction (GTK_MENU_BUTTON(widget), GTK_ARROW_UP);
		gtk_widget_set_halign (widget, GTK_ALIGN_END);
		image = gtk_image_new_from_icon_name (ICONNAME_EMBLEM_SYSTEM, GTK_ICON_SIZE_MENU);
		g_object_set (widget, "image", image,  NULL);

	//gmenu test (see test folder into gtk)
GMenu *menu, *section;

	menu = g_menu_new ();
	section = g_menu_new ();
	g_menu_append (section, _("Spending/Month")  , "actions.view::expmon");
	g_menu_append (section, _("Account Balance") , "actions.view::accbal");
	g_menu_append (section, _("Global Balance")  , "actions.view::allbal");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	GSimpleActionGroup *group = g_simple_action_group_new ();
	data->hubtim_action_group = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS (actions), data);

	gtk_widget_insert_action_group (widget, "actions", G_ACTION_GROUP(group));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (widget), G_MENU_MODEL (menu));


	data->CY_hubtim_range = make_daterange(NULL, DATE_RANGE_CUSTOM_HIDDEN);
	gtk_box_pack_end (GTK_BOX (tbar), data->CY_hubtim_range, FALSE, FALSE, 0);


	//hbtk_radio_button_connect (GTK_CONTAINER(data->RA_type), "toggled", G_CALLBACK (ui_hub_reptime_populate), NULL);

	g_signal_connect (data->CY_hubtim_range, "changed", G_CALLBACK (ui_hub_reptime_populate), NULL);
	
	return hub;
}

