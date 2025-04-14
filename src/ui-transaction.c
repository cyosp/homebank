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
#include "hbtk-decimalentry.h"

#include "ui-transaction.h"
#include "gtk-dateentry.h"
#include "hbtk-switcher.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-account.h"
#include "ui-txn-split.h"
#include "ui-tag.h"


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

//extern HbKvData CYA_TXN_STATUS[];
extern HbKivData CYA_TXN_STATUSIMG[];


extern gchar *CYA_TXN_TYPE[];


static void ui_popover_tpl_populate(struct deftransaction_data *data, GList *srclist);


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void	deftransaction_set_amount_currency(Account *acc, GtkWidget *stamount, GtkWidget *lbcurr)
{
guint curdigits = 2;
gchar *curlabel = "";

	if( acc != NULL )
	{
	Currency *cur =	da_cur_get(acc->kcur);

		curdigits = (cur != NULL) ? cur->frac_digits : 2;
		curlabel  = strlen(cur->iso_code) == 3 ? cur->iso_code : cur->symbol;
	}

	//gtk_spin_button_set_digits (GTK_SPIN_BUTTON(stamount), curdigits);
	hbtk_decimal_entry_set_digits(HBTK_DECIMAL_ENTRY(stamount), curdigits);
	gtk_label_set_label(GTK_LABEL(lbcurr), curlabel);
}


static void	deftransaction_set_amount_xfer(struct deftransaction_data *data)
{
Account *srcacc, *dstacc;
gdouble srcamt, dstamt;
gboolean haswarn = FALSE;

	if( data->action != TXN_DLG_ACTION_ADD )
		goto end;

	DB( g_print("\n[ui-transaction] set xfer amount\n") );

	srcacc = ui_acc_entry_popover_get(GTK_BOX(data->PO_acc));
	dstacc = ui_acc_entry_popover_get(GTK_BOX(data->PO_accto));
	if( srcacc != NULL && dstacc != NULL )
	{
		//srcamt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		srcamt = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_amount));
		//return 0 if convertion fail
		dstamt = hb_amount_convert(-srcamt, srcacc->kcur, dstacc->kcur);

		if( hb_amount_cmp(dstamt, 0.0) == 0 )
			haswarn = TRUE;
		else
			//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_xferamt), dstamt);
			hbtk_decimal_entry_set_value(HBTK_DECIMAL_ENTRY(data->ST_xferamt), dstamt);
	}

end:

	hb_widget_visible(data->IM_xfernorate, haswarn);
}


static void deftransaction_update(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gint type, paymode;
gboolean sensitive, visible, xferamtvisible;
Account *srcacc, *dstacc;
gchar *lbto;

	DB( g_print("\n[ui-transaction] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type    = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_type));
	paymode = paymode_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));

	//template: hide date
	visible = (data->type == TXN_DLG_TYPE_TPL) ? FALSE : TRUE;
	hb_widget_visible(data->LB_date, visible);
	hb_widget_visible(data->GR_date, visible);

	//xfer: hide split+paymode / show accto
	visible = (type == TXN_TYPE_INTXFER) ? FALSE : TRUE;
	DB( g_print(" is xfer %d: hide split/pay, show accto\n", !visible) );
	hb_widget_visible(data->BT_split, visible);
	hb_widget_visible(data->LB_mode , visible);
	hb_widget_visible(data->NU_mode , visible);
	hb_widget_visible(data->LB_accto, !visible);
	hb_widget_visible(data->PO_accto, !visible);

	//5.8
	if( PREFS->xfer_syncdate == FALSE && data->action == TXN_DLG_ACTION_EDIT && data->type == TXN_DLG_TYPE_TXN )
	{
		hb_widget_visible(data->LB_dateto, !visible);
		hb_widget_visible(data->PO_dateto, !visible);
	}

	//this code is duplicated into paymode
	visible = (paymode == PAYMODE_CHECK) ? TRUE : FALSE;
	visible = (type == TXN_TYPE_INTXFER) ? FALSE : visible;
	hb_widget_visible(data->CM_cheque, visible);

	//disable amount+category if split is set
	sensitive = (data->ope->flags & (OF_SPLIT)) ? FALSE : TRUE;
	DB( g_print(" is plit %d: disable amt/cat\n", !sensitive) );
	gtk_widget_set_sensitive(data->RA_type, sensitive);
	gtk_widget_set_sensitive(data->ST_amount, sensitive);
	gtk_widget_set_sensitive(data->PO_cat, sensitive);

	DB( g_print(" action:%d type:%d\n", data->action, data->type) );

	//srcacc = ui_acc_comboboxentry_get(GTK_COMBO_BOX(data->PO_acc));
	//dstacc = ui_acc_comboboxentry_get(GTK_COMBO_BOX(data->PO_accto));
	srcacc = ui_acc_entry_popover_get(GTK_BOX(data->PO_acc));
	dstacc = ui_acc_entry_popover_get(GTK_BOX(data->PO_accto));

	//#1676162 update the nb digits of amount
	//set digits and currency label
	deftransaction_set_amount_currency(srcacc, data->ST_amount, data->LB_curr);

	lbto   = _("_To:");
	xferamtvisible = FALSE;
	if( type == TXN_TYPE_INTXFER )
	{
	//gdouble amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	gdouble amount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_amount));

		DB( g_print(" xfer stuff, amt=%.2f\n", amount) );

		//5.8 test
		lbto   = ( amount <= 0.0 ) ? _("_To:")   : _("_From:");

		//#1673260 show target amount if != kcur
		if(srcacc != NULL && dstacc != NULL)
		{
			xferamtvisible = (srcacc->kcur == dstacc->kcur) ? FALSE : TRUE;
		}

		//set digits and currency label
		deftransaction_set_amount_currency(dstacc, data->ST_xferamt, data->LB_xfercurr);
		deftransaction_set_amount_xfer(data);
	}
	else
		hb_widget_visible(data->IM_xfernorate, FALSE);

	DB( g_print(" lblto: '%s'\n", lbto ) );
	DB( g_print(" show tgt amt %d\n", xferamtvisible ) );

	if( PREFS->xfer_syncdate == FALSE && data->action == TXN_DLG_ACTION_EDIT && data->type == TXN_DLG_TYPE_TXN )
	{
		gtk_label_set_text_with_mnemonic (GTK_LABEL(data->LB_dateto), lbto);
	}
	gtk_label_set_text_with_mnemonic (GTK_LABEL(data->LB_accto) , lbto);
	hb_widget_visible(data->ST_xferamt , xferamtvisible);
	hb_widget_visible(data->LB_xfercurr, xferamtvisible);

	// item validation
	sensitive = FALSE;
	if( (data->type == TXN_DLG_TYPE_TPL) )
	{
		DB( g_print(" add tpl no check\n") );
		sensitive = TRUE;
	}
	else
	{
		DB( g_print(" eval src/dst acc\n") );

		sensitive = ( (srcacc != NULL) && (srcacc->key != 0) ) ? TRUE : FALSE;
		if( (type == TXN_TYPE_INTXFER) )
		{
			sensitive = FALSE;
			//#1858682 still disabled > faulty test a copy/paste acc instead of accto...
			//todo
			if( (srcacc != NULL) && (dstacc != NULL) )
			{
				if( (dstacc->key != 0)
				 && (dstacc->key != srcacc->key)
				 //#1673260 xfer
				 //&& (dstacc->kcur == srcacc->kcur)
				) { sensitive = TRUE; }
			}
		}
	}

	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->dialog), GTK_RESPONSE_ACCEPT, sensitive);
	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->dialog), HB_RESPONSE_ADD    , sensitive);
	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->dialog), HB_RESPONSE_ADDKEEP, sensitive);

}


static void deftransaction_update_warnsign(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gboolean warning = FALSE;
gdouble amount;
gint type;
Category *cat;

	DB( g_print("\n[ui-transaction] update warning sign\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#1830707 no warning for xfer
	type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
	if( type != TXN_TYPE_INTXFER )
	{
		//cat = ui_cat_comboboxentry_get(GTK_COMBO_BOX(data->PO_cat));
		cat = ui_cat_entry_popover_get(GTK_BOX(data->PO_cat));
		if(cat != NULL && cat->key > 0)
		{
			//amount = hb_amount_round(gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount)), 2);
			amount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_amount));
			if(amount != 0.0)
			{
				type = (amount > 0) ? 1 : -1;
				warning = (category_type_get(cat) != type) ? TRUE : FALSE;
			}
		}
	}

	if(warning)
	{
		gtk_widget_show_all(data->IB_warnsign);
		//#GTK+710888: hack waiting a GTK fix
		gtk_widget_queue_resize (data->IB_warnsign);
	}
	else
		gtk_widget_hide(data->IB_warnsign);

}


static void deftransaction_cb_payee_changed(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Category *cat;
gint paymode;
Payee *pay;

	DB( g_print("\n[ui-transaction] update payee\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//pay = ui_pay_comboboxentry_get(GTK_COMBO_BOX(data->PO_pay));
	pay = ui_pay_entry_popover_get(GTK_BOX(data->PO_pay));
	if( pay != NULL )
	{
		// only set for empty category
		// #1635053 and also paymode unset
		// #1817278 and independently
		//cat = ui_cat_comboboxentry_get(GTK_COMBO_BOX(data->PO_cat));
		cat = ui_cat_entry_popover_get(GTK_BOX(data->PO_cat));
		if( (cat == NULL || cat->key == 0) )
		{
			DB( g_print("set cat to %d\n", pay->kcat) );
			g_signal_handlers_block_by_func (G_OBJECT (ui_cat_entry_popover_get_entry(GTK_BOX(data->PO_cat))), G_CALLBACK (deftransaction_update_warnsign), NULL);
			//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), pay->kcat);
			ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), pay->kcat);
			g_signal_handlers_unblock_by_func (G_OBJECT (ui_cat_entry_popover_get_entry(GTK_BOX(data->PO_cat))), G_CALLBACK (deftransaction_update_warnsign), NULL);
		}

		paymode = paymode_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
		if( (paymode == PAYMODE_NONE) )
		{
			DB( g_print("set paymode to %d\n", pay->paymode) );
			paymode_combo_box_set_active(GTK_COMBO_BOX(data->NU_mode), pay->paymode);
		}
	}
}


static void deftransaction_set_cheque(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Account *acc;
guint cheque;
gint type, paymode;
gchar *cheque_str;

	DB( g_print("\n[ui-transaction] set_cheque\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->action != TXN_DLG_ACTION_EDIT )
	{
		paymode = paymode_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
		if(paymode == PAYMODE_CHECK)
		{
			type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
			if( type == TXN_TYPE_EXPENSE )
			{
				//acc = ui_acc_comboboxentry_get(GTK_COMBO_BOX(data->PO_acc));
				acc = ui_acc_entry_popover_get(GTK_BOX(data->PO_acc));
				//#1410166
				if(acc != NULL)
				{
					cheque = ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cheque))==TRUE ? acc->cheque2 : acc->cheque1 );
					//#1915660 only when cheque number is set
					if( cheque > 0 )
					{
						cheque_str = g_strdup_printf("%d", cheque + 1);
						DB( g_print(" - should fill for acc %d '%s' chequenr='%s'\n", acc->key, acc->name, cheque_str) );
						gtk_entry_set_text(GTK_ENTRY(data->ST_number), cheque_str);
						g_free(cheque_str);
					}
				}
			}
			else
			if( type == TXN_TYPE_INCOME )
			{
				gtk_entry_set_text(GTK_ENTRY(data->ST_number), "");
			}

		}
	}

}


static void deftransaction_cb_accfrom_changed(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Account *srcacc;

	DB( g_print("\n[ui-transaction] accfrom change > update accto\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//srcacc = ui_acc_comboboxentry_get(GTK_COMBO_BOX(data->PO_acc));
	srcacc = ui_acc_entry_popover_get(GTK_BOX(data->PO_acc));
	if( srcacc )
	{
		//ui_acc_comboboxentry_populate_except(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, srcacc->key, ACC_LST_INSERT_NORMAL);
		ui_acc_entry_popover_populate_except(GTK_BOX(data->PO_accto), GLOBALS->h_acc, srcacc->key, ACC_LST_INSERT_NORMAL);
	}

	deftransaction_update(widget, user_data);

}


//#1673260
static gboolean deftransaction_cb_dstamount_focusout(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
struct deftransaction_data *data;
gint type;
gdouble amount;

	DB( g_print("\n[ui-transaction] dst amount focus-out-event %d\n", gtk_widget_is_focus(widget)) );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));

	// when add xfer, dst amount must be positive
	if( (type == TXN_TYPE_INTXFER) && (data->action == TXN_DLG_ACTION_ADD) )
	{
		//amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_xferamt));
		amount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_xferamt));
		if( amount < 0 )
		{
			g_signal_handlers_block_by_func(G_OBJECT(data->ST_xferamt), G_CALLBACK(deftransaction_cb_dstamount_focusout), NULL);
			//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_xferamt), ABS(amount));
			hbtk_decimal_entry_set_value(HBTK_DECIMAL_ENTRY(data->ST_xferamt), ABS(amount));
			g_signal_handlers_unblock_by_func(G_OBJECT(data->ST_xferamt), G_CALLBACK(deftransaction_cb_dstamount_focusout), NULL);
		}
	}
	return FALSE;
}

static gboolean deftransaction_cb_date_changed(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
    DB(g_print("\n[ui-transaction] date change\n") );

	struct deftransaction_data *data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	Account * sourceAccount = ui_acc_entry_popover_get(GTK_BOX(data->PO_acc));
	Account * targetAccount = ui_acc_entry_popover_get(GTK_BOX(data->PO_accto));
	if(sourceAccount != NULL && targetAccount != NULL) {
	   gchar * sourceAccountName = sourceAccount->bankname;
	   DB(g_print("Source account name: %s\n", sourceAccountName));
	   gchar * targetAccountName = targetAccount->bankname;
	   DB(g_print("Target account name: %s\n", targetAccountName));
	      if(sourceAccountName != NULL && targetAccountName != NULL && strcmp(sourceAccountName, targetAccountName) == 0) {
		    gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_dateto), gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_date)));
	        DB(g_print("Link target account date to source account date\n"));
	      }
	 }
}


static gboolean deftransaction_cb_amount_focusout(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
struct deftransaction_data *data;
gint type;
gdouble amount;
gboolean change;

	DB( g_print("\n[ui-transaction] amount focus-out-event %d\n", gtk_widget_is_focus(widget)) );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
	//amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	amount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_amount));

	change = FALSE;
	if( type == TXN_TYPE_INTXFER )
	{
		DB( g_print(" is xfer\n") );
		// for internal transfer add, amount must be expense by default
		if( data->action == TXN_DLG_ACTION_ADD && amount > 0)
			change = TRUE;
		//#2101050
		if( data->action == TXN_DLG_ACTION_EDIT && !data->isxferdst && amount > 0)
			change = TRUE;
	}
	else
	{
		DB( g_print(" is not xfer\n") );
		if( hb_amount_type_match(amount, type) == FALSE )
			change = TRUE;
	}

	if( change == TRUE )
	{
		g_signal_handlers_block_by_func(G_OBJECT(data->ST_amount), G_CALLBACK(deftransaction_cb_amount_focusout), NULL);
		DB( g_print(" force value to %.2f\n", amount * -1) );
		//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), amount * -1);
		hbtk_decimal_entry_set_value(HBTK_DECIMAL_ENTRY(data->ST_amount), amount * -1);
		g_signal_handlers_unblock_by_func(G_OBJECT(data->ST_amount), G_CALLBACK(deftransaction_cb_amount_focusout), NULL);
	}

	if( type == TXN_TYPE_INTXFER )
	{
		DB( g_print(" - call set amt xfer\n") );
		deftransaction_set_amount_xfer(data);
	}
	else
	{
		//#1872329 fill-in cheque if condition match
		DB( g_print(" - call set cheque\n") );
		deftransaction_set_cheque(widget, NULL);
	}

	DB( g_print(" - call warnsign\n") );
	deftransaction_update_warnsign(widget, NULL);

	return FALSE;
}


static void deftransaction_cb_type_toggled(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gint type;

	DB( g_print("\n[ui-transaction] type change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_type));

	DB( g_print(" type: %d\n", type ) );

	if( type == TXN_TYPE_INTXFER )
	{
		if( !(data->action == TXN_DLG_ACTION_EDIT) )
			deftransaction_cb_accfrom_changed(widget, user_data);
	}

	//#1882456
	ui_cat_entry_popover_sort_type(GTK_BOX(data->PO_cat), type);

	deftransaction_cb_amount_focusout(widget, NULL, user_data);

	deftransaction_update(widget, user_data);

}


static void deftransaction_set(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Transaction *entry;
gint type;
gchar *tagstr;

	DB( g_print("\n[ui-transaction] set\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	entry = data->ope;

	DB( g_print(" - ope=%p data=%p\n", data->ope, data) );


	//TRANSFER has priority on INCOME
	//type = TXN_TYPE_NONE;
	if(entry->flags & OF_INTXFER)
		type = TXN_TYPE_INTXFER;
	else
	{
		type = TXN_TYPE_EXPENSE;
		if(entry->flags & OF_INCOME)
			type = TXN_TYPE_INCOME;
	}

	g_signal_handlers_block_by_func(G_OBJECT(data->RA_type), G_CALLBACK(deftransaction_cb_type_toggled), NULL);
	hbtk_switcher_set_active(HBTK_SWITCHER(data->RA_type), type);
	g_signal_handlers_unblock_by_func(G_OBJECT(data->RA_type), G_CALLBACK(deftransaction_cb_type_toggled), NULL);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_date), (guint)entry->date);

	hbtk_entry_set_text(GTK_ENTRY(data->ST_memo), entry->memo);

	g_signal_handlers_block_by_func(G_OBJECT(data->ST_amount), G_CALLBACK(deftransaction_cb_amount_focusout), NULL);
	//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), entry->amount);
	hbtk_decimal_entry_set_value(HBTK_DECIMAL_ENTRY(data->ST_amount), entry->amount);
	g_signal_handlers_unblock_by_func(G_OBJECT(data->ST_amount), G_CALLBACK(deftransaction_cb_amount_focusout), NULL);

	//#1673260
	if( type == TXN_TYPE_INTXFER )
	{
		g_signal_handlers_block_by_func(G_OBJECT(data->ST_xferamt), G_CALLBACK(deftransaction_cb_dstamount_focusout), NULL);
		//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_xferamt), entry->xferamount);
		hbtk_decimal_entry_set_value(HBTK_DECIMAL_ENTRY(data->ST_xferamt), entry->xferamount);
		g_signal_handlers_unblock_by_func(G_OBJECT(data->ST_xferamt), G_CALLBACK(deftransaction_cb_dstamount_focusout), NULL);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_cheque), (entry->flags & OF_CHEQ2) ? 1 : 0);

	hbtk_entry_set_text(GTK_ENTRY(data->ST_number), entry->number);
	//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), entry->kcat);
	ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), entry->kcat);

	//#1910857 don't trigger autofill when set payee
	g_signal_handlers_block_by_func(G_OBJECT(ui_pay_entry_popover_get_entry(GTK_BOX(data->PO_pay))), G_CALLBACK(deftransaction_cb_payee_changed), NULL);
	//ui_pay_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_pay), entry->kpay);
	ui_pay_entry_popover_set_active(GTK_BOX(data->PO_pay), entry->kpay);
	g_signal_handlers_unblock_by_func(G_OBJECT(ui_pay_entry_popover_get_entry(GTK_BOX(data->PO_pay))), G_CALLBACK(deftransaction_cb_payee_changed), NULL);

	tagstr = tags_tostring(entry->tags);
	hbtk_entry_set_text(GTK_ENTRY(data->ST_tags), tagstr);
	g_free(tagstr);

	hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_status), entry->status);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_remind), (entry->flags & OF_REMIND) ? 1 : 0);

	//as we trigger an event on this
	//let's place it at the end to avoid missvalue on the trigger function
	//g_signal_handlers_block_by_func (G_OBJECT (data->PO_acc), G_CALLBACK (deftransaction_cb_accfrom_changed), NULL);
	g_signal_handlers_block_by_func (G_OBJECT (ui_acc_entry_popover_get_entry(GTK_BOX(data->PO_acc))), G_CALLBACK (deftransaction_cb_accfrom_changed), NULL);
	if( entry->kacc > 0 )
	{
		//ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_acc), entry->kacc);
		ui_acc_entry_popover_set_active(GTK_BOX(data->PO_acc), entry->kacc);
	}
	else  //1829007 set first item if only 1 account
	{
		//#1859077 >=5.3 no default account, as we should pass acckey here
		ui_acc_entry_popover_set_single(GTK_BOX(data->PO_acc));
	}
	//g_signal_handlers_unblock_by_func (G_OBJECT (data->PO_acc), G_CALLBACK (deftransaction_cb_accfrom_changed), NULL);
	g_signal_handlers_unblock_by_func (G_OBJECT (ui_acc_entry_popover_get_entry(GTK_BOX(data->PO_acc))), G_CALLBACK (deftransaction_cb_accfrom_changed), NULL);

	//ui_acc_comboboxentry_populate_except(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, entry->kacc, ACC_LST_INSERT_NORMAL);
	ui_acc_entry_popover_populate_except(GTK_BOX(data->PO_accto), GLOBALS->h_acc, entry->kacc, ACC_LST_INSERT_NORMAL);
	//ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_accto), entry->kxferacc);
	ui_acc_entry_popover_set_active(GTK_BOX(data->PO_accto), entry->kxferacc);

	paymode_combo_box_set_active(GTK_COMBO_BOX(data->NU_mode), entry->paymode);

}



static void deftransaction_cb_split_clicked(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Transaction *ope;
gdouble amount;
Account *srcacc;
gint type, nbsplit;
guint32 kcur, date;

	DB( g_print("\n[ui-transaction] cb split clicked\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ope = data->ope;

	type   = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
	date   = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_date));
	//amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	amount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_amount));
	srcacc = ui_acc_entry_popover_get(GTK_BOX(data->PO_acc));
	kcur   = (srcacc != NULL) ? srcacc->kcur : GLOBALS->kcur;


	ui_split_dialog(data->dialog, &ope->splits, type, date, amount, kcur, &deftransaction_set_amount_from_split);

	DB( g_print(" - after closed dialog\n") );
	/* old stuffs */
	//# 1419476 empty category when no split either...
	if( (ope->flags & (OF_SPLIT)) )
	{
		//# 1416624 empty category when split
		g_signal_handlers_block_by_func (G_OBJECT (data->PO_cat), G_CALLBACK (deftransaction_update_warnsign), NULL);
		//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), 0);
		ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), 0);
		g_signal_handlers_unblock_by_func (G_OBJECT (data->PO_cat), G_CALLBACK (deftransaction_update_warnsign), NULL);
	}

	//eval split to garantee disabled items
	ope->flags &= ~(OF_SPLIT);
	nbsplit = da_splits_length(ope->splits);
	if(nbsplit > 0)
		data->ope->flags |= (OF_SPLIT);

	DB( g_print(" - call update\n") );
	deftransaction_update(data->dialog, NULL);
}


static void deftransaction_paymode(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gint type, paymode;
gboolean visible;

	DB( g_print("\n[ui-transaction] paymode change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type    = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
	paymode = paymode_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));

	visible = (paymode == PAYMODE_CHECK) ? TRUE : FALSE;
	visible = (type == TXN_TYPE_INTXFER) ? FALSE : visible;
	hb_widget_visible(data->CM_cheque, visible);

	/* todo: prefill the cheque number ? */
	deftransaction_set_cheque(widget, user_data);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


void deftransaction_set_amount_from_split(GtkWidget *widget, gdouble amount)
{
struct deftransaction_data *data;
gint type;

	DB( g_print("\n[ui-transaction] set_amount_from_split\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("- amount=%.2f\n", amount) );

	//#1885413 enable sign invert from split dialog
	type = (amount < 0.0) ?	TXN_TYPE_EXPENSE : TXN_TYPE_INCOME;
	g_signal_handlers_block_by_func(data->RA_type, G_CALLBACK(deftransaction_cb_type_toggled), NULL);
	hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_type), type);
	g_signal_handlers_unblock_by_func(data->RA_type, G_CALLBACK(deftransaction_cb_type_toggled), NULL);

	data->ope->amount = amount;
	//gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), amount);
	hbtk_decimal_entry_set_value(HBTK_DECIMAL_ENTRY(data->ST_amount), amount);

	deftransaction_update(widget, NULL);
}


void deftransaction_get(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Transaction *entry;
gchar *txt;
gint type, active;

	DB( g_print("\n[ui-transaction] get\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	entry = data->ope;
	DB( g_print(" txn: %p\n", entry) );

	//if( entry == NULL )
	//	return;

	type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
	entry->flags &= ~(OF_INCOME|OF_INTXFER);
	if( type == TXN_TYPE_INCOME)
		entry->flags |= OF_INCOME;
	if( type == TXN_TYPE_INTXFER)
		entry->flags |= OF_INTXFER;
	DB( g_print(" type: %d\n", type) );

	entry->date = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_date));
	DB( hb_print_date(entry->date, " date:") );

	//#1988594 ensure amount are updated

	//entry->amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	entry->amount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_amount));
	DB( g_print(" amount: %f '%s'\n", entry->amount, gtk_entry_get_text(GTK_ENTRY(data->ST_amount))) );

	//entry->xferamount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_xferamt));
	entry->xferamount = hbtk_decimal_entry_get_value(HBTK_DECIMAL_ENTRY(data->ST_xferamt));
	DB( g_print(" xferamount %f '%s'\n", entry->xferamount, gtk_entry_get_text(GTK_ENTRY(data->ST_xferamt))) );

	//entry->kacc     = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));
	entry->kacc     = ui_acc_entry_popover_get_key(GTK_BOX(data->PO_acc));
	//entry->kxferacc = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_accto));
	entry->kxferacc = ui_acc_entry_popover_get_key(GTK_BOX(data->PO_accto));
	DB( g_print(" srcacc: %d\n", entry->kacc) );
	DB( g_print(" dstacc: %d\n", entry->kxferacc) );

	//free any previous string
	if(	entry->memo )
	{
		g_free(entry->memo);
		entry->memo = NULL;
	}
	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_memo));
	// ignore if entry is empty
	if (txt && *txt)
	{
		entry->memo = g_strdup(txt);

		//#1716182 add into memo autocomplete
		if(PREFS->txn_memoacp == TRUE)
		{
			if( da_transaction_insert_memo(entry->memo, entry->date) )
			{
			GtkEntryCompletion *completion;
			GtkTreeModel *model;
			GtkTreeIter  iter;

				DB( g_print(" add memo to completion\n") );
			completion = gtk_entry_get_completion (GTK_ENTRY(data->ST_memo));
			model = gtk_entry_completion_get_model (completion);
			gtk_list_store_insert_with_values(GTK_LIST_STORE(model), &iter, -1,
				0, txt,
				-1);
		}
	}
	}
	DB( g_print(" memo: '%s'\n", entry->memo) );

	//free any previous string
	if(	entry->number )
	{
		g_free(entry->number);
		entry->number = NULL;
	}
	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_number));
	// ignore if entry is empty
	if (txt && *txt)
	{
		entry->number = g_strdup(txt);
	}
	DB( g_print(" info: '%s'\n", entry->number) );

	entry->paymode  = paymode_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
	//entry->kcat     = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_cat));
	entry->kcat     = ui_cat_entry_popover_get_key_add_new(GTK_BOX(data->PO_cat));
	//entry->kpay     = ui_pay_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_pay));
	entry->kpay     = ui_pay_entry_popover_get_key_add_new(GTK_BOX(data->PO_pay));
	DB( g_print(" paymode: %d\n", entry->paymode) );
	DB( g_print(" cat: %d\n", entry->kcat) );
	DB( g_print(" pay: %d\n", entry->kpay) );

	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_tags));
	DB( g_print(" tags: '%s'\n", txt) );
	g_free(entry->tags);
	entry->tags = tags_parse(txt);

	//entry->status = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_status));
	entry->status = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_status));
	DB( g_print(" status: '%d'\n", entry->status) );

	entry->flags &= ~(OF_REMIND);
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_remind));
	if(active == 1)
		entry->flags |= OF_REMIND;

	// consistency checks

	//#617936/#1988594 ensure amount sign
	if( data->action == TXN_DLG_ACTION_ADD )
	{
		entry->amount = ABS(entry->amount);
		entry->xferamount = ABS(entry->xferamount);
		if( (type == TXN_TYPE_EXPENSE) || (type == TXN_TYPE_INTXFER) )
		{
			entry->amount = -entry->amount;
		}
	}

	//#1615245: moved here, after get combo entry key
	//if( entry->paymode != PAYMODE_INTXFER )
	if( !(entry->flags & OF_INTXFER) )
	{
		//#677351: revert kxferacc to 0
		entry->kxferacc = 0;
	}
	else
		entry->paymode = PAYMODE_NONE;

	/* flags */
	//entry->flags = 0;
	entry->flags &= ~(OF_SPLIT|OF_ADVXFER|OF_CHEQ2);	//remove existing flags

	//1859117: keep the split flag
	if( da_splits_length (entry->splits) > 0 )
		entry->flags |= OF_SPLIT; //Flag that Splits are active

	if(	data->action == TXN_DLG_ACTION_ADD || data->action == TXN_DLG_ACTION_INHERIT)
		entry->dspflags |= FLAG_TMP_ADDED;

	if(	data->action == TXN_DLG_ACTION_EDIT)
		entry->dspflags |= FLAG_TMP_EDITED;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cheque));
	if(active == 1)
		entry->flags |= OF_CHEQ2;

	//#1673260
	if( type == TXN_TYPE_INTXFER )
	{
	Account *srcacc, *dstacc;

		srcacc = da_acc_get(entry->kacc);
		dstacc = da_acc_get(entry->kxferacc);
		if( srcacc && dstacc )
		{
			if( srcacc->kcur != dstacc->kcur )
			{
				DB( g_print(" get OF_ADVXFER\n") );
				entry->flags |= OF_ADVXFER;
			}
		}
	}
	else
		entry->xferamount = 0;

	//active = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_amount));
	//we keep this in case the user want to force the type
	da_transaction_set_flag(entry);

}


static gboolean confirm_handler(struct deftransaction_data *data)
{
	DB( g_print("\n[ui-transaction] hide confirm text\n") );

	//#1859088 normally dispose remove the event source, so we don't land here after timeout
	hb_widget_visible(data->LB_msgadded, FALSE);

	data->evtsrcid = 0;

	return FALSE;
}


void deftransaction_external_confirm(GtkWidget *dialog, Transaction *ope)
{
struct deftransaction_data *data;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gchar *txt;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(dialog, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[ui-transaction] display confirm text\n") );

	if(data->evtsrcid > 0 )
		return;

	hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, ope->amount, ope->kcur, GLOBALS->minor);
	txt = g_strdup_printf(_("Transaction of %s created."), buf);
	gtk_label_set_text(GTK_LABEL(data->LB_msgadded), txt);
	g_free(txt);
	hb_widget_visible(data->LB_msgadded, TRUE);
	data->evtsrcid = g_timeout_add(5000, (GSourceFunc) confirm_handler, (gpointer) data);
}


/*
** called from outside (ledger/report detail)
*/
gint deftransaction_external_edit(GtkWindow *parent, Transaction *old_txn, Transaction *new_txn)
{
struct deftransaction_data *data;
GtkWidget *dialog;
gboolean result, accchanged;
Transaction *child;
Account *acc;

	DB( g_print("\n------------------------\n") );
	DB( g_print("\n[ui-transaction] external edit (from out)\n") );

	dialog = create_deftransaction_window(GTK_WINDOW(parent), TXN_DLG_ACTION_EDIT, TXN_DLG_TYPE_TXN, 0);

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(dialog, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" -- test xfer\n") );
	//5.7 test if xfer src or dst
	// and disable expense or income as well to avoid mistake
	data->isxferdst = FALSE;
	child = NULL;
	if( old_txn->flags & OF_INTXFER )
	{
		//use old in case of dst_acc change
		child = transaction_xfer_child_strong_get(old_txn);

		if( old_txn->amount < 0 )
		{
			//disable income
			hbtk_switcher_set_nth_sensitive(HBTK_SWITCHER(data->RA_type), 1, FALSE);
		}

		if( old_txn->amount > 0 )
		{
			data->isxferdst = TRUE;
			//disable expense
			hbtk_switcher_set_nth_sensitive(HBTK_SWITCHER(data->RA_type), 0, FALSE);
		}

		//#1867979 todate
		if( PREFS->xfer_syncdate == FALSE )
			gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_dateto), (guint)child->date);
	}

	DB( g_print(" xfer is target: %d\n", data->isxferdst) );

	deftransaction_set_transaction(dialog, new_txn);

	DB( g_print(" ** dialog run **\n") );

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	DB( g_print(" ** dialog ended :: result=%d**\n", result) );

	accchanged = FALSE;

	if(result == GTK_RESPONSE_ACCEPT)
	{
		deftransaction_get(dialog, NULL);

		account_balances_sub(old_txn);
		account_balances_add(new_txn);

		accchanged = TRUE;
		/* ok different case here

			* new is intxfer
				a) old was not
					check for existing child or add it
				b) old was
					sync (acc change is inside now)

			* new is not intxfer
				a) old was
					manage break intxfer

			* always manage account change
		*/

		acc = da_acc_get(new_txn->kacc);

		//#1931816: sort if date changed
		if(old_txn->date != new_txn->date)
		{
			da_transaction_queue_sort(acc->txn_queue);
			accchanged = TRUE;
		}

		//if( new_txn->paymode == PAYMODE_INTXFER )
		if( new_txn->flags & OF_INTXFER )
		{
			// change to an internal xfer
			if( !(old_txn->flags & OF_INTXFER) )
			{
			gint tmpxferresult;
				// this call can popup a user dialog to choose
				tmpxferresult = transaction_xfer_search_or_add_child(GTK_WINDOW(dialog), FALSE, new_txn, new_txn->kxferacc);
				if( tmpxferresult == GTK_RESPONSE_CANCEL )
					accchanged = FALSE;
			}
			else	// just sync the existing xfer
			{
				//#1584342 was faultly old_txn
				transaction_xfer_child_sync(new_txn, child);
				//#1867979 todate
				if( PREFS->xfer_syncdate == FALSE )
					child->date = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_dateto));

				accchanged = TRUE;
			}
		}
		else
		{
			//#1250061 : manage ability to break an internal xfer
			if(old_txn->flags & OF_INTXFER)
			{
			gint break_result;

				DB( g_print(" - should break internal xfer\n") );

				break_result = ui_dialog_msg_confirm_alert(
						GTK_WINDOW(parent),
						NULL,
						_("Do you want to break the internal transfer?\n\n"
						  "Proceeding will delete the target transaction."),
						_("_Break"),
						TRUE
					);

				if(break_result == GTK_RESPONSE_OK)
				{
					//we must use old_txn to ensure get the child
					//#1663789 but we must clean new as well
					transaction_xfer_remove_child(old_txn);
					transaction_xfer_change_to_normal(new_txn);
					accchanged = TRUE;
				}
				else	//force paymode to internal xfer
				{
					//new_txn->paymode = PAYMODE_INTXFER;
					new_txn->flags |= OF_INTXFER;
				}
			}
		}

		//#1638035: manage account change
		if( old_txn->kacc != new_txn->kacc )
		{
			//todo: maybe we should restrict this also to same currency account
			//=> no pb for normal, and intxfer is restricted by ui (in theory)
			transaction_acc_move(new_txn, old_txn->kacc, new_txn->kacc);
			accchanged = TRUE;
		}

		//#1581863 store reconciled date
		if( (old_txn->status != new_txn->status) && (new_txn->status == TXN_STATUS_RECONCILED) )
		{
			if(acc)
				acc->rdate = GLOBALS->today;
		}
	}
	//#1883403 we may have changed the split amount but if we cancel it must be restored
	else
	{
		if( old_txn->flags & OF_SPLIT )
			new_txn->amount = old_txn->amount;
	}

	/* update account flag */
	if( accchanged == TRUE )
	{
		DB( g_print(" mark acc as changed\n") );
		acc = da_acc_get(new_txn->kacc);
		if(acc)
			acc->dspflags |= FLAG_ACC_TMP_EDITED;
	}

	deftransaction_dispose(dialog, NULL);
	gtk_window_destroy (GTK_WINDOW(dialog));

	return result;
}


void deftransaction_set_transaction(GtkWidget *widget, Transaction *ope)
{
struct deftransaction_data *data;

	DB( g_print("\n[ui-transaction] set transaction (from out)\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//if( ope == NULL )
	//	return;

	data->ope = ope;

	DB( g_print(" - ope=%p data=%p\n", data->ope, data) );

	DB( g_print(" - call set\n") );
	deftransaction_set(widget, NULL);

	DB( g_print(" - call update\n") );
	deftransaction_update(widget, NULL);

	DB( g_print(" - call warnsign\n") );
	deftransaction_update_warnsign(widget, NULL);

}

// end external call

static void deftransaction_setup(struct deftransaction_data *data)
{

	DB( g_print("\n[ui-transaction] setup\n") );

	//DB( g_print(" init data\n") );


	DB( g_print(" populate\n") );

	if( data->showtemplate )
	{
		ui_popover_tpl_populate (data, GLOBALS->arc_list);
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(data->modelfilter));
	}

	//ui_acc_comboboxentry_populate(GTK_COMBO_BOX(data->PO_acc), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);
	ui_acc_entry_popover_populate(GTK_BOX(data->PO_acc), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);
	//5.3 it seems no need to populate @init
	//ui_acc_comboboxentry_populate(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);
	//ui_acc_entry_popover_populate(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);

	//5.3 done in popover
	//ui_cat_comboboxentry_populate(GTK_COMBO_BOX(data->PO_cat), GLOBALS->h_cat);

	//5.2.7 done in popover
	//ui_pay_comboboxentry_populate(GTK_COMBO_BOX(data->PO_pay), GLOBALS->h_pay);
	//ui_tag_combobox_populate(GTK_COMBO_BOX_TEXT(data->CY_tags));

	//DB( g_print(" set widgets default\n") );

	//DB( g_print(" connect widgets signals\n") );

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void ui_popover_tpl_onRowActivated(GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct deftransaction_data *data;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");
	model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
	Archive *arc;
	Transaction *txn;

		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, LST_DSPTPL_DATAS, &arc, -1);

		txn = data->ope;
		da_transaction_init_from_template(txn, arc);

		DB( g_print(" calls\n") );

		deftransaction_set(GTK_WIDGET(treeview), NULL);
		deftransaction_paymode(GTK_WIDGET(treeview), NULL);
		deftransaction_update(GTK_WIDGET(treeview), NULL);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(data->MB_template), FALSE);
	}
}


static gint
ui_popover_tpl_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint retval = 0;
Archive *entry1, *entry2;

    gtk_tree_model_get(model, a, LST_DSPTPL_DATAS, &entry1, -1);
    gtk_tree_model_get(model, b, LST_DSPTPL_DATAS, &entry2, -1);

	retval = hb_string_utf8_compare(entry1->memo, entry2->memo);

    return retval;
}


static void ui_popover_tpl_populate(struct deftransaction_data *data, GList *srclist)
{
GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
GString *tpltitle;

	//insert all glist item into treeview
	model  = data->model;
	gtk_list_store_clear(GTK_LIST_STORE(model));

	tpltitle = g_string_sized_new(255);

	list = g_list_first(srclist);
	while (list != NULL)
	{
	Archive *item = list->data;

		//#1968249 build a non empty label, when memo/payee/category are empty
		da_archive_get_display_label(tpltitle, item);

		gtk_list_store_insert_with_values(GTK_LIST_STORE(model), &iter, -1,
			LST_DSPTPL_DATAS, item,
			LST_DSPTPL_NAME, tpltitle->str,
				-1);

		//DB( g_print(" populate_treeview: %d %08x\n", i, list->data) );

		list = g_list_next(list);
	}

	g_string_free(tpltitle, TRUE);
}


static void
ui_popover_tpl_refilter (GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data = user_data;

	DB( g_print(" text changed\n") );

	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(data->modelfilter));
}


static gboolean ui_popover_tpl_func_visible (GtkTreeModel *model, GtkTreeIter  *iter, gpointer user_data)
{
struct deftransaction_data *data = user_data;
Archive *entry;
gchar *str;
gboolean visible = TRUE;
gboolean showsched;
gboolean showallacc;

	showsched  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(data->CM_showsched));
	showallacc = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(data->CM_showallacc));

	gchar *needle = g_ascii_strdown(gtk_entry_get_text(GTK_ENTRY(data->ST_search)), -1);

	gtk_tree_model_get (model, iter,
		LST_DSPTPL_DATAS, &entry,
		LST_DSPTPL_NAME, &str, -1);

	if( entry )
	{
		if( !showallacc && (data->kacc != 0) && (entry->kacc != data->kacc) )
			visible = FALSE;
		else
		{
			if( (entry->rec_flags & TF_RECUR) && !showsched)
			{
				visible = FALSE;
			}
			else
			{
				gchar *haystack = g_ascii_strdown(str, -1);

				if (str && g_strrstr (haystack, needle) == NULL )
				{
					visible = FALSE;
				}

				DB( g_print("filter: '%s' '%s' %d\n", str, needle, visible) );

				g_free(haystack);
			}
		}
	}
	g_free(needle);
	g_free (str);

	return visible;
}


static GtkWidget *ui_popover_tpl_create(struct deftransaction_data *data)
{
GtkListStore *store;
GtkCellRenderer *renderer;
GtkTreeViewColumn *column;
GtkWidget *box, *widget, *scrollwin, *treeview;

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	widget = make_search();
	data->ST_search = widget;
	gtk_box_prepend (GTK_BOX(box), widget);


	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	hbtk_box_prepend (GTK_BOX(box), scrollwin);

	store = gtk_list_store_new(NUM_LST_DSPTPL,
		G_TYPE_POINTER,
		G_TYPE_STRING);

	data->model = GTK_TREE_MODEL(store);

	//#1865361 txn dialog template list switch sort order on each save
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_popover_tpl_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	data->modelfilter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(data->model), NULL));
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(data->modelfilter), ui_popover_tpl_func_visible, data, NULL);


	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(data->modelfilter));
	data->LV_arc = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	gtk_widget_grab_focus(treeview);

	/* column for bug numbers */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
		                                             renderer,
		                                             "text",
		                                             LST_DSPTPL_NAME,
		                                             NULL);
	//gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), FALSE);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	widget = gtk_check_button_new_with_mnemonic(_("Show _scheduled"));
	data->CM_showsched = widget;
	gtk_box_prepend (GTK_BOX(box), widget);

	widget = gtk_check_button_new_with_mnemonic(_("Show _all accounts"));
	data->CM_showallacc = widget;
	gtk_box_prepend (GTK_BOX(box), widget);

	gtk_widget_show_all (box);

	//#1796564 hide show all template if no account
	gtk_widget_set_visible (data->CM_showallacc, data->kacc == 0 ? FALSE : TRUE);

	//signals
	g_signal_connect (data->CM_showsched, "toggled", G_CALLBACK (ui_popover_tpl_refilter), data);
	g_signal_connect (data->CM_showallacc, "toggled", G_CALLBACK (ui_popover_tpl_refilter), data);
	g_signal_connect (data->ST_search, "search-changed", G_CALLBACK (ui_popover_tpl_refilter), data);

	return box;
}



static GtkWidget *deftransaction_create_template(struct deftransaction_data *data)
{
GtkWidget *box, *menubutton, *image;

	menubutton = gtk_menu_button_new ();
	data->MB_template = menubutton;
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	image = hbtk_image_new_from_icon_name_16 ("pan-down-symbolic");
	gtk_box_prepend (GTK_BOX(box), image);
	gtk_container_add(GTK_CONTAINER(menubutton), box);
	gtk_widget_set_tooltip_text(menubutton, _("Use a template"));

	gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_DOWN );
	gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
	gtk_widget_show_all(menubutton);

	GtkWidget *template = ui_popover_tpl_create(data);
	GtkWidget *popover = create_popover (menubutton, template, GTK_POS_BOTTOM);
	gtk_widget_set_size_request (popover, 2*HB_MINWIDTH_LIST, PHI*HB_MINHEIGHT_LIST);

	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);

	g_signal_connect (GTK_TREE_VIEW(data->LV_arc), "row-activated", G_CALLBACK (ui_popover_tpl_onRowActivated), NULL);

	return menubutton;
}


static gboolean
deftransaction_getgeometry(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
struct WinGeometry *wg;

	DB( g_print("\n[ui-transaction] get geometry\n") );

	//store size
	wg = &PREFS->txn_wg;
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, NULL);

	DB( g_print(" window: w=%d\n", wg->w) );

	return FALSE;
}


//#1681532 free in destroy event
static void deftransaction_cb_destroy(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;

	DB( g_print("\n[ui-transaction] --destroy cb\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" free data\n") );
	g_free(data);
}


//note: this is called from external usage
void deftransaction_dispose(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;

	DB( g_print("\n[ui-transaction] dispose\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#1859088 remove any pending event source (g_timeout_add)
	if(data->evtsrcid > 0)
		g_source_remove(data->evtsrcid);

	deftransaction_getgeometry(data->dialog, NULL, data);

	//#1681532 don't free here
	//g_free(data);
}


GtkWidget *create_deftransaction_window (GtkWindow *parent, HbTxnDlgAction action, HbTxnDlgType type, guint32 kacc)
{
struct deftransaction_data *data;
struct WinGeometry *wg;
GtkWidget *dialog, *content;
GtkWidget *bar;
GtkWidget *group_grid, *hbox, *label, *widget;
gchar *title;
gint row;

	DB( g_print("\n[ui-transaction] new\n") );

	data = g_malloc0(sizeof(struct deftransaction_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new();
	gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(parent));

	//store our window private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" - window=%p, inst_data=%p\n", dialog, data) );

	data->dialog = dialog;
	data->action = action;
	data->type = type;
	data->kacc = kacc;

	// if you add/delete response_id also change into deftransaction_update
	if(action == TXN_DLG_ACTION_EDIT)
	{
		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
		    _("_Cancel"),	GTK_RESPONSE_REJECT,
			_("_OK"),		GTK_RESPONSE_ACCEPT,
			NULL);
	}
	else	//ADD_INHERIT
	{
		gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Close"), GTK_RESPONSE_REJECT);
		if( type == TXN_DLG_TYPE_TXN )
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Add & _Keep"),	HB_RESPONSE_ADDKEEP);
		if( !(type == TXN_DLG_TYPE_SCH) )
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Add"), HB_RESPONSE_ADD);
		else
			gtk_dialog_add_button (GTK_DIALOG(dialog), _("_Post"), HB_RESPONSE_ADD);
	}

	title = NULL;
	switch(action)
	{
		case TXN_DLG_ACTION_NONE:
			title = NULL;
			break;
		case TXN_DLG_ACTION_ADD:
			title = _("Add transaction");
			if(type == TXN_DLG_TYPE_TPL) { title = _("Add template"); }
			else if(type == TXN_DLG_TYPE_SCH) { title = _("Post scheduled"); }
			break;
		case TXN_DLG_ACTION_INHERIT:
			title = _("Inherit transaction");
			if(type == TXN_DLG_TYPE_TPL)
				title = _("Inherit template");
			break;
		case TXN_DLG_ACTION_EDIT:
			title = _("Edit transaction");
			if(type == TXN_DLG_TYPE_TPL)
				title = _("Edit template");
			break;
	}

	gtk_window_set_title (GTK_WINDOW(dialog), title);

	DB( g_print(" action:%d '%s', dlgtype:%d \n", action, title, type ) );

	//gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);

	//dialog contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	//group main
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	hb_widget_set_margin(GTK_WIDGET(group_grid), SPACING_LARGE);
	gtk_box_prepend (GTK_BOX (content), group_grid);

	row=0;
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup (HBTK_SWITCHER(widget), CYA_TXN_TYPE, TRUE);
	data->RA_type = widget;
	gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 0, row, 5, 1);
	gtk_widget_set_margin_bottom(widget, SPACING_MEDIUM);

	data->showtemplate = FALSE;
	if( (data->type == TXN_DLG_TYPE_TXN) && (da_archive_length() > 0) )
	{
		if( (data->action != TXN_DLG_ACTION_EDIT) || PREFS->txn_showtemplate )
		{
			data->showtemplate = TRUE;
			widget = deftransaction_create_template(data);
			gtk_widget_set_halign (widget, GTK_ALIGN_END);
			gtk_grid_attach (GTK_GRID (group_grid), widget, 4, row, 1, 1);
			gtk_widget_set_margin_bottom(widget, SPACING_MEDIUM);
		}
	}

	row++;
	label = make_label_widget(_("_Date:"));
	data->LB_date = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	data->GR_date = hbox;
	//gtk_widget_set_hexpand(hbox, FALSE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 4, 1);

		widget = gtk_date_entry_new(label);
		data->PO_date = widget;
		//gtk_widget_set_halign(widget, GTK_ALIGN_START);
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
		//gtk_widget_set_tooltip_text(widget, _("Date accepted here are:\nday,\nday/month or month/day,\nand complete date into your locale"));
		gtk_widget_set_tooltip_text(widget, _("- type: d, d/m, m/d a complete date\n- use arrow key + ctrl or shift\n- empty for today"));
		gtk_box_prepend (GTK_BOX (hbox), widget);

	//5.8 xfer date
	if( PREFS->xfer_syncdate == FALSE && data->action == TXN_DLG_ACTION_EDIT && data->type == TXN_DLG_TYPE_TXN )
	{
		row++;
		label = make_label_widget(_("T_o:"));
		data->LB_dateto = label;
		gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
		widget = gtk_date_entry_new(label);
		gtk_widget_set_halign(widget, GTK_ALIGN_START);
		data->PO_dateto = widget;
		gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 3, 1);
	}

	row++;
	label = make_label_widget(_("Amou_nt:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	//gtk_widget_set_hexpand(hbox, FALSE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 4, 1);

		//widget = make_amount(label);
		widget = hbtk_decimal_entry_new(label);
		data->ST_amount = widget;
		//gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, ICONNAME_HB_TOGGLE_SIGN);
		//gtk_entry_set_icon_tooltip_text(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, _("Toggle amount sign"));
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = make_image_button(ICONNAME_HB_BUTTON_SPLIT, _("Transaction splits"));
		data->BT_split = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		label = make_label(NULL, 0, 0.5);
		data->LB_curr = label;
		gtk_widget_set_margin_start(label, SPACING_SMALL);
		gtk_box_prepend (GTK_BOX (hbox), label);


	//#1673260
	row++;
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	//gtk_widget_set_hexpand(hbox, FALSE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 3, 1);

		//widget = make_amount(label);
		widget = hbtk_decimal_entry_new(label);
		data->ST_xferamt = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		label = make_label(NULL, 0, 0.5);
		data->LB_xfercurr = label;
		//gtk_widget_set_margin_start(label, SPACING_SMALL);
		gtk_box_prepend (GTK_BOX (hbox), label);

		widget = hbtk_image_new_from_icon_name_16(ICONNAME_WARNING);
		data->IM_xfernorate = widget;
		gtk_widget_set_tooltip_text (widget, _("No rate available to auto fill"));
		gtk_box_prepend (GTK_BOX (hbox), widget);


	/*row++;
	label = make_label_widget(_("A_ccount:"));
	data->LB_accfrom = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_acc_comboboxentry_new(label);
	data->PO_acc = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);*/

	row++;
	label = make_label_widget(_("A_ccount:"));
	data->LB_accfrom = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_acc_entry_popover_new(label);
	data->PO_acc = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 4, 1);

	gtk_widget_set_margin_top(label, SPACING_MEDIUM);
	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);

	/*row++;
	label = make_label_widget(_("T_o:"));
	data->LB_accto = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_acc_comboboxentry_new(label);
	data->PO_accto = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);*/

	row++;
	label = make_label_widget(_("T_o:"));
	data->LB_accto = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_acc_entry_popover_new(label);
	data->PO_accto = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 3, 1);


	row++;
	label = make_label_widget(_("Pa_yment:"));
	data->LB_mode = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_paymode(label);
	data->NU_mode = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);
	widget = gtk_check_button_new_with_mnemonic(_("Book _2"));
	data->CM_cheque = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 3, row, 2, 1);


	row++;
	label = make_label_widget(_("_Number:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_string(label);
	data->ST_number = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	label = make_label_widget(_("_Payee:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_widget_set_hexpand(hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 4, 1);

		widget = ui_pay_entry_popover_new(label);
		data->PO_pay = widget;
		gtk_widget_set_hexpand(widget, TRUE);
		hbtk_box_prepend (GTK_BOX (hbox), widget);
		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
		gtk_widget_set_tooltip_text(widget, _("- type some letter for autocompletion\n- type new text to create entry"));
		gtk_box_prepend (GTK_BOX (hbox), widget);

	gtk_widget_set_margin_top(label, SPACING_MEDIUM);
	gtk_widget_set_margin_top(hbox, SPACING_MEDIUM);


	row++;
	label = make_label_widget(_("Cate_gory:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_widget_set_hexpand(hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 4, 1);

		//widget = ui_cat_comboboxentry_new(label);
		widget = ui_cat_entry_popover_new(label);
		data->PO_cat = widget;
		hbtk_box_prepend (GTK_BOX (hbox), widget);
		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
		//gtk_widget_set_tooltip_text(widget, _("Autocompletion and direct seizure\nis available"));
		gtk_widget_set_tooltip_text(widget, _("- type some letter for autocompletion\n- type new text to create entry"));
		gtk_box_prepend (GTK_BOX (hbox), widget);

	/*
	row++;
	label = make_label_widget(_("_Status:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = hbtk_combo_box_new_with_data (label, CYA_TXN_STATUS);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	data->CY_status = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);
	*/

	//#1847622 transaction editor: "Status drop" down menu
	row++;
	label = make_label_widget(_("_Status:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_widget_set_hexpand(hbox, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 4, 1);

		widget = hbtk_switcher_new(GTK_ORIENTATION_HORIZONTAL);
		hbtk_switcher_setup_with_data(HBTK_SWITCHER(widget), label, CYA_TXN_STATUSIMG, TRUE);
		data->RA_status = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = make_image_toggle_button(ICONNAME_HB_ITEM_REMIND, _("Remind"));
		data->CM_remind = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

	gtk_widget_set_margin_top(label, SPACING_MEDIUM);
	gtk_widget_set_margin_top(hbox, SPACING_MEDIUM);

	row++;
	label = make_label_widget(_("M_emo:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_memo_entry(label);
	data->ST_memo = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 4, 1);

	row++;
	label = make_label_widget(_("_Tags:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(hbox)), GTK_STYLE_CLASS_LINKED);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 4, 1);

		widget = make_string(label);
		data->ST_tags = widget;
		hbtk_box_prepend (GTK_BOX (hbox), widget);

		widget = ui_tag_popover_list(data->ST_tags);
		data->CY_tags = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

	DB( g_print(" -- showall\n") );
	gtk_widget_show_all(group_grid);

	row++;
	bar = gtk_info_bar_new ();
	data->IB_warnsign = bar;
	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	label = gtk_label_new (_("Warning: amount and category sign don't match"));
	hbtk_box_prepend (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label);
	gtk_grid_attach (GTK_GRID (group_grid), bar, 1, row, 4, 1);

	//#1831975 visual add confirmation
	row++;
	label = gtk_label_new(NULL);
	data->LB_msgadded = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 4, 1);

	//setup, init and show window
	deftransaction_setup(data);

	wg = &PREFS->txn_wg;
	gtk_window_set_default_size(GTK_WINDOW(dialog), wg->w, -1);

	DB( g_print(" -- signal\n") );

	// connect dialog signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (deftransaction_cb_destroy), NULL);

	//debug signal not to release
	//g_signal_connect (dialog, "configure-event", G_CALLBACK (deftransaction_getgeometry), (gpointer)data);

	g_signal_connect (data->RA_type  , "changed", G_CALLBACK (deftransaction_cb_type_toggled), NULL);

	g_signal_connect (data->PO_date, "changed", G_CALLBACK (deftransaction_cb_date_changed), NULL);

	//g_signal_connect (data->PO_acc  , "changed", G_CALLBACK (deftransaction_cb_accfrom_changed), NULL);
	g_signal_connect (ui_acc_entry_popover_get_entry(GTK_BOX(data->PO_acc)), "changed", G_CALLBACK (deftransaction_cb_accfrom_changed), NULL);
	//g_signal_connect (data->PO_accto, "changed", G_CALLBACK (deftransaction_update), NULL);
	g_signal_connect (ui_acc_entry_popover_get_entry(GTK_BOX(data->PO_accto)), "changed", G_CALLBACK (deftransaction_update), NULL);

	g_signal_connect_after (data->ST_amount, "value-changed", G_CALLBACK (deftransaction_cb_amount_focusout), NULL);
	g_signal_connect_after (data->ST_amount, "focus-out-event", G_CALLBACK (deftransaction_cb_amount_focusout), NULL);

	g_signal_connect (data->BT_split, "clicked", G_CALLBACK (deftransaction_cb_split_clicked), NULL);

	//#1673260
	g_signal_connect_after (data->ST_xferamt, "value-changed", G_CALLBACK (deftransaction_cb_dstamount_focusout), NULL);
	g_signal_connect_after (data->ST_xferamt, "focus-out-event", G_CALLBACK (deftransaction_cb_dstamount_focusout), NULL);


	g_signal_connect (data->NU_mode  , "changed", G_CALLBACK (deftransaction_paymode), NULL);
	g_signal_connect (data->CM_cheque, "toggled", G_CALLBACK (deftransaction_paymode), NULL);

	//g_signal_connect (data->PO_pay  , "changed", G_CALLBACK (deftransaction_cb_payee_changed), NULL);
	g_signal_connect (ui_pay_entry_popover_get_entry(GTK_BOX(data->PO_pay)), "changed", G_CALLBACK (deftransaction_cb_payee_changed), NULL);
	//g_signal_connect (data->PO_cat  , "changed", G_CALLBACK (deftransaction_update_warnsign), NULL);
	g_signal_connect (ui_cat_entry_popover_get_entry(GTK_BOX(data->PO_cat)), "changed", G_CALLBACK (deftransaction_update_warnsign), NULL);

	DB( g_print(" -- return\n") );

	return dialog;
}

