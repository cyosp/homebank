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

#ifndef __HB_MANWINDOW_GTK_H__
#define __HB_MANWINDOW_GTK_H__


typedef struct _pnlaccgrp PnlAccGrp;

struct _pnlaccgrp
{
	gchar		*name;
	//gshort		type;
	//gshort		pos;
	//gboolean	expanded;
	gboolean	showtotal;
	gdouble		bal_clear;
	gdouble		bal_recon;
	gdouble		bal_today;
	gdouble		bal_future;
	GPtrArray	*acclist;
};



struct hbfile_data
{
	GtkWidget	*window;
	GtkWidget	*dbgchange;
	GtkCssProvider *provider;

	GtkWidget	*menubar;
	GtkWidget	*ME_menufile, *ME_menuedit, *ME_menuview, *ME_menuman, *ME_menutxn, *ME_menurep, *ME_menutool, *ME_menuhelp;
	GtkWidget	*MI_new, *MI_open, *MI_save, *MI_saveas, *MI_import, *MI_exportqif, *MI_revert, *MI_openbak, *MI_properties, *MI_close, *MI_quit;
	GtkWidget	*MI_prefs, *MI_showtbar, *MI_showspend, *MI_showbotlist, *MI_eurominor;
	GtkWidget	*MI_manwal, *MI_manacc, *MI_manpay, *MI_mancat, *MI_mantpl, *MI_manbud, *MI_manbudtable, *MI_manasg, *MI_mancur, *MI_mantag;
	GtkWidget	*MI_txnadd, *MI_txnshow, *MI_txnshowall, *MI_scheduler, *MI_addscheduled; 
	GtkWidget	*MI_repstat, *MI_reptime, *MI_repbal, *MI_repbudg, *MI_repvehi;
	GtkWidget	*MI_welcome, *MI_filestats, *MI_anonymise;
	GtkWidget	*MI_contents, *MI_online, *MI_updates, *MI_relnote, *MI_problem, *MI_translate, *MI_about;

	GtkWidget	*toolbar;
	GtkWidget	*BT_new, *BT_open, *BT_save;
	GtkWidget	*BT_manacc, *BT_manpay, *BT_mancat, *BT_mantpl, *BT_manbud, *BT_manasg;
	GtkWidget	*BT_txnshow, *BT_txnadd;
	GtkWidget	*BT_repstat, *BT_reptime, *BT_repbal, *BT_repbudg, *BT_repvehi;
	GtkWidget	*BT_help, *BT_donate;
	 
	GtkWidget	*vpaned;
	GtkWidget	*hpaned;

	/* panel: your account */
	GtkWidget	*LV_acc;
	GtkWidget   *BT_expandall;
	GtkWidget   *BT_collapseall;
	gboolean	showall;
	GSimpleActionGroup *action_group_acc;
	GSimpleActionGroup *action_group_top;
	GHashTable  *h_accgrp;
	PnlAccGrp	*totaccgrp;

	GtkWidget	*GR_top;
	GtkWidget	*LV_top;
	gdouble		toptotal;
	GtkWidget	*CY_range;
	GtkWidget	*RE_pie;

	GtkWidget	*GR_upc;

	GtkWidget	*LV_upc;
	GtkWidget   *LB_maxpostdate;
	GtkWidget   *BT_sched_skip;
	GtkWidget   *BT_sched_post;
	GtkWidget   *BT_sched_editpost;
	GtkWidget	*CY_sched_range;

	GtkWidget	*stack;
	// max is 2=HUB_TXN_TYPE_REMIND, so 3 elt
	GtkWidget	*LV_txn[3];
	 
	gchar	*wintitle;

	Account *acc;

	gint	busy;

	GtkRecentManager *recent_manager;
	GtkWidget *recent_menu;

	Filter		*filter;
	
	/*
	UBYTE	accnum;
	UBYTE	pad0;
	struct	Account *acc;

	ULONG	mindate, maxdate;
	ULONG	change;
	ULONG	keyvalue;
	UBYTE	title[140];
	UBYTE	Filename[108];
	UBYTE	csvpath[108];
	*/
};


enum {
	HB_WELCOME_OPENLAST = 2,
	HB_WELCOME_CREATENEW,
	HB_WELCOME_OPENEXISTING,
	HB_WELCOME_OPENSAMPLE,
	HB_WELCOME_READMANUAL,
	HB_WELCOME_CONFIGPREF,
};



//temporary ?
void ui_mainwindow_defarchive(Archive *arc);


GtkWidget *create_hbfile_window(GtkWidget *do_widget);
void ui_mainwindow_populate_accounts(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_open_check(GtkWidget *widget, gchar *filepath);
void ui_mainwindow_open_internal(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_update(GtkWidget *widget, gpointer user_data);
gint ui_mainwindow_action_help_welcome(void);

#endif /* __HB_MANWINDOW_GTK_H__ */
