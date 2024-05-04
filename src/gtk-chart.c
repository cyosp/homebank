/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2023 Maxime DOYEN
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


#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#include "homebank.h"
#include "gtk-chart-colors.h"
#include "gtk-chart.h"

//TODO: move this
#include "list-report.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0


#define DB(x);
//#define DB(x) (x);

//calculation
#define DBC(x);
//#define DBC(x) (x);

//graph
#define DBG(x);
//#define DBG(x) (x);

//DB dynamics
#define DBD(x);
//#define DBD(x) (x);

//tooltip
#define DBT(x);
//#define DBT(x) (x);


//TODO: ugly hack for total line
#define LST_REPDIST_POS_TOTAL G_MAXINT


#define DYNAMICS	TRUE

#define DBGDRAW_RECT	FALSE
#define DBGDRAW_TEXT	FALSE
#define DBGDRAW_ITEM	FALSE


static GtkBoxClass *gtk_chart_parent_class = NULL;

const double dashed3[] = {3.0};


static void chart_set_font_size(GtkChart *chart, PangoLayout *layout, gint font_size);
static gchar *chart_print_int(GtkChart *chart, gint value);
static gchar *chart_print_scalerate(GtkChart *chart, gdouble value);
static gboolean drawarea_full_redraw(GtkWidget *widget, gpointer user_data);
static void chart_clear(GtkChart *chart);


/* = = = = = = = = = = = = = = = = */


static void chart_set_font_size(GtkChart *chart, PangoLayout *layout, gint font_size)
{
PangoAttrList *attrs;
PangoAttribute *attr;
double scale = PANGO_SCALE_MEDIUM;

	//PANGO_SCALE_MEDIUM = normal size

	//DB( g_print("\n[chart] set font size\n") );

	switch(font_size)
	{
		case CHART_FONT_SIZE_TITLE:
			//size = chart->pfd_size + 3;
			scale = PANGO_SCALE_X_LARGE;
			break;
		case CHART_FONT_SIZE_SUBTITLE:
			//size = chart->pfd_size + 1;
			scale = PANGO_SCALE_LARGE;
			break;
		//case CHART_FONT_SIZE_NORMAL:
			//size = chart->pfd_size - 1;
		//	break;
		case CHART_FONT_SIZE_SMALL:
			if(chart->smallfont == TRUE)
				//size = chart->pfd_size - 2;
				scale = PANGO_SCALE_SMALL;
			else
				//size = chart->pfd_size - 1;
				scale = PANGO_SCALE_MEDIUM;
			break;
	}

	//DB( g_print(" size=%d\n", size) );

	attrs = pango_attr_list_new ();
	attr  = pango_attr_scale_new(scale);
	pango_attr_list_insert (attrs, attr);

	pango_layout_set_attributes (layout, attrs);

	pango_layout_set_font_description (layout, chart->pfd);

	//pango_attribute_destroy(attr);
	pango_attr_list_unref (attrs);

}


/* bar section */


//https://stackoverflow.com/questions/326679/choosing-an-attractive-linear-scale-for-a-graphs-y-axis

static float CalculateStepSize(float range, float targetSteps)
{
    // calculate an initial guess at step size
    float tempStep = range/targetSteps;

    // get the magnitude of the step size
    float mag = (float)floor(log10(tempStep));
    float magPow = (float)pow(10, mag);

    // calculate most significant digit of the new step size
    float magMsd = (int)(tempStep/magPow + 0.5);

    // promote the MSD to either 1, 2, or 5
    if (magMsd > 5.0)
        magMsd = 10.0f;
    else if (magMsd > 2.0)
        magMsd = 5.0f;
    else if (magMsd >= 1.0)
        magMsd = 2.0f;

    return magMsd*magPow;
}


static void colchart_compute_range(GtkChart *chart, HbtkDrawContext *drawctx)
{
double lobound, hibound;

	DBC( g_print("\n[column] compute range\n") );

	lobound=chart->rawmin;
	hibound=chart->rawmax;

	/* comptute max ticks */
	drawctx->range = chart->rawmax - chart->rawmin;
	gint maxticks = MIN(10,floor(drawctx->graph.height / (drawctx->font_h * 2)));

	if( chart->type == CHART_TYPE_STACK100 )
	{
		lobound = 0;
		hibound = 100;
		drawctx->range = 100;
	}

	DBC( g_print(" raw :: [%.2f - %.2f] range=%.2f\n", chart->rawmin, chart->rawmax, drawctx->range) );
	DBC( g_print(" raw :: maxticks=%d (%g / (%g*2))\n", maxticks, drawctx->graph.height, drawctx->font_h) );
	DBC( g_print("\n") );

	drawctx->unit  = CalculateStepSize((hibound-lobound), maxticks);
	drawctx->min   = -drawctx->unit * ceil(-lobound/drawctx->unit);
	drawctx->max   = drawctx->unit * ceil(hibound/drawctx->unit);
	drawctx->range = drawctx->max - drawctx->min;
	drawctx->div   = drawctx->range / drawctx->unit;

	DBC( g_print(" end :: interval=%.2f, ticks=%d\n", drawctx->unit, drawctx->div) );
	DBC( g_print(" end :: [%.2f - %.2f], range=%.2f\n", drawctx->min, drawctx->max, drawctx->range) );

	DBC( g_print("[column] end compute range\n\n") );

}


static void colchart_calculation(GtkChart *chart, HbtkDrawContext *drawctx)
{
gint blkw, maxvisi;
gint nb_items;

	DBC( g_print("\n[colchart] calculation\n") );

	nb_items = chart->nb_items;
	if( chart->type == CHART_TYPE_STACK || chart->type == CHART_TYPE_STACK100)
		nb_items = chart->nb_cols;

	/* from fusionchart
	the bar has a default width of 41
	min space is 3 and min barw is 8
	*/

	// new computing
	if( chart->usrbarw > 0.0 )
	{
		blkw = chart->usrbarw;
		drawctx->barw = blkw - 3;
	}
	else
	{
		//minvisi = floor(chart->graph.width / (GTK_CHART_MINBARW+3) );
		maxvisi = floor(drawctx->graph.width / (GTK_CHART_MAXBARW+3) );

		//DBC( g_print(" width=%.2f, nb=%d, minvisi=%d, maxvisi=%d\n", chart->graph.width, nb_items, minvisi, maxvisi) );

		if( nb_items <= maxvisi )
		{
			drawctx->barw = GTK_CHART_MAXBARW;
			blkw = GTK_CHART_MAXBARW + ( drawctx->graph.width - (nb_items*GTK_CHART_MAXBARW) ) / nb_items;
		}
		else
		{
			blkw = MAX(GTK_CHART_MINBARW, floor(drawctx->graph.width / nb_items));
			drawctx->barw = blkw - 3;
		}
	}

	if(chart->dual)
		drawctx->barw = drawctx->barw / 2;

	DBC( g_print(" blkw=%d, barw=%2.f\n", blkw, drawctx->barw) );

	drawctx->blkw = blkw;
	drawctx->visible = drawctx->graph.width / blkw;
	drawctx->visible = MIN(drawctx->visible, nb_items);

	DBC( g_print(" blkw=%.2f, barw=%.2f, visible=%d\n", drawctx->blkw, drawctx->barw, drawctx->visible) );

	drawctx->ox = drawctx->l;
	drawctx->oy = drawctx->t + drawctx->h;
	if(drawctx->range > 0)
		drawctx->oy = floor(drawctx->graph.y + (drawctx->max/drawctx->range) * drawctx->graph.height);

	DBC( g_print(" + ox=%.2f oy=%.2f\n", drawctx->ox, drawctx->oy) );

}


/*
** draw the scale
*/
static void colchart_draw_scale(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{
double x, y, y2;
gdouble curxval;
gint i, first;
PangoLayout *layout;
gchar *valstr;
int tw, th;

	DBG( g_print("\n[column] draw scale\n") );

	first = gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));


	/* clip */
	//cairo_rectangle(cr, CHART_MARGIN, 0, chart->w, chart->h + CHART_MARGIN);
	//cairo_clip(cr);

	layout = pango_cairo_create_layout (cr);
	chart_set_font_size(chart, layout, CHART_FONT_SIZE_SMALL);
	cairo_set_line_width(cr, 1.0);

	// Y-scale element (horiz lines + labels (amounts))
	DBG( g_print("\n -- scale-y: [%d - %d]\n", 0, drawctx->div) );

	curxval = drawctx->max;
	cairo_set_dash(cr, 0, 0, 0);
	for(i=0 ; i <= drawctx->div ; i++)
	{

		y = 0.5 + floor (drawctx->graph.y + ((i * drawctx->unit) / drawctx->range) * drawctx->graph.height);
		//DB( g_print(" + i=%d :: y=%f (%f / %f) * %f\n", i, y, i*chart->unit, chart->range, chart->graph.height) );

		//y-horiz line
		if(!drawctx->isprint)
			cairo_user_set_rgbacol (cr, &global_colors[THTEXT], ( curxval == 0.0 ) ? 0.8 : 0.1);
		else
			cairo_user_set_rgbacol (cr, &global_colors[BLACK], ( curxval == 0.0 ) ? 0.8 : 0.1);
		cairo_move_to (cr, drawctx->graph.x, y);
		cairo_line_to (cr, drawctx->graph.x + drawctx->graph.width, y);
		cairo_stroke (cr);

		//y-axis label
		if( chart->type != CHART_TYPE_STACK100 )
			valstr = chart_print_int(chart, (gint)curxval);
		else
			valstr = chart_print_scalerate(chart, curxval * 100 / drawctx->range );
		
		pango_layout_set_text (layout, valstr, -1);
		pango_layout_get_size (layout, &tw, &th);

		if(!drawctx->isprint)
			cairo_user_set_rgbacol (cr, &global_colors[THTEXT], 0.78);
		else
			cairo_user_set_rgbacol (cr, &global_colors[BLACK], 0.78);

		cairo_move_to (cr, drawctx->graph.x - (tw / PANGO_SCALE) - 2, y - ((th / PANGO_SCALE)*0.8) );
		pango_cairo_show_layout (cr, layout);

		curxval -= drawctx->unit;
	}

	// X-scale element (vert line + label (item name)
	if( chart->dual == TRUE || chart->show_xval == TRUE )
	{
	gint lstr = drawctx->l;
	gint tx;

		x = drawctx->graph.x + (drawctx->blkw/2);
		y = drawctx->t + drawctx->h - drawctx->font_h;

		DBG( g_print("\n -- scale-x: [%d - %d] visi %d\n", first, first+drawctx->visible, drawctx->visible) );

		for(i=first ; i<(first+drawctx->visible) ; i++)
		{

			/*if( chart->dual == TRUE )
			{
				tx = 0.5 + x + (drawctx->blkw/2);
				cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.8);
				cairo_move_to(cr, tx, y0+2);
				cairo_line_to(cr, tx, y0-2);
				cairo_stroke(cr);
			}*/

			if( chart->show_xval == TRUE )
			{
				valstr = "";
				
				//TODO: we miss a vertical line here
				switch(chart->type)
				{
					case CHART_TYPE_COL:
					case CHART_TYPE_LINE:
					case CHART_TYPE_PIE:
						{
							ChartItem *item = &g_array_index(chart->items, ChartItem, i);
							valstr = item->label;
						}
						break;
					case CHART_TYPE_STACK:
					case CHART_TYPE_STACK100:			
						valstr = chart->collabel[i];	
						break;
				}

				DBG( g_print(" x: %2d '%s'", i, valstr) );

				pango_layout_set_text (layout, valstr, -1);
				pango_layout_get_size (layout, &tw, &th);
				tx = x - ((tw / PANGO_SCALE)/2);
				if( tx >= lstr )
				{
					DBG( g_print(" drawed") );

					if(!drawctx->isprint)
						cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.78);
					else
						cairo_user_set_rgbacol(cr, &global_colors[BLACK], 0.78);
	
					cairo_move_to(cr, tx, y);
					pango_cairo_show_layout (cr, layout);
					lstr = x + ((tw / PANGO_SCALE)/2) + CHART_SPACING;

					// draw a marker
					cairo_move_to(cr, x, drawctx->graph.y + drawctx->graph.height+3);
					cairo_line_to(cr, x, drawctx->graph.y + drawctx->graph.height);
					cairo_stroke(cr);

				}

				DBG( g_print("\n") );
			}

			x += drawctx->blkw;
		}
	}


	/* average */

	if( chart->show_average )
	{
		if( chart->average < 0 )
		{
			y  = 0.5 + drawctx->oy + (ABS(chart->average)/drawctx->range) * drawctx->graph.height;
		}
		else
		{
			y  = 0.5 + drawctx->oy - (ABS(chart->average)/drawctx->range) * drawctx->graph.height;
		}

		DBG( g_print(" -- average: x%d, y%f, w%d\n", drawctx->l, y, drawctx->w) );

		if(!drawctx->isprint)
			cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 1.0);
		else
			cairo_user_set_rgbacol(cr, &global_colors[BLACK], 1.0);

		cairo_set_dash (cr, dashed3, 1, 0);
		cairo_move_to(cr, drawctx->graph.x, y);
		cairo_line_to(cr, drawctx->graph.x + drawctx->graph.width, y);
		cairo_stroke(cr);
	}

	/* overdrawn */

	if( chart->show_over )
	{
		//if(chart->minimum != 0 && chart->minimum >= chart->min)
		if(chart->minimum >= drawctx->min)
		{

			DBG( g_print(" -- minimum: min=%.2f range=%.2f\n", drawctx->min, drawctx->range) );


			if( chart->minimum < 0 )
			{
				y  = 0.5 + drawctx->oy + (ABS(chart->minimum)/drawctx->range) * drawctx->graph.height;
			}
			else
			{
				y  = 0.5 + drawctx->oy - (ABS(chart->minimum)/drawctx->range) * drawctx->graph.height;
			}

			y2 = (ABS(drawctx->min)/drawctx->range) * drawctx->graph.height - (y - drawctx->oy) + 1;

			cairo_set_source_rgba(cr, COLTOCAIRO(255), COLTOCAIRO(0), COLTOCAIRO(0), AREA_ALPHA / 2);

			DBG( g_print(" draw over: x%d, y%f, w%d, h%f\n", drawctx->l, y, drawctx->w, y2) );

			cairo_rectangle(cr, drawctx->graph.x, y, drawctx->graph.width, y2 );
			cairo_fill(cr);

			cairo_set_source_rgb(cr, COLTOCAIRO(255), COLTOCAIRO(0), COLTOCAIRO(0));
			cairo_set_dash (cr, dashed3, 1, 0);
			cairo_move_to(cr, drawctx->graph.x, y);
			cairo_line_to(cr, drawctx->graph.x + drawctx->graph.width, y);
			cairo_stroke(cr);
		}
	}

	g_object_unref (layout);
}


/* = = = = = = = = = = = = = = = = */
/* line section */

/*
** draw all visible lines
*/
static void linechart_draw_plot(cairo_t *cr, double x, double y, double r, GtkChart *chart, gboolean isprint)
{
	cairo_set_line_width(cr, r / 2);

	if(!isprint)
		cairo_user_set_rgbcol(cr, &global_colors[THBASE]);
	else
		cairo_user_set_rgbcol(cr, &global_colors[WHITE]);

	cairo_arc(cr, x, y, r, 0, 2*M_PI);
	cairo_stroke_preserve(cr);

	//cairo_set_source_rgb(cr, COLTOCAIRO(0), COLTOCAIRO(119), COLTOCAIRO(204));
	cairo_user_set_rgbcol(cr, &chart->color_scheme.colors[chart->color_scheme.cs_blue]);
	cairo_fill(cr);
}


static void linechart_draw_lines(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{
double x, y, x2, y2, firstx, lastx, linew;
gint first, i;

	DBG( g_print("\n[line] draw lines\n") );

	x = drawctx->graph.x;
	y = drawctx->oy;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	/* clip */
	//cairo_rectangle(cr, CHART_MARGIN, 0, chart->w, chart->h + CHART_MARGIN);
	//cairo_clip(cr);


	#if DBGDRAW_ITEM == 1
	x2 = x + 0.5;
	cairo_set_line_width(cr, 1.0);
	double dashlength = 4;
	cairo_set_dash (cr, &dashlength, 1, 0);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0); // violet
	for(i=first; i<=(first+drawctx->visible) ;i++)
	{
		cairo_move_to(cr, x2, drawctx->graph.y);
		cairo_line_to(cr, x2, drawctx->graph.x + drawctx->graph.height);
		x2 += drawctx->blkw;
	}
	cairo_stroke(cr);

	cairo_set_dash (cr, &dashlength, 0, 0);
	#endif

	//todo: it should be possible to draw line & plot together using surface and composite fill, or sub path ??
	lastx = x;
	firstx = x;
	linew = 4.0;
	if(drawctx->barw < 24)
	{
		linew = 1 + (drawctx->barw / 8.0);
	}

	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_set_line_width(cr, linew);
	cairo_set_dash(cr, 0, 0, 0);

	for(i=first; i<(first+drawctx->visible) ;i++)
	{
	ChartItem *item = &g_array_index(chart->items, ChartItem, i);

		x2 = x + (drawctx->blkw)/2;
		y2 = drawctx->oy - (item->serie1 / drawctx->range) * drawctx->graph.height;
		if( i == first)
		{
			firstx = x2;
			cairo_move_to(cr, x2, y2);
		}
		else
		{
			if( i < (chart->nb_items) )
			{
				cairo_line_to(cr, x2, y2);
				lastx = x2;
			}
			else
				lastx = x2 - drawctx->barw;
		}

		x += drawctx->blkw;
	}

	cairo_user_set_rgbcol(cr, &chart->color_scheme.colors[chart->color_scheme.cs_blue]);
	cairo_stroke_preserve(cr);

	cairo_line_to(cr, lastx, y);
	cairo_line_to(cr, firstx, y);
	cairo_close_path(cr);

	cairo_user_set_rgbacol(cr, &chart->color_scheme.colors[chart->color_scheme.cs_blue], AREA_ALPHA);
	cairo_fill(cr);

	x = drawctx->graph.x;
	y = drawctx->oy;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	// draw plots
	for(i=first; i<(first+drawctx->visible) ;i++)
	{
	ChartItem *item = &g_array_index(chart->items, ChartItem, i);

		x2 = x + (drawctx->blkw)/2;
		y2 = drawctx->oy - (item->serie1 / drawctx->range) * drawctx->graph.height;

		//test draw vertical selection line
		if( i == chart->hover )
		{

			if(!drawctx->isprint)
				cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.1);
			else
				cairo_user_set_rgbacol(cr, &global_colors[BLACK], 0.1);

			//cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); //blue
			cairo_set_line_width(cr, 1.0);
			cairo_move_to(cr, x2, drawctx->graph.y);
			cairo_line_to(cr, x2, drawctx->t + drawctx->h - drawctx->font_h);
			cairo_stroke(cr);
		}


		linechart_draw_plot(cr,  x2, y2, i == chart->hover ? linew+1 : linew, chart, drawctx->isprint);


		x += drawctx->blkw;
	}
}


/*
** draw all visible bars
*/
static void colchart_draw_bars(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{
double x, x2, y2, h;
gint i, first;

	DBG( g_print("\n[column] draw bars\n") );

	x = drawctx->graph.x;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	DBG( g_print(" x=%.2f first=%d, blkw=%.2f, barw=%.2f\n", x, first, drawctx->blkw, drawctx->barw ) );

	#if DBGDRAW_ITEM == 1
	x2 = x + 0.5;
	cairo_set_line_width(cr, 1.0);
	double dashlength;
	dashlength = 4;
	cairo_set_dash (cr, &dashlength, 1, 0);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0); // violet
	for(i=first; i<=(first+drawctx->visible) ;i++)
	{
		cairo_move_to(cr, x2, drawctx->graph.y);
		cairo_line_to(cr, x2, drawctx->graph.x + drawctx->graph.height);
		x2 += drawctx->blkw;
	}
	cairo_stroke(cr);
	cairo_set_dash (cr, &dashlength, 0, 0);
	#endif

	for(i=first; i<(first+drawctx->visible) ;i++)
	{
	ChartItem *item = &g_array_index(chart->items, ChartItem, i);
	gint color;
	gint barw = drawctx->barw;

		//if(!chart->datas1[i]) goto nextbar;

		if(!chart->show_mono)
			color = i % chart->color_scheme.nb_cols;
		else
			color = chart->color_scheme.cs_green;

		cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[color], i == chart->hover);

		x2 = x + (drawctx->blkw/2) - 1;
		x2 = !chart->dual ? x2 - (barw/2) : x2 - barw - 1;

		//exp/inc/bal
		if(item->serie1)
		{
			h = floor((item->serie1 / drawctx->range) * drawctx->graph.height);
			y2 = drawctx->oy - h;
			if(item->serie1 < 0.0)
			{
				y2 += 1;
				if(chart->show_mono)
				{
					color = chart->color_scheme.cs_red;
					cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[color], i == chart->hover);
				}
			}
			//DBG( g_print(" + i=%d :: y2=%f h=%f (%f / %f) * %f\n", i, y2, h, chart->datas1[i], chart->range, chart->graph.height ) );

			cairo_rectangle(cr, x2+2, y2, barw, h);
			cairo_fill(cr);

		}

		if( chart->dual && item->serie2)
		{
			x2 = x2 + barw + 1;
			h = floor((item->serie2 / drawctx->range) * drawctx->graph.height);
			y2 = drawctx->oy - h;

			cairo_rectangle(cr, x2+2, y2, barw, h);
			cairo_fill(cr);

		}

		x += drawctx->blkw;
	}
}


/*
** get the bar under the mouse pointer
*/
static gint colchart_get_hover(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
HbtkDrawContext *drawctx = &chart->context;
gint retval;
gint index, first, px;

	retval = -1;

	if( x <= (drawctx->l+drawctx->w) && x >= drawctx->graph.x && y >= drawctx->graph.y && y <= (drawctx->t+drawctx->h) )
	{
		px = (x - drawctx->graph.x);
		//py = (y - chart->oy);
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
		index = first + (px / drawctx->blkw);

		if(index < chart->nb_items)
			retval = index;
	}

	return(retval);
}


static void colchart_first_changed( GtkAdjustment *adj, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
//gint first;

	DB( g_print("\n[column] first changed\n") );

	//first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

	//DB( g_print(" first=%d\n", first) );

/*
	DB( g_print(" scrollbar\n adj=%8x, low=%.2f upp=%.2f val=%.2f step=%.2f page=%.2f size=%.2f\n", adj,
		adj->lower, adj->upper, adj->value, adj->step_increment, adj->page_increment, adj->page_size) );
 */
    /* Set the number of decimal places to which adj->value is rounded */
    //gtk_scale_set_digits (GTK_SCALE (hscale), (gint) adj->value);
    //gtk_scale_set_digits (GTK_SCALE (vscale), (gint) adj->value);

	drawarea_full_redraw (chart->drawarea, chart);

	DB( g_print(" gtk_widget_queue_draw\n") );
	gtk_widget_queue_draw(chart->drawarea);

}

/*
** scrollbar set values for upper, page size, and also show/hide
*/
static void colchart_scrollbar_setvalues(GtkChart *chart)
{
HbtkDrawContext *drawctx = &chart->context;
GtkAdjustment *adj = chart->adjustment;
gint first, nb_items;

	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

	DB( g_print("\n[column] sb_set_values\n") );

	first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

	nb_items = chart->nb_items;
	if( chart->type == CHART_TYPE_STACK || chart->type == CHART_TYPE_STACK100)
		nb_items = chart->nb_cols - 1;

	DB( g_print(" entries=%d, visible=%d\n", nb_items, drawctx->visible) );
	DB( g_print(" first=%d, upper=%d, pagesize=%d\n", first, nb_items, drawctx->visible) );

	g_object_freeze_notify(G_OBJECT(adj));

	gtk_adjustment_set_upper(adj, (gdouble)nb_items);
	gtk_adjustment_set_page_size(adj, (gdouble)drawctx->visible);
	gtk_adjustment_set_page_increment(adj, (gdouble)drawctx->visible);

	if(first+drawctx->visible > nb_items)
	{
		DB( g_print(" set value to %d\n", nb_items - drawctx->visible) );
		gtk_adjustment_set_value(adj, (gdouble)nb_items - drawctx->visible);
	}

	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION < 18) )
	gtk_adjustment_changed (adj);
	#endif

	g_object_thaw_notify(G_OBJECT(adj));

	if( drawctx->visible < nb_items )
		gtk_widget_hide(GTK_WIDGET(chart->scrollbar));
	else
		gtk_widget_show(GTK_WIDGET(chart->scrollbar));

}


/* = = = = = = = = = = = = = = = = */
/* stack section */


// used for stack chart
void chart_data_series(GtkChart *chart, GtkTreeView *treeview, gint nbrows, gint nbcols)
{
GtkTreeModel *model;
GtkTreeIter iter;
gboolean valid;
gdouble tmpmin, tmpmax;
gint rowid, colid;

	DB( g_print("\n[chart] data series (col graph)\n") );

	g_return_if_fail(GTK_IS_CHART(chart));
	g_return_if_fail(GTK_IS_TREE_VIEW(treeview));

	chart_clear(chart);

	model  = gtk_tree_view_get_model (treeview);

	DB( g_print(" nbrows=%d nbcols=%d struct=%ld\n", nbrows, nbcols, sizeof(ChartItem)) );

	if( nbrows <=0 || nbcols <= 0 )
		goto end;

	DB( g_print(" nbrows=%d nbcols=%d\n", nbrows, nbcols) );

	chart->model = model;
	chart->nb_items = nbrows;

	//TODO temp test - allocate data for sum
	chart->colsum   = g_malloc0(sizeof(gdouble)*nbcols);
	chart->collabel = g_malloc0(sizeof(gpointer)*nbcols);
	chart->nb_cols = nbcols;

	chart->items = g_array_sized_new(FALSE, FALSE, sizeof(ChartItem), chart->nb_items);

	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter);
	rowid = 0;
	while (valid && rowid < nbrows)
	{
	ChartItem item;
	gchar *label;

		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
			LST_REPDIST2_LABEL, &label,
			-1);

		//column total
		//TODO: this is useless, as last row already has the column sum !
		for(colid=0;colid<nbcols;colid++)
		{
		DataRow *dr;
		gdouble value;
		
			gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
				//LST_STORE_TRENDROW=6
				LST_REPDIST2_ROW, &dr,
				-1);

			value = da_datarow_get_cell_sum (dr, colid);
			chart->colsum[colid] += ABS(value);

			DB( g_print(" col %3d row %3d val=%.2f\n", colid, rowid, value) );
		}

		memset(&item, 0, sizeof(item));
		item.label = label;
		g_array_append_vals(chart->items, &item, 1);

		valid = gtk_tree_model_iter_next (model, &iter);
		rowid++;
	}

	//debug colsum
	//get minmax
	tmpmin = 0.0;
	tmpmax = 0.0;

	DB( g_print(" colsum: ") );
	for(colid=0;colid<nbcols;colid++)
	{
		tmpmax = MAX(tmpmax, ABS(chart->colsum[colid]));
		DB( g_print("%.2f|", chart->colsum[colid]) );
	}
	DB( g_print("\n") );



	DB( g_print(" collabel: ") );
	for(colid=0;colid<nbcols;colid++)
	{
	GtkTreeViewColumn *column;

		if( (column = gtk_tree_view_get_column (treeview, colid+1) ))
		{
			chart->collabel[colid] = (gchar *)gtk_tree_view_column_get_title(column);
		}

		DB( g_print("%s|", chart->collabel[colid]) );
	}
	DB( g_print("\n") );


	// ensure rawmin rawmax not equal
	if(tmpmin == tmpmax)
	{
		tmpmin = 0;
		tmpmax = 100;
	}

	DB( g_print(" rawmin=%.2f rawmax=%.2f\n", tmpmin, tmpmax) );

	chart->rawmin = tmpmin;
	chart->rawmax = tmpmax;

	DB( g_print("[chart] end data series (col graph)\n") );
end:
	gtk_chart_queue_redraw(chart);
}



static void colchart_draw_stacks(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{
double x, x2, y2, h, rate, srate;
gint i, r, first;
GtkTreeModel *model = chart->model;
GtkTreeIter iter;
gboolean valid;

	DBG( g_print("\n[column] draw stacks\n") );

	x = drawctx->graph.x;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
	rate = 0;

	cairo_set_line_width(cr, 1.0);

	DBG( g_print(" x=%.2f first=%d, blkw=%.2f, barw=%.2f\n", x, first, drawctx->blkw, drawctx->barw ) );
	DBG( g_print(" mono=%d\n", chart->show_mono ) );

	x2 = 0.5 + floor(x + (drawctx->blkw - drawctx->barw)/2);

	DBG( g_print(" x2=%.0f, height=%.0f\n", x2, drawctx->graph.height) );

	//foreach cols
	for(i=first; i<(first+drawctx->visible) ;i++)
	//for(i=0; i<chart->nb_items ;i++)
	{
//		ChartItem *item = &g_array_index(chart->items, ChartItem, i);


		y2 = 0.5 + floor(drawctx->oy);

		DBG( g_print(" draw blk %d\n", i) );

		//Each rows
		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter);
		r = 0;
		srate = 0;
		while (valid)
		{
		DataRow *dr;
		gdouble value;
		gint id, color;
		gboolean hover;

			gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
				LST_REPDIST2_POS, &id,
				LST_REPDIST2_ROW, &dr,
				-1);

			if( id != LST_REPDIST_POS_TOTAL )
			{
				value = da_datarow_get_cell_sum (dr, i);

				hover = (i == chart->colhover) && (r == chart->hover) ? TRUE : FALSE;

				color = r % chart->color_scheme.nb_cols;
				cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[color], hover);

				//for stacked

				if(chart->type == CHART_TYPE_STACK)
					rate = ABS(value/drawctx->range);
				else if(chart->type == CHART_TYPE_STACK100)
					//for stacked100
					rate = ABS(value/chart->colsum[i]);

				//h = 0.5 + floor(rate * drawctx->graph.height);
				h = rate * drawctx->graph.height;

				//debug sum of rate 
				srate += h;

				DBG( g_print("  row %2d value=%7.2f x2=%7.2f, y2=%7.2f rate=%f height=%7.2f sheight=%f\n", r, value, x2+2, y2, 100*rate, h, srate) );

				cairo_rectangle(cr, x2+2, y2-h, drawctx->barw, h);
				cairo_fill(cr);

				y2 = y2 - h;
			}

			valid = gtk_tree_model_iter_next (model, &iter);
			r++;
		}

		x2 += drawctx->blkw;

	}

}




/*
** get the bar under the mouse pointer
*/
static gint colchart_stack_get_hover(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
HbtkDrawContext *drawctx = &chart->context;
gint retval;
gint index, first, px, py, r;
double y2, h, rate;
GtkTreeModel *model = chart->model;
GtkTreeIter iter;
gboolean valid;

	retval = -1;

	if( x <= (drawctx->l+drawctx->w) && x >= drawctx->graph.x && y >= drawctx->graph.y && y <= (drawctx->t+drawctx->h) )
	{
		px = (x - drawctx->graph.x);
		py = (drawctx->oy - y);
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
		index = first + (px / drawctx->blkw);
		rate = 0;

		DBD( g_print(" x=%d y=%d px=%d py=%d oy=%f\n", x, y, px, py, drawctx->oy) );

		//we are hover column 'index'
		if(index < chart->nb_cols)
		{
			y2 = 0;

			chart->colhover = index;

			//Each rows
			valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter);
			r = 0;
			while (valid)
			{
			DataRow *dr;
			gdouble value;
			gint id;

				gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
					LST_REPDIST2_POS, &id,
					LST_REPDIST2_ROW, &dr,
					-1);

				if( id != LST_REPDIST_POS_TOTAL )
				{
					value = da_datarow_get_cell_sum (dr, index);

					if(chart->type == CHART_TYPE_STACK)
						rate = ABS(value/drawctx->range);
					else if(chart->type == CHART_TYPE_STACK100)
					//for stacked100
						rate = ABS(value/chart->colsum[index]);


					//h = 0.5 + floor(rate * drawctx->graph.height);
					h = rate * drawctx->graph.height;

					DBD( g_print("  col=%02d row=%2d value=%.0f y2=%.0f rate=%f height=%.0f\n", index, r, value, y2, rate, h) );

					if( py < y2+h )
					{
						retval = r;
						break;
					}

					y2 = y2 + h;
				}

				valid = gtk_tree_model_iter_next (model, &iter);
				r++;
			}

		}

	}

	DBD( g_print("  stack active=%d\n", retval) );

	return(retval);
}




/* = = = = = = = = = = = = = = = = */
/* pie section */

static void piechart_draw_slices(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{

	if(chart->nb_items <= 0 || chart->total == 0.0)
		return;

	DBG( g_print("\n[pie] draw slices\n") );


		//cairo drawing

		double a1 = 0 * (M_PI / 180);
		double a2 = 360 * (M_PI / 180);

		//g_print("angle1=%.2f angle2=%.2f\n", a1, a2);

		double cx = drawctx->ox;
		double cy = drawctx->oy;
		double radius = drawctx->rayon/2;
		gint i;
		double dx, dy;
		double sum = 0.0;
		gint color;

		DBG( g_print("rayon=%d\n", drawctx->rayon) );
		DBG( g_print("total=%.f\n", chart->total) );

		for(i=0; i< chart->nb_items ;i++)
		{
		ChartItem *item = &g_array_index(chart->items, ChartItem, i);

			a1 = ((360 * (sum / chart->total)) - 90) * (M_PI / 180);
			sum += ABS(item->serie1);
			a2 = ((360 * (sum / chart->total)) - 90) * (M_PI / 180);
			//if(i < chart->nb_items-1) a2 += 0.0175;

			dx = cx;
			dy = cy;

			DBG( g_print("- s%2d: %.2f%% a1=%.2f a2=%.2f | %s %.f\n", i, sum / chart->total, a1, a2, item->label, item->serie1) );


			cairo_move_to(cr, dx, dy);
			cairo_arc(cr, dx, dy, radius, a1, a2);

			#if CHART_PARAM_PIE_LINE == TRUE
				cairo_set_line_width(cr, 2.0);
				cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
				cairo_line_to(cr, cx, cy);
				cairo_stroke_preserve(cr);
			#endif

			//g_print("color : %f %f %f\n", COLTOCAIRO(colors[i].r), COLTOCAIRO(colors[i].g), COLTOCAIRO(colors[i].b));

			color = i % chart->color_scheme.nb_cols;
			cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[color], i == chart->hover);
			cairo_fill(cr);
		}



#if CHART_PARAM_PIE_DONUT == TRUE
	a1 = 0;
	a2 = 2 * M_PI;

	//original
	//radius = (gint)((chart->rayon/3) * (1 / PHI));
	//5.1
	//radius = (gint)((chart->rayon/2) * 2 / 3);
	//ynab
	//piehole value from 0.4 to 0.6 will look best on most charts
	radius = (gint)(drawctx->rayon/2) * CHART_PARAM_PIE_HOLEVALUE;

	if(!drawctx->isprint)
		cairo_user_set_rgbcol(cr, &global_colors[THBASE]);
	else
		cairo_user_set_rgbcol(cr, &global_colors[WHITE]);

	cairo_arc(cr, cx, cy, radius, a1, a2);
	cairo_fill(cr);
#endif

}


static gint piechart_get_hover(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
HbtkDrawContext *drawctx = &chart->context;
gint retval, px, py;
gint index;
double radius, h;

	DBD( g_print("\n[pie] get hover\n") );

	retval = -1;

	px = x - drawctx->ox;
	py = y - drawctx->oy;
	h  = sqrt( pow(px,2) + pow(py,2) );
	radius = drawctx->rayon / 2;

	if(h <= radius && h >= (radius * CHART_PARAM_PIE_HOLEVALUE) )
	{
	double angle, b;

		b     = (acos(px / h) * 180) / M_PI;
		angle = py > 0 ? b : 360 - b;
		angle += 90;
		if(angle > 360) angle -= 360;
		//angle = 360 - angle;

		//todo optimize
		gdouble cumul = 0;
		for(index=0; index< chart->nb_items ;index++)
		{
		ChartItem *item = &g_array_index(chart->items, ChartItem, index);

			cumul += ABS(item->serie1/chart->total)*360;
			if( cumul > angle )
			{
				retval = index;
				break;
			}
		}

		//DBD( g_print(" inside: x=%d, y=%d\n", x, y) );
		//DBD( g_print(" inside: b=%f angle=%f, slice is %d\n", b, angle, index) );
	}
	return(retval);
}


static void piechart_calculation(GtkChart *chart, HbtkDrawContext *drawctx)
{
//GtkWidget *drawarea = chart->drawarea;
gint w, h;

	DBC( g_print("\n[pie] calculation\n") );

	w = drawctx->graph.width;
	h = drawctx->graph.height;

	drawctx->rayon = MIN(w, h);
	drawctx->mark = 0;

	#if CHART_PARAM_PIE_MARK == TRUE
	gint m = floor(drawctx->rayon / 100);
	m = MAX(2, m);
	drawctx->rayon -= (m * 2);
	drawctx->mark = m;
	#endif

	drawctx->ox = drawctx->graph.x + (w / 2);
	drawctx->oy = drawctx->graph.y + (drawctx->rayon / 2);

	DBC( g_print(" center: %g, %g - R=%d, mark=%d\n", drawctx->ox, drawctx->oy, drawctx->rayon, drawctx->mark) );

}


/* = = = = = = = = = = = = = = = = */


/*
** print a integer number
*/
static gchar *chart_print_int(GtkChart *chart, gint value)
{
	hb_strfmon_int(chart->buffer1, CHART_BUFFER_LENGTH-1, (gdouble)value, chart->kcur, chart->minor);
	return chart->buffer1;
}


/*
** print a scale rate number
*/
static gchar *chart_print_scalerate(GtkChart *chart, gdouble value)
{
	g_snprintf (chart->buffer1, CHART_BUFFER_LENGTH-1, "%.0f%%", value);
	return chart->buffer1;
}


/*
** print a rate number
*/
static gchar *chart_print_rate(GtkChart *chart, gdouble value)
{
	g_snprintf (chart->buffer1, CHART_BUFFER_LENGTH-1, "%.2f%%", value);
	return chart->buffer1;
}


/*
** print a double number
*/
static gchar *chart_print_double(GtkChart *chart, gchar *buffer, gdouble value)
{
	hb_strfmon(buffer, CHART_BUFFER_LENGTH-1, value, chart->kcur, chart->minor);
	return buffer;
}


/*
** clear any allocated memory
*/
static void chart_clear(GtkChart *chart)
{
HbtkDrawContext *drawctx = &chart->context;
gint i;

	DB( g_print("\n[gtkchart] clear\n") );

	//free & clear any previous allocated datas
	if(chart->title != NULL)
	{
		g_free(chart->title);
		chart->title = NULL;
	}

	if(chart->subtitle != NULL)
	{
		g_free(chart->subtitle);
		chart->subtitle = NULL;
	}

	if(chart->items != NULL)
	{
		for(i=0;i<chart->nb_items;i++)
		{
		ChartItem *item = &g_array_index(chart->items, ChartItem, i);

			g_free(item->label);	//we free label as it comes from a model_get into setup_with_model
			g_free(item->legend);
		}
		g_array_free(chart->items, TRUE);
		chart->items =  NULL;
	}

	chart->nb_items = 0;

	chart->total = 0;

	drawctx->range = 0;
	chart->rawmin = 0;
	chart->rawmax = 0;


	//TODO: temp test
	g_free(chart->colsum);
	chart->colsum = NULL;
	g_free(chart->collabel);
	chart->collabel = NULL;

	chart->hover = -1;
	chart->lasthover = -1;

	chart->colhover = -1;
	chart->lastcolhover = -1;

}


/*
** setup our chart with a model and column
*/
static void chart_setup_with_model(GtkChart *chart, GtkTreeModel *list_store, guint column1, guint column2)
{
gint rowid, i;
gboolean valid;
GtkTreeIter iter;

	DB( g_print("\n[chart] setup with model\n") );

	chart_clear(chart);

	//#1870390 add total into listview & exclude from charts
	//chart->nb_items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL);
	chart->nb_items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL);

	chart->items = g_array_sized_new(FALSE, FALSE, sizeof(ChartItem), chart->nb_items);

	DB( g_print(" nb=%d, struct=%d\n", chart->nb_items, (gint)sizeof(ChartItem)) );

	chart->dual = (column1 == column2) ? FALSE : TRUE;

	/* Get the first iter in the list */
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list_store), &iter);
	rowid = 0;
	while (valid)
    {
	gint id;
	gchar *label;
	gdouble value1, value2;
	ChartItem item;

		/* column 0: pos (gint) */
		/* column 1: key (gint) */
		/* column 2: label (gchar) */
		/* column x: values (double) */

		gtk_tree_model_get (GTK_TREE_MODEL(list_store), &iter,
			0, &id,	//useless now
			2, &label,
			column1, &value1,
			column2, &value2,
			-1);

		//#1870390 add total into listview & exclude from charts
		if( id == LST_REPDIST_POS_TOTAL )
		{
			//subtract the LST_REPDIST_POS_TOTAL line not to be drawed
			chart->nb_items--;
			goto next;
		}

		if(chart->dual || chart->abs)
		{
			value1 = ABS(value1);
			value2 = ABS(value2);
		}

		DB( g_print("%d: '%s' %.2f %2f\n", rowid, label, value1, value2) );

		/* data1 value storage & min, max compute */
		chart->rawmin = MIN(chart->rawmin, value1);
		chart->rawmax = MAX(chart->rawmax, value1);

		if( chart->dual )
		{
			/* data2 value storage & min, max compute */
			chart->rawmin = MIN(chart->rawmin, value2);
			chart->rawmax = MAX(chart->rawmax, value2);
		}

		item.label = label;
		item.serie1 = value1;
		item.serie2 = value2;
		g_array_append_vals(chart->items, &item, 1);

		/* ensure rawmin rawmax not equal */
		if(chart->rawmin == chart->rawmax)
		{
			chart->rawmin = 0;
			chart->rawmax = 100;
		}

		/* pie chart total sum */
		chart->total += ABS(value1);

		//leak
		//don't g_free(label); here done into chart_clear
next:
		valid = gtk_tree_model_iter_next (list_store, &iter);
		rowid++;
	}

	// compute rate for legend for bar/pie
	for(i=0;i<chart->nb_items;i++)
	{
	ChartItem *item = &g_array_index(chart->items, ChartItem, i);
	gchar *strval;

		strval = chart_print_double(chart, chart->buffer1, item->serie1);
		item->rate = ABS(item->serie1*100/chart->total);
		item->legend = g_markup_printf_escaped("%s\n%s (%.f%%)", item->label, strval, item->rate);
	}

	//g_print("total is %.2f\n", total);
	//ensure the widget is mapped
	//gtk_widget_map(chart);

}


static void chart_layout_area(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{
PangoLayout *layout;
gchar *valstr;
int tw, th;
gint i;

	DBC( g_print("\n[gtkchart] layout area\n") );

	DBC( g_print(" drawctx: %p\n", drawctx) );

	DBC( g_print(" area    : %d %d %d %d\n", drawctx->l, drawctx->t, drawctx->w, drawctx->h ) );

	/* Create a PangoLayout, set the font and text */
	layout = pango_cairo_create_layout (cr);

	DBC( g_print(" is print: %d\n", drawctx->isprint) );

	// compute title
	drawctx->title_zh = 0;
	if(chart->title != NULL)
	{
		chart_set_font_size(chart, layout, CHART_FONT_SIZE_TITLE);
		pango_layout_set_text (layout, chart->title, -1);
		pango_layout_get_size (layout, &tw, &th);
		drawctx->title_zh = (th / PANGO_SCALE);
		DBC( g_print(" - title: %s w=%d h=%d\n", chart->title, tw, th) );
	}

	// compute subtitle
	drawctx->subtitle_zh = 0;
	if(chart->subtitle != NULL)
	{
		chart_set_font_size(chart, layout, CHART_FONT_SIZE_SUBTITLE);
		pango_layout_set_text (layout, chart->subtitle, -1);
		pango_layout_get_size (layout, &tw, &th);
		drawctx->subtitle_zh = (th / PANGO_SCALE);
		DBC( g_print(" - title: %s w=%d h=%d\n", chart->subtitle, tw, th) );
	}
	drawctx->subtitle_y = drawctx->t + drawctx->title_zh;

	drawctx->graph.y = drawctx->t + drawctx->title_zh + drawctx->subtitle_zh;
	drawctx->graph.height = drawctx->h - drawctx->title_zh - drawctx->subtitle_zh;

	if(drawctx->title_zh > 0 || drawctx->subtitle_zh > 0)
	{
		drawctx->graph.y += CHART_MARGIN;
		drawctx->graph.height -= CHART_MARGIN;
	}

	// compute other text
	chart_set_font_size(chart, layout, CHART_FONT_SIZE_NORMAL);

	// y-axis labels (amounts)
	drawctx->scale_w = 0;
	colchart_compute_range(chart, drawctx);

	valstr = chart_print_int(chart, (gint)drawctx->min);
	pango_layout_set_text (layout, valstr, -1);
	pango_layout_get_size (layout, &tw, &th);
	drawctx->scale_w = (tw / PANGO_SCALE);

	valstr = chart_print_int(chart, (gint)drawctx->max);
	pango_layout_set_text (layout, valstr, -1);
	pango_layout_get_size (layout, &tw, &th);
	drawctx->scale_w = MAX(drawctx->scale_w, (tw / PANGO_SCALE));

	DBC( g_print(" scale   : %d,%d %g,%g\n", drawctx->l, 0, drawctx->scale_w, 0.0) );

	// todo: compute maxwidth of item labels
	double label_w = 0;
	for(i=0;i<chart->nb_items;i++)
	{
	ChartItem *item = &g_array_index(chart->items, ChartItem, i);

		//DBC( g_print(" legend width for %d '%s'\n", i, item->label) );

		// label width
		pango_layout_set_text (layout, item->label, -1);
		pango_layout_get_size (layout, &tw, &th);
		label_w = MAX(label_w, (tw / PANGO_SCALE));
	}
	drawctx->label_w = label_w;

	// compute font height
	drawctx->font_h = (th / PANGO_SCALE);

	// compute graph region
	switch(chart->type)
	{
		case CHART_TYPE_LINE:
		case CHART_TYPE_COL:
		case CHART_TYPE_STACK:
		case CHART_TYPE_STACK100:
			drawctx->graph.x = drawctx->l + drawctx->scale_w + 2;
			drawctx->graph.width  = drawctx->w - drawctx->scale_w - 2;
			break;
		case CHART_TYPE_PIE:
			drawctx->graph.x = drawctx->l;
			drawctx->graph.width  = drawctx->w;
			break;
	}

	DBC( g_print(" - graph : %g,%g %g,%g\n", drawctx->graph.x, drawctx->graph.y, drawctx->graph.width, drawctx->graph.height) );


	if( chart->type != CHART_TYPE_PIE )
	{
		drawctx->graph.y += drawctx->font_h;
		drawctx->graph.height -= drawctx->font_h;

		if( chart->show_xval )
			drawctx->graph.height -= (drawctx->font_h + CHART_SPACING);
	}

	// compute: each legend column width, and legend width
	if(chart->show_legend)
	{
		chart_set_font_size(chart, layout, CHART_FONT_SIZE_SMALL);

		//we compute the rate text here to get the font height
		pango_layout_set_text (layout, "00.00 %", -1);
		pango_layout_get_size (layout, &tw, &th);

		drawctx->legend_font_h = (th / PANGO_SCALE);

		// labels not much than 1/4 of graph
		gdouble lw = floor(drawctx->graph.width / 4);
		drawctx->legend_label_w = MIN(drawctx->label_w, lw);

		drawctx->legend.width = drawctx->legend_font_h + CHART_SPACING + drawctx->legend_label_w;
		drawctx->legend.height = MIN(floor(chart->nb_items * drawctx->legend_font_h * CHART_LINE_SPACING), drawctx->graph.height);

		if(chart->show_legend_wide )
		{
			//get rate width
			drawctx->legend_rate_w = (tw / PANGO_SCALE);

			//#1921741 text overlap
			//get value width		
			drawctx->legend_value_w = 0;
			valstr = chart_print_double(chart, chart->buffer1, drawctx->min);
			pango_layout_set_text (layout, valstr, -1);
			pango_layout_get_size (layout, &tw, &th);
			drawctx->legend_value_w = (tw / PANGO_SCALE);

			valstr = chart_print_double(chart, chart->buffer1, drawctx->max);
			pango_layout_set_text (layout, valstr, -1);
			pango_layout_get_size (layout, &tw, &th);
			drawctx->legend_value_w = MAX(drawctx->legend_value_w, (tw / PANGO_SCALE));
			
			drawctx->legend.width += CHART_SPACING + drawctx->legend_value_w + CHART_SPACING + drawctx->legend_rate_w;
		}

		//if legend visible, substract
		drawctx->graph.width -= (drawctx->legend.width + CHART_MARGIN);

		drawctx->legend.x = drawctx->graph.x + drawctx->graph.width + CHART_MARGIN;
		drawctx->legend.y = drawctx->graph.y;
	}

	DBC( g_print(" graph   : %g %g %g %g\n", drawctx->graph.x, drawctx->graph.y, drawctx->graph.width, drawctx->graph.height ) );
	DBC( g_print(" legend  : %g %g %g %g\n", drawctx->legend.x, drawctx->legend.y, drawctx->legend.width, drawctx->legend.height ) );

	// compute graph region
	switch(chart->type)
	{
		case CHART_TYPE_LINE:
		case CHART_TYPE_COL:
		case CHART_TYPE_STACK:
		case CHART_TYPE_STACK100:
			colchart_calculation(chart, drawctx);
			break;
		case CHART_TYPE_PIE:
			piechart_calculation(chart, drawctx);
			break;
	}

	g_object_unref (layout);

}


static void chart_recompute(GtkChart *chart)
{
HbtkDrawContext *drawctx = &chart->context;
//GdkWindow *gdkwindow;
cairo_surface_t *surf = NULL;
cairo_t *cr;
GtkAllocation allocation;


	DB( g_print("\n[gtkchart] recompute layout\n") );


	if( !gtk_widget_get_realized(chart->drawarea) || chart->surface == NULL )
		return;

	gtk_widget_get_allocation(chart->drawarea, &allocation);

	/*
	//removed deprecated call to gdk_cairo_create
	gdkwindow = gtk_widget_get_window(chart->drawarea);
	if(!gdkwindow)
	{
		surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
		cr = cairo_create (surf);
	}
	else
		cr = gdk_cairo_create (gdkwindow);
	*/

	cr = cairo_create (chart->surface);
	
	drawctx->l = CHART_MARGIN;
	drawctx->t = CHART_MARGIN;
	drawctx->w = allocation.width - (CHART_MARGIN*2);
	drawctx->h = allocation.height - (CHART_MARGIN*2);

	chart_layout_area(cr, chart, drawctx);

	cairo_destroy(cr);
	cairo_surface_destroy(surf);




	//TODO: simplify this
	switch(chart->type)
	{
		case CHART_TYPE_LINE:
		case CHART_TYPE_COL:
		case CHART_TYPE_STACK:
		case CHART_TYPE_STACK100:
			gtk_adjustment_set_value(chart->adjustment, 0);
			colchart_scrollbar_setvalues(chart);
			gtk_widget_show(chart->scrollbar);
			break;
		case CHART_TYPE_PIE:
			gtk_widget_hide(chart->scrollbar);
			break;

	}
}


/* = = = = = = = = = = = = = = = = */


static void chart_draw_part_static(cairo_t *cr, GtkChart *chart, HbtkDrawContext *drawctx)
{
PangoLayout *layout;
int tw, th;


	/*debug help draws */
	#if DBGDRAW_RECT == 1
		//clip area
		cairo_set_line_width(cr, 1.0);
		cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); //green
		cairo_rectangle(cr, drawctx->l+0.5, drawctx->t+0.5, drawctx->w, drawctx->h);
		cairo_stroke(cr);

		//graph area
		cairo_set_source_rgb(cr, 1.0, 0.5, 0.0); //orange
		cairo_rectangle(cr, drawctx->graph.x+0.5, drawctx->graph.y+0.5, drawctx->graph.width, drawctx->graph.height);
		cairo_stroke(cr);
	#endif

	layout = pango_cairo_create_layout (cr);

	if(!drawctx->isprint)
		cairo_user_set_rgbcol(cr, &global_colors[THTEXT]);
	else
		cairo_user_set_rgbcol(cr, &global_colors[BLACK]);

	// draw title
	if(chart->title)
	{

		chart_set_font_size(chart, layout, CHART_FONT_SIZE_TITLE);
		pango_layout_set_text (layout, chart->title, -1);
		pango_layout_get_size (layout, &tw, &th);

		cairo_move_to(cr, drawctx->l, drawctx->t);
		pango_cairo_show_layout (cr, layout);

		#if DBGDRAW_TEXT == 1
			double dashlength;
			cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); //blue
			dashlength = 3;
			cairo_set_dash (cr, &dashlength, 1, 0);
			//cairo_move_to(cr, chart->l, chart->t);
			cairo_rectangle(cr, drawctx->l+0.5, drawctx->t+0.5, (tw / PANGO_SCALE), (th / PANGO_SCALE));
			cairo_stroke(cr);
		#endif

	}

	// draw period


	g_object_unref (layout);

	//test legend
	if(chart->show_legend)
	{
	gint i;
	gchar *valstr;
	gint x, y;
	gint radius;
	gint color;

		DBG( g_print("\n[chart] draw legend\n") );

		layout = pango_cairo_create_layout (cr);
		chart_set_font_size(chart, layout, CHART_FONT_SIZE_SMALL);
		pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

		x = drawctx->legend.x;
		y = drawctx->legend.y;
		radius = drawctx->legend_font_h;

		#if DBGDRAW_RECT == 1
			double dashlength;
			cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);	//orange
			dashlength = 3;
			cairo_set_dash (cr, &dashlength, 1, 0);
			//cairo_move_to(cr, x, y);
			cairo_rectangle(cr, drawctx->legend.x+0.5, drawctx->legend.y+0.5, drawctx->legend.width, drawctx->legend.height);
			cairo_stroke(cr);
		#endif

		for(i=0; i< chart->nb_items ;i++)
		{
		ChartItem *item = &g_array_index(chart->items, ChartItem, i);

			if(item)
			{
				DBG( g_print(" draw %2d of (%d) '%s' y=%d\n", i, chart->nb_items, item->label, y) );

				#if DBGDRAW_TEXT == 1
					double dashlength;
					cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); //blue
					dashlength = 3;
					cairo_set_dash (cr, &dashlength, 1, 0);
					//cairo_move_to(cr, x, y);
					cairo_rectangle(cr, x+0.5, y+0.5, drawctx->legend_font_h, drawctx->legend_font_h);
					cairo_stroke(cr);
					cairo_rectangle(cr, x+drawctx->legend_font_h + CHART_SPACING+0.5, y+0.5, drawctx->legend_label_w, drawctx->legend_font_h);
					cairo_stroke(cr);
				#endif

				// check if enought height to draw
				if( chart->nb_items - i > 1 )
				{
					if( (y + floor(2 * radius * CHART_LINE_SPACING)) > (drawctx->t + drawctx->h) )
					{
						DBG( g_print(" print ...\n\n") );
						pango_layout_set_text (layout, "...", -1);
						cairo_move_to(cr, x + radius + CHART_SPACING, y);
						pango_cairo_show_layout (cr, layout);
						break;
					}
				}

				// 1: palette
				cairo_arc(cr, x + (radius/2), y + (radius/2), (radius/2), 0, 2 * M_PI);
				color = i % chart->color_scheme.nb_cols;
				cairo_user_set_rgbcol(cr, &chart->color_scheme.colors[color]);
				cairo_fill(cr);

				if(!drawctx->isprint)
					cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.78);
				else
					cairo_user_set_rgbacol(cr, &global_colors[BLACK], 0.78);

				// 2: label
				valstr = item->label;
				pango_layout_set_text (layout, valstr, -1);
				pango_layout_set_width(layout, drawctx->legend_label_w * PANGO_SCALE);
				cairo_move_to(cr, x + drawctx->legend_font_h + CHART_SPACING, y);
				pango_cairo_show_layout (cr, layout);

				if( chart->show_legend_wide )
				{
					pango_layout_set_width(layout, -1);

					#if DBGDRAW_TEXT == 1
						// 3: value
						double dashlength;
						cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); //blue
						dashlength = 3;
						cairo_set_dash (cr, &dashlength, 1, 0);
						//cairo_move_to(cr, x, y);
						cairo_rectangle(cr, x + drawctx->legend_font_h + drawctx->legend_label_w + (CHART_SPACING*2), y+0.5, drawctx->legend_value_w, drawctx->legend_font_h);
						cairo_stroke(cr);

						// 4: rate
						cairo_set_dash (cr, &dashlength, 1, 0);
						//cairo_move_to(cr, x, y);
						cairo_rectangle(cr, x + drawctx->legend_font_h + drawctx->legend_label_w + drawctx->legend_value_w + (CHART_SPACING*3), y+0.5, drawctx->legend_rate_w, drawctx->legend_font_h);
						cairo_stroke(cr);
					#endif

					if(!drawctx->isprint)
						cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.78);
					else
						cairo_user_set_rgbacol(cr, &global_colors[BLACK], 0.78);

					// 3: value
					valstr = chart_print_double(chart, chart->buffer1, item->serie1);
					pango_layout_set_text (layout, valstr, -1);
					pango_layout_get_size (layout, &tw, &th);
					cairo_move_to(cr, x + drawctx->legend_font_h + drawctx->legend_label_w + (CHART_SPACING*2) + drawctx->legend_value_w - (tw/PANGO_SCALE), y);
					pango_cairo_show_layout (cr, layout);

					// 4: rate
					valstr = chart_print_rate(chart, item->rate);
					pango_layout_set_text (layout, valstr, -1);
					pango_layout_get_size (layout, &tw, &th);
					cairo_move_to(cr, x + drawctx->legend_font_h + drawctx->legend_label_w + drawctx->legend_value_w + drawctx->legend_rate_w + (CHART_SPACING*3) - (tw/PANGO_SCALE), y);
					pango_cairo_show_layout (cr, layout);
				}

				//the radius contains the font height here
				//y += floor(chart->font_h * CHART_LINE_SPACING);
				y += floor(radius * CHART_LINE_SPACING);
			}
		}

		g_object_unref (layout);

	}
}


static gboolean drawarea_full_redraw(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
HbtkDrawContext *drawctx = &chart->context;
GtkAllocation allocation;
cairo_t *cr;


	DBG( g_print("\n[gtkchart] drawarea full redraw\n") );

	cr = cairo_create (chart->surface);

#if MYDEBUG == 1
cairo_font_face_t *ff;
cairo_scaled_font_t *sf;

	ff = cairo_get_font_face(cr);
	sf = cairo_get_scaled_font(cr);

	g_print(" cairo ff = '%s'\n", cairo_toy_font_face_get_family(ff) );

	ff = cairo_scaled_font_get_font_face(sf);
	g_print(" cairo sf = '%s'\n", cairo_toy_font_face_get_family(ff) );

	//cairo_set_font_face(cr, ff);
#endif

	gtk_widget_get_allocation (GTK_WIDGET (widget), &allocation);

	GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (widget));

	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_VIEW);

	gtk_render_background (context, cr, 0.0, 0.0, allocation.width, allocation.height);

    gtk_style_context_restore (context);


	if( (chart->type == CHART_TYPE_NONE) || (chart->nb_items == 0) )
		goto end;


	chart_draw_part_static(cr, chart, drawctx);


	switch(chart->type)
	{
		case CHART_TYPE_COL:
		case CHART_TYPE_LINE:
		case CHART_TYPE_STACK:
		case CHART_TYPE_STACK100:
			{
			cairo_t *crs;

			crs = cairo_create (chart->surface);

			colchart_draw_scale(crs, chart, drawctx);
			cairo_destroy(crs);
			}
			break;
	}

end:
	cairo_destroy(cr);

	return TRUE;
}


//TODO: we should not rely on this, but get the color in realtime
//		based on the widget state (to support backdrop & disable state)
static void
drawarea_get_style(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
GtkStyleContext *context;
PangoFontDescription *desc;
gboolean colfound;
GdkRGBA color;

	DB( g_print("\n[gtkchart] drawarea get style \n") );

	context = gtk_widget_get_style_context (widget);

	chart_color_global_default();

	// get base color
	colfound = gtk_style_context_lookup_color(context, "theme_base_color", &color);
	if(!colfound)
		colfound = gtk_style_context_lookup_color(context, "base_color", &color);

	if( colfound )
	{
		struct rgbcol *tcol = &global_colors[THBASE];
		tcol->r = color.red * 255;
		tcol->g = color.green * 255;
		tcol->b = color.blue * 255;
		DB( g_print(" - theme base col: %x %x %x\n", tcol->r, tcol->g, tcol->b) );
	}

	//get text color
	colfound = gtk_style_context_lookup_color(context, "theme_text_color", &color);
	if(!colfound)
		//#1916932 colfound was not assigned
		colfound = gtk_style_context_lookup_color(context, "text_color", &color);


	if( colfound )
	{
		struct rgbcol *tcol = &global_colors[THTEXT];
		tcol->r = color.red * 255;
		tcol->g = color.green * 255;
		tcol->b = color.blue * 255;
		DB( g_print(" - theme text (bg) col: %x %x %x\n", tcol->r, tcol->g, tcol->b) );
	}

	//commented 5.1.5
	//drawarea_full_redraw(widget, user_data);

	/* get and copy the font */
	gtk_style_context_get(context, GTK_STATE_FLAG_NORMAL, "font", &desc, NULL);
	if(chart->pfd)
	{
		pango_font_description_free (chart->pfd);
		chart->pfd = NULL;
	}
	chart->pfd = pango_font_description_copy(desc);


	//#1919063 overwrite pango size with device unit taken from css
	GValue value = G_VALUE_INIT;
	gtk_style_context_get_property(context, "font-size", gtk_style_context_get_state(context), &value);

	DB( g_print("font-size is: %f\n", g_value_get_double(&value) ) );

	pango_font_description_set_absolute_size (chart->pfd, (gint)g_value_get_double(&value)*PANGO_SCALE);

	g_value_unset(&value);
}


static gboolean
drawarea_configure_event_callback (GtkWidget         *widget,
                          GdkEventConfigure *event,
                          gpointer           user_data)
{
GtkChart *chart = GTK_CHART(user_data);
GtkAllocation allocation;


	DB( g_print("\n[gtkchart] drawarea configure \n") );

	gtk_widget_get_allocation (widget, &allocation);

	DB( g_print(" w=%d h=%d\n", allocation.width, allocation.height) );


	if (chart->surface)
		cairo_surface_destroy (chart->surface);

	chart->surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               allocation.width,
                                               allocation.height);


	if( gtk_widget_get_realized(widget))
	{
		chart_recompute(chart);
		drawarea_full_redraw(widget, chart);
	}

	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}


static void drawarea_realize_callback(GtkWidget *widget, gpointer   user_data)
{
//GtkChart *chart = GTK_CHART(user_data);

	DB( g_print("\n[gtkchart] drawarea realize\n") );

	DB( g_print(" user_data=%p\n", user_data) );


	//chart_recompute(chart);

}


static void drawarea_state_changed_callback(GtkWidget *widget, GtkStateFlags flags, gpointer   user_data)
{
GtkChart *chart = GTK_CHART(user_data);

	DB( g_print("\n[gtkchart] drawarea state_changed\n") );

	DB( g_print(" user_data=%p\n", user_data) );

	drawarea_get_style(widget, chart);
	drawarea_full_redraw(widget, chart);
	gtk_widget_queue_draw( widget );

}


static void drawarea_style_updated_callback(GtkWidget *widget, gpointer   user_data)
{
GtkChart *chart = GTK_CHART(user_data);

	DB( g_print("\n[gtkchart] drawarea style updated\n") );

	DB( g_print(" user_data=%p\n", user_data) );

	drawarea_get_style(widget, chart);
	drawarea_full_redraw(widget, chart);
	gtk_widget_queue_draw( widget );

}


static gboolean drawarea_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);

	if( !gtk_widget_get_realized(widget) || chart->surface == NULL )
		return FALSE;

	DB( g_print("\n[gtkchart] drawarea draw callback\n") );

	DB( g_print(" state flags: %d\n", gtk_widget_get_state_flags(chart->drawarea)) );

	cairo_set_source_surface (cr, chart->surface, 0, 0);
	cairo_paint (cr);

	switch(chart->type)
	{
		case CHART_TYPE_COL:
			colchart_draw_bars(cr, chart, &chart->context);
			break;
		case CHART_TYPE_LINE:
			linechart_draw_lines(cr, chart, &chart->context);
			break;
		case CHART_TYPE_PIE:
			piechart_draw_slices(cr, chart, &chart->context);
			break;
		case CHART_TYPE_STACK:
		case CHART_TYPE_STACK100:
			colchart_draw_stacks(cr, chart, &chart->context);
			break;
	}

	// event handled
	return TRUE;
}


static gboolean drawarea_querytooltip_callback(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gchar *strval, *strval2;
gchar *buffer = NULL;
gboolean retval = FALSE;

	if(chart->surface == NULL)
		return FALSE;

	DBT( g_print("\n[gtkchart] drawarea querytooltip\n") );

	DBT( g_print(" x=%d, y=%d kbm=%d\n", x, y, keyboard_mode) );
	if(chart->lasthover != chart->hover)
	{
		goto end;
	}

	if(chart->hover >= 0)
	{
	ChartItem *item = &g_array_index(chart->items, ChartItem, chart->hover);

		if( chart->type != CHART_TYPE_STACK && chart->type != CHART_TYPE_STACK100 )
		{
			strval = chart_print_double(chart, chart->buffer1, item->serie1);
			if( !chart->dual )
			{
				//#1420495 don't use g_markup_printf_escaped
				if( chart->type == CHART_TYPE_PIE )
					buffer = g_strdup_printf("%s\n%s\n%.2f%%", item->label, strval, item->rate);
				else
					buffer = g_strdup_printf("%s\n%s", item->label, strval);

			}
			else
			{
				strval2 = chart_print_double(chart, chart->buffer2, item->serie2);
				buffer = g_strdup_printf("%s\n+%s\n%s", item->label, strval2, strval);
			}
		}
		else
		{
		gint colid = chart->colhover;	
		GtkTreeIter iter;
		DataRow *dr;
		gdouble value, rate = 0.0;

			if( colid > -1 )
			{
				gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(chart->model), &iter, NULL, chart->hover);
				
				gtk_tree_model_get (GTK_TREE_MODEL(chart->model), &iter,
					//LST_STORE_TRENDROW=6
					LST_REPDIST2_ROW, &dr,
					-1);

				value = da_datarow_get_cell_sum (dr, colid);
				strval = chart_print_double(chart, chart->buffer1, value);
				
				//#1996223 always show rate
				//if( chart->type == CHART_TYPE_STACK )
				//	buffer = g_strdup_printf("%s\n%s\n%s", chart->collabel[colid], item->label, strval);
				//else
				//{
					if( chart->colsum[colid] > 0 )
						rate = (value * 100) / chart->colsum[colid];
					buffer = g_strdup_printf("%s\n%s\n%s\n%.2f%%", chart->collabel[colid], item->label, strval, rate);
				//}
			}
		}

		gtk_tooltip_set_text(tooltip, buffer);
		//gtk_label_set_markup(GTK_LABEL(chart->ttlabel), buffer);
		g_free(buffer);
		retval = TRUE;
	}

end:
	chart->lasthover = chart->hover;

	return retval;
}


static gboolean drawarea_motionnotifyevent_callback(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
HbtkDrawContext *drawctx = &chart->context;
gboolean retval = TRUE;
gint x, y;

	if(chart->surface == NULL || chart->nb_items == 0)
		return FALSE;

	DBD( g_print("\n[gtkchart] drawarea motion\n") );
	x = event->x;
	y = event->y;

	//DBD( g_print(" x=%d, y=%d\n", x, y) );

	chart->hover = -1;
	chart->colhover = -1;

	//todo see this
	/*if(event->is_hint)
	{
		DB( g_print(" is hint\n") );

		gdk_window_get_device_position(event->window, event->device, &x, &y, NULL);
		//gdk_window_get_pointer(event->window, &x, &y, NULL);
		//return FALSE;
	}*/

		switch(chart->type)
		{
			case CHART_TYPE_COL:
			case CHART_TYPE_LINE:
				chart->hover = colchart_get_hover(widget, x, y, chart);
				break;
			case CHART_TYPE_STACK:
			case CHART_TYPE_STACK100:
				chart->hover = colchart_stack_get_hover(widget, x, y, chart);
				break;
			case CHART_TYPE_PIE:
				chart->hover = piechart_get_hover(widget, x, y, chart);
				break;
		}

	//test: eval legend
	if( chart->type != CHART_TYPE_STACK && chart->type != CHART_TYPE_STACK100 )
	{
		if( chart->show_legend && chart->hover == - 1)
		{
			DBD( g_print(" hover legend\n") );

			if( x >= drawctx->legend.x && (x <= (drawctx->legend.x+drawctx->legend.width ))
			&&  y >= drawctx->legend.y && (y <= (drawctx->legend.y+drawctx->legend.height ))
			)
			{
				//use the radius a font height here
				chart->hover = (y - drawctx->legend.y) / floor(drawctx->legend_font_h * CHART_LINE_SPACING);
			}

			if( chart->hover > chart->nb_items - 1)
			{
				chart->hover = -1;
			}
		}
	}

	// rollover redraw ?
	DBD( g_print(" hover: last=%d, curr=%d\n", chart->lasthover, chart->hover) );

	if( (chart->lasthover != chart->hover) || (chart->lastcolhover != chart->colhover) )
	{
	GdkRectangle update_rect;
	gint first;

		DBD( g_print(" motion rollover redraw :: hover=%d\n", chart->hover) );

		first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

		switch( chart->type )
		{
			case CHART_TYPE_PIE:
			case CHART_TYPE_STACK:
			case CHART_TYPE_STACK100:
				//invalidate all graph area
				update_rect.x = drawctx->graph.x;
				update_rect.y = drawctx->graph.y;
				update_rect.width = drawctx->graph.width;
				update_rect.height = drawctx->graph.height;

				/* Now invalidate the affected region of the drawing area. */
				gdk_window_invalidate_rect (gtk_widget_get_window (widget), &update_rect, FALSE);
				break;

			case CHART_TYPE_COL:
			case CHART_TYPE_LINE:
				//invalidate last rollover block
				if(chart->lasthover != -1)
				{
					update_rect.x = drawctx->graph.x + (chart->lasthover - first) * drawctx->blkw;
					update_rect.y = drawctx->graph.y - 6;
					update_rect.width = drawctx->blkw;
					update_rect.height = drawctx->graph.height + 12;

					// Now invalidate the affected region of the drawing area
					gdk_window_invalidate_rect (gtk_widget_get_window (widget), &update_rect, FALSE);
				}

				//current item block
				update_rect.x = drawctx->graph.x + (chart->hover - first) * drawctx->blkw;
				update_rect.y = drawctx->graph.y - 6;
				update_rect.width = drawctx->blkw;
				update_rect.height = drawctx->graph.height + 12;

				/* Now invalidate the affected region of the drawing area. */
				gdk_window_invalidate_rect (gtk_widget_get_window (widget), &update_rect, FALSE);
				break;

		}
	}

	DBD( g_print(" x=%d, y=%d, time=%d\n", x, y, event->time) );

	//if(inlegend != TRUE)
	//	gtk_tooltip_trigger_tooltip_query(gtk_widget_get_display(chart->drawarea));

	chart->lastcolhover = chart->colhover;

	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* public functions */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void draw_page (GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data)
{
GtkChartPrintData *data = user_data;
HbtkDrawContext *drawctx;
cairo_t *cr;
gdouble t, b, l, r, w, h;

	if(data->chart->type == CHART_TYPE_NONE )
		return;

	gtk_print_context_get_hard_margins(context, &t, &b, &l, &r);
	w = gtk_print_context_get_width(context);
	h = gtk_print_context_get_height(context);


	//TODO: test keeping a ratio and handle orientation myself
	/*
	settings = gtk_print_operation_get_print_settings(operation);
	ratio = (height < width) ? width/height : height/width;

	DB( g_print(" orientation: %d\n", gtk_print_settings_get_orientation(settings)) );
	DB( g_print(" w=%g h=%g // ratio %g\n", width, height, ratio) );

	if( height < width )
		height = width * ratio;
	*/

	//setup our context
	drawctx = &data->drawctx;
	drawctx->isprint = TRUE;
	drawctx->l = l;
	drawctx->t = t;
	drawctx->w = w;
	drawctx->h = h;

	cr = gtk_print_context_get_cairo_context (context);

	cairo_rectangle (cr, l, t, w - r, h - b);
	cairo_clip(cr);

	cairo_set_line_width (cr, 1);

	//draw debug rectangle
	/*
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); //red
	cairo_rectangle(cr, 0, 0, width-r, height-b);
	cairo_stroke(cr);
	*/

	//save usrbarw
	gint tmpusrbar = data->chart->usrbarw;
	//hack it to -1
	data->chart->usrbarw = 0.0; // to span


	chart_layout_area(cr, data->chart, drawctx);
	chart_draw_part_static(cr, data->chart, drawctx);
	if(data->chart->type != CHART_TYPE_PIE)
		colchart_draw_scale(cr, data->chart, drawctx);
	
	switch(data->chart->type)
	{
		case CHART_TYPE_COL:
			colchart_draw_bars(cr, data->chart, drawctx);
			break;
		case CHART_TYPE_PIE:
			piechart_draw_slices(cr, data->chart, drawctx);
			break;
		case CHART_TYPE_LINE:
			linechart_draw_lines(cr, data->chart, drawctx);
			break;
		case CHART_TYPE_STACK:
		case CHART_TYPE_STACK100:
			colchart_draw_stacks(cr, data->chart, drawctx);
			break;
	}

	//restore usrbarw
	data->chart->usrbarw = tmpusrbar;
}


void gtk_chart_print(GtkChart *chart, GtkWindow *parent, gchar *dirname, gchar *filename)
{
GtkChartPrintData *data;
GtkPrintOperation *operation;
GtkPrintSettings *settings;
gchar *ext, *uri = NULL;
GError *error = NULL;

	g_return_if_fail (GTK_IS_CHART (chart));

	data = g_new0 (GtkChartPrintData, 1);

	data->chart = chart;

	settings = gtk_print_settings_new ();
	//TODO: this doesn't work for unknown reason...
	gtk_print_settings_set_orientation(settings, GTK_PAGE_ORIENTATION_LANDSCAPE);

	if( dirname !=  NULL && filename != NULL )
	{
		if (g_strcmp0 (gtk_print_settings_get (settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT), "ps") == 0)
		ext = ".ps";
		else if (g_strcmp0 (gtk_print_settings_get (settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT), "svg") == 0)
		ext = ".svg";
		else
		ext = ".pdf";
		uri = g_strconcat ("file://", dirname, "/", filename, ext, NULL);
		gtk_print_settings_set (settings, GTK_PRINT_SETTINGS_OUTPUT_URI, uri);
	}


	operation = gtk_print_operation_new ();

	gtk_print_operation_set_n_pages (operation, 1);
	gtk_print_operation_set_use_full_page (operation, FALSE);
	gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
	gtk_print_operation_set_embed_page_setup (operation, TRUE);

	gtk_print_operation_set_print_settings (operation, settings);


	GtkPageSetup *ps = gtk_page_setup_new();
	 
	
	if( ps )
		gtk_page_setup_set_orientation(ps, GTK_PAGE_ORIENTATION_LANDSCAPE);
	else
		g_print("pagesetup fail\n");
	
	gtk_print_operation_set_default_page_setup(operation, ps);


	//g_signal_connect (G_OBJECT (operation), "begin-print", G_CALLBACK (begin_print), data);
	g_signal_connect (G_OBJECT (operation), "draw-page", G_CALLBACK (draw_page), data);
	//g_signal_connect (G_OBJECT (operation), "end-print", G_CALLBACK (end_print), data);

	gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW (parent), &error);

	//to use with GTK_PRINT_OPERATION_ACTION_EXPORT
	//gtk_print_operation_set_export_filename(operation, "/home/max/Desktop/testpdffile.pdf");
	//gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_EXPORT, GTK_WINDOW (window), &error);


	g_object_unref (operation);
	g_object_unref (settings);
	g_free (uri);

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

	g_free(data);
}




void gtk_chart_queue_redraw(GtkChart *chart)
{
	DB( g_print("\n[gtkchart] queue redraw\n") );

	if( gtk_widget_get_realized(chart->drawarea) )
	{
		chart_recompute(chart);
		drawarea_full_redraw(chart->drawarea, chart);
		DB( g_print(" gtk_widget_queue_draw\n") );
		gtk_widget_queue_draw( chart->drawarea );
	}
}


/*
** empty the chart
*/
void gtk_chart_set_datas_none(GtkChart *chart)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set no datas\n") );

	chart_clear(chart);
	gtk_chart_queue_redraw(chart);
}


/*
** change the model and/or column
*/
void gtk_chart_set_datas(GtkChart *chart, GtkTreeModel *model, guint column, gchar *title, gchar *subtitle)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set datas\n") );

	if( GTK_IS_TREE_MODEL(model) )
	{
		chart_setup_with_model(chart, model, column, column);
		if(title != NULL)
			chart->title = g_strdup(title);
		if(subtitle != NULL)
			chart->subtitle = g_strdup(subtitle);

		gtk_chart_queue_redraw(chart);
	}
	else
	{
		chart_clear(chart);
	}
}

/*
** change the model and/or column
*/
void gtk_chart_set_dualdatas(GtkChart *chart, GtkTreeModel *model, guint column1, guint column2, gchar *title, gchar *subtitle)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set dualdatas\n") );

	if( GTK_IS_TREE_MODEL(model) )
	{
		chart_setup_with_model(chart, model, column1, column2 );
		if(title != NULL)
			chart->title = g_strdup(title);
		if(subtitle != NULL)
			chart->subtitle = g_strdup(subtitle);

		gtk_chart_queue_redraw(chart);
	}
	else
	{
		chart_clear(chart);
	}
}


/*
** change the type dynamically
*/
void gtk_chart_set_type(GtkChart * chart, gint type)
{
	g_return_if_fail (GTK_IS_CHART (chart));
	//g_return_if_fail (type < CHART_TYPE_MAX);

	DB( g_print("\n[gtkchart] set type %d\n", type) );

	chart->type = type;
	chart->dual = FALSE;

	gtk_chart_queue_redraw(chart);
}

/* = = = = = = = = = = parameters = = = = = = = = = = */

void gtk_chart_set_color_scheme(GtkChart * chart, gint index)
{
	colorscheme_init(&chart->color_scheme, index);
}



/*
** set the minor parameters
*/
void gtk_chart_set_minor_prefs(GtkChart * chart, gdouble rate, gchar *symbol)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set minor prefs\n") );

	chart->minor_rate   = rate;
	chart->minor_symbol = symbol;
}


void gtk_chart_set_absolute(GtkChart * chart, gboolean abs)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set absolute\n") );

	chart->abs = abs;
}


void gtk_chart_set_currency(GtkChart * chart, guint32 kcur)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->kcur = kcur;
}


/*
** set the overdrawn minimum
*/
void gtk_chart_set_overdrawn(GtkChart * chart, gdouble minimum)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set overdrawn\n") );

	chart->minimum = minimum;

	//if(chart->type == CHART_TYPE_LINE)
	//	chart_recompute(chart);
}


/*
** set the barw
*/
void gtk_chart_set_barw(GtkChart * chart, gdouble barw)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set barw\n") );

	if( barw >= GTK_CHART_MINBARW && barw <= GTK_CHART_MAXBARW )
		chart->usrbarw = barw;
	else
		chart->usrbarw = 0;


	if(chart->type != CHART_TYPE_PIE)
		gtk_chart_queue_redraw(chart);
}

/*
** set the show mono (colors)
*/
void gtk_chart_set_showmono(GtkChart * chart, gboolean mono)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->show_mono = mono;

	//if(chart->type != CHART_TYPE_PIE)
	//	gtk_chart_queue_redraw(chart);
}


void gtk_chart_set_smallfont(GtkChart * chart, gboolean small)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->smallfont = small;

	//if(chart->type != CHART_TYPE_PIE)
	//	gtk_chart_queue_redraw(chart);
}


/* = = = = = = = = = = visibility = = = = = = = = = = */

/*
** change the legend visibility
*/
void gtk_chart_show_legend(GtkChart * chart, gboolean visible, gboolean showextracol)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->show_legend = visible;
	chart->show_legend_wide = showextracol;
	//gtk_chart_queue_redraw(chart);
}

/*
** change the x-value visibility
*/
void gtk_chart_show_xval(GtkChart * chart, gboolean visible)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set show xval\n") );

	chart->show_xval = visible;

	//if(chart->type != CHART_TYPE_PIE)
	//	gtk_chart_queue_redraw(chart);
}


void gtk_chart_show_average(GtkChart * chart, gdouble value, gboolean visible)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set show average %f\n", value) );

	chart->average = value;
	chart->show_average = visible;

	//if(chart->type == CHART_TYPE_LINE)
	//	chart_recompute(chart);
}



/*
** chnage the overdrawn visibility
*/
void gtk_chart_show_overdrawn(GtkChart * chart, gboolean visible)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set show overdrawn\n") );

	chart->show_over = visible;

	//if(chart->type == CHART_TYPE_LINE)
	//	chart_recompute(chart);
}


/*
** change the minor visibility
*/
void gtk_chart_show_minor(GtkChart * chart, gboolean minor)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	DB( g_print("\n[gtkchart] set show minor\n") );

	chart->minor = minor;

	//if(chart->type != CHART_TYPE_PIE)
	//	gtk_chart_queue_redraw(chart);

}


/* = = = = = = = = = = = = = = = = */


static void
gtk_chart_dispose (GObject *gobject)
{
//GtkChart *chart = GTK_CHART (object);

	DB( g_print("\n[gtkchart] dispose\n") );

	/* In dispose(), you are supposed to free all types referenced from this
	* object which might themselves hold a reference to self. Generally,
	* the most simple solution is to unref all members on which you own a
	* reference.
	*/

	/* dispose() might be called multiple times, so we must guard against
	* calling g_object_unref() on an invalid GObject by setting the member
	* NULL; g_clear_object() does this for us, atomically.
	*/
	//g_clear_object (&self->priv->an_object);

	/* Always chain up to the parent class; there is no need to check if
	* the parent class implements the dispose() virtual function: it is
	* always guaranteed to do so
	*/
	G_OBJECT_CLASS (gtk_chart_parent_class)->dispose (gobject);
}


static void
gtk_chart_init (GtkChart * chart)
{
GtkWidget *widget, *vbox, *frame;

	DB( g_print("\n[gtkchart] init\n") );


	chart->surface = NULL;
	chart->items = NULL;
 	chart->title = NULL;

	//TODO temp test
	chart->colsum = NULL;
	chart->collabel = NULL;

  	chart->pfd = NULL;
	chart->abs = FALSE;
	chart->dual = FALSE;
	chart->usrbarw = 0.0;

	chart->show_legend = TRUE;
	chart->show_legend_wide = FALSE;
	chart->show_mono = FALSE;
	chart->hover = -1;
	chart->lasthover = -1;
	chart->colhover = -1;
	chart->lastcolhover = -1;

 	chart->minor_rate = 1.0;

 	chart->nb_items = 0;
 	//TODO
	//chart->barw = GTK_CHART_BARW;


	gtk_chart_set_color_scheme(chart, CHART_COLMAP_HOMEBANK);

	widget=GTK_WIDGET(chart);

	gtk_box_set_homogeneous(GTK_BOX(widget), FALSE);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (widget), vbox, TRUE, TRUE, 0);

	/* drawing area */
	frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

	chart->drawarea = gtk_drawing_area_new();
	//gtk_widget_set_double_buffered (GTK_WIDGET(widget), FALSE);

	gtk_container_add( GTK_CONTAINER(frame), chart->drawarea );
	gtk_widget_set_size_request(chart->drawarea, 100, 100 );
#if DYNAMICS == 1
	gtk_widget_set_has_tooltip(chart->drawarea, TRUE);
#endif
	gtk_widget_show(chart->drawarea);

#if MYDEBUG == 1
	/*GtkStyle *style;
	PangoFontDescription *font_desc;

	g_print("draw_area font\n");

	style = gtk_widget_get_style(GTK_WIDGET(chart->drawarea));
	font_desc = style->font_desc;

	g_print("family: %s\n", pango_font_description_get_family(font_desc) );
	g_print("size: %d (%d)\n", pango_font_description_get_size (font_desc), pango_font_description_get_size (font_desc )/PANGO_SCALE );
	*/
#endif

	/* scrollbar */
    chart->adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 1.0, 1.0));
    chart->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (chart->adjustment));
    gtk_box_pack_start (GTK_BOX (vbox), chart->scrollbar, FALSE, TRUE, 0);
	gtk_widget_show(chart->scrollbar);

	g_signal_connect( G_OBJECT(chart->drawarea), "configure-event", G_CALLBACK (drawarea_configure_event_callback), chart);
	g_signal_connect( G_OBJECT(chart->drawarea), "realize", G_CALLBACK(drawarea_realize_callback), chart ) ;
	g_signal_connect( G_OBJECT(chart->drawarea), "draw", G_CALLBACK(drawarea_draw_callback), chart ) ;

	g_signal_connect( G_OBJECT(chart->drawarea), "state-flags-changed", G_CALLBACK(drawarea_state_changed_callback), chart ) ;
	g_signal_connect( G_OBJECT(chart->drawarea), "style-updated", G_CALLBACK(drawarea_style_updated_callback), chart ) ;

#if DYNAMICS == 1
	gtk_widget_add_events(GTK_WIDGET(chart->drawarea),
		GDK_EXPOSURE_MASK |
		GDK_POINTER_MOTION_MASK
		//GDK_POINTER_MOTION_HINT_MASK
		//GDK_BUTTON_PRESS_MASK |
		//GDK_BUTTON_RELEASE_MASK
		);

	g_signal_connect( G_OBJECT(chart->drawarea), "query-tooltip", G_CALLBACK(drawarea_querytooltip_callback), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "motion-notify-event", G_CALLBACK(drawarea_motionnotifyevent_callback), chart );
#endif
	g_signal_connect (G_OBJECT(chart->adjustment), "value-changed", G_CALLBACK (colchart_first_changed), chart);

	//g_signal_connect( G_OBJECT(chart->drawarea), "map-event", G_CALLBACK(chart_map), chart ) ;
	//g_signal_connect( G_OBJECT(chart->drawarea), "button-press-event", G_CALLBACK(chart_button_press), chart );
	//g_signal_connect( G_OBJECT(chart->drawarea), "button-release-event", G_CALLBACK(chart_button_release), chart );
}


static void
gtk_chart_finalize (GObject * object)
{
GtkChart *chart = GTK_CHART (object);

	DB( g_print("\n[gtkchart] finalize\n") );

	chart_clear(chart);

	if(chart->pfd)
	{
		pango_font_description_free (chart->pfd);
		chart->pfd = NULL;
	}

	if (chart->surface)
	{
		cairo_surface_destroy (chart->surface);
		chart->surface = NULL;
	}

	G_OBJECT_CLASS (gtk_chart_parent_class)->finalize (object);
}


static void
gtk_chart_class_init (GtkChartClass * class)
{
GObjectClass *gobject_class = G_OBJECT_CLASS (class);
//GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	DB( g_print("\n[gtkchart] class init\n") );

	//gobject_class->get_property = gtk_chart_get_property;
	//gobject_class->set_property = gtk_chart_set_property;
	gobject_class->dispose = gtk_chart_dispose;
	gobject_class->finalize = gtk_chart_finalize;

	//widget_class->size_allocate = gtk_chart_size_allocate;

}


static void
gtk_chart_class_intern_init (gpointer klass)
{
	gtk_chart_parent_class = g_type_class_peek_parent (klass);
	gtk_chart_class_init ((GtkChartClass *) klass);
}


GType
gtk_chart_get_type ()
{
static GType chart_type = 0;

	if (!chart_type)
    {
		static const GTypeInfo chart_info =
		{
		sizeof (GtkChartClass),
		NULL,		/* base_init */
		NULL,		/* base_finalize */
		(GClassInitFunc) gtk_chart_class_intern_init,
		NULL,		/* class_finalize */
		NULL,		/* class_data */
		sizeof (GtkChart),
		0,		/* n_preallocs */
		(GInstanceInitFunc) gtk_chart_init,
		NULL
		};

		chart_type = g_type_register_static (GTK_TYPE_BOX, "GtkChart",
							 &chart_info, 0);

	}
	return chart_type;
}


GtkWidget *
gtk_chart_new (gint type)
{
GtkChart *chart;

	DB( g_print("\n======================================================\n") );
	DB( g_print("\n[gtkchart] new\n") );

	chart = g_object_new (GTK_TYPE_CHART, NULL);
	chart->type = type;

	return GTK_WIDGET(chart);
}



