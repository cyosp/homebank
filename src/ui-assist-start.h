/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2023 Maxime DOYEN
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

#ifndef __HB_UI_ASSIST_START_GTK_H__
#define __HB_UI_ASSIST_START_GTK_H__


#define PAGE_WELCOME		0
#define PAGE_GENERAL		1
#define PAGE_CURRENCIES		2
#define PAGE_CATEGORIES		3
#define PAGE_ACCOUNTS		4


struct assist_start_data
{
	GtkWidget	*dialog;
	//GtkWidget	*pages[NUM_PAGE];
	GtkWidget	*ST_owner;

	GtkWidget   *LB_cur_base, *BT_cur_change;;
	GtkWidget	*CM_cur_add, *LB_cur_others, *BT_cur_add;

	GtkWidget	*GR_file;
	GtkWidget	*TX_file;
	GtkWidget	*TX_preview;
	GtkWidget	*ok_image, *ko_image;
	GtkWidget	*CM_load;

	GtkWidget	*CM_acc_add;
	GtkWidget	*GR_acc;
	GtkWidget	*ST_name;
	GtkWidget	*CY_type;

	Currency4217 *curfmt;
	gchar		*pathfilename;
	GPtrArray	*cur_arr;
};



GtkWidget *ui_newfile_assitant_new(void);


#endif
