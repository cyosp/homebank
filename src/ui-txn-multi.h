/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2025 Maxime DOYEN
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

#ifndef __UI_TXN_MULTI_H__
#define __UI_TXN_MULTI_H__


struct ui_multipleedit_dialog_data
{
	GtkWidget	*dialog;

	GtkWidget	*CM_date, *PO_date;
	GtkWidget	*CM_amount, *ST_amount;
	GtkWidget	*LB_acc, *CM_acc, *PO_acc;
	GtkWidget	*LB_mode, *CM_mode, *NU_mode;
	GtkWidget	*CM_number, *ST_number;
	GtkWidget	*CM_pay, *PO_pay;
	GtkWidget	*CM_cat, *PO_cat;
	GtkWidget	*CM_memo, *ST_memo;
	GtkWidget	*CM_tags, *ST_tags, *CY_tags;
	GtkWidget	*CM_xfer, *PO_accto;
	 
	GtkTreeView	*treeview;
	guint32		kacc;
	gboolean	has_xfer;
};

void ui_multipleedit_dialog_prefill( GtkWidget *widget, Transaction *ope, gint column_id );
gint ui_multipleedit_dialog_apply( GtkWidget *widget, gboolean *do_sort );
GtkWidget *ui_multipleedit_dialog_new(GtkWindow *parent, GtkTreeView *treeview);


#endif /* __UI_TXN_MULTI_H__ */
