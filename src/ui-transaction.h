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

#ifndef __HB_TRANSACTION_GTK_H__
#define __HB_TRANSACTION_GTK_H__

#include "ui-split.h"


/* official GTK_RESPONSE are negative */
#define HB_RESPONSE_ADD		 1
#define HB_RESPONSE_ADDKEEP	 2


enum {
	HID_AMOUNT,
	MAX_HID_AMOUNT
};


struct deftransaction_data
{
	GtkWidget	*dialog;

	GtkWidget	*RA_type;
	GtkWidget	*LB_date, *GR_date;
	GtkWidget	*PO_date;	//5.7 removed *LB_wday;
	GtkWidget	*PO_pay;
	GtkWidget	*ST_memo;
	GtkWidget	*ST_amount, *BT_split, *LB_curr;
	GtkWidget	*ST_xferamt, *LB_xfercurr, *IM_xfernorate;
	GtkWidget	*CM_cheque;

	GtkWidget	*LB_mode, *NU_mode;
	GtkWidget	*LB_info, *ST_info;
	GtkWidget	*PO_cat;
	GtkWidget	*LB_accfrom, *PO_acc;
	GtkWidget	*LB_accto, *PO_accto;
	GtkWidget	*ST_tags, *CY_tags;
	//GtkWidget   *CY_status;
	GtkWidget   *RA_status;

	GtkWidget   *IB_warnsign;
	GtkWidget	*LB_msgadded;

	/* popover */
	GtkWidget   *MB_template;
	GtkTreeModel *model;
	GtkTreeModelFilter *modelfilter;
	GtkWidget   *LV_arc;
	GtkWidget   *CM_showsched;
	GtkWidget   *CM_showallacc;
	GtkWidget   *ST_search;

	HbTxnDlgAction	action;
	HbTxnDlgType    type;

	guint		evtsrcid;
	gint		accnum;
	guint32		kacc;
	gboolean	showtemplate;
	gboolean	isxferdst;		//edit: true if this is a xfer target (amt > 0) 

	Transaction *ope;
};


enum
{
	LST_DSPTPL_DATAS,
	LST_DSPTPL_NAME,
	NUM_LST_DSPTPL
};


GtkWidget *create_deftransaction_window (GtkWindow *parent, HbTxnDlgAction action, HbTxnDlgType type, guint32 kacc);
void deftransaction_set_amount(GtkWidget *widget, gdouble amount);
void deftransaction_external_confirm(GtkWidget *dialog, Transaction *ope);
gint deftransaction_external_edit(GtkWindow *parent, Transaction *old_txn, Transaction *new_txn);
void deftransaction_set_transaction(GtkWidget *widget, Transaction *ope);
void deftransaction_get			(GtkWidget *widget, gpointer user_data);
void deftransaction_add			(GtkWidget *widget, gpointer user_data);
void deftransaction_dispose(GtkWidget *widget, gpointer user_data);
void deftransaction_set_amount_from_split(GtkWidget *widget, gdouble amount);

#endif
