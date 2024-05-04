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

#ifndef __HB_ARCHIVE_GTK_H__
#define __HB_ARCHIVE_GTK_H__


enum {
	LST_DEFARC_SORT_DATE = 1,
	LST_DEFARC_SORT_MEMO,
	LST_DEFARC_SORT_PAYEE,
	LST_DEFARC_SORT_CATEGORY,
	LST_DEFARC_SORT_AMOUNT,
	LST_DEFARC_SORT_ACCOUNT
};


enum {
	HID_ARC_MEMO,
	HID_ARC_VALID,
	HID_ARC_REMIND,
	HID_ARC_MAX
};


struct ui_arc_manage_data
{
	GtkWidget	*dialog;

	GList		*tmp_list;
	gint		change;
	Archive		*ext_arc;

	GtkWidget	*BT_typsch, *BT_typtpl;
	GtkWidget	*ST_search;
	GtkWidget	*LV_arc;
	GtkWidget	*BT_add, *BT_edit, *BT_rem;

	GtkWidget	*MB_schedule, *PO_schedule;
	
	GtkWidget	*CM_auto;
	GtkWidget	*LB_next, *PO_next;
	GtkWidget	*LB_every, *NB_every;
	GtkWidget   *LB_weekend, *CY_weekend;
	GtkWidget	*CY_unit;

	GtkWidget	*EX_options;
	GtkWidget	*CM_limit;
	GtkWidget	*NB_limit;
	GtkWidget	*LB_posts;
};


GtkWidget *ui_arc_manage_dialog (Archive *ext_arc);

#endif
