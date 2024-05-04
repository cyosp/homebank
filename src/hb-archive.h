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

#ifndef __HB_ARCHIVE_H__
#define __HB_ARCHIVE_H__

#include "hb-transaction.h"
#include "hb-split.h"

typedef struct _archive		Archive;

struct _archive
{
	guint32		key;

	gdouble		amount;
	guint32		kacc;
	gushort		paymode;
	gushort		flags;
	guint32		kpay;
	guint32		kcat;
	gchar		*memo;

	//guint32		date;
	//gushort		pos;
	gushort     status;
	gchar		*info;
	guint32		*tags;
	//guint32		kxfer;		//strong link xfer key
	guint32		kxferacc;
	gdouble		xferamount;	//xfer target alount
	
	GPtrArray	*splits;

	guint32		nextdate;
	gushort		daygap;
	gushort		every;
	gushort		unit;
	gushort		limit;
	gushort		weekend;
};


/* list define archive (defarchive) */
enum
{
	LST_DEFARC_DATAS,
//	LST_DEFARC_OLDPOS,
//	LST_DEFARC_AUTO,
	NUM_LST_DEFARC
};


enum
{
	ARC_POSTMODE_DUEDATE,
	ARC_POSTMODE_PAYOUT,
	ARC_POSTMODE_ADVANCE
};


/*
** scheduled unit
*/
enum
{
	AUTO_UNIT_DAY,
	AUTO_UNIT_WEEK,
	AUTO_UNIT_MONTH,
	//AUTO_UNIT_QUARTER,
	AUTO_UNIT_YEAR
};


enum
{
	ARC_WEEKEND_POSSIBLE,
	ARC_WEEKEND_BEFORE,
	ARC_WEEKEND_AFTER,
	ARC_WEEKEND_SKIP
};


enum
{
	FLT_SCHEDULED_THISMONTH = 1,
	FLT_SCHEDULED_NEXTMONTH,
	FLT_SCHEDULED_NEXT30DAYS,
	FLT_SCHEDULED_NEXT60DAYS,
	FLT_SCHEDULED_NEXT90DAYS,
	FLT_SCHEDULED_ALLDATE
};



Archive *da_archive_malloc(void);
Archive *da_archive_clone(Archive *src_item);
guint archive_add_get_nbdays(void);
void da_archive_free(Archive *item);
void da_archive_destroy(GList *list);


guint da_archive_length(void);
void da_archive_stats(gint *nbtpl, gint *nbsch);
gboolean da_archive_append(Archive *item);
gboolean da_archive_append_new(Archive *item);
guint32 da_archive_get_max_key(void);
Archive *da_archive_get(guint32 key);


void da_archive_get_display_label(GString *tpltitle, Archive *item);
void da_archive_consistency(Archive *item);

Archive *da_archive_init_from_transaction(Archive *arc, Transaction *txn, gboolean fromregister);

GList *da_archive_glist_sorted(gint column);


void scheduled_nextdate_weekend_adjust(Archive *arc);
gboolean scheduled_is_postable(Archive *arc);
guint32 scheduled_get_postdate(Archive *arc, guint32 postdate);
guint32 scheduled_get_latepost_count(Archive *arc, guint32 jrefdate);
guint32 scheduled_date_advance(Archive *arc);

void scheduled_date_get_show_minmax(gint select, guint32 *mindate, guint32 *maxdate);

guint32 scheduled_date_get_post_max(guint32 start, gint auto_smode, gint auto_nbdays, gint auto_weekday, gint nbmonth);
gint scheduled_post_all_pending(void);


#endif

