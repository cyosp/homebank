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

#ifndef __LIST_REPORT__H__
#define __LIST_REPORT__H__


enum
{
	LST_REPORT_POS,	//keep for compatibility with chart
	LST_REPORT_KEY,
	LST_REPORT_NAME,
	//LST_REPORT_ROW,
	LST_REPORT_EXPENSE,
	LST_REPORT_INCOME,
	LST_REPORT_TOTAL,
	NUM_LST_REPORT
};

#define LST_REPORT_POS_TOTAL	G_MAXINT


// --- time stuff ---

//time maximum column (4 years = 365,25 * 4)
#define LST_REP_COLID_MAX		1461

//special column id
#define LST_REP_COLID_POS     LST_REP_COLID_MAX + 10
#define LST_REP_COLID_AVERAGE LST_REP_COLID_MAX + 11
#define LST_REP_COLID_TOTAL   LST_REP_COLID_MAX + 12


enum {
	LST_REPORT2_POS,
	LST_REPORT2_KEY,
	LST_REPORT2_LABEL,
	LST_REPORT2_ROW,
	NUM_LST_REPORT2
};


struct lst_report_data
{
	GtkWidget	*treeview;

//	guint		intvl;
//	guint		n_cols;
//	DataCol		**cols;
	gdouble		tot_exp;
	gdouble		tot_inc;
	
};



GtkTreeStore *lst_report_new(void);
GtkWidget *lst_report_create(void);
gboolean lst_report_get_top_level (GtkTreeModel *liststore, guint32 key, GtkTreeIter *return_iter);
GString *lst_report_to_string(GtkTreeView *treeview, gint src, gchar *title, gboolean clipboard);


gboolean lst_rep_time_get_top_level (GtkTreeModel *liststore, guint32 key, GtkTreeIter *return_iter);
GString *lst_rep_time_to_string(GtkTreeView *treeview, gint src, gchar *title, gboolean clipboard);
GtkWidget *lst_rep_time_createtype(GtkListStore *store);
GtkTreeStore *lst_rep_time_new(void);
GtkWidget *lst_rep_time_create(void);
void lst_rep_time_renewcol(GtkTreeView *treeview, GtkTreeModel *model, DataTable *dt, gboolean avg);


#endif
