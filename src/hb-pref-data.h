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

#ifndef __HB_PREF_DATA_H__
#define __HB_PREF_DATA_H__

#include <glib.h>


typedef struct 
{
	gchar   *locale;
	gchar   *name;
} LangName;


typedef struct
{
	gushort		id;
	gushort		mceii;
	gchar		*iso;
	gchar		*name;
	gdouble		value;
	//gchar		*prefix_symbol;		/* max symbol is 3 digits in unicode */
	//gchar		*suffix_symbol;		/* but mostly is 1 digit */
	gchar		*symbol;
	gboolean	sym_prefix;
	gchar		*decimal_char;
	gchar		*grouping_char;
	gushort		frac_digits;
} EuroParams;



gboolean euro_country_is_mceii(gint ctryid);
EuroParams *euro_country_get(guint ctryid);
gboolean euro_country_notmceii_rate_update(guint ctryid);

gchar *languagename_get(const gchar *locale);

#endif
