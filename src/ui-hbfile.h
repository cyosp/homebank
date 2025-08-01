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

#ifndef __HB_HBFILE_GTK_H__
#define __HB_HBFILE_GTK_H__

struct defhbfile_data
{
	GtkWidget	*ST_owner;

	GtkWidget	*RA_postmode;
	GtkWidget	*GR_payout, *GR_advance;
	GtkWidget	*NU_weekday, *NU_nbmonths;
	GtkWidget	*NU_nbdays;
	GtkWidget	*LB_maxpostdate;

	GtkWidget	*PO_grp;
	GtkWidget	*ST_earnbyh;

	gint		change;
};


GtkWidget *create_defhbfile_dialog (void);

#endif
