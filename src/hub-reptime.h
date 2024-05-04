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

#ifndef __HUB_REPTIME_H__
#define __HUB_REPTIME_H__


enum {
	HUB_TIM_VIEW_NONE,
	HUB_TIM_VIEW_SPENDING,
	HUB_TIM_VIEW_ACCBALANCE,
	HUB_TIM_VIEW_ALLBALANCE
};


/* list top spending */
enum
{
	LST_REPTIME_POS,
	LST_REPTIME_KEY,
	LST_REPTIME_LABEL,
	LST_REPTIME_ROW,
	NUM_LST_REPTIME
};



void ui_hub_reptime_update(GtkWidget *widget, gpointer user_data);
void ui_hub_reptime_clear(GtkWidget *widget, gpointer user_data);
void ui_hub_reptime_populate(GtkWidget *widget, gpointer user_data);

void ui_hub_reptime_setup(struct hbfile_data *data);
void ui_hub_reptime_dispose(struct hbfile_data *data);
GtkWidget *ui_hub_reptime_create(struct hbfile_data *data);


#endif