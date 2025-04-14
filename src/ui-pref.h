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

#ifndef __HB_PREFERENCE_GTK_H__
#define __HB_PREFERENCE_GTK_H__


struct defpref_data
{

//-- common
	GtkWidget	*dialog;

	GtkWidget	*LV_page;
	GtkWidget	*GR_page;
	GtkWidget	*label;
	GtkWidget	*image;
	GtkWidget   *BT_clear;

//-- general
	GtkWidget	*CM_show_splash;
	GtkWidget	*CM_load_last;
	GtkWidget	*CM_append_scheduled;
	GtkWidget   *CM_do_update_currency;

//-- interface
	GtkWidget	*CM_custom_colors;
	GtkWidget	*CM_custom_bg_future;
	GtkWidget	*CY_gridlines;
	GtkWidget	*CM_rep_smallfont;
	GtkWidget	*CY_toolbar;

	GtkWidget	*CM_gtk_darktheme;
	GtkWidget	*CY_icontheme;
	GtkWidget	*CM_iconsymbolic;
	GtkWidget	*CM_gtk_override;
	GtkWidget	*LB_gtk_fontsize, *NB_gtk_fontsize;

	GtkWidget	*CY_color_scheme;
	GtkWidget   *DA_colors;
	GtkWidget	*CM_use_palette;
	GtkWidget	*CP_exp_color;
	GtkWidget	*CP_inc_color;
	GtkWidget	*CP_warn_color;
	GtkWidget	*CP_fut_bg_color;

//-- locale
	GtkWidget	*CY_language;
	GtkWidget	*LB_date, *ST_datefmt;
	GtkWidget   *NB_fiscyearday;
	GtkWidget   *CY_fiscyearmonth;
	GtkWidget	*CM_unitismile;
	GtkWidget	*CM_unitisgal;

//-- transactions
	GtkWidget	*CY_daterange_txn;
	GtkWidget   *ST_datefuture_nbdays;
	GtkWidget	*CM_hide_reconciled;
	GtkWidget	*CM_show_remind;
	GtkWidget	*CM_show_void;
	GtkWidget	*CM_include_remind;
	GtkWidget	*CM_lock_reconciled;
	GtkWidget	*CM_safe_pend_recon;
	GtkWidget	*CM_safe_pend_past;
	GtkWidget	*ST_safe_pend_past_days;

	GtkWidget	*CM_herit_date;
	GtkWidget	*CM_herit_grpflg;
	GtkWidget	*CM_memoacp;
	GtkWidget	*ST_memoacp_days;
	GtkWidget	*CM_show_confirm;
	GtkWidget	*CM_show_template;

	GtkWidget	*CM_xfer_showdialog;
	GtkWidget	*ST_xfer_daygap;
	GtkWidget	*CM_xfer_syncdate;
	GtkWidget	*CM_xfer_syncstat;
	//5.8 paymode
	GtkWidget	*LV_paymode;

//-- import/export
	GtkWidget	*CY_dtex_datefmt;
	GtkWidget	*CY_dtex_ofxname;
	GtkWidget	*CY_dtex_ofxmemo;
	GtkWidget	*CM_dtex_qifmemo;
	GtkWidget	*CM_dtex_qifswap;
	GtkWidget	*CM_dtex_ucfirst;
	GtkWidget	*CY_dtex_csvsep;

//-- report
	//GtkWidget	*CY_daterange_wal;
	GtkWidget	*ST_maxspenditems;
	GtkWidget	*CY_daterange_rep;
	GtkWidget	*CM_stat_byamount;
	GtkWidget	*CM_stat_showdetail;
	GtkWidget	*CM_stat_showrate;
	GtkWidget	*CM_stat_incxfer;
	GtkWidget	*CM_budg_showdetail;
	GtkWidget	*CM_budg_unexclsub;

//-- forecast
	GtkWidget	*CM_forecast;
	GtkWidget	*LB_forecast_nbmonth;
	GtkWidget	*ST_forecast_nbmonth;

//-- backup
	GtkWidget   *CM_bak_is_automatic;
	GtkWidget	*GR_bak_freq;
	GtkWidget   *LB_bak_max_num_copies, *NB_bak_max_num_copies;

//-- folders
	GtkWidget	*ST_path_hbfile, *BT_path_hbfile;
	GtkWidget	*ST_path_hbbak, *BT_path_hbbak;
	GtkWidget	*ST_path_import, *BT_path_import;
	GtkWidget	*ST_path_export, *BT_path_export;
	//GtkWidget	*ST_path_attach, *BT_path_attach;

//-- euro
	gint		country;
	GtkWidget	*CM_euro_enable;
	GtkWidget	*LB_euro_preset, *CY_euro_preset;
	GtkWidget	*GRP_configuration;
	GtkWidget	*ST_euro_country;
	GtkWidget	*LB_euro_src;
	GtkWidget	*NB_euro_value;
	GtkWidget	*LB_euro_dst;

	GtkWidget	*GRP_format;
	GtkWidget	*LB_numbereuro;
	GtkWidget	*ST_euro_symbol;
	GtkWidget	*CM_euro_isprefix;
	GtkWidget	*ST_euro_decimalchar;	
	GtkWidget	*NB_euro_fracdigits;
	GtkWidget	*ST_euro_groupingchar;	

	//-- advanced
	GtkWidget	*ST_adv_apirate_url;
	GtkWidget	*ST_adv_apirate_key;
};


enum {
	PRF_PATH_WALLET,
	PRF_PATH_BACKUP,
	PRF_PATH_IMPORT,
	PRF_PATH_EXPORT,
	PRF_PATH_ATTACH,
};


struct pref_list_datas {
	gshort		level;
	gshort		key;
	const gchar	*iconname;
	const gchar	*label;
};


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


void free_pref_icons(void);
void load_pref_icons(void);

GtkWidget *defpref_dialog_new (void);

#endif
