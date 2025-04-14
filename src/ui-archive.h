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

#ifndef __HB_ARCHIVE_GTK_H__
#define __HB_ARCHIVE_GTK_H__


enum {
	HID_ARC_MEMO,
	HID_ARC_VALID,
	HID_ARC_REMIND,
	HID_ARC_MAX
};


struct ui_arc_manage_data
{
	GtkWidget	*dialog;
	gboolean	mapped_done;

	GList		*tmp_list;
	gint		change;
	Archive		*ext_arc;

	GtkWidget	*BT_typsch, *BT_typtpl;
	GtkWidget	*ST_search;
	GtkWidget	*LV_arc;
	GtkWidget	*BT_add, *BT_rem, *BT_edit, *BT_dup, *BT_schedule;

	//recurrent popover
	GtkWidget	*PO_recurrent, *GR_recurrent;
	GtkWidget	*SW_recurrent;
	GtkWidget	*RA_rec_freq;
	GtkWidget	*LB_next, *PO_next;
	GtkWidget	*IM_wrnwe;
	GtkWidget	*LB_rec_every, *NB_rec_every, *LB_rec_every2;
	GtkWidget	*CM_relative, *LB_relative, *CY_ordinal, *CY_weekday;
	GtkWidget   *LB_weekend, *CY_weekend;
	GtkWidget	*EX_options;
	GtkWidget	*CM_limit;
	GtkWidget	*NB_limit;
	GtkWidget	*LB_posts;
};


GtkWidget *ui_arc_manage_dialog (Archive *ext_arc);

#endif
