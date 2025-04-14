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

#include "ui-widgets.h"


/* = = = = = = = = = = = = = = = = = = = = */

//chart
gchar *CHART_CATEGORY = N_("Category");


/* = = = = = = = = = = = = = = = = = = = = */


//hub, acc, imp
HbKvData CYA_ACC_TYPE[] = 
{
	{ ACC_TYPE_NONE,		N_("(no type)") },
	{ ACC_TYPE_BANK,		N_("Bank")	},
	{ ACC_TYPE_CASH,		N_("Cash")	},
	{ ACC_TYPE_ASSET,		N_("Asset")	},
	{ ACC_TYPE_CREDITCARD,	N_("Credit card") },
	{ ACC_TYPE_LIABILITY,	N_("Liability") },
	{ ACC_TYPE_CHECKING, 	N_("Checking") },
	{ ACC_TYPE_SAVINGS, 	N_("Savings") },

//	{ ACC_TYPE_MUTUALFUND, 	N_("Mutual Fund") },
//	{ ACC_TYPE_INCOME, 		N_("Income") },
//	{ ACC_TYPE_EXPENSE, 	N_("Expense") },
//	{ ACC_TYPE_EQUITY, 		N_("Equity") },

	{ 0, NULL }
};

//bud, cat
gchar *CYA_CAT_TYPE[] = { 
	N_("Expense"), 
	N_("Income"),
	NULL
};

gchar *CYA_ARC_FREQ[] = { 
	N_("Daily"), 
	N_("Weekly"),
	N_("Monthly"),
	N_("Yearly"), 
	NULL
};

gchar *CYA_ARC_FREQ2[] = { 
	N_("day(s)"), 
	N_("week(s)"),
	N_("month(s)"),
	N_("year(s)"), 
	NULL
};


//arc
HbKvData CYA_ARC_ORDINAL[] = {
	{ AUTO_ORDINAL_FIRST,	N_("First") },
	{ AUTO_ORDINAL_SECOND,	N_("Second") },
	{ AUTO_ORDINAL_THIRD,	N_("Third") },
	{ AUTO_ORDINAL_FOURTH,	N_("Fourth") },
	{ AUTO_ORDINAL_LAST,	N_("Last") },
	{ 0, NULL }
};


//arc
HbKvData CYA_ARC_WEEKDAY[] = {
	{ AUTO_WEEKDAY_DAY,	N_("Day") },

	{ AUTO_WEEKDAY_MONDAY,		N_("Monday") },
	{ AUTO_WEEKDAY_TUESDAY,		N_("Tuesday") },
	{ AUTO_WEEKDAY_WEDNESDAY,	N_("Wednesday") },
	{ AUTO_WEEKDAY_THURSDAY,	N_("Thursday") },
	{ AUTO_WEEKDAY_FRIDAY,		N_("Friday") },
	{ AUTO_WEEKDAY_SATURDAY,	N_("Saturday") },
	{ AUTO_WEEKDAY_SUNDAY,		N_("Sunday") },
	{ 0, NULL }
};


//arc
HbKvData CYA_ARC_WEEKEND[] = { 
	{ ARC_WEEKEND_POSSIBLE,	N_("Possible") },
	{ ARC_WEEKEND_BEFORE,	N_("Before") },
	{ ARC_WEEKEND_AFTER,	N_("After") },
	{ ARC_WEEKEND_SKIP, 	N_("Skip") },	//added 5.6
	{ 0, NULL }
};

//txn, arc
gchar *CYA_TXN_TYPE[] = { 
	N_("Expense"),
	N_("Income"),
	N_("Transfer"),
	NULL
};

/*HbKvData CYA_TXN_STATUS[] = 
{
	{ TXN_STATUS_NONE,			N_("None") },
	{ TXN_STATUS_CLEARED, 		N_("Cleared") },
	{ TXN_STATUS_RECONCILED, 	N_("Reconciled") },
	{ TXN_STATUS_REMIND, 		N_("Remind") },
	{ TXN_STATUS_VOID, 			N_("Void") },
	{ 0, NULL }
};*/

//this is a test
//txn
HbKivData CYA_TXN_STATUSIMG[] = 
{
	{ TXN_STATUS_NONE,			NULL, N_("None") },
	{ TXN_STATUS_CLEARED, 		ICONNAME_HB_ITEM_CLEAR, N_("Cleared") },
	{ TXN_STATUS_RECONCILED, 	ICONNAME_HB_ITEM_RECON, N_("Reconciled") },
	//{ TXN_STATUS_REMIND, 		ICONNAME_HB_ITEM_REMIND, N_("Remind") },
	{ TXN_STATUS_VOID, 			ICONNAME_HB_ITEM_VOID, N_("Void") },
	{ 0, NULL, NULL }
};

//asg
gchar *CYA_ASG_FIELD[] = { 
	N_("Memo"), 
	N_("Payee"), 
	NULL
};


/* = = = = = = = = = = = = = = = = = = = = */


//bal, bud, sta
gchar *CYA_REPORT_MODE[] =
{
	N_("Total"),
	N_("Time"),
	NULL
};


HbKvData CYA_REPORT_SRC[] = {
	{ REPORT_GRPBY_CATEGORY,		N_("Category") },
	//{ REPORT_GRPBY_SUBCATEGORY,	N_("Subcategory") },
	{ REPORT_GRPBY_PAYEE,			N_("Payee") },
	{ REPORT_GRPBY_ACCOUNT,		N_("Account") },
	{ REPORT_GRPBY_ACCGROUP,		N_("Account Group") },
	{ REPORT_GRPBY_TAG,			N_("Tag") },
	{ REPORT_GRPBY_MONTH,			N_("Month") },
	{ REPORT_GRPBY_YEAR,			N_("Year") },
	{ 0, NULL }
};


HbKvData CYA_REPORT_TYPE[] = { 
	{ REPORT_TYPE_EXPENSE,	N_("Expense") },
	{ REPORT_TYPE_INCOME,	N_("Income") },
	{ REPORT_TYPE_TOTAL,	N_("Total")} ,
	{ 0, NULL }
};


HbKvData CYA_REPORT_GRPBY_TREND[] = {
	{ REPORT_GRPBY_ACCOUNT, 	N_("Account") },
	{ REPORT_GRPBY_CATEGORY,	N_("Category") },
	{ REPORT_GRPBY_PAYEE,		N_("Payee") },
	{ REPORT_GRPBY_TAG,		N_("Tag") },
	{ 0, NULL }
};


HbKvData CYA_REPORT_INTVL[] = {
	{ REPORT_INTVL_DAY,		  N_("Day") },
	{ REPORT_INTVL_WEEK,	  N_("Week") },
	{ REPORT_INTVL_FORTNIGHT, N_("Fortnight") },
	{ REPORT_INTVL_MONTH,	  N_("Month") },
	{ REPORT_INTVL_QUARTER,	  N_("Quarter") },
	{ REPORT_INTVL_HALFYEAR,  N_("Half Year") },
	{ REPORT_INTVL_YEAR,	  N_("Year") },
	{ 0, NULL }
};




/* = = = = = = = = = = = = = = = = = = = = */

//flt
gchar *RA_FILTER_MODE[] =
{
	N_("Include"),
	N_("Exclude"),
	NULL
};


HbKvData CYA_FLT_RANGE_DWF[] = {
	{  FLT_RANGE_LAST_DAY       , N_("Yesterday") },
	{  FLT_RANGE_THIS_DAY       , N_("Today") },
	{  FLT_RANGE_NEXT_DAY       , N_("Tomorrow") },

	{  FLT_RANGE_LAST_WEEK      , N_("Last Week") },
	{  FLT_RANGE_THIS_WEEK      , N_("This Week") },
	{  FLT_RANGE_NEXT_WEEK      , N_("Next Week") },

	{  FLT_RANGE_LAST_FORTNIGHT , N_("Last Fortnight") },
	{  FLT_RANGE_THIS_FORTNIGHT , N_("This Fortnight") },
	{  FLT_RANGE_NEXT_FORTNIGHT , N_("Next Fortnight") },

	{ 0, NULL }
};


HbKvData CYA_FLT_RANGE_MQY[] = {
	{  FLT_RANGE_LAST_MONTH     , N_("Last Month") },
	{  FLT_RANGE_THIS_MONTH     , N_("This Month") },
	{  FLT_RANGE_NEXT_MONTH     , N_("Next Month") },

	{  FLT_RANGE_LAST_QUARTER   , N_("Last Quarter") },
	{  FLT_RANGE_THIS_QUARTER   , N_("This Quarter") },
	{  FLT_RANGE_NEXT_QUARTER   , N_("Next Quarter") },

	{  FLT_RANGE_LAST_YEAR      , N_("Last Year") },
	{  FLT_RANGE_THIS_YEAR      , N_("This Year") },
	{  FLT_RANGE_NEXT_YEAR      , N_("Next Year") },

	{  HBTK_IS_SEPARATOR, "" },
	{  HBTK_IS_SEPARATOR, "" },
	{  HBTK_IS_SEPARATOR, "" },

	{ 0, NULL }
};


HbKvData CYA_FLT_RANGE_YTO[] = {
	{  FLT_RANGE_TODATE_YEAR    , N_("Year to date") },
	{  FLT_RANGE_TODATE_MONTH   , N_("Month to date") },
	{  FLT_RANGE_TODATE_ALL     , N_("All to date") },
	{ 0, NULL }
};

HbKvData CYA_FLT_RANGE_LASTXXD[] = {
	{  FLT_RANGE_LAST_90DAYS    , N_("Last 90 Days") },
	{  FLT_RANGE_LAST_60DAYS    , N_("Last 60 Days") },
	{  FLT_RANGE_LAST_30DAYS    , N_("Last 30 Days") },
	{ 0, NULL }
};


HbKvData CYA_FLT_RANGE_COMMON[] = {
	{  FLT_RANGE_LAST_12MONTHS  , N_("Last 12 Months") },
	{  FLT_RANGE_MISC_30DAYS    , N_("30 Days Around") },
	{  FLT_RANGE_MISC_ALLDATE   , N_("All Date") },
	{ 0, NULL }
};


HbKvData CYA_FLT_RANGE_CUSTOM[] = {
	//5.7 added back
	{  FLT_RANGE_MISC_CUSTOM   , N_("Custom") },
	{ 0, NULL }
};


HbKvData CYA_FLT_SCHEDULED[] = {
	{ FLT_SCHEDULED_THISMONTH,		N_("This month") },
	{ FLT_SCHEDULED_NEXTMONTH,		N_("Next month") },
	{ HBTK_IS_SEPARATOR, "" },
	{ FLT_SCHEDULED_NEXT30DAYS,		N_("Next 30 days") },
	{ FLT_SCHEDULED_NEXT60DAYS,		N_("Next 60 days") },
	{ FLT_SCHEDULED_NEXT90DAYS,		N_("Next 90 days") },
	{ HBTK_IS_SEPARATOR, "" },
	{ FLT_SCHEDULED_MAXPOSTDATE,	N_("Maximum Post Date") },	
	{ HBTK_IS_SEPARATOR, "" },
	{ FLT_SCHEDULED_ALLDATE,		N_("All") },
	{ 0, NULL }
};



//repbud
HbKvData CYA_KIND[] = {
	{ REPORT_TYPE_ALL, N_("Exp. & Inc.") },
	{ REPORT_TYPE_EXPENSE, N_("Expense") },
	{ REPORT_TYPE_INCOME, N_("Income") },
	{ 0, NULL }
};


//ledger
HbKvData CYA_FLT_TYPE[] = {
	{ FLT_TYPE_ALL,	N_("Any Type") },
	{ HBTK_IS_SEPARATOR, "" },
	{ FLT_TYPE_EXPENSE,	N_("Expense") },
	{ FLT_TYPE_INCOME,	N_("Income") },
	{ FLT_TYPE_INTXFER,	N_("Transfer") },
	{ 0, NULL }
};


//ledger
HbKvData CYA_FLT_STATUS[] = {
	{ FLT_STATUS_ALL,	N_("Any Status") },
	{ HBTK_IS_SEPARATOR, "" },
	{ FLT_STATUS_CLEARED,	N_("Cleared") },
	{ FLT_STATUS_UNCLEARED,	N_("Uncleared") },
	{ FLT_STATUS_RECONCILED,	N_("Reconciled") },
	{ FLT_STATUS_UNRECONCILED,	N_("Unreconciled") },
	{ HBTK_IS_SEPARATOR, "" },
	{ FLT_STATUS_UNCATEGORIZED,	N_("Uncategorized") },
	//5.9
	{ FLT_STATUS_UNAPPROVED,	N_("Unapproved") },
	{ 0, NULL }
};


/* = = = = = = = = = = = = = = = = = = = = */


HbKvData CYA_TOOLBAR_STYLE[] = {
	{ 0, N_("System defaults") },
	{ 1, N_("Icons only") },
	{ 2, N_("Text only") },
	{ 3, N_("Text under icons") },
	{ 4, N_("Text beside icons") },
	{ 0, NULL }
};


HbKvData CYA_GRID_LINES[] = {
	{ GTK_TREE_VIEW_GRID_LINES_NONE, N_("None") },
	{ GTK_TREE_VIEW_GRID_LINES_HORIZONTAL, N_("Horizontal") },
	{ GTK_TREE_VIEW_GRID_LINES_VERTICAL, N_("Vertical") },
	{ GTK_TREE_VIEW_GRID_LINES_BOTH, N_("Both") },
	{ 0, NULL }
};

HbKvData CYA_IMPORT_DATEORDER[] = {
	{ PRF_DATEFMT_MDY, N_("m-d-y") },
	{ PRF_DATEFMT_DMY, N_("d-m-y") },
	{ PRF_DATEFMT_YMD, N_("y-m-d") },
	{ 0, NULL }
};

HbKvData CYA_IMPORT_OFXNAME[] = {
	{ PRF_OFXNAME_IGNORE, N_("Ignore") },
	{ PRF_OFXNAME_MEMO, N_("Memo") },
	{ PRF_OFXNAME_PAYEE, N_("Payee") },
	{ PRF_OFXNAME_NUMBER, N_("Number") },
	{ 0, NULL }
};

HbKvData CYA_IMPORT_OFXMEMO[] = {
	{ PRF_OFXMEMO_IGNORE, N_("Ignore") },
	{ PRF_OFXMEMO_NUMBER, N_("Append to Number") },
	{ PRF_OFXMEMO_MEMO, N_("Append to Memo") },
	{ PRF_OFXMEMO_PAYEE, N_("Append to Payee") },
	{ 0, NULL }
};

HbKvData CYA_IMPORT_CSVSEPARATOR[] = {
	{ PRF_DTEX_CSVSEP_TAB, N_("Tab") },
	{ PRF_DTEX_CSVSEP_COMMA, N_("Comma") },
	{ PRF_DTEX_CSVSEP_SEMICOLON, N_("Semicolon") },
	{ PRF_DTEX_CSVSEP_SPACE, N_("Space") },
	{ 0, NULL }
};


//pref
HbKvData CYA_MONTHS[] =
{
	{ 1, N_("January") },
	{ 2, N_("February") },
	{ 3, N_("March") },
	{ 4, N_("April") },
	{ 5, N_("May") },
	{ 6, N_("June") },
	{ 7, N_("July") },
	{ 8, N_("August") },
	{ 9, N_("September") },
	{ 10, N_("October") },
	{ 11, N_("November") },
	{ 12, N_("December") },
	{ 0, NULL }
};


//rep, bud, repbud
gchar *CYA_ABMONTHS[] =
{
	NULL,
	N_("Jan"),
	N_("Feb"),
	N_("Mar"),
	N_("Apr"),
	N_("May"),
	N_("Jun"),
	N_("Jul"),
	N_("Aug"),
	N_("Sep"),
	N_("Oct"),
	N_("Nov"),
	N_("Dec"),
	NULL
};


