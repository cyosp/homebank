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
#include "hb-import.h"


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


/* = = = = = = = = = = = = = = = = */


static HbFileType
_str_search_marker(gchar *rawtext, gsize length)
{

	if( g_str_has_prefix(rawtext, "!") )
	{
		if( g_str_has_prefix(rawtext, "!Type")
		 || g_str_has_prefix(rawtext, "!Account")
		 ||	g_str_has_prefix(rawtext, "!Option")
		) return FILETYPE_QIF;

		if( g_str_has_prefix(rawtext, "!type")
		 || g_str_has_prefix(rawtext, "!account")
		 ||	g_str_has_prefix(rawtext, "!option")
		) return FILETYPE_QIF;
	}

	if( hb_csv_test_line(rawtext) )
		return FILETYPE_CSV_HB;

	if( g_strstr_len(rawtext, -1, "<OFX>") != NULL )
		return FILETYPE_OFX;
	if( g_strstr_len(rawtext, -1, "<ofx>") != NULL )
		return FILETYPE_OFX;

	if( g_str_has_prefix(rawtext, "<homebank v="))
		return FILETYPE_HOMEBANK;

	return FILETYPE_UNKNOWN;
}


static HbFileType
homebank_alienfile_recognize(gchar *filename)
{
GIOChannel *io;
HbFileType retval = FILETYPE_UNKNOWN;
gchar *tmpstr;
gsize length, eol_pos;
gint io_stat;
GError *err = NULL;

	DB( g_print("\n[homebank] alienfile_recognize\n") );

	io = g_io_channel_new_file(filename, "r", NULL);
	if(io != NULL)
	{
	guint n_line = 0;

		//set to binary mode
		g_io_channel_set_encoding(io, NULL, NULL);	

		//#1895478 5.4.4 : 25 => 48
		while( retval == FILETYPE_UNKNOWN && n_line <= 48 )
		{
			io_stat = g_io_channel_read_line(io, &tmpstr, &length, &eol_pos, &err);
			if( io_stat == G_IO_STATUS_NORMAL)
			{
				n_line++;
				#if MYDEBUG
				//5.9.2 fast remove eol
				if( length > 0 && eol_pos <= length )
					tmpstr[eol_pos] = 0;
				g_print(" line %d: ->|%s|<- %ld %ld\n", n_line, tmpstr, length, eol_pos);
				#endif

				retval = _str_search_marker(tmpstr, length);
				g_free(tmpstr);
			}
			else
			{
				if( io_stat == G_IO_STATUS_EOF )
				{
					DB( g_print(" eof reached\n") );
					break;
				}
				if( io_stat == G_IO_STATUS_ERROR )
				{
					DB (g_print(" + ERROR %s\n", err->message));
					g_error_free(err);
					err=NULL;
					break;
				}
			}
		}
		g_io_channel_unref (io);
	}

	#if MYDEBUG
	gchar *label[NUM_FILETYPE]={"???","hb","ofx","qif","csv"};
	g_print(" > type is %s", label[retval]);
	#endif
	return retval;
}


static void 
da_import_context_gen_txn_destroy(ImportContext *context)
{
GList *list;

	DB( g_print("\n[import] free gen txn list\n") );
	list = g_list_first(context->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *gentxn = list->data;
		da_gen_txn_free(gentxn);
		list = g_list_next(list);
	}
	g_list_free(context->gen_lst_txn);
	context->gen_lst_txn = NULL;
}


static void 
da_import_context_gen_acc_destroy(ImportContext *context)
{
GList *list;

	DB( g_print("\n[import] free gen acc list\n") );
	list = g_list_first(context->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;
		da_gen_acc_free(genacc);
		list = g_list_next(list);
	}
	g_list_free(context->gen_lst_acc);
	context->gen_lst_acc = NULL;

}


static void 
da_import_context_clear(ImportContext *context)
{
	DB( g_print("\n[import] context clear\n") );

	da_import_context_gen_txn_destroy(context);
	da_import_context_gen_acc_destroy(context);
	context->gen_next_acckey = 1;

}


void 
da_import_context_destroy(ImportContext *context)
{
GList *list;

	DB( g_print("\n[import] context destroy\n") );

	da_import_context_gen_txn_destroy(context);
	da_import_context_gen_acc_destroy(context);

	DB( g_print(" free gen file list\n") );
	list = g_list_first(context->gen_lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;
		da_gen_file_free(genfile);
		list = g_list_next(list);
	}
	g_list_free(context->gen_lst_file);
	context->gen_lst_file = NULL;
}


void 
da_import_context_new(ImportContext *context)
{
	context->gen_lst_file = NULL;

	context->gen_lst_acc  = NULL;
	context->gen_lst_txn  = NULL;
	context->gen_next_acckey = 1;
}


/* = = = = = = = = = = = = = = = = */

GenFile *
da_gen_file_malloc(void)
{
	return g_malloc0(sizeof(GenFile));
}

void 
da_gen_file_free(GenFile *genfile)
{
	if(genfile != NULL)
	{
		if(genfile->filepath != NULL)
			g_free(genfile->filepath);

		//#2111468 add error log
		if(genfile->errlog != NULL)
			g_free(genfile->errlog);

		g_free(genfile);
	}
}


GenFile *
da_gen_file_get(GList *lst_file, guint32 key)
{
GenFile *existfile = NULL;
GList *list;

	DB( g_print("\n[genfile] get %d\n", key) );

	list = g_list_first(lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;

		if( key == genfile->key )
		{
			existfile = genfile;
			DB( g_print(" found\n") );
			break;
		}
		list = g_list_next(list);
	}
	return existfile;
}


static GenFile *
da_gen_file_get_by_name(GList *lst_file, gchar *filepath)
{
GenFile *existfile = NULL;
GList *list;

	DB( g_print("\n[genfile] get by name\n") );

	list = g_list_first(lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;

		DB( g_print(" strcasecmp '%s' '%s'\n", filepath, genfile->filepath) );
	
		if(!strcasecmp(filepath, genfile->filepath))
		{
			existfile = genfile;
			DB( g_print(" found\n") );
			break;
		}
		list = g_list_next(list);
	}

	return existfile;
}


GenFile *
da_gen_file_append_from_filename(ImportContext *ictx, gchar *filename)
{
GenFile *genfile = NULL;
gint filetype;

	DB( g_print("\n[genfile] append from\n") );
	DB( g_print(" filename:'%s'\n", filename) );

	filetype = homebank_alienfile_recognize(filename);

	// we keep everything here
	//if( (filetype == FILETYPE_OFX) || (filetype == FILETYPE_QIF) || (filetype == FILETYPE_CSV_HB) )
	if( filetype != FILETYPE_HOMEBANK )
	{
	GenFile *existgenfile;

		existgenfile = da_gen_file_get_by_name(ictx->gen_lst_file, filename);
		if(existgenfile == NULL)
		{
			genfile = da_gen_file_malloc();
			genfile->filepath = g_strdup(filename);
			//FILETYPE_UNKNOWN = invalid
			genfile->filetype = filetype;
			
			//append to list
			DB( g_print(" add to list\n") );
			genfile->key = g_list_length (ictx->gen_lst_file) + 1;
			ictx->gen_lst_file = g_list_append(ictx->gen_lst_file, genfile);
		}
	}

	return genfile;
}


/* = = = = = = = = = = = = = = = = */


GenAcc *
da_gen_acc_malloc(void)
{
	return g_malloc0(sizeof(GenAcc));
}

void 
da_gen_acc_free(GenAcc *genacc)
{
	if(genacc != NULL)
	{
		if(genacc->name != NULL)
			g_free(genacc->name);
		if(genacc->number != NULL)
			g_free(genacc->number);

		g_free(genacc);
	}
}


GenAcc *
da_gen_acc_get_by_key(GList *lst_acc, guint32 key)
{
GenAcc *existacc = NULL;
GList *list;

	DB( g_print("da_gen_acc_get_by_key\n") );
	
	list = g_list_first(lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;

		if( key == genacc->key )
		{
			existacc = genacc;
			break;
		}
		list = g_list_next(list);
	}
	return existacc;
}


static GenAcc *
da_gen_acc_get_by_name(GList *lst_acc, gchar *name)
{
GenAcc *existacc = NULL;
GList *list;

	DB( g_print("da_gen_acc_get_by_name\n") );

	list = g_list_first(lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;

		//DB( g_print(" strcasecmp '%s' '%s'\n", name, genacc->name) );
	
		if(!strcasecmp(name, genacc->name))
		{
			existacc = genacc;
			//DB( g_print(" found\n") );
			break;
		}
		list = g_list_next(list);
	}

	return existacc;
}


/* = = = = = = = = = = = = = = = = */


Account *
hb_import_acc_find_existing(gchar *name, gchar *number)
{
Account *retacc = NULL;
GList *lacc, *list;

	DB( g_print("\n[import] acc_find_existing\n") );

	DB( g_print(" - search number '%s'\n", number) );
	lacc = list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *acc = list->data;
		
		//DB( g_print(" - eval acc '%s' or '%s'\n", acc->name, acc->number) );
		if(number != NULL && acc->number && strlen(acc->number) )
		{
			//prefer identifying with number & search number into acc->number
			if(g_strstr_len(number, -1, acc->number) != NULL)
			{
				DB( g_print(" - match number '%s'\n", acc->number) );
				retacc = acc;
				break;
			}
		}
		list = g_list_next(list);
	}

	//# 1815964 only test name if all number test failed
	//if not found try with name	
	if(retacc == NULL)
	{
		DB( g_print(" - search name '%s'\n", name) );
		list = g_list_first(lacc);
		while (list != NULL)
		{
		Account *acc = list->data;
			
			//DB( g_print(" - eval acc '%s' or '%s'\n", acc->name, acc->number) );
			if(retacc == NULL && name != NULL)
			{
				if(g_strstr_len(name, -1, acc->name) != NULL)
				{
					DB( g_print(" - match name '%s'\n", acc->name) );
					retacc = acc;
					break;
				}
			}
			list = g_list_next(list);
		}
	}

	g_list_free(lacc);

	return retacc;
}


GenAcc *
hb_import_gen_acc_get_next(ImportContext *ictx, gint filetype, gchar *name, gchar *number)
{
GenAcc *newacc;

	DB( g_print("\n[import] acc_get_next\n") );

	DB( g_print(" - type='%d', name='%s', number='%s'\n", filetype, name, number) );

	// try to find a same name account
	if( name != NULL )
	{
		newacc = da_gen_acc_get_by_name(ictx->gen_lst_acc, name);
		if(newacc != NULL)
		{
			DB( g_print(" - found existing '%s'\n", name) );
			goto end;
		}
	}

	newacc = da_gen_acc_malloc();
	if(newacc)
	{
		newacc->kfile = ictx->curr_kfile;
		newacc->key = ictx->gen_next_acckey++;
		newacc->kacc = DST_ACC_GLOBAL;
		
		if(name != NULL)
		{
			newacc->is_unamed = FALSE;
			newacc->name = g_strdup(name);
		}
		else
		{
		GenFile *genfile;
		gchar *basename;
		
			newacc->is_unamed = TRUE;

			genfile = da_gen_file_get (ictx->gen_lst_file, newacc->kfile);
			basename = g_path_get_basename(genfile->filepath);
			
			newacc->name = g_strdup_printf("%s %d", basename, newacc->key);
			g_free(basename);
		}
		
		if(number != NULL)
			newacc->number = g_strdup(number);

		ictx->gen_lst_acc = g_list_append(ictx->gen_lst_acc, newacc);
	}

	DB( g_print(" - create new '%s'\n", newacc->name) );

end:
	newacc->filetype = filetype;
	ictx->curr_kacc = newacc->key;

	return newacc;
}


/* = = = = = = = = = = = = = = = = */


GenTxn *
da_gen_txn_malloc(void)
{
	return g_malloc0(sizeof(GenTxn));
}


void 
da_gen_txn_free(GenTxn *gentxn)
{
gint i;

	if(gentxn != NULL)
	{
		if(gentxn->account != NULL)
			g_free(gentxn->account);

		if(gentxn->rawnumber != NULL)
			g_free(gentxn->rawnumber);
		if(gentxn->rawpayee != NULL)
			g_free(gentxn->rawpayee);
		if(gentxn->rawmemo != NULL)
			g_free(gentxn->rawmemo);

		// 5.5.1 add OFX fitid
		if(gentxn->fitid != NULL)
			g_free(gentxn->fitid);
		if(gentxn->date != NULL)
			g_free(gentxn->date);
		if(gentxn->number != NULL)
			g_free(gentxn->number);
		if(gentxn->payee != NULL)
			g_free(gentxn->payee);
		if(gentxn->memo != NULL)
			g_free(gentxn->memo);
		if(gentxn->category != NULL)
			g_free(gentxn->category);
		if(gentxn->tags != NULL)
			g_free(gentxn->tags);

		for(i=0;i<TXN_MAX_SPLIT;i++)
		{
		GenSplit *s = &gentxn->splits[i];
		
			if(s->memo != NULL)
				g_free(s->memo);
			if(s->category != NULL)
				g_free(s->category);	
		}

		if(gentxn->lst_existing != NULL)
		{
			g_list_free(gentxn->lst_existing);
			gentxn->lst_existing = NULL;
		}

		g_free(gentxn);
	}
}

static gint 
da_gen_txn_compare_func(GenTxn *a, GenTxn *b)
{
gint retval = (gint)(a->julian - b->julian); 

	//5.8 #2063416 same date txn
	if(!retval)
		retval = a->row - b->row;
	if(!retval)
		retval = (ABS(a->amount) - ABS(b->amount)) > 0 ? 1 : -1;
	return (retval);
}


GList *
da_gen_txn_sort(GList *list)
{
	return( g_list_sort(list, (GCompareFunc)da_gen_txn_compare_func));
}


void 
da_gen_txn_move(GenTxn *sgentxn, GenTxn *dgentxn)
{
	if(sgentxn != NULL && dgentxn != NULL)
	{
		memcpy(dgentxn, sgentxn, sizeof(GenTxn));
		memset(sgentxn, 0, sizeof(GenTxn));
	}
}


void 
da_gen_txn_append(ImportContext *ctx, GenTxn *gentxn)
{
	gentxn->kfile = ctx->curr_kfile;
	gentxn->kacc  = ctx->curr_kacc;
	gentxn->to_import = TRUE;
	//perf must use preprend, see glib doc
	//sort will be done later, so we don't care here
	ctx->gen_lst_txn = g_list_prepend(ctx->gen_lst_txn, gentxn);
}


/* = = = = = = = = = = = = = = = = */


static void _string_utf8_ucfirst(gchar **str)
{
gint str_len;
gchar *first, *lc;

	if( *str == NULL )
		return;
	
	str_len = strlen(*str);
	if( str_len <= 1 )
		return;

	first = g_utf8_strup(*str, 1);
	lc    = g_utf8_strdown( g_utf8_next_char(*str), -1 );
	g_free(*str);
	*str = g_strjoin(NULL, first, lc, NULL);
	g_free(first);
	g_free(lc);
}


static gchar *
_string_concat(gchar *str, gchar *addon)
{
gchar *retval;

	DB( g_print(" - concat '%s' + '%s'\n", str, addon) );

	if(str == NULL)
		retval = g_strdup(addon);
	else
	{
		retval = g_strjoin(" ", str, addon, NULL);
		g_free(str);
	}

	DB( g_print(" - retval='%s'\n", retval) );	
	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

gchar *hb_import_filetype_char_get(GenAcc *genacc)
{
gchar *retval = "";

	switch(genacc->filetype)
	{
#ifndef NOOFX
		case FILETYPE_OFX:
			retval = "OFX/QFX";
			break;
#endif
		case FILETYPE_QIF:
			retval = "QIF";
			break;

		case FILETYPE_CSV_HB:
			retval = "CSV";
			break;
	}

	return retval;
}


void 
hb_import_load_all(ImportContext *ictx)
{
GList *list;

	DB( g_print("\n[import] load all\n") );

	da_import_context_clear (ictx);
	
	list = g_list_first(ictx->gen_lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;
	
		if(genfile->filetype != FILETYPE_UNKNOWN)
		{
			//todo: move this to alien analysis
			genfile->encoding = homebank_file_getencoding(genfile->filepath);
		
			ictx->curr_kfile = genfile->key;

			DB( g_print(" -> key = '%d'\n", genfile->key) );
			DB( g_print(" -> filepath = '%s'\n", genfile->filepath) );
			DB( g_print(" -> encoding = '%s'\n", genfile->encoding) );

			genfile->loaded = FALSE;
			genfile->invaliddatefmt = FALSE;		
	
			switch(genfile->filetype)
			{
		#ifndef NOOFX
				case FILETYPE_OFX:
					homebank_ofx_import(ictx, genfile);
					break;
		#endif
				case FILETYPE_QIF:
					homebank_qif_import(ictx, genfile);
					break;

				case FILETYPE_CSV_HB:
					homebank_csv_import(ictx, genfile);
					break;
			}

			genfile->loaded = TRUE;
		}

		list = g_list_next(list);
	}
	
	// sort by date
	ictx->gen_lst_txn = da_gen_txn_sort(ictx->gen_lst_txn);

}


gint 
hb_import_gen_acc_count_txn(ImportContext *ictx, GenAcc *genacc)
{
GList *list;
gint count = 0;

	DB( g_print("\n[import] gen_acc_count_txn\n") );
	
	genacc->n_txnall = 0;
	genacc->n_txnimp = 0;

	list = g_list_first(ictx->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *gentxn = list->data;

		if(gentxn->kacc == genacc->key)
		{
			genacc->n_txnall++;
			count++;

			DB( g_print(" count %03d: gentxn in=%d dup=%d '%s'\n", count, gentxn->to_import, gentxn->is_dst_similar, gentxn->memo) );

			if(gentxn->to_import) 
				genacc->n_txnimp++;
		}
		list = g_list_next(list);
	}
	return count;
}


/**
 * uncheck duplicate within the import context files
 */
gint 
hb_import_gen_txn_check_duplicate(ImportContext *ictx, GenAcc *genacc)
{
GList *list1, *list2;
gboolean isimpsimilar = FALSE;
gint count = 0;

	DB( g_print("\n[import] gen_txn_check_duplicate\n") );

	list1 = g_list_first(ictx->gen_lst_txn);
	while (list1 != NULL)
	{
	GenTxn *gentxn1 = list1->data;

		isimpsimilar = FALSE;

		DB( g_print(" 1eval %d %dd %.2f '%s'\n", gentxn1->kacc, gentxn1->julian, gentxn1->amount, gentxn1->fitid) );

		if( (genacc->key == gentxn1->kacc) && (gentxn1->julian != 0) ) //same account, valid date
		{
			list2 = g_list_next(list1);
			while (list2 != NULL)
			{
			GenTxn *gentxn2 = list2->data;

				if( (gentxn2->julian > gentxn1->julian) || (isimpsimilar == TRUE) )
					break;

				DB( g_print("  2eval %d %d %.2f '%s'in=%d dup=%d\n", gentxn2->kacc, gentxn2->julian, gentxn2->amount, gentxn2->fitid, gentxn1->to_import, gentxn1->is_imp_similar) );

				// 5.5.1 add OFX fitid
				if( (genacc->filetype == FILETYPE_OFX) )
				{
				GenAcc *gacc1, *gacc2;
				
					//#1919080 check also account
					gacc1 = da_gen_acc_get_by_key(ictx->gen_lst_acc, gentxn1->kacc);
					gacc2 = da_gen_acc_get_by_key(ictx->gen_lst_acc, gentxn2->kacc);

					if( gacc1 != NULL && gacc2 != NULL )
					{
					gint resacc, resfitid;

						DB( g_print("  acc1='%s' <> acc2='%s'\n", gacc1->number, gacc2->number) );

						resacc   = hb_string_ascii_compare(gacc1->number, gacc2->number);
						resfitid = hb_string_ascii_compare(gentxn2->fitid, gentxn1->fitid);
					
						DB( g_print("  eval with fitid\n") );
						if( !resacc && !resfitid )
						{
							isimpsimilar = TRUE;
							DB( g_print("    found dup fitid\n") );
						}
					}
				}
				else
				{
					//DB( g_print(" eval with data\n") );
					//todo: maybe reinforce controls here
					if( (gentxn2->kacc == gentxn1->kacc) 
						&& (gentxn2->julian == gentxn1->julian)
						&& (gentxn2->amount == gentxn1->amount)
						&& (hb_string_compare(gentxn2->memo, gentxn1->memo) == 0)
						&& (hb_string_compare(gentxn2->payee, gentxn1->payee) == 0)
					    //#1954017 add comparison on category
					    && (hb_string_compare(gentxn2->category, gentxn1->category) == 0)
					    //#2114674 add comparison to number (cheque)
					    && (hb_string_compare(gentxn2->number, gentxn1->number) == 0)
					  )
					{
						isimpsimilar = TRUE;
						DB( g_print("   found dup data\n") );
					}
				}
				
				list2 = g_list_next(list2);
			}

			if( isimpsimilar == TRUE )
			{
				gentxn1->to_import = FALSE;
				gentxn1->is_imp_similar = TRUE;
				count++;
			}

		}
		list1 = g_list_next(list1);
	}

	genacc->n_txnsimimp = count;
	return count;
}


/**
 * uncheck existing txn into target account
 *
 */
gint 
hb_import_gen_txn_check_target_similar(ImportContext *ictx, GenAcc *genacc)
{
GList *list1, *list2;
gdouble amount;
gint count = 0;

	DB( g_print("\n[import] gen_txn_check_target_similar\n") );

	list1 = g_list_first(ictx->gen_lst_txn);
	while (list1 != NULL)
	{
	GenTxn *gentxn = list1->data;

		DB( g_print("\n--------\n src: a:%d d:%d %.17g '%s'\n", gentxn->kacc, gentxn->julian, gentxn->amount, gentxn->memo) );

		if(genacc->key == gentxn->kacc)
		{
			//#5.5.1, commented and reported below
			//as it override the state made with a call to hb_import_gen_txn_check_duplicate
			//gentxn->to_import = TRUE;
			gentxn->is_dst_similar = FALSE;

			if(genacc->kacc == DST_ACC_SKIP)
			{		
				gentxn->to_import = FALSE;
			}
			else
			{
			Account *acc = da_acc_get(genacc->kacc);
			
				if(acc != NULL)
				{
					//clear previous existing
					if(gentxn->lst_existing != NULL)
					{
						g_list_free(gentxn->lst_existing);
						gentxn->lst_existing = NULL;
					}

					//#1866456
					amount = (gentxn->togamount == TRUE) ? -gentxn->amount : gentxn->amount;

					// try to find existing transaction
					list2 = g_queue_peek_tail_link(acc->txn_queue);
					while (list2 != NULL)
					{
					Transaction *txn = list2->data;

						DB( g_print("  evl: a:%d d:%d %.17g '%s'\n", txn->kacc, txn->date, txn->amount, txn->memo) );

						//break if the date goes below the gentxn date + gap
						if( txn->date < (gentxn->julian - ictx->opt_daygap) )
							break;

						//#1586211 add of date tolerance
						//todo: maybe reinforce controls here
						if( ( genacc->kacc == txn->kacc ) 
						 && ( gentxn->julian <= (txn->date + ictx->opt_daygap) )
						 && ( gentxn->julian >= (txn->date - ictx->opt_daygap) )
						 //#2012999
						 && ( hb_amount_cmp(amount, txn->amount) == 0 )
						)
						{
							gentxn->lst_existing = g_list_append(gentxn->lst_existing, txn);
							gentxn->to_import = FALSE;
							gentxn->is_dst_similar = TRUE;
							count++;

							DB( g_print("    => found dst acc dup %d %.17g '%s' in=%d, dup=%d\n", gentxn->julian, amount, gentxn->memo, gentxn->to_import, gentxn->is_dst_similar) );
						}
						
						list2 = g_list_previous(list2);
					}
				}
				
			}
			
			//#5.5.1 added check here
			if( (gentxn->is_dst_similar == TRUE) || (gentxn->is_imp_similar == TRUE) )		
			{
				gentxn->to_import = FALSE;
			}
		}

		list1 = g_list_next(list1);
	}

	genacc->n_txnsimdst = count;
	return count;
}


/**
 * try to identify xfer for OFX
 *
 */
static gint 
hb_import_gen_xfer_eval(ImportContext *ictx, GList *list)
{
GList *root, *list1, *list2;
GList *match = NULL;
gint count = 0;

	DB( g_print("\n[import] gen xfer eval\n") );

	DB( g_print(" n_txn=%d\n", g_list_length(list)) );

	root = da_transaction_sort(list);
	
	root = list1 = g_list_first(list);
	while (list1 != NULL)
	{
	Transaction *txn1 = list1->data;
	GenAcc *acc;

		DB( g_print(" list:%p txn:%p\n", ictx->gen_lst_acc, txn1) );
		if( txn1 != NULL )
		{	
			acc = da_gen_acc_get_by_key(ictx->gen_lst_acc, txn1->kacc);

			DB( g_print(" src: kacc:%d dat:%d amt:%.2f %s kfxacc:%d\n", txn1->kacc, txn1->date, txn1->amount, txn1->memo, txn1->kxferacc) );

			if( (acc != NULL) && (acc->filetype == FILETYPE_OFX) )
			{
				match = NULL;
				count = 0;
				list2 = g_list_next(root);
				while (list2 != NULL)
				{
				Transaction *txn2 = list2->data;

					if(!txn2)
						goto next;

					//DB( g_print(" -- chk: kacc:%d dat:%d amt:%.2f %s\n", txn2->kacc, txn2->date, txn2->amount, txn2->memo) );
					if( (txn2->date > txn1->date) )
						break;

					if( (txn2 == txn1) || (txn2->flags & OF_INTXFER) )
						goto next;

					//todo: maybe reinforce controls here
					if( (txn2->kacc != txn1->kacc) 
						&& (txn2->date == txn1->date)
						&& (txn2->amount == -txn1->amount)
						&& (hb_string_compare(txn2->memo, txn1->memo) == 0)
					  )
					{
						DB( g_print("  match: kacc:%d dat:%d amt:%.2f %s kfxacc:%d\n", txn2->kacc, txn2->date, txn2->amount, txn2->memo, txn2->kxferacc) );
						match = g_list_append(match, txn2);
						count++;
					}
				next:
					list2 = g_list_next(list2);
				}
			
				if(count == 1)  //we found a single potential xfer, transform it
				{
				Transaction *txn2 ;

					DB( g_print("  single found => convert both\n") );

					list2 = g_list_first(match);	
					txn2 = list2->data;
					if( txn1 && txn2 )
					{
						txn1->flags |= OF_INTXFER;
						transaction_xfer_change_to_child(txn1, txn2);
					}					
				}
				// if more than one, we cannot be sure
				g_list_free(match);
			}
		}
		list1 = g_list_next(list1);
	}
	
	return count;
}


/**
 * apply the user option: date format, payee/memo/info mapping
 *
 */
gboolean 
hb_import_option_apply(ImportContext *ictx, GenAcc *genacc)
{
GList *list;
gint i;

	DB( g_print("\n[import] option apply\n") );

	DB( g_print(" - type=%d\n", genacc->filetype) );

	genacc->n_txnbaddate = 0;

	i=0;
	list = g_list_first(ictx->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *gentxn = list->data;

		if(gentxn->kacc == genacc->key)
		{
			if(genacc->filetype != FILETYPE_OFX)
			{
				gentxn->julian = hb_date_get_julian(gentxn->date, ictx->opt_dateorder);
				if( gentxn->julian == 0 )
				{
					genacc->n_txnbaddate++;
				}
			}

			if(genacc->filetype == FILETYPE_OFX)
			{
				DB( g_print(" - ofx option apply\n") );

				g_free(gentxn->payee);
				g_free(gentxn->memo);
				g_free(gentxn->number);
				gentxn->payee = NULL;
				gentxn->memo = NULL;
				gentxn->number = NULL;

				// OFX:check_number
				gentxn->number = g_strdup(gentxn->rawnumber);

				//#1791482 map name to info (concat only)
				switch(ictx->opt_ofxname)
				{
					//ofxname is stored into rawpayee
					//case 0: PRF_OFXNAME_IGNORE
					case PRF_OFXNAME_MEMO:
						gentxn->memo = g_strdup(gentxn->rawpayee);
						break;
					case PRF_OFXNAME_PAYEE:
						gentxn->payee = g_strdup(gentxn->rawpayee);
						break;
					case PRF_OFXNAME_NUMBER:
						//#1909323 no need to free here
						//g_free(gentxn->number);
						gentxn->number = _string_concat(gentxn->number, gentxn->rawpayee);
						break;
				}

				if(gentxn->rawmemo != NULL)
				{
					switch(ictx->opt_ofxmemo)
					{
						//case 0: PRF_OFXMEMO_IGNORE
						case PRF_OFXMEMO_NUMBER:
							gentxn->number = _string_concat(gentxn->number, gentxn->rawmemo);
							break;

						case PRF_OFXMEMO_MEMO:
							gentxn->memo = _string_concat(gentxn->memo, gentxn->rawmemo);					
							break;

						case PRF_OFXMEMO_PAYEE:
							gentxn->payee = _string_concat(gentxn->payee, gentxn->rawmemo);					
							break;
					}
				}

				DB( g_print(" - payee is '%s'\n", gentxn->payee) );
				DB( g_print(" - memo is '%s'\n", gentxn->memo) );
				DB( g_print(" - info is '%s'\n", gentxn->number) );
				DB( g_print("\n") );

			}
			else
			if(genacc->filetype == FILETYPE_QIF)
			{
				DB( g_print(" - qif option apply\n") );

				g_free(gentxn->payee);
				g_free(gentxn->memo);
				gentxn->payee = NULL;
				gentxn->memo = NULL;

				if(!ictx->opt_qifswap)
				{
					gentxn->payee = g_strdup(gentxn->rawpayee);
					if(ictx->opt_qifmemo)
						gentxn->memo = g_strdup(gentxn->rawmemo);
				}
				else
				{
					gentxn->payee = g_strdup(gentxn->rawmemo);
					if(ictx->opt_qifmemo)
						gentxn->memo = g_strdup(gentxn->rawpayee);
				}

				DB( g_print(" - payee is '%s'\n", gentxn->payee) );
				DB( g_print(" - memo is '%s'\n", gentxn->memo) );

			}
			else
			if(genacc->filetype == FILETYPE_CSV_HB)
			{
				DB( g_print(" - csv option apply\n") );

				DB( g_print(" [%d] row=%d\n", i, gentxn->row) );

				//#1791656 missing: info, payee and tags
				g_free(gentxn->payee);
				g_free(gentxn->memo);
				g_free(gentxn->number);

				gentxn->payee = g_strdup(gentxn->rawpayee);
				gentxn->memo = g_strdup(gentxn->rawmemo);
				gentxn->number = g_strdup(gentxn->rawnumber);
			}
			
			//at last do ucfirst
			if( (ictx->opt_ucfirst == TRUE) )
			{
				_string_utf8_ucfirst(&gentxn->memo);
				_string_utf8_ucfirst(&gentxn->payee);
				//category ?
			}

			//1866456 and toggle amount
			gentxn->togamount = ictx->opt_togamount;


		}
		i++;
		list = g_list_next(list);
	}
	
	DB( g_print(" - nb_err=%d\n", genacc->n_txnbaddate) );
	
	return genacc->n_txnbaddate == 0 ? TRUE : FALSE;
}


/**
 * convert a GenTxn to a Transaction
 *
 */
Transaction *
hb_import_convert_txn(GenAcc *genacc, GenTxn *gentxn)
{
Transaction *newope;
Account *accitem;
Payee *payitem;
Category *catitem;
gint nsplit;

	DB( g_print("\n[import] convert txn\n") );

	newope = NULL;

	DB( g_print(" - gentxt '%s' %s %s\n", gentxn->account, gentxn->date, gentxn->memo) );
	DB( g_print(" - genacc '%s' '%p' k=%d\n", gentxn->account, genacc, genacc->kacc) );

	if( genacc != NULL )
	{
		newope = da_transaction_malloc();
	
		newope->kacc         = genacc->kacc;
		newope->date		 = gentxn->julian;
		newope->paymode		 = gentxn->paymode;
		newope->number		 = g_strdup(gentxn->number);
		newope->memo		 = g_strdup(gentxn->memo);
		newope->amount		 = gentxn->amount;

		//#773282 invert amount for ccard accounts
		//todo: manage this (qif), it is not set to true anywhere
		//= it is into the qif account see hb-import-qif.c
		//if(ictx->is_ccard)
		//	gentxn->amount *= -1;

		//#1866456 manual toggle amount
		if( gentxn->togamount == TRUE )
		{
			newope->amount = -gentxn->amount;
		}

		// payee + append
		if( gentxn->payee != NULL )
		{
			payitem = da_pay_get_by_name(gentxn->payee);
			if(payitem == NULL)
			{
				//DB( g_print(" -> append pay: '%s'\n", item->payee ) );

				payitem = da_pay_malloc();
				payitem->name = g_strdup(gentxn->payee);
				//payitem->imported = TRUE;
				da_pay_append(payitem);

				//ictx->cnt_new_pay += 1;
			}
			newope->kpay = payitem->key;
		}

		// LCategory of transaction
		// L[Transfer account name]
		// LCategory of transaction/Class of transaction
		// L[Transfer account]/Class of transaction
		if( gentxn->category != NULL )
		{
			if(g_str_has_prefix(gentxn->category, "["))	// this is a transfer account name
			{
			gchar *accname;

				//DB ( g_print(" -> transfer to: '%s'\n", item->category) );

				//remove brackets
				accname = hb_strdup_nobrackets(gentxn->category);

				// search target account + append if not exixts
				accitem = da_acc_get_by_name(accname);
				if(accitem == NULL)
				{
					DB( g_print(" -> append int xfer dest acc: '%s'\n", accname ) );

					accitem = da_acc_malloc();
					accitem->name = g_strdup(accname);
					//accitem->imported = TRUE;
					//accitem->imp_name = g_strdup(accname);
					da_acc_append(accitem);
				}

				newope->kxferacc = accitem->key;
				newope->flags |= OF_INTXFER;

				g_free(accname);
			}
			else
			{
				//DB ( g_print(" -> append cat: '%s'\n", item->category) );

				catitem = da_cat_append_ifnew_by_fullname(gentxn->category);
				if( catitem != NULL )
				{
					//ictx->cnt_new_cat += 1;
					newope->kcat = catitem->key;
				}
			}
		}

		//#1791656 miss tags also...
		if( gentxn->tags != NULL )
		{
			g_free(newope->tags);
			newope->tags = tags_parse(gentxn->tags);
		}

		// splits, if not a xfer
		//TODO: it seems this never happen
		if( gentxn->paymode != OLDPAYMODE_INTXFER )
		//if( !(gentxn->flags & OF_INTXFER) )
		{
			if( gentxn->nb_splits > 0 )
			{
				newope->splits = da_split_new();
				for(nsplit=0;nsplit<gentxn->nb_splits;nsplit++)
				{
				GenSplit *s = &gentxn->splits[nsplit];
				Split *hbs;
				guint32 kcat = 0;
			
					DB( g_print(" -> append split %d: '%s' '%.2f' '%s'\n", nsplit, s->category, s->amount, s->memo) );

					if( s->category != NULL )
					{
						catitem = da_cat_append_ifnew_by_fullname(s->category);
						if( catitem != NULL )
						{
							kcat = catitem->key;
						}
					}

					//todo: remove this when no more use ||
					hb_string_remove_char('|', s->memo);
					hbs = da_split_malloc ();
					hbs->kcat   = kcat;
					hbs->memo   = g_strdup(s->memo);
					hbs->amount = s->amount;

					//#1866456 manual toggle amount
					if( gentxn->togamount == TRUE )
					{
						hbs->amount = -s->amount;
					}
					da_splits_append(newope->splits, hbs);
					hbs = NULL;				
				}
			}
		}
		
		newope->dspflags |= FLAG_TMP_ADDED;
		
		da_transaction_set_flag(newope);

		if( gentxn->reconciled )
		{
		Account *acc = da_acc_get(newope->kacc);

			newope->status = TXN_STATUS_RECONCILED;
			//#1581863 store reconciled date
			if( acc != NULL )
				acc->rdate = GLOBALS->today;
		}
		else
		if( gentxn->cleared )
			newope->status = TXN_STATUS_CLEARED;
	}
	return newope;
}


void 
hb_import_apply(ImportContext *ictx)
{
GList *list, *lacc;
GList *txnlist;
guint changes = 0;
guint nbofxtxn = 0;

	DB( g_print("\n[import] apply\n") );

	DB( g_print("\n--1-- insert acc\n") );

	//create accounts
	list = g_list_first(ictx->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;

		DB( g_print(" genacc: %d %s %s => %d\n", genacc->key, genacc->name, genacc->number, genacc->kacc) );

		//we do create the common account once
		if( (genacc->kacc == DST_ACC_GLOBAL) )
		{
		gchar *globalname = _("imported account");
		Account *acc = da_acc_get_by_name(globalname);
		
			if( acc == NULL )
			{
				acc = da_acc_malloc ();
				acc->name = g_strdup(globalname);
				if( da_acc_append(acc) )
				{
					changes++;
				}
			}
			//store the target acc key
			genacc->kacc = acc->key;
		}
		else
		if( (genacc->kacc == DST_ACC_NEW) )
		{
		Account *acc = da_acc_malloc ();
		
			acc->name = g_strdup(genacc->name);
			if( da_acc_append(acc) )
			{
				acc->number  = g_strdup(genacc->number);
				acc->initial = genacc->initial;
				
				//store the target acc key
				genacc->kacc = acc->key;
				changes++;
			}
			//#5.6.2 fix leak
			else
			{
				da_acc_free(acc);
			}
		}
		
		list = g_list_next(list);
	}

	// also keep every transactions into a temporary list
	// we do this to keep a finished real txn list for detect xfer below
	DB( g_print("\n--2-- insert txn\n") );

	txnlist = NULL;
	lacc = g_list_first(ictx->gen_lst_acc);
	while (lacc != NULL)
	{
	GenAcc *genacc = lacc->data;

		DB( g_print(" => genacc='%s'\n", genacc->name) );

		if(genacc->kacc != DST_ACC_SKIP)
		{
			list = g_list_first(ictx->gen_lst_txn);
			while (list != NULL)
			{
			GenTxn *gentxn = list->data;

				if(gentxn->kacc == genacc->key && gentxn->to_import == TRUE)
				{
				Transaction *txn, *dtxn;
		
					txn = hb_import_convert_txn(genacc, gentxn);
					if( txn )
					{
						dtxn = transaction_add(NULL, FALSE, txn);
						//perf must use preprend, see glib doc
						DB( g_print(" prepend %p to txnlist\n", dtxn) );
						//#2000480 avoid insert null txn
						if( dtxn != NULL )
						{
							//#1875100 add pending
							if(ictx->set_pending == TRUE)
								dtxn->flags |= OF_ISIMPORT;

							txnlist = g_list_prepend(txnlist, dtxn);					
						}
						da_transaction_free(txn);
						//#1820618 forgot to report changes count
						changes++;
					}
				}
				if( genacc->filetype == FILETYPE_OFX )
					nbofxtxn++;
				
				list = g_list_next(list);
			}
		}
		else
		{
			DB( g_print(" marked as skipped\n") );
		}
		lacc = g_list_next(lacc);
	}

	//auto from payee
	if( ictx->do_auto_payee == TRUE )
	{
		DB( g_print("\n--3-- call auto payee\n") );
		transaction_auto_all_from_payee(txnlist);
	}
		
	//auto assign
	if( ictx->do_auto_assign == TRUE )
	{
		DB( g_print("\n--4-- call auto assign\n") );
		transaction_auto_assign(txnlist, 0, FALSE);
	}	

	//check for ofx internal xfer
	if( nbofxtxn > 0 )
	{
		DB( g_print("\n--5-- call hb_import_gen_xfer_eval\n") );
		hb_import_gen_xfer_eval(ictx, txnlist);
	}

	DB( g_print("\n--end-- adding %d changes\n", changes) );
	GLOBALS->changes_count += changes;

	g_list_free(txnlist);

}


#if MYDEBUG
static void _import_context_debug_file_list(ImportContext *ctx)
{
GList *list;

	g_print("\n--debug-- file list %d\n", g_list_length(ctx->gen_lst_file) );

	list = g_list_first(ctx->gen_lst_file);
	while (list != NULL)
	{
	GenFile *item = list->data;

		g_print(" genfile: %d '%s' '%s'\ndf=%d invalid=%d\n", item->key, item->filepath, item->encoding, item->datefmt, item->invaliddatefmt);

		list = g_list_next(list);
	}

}

static void _import_context_debug_acc_list(ImportContext *ctx)
{
GList *list;

	g_print("\n--debug-- acc list %d\n", g_list_length(ctx->gen_lst_acc) );

	list = g_list_first(ctx->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *item = list->data;

		g_print(" genacc: %d %s %s => %d\n", item->key, item->name, item->number, item->kacc);

		list = g_list_next(list);
	}

}


static void _import_context_debug_txn_list(ImportContext *ctx)
{
GList *list;

	g_print("\n--debug-- txn list %d\n", g_list_length(ctx->gen_lst_txn) );

	list = g_list_first(ctx->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *item = list->data;

		g_print(" gentxn: (%d) %s %s (%d) %s %.2f\n", item->kfile, item->account, item->date, item->julian, item->memo, item->amount);

		list = g_list_next(list);
	}

}

#endif

