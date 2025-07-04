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

/* Win32 language lookup table:
 * Copyright (C) 2007-2008 Dieter Verfaillie <dieterv@optionexplicit.be>
 */

#include "homebank.h"

#include <locale.h>
#include <glib.h>

#ifdef G_OS_WIN32
#define WINVER 0x0501
#define _WIN32_WINNT   0x0501
#define _WIN32_WINDOWS 0x0501

#include <windows.h>
#include <winnls.h>
#endif

#include "language.h"

/****************************************************************************/
/* Debug macros																*/
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif


void
language_init (const gchar *language)
{

	DB( g_print ("\n[language] init\n") );

	DB( g_print (" pref is '%s'\n", language) );

#ifdef G_OS_WIN32
  if (! language)
    {
      /* FIXME: This is a hack. gettext doesn't pick the right language
       * by default on Windows, so we enforce the right one. The
       * following code is an adaptation of Python code from
       * pynicotine. For reasons why this approach is needed, and why
       * the GetLocaleInfo() approach in other libs falls flat, see:
       * http://blogs.msdn.com/b/michkap/archive/2007/04/15/2146890.aspx
       */

      switch (GetUserDefaultUILanguage())
        {
        case 1078:
          language = "af";    /* Afrikaans - South Africa */
          break;
        case 1052:
          language = "sq";    /* Albanian - Albania */
          break;
        case 1118:
          language = "am";    /* Amharic - Ethiopia */
          break;
        case 1025:
          language = "ar";    /* Arabic - Saudi Arabia */
          break;
        case 5121:
          language = "ar";    /* Arabic - Algeria */
          break;
        case 15361:
          language = "ar";    /* Arabic - Bahrain */
          break;
        case 3073:
          language = "ar";    /* Arabic - Egypt */
          break;
        case 2049:
          language = "ar";    /* Arabic - Iraq */
          break;
        case 11265:
          language = "ar";    /* Arabic - Jordan */
          break;
        case 13313:
          language = "ar";    /* Arabic - Kuwait */
          break;
        case 12289:
          language = "ar";    /* Arabic - Lebanon */
          break;
        case 4097:
          language = "ar";    /* Arabic - Libya */
          break;
        case 6145:
          language = "ar";    /* Arabic - Morocco */
          break;
        case 8193:
          language = "ar";    /* Arabic - Oman */
          break;
        case 16385:
          language = "ar";    /* Arabic - Qatar */
          break;
        case 10241:
          language = "ar";    /* Arabic - Syria */
          break;
        case 7169:
          language = "ar";    /* Arabic - Tunisia */
          break;
        case 14337:
          language = "ar";    /* Arabic - U.A.E. */
          break;
        case 9217:
          language = "ar";    /* Arabic - Yemen */
          break;
        case 1067:
          language = "hy";    /* Armenian - Armenia */
          break;
        case 1101:
          language = "as";    /* Assamese */
          break;
        case 2092:
          language = NULL;    /* Azeri (Cyrillic) */
          break;
        case 1068:
          language = NULL;    /* Azeri (Latin) */
          break;
        case 1069:
          language = "eu";    /* Basque */
          break;
        case 1059:
          language = "be";    /* Belarusian */
          break;
        case 1093:
          language = "bn";    /* Bengali (India) */
          break;
        case 2117:
          language = "bn";    /* Bengali (Bangladesh) */
          break;
        case 5146:
          language = "bs";    /* Bosnian (Bosnia/Herzegovina) */
          break;
        case 1026:
          language = "bg";    /* Bulgarian */
          break;
        case 1109:
          language = "my";    /* Burmese */
          break;
        case 1027:
          language = "ca";    /* Catalan */
          break;
        case 1116:
          language = NULL;    /* Cherokee - United States */
          break;
        case 2052:
          language = "zh";    /* Chinese - People"s Republic of China */
          break;
        case 4100:
          language = "zh";    /* Chinese - Singapore */
          break;
        case 1028:
          language = "zh";    /* Chinese - Taiwan */
          break;
        case 3076:
          language = "zh";    /* Chinese - Hong Kong SAR */
          break;
        case 5124:
          language = "zh";    /* Chinese - Macao SAR */
          break;
        case 1050:
          language = "hr";    /* Croatian */
          break;
        case 4122:
          language = "hr";    /* Croatian (Bosnia/Herzegovina) */
          break;
        case 1029:
          language = "cs";    /* Czech */
          break;
        case 1030:
          language = "da";    /* Danish */
          break;
        case 1125:
          language = "dv";    /* Divehi */
          break;
        case 1043:
          language = "nl";    /* Dutch - Netherlands */
          break;
        case 2067:
          language = "nl";    /* Dutch - Belgium */
          break;
        case 1126:
          language = NULL;    /* Edo */
          break;
        case 1033:
          language = "en";    /* English - United States */
          break;
        case 2057:
          language = "en";    /* English - United Kingdom */
          break;
        case 3081:
          language = "en";    /* English - Australia */
          break;
        case 10249:
          language = "en";    /* English - Belize */
          break;
        case 4105:
          language = "en";    /* English - Canada */
          break;
        case 9225:
          language = "en";    /* English - Caribbean */
          break;
        case 15369:
          language = "en";    /* English - Hong Kong SAR */
          break;
        case 16393:
          language = "en";    /* English - India */
          break;
        case 14345:
          language = "en";    /* English - Indonesia */
          break;
        case 6153:
          language = "en";    /* English - Ireland */
          break;
        case 8201:
          language = "en";    /* English - Jamaica */
          break;
        case 17417:
          language = "en";    /* English - Malaysia */
          break;
        case 5129:
          language = "en";    /* English - New Zealand */
          break;
        case 13321:
          language = "en";    /* English - Philippines */
          break;
        case 18441:
          language = "en";    /* English - Singapore */
          break;
        case 7177:
          language = "en";    /* English - South Africa */
          break;
        case 11273:
          language = "en";    /* English - Trinidad */
          break;
        case 12297:
          language = "en";    /* English - Zimbabwe */
          break;
        case 1061:
          language = "et";    /* Estonian */
          break;
        case 1080:
          language = "fo";    /* Faroese */
          break;
        case 1065:
          language = NULL;    /* Farsi */
          break;
        case 1124:
          language = NULL;    /* Filipino */
          break;
        case 1035:
          language = "fi";    /* Finnish */
          break;
        case 1036:
          language = "fr";    /* French - France */
          break;
        case 2060:
          language = "fr";    /* French - Belgium */
          break;
        case 11276:
          language = "fr";    /* French - Cameroon */
          break;
        case 3084:
          language = "fr";    /* French - Canada */
          break;
        case 9228:
          language = "fr";    /* French - Democratic Rep. of Congo */
          break;
        case 12300:
          language = "fr";    /* French - Cote d"Ivoire */
          break;
        case 15372:
          language = "fr";    /* French - Haiti */
          break;
        case 5132:
          language = "fr";    /* French - Luxembourg */
          break;
        case 13324:
          language = "fr";    /* French - Mali */
          break;
        case 6156:
          language = "fr";    /* French - Monaco */
          break;
        case 14348:
          language = "fr";    /* French - Morocco */
          break;
        case 58380:
          language = "fr";    /* French - North Africa */
          break;
        case 8204:
          language = "fr";    /* French - Reunion */
          break;
        case 10252:
          language = "fr";    /* French - Senegal */
          break;
        case 4108:
          language = "fr";    /* French - Switzerland */
          break;
        case 7180:
          language = "fr";    /* French - West Indies */
          break;
        case 1122:
          language = "fy";    /* Frisian - Netherlands */
          break;
        case 1127:
          language = NULL;    /* Fulfulde - Nigeria */
          break;
        case 1071:
          language = "mk";    /* FYRO Macedonian */
          break;
        case 2108:
          language = "ga";    /* Gaelic (Ireland) */
          break;
        case 1084:
          language = "gd";    /* Gaelic (Scotland) */
          break;
        case 1110:
          language = "gl";    /* Galician */
          break;
        case 1079:
          language = "ka";    /* Georgian */
          break;
        case 1031:
          language = "de";    /* German - Germany */
          break;
        case 3079:
          language = "de";    /* German - Austria */
          break;
        case 5127:
          language = "de";    /* German - Liechtenstein */
          break;
        case 4103:
          language = "de";    /* German - Luxembourg */
          break;
        case 2055:
          language = "de";    /* German - Switzerland */
          break;
        case 1032:
          language = "el";    /* Greek */
          break;
        case 1140:
          language = "gn";    /* Guarani - Paraguay */
          break;
        case 1095:
          language = "gu";    /* Gujarati */
          break;
        case 1128:
          language = "ha";    /* Hausa - Nigeria */
          break;
        case 1141:
          language = NULL;    /* Hawaiian - United States */
          break;
        case 1037:
          language = "he";    /* Hebrew */
          break;
        case 1081:
          language = "hi";    /* Hindi */
          break;
        case 1038:
          language = "hu";    /* Hungarian */
          break;
        case 1129:
          language = NULL;    /* Ibibio - Nigeria */
          break;
        case 1039:
          language = "is";    /* Icelandic */
          break;
        case 1136:
          language = "ig";    /* Igbo - Nigeria */
          break;
        case 1057:
          language = "id";    /* Indonesian */
          break;
        case 1117:
          language = "iu";    /* Inuktitut */
          break;
        case 1040:
          language = "it";    /* Italian - Italy */
          break;
        case 2064:
          language = "it";    /* Italian - Switzerland */
          break;
        case 1041:
          language = "ja";    /* Japanese */
          break;
        case 1099:
          language = "kn";    /* Kannada */
          break;
        case 1137:
          language = "kr";    /* Kanuri - Nigeria */
          break;
        case 2144:
          language = "ks";    /* Kashmiri */
          break;
        case 1120:
          language = "ks";    /* Kashmiri (Arabic) */
          break;
        case 1087:
          language = "kk";    /* Kazakh */
          break;
        case 1107:
          language = "km";    /* Khmer */
          break;
        case 1111:
          language = NULL;    /* Konkani */
          break;
        case 1042:
          language = "ko";    /* Korean */
          break;
        case 1088:
          language = "ky";    /* Kyrgyz (Cyrillic) */
          break;
        case 1108:
          language = "lo";    /* Lao */
          break;
        case 1142:
          language = "la";    /* Latin */
          break;
        case 1062:
          language = "lv";    /* Latvian */
          break;
        case 1063:
          language = "lt";    /* Lithuanian */
          break;
        case 1086:
          language = "ms";    /* Malay - Malaysia */
          break;
        case 2110:
          language = "ms";    /* Malay - Brunei Darussalam */
          break;
        case 1100:
          language = "ml";    /* Malayalam */
          break;
        case 1082:
          language = "mt";    /* Maltese */
          break;
        case 1112:
          language = NULL;    /* Manipuri */
          break;
        case 1153:
          language = "mi";    /* Maori - New Zealand */
          break;
        case 1102:
          language = "mr";    /* Marathi */
          break;
        case 1104:
          language = "mn";    /* Mongolian (Cyrillic) */
          break;
        case 2128:
          language = "mn";    /* Mongolian (Mongolian) */
          break;
        case 1121:
          language = "ne";    /* Nepali */
          break;
        case 2145:
          language = "ne";    /* Nepali - India */
          break;
        case 1044:
          language = "no";    /* Norwegian (Bokmￃﾥl) */
          break;
        case 2068:
          language = "no";    /* Norwegian (Nynorsk) */
          break;
        case 1096:
          language = "or";    /* Oriya */
          break;
        case 1138:
          language = "om";    /* Oromo */
          break;
        case 1145:
          language = NULL;    /* Papiamentu */
          break;
        case 1123:
          language = "ps";    /* Pashto */
          break;
        case 1045:
          language = "pl";    /* Polish */
          break;
        case 1046:
          language = "pt";    /* Portuguese - Brazil */
          break;
        case 2070:
          language = "pt";    /* Portuguese - Portugal */
          break;
        case 1094:
          language = "pa";    /* Punjabi */
          break;
        case 2118:
          language = "pa";    /* Punjabi (Pakistan) */
          break;
        case 1131:
          language = "qu";    /* Quecha - Bolivia */
          break;
        case 2155:
          language = "qu";    /* Quecha - Ecuador */
          break;
        case 3179:
          language = "qu";    /* Quecha - Peru */
          break;
        case 1047:
          language = "rm";    /* Rhaeto-Romanic */
          break;
        case 1048:
          language = "ro";    /* Romanian */
          break;
        case 2072:
          language = "ro";    /* Romanian - Moldava */
          break;
        case 1049:
          language = "ru";    /* Russian */
          break;
        case 2073:
          language = "ru";    /* Russian - Moldava */
          break;
        case 1083:
          language = NULL;    /* Sami (Lappish) */
          break;
        case 1103:
          language = "sa";    /* Sanskrit */
          break;
        case 1132:
          language = NULL;    /* Sepedi */
          break;
        case 3098:
          language = "sr";    /* Serbian (Cyrillic) */
          break;
        case 2074:
          language = "sr";    /* Serbian (Latin) */
          break;
        case 1113:
          language = "sd";    /* Sindhi - India */
          break;
        case 2137:
          language = "sd";    /* Sindhi - Pakistan */
          break;
        case 1115:
          language = "si";    /* Sinhalese - Sri Lanka */
          break;
        case 1051:
          language = "sk";    /* Slovak */
          break;
        case 1060:
          language = "sl";    /* Slovenian */
          break;
        case 1143:
          language = "so";    /* Somali */
          break;
        case 1070:
          language = NULL;    /* Sorbian */
          break;
        case 3082:
          language = "es";    /* Spanish - Spain (Modern Sort) */
          break;
        case 1034:
          language = "es";    /* Spanish - Spain (Traditional Sort) */
          break;
        case 11274:
          language = "es";    /* Spanish - Argentina */
          break;
        case 16394:
          language = "es";    /* Spanish - Bolivia */
          break;
        case 13322:
          language = "es";    /* Spanish - Chile */
          break;
        case 9226:
          language = "es";    /* Spanish - Colombia */
          break;
        case 5130:
          language = "es";    /* Spanish - Costa Rica */
          break;
        case 7178:
          language = "es";    /* Spanish - Dominican Republic */
          break;
        case 12298:
          language = "es";    /* Spanish - Ecuador */
          break;
        case 17418:
          language = "es";    /* Spanish - El Salvador */
          break;
        case 4106:
          language = "es";    /* Spanish - Guatemala */
          break;
        case 18442:
          language = "es";    /* Spanish - Honduras */
          break;
        case 58378:
          language = "es";    /* Spanish - Latin America */
          break;
        case 2058:
          language = "es";    /* Spanish - Mexico */
          break;
        case 19466:
          language = "es";    /* Spanish - Nicaragua */
          break;
        case 6154:
          language = "es";    /* Spanish - Panama */
          break;
        case 15370:
          language = "es";    /* Spanish - Paraguay */
          break;
        case 10250:
          language = "es";    /* Spanish - Peru */
          break;
        case 20490:
          language = "es";    /* Spanish - Puerto Rico */
          break;
        case 21514:
          language = "es";    /* Spanish - United States */
          break;
        case 14346:
          language = "es";    /* Spanish - Uruguay */
          break;
        case 8202:
          language = "es";    /* Spanish - Venezuela */
          break;
        case 1072:
          language = NULL;    /* Sutu */
          break;
        case 1089:
          language = "sw";    /* Swahili */
          break;
        case 1053:
          language = "sv";    /* Swedish */
          break;
        case 2077:
          language = "sv";    /* Swedish - Finland */
          break;
        case 1114:
          language = NULL;    /* Syriac */
          break;
        case 1064:
          language = "tg";    /* Tajik */
          break;
        case 1119:
          language = NULL;    /* Tamazight (Arabic) */
          break;
        case 2143:
          language = NULL;    /* Tamazight (Latin) */
          break;
        case 1097:
          language = "ta";    /* Tamil */
          break;
        case 1092:
          language = "tt";    /* Tatar */
          break;
        case 1098:
          language = "te";    /* Telugu */
          break;
        case 1054:
          language = "th";    /* Thai */
          break;
        case 2129:
          language = "bo";    /* Tibetan - Bhutan */
          break;
        case 1105:
          language = "bo";    /* Tibetan - People"s Republic of China */
          break;
        case 2163:
          language = "ti";    /* Tigrigna - Eritrea */
          break;
        case 1139:
          language = "ti";    /* Tigrigna - Ethiopia */
          break;
        case 1073:
          language = "ts";    /* Tsonga */
          break;
        case 1074:
          language = "tn";    /* Tswana */
          break;
        case 1055:
          language = "tr";    /* Turkish */
          break;
        case 1090:
          language = "tk";    /* Turkmen */
          break;
        case 1152:
          language = "ug";    /* Uighur - China */
          break;
        case 1058:
          language = "uk";    /* Ukrainian */
          break;
        case 1056:
          language = "ur";    /* Urdu */
          break;
        case 2080:
          language = "ur";    /* Urdu - India */
          break;
        case 2115:
          language = "uz";    /* Uzbek (Cyrillic) */
          break;
        case 1091:
          language = "uz";    /* Uzbek (Latin) */
          break;
        case 1075:
          language = "ve";    /* Venda */
          break;
        case 1066:
          language = "vi";    /* Vietnamese */
          break;
        case 1106:
          language = "cy";    /* Welsh */
          break;
        case 1076:
          language = "xh";    /* Xhosa */
          break;
        case 1144:
          language = NULL;    /* Yi */
          break;
        case 1085:
          language = "yi";    /* Yiddish */
          break;
        case 1130:
          language = "yo";    /* Yoruba */
          break;
        case 1077:
          language = "zu";    /* Zulu */
          break;
        default:
          language = NULL;
        }
    }

	DB( g_print (" mswin detection is '%s'\n", language) );

#endif

  /*  We already set the locale according to the environment, so just
   *  return early if no language is set in gimprc.
   */
  if (! language)
    return;

	#if MYDEBUG == 1
	const gchar * const *locales = g_get_language_names();
	
	g_print(" system LANGUAGE\n");
	for (; locales != NULL && *locales != NULL; locales++)
	{
		g_print(" %s\n", *locales);
	}
	
    
   #endif

	DB( g_print (" setenv to '%s' and LC_ALL to empty\n", language) );

	g_setenv ("LANGUAGE", language, TRUE);
	setlocale (LC_ALL, "");
}
