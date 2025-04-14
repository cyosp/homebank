/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2025 Maxime DOYEN
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

#include "rep-vehicle.h"

#include "list-operation.h"
#include "gtk-chart.h"
#include "gtk-dateentry.h"

#include "dsp-mainwindow.h"
#include "ui-category.h"

#include "ui-dialogs.h"
#include "ui-widgets.h"

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


/* prototypes */
static void repvehicle_export_csv(GtkWidget *widget, gpointer user_data);
static void repvehicle_compute(GtkWidget *widget, gpointer user_data);
static void repvehicle_update(GtkWidget *widget, gpointer user_data);
static void repvehicle_setup_categories(struct repvehicle_data *data, GArray *array);

static GtkWidget *list_vehicle_create(void);


static void repvehicle_date_change(GtkWidget *widget, gpointer user_data)
{
struct repvehicle_data *data;

	DB( g_print("\n[vehiclecost] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	//5.8 check for error
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPVEHICLE_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPVEHICLE_RANGE]);


	repvehicle_compute(widget, NULL);

}


static void repvehicle_action_refresh(GtkWidget *toolbutton, gpointer user_data)
{
struct repvehicle_data *data = user_data;

	repvehicle_compute(data->window, NULL);
}

static void repvehicle_action_export(GtkWidget *toolbutton, gpointer user_data)
{
struct repvehicle_data *data = user_data;

	repvehicle_export_csv(data->window, NULL);
}



static void repvehicle_range_change(GtkWidget *widget, gpointer user_data)
{
struct repvehicle_data *data;
gint range;

	DB( g_print("\n[vehiclecost] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));

	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);

		//#2046032 set min/max date for both widget
		//5.8 check for error
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);


		g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPVEHICLE_MINDATE]);
		g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPVEHICLE_MAXDATE]);

		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);
		
		g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPVEHICLE_MINDATE]);
		g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPVEHICLE_MAXDATE]);

		repvehicle_compute(widget, NULL);
	}
}

static gint repvehicle_transaction_compare_func(CarCost *a, CarCost *b)
{
gint retval;

	//retval = (gint)(a->ope->date - b->ope->date);
	//if( retval == 0 )
		retval = a->meter - b->meter;

	return retval;
}




static void repvehicle_export_csv(GtkWidget *widget, gpointer user_data)
{
struct repvehicle_data *data;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gchar *filename = NULL;
GIOChannel *io;
gchar *outstr, *name;

	DB( g_print("\n[repvehicle] export csv\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	name = "hb-vehicle.csv";

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{
			// header
			outstr = g_strdup_printf("%s;%s;%s;%s;%s;%s;%s;%s\n", 
			_("Date"),
			_("Meter"),
			_("Fuel"),
			_("Price"),
			_("Amount"),
			_("Dist."),
			PREFS->vehicle_unit_100,
			PREFS->vehicle_unit_distbyvol
			);
			g_io_channel_write_chars(io, outstr, -1, NULL, NULL);


			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
			valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
			while (valid)
			{
			guint32 julian;
			gint meter, dist;
			//#1947931 distbyvol in double not int
			gdouble fuel, price, amount, centkm, distbyvol; 
			gboolean partial;
			gchar datebuf[16];

				gtk_tree_model_get (model, &iter,
					LST_CAR_DATE    , &julian,
					LST_CAR_METER   , &meter,
					LST_CAR_FUEL    , &fuel,
					LST_CAR_PRICE   , &price,
					LST_CAR_AMOUNT  , &amount,
					LST_CAR_DIST    , &dist,
					LST_CAR_100KM   , &centkm,
				    LST_CAR_DISTBYVOL, &distbyvol,
				    LST_CAR_PARTIAL, &partial,
                    -1);
				
				hb_sprint_date(datebuf, julian);

				outstr = g_strdup_printf("%s;%d;%.2f;%.2f;%.2f;%d;%.2f;%.2f;%d\n", 
					datebuf, meter, fuel, price, amount, dist, centkm, distbyvol, partial);
				g_io_channel_write_chars(io, outstr, -1, NULL, NULL);

				DB( g_print("%s", outstr) );

				g_free(outstr);

				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
			}

			g_io_channel_unref (io);
		}

		g_free( filename );
	}
}


//#1277622
static gboolean
repvehicle_eval_memofield(CarCost *item, gchar *text)
{
gboolean retval = FALSE;
gchar *d, *v1, *v2;
gint len;

	if(item != NULL)
	{
		item->meter   = 0;
		item->fuel    = 0.0;
		item->partial = FALSE;
	}
	if( text != NULL)
	{
		//TODO: optim here
		len = strlen(text);
		d = g_strstr_len(text, len, "d=");
		v1 = g_strstr_len(text, len, "v=");
		v2 = g_strstr_len(text, len, "v~");
		if(d && (v1 || v2))
		{
			retval = TRUE;
			if(item != NULL)
			{
				item->meter	= atol(d+2);
				if(v1)
				{
					item->fuel	= g_strtod(v1+2, NULL);
				}
				else
				{
					item->fuel	= g_strtod(v2+2, NULL);
					item->partial = TRUE;
				}
			}
		}
	}
	return retval;
}


static gboolean
my_g_array_exists(GArray *array, guint32 kcat)
{
gboolean retval = FALSE;
Category *cat;
guint32 *key, i;

	//#2000452 removed binary_search
	for(i=0;i<array->len;i++)
	{
		key = &g_array_index(array, guint32, i);
		cat = da_cat_get(*key);
		if( (cat != NULL) && (cat->key == kcat) )
		{
			retval = TRUE;
			break;
		}
	}

	DB( g_print("  normal search %d ? %d\n", kcat, retval) );

	return retval;
}


static void
my_garray_add(GArray *array, guint32 key)
{
Category *cat;

	DB( g_print("\n[vehiclecost] garray_add\n") );

	cat = da_cat_get(key);

	if(!cat)
		return;

	//#1873660 add parent as well
	if( cat->parent > 0 )
	{
		if( my_g_array_exists(array, cat->parent) == FALSE )
		{
			DB( g_print(" store kcat=%d '%s' (parent)\n", cat->parent, cat->fullname) );
			g_array_append_vals(array, &cat->parent, 1);
		}
	}
	
	//add category
	if( my_g_array_exists(array, cat->key) == FALSE )
	{
		DB( g_print(" store kcat=%d '%s'\n", cat->key, cat->name) );
		g_array_append_vals(array, &cat->key, 1);
	}
}


static void repvehicle_compute(GtkWidget *widget, gpointer user_data)
{
struct repvehicle_data *data;
GArray *ga_cat;
GList *list;

	DB( g_print("\n[vehiclecost] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	// clear the glist
	da_vehiclecost_destroy(data->vehicle_list);
	data->vehicle_list = NULL;

	g_queue_free (data->txn_queue);
	data->txn_queue = hbfile_transaction_get_partial(data->filter->mindate, data->filter->maxdate);

	// pass1 to collect categories
	ga_cat = g_array_new(FALSE, FALSE, sizeof(guint32));

	list = g_queue_peek_head_link(data->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;

		// eval normal transaction
		if(!(ope->flags & OF_SPLIT))
		{
			if( repvehicle_eval_memofield(NULL, ope->memo) == TRUE )
			{
				my_garray_add(ga_cat, ope->kcat);
			}
		}
		else
		{
		guint i, nbsplit = da_splits_length(ope->splits);
		Split *split;

			for(i=0;i<nbsplit;i++)
			{
				split = da_splits_get(ope->splits, i);
				if( repvehicle_eval_memofield(NULL, split->memo) == TRUE )
				{
					my_garray_add(ga_cat, split->kcat);
				}
			}
		}

		list = g_list_next(list);
	}


	//here ga_cat contains cat+subcat of txn where there is vehicle cost data d= v=


	// pass2: collect transactions + fill carcost items
	list = g_queue_peek_head_link(data->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;
	Category *cat;
	CarCost tmp, *item;

		//TODO: rely on a persistent flag // attribution ? 
		// eval normal transaction
		if(!(ope->flags & OF_SPLIT))
		{
			cat = da_cat_get(ope->kcat);
			if( (cat != NULL) )
			{
				if( (my_g_array_exists(ga_cat, cat->key) || my_g_array_exists(ga_cat, cat->parent) ) )
				{
					if( repvehicle_eval_memofield(&tmp, ope->memo) == TRUE )
					{
						item = da_vehiclecost_malloc();
						item->kcat = ope->kcat;
						item->date = ope->date;
						item->memo = ope->memo;
						item->amount = hb_amount_base(ope->amount, ope->kcur);
						item->meter = tmp.meter;
						item->fuel = tmp.fuel;
						item->partial = tmp.partial;
						data->vehicle_list = g_list_prepend(data->vehicle_list, item);
						DB( g_print(" store txn kcat=%d acc=%d %4.2f '%s' \n", ope->kcat, ope->kacc, ope->amount, ope->memo) );
					}
					else
					{
						item = da_vehiclecost_malloc();
						item->kcat = ope->kcat;
						item->date = ope->date;
						item->amount = hb_amount_base(ope->amount, ope->kcur);
						item->meter = 0;
						data->vehicle_list = g_list_prepend(data->vehicle_list, item);
						DB( g_print(" store txn kcat=%d acc=%d %4.2f '%s' (other)\n", ope->kcat, ope->kacc, ope->amount, ope->memo) );
					}
				}
			}
		}
		// eval split transaction
		else
		{
		guint i, nbsplit = da_splits_length(ope->splits);
		Split *split;

			for(i=0;i<nbsplit;i++)
			{
				split = da_splits_get(ope->splits, i);
				cat = da_cat_get(split->kcat);
				if( (cat != NULL) )
				{
					if( (my_g_array_exists(ga_cat, cat->key) || my_g_array_exists(ga_cat, cat->parent) ) )
					{
						if( repvehicle_eval_memofield(&tmp, split->memo) == TRUE )
						{
							item = da_vehiclecost_malloc();
							item->kcat = split->kcat;
							item->date = ope->date;
							item->memo = split->memo;
							item->amount = hb_amount_base(split->amount, ope->kcur);
							item->meter = tmp.meter;
							item->fuel = tmp.fuel;
							item->partial = tmp.partial;
							data->vehicle_list = g_list_prepend(data->vehicle_list, item);
							DB( g_print(" store txn kcat=%d acc=%d %4.2f '%s' (split)\n", split->kcat, ope->kacc, split->amount, split->memo) );
						}
						else
						{
							item = da_vehiclecost_malloc();
							item->kcat = split->kcat;
							item->date = ope->date;
							item->amount = hb_amount_base(split->amount, ope->kcur);
							item->meter = tmp.meter;
							data->vehicle_list = g_list_prepend(data->vehicle_list, item);
							DB( g_print("- store txn kcat=%d acc=%d %4.2f '%s' (other plit)\n", ope->kcat, ope->kacc, ope->amount, ope->memo) );
						}
					}
				}				

			}
		}

		list = g_list_next(list);
	}

	repvehicle_setup_categories(data, ga_cat);
	g_array_free(ga_cat, TRUE);

	// sort by meter #399170
	data->vehicle_list = g_list_sort(data->vehicle_list, (GCompareFunc)repvehicle_transaction_compare_func);

	repvehicle_update(widget, NULL);
}


static void repvehicle_update(GtkWidget *widget, gpointer user_data)
{
struct repvehicle_data *data;
GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
gchar *buf;
guint32 selkey;
gint nb_refuel = 0;
int nb_fullrefuel = 0; 
guint lastmeter = 0;

	DB( g_print("\n[vehiclecost] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	// get the category key
	//selkey = ui_cat_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_cat));
	selkey = ui_cat_entry_popover_get_key(GTK_BOX(data->PO_cat));

	DB( g_print(" selkey=%d\n\n", selkey) );

	// clear and detach our model
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	gtk_list_store_clear (GTK_LIST_STORE(model));
	g_object_ref(model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL);

	data->total_misccost = 0;
	data->total_fuelcost = 0;
	data->total_fuel	 = 0;
	data->total_dist	 = 0;

	gdouble partial_fuel = 0;
	guint	partial_dist = 0;

	if( selkey == 0 )
		goto noselkey;


	list = g_list_first(data->vehicle_list);
	while (list != NULL)
	{
	CarCost *item = list->data;
	Category *itemcat;
	gint dist;
	gdouble centkm;
	gdouble distbyvol;
	gdouble amount;

		itemcat = da_cat_get(item->kcat);
		if(! itemcat )
			continue;

		if( (itemcat->key == selkey || itemcat->parent == selkey) )
		{
			DB( g_print(" add treeview kcat=%d %s\n", item->kcat, item->memo) );

			amount = item->amount;

			if( item->meter == 0 )
			{
				data->total_misccost += amount;
			}
			else
			{
				if(nb_refuel > 0 )
				{
					//previtem = g_list_nth_data(data->vehicle_list, nb_refuel-1);
					//if(previtem != NULL) previtem->dist = item->meter - previtem->meter;
					//DB( g_print(" + previous item dist = %d\n", item->meter - previtem->meter) );
					item->dist = item->meter - lastmeter;

					//DB( g_print(" + last meter = %d\n", lastmeter) );

				}

				lastmeter = item->meter;
				nb_refuel++;

				//DB( g_print("\n eval %02d :: d=%d v=%.2f $%.2f dist=%d\n", nb_refuel, item->meter, item->fuel, amount, item->dist) );
				//DB( g_print(" + %s :: pf=%.2f pd=%d\n", item->partial ? "partial" : "full", partial_fuel, partial_dist) );

				centkm = 0;
				dist = 0;

				//bugfix #159066 partial/full
				if(item->partial == FALSE)
				{
					//#1836380 if we don't have a full already, the computing will be wrong
					if( nb_fullrefuel > 0 )
					{
						// full refuel after partial
						if(partial_fuel && partial_dist)
						{
							partial_fuel += item->fuel;
							partial_dist += item->dist;
							dist = item->dist;
							centkm = partial_dist != 0 ? partial_fuel * 100 / partial_dist : 0;
							//DB( g_print(" + centkm=%.2f %.2f * 100 / %d (full after partial)\n", centkm, partial_fuel, partial_dist) );
						}
						else
						{
							dist = item->dist;
							centkm = item->dist != 0 ? item->fuel * 100 / item->dist : 0;
							//DB( g_print(" + centkm=%.2f :: %.2f * 100 / %d (full after full)\n", centkm, item->fuel, item->dist) );
						}
					}
					partial_fuel = 0;
					partial_dist = 0;
					nb_fullrefuel++;
				}
				// partial refuel
				else
				{
					partial_fuel += item->fuel;
					partial_dist += item->dist;
					dist = item->dist;
					//DB( g_print(" + centkm= not computable\n") );
				}

				distbyvol = 0;
				if(centkm != 0)
					//#2073233 round to 1 digit
					distbyvol = hb_amount_round((1/centkm)*100, 1);

		    	gtk_list_store_append (GTK_LIST_STORE(model), &iter);

				gtk_list_store_set (GTK_LIST_STORE(model), &iter,
					LST_CAR_DATE,	item->date,
					LST_CAR_MEMO,	item->memo,
					LST_CAR_METER,	item->meter,
					LST_CAR_FUEL,	item->fuel,
					LST_CAR_PRICE,	ABS(amount) / item->fuel,
					LST_CAR_AMOUNT,	amount,
					LST_CAR_DIST,	dist,
					LST_CAR_100KM,	centkm,
				    LST_CAR_DISTBYVOL,	distbyvol,
				    LST_CAR_PARTIAL,	item->partial,
					-1);

				//DB( g_print("\n insert d=%d v=%4.2f $%8.2f %d %5.2f\n", item->meter, item->fuel, amount, dist, centkm) );

				if(item->dist)
				{
					data->total_fuelcost += amount;
					data->total_fuel     += item->fuel;
					data->total_dist     += item->dist;
				}


			}
		}
		list = g_list_next(list);
	}


noselkey:
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
	g_object_unref(model);


	gdouble coef = data->total_dist ? 100 / (gdouble)data->total_dist : 0;

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));


	DB( g_print(" coef = 100 / %.2f = %.2f\n", (gdouble)data->total_dist, coef) );

	// row 1 is for 100km
	/*
	gtk_label_set_text(GTK_LABEL(data->LA_total[1][1]), "1:1");	//Consumption
	gtk_label_set_text(GTK_LABEL(data->LA_total[2][1]), "2:1");	//Fuel cost
	gtk_label_set_text(GTK_LABEL(data->LA_total[3][1]), "3:1");	//Other cost
	gtk_label_set_text(GTK_LABEL(data->LA_total[4][1]), "4:1");	//Total cost
	*/

	// 100km fuel
	buf = g_strdup_printf(PREFS->vehicle_unit_vol, data->total_fuel * coef);
	gtk_label_set_text(GTK_LABEL(data->LA_avera[CAR_RES_FUEL]), buf);
	g_free(buf);

	// 100km fuelcost
	//hb_label_set_colvaluecurr(GTK_LABEL(data->LA_avera[CAR_RES_FUELCOST]), data->total_fuelcost * coef, GLOBALS->kcur);
	hb_label_set_colvalue(GTK_LABEL(data->LA_avera[CAR_RES_FUELCOST]), data->total_fuelcost * coef, GLOBALS->kcur, GLOBALS->minor);

	// 100km other cost
	//hb_label_set_colvaluecurr(GTK_LABEL(data->LA_avera[CAR_RES_OTHERCOST]), data->total_misccost * coef, GLOBALS->kcur);
	hb_label_set_colvalue(GTK_LABEL(data->LA_avera[CAR_RES_OTHERCOST]), data->total_misccost * coef, GLOBALS->kcur, GLOBALS->minor);

	// 100km cost
	//hb_label_set_colvaluecurr(GTK_LABEL(data->LA_avera[CAR_RES_TOTALCOST]), (data->total_fuelcost + data->total_misccost) * coef, GLOBALS->kcur);
	hb_label_set_colvalue(GTK_LABEL(data->LA_avera[CAR_RES_TOTALCOST]), (data->total_fuelcost + data->total_misccost) * coef, GLOBALS->kcur, GLOBALS->minor);


	// row 2 is for total
	/*
	gtk_label_set_text(GTK_LABEL(data->LA_total[1][2]), "1:2");	//Consumption
	gtk_label_set_text(GTK_LABEL(data->LA_total[2][2]), "2:2");	//Fuel cost
	gtk_label_set_text(GTK_LABEL(data->LA_total[3][2]), "3:2");	//Other cost
	gtk_label_set_text(GTK_LABEL(data->LA_total[4][2]), "4:2");	//Total
	*/

	// total distance
	buf = g_strdup_printf(PREFS->vehicle_unit_dist0, data->total_dist);
	gtk_label_set_text(GTK_LABEL(data->LA_total[CAR_RES_METER]), buf);
	g_free(buf);

	// total fuel
	buf = g_strdup_printf(PREFS->vehicle_unit_vol, data->total_fuel);
	gtk_label_set_text(GTK_LABEL(data->LA_total[CAR_RES_FUEL]), buf);
	g_free(buf);

	// total fuelcost
	//hb_label_set_colvaluecurr(GTK_LABEL(data->LA_total[CAR_RES_FUELCOST]), data->total_fuelcost, GLOBALS->kcur);
	hb_label_set_colvalue(GTK_LABEL(data->LA_total[CAR_RES_FUELCOST]), data->total_fuelcost, GLOBALS->kcur, GLOBALS->minor);

	// total other cost
	//hb_label_set_colvaluecurr(GTK_LABEL(data->LA_total[CAR_RES_OTHERCOST]), data->total_misccost, GLOBALS->kcur);
	hb_label_set_colvalue(GTK_LABEL(data->LA_total[CAR_RES_OTHERCOST]), data->total_misccost, GLOBALS->kcur, GLOBALS->minor);

	// total cost
	//hb_label_set_colvaluecurr(GTK_LABEL(data->LA_total[CAR_RES_TOTALCOST]), data->total_fuelcost + data->total_misccost, GLOBALS->kcur);
	hb_label_set_colvalue(GTK_LABEL(data->LA_total[CAR_RES_TOTALCOST]), data->total_fuelcost + data->total_misccost, GLOBALS->kcur, GLOBALS->minor);

}


static void repvehicle_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct repvehicle_data *data;

	DB( g_print("\n[vehiclecost] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	repvehicle_update(widget, NULL);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));


	/*
	statistic_update_total(widget,NULL);

	//hbfile_update(data->LV_acc, (gpointer)4);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));
	gtk_chart_show_minor(GTK_CHART(data->RE_bar), minor);
	gtk_chart_show_minor(GTK_CHART(data->RE_pie), minor);
	*/

}


static void repvehicle_setup_categories(struct repvehicle_data *data, GArray *array)
{
Category *cat;
guint32 kcat, *key, i;

	DB( g_print("\n[vehiclecost] setup categories\n") );

	kcat = GLOBALS->vehicle_category;

	//g_signal_handler_block(data->PO_cat, data->handler_id[HID_REPVEHICLE_VEHICLE]);

	if( array != NULL )
	{
		//get previous category
		//kcat = ui_cat_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_cat));
		kcat = ui_cat_entry_popover_get_key(GTK_BOX(data->PO_cat));

		//populate with the array
		//ui_cat_combobox_entry_clear(GTK_COMBO_BOX(data->PO_cat));
		ui_cat_entry_popover_clear(GTK_BOX(data->PO_cat));

		cat = da_cat_get(0);
		if(cat)
		{
			DB( g_print(" add %d '%s'\n", cat->key, cat->fullname) );
			//ui_cat_comboboxentry_add(GTK_COMBO_BOX(data->PO_cat), cat);
			ui_cat_entry_popover_add(GTK_BOX(data->PO_cat), cat);
		}

		for(i=0;i<array->len;i++)
		{
			key = &g_array_index(array, guint32, i);
			cat = da_cat_get(*key);
			if(cat)
			{
				DB( g_print(" add %d %s\n", cat->key, cat->fullname) );
				//ui_cat_comboboxentry_add(GTK_COMBO_BOX(data->PO_cat), cat);
				ui_cat_entry_popover_add(GTK_BOX(data->PO_cat), cat);
			}
		}
	}

	//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), kcat);
	ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), kcat);
	
	//g_signal_handler_unblock(data->PO_cat, data->handler_id[HID_REPVEHICLE_VEHICLE]);
}


static GtkWidget *
repvehicle_toolbar_create(struct repvehicle_data *data)
{
GtkWidget *toolbar, *button;

	toolbar = gtk_toolbar_new();
	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REFRESH, _("Refresh"), _("Refresh results"));
	data->BT_refresh = button;
	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_FILE_EXPORT, _("Export"), _("Export as CSV"));
	data->BT_export = button;

	return toolbar;
}


//reset the filter
static void repvehicle_filter_setup(struct repvehicle_data *data)
{
	DB( g_print("\n[vehiclecost] reset filter\n") );

	filter_reset(data->filter);

	/* 3.4 : make int transfer out of stats */
	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, 0);
	filter_preset_type_set(data->filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);
	
	//g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPVEHICLE_MINDATE]);
	//g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPVEHICLE_MAXDATE]);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);

	//g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPVEHICLE_MINDATE]);
	//g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPVEHICLE_MAXDATE]);

}


static void repvehicle_window_setup(struct repvehicle_data *data)
{
	DB( g_print("\n[vehiclecost] setup\n") );

	DB( g_print(" init data\n") );

	repvehicle_filter_setup(data);

	DB( g_print(" populate\n") );

	repvehicle_setup_categories(data, NULL);

	DB( g_print(" set widgets default\n") );

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_rep);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor),GLOBALS->minor);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);

	DB( g_print(" connect widgets signals\n") );
	

	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (repvehicle_toggle_minor), NULL);

    data->handler_id[HID_REPVEHICLE_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repvehicle_date_change), (gpointer)data);
    data->handler_id[HID_REPVEHICLE_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repvehicle_date_change), (gpointer)data);

	data->handler_id[HID_REPVEHICLE_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repvehicle_range_change), NULL);

	//data->handler_id[HID_REPVEHICLE_VEHICLE] = g_signal_connect (data->PO_cat, "changed", G_CALLBACK (repvehicle_update), NULL);
	data->handler_id[HID_REPVEHICLE_VEHICLE] = g_signal_connect (ui_cat_entry_popover_get_entry(GTK_BOX(data->PO_cat)), "changed", G_CALLBACK (repvehicle_update), NULL);

	g_signal_connect (G_OBJECT (data->BT_refresh), "clicked", G_CALLBACK (repvehicle_action_refresh), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_export) , "clicked", G_CALLBACK (repvehicle_action_export), (gpointer)data);


}


static gboolean repvehicle_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repvehicle_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[vehiclecost] window mapped\n") );

	//setup, init and show window
	repvehicle_window_setup(data);
	repvehicle_compute(data->window, NULL);

	data->mapped_done = TRUE;

	return FALSE;
}


static gboolean
repvehicle_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repvehicle_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[vehiclecost] dispose\n") );

	g_queue_free (data->txn_queue);
	
	da_vehiclecost_destroy(data->vehicle_list);

	da_flt_free(data->filter);
	
	g_free(data);

	//store position and size
	wg = &PREFS->cst_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

	//enable define windows
	GLOBALS->define_off--;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	//unref window to our open window list
	GLOBALS->openwindows = g_slist_remove(GLOBALS->openwindows, widget);

	return FALSE;
}


//allocate our object/memory
static void repvehicle_window_acquire(struct repvehicle_data *data)
{
	DB( g_print("\n[vehiclecost] acquire\n") );

	data->txn_queue = g_queue_new ();
	data->filter = da_flt_malloc();
	data->vehicle_list = NULL;


}


// the window creation
GtkWidget *repvehicle_window_new(void)
{
struct repvehicle_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainbox, *vbox, *scrollwin, *treeview;
GtkWidget *label, *widget, *table;
gint row, col;

	DB( g_print("\n[vehiclecost] new\n") );

	data = g_malloc0(sizeof(struct repvehicle_data));
	if(!data) return NULL;

	repvehicle_window_acquire(data);

	//disable define windows
	GLOBALS->define_off++;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

    /* create window, etc */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->window = window;
	//ref window to our open window list
	GLOBALS->openwindows = g_slist_prepend(GLOBALS->openwindows, window);

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	gtk_window_set_title (GTK_WINDOW (window), _("Vehicle cost report"));

	//window contents
	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	hb_widget_set_margin(GTK_WIDGET(mainbox), SPACING_SMALL);
	gtk_window_set_child(GTK_WINDOW(window), mainbox);

	//control part
	table = gtk_grid_new ();
	gtk_widget_set_hexpand (GTK_WIDGET(table), FALSE);
    gtk_box_prepend (GTK_BOX (mainbox), table);

	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0;
	label = make_label_group(_("Display"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	//label = make_label_widget(_("Vehi_cle:"));
	//#2001566 make label consistent with properties dialog
	label = make_label_widget(_("_Category:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);

	//widget = ui_cat_comboboxentry_new(label);
	widget = ui_cat_entry_popover_new(label);
	data->PO_cat = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);


	//-- filter
	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	row++;
	label = make_label_group(_("Filter"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);
	
	row++;
	//label = make_label_group(_("Date filter"));
	label = make_label_group(_("Date"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 2, 1);

	row++;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->CY_range = make_daterange(label, DATE_RANGE_FLAG_CUSTOM_DISABLE);
	gtk_grid_attach (GTK_GRID (table), data->CY_range, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_From:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->PO_mindate = gtk_date_entry_new(label);
	gtk_grid_attach (GTK_GRID (table), data->PO_mindate, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_To:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->PO_maxdate = gtk_date_entry_new(label);
	gtk_grid_attach (GTK_GRID (table), data->PO_maxdate, 2, row, 1, 1);


	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (vbox, SPACING_SMALL);
    hbtk_box_prepend (GTK_BOX (mainbox), vbox);

	widget = repvehicle_toolbar_create(data);
	data->TB_bar = widget;
	gtk_box_prepend (GTK_BOX (vbox), widget);
	

	// total
	table = gtk_grid_new ();
	gtk_widget_set_hexpand (GTK_WIDGET(table), FALSE);
    gtk_box_prepend (GTK_BOX (vbox), table);

	hb_widget_set_margin(GTK_WIDGET(table), SPACING_SMALL);
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0; col = 1;

	label = make_label_widget(_("Meter:"));
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	col++;
	label = make_label_widget(_("Consumption:"));
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	col++;
	label = make_label_widget(_("Fuel cost:"));
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	col++;
	label = make_label_widget(_("Other cost:"));
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	col++;
	label = make_label_widget(_("Total cost:"));
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	row++;
	col = 0;
	label = make_label_widget(PREFS->vehicle_unit_100);
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	for(col = 1;col<MAX_CAR_RES;col++)
	{
		label = make_label(NULL, 1.0, 0.5);
		gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);
		data->LA_avera[col] = label;
	}

	row++;
	col = 0;
	label = make_label_widget(_("Total"));
	gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);

	for(col = 1;col<MAX_CAR_RES;col++)
	{
		label = make_label(NULL, 1.0, 0.5);
		gtk_grid_attach (GTK_GRID (table), label, col, row, 1, 1);
		data->LA_total[col] = label;
	}
	

	//detail
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	treeview = list_vehicle_create();
	data->LV_report = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
    hbtk_box_prepend (GTK_BOX (vbox), scrollwin);


	// connect dialog signals
    g_signal_connect (window, "delete-event", G_CALLBACK (repvehicle_window_dispose), (gpointer)data);
	g_signal_connect (window, "map-event"   , G_CALLBACK (repvehicle_window_mapped), NULL);

	// setup, init and show window
	wg = &PREFS->cst_wg;
	if( wg->l && wg->t )
		gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
	gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);

	/* toolbar */
	if(PREFS->toolbar_style == 0)
		gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
	else
		gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);

	gtk_widget_show_all (window);

	//minor ?
	hb_widget_visible(data->CM_minor, PREFS->euro_active);

	return(window);
}


/*
** ============================================================================
*/

static void list_vehicle_cell_data_func_date (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GDate *date;
guint32 julian;
gchar buf[256];

	gtk_tree_model_get(model, iter,
		LST_CAR_DATE, &julian,
		-1);

	date = g_date_new_julian (julian);
	g_date_strftime (buf, 256-1, PREFS->date_format, date);
	g_date_free(date);

	g_object_set(renderer, "text", buf, NULL);
}

static void list_vehicle_cell_data_func_distbyvol (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gdouble distance;
gchar *text;

	gtk_tree_model_get(model, iter, user_data, &distance, -1);

	if(distance != 0)
	{
		text = g_strdup_printf(PREFS->vehicle_unit_dist1, distance);
		g_object_set(renderer, "text", text, NULL);
		g_free(text);
	}
	else
		g_object_set(renderer, "text", "-", NULL);
}

static void list_vehicle_cell_data_func_distance (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
guint distance;
gchar *text;

	gtk_tree_model_get(model, iter, user_data, &distance, -1);

	if(distance != 0)
	{
		text = g_strdup_printf(PREFS->vehicle_unit_dist0, distance);
		g_object_set(renderer, "text", text, NULL);
		g_free(text);
	}
	else
		g_object_set(renderer, "text", "-", NULL);
}

static void list_vehicle_cell_data_func_volume (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gdouble volume;
gboolean partial;
gchar *text;

	gtk_tree_model_get(model, iter, user_data, &volume, LST_CAR_PARTIAL, &partial, -1);

	if(volume != 0)
	{
		text = g_strdup_printf(PREFS->vehicle_unit_vol, volume);
		g_object_set(renderer, 
			"text", text,
		    "style-set", TRUE,
			"style", partial ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,   
		    NULL);

		g_free(text);
	}
	else
		g_object_set(renderer, "text", "-", NULL);
}

static void list_vehicle_cell_data_func_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble  value;
gchar *color;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	gtk_tree_model_get(model, iter,
		user_data, &value,
		-1);

	if( value )
	{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);

		color = get_normal_color_amount(value);

		g_object_set(renderer,
			"foreground",  color,
			"text", buf,
			NULL);	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
}

static GtkTreeViewColumn *list_vehicle_column_volume(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_vehicle_cell_data_func_volume, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


static GtkTreeViewColumn *list_vehicle_column_distbyvol(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_vehicle_cell_data_func_distbyvol, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


static GtkTreeViewColumn *list_vehicle_column_distance(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_vehicle_cell_data_func_distance, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}

static GtkTreeViewColumn *list_vehicle_column_amount(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_vehicle_cell_data_func_amount, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


/*
** create our statistic list
*/
static GtkWidget *list_vehicle_create(void)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	/* create list store */
	store = gtk_list_store_new(
	  	NUM_LST_CAR,
		G_TYPE_UINT,	//date
		G_TYPE_STRING,	//memo
		G_TYPE_UINT,	//meter
		G_TYPE_DOUBLE,	//fuel
		G_TYPE_DOUBLE,	//price
		G_TYPE_DOUBLE,  //amount
		G_TYPE_UINT,	//dist
		G_TYPE_DOUBLE,	//100km
	    G_TYPE_DOUBLE,	//distbyvol
	    G_TYPE_BOOLEAN	//ispartial
	);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	/* column date */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Date"));
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	renderer = gtk_cell_renderer_text_new();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_CAR_DATE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_vehicle_cell_data_func_date, NULL, NULL);

/*
	LST_CAR_DATE,
	LST_CAR_MEMO,
	LST_CAR_METER,
	LST_CAR_FUEL,
	LST_CAR_PRICE,
	LST_CAR_AMOUNT,
	LST_CAR_DIST,
	LST_CAR_100KM

*/

	/* column: Memo */
/*
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Memo"));
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", LST_CAR_MEMO);
	//gtk_tree_view_column_set_cell_data_func(column, renderer, repvehicle_text_cell_data_function, NULL, NULL);
*/

	/* column: Meter */
	column = list_vehicle_column_distance(_("Meter"), LST_CAR_METER);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Fuel load */
	column = list_vehicle_column_volume(_("Fuel"), LST_CAR_FUEL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Price by unit */
	column = list_vehicle_column_amount(_("Price"), LST_CAR_PRICE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Amount */
	column = list_vehicle_column_amount(_("Amount"), LST_CAR_AMOUNT);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Distance done */
	column = list_vehicle_column_distance(_("Dist."), LST_CAR_DIST);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: consumption for 100Km */
	column = list_vehicle_column_volume(PREFS->vehicle_unit_100, LST_CAR_100KM);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: km by liter (distance by volume */
	column = list_vehicle_column_distbyvol(PREFS->vehicle_unit_distbyvol, LST_CAR_DISTBYVOL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);


  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);

	return(view);
}
