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

#include "homebank.h"
#include "hb-hbfile.h"
#include "hb-archive.h"
#include "hb-transaction.h"
#include "ui-flt-widget.h"


/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


gboolean hbfile_file_isbackup(gchar *filepath)
{
gboolean retval = FALSE;

	if( filepath == NULL )
		return FALSE;

	if( g_str_has_suffix(filepath, "xhb~") || g_str_has_suffix(filepath, "bak") )
		retval = TRUE;

	return retval;
}


gboolean hbfile_file_hasrevert(gchar *filepath)
{
gchar *bakfilepath;

	bakfilepath = hb_filename_new_with_extension(GLOBALS->xhb_filepath, "xhb~");

	DB( g_print(" test bak exists '%s'\n", bakfilepath) );

	GLOBALS->xhb_hasrevert = g_file_test(bakfilepath, G_FILE_TEST_EXISTS);
	g_free(bakfilepath);
	//todo check here if need to return something
	return GLOBALS->xhb_hasrevert;
}


//#1750161
guint64 hbfile_file_get_time_modified(gchar *filepath)
{
guint64 retval = 0ULL;
GFile *gfile;
GFileInfo *gfileinfo;

	DB( g_print("\n[hbfile] get time modified\n") );

	gfile = g_file_new_for_path(filepath);
	gfileinfo = g_file_query_info (gfile, G_FILE_ATTRIBUTE_TIME_MODIFIED, 0, NULL, NULL);
	if( gfileinfo )
	{
		retval = g_file_info_get_attribute_uint64 (gfileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
		DB( g_print(" '%s' last access = %lu\n", filepath, retval) );	
		
		//add 5.6.2 if file opened more than 24h, currencies are obsolete
		GDateTime *dtf = g_file_info_get_modification_date_time(gfileinfo);
		gchar *dts = g_date_time_format_iso8601(dtf);
		GDateTime *dtn = g_date_time_new_now_local();
		GTimeSpan ts = g_date_time_difference(dtn, dtf);
		DB( g_print(" modif datetime='%s' since %ld, %ld hours\n", dts, ts, ts/G_TIME_SPAN_HOUR) );		
				
		GLOBALS->xhb_obsoletecurr = ((ts/G_TIME_SPAN_HOUR) > 24) ? TRUE : FALSE;

		g_free(dts);
		g_date_time_unref(dtf);
		g_date_time_unref(dtn);
		
		g_object_unref(gfileinfo);
	}
	g_object_unref(gfile);

	return retval;
}


void hbfile_file_default(void)
{
	DB( g_print("\n[hbfile] default\n") );

	//todo: maybe translate this also
	hbfile_change_filepath(g_build_filename(PREFS->path_hbfile, "untitled.xhb", NULL));
	GLOBALS->hbfile_is_new = TRUE;
	GLOBALS->hbfile_is_bak = FALSE;
	GLOBALS->xhb_timemodified = 0ULL;
	
	DB( g_print("- path_hbfile is '%s'\n", PREFS->path_hbfile) );
	DB( g_print("- xhb_filepath is '%s'\n", GLOBALS->xhb_filepath) );
}



/*
static gint hbfile_file_load_xhb(gchar *filepath)
{






}


static void hbfile_file_load_backup_xhb(void)
{
//todo: get from dialog.c, and split between dilaog.c/hbfile.c



}
*/


void hbfile_replace_basecurrency(Currency4217 *curfmt)
{
Currency *item;
guint32 oldkcur;

	DB( g_print("\n[hbfile] replace base currency\n") );
	
	oldkcur = GLOBALS->kcur;
	da_cur_delete(oldkcur);
	item = currency_add_from_user(curfmt);
	GLOBALS->kcur = item->key;
	
	DB( g_print(" %d ==> %d %s\n", oldkcur, GLOBALS->kcur, item->iso_code) );
}


void hbfile_change_basecurrency(guint32 key)
{
GList *list;
//guint32 oldkcur;
	
	// set every rate to 0
	list = g_hash_table_get_values(GLOBALS->h_cur);
	while (list != NULL)
	{
	Currency *entry = list->data;

		if(entry->key != GLOBALS->kcur)
		{
			entry->rate = 0.0;
			entry->mdate = 0;
		}
			
		list = g_list_next(list);
	}
	g_list_free(list);	

	//oldkcur = GLOBALS->kcur;
	GLOBALS->kcur = key;

	//#1851103: absolutely no reason to do that
	// update account with old base currency
	/*list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *acc = list->data;

		if( acc->kcur == oldkcur )
			acc->kcur = key;

		list = g_list_next(list);
	}
	g_list_free(list);
	*/

	GLOBALS->changes_count++;
}


static GQueue *hbfile_transaction_get_partial_internal(guint32 minjulian, guint32 maxjulian, gushort exclusionflags)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GQueue *txn_queue;

	txn_queue = g_queue_new ();

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		if( (acc->flags & exclusionflags) )
			goto next_acc;

		lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			if( txn->date < minjulian ) //no need to go below mindate
				break;
				
			//#1886123 include remind based on user prefs
			if( !transaction_is_balanceable(txn) )
				goto prev_txn;

			if( (txn->date >= minjulian) && (txn->date <= maxjulian) )
			{
				g_queue_push_head (txn_queue, txn);
			}
		prev_txn:
			lnk_txn = g_list_previous(lnk_txn);
		}
	
	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	return txn_queue;
}


GQueue *hbfile_transaction_get_partial(guint32 minjulian, guint32 maxjulian)
{
	//#1674045 only rely on nosummary
	//return hbfile_transaction_get_partial_internal(minjulian, maxjulian, (AF_CLOSED|AF_NOREPORT));
	return hbfile_transaction_get_partial_internal(minjulian, maxjulian, (AF_NOREPORT));
}


GQueue *hbfile_transaction_get_partial_budget(guint32 minjulian, guint32 maxjulian)
{
	//#1674045 only rely on nosummary
	//return hbfile_transaction_get_partial_internal(minjulian, maxjulian, (AF_CLOSED|AF_NOREPORT|AF_NOBUDGET));
	return hbfile_transaction_get_partial_internal(minjulian, maxjulian, (AF_NOREPORT|AF_NOBUDGET));
}


void hbfile_sanity_check(void)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GList *lxxx, *list;

	DB( g_print("\n[hbfile] !! full sanity check !! \n") );

	DB( g_print(" - transaction\n") );
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			da_transaction_consistency(txn);
			lnk_txn = g_list_next(lnk_txn);
		}
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	DB( g_print(" - scheduled/template\n") );
	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;

		da_archive_consistency(entry);
		list = g_list_next(list);
	}


	DB( g_print(" - account\n") );
	lxxx = list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *item = list->data;

		da_acc_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	
	DB( g_print(" - payee\n") );
	lxxx = list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *item = list->data;

		da_pay_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	
	DB( g_print(" - category\n") );
	lxxx = list = g_hash_table_get_values(GLOBALS->h_cat);
	while (list != NULL)
	{
	Category *item = list->data;

		da_cat_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	
	DB( g_print(" - assignments\n") );
	lxxx = list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *item = list->data;

		da_asg_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	//#2018414 replace any space by -
	DB( g_print(" - tags\n") );
	lxxx = list = g_hash_table_get_values(GLOBALS->h_tag);
	while (list != NULL)
	{
	Tag *item = list->data;

		da_tag_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(lxxx);

}


void hbfile_anonymize(void)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GList *lxxx, *list;
guint cnt;

	DB( g_print("\n[hbfile] anonymize\n") );

	// owner
	hbfile_change_owner(g_strdup("An0nym0us"));
	GLOBALS->changes_count++;
	GLOBALS->hbfile_is_new = TRUE;

	// filename
	hbfile_change_filepath(g_build_filename(PREFS->path_hbfile, "anonymized.xhb", NULL));

	// accounts
	lxxx = list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
		//#2026641
		da_acc_anonymize(list->data);

		GLOBALS->changes_count++;
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	//payees
	lxxx = list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *item = list->data;

		if(item->key != 0)
		{
			g_free(item->name);
			item->name = g_strdup_printf("payee %d", item->key);
			
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	//categories
	//lxxx = list = g_hash_table_get_values(GLOBALS->h_cat);
	lxxx = list = category_glist_sorted(HB_GLIST_SORT_KEY);
	while (list != NULL)
	{
	Category *item = list->data;

		if(item->key != 0)
		{
			//#2026641
			da_cat_anonymize(item);
			
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	//tags
	lxxx = list = g_hash_table_get_values(GLOBALS->h_tag);
	while (list != NULL)
	{
	Tag *item = list->data;

		if(item->key != 0)
		{
			g_free(item->name);
			item->name = g_strdup_printf("tag %d", item->key);
			
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	//assigns
	lxxx = list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *item = list->data;

		if(item->key != 0)
		{
			g_free(item->search);
			item->search = g_strdup_printf("assign %d", item->key);
			//#2026641 assignment notes
			g_free(item->notes);
			item->notes = NULL;
			
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(lxxx);

	//archives
	cnt = 0;
	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *item = list->data;
	
		g_free(item->memo);
		item->memo = g_strdup_printf("archive %d", cnt++);
		GLOBALS->changes_count++;

		//#2026641 splits memo as well	
		if(item->flags & OF_SPLIT)
		{
			cnt = da_splits_anonymize(item->splits);
			GLOBALS->changes_count += cnt;
		}

		list = g_list_next(list);
	}

	//transaction
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *item = lnk_txn->data;

			g_free(item->number);
			item->number = NULL;
			g_free(item->memo);
			item->memo = g_strdup_printf("memo %d", item->date);
			GLOBALS->changes_count++;
		
			if(item->flags & OF_SPLIT)
			{
				//#2026641
				cnt = da_splits_anonymize(item->splits);
				GLOBALS->changes_count += cnt;
			}
			lnk_txn = g_list_next(lnk_txn);
		}
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


void hbfile_change_owner(gchar *owner)
{
	g_free(GLOBALS->owner);
	GLOBALS->owner = (owner != NULL) ? owner : NULL;
}


void hbfile_change_filepath(gchar *filepath)
{
	g_free(GLOBALS->xhb_filepath);
	GLOBALS->xhb_filepath = (filepath != NULL) ? filepath : NULL;
}


void hbfile_cleanup(gboolean file_clear)
{
GSList *list;
//Transaction *txn;

	DB( g_print("\n[hbfile] cleanup\n") );
	DB( g_print("- file clear is %d\n", file_clear) );
	
	// Free data storage
	/*txn = g_trash_stack_pop(&GLOBALS->txn_stk);
	while( txn != NULL )
	{
		da_transaction_free (txn);
		txn = g_trash_stack_pop(&GLOBALS->txn_stk);
	}*/
	list = GLOBALS->deltxn_list;
	while(list != NULL)
	{
		da_transaction_free (list->data);
		list = g_slist_next(list);
	}
	g_slist_free(GLOBALS->deltxn_list);

	g_slist_free(GLOBALS->openwindows);

	da_transaction_destroy();
	da_archive_destroy(GLOBALS->arc_list);
	g_hash_table_destroy(GLOBALS->h_memo);

	da_flt_destroy();
	g_object_unref(GLOBALS->fltmodel);

	da_asg_destroy();
	da_tag_destroy();
	da_cat_destroy();
	da_pay_destroy();
	da_acc_destroy();
	da_grp_destroy();
	da_cur_destroy();

	hbfile_change_owner(NULL);

	if(file_clear)
		hbfile_change_filepath(NULL);

}




void hbfile_setup(gboolean file_clear)
{

	DB( g_print("\n[hbfile] setup\n") );
	DB( g_print("- file clear is %d\n", file_clear) );

	// Allocate data storage
	da_cur_new();
	da_grp_new();
	da_acc_new();
	//txn queue is alloc/free into account
	da_pay_new();
	da_cat_new();
	da_tag_new();
	da_asg_new();

	GLOBALS->fltmodel = lst_lst_favfilter_model_new();
	da_flt_new();

	GLOBALS->h_memo = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
	GLOBALS->arc_list = NULL;

	//5.5.1 list of opened windows
	GLOBALS->openwindows = NULL;

	//#1419304 we keep the deleted txn to a trash stack	
	//GLOBALS->txn_stk = NULL;
	GLOBALS->deltxn_list = NULL;

	if(file_clear == TRUE)
	{
		hbfile_file_default();
	}
	else
	{
		GLOBALS->hbfile_is_new = FALSE;
	}

	hbfile_change_owner(g_strdup(_("Unknown")));
	
	GLOBALS->kcur = 1;

	GLOBALS->vehicle_category = 0;
	
	GLOBALS->auto_smode = ARC_POSTMODE_DUEDATE;
	GLOBALS->auto_weekday = 1;
	GLOBALS->auto_nbmonths = 1;
	GLOBALS->auto_nbdays = 0;

	GLOBALS->changes_count = 0;

	GLOBALS->xhb_hasrevert = FALSE;

}

