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

#ifndef __HB_PREFERENCES_H__
#define __HB_PREFERENCES_H__

#include "hb-currency.h"
#include "list-account.h"
#include "list-scheduled.h"


#define DEFAULT_FORMAT_DATE			"%x"

#define MAX_FRAC_DIGIT		6

//Tango light
#define LIGHT_EXP_COLOR		"#fcaf3e"	//Orange
#define LIGHT_INC_COLOR		"#8ae234"	//Chameleon
#define LIGHT_WARN_COLOR	"#ef2929"	//Scarlett Red

//Tango medium
#define MEDIUM_EXP_COLOR	"#f57900"	//Orange
#define MEDIUM_INC_COLOR	"#73d216"	//Chameleon
#define MEDIUM_WARN_COLOR	"#cc0000"	//Scarlett Red

//Tango dark
#define DEFAULT_EXP_COLOR		"#ce5c00"	//Orange
#define DEFAULT_INC_COLOR		"#4e9a36"	//Chameleon
#define DEFAULT_WARN_COLOR		"#a40000"	//Scarlett Red




/*
** Preference datas
*/

struct WinGeometry
{
	gint		l, t, w, h, s;
};


struct Preferences
{
	//general
	gboolean	showsplash;
	gboolean	showwelcome;
	gboolean	loadlast;
	gboolean	appendscheduled;
	gboolean	do_update_currency;

	//top spending
	//gint		date_range_wal;
	gint		rep_maxspenditems;

	//interface
	gshort		toolbar_style;
	gshort		grid_lines;

	gboolean	gtk_override;
	gshort		gtk_fontsize;
	gboolean	gtk_darktheme;

	gchar		*icontheme;

	gboolean	custom_colors;
	gchar		*color_exp;
	gchar		*color_inc;
	gchar		*color_warn;

	//locale
	gchar		*language;
	gchar		*date_format;
	gshort		fisc_year_day;
	gshort		fisc_year_month;
	gboolean	vehicle_unit_ismile;	// true if unit is mile, default Km
	gboolean	vehicle_unit_isgal;		// true if unit is gallon, default Liter

	//transactions
	//-- register
	gint		date_range_txn;
	gint		date_future_nbdays;
	gboolean	hidereconciled;
	gboolean    showremind;
	gboolean    showvoid;
	gboolean	includeremind;
	gboolean	lockreconciled;
	//-- dialog
	gboolean	heritdate;
	gboolean	txn_memoacp;
	gshort		txn_memoacp_days;
	gboolean	txn_showtemplate;
	gboolean	txn_showconfirm;

	//txn xfer
	gboolean	xfer_showdialog;
	gshort		xfer_daygap;
	gboolean	xfer_syncstat;

	//5.8 paymode
	gint 		lst_paymode[NUM_PAYMODE_KEY+1];

	//import/export
	gint		dtex_datefmt;
	gint		dtex_daygap;
	gint		dtex_ofxname;
	gint		dtex_ofxmemo;
	gboolean	dtex_qifmemo;
	gboolean	dtex_qifswap;
	gboolean	dtex_ucfirst;
	gint		dtex_csvsep;

	//report options
	gint		date_range_rep;
	gint		report_color_scheme;
	gboolean	rep_smallfont;
	gboolean	stat_byamount;
	gboolean	stat_showrate;
	gboolean	stat_showdetail;
	gboolean	stat_includexfer;
	gboolean	budg_showdetail;
	//5.7
	gboolean	rep_forcast;
	gint		rep_forecat_nbmonth;


	//backup option
	gboolean	bak_is_automatic;
	gshort		bak_max_num_copies;

	//folders
	gchar		*path_hbfile;
	gchar		*path_hbbak;
	gchar		*path_import;
	gchar		*path_export;
	gchar		*path_attach;

	//currency api
	gchar		*api_rate_url;
	gchar		*api_rate_key;

	//euro zone
	gboolean	euro_active;
	gint		euro_country;
	gdouble		euro_value;
	Currency	minor_cur;


	//---- others data (not in pref dialog) -----
	gboolean	dtex_nointro;
	gboolean	dtex_dodefpayee;
	gboolean	dtex_doautoassign;

	gchar	    IntCurrSymbol[8];

	gint 		lst_impope_columns[NUM_LST_DSPOPE+1];

	//register list column
	gint 		lst_ope_columns[NUM_LST_DSPOPE+1];
	gint 		lst_ope_col_width[NUM_LST_DSPOPE+1];
	gint		lst_ope_sort_id;	// -- implicit --
	gint		lst_ope_sort_order; // -- implicit --

	//detail list column
	gint 		lst_det_columns[NUM_LST_DSPOPE+1];
	gint 		lst_det_col_width[NUM_LST_DSPOPE+1];

	/* windows/dialogs size an position */
	struct WinGeometry	wal_wg;
	struct WinGeometry	acc_wg;

	struct WinGeometry	sta_wg;
	struct WinGeometry	tme_wg;
	struct WinGeometry	ove_wg;
	struct WinGeometry	bud_wg;
	struct WinGeometry	cst_wg;

	struct WinGeometry	txn_wg;
	struct WinGeometry	dbud_wg;

	// main window stuffs
	gboolean	wal_toolbar;
	gboolean	wal_totchart;
	gboolean	wal_timchart;
	gboolean	wal_upcoming;

	gint		wal_vpaned;
	gint		wal_hpaned;

	//home panel
	gshort		pnl_acc_col_acc_width;
	gint 		lst_acc_columns[NUM_LST_COL_DSPACC+1];
	gshort		pnl_acc_show_by;

	//hub total/time
	gshort		hub_tot_view;
	gshort		hub_tot_range;
	gshort		hub_tot_raw;

	gshort		hub_tim_view;
	gshort		hub_tim_range;
	gshort		hub_tim_raw;

	//5.8 scheduled column order
	gint 		lst_sch_columns[NUM_COL_SCH_UID+1];
	gshort		pnl_upc_col_pay_show;
	gshort		pnl_upc_col_pay_width;
	gshort		pnl_upc_col_cat_show;
	gshort		pnl_upc_col_cat_width;
	gshort		pnl_upc_col_mem_show;
	gshort		pnl_upc_col_mem_width;
	gint		pnl_upc_range;

	gchar		*pnl_list_tab;

	//vehiclecost units (mile/gal or km/liters)

	gchar	   *vehicle_unit_dist0;
	gchar	   *vehicle_unit_dist1;
	gchar	   *vehicle_unit_vol;
	gchar	   *vehicle_unit_100;
	gchar	   *vehicle_unit_distbyvol;

};

gint homebank_pref_list_column_get(gint *cols_id, gint uid, gint maxcol);

void homebank_pref_setdefault_lst_ope_columns(void);
void homebank_pref_setdefault_lst_det_columns(void);
void homebank_pref_setdefault_lst_sch_columns(void);

void homebank_pref_setdefault_win(void);
void homebank_prefs_set_default(void);
void homebank_pref_free(void);
void homebank_pref_createformat(void);
void homebank_pref_init_measurement_units(void);


void homebank_pref_icon_symbolic(gboolean active);
void homebank_pref_apply(void);


gboolean homebank_pref_load(void);
gboolean homebank_pref_save(void);
void homebank_pref_setdefault(void);

#endif
