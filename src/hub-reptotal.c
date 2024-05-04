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

#include "hub-reptotal.h"
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


static gint list_topspending_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint pos1, pos2, retval = 0;
gdouble val1, val2;

	gtk_tree_model_get(model, a,
		LST_TOPSPEND_POS, &pos1,
		LST_TOPSPEND_AMOUNT, &val1,
		-1);
	gtk_tree_model_get(model, b,
		LST_TOPSPEND_POS, &pos2,
		LST_TOPSPEND_AMOUNT, &val2,
		-1);

	//#1933164 should return
	// > 0 if a sorts before b 
	// = 0 if a sorts with b
	// < 0 if a sorts after b

	switch(sortcol)
	{
		case LST_TOPSPEND_POS:
			retval = pos1 - pos2;
			//DB( g_print(" sort %3d = %3d :: %d\n", pos1, pos2, retval) );
			break;
		case LST_TOPSPEND_AMOUNT:
			//retval = (ABS(val1) - ABS(val2)) > 0 ? -1 : 1;
			retval = (val1 - val2) > 0 ? -1 : 1;
			//DB( g_print(" sort %.2f = %.2f :: %d\n", val1, val2, retval) );
			break;
	}

	return retval;
}


static GtkWidget *create_list_topspending(void)
{
GtkTreeStore *store;
GtkWidget *view;

	/* create list store */
	store = lst_report_new();

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	//5.7
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_TOPSPEND_POS    , list_topspending_compare_func, GINT_TO_POINTER(LST_TOPSPEND_POS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_TOPSPEND_AMOUNT    , list_topspending_compare_func, GINT_TO_POINTER(LST_TOPSPEND_AMOUNT), NULL);
	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), list_topspending_compare_func, NULL, NULL);

	return(view);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


void ui_hub_reptotal_update(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
gchar *title = NULL;
gchar strbuffer[G_ASCII_DTOSTR_BUF_SIZE];

	DB( g_print("\n[hub-total] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//TODO: reuse this ?
	hb_strfmon(strbuffer, G_ASCII_DTOSTR_BUF_SIZE-1, data->hubtot_total, GLOBALS->kcur, GLOBALS->minor);	

	switch( PREFS->hub_tot_view )
	{
		case HUB_TOT_VIEW_CATEGORY: 
			title = g_strdup_printf(_("Top %d Spending / Category"), PREFS->rep_maxspenditems);
			break;
		case HUB_TOT_VIEW_PAYEE:
			title = g_strdup_printf(_("Top %d Spending / Payee"), PREFS->rep_maxspenditems);
			break;
		case HUB_TOT_VIEW_ACCOUNT:
			title = g_strdup_printf(_("Top %d Spending / Account"), PREFS->rep_maxspenditems);
			break;
		case HUB_TOT_VIEW_BALANCE: 
			title = g_strdup_printf(_("Account Balance"));
			break;
	}

	gtk_chart_set_color_scheme(GTK_CHART(data->RE_hubtot_chart), PREFS->report_color_scheme);
	gtk_chart_set_smallfont (GTK_CHART(data->RE_hubtot_chart), PREFS->rep_smallfont);
	gtk_chart_set_currency(GTK_CHART(data->RE_hubtot_chart), GLOBALS->kcur);

	//set column1 != column2 will dual display
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_hubtot));
	gtk_chart_set_datas_total(GTK_CHART(data->RE_hubtot_chart), model, LST_TOPSPEND_AMOUNT, LST_TOPSPEND_AMOUNT, title, strbuffer);

	g_free(title);
}


void ui_hub_reptotal_clear(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;

	DB( g_print("\n[hub-total] clear\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_hubtot));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

}


void ui_hub_reptotal_populate(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter  iter, parent, *tmpparent;
DataTable *dt;
gint range;
gint tmpsrc;
guint i, max_items;
gdouble total, other;
gboolean tmpaccbal, valid;

	DB( g_print("\n[hub-total] populate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpsrc = REPORT_SRC_CATEGORY;
	//tmptype = REPORT_TYPE_EXPENSE;
	tmpaccbal = FALSE;

	switch( PREFS->hub_tot_view )
	{
		case HUB_TOT_VIEW_CATEGORY: tmpsrc = REPORT_SRC_CATEGORY; break;
		case HUB_TOT_VIEW_PAYEE   : tmpsrc = REPORT_SRC_PAYEE; break;
		case HUB_TOT_VIEW_ACCOUNT : tmpsrc = REPORT_SRC_ACCOUNT; break;
		case HUB_TOT_VIEW_BALANCE : 
			tmpsrc = REPORT_SRC_ACCOUNT; 
			tmpaccbal = TRUE;
			break;
	}

	//type  = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_type));
	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_hubtot_range));
	PREFS->hub_tot_range = range;

	DB( g_print(" range=%d\n", range) );

	//if(range == FLT_RANGE_MISC_CUSTOM)
	//	return;

	filter_preset_daterange_set(data->hubtot_filter, range, 0);
	//#1989211 option to include xfer by default
	if(PREFS->stat_includexfer == FALSE)
		filter_preset_type_set(data->hubtot_filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);
	else
		filter_preset_type_set(data->hubtot_filter, FLT_TYPE_ALL, FLT_INCLUDE);


	DB( hb_print_date(data->hubtot_filter->mindate, "min:") );
	DB( hb_print_date(data->hubtot_filter->maxdate, "max:") );

	total = 0.0;

	GQueue *txn_queue = hbfile_transaction_get_partial(data->hubtot_filter->mindate, data->hubtot_filter->maxdate);
	if(!tmpaccbal)
	{
		dt = report_compute(tmpsrc, REPORT_INTVL_NONE, data->hubtot_filter, txn_queue, FALSE, FALSE);
	}
	else
		dt = report_compute(tmpsrc, REPORT_INTVL_NONE, data->hubtot_filter, txn_queue, FALSE, TRUE);
		//dt = report_compute_balance(tmpsrc, REPORT_INTVL_NONE, data->hubtot_filter, TRUE);
	g_queue_free (txn_queue);

	if(dt)
	{
		//todo: should use clear func
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_hubtot));
		gtk_tree_store_clear (GTK_TREE_STORE(model));

		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_hubtot), NULL); /* Detach model from view */

		DB( g_print("\n -- populate total listview %d rows --\n", dt->nbrows) );

		// insert into the treeview
		for(i=0 ; i<dt->nbrows ; i++)
		{
		DataRow *dr;
		gdouble value;
		guint32 reskey;

			//since 5.7 we use the dt-keylst here to insert cat before subcat
			reskey = dt->keylist[i];
			dr = report_data_get_row(dt, reskey);

			//if( tmptype == REPORT_TYPE_EXPENSE && !dr->expense[0] ) continue;
			//if( tmptype == REPORT_TYPE_INCOME && !dr->income[1] ) continue;
			if( !dr->rowexp && !dr->rowinc )
			{
				DB( g_print(" - %d : %s k='%d' %.2f %.2f, skipped no data\n", i, dr->label, reskey, dr->rowexp, dr->rowinc) );
				continue;
			}

			//if( tmpsrc == REPORT_SRC_ACCOUNT && (i == 0) )
			//	continue;

			//#2031245
			if( tmpaccbal == TRUE )
				value = dr->rowexp + dr->rowinc;
			else
				value = dr->rowexp; 
			
			// manage the toplevel for category
			tmpparent = NULL;
			if( tmpsrc == REPORT_SRC_CATEGORY )
			{
			Category *tmpcat = da_cat_get(reskey);
				if( tmpcat != NULL)
				{
					//if( list_topspending_get_top_level (GTK_TREE_MODEL(model), tmpcat->parent, &parent) == TRUE )
					if( hbtk_tree_store_get_top_level(GTK_TREE_MODEL(model), LST_TOPSPEND_KEY, tmpcat->parent, &parent) )
					{
						tmpparent = &parent;
					}
					
					//compute total
					if( tmpcat->parent == 0 )
					{
						if(value < 0.0 )
							total += value;
					}
				}
			}
			else
			{
				if(value < 0.0 || tmpaccbal == TRUE )
					total += value;
			}
	
			if( value < 0.0 || tmpaccbal == TRUE )
			{
				// append test
				gtk_tree_store_append (GTK_TREE_STORE(model), &iter, tmpparent);
				gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
					  LST_TOPSPEND_POS, i,
					  LST_TOPSPEND_KEY, reskey,
					  LST_TOPSPEND_NAME, dr->label,
					  LST_TOPSPEND_AMOUNT, value,
					  //LST_TOPSPEND_RATE, (gint)(((ABS(value)*100)/ABS(total)) + 0.5),
					  -1);

				DB( g_print(" - %d : %s k='%d' v='%f' %f %f, added\n", i, dr->label, reskey, value, dr->rowexp, dr->rowinc) );
			}
			else
			{
				DB( g_print(" - %d : %s k='%d' v='%f' %f %f, skipped\n", i, dr->label, reskey, value, dr->rowexp, dr->rowinc) );
			}

		}

		//sort by expense descending
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), LST_TOPSPEND_AMOUNT, GTK_SORT_DESCENDING);

		//5.7 order & limitation moved here
		if( tmpaccbal == FALSE )
		{
			other = 0.0;
			i = 0;
			max_items = (guint)PREFS->rep_maxspenditems;

			{
			GtkTreeIter remiter;
			gdouble othamt;
			gboolean do_remove;

				DB( g_print(" aggregate items\n") );
				valid = gtk_tree_model_get_iter_first(model, &iter);
				while( valid )
				{
					gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
						  LST_TOPSPEND_POS, i++,
						  -1);

					do_remove = (i > max_items ) ? TRUE : FALSE;

					remiter = iter;
					valid = gtk_tree_model_iter_next(model, &iter);

					if( do_remove )
					{
						gtk_tree_model_get (GTK_TREE_MODEL(model), &remiter,
							LST_TOPSPEND_AMOUNT, &othamt,
							-1);
							
						if(othamt < 0.0)
							other += othamt;

						DB( g_print(" other += %.2f\n", othamt) );
						hbtk_tree_store_remove_iter_with_child(model, &remiter);
					}
				}

				// append 'Others'
				if(ABS(other) > 0)
				{
					DB( g_print(" - %d : %s k='%d' v='%f'\n", max_items+1, "Other", 0, other) );

					gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);
					gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
						  LST_TOPSPEND_POS, max_items+1,
						  LST_TOPSPEND_KEY, 0,
						  LST_TOPSPEND_NAME, _("Other"),
						  LST_TOPSPEND_AMOUNT, other,
						  //LST_TOPSPEND_RATE, (gint)(((ABS(other)*100)/ABS(total)) + 0.5),
						  -1);
				}

			}

			gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), LST_TOPSPEND_POS, GTK_SORT_ASCENDING);
		}

		/* Re-attach model to view */
  		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_hubtot), model);
		g_object_unref(model);

		// update chart and widgets
		{
		gchar *daterange;

			data->hubtot_total = total;
			ui_hub_reptotal_update(widget, data);
			
			daterange = filter_daterange_text_get(data->hubtot_filter);
			gtk_widget_set_tooltip_markup(GTK_WIDGET(data->CY_hubtot_range), daterange);
			g_free(daterange);
		}

		//TODO: later needs to keep this until dispose LV
		da_datatable_free (dt);

	}
	

}


static void
ui_hub_reptotal_activate_radio (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
GVariant *old_state, *new_state;

	old_state = g_action_get_state (G_ACTION (action));
	new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));

	DB( g_print ("Radio action %s activated, state changes from %s to %s\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_string (old_state, NULL),
           g_variant_get_string (new_state, NULL)) );

	PREFS->hub_tot_view = HUB_TOT_VIEW_NONE;

	if( !strcmp("cat", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tot_view = HUB_TOT_VIEW_CATEGORY;
	else	
	if( !strcmp("pay", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tot_view = HUB_TOT_VIEW_PAYEE;
	else
	if( !strcmp("acc", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tot_view = HUB_TOT_VIEW_ACCOUNT;
	else
	if( !strcmp("bal", g_variant_get_string(new_state, NULL)) )
		PREFS->hub_tot_view = HUB_TOT_VIEW_BALANCE;

	g_simple_action_set_state (action, new_state);
	g_variant_unref (old_state);

	ui_hub_reptotal_populate(GLOBALS->mainwindow, NULL);
}


static const GActionEntry actions[] = {
//	name, function(), type, state, 
	{ "view", ui_hub_reptotal_activate_radio ,  "s", "'cat'", NULL, {0,0,0} }
};


void ui_hub_reptotal_setup(struct hbfile_data *data)
{
GAction *action;
GVariant *new_state;

	data->hubtot_filter = da_flt_malloc();
	filter_reset(data->hubtot_filter);
	
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_hubtot_range), PREFS->hub_tot_range);

	//#1989211 option to include xfer by default
	if(PREFS->stat_includexfer == FALSE)
		filter_preset_type_set(data->hubtot_filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);

	if( !G_IS_SIMPLE_ACTION_GROUP(data->hubtot_action_group) )
		return;

	action = g_action_map_lookup_action (G_ACTION_MAP (data->hubtot_action_group), "view");
	if( action )
	{
	const gchar *value = "cat";

		switch( PREFS->hub_tot_view )
		{
			case HUB_TOT_VIEW_CATEGORY: value = "cat"; break;
			case HUB_TOT_VIEW_PAYEE:    value = "pay"; break;
			case HUB_TOT_VIEW_ACCOUNT:  value = "acc"; break;
			case HUB_TOT_VIEW_BALANCE:  value = "bal"; break;
		}
		
		new_state = g_variant_new_string (value);
		g_simple_action_set_state (G_SIMPLE_ACTION (action), new_state);
	}
}


void ui_hub_reptotal_dispose(struct hbfile_data *data)
{
	gtk_chart_set_datas_none(GTK_CHART(data->RE_hubtot_chart));
	da_flt_free(data->hubtot_filter);
	data->hubtot_filter = NULL;
	
}


GtkWidget *ui_hub_reptotal_create(struct hbfile_data *data)
{
GtkWidget *hub, *hbox, *bbox, *tbar;
GtkWidget *label, *widget, *image;

	DB( g_print("\n[hub-total] create\n") );

	// /!\ this widget has to be freed
	widget = (GtkWidget *)create_list_topspending();
	data->LV_hubtot = widget;

	hub = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hb_widget_set_margins(GTK_WIDGET(hub), 0, SPACING_SMALL, SPACING_SMALL, SPACING_SMALL);
	data->GR_hubtot = hub;

#if SHOW_TREE_VIEW == 1
	GtkWidget *scrollwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX(hub), scrollwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), data->LV_hubtot);
	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (data->LV_hubtot), GTK_TREE_VIEW_GRID_LINES_BOTH);
#endif


	/* chart + listview */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (hub), hbox, TRUE, TRUE, 0);

	widget = gtk_chart_new(CHART_TYPE_PIE);
	data->RE_hubtot_chart = widget;
	gtk_chart_set_minor_prefs(GTK_CHART(widget), PREFS->euro_value, PREFS->minor_cur.symbol);
	gtk_chart_set_currency(GTK_CHART(widget), GLOBALS->kcur);
	gtk_chart_show_legend(GTK_CHART(widget), TRUE, TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	//list toolbar
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (hub), tbar, FALSE, FALSE, 0);

	label = make_label_group(_("Total chart"));
	data->LB_hubtot = label;
	gtk_box_pack_start (GTK_BOX (tbar), label, FALSE, FALSE, 0);

	/* total + date range */
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = gtk_menu_button_new();
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		gtk_menu_button_set_direction (GTK_MENU_BUTTON(widget), GTK_ARROW_UP);
		gtk_widget_set_halign (widget, GTK_ALIGN_END);
		image = gtk_image_new_from_icon_name (ICONNAME_EMBLEM_SYSTEM, GTK_ICON_SIZE_MENU);
		g_object_set (widget, "image", image,  NULL);

	GSimpleActionGroup *group = g_simple_action_group_new ();
	data->hubtot_action_group = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS (actions), data);
	gtk_widget_insert_action_group (widget, "actions", G_ACTION_GROUP(group));

	//gmenu test (see test folder into gtk)
GMenu *menu, *section;

	menu = g_menu_new ();
	section = g_menu_new ();
	g_menu_append (section, _("Category") , "actions.view::cat");
	g_menu_append (section, _("Payee")    , "actions.view::pay");
	g_menu_append (section, _("Account")  , "actions.view::acc");
	g_menu_append (section, _("Balance")  , "actions.view::bal");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (widget), G_MENU_MODEL (menu));


	data->CY_hubtot_range = make_daterange(NULL, DATE_RANGE_CUSTOM_HIDDEN);
	gtk_box_pack_end (GTK_BOX (tbar), data->CY_hubtot_range, FALSE, FALSE, 0);

	//hbtk_radio_button_connect (GTK_CONTAINER(data->RA_type), "toggled", G_CALLBACK (ui_hub_reptotal_populate), NULL);

	g_signal_connect (data->CY_hubtot_range, "changed", G_CALLBACK (ui_hub_reptotal_populate), NULL);
	
	return hub;
}

