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
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "homebank.h"

#include "ui-dialogs.h"
#include "list-operation.h"

#include "hb-currency.h"
#include "ui-currency.h"


/* = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

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



/* = = = = = = = = = = = = = = = = = = = = */


static void my_ui_dialog_add_action_class(GtkDialog *dialog, gint response_id, gchar *class_name)
{
GtkWidget *button = gtk_dialog_get_widget_for_response(dialog, response_id);

	if( button)
	{
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)), class_name);
	}
}


/* Confirmation Alert dialog */

gint ui_dialog_msg_confirm_alert(GtkWindow *parent, gchar *title, gchar *secondtext, gchar *actionverb, gboolean destructive)
{
GtkWidget *dialog;
gint retval;

	dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
	                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                  GTK_MESSAGE_WARNING,
	                                  GTK_BUTTONS_NONE,
	                                  title,
	                                  NULL
	                                  );

		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
		    _("_Cancel"), GTK_RESPONSE_CANCEL,
			actionverb, GTK_RESPONSE_OK,
			NULL);

	if(secondtext)
	{
		g_object_set(GTK_MESSAGE_DIALOG (dialog), "secondary-text", secondtext, NULL);
		//gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), secondtext);
	}

	//5.6 style button
	gchar *style = destructive ? GTK_STYLE_CLASS_DESTRUCTIVE_ACTION : GTK_STYLE_CLASS_SUGGESTED_ACTION;
	my_ui_dialog_add_action_class(GTK_DIALOG(dialog), GTK_RESPONSE_OK, style);

	gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
	
	retval = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	return retval;
}




/* Message dialog */




gint ui_dialog_msg_question(GtkWindow *parent, gchar *title, gchar *message_format, ...)
{
GtkWidget *dialog;
gchar* msg = NULL;
va_list args;
gint retval;

	dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
	                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                  GTK_MESSAGE_QUESTION,
	                                  GTK_BUTTONS_YES_NO,
	                                  title,
	                                  NULL
	                                  );

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", msg);

      g_free (msg);
    }

	gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_NO);
	
	retval = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	return retval;
}

/*
** open a info/error dialog for user information purpose
*/
void ui_dialog_msg_infoerror(GtkWindow *parent, GtkMessageType type, gchar *title, gchar *message_format, ...)
{
GtkWidget *dialog;
gchar* msg = NULL;
va_list args;


	dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
	                                  GTK_DIALOG_DESTROY_WITH_PARENT,
	                                  type,
	                                  GTK_BUTTONS_OK,
	                                  "%s",
	                                  title
	                                  );

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", msg);

      g_free (msg);
    }

	 gtk_dialog_run (GTK_DIALOG (dialog));
	 gtk_widget_destroy (dialog);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


void ui_dialog_file_statistics(void)
{
GtkWidget *dialog, *content_area, *box, *group_grid;
GtkWidget *label, *widget;
//gchar *tmpstr;
gint row, count, count2;

	dialog = gtk_dialog_new_with_buttons (_("File statistics"),
		GTK_WINDOW (GLOBALS->mainwindow),
		0,
		_("_Close"),
		GTK_RESPONSE_ACCEPT,
		NULL);

	gtk_window_set_default_size (GTK_WINDOW(dialog), HB_MINWIDTH_LIST, -1);
	
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_container_set_border_width (GTK_CONTAINER(box), SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (content_area), box, FALSE, FALSE, 0);

	// group :: file title
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (box), group_grid, FALSE, FALSE, 0);

	row = 1;
	label = make_label_widget(_("Account"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_acc_length ();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Transaction"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_transaction_length();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	da_archive_stats(&count, &count2);

	row++;
	label = make_label_widget(_("Scheduled"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	ui_label_set_integer(GTK_LABEL(widget), count2);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Template"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Assignment"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_asg_length ();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	widget = gtk_separator_new(	GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

	// group :: file title
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (box), group_grid, FALSE, FALSE, 0);

	row = 1;
	label = make_label_widget(_("Payee"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_pay_length ();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Category"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_cat_length ();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Tag"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_tag_length ();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Currency"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label(NULL, 1.0, 0.5);
	count = da_cur_length ();
	ui_label_set_integer(GTK_LABEL(widget), count);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	
	gtk_widget_show_all(box);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if(result == GTK_RESPONSE_ACCEPT)
	{

	}

	// cleanup and destroy
	gtk_widget_destroy (dialog);

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

struct dialog_currency_data
{
	GtkWidget   *dialog;
	GtkWidget   *LB_currency;
	GtkWidget   *BT_change;
	Currency4217 *curfmt;
};

static void ui_dialog_upgrade_choose_currency_change_action(GtkWidget *widget, gpointer user_data)
{
struct dialog_currency_data *data = user_data;
struct curSelectContext selectCtx;

	data->curfmt = NULL;

	ui_cur_select_dialog_new(GTK_WINDOW(data->dialog), CUR_SELECT_MODE_BASE, &selectCtx);
	if( selectCtx.cur_4217 != NULL )
	{
	Currency4217 *curfmt;
	gchar label[128];
	gchar *name;
	
		curfmt = selectCtx.cur_4217;	
		
		DB( g_printf("- user selected: '%s' '%s'\n", curfmt->curr_iso_code, curfmt->name) );

		data->curfmt = curfmt;

		name = curfmt->name;

		g_snprintf(label, 127, "%s - %s", curfmt->curr_iso_code, name);
		gtk_label_set_text (GTK_LABEL(data->LB_currency), label);
	}
}


static void ui_dialog_upgrade_choose_currency_fill(struct dialog_currency_data *data)
{
Currency *cur;
gchar label[128];

	data->curfmt = NULL;

	cur = da_cur_get (GLOBALS->kcur);

	g_snprintf(label, 127, "%s - %s", cur->iso_code, cur->name);
	gtk_label_set_text (GTK_LABEL(data->LB_currency), label);
}



void ui_dialog_upgrade_choose_currency(void)
{
struct dialog_currency_data *data;
GtkWidget *dialog, *content_area, *content_grid, *group_grid;
GtkWidget *label, *widget;
gint crow, row;

	data = g_malloc0(sizeof(struct dialog_currency_data));
	if(!data) return;

	dialog = gtk_dialog_new_with_buttons (_("Upgrade"),
		GTK_WINDOW (GLOBALS->mainwindow),
		0,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_OK"), GTK_RESPONSE_ACCEPT,
		NULL);

	data->dialog = dialog;

	widget = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_window_set_focus(GTK_WINDOW(dialog), widget);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER(content_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content_area), content_grid, TRUE, TRUE, 0);

	crow = 0;
	// group :: file title
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);

	row = 0;
	label = make_label(_("Select a base currency"), 0, 0);
	gimp_label_set_attributes(GTK_LABEL(label), 
		PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, 
		PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE, 
		-1);
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 3, 1);

	row++;
	label = make_label(
		_("Starting v5.1, HomeBank can manage several currencies\n" \
		  "if the currency below is not correct, please change it:"), 0, 0);
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 3, 1);

	row++;
	label = make_label_widget(_("Currency:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_label (NULL, 0, 0.5);
	data->LB_currency = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	widget = gtk_button_new_with_mnemonic (_("_Change"));
	data->BT_change = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	g_signal_connect (G_OBJECT (data->BT_change), "clicked", G_CALLBACK (ui_dialog_upgrade_choose_currency_change_action), data);


	ui_dialog_upgrade_choose_currency_fill(data);

	gtk_widget_show_all(content_grid);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if(result == GTK_RESPONSE_ACCEPT)
	{

		if( data->curfmt != NULL )
		{
			hbfile_replace_basecurrency(data->curfmt);
		}
	}

	// in any case set every accounts to base currency
	GList *list;
	list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *acc = list->data;

		account_set_currency(acc, GLOBALS->kcur);
		list = g_list_next(list);
	}
	g_list_free(list);
	
	// cleanup and destroy
	gtk_widget_destroy (dialog);

	g_free(data);

	// make sure dialog is gone
	while (gtk_events_pending ())
		gtk_main_iteration ();

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */




static GtkFileFilter *ui_file_chooser_add_filter(GtkFileChooser *chooser, gchar *name, gchar *pattern)
{
	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, name);
	gtk_file_filter_add_pattern (filter, pattern);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(chooser), filter);

	return filter;
}


gboolean ui_file_chooser_qif(GtkWindow *parent, gchar **storage_ptr)
{
GtkWidget *chooser;
gboolean retval;

	DB( g_print("(homebank) chooser save qif\n") );

	chooser = gtk_file_chooser_dialog_new (
					_("Export as QIF"),
					GTK_WINDOW(parent),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					_("_Cancel"), GTK_RESPONSE_CANCEL,
					_("_Save"), GTK_RESPONSE_ACCEPT,
					NULL);

	//todo: change this ?
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(chooser), PREFS->path_export);
	ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("QIF files"), "*.[Qq][Ii][Ff]");
	ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("All files"), "*");

	retval = FALSE;
	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
	{
	gchar *tmpfilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
	
		*storage_ptr = hb_filename_new_with_extension(tmpfilename, "qif");
		g_free(tmpfilename);
		retval = TRUE;
	}

	gtk_widget_destroy (chooser);

	return retval;
}



/*
** open a file chooser dialog and store filename to GLOBALS if OK
*/
gboolean ui_file_chooser_csv(GtkWindow *parent, GtkFileChooserAction action, gchar **storage_ptr, gchar *name)
{
GtkWidget *chooser;
gchar *title;
gchar *button;
gboolean retval;
gchar *path;

	DB( g_print("(hombank) csvfile chooser csv %d\n", action) );

	if( action == GTK_FILE_CHOOSER_ACTION_OPEN )
	{
		title = _("Import from CSV");
		button = _("_Open");
		path = PREFS->path_import;
	}
	else
	{
		title = _("Export as CSV");
		button = _("_Save");
		path = PREFS->path_export;
	}

	chooser = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW(parent),
					action,	//GTK_FILE_CHOOSER_ACTION_OPEN,
					_("_Cancel"), GTK_RESPONSE_CANCEL,
					button, GTK_RESPONSE_ACCEPT,
					NULL);

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(chooser), path);

	if(name != NULL)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), name);

	ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("CSV files"), "*.[Cc][Ss][Vv]");
	ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("All files"), "*");

	retval = FALSE;
	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
	{
	gchar *tmpfilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
	
		if( action == GTK_FILE_CHOOSER_ACTION_SAVE )
		{
			*storage_ptr = hb_filename_new_with_extension(tmpfilename, "csv");
			g_free(tmpfilename);
		}
		else
		{
			*storage_ptr = tmpfilename;
		}
		retval = TRUE;
	}

	gtk_widget_destroy (chooser);

	return retval;
}

/*
** open a file chooser dialog and store filename to GLOBALS if OK
*/
gboolean ui_file_chooser_xhb(GtkFileChooserAction action, gchar **storage_ptr, gboolean bakmode)
{
GtkWidget *chooser;
gchar *title;
gchar *button;
gboolean retval;

	DB( g_print("(ui-dialog) file chooser xhb %d\n", action) );

	if( action == GTK_FILE_CHOOSER_ACTION_OPEN )
	{
		title = (bakmode==FALSE) ? _("Open HomeBank file") : _("Open HomeBank backup file");
		button = _("_Open");
	}
	else
	{
		title = _("Save HomeBank file as");
		button = _("_Save");
	}

	chooser = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW(GLOBALS->mainwindow),
					action,	//GTK_FILE_CHOOSER_ACTION_OPEN,
					_("_Cancel"), GTK_RESPONSE_CANCEL,
					button, GTK_RESPONSE_ACCEPT,
					NULL);

	if( action == GTK_FILE_CHOOSER_ACTION_OPEN )
	{
		if( bakmode == FALSE )
		{
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(chooser), PREFS->path_hbfile);
			ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("HomeBank files"), "*.[Xx][Hh][Bb]");
			ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("All files"), "*");
		}
		else
		{
		gchar *pattern;
		GtkFileFilter *flt;

			//#1864176 open backup should open the backup folder
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(chooser), PREFS->path_hbbak);

			pattern = hb_filename_backup_get_filtername(GLOBALS->xhb_filepath);
			flt = ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("File backup"), pattern);
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), flt);
			g_free(pattern);

			ui_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), _("All backups"), "*.[Bb][Aa][Kk]");
		}
	}
	else /* GTK_FILE_CHOOSER_ACTION_SAVE */
	{
	gchar *basename, *dirname;
		
		basename = g_path_get_basename(GLOBALS->xhb_filepath);
		dirname  = g_path_get_dirname (GLOBALS->xhb_filepath);
		//gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(chooser), GLOBALS->xhb_filepath);

		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(chooser), dirname);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), basename);
		
		g_free(dirname);
		g_free(basename);	
	}

	retval = FALSE;
	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
	{
		*storage_ptr = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		retval = TRUE;
	}

	gtk_widget_destroy (chooser);

	return retval;
}


/*
**
*/
gboolean ui_file_chooser_folder(GtkWindow *parent, gchar *title, gchar **storage_ptr)
{
GtkWidget *chooser;
gboolean retval;

	DB( g_print("(ui-dialog) folder chooser\n") );

	chooser = gtk_file_chooser_dialog_new (title,
					parent,
					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					_("_Cancel"), GTK_RESPONSE_CANCEL,
					_("_Open"), GTK_RESPONSE_ACCEPT,
					NULL);

	DB( g_print(" - set folder %s\n", *storage_ptr) );

	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(chooser), *storage_ptr);

	retval = FALSE;
	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
	{
    gchar *filename;

		//nb: filename must be freed with g_free
	    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

		DB( g_print("- folder %s\n", filename) );

		//todo: dangerous to do this here, review please !
		g_free(*storage_ptr);
		*storage_ptr = filename;

		DB( g_print("- folder stored: %s\n", *storage_ptr) );


		retval = TRUE;
	}

	gtk_widget_destroy (chooser);

	return retval;
}



/*
** request the user to save last change
*/
gboolean ui_dialog_msg_savechanges(GtkWidget *widget, gpointer user_data)
{
gboolean retval = TRUE;
GtkWidget *dialog;

  	if(GLOBALS->changes_count)
	{
	gint result;

		dialog = gtk_message_dialog_new
		(
			GTK_WINDOW(GLOBALS->mainwindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING,
			//GTK_MESSAGE_INFO,
			GTK_BUTTONS_NONE,
			_("Save changes to the file before closing?")
		);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("If you don't save, changes will be permanently lost.\nNumber of changes: %d."),
			GLOBALS->changes_count
			);

		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
		    _("Close _without saving"), 0,
		    _("_Cancel"), 1,
			_("_Save"), 2,
			NULL);

		//5.6 style button
		my_ui_dialog_add_action_class(GTK_DIALOG(dialog), 0, GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

		gtk_dialog_set_default_response(GTK_DIALOG( dialog ), 2);

		result = gtk_dialog_run( GTK_DIALOG( dialog ) );
		gtk_widget_destroy( dialog );

		if(result == 1 || result == GTK_RESPONSE_DELETE_EVENT)
		{
			retval = FALSE;
		}
		else
		{
			if(result == 2)
			{
				DB( g_print(" + should quick save %s\n", GLOBALS->xhb_filepath) );
				//todo: should migrate this
				//#1720377 also backup 
				homebank_file_ensure_xhb(NULL);
				homebank_backup_current_file();
				homebank_save_xml(GLOBALS->xhb_filepath);
			}
		}

	}
	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


struct dialog_export_csv_data
{
	gboolean  showall;
	GtkWidget *dialog;
	GtkWidget *CM_split, *CM_status;
	GtkWidget *IM_warn, *LB_warn;
};


static void ui_dialog_export_csv_update(GtkWidget *widget, gpointer user_data)
{
struct dialog_export_csv_data *data = user_data;
gboolean hassplit, hasstatus, visible;

	hassplit  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(data->CM_split));
	hasstatus = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(data->CM_status));

	visible = data->showall | hassplit | hasstatus;
	hb_widget_visible(data->IM_warn, visible);
	hb_widget_visible(data->LB_warn, visible);
}


gint ui_dialog_export_csv(GtkWindow *parent, gchar **storage_ptr, gboolean *split_ptr, gboolean *status_ptr, gboolean showall)
{
struct dialog_export_csv_data *data;
GtkWidget *dialog, *content_area, *content_grid, *group_grid;
GtkWidget *label, *widget, *BT_folder, *ST_name;
gchar *tmpstr;
gint crow, row;


	data = g_malloc0(sizeof(struct dialog_export_csv_data));
	if(!data) return GTK_RESPONSE_CANCEL;

	data->showall = showall;
	
	dialog = gtk_dialog_new_with_buttons (_("Export as CSV"),
		GTK_WINDOW (parent),
		0,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("Export as _CSV"), GTK_RESPONSE_ACCEPT,
		NULL);

	gtk_window_set_default_size (GTK_WINDOW(dialog), HB_MINWIDTH_LIST, -1);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER(content_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content_area), content_grid, TRUE, TRUE, 0);

	crow = 0;
	// group :: file title
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);

	row = 0;

	row++;
	label = make_label_widget(_("Folder:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	BT_folder = gtk_file_chooser_button_new (_("Pick a Folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_grid_attach (GTK_GRID (group_grid), BT_folder, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Filename:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	ST_name = make_string (label);
	gtk_widget_set_hexpand(ST_name, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), ST_name, 2, row, 1, 1);

	row++;
	label = make_label_group(_("Options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 3, 1);

	row++;
	data->CM_status = gtk_check_button_new_with_mnemonic (_("Add Status column"));
	gtk_grid_attach (GTK_GRID (group_grid), data->CM_status, 1, row, 2, 1);
	
	row++;
	data->CM_split = gtk_check_button_new_with_mnemonic (_("Detail split lines"));
	gtk_grid_attach (GTK_GRID (group_grid), data->CM_split, 1, row, 2, 1);

	//warning text
	row++;
	widget = gtk_image_new_from_icon_name (ICONNAME_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	gtk_widget_set_margin_top (widget, SPACING_MEDIUM);
	data->IM_warn = widget;
	//                       123456789012345678901234567890123456789012345678901234567890
	label = gtk_label_new(_("The file will not be in HomeBank CSV format, because you export\n" \
							"from 'All transaction', or you selected an option."));
	data->LB_warn = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 2, row, 1, 1);

	/* signals */
	g_signal_connect (data->CM_split  , "toggled", G_CALLBACK (ui_dialog_export_csv_update), data);
	g_signal_connect (data->CM_status , "toggled", G_CALLBACK (ui_dialog_export_csv_update), data);
	

	//setup
	tmpstr = g_path_get_dirname(*storage_ptr);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(BT_folder), tmpstr);
	g_free(tmpstr);
	
	tmpstr = g_path_get_basename(*storage_ptr);
	gtk_entry_set_text(GTK_ENTRY(ST_name), tmpstr);
	g_free(tmpstr);


	gtk_widget_show_all(content_grid);
	ui_dialog_export_csv_update(dialog, data);
	
	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if(result == GTK_RESPONSE_ACCEPT)
	{
	gchar *hostname;
	//#300380 fixed export path problem (was always the export of preference)
	//not to be used -- gchar *nufolder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(BT_folder));
	gchar *urifolder  = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(BT_folder));
	gchar *nufolder   = g_filename_from_uri(urifolder, &hostname, NULL);
	gchar *nufilename = hb_filename_new_with_extension((gchar *)gtk_entry_get_text (GTK_ENTRY(ST_name)), "csv");	

		g_free(*storage_ptr);
		*storage_ptr = g_build_filename(nufolder, nufilename, NULL);

		g_free(nufilename);
		g_free(nufolder);
		g_free(urifolder);
		
		if( split_ptr != NULL )
		{
			*split_ptr = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(data->CM_split));
		}

		if( status_ptr != NULL )
		{
			*status_ptr = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(data->CM_status));
		}
		
	}

	// cleanup and destroy
	gtk_widget_destroy (dialog);

	g_free(data);
	
	return result;
}


gint ui_dialog_export_pdf(GtkWindow *parent, gchar **storage_ptr)
{
GtkWidget *dialog, *content_area, *content_grid, *group_grid;
GtkWidget *label, *widget, *BT_folder, *ST_name;
gchar *tmpstr;
gint crow, row;

	dialog = gtk_dialog_new_with_buttons (_("Export as PDF"),
		GTK_WINDOW (parent),
		0,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("Export as _PDF"), GTK_RESPONSE_ACCEPT,
		NULL);

	gtk_window_set_default_size (GTK_WINDOW(dialog), HB_MINWIDTH_LIST, -1);
	
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER(content_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content_area), content_grid, TRUE, TRUE, 0);

	crow = 0;
	// group :: file title
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow++, 1, 1);

	row = 0;
	/*
	widget = gtk_image_new_from_icon_name (ICONNAME_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 0, row, 1, 1);
	label = gtk_label_new("This feature is still in development state,\n(maybe not stable) so use it at your own risk!");
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	*/

	row++;
	label = make_label_widget(_("Folder:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	BT_folder = gtk_file_chooser_button_new (_("Pick a Folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_grid_attach (GTK_GRID (group_grid), BT_folder, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("Filename:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	ST_name = make_string (label);
	gtk_widget_set_hexpand(ST_name, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), ST_name, 1, row, 1, 1);

	row++;
	row++;
	widget = gtk_image_new_from_icon_name (ICONNAME_INFO, GTK_ICON_SIZE_DIALOG);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 0, row, 1, 1);
	//                       123456789012345678901234567890123456789012345678901234567890
	label = gtk_label_new(_("With HomeBank, printing is oriented towards an eco-responsible\n" \
							"attitude towards the most widespread digital format: PDF format. "));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);


	//setup
	tmpstr = g_path_get_dirname(*storage_ptr);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(BT_folder), tmpstr);
	g_free(tmpstr);
	
	tmpstr = g_path_get_basename(*storage_ptr);
	gtk_entry_set_text(GTK_ENTRY(ST_name), tmpstr);
	g_free(tmpstr);


	gtk_widget_show_all(content_grid);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if(result == GTK_RESPONSE_ACCEPT)
	{
	gchar *hostname;
	//#300380 fixed export path problem (was always the export of preference)
	//not to be used -- gchar *nufolder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(BT_folder));
	gchar *urifolder  = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(BT_folder));
	gchar *nufolder   = g_filename_from_uri(urifolder, &hostname, NULL);
	gchar *nufilename = hb_filename_new_with_extension((gchar *)gtk_entry_get_text (GTK_ENTRY(ST_name)), "pdf");	

		g_free(*storage_ptr);
		*storage_ptr = g_build_filename(nufolder, nufilename, NULL);

		g_free(nufilename);
		g_free(nufolder);
		g_free(urifolder);
	}

	// cleanup and destroy
	gtk_widget_destroy (dialog);

	return result;
}





/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

struct xfer_data
{
	GtkWidget   *dialog;
	GtkWidget	*srctreeview;
	GtkWidget	*lb_several;
	GtkWidget	*treeview;
};


//#1982036 make releavant column to be displayed
static gint lst_xfer_columns[NUM_LST_DSPOPE+1] = {
	LST_DSPOPE_STATUS,
	LST_DSPOPE_MATCH,
	LST_DSPOPE_ACCOUNT,
	LST_DSPOPE_DATE,
	LST_DSPOPE_AMOUNT,
	LST_DSPOPE_CLR,
	LST_DSPOPE_MEMO,
	LST_DSPOPE_INFO,
	LST_DSPOPE_PAYEE,
	LST_DSPOPE_CATEGORY,
	LST_DSPOPE_TAGS,
	-LST_DSPOPE_EXPENSE,
	-LST_DSPOPE_INCOME,
	-LST_DSPOPE_BALANCE
};


static void ui_dialog_transaction_xfer_select_child_cb(GtkWidget *widget, gpointer user_data)
{
struct xfer_data *data;
GtkTreeSelection *selection;
gboolean sensitive;
gint count;

	DB( g_print("\n(xfer select) toggle choice\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->treeview));
	count = gtk_tree_selection_count_selected_rows(selection);

	sensitive = (count > 0) ? TRUE : FALSE;
	gtk_dialog_set_response_sensitive(GTK_DIALOG(data->dialog), HB_RESPONSE_SELECTION, sensitive);

	DB( g_print(" test count %d sensitive %d\n", count, sensitive) );

}

static void ui_dialog_transaction_xfer_select_child_selection_cb(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_dialog_transaction_xfer_select_child_cb(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


gint ui_dialog_transaction_xfer_select_child(GtkWindow *parent, Transaction *stxn, GList *matchlist, Transaction **child)
{
struct xfer_data *data;
GtkWidget *dialog, *content, *mainvbox, *sw, *label;
GtkTreeModel *newmodel;
GtkTreeIter newiter;

	DB( g_print("\n(xfer select) new\n") );


	data = g_malloc0(sizeof(struct xfer_data));
	if(!data) return 0;
	
	dialog = gtk_dialog_new_with_buttons (
    			_("Select action for target creation"),
    			parent,
			    0,
			    _("_Cancel"), GTK_RESPONSE_CANCEL,
			    _("Create _New"), HB_RESPONSE_CREATE_NEW,
			    _("Use _Selection"), HB_RESPONSE_SELECTION,
			    NULL);

	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	data->dialog = dialog;

	//5/6 set a nice dialog size
/*
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 3:2
	dw = (dh * 3) / 2;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);
*/

	//hide close button
	//gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);

	//gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	//gtk_window_set_default_size (GTK_WINDOW (dialog), 800, 494);

	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (content), mainvbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (mainvbox), SPACING_LARGE);

	label = make_label_group(_("Source transfer"));
	gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	// source listview
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
//	gtk_widget_set_size_request(sw, -1, HB_MINWIDTH_LIST/2);
	gtk_widget_set_margin_left(sw, SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (mainvbox), sw, FALSE, FALSE, 0);

	data->srctreeview = create_list_transaction(LIST_TXN_TYPE_XFERSOURCE, lst_xfer_columns);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->srctreeview)), GTK_SELECTION_NONE);
	gtk_container_add (GTK_CONTAINER (sw), data->srctreeview);


	// target listview
	label = make_label_group(_("Target association suggested"));
	gtk_widget_set_margin_top(label, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	label = make_label(_(
		"HomeBank has found some transaction that may be " \
		"the associated transaction for the internal transfer."), 0.0, 0.5
		);
	data->lb_several = label;
	gtk_widget_set_margin_left(label, SPACING_MEDIUM);
	/*gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);*/
	gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(sw, -1, HB_MINWIDTH_LIST*1.5);
	gtk_widget_set_margin_left(sw, SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (mainvbox), sw, TRUE, TRUE, 0);

	data->treeview = create_list_transaction(LIST_TXN_TYPE_XFERTARGET, lst_xfer_columns);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->treeview)), GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (sw), data->treeview);

	DB( g_print(" populate src\n") );

	/* populate source */
	if( stxn != NULL )
	{
		newmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(data->srctreeview));
		gtk_tree_store_clear (GTK_TREE_STORE(newmodel));
	
		gtk_tree_store_append (GTK_TREE_STORE(newmodel), &newiter, NULL);

		//#1830523/#1840393
		gtk_tree_store_set (GTK_TREE_STORE(newmodel), &newiter,
			MODEL_TXN_POINTER, stxn,
			MODEL_TXN_SPLITAMT, stxn->amount,
			-1);
	}

	DB( g_print(" populate src\n") );

	/* populate target */
	newmodel = gtk_tree_view_get_model(GTK_TREE_VIEW(data->treeview));
	gtk_tree_store_clear (GTK_TREE_STORE(newmodel));

	GList *tmplist = g_list_first(matchlist);
	while (tmplist != NULL)
	{
	Transaction *tmp = tmplist->data;

		/* append to our treeview */
			gtk_tree_store_append (GTK_TREE_STORE(newmodel), &newiter, NULL);

			//#1830523/#1840393
			gtk_tree_store_set (GTK_TREE_STORE(newmodel), &newiter,
			MODEL_TXN_POINTER, tmp,
			MODEL_TXN_SPLITAMT, tmp->amount,
			-1);

		//DB( g_print(" - fill: %s %.2f %x\n", item->memo, item->amount, (unsigned int)item->same) );

		tmplist = g_list_next(tmplist);
	}


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->treeview)), "changed", G_CALLBACK (ui_dialog_transaction_xfer_select_child_selection_cb), NULL);

	gtk_widget_show_all(mainvbox);

	//#1982036 show essential field on the left : no need to leave space
	DB( g_print(" autosize\n") );

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(data->srctreeview));
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(data->treeview));

	//5.5.7: add sort by match descending
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(newmodel), LST_DSPOPE_MATCH, GTK_SORT_DESCENDING);

	//initialize
	gtk_dialog_set_response_sensitive(GTK_DIALOG(data->dialog), HB_RESPONSE_SELECTION, FALSE);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	*child = NULL;
	if(result == HB_RESPONSE_SELECTION)
	{
	GtkTreeSelection *selection;
	GtkTreeModel		 *model;
	GtkTreeIter			 iter;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, child, -1);
		}
	}

	DB( g_print(" return %d child = %p\n", result, child) );

	
	// cleanup and destroy
	gtk_widget_destroy (dialog);

	g_free(data);
	
	return result;
}



