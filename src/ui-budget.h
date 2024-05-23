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

#ifndef __HB_BUDGET_GTK_H__
#define __HB_BUDGET_GTK_H__

enum
{
  COL_NAME = 0,
  COL_OLDINDEX,
  NUM_COLS
};


#define FIELD_TYPE 15

struct ui_bud_manage_data
{
	GList		*tmp_list;
	gint		change;
	Category	*lastcatitem;

	GtkWidget	*dialog;
	GActionGroup * actions;

	gboolean	mapped_done;

	GtkWidget	*LV_cat;
	GtkWidget   *BT_expand;
	GtkWidget   *BT_collapse;

	GtkWidget	*RA_type;
	GtkWidget   *label_budget;
	GtkWidget	*CM_type[2];
	GtkWidget	*label[13];	//0 index is for All (not displayed)
	GtkWidget	*spinner[13];	//0 index is for All
	GtkWidget   *label_options;
	GtkWidget	*CM_force;

	GtkWidget	*TX_totexp, *TX_totinc, *TX_totbal;
	
	GtkWidget	*BT_clear;

	Category	*cat;

	gdouble totexp, totinc;

};




GtkWidget *ui_bud_manage_dialog (void);

#endif
