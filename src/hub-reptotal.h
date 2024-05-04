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

#ifndef __HUB_SPENDING_H__
#define __HUB_SPENDING_H__



enum {
	HUB_TOT_VIEW_NONE,
	HUB_TOT_VIEW_TOPCAT,
	HUB_TOT_VIEW_TOPPAY,
	HUB_TOT_VIEW_TOPACC,
	HUB_TOT_VIEW_ACCBAL,
	//5.7.3
	HUB_TOT_VIEW_GRPBAL
};


/* list top spending */
enum
{
	LST_TOPSPEND_POS,	//fake for pie
	LST_TOPSPEND_KEY,	//fake for pie
	LST_TOPSPEND_NAME,
	LST_TOPSPEND_AMOUNT,
	LST_TOPSPEND_RATE,

	NUM_LST_TOPSPEND
};



void ui_hub_reptotal_update(GtkWidget *widget, gpointer user_data);
void ui_hub_reptotal_clear(GtkWidget *widget, gpointer user_data);
void ui_hub_reptotal_populate(GtkWidget *widget, gpointer user_data);

void ui_hub_reptotal_setup(struct hbfile_data *data);
void ui_hub_reptotal_dispose(struct hbfile_data *data);
GtkWidget *ui_hub_reptotal_create(struct hbfile_data *data);


#endif
