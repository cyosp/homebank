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

#ifndef __LIST_REPORT__H__
#define __LIST_REPORT__H__




enum
{
	LST_REPDIST_POS,	//keep for compatibility with chart
	LST_REPDIST_KEY,
	LST_REPDIST_NAME,
	LST_REPDIST_EXPENSE,
	LST_REPDIST_EXPRATE,
	LST_REPDIST_INCOME,
	LST_REPDIST_INCRATE,
	LST_REPDIST_TOTAL,
	LST_REPDIST_TOTRATE,
	NUM_LST_REPDIST
};

#define LST_REPDIST_POS_TOTAL G_MAXINT

//test
enum {
	LST_REPDIST2_POS,
	LST_REPDIST2_KEY,
	LST_REPDIST2_LABEL,
	LST_REPDIST2_ROW,
	NUM_LST_REPDIST2
};


GtkWidget *lst_rep_total_create(void);
GString *lst_rep_total_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard);


GString *lst_rep_time_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard);
GtkWidget *lst_rep_time_createtype(GtkListStore *store);
GtkWidget *lst_rep_time_create(void);
void lst_rep_time_renewcol(GtkTreeView *treeview, guint32 nbintvl, guint32 jfrom, gint intvl, gboolean avg);


#endif