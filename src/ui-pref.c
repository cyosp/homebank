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

#include "ui-dialogs.h"
#include "ui-widgets.h"
#include "hbtk-switcher.h"

#include "hb-pref-data.h"
#include "ui-pref.h"
#include "dsp-mainwindow.h"
#include "gtk-chart-colors.h"

#include "ui-currency.h"


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


enum {
	LST_PREF_UID,
	LST_PREF_ICONNAME,
	LST_PREF_LABEL,
	LST_PREF_PAGENUM,

	LST_PREF_MAX
};


//TODO: this is not used
//could be to save last page during session
enum
{
	PREF_GENERAL,
	PREF_INTERFACE,
		PREF_THEMING,
		PREF_COLOR,
	PREF_LOCALE,	//old DISPLAY
	PREF_TXN,		//old COLUMNS
		PREF_TXN_DIALOG,
		PREF_TXN_TRANSFER,
		PREF_PAYMODE,
	PREF_IMPORT,
	PREF_REPORT,
	PREF_FORECAST,
	PREF_BACKUP,
	PREF_FOLDERS,
	PREF_EURO,
	PREF_ADVANCED,

	PREF_MAX
};


struct pref_list_datas CYA_PREF_GROUP[PREF_MAX+1] = 
{
//	  level, identifier			 iconname			   label
	{ 1, PREF_GENERAL		,	"prf-general"  ,			N_("General") },
	{ 1, PREF_INTERFACE		,	"prf-interface",			N_("Interface") },
		{ 2, PREF_INTERFACE	,	"prf-interface-theme",		N_("Theming") },
		{ 2, PREF_INTERFACE	,	"prf-interface-color",		N_("Color") },
	{ 1, PREF_LOCALE		,	"prf-locale",				N_("Locale") },
	{ 1, PREF_TXN			,	"prf-transaction",			N_("Transactions") },
		{ 2, PREF_TXN_DIALOG,	"prf-transaction-dialog"  ,	N_("Dialog") },
		{ 2, PREF_TXN_TRANSFER,	"prf-transaction-transfer",	N_("Transfer") },
		{ 2, PREF_PAYMODE	,	"prf-transaction-payment",	N_("Payment") },
	{ 1, PREF_IMPORT		,	"prf-import"   ,			N_("Import/Export") },
	{ 1, PREF_REPORT		,	"prf-report"   ,			N_("Report") },
	{ 1, PREF_FORECAST		,	"prf-forecast" ,			N_("Forecast") },
	{ 1, PREF_BACKUP		,	"prf-backup"   ,			N_("Backup") },
	{ 1, PREF_FOLDERS		,	"prf-folder"   ,			N_("Folders") },
	{ 1, PREF_EURO			,	"prf-euro"     ,			N_("Euro minor") },
	{ 1, PREF_ADVANCED		,	"prf-advanced" ,			N_("Advanced") },

	{ 0, 0, NULL , NULL }
};



extern HbKvData CYA_TOOLBAR_STYLE[];
extern HbKvData CYA_GRID_LINES[];
extern HbKvData CYA_IMPORT_DATEORDER[];
extern HbKvData CYA_IMPORT_OFXNAME[];
extern HbKvData CYA_IMPORT_OFXMEMO[];
extern HbKvData CYA_IMPORT_CSVSEPARATOR[];
extern HbKvData CYA_CHART_COLORSCHEME[];
extern HbKvData CYA_MONTHS[];


extern EuroParams euro_params[];
extern guint nb_euro_params;
extern EuroParams euro_params_euro;
extern LangName languagenames[];



static GtkWidget *pref_list_create(void);


static gint
ui_language_combobox_compare_func (GtkTreeModel *model, GtkTreeIter  *a, GtkTreeIter  *b, gpointer      userdata)
{
gint retval = 0;
gchar *code1, *code2;
gchar *name1, *name2;

    gtk_tree_model_get(model, a, 0, &code1, 1, &name1, -1);
    gtk_tree_model_get(model, b, 0, &code2, 1, &name2, -1);

	//keep system laguage on top
	if(code1 == NULL) name1 = NULL;
	if(code2 == NULL) name2 = NULL;
	
    retval = hb_string_utf8_compare(name1, name2);

    g_free(name2);
    g_free(name1);

  	return retval;
}


static const gchar *
ui_language_combobox_get_name(const gchar *locale)
{
const gchar *lang;

	DB( g_print("[ui_language_combobox_get_name]\n") );

	// A locale directory name is typically of the form language[_territory]
	lang = languagename_get (locale);
	if (! lang)
	{
	const gchar *delimiter = strchr (locale, '_'); //  strip off the territory suffix

		if (delimiter)
		{
		gchar *copy = g_strndup (locale, delimiter - locale);
			lang = languagename_get (copy);
			g_free (copy);
		}

		if(! lang)
		{
			g_warning(" locale name not found '%s'", locale);
			lang = locale;
		}
		
	}

	return lang;
}


static void
ui_language_combobox_populate(GtkWidget *combobox)
{
GtkTreeModel *model;
GtkTreeIter  iter;
GDir *dir;
const gchar *dirname;

	DB( g_print("\n[ui-pref] lang populate\n") );

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter, 
	                    0, NULL,
	                    1, _("System Language"),
	                    -1);

	DB( g_print(" open '%s'\n",homebank_app_get_locale_dir () ) );

	dir = g_dir_open (homebank_app_get_locale_dir (), 0, NULL);
	if (! dir)
		return;

	while ((dirname = g_dir_read_name (dir)) != NULL)
	{
	gchar *filename = g_build_filename (homebank_app_get_locale_dir (),
		                      dirname,
		                      "LC_MESSAGES",
		                      GETTEXT_PACKAGE ".mo",
		                      NULL);
		DB( g_print("- seek for '%s'\n", filename) );
		if (g_file_test (filename, G_FILE_TEST_EXISTS))
		{
		const gchar *lang;
		gchar *label;
			
			gtk_list_store_append (GTK_LIST_STORE(model), &iter);

			lang = ui_language_combobox_get_name(dirname);
			label = g_strdup_printf ("%s [%s]", lang, dirname);

			gtk_list_store_set (GTK_LIST_STORE(model), &iter, 
					            0, dirname,
					            1, label,
					            -1);
			g_free(label);

		}
		g_free (filename);

	}
	g_dir_close (dir);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);


}


static GtkWidget *
ui_language_combobox_new(GtkWidget *label)
{
GtkListStore *store;
GtkWidget *combobox;
GtkCellRenderer *renderer;

	DB( g_print("\n[ui-pref] lang combo new\n") );

	store = gtk_list_store_new (2,
		G_TYPE_STRING,
		G_TYPE_STRING
		);
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_language_combobox_compare_func, NULL, NULL);

	combobox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, "text", 1, NULL);

	gtk_combo_box_set_id_column( GTK_COMBO_BOX(combobox), 0);
		
	g_object_unref(store);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), combobox);

	ui_language_combobox_populate(combobox);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	
	return combobox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


static gint ui_euro_combobox_id_to_active(gint id)
{
guint retval = 0;

	DB( g_print("\n[ui-pref] ui_euro_combobox_id_to_active\n") );

	for (guint i = 0; i < nb_euro_params; i++)
	{
		if( euro_params[i].id == id )
		{
			retval = i;
			DB( g_print("- id (country)=%d => %d - %s\n", id, i, euro_params[i].name) );
			break;
		}
	}

	return retval;
}



static gint ui_euro_combobox_active_to_id(gint active)
{
gint id;

	DB( g_print("\n[ui-pref] ui_euro_combobox_active_to_id\n") );

	DB( g_print("- to %d\n", active) );

	id = 0;
	if( active < (gint)nb_euro_params )
	{
		id = euro_params[active].id;
		DB( g_print("- id (country)=%d '%s'\n", id, euro_params[active].name) );
	}
	return id;
}


static GtkWidget *ui_euro_combobox_new(GtkWidget *label)
{
GtkWidget *combobox;
guint i;

	DB( g_print("\n[ui-pref] make euro preset\n") );

	combobox = gtk_combo_box_text_new();
	for (i = 0; i < nb_euro_params; i++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), euro_params[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), combobox);

	return combobox;
}


static void defpref_pathselect(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gint type = GPOINTER_TO_INT(user_data);
gchar **path;
gchar *title;
GtkWidget *entry;
gboolean r;

	DB( g_print("\n[ui-pref] path select\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	switch( type )
	{
		case PRF_PATH_WALLET:
			path = &PREFS->path_hbfile;
			entry = data->ST_path_hbfile;
			title = _("Choose a default HomeBank files folder");
			break;
		case PRF_PATH_BACKUP:
			path = &PREFS->path_hbbak;
			entry = data->ST_path_hbbak;
			title = _("Choose a default HomeBank backup files folder");
			break;
		case PRF_PATH_IMPORT:
			path = &PREFS->path_import;
			entry = data->ST_path_import;
			title = _("Choose a default import folder");
			break;
		case PRF_PATH_EXPORT:
			path = &PREFS->path_export;
			entry = data->ST_path_export;
			title = _("Choose a default export folder");
			break;
		default:
			return;
	}

	DB( g_print(" - hbfile %p %s at %p\n" , PREFS->path_hbfile, PREFS->path_hbfile, &PREFS->path_hbfile) );
	DB( g_print(" - import %p %s at %p\n" , PREFS->path_import, PREFS->path_import, &PREFS->path_import) );
	DB( g_print(" - export %p %s at %p\n" , PREFS->path_export, PREFS->path_export, &PREFS->path_export) );


	DB( g_print(" - before: %s %p\n" , *path, path) );

	r = ui_file_chooser_folder(GTK_WINDOW(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), title, path);


	DB( g_print(" - after: %s\n", *path) );

	if( r == TRUE )
		gtk_entry_set_text(GTK_ENTRY(entry), *path);


}


/*
** update the date sample label
*/
static void defpref_date_sample(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gchar buffer[256];
const gchar *fmt;
GDate *date;

	DB( g_print("\n[ui-pref] date sample\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	fmt = gtk_entry_get_text(GTK_ENTRY(data->ST_datefmt));
	date = g_date_new_julian (GLOBALS->today);
	g_date_strftime (buffer, 256-1, fmt, date);
	g_date_free(date);

	gtk_label_set_text(GTK_LABEL(data->LB_date), buffer);

}


/*
** update the number sample label
*/
static void defpref_numbereuro_sample(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
Currency cur;
gchar formatd_buf[G_ASCII_DTOSTR_BUF_SIZE];
gchar buf[128];

	DB( g_print("\n[ui-pref] number sample\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	cur.symbol = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_euro_symbol));
	cur.sym_prefix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_euro_isprefix));
	cur.decimal_char  = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_euro_decimalchar));
	cur.grouping_char = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_euro_groupingchar));
	cur.frac_digits   = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_euro_fracdigits));

	da_cur_initformat (&cur);
	
	DB( g_print("fmt: %s\n", cur.format) );

	g_ascii_formatd(formatd_buf, sizeof (formatd_buf), cur.format, HB_NUMBER_SAMPLE);
	hb_str_formatd(buf, 127, formatd_buf, &cur, TRUE);

	gtk_label_set_text(GTK_LABEL(data->LB_numbereuro), buf);

}


/*
** enable/disable euro
*/
static void defpref_eurotoggle(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-pref] euro toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_euro_enable));

	gtk_widget_set_sensitive(data->LB_euro_preset, sensitive);
	gtk_widget_set_sensitive(data->CY_euro_preset, sensitive);
	gtk_widget_set_sensitive(data->GRP_configuration, sensitive);
	gtk_widget_set_sensitive(data->GRP_format	 , sensitive);
}


/*
** set euro value widget from a country
*/
static void defpref_eurosetcurrency(GtkWidget *widget, gint country)
{
struct defpref_data *data;
EuroParams *euro;
gchar *buf, *buf2;
gint active;
	
	DB( g_print("\n[ui-pref] eurosetcurrency\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	
	active = ui_euro_combobox_id_to_active(country);
	euro = &euro_params[active];
	buf = g_strdup_printf("%s - %s", euro->iso, euro->name);
	gtk_label_set_markup(GTK_LABEL(data->ST_euro_country), buf);
	g_free(buf);

	//5.9 change label
	if(euro->mceii == FALSE)
	{
		buf  = g_strdup_printf("1 %s _=", euro->iso);
		buf2 = g_strdup("EUR");
	}
	else
	{
		buf = g_strdup("1 EUR _=");
		buf2 = g_strdup(euro->iso);
	}

	gtk_label_set_text_with_mnemonic(GTK_LABEL(data->LB_euro_src), buf);
	gtk_label_set_text_with_mnemonic(GTK_LABEL(data->LB_euro_dst), buf2);
	g_free(buf);
	g_free(buf2);
	
}


/*
** set euro value widget from a country
*/
static void defpref_europreset(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
EuroParams *euro;
gint active;

	DB( g_print("\n[ui-pref] euro preset\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	active = gtk_combo_box_get_active (GTK_COMBO_BOX(data->CY_euro_preset));
	data->country = ui_euro_combobox_active_to_id (active);

	defpref_eurosetcurrency(widget, data->country);

	euro = &euro_params[active];

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_euro_value), euro->value);

	//#2066110 force EUR for non mceii
	if( euro->mceii == FALSE)
	{
		euro = &euro_params_euro;
		if( euro_country_notmceii_rate_update(data->country) )
		{
			DB( g_print(" >update rate\n") );
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_euro_value), PREFS->euro_value);
		}
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_euro_fracdigits), euro->frac_digits);

	gtk_entry_set_text(GTK_ENTRY(data->ST_euro_symbol)   , euro->symbol);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_euro_isprefix), euro->sym_prefix);
	gtk_entry_set_text(GTK_ENTRY(data->ST_euro_decimalchar) , euro->decimal_char);
	gtk_entry_set_text(GTK_ENTRY(data->ST_euro_groupingchar), euro->grouping_char);

}


static void defpref_colorschemetoggle(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-pref] color scheme toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_use_palette)) == TRUE )
	{
	GtkColorScheme scheme;
	GdkRGBA rgba;
	gint index;

		index = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_color_scheme));

		colorscheme_init(&scheme, index);

		colorsheme_col8_to_rgba(&scheme.colors[scheme.cs_orange], &rgba);
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_exp_color), &rgba);

		colorsheme_col8_to_rgba(&scheme.colors[scheme.cs_green], &rgba);
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_inc_color), &rgba);

		colorsheme_col8_to_rgba(&scheme.colors[scheme.cs_red], &rgba);
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_warn_color), &rgba);

		colorsheme_col8_to_rgba(&scheme.colors[scheme.cs_blue], &rgba);
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_fut_bg_color), &rgba);
	}

	sensitive = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_use_palette));

	gtk_widget_set_sensitive(data->CP_exp_color, sensitive);
	gtk_widget_set_sensitive(data->CP_inc_color, sensitive);
	gtk_widget_set_sensitive(data->CP_warn_color, sensitive);
	gtk_widget_set_sensitive(data->CP_fut_bg_color, sensitive);

}


static void defpref_memotoggle(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-pref] memo acp toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_memoacp));
	gtk_widget_set_sensitive(data->ST_memoacp_days, sensitive);
}


static void defpref_gtkoverridetoggle(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-pref] gtk override toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_gtk_override));
	gtk_widget_set_sensitive(data->LB_gtk_fontsize, sensitive);
	gtk_widget_set_sensitive(data->NB_gtk_fontsize, sensitive);
}


static void defpref_backuptoggle(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-pref] backup toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_bak_is_automatic));
	gtk_widget_set_sensitive(data->LB_bak_max_num_copies, sensitive);
	gtk_widget_set_sensitive(data->NB_bak_max_num_copies, sensitive);
	gtk_widget_set_sensitive(data->GR_bak_freq          , sensitive);
}


static void defpref_color_scheme_changed(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;

	DB( g_print("\n[ui-pref] color scheme changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	gtk_widget_queue_draw (data->DA_colors);
	defpref_colorschemetoggle(widget, user_data);
}


/*
** set :: fill in widgets from PREFS structure
*/

static void defpref_set(struct defpref_data *data)
{
GdkRGBA rgba;

	DB( g_print("\n[ui-pref] set\n") );

	// general
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_show_splash), PREFS->showsplash);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_load_last), PREFS->loadlast);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_append_scheduled), PREFS->appendscheduled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_do_update_currency), PREFS->do_update_currency);
	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_maxspenditems), PREFS->rep_maxspenditems);
	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_fiscyearday), PREFS->fisc_year_day );
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_fiscyearmonth), PREFS->fisc_year_month);


	// files/backup
	gtk_entry_set_text(GTK_ENTRY(data->ST_path_hbfile), PREFS->path_hbfile);
	gtk_entry_set_text(GTK_ENTRY(data->ST_path_hbbak), PREFS->path_hbbak);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_bak_is_automatic), PREFS->bak_is_automatic);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_bak_max_num_copies), PREFS->bak_max_num_copies);

	// interface
	if(PREFS->language != NULL)
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_language), PREFS->language);
	else
		gtk_combo_box_set_active (GTK_COMBO_BOX(data->CY_language), 0);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_toolbar), PREFS->toolbar_style);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_gtk_override), PREFS->gtk_override);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_gtk_fontsize), PREFS->gtk_fontsize);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_gtk_darktheme), PREFS->gtk_darktheme);

	gtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_icontheme), PREFS->icontheme);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_iconsymbolic), PREFS->icon_symbolic);

	//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_image_size), PREFS->image_size);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_custom_colors), PREFS->custom_colors);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_custom_bg_future), PREFS->custom_colors);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_use_palette), PREFS->color_use_palette);

	gdk_rgba_parse(&rgba, PREFS->color_exp);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_exp_color), &rgba);
	gdk_rgba_parse(&rgba, PREFS->color_inc);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_inc_color), &rgba);
	gdk_rgba_parse(&rgba, PREFS->color_warn);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_warn_color), &rgba);
	gdk_rgba_parse(&rgba, PREFS->color_bg_future);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->CP_fut_bg_color), &rgba);


	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_ruleshint), PREFS->rules_hint);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_gridlines), PREFS->grid_lines);

	// transactions
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_daterange_txn), PREFS->date_range_txn);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_datefuture_nbdays), PREFS->date_future_nbdays);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_hide_reconciled), PREFS->hidereconciled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_show_remind), PREFS->showremind);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_show_void), PREFS->showvoid);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_include_remind), PREFS->includeremind);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_lock_reconciled), PREFS->safe_lock_recon);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_safe_pend_recon), PREFS->safe_pend_recon);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_safe_pend_past), PREFS->safe_pend_past);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_safe_pend_past_days), PREFS->safe_pend_past_days);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_herit_date), PREFS->heritdate);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_show_confirm), PREFS->txn_showconfirm);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_show_template), PREFS->txn_showtemplate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_memoacp), PREFS->txn_memoacp);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_memoacp_days), PREFS->txn_memoacp_days);
	
	//xfer
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_xfer_showdialog), PREFS->xfer_showdialog);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_xfer_daygap), PREFS->xfer_daygap);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_xfer_syncdate), PREFS->xfer_syncdate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_xfer_syncstat), PREFS->xfer_syncstat);

	// display format
	gtk_entry_set_text(GTK_ENTRY(data->ST_datefmt), PREFS->date_format);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_unitismile), PREFS->vehicle_unit_ismile);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_unitisgal), PREFS->vehicle_unit_isgal);

	// import/export
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_dtex_datefmt), PREFS->dtex_datefmt);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_dtex_ucfirst), PREFS->dtex_ucfirst);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_dtex_ofxname), PREFS->dtex_ofxname);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_dtex_ofxmemo), PREFS->dtex_ofxmemo);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_dtex_qifmemo), PREFS->dtex_qifmemo);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_dtex_qifswap), PREFS->dtex_qifswap);
	gtk_entry_set_text(GTK_ENTRY(data->ST_path_import), PREFS->path_import);
	gtk_entry_set_text(GTK_ENTRY(data->ST_path_export), PREFS->path_export);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_dtex_csvsep), PREFS->dtex_csvsep);

	// report
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_daterange_rep), PREFS->date_range_rep);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_color_scheme), PREFS->report_color_scheme);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_rep_smallfont), PREFS->rep_smallfont);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_stat_byamount), PREFS->stat_byamount);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_stat_showrate), PREFS->stat_showrate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_stat_showdetail), PREFS->stat_showdetail);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_stat_incxfer), PREFS->stat_includexfer);
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_budg_showdetail), PREFS->budg_showdetail);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_budg_unexclsub), PREFS->budg_unexclsub);

	//forecast
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forecast), PREFS->rep_forcast);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_forecast_nbmonth), PREFS->rep_forecat_nbmonth);


	/* euro */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_euro_enable), PREFS->euro_active);
	//gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_euro_preset), PREFS->euro_country);
	data->country = PREFS->euro_country;
	defpref_eurosetcurrency(data->dialog, data->country);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_euro_value), PREFS->euro_value);
	hbtk_entry_set_text(GTK_ENTRY(data->ST_euro_symbol), PREFS->minor_cur.symbol);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_euro_isprefix), PREFS->minor_cur.sym_prefix);
	hbtk_entry_set_text(GTK_ENTRY(data->ST_euro_decimalchar), PREFS->minor_cur.decimal_char);
	hbtk_entry_set_text(GTK_ENTRY(data->ST_euro_groupingchar), PREFS->minor_cur.grouping_char);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_euro_fracdigits), PREFS->minor_cur.frac_digits);

	//gtk_entry_set_text(GTK_ENTRY(data->ST_euro_symbol), PREFS->euro_symbol);
	//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_euro_nbdec), PREFS->euro_nbdec);
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_euro_thsep), PREFS->euro_thsep);
	
	//advanced
	gtk_entry_set_text(GTK_ENTRY(data->ST_adv_apirate_url), PREFS->api_rate_url);
	gtk_entry_set_text(GTK_ENTRY(data->ST_adv_apirate_key), PREFS->api_rate_key);

}


/*
** get :: fill PREFS structure from widgets
*/
#define RGBA_TO_INT(x) (int)(x*255)

static gchar *gdk_rgba_to_hex(GdkRGBA *rgba)
{
	return g_strdup_printf("#%02x%02x%02x", RGBA_TO_INT(rgba->red), RGBA_TO_INT(rgba->green), RGBA_TO_INT(rgba->blue));
}


static void defpref_get(struct defpref_data *data)
{
GdkRGBA rgba;
const gchar *active_id;
const gchar *datfmt;

	DB( g_print("\n[ui-pref] get\n") );

	// general
	PREFS->showsplash  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_show_splash));
	PREFS->loadlast  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_load_last));
	PREFS->appendscheduled  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_append_scheduled));
	PREFS->do_update_currency  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_do_update_currency));
	
	//PREFS->date_range_wal = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_daterange_wal));
	PREFS->rep_maxspenditems = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_maxspenditems));

	PREFS->fisc_year_day = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_fiscyearday));
	PREFS->fisc_year_month = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_fiscyearmonth));

	// files/backup
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_path_hbfile), &PREFS->path_hbfile);
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_path_hbbak) , &PREFS->path_hbbak);

	PREFS->bak_is_automatic  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_bak_is_automatic));
	PREFS->bak_max_num_copies = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_bak_max_num_copies));

	g_free(PREFS->language);
	PREFS->language = NULL;
	active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_language));
	if(active_id != NULL)
	{
		PREFS->language = g_strdup(active_id);
	}
	
	PREFS->toolbar_style = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_toolbar));
	//PREFS->image_size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_image_size));

	PREFS->gtk_override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_gtk_override));
	PREFS->gtk_fontsize = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_gtk_fontsize));
	PREFS->gtk_darktheme = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_gtk_darktheme));

	//icontheme
	active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_icontheme));
	if(active_id != NULL)
	{
		g_free(PREFS->icontheme);
		PREFS->icontheme = g_strdup(active_id);
	}
	PREFS->icon_symbolic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_iconsymbolic));

	PREFS->custom_colors = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_custom_colors));
	PREFS->custom_bg_future = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_custom_bg_future));
	PREFS->color_use_palette = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_use_palette));

	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(data->CP_exp_color), &rgba);
	g_free(PREFS->color_exp);
	PREFS->color_exp = gdk_rgba_to_hex(&rgba);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(data->CP_inc_color), &rgba);
	g_free(PREFS->color_inc);
	PREFS->color_inc = gdk_rgba_to_hex(&rgba);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(data->CP_warn_color), &rgba);
	g_free(PREFS->color_warn);
	PREFS->color_warn = gdk_rgba_to_hex(&rgba);
	
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(data->CP_fut_bg_color), &rgba);
	g_free(PREFS->color_bg_future);
	PREFS->color_bg_future = gdk_rgba_to_hex(&rgba);

	//PREFS->rules_hint = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_ruleshint));
	PREFS->grid_lines = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_gridlines));
	//list_txn_colpref_get(GTK_TREE_VIEW(data->LV_opecolumns), PREFS->lst_ope_columns);

	// transaction 
	PREFS->date_range_txn = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_daterange_txn));
	PREFS->date_future_nbdays  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_datefuture_nbdays));
	PREFS->hidereconciled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_hide_reconciled));
	PREFS->showremind = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_show_remind));
	PREFS->showvoid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_show_void));
	PREFS->includeremind = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_include_remind));
	PREFS->safe_lock_recon = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_lock_reconciled));
	PREFS->safe_pend_recon = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_safe_pend_recon));
	PREFS->safe_pend_past = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_safe_pend_past));
	PREFS->safe_pend_past_days = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_safe_pend_past_days));

	PREFS->heritdate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_herit_date));

	PREFS->txn_showconfirm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_show_confirm));
	PREFS->txn_showtemplate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_show_template));
	PREFS->txn_memoacp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_memoacp));
	PREFS->txn_memoacp_days = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_memoacp_days));
	// txn xfer
	PREFS->xfer_showdialog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_xfer_showdialog));
	PREFS->xfer_daygap = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_xfer_daygap));
	PREFS->xfer_syncdate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_xfer_syncdate));
	PREFS->xfer_syncstat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_xfer_syncstat));

	// display format
	//1903437 don't allow empty/invalid entry
	datfmt = gtk_entry_get_text(GTK_ENTRY(data->ST_datefmt));
	if( strlen(datfmt) == 0 )
		datfmt = "%x";
	g_free(PREFS->date_format);
	PREFS->date_format = g_strdup(datfmt);
	PREFS->vehicle_unit_ismile = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_unitismile));
	PREFS->vehicle_unit_isgal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_unitisgal));

	// import/export
	PREFS->dtex_datefmt = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_dtex_datefmt));
	PREFS->dtex_ucfirst = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_dtex_ucfirst));
	PREFS->dtex_ofxname = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_dtex_ofxname));
	PREFS->dtex_ofxmemo = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_dtex_ofxmemo));
	PREFS->dtex_qifmemo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_dtex_qifmemo));
	PREFS->dtex_qifswap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_dtex_qifswap));
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_path_import), &PREFS->path_import);
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_path_export), &PREFS->path_export);
	PREFS->dtex_csvsep = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_dtex_csvsep)); 

	// report
	PREFS->date_range_rep = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_daterange_rep));
	PREFS->report_color_scheme = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_color_scheme));
	PREFS->rep_smallfont   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_rep_smallfont));

	PREFS->stat_byamount   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_stat_byamount));
	PREFS->stat_showrate   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_stat_showrate));
	PREFS->stat_showdetail = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_stat_showdetail));
	PREFS->stat_includexfer = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_stat_incxfer));
	PREFS->budg_showdetail = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_budg_showdetail));
	PREFS->budg_unexclsub = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_budg_unexclsub));

	//forecast
	PREFS->rep_forcast = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forecast));
	PREFS->rep_forecat_nbmonth = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_forecast_nbmonth));


	// euro minor
	PREFS->euro_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_euro_enable));
	PREFS->euro_country = data->country;
	PREFS->euro_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_euro_value));
	//strcpy(PREFS->euro_symbol, gtk_entry_get_text(GTK_ENTRY(data->ST_euro_symbol)));
	//PREFS->euro_nbdec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_euro_nbdec));
	//PREFS->euro_thsep = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_euro_thsep));
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_euro_symbol), &PREFS->minor_cur.symbol);
	PREFS->minor_cur.sym_prefix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_euro_isprefix));
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_euro_decimalchar), &PREFS->minor_cur.decimal_char);
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_euro_groupingchar), &PREFS->minor_cur.grouping_char);
	PREFS->minor_cur.frac_digits = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_euro_fracdigits));
	//PREFS->chart_legend = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_chartlegend));
	
	//advanced
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_adv_apirate_url), &PREFS->api_rate_url);
	hbtk_entry_replace_text(GTK_ENTRY(data->ST_adv_apirate_key), &PREFS->api_rate_key);


	paymode_list_get_order(GTK_TREE_VIEW(data->LV_paymode));

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static GtkWidget *defpref_page_txn_payment (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget, *scrollwin;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Payment
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	label = make_label_group(_("Payment shows & chooses"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("Use drag & drop to reorder"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 2, 1);

	row++;
	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_grid_attach (GTK_GRID (group_grid), scrollwin, 1, row, 2, 1);
	widget = make_paymode_list();
	data->LV_paymode = widget;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);
	gtk_widget_set_hexpand(scrollwin, TRUE);
	gtk_widget_set_vexpand(scrollwin, TRUE);


	return content_grid;
}



static GtkWidget *defpref_page_advanced (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Advanced options
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	label = make_label_group(_("Currency API"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("Url:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_adv_apirate_url = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_left(_("Key:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_adv_apirate_key = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	return content_grid;
}



static GtkWidget *defpref_page_import (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Date options
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("General options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	label = make_label_left(_("Date order:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_DATEORDER);
	data->CY_dtex_datefmt = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Sentence _case memo/payee"));
	data->CM_dtex_ucfirst = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);
	

	// group :: OFX/QFX options
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("OFX/QFX options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	label = make_label_left(_("OFX _Name:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_OFXNAME);
	data->CY_dtex_ofxname = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_left(_("OFX _Memo:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_OFXMEMO);
	data->CY_dtex_ofxmemo = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	// group :: QIF options
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("QIF options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("_Import memos"));
	data->CM_dtex_qifmemo = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	widget = gtk_check_button_new_with_mnemonic (_("_Swap memos with payees"));
	data->CM_dtex_qifswap = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	// group :: other options
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("CSV options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("(transaction import only)"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 2, 1);

	row++;
	label = make_label_left(_("Separator:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_IMPORT_CSVSEPARATOR);
	data->CY_dtex_csvsep = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	return content_grid;
}

#define cube_dim 16

static gboolean
draw_callback (GtkWidget *widget,
               cairo_t   *cr,
               gpointer   user_data)
{
struct defpref_data *data = user_data;
gint index;
GtkColorScheme scheme;
gint w, h;
gint i, x, y;

	index = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_color_scheme));

	colorscheme_init(&scheme, index);
	
	gtk_widget_get_size_request (widget, &w, &h);
	x = y = 0;
	for(i=0;i<scheme.nb_cols;i++)
	{
		cairo_user_set_rgbcol (cr, &scheme.colors[i]);
		cairo_rectangle(cr, x, y, cube_dim, cube_dim);
		cairo_fill(cr);
		x += 1 + cube_dim;
		if( i == 15 )
		{ x = 0; y = 1 + cube_dim; }
	}

	return TRUE;
}



static GtkWidget *defpref_page_reports (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Main window reports
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Main window reports"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
/* removed 5.7
	label = make_label_left(_("_Range:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_daterange(label, DATE_RANGE_CUSTOM_HIDDEN);
	data->CY_daterange_wal = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);
*/

//	row++;
	label = make_label_left(_("Max _items:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_numeric(label, 5, 20);
	data->ST_maxspenditems = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);



	// group :: Initial filter
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Initial filter"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	label = make_label_left(_("_Range:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_daterange(label, DATE_RANGE_FLAG_CUSTOM_HIDDEN);
	data->CY_daterange_rep = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	// group :: Statistics options
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Statistics options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Show by _amount"));
	data->CM_stat_byamount = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Show _rate column"));
	data->CM_stat_showrate = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Show _details"));
	data->CM_stat_showdetail = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Include _transfer"));
	data->CM_stat_incxfer = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	// group :: Budget options
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Budget options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Show _details"));
	data->CM_budg_showdetail = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Exclude subcategories from unbudgeted line"));
	data->CM_budg_unexclsub = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	return content_grid;
}


static void defpref_cb_forecast_activate(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-pref] forecats activate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forecast));

	gtk_widget_set_sensitive(data->LB_forecast_nbmonth	 , sensitive);
	gtk_widget_set_sensitive(data->ST_forecast_nbmonth 	 , sensitive);
}



static GtkWidget *defpref_page_forecast (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	//5.7 forecast
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	label = make_label_group(_("Forecast"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Enable _forecast"));
	data->CM_forecast = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);
	
	row++;
	label = make_label_left(_("Month number:"));
	data->LB_forecast_nbmonth = label;
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_numeric(NULL, 1, 36);
	data->ST_forecast_nbmonth = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	return content_grid;
}


static GtkWidget *defpref_page_euro (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget, *expander;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: General
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("General"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 4, 1);

	row=1;
	widget = gtk_check_button_new_with_mnemonic (_("_Enable"));
	data->CM_euro_enable = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	label = make_label_left(_("_Preset:"));
	data->LB_euro_preset =label;
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_euro_combobox_new (label);
	data->CY_euro_preset = widget;
	gtk_widget_set_margin_start (label, 2*SPACING_LARGE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	// group :: Configuration
	crow++;
    group_grid = gtk_grid_new ();
	data->GRP_configuration = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Configuration"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 4, 1);

	row=1;
	widget = make_label_left(NULL);
	data->ST_euro_country = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	label = make_label_left("1 EUR _=");
	data->LB_euro_src = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_exchange_rate(label);
	data->NB_euro_value = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);
	label = make_label_left(NULL);
	data->LB_euro_dst = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	// group :: Numbers format
	crow++;
    group_grid = gtk_grid_new ();
	data->GRP_format = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Format"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 4, 1);

	row = 1;
	widget = make_label_left(NULL);
	data->LB_numbereuro = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	expander = gtk_expander_new_with_mnemonic(_("_Customize"));
	gtk_grid_attach (GTK_GRID (group_grid), expander, 1, row, 1, 1);

    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_expander_set_child(GTK_EXPANDER(expander), group_grid);
	
	row = 0;
	label = make_label_left(_("_Symbol:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string_maxlength(label, 3);
	data->ST_euro_symbol = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Is pre_fix"));
	data->CM_euro_isprefix = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_left(_("_Decimal char:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string_maxlength(label, 1);
	data->ST_euro_decimalchar = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_left(_("_Frac digits:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_numeric(label, 0.0, 6.0);
	data->NB_euro_fracdigits = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_left(_("_Grouping char:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string_maxlength(label, 1);
	data->ST_euro_groupingchar = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	return content_grid;
}


static GtkWidget *defpref_page_locale (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget, *expander, *hbox;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: General
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("User interface"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("_Language:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_language_combobox_new(label);
	data->CY_language = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	row++;
	label = make_label_left(_("Date display:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_label_left(NULL);
	data->LB_date = widget;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(widget)), GTK_STYLE_CLASS_DIM_LABEL);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	expander = gtk_expander_new_with_mnemonic(_("C_ustomize"));
	gtk_grid_attach (GTK_GRID (group_grid), expander, 2, row, 1, 1);

    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_expander_set_child (GTK_EXPANDER(expander), group_grid);

	row++;
	label = make_label_left(_("_Format:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	data->ST_datefmt = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 3, row, 1, 1);

	
	gtk_widget_set_tooltip_text(widget,
	_("%a locale's abbreviated weekday name.\n"
"%A locale's full weekday name. \n"
"%b locale's abbreviated month name. \n"
"%B locale's full month name. \n"
"%c locale's appropriate date and time representation. \n"
"%C century number (the year divided by 100 and truncated to an integer) as a decimal number [00-99]. \n"
"%d day of the month as a decimal number [01,31]. \n"
"%D same as %m/%d/%y. \n"
"%e day of the month as a decimal number [1,31]; a single digit is preceded by a space. \n"
"%j day of the year as a decimal number [001,366]. \n"
"%m month as a decimal number [01,12]. \n"
"%p locale's appropriate date representation. \n"
"%y year without century as a decimal number [00,99]. \n"
"%Y year with century as a decimal number.")
);

	row++;
	widget = make_label_left(NULL);
	gtk_label_set_markup (GTK_LABEL(widget), "<small><a href=\"https://man7.org/linux/man-pages/man3/strftime.3.html\">online reference</a></small>");
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);




	// group :: Fiscal year
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	label = make_label_group(_("Fiscal year"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	//TRANSLATORS: (fiscal year) starts on
	label = make_label_left(_("Starts _on:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);
	widget = make_numeric (label, 1, 28);
	data->NB_fiscyearday = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);
	widget = hbtk_combo_box_new_with_data (NULL, CYA_MONTHS);
	data->CY_fiscyearmonth = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);



	// group :: Measurement units
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Measurement units"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Use _miles for meter"));
	data->CM_unitismile = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Use _gallon for fuel"));
	data->CM_unitisgal = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	return content_grid;
}


static GtkWidget *defpref_page_txn (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: txn list
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("General"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Always show remind"));
	data->CM_show_remind = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Always show void"));
	data->CM_show_void = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Include remind into balance and report"));
	data->CM_include_remind = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	crow++;
	// group :: safety
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Safety"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Lock reconciled for any changes"));
	data->CM_lock_reconciled = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Approve additions before last reconciliation"));
	data->CM_safe_pend_recon = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Approve additions"));
	data->CM_safe_pend_past = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	widget = make_numeric(NULL, 0, 366);
	data->ST_safe_pend_past_days = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//TRANSLATORS: there is a spinner on the left of this label, and so you have 0....x days as pending
	label = make_label_left(_("days before today's date"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);


	crow++;
	// group :: txn list
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Ledger window"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("_Range:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_daterange(label, DATE_RANGE_FLAG_CUSTOM_HIDDEN);
	data->CY_daterange_txn = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_left(_("_Show future:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	widget = make_numeric(NULL, 0, 366);
	data->ST_datefuture_nbdays = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//TRANSLATORS: there is a spinner on the left of this label, and so you have 0....x days in advance the current date
	label = make_label_left(_("days ahead"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Hide reconciled"));
	data->CM_hide_reconciled = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	return content_grid;
}

static GtkWidget *defpref_page_txn_dialog (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);


	// group :: txn dialog
	crow = 0;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Transaction dialog"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("_Keep the last date when multiple add or inherit"));
	data->CM_herit_date = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Enable _memo autocomplete with"));
	data->CM_memoacp = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	widget = make_numeric(NULL, 0, 1460);
	data->ST_memoacp_days = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);
	label = make_label_left(_("rolling days"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Show add confirmation text for 5s"));
	data->CM_show_confirm = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Show template list when edit"));
	data->CM_show_template = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);


	return content_grid;
}



static GtkWidget *defpref_page_txn_transfer (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	// group :: transfer
	crow = 0;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Behavior"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("When adding, always show selection _action for target dialog"));
	//widget = gtk_check_button_new_with_mnemonic (_("Always prompt"));
	data->CM_xfer_showdialog = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	label = make_label_left(_("Date _gap to find a target:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_numeric(NULL, 2, 7);
	data->ST_xfer_daygap = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);
	label = make_label_left(_("days"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 3, row, 1, 1);

	// group :: transfer
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Synchronize"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("_Date"));
	data->CM_xfer_syncdate = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("_Status"));
	data->CM_xfer_syncstat = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);


	return content_grid;
}




/* test */
static void defpref_icons_changed_cb(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
const gchar *active_id;
gboolean symbolic;
	
	DB( g_print("\n[ui-pref] cb icon changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_icontheme));
	symbolic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_iconsymbolic));

	DB( g_print(" name: %s, sym:%d\n", active_id, symbolic ) );

	//gtk_settings_set_string_property (gtk_settings_get_default (), "gtk-icon-theme-name", active_id, "gtkrc:0");
	g_object_set(gtk_settings_get_default (), "gtk-icon-theme-name", active_id, NULL);

	homebank_pref_icon_symbolic(symbolic);


}
/* end test */


static GtkWidget *defpref_page_intf_theming (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow =0;
	// group :: Theming
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	label = make_label_group(_("Theme"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row=1;
#ifdef G_OS_UNIX
	label = make_label_left(_("Dark mode:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	gchar *txt = _("System has no preference");
	if( GLOBALS->color_scheme == PREFER_DARK )
		txt = _("System prefer dark");
	else
	if( GLOBALS->color_scheme == PREFER_LIGHT )
		txt = _("System prefer light");
	label = make_label_left(txt);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(label)), GTK_STYLE_CLASS_DIM_LABEL);
	gtk_grid_attach (GTK_GRID (group_grid), label, 2, row, 1, 1);

	row++;
 #endif

	widget = gtk_check_button_new_with_mnemonic (_("Use _dark mode if available"));
	data->CM_gtk_darktheme = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);


	// group :: Icons
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Icons"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row=1;
	label = make_label_left(_("_Icon theme:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new (label);

	//future
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "Default",	"Default");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "hicolor",	"Legacy");

	data->CY_icontheme = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Use _symbolic icons if available"));
	data->CM_iconsymbolic = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);



	// group :: GTK override
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Gtk settings"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Ov_erride"));
	data->CM_gtk_override = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);
	
	row++;	
	label = make_label_left(_("_Font size:"));
	data->LB_gtk_fontsize = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_numeric(label, 8, 16);
	data->NB_gtk_fontsize = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	return content_grid;
}


static GtkWidget *defpref_page_intf_color (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Theming
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	row = 1;
	label = make_label_group(_("Chart"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row++;
	label = make_label_left(_("_Palette:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_array(label, chart_colors);
	data->CY_color_scheme = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	widget = gtk_drawing_area_new ();
	data->DA_colors = widget;
	gtk_widget_set_size_request (widget, (1+cube_dim)*16, (1+cube_dim)*2);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 3, 1);

	g_signal_connect (data->DA_colors, "draw", G_CALLBACK (draw_callback), data);

	// group :: Others
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Others"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("_Use colors from the chart palette"));
	data->CM_use_palette = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_color_button_new ();
	data->CP_exp_color = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	label = make_label_left(_("_Expense"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 2, row, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), widget);

	row++;
	widget = gtk_color_button_new ();
	data->CP_inc_color = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	label = make_label_left(_("_Income"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 2, row, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), widget);

	row++;
	widget = gtk_color_button_new ();
	data->CP_warn_color = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	label = make_label_left(_("_Warning"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 2, row, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), widget);

	row++;
	widget = gtk_color_button_new ();
	data->CP_fut_bg_color = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	label = make_label_left(_("Background _future"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 2, row, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), widget);

	return content_grid;
}


static GtkWidget *defpref_page_intf (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: General
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("General"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Color the _amounts"));
	data->CM_custom_colors = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Color the _background of future transactions"));
	data->CM_custom_bg_future = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	//widget = gtk_check_button_new_with_mnemonic (_("Enable rows in alternating colors"));
	//data->CM_ruleshint = widget;
	label = make_label_left(_("_Grid line:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_GRID_LINES);
	data->CY_gridlines = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	// group ::Charts options
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Charts options"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Smaller legend _font"));
	data->CM_rep_smallfont = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);



	// group :: Deprecated
	crow += 5;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group("Deprecated");
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("_Toolbar:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_TOOLBAR_STYLE);
	data->CY_toolbar = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);


	return content_grid;
}


static GtkWidget *defpref_page_filebackup (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *hbox, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Backup
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Backup"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("_Enable automatic backups"));
	data->CM_bak_is_automatic = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	label = make_label_left(_("_Number of backups to keep:"));
	data->LB_bak_max_num_copies = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_numeric (label, 1, 99);
	data->NB_bak_max_num_copies = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	data->GR_bak_freq = hbox;
	//gtk_widget_set_hexpand (hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 2, 1);

		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
		gtk_box_prepend (GTK_BOX (hbox), widget);
		label = make_label_left(_("Backup frequency is once a day"));
		gtk_box_prepend (GTK_BOX (hbox), label);


	return content_grid;
}


static GtkWidget *defpref_page_folders (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *hbox, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Files folder
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("HomeBank files"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label_left(_("_Wallets:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand (hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

	widget = make_string(label);
	data->ST_path_hbfile = widget;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(widget)), GTK_STYLE_CLASS_LINKED);
	hbtk_box_prepend (GTK_BOX (hbox), widget);

	//widget = gtk_button_new_with_label("...");
	widget = gtk_button_new_from_icon_name(ICONNAME_FOLDER, GTK_ICON_SIZE_BUTTON);
	data->BT_path_hbfile = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);

	row++;
	label = make_label_left(_("_Backups:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand (hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

	widget = make_string(label);
	data->ST_path_hbbak = widget;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(widget)), GTK_STYLE_CLASS_LINKED);
	hbtk_box_prepend (GTK_BOX (hbox), widget);

	//widget = gtk_button_new_with_label("...");
	widget = gtk_button_new_from_icon_name(ICONNAME_FOLDER, GTK_ICON_SIZE_BUTTON);
	data->BT_path_hbbak = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);

	
	// group :: Files folder
	crow++;
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Exchange files"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);
	
	row = 1;
	label = make_label_left(_("_Import:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand (hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

	widget = make_string(label);
	data->ST_path_import = widget;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(widget)), GTK_STYLE_CLASS_LINKED);
	hbtk_box_prepend (GTK_BOX (hbox), widget);

	//widget = gtk_button_new_with_label("...");
	widget = gtk_button_new_from_icon_name(ICONNAME_FOLDER, GTK_ICON_SIZE_BUTTON);
	data->BT_path_import = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);

	row++;
	label = make_label_left(_("_Export:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand (hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

	widget = make_string(label);
	data->ST_path_export = widget;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(widget)), GTK_STYLE_CLASS_LINKED);
	hbtk_box_prepend (GTK_BOX (hbox), widget);

	//widget = gtk_button_new_with_label("...");
	widget = gtk_button_new_from_icon_name(ICONNAME_FOLDER, GTK_ICON_SIZE_BUTTON);
	data->BT_path_export = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);



	return content_grid;
}


static GtkWidget *defpref_page_general (struct defpref_data *data)
{
GtkWidget *content_grid, *group_grid, *label, *widget;
gint crow, row;

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);

	crow = 0;
	// group :: Program start
    group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	label = make_label_group(_("Program start"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic (_("Show splash screen"));
	data->CM_show_splash = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Load last opened file"));
	data->CM_load_last = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Post pending scheduled transactions"));
	data->CM_append_scheduled = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Update currencies online"));
	data->CM_do_update_currency = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);


	return content_grid;
}


static void defpref_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
struct defpref_data *data;
GtkWidget *notebook;
GtkTreeView *treeview;
GtkTreeModel *model;
GtkTreeIter iter;
GValue val = { 0, };
gint page_num;

	DB( g_print("\n[ui-pref] selection\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		notebook = GTK_WIDGET(user_data);
		treeview = gtk_tree_selection_get_tree_view(treeselection);
		data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

		gtk_tree_model_get_value(model, &iter, LST_PREF_PAGENUM, &val);
		page_num = g_value_get_int (&val);
		DB( g_print(" pagenum: %d\n", page_num) );
		g_value_unset (&val);


		gtk_tree_model_get_value(model, &iter, LST_PREF_LABEL, &val);
		gtk_label_set_text (GTK_LABEL (data->label), g_value_get_string (&val));
		g_value_unset (&val);

		gtk_tree_model_get_value(model, &iter, LST_PREF_ICONNAME, &val);
		//gtk_image_set_from_pixbuf (GTK_IMAGE (data->image), g_value_get_object (&val));
		gtk_image_set_from_icon_name(GTK_IMAGE (data->image), g_value_get_string (&val), GTK_ICON_SIZE_DIALOG);
		g_value_unset (&val);

		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);

		//defpref_change_page(GTK_WIDGET(gtk_tree_selection_get_tree_view(treeselection)), GINT_TO_POINTER(page));
	}

}


/*
** add an empty new account to our temp GList and treeview
*/
static void defpref_reset(GtkWidget *widget, gpointer user_data)
{
struct defpref_data *data;
gint result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(defpref_reset) (data=%p)\n", data) );

	result = ui_dialog_msg_confirm_alert(
		GTK_WINDOW(data->dialog),
		_("Reset All Preferences"),
		_("Do you really want to reset\nall preferences to default\nvalues?"),
	    _("_Reset"),
		TRUE
		);
	if( result == GTK_RESPONSE_OK )
	{
		homebank_pref_setdefault();
		defpref_set(data);
	}
	
}


// the dialog creation
GtkWidget *defpref_dialog_new (void)
{
struct defpref_data *data;
GtkWidget *window, *content, *mainvbox;
GtkWidget *hbox, *vbox, *scrollwin, *widget, *notebook, *page, *image, *label;

	data = g_malloc0(sizeof(struct defpref_data));
	
      window = gtk_dialog_new_with_buttons (_("Preferences"),
			GTK_WINDOW(GLOBALS->mainwindow),
			0,	//no flags
			NULL, //no buttons
			NULL);

	widget = gtk_dialog_add_button(GTK_DIALOG(window), _("_Reset"),	55);
	gtk_widget_set_margin_end(widget, SPACING_LARGE);
	gtk_dialog_add_button(GTK_DIALOG(window), _("_Cancel"),	GTK_RESPONSE_REJECT);
	gtk_dialog_add_button(GTK_DIALOG(window), _("_OK"),    GTK_RESPONSE_ACCEPT);

	data->dialog = window;

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);

	content = gtk_dialog_get_content_area(GTK_DIALOG (window));			// return a vbox
	mainvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	hbtk_box_prepend (GTK_BOX (content), mainvbox);

	hb_widget_set_margin(GTK_WIDGET(mainvbox), SPACING_MEDIUM);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	hbtk_box_prepend (GTK_BOX (mainvbox), hbox);

	//left part
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	gtk_box_prepend (GTK_BOX (hbox), vbox);
	
	//list
	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    hbtk_box_prepend (GTK_BOX (vbox), scrollwin);
	widget = pref_list_create();
	data->LV_page = widget;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), widget);


	//right part : notebook
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	hbtk_box_prepend (GTK_BOX (hbox), vbox);
	gtk_widget_show (vbox);

	//header
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(hbox, "hbprfhead");
	gtk_box_prepend (GTK_BOX (vbox), hbox);
	gtk_widget_show (hbox);

	GtkStyleContext *context = gtk_widget_get_style_context (hbox);
	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION <= 18) )
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_LIST_ROW);
		gtk_widget_set_state_flags(hbox, GTK_STATE_FLAG_SELECTED, TRUE);
	#else
	GtkCssProvider *provider;
		provider = gtk_css_provider_new ();
		gtk_css_provider_load_from_data (provider, 
		"#hbprfhead { color: @theme_selected_fg_color; background-color: @theme_selected_bg_color; }"
		, -1, NULL);
		gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(provider), G_MAXUINT);
	
	//	gtk_style_context_set_state(context, GTK_STATE_FLAG_SELECTED);
	#endif

	label = gtk_label_new (NULL);
	hb_widget_set_margins(GTK_WIDGET(label), SPACING_SMALL, 0, SPACING_SMALL, SPACING_SMALL);
	gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_XX_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);

	gtk_box_prepend (GTK_BOX (hbox), label);
	gtk_widget_show (label);
	data->label = label;

	image = gtk_image_new ();
	hb_widget_set_margins(GTK_WIDGET(image), SPACING_SMALL, SPACING_SMALL, SPACING_SMALL, 0);
	gtk_box_append (GTK_BOX (hbox), image);
	gtk_widget_show (image);
	data->image = image;

	//notebook
	notebook = gtk_notebook_new();
	data->GR_page = notebook;
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    hbtk_box_prepend (GTK_BOX (vbox), notebook);


	//general
	page = defpref_page_general(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//interface
	page = defpref_page_intf(data);
	scrollwin = make_scrolled_window_ns(GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), page);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin, NULL);

		//theming
		page = defpref_page_intf_theming(data);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

		//color
		page = defpref_page_intf_color(data);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);


	//locale
	page = defpref_page_locale(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//transaction
	page = defpref_page_txn(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

		//dialog
		page = defpref_page_txn_dialog(data);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

		//transfer
		page = defpref_page_txn_transfer(data);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

		//5.8 temporary
		page = defpref_page_txn_payment(data);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//import
	page = defpref_page_import(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//report
	page = defpref_page_reports(data);
	scrollwin = make_scrolled_window_ns(GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), page);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin, NULL);

	//forecast
	page = defpref_page_forecast(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);
	

	//backup
	page = defpref_page_filebackup(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//folders
	page = defpref_page_folders(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//euro
	page = defpref_page_euro(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

	//advanced
	page = defpref_page_advanced(data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);


	//todo:should move this
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_euro_enable), PREFS->euro_active);


	//connect all our signals

	g_signal_connect (data->CY_icontheme, "changed", G_CALLBACK (defpref_icons_changed_cb), NULL);
	g_signal_connect (data->CM_iconsymbolic, "toggled", G_CALLBACK (defpref_icons_changed_cb), NULL);


	g_signal_connect (data->CM_gtk_override, "toggled", G_CALLBACK (defpref_gtkoverridetoggle), NULL);

	g_signal_connect (data->CM_bak_is_automatic, "toggled", G_CALLBACK (defpref_backuptoggle), NULL);

	g_signal_connect (data->CM_memoacp, "toggled", G_CALLBACK (defpref_memotoggle), NULL);
	
	//path selector
	g_signal_connect (data->BT_path_hbfile, "pressed", G_CALLBACK (defpref_pathselect), GINT_TO_POINTER(PRF_PATH_WALLET));
	g_signal_connect (data->BT_path_hbbak , "pressed", G_CALLBACK (defpref_pathselect), GINT_TO_POINTER(PRF_PATH_BACKUP));
	g_signal_connect (data->BT_path_import, "pressed", G_CALLBACK (defpref_pathselect), GINT_TO_POINTER(PRF_PATH_IMPORT));
	g_signal_connect (data->BT_path_export, "pressed", G_CALLBACK (defpref_pathselect), GINT_TO_POINTER(PRF_PATH_EXPORT));

	g_signal_connect (data->CM_use_palette, "toggled", G_CALLBACK (defpref_colorschemetoggle), NULL);
	//g_signal_connect (data->CM_custom_colors, "toggled", G_CALLBACK (defpref_colortoggle), NULL);


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_page)), "changed", G_CALLBACK (defpref_selection), notebook);

	g_signal_connect (data->CM_euro_enable, "toggled", G_CALLBACK (defpref_eurotoggle), NULL);

    g_signal_connect (data->CY_euro_preset, "changed", G_CALLBACK (defpref_europreset), NULL);

	//forecast
	g_signal_connect (data->CM_forecast, "toggled", G_CALLBACK (defpref_cb_forecast_activate), NULL);

	//date
    g_signal_connect (data->ST_datefmt, "changed", G_CALLBACK (defpref_date_sample), NULL);

	//report
	g_signal_connect (data->CY_color_scheme, "changed", G_CALLBACK (defpref_color_scheme_changed), NULL);


	//euro number
    g_signal_connect (data->ST_euro_symbol   , "changed", G_CALLBACK (defpref_numbereuro_sample), NULL);
	g_signal_connect (data->CM_euro_isprefix, "toggled", G_CALLBACK (defpref_numbereuro_sample), NULL);
	g_signal_connect (data->ST_euro_decimalchar , "changed", G_CALLBACK (defpref_numbereuro_sample), NULL);
    g_signal_connect (data->ST_euro_groupingchar, "changed", G_CALLBACK (defpref_numbereuro_sample), NULL);
    g_signal_connect (data->NB_euro_fracdigits, "value-changed", G_CALLBACK (defpref_numbereuro_sample), NULL);

	//g_signal_connect (data->BT_default, "pressed", G_CALLBACK (defpref_currency_change), NULL);


	//setup, init and show window
	//defhbfile_setup(data);
	//defhbfile_update(data->LV_arc, NULL);

	defpref_set(data);

	defpref_gtkoverridetoggle(window, NULL);
	defpref_memotoggle(window, NULL);
	defpref_backuptoggle (window, NULL);
	//defpref_colortoggle(window, NULL);
	defpref_eurotoggle(window, NULL);
	defpref_cb_forecast_activate(window, NULL);

	gtk_window_resize(GTK_WINDOW(window), 640, 256);

	gtk_widget_show_all (window);



	//gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_page)));
	//select last page
	DB( g_print(" select %d:%d\n", PREFS->lastlvl1, PREFS->lastlvl2) );
	if( PREFS->lastlvl1 > 0 )
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(PREFS->lastlvl1, PREFS->lastlvl2, -1);
		gtk_tree_selection_select_path (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_page)), path);
		gtk_tree_path_free(path);
	}
	else
	{
		//select first row
		DB( g_print(" select first\n") );
		GtkTreePath *path = gtk_tree_path_new_first ();
		gtk_tree_selection_select_path (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_page)), path);
		gtk_tree_path_free(path);
	}
	
	//defpref_selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_page)), NULL);



	gint result;
	gchar *old_lang, *old_path_hbbak;


		//wait for the user
		result = gtk_dialog_run (GTK_DIALOG (window));

		switch( result )
		{
			case GTK_RESPONSE_ACCEPT:

				//user attention needed if change is
				//language/backup path
				old_lang = g_strdup(PREFS->language);
				old_path_hbbak = g_strdup(PREFS->path_hbbak);

				defpref_get(data);
				homebank_pref_save();
				ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL));

				DB( g_print("old='%s' new='%s'\n", old_lang, PREFS->language) );
				
				//if(g_ascii_strncasecmp(old_lang == NULL ? "" : old_lang, PREFS->language == NULL ? "" : PREFS->language, -1) != 0)
				if( hb_string_ascii_compare(old_lang, PREFS->language) != 0 )
				{
					ui_dialog_msg_infoerror(GTK_WINDOW(window), GTK_MESSAGE_INFO,
						_("Info"),
						_("You will have to restart HomeBank\nfor the language change to take effect.")
					);
				}

				if( hb_string_ascii_compare(old_path_hbbak, PREFS->path_hbbak) != 0 )
				{
					ui_dialog_msg_infoerror(GTK_WINDOW(window), GTK_MESSAGE_INFO,
						_("Info"),
						_("The backup directory has changed,\nyou may need to copy the '.bak' file to this new location.")
					);
				}
				
				g_free(old_lang);
				g_free(old_path_hbbak);
				break;

			case 55:
				defpref_reset (window, NULL);
				break;
		}
	
		//store last page selection
		{
		GtkTreeModel *model;
		GtkTreeIter iter;
		gint depth;

			if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_page)), &model, &iter))
			{
				//store last page
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				gint *indices = gtk_tree_path_get_indices_with_depth(path, &depth);
				if( indices != NULL )
				{
					PREFS->lastlvl1 = indices[0];
					if(depth > 1)
						PREFS->lastlvl2 = indices[1];
					else
						PREFS->lastlvl2 = 0;
				}
				DB( g_print(" stored: %d:%d\n", PREFS->lastlvl1, PREFS->lastlvl2) );
				gtk_tree_path_free(path);


			}
		}


	// cleanup and destroy
	//defhbfile_cleanup(data, result);
	gtk_window_destroy (GTK_WINDOW(window));

	g_free(data);
	
	return window;
}

// -------------------------------


static GtkWidget *pref_list_create(void)
{
GtkTreeStore *store;
GtkWidget *treeview;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;
GtkTreeIter    iter;
struct pref_list_datas *tmp;
gint i;

	/* create list store */
	store = gtk_tree_store_new(
	  	LST_PREF_MAX,
		G_TYPE_INT,		//unique id
		G_TYPE_STRING,	//icon
		G_TYPE_STRING,	//label
		G_TYPE_INT		//pagenum
		);

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (treeview), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_SINGLE);

	/* column 1: icon */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set(renderer, "stock-size", GTK_ICON_SIZE_DND, NULL);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "icon-name", LST_PREF_ICONNAME, NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", LST_PREF_LABEL, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//populate our combobox model
	for(i=0;i<PREF_MAX;i++)
	{
		tmp = &CYA_PREF_GROUP[i];
		if( tmp->label == NULL )
			break;

		if( tmp->level == 1 )
		{
			gtk_tree_store_insert_with_values(store, &iter, NULL, -1,
				LST_PREF_UID		, tmp->key,
				LST_PREF_ICONNAME	, tmp->iconname,
				LST_PREF_LABEL    	, _(tmp->label),
				LST_PREF_PAGENUM	, i,
				-1);
		}
		else
		{
			gtk_tree_store_insert_with_values(store, NULL, &iter, -1,
				LST_PREF_UID		, tmp->key,
				LST_PREF_ICONNAME	, tmp->iconname,
				LST_PREF_LABEL    	, _(tmp->label),
				LST_PREF_PAGENUM	, i,
				-1);

		}
	}

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));

	return(treeview);
}


