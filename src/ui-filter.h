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

#ifndef __HB_FILTER_GTK_H__
#define __HB_FILTER_GTK_H__


#define	FLT_PAGE_NAME_DAT		"dat"
#define	FLT_PAGE_NAME_TYP		"typ"
#define	FLT_PAGE_NAME_STA		"sta"
#define	FLT_PAGE_NAME_ACC		"acc"
#define	FLT_PAGE_NAME_CAT		"cat"
#define	FLT_PAGE_NAME_PAY		"pay"
#define	FLT_PAGE_NAME_TAG		"tag"
#define	FLT_PAGE_NAME_PMT		"pmt"
#define	FLT_PAGE_NAME_TXT		"txt"


/* official GTK_RESPONSE are negative */
#define HB_RESPONSE_FLT_SAVE_USE	33
#define HB_RESPONSE_FLT_RESET		55


enum
{
	LST_DEFFLT_TOGGLE,
	LST_DEFFLT_DATAS,
	NUM_LST_DEFFLT
};


struct ui_flt_list_data
{
	GtkWidget	*gr_criteria;
	GtkWidget	*tb_bar, *bt_all, *bt_non, *bt_inv;
};


struct ui_flt_manage_data
{
	Filter		*filter;

	gboolean	saveable;
	gboolean	show_account;


	GtkWidget	*dialog;
	GtkWidget	*stack;

	GtkWidget	*SW_enabled[FLT_GRP_MAX];
	GtkWidget	*RA_matchmode[FLT_GRP_MAX];
	GtkWidget	*GR_page[FLT_GRP_MAX];

	GtkWidget	*CY_range;
	GtkWidget	*LB_mindate, *LB_maxdate;
	GtkWidget	*PO_mindate, *PO_maxdate;
	GtkWidget	*CY_month, *NB_year;

	GtkWidget	*CM_typnexp, *CM_typninc, *CM_typxexp, *CM_typxinc;
	
	GtkWidget	*CM_stanon, *CM_staclr, *CM_starec;
	
	GtkWidget	*GR_force;

	GtkWidget	*CM_forceadd, *CM_forcechg, *CM_forceremind, *CM_forcevoid;

	GtkWidget	*CM_paymode[NUM_PAYMODE_MAX];

	GtkWidget	*ST_minamount, *ST_maxamount;

	GtkWidget	*CM_exact;
	GtkWidget	*ST_number, *ST_memo;

	GtkWidget	*LV_acc;
	GtkWidget	*LV_pay;
	GtkWidget	*LV_tag;
	GtkWidget	*LV_cat;
	GtkWidget	*BT_expand, *BT_collapse;
};


/* = = = = = = = = = = */

gint ui_flt_manage_dialog_new(GtkWindow *parentwindow, Filter *filter, gboolean show_account, gboolean txnmode);


#endif

