/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2025 Maxime DOYEN
 *
 *	This file is part of HomeBank.
 *
 *	HomeBank is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	HomeBank is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
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


// 0:date; 1:paymode; 2:info; 3:payee, 4:wording; 5:amount; 6:category; 7:tags
static gint csvtype[8] = {
	CSV_DATE, CSV_INT, CSV_STRING, CSV_STRING, CSV_STRING, CSV_DOUBLE, CSV_STRING, CSV_STRING
};


//5.9.2 multiple enclosed were wrong decoded
static gchar *hb_csv_strndup (gchar *str, gsize n)
{
gchar *new_str = NULL;

	if (str)
	{
	gboolean enclosed = FALSE;

		//test for enclosed
		if( n >= 2)
		{
			if( str[0]=='\"' && str[n-1]=='\"' )
				enclosed = TRUE;
		}

		if( !enclosed )
			new_str = g_strndup(str, n);
		else
		{
		gchar *s, *d;
		gsize len = n-2;

			//duplicate without enclosed + decode double quote
			new_str = g_new (gchar, len+1);
			s = str+1;
			d = new_str;
			while( *s && len > 0 )
			{
				//todo: ? replace &amp; &lt; &gt; &apos; &quot;
				*d++ = *s;
				if( *s=='\"' && s[1]=='\"' )
				{
					s++;
					len--;
				}
				s++;
				len--;
			}
			*d = 0;
		}
	}
	return new_str;
}


static gchar *hb_csv_find_delimiter(gchar *string, gchar delimiter)
{
gchar *s = string;
gboolean enclosed = FALSE;

	while( *s != '\0' )
	{
		if( (*s == delimiter) && (enclosed == FALSE) )
			break;

		if( *s == '\"' )
		{
			enclosed = !enclosed;
		}
	
		s++;
	}
	
	return s;
}


static gboolean hb_csv_row_valid(GString *node, guint n_line, gchar **str_array, guint nbcolumns, gint *csvtype)
{
gboolean valid = TRUE;
guint n_arrcol, i;
extern int errno;

#if MYDEBUG == 1
gchar *type[5] = { "string", "date", "int", "double" };
gint lasttype;
#endif

	//DB( g_print("\n[import-csv] row valid\n") );

	n_arrcol = g_strv_length( str_array );
	if( n_arrcol != nbcolumns )
	{
		valid = FALSE;
		g_string_append_printf(node, "line %d: %d column(s) are missing\n", n_line, nbcolumns - n_arrcol );
		DB( g_print(" >fail: %d columns\n", n_arrcol ) );
		goto csvend;
	}

	for(i=0;i<nbcolumns;i++)
	{
#if MYDEBUG == 1
		lasttype = csvtype[i];
#endif

		if(valid == FALSE)
		{
			g_string_append_printf(node, "line %d: invalid data in column %d\n", n_line, i);
			DB( g_print(" >fail: column %d, type: %s\n", i, type[lasttype]) );
			goto csvend;
		}

		switch( csvtype[i] )
		{
			case CSV_DATE:
				valid = hb_string_isdate(str_array[i]);
				break;
			case CSV_STRING:
				valid = hb_string_isprint(str_array[i]);
				break;
			case CSV_INT:
				valid = hb_string_isdigit(str_array[i]);
				break;
			case CSV_DOUBLE	:
				//todo: use strtod (to take care or . or ,)
				g_ascii_strtod(str_array[i], NULL);
				//todo: see this errno
				if( errno )
				{
					DB( g_print("errno: %d\n", errno) );
					valid = FALSE;
				}
				break;
		}
	}
csvend:
	return valid;
}


static gchar **hb_csv_row_get(gchar *string, gchar delimiter, gint max_tokens)
{
GSList *string_list = NULL, *slist;
gchar **str_array, *s;
guint n = 0;
gchar *remainder;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != '\0', NULL);

	if (max_tokens < 1)
		max_tokens = G_MAXINT;

	remainder = string;
	s = hb_csv_find_delimiter (remainder, delimiter);
	if (s)
	{
		while (--max_tokens && s && *s != '\0')
		{
		gsize len;

			len = s - remainder;
			string_list = g_slist_prepend (string_list, hb_csv_strndup (remainder, len));
			DB( g_print("  array[%d] = '%s'\n", n, (gchar *)string_list->data) );
			n++;
			remainder = s + 1;
			s = hb_csv_find_delimiter (remainder, delimiter);
		}
	}
	if (*string)
	{
	gsize len;
	
		len = s - remainder;
		string_list = g_slist_prepend (string_list, hb_csv_strndup (remainder, len));
		DB( g_print("  array[%d] = '%s'\n", n, (gchar *)string_list->data) );
		n++;
	}

	str_array = g_new (gchar*, n + 1);

	str_array[n--] = NULL;
	for (slist = string_list; slist; slist = slist->next)
		str_array[n--] = slist->data;

	g_slist_free (string_list);

	return str_array;
}


static gchar hb_csv_get_separator(void)
{
static const gchar sep[] = PRF_DTEX_CSVSEP_BUFFER;
	return sep[PREFS->dtex_csvsep];	
}


// 5.9.2: we fast check columns+enclosed and no more type
gboolean hb_csv_test_line(gchar *rawline)
{
gchar sep;
gchar *s;
gboolean isvalid = FALSE;
guint n_column = 1;
guint n_enclosed = 0;
gboolean enclosed = FALSE;

	sep = hb_csv_get_separator();
	s = rawline;
	while( *s != '\0' )
	{
		if( *s == sep && (enclosed == FALSE) )
			n_column++;
		else
		if( *s == '\"' )
		{
			enclosed = !enclosed;
			n_enclosed++;
		}
		s++;
	}

	if( (n_column == 8) && ((n_enclosed % 2) == 0) )
		isvalid = TRUE;

	DB( g_print(" >%d || n_col:%d n_enclosed:%d\n", isvalid, n_column, n_enclosed) );

	return isvalid;
}


GList *homebank_csv_import(ImportContext *ictx, GenFile *genfile)
{
GIOChannel *io;
//GList *list = NULL;

	DB( g_print("\n[import-csv] homebank csv\n") );

	io = g_io_channel_new_file(genfile->filepath, "r", NULL);
	if(io != NULL)
	{
	GString *node;
	gchar *tmpstr;
	gchar sep;
	
	gint io_stat;
	gboolean isvalid;
	GenAcc *newacc;
	GError *err = NULL;

		node = g_string_new(NULL);

		newacc = hb_import_gen_acc_get_next(ictx, FILETYPE_CSV_HB, NULL, NULL);

		if( genfile->encoding != NULL )
		{
			g_io_channel_set_encoding(io, genfile->encoding, NULL);
		}

		sep = hb_csv_get_separator();

		genfile->n_error = 0;
		for(guint n_line = 1;;n_line++)
		{
		gsize length, eol_pos;

			io_stat = g_io_channel_read_line(io, &tmpstr, &length, &eol_pos, &err);
			if( io_stat == G_IO_STATUS_EOF)
				break;
			if( io_stat == G_IO_STATUS_ERROR )
			{
				DB (g_print(" + ERROR %s\n",err->message));
				break;
			}
			if( io_stat == G_IO_STATUS_NORMAL)
			{
			gchar **str_array;

				//test strip crlf
				if( eol_pos > 0 )
					tmpstr[eol_pos] = 0;

				DB( g_print("\n--------\nreadline %d: [%s] %ld %ld\n", n_line, tmpstr, length, eol_pos) );

				//hex_dump(tmpstr, (guint)length+2);

				//#1844892 wish: detect/skip UTF-8 BOM (Excel CSV files)
				if(n_line == 1)
					hb_string_strip_utf8_bom(tmpstr);

				hb_string_strip_crlf(tmpstr);
				str_array = hb_csv_row_get(tmpstr, sep, 8);
				isvalid = hb_csv_row_valid(node, n_line, str_array, 8, csvtype);
				if( !isvalid )
				{
					DB( g_print("csv parse: line %d, invalid column count or data", n_line) );
					genfile->n_error++;
				}
				else
				{
				GenTxn *newope = da_gen_txn_malloc();

					DB( g_print(" adding txn\n" ) );

					//5.8 #2063416 same date txn
					newope->row         = n_line;
					/* convert to generic transaction */
					newope->date		= g_strdup(str_array[0]);			
					newope->paymode		= atoi(str_array[1]);
					//todo: reinforce controls here
					// csv file are standalone, so no way to link a target txn
					//added 5.1.8 forbid to import 5=internal xfer
					if(newope->paymode == OLDPAYMODE_INTXFER)
						newope->paymode = PAYMODE_XFER;
					newope->rawnumber	= g_strdup(str_array[2]);
					newope->rawpayee	= g_strdup(g_strstrip(str_array[3]));						
					newope->rawmemo		= g_strdup(str_array[4]);
					newope->amount		= hb_qif_parser_get_amount(str_array[5]);
					newope->category	= g_strdup(g_strstrip(str_array[6]));
					newope->tags		= g_strdup(str_array[7]);
					newope->account		= g_strdup(newacc->name);

					/* todo: move this eval date valid */
					//guint32 juliantmp = hb_date_get_julian(str_array[0], ictx->datefmt);
					///if( juliantmp == 0 )
					//	ictx->cnt_err_date++;

					/*
					DB( g_print(" storing %s : %s : %s :%s : %s : %s : %s : %s\n",
						str_array[0], str_array[1], str_array[2],
						str_array[3], str_array[4], str_array[5],
						str_array[6], str_array[7]
						) );
					*/

					da_gen_txn_append(ictx, newope);

					g_strfreev (str_array);
				}

				g_free(tmpstr);
			}
		}
		g_io_channel_unref (io);

		//todo: tmp test
		if( genfile->n_error > 0 )
		{
			genfile->errlog = g_string_free(node, FALSE);
		}
	/*
		ui_dialog_msg_infoerror(data->window, error > 0 ? GTK_MESSAGE_ERROR : GTK_MESSAGE_INFO,
			_("Transaction CSV import result"),
			_("%d transactions inserted\n%d errors in the file"),
			count, error);
		*/
	}

	return ictx->gen_lst_txn;
}


