/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2024 Maxime DOYEN
 *
 *	This file is part of HomeBank.
 *
 *	HomeBank is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	HomeBank is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */

#include "homebank.h"

#include "ui-assist-start.h"
#include "dsp-mainwindow.h"
#include "hub-account.h"
#include "ui-currency.h"


#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;

extern HbKvData CYA_ACC_TYPE[];

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static GtkWidget *
ui_newfile_page_intro_create (GtkWidget *assistant, struct assist_start_data *data)
{
GtkWidget *mainbox, *label;

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	//gtk_widget_set_halign(mainbox, GTK_ALIGN_CENTER);
	//gtk_widget_set_valign(mainbox, GTK_ALIGN_CENTER);

	//assistant box is 12, so add 6
	hb_widget_set_margin(mainbox, SPACING_SMALL);

	label = make_label(
	      _("This assistant will help you setup a minimum configuration\n" \
	        "for a new HomeBank file."), 0, 0);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, SPACING_SMALL);

	label = make_label(
	      _("All the elements you setup here can be changed later if required."), 0, 0);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, SPACING_SMALL);

	
	label = make_label(
	    _("No changes will be made until you click \"Apply\"\n" \
	      "at the end of this assistant."), 0., 0.0);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, SPACING_SMALL);


	gtk_widget_show_all (mainbox);

	return mainbox;
}


/* = = = = = = = = = = = = = = = = */


static void
ui_newfile_entry_changed (GtkWidget *widget, gpointer data)
{
GtkAssistant *assistant = GTK_ASSISTANT (data);
GtkWidget *current_page;
gint page_number;
gchar *text;

	page_number = gtk_assistant_get_current_page (assistant);
	current_page = gtk_assistant_get_nth_page (assistant, page_number);
	//#1837838: complete space or leadin/trialin space is possible
	text = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));
	g_strstrip(text);
	
	if (strlen(text) > 0)
		gtk_assistant_set_page_complete (assistant, current_page, TRUE);
	else
		gtk_assistant_set_page_complete (assistant, current_page, FALSE);

	g_free(text);
}


static void
ui_newfile_page_general_fill (GtkWidget *assistant, struct assist_start_data *data)
{
Currency *cur;
gchar label[128];

	DB( g_print("\n[ui-start] property_fill\n") );


	gtk_entry_set_text(GTK_ENTRY(data->ST_owner), g_get_real_name ());

	cur = da_cur_get (GLOBALS->kcur);

	g_snprintf(label, 127, "%s (%s)", cur->iso_code, cur->name);
	gtk_label_set_text (GTK_LABEL(data->LB_cur_base), label);
	
}


static GtkWidget *
ui_newfile_page_general_create (GtkWidget *assistant, struct assist_start_data *data)
{
GtkWidget *mainbox, *hbox, *label, *widget;

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	//gtk_widget_set_halign(mainbox, GTK_ALIGN_CENTER);
	//gtk_widget_set_valign(mainbox, GTK_ALIGN_CENTER);

	//assistant box is 12, so add 6
	hb_widget_set_margin(mainbox, SPACING_SMALL);

	//                    123456789012345678901234567890123456789012345678901234567890
	label = make_label(_("HomeBank will display a title for the main window,\n" \
	                     "it can be a free label or your name."), 0, 0.5);
	gtk_widget_set_margin_bottom(label, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, 0);

	
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	//gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (mainbox), hbox, FALSE, FALSE, 0);
	
	label = make_label_widget(_("_Title:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	widget = make_string(label);
	data->ST_owner = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	//TODO: later we will let the user choose to use: 
	//budget
	//cleared/validated ...
	//life energy
	
	g_signal_connect (G_OBJECT (data->ST_owner), "changed", G_CALLBACK (ui_newfile_entry_changed), assistant);
	
	gtk_widget_show_all (mainbox);

	return mainbox;
}


/* = = = = = = = = = = = = = = = = */

static void ui_newfile_page_currency_add_action(GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data;
struct curSelectContext selectCtx;
gint result;

	DB( g_print("\n[ui-start] property_add_action\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	result = ui_cur_select_dialog_new(GTK_WINDOW(data->dialog), CUR_SELECT_MODE_NOCUSTOM, &selectCtx);
	if( result == GTK_RESPONSE_ACCEPT )
	{
		if( selectCtx.cur_4217 != NULL )
		{
		Currency4217 *curfmt = selectCtx.cur_4217;

			DB( g_print(" adding '%s' (%s)\n", curfmt->curr_iso_code, curfmt->name) );
			g_ptr_array_add(data->cur_arr, curfmt);

			//TODO: refresh the label
			GString *strcur = g_string_new(NULL);
			for(guint i=0;i<data->cur_arr->len;i++)
			{
			Currency4217 *elt = g_ptr_array_index(data->cur_arr, i);

				g_string_append_printf(strcur, "%s (%s)\r\n", elt->curr_iso_code, elt->name);
			}

			gtk_label_set_text (GTK_LABEL(data->LB_cur_others), strcur->str);

			g_string_free(strcur, TRUE);
		}
	}
}

static void ui_newfile_page_currency_change_action(GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data;
struct curSelectContext selectCtx;
	
	DB( g_print("\n[ui-start] property_change_action\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

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

		g_snprintf(label, 127, "%s (%s)", curfmt->curr_iso_code, name);
		gtk_label_set_text (GTK_LABEL(data->LB_cur_base), label);
	}
}


static void
ui_newfile_page_currency_cb_toggle(GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data;
gboolean sensitive;
	
	DB( g_print("\n[ui-start] property_cb_toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	
	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cur_add));

	gtk_widget_set_sensitive(data->LB_cur_others, sensitive);
	gtk_widget_set_sensitive(data->BT_cur_add, sensitive);
}



static GtkWidget *
ui_newfile_page_currency_create (GtkWidget *assistant, struct assist_start_data *data)
{
GtkWidget *mainbox, *hbox, *label, *widget;
GtkWidget *scrollwin;

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	//assistant box is 12, so add 6
	hb_widget_set_margin(mainbox, SPACING_SMALL);

	//                    123456789012345678901234567890123456789012345678901234567890
	label = make_label(_("HomeBank support multiple currencies. The base currency is\n" \
	                     "the default for new accounts and reports."), 0, 0.5);
	gtk_widget_set_margin_bottom(label, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (mainbox), hbox, FALSE, FALSE, 0);

	label = make_label_widget(_("Base:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	widget = make_label (NULL, 0, 0.5);
	data->LB_cur_base = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	widget = gtk_button_new_with_mnemonic (_("_Change"));
	data->BT_cur_change = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);


	widget = gtk_check_button_new_with_mnemonic (_("Setup additional currencies"));
	data->CM_cur_add = widget;
	gtk_widget_set_margin_top(widget, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainbox), widget, FALSE, FALSE, 0);

	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (mainbox), scrollwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), 0.75*HB_MINHEIGHT_LIST);
	widget = make_label(NULL, 0.0, 0.0);
	data->LB_cur_others = widget;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);

	widget = gtk_button_new_with_mnemonic (_("_Add"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	data->BT_cur_add = widget;
	gtk_box_pack_start (GTK_BOX (mainbox), widget, FALSE, FALSE, 0);

	gtk_widget_set_sensitive(data->LB_cur_others, FALSE);
	gtk_widget_set_sensitive(data->BT_cur_add, FALSE);
	
	g_signal_connect (G_OBJECT (data->BT_cur_change), "clicked", G_CALLBACK (ui_newfile_page_currency_change_action), data);
	g_signal_connect (G_OBJECT (data->CM_cur_add), "toggled", G_CALLBACK (ui_newfile_page_currency_cb_toggle), data);
	g_signal_connect (G_OBJECT (data->BT_cur_add), "clicked", G_CALLBACK (ui_newfile_page_currency_add_action), data);
	
	gtk_widget_show_all (mainbox);

	return mainbox;
}


/* = = = = = = = = = = = = = = = = */


static void
hb_string_clean_csv_category(gchar *str)
{
gchar *s = str;
gchar *d = str;

	if(str)
	{
		while( *s )
		{
			if( !(*s==';' || *s=='1' || *s=='-' || *s=='+' || *s=='2') )
			{
				*d++ = *s;
			}
			if( *s=='2' )
			{
				*d++ = ' ';
				*d++ = '-';
				*d++ = ' ';
			}
			s++;
		}
		*d = 0;
	}

}


static void
ui_newfile_page_categories_cb_toggle(GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data;
gboolean sensitive;
	
	DB( g_print("\n[ui-start] categories_cb_toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	
	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_load));

	gtk_widget_set_sensitive(data->GR_file, sensitive);
	gtk_widget_set_sensitive(data->TX_preview, sensitive);
}


static void
ui_newfile_page_categories_fill (GtkWidget *assistant, struct assist_start_data *data)
{
gchar *lang;
gchar *content;

	data->pathfilename = category_find_preset(&lang);

	//test no file
	//data->pathfilename = NULL;
		
	if(data->pathfilename != NULL)
	{
		gtk_label_set_label(GTK_LABEL(data->TX_file), lang);
		gtk_widget_show(data->CM_load);
		gtk_widget_show(data->ok_image);
		gtk_widget_hide(data->ko_image);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_load), TRUE);
		
		/* preview */
		if( g_file_get_contents(data->pathfilename, &content, NULL, NULL) )
		{
			hb_string_clean_csv_category(content);

			gtk_label_set_label(GTK_LABEL(data->TX_preview), content);

			g_free(content);
		}
	}
	else
	{
		gtk_widget_hide(data->CM_load);
		gtk_label_set_label(GTK_LABEL(data->TX_file), _("Not found"));
		gtk_widget_show(data->ko_image);
		gtk_widget_hide(data->ok_image);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_load), FALSE);
	}
}


static GtkWidget *
ui_newfile_page_categories_create (GtkWidget *assistant, struct assist_start_data *data)
{
GtkWidget *mainbox, *hbox, *label, *widget;
GtkWidget *scrollwin;

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	//assistant box is 12, so add 6
	hb_widget_set_margin(mainbox, SPACING_SMALL);

	//                    123456789012345678901234567890123456789012345678901234567890
	label = make_label(_("HomeBank can prefill the categories for your language\n" \
	                     "if a CSV file is available and provided by the community."), 0, 0.5);
	gtk_widget_set_margin_bottom(label, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_mnemonic (_("Setup categories for my language"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	//gtk_widget_set_margin_bottom(widget, SPACING_LARGE);
	data->CM_load = widget;
	gtk_box_pack_start (GTK_BOX (mainbox), widget, FALSE, FALSE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	data->GR_file = hbox;
	gtk_box_pack_start (GTK_BOX (mainbox), hbox, FALSE, FALSE, 0);

	label = make_label_widget(_("Preset file:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	widget = gtk_image_new_from_icon_name(ICONNAME_HB_FILE_VALID, GTK_ICON_SIZE_LARGE_TOOLBAR);
	data->ok_image = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = gtk_image_new_from_icon_name(ICONNAME_HB_FILE_INVALID, GTK_ICON_SIZE_LARGE_TOOLBAR);
	data->ko_image = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = make_label(NULL, 0.0, 0.5);
	data->TX_file = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);


	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), 0.75*HB_MINHEIGHT_LIST);
	gtk_box_pack_start (GTK_BOX (mainbox), scrollwin, TRUE, TRUE, 0);
	widget = make_label(NULL, 0.0, 0.5);
	data->TX_preview = widget;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);

	g_signal_connect (G_OBJECT (data->CM_load), "toggled", G_CALLBACK (ui_newfile_page_categories_cb_toggle), data);
	
	gtk_widget_show_all (mainbox);

	gtk_widget_hide(data->ok_image);
	gtk_widget_hide(data->ko_image);
	
	return mainbox;
}


/* = = = = = = = = = = = = = = = = */


static void
ui_newfile_page_account_cb_eval(GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data;
GtkWidget *current_page;
gint page_number;
gchar *text;
gboolean sensitive, valid;

	DB( g_print("\n[ui-start] account_cb_eval\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	page_number = gtk_assistant_get_current_page (GTK_ASSISTANT(data->dialog));

	//#1837838: complete space or leading/trailing space is possible
	text = g_strdup(gtk_entry_get_text (GTK_ENTRY (data->ST_name)));
	g_strstrip(text);
	valid = (strlen(text) > 0) ? TRUE : FALSE;
	g_free(text);
	
	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_acc_add));
	gtk_widget_set_sensitive(data->GR_acc, sensitive);

	if(!sensitive)
		valid = TRUE;
	current_page = gtk_assistant_get_nth_page (GTK_ASSISTANT(data->dialog), page_number);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(data->dialog), current_page, valid);
	
}



static GtkWidget *
ui_newfile_page_account_create (GtkWidget *assistant, struct assist_start_data *data)
{
GtkWidget *mainbox, *group_grid, *label, *widget;

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	//assistant box is 12, so add 6
	hb_widget_set_margin(mainbox, SPACING_SMALL);

	//                    123456789012345678901234567890123456789012345678901234567890
	label = make_label(_("HomeBank enables to import your accounts from downloaded\n" \
	                     "financial institution files, or you can create your account manually."), 0, 0.5);
	gtk_widget_set_margin_bottom(label, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainbox), label, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_mnemonic (_("Create my first account"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	//gtk_widget_set_margin_bottom(widget, SPACING_LARGE);
	data->CM_acc_add = widget;
	gtk_box_pack_start (GTK_BOX (mainbox), widget, FALSE, FALSE, 0);

    group_grid = gtk_grid_new ();
	data->GR_acc = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (mainbox), group_grid, TRUE, TRUE, 0);
	
	label = make_label_widget(_("_Name:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 1, 1);
	widget = make_string(label);
	gtk_widget_set_hexpand(widget, TRUE);
	data->ST_name = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, 0, 1, 1);
	
	label = make_label_widget(_("_Type:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 1, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_ACC_TYPE);
	gtk_widget_set_hexpand(widget, TRUE);
	data->CY_type = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, 1, 1, 1);

	gtk_widget_show_all (mainbox);

	g_signal_connect (G_OBJECT (data->ST_name), "changed", G_CALLBACK (ui_newfile_page_account_cb_eval), data);
	g_signal_connect (G_OBJECT (data->CM_acc_add), "toggled", G_CALLBACK (ui_newfile_page_account_cb_eval), data);
	
	return mainbox;
}


/* = = = = = = = = = = = = = = = = */


static GtkWidget *
ui_newfile_page_confirmation_create (GtkWidget *assistant, struct assist_start_data *data)
{
GtkWidget *label;

	label = gtk_label_new (_("This is a confirmation page,\n\npress 'Apply' to apply changes"));

	gtk_widget_show (label);

	return label;
}


/* = = = = = = = = = = = = = = = = */



static void
ui_newfile_assistant_prepare (GtkWidget *assistant, GtkWidget *page, gpointer user_data)
{
struct assist_start_data *data = user_data;
gint current_page;
//gint n_pages;


	DB( g_print("\n[ui-start] prepare\n") );


  current_page = gtk_assistant_get_current_page (GTK_ASSISTANT (assistant));
  //n_pages = gtk_assistant_get_n_pages (GTK_ASSISTANT (assistant));


	switch( current_page  )
	{
		case PAGE_GENERAL:
			ui_newfile_page_general_fill(assistant, data);
			break;

		case PAGE_CATEGORIES:
			ui_newfile_page_categories_fill(assistant, data);
			break;

	}
}


static void
ui_newfile_assistant_apply (GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data = user_data;
Account *item;

	DB( g_print("\n[ui-start] apply\n") );


	/* set owner */
	gchar *owner = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_owner));
	if (owner && *owner)
	{
		hbfile_change_owner(g_strdup(owner));
		GLOBALS->changes_count++;
	}

	if( data->curfmt != NULL )
	{
		hbfile_replace_basecurrency(data->curfmt);
	}

	// init other currencies
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cur_add)))
	{
	Currency *item = NULL;
	guint i;

		for(i=0;i<data->cur_arr->len;i++)
		{
		Currency4217 *curfmt = g_ptr_array_index(data->cur_arr, i);

			DB( g_printf("- curr creating: '%s' '%s'\n", curfmt->curr_iso_code, curfmt->name) );
			item = da_cur_get_by_iso_code(curfmt->curr_iso_code);
			if( item == NULL )
			{
				item = currency_add_from_user(curfmt);
			}
		}
	}
	
	/* load preset categories */
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_load)))
	{
		if(data->pathfilename != NULL)
		{
		gchar *error;
			category_load_csv(data->pathfilename, &error);
			//DB( g_print(" -> loaded=%d\n", ok) );
		}
	}

	/* initialise an account */
	// init other currencies
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_acc_add)))
	{
		item = da_acc_malloc();

		gchar *txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_name));
		if (txt && *txt)
		{
			item->name = g_strdup(txt);
			//#1837838 remove extra lead/tail space
			g_strstrip(item->name);
		}

		item->kcur = GLOBALS->kcur;
		item->type = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));

		da_acc_append(item);
		GLOBALS->changes_count++;
	}

	//our global list has changed, so update the treeview
	ui_hub_account_populate(GLOBALS->mainwindow, NULL);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));

}


static void
ui_newfile_assitant_close_cancel (GtkWidget *widget, gpointer user_data)
{
struct assist_start_data *data = user_data;

	DB( g_print("\n[ui-start] close/cancel\n") );


	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	gtk_window_destroy (GTK_WINDOW(data->dialog));

	g_free(data->pathfilename);
	g_ptr_array_free(data->cur_arr, TRUE);

	g_free(data);

}


GtkWidget*
ui_newfile_assitant_new (void)
{
struct assist_start_data *data;
GtkWidget *assistant, *page;
//gint w, h, dw, dh;

	DB( g_print("\n[ui-start] new\n") );


	data = g_malloc0(sizeof(struct assist_start_data));
	if(!data) return NULL;

	data->cur_arr = g_ptr_array_new();
	
	assistant = gtk_assistant_new ();
	data->dialog = assistant;

	//set a nice dialog size
	/*
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 2:3
	dw = (dh * 2) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(assistant), dw, dh);
	*/
	
	//store our window private data
	g_object_set_data(G_OBJECT(assistant), "inst_data", (gpointer)data);
	//DB( g_print("** (import) window=%x, inst_data=%x\n", assistant, data) );

	gtk_window_set_modal(GTK_WINDOW (assistant), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(assistant), GTK_WINDOW(GLOBALS->mainwindow));

	page = ui_newfile_page_intro_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Start File Setup"));
	gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);

	page = ui_newfile_page_general_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("File Options"));
	gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_INTRO);
	
	page = ui_newfile_page_currency_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Choose Currencies"));
	gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);
	
	page = ui_newfile_page_categories_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Choose Categories"));
	gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);
	
	page = ui_newfile_page_account_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Create Account"));
	
	page = ui_newfile_page_confirmation_create (assistant, data);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
	gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Finish File Setup"));
	
	

	g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (ui_newfile_assitant_close_cancel), data);
	g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (ui_newfile_assitant_close_cancel), data);
	g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (ui_newfile_assistant_apply), data);
	g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (ui_newfile_assistant_prepare), data);

#ifdef G_OS_WIN32
	hbtk_assistant_hack_button_order(GTK_ASSISTANT(assistant));
#endif
	
	gtk_widget_show (assistant);

	return assistant;
}
