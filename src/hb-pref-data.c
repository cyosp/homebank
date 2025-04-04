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


#include "homebank.h"

#include "hb-pref-data.h"

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


/*
source:
 http://en.wikipedia.org/wiki/Currencies_of_the_European_Union
 http://www.xe.com/euro.php
 http://fr.wikipedia.org/wiki/Liste_des_unit%C3%A9s_mon%C3%A9taires_remplac%C3%A9es_par_l%27euro
 http://www.inter-locale.com/LocalesDemo.jsp
*/
EuroParams euro_params[] =
{
//    id, mceii, ISO  , country           , rate    , symb  , prfx , dec, grp, frac
// ---------------------------------------------------------------------
	{  0, TRUE , ""   , "--------"        , 0.0		, ""    , FALSE, ",", ".", 2  },
	{  1, TRUE , "ATS", "Austria"         , 13.7603	, "S"   , TRUE , ",", ".", 2  },	// -S 1.234.567,89
	{  2, TRUE , "BEF", "Belgium"         , 40.3399	, "BF"  , TRUE , ",", ".", 2  },	// BF 1.234.567,89 -
	{ 20, FALSE, "BGN", "Bulgaria*"       , 1.95583	, "лв." , TRUE , ",", " ", 2  },	// fixé
	{ 24, TRUE , "HRK", "Croatia"         , 7.5345  , "kn"  , FALSE, "" , ".", 0  },	//
	{ 14, TRUE , "CYP", "Cyprus"          , 0.585274, "£"   , TRUE , ",", "" , 2  },	//
	{ 23, FALSE, "CZK", "Czech Republic*" , 0.0 	, "Kč"  , FALSE, ",", " ", 2  },	// non-fixé - 2015 earliest
	{ 26, FALSE, "DKK", "Denmark*"        , 7.4603	, "kr." , TRUE , ".", ",", 2  },	// fixé
	{ 17, TRUE , "EEK", "Estonia"         , 15.6466 , "kr"  , FALSE, ",", " ", 2  },	//
	{  3, TRUE , "FIM", "Finland"         , 5.94573	, "mk"  , FALSE, ",", " ", 2  },	// -1 234 567,89 mk
	{  4, TRUE , "FRF", "France"          , 6.55957	, "F"   , FALSE, ",", " ", 2  },	// -1 234 567,89 F
	{  5, TRUE , "DEM", "Germany"         , 1.95583	, "DM"  , FALSE, ",", ".", 2  },	// -1.234.567,89 DM
	{  6, TRUE , "GRD", "Greece"          , 340.750	, "d"   , TRUE , ".", ",", 2  },	// ??
	{ 21, FALSE, "HUF", "Hungary*"        , 0.0  	, "Ft"  , TRUE , ",", " ", 2  },	// non-fixé - No current target for euro
	{  7, TRUE , "IEP", "Ireland"         , 0.787564, "£"   , TRUE , ".", ",", 2  },	// -£1,234,567.89
	{  8, TRUE , "ITL", "Italy"           , 1936.27	, "L"   , TRUE , "" , ".", 0  },	// L -1.234.567
	{ 18, TRUE , "LVL", "Latvia"          , 0.702804, "lat.", FALSE, ",", "" , 2  },	// jan. 2014
	{ 19, TRUE , "LTL", "Lithuania"       , 3.45280	, "Lt"  , FALSE, ",", "" , 2  },	// jan. 2015
	{  9, TRUE , "LUF", "Luxembourg"      , 40.3399	, "LU"  , TRUE , ",", ".", 2  },	// LU 1.234.567,89 -
	{ 15, TRUE , "MTL", "Malta"           , 0.429300, "Lm"  , TRUE , ",", "" , 2  },	//
	{ 10, TRUE , "NLG", "Netherlands"     , 2.20371	, "F"   , TRUE , ",", ".", 2  },	// F 1.234.567,89-
	{ 25, FALSE, "PLN", "Poland*"         , 0.0     , "zł"  , FALSE, ",", "" , 2  },	// non-fixé - No current target for euro
	{ 11, TRUE , "PTE", "Portugal"        , 200.482	, "Esc.", FALSE, "$", ".", 2  },	// -1.234.567$89 Esc.
	{ 22, FALSE, "RON", "Romania*"        , 0.0  	, "Leu" , FALSE, ",", ".", 2  },	// non-fixé - 2015 target for euro earliest
	{ 16, TRUE , "SKK", "Slovak Republic" , 30.12600, "Sk"  , FALSE, ",", " ", 2  },	//
	{ 13, TRUE , "SIT", "Slovenia"        , 239.640	, "tol" , TRUE , ",", ".", 2  },	//
	{ 12, TRUE , "ESP", "Spain"           , 166.386	, "Pts" , TRUE , "" , ".", 0  },	// -Pts 1.234.567
	{ 27, FALSE, "SEK", "Sweden*"         , 0.0	    , "kr"  , FALSE, ",", " ", 2  },	// non-fixé
	//United Kingdom
//	{ "   ", ""    , 1.00000	, ""   , ""  , FALSE, ",", "", 2  },
};

guint nb_euro_params = G_N_ELEMENTS(euro_params);


//European — €1.234.567,89 EUR
EuroParams euro_params_euro =
{
//    id, mceii, ISO  , country           , rate    , symb  , prfx , dec, grp, frac
// ---------------------------------------------------------------------
	  0, TRUE , "EUR"   , "Non MCEII"     , 0.0	, "€"    , TRUE, ",", ".", 2  ,
};



EuroParams *euro_country_get(guint ctryid)
{
	DB( g_print("\n[pref-data] euro_country_get\n") );

	for (guint i = 0; i < G_N_ELEMENTS (euro_params); i++)
	{
		if( euro_params[i].id == ctryid )
		{
			return &euro_params[i];
		}
	}
	return NULL;
}


gboolean euro_country_is_mceii(gint ctryid)
{
gboolean retval = FALSE;

	DB( g_print("\n[pref-data] euro_country_is_mceii\n") );
	for (guint i = 0; i < G_N_ELEMENTS (euro_params); i++)
	{
		if( euro_params[i].id == ctryid )
		{
			retval = euro_params[i].mceii;
			DB( g_print(" id (country)=%d => %d mceii %d\n", ctryid, i, euro_params[i].mceii) );
			break;
		}
	}

	return retval;
}


gboolean euro_country_notmceii_rate_update(guint ctryid)
{
	DB( g_print("\n[pref-data] euro_country_notmceii_rate_update\n") );

	if( PREFS->euro_mceii == FALSE )
	{
	Currency *base = da_cur_get (GLOBALS->kcur);
	EuroParams *ctry = euro_country_get(ctryid);

		if( base && ctry )
		{
			DB( g_print(" check update minor rate: %s == %s ?\n", base->iso_code, ctry->iso ) );
			if( !strcmp(base->iso_code, ctry->iso) )
			{
			Currency *eur = da_cur_get_by_iso_code("EUR");
			
				if( eur != NULL )
				{
					PREFS->euro_value = eur->rate;
					DB( g_print(" >update euro minor rate to %.6f for %s\n", eur->rate, ctry->iso ) );
					return TRUE;
				}
			}
			else
			{
				DB( g_print(" >skip: base is different\n" ) );
			}
		}
	}
	return FALSE;
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


LangName languagenames[] =
{
// af ar ast be bg ca cs cy da de el en_AU en_CA en_GB es et eu fa fi fr ga gl he hr hu id is it 
//ja ka ko lt lv ms nb nds nl oc pl pt_BR pt pt_PT ro ru si sk sl sr sv tr uk vi zh_CN zh_TW
	
	{ "aa", "Afar" },
	{ "ab", "Abkhazian" },
	{ "ae", "Avestan" },
	{ "af", "Afrikaans" },
	{ "ak", "Akan" },
	{ "am", "Amharic" },
	{ "an", "Aragonese" },
	{ "ar", "Arabic" },
	{ "as", "Assamese" },
	{ "ast", "Asturian, Bable, Leonese, Asturleonese" },
	{ "av", "Avaric" },
	{ "ay", "Aymara" },
	{ "az", "Azerbaijani" },
	{ "ba", "Bashkir" },
	{ "be", "Belarusian" },
	{ "bg", "Bulgarian" },
	{ "bh", "Bihari" },
	{ "bi", "Bislama" },
	{ "bm", "Bambara" },
	{ "bn", "Bengali" },
	{ "bo", "Tibetan" },
	{ "br", "Breton" },
	{ "bs", "Bosnian" },
	{ "ca", "Catalan" },
	{ "ce", "Chechen" },
	{ "ch", "Chamorro" },
	{ "ckb", "Kurdish, Central" },
	{ "co", "Corsican" },
	{ "cr", "Cree" },
	{ "cs", "Czech" },
	{ "cu", "Old Church Slavonic" },
	{ "cv", "Chuvash" },
	{ "cy", "Welsh" },
	{ "da", "Danish" },
	{ "de", "German" },
	{ "dv", "Divehi" },
	{ "dz", "Dzongkha" },
	{ "ee", "Ewe" },
	{ "el", "Greek" },
	{ "en", "English" },
	{ "eo", "Esperanto" },
	{ "es", "Spanish" },
	{ "et", "Estonian" },
	{ "eu", "Basque" },
	{ "fa", "Persian" },
	{ "ff", "Fulah" },
	{ "fi", "Finnish" },
	{ "fj", "Fijian" },
	{ "fo", "Faroese" },
	{ "fr", "French" },
	{ "fy", "Western Frisian" },
	{ "ga", "Irish" },
	{ "gd", "Scottish Gaelic" },
	{ "gl", "Galician" },
	{ "gn", "Guarani" },
	{ "gu", "Gujarati" },
	{ "gv", "Manx" },
	{ "ha", "Hausa" },
	{ "he", "Hebrew" },
	{ "hi", "Hindi" },
	{ "ho", "Hiri Motu" },
	{ "hr", "Croatian" },
	{ "ht", "Haitian" },
	{ "hu", "Hungarian" },
	{ "hy", "Armenian" },
	{ "hz", "Herero" },
	{ "ia", "Interlingua" },
	{ "id", "Indonesian" },
	{ "ie", "Interlingue" },
	{ "ig", "Igbo" },
	{ "ii", "Sichuan Yi" },
	{ "ik", "Inupiaq" },
	{ "io", "Ido" },
	{ "is", "Icelandic" },
	{ "it", "Italian" },
	{ "iu", "Inuktitut" },
	{ "ja", "Japanese" },
	{ "jv", "Javanese" },
	{ "ka", "Georgian" },
	{ "kg", "Kongo" },
	{ "ki", "Kikuyu" },
	{ "kj", "Kwanyama" },
	{ "kk", "Kazakh" },
	{ "kl", "Kalaallisut" },
	{ "km", "Khmer" },
	{ "kn", "Kannada" },
	{ "ko", "Korean" },
	{ "kr", "Kanuri" },
	{ "ks", "Kashmiri" },
	{ "ku", "Kurdish" },
	{ "kv", "Komi" },
	{ "kw", "Cornish" },
	{ "ky", "Kirghiz" },
	{ "la", "Latin" },
	{ "lb", "Luxembourgish" },
	{ "lg", "Ganda" },
	{ "li", "Limburgish" },
	{ "ln", "Lingala" },
	{ "lo", "Lao" },
	{ "lt", "Lithuanian" },
	{ "lu", "Luba-Katanga" },
	{ "lv", "Latvian" },
	{ "mg", "Malagasy" },
	{ "mh", "Marshallese" },
	{ "mi", "Māori" },
	{ "mk", "Macedonian" },
	{ "ml", "Malayalam" },
	{ "mn", "Mongolian" },
	{ "mo", "Moldavian" },
	{ "mr", "Marathi" },
	{ "ms", "Malay" },
	{ "mt", "Maltese" },
	{ "my", "Burmese" },
	{ "na", "Nauru" },
	{ "nb", "Norwegian Bokmål" },
	{ "nd", "North Ndebele" },
	{ "nds", "Low German, Low Saxon" },
	{ "ne", "Nepali" },
	{ "ng", "Ndonga" },
	{ "nl", "Dutch" },
	{ "nn", "Norwegian Nynorsk" },
	{ "no", "Norwegian" },
	{ "nr", "South Ndebele" },
	{ "nv", "Navajo" },
	{ "ny", "Chichewa" },
	{ "oc", "Occitan" },
	{ "oj", "Ojibwa" },
	{ "om", "Oromo" },
	{ "or", "Oriya" },
	{ "os", "Ossetian" },
	{ "pa", "Panjabi" },
	{ "pi", "Pāli" },
	{ "pl", "Polish" },
	{ "ps", "Pashto" },
	{ "pt", "Portuguese" },
	{ "qu", "Quechua" },
	{ "rm", "Romansh" },
	{ "rn", "Kirundi" },
	{ "ro", "Romanian" },
	{ "ru", "Russian" },
	{ "rw", "Kinyarwanda" },
	{ "sa", "Sanskrit" },
	{ "sc", "Sardinian" },
	{ "sd", "Sindhi" },
	{ "se", "Northern Sami" },
	{ "sg", "Sango" },
	{ "si", "Sinhalese" },
	{ "sk", "Slovak" },
	{ "sl", "Slovene" },
	{ "sm", "Samoan" },
	{ "sn", "Shona" },
	{ "so", "Somali" },
	{ "sq", "Albanian" },
	{ "sr", "Serbian" },
	{ "ss", "Swati" },
	{ "st", "Sotho" },
	{ "su", "Sundanese" },
	{ "sv", "Swedish" },
	{ "sw", "Swahili" },
	{ "ta", "Tamil" },
	{ "te", "Telugu" },
	{ "tg", "Tajik" },
	{ "th", "Thai" },
	{ "ti", "Tigrinya" },
	{ "tk", "Turkmen" },
	{ "tl", "Tagalog" },
	{ "tn", "Tswana" },
	{ "to", "Tonga" },
	{ "tr", "Turkish" },
	{ "ts", "Tsonga" },
	{ "tt", "Tatar" },
	{ "tw", "Twi" },
	{ "ty", "Tahitian" },
	{ "ug", "Uighur" },
	{ "uk", "Ukrainian" },
	{ "ur", "Urdu" },
	{ "uz", "Uzbek" },
	{ "ve", "Venda" },
	{ "vi", "Viêt Namese" },
	{ "vo", "Volapük" },
	{ "wa", "Walloon" },
	{ "wo", "Wolof" },
	{ "xh", "Xhosa" },
	{ "yi", "Yiddish" },
	{ "yo", "Yoruba" },
	{ "za", "Zhuang" },
	{ "zh", "Chinese" },
	{ "zu", "Zulu" }

};


gchar *languagename_get(const gchar *locale)
{
	DB( g_print("\n[pref-data] languagename_get\n") );

	for (guint i = 0; i < G_N_ELEMENTS (languagenames); i++)
	{
		if( g_ascii_strncasecmp(locale, languagenames[i].locale, -1) == 0 )
			return languagenames[i].name;
	}

	return NULL;
}

