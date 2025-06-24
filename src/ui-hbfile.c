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

#include "ui-widgets.h"
#include "hbtk-switcher.h"

#include "ui-hbfile.h"
#include "ui-category.h"


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


gchar *CYA_TXN_POSTMODE[] = { 
	N_("Due Date"),
	N_("Next Payout"),
	N_("In Advance"),
	NULL
};


/* = = = = = = = = = = = = = = = = */


static void defhbfile_cb_update_maxpostdate(GtkWidget *widget, gpointer user_data)
{
struct defhbfile_data *data;
gint smode, weekday, nbdays, nbmonths;
guint32 maxpostdate;
gchar buffer[256], *newtext;
GDate *date;

	DB( g_print("\n[ui-hbfile] update maxpostdate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	smode    = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_postmode));
	nbdays   = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NU_nbdays));

	weekday  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NU_weekday));
	nbmonths = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NU_nbmonths));

	DB( g_print(" -> postmode=%d\n", smode) );

	hb_widget_visible(data->GR_payout, smode == ARC_POSTMODE_PAYOUT ? TRUE : FALSE);
	hb_widget_visible(data->GR_advance, smode == ARC_POSTMODE_ADVANCE ? TRUE : FALSE);
	
	//fill in the max date evaluation
	maxpostdate = scheduled_date_get_post_max(GLOBALS->today, smode, nbdays, weekday, nbmonths);
	date = g_date_new_julian (maxpostdate);
	g_date_strftime (buffer, 256-1, PREFS->date_format, date);

	//#2102726 
	newtext = g_strdup_printf(_("Maximum post date is %s (included)"), buffer);
	gtk_label_set_text(GTK_LABEL(data->LB_maxpostdate), newtext);
	g_date_free(date);
	g_free(newtext);
}


/*
** get widgets contents from the selected account
*/
static void defhbfile_get(GtkWidget *widget, gpointer user_data)
{
struct defhbfile_data *data;
gchar	*owner;
guint32	vehicle;
gint	smode, weekday, nbdays, nbmonths;
gdouble earnbyh;

	DB( g_print("\n[ui-hbfile] get\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	// get values
	owner = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_owner));
	//vehicle   = ui_cat_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_grp));
	vehicle   = ui_cat_entry_popover_get_key(GTK_BOX(data->PO_grp));

	smode = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_postmode));
	weekday = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NU_weekday));
	nbdays  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NU_nbdays));
	nbmonths  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NU_nbmonths));

	earnbyh = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_earnbyh));

	// check for changes
	if(strcasecmp(owner, GLOBALS->owner))
		data->change++;

	if(smode != GLOBALS->auto_smode)
		data->change++;
	if(weekday != GLOBALS->auto_weekday)
		data->change++;
	if(nbmonths != GLOBALS->auto_nbmonths)
		data->change++;
	if(nbdays != GLOBALS->auto_nbdays)
		data->change++;

	if(earnbyh != GLOBALS->lifen_earnbyh)
		data->change++;

	if(vehicle != GLOBALS->vehicle_category)
		data->change++;


	// update
	if (owner && *owner)
		hbfile_change_owner(g_strdup(owner));

	GLOBALS->vehicle_category = vehicle;
	GLOBALS->auto_smode   = smode;
	GLOBALS->auto_weekday = weekday;
	GLOBALS->auto_nbmonths  = nbmonths;
	GLOBALS->auto_nbdays  = nbdays;
	
	GLOBALS->lifen_earnbyh = earnbyh;

	DB( g_print(" -> owner %s\n", GLOBALS->owner) );
	DB( g_print(" -> ccgrp %d\n", GLOBALS->vehicle_category) );
	DB( g_print(" -> smode %d\n", GLOBALS->auto_smode) );
	DB( g_print(" -> weekday %d\n", GLOBALS->auto_weekday) );
	DB( g_print(" -> nbmonths %d\n", GLOBALS->auto_nbmonths) );
	DB( g_print(" -> nbdays %d\n", GLOBALS->auto_nbdays) );

}


/*
** set widgets contents from the selected account
*/
static void defhbfile_set(GtkWidget *widget, gpointer user_data)
{
struct defhbfile_data *data;

	DB( g_print("\n[ui-hbfile] set\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" -> ccgrp %d\n", GLOBALS->vehicle_category) );
	DB( g_print(" -> autoinsert %d\n", GLOBALS->auto_nbdays) );

	if(GLOBALS->owner) gtk_entry_set_text(GTK_ENTRY(data->ST_owner), GLOBALS->owner);
	//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_grp), GLOBALS->vehicle_category);
	ui_cat_entry_popover_set_active(GTK_BOX(data->PO_grp), GLOBALS->vehicle_category);

	hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_postmode), GLOBALS->auto_smode);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NU_nbdays), GLOBALS->auto_nbdays);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NU_weekday), GLOBALS->auto_weekday);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NU_nbmonths), GLOBALS->auto_nbmonths);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_earnbyh), GLOBALS->lifen_earnbyh);

	defhbfile_cb_update_maxpostdate(widget, user_data);
}


static void defhbfile_cb_postmode_toggled(GtkWidget *radiobutton, gpointer user_data)
{
//struct defhbfile_data *data;
//gint postmode;
	
	DB( g_print("\n[ui-hbfile] toggle postmode\n") );

	defhbfile_cb_update_maxpostdate(radiobutton, NULL);
}


static gboolean defhbfile_cleanup(struct defhbfile_data *data, gint result)
{
gboolean doupdate = FALSE;

	DB( g_print("\n[ui-hbfile] cleanup\n") );

	if(result == GTK_RESPONSE_ACCEPT)
	{
		defhbfile_get(data->ST_owner, NULL);


		DB( g_print(" -> GLOBAL change = %d\n", GLOBALS->changes_count) );

		DB( g_print(" -> we update, change = %d\n", data->change) );


		GLOBALS->changes_count += data->change;
	}
	return doupdate;
}


static void defhbfile_setup(struct defhbfile_data *data)
{
	DB( g_print("\n[ui-hbfile] setup\n") );

	data->change = 0;

	//5.5 done in popover
	//ui_cat_comboboxentry_populate(GTK_COMBO_BOX(data->PO_grp), GLOBALS->h_cat);

	defhbfile_set(data->ST_owner, NULL);

}


GtkWidget *create_defhbfile_dialog (void)
{
struct defhbfile_data *data;
GtkWidget *dialog, *content_area, *hbox, *content_grid, *group_grid;
GtkWidget *label, *widget;
gint crow, row;

	DB( g_print("\n[ui-hbfile] new\n") );

	data = g_malloc0(sizeof(struct defhbfile_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("File properties"),
				GTK_WINDOW(GLOBALS->mainwindow),
				0,
				_("_Cancel"),
				GTK_RESPONSE_REJECT,
				_("_OK"),
				GTK_RESPONSE_ACCEPT,
				NULL);

	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" - window=%p, inst_data=%p\n", dialog, data) );

	gtk_window_set_resizable(GTK_WINDOW (dialog), FALSE);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));	// return a vbox

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	hb_widget_set_margin(GTK_WIDGET(content_grid), SPACING_LARGE);
	hbtk_box_prepend (GTK_BOX (content_area), content_grid);

	crow = 0;
	// group :: General
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);
	
	label = make_label_group(_("General"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_widget(_("_Title:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_owner = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	// group :: Scheduled transactions
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);
	
	label = make_label_group(_("Scheduled transactions"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	//message of post date
	label = make_label(NULL, 0.0, 0.5);
	data->LB_maxpostdate = label;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(label)), GTK_STYLE_CLASS_DIM_LABEL);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 2, 1);

	row++;
	label = make_label_widget(_("Mode:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_TXN_POSTMODE, TRUE);
	data->RA_postmode = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//next payout group
	row++;
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	data->GR_payout = hbox;
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

		widget = make_numeric(NULL, 1, 28);
		data->NU_weekday = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		label = make_label(_("of each"), 0.0, 0.5);
		gtk_box_prepend (GTK_BOX (hbox), label);

		widget = make_numeric(NULL, 1, 12);
		data->NU_nbmonths = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		label = make_label(_("month"), 0.0, 0.5);
		gtk_box_prepend (GTK_BOX (hbox), label);

	//in advance group
	row++;
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	data->GR_advance = hbox;
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

		widget = make_numeric(NULL, 1, 366);
		data->NU_nbdays = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		//TRANSLATORS: there is a spinner on the left of this label, and so you have 0....x days in advance the current date
		label = make_label(_("days"), 0.0, 0.5);
		gtk_box_prepend (GTK_BOX (hbox), label);


	// group :: life energy
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);

	label = make_label_group(_("Life Energy"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	label = make_label_widget(_("_Earn by hour:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_amount_pos(label);
	data->ST_earnbyh = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	
	// group :: Vehicle cost
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);

	label = make_label_group(_("Vehicle cost"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_widget(_("_Category:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	//widget = ui_cat_comboboxentry_new(label);
	widget = ui_cat_entry_popover_new(label);
	data->PO_grp = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	gtk_widget_show_all (dialog);

	//setup, init and show window
	defhbfile_setup(data);
	//defhbfile_update(data->LV_arc, NULL);


	g_signal_connect (data->RA_postmode, "changed", G_CALLBACK (defhbfile_cb_postmode_toggled), &dialog);

	g_signal_connect (data->NU_nbdays, "value-changed", G_CALLBACK (defhbfile_cb_update_maxpostdate), NULL);
	g_signal_connect (data->NU_weekday, "value-changed", G_CALLBACK (defhbfile_cb_update_maxpostdate), NULL);
	g_signal_connect (data->NU_nbmonths, "value-changed", G_CALLBACK (defhbfile_cb_update_maxpostdate), NULL);


	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	defhbfile_cleanup(data, result);
	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);
	
	return dialog;
}


