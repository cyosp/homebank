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

#ifndef __HB_ENUMS_H__
#define __HB_ENUMS_H__


/*
** paymode pixbuf
*/
#define OLDPAYMODE_INTXFER 5


enum
{
	PAYMODE_NONE,
	PAYMODE_CCARD,
	PAYMODE_CHECK,
	PAYMODE_CASH,
	PAYMODE_XFER,
	PAYMODE_OBSOLETEINTXFER,
	/* 4.1 new payments here */
	PAYMODE_DCARD = 6,
	PAYMODE_REPEATPMT,
	PAYMODE_EPAYMENT,
	PAYMODE_DEPOSIT,
	PAYMODE_FEE,
	/* 4.6 new paymode */
	PAYMODE_DIRECTDEBIT,
//	PAYMODE_,
	NUM_PAYMODE_MAX
};


/* list display transaction (dsp_account) */
enum
{
	LST_DSPOPE_STATUS = 1,	/*  1 fake column */
	LST_DSPOPE_DATE,	/*  2 fake column */
	LST_DSPOPE_INFO,	/*  3 fake column */
	LST_DSPOPE_PAYEE,	/*  4 fake column */
	LST_DSPOPE_MEMO,	/*  5 fake column */
	LST_DSPOPE_AMOUNT,	/*  6 fake column */
	LST_DSPOPE_EXPENSE,	/*  7 fake column */
	LST_DSPOPE_INCOME,	/*  8 fake column */
	LST_DSPOPE_CATEGORY,/*  9 fake column */
	LST_DSPOPE_TAGS,	/* 10 fake column */
	LST_DSPOPE_BALANCE, /* 11 fake column */
	LST_DSPOPE_CLR,     /* 12 fake column */
	/* here we insert account column, only used for detail */
	LST_DSPOPE_ACCOUNT, /* 13 fake column : not stored */
	LST_DSPOPE_MATCH, /* 14 fake column : not stored */
	NUM_LST_DSPOPE
};


//sort for various glist
enum
{
	HB_GLIST_SORT_KEY,	//0
	HB_GLIST_SORT_NAME,	//1
	HB_GLIST_SORT_POS	//2
};


//
enum
{
	HB_LIST_QUICK_SELECT_UNSET,
	HB_LIST_QUICK_SELECT_ALL,
	HB_LIST_QUICK_SELECT_NONE,
	HB_LIST_QUICK_SELECT_INVERT
};


/*
** list pixbuf (account/transaction)
*//*
enum
{
	LST_PIXBUF_ADD,
	LST_PIXBUF_EDIT,
	LST_PIXBUF_REMIND,
	LST_PIXBUF_VALID,
	LST_PIXBUF_AUTO,
	LST_PIXBUF_WARNING,
	NUM_LST_PIXBUF
};*/


/*
** toolbar item type
*//*
enum
{
	TOOLBAR_SEPARATOR,
	TOOLBAR_BUTTON,
	TOOLBAR_TOGGLE
};*/


#endif


