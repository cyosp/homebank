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

#include "hb-currency.h"
#include "hb-pref-data.h"
#include <libsoup/soup.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

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

extern Currency4217 iso4217cur[];
extern guint n_iso4217cur;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void
da_cur_free(Currency *item)
{
	DB( g_print("\n[da_cur] free\n") );
	if(item != NULL)
	{
		DB( g_print(" %d, %s\n", item->key, item->iso_code) );

		g_free(item->name);
		g_free(item->iso_code);
		g_free(item->symbol);
		g_free(item->decimal_char);
		g_free(item->grouping_char);

		g_free(item);
	}
}


Currency *
da_cur_malloc(void)
{
	DB( g_print("\n[da_cur] malloc\n") );
	return g_malloc0(sizeof(Currency));
}


void
da_cur_destroy(void)
{
	DB( g_print("\n[da_cur] destroy\n") );
	g_hash_table_destroy(GLOBALS->h_cur);
}

void
da_cur_new(void)
{
Currency4217 *curfmt;

	DB( g_print("\n[da_cur] new\n") );
	GLOBALS->h_cur = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_cur_free);

	// insert default base currency
	curfmt = iso4217format_get(PREFS->IntCurrSymbol);
	if(curfmt == NULL)
		curfmt = iso4217format_get("USD");

	if(curfmt)
	{
	DB( g_printf(" curfmt %p\n", curfmt) );

		currency_add_from_user(curfmt);
	}
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
static void da_cur_max_key_ghfunc(gpointer key, Currency *item, guint32 *max_key)
{
	*max_key = MAX(*max_key, item->key);
}

static gboolean da_cur_name_grfunc(gpointer key, Currency *item, gchar *name)
{
	if( name && item->name )
	{
		if(!strcasecmp(name, item->name))
			return TRUE;
	}
	return FALSE;
}

static gboolean da_cur_iso_grfunc(gpointer key, Currency *item, gchar *iso)
{
	if( iso && item->iso_code )
	{
		if(!strcasecmp(iso, item->iso_code))
			return TRUE;
	}
	return FALSE;
}

guint
da_cur_length(void)
{
	return g_hash_table_size(GLOBALS->h_cur);
}

gboolean
da_cur_delete(guint32 key)
{
	DB( g_print("\n[da_cur] delete %d\n", key) );

	return g_hash_table_remove(GLOBALS->h_cur, &key);
}


void
da_cur_init_from4217(Currency *cur, Currency4217 *curfmt)
{
	cur->symbol        = g_strdup(curfmt->curr_symbol);
	cur->sym_prefix    = curfmt->curr_is_prefix;
	cur->decimal_char  = g_strdup(curfmt->curr_dec_char);
	cur->grouping_char = g_strdup(curfmt->curr_grp_char);
	cur->frac_digits   = curfmt->curr_frac_digit;
	da_cur_initformat(cur);
}


void
da_cur_initformat(Currency *item)
{
	DB( g_print("\n[da_cur] init format\n") );

	DB( g_print(" k:%d iso:'%s' symbol:'%s' digit:%d\n", item->key, item->iso_code, item->symbol, item->frac_digits) );

	// for formatd
	g_snprintf(item->format , 8-1, "%%.%df", item->frac_digits);

	if(item->sym_prefix == TRUE)
		g_snprintf(item->monfmt , 32-1, "%s %%s", item->symbol);
	else
		g_snprintf(item->monfmt , 32-1, "%%s %s", item->symbol);

	DB( g_print(" numfmt '%s'\n", item->format) );
	DB( g_print(" monfmt '%s'\n", item->monfmt) );
}


gboolean
da_cur_insert(Currency *item)
{
guint32 *new_key;

	DB( g_print("\n[da_cur] insert\n") );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;
	g_hash_table_insert(GLOBALS->h_cur, new_key, item);

	da_cur_initformat(item);

	return TRUE;
}


gboolean
da_cur_append(Currency *item)
{
Currency *existitem;
guint32 *new_key;

	DB( g_print("\n[da_cur] append\n") );

	/* ensure no duplicate */
	existitem = da_cur_get_by_name( item->name );
	if( existitem == NULL )
	{
		new_key = g_new0(guint32, 1);
		*new_key = da_cur_get_max_key() + 1;
		item->key = *new_key;
		//item->pos = da_cur_length() + 1;

		DB( g_print(" insert id: %d\n", *new_key) );

		g_hash_table_insert(GLOBALS->h_cur, new_key, item);

		da_cur_initformat(item);

		return TRUE;
	}

	DB( g_print(" %s already exist: %d\n", item->iso_code, item->key) );

	return FALSE;
}


guint32
da_cur_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_cur, (GHFunc)da_cur_max_key_ghfunc, &max_key);
	return max_key;
}


Currency *
da_cur_get_by_name(gchar *name)
{
	DB( g_print("\n[da_cur] get_by_name\n") );

	return g_hash_table_find(GLOBALS->h_cur, (GHRFunc)da_cur_name_grfunc, name);
}


Currency *
da_cur_get_by_iso_code(gchar *iso_code)
{
	DB( g_print("\n[da_cur] get_by_iso_code\n") );

	return g_hash_table_find(GLOBALS->h_cur, (GHRFunc)da_cur_iso_grfunc, iso_code);
}


Currency *
da_cur_get(guint32 key)
{
	//only use when needed
	//DB( g_print("\n[da_cur] get\n") );

	return g_hash_table_lookup(GLOBALS->h_cur, &key);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


gboolean
currency_is_euro(guint32 key)
{
Currency *item;
gboolean retval = FALSE;

	item = da_cur_get(key);
	if( item && item->iso_code )
	{
		if(!strcasecmp("EUR", item->iso_code))
			retval = TRUE;
	}
	return retval;
}


/**
 * currency_is_used:
 *
 * controls if a currency is used [base or account]
 *
 * Return value: TRUE if used, FALSE, otherwise
 */
gboolean
currency_is_used(guint32 key)
{
GList *list;
gboolean retval;

	DB( g_printf("\n[currency] is used\n") );

	if(GLOBALS->kcur == key)
		return TRUE;

	retval = FALSE;

	list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *item = list->data;

		if(item->kcur == key)
		{
			retval = TRUE;
			goto end;
		}
		list = g_list_next(list);
	}

end:
	g_list_free(list);

	return retval;
}


Currency4217 *iso4217format_get(gchar *code)
{
Currency4217 *cur;
guint i;

	DB( g_printf("\n[currency] iso4217 format get\n") );


	for (i = 0; i< n_iso4217cur; i++)
	{
		cur = &iso4217cur[i];
		if(g_strcmp0(cur->curr_iso_code, code) == 0)
		{
			return cur;
		}
	}
	return NULL;
}


//https://msdn.microsoft.com/en-us/library/windows/desktop/dd464799%28v=vs.85%29.aspx
//https://github.com/Samsung/icu/blob/master/source/i18n/winnmfmt.cpp
#ifdef G_OS_WIN32

#define NEW_ARRAY(type,count) (type *) g_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) g_free((void *) (array))


static void win_getCurrencyFormat(CURRENCYFMTW *fmt, const wchar_t *windowsLocaleName)
{
//wchar_t buf[10];

	DB( g_printf("\n[currency] win get system format\n") );

	GetLocaleInfoEx(windowsLocaleName, LOCALE_RETURN_NUMBER|LOCALE_ICURRDIGITS, (LPWSTR) &fmt->NumDigits, sizeof(UINT));
	//GetLocaleInfoEx(windowsLocaleName, LOCALE_RETURN_NUMBER|LOCALE_ILZERO, (LPWSTR) &fmt->LeadingZero, sizeof(UINT));

	//GetLocaleInfoEx(windowsLocaleName, LOCALE_SMONGROUPING, (LPWSTR)buf, sizeof(buf));
	//fmt->Grouping = getGrouping(buf);

	fmt->lpDecimalSep = NEW_ARRAY(wchar_t, 6);
	GetLocaleInfoEx(windowsLocaleName, LOCALE_SMONDECIMALSEP,  fmt->lpDecimalSep,  6);

	fmt->lpThousandSep = NEW_ARRAY(wchar_t, 6);
	GetLocaleInfoEx(windowsLocaleName, LOCALE_SMONTHOUSANDSEP, fmt->lpThousandSep, 6);

	//GetLocaleInfoEx(windowsLocaleName, LOCALE_RETURN_NUMBER|LOCALE_INEGCURR,  (LPWSTR) &fmt->NegativeOrder, sizeof(UINT));
	GetLocaleInfoEx(windowsLocaleName, LOCALE_RETURN_NUMBER|LOCALE_ICURRENCY, (LPWSTR) &fmt->PositiveOrder, sizeof(UINT));

	fmt->lpCurrencySymbol = NEW_ARRAY(wchar_t, 8);
	GetLocaleInfoEx(windowsLocaleName, LOCALE_SCURRENCY, (LPWSTR) fmt->lpCurrencySymbol, 8);
}


static void win_freeCurrencyFormat(CURRENCYFMTW *fmt)
{
	DB( g_printf("\n[currency] win free system format\n") );

	if (fmt != NULL) {
		DELETE_ARRAY(fmt->lpCurrencySymbol);
		DELETE_ARRAY(fmt->lpThousandSep);
		DELETE_ARRAY(fmt->lpDecimalSep);
	}
}


static void win_currency_get_system_iso(void)
{
LPWSTR lpIntlSymbol;

	DB( g_printf("\n[currency] get system ISO\n") );

	lpIntlSymbol = NEW_ARRAY(wchar_t, 9);
	GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SINTLSYMBOL,  lpIntlSymbol,  9);

	DB( wprintf(L"raw lpIntlSymbol: '%hs'\n", lpIntlSymbol) );

	gchar *tmpstr = g_utf16_to_utf8(lpIntlSymbol, -1, NULL, NULL, NULL);
	g_stpcpy (PREFS->IntCurrSymbol, tmpstr);
	g_free(tmpstr);

	DELETE_ARRAY(lpIntlSymbol);
}


static void win_currency_format_system_fill(Currency *item)
{
CURRENCYFMTW currency;

	DB( g_printf("\n[currency] windows format system fill '%s'\n", item->iso_code) );

	win_getCurrencyFormat(&currency, LOCALE_NAME_USER_DEFAULT);

	#if MYDEBUG == 1
	  printf("raw ptr = %p\n", &currency);
	  printf("raw NumDigits: '%d'\n", currency.NumDigits);
	//note this will not print well on windows console which is not unicode compliant
	wprintf(L"raw lpDecimalSep: '%hs'\n", currency.lpDecimalSep);
	wprintf(L"raw lpThousandSep: '%hs'\n", currency.lpThousandSep);
	wprintf(L"raw lpCurrencySymbol: '%hs'\n", currency.lpCurrencySymbol);
	#endif

	item->symbol        = g_utf16_to_utf8(currency.lpCurrencySymbol, -1, NULL, NULL, NULL);
	item->sym_prefix    = (currency.PositiveOrder & 1) ? FALSE : TRUE;
	item->decimal_char  = g_utf16_to_utf8(currency.lpDecimalSep, -1, NULL, NULL, NULL);
	item->grouping_char = g_utf16_to_utf8(currency.lpThousandSep, -1, NULL, NULL, NULL);
	item->frac_digits   = currency.NumDigits;

	win_freeCurrencyFormat(&currency);
}

#endif


#ifdef G_OS_UNIX


static void unix_currency_get_system_iso(void)
{
struct lconv *lc = localeconv();

	DB( g_printf("\n[currency] get system ISO\n") );

	g_stpcpy (PREFS->IntCurrSymbol, lc->int_curr_symbol);
}


static void unix_currency_format_system_fill(Currency *item)
{
struct lconv *lc = localeconv();

	DB( g_printf("\n[currency] linux format system fill '%s'\n", item->iso_code) );

	#if MYDEBUG == 1
	g_print("raw ptr = %p\n", lc);
	g_print("raw   int_curr_symbol = '%s'\n", lc->int_curr_symbol);
	g_print("raw     ?_cs_precedes = '%d' '%d'\n", lc->p_cs_precedes, lc->n_cs_precedes);
	g_print("raw mon_decimal_point = '%s'\n", lc->mon_decimal_point);
	g_print("raw mon_thousands_sep = '%s'\n", lc->mon_thousands_sep);
	g_print("raw       frac_digits = '%d'\n", lc->frac_digits);
	#endif

	item->symbol        = g_strdup(lc->currency_symbol);
	item->sym_prefix    = ( lc->p_cs_precedes || lc->n_cs_precedes ) ? TRUE : FALSE;
	item->decimal_char  = g_strdup(lc->mon_decimal_point);
	item->grouping_char = g_strdup(lc->mon_thousands_sep);
	item->frac_digits   = lc->frac_digits;

	//fix 378992/421228
	if( item->frac_digits > MAX_FRAC_DIGIT )
	{
		item->frac_digits = 2;
		g_free(item->decimal_char);
		item->decimal_char = g_strdup(".");
	}
}
#endif


void currency_get_system_iso(void)
{
	DB( g_print("\n[currency] get system ISO code\n") );

#ifdef G_OS_UNIX
	unix_currency_get_system_iso();
#else
	#ifdef G_OS_WIN32
	win_currency_get_system_iso();
	#else
 	g_stpcpy (PREFS->IntCurrSymbol, "USD");
	#endif
#endif

	g_strstrip(PREFS->IntCurrSymbol);

	DB( g_print("- stored '%s' as default\n", PREFS->IntCurrSymbol) );

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


Currency *currency_add_from_user(Currency4217 *curfmt)
{
Currency *item;

	DB( g_printf("\n[currency] found adding %s\n", curfmt->curr_iso_code) );

	item = da_cur_malloc();
	//no mem alloc here
	//item->key = i;
	if(curfmt != NULL)
	{
		item->name = g_strdup(curfmt->name);
		//item->country = cur.country_name;
		item->iso_code = g_strdup(curfmt->curr_iso_code);

		//1634615 if the currency match the system, fill with it
		if(!strcmp(item->iso_code, PREFS->IntCurrSymbol))
		{
			#ifdef G_OS_UNIX
			unix_currency_format_system_fill(item);
			#else
				#ifdef G_OS_WIN32
				win_currency_format_system_fill(item);
				#endif
			#endif
		}
		else
		{
			//init from our internal iso 4217 preset
			item->frac_digits   = curfmt->curr_frac_digit;
			item->symbol        = g_strdup(curfmt->curr_symbol);
			item->sym_prefix    = curfmt->curr_is_prefix;
			item->decimal_char  = g_strdup(curfmt->curr_dec_char);
			item->grouping_char = g_strdup(curfmt->curr_grp_char);
		}
	}
	else
	{
		item->name        = g_strdup("unknown");
		//item->country     = cur.country_name;
		item->iso_code    = g_strdup("XXX");
		item->frac_digits = 2;
		item->sym_prefix  = FALSE;
	}

	//#1862540 if symbol/decimal/grouping char are null = crash on hb_str_formatd()
	if( !item->symbol )
		item->symbol = g_strdup("XXX");

	if( !item->decimal_char )
		item->decimal_char = g_strdup(".");

	if( !item->grouping_char )
		item->grouping_char = g_strdup("");

	#if MYDEBUG == 1
	g_printf("        symbol ='%s'\n", item->symbol);
	g_printf("    sym_prefix ='%d'\n", item->sym_prefix);
	g_printf("  decimal_char ='%s'\n", item->decimal_char);
	g_printf(" grouping_char ='%s'\n", item->grouping_char);
	g_printf("   frac_digits ='%d'\n", item->frac_digits);
	#endif

	da_cur_append(item);

	return item;
}


static gboolean currency_rate_update(gchar *isocode, gdouble rate, guint32 date)
{
gboolean retval = FALSE;
Currency *cur;

	DB( g_print("\n[currency] rate update\n") );

	cur = da_cur_get_by_iso_code (isocode);
	if(cur)
	{
		DB( g_print(" found cur='%s'\n", cur->iso_code) );

		//#2002650 test if there is a change
		if( rate != cur->rate )
		{
			GLOBALS->changes_count++;
			retval = TRUE;
		}
		#if MYDEBUG == 1
		else
		{
			DB( g_print(" rate was identical\n") );
		}
		#endif

		cur->rate   = rate;
		cur->mdate  = date;		

	}

	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* currency API
 * discontinued see #1730527, #1785210
 */

/* real open source fixer API */
/* DNS should be: https://frankfurter.app
 * see https://github.com/fixerAPI/fixer/issues/107
 */
 
/*
** 5.7 
** https://api.exchangerate.host/latest?base=EUR&symbols=USD,CHF,AUD,CAD,JPY,CNY,GBP
** http://data.fixer.io/api/latest?access_key=xxxx&base=EUR&symbols=USD,CHF,AUD,CAD,JPY,CNY,GBP
*/ 


/* old
** api.fixer.io deprecated since 30/04/2019
** QS: https://api.fixer.io/latest?base=EUR&symbols=USD,CHF,AUD,CAD,JPY,CNY,GBP
**
** test API
** gchar fixeriojson[] =
** "{    }";
** "	{	\r	\"base\"	:	\"EUR\", \
** \"date\":	\n\r		\"2017-12-04\", \
** \"rates\"	\n\n		:{\"AUD\":1.5585,\"CAD\":1.5034,\"CHF\":1.1665,\"CNY\":7.8532,\"GBP\":0.87725,\"JPY\":133.91,\"USD\":1.1865 \
** }  	}";
*/


static gboolean api_fixerio_parse(GBytes *body, GError **error)
{
gchar *rawjson;
gchar *p;
gchar strbuf[48];
gchar isocode[8];
gdouble rate;
guint32 date = GLOBALS->today;
guint count = 0;

	DB( g_print("\n[currency] api fixerio parse\n") );

	if(body)
	{
		//there is no need of a complex JSON parser here, so let's hack !
		rawjson = g_strdup(g_bytes_get_data(body, NULL));
		hb_string_inline(rawjson);

		DB( g_printf(" body: '%s'\n", rawjson ) );

		//get date
		p = g_strstr_len(rawjson, -1, "\"date\"");
		if(p)
		{
			strncpy(strbuf, p+8, 10);
			strbuf[10]='\0';
			date = hb_date_get_julian(strbuf, PRF_DATEFMT_YMD);
			DB( g_printf(" date: %.10s %d\n", strbuf, date) );
		}

		//get rates
		p = g_strstr_len(rawjson, -1, "\"rates\"");
		if(p)
		{
			p = p+8;
			do
			{
				p = hb_string_copy_jsonpair(strbuf, p);
				strncpy(isocode, strbuf, 3);
				isocode[3]='\0';
				rate = g_ascii_strtod(strbuf+4, NULL);
				DB( g_printf(" pair: '%s' > '%s' %f\n", strbuf, isocode, rate ) );

				if( currency_rate_update(isocode, rate, date) )
					count++;
			}
			while( p != NULL );
		}

		g_free(rawjson);

	}

	return( (count > 0) ? TRUE : FALSE);
}


static gchar *api_fixerio_query_build(void)
{
GList *list;
GString *node;
Currency *base;
Currency *item;
gint i;

	DB( g_print("\n[currency] api fixerio query build\n") );

	base = da_cur_get (GLOBALS->kcur);

	node = g_string_sized_new(512);
	//g_string_append_printf(node, "https://api.frankfurter.app/latest?base=%s&symbols=", base->iso_code);

	//2017410 + 5.7 base url from prefs
	//g_string_append_printf(node, "%s/latest?", PREFS->api_rate_url);
	//5.7.2 get full hostname from prefs
	g_string_append_printf(node, "%s?", PREFS->api_rate_url);

	//2017410 + 5.7 key from prefs
	if( PREFS->api_rate_key != NULL && strlen(PREFS->api_rate_key) > 8 )
		g_string_append_printf(node, "access_key=%s&", PREFS->api_rate_key);

	//base currency
	g_string_append_printf(node, "base=%s&symbols=", base->iso_code);


	list = g_hash_table_get_values(GLOBALS->h_cur);
	i = g_list_length (list);
	while (list != NULL)
	{
	item = list->data;

		if( (item->key != GLOBALS->kcur) && (strlen(item->iso_code) == 3) )
		{
			g_string_append_printf(node, "%s", item->iso_code);
			if(i > 1)
			{
				g_string_append(node, ",");
			}
		}
		i--;
		list = g_list_next(list);
	}
	g_list_free(list);

	return g_string_free(node, FALSE);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


gboolean currency_online_sync(GError **error, GString *node)
{
SoupSession *session;
SoupMessage *msg;
GBytes *body;
gchar *query;
guint status;
gboolean retval = TRUE;

	DB( g_printf("\n[currency] sync online\n") );

	query = api_fixerio_query_build();
	DB( g_printf(" query: '%s'\n", query) );

	/*
	//test API
	retval = api_fixerio_parse(fixeriojson, error);
	*/
	if( node != NULL )
	{
		g_string_append(node, query);
		g_string_append(node, "\n");
	}

	session = soup_session_new ();
	msg = soup_message_new ("GET", query);
	if(msg != NULL)
	{
		//soup_session_send_message (session, msg);

		//DB( g_print("status_code: %d %d\n", msg->status_code, SOUP_STATUS_IS_SUCCESSFUL(msg->status_code) ) );
		body = soup_session_send_and_read (session, msg, NULL, error);
		status = soup_message_get_status (msg);
		DB( g_print(" status_code: %d %d\n", status, SOUP_STATUS_IS_SUCCESSFUL(status) ) );
		DB( g_print(" reason: '%s'\n", soup_message_get_reason_phrase(msg)) );
		DB( g_print(" datas: '%s'\n", (gchar *)g_bytes_get_data(body, NULL)) );

		if( node != NULL )
		{
			g_string_append_printf(node, "status_code: %d\n", status);
			g_string_append_printf(node, "reason: '%s'\n", soup_message_get_reason_phrase(msg));
			g_string_append_printf(node, "datas: '%s'\n", (gchar *)g_bytes_get_data(body, NULL));
		}

		//if( SOUP_STATUS_IS_SUCCESSFUL(msg->status_code) == TRUE )
		if( SOUP_STATUS_IS_SUCCESSFUL(status) == TRUE )
		{
			//#1750426 ignore the retval here (false when no rate was found, as we don't care)
			//api_fixerio_parse(msg->response_body->data, error);
			api_fixerio_parse(body, error);
			
			//#2066110 update euro minor rate
			if(PREFS->euro_active == TRUE)
			{
				euro_country_notmceii_rate_update(PREFS->euro_country);
			}
		}
		else
		{
			//*error = g_error_new_literal(1, msg->status_code, msg->reason_phrase);
			*error = g_error_new_literal(1, status, soup_message_get_reason_phrase(msg) );
			retval = FALSE;
		}

		g_bytes_unref (body);
		g_object_unref (msg);
	}
	else
	{
		*error = g_error_new_literal(1, 0, "cannot parse URI");
		retval = FALSE;
	}

	g_free(query);

	soup_session_abort (session);

	g_object_unref(session);

	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


//struct iso_4217_currency iso_4217_currencies[];

/*debug testing
static void fill_currency(void)
{
gint i;
struct iso_4217_currency cur;
Currency *item;

	for (i = 0; i< 500; i++)
	{
		cur = iso_4217_currencies[i];

		if(cur.iso_code == NULL)
			break;

		item = da_cur_malloc();
		//no mem alloc here
		item->key = i;
		item->name = cur.currency;
		item->country = cur.country;
		item->iso_code = cur.iso_code;

		da_cur_insert(item);

	}



}*/

//https://en.wikipedia.org/wiki/ISO_4217

Currency4217 iso4217cur[] =
{
	{ "AED", 2, ".", ",", TRUE, "د.إ.‏", "UAE Dirham" },
	{ "AFN", 2, ",", ",", TRUE, "؋", "Afghani" },
	{ "ALL", 2, ",", " ", FALSE, "Lekë", "Lek" },
	{ "AMD", 2, ".", ",", FALSE, "֏", "Armenian Dram" },
	{ "ANG", 2, ",", ",", TRUE, "NAf.", "Netherlands Antillian Guilder" },
	{ "AOA", 2, ",", " ", FALSE, "Kz", "Kwanza" },
	{ "ARS", 2, ",", ".", TRUE, "$", "Argentine Peso" },
	{ "AUD", 2, ".", ",", TRUE, "$", "Australian Dollar" },
	{ "AWG", 2, ",", ".", TRUE, "Afl.", "Aruban Guilder" },
	{ "AZN", 2, ",", " ", TRUE, "₼", "Azerbaijanian Manat" },
	{ "BAM", 2, ",", ".", FALSE, "KM", "Convertible Marks" },
	{ "BBD", 2, ".", ",", TRUE, "$", "Barbados Dollar" },
	{ "BDT", 2, ".", ",", TRUE, "৳", "Taka" },
	{ "BGN", 2, ",", " ", FALSE, "лв.", "Bulgarian Lev" },
	{ "BHD", 3, ".", ",", TRUE, "د.ب.‏", "Bahraini Dinar" },
	{ "BIF", 0, ",", " ", FALSE, "FBu", "Burundi Franc" },
	{ "BMD", 2, ".", ",", TRUE, "$", "Bermudian Dollar" },
	{ "BND", 2, ",", ".", TRUE, "$", "Brunei Dollar" },
	{ "BOB", 2, ",", ".", TRUE, "Bs", "Boliviano" },
	{ "BOV", 2, ".", "", FALSE, "BOV", "Mvdol" },
	{ "BRL", 2, ",", ".", TRUE, "R$", "Brazilian Real" },
	{ "BSD", 2, ".", ",", TRUE, "$", "Bahamian Dollar" },
	{ "BTN", 2, ".", ",", TRUE, "Nu.", "Ngultrum" },
	{ "BWP", 2, ".", " ", TRUE, "P", "Pula" },
	{ "BYN", 0, ",", " ", FALSE, "Br", "Belarussian Ruble" },
	{ "BYR", 0, ",", " ", FALSE, "Br", "Old Belarussian Ruble" },
	{ "BZD", 2, ".", ",", TRUE, "$", "Belize Dollar" },
	{ "CAD", 2, ",", " ", TRUE, "$", "Canadian Dollar" },
	{ "CDF", 2, ",", " ", TRUE, "FC", "Congolese Franc" },
	{ "CHE", 2, ".", "", FALSE, "CHE", "WIR Euro" },
	{ "CHF", 2, ",", "'", TRUE, "CHF", "Swiss Franc" },
	{ "CHW", 2, ".", "", FALSE, "CHW", "WIR Franc" },
	{ "CLF", 4, ".", "", FALSE, "CLF", "Unidades de fomento" },
	{ "CLP", 0, ",", ".", TRUE, "$", "Chilean Peso" },
	{ "CNY", 2, ".", ",", TRUE, "¥", "Yuan Renminbi" },
	{ "COP", 2, ",", ".", TRUE, "$", "Colombian Peso" },
	{ "COU", 2, ".", "", FALSE, "COU", "Unidad de Valor Real" },
	{ "CRC", 0, ",", ".", TRUE, "₡", "Costa Rican Colon" },
	{ "CUP", 2, ".", ",", TRUE, "$", "Cuban Peso" },
	{ "CVE", 2, "$", " ", FALSE, "​", "Cape Verde Escudo" },
	{ "CYP", 2, ".", "", FALSE, "CYP", "Cyprus Pound" },
	{ "CZK", 2, ",", " ", FALSE, "Kč", "Czech Koruna" },
	{ "DJF", 0, ",", " ", TRUE, "Fdj", "Djibouti Franc" },
	{ "DKK", 2, ",", ".", TRUE, "kr", "Danish Krone" },
	{ "DOP", 2, ".", ",", TRUE, "$", "Dominican Peso" },
	{ "DZD", 2, ",", " ", FALSE, "DA", "Algerian Dinar" },
	{ "EEK", 2, ".", "", FALSE, "EEK", "Kroon" },
	{ "EGP", 2, ".", ",", TRUE, "ج.م.‏", "Egyptian Pound" },
	{ "ERN", 2, ".", ",", TRUE, "Nfk", "Nakfa" },
	{ "ETB", 2, ".", ",", TRUE, "Br", "Ethiopian Birr" },
	{ "EUR", 2, ",", " ", TRUE, "€", "Euro" },
	{ "FJD", 2, ".", ",", TRUE, "$", "Fiji Dollar" },
	{ "FKP", 2, ".", ",", TRUE, "£", "Falkland Islands Pound" },
	{ "GBP", 2, ".", ",", TRUE, "£", "Pound Sterling" },
	{ "GEL", 2, ",", " ", TRUE, "₾", "Lari" },
	{ "GHS", 2, ".", ",", TRUE, "GH₵", "Ghana Cedi" },
	{ "GIP", 2, ".", ",", TRUE, "£", "Gibraltar Pound" },
	{ "GMD", 2, ".", ",", TRUE, "D", "Dalasi" },
	{ "GNF", 0, ",", " ", TRUE, "FG", "Guinea Franc" },
	{ "GTQ", 2, ".", ",", TRUE, "Q", "Quetzal" },
	{ "GYD", 2, ".", ",", TRUE, "$", "Guyana Dollar" },
	{ "HKD", 2, ".", ",", TRUE, "$", "Hong Kong Dollar" },
	{ "HNL", 2, ".", ",", TRUE, "L", "Lempira" },
	{ "HRK", 2, ",", ".", FALSE, "kn", "Croatian Kuna" },
	{ "HTG", 2, ",", " ", FALSE, "G", "Gourde" },
	{ "HUF", 2, ",", " ", FALSE, "HUF", "Forint" },
	{ "IDR", 2, ",", ".", TRUE, "Rp", "Rupiah" },
	{ "ILS", 2, ".", ",", TRUE, "₪", "New Israeli Sheqel" },
	{ "INR", 2, ".", ",", TRUE, "₹", "Indian Rupee" },
	{ "IQD", 3, ".", ",", TRUE, "د.ع.‏", "Iraqi Dinar" },
	{ "IRR", 2, "/", ",", TRUE, "ريال", "Iranian Rial" },
	{ "ISK", 0, ",", ".", FALSE, "ISK", "Iceland Krona" },
	{ "JMD", 2, ".", ",", TRUE, "$", "Jamaican Dollar" },
	{ "JOD", 3, ".", ",", TRUE, "د.ا.‏", "Jordanian Dinar" },
	{ "JPY", 0, ".", ",", TRUE, "¥", "Yen" },
	{ "KES", 2, ".", ",", TRUE, "Ksh", "Kenyan Shilling" },
	{ "KGS", 2, ",", " ", FALSE, "сом", "Som" },
	{ "KHR", 2, ".", ",", FALSE, "៛", "Riel" },
	{ "KMF", 0, ",", " ", TRUE, "CF", "Comoro Franc" },
	{ "KPW", 2, ".", "", FALSE, "KPW", "North Korean Won" },
	{ "KRW", 0, ".", ",", TRUE, "₩", "Won" },
	{ "KWD", 3, ".", ",", TRUE, "د.ك.‏", "Kuwaiti Dinar" },
	{ "KYD", 2, ".", ",", TRUE, "$", "Cayman Islands Dollar" },
	{ "KZT", 2, ",", " ", FALSE, "₸", "Tenge" },
	{ "LAK", 2, ",", ".", TRUE, "₭", "Kip" },
	{ "LBP", 2, ".", ",", TRUE, "ل.ل.‏", "Lebanese Pound" },
	{ "LKR", 2, ".", ",", TRUE, "Rs.", "Sri Lanka Rupee" },
	{ "LRD", 2, ".", ",", TRUE, "$", "Liberian Dollar" },
	{ "LSL", 2, ".", "", FALSE, "LSL", "Loti" },
	{ "LTL", 2, ".", "", FALSE, "LTL", "Lithuanian Litas" },
	{ "LVL", 2, ".", "", FALSE, "LVL", "Latvian Lats" },
	{ "LYD", 3, ".", ",", TRUE, "د.ل.‏", "Libyan Dinar" },
	{ "MAD", 2, ",", " ", FALSE, "DH", "Moroccan Dirham" },
	{ "MDL", 2, ",", " ", FALSE, "L", "Moldovan Leu" },
	{ "MGA", 2, ",", " ", TRUE, "Ar", "Malagasy Ariary" },
	{ "MKD", 2, ",", " ", TRUE, "den", "Denar" },
	{ "MMK", 2, ".", ",", TRUE, "K", "Kyat" },
	{ "MNT", 2, ".", ",", TRUE, "₮", "Tugrik" },
	{ "MOP", 2, ",", " ", TRUE, "MOP", "Pataca" },
	{ "MRO", 0, ",", " ", TRUE, "UM", "Ouguiya" },
	{ "MTL", 2, ".", "", FALSE, "MTL", "Maltese Lira" },
	{ "MUR", 2, ",", " ", TRUE, "Rs", "Mauritius Rupee" },
	{ "MVR", 2, ".", ",", FALSE, "ރ.", "Rufiyaa" },
	{ "MWK", 2, ".", ",", TRUE, "MK", "Kwacha" },
	{ "MXN", 2, ".", ",", TRUE, "$", "Mexican Peso" },
	{ "MXV", 2, ".", "", FALSE, "MXV", "Mexican Unidad de Inversion (UDI)" },
	{ "MYR", 2, ".", ",", TRUE, "RM", "Malaysian Ringgit" },
	{ "MZN", 2, ",", " ", FALSE, "MTn", "Metical" },
	{ "NAD", 2, ",", " ", TRUE, "$", "Namibia Dollar" },
	{ "NGN", 2, ".", ",", TRUE, "₦", "Naira" },
	{ "NIO", 2, ".", ",", TRUE, "C$", "Cordoba Oro" },
	{ "NOK", 2, ",", " ", TRUE, "kr", "Norwegian Krone" },
	{ "NPR", 2, ".", ",", TRUE, "रु", "Nepalese Rupee" },
	{ "NZD", 2, ".", ",", TRUE, "$", "New Zealand Dollar" },
	{ "OMR", 3, ".", ",", TRUE, "ر.ع.‏", "Rial Omani" },
	{ "PAB", 2, ".", ",", TRUE, "B/.", "Balboa" },
	{ "PEN", 2, ".", ",", TRUE, "S/.", "Nuevo Sol" },
	{ "PGK", 2, ".", ",", TRUE, "K", "Kina" },
	{ "PHP", 2, ",", ",", TRUE, "₱", "Philippine Peso" },
	{ "PKR", 2, ".", ",", TRUE, "Rs", "Pakistan Rupee" },
	{ "PLN", 2, ",", " ", FALSE, "zł", "Zloty" },
	{ "PYG", 0, ",", ".", TRUE, "₲", "Guarani" },
	{ "QAR", 2, ".", ",", TRUE, "ر.ق.‏", "Qatari Rial" },
	{ "RON", 2, ",", ".", FALSE, "RON", "New Leu" },
	{ "RSD", 2, ",", ".", FALSE, "RSD", "Serbian Dinar" },
	{ "RUB", 2, ",", " ", TRUE, "₽", "Russian Ruble" },
	{ "RWF", 0, ",", " ", TRUE, "RF", "Rwanda Franc" },
	{ "SAR", 2, ".", ",", TRUE, "ر.س.‏", "Saudi Riyal" },
	{ "SBD", 2, ".", ",", TRUE, "$", "Solomon Islands Dollar" },
	{ "SCR", 2, ",", " ", TRUE, "SR", "Seychelles Rupee" },
	{ "SDG", 2, ".", ",", TRUE, "SDG", "Sudanese Pound" },
	{ "SEK", 2, ",", ".", FALSE, "kr", "Swedish Krona" },
	{ "SGD", 2, ".", ",", TRUE, "$", "Singapore Dollar" },
	{ "SHP", 2, ".", ",", TRUE, "£", "Saint Helena Pound" },
	{ "SLL", 2, ".", ",", TRUE, "Le", "Leone" },
	{ "SOS", 2, ".", ",", TRUE, "S", "Somali Shilling" },
	{ "SRD", 2, ",", ".", TRUE, "$", "Surinam Dollar" },
	{ "STD", 0, ",", " ", FALSE, "Db", "Dobra" },
	{ "SVC", 2, ".", "", FALSE, "SVC", "El Salvador Colon" },
	{ "SYP", 2, ",", " ", TRUE, "LS", "Syrian Pound" },
	{ "SZL", 2, ",", " ", TRUE, "E", "Lilangeni" },
	{ "THB", 2, ".", ",", TRUE, "฿", "Baht" },
	{ "TJS", 2, ",", " ", FALSE, "смн", "Somoni" },
	{ "TMM", 2, ".", "", FALSE, "TMM", "Manat" },
	{ "TND", 3, ",", " ", TRUE, "DT", "Tunisian Dinar" },
	{ "TOP", 2, ".", ",", TRUE, "T$", "Pa'anga" },
	{ "TRY", 2, ",", ".", FALSE, "₺", "New Turkish Lira" },
	{ "TTD", 2, ".", ",", TRUE, "$", "Trinidad and Tobago Dollar" },
	{ "TWD", 2, ".", ",", TRUE, "NT$", "New Taiwan Dollar" },
	{ "TZS", 2, ".", ",", TRUE, "TSh", "Tanzanian Shilling" },
	{ "UAH", 2, ",", " ", FALSE, "₴", "Hryvnia" },
	{ "UGX", 0, ".", ",", TRUE, "USh", "Uganda Shilling" },
	{ "USD", 2, ",", " ", TRUE, "$", "US Dollar" },
	{ "USN", 2, ".", "", FALSE, "USN", "US Dollar (Next day)" },
	{ "USS", 2, ".", "", FALSE, "USS", "US Dollar (Same day)" },
	{ "UYI", 0, ".", "", FALSE, "UYI", "Uruguay Peso en Unidades Indexadas" },
	{ "UYU", 2, ",", ".", TRUE, "$", "Peso Uruguayo" },
	{ "UZS", 2, ",", " ", TRUE, "soʻm", "Uzbekistan Sum" },
	{ "VEF", 2, ",", ".", TRUE, "Bs.", "Bolivar Fuerte" },
	{ "VND", 0, ",", ".", FALSE, "₫", "Dong" },
	{ "VUV", 0, ",", " ", TRUE, "VT", "Vatu" },
	{ "WST", 2, ".", ",", TRUE, "WS$", "Tala" },
	{ "XAF", 0, ",", " ", TRUE, "FCFA", "CFA Franc BEAC" },
	{ "XAG", 2, ".", "", FALSE, "XAG", "Silver" },
	{ "XAU", 2, ".", "", FALSE, "XAU", "Gold" },
	{ "XBA", 2, ".", "", FALSE, "XBA", "European Composite Unit (EURCO)" },
	{ "XBB", 2, ".", "", FALSE, "XBB", "European Monetary Unit (E.M.U.-6)" },
	{ "XBC", 2, ".", "", FALSE, "XBC", "European Unit of Account 9 (E.U.A.-9)" },
	{ "XBD", 2, ".", "", FALSE, "XBD", "European Unit of Account 17 (E.U.A.-17)" },
	{ "XCD", 2, ",", " ", TRUE, "$", "East Caribbean Dollar" },
	{ "XDR", 2, ",", " ", TRUE, "XDR", "Special Drawing Rights" },
	{ "XFO", 0, ".", "", FALSE, "XFO", "Gold-Franc" },
	{ "XFU", 2, ".", "", FALSE, "XFU", "UIC-Franc" },
	{ "XOF", 0, ",", " ", TRUE, "CFA", "CFA Franc BCEAO" },
	{ "XPD", 2, ".", "", FALSE, "XPD", "Palladium" },
	{ "XPF", 0, ",", " ", FALSE, "FCFP", "CFP Franc" },
	{ "XPT", 2, ".", "", FALSE, "XPT", "Platinum" },
	{ "XTS", 2, ".", "", FALSE, "XTS", "Code for testing purposes" },
	{ "XXX", 2, ".", "", FALSE, "XXX", "No currency" },
	{ "YER", 2, ".", ",", TRUE, "ر.ي.‏", "Yemeni Rial" },
	{ "ZAR", 2, ",", " ", TRUE, "R", "Rand" },
	{ "ZMK", 2, ".", "", FALSE, "ZMK", "Zambian Kwacha" },
	{ "ZWD", 2, ".", "", FALSE, "ZWD", "Zimbabwe Dollar" }
};
guint n_iso4217cur = G_N_ELEMENTS (iso4217cur);

