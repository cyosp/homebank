/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2024 Maxime DOYEN
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


/* = = = = = = = = = = = = = = = = = = = = */

gchar *CHART_CATEGORY = N_("Category");


gchar *CYA_TXN_TYPE[] = { 
	N_("Expense"),
	N_("Income"),
	N_("Transfer"),
	NULL
};


gchar *CYA_CAT_TYPE[] = { 
	N_("Expense"), 
	N_("Income"), 
	NULL
};


gchar *CYA_ASG_FIELD[] = { 
	N_("Memo"), 
	N_("Payee"), 
	NULL
};


gchar *CYA_REPORT_MODE[] =
{
	N_("Total"),
	N_("Time"),
	NULL
};


HbKvData CYA_REPORT_SRC[] = {
	{ REPORT_SRC_CATEGORY,		N_("Category") },
	//{ REPORT_SRC_SUBCATEGORY,	N_("Subcategory") },
	{ REPORT_SRC_PAYEE,			N_("Payee") },
	{ REPORT_SRC_ACCOUNT,		N_("Account") },
	{ REPORT_SRC_ACCGROUP,		N_("Account Group") },
	{ REPORT_SRC_TAG,			N_("Tag") },
	{ REPORT_SRC_MONTH,			N_("Month") },
	{ REPORT_SRC_YEAR,			N_("Year") },
	{ 0, NULL }
};


HbKvData CYA_REPORT_TYPE[] = { 
	{ REPORT_TYPE_EXPENSE,	N_("Expense") },
	{ REPORT_TYPE_INCOME,	N_("Income") },
	{ REPORT_TYPE_TOTAL,	N_("Total")} ,
	{ 0, NULL }
};


HbKvData CYA_REPORT_SRC_TREND[] = {
	{ REPORT_SRC_ACCOUNT, 	N_("Account") },
	{ REPORT_SRC_CATEGORY,	N_("Category") },
	{ REPORT_SRC_PAYEE,		N_("Payee") },
	{ REPORT_SRC_TAG,		N_("Tag") },
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


//soon obsolete for merged trendtime
gchar *RA_REPORT_TIME_MODE[] = { 
	N_("Total"), 
	N_("Trend"), 
	N_("Balance"),
	NULL
};


gchar *RA_FILTER_MODE[] =
{
	N_("Include"),
	N_("Exclude"),
	NULL
};


/* = = = = = = = = = = = = = = = = = = = = */


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


HbKvData CYA_FLT_RANGE_NORMAL[] = {
	{  FLT_RANGE_LAST_30DAYS    , N_("Last 30 Days") },
	{  FLT_RANGE_LAST_60DAYS    , N_("Last 60 Days") },
	{  FLT_RANGE_LAST_90DAYS    , N_("Last 90 Days") },

	{  FLT_RANGE_LAST_12MONTHS  , N_("Last 12 Months") },
	{  FLT_RANGE_MISC_30DAYS    , N_("30 Days Around") },
	{  FLT_RANGE_MISC_ALLDATE   , N_("All Date") },
	{ 0, NULL }
};

HbKvData CYA_FLT_RANGE_BUDGET[] = {
	{  FLT_RANGE_LAST_12MONTHS  , N_("Last 12 Months") },
	{  FLT_RANGE_LAST_6MONTHS   , N_("Last 6 Months") },
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
HbKivData CYA_TXN_STATUSIMG[] = 
{
	{ TXN_STATUS_NONE,			NULL, N_("None") },
	{ TXN_STATUS_CLEARED, 		ICONNAME_HB_OPE_CLEARED, N_("Cleared") },
	{ TXN_STATUS_RECONCILED, 	ICONNAME_HB_OPE_RECONCILED, N_("Reconciled") },
	{ TXN_STATUS_REMIND, 		ICONNAME_HB_OPE_REMIND, N_("Remind") },
	{ TXN_STATUS_VOID, 			ICONNAME_HB_OPE_VOID, N_("Void") },
	{ 0, NULL, NULL }
};


gchar *CYA_ARC_UNIT[] = {
	N_("Day"), 
	N_("Week"), 
	N_("Month"), 
	N_("Year"), 
	NULL
};


gchar *RA_ARC_WEEKEND[] = { 
	N_("Possible"), 
	N_("Before"), 
	N_("After"), 
	//added 5.6
	N_("Skip"), 
	NULL
};


gchar *CYA_KIND[] = {
	N_("Exp. & Inc."),
	N_("Expense"),
	N_("Income"),
	NULL
};


gchar *CYA_FLT_TYPE[] = {
	N_("Any Type"),
	"",
	N_("Expense"),
	N_("Income"),
	N_("Transfer"),
	NULL
};


gchar *CYA_FLT_STATUS[] = {
	N_("Any Status"),
	"",
	N_("Cleared"),
	N_("Uncleared"),
	N_("Reconciled"),
	N_("Unreconciled"),
	"",
	N_("Uncategorized"),
	NULL
};


/*gchar *OLD_CYA_FLT_RANGE[] = {
	N_("This month"),
	N_("Last month"),
	N_("This quarter"),
	N_("Last quarter"),
	N_("This year"),
	N_("Last year"),
	"",
	N_("Last 30 days"),
	N_("Last 60 days"),
	N_("Last 90 days"),
	N_("Last 12 months"),
	"",
	N_("Other..."),
	"",
	N_("All date"),
	NULL
};*/


gchar *CYA_WEEKDAY[] =
{
	"",
	N_("Mon"),
	N_("Tue"),
	N_("Wed"),
	N_("Thu"),
	N_("Fri"),
	N_("Sat"),
	N_("Sun"),
};


/* = = = = = = = = = = = = = = = = = = = = */


//in prefs.c only
// in repbudget
//widget
gchar *CYA_MONTHS[] =
{
	N_("January"),
	N_("February"),
	N_("March"),
	N_("April"),
	N_("May"),
	N_("June"),
	N_("July"),
	N_("August"),
	N_("September"),
	N_("October"),
	N_("November"),
	N_("December"),
	NULL
};


//hb_report.c rep_time.c ui_budget
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


