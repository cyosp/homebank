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

#include "hub-account.h"

#include "dsp-mainwindow.h"

#include "list-account.h"

#include "hub-scheduled.h"
#include "hub-reptotal.h"
#include "hub-reptime.h"
#include "hub-transaction.h"

#include "dsp-account.h"
#include "ui-assist-import.h"
#include "ui-assist-start.h"
#include "ui-account.h"
#include "ui-currency.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-archive.h"
#include "ui-assign.h"
#include "ui-budget.h"
#include "ui-budget-tabview.h"
#include "ui-pref.h"
#include "ui-hbfile.h"
#include "ui-transaction.h"
#include "ui-tag.h"

#include "rep-balance.h"
#include "rep-budget.h"
#include "rep-stats.h"
#include "rep-time.h"
#include "rep-vehicle.h"

#include "gtk-chart.h"

//old url prior 2019
//#define HOMEBANK_URL_HELP           "http://homebank.free.fr/help/"
//#define HOMEBANK_URL_HELP_ONLINE    "https://launchpad.net/homebank/+addquestion"
//#define HOMEBANK_URL_HELP_PROBLEM   "https://launchpad.net/homebank/+filebug"
//#define HOMEBANK_URL_HELP_TRANSLATE "https://launchpad.net/homebank/+translations"

#define HOMEBANK_URL_HELP           "index.html"
#define HOMEBANK_URL_HELP_ONLINE    "http://homebank.free.fr/support.php"
#define HOMEBANK_URL_HELP_DONATE    "http://homebank.free.fr/donate.php"
#define HOMEBANK_URL_HELP_UPDATES   "http://homebank.free.fr/downloads.php"
#define HOMEBANK_URL_HELP_PROBLEM   "http://homebank.free.fr/development.php#bug"
#define HOMEBANK_URL_HELP_TRANSLATE "http://homebank.free.fr/development.php#translate"


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


/* our functions prototype */

void ui_mainwindow_open(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_save(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_revert(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_action(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_toggle_minor(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_clear(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_update(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_addtransactions(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_recent_add (struct hbfile_data *data, const gchar *path);

void ui_mainwindow_recent_add (struct hbfile_data *data, const gchar *path);


static void ui_mainwindow_action_toggle_minor(GtkCheckMenuItem *menuitem, gpointer     user_data);

/* TODO: a bouger */


/*
**
*/
void ui_mainwindow_revert(GtkWidget *widget, gpointer user_data)
{
//struct hbfile_data *data;
gchar *basename;
gchar *title;
gchar *secondtext;
gint result;

	DB( g_print("\n[ui-mainwindow] revert\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	basename = g_path_get_basename(GLOBALS->xhb_filepath);
	title = g_strdup_printf (
		_("Revert unsaved changes to file '%s'?"), basename);

	secondtext = 
		_("- Changes made to the file will be permanently lost\n"
		"- File will be reloaded from the last save (.xhb~)");

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(GLOBALS->mainwindow),
			title,
			secondtext,
			_("_Revert"),
			TRUE
		);

	g_free(title);
	g_free(basename);

	if( result == GTK_RESPONSE_OK )
	{
		DB( g_print(" - should revert\n") );
		
		hbfile_change_filepath(hb_filename_new_with_extension(GLOBALS->xhb_filepath, "xhb~"));
		ui_mainwindow_open_internal(widget, NULL);
		hbfile_change_filepath(hb_filename_new_with_extension(GLOBALS->xhb_filepath, "xhb"));
	}

}


static void
activate_url (GtkAboutDialog *about,
	      const gchar    *link,
	      gpointer        data)
{
	DB( g_print("activate url %s\n", link) );
	
	homebank_util_url_show (link);
}

static void hbfile_about(void)
{
GtkWidget *dialog;
GdkPixbuf *pixbuf;
gchar *pathfilename;
gchar *version;

  static const gchar *artists[] = {
    "Maxime DOYEN",
    NULL
  };

  static const gchar *authors[] = {
    "Lead developer:\n" \
    "Maxime DOYEN",
    "\nContributor:\n" \
    "Ga\xc3\xabtan LORIDANT (Maths formulas for charts)\n",
    NULL
  };

/*
  const gchar *documenters[] = {
    "Maxime DOYEN",
    NULL
  };
*/

	static const gchar *copyright = "Copyright \xc2\xa9 1995-2023 - Maxime DOYEN";



	version = g_strdup_printf (PACKAGE_VERSION "\n<small>Running against GTK+ %d.%d.%d</small>",
                                                     gtk_get_major_version (),
                                                     gtk_get_minor_version (),
                                                     gtk_get_micro_version ());
	
	dialog = gtk_about_dialog_new();

	gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(GLOBALS->mainwindow));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	
	gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(dialog), g_get_application_name ());
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), version);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), copyright);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Free, easy, personal accounting for everyone"));
	gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
	
	//gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), );
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://homebank.free.fr");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog), "Visit the HomeBank website");

	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), "homebank");

	pathfilename = g_build_filename(homebank_app_get_images_dir(), "splash.png", NULL);
	pixbuf = gdk_pixbuf_new_from_file(pathfilename, NULL);
	g_free(pathfilename);

	if( pixbuf )
	{
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
		g_object_unref (pixbuf);
	}
	
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), artists);
	//gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog), );
	//gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), );

	g_signal_connect (dialog, "activate-link", G_CALLBACK (activate_url), NULL);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(version);
	
}


/* hbfile action functions -------------------- */
static void ui_mainwindow_action_new(void)
{
GtkWidget *widget = GLOBALS->mainwindow;

	if( ui_dialog_msg_savechanges(widget,NULL) == TRUE )
	{
		//clear all, and init GLOBALS->xhb_filepath to default
		ui_mainwindow_clear(widget, GINT_TO_POINTER(TRUE)); // GPOINTER_TO_INT(
		ui_mainwindow_update(widget, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));

		ui_newfile_assitant_new();
	}
}

static void ui_mainwindow_action_open(void)
{
	ui_mainwindow_open(GLOBALS->mainwindow, GINT_TO_POINTER(FALSE));
}

static void ui_mainwindow_action_openbak(void)
{
	ui_mainwindow_open(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));
}

static void ui_mainwindow_action_save(void)
{
	ui_mainwindow_save(GLOBALS->mainwindow, GINT_TO_POINTER(FALSE));
}

static void ui_mainwindow_action_saveas(void)
{
	ui_mainwindow_save(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));
}

static void ui_mainwindow_action_revert(void)
{
	ui_mainwindow_revert(GLOBALS->mainwindow, NULL);
}

static void ui_mainwindow_action_close(void)
{
GtkWidget *widget = GLOBALS->mainwindow;

	if( ui_dialog_msg_savechanges(widget,NULL) == TRUE )
	{
		//clear all, and init GLOBALS->xhb_filepath to default
		ui_mainwindow_clear(widget, GINT_TO_POINTER(TRUE));
		ui_mainwindow_update(widget, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
	}

}


static void ui_mainwindow_action_quit(void)
{
	gtk_window_close(GTK_WINDOW(GLOBALS->mainwindow));
}


static void ui_mainwindow_action_file_statistics(void)
{
	ui_dialog_file_statistics();
}


static void ui_mainwindow_action_properties(void)
{
	create_defhbfile_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
}

static void ui_mainwindow_action_anonymize(void)
{
gint result;
gchar *title;
gchar *secondtext;

	title = _("Are you sure you want to anonymize the file?");

	secondtext = 
		_("Proceeding will anonymize any text, \n"
		"like 'account x', 'payee y', 'memo z', ...");

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(GLOBALS->mainwindow),
			title,
			secondtext,
			_("_Anonymize"),
			TRUE
		);

	//#1707201
	//if( result == GTK_RESPONSE_CANCEL )
	//	return;	
	if( result == GTK_RESPONSE_OK )
	{
		hbfile_anonymize();
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
	}
}


static void ui_mainwindow_action_defcurrency(void)
{
	ui_cur_manage_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
}


static void ui_mainwindow_action_defaccount(void)
{
	ui_acc_manage_dialog();

	//our global list has changed, so update the treeview
	//todo: optimize this, should not call compute balance here
	account_compute_balances ();
	ui_hub_account_populate(GLOBALS->mainwindow, NULL);

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_defpayee(void)
{
	ui_pay_manage_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_defcategory(void)
{
	ui_cat_manage_dialog();
	//todo:why refresh upcoming here??
	//ui_mainwindow_populate_upcoming(GLOBALS->mainwindow, NULL);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
}


//TODO: not ideal to do this
void ui_mainwindow_defarchive(Archive *arc)
{
struct hbfile_data *data;
GtkTreeModel *model;

	data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	// upcoming list have direct pointer to the arc (which may change during edit)
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc));
	gtk_list_store_clear (GTK_LIST_STORE(model));

	ui_arc_manage_dialog(arc);

	ui_hub_scheduled_populate(GLOBALS->mainwindow, NULL);

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void ui_mainwindow_action_defarchive(void)
{
	ui_mainwindow_defarchive(NULL);
}


static void ui_mainwindow_action_defbudget(void)
{
	ui_bud_manage_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_defbudgettable(void)
{
	ui_bud_tabview_manage_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_defassign(void)
{

	ui_asg_manage_dialog();

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void ui_mainwindow_action_deftag(void)
{

	ui_tag_manage_dialog();

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void ui_mainwindow_action_preferences(void)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
gboolean prv_includeremind;

	//#1914935 store previous includeremind state to detect a change
	prv_includeremind = PREFS->includeremind;

	defpref_dialog_new();

	if(!PREFS->euro_active)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data->MI_eurominor), FALSE);
		//gtk_toggle_action_set_active(action, FALSE);
		//ui_mainwindow_action_toggle_minor(GTK_CHECK_MENU_ITEM(data->MI_eurominor), NULL);
	}

	homebank_pref_apply ();
	
	//#1914935 if include remind change, we update balance
	if( prv_includeremind != PREFS->includeremind )
	{
		account_compute_balances();
	}
	
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL+UF_REFRESHALL));
}

/* display action */

static void ui_mainwindow_action_toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer     user_data)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	PREFS->wal_toolbar = gtk_check_menu_item_get_active(menuitem);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL));
}

static void ui_mainwindow_action_toggle_upcoming(GtkCheckMenuItem *menuitem, gpointer     user_data)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
gint flags = UF_VISUAL;

	PREFS->wal_upcoming = gtk_check_menu_item_get_active(menuitem);
	if( PREFS->wal_upcoming == TRUE )
		flags |= UF_REFRESHALL;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(flags));
}

static void ui_mainwindow_action_toggle_totchart(GtkCheckMenuItem *menuitem, gpointer     user_data)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
gint flags = UF_VISUAL;

	PREFS->wal_totchart = gtk_check_menu_item_get_active(menuitem);
	if( PREFS->wal_totchart == TRUE )
		flags |= UF_REFRESHALL;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(flags));
}

static void ui_mainwindow_action_toggle_timchart(GtkCheckMenuItem *menuitem, gpointer     user_data)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
gint flags = UF_VISUAL;

	PREFS->wal_timchart = gtk_check_menu_item_get_active(menuitem);
	if( PREFS->wal_timchart == TRUE )
		flags |= UF_REFRESHALL;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(flags));
}

static void ui_mainwindow_action_toggle_minor(GtkCheckMenuItem *menuitem, gpointer     user_data)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	GLOBALS->minor = gtk_check_menu_item_get_active(menuitem);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_acc));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_upc));

	// top spending
	gtk_chart_show_minor(GTK_CHART(data->RE_hubtot_chart), GLOBALS->minor);
	
	ui_hub_reptotal_update(data->window, data);
	ui_hub_reptime_update(data->window, data);

}


static void
ui_mainwindow_action_showalltransactions(GtkMenuItem *menuitem, gpointer     user_data)
{
GtkWidget *window;
		
	if( GLOBALS->alltxnwindow == NULL )
	{
		window = register_panel_window_new(NULL);
		register_panel_window_init(window, NULL);
	}
	else
	{
		if(GTK_IS_WINDOW(GLOBALS->alltxnwindow))
			gtk_window_present(GTK_WINDOW(GLOBALS->alltxnwindow));
	}
}


static void
ui_mainwindow_action_showtransactions(GtkMenuItem *menuitem, gpointer     user_data)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
GtkWidget *window;
		
	//TODO: change this
	if( data->acc && data->acc->window == NULL )
	{
		window = register_panel_window_new(data->acc);
		register_panel_window_init(window, NULL);
	}
	else
	{
		if(GTK_IS_WINDOW(data->acc->window))
			gtk_window_present(GTK_WINDOW(data->acc->window));
	}
	
}


static void ui_mainwindow_action_addtransactions(void)
{
	ui_mainwindow_addtransactions(GLOBALS->mainwindow, NULL);
}

static void ui_mainwindow_action_checkscheduled(void)
{
	ui_hub_scheduled_postall(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));
}

static void ui_mainwindow_action_statistic(void)
{
	repstats_window_new();
}

static void ui_mainwindow_action_trendtime(void)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
	
	reptime_window_new(data->acc != NULL ? data->acc->key : 0);
}

static void ui_mainwindow_action_budget(void)
{
	repbudget_window_new();
}

static void ui_mainwindow_action_balance(void)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	repbalance_window_new(data->acc != NULL ? data->acc->key : 0);
}

static void ui_mainwindow_action_vehiclecost(void)
{
	repvehicle_window_new();
}

static void ui_mainwindow_action_import(void)
{
/*const gchar *name;
gint filetype = FILETYPE_UNKNOWN;

	name = gtk_action_get_name(action);

	if( g_str_has_suffix (name, "QIF"))
	   filetype= FILETYPE_QIF;
	else
	if( g_str_has_suffix (name, "OFX"))
	   filetype= FILETYPE_OFX;
	else
	if( g_str_has_suffix (name, "CSV"))
	   filetype= FILETYPE_CSV_HB;*/

	//DB( g_print("action %s type=%d\n", name, filetype) );

	ui_import_assistant_new(NULL);

}


static void ui_mainwindow_action_about(void)
{
	hbfile_about();


}


static void ui_mainwindow_action_export(void)
{
gchar *filename;

	if( ui_file_chooser_qif(NULL, &filename) == TRUE )
	{
		hb_export_qif_account_all(filename);
		g_free( filename );
	}
}


static void ui_mainwindow_action_help(void)
{
gchar *link;

    link = g_build_filename("file:///", homebank_app_get_help_dir(), HOMEBANK_URL_HELP, NULL );
	homebank_util_url_show (link);
    g_free(link);
}


static void ui_mainwindow_action_help_releasenotes(void)
{
gchar *link;

	#ifdef G_OS_WIN32
    	link = g_build_filename("file:///", homebank_app_get_datas_dir(), "ChangeLog.txt", NULL );
	#else
		link = g_build_filename("file:///", homebank_app_get_datas_dir(), "ChangeLog", NULL );
	#endif
	homebank_util_url_show (link);
    g_free(link);
}


//todo: move this to a ui-assist-welcome.c



static void ui_mainwindow_action_help_welcome_cb (GtkWidget *widget, gpointer user_data)
{
GtkWidget *dialog;
gint response = GPOINTER_TO_INT(user_data);

	dialog = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
	if( GTK_IS_DIALOG(dialog) )
		gtk_dialog_response (GTK_DIALOG(dialog), response);
}


gint ui_mainwindow_action_help_welcome(void)
{
GtkWidget *dialog, *content_area;
GtkWidget *mainvbox, *table, *widget, *label, *check;
gchar *pathfilename;

	dialog = gtk_dialog_new_with_buttons (_("Welcome to HomeBank"),
			GTK_WINDOW(GLOBALS->mainwindow),
			0,
			_("_Close"),
			//_("_Exit"),
			GTK_RESPONSE_CLOSE,
			NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	table = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content_area), table, FALSE, FALSE, 0);

	//get our icon
	pathfilename = g_build_filename(homebank_app_get_images_dir(), "homebank-icon.svg", NULL);
	if (g_file_test (pathfilename, G_FILE_TEST_EXISTS) == TRUE)
		widget = gtk_image_new_from_file((const gchar *)pathfilename);
	else
		widget = gtk_image_new_from_icon_name ("homebank", GTK_ICON_SIZE_DIALOG);
	g_free(pathfilename);
	gtk_grid_attach (GTK_GRID (table), widget, 0, 0, 1, 2);
	
	label = make_label (_("HomeBank"), 0, 0);
	gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, PANGO_ATTR_SCALE,  PANGO_SCALE_XX_LARGE, -1);
	gtk_grid_attach (GTK_GRID (table), label, 1, 0, 1, 1);

	label = make_label (_("Free, easy, personal accounting for everyone"), 0, 0);
	gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL, -1);
	gtk_grid_attach (GTK_GRID (table), label, 1, 1, 1, 1);

	
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start (GTK_BOX (content_area), widget, FALSE, FALSE, 0);

	
	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content_area), mainvbox, FALSE, FALSE, 0);
	gtk_widget_set_halign (mainvbox, GTK_ALIGN_CENTER);

	//label = make_label (_("What do you want to do:"), 0, 0);
	//gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, -1);
	//gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	widget = gtk_button_new_with_mnemonic(_("Open _last opened file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome_cb), GINT_TO_POINTER(HB_WELCOME_OPENLAST));

	widget = gtk_button_new_with_mnemonic(_("Create a _new file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome_cb), GINT_TO_POINTER(HB_WELCOME_CREATENEW));

	widget = gtk_button_new_with_mnemonic(_("_Open an existing file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome_cb), GINT_TO_POINTER(HB_WELCOME_OPENEXISTING));

	widget = gtk_button_new_with_mnemonic(_("Open the _example file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome_cb), GINT_TO_POINTER(HB_WELCOME_OPENSAMPLE));

	widget = gtk_button_new_with_mnemonic(_("Read HomeBank _Manual"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome_cb), GINT_TO_POINTER(HB_WELCOME_READMANUAL));
	
	widget = gtk_button_new_with_mnemonic(_("Configure _preferences"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome_cb), GINT_TO_POINTER(HB_WELCOME_CONFIGPREF));

	check = gtk_check_button_new_with_mnemonic (_("Show this window next time"));
	gtk_box_pack_end (GTK_BOX (mainvbox), check, FALSE, FALSE, SPACING_SMALL);

	//init
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), PREFS->showwelcome);

	//connect all our signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);

	gtk_widget_show_all (dialog);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	PREFS->showwelcome = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check));

	// cleanup and destroy
	gtk_window_destroy (GTK_WINDOW(dialog));

	// do appropriate action
	switch(result)
	{
		case HB_WELCOME_OPENLAST:
			{
			gchar *rawfilepath = homebank_lastopenedfiles_load();
			ui_mainwindow_open_check(GLOBALS->mainwindow, rawfilepath);			
			}
			break;
		case HB_WELCOME_READMANUAL:
			ui_mainwindow_action_help();
			break;
		case HB_WELCOME_CONFIGPREF:
			ui_mainwindow_action_preferences();
			break;
		case HB_WELCOME_CREATENEW:
			ui_mainwindow_action_new();
			break;
		case HB_WELCOME_OPENEXISTING:
			ui_mainwindow_action_open();
			break;
		case HB_WELCOME_OPENSAMPLE:
			hbfile_change_filepath(g_build_filename(homebank_app_get_datas_dir(), "example.xhb", NULL));
			ui_mainwindow_open_internal(GLOBALS->mainwindow, NULL);
			break;
	}

	return result;
}


static void ui_mainwindow_action_help_updates(void)
{
const gchar *link = HOMEBANK_URL_HELP_UPDATES;

	homebank_util_url_show (link);
}


static void ui_mainwindow_action_help_online(void)
{
const gchar *link = HOMEBANK_URL_HELP_ONLINE;

	homebank_util_url_show (link);
}

static void ui_mainwindow_action_help_donate(void)
{
const gchar *link = HOMEBANK_URL_HELP_DONATE;

	homebank_util_url_show (link);
}


static void ui_mainwindow_action_help_translate(void)
{
const gchar *link = HOMEBANK_URL_HELP_TRANSLATE;

	homebank_util_url_show (link);
}


static void ui_mainwindow_action_help_problem(void)
{
const gchar *link = HOMEBANK_URL_HELP_PROBLEM;

	homebank_util_url_show (link);
}


/* hbfile functions -------------------- */


/*
**
*/
static void ui_mainwindow_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_mainwindow_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(UF_SENSITIVE));
}


static void ui_mainwindow_close_openbooks(void)
{
GList *lacc, *elt;

	DB( g_print("\n[ui-mainwindow] close openbooks\n") );

	lacc = elt = g_hash_table_get_values(GLOBALS->h_acc);
	while (elt != NULL)
	{
	Account *item = elt->data;

		if(item->window)
		{
			gtk_window_destroy (GTK_WINDOW(item->window));
			item->window = NULL;
		}

		elt = g_list_next(elt);
	}
	g_list_free(lacc);

}



/*
**
*/
void ui_mainwindow_clear(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gboolean file_clear = GPOINTER_TO_INT(user_data);
GSList *list;

	DB( g_print("\n[ui-mainwindow] clear\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	// Close opened account window
	// Clear TreeView
	ui_mainwindow_close_openbooks();
	
	//5.5.1 should close any toplevel windows as well (reports...)
	DB( g_print(" closing %d windows\n", g_slist_length(GLOBALS->openwindows) ));
	list = GLOBALS->openwindows;
	while( list != NULL)
	{
	GtkWindow *window = GTK_WINDOW(list->data);
	
		DB( g_print(" closing type:%d title:'%s'\n", 
			gtk_window_get_window_type(window),
			gtk_window_get_title(window) ));

		gtk_window_destroy (GTK_WINDOW(window));
		list = g_slist_next(list);	
	}
	
	gtk_tree_store_clear(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc))));

	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc))));
	
	//TODO those 2 do nothing
	ui_hub_reptotal_clear(GLOBALS->mainwindow, NULL);
	ui_hub_reptime_clear(GLOBALS->mainwindow, NULL);

	data->showall = FALSE;
	ui_hub_account_dispose(data);
	ui_hub_account_setup(data);

	ui_hub_reptotal_dispose(data);
	ui_hub_reptotal_setup(data);

	ui_hub_reptime_dispose(data);
	ui_hub_reptime_setup(data);
	
	hbfile_cleanup(file_clear);
	hbfile_setup(file_clear);

}


/*
** add some transactions directly
*/
void ui_mainwindow_addtransactions(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkWidget *window;
gint result = HB_RESPONSE_ADD;
gint kacc, count;
Transaction *ope;
	
	DB( g_print("\n[ui-mainwindow] add transactions\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	/* init the transaction */
	homebank_app_date_get_julian();

	//#1656531
	kacc = (data->acc != NULL) ? data->acc->key : 0;
	window = create_deftransaction_window(GTK_WINDOW(data->window), TXN_DLG_ACTION_ADD, TXN_DLG_TYPE_TXN, kacc);
	ope = da_transaction_malloc();
	count = 0;
	while(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP)
	{
		/* fill in the transaction */
		if( result == HB_RESPONSE_ADD )
		{
			da_transaction_init(ope, kacc);
		}

		deftransaction_set_transaction(window, ope);

		result = gtk_dialog_run (GTK_DIALOG (window));

		DB( g_print(" - dialog result is %d\n", result) );

		if(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP || result == GTK_RESPONSE_ACCEPT)
		{
			deftransaction_get(window, NULL);
			transaction_add(GTK_WINDOW(GLOBALS->mainwindow), ope);
			//#1831975
			if(PREFS->txn_showconfirm)
				deftransaction_external_confirm(window, ope);
			
			DB( g_print(" - added 1 transaction to %d\n", ope->kacc) );

			ui_hub_account_compute(GLOBALS->mainwindow, NULL);
			
			count++;
		}
	}

	da_transaction_free(ope);
	
	deftransaction_dispose(window, NULL);
	gtk_window_destroy (GTK_WINDOW(window));

	/* todo optimize this */
	if(count > 0)
	{
		GLOBALS->changes_count += count;
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE+UF_REFRESHALL));
	}
}


static gboolean ui_mainwindow_open_backup_check_confirm(gchar *filepath)
{
gboolean retval = FALSE;
gchar *basename, *secondtext;
gboolean result;

	basename = g_path_get_basename(filepath);
	secondtext = g_strdup_printf (
	_("You are about to open the backup file '%s'.\n\nAre you sure you want to do this?"), basename);		

	result = ui_dialog_msg_confirm_alert(
		GTK_WINDOW(GLOBALS->mainwindow),
		_("Open the backup file?"),
		secondtext,
		_("_Open backup"),
		FALSE
	);	

	g_free(secondtext);
	g_free(basename);

	if( result == GTK_RESPONSE_OK )
		retval = TRUE;
	
	return retval;
}


void ui_mainwindow_open_check(GtkWidget *widget, gchar *filepath)
{
//struct hbfile_data *data;
gboolean doopen = TRUE;

	if( filepath == NULL )
		return;

	DB( g_print("\n[ui-mainwindow] open check\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	
	DB( g_print(" path='%s'\n", filepath) );

	if (g_file_test (filepath, G_FILE_TEST_EXISTS) == TRUE)
	{
		//#1710955 test for backup open
		if( hbfile_file_isbackup(filepath) )
		{
			if( ui_mainwindow_open_backup_check_confirm(filepath) == TRUE )
			{
				GLOBALS->hbfile_is_bak = TRUE;
				doopen = TRUE;
			}
			else
			{
				doopen = FALSE;
			}
		}
	}
	else
	{
		g_warning (_("Unable to open '%s', the file does not exist.\n"), filepath);
	}

	if(doopen)
	{
		hbfile_change_filepath(g_strdup(filepath));
		ui_mainwindow_open_internal(widget, NULL);
	}	

	g_free(filepath);
}


void ui_mainwindow_open(GtkWidget *widget, gpointer user_data)
{
//struct hbfile_data *data;
gboolean bakmode = GPOINTER_TO_INT(user_data);;
gboolean doopen = TRUE;
gchar *filename = NULL;

	DB( g_print("\n[ui-mainwindow] open\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#1791554 do ask for save confirm
	if( bakmode != TRUE )
		doopen = ui_dialog_msg_savechanges(widget,NULL);

	if( doopen == TRUE )
	{
		if( ui_file_chooser_xhb(GTK_FILE_CHOOSER_ACTION_OPEN, &filename, bakmode) == TRUE )
		{
			ui_mainwindow_open_check(widget, filename);
		}
	}
}


/*
 *	open the file stored in GLOBALS->xhb_filepath
 */
void ui_mainwindow_open_internal(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gint r;

	DB( g_print("\n[ui-mainwindow] open internal\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( GLOBALS->xhb_filepath != NULL )
	{
		DB( g_print(" - filename: '%s'\n", GLOBALS->xhb_filepath) );

		ui_mainwindow_clear(GLOBALS->mainwindow, GINT_TO_POINTER(FALSE));
		GLOBALS->hbfile_is_new = FALSE;

		r = homebank_load_xml(GLOBALS->xhb_filepath);
		if( r == XML_OK )
		{
			DB( g_print(" - file loaded ok : rcode=%d\n", r) );

			GLOBALS->xhb_timemodified = hbfile_file_get_time_modified(GLOBALS->xhb_filepath);
			hbfile_file_hasrevert(GLOBALS->xhb_filepath);
			
			if(PREFS->appendscheduled)
			{
				DB( g_print(" - auto: post all pending\n") );
				scheduled_post_all_pending();
			}

			if(PREFS->do_update_currency)
			{
				DB( g_print(" - auto: update currency\n") );
				//TODO: currency_needs_online
				ui_cur_manage_dialog_update_currencies(GTK_WINDOW(GLOBALS->mainwindow));
			}

			homebank_lastopenedfiles_save();

			//#1931816 should always sort all txn once
			DB( g_print(" - sort txn all account\n") );
			account_transaction_sort();

			//TODO: delete this after computing done at xml read
			DB( g_print(" - compute balance\n") );
			account_compute_balances();

			ui_mainwindow_recent_add(data, GLOBALS->xhb_filepath);
		}
		else
		{
		gchar *msg = _("Unknown error");

			switch(r)
			{
				case XML_IO_ERROR:
					msg = _("I/O error for file '%s'.");
					break;
				case XML_FILE_ERROR:
					msg = _("The file '%s' is not a valid HomeBank file.");
					break;
				case XML_VERSION_ERROR:
					msg = _("The file '%s' was saved with a higher version of HomeBank\nand cannot be loaded by the current version.");
					break;
			}

			ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_ERROR,
				_("File error"),
				msg,
				GLOBALS->xhb_filepath
				);

			ui_mainwindow_clear(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));

		}

		DB( g_print(" - start update window\n") );

		ui_hub_account_populate(GLOBALS->mainwindow, NULL);
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_VISUAL+UF_REFRESHALL));
	}


}


/*
**
*/
void ui_mainwindow_save(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gboolean saveas = GPOINTER_TO_INT(user_data);
gchar *filename = NULL;
gint r = XML_UNSET;

	DB( g_print("\n[ui-mainwindow] save\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( GLOBALS->hbfile_is_new == TRUE )
		saveas = 1;

	//#1710955 test for backup open
	if( GLOBALS->hbfile_is_bak == TRUE )
	{
		//todo: later for backup, should also remove datetime and .bak
		hbfile_change_filepath(hb_filename_new_with_extension(GLOBALS->xhb_filepath, "xhb"));
		saveas = 1;
	}

	if(saveas == 1)
	{
		if(ui_file_chooser_xhb(GTK_FILE_CHOOSER_ACTION_SAVE, &filename, FALSE) == TRUE)
		{
			DB( g_print(" + should save as '%s'\n", filename) );
			homebank_file_ensure_xhb(filename);
			homebank_backup_current_file();
			r = homebank_save_xml(GLOBALS->xhb_filepath);
			GLOBALS->hbfile_is_new = FALSE;
			GLOBALS->hbfile_is_bak = FALSE;
		}
		else
			return;
	}
	else
	{
	guint64 time_modified = hbfile_file_get_time_modified (GLOBALS->xhb_filepath);
	gint result = GTK_RESPONSE_OK;

		DB( g_print(" + should quick save '%s'\n + time: open=%lu :: now=%lu\n", GLOBALS->xhb_filepath, GLOBALS->xhb_timemodified, time_modified) );

		if( GLOBALS->xhb_timemodified != time_modified )
		{
			result = ui_dialog_msg_confirm_alert(
					GTK_WINDOW(GLOBALS->mainwindow),
					_("The file has been modified since reading it."),
					_("If you save it, all the external changes could be lost. Save it anyway?"),
					_("S_ave Anyway"),
					TRUE
				);
				
			if( result != GTK_RESPONSE_OK )
				return;
		}

		DB( g_print(" + saving...\n") );
		homebank_file_ensure_xhb(NULL);
		homebank_backup_current_file();
		r = homebank_save_xml(GLOBALS->xhb_filepath);
	}

	if(r == XML_OK)
	{
		DB( g_print(" + OK...\n") );
		GLOBALS->changes_count = 0;
		GLOBALS->xhb_timemodified = hbfile_file_get_time_modified (GLOBALS->xhb_filepath);
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_VISUAL));
	}
	else
	{
	gchar *msg = _("I/O error for file '%s'.");

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_ERROR,
			_("File error"),
			msg,
			GLOBALS->xhb_filepath
			);
	}
}


void ui_mainwindow_update(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gint flags;

	DB( g_print("\n[ui-mainwindow] update %p\n", user_data) );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	flags = GPOINTER_TO_INT(user_data);

	/* changes */
#if MYDEBUG == 1
	gchar *chgtxt = g_strdup_printf("%d", GLOBALS->changes_count);
	gtk_label_set_label(GTK_LABEL(data->dbgchange), chgtxt);
	g_free(chgtxt);
#endif	

	/* set window title */
	if(flags & UF_TITLE)
	{
	gchar *basename;
	gchar *changed;

		DB( g_print(" 1: wintitle %p\n", data->wintitle) );

		basename = g_path_get_basename(GLOBALS->xhb_filepath);
		g_free(data->wintitle);
		changed = (GLOBALS->changes_count > 0) ? "*" : "";
		data->wintitle = g_strdup_printf("%s%s - %s - " PROGNAME, changed, basename, GLOBALS->owner);
	    gtk_window_set_title (GTK_WINDOW (gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), data->wintitle);
		g_free(basename);
	}

	/* update disabled things */
	if(flags & UF_SENSITIVE)
	{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreePath		*path;
	gboolean	active, sensitive;

		DB( g_print(" 2: disabled, opelist count\n") );

		//#1656531
		data->acc = NULL;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc));
		active = gtk_tree_selection_get_selected(selection, &model, &iter);
		if(active)
		{
		Account *acc;
		gint depth;

			path = gtk_tree_model_get_path(model, &iter);
			depth =	gtk_tree_path_get_depth(path);

			if( depth > 1 )
			{
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, LST_DSPACC_DATAS, &acc, -1);
				data->acc = acc;
				DB( g_print(" depth is %d, acc=%p\n", depth, acc) );
			}
			else
				active = FALSE;
		}

	//5.7 browse account website
		sensitive = account_has_website(data->acc);
		DB( g_print(" account has url: %d\n", sensitive) );
		gtk_widget_set_sensitive(data->BT_browse, sensitive);

		DB( g_print(" changes %d - new %d\n", GLOBALS->changes_count, GLOBALS->hbfile_is_new) );

	// save
		DB( g_print(" save\n") );
		sensitive = (GLOBALS->changes_count != 0 ) ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_save, sensitive);
		gtk_widget_set_sensitive(data->BT_save, sensitive);
		
	// backup
		DB( g_print(" has changes & revert\n") );
		sensitive = ( (GLOBALS->changes_count != 0) && GLOBALS->xhb_hasrevert ) ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_revert, sensitive);
		//1859346 restore backup avail anytime
		//gtk_widget_set_sensitive(data->MI_openbak, sensitive);
		
	// report dialog ope, disable manage acc/pay/cat/bud/tag
		DB( g_print(" has report open ?\n") );
		sensitive = GLOBALS->define_off == 0 ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_manacc, sensitive);
		gtk_widget_set_sensitive(data->MI_manpay, sensitive);
		gtk_widget_set_sensitive(data->MI_mancat, sensitive);
		gtk_widget_set_sensitive(data->MI_manbud, sensitive);
		gtk_widget_set_sensitive(data->MI_manbudtable, sensitive);
		gtk_widget_set_sensitive(data->MI_mantag, sensitive);
		gtk_widget_set_sensitive(data->MI_prefs, sensitive);

		gtk_widget_set_sensitive(data->BT_manacc, sensitive);
		gtk_widget_set_sensitive(data->BT_manpay, sensitive);
		gtk_widget_set_sensitive(data->BT_mancat, sensitive);
		gtk_widget_set_sensitive(data->BT_manbud, sensitive);
		
	// empty account list: disable Archives, Edit, Filter, Add, Statistics, Overdrawn, Car Cost
		DB( g_print(" has acc ?\n") );
		sensitive = da_acc_length() > 0 ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_close, sensitive);
		gtk_widget_set_sensitive(data->MI_exportqif, sensitive);
		gtk_widget_set_sensitive(data->MI_anonymise, sensitive);
		gtk_widget_set_sensitive(data->MI_mantpl, sensitive);
		gtk_widget_set_sensitive(data->MI_txnadd, sensitive);
		gtk_widget_set_sensitive(data->MI_txnshow, sensitive);
		gtk_widget_set_sensitive(data->MI_txnshowall, sensitive);
		gtk_widget_set_sensitive(data->MI_repstat, sensitive);
		gtk_widget_set_sensitive(data->MI_reptime, sensitive);
		gtk_widget_set_sensitive(data->MI_repbal,  sensitive);
		gtk_widget_set_sensitive(data->MI_repbudg, sensitive);
		gtk_widget_set_sensitive(data->MI_repvehi, sensitive);

		gtk_widget_set_sensitive(data->BT_mantpl,  sensitive);
		gtk_widget_set_sensitive(data->BT_txnshow, sensitive);
		gtk_widget_set_sensitive(data->BT_txnadd,  sensitive);
		gtk_widget_set_sensitive(data->BT_repstat, sensitive);
		gtk_widget_set_sensitive(data->BT_reptime, sensitive);
		gtk_widget_set_sensitive(data->BT_repbal,  sensitive);
		gtk_widget_set_sensitive(data->BT_repbudg, sensitive);
		gtk_widget_set_sensitive(data->BT_repvehi, sensitive);
		
	// empty category list: disable Budget
		DB( g_print(" has cat ?\n") );
		sensitive = da_cat_length() > 1 ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_manbud, sensitive);
		gtk_widget_set_sensitive(data->MI_manbudtable, sensitive);
		gtk_widget_set_sensitive(data->BT_manbud, sensitive);

		
		//#1501129 no need to disable, P & C can be created from assign dialog
		//sensitive = ((da_cat_length() > 1) || (da_pay_length() > 1)) ? TRUE : FALSE;
		//gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Assign"), sensitive);
		gtk_widget_set_sensitive(data->MI_manasg, sensitive);
		gtk_widget_set_sensitive(data->BT_manasg, sensitive);

	// empty archive list: disable scheduled check
		DB( g_print(" has archive ?\n") );
		sensitive = da_archive_length() > 0 ? TRUE : FALSE;
		gtk_widget_set_sensitive(data->MI_addscheduled, sensitive);

	// no active account: disable Edit, Over
		DB( g_print(" account active ?\n") );
		sensitive = (active == TRUE ) ? TRUE : FALSE;
		if(data->acc && data->acc->window != NULL)
			sensitive = FALSE;
		gtk_widget_set_sensitive(data->MI_txnshow, sensitive);
		gtk_widget_set_sensitive(data->BT_txnshow, sensitive);
	}

	/* update toolbar, list */
	if(flags & UF_VISUAL)
	{
		DB( g_print(" 8: visual\n") );

		if(PREFS->toolbar_style == 0)
			gtk_toolbar_unset_style(GTK_TOOLBAR(data->toolbar));
		else
			gtk_toolbar_set_style(GTK_TOOLBAR(data->toolbar), PREFS->toolbar_style-1);

		gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (data->LV_acc), PREFS->grid_lines);
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_acc));

		gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (data->LV_upc), PREFS->grid_lines);
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_upc));

		//TODO: should add txn listview here as well
		DB( g_print(" - show toolbar=%d\n", PREFS->wal_toolbar) );
		hb_widget_visible(data->toolbar, PREFS->wal_toolbar); 

		DB( g_print(" - show totchart=%d\n", PREFS->wal_totchart) );
		hb_widget_visible(data->GR_hubtot, PREFS->wal_totchart); 

		DB( g_print(" - show timchart=%d\n", PREFS->wal_timchart) );
		hb_widget_visible(data->GR_hubtim, PREFS->wal_timchart); 

		DB( g_print(" - show upcoming=%d\n", PREFS->wal_upcoming) );
		hb_widget_visible(data->GR_upc, PREFS->wal_upcoming); 

		DB( g_print(" minor %d\n", PREFS->euro_active) );
		hb_widget_visible(data->MI_eurominor, PREFS->euro_active);
	}

	if(flags & UF_REFRESHALL)
	{
		DB( g_print(" 16: refreshall\n") );

		ui_hub_account_compute(GLOBALS->mainwindow, NULL);
		//5.7 don't process if not visible...
		if( PREFS->wal_totchart )
			ui_hub_reptotal_populate(GLOBALS->mainwindow, NULL);
		if( PREFS->wal_timchart )
			ui_hub_reptime_populate(GLOBALS->mainwindow, NULL);
		if( PREFS->wal_upcoming )
		{
			ui_hub_scheduled_populate(GLOBALS->mainwindow, NULL);
			ui_hub_transaction_populate(data);
		}
	}

}


static void
  ui_mainwindow_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
  {
    GtkTreeModel *model;
    GtkTreeIter   iter;

    DB( g_print ("\n[ui-mainwindow] A row has been double-clicked!\n") );

    model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
	Account *acc;
	gint dt;

       gtk_tree_model_get(model, &iter, LST_DSPACC_DATAS, &acc, LST_DSPACC_DATATYPE, &dt, -1);

		if( (dt == DSPACC_TYPE_NORMAL) && (acc != NULL) )
		{

       DB( g_print ("Double-clicked row contains name %s\n", acc->name) );

		ui_mainwindow_action_showtransactions(NULL, userdata);

       //g_free(name);
    	}
    }
  }


static void ui_mainwindow_destroy(GtkTreeView *treeview, gpointer user_data)
{
	DB( g_print("\n[ui-mainwindow] destroy\n") );

}


/*
**
*/
static gboolean ui_mainwindow_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct hbfile_data *data = user_data;
struct WinGeometry *wg;
gboolean retval = FALSE;

	DB( g_print("\n[ui-mainwindow] delete-event\n") );

	//store position and size
	wg = &PREFS->wal_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(widget));
	GdkWindowState state = gdk_window_get_state(gdk_window);
	wg->s = (state & GDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d s=%d, state=%d\n", wg->l, wg->t, wg->w, wg->h, wg->s, state & GDK_WINDOW_STATE_MAXIMIZED) );

 	PREFS->wal_vpaned = gtk_paned_get_position(GTK_PANED(data->vpaned));
 	PREFS->wal_hpaned = gtk_paned_get_position(GTK_PANED(data->hpaned));
	DB( g_print(" - vpaned=%d hpaned=%d\n", PREFS->wal_vpaned, PREFS->wal_hpaned) );

	if(PREFS->pnl_list_tab)
		g_free(PREFS->pnl_list_tab);
	PREFS->pnl_list_tab = g_strdup(gtk_stack_get_visible_child_name(GTK_STACK(data->stack)));
	
	//todo
	if(ui_dialog_msg_savechanges(widget, NULL) == FALSE)
	{
		retval = TRUE;
	}
	else
	{
		//TODO: retval is useless and below should move to destroy
		retval = TRUE;

		//ui_hub_xxx_dispose(data);
		gtk_widget_destroy(data->LV_hubtot);
		ui_hub_reptotal_dispose(data);

		gtk_widget_destroy(data->LV_hubtim);
		ui_hub_reptime_dispose(data);

		ui_hub_account_dispose(data);
		
		g_free(data->wintitle);
		g_free(user_data);
		
		gtk_main_quit();
	}

	//TRUE:stop other handlers from being invoked for the event | FALSE: propagate
	return retval;
}

#define HB_MIMETYPE "application/x-homebank"

static void
ui_mainwindow_cb_recent_chooser_clear (GtkRecentChooser *chooser, struct hbfile_data *data)
{
GList *list;

	DB( g_print("\n[ui-mainwindow] recent chooser clear\n") );

	list = gtk_recent_manager_get_items(data->recent_manager);
	while (list != NULL)
	{
	GtkRecentInfo *recentinfo = list->data;

		if( !g_strcmp0(HB_MIMETYPE, gtk_recent_info_get_mime_type(recentinfo)) )
		{
		GError *error = NULL;
		const gchar *uri = gtk_recent_info_get_uri(recentinfo);
			
			//DB( g_print(" mime-type='%s'\n", gtk_recent_info_get_mime_type(recentinfo)) );

			gtk_recent_manager_remove_item(data->recent_manager, uri, &error);
			if (error)
			{
				g_warning ("Could not remove uri \"%s\": %s", uri, error->message);
				g_error_free (error);
			}
		}		
		gtk_recent_info_unref(recentinfo);
		list = g_list_next(list);
	}

	g_list_free(list);
}


static void
ui_mainwindow_recent_chooser_item_activated_cb (GtkRecentChooser *chooser, struct hbfile_data *data)
{
gchar *uri, *path;
GError *error = NULL;

	DB( g_print("\n[ui-mainwindow] recent chooser activated\n") );
	
	uri = gtk_recent_chooser_get_current_uri (chooser);

	DB( g_print(" '%s'\n", uri) );

	
	path = g_filename_from_uri (uri, NULL, &error);
	if (error)
	{
		g_warning ("Could not convert uri \"%s\" to a local path: %s", uri, error->message);
		g_error_free (error);
		return;
	}

	if( ui_dialog_msg_savechanges(data->window, NULL) == TRUE )
	{

		//todo: FixMe
		/*
		if (! load)
		{
			gpw_recent_remove (gpw, path);
		}
		*/

		hbfile_change_filepath(path);
		ui_mainwindow_open_internal(data->window, NULL);
	}
	else
	{
		g_free (path);
	}
	g_free (uri);
}


void ui_mainwindow_recent_add (struct hbfile_data *data, const gchar *path)
{
	GtkRecentData *recent_data;
	gchar *uri;
	GError *error = NULL;

	DB( g_print("\n[ui-mainwindow] recent_add\n") );

	DB( g_print(" - file has .xhb suffix = %d\n", g_str_has_suffix (path, ".xhb") ) );

	if( g_str_has_suffix (path, ".xhb") == FALSE )	//ignore reverted file
		return;

	uri = g_filename_to_uri (path, NULL, &error);
	if (error)
	{
		g_warning ("Could not convert uri \"%s\" to a local path: %s", uri, error->message);
		g_error_free (error);
		return;
	}

	recent_data = g_slice_new (GtkRecentData);

	recent_data->display_name   = NULL;
	recent_data->description    = NULL;
	recent_data->mime_type      = HB_MIMETYPE;
	recent_data->app_name       = (gchar *) g_get_application_name ();
	recent_data->app_exec       = g_strjoin (" ", g_get_prgname (), "%u", NULL);
	recent_data->groups         = NULL;
	recent_data->is_private     = FALSE;

	if (!gtk_recent_manager_add_full (data->recent_manager,
				          uri,
				          recent_data))
	{
      		g_warning ("Unable to add '%s' to the list of recently used documents", uri);
	}

	g_free (uri);
	g_free (recent_data->app_exec);
	g_slice_free (GtkRecentData, recent_data);

}


enum
{
	TARGET_URI_LIST
};

static GtkTargetEntry drop_types[] =
{
	{"text/uri-list", 0, TARGET_URI_LIST}
};

static void ui_mainwindow_drag_data_received (GtkWidget *widget,
			GdkDragContext *context,
			gint x, gint y,
			GtkSelectionData *selection_data,
			guint info, guint time, GtkWindow *window)
{
gchar **uris, **str;
gchar *newseldata;
gint n_uris, filetype, slen;
GError *error = NULL;
	
	if (info != TARGET_URI_LIST)
		return;

	DB( g_print("\n[ui-mainwindow] drag_data_received\n") );

	/* On MS-Windows, it looks like `selection_data->data' is not NULL terminated. */
	slen = gtk_selection_data_get_length(selection_data);
	newseldata = g_new (gchar, slen + 1);
	memcpy (newseldata, gtk_selection_data_get_data(selection_data), slen);
	newseldata[slen] = 0;
	//DB( g_print(" - seldata ='%s'\n", gtk_selection_data_get_data(selection_data) ) );
	//DB( g_print(" - newseldata ='%s'\n", newseldata ) );
	
	uris = g_uri_list_extract_uris (newseldata);
	n_uris = g_strv_length(uris);
	DB( g_print(" - dragged %d files (len=%d)\n", n_uris, slen ) );

	g_free(newseldata);

	//single file: check for xhb
	if(n_uris == 1)
	{		
		filetype = hb_filename_type_get_by_extension(*uris);

		DB( g_print(" - filetype is homebank (%d)\n", filetype) );

		if( filetype == FILETYPE_HOMEBANK )
		{
		gchar *path = g_filename_from_uri (*uris, NULL, &error);

			if( path != NULL )
			{
				DB( g_print(" - path is '%s'\n", path) );
				hbfile_change_filepath(g_strdup(path));
				ui_mainwindow_open_internal(GTK_WIDGET(window), NULL);
				goto end_drop;
			}
			else
			{
				g_warning ("Could not convert uri to local path: %s", error->message);
				g_error_free (error);
			}
			g_free (path);
		}
		/* we no more manage error here
		ui_dialog_msg_infoerror(GTK_WINDOW(window), GTK_MESSAGE_ERROR,
					_("File error"),
					_("The file %s is not a valid HomeBank file."),
					path);
		*/
	}

	//collect known filetype to import
	DB( g_print(" - collect %d files\n", n_uris) );
	
	gchar **paths = g_new (gchar *, n_uris + 1);
	slen = 0;
	for (str = uris; *str; str++)
	{
		filetype = hb_filename_type_get_by_extension(*str);
		if( filetype != FILETYPE_HOMEBANK && filetype != FILETYPE_UNKNOWN )
		{
		gchar *path = g_filename_from_uri (*str, NULL, NULL);

			if( path != NULL )
			{
				DB( g_print(" - append %d '%s'\n", slen, path ) );
				paths[slen++] = path;
			}
		}   
	}
	paths[slen] = NULL;

	if( slen > 0 )
	{
		ui_import_assistant_new( paths );
	}
	
	
end_drop:
	g_strfreev (uris);
}


static GtkWidget *ui_mainwindow_create_recent_chooser_menu (struct hbfile_data *data)
{
GtkWidget *recent_menu, *menuitem;
GtkRecentFilter *filter;

	recent_menu = gtk_recent_chooser_menu_new_for_manager (data->recent_manager);
	gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER (recent_menu), FALSE);
	gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (recent_menu), GTK_RECENT_SORT_MRU);
	//todo: add a user pref for this
	gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER (recent_menu), 10);
	gtk_recent_chooser_set_show_icons (GTK_RECENT_CHOOSER (recent_menu), FALSE);
	//gtk_recent_chooser_menu_set_show_numbers (GTK_RECENT_CHOOSER_MENU (recent_menu), TRUE);

	filter = gtk_recent_filter_new ();
	//gtk_recent_filter_add_application (filter, g_get_application_name());
	gtk_recent_filter_add_pattern (filter, "*.[Xx][Hh][Bb]");
	gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (recent_menu), filter);

	gtk_menu_shell_append (GTK_MENU_SHELL (recent_menu), gtk_separator_menu_item_new());
	menuitem = hbtk_menu_add_menuitem(recent_menu, _("Clear") );

	g_signal_connect (recent_menu, "item-activated", G_CALLBACK (ui_mainwindow_recent_chooser_item_activated_cb), data);
	g_signal_connect (menuitem, "activate", G_CALLBACK (ui_mainwindow_cb_recent_chooser_clear), (gpointer)data);
	
	return recent_menu;
}


static GtkWidget *
ui_mainwindow_menubar_create(struct hbfile_data *data)
{
GtkWidget *menubar;
GtkWidget *menu, *submenu;
GtkWidget *menuitem;
GtkAccelGroup *accel_group = NULL;

	menubar = gtk_menu_bar_new ();

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(data->window), accel_group);

	menu = hbtk_menubar_add_menu(menubar, _("_File"), &data->ME_menufile);

		data->MI_new        = menuitem = hbtk_menu_add_menuitem(menu, _("_New"));
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
		data->MI_open       = menuitem = hbtk_menu_add_menuitem(menu, _("_Open..."));
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		menuitem            = hbtk_menu_add_menuitem(menu, _("Open _Recent"));
		submenu = ui_mainwindow_create_recent_chooser_menu (data);
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
		                                                        
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_save       = menuitem = hbtk_menu_add_menuitem(menu, _("_Save"));
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		data->MI_saveas     = menuitem = hbtk_menu_add_menuitem(menu, _("Save _As..."));
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_s, GDK_SHIFT_MASK|GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_import     = hbtk_menu_add_menuitem(menu, _("Import..."));
		data->MI_exportqif  = hbtk_menu_add_menuitem(menu, _("Export as QIF..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_revert     = hbtk_menu_add_menuitem(menu, _("Revert"));
		data->MI_openbak    = hbtk_menu_add_menuitem(menu, _("Restore backup"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_properties = hbtk_menu_add_menuitem(menu, _("Properties..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_close      = menuitem = hbtk_menu_add_menuitem(menu, _("_Close"));
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		data->MI_quit       = menuitem = hbtk_menu_add_menuitem(menu, _("_Quit"));
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	
	menu = hbtk_menubar_add_menu(menubar, _("_Edit"), &data->ME_menuedit);

		data->MI_prefs = hbtk_menu_add_menuitem(menu, _("Preferences..."));
	
	menu = hbtk_menubar_add_menu(menubar, _("_View"), &data->ME_menuview);

		data->MI_showtbar = menuitem = gtk_check_menu_item_new_with_mnemonic(_("_Toolbar"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		data->MI_showtotchart = menuitem = gtk_check_menu_item_new_with_mnemonic(_("T_otal Chart"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		data->MI_showtimchart = menuitem = gtk_check_menu_item_new_with_mnemonic(_("T_ime Chart"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		data->MI_showbotlist = menuitem = gtk_check_menu_item_new_with_mnemonic(_("_Bottom Lists"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_eurominor = menuitem = gtk_check_menu_item_new_with_mnemonic(_("Euro minor"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	menu = hbtk_menubar_add_menu(menubar, _("_Manage"), &data->ME_menuman);

		data->MI_manwal = hbtk_menu_add_menuitem(menu, _("Wallet..."));
		data->MI_manacc = hbtk_menu_add_menuitem(menu, _("Acc_ounts..."));
		data->MI_manpay = hbtk_menu_add_menuitem(menu, _("_Payees..."));
		data->MI_mancat = hbtk_menu_add_menuitem(menu, _("Categories..."));
		data->MI_mantpl = hbtk_menu_add_menuitem(menu, _("Scheduled/Template..."));
		data->MI_manbud = hbtk_menu_add_menuitem(menu, _("Budget..."));
		data->MI_manbudtable = hbtk_menu_add_menuitem(menu, _("Budget (table view)..."));
		data->MI_manasg = hbtk_menu_add_menuitem(menu, _("Assignments..."));
		data->MI_mancur = hbtk_menu_add_menuitem(menu, _("Currencies..."));
		data->MI_mantag = hbtk_menu_add_menuitem(menu, _("Tags..."));
	
	menu = hbtk_menubar_add_menu(menubar, _("_Transactions"), &data->ME_menutxn);

		data->MI_txnadd       = hbtk_menu_add_menuitem(menu, _("Add..."));
		data->MI_txnshow      = hbtk_menu_add_menuitem(menu, _("Show..."));
		data->MI_txnshowall   = hbtk_menu_add_menuitem(menu, _("Show all...") );
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_scheduler    = hbtk_menu_add_menuitem(menu, _("Set scheduler..."));
		data->MI_addscheduled = hbtk_menu_add_menuitem(menu, _("Post scheduled"));
	
	menu = hbtk_menubar_add_menu(menubar, _("_Reports"), &data->ME_menurep);

		data->MI_repstat = hbtk_menu_add_menuitem(menu, _("_Statistics..."));
		data->MI_reptime = hbtk_menu_add_menuitem(menu, _("_Trend Time...") );
		data->MI_repbal  = hbtk_menu_add_menuitem(menu, _("_Balance...") );
		data->MI_repbudg = hbtk_menu_add_menuitem(menu, _("B_udget..."));
		data->MI_repvehi = hbtk_menu_add_menuitem(menu, _("_Vehicle cost..."));
	
	menu = hbtk_menubar_add_menu(menubar, _("_Tools"), &data->ME_menutool);

		data->MI_welcome   = hbtk_menu_add_menuitem(menu, _("Show welcome dialog..."));
		data->MI_filestats = hbtk_menu_add_menuitem(menu, _("File statistics...") );
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_anonymise = hbtk_menu_add_menuitem(menu, _("Anonymize..."));
	
	menu = hbtk_menubar_add_menu(menubar, _("_Help"), &data->ME_menuhelp);

		data->MI_contents  = menuitem = hbtk_menu_add_menuitem(menu, _("_Contents") );
		gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_F1, 0, GTK_ACCEL_VISIBLE); 
		data->MI_online    = hbtk_menu_add_menuitem(menu, _("Get Help Online...") );
		//donate also here ?
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_updates   = hbtk_menu_add_menuitem(menu, _("Check for updates...") );
		data->MI_relnote   = hbtk_menu_add_menuitem(menu, _("Release Notes") );
		data->MI_problem   = hbtk_menu_add_menuitem(menu, _("Report a Problem...")  );
		data->MI_translate = hbtk_menu_add_menuitem(menu, _("Translate this Application...") );
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		data->MI_about     = hbtk_menu_add_menuitem(menu, _("_About") );
	
	return menubar;
}


static GtkWidget *
ui_mainwindow_toolbar_create(struct hbfile_data *data)
{
GtkWidget *toolbar, *button, *menu;

	toolbar = gtk_toolbar_new();
	
	data->BT_new  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_FILE_NEW, _("New"), _("Create a new file"));
	data->BT_open = button = gtk_widget_new(GTK_TYPE_MENU_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_FILE_OPEN,
		"label", _("Open"),
	    "tooltip-text", _("Open a file"),
	    NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM(button), 1);
	menu = ui_mainwindow_create_recent_chooser_menu(data);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button), menu);
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (button), _("Open a recently used file"));
	
	data->BT_save = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_FILE_SAVE, _("Save"), _("Save the current file"));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	data->BT_manacc  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_ACCOUNT , _("Account"), _("Manage the accounts"));
	data->BT_manpay  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_PAYEE   , _("Payee"), _("Manage the payees"));
	data->BT_mancat  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_CATEGORY, _("Category"), _("Manage the categories"));
	data->BT_mantpl  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_ARCHIVE , _("Scheduled/Template"), _("Manage the scheduled/template transactions"));
	data->BT_manbud  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_BUDGET  , _("Budget"), _("Manage the budget"));
	data->BT_manasg  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_ASSIGN  , _("Assignment"), _("Manage the automatic assignments"));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	data->BT_txnshow = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_SHOW, _("Show"), _("Shows selected account transactions"));
	data->BT_txnadd  = button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_OPE_ADD, _("Add"), _("Add transactions"));
	g_object_set(button, "is-important", TRUE, NULL);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	data->BT_repstat = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REP_STATS , _("Statistics"), _("Open the Statistics report"));
	data->BT_reptime = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REP_TIME  , _("Trend time"), _("Open the Trend Time report"));
	data->BT_repbal  = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REP_BALANCE, _("Balance"), _("Open the Balance report"));
	data->BT_repbudg = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REP_BUDGET, _("Budget"),  _("Open the Budget report"));
	data->BT_repvehi = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REP_CAR   , _("Vehicle cost"), _("Open the Vehicle cost report"));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	data->BT_help   = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_HELP  , _("Help"), _("Get Help Online..."));
	data->BT_donate = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_DONATE, _("Donate"), _("Donate to HomeBank project"));

	return toolbar;
}



/*
** the window creation
*/
GtkWidget *create_hbfile_window(GtkWidget *do_widget)
{
struct hbfile_data *data;
struct WinGeometry *wg;
GtkWidget *mainvbox, *vbox, *box, *vpaned, *hpaned, *sidebar, *stack;
GtkWidget *widget, *page, *menubar, *toolbar;
GtkWidget *window;

	DB( g_print("\n[ui-mainwindow] create main window\n") );

	data = g_malloc0(sizeof(struct hbfile_data));
	if(!data) return NULL;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	// this is our mainwindow, so store it to GLOBALS data
	data->window = window;
	GLOBALS->mainwindow = window;

	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_window_set_child(GTK_WINDOW(window), mainvbox);

	//menu and toolbar
	data->recent_manager = gtk_recent_manager_get_default ();

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (mainvbox), box, FALSE, FALSE, 0);

	menubar = ui_mainwindow_menubar_create(data);
	gtk_box_pack_start (GTK_BOX (box), menubar, TRUE, TRUE, 0);

#if HB_UNSTABLE_SHOW == TRUE
	menubar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(menubar)), GTK_STYLE_CLASS_MENUBAR);
	gtk_box_pack_start (GTK_BOX (box), menubar, FALSE, FALSE, 0);

	widget = gtk_label_new("Release Candidate Version  ");
	gimp_label_set_attributes (GTK_LABEL (widget), 
		PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, 
		PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
		-1);
	gtk_box_pack_end (GTK_BOX (menubar), widget, FALSE, FALSE, 0);
	widget = gtk_image_new_from_icon_name (ICONNAME_WARNING, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_end (GTK_BOX (menubar), widget, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_markup(GTK_WIDGET(menubar), 
	   	"<b>Guidelines:</b>\n"
		"- use only for TESTING or find BUGs\n"
	    "- use A COPY of your data files\n"
	    "- BUGs or CRASH can still occur"
		);
#endif

#if MYDEBUG == 1
	widget = make_label(NULL, 1.0, 0.5);
	gtk_widget_set_margin_end(widget, 12);
	data->dbgchange = widget;
	gtk_box_pack_end (GTK_BOX (box), widget, FALSE, FALSE, 0);
#endif


	toolbar = ui_mainwindow_toolbar_create(data);
	data->toolbar = toolbar;
	gtk_box_pack_start (GTK_BOX (mainvbox), toolbar, FALSE, FALSE, 0);
	
	/* Add the main area */
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (mainvbox), vbox, TRUE, TRUE, 0);

	vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	data->vpaned = vpaned;
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);

	// top part
	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	data->hpaned = hpaned;
	gtk_paned_pack1 (GTK_PANED(vpaned), hpaned, FALSE, FALSE);

		widget = ui_hub_account_create(data);
		//gtk_widget_set_size_request (widget, 100, -1);
		gtk_paned_pack1 (GTK_PANED(hpaned), widget, FALSE, FALSE);

		//5.7 add home time chart
		box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_set_homogeneous(GTK_BOX(box), TRUE);
		gtk_paned_pack2 (GTK_PANED(hpaned), box, TRUE, FALSE);
		
		widget = ui_hub_reptotal_create(data);
		//gtk_widget_set_size_request (widget, -1, 100);
		gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 0);

		widget = ui_hub_reptime_create(data);
		gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 0);


	// bottom part
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	data->GR_upc = box;
	gtk_paned_pack2 (GTK_PANED(vpaned), box, TRUE, FALSE);

	sidebar = gtk_stack_sidebar_new ();
	gtk_box_pack_start (GTK_BOX (box), sidebar, FALSE, FALSE, 0);

	stack = gtk_stack_new ();
	//gtk_stack_set_transition_type (GTK_STACK (stack), GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
	gtk_stack_sidebar_set_stack (GTK_STACK_SIDEBAR (sidebar), GTK_STACK (stack));
	data->stack = stack;
    gtk_box_pack_start (GTK_BOX (box), stack, TRUE, TRUE, 0);
	
		page = ui_hub_scheduled_create(data);
		gtk_stack_add_titled (GTK_STACK (stack), page, "sched", _("Scheduled"));
		//gtk_paned_pack2 (GTK_PANED(vpaned), widget, TRUE, FALSE);

		page = ui_hub_transaction_create(data, HUB_TXN_TYPE_FUTURE);
		gtk_stack_add_titled (GTK_STACK (stack), page, "futur", _("Future"));

		page = ui_hub_transaction_create(data, HUB_TXN_TYPE_REMIND);
		gtk_stack_add_titled (GTK_STACK (stack), page, "remin", _("Remind"));
	

	//setup, init and show window
	wg = &PREFS->wal_wg;
	if(wg->s == 0)
	{
		gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
		gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);
	}
	else
		gtk_window_maximize(GTK_WINDOW(window));

	gtk_widget_show_all (window);

	//#1662197/1660910 moved after resize/show
	DB( g_print(" - vpaned=%d hpaned=%d\n", PREFS->wal_vpaned, PREFS->wal_hpaned) );

	if(PREFS->wal_hpaned > 0)
		gtk_paned_set_position(GTK_PANED(data->hpaned), PREFS->wal_hpaned);
	if(PREFS->wal_vpaned > 0)
		gtk_paned_set_position(GTK_PANED(data->vpaned), PREFS->wal_vpaned);

	if( PREFS->pnl_list_tab != NULL )
		gtk_stack_set_visible_child_name (GTK_STACK(data->stack), PREFS->pnl_list_tab);

	
	//todo: move this elsewhere
	DB( g_print(" - setup stuff\n") );

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data->MI_showtbar), PREFS->wal_toolbar);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data->MI_showtotchart), PREFS->wal_totchart);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data->MI_showtimchart), PREFS->wal_timchart);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data->MI_showbotlist), PREFS->wal_upcoming);

	/* Drag and drop support, set targets to NULL because we add the
	   default uri_targets below */

	/* support for opening a file by dragging onto the project window */
	gtk_drag_dest_set (GTK_WIDGET (window),
			   GTK_DEST_DEFAULT_ALL,
			   drop_types,
	           G_N_ELEMENTS (drop_types),
			   GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (window), "drag-data-received",
			  G_CALLBACK (ui_mainwindow_drag_data_received), window);

	//connect all our signals
	DB( g_print(" - connect signals\n") );

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), "changed", G_CALLBACK (ui_mainwindow_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_acc    ), "row-activated", G_CALLBACK (ui_mainwindow_onRowActivated), GINT_TO_POINTER(2));

	/* GtkWindow events */
    g_signal_connect (window, "delete-event", G_CALLBACK (ui_mainwindow_dispose), (gpointer)data);
	g_signal_connect (window, "destroy", G_CALLBACK (ui_mainwindow_destroy), NULL);

	//menu signals
	g_signal_connect (data->MI_new , "activate", G_CALLBACK (ui_mainwindow_action_new), (gpointer)data);
	g_signal_connect (data->MI_open , "activate", G_CALLBACK (ui_mainwindow_action_open), (gpointer)data);
	g_signal_connect (data->MI_save , "activate", G_CALLBACK (ui_mainwindow_action_save), (gpointer)data);
	g_signal_connect (data->MI_saveas , "activate", G_CALLBACK (ui_mainwindow_action_saveas), (gpointer)data);
	g_signal_connect (data->MI_import , "activate", G_CALLBACK (ui_mainwindow_action_import), (gpointer)data);
	g_signal_connect (data->MI_exportqif , "activate", G_CALLBACK (ui_mainwindow_action_export), (gpointer)data);
	g_signal_connect (data->MI_revert , "activate", G_CALLBACK (ui_mainwindow_action_revert), (gpointer)data);
	g_signal_connect (data->MI_openbak , "activate", G_CALLBACK (ui_mainwindow_action_openbak), (gpointer)data);
	g_signal_connect (data->MI_properties , "activate", G_CALLBACK (ui_mainwindow_action_properties), (gpointer)data);
	g_signal_connect (data->MI_close , "activate", G_CALLBACK (ui_mainwindow_action_close), (gpointer)data);
	g_signal_connect (data->MI_quit , "activate", G_CALLBACK (ui_mainwindow_action_quit), (gpointer)data);

	g_signal_connect (data->MI_prefs , "activate", G_CALLBACK (ui_mainwindow_action_preferences), (gpointer)data);

	g_signal_connect (data->MI_showtbar , "toggled", G_CALLBACK (ui_mainwindow_action_toggle_toolbar), (gpointer)data);
	g_signal_connect (data->MI_showtotchart , "toggled", G_CALLBACK (ui_mainwindow_action_toggle_totchart), (gpointer)data);
	g_signal_connect (data->MI_showtimchart , "toggled", G_CALLBACK (ui_mainwindow_action_toggle_timchart), (gpointer)data);
	g_signal_connect (data->MI_showbotlist , "toggled", G_CALLBACK (ui_mainwindow_action_toggle_upcoming), (gpointer)data);
	g_signal_connect (data->MI_eurominor , "toggled", G_CALLBACK (ui_mainwindow_action_toggle_minor), (gpointer)data);

	g_signal_connect (data->MI_manwal , "activate", G_CALLBACK (ui_mainwindow_action_properties), (gpointer)data);
	g_signal_connect (data->MI_manacc , "activate", G_CALLBACK (ui_mainwindow_action_defaccount), (gpointer)data);
	g_signal_connect (data->MI_manpay , "activate", G_CALLBACK (ui_mainwindow_action_defpayee), (gpointer)data);
	g_signal_connect (data->MI_mancat , "activate", G_CALLBACK (ui_mainwindow_action_defcategory), (gpointer)data);
	g_signal_connect (data->MI_mantpl , "activate", G_CALLBACK (ui_mainwindow_action_defarchive), (gpointer)data);
	g_signal_connect (data->MI_manbud , "activate", G_CALLBACK (ui_mainwindow_action_defbudget), (gpointer)data);
	g_signal_connect (data->MI_manbudtable , "activate", G_CALLBACK (ui_mainwindow_action_defbudgettable), (gpointer)data);
	g_signal_connect (data->MI_manasg , "activate", G_CALLBACK (ui_mainwindow_action_defassign), (gpointer)data);
	g_signal_connect (data->MI_mancur , "activate", G_CALLBACK (ui_mainwindow_action_defcurrency), (gpointer)data);
	g_signal_connect (data->MI_mantag , "activate", G_CALLBACK (ui_mainwindow_action_deftag), (gpointer)data);

	g_signal_connect (data->MI_txnadd , "activate", G_CALLBACK (ui_mainwindow_action_addtransactions), (gpointer)data);
	g_signal_connect (data->MI_txnshow , "activate", G_CALLBACK (ui_mainwindow_action_showtransactions), (gpointer)data);
	g_signal_connect (data->MI_txnshowall , "activate", G_CALLBACK (ui_mainwindow_action_showalltransactions), (gpointer)data);
	g_signal_connect (data->MI_scheduler , "activate", G_CALLBACK (ui_mainwindow_action_properties), (gpointer)data);
	g_signal_connect (data->MI_addscheduled , "activate", G_CALLBACK (ui_mainwindow_action_checkscheduled), (gpointer)data);
	
	g_signal_connect (data->MI_repstat , "activate", G_CALLBACK (ui_mainwindow_action_statistic), (gpointer)data);
	g_signal_connect (data->MI_reptime , "activate", G_CALLBACK (ui_mainwindow_action_trendtime), (gpointer)data);
	g_signal_connect (data->MI_repbal  , "activate", G_CALLBACK (ui_mainwindow_action_balance), (gpointer)data);
	g_signal_connect (data->MI_repbudg , "activate", G_CALLBACK (ui_mainwindow_action_budget), (gpointer)data);
	g_signal_connect (data->MI_repvehi , "activate", G_CALLBACK (ui_mainwindow_action_vehiclecost), (gpointer)data);

	g_signal_connect (data->MI_welcome , "activate", G_CALLBACK (ui_mainwindow_action_help_welcome), (gpointer)data);
	g_signal_connect (data->MI_filestats , "activate", G_CALLBACK (ui_mainwindow_action_file_statistics), (gpointer)data);
	g_signal_connect (data->MI_anonymise , "activate", G_CALLBACK (ui_mainwindow_action_anonymize), (gpointer)data);

	g_signal_connect (data->MI_contents , "activate", G_CALLBACK (ui_mainwindow_action_help), (gpointer)data);
	g_signal_connect (data->MI_online , "activate", G_CALLBACK (ui_mainwindow_action_help_online), (gpointer)data);
	g_signal_connect (data->MI_updates , "activate", G_CALLBACK (ui_mainwindow_action_help_updates), (gpointer)data);
	g_signal_connect (data->MI_relnote , "activate", G_CALLBACK (ui_mainwindow_action_help_releasenotes), (gpointer)data);
	g_signal_connect (data->MI_problem , "activate", G_CALLBACK (ui_mainwindow_action_help_problem), (gpointer)data);
	g_signal_connect (data->MI_translate , "activate", G_CALLBACK (ui_mainwindow_action_help_translate), (gpointer)data);
	g_signal_connect (data->MI_about , "activate", G_CALLBACK (ui_mainwindow_action_about), (gpointer)data);

	
	//toolbar signals
	g_signal_connect (G_OBJECT (data->BT_new   ), "clicked", G_CALLBACK (ui_mainwindow_action_new), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_open  ), "clicked", G_CALLBACK (ui_mainwindow_action_open), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_save  ), "clicked", G_CALLBACK (ui_mainwindow_action_save), (gpointer)data);

	g_signal_connect (G_OBJECT (data->BT_manacc), "clicked", G_CALLBACK (ui_mainwindow_action_defaccount), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_manpay), "clicked", G_CALLBACK (ui_mainwindow_action_defpayee), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_mancat), "clicked", G_CALLBACK (ui_mainwindow_action_defcategory), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_mantpl), "clicked", G_CALLBACK (ui_mainwindow_action_defarchive), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_manbud), "clicked", G_CALLBACK (ui_mainwindow_action_defbudgettable), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_manasg), "clicked", G_CALLBACK (ui_mainwindow_action_defassign), (gpointer)data);

	g_signal_connect (G_OBJECT (data->BT_txnshow), "clicked", G_CALLBACK (ui_mainwindow_action_showtransactions), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_txnadd), "clicked", G_CALLBACK (ui_mainwindow_action_addtransactions), (gpointer)data);
	
	g_signal_connect (G_OBJECT (data->BT_repstat), "clicked", G_CALLBACK (ui_mainwindow_action_statistic), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_reptime), "clicked", G_CALLBACK (ui_mainwindow_action_trendtime), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_repbal ), "clicked", G_CALLBACK (ui_mainwindow_action_balance), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_repbudg), "clicked", G_CALLBACK (ui_mainwindow_action_budget), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_repvehi), "clicked", G_CALLBACK (ui_mainwindow_action_vehiclecost), (gpointer)data);

	g_signal_connect (G_OBJECT (data->BT_help  ), "clicked", G_CALLBACK (ui_mainwindow_action_help_online), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_donate), "clicked", G_CALLBACK (ui_mainwindow_action_help_donate), (gpointer)data);

	return window;
}

