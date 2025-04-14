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

#include "hb-export.h"
#include "list-operation.h"

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


/* = = = = = = = = = = = = = = = = = = = = */


static void hb_export_qif_elt_txn(GIOChannel *io, Account *acc, gboolean allxfer)
{
GString *elt;
GList *list;
GDate *date;
char amountbuf[G_ASCII_DTOSTR_BUF_SIZE];
gchar *sbuf;
gint count, i;

	elt = g_string_sized_new(255);

	date = g_date_new ();
	
	list = g_queue_peek_head_link(acc->txn_queue);
	while (list != NULL)
	{
	Transaction *txn = list->data;
	Payee *payee;
	Category *cat;

		//#1915609 qif export of multiple account double xfer line
		if( (allxfer == FALSE) && (txn->flags & OF_INTXFER) && txn->amount > 0 )
			goto nexttxn;

		g_date_set_julian (date, txn->date);
		//#1270876
		switch(PREFS->dtex_datefmt)
		{
			case PRF_DATEFMT_MDY: //"m-d-y"  
				g_string_append_printf (elt, "D%02d/%02d/%04d\n", 
					g_date_get_month(date),
					g_date_get_day(date),
					g_date_get_year(date)
					);
				break;
			case PRF_DATEFMT_DMY: //"d-m-y"
				g_string_append_printf (elt, "D%02d/%02d/%04d\n", 
					g_date_get_day(date),
					g_date_get_month(date),
					g_date_get_year(date)
					);
				break;
			case PRF_DATEFMT_YMD: //"y-m-d"
				g_string_append_printf (elt, "D%04d/%02d/%02d\n", 
					g_date_get_year(date),
					g_date_get_month(date),
					g_date_get_day(date)
					);
				break;
		}			

		//g_ascii_dtostr (amountbuf, sizeof (amountbuf), txn->amount);
		g_ascii_formatd (amountbuf, sizeof (amountbuf), "%.2f", txn->amount);
		g_string_append_printf (elt, "T%s\n", amountbuf);

		//#2051307 to prevent ? v to be exported as qif
		sbuf = "";
		if( txn->status == TXN_STATUS_CLEARED || txn->status == TXN_STATUS_RECONCILED)
			transaction_get_status_string(txn);

		g_string_append_printf (elt, "C%s\n", sbuf);

		if( txn->paymode == PAYMODE_CHECK)
			g_string_append_printf (elt, "N%s\n", txn->number);

		//Ppayee
		payee = da_pay_get(txn->kpay);
		if(payee)
			g_string_append_printf (elt, "P%s\n", payee->name);

		// Mmemo
		g_string_append_printf (elt, "M%s\n", txn->memo);

		// LCategory of transaction
		// L[Transfer account name]
		// LCategory of transaction/Class of transaction
		// L[Transfer account]/Class of transaction
		if( (txn->flags & OF_INTXFER) && (txn->kacc == acc->key) )
		{
		//#579260
			Account *dstacc = da_acc_get(txn->kxferacc);
			if(dstacc)
				g_string_append_printf (elt, "L[%s]\n", dstacc->name);
		}
		else
		{
			cat = da_cat_get(txn->kcat);
			if(cat)
			{
				g_string_append_printf (elt, "L%s\n", cat->fullname);
			}
		}

		// splits
		count = da_splits_length(txn->splits);
		for(i=0;i<count;i++)
		{
		Split *s = da_splits_get(txn->splits, i);
				
			cat = da_cat_get(s->kcat);
			if(cat)
			{
				g_string_append_printf (elt, "S%s\n", cat->fullname);
			}	
				
			g_string_append_printf (elt, "E%s\n", s->memo);
			
			g_ascii_formatd (amountbuf, sizeof (amountbuf), "%.2f", s->amount);
			g_string_append_printf (elt, "$%s\n", amountbuf);
		}
		
		g_string_append (elt, "^\n");

nexttxn:
		list = g_list_next(list);
	}

	g_io_channel_write_chars(io, elt->str, -1, NULL, NULL);
	
	g_string_free(elt, TRUE);

	g_date_free(date);
	
}



static void hb_export_qif_elt_acc(GIOChannel *io, Account *acc)
{
GString *elt;
gchar *type;
	
	elt = g_string_sized_new(255);
	
	// account export
	//#987144 fixed account type
	switch(acc->type)
	{
		case ACC_TYPE_BANK : type = "Bank"; break;
		case ACC_TYPE_CASH : type = "Cash"; break;
		case ACC_TYPE_ASSET : type = "Oth A"; break;
		case ACC_TYPE_CREDITCARD : type = "CCard"; break;
		case ACC_TYPE_LIABILITY : type = "Oth L"; break;
		default : type = "Bank"; break;
	}

	g_string_assign(elt, "!Account\n");
	g_string_append_printf (elt, "N%s\n", acc->name);
	g_string_append_printf (elt, "T%s\n", type);
	g_string_append (elt, "^\n");
	g_string_append_printf (elt, "!Type:%s\n", type);

	g_io_channel_write_chars(io, elt->str, -1, NULL, NULL);
	
	g_string_free(elt, TRUE);
}


void hb_export_qif_account_single(gchar *filename, Account *acc)
{
GIOChannel *io;
	
	io = g_io_channel_new_file(filename, "w", NULL);
	if(io == NULL)
	{
		g_message("file error on: %s", filename);
		//retval = XML_IO_ERROR;
	}
	else
	{
		hb_export_qif_elt_acc(io, acc);
		hb_export_qif_elt_txn(io, acc, TRUE);	
		g_io_channel_unref (io);
	}
}


void hb_export_qif_account_all(gchar *filename)
{
GIOChannel *io;
GList *lacc, *list;

	io = g_io_channel_new_file(filename, "w", NULL);
	if(io == NULL)
	{
		g_message("file error on: %s", filename);
		//retval = XML_IO_ERROR;
	}
	else
	{
		//5.5.1 save accounts in order
		//lacc = list = g_hash_table_get_values(GLOBALS->h_acc);
		lacc = list = account_glist_sorted(HB_GLIST_SORT_KEY);
		while (list != NULL)
		{
		Account *item = list->data;

			hb_export_qif_elt_acc(io, item);
			hb_export_qif_elt_txn(io, item, FALSE);	

			list = g_list_next(list);
		}
		g_list_free(lacc);

		g_io_channel_unref (io);
	}

}


/* = = = = = = = = = = = = = = = = = = = = */

#define HELPDRAW 0

#define HEX_R(xcol) (((xcol>>24) & 0xFF)/255)
#define HEX_G(xcol) (((xcol>>16) & 0xFF)/255)
#define HEX_B(xcol) (((xcol>> 8) & 0xFF)/255)


#if HELPDRAW == 1
static void hb_pdf_draw_help_rect(cairo_t *cr, gint32 xcol, double x, double y, double w, double h)
{
	cairo_save (cr);
	
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, HEX_R(xcol), HEX_G(xcol), HEX_B(xcol), 1.0);	//alpha is unused for now
	cairo_rectangle (cr, x, y, w, h);
	cairo_stroke(cr);
	
	cairo_restore(cr);
}
#endif


// references
// https://www.blurb.com/blog/choosing-a-font-for-print-6-things-you-should-know/
// https://plumgroveinc.com/choosing-a-font-for-print-2/

#define		HB_PRINT_FONT_HEAD_POINT 5
#define		HB_PRINT_SPACING 6

typedef struct
{
	gboolean	statement;
	gdouble	font_size;

	gchar	*tabtext;
	gchar	*title;
	gchar	**lines;

	gint	header_height;

	gint	numpagerow;
	gint	numpagecol;
	gint	num_columns;
	
	gint	*col_width;
	gint8	*col_align;		//0 if right, 1 if left
	gint8	*leftcols;		//-1 terminated index of col left aligned

	gint	lines_per_page;
	gint	num_lines;
	gint	num_pages;
} PrintData;


static gint 
hb_print_listview_get_idx_for_pagecol(PrintData *data, gint width, gint pagecol)
{
gint col, availw, numbreak;

	//DB( g_print(" get col for pagerow %d\n", pagecol) );

	availw = width;
	numbreak = 1;
	for(col=0 ; col < data->num_columns ; col++)
	{
		//DB( g_print(" ++ col=%d, curw=%d width=%d, numbrk=%d\n", col, currw, width, numbreak) );
		if( numbreak >= pagecol )
			break;

		availw -= data->col_width[col];
		// new page column ?
		if( availw < data->col_width[col+1] )
		{
			//DB( g_print(" ++ --break--\n") );
			numbreak++;
			availw = width;
		}
	}

	//DB( g_print(" return %d\n", col) );
	return col;
}


static void
hb_print_listview_end_print (GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data)
{
PrintData *data = (PrintData *)user_data;

	g_free(data->col_width);
	g_free(data->col_align);
	g_strfreev (data->lines);
	g_free (data);
}


static void
hb_print_listview_begin_print (GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data)
{
PrintData *data = (PrintData *)user_data;
int i, j, count;
double width, height;
gchar **columns;
PangoLayout *layout;
PangoFontDescription *desc;
gint text_width, text_height, line_height;

	DB( g_print("\n-- begin print --\n") );

	width  = gtk_print_context_get_width (context);
	height = gtk_print_context_get_height (context);

	line_height = data->font_size + 3;

	layout = gtk_print_context_create_pango_layout (context);
	desc = pango_font_description_from_string ("Helvetica");

	//compute header height
	pango_font_description_set_size (desc, (data->font_size + HB_PRINT_FONT_HEAD_POINT) * PANGO_SCALE);
	pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);

	pango_layout_set_text (layout, data->title, -1);
	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	
	//1 line space + column title + spacer
	data->header_height = text_height + (data->font_size * 2);

	height -= data->header_height + (2 * HB_PRINT_SPACING);

	data->lines = g_strsplit (data->tabtext, "\n", 0);

	//todo: test if line > 1

	//get number of column from title
	columns = g_strsplit (data->lines[0], "\t", 0);
	data->num_columns = g_strv_length(columns);
	//debug
	/*for(i=0;i<data->num_columns;i++)
	{
		DB( g_print(" %02d: %s\n", i, columns[i]) );
	}*/	
	
	g_strfreev (columns);

	//alloc memory
	data->col_width = g_malloc0 (sizeof(gint)*(data->num_columns + 1));
	data->col_align = g_malloc0 (sizeof(gint8)*(data->num_columns + 1));


	pango_font_description_set_size (desc, data->font_size * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	DB( g_print(" - compute column width - \n") );
	//TODO: maybe for text column we should remove outliers here

	i = 0;
	count = 0;
	while (data->lines[i] != NULL)
	{
		DB( g_print(" eval line %03d: '%s'\n", i, data->lines[i]) );

		//skip empty lines
		if( strlen(data->lines[i]) > 1 )
		{
			columns = g_strsplit (data->lines[i], "\t", 0);
			j = 0;
			while (columns[j] != NULL)
			{
				pango_layout_set_text (layout, columns[j], -1);
				pango_layout_get_pixel_size (layout, &text_width, &text_height);

				//DB( g_print("  %d : '%s' %d %d\n", j, columns[j], text_width, text_width / PANGO_SCALE ) );

				//add a width
				text_width += HB_PRINT_SPACING;

				data->col_width[j] = MAX(data->col_width[j], text_width);
				j++;
			}
			g_strfreev (columns);
			count++;
		}
		else
		{
			DB( g_print(" skipped\n") );
		}
		i++;
	}
	
	g_object_unref (layout);


	data->num_lines = count;
	data->lines_per_page = floor (height / line_height);
	data->numpagerow = (data->num_lines - 1) / data->lines_per_page + 1;

	DB( g_print(" num_lines: %d\n", data->num_lines) );
	DB( g_print(" lines_per_page: %d\n", data->lines_per_page) );
	DB( g_print(" numpagerow: %d\n", data->numpagerow) );
	DB( g_print(" num_colums: %d\n", data->num_columns) );

	DB( g_print(" width: %f\n", width) );

	//todo: clamp columns for statement 
	// (account) date info payee memo amount clr balance
	//taken from 5.7.2
	if( data->statement == TRUE )
	{	
	gdouble tmp = width - data->col_width[0] - data->col_width[4] - data->col_width[5] - data->col_width[6];

		DB( g_print("\n - statement on - \n") );

		DB( g_print(" substract: %d %d %d %d\n",  data->col_width[0] , data->col_width[4] , data->col_width[5] , data->col_width[6]) );
		DB( g_print(" tmp %f\n", tmp) );
		
		data->col_width[1] = tmp / 4;	//info
		data->col_width[2] = tmp / 4;	//payee
		data->col_width[3] = 2*tmp / 4;	//memo
	}


	DB( g_print("\n - compute numpagecol - \n") );

	data->numpagecol = 1;

	gint availw = width;
	for(i=0;i<data->num_columns;i++)
	{
		availw -= data->col_width[i];
		DB( g_print(" colw[%d]=%d, availw=%d\n", i, data->col_width[i], availw) );

		// new page column ?
		if( availw < data->col_width[i+1] ) 
		{
			DB( g_print(" --break--\n") );
			data->numpagecol++;
			availw = width;
		}
	}


	DB( g_print("\n - assign column alignment - \n") );

	//column 0 is left by default
	data->col_align[0] = 1;
	//affect left align columns
	if( data->leftcols != NULL )
	{
		for(i=0;i<10;i++)
		{
		gint index = data->leftcols[i];
		
			if( index == -1 )
				break;
			data->col_align[index] = 1;
			DB( g_print(" column %d i left align\n", index) );
		}
	}

	DB( g_print(" numpagecol: %d\n", data->numpagecol) );

	data->num_pages = data->numpagerow * data->numpagecol;
	DB( g_print(" num_pages:%d\n", data->num_pages) );



	gtk_print_operation_set_n_pages (operation, data->num_pages);
}


static void hb_print_listview_draw_line(PrintData *data, gchar *line, gint firstcol, gint lastcol, gint y, cairo_t *cr, PangoLayout *layout)
{
gchar **columns;
gint text_width, text_height;
gint nbcol, j, x;

	columns = g_strsplit (line, "\t", 0);

	//#xxxxxxxx restrict to real column
	nbcol = g_strv_length(columns);
	DB( g_print(" draw line col %d to col %d, real is %d\n", firstcol, lastcol, nbcol ));

	lastcol = MIN(lastcol, nbcol);

	x = 0;
	//for(j=0;j<data->num_columns;j++)
	for(j=firstcol ; j<lastcol ; j++)
	{
		//DB( g_print(" +%03d '%s'\n", j, columns[j]) );
		if( columns[j] != NULL )
		{		
			//DB( g_print(" print r%d:c%d '%s'\n", i, j, columns[j]) );
			pango_layout_set_text (layout, columns[j], -1);
			
			//5.7.4 add
			pango_layout_set_width(layout, (data->col_width[j] - HB_PRINT_SPACING) * PANGO_SCALE);
			pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
			
			//do align: 0=right, 1=left
			if( data->col_align[j] == 0 )
			{
				pango_layout_get_pixel_size (layout, &text_width, &text_height);
				cairo_move_to(cr, x + data->col_width[j] - text_width - HB_PRINT_SPACING, y);
			}
			else
				cairo_move_to(cr, x, y);

			pango_cairo_show_layout (cr, layout);
			x += data->col_width[j];
		}
		else
			g_warning(" null print column %d", j);
	}

	g_strfreev (columns);
}



//print is done from left to right
//page 1&2 will be the first column to fit, then page 3&4 the other columns
static void
hb_print_listview_draw_page (GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data)
{
PrintData *data = (PrintData *)user_data;
cairo_t *cr;
PangoLayout *layout;
gint text_width, text_height;
gdouble width, height;
gint line, i, y;
gint pagecol;
PangoFontDescription *desc;
gchar *page_str;
GDate date;
gchar buffer[256];
double tmpval;

	#if MYDEBUG == 1
	gint pagerow = page_nr%data->numpagerow;
	#endif

	tmpval = (double)(page_nr+1)/(double)data->numpagerow;
	pagecol = ceil(tmpval);

	DB( g_print("\n-- draw page %d, pagerow=%d pagecol=%d (tmp=%f)\n", page_nr, pagerow, pagecol, tmpval) );

	cr = gtk_print_context_get_cairo_context (context);
	width  = gtk_print_context_get_width (context);
	height = gtk_print_context_get_height (context);

	//helpdraw
	#if HELPDRAW == 1
	hb_pdf_draw_help_rect(cr, 0x0000FF00, 0, 0, width, 0 + data->header_height);
	hb_pdf_draw_help_rect(cr, 0x00FFFF00, 0, 0 + data->header_height, width, height - (data->header_height + 9 + (2* HB_PRINT_SPACING)));
	hb_pdf_draw_help_rect(cr, 0x00FF0000, 0, height - 9 - HB_PRINT_SPACING, width, 9 + HB_PRINT_SPACING);
	#endif

	/*
	cairo_rectangle (cr, 0, 0, width, data->header_height);

	cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
	cairo_fill_preserve (cr);

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_set_line_width (cr, 1);
	cairo_stroke (cr);
	*/

	//header
	layout = gtk_print_context_create_pango_layout (context);

	desc = pango_font_description_from_string ("Helvetica");
	pango_font_description_set_size (desc, (data->font_size + HB_PRINT_FONT_HEAD_POINT) * PANGO_SCALE);
	pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_text (layout, data->title, -1);
	pango_layout_get_pixel_size (layout, &text_width, &text_height);

	if (text_width > width)
	{
		pango_layout_set_width (layout, width);
		pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_START);
		pango_layout_get_pixel_size (layout, &text_width, &text_height);
	}

	//left
	cairo_move_to (cr, 0, 0);
	//center
	//cairo_move_to (cr, (width - text_width) / 2, 0);
	pango_cairo_show_layout (cr, layout);

	g_object_unref (layout);


	layout = gtk_print_context_create_pango_layout (context);
	desc = pango_font_description_from_string ("Helvetica");
	pango_font_description_set_size (desc, data->font_size * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);

	gint firstcol = hb_print_listview_get_idx_for_pagecol(data, width, pagecol);
	gint lastcol  = hb_print_listview_get_idx_for_pagecol(data, width, pagecol+1);

	//line header
	y = data->header_height - data->font_size;
	pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
	pango_layout_set_font_description (layout, desc);
	hb_print_listview_draw_line(data, data->lines[0], firstcol, lastcol, y, cr, layout);

	//helpdraw
	#if HELPDRAW == 1
	gint x = 0;
	gchar **columns = g_strsplit (data->lines[0], "\t", 0);
	for(gint j=firstcol ; j<lastcol ; j++)
	{
		//DB( g_print(" +%03d '%s'\n", j, columns[j]) );
		if( columns[j] != NULL )
		{		
			hb_pdf_draw_help_rect(cr, 0xFF000000, x, y, data->col_width[j] - HB_PRINT_SPACING, height - (data->header_height + 9 + (2* HB_PRINT_SPACING)) );
			x += data->col_width[j];
		}
	}
	g_strfreev (columns);
	#endif

	y = data->header_height + HB_PRINT_SPACING;
	pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);


	//lines
	line = (page_nr%data->numpagerow) * data->lines_per_page;
	if( line == 0 )	//skip title line
		line++;

	DB( g_print(" print lines from %d to %d (max)\n", line, line+data->lines_per_page) );
	DB( g_print(" print cols from %d\n", firstcol) );

	for (i = 0; i < data->lines_per_page && line < data->num_lines; i++)
	{
		hb_print_listview_draw_line(data, data->lines[line], firstcol, lastcol, y, cr, layout);
		y += data->font_size + 3;
		line++;
	}
	g_object_unref (layout);


	//footer
	layout = gtk_print_context_create_pango_layout (context);
	desc = pango_font_description_from_string ("Helvetica");
	pango_font_description_set_size (desc, 9 * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	y = height - 9;

	//left: date
	g_date_set_julian (&date, GLOBALS->today);
	g_date_strftime (buffer, 256-1, "%a %x", &date);
	pango_layout_set_text (layout, buffer, -1);
	pango_layout_set_width (layout, -1);
	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	cairo_move_to (cr, 0, y);
	pango_cairo_show_layout (cr, layout);

	//right: page
	page_str = g_strdup_printf ("page %d/%d", page_nr + 1, data->num_pages);
	pango_layout_set_text (layout, page_str, -1);
	g_free (page_str);

	pango_layout_set_width (layout, -1);
	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	cairo_move_to (cr, width - text_width - 4, y);
	pango_cairo_show_layout (cr, layout);
	g_object_unref (layout);
  
}


//note: statement is hardcoded for account print only
void
hb_print_listview(GtkWindow *parent, gchar *tabtext, gint8 *leftcols, gchar *title, gchar *filepath, gboolean statement)
{
GtkPrintOperation *operation;
GtkPrintSettings *settings;
PrintData *data;
GError *error = NULL;

	data = g_new0 (PrintData, 1);
	
	data->statement = statement;
	
	data->font_size = 12.0;
	data->tabtext   = tabtext;
	data->title     = title;
	data->leftcols  = leftcols;

	DB( g_print("tabtext debug:\n%s\n", tabtext) );

	operation = gtk_print_operation_new ();

	g_signal_connect (G_OBJECT (operation), "begin-print", G_CALLBACK (hb_print_listview_begin_print), data);
	g_signal_connect (G_OBJECT (operation), "draw-page"  , G_CALLBACK (hb_print_listview_draw_page), data);
	g_signal_connect (G_OBJECT (operation), "end-print"  , G_CALLBACK (hb_print_listview_end_print), data);	

	gtk_print_operation_set_use_full_page (operation, FALSE);
	gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
	gtk_print_operation_set_embed_page_setup (operation, TRUE);

	settings = gtk_print_settings_new ();

	gtk_print_settings_set (settings, GTK_PRINT_SETTINGS_OUTPUT_BASENAME, filepath);
	gtk_print_operation_set_print_settings (operation, settings);

	gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW (parent), &error);

	g_object_unref (operation);
	g_object_unref (settings);

	if (error)
	{
	GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"%s", error->message);
		g_error_free (error);

		g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);

		gtk_widget_show (dialog);
	}

}


