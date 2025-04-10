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

#ifndef __HB_PREFERENCE_GTK_H__
#define __HB_PREFERENCE_GTK_H__


struct defpref_data
{
	// common
	GtkWidget	*dialog;

	GtkWidget	*LV_page;
	GtkWidget	*GR_page;
	GtkWidget	*label;
	GtkWidget	*image;
	GtkWidget   *BT_clear;

	// general
	GtkWidget	*CM_show_splash;
	GtkWidget	*CM_load_last;
	GtkWidget	*CM_append_scheduled;
	GtkWidget   *CM_do_update_currency;
	GtkWidget	*ST_path_hbfile, *BT_path_hbfile;
	GtkWidget	*ST_path_hbbak, *BT_path_hbbak;
	GtkWidget   *CM_bak_is_automatic;
	GtkWidget	*GR_bak_freq;
	GtkWidget   *LB_bak_max_num_copies, *NB_bak_max_num_copies;
	//GtkWidget	*CY_daterange_wal;
	GtkWidget	*ST_maxspenditems;
	GtkWidget   *NB_fiscyearday;
	GtkWidget   *CY_fiscyearmonth;



	GtkWidget	*CY_language;
	GtkWidget	*CY_toolbar;
	GtkWidget	*CM_gtk_override;
	GtkWidget	*LB_gtk_fontsize, *NB_gtk_fontsize;
	GtkWidget	*CM_gtk_darktheme;
	GtkWidget	*CY_icontheme;

	GtkWidget	*LB_colors, *CY_colors;

	GtkWidget   *GR_colors;
	GtkWidget	*CM_custom_colors;
	GtkWidget	*LB_exp_color, *CP_exp_color;
	GtkWidget	*CP_inc_color;
	GtkWidget	*CP_warn_color;
	//GtkWidget	*CM_ruleshint;
	GtkWidget	*CY_gridlines;

	//GtkWidget	*LV_opecolumns;
	//GtkWidget	*BT_go_up;
	//GtkWidget	*BT_go_down;

	
	GtkWidget	*CM_runwizard;

	GtkWidget	*ST_path_import, *BT_path_import;
	GtkWidget	*ST_path_export, *BT_path_export;

	GtkWidget	*CM_herit_date;
	GtkWidget	*CM_show_confirm;
	GtkWidget	*CM_show_template;
	GtkWidget	*CM_hide_reconciled;
	GtkWidget	*CM_show_remind;
	GtkWidget	*CM_show_void;
	GtkWidget	*CM_include_remind;
	GtkWidget	*CM_lock_reconciled;
	GtkWidget	*CM_memoacp;
	GtkWidget	*ST_memoacp_days;
	
	GtkWidget	*CM_xfer_showdialog;
	GtkWidget	*ST_xfer_daygap;
	GtkWidget	*CM_xfer_syncdate;
	GtkWidget	*CM_xfer_syncstat;

	GtkWidget	*ST_datefmt;
	GtkWidget	*LB_date;

	GtkWidget	*CM_unitismile;
	GtkWidget	*CM_unitisgal;

	GtkWidget	*CY_daterange_txn;
	GtkWidget   *ST_datefuture_nbdays;
	GtkWidget	*CY_daterange_rep;

	//5.8 paymode
	GtkWidget	*LV_paymode;

	/* currencies */
	GtkWidget	*LB_default;
	GtkWidget	*BT_default; 
	
	GtkWidget	*CM_euro_enable;
	GtkWidget	*GRP_configuration;
	GtkWidget	*GRP_format;
	 
	GtkWidget	*LB_euro_preset, *CY_euro_preset;
	GtkWidget	*ST_euro_country;
	GtkWidget	*LB_euro_src;
	GtkWidget	*NB_euro_value;
	GtkWidget	*LB_euro_dst;

	GtkWidget	*ST_euro_symbol;
	GtkWidget	*CM_euro_isprefix;
	GtkWidget	*ST_euro_decimalchar;	
	GtkWidget	*ST_euro_groupingchar;	
	GtkWidget	*NB_euro_fracdigits;
	GtkWidget	*LB_numbereuro;

	//GtkWidget	*ST_euro_symbol;
	//GtkWidget	*NB_euro_nbdec;
	//GtkWidget	*CM_euro_thsep;

	GtkWidget	*CM_stat_byamount;
	GtkWidget	*CM_stat_showdetail;
	GtkWidget	*CM_stat_showrate;
	GtkWidget	*CM_stat_incxfer;

	GtkWidget	*CM_budg_showdetail;

	//forecast
	GtkWidget	*CM_forecast;
	GtkWidget	*LB_forecast_nbmonth;
	GtkWidget	*ST_forecast_nbmonth;

	GtkWidget	*CY_color_scheme;
	GtkWidget   *DA_colors;
	GtkWidget	*CM_rep_smallfont;

	GtkWidget	*CM_chartlegend;

	GtkWidget	*CY_dtex_datefmt;
	GtkWidget	*CY_dtex_ofxname;
	GtkWidget	*CY_dtex_ofxmemo;
	GtkWidget	*CM_dtex_qifmemo;
	GtkWidget	*CM_dtex_qifswap;
	GtkWidget	*CM_dtex_ucfirst;
	GtkWidget	*CY_dtex_csvsep;
	 
	gint		country;

	//advanced
	GtkWidget	*ST_adv_apirate_url;
	GtkWidget	*ST_adv_apirate_key;

};


enum {
	PRF_PATH_WALLET,
	PRF_PATH_BACKUP,
	PRF_PATH_IMPORT,
	PRF_PATH_EXPORT,
};

void free_pref_icons(void);
void load_pref_icons(void);

GtkWidget *defpref_dialog_new (void);

#endif
