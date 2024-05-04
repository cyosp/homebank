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

#ifndef __LIST_UPCOMING__H__
#define __LIST_UPCOMING__H__


enum
{
	LST_DSPUPC_DATAS,
	LST_DSPUPC_NEXT,
	LST_DSPUPC_MEMO,
	LST_DSPUPC_EXPENSE,
	LST_DSPUPC_INCOME,
	LST_DSPUPC_NB_LATE,
	NUM_LST_DSPUPC
};


enum
{
	COL_DSPUPC_LATE = 1,
	COL_DSPUPC_STILL,
	COL_DSPUPC_NEXTDATE,
	COL_DSPUPC_PAYEE,
	COL_DSPUPC_CATEGORY,
	COL_DSPUPC_MEMO,
	COL_DSPUPC_EXPENSE,
	COL_DSPUPC_INCOME,
	COL_DSPUPC_ACCOUNT,
	NUM_LST_COL_DSPUPC
};


struct lst_sch_data
{
	GtkWidget	*treeview;
	GtkWidget	*menu;
};


GtkWidget *lst_sch_widget_new(void);


#endif
