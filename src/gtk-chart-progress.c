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
#include "gtk-chart-progress.h"

#include "rep-budget.h"

extern gchar *CHART_CATEGORY;

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/


#define DB(x);
//#define DB(x) (x);

//calculation
#define DBC(x);
//#define DBC(x) (x);

//DB dynamics
#define DBD(x);
//#define DBD(x) (x);


#define DYNAMICS 1

#define DBGDRAW_RECT 0
#define DBGDRAW_TEXT 0
#define DBGDRAW_ITEM 0



/* --- prototypes --- */
static void ui_chart_progress_class_init      (ChartProgressClass *klass);
static void ui_chart_progress_init            (ChartProgress      *chart);
static void ui_chart_progress_destroy         (GtkWidget     *chart);
/*static void	ui_chart_progress_set_property		 (GObject           *object,
						  guint              prop_id,
						  const GValue      *value,
						  GParamSpec        *pspec);*/

static gboolean drawarea_configure_event_callback (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
static gboolean drawarea_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static void drawarea_style_changed_callback(GtkWidget *widget, gpointer   user_data);
static gboolean drawarea_scroll_event_callback( GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
static gboolean drawarea_motionnotifyevent_callback(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void ui_chart_progress_first_changed( GtkAdjustment *adj, gpointer user_data);

//static void ui_chart_progress_clear(ChartProgress *chart);

static gboolean drawarea_full_redraw(GtkWidget *widget, gpointer user_data);
static void ui_chart_progress_queue_redraw(ChartProgress *chart);

/* --- variables --- */
static GtkBoxClass *parent_class = NULL;


/* --- functions --- */



static void ui_chart_progress_set_font_size(ChartProgress *chart, PangoLayout *layout, gint font_size)
{
PangoAttrList *attrs;
PangoAttribute *attr;
double scale = PANGO_SCALE_MEDIUM;

	//PANGO_SCALE_MEDIUM = normal size

	//DB( g_print("\n[chartprogress] set font size\n") );

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
			//size = chart->pfd_size - 2;
			scale = PANGO_SCALE_SMALL;
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


/*
** print a integer number
*/
static gchar *ui_chart_progress_print_int(ChartProgress *chart, gdouble value)
{

	hb_strfmon(chart->buffer, CHART_BUFFER_LENGTH-1, value, chart->kcur, chart->minor);
	return chart->buffer;
}


/*
** get the bar under the mouse pointer
*/
static gint ui_chart_progress_get_hover(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
HbtkDrawProgContext *context = &chart->context;
gint retval, first, index, py;
gint blkw = context->blkw;
double oy;
gboolean docursor = FALSE;

	DB( g_print("\n[chartprogress] get hover\n") );

	retval = -1;

	oy = context->t + context->title_zh + context->header_zh + context->subtitle_zh;

	//DB( g_print(" y=%d, oy=%f, cb=%f\n", y, oy, chart->b) );

	if( (y <= context->b && y >= oy) && (x >= context->l && x <= context->r) )
	{
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
		py = (y - oy);
		index = first + (py / blkw);


		if(index < chart->nb_items)
		{
		StackItem *item = &g_array_index(chart->items, StackItem, index);
		
			retval = index;
			if( item->n_child > 1 )
				docursor = TRUE;
		}
		DB( g_print(" hover=%d\n", retval) );
	}

	//5.7 cursor change
	{
	GdkWindow *gdkwindow;
	GdkCursor *cursor;

		gdkwindow = gtk_widget_get_window (GTK_WIDGET(widget));
		cursor = gdk_cursor_new_for_display(gdk_window_get_display(gdkwindow), (docursor == TRUE) ? GDK_HAND2 : GDK_ARROW );
		gdk_window_set_cursor (gdkwindow, cursor);

		if(GDK_IS_CURSOR(cursor))
			g_object_unref(cursor);
	}

	return(retval);
}


static void ui_chart_progress_first_changed( GtkAdjustment *adj, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
//gint first;

	DB( g_print("\n[chartprogress] bar first changed\n") );

	//first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

	//DB( g_print(" first=%d\n", first) );

/*
	DB( g_print("scrollbar\n adj=%8x, low=%.2f upp=%.2f val=%.2f step=%.2f page=%.2f size=%.2f\n", adj,
		adj->lower, adj->upper, adj->value, adj->step_increment, adj->page_increment, adj->page_size) );
 */
    /* Set the number of decimal places to which adj->value is rounded */
    //gtk_scale_set_digits (GTK_SCALE (hscale), (gint) adj->value);
    //gtk_scale_set_digits (GTK_SCALE (vscale), (gint) adj->value);
    drawarea_full_redraw (chart->drawarea, chart);
	gtk_widget_queue_draw(chart->drawarea);

}

/*
** scrollbar set values for upper, page size, and also show/hide
*/
static void ui_chart_progress_scrollbar_setvalues(ChartProgress *chart)
{
GtkAdjustment *adj = chart->adjustment;
HbtkDrawProgContext *context = &chart->context;
gint first;

	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

	DB( g_print("\n[chartprogress] sb_set_values\n") );

	//if(visible < entries)
	//{
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

		DB( g_print(" entries=%d, visible=%d\n", chart->nb_items, context->visible) );
		DB( g_print(" first=%d, upper=%d, pagesize=%d\n", first, chart->nb_items, context->visible) );

		gtk_adjustment_set_upper(adj, (gdouble)chart->nb_items);
		gtk_adjustment_set_page_size(adj, (gdouble)context->visible);
		gtk_adjustment_set_page_increment(adj, (gdouble)context->visible);

		if(first+context->visible > chart->nb_items)
		{
			gtk_adjustment_set_value(adj, (gdouble)chart->nb_items - context->visible);
		}

		#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION < 18) )
			gtk_adjustment_changed (adj);
		#endif

		//gtk_widget_show(GTK_WIDGET(scrollbar));
	//}
	//else
		//gtk_widget_hide(GTK_WIDGET(scrollbar));


}


static void ui_chart_progress_clear_items(ChartProgress *chart)
{
gint i;

	DB( g_print("\n[chartprogress] clear\n") );

	if(chart->items != NULL)
	{
		for(i=0;i<chart->nb_items;i++)
		{
		StackItem *item = &g_array_index(chart->items, StackItem, i);

			g_free(item->label);	//we free label as it comes from a model_get into setup_with_model
			g_free(item->status);	//we free status as it comes from a model_get into setup_with_model
		}
		g_array_free(chart->items, TRUE);
		chart->items =  NULL;
	}

	chart->nb_items = 0;

}


static void ui_chart_progress_clear(ChartProgress *chart)
{
	DB( g_print("\n[chartprogress] clear\n") );

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

	ui_chart_progress_clear_items(chart);

}


static void ui_chart_progress_setup_with_model(ChartProgress *chart, gint indice)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint i;
gboolean valid = FALSE;

	DB( g_print("\n[chartprogress] setup with model\n") );

	model = chart->model;

	DB( g_print(" indice: %d\n", indice) );

	if( indice < 0 )
	{
		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter);
		chart->nb_items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
		gtk_widget_hide(chart->breadcrumb);
	}
	else
	{
	GtkTreePath *path = gtk_tree_path_new_from_indices(indice, -1);
	gchar *pathstr, *itrlabel;

		pathstr = gtk_tree_path_to_string(path);
		DB( g_print(" total: path: %s\n", pathstr) );

		gtk_tree_model_get_iter(model, &iter, path);
		chart->nb_items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), &iter);

		// update the breadcrumb
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
			LST_BUDGET_NAME, &itrlabel,
			-1);
		gchar *bc = g_markup_printf_escaped("<a href=\"root\">%s</a> &gt; %s", CHART_CATEGORY, itrlabel);
		gtk_label_set_markup(GTK_LABEL(chart->breadcrumb), bc);
		g_free(bc);
		gtk_widget_show(chart->breadcrumb);

		// move to xx:0	
		gtk_tree_path_append_index(path, 0);
		valid = gtk_tree_model_get_iter(model, &iter, path);

		DB( g_print(" total: path: %s\n", pathstr) );
	
		gtk_tree_path_free(path);
		g_free(pathstr);	
	}

	chart->items = g_array_sized_new(FALSE, FALSE, sizeof(StackItem), chart->nb_items);
	DB( g_print(" nbitems=%d, struct=%d\n", chart->nb_items, (gint)sizeof(StackItem)) );

	
	i = 0;
	while (valid)
    {
	gint pos;
	gchar *label, *status;
	gdouble	value1, value2;
	StackItem item;

		//TODO: remove id here....
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
			LST_BUDGET_POS, &pos,
			//LST_BUDGET_KEY, &key,
			LST_BUDGET_NAME, &label,
			LST_BUDGET_SPENT, &value1,
			LST_BUDGET_BUDGET, &value2,
			//LST_BUDGET_FULFILLED, &toto,
			//LST_BUDGET_RESULT, &result,
			LST_BUDGET_STATUS, &status,
			-1);

		item.label = label;
		item.spent = value1;
		item.budget = value2;
		item.status = status;
		item.n_child = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), &iter);		

		/* additional pre-compute */
		item.result = item.spent - item.budget;
		item.rawrate = 0;
		if(ABS(item.budget) > 0)
		{
			item.rawrate = item.spent / item.budget;
		}

		item.warn = item.result < 0.0 ? TRUE : FALSE;

		item.rate = CLAMP(item.rawrate, 0, 1.0);

		g_array_append_vals(chart->items, &item, 1);

		//don't g_free(label); here done into chart_clear
		//don't g_free(status); here done into chart_clear

		i++;
		valid = gtk_tree_model_iter_next (model, &iter);
	}

}


static void chart_progress_layout_area(cairo_t *cr, ChartProgress *chart, HbtkDrawProgContext *context)
{
PangoLayout *layout;
gchar *valstr;
int tw, th, bch;
gint blkw;
gint i;

	DB( g_print("\n[chartprogress] layout area\n") );

	DB( g_print(" print %d ctx:%p\n", context->isprint, context) );


	/* Create a PangoLayout, set the font and text */
	layout = pango_cairo_create_layout (cr);


	// compute title
	context->title_zh = 0;
	if(chart->title)
	{
		//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_TITLE * PANGO_SCALE);
		ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_TITLE);
		pango_layout_set_font_description (layout, chart->pfd);

		pango_layout_set_text (layout, chart->title, -1);
		pango_layout_get_size (layout, &tw, &th);
		context->title_zh = (th / PANGO_SCALE) + CHART_SPACING;
		DBC( g_print(" - title: %s w=%d h=%d\n", chart->title, tw, th) );
	}

	// compute period
	context->subtitle_zh = 0;
	if(chart->subtitle)
	{
		//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_PERIOD * PANGO_SCALE);
		ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_SUBTITLE);
		pango_layout_set_font_description (layout, chart->pfd);

		pango_layout_set_text (layout, chart->subtitle, -1);
		pango_layout_get_size (layout, &tw, &th);
		context->subtitle_zh = (th / PANGO_SCALE) + CHART_SPACING;
		DBC( g_print(" - period: %s w=%d h=%d\n", chart->subtitle, tw, th) );
	}

	// compute other text
	//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_NORMAL * PANGO_SCALE);
	ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_NORMAL);
	pango_layout_set_font_description (layout, chart->pfd);

	//breadcrumb top et position
	ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_NORMAL);
	pango_layout_set_text (layout, "Category", -1);
	pango_layout_get_size (layout, &tw, &th);
	bch = (th / PANGO_SCALE);
	gtk_widget_set_margin_top(chart->breadcrumb, context->subtitle_y);


	double title_w = 0;
	context->bud_col_w = 0;
	context->rel_col_w = 0;

	gdouble maxbudget = 0;
	gdouble maxresult = 0;
	gdouble result;
	for(i=0;i<chart->nb_items;i++)
	{
	StackItem *item = &g_array_index(chart->items, StackItem, i);

		// category width
		if( item->label != NULL )
		{
			pango_layout_set_text (layout, item->label, -1);
			pango_layout_get_size (layout, &tw, &th);
			title_w = MAX(title_w, (tw / PANGO_SCALE));
		}

		DBC( g_print(" - calc '%s' title_w=%f (w=%d)\n", item->label, title_w, tw) );

		//result = ABS(chart->spent[i]) - ABS(chart->budget[i]);
		result = ABS(item->spent - item->budget);

		maxbudget = MAX(maxbudget, ABS(item->budget) );
		maxresult = MAX(maxresult, result);

		DBC( g_print(" - maxbudget maxbudget=%f (w=%d)\n", maxbudget, tw) );

		if( item->status != NULL )
		{
			pango_layout_set_text (layout, item->status, -1);
			pango_layout_get_size (layout, &tw, &th);
			context->rel_col_w = MAX(context->rel_col_w, (tw / PANGO_SCALE));
		}
	}

	context->rel_col_w += CHART_SPACING;

	// compute budget/result width
	valstr = ui_chart_progress_print_int(chart, -maxbudget);
	pango_layout_set_text (layout, valstr, -1);
	pango_layout_get_size (layout, &tw, &th);
	context->bud_col_w = (tw / PANGO_SCALE);
	pango_layout_set_text (layout, chart->budget_title, -1);
	pango_layout_get_size (layout, &tw, &th);
	context->bud_col_w = MAX(context->bud_col_w, (tw / PANGO_SCALE));
	DB( g_print(" budget-col: w=%f, %.2f, '%s'\n", context->bud_col_w, maxbudget, valstr) );


	valstr = ui_chart_progress_print_int(chart, -maxresult);
	pango_layout_set_text (layout, valstr, -1);
	pango_layout_get_size (layout, &tw, &th);
	context->res_col_w = (tw / PANGO_SCALE);
	pango_layout_set_text (layout, chart->result_title, -1);
	pango_layout_get_size (layout, &tw, &th);
	context->res_col_w = MAX(context->res_col_w, (tw / PANGO_SCALE));
	DB( g_print(" result-col: w=%f, %.2f, '%s'\n", context->res_col_w, maxresult, valstr) );


	// collect other width, add margins
	context->header_zh = (th / PANGO_SCALE) + CHART_SPACING;
	//chart->title_y = chart->t;
	context->subtitle_y = context->t + context->title_zh;
	context->header_y = context->subtitle_y + context->subtitle_zh;

	//old pre 5.6	
	//context->cat_col_w = title_w + CHART_SPACING;
	double rem_w = context->w - context->bud_col_w - context->res_col_w - context->rel_col_w - (double)(CHART_SPACING*4);
	if( (title_w + CHART_SPACING) <= (rem_w/3) )
	{
		context->cat_col_w   = title_w + CHART_SPACING;
	}
	else
	{
		context->cat_col_w = 	(rem_w/3);
	}

	context->graph_width  = context->w - context->cat_col_w - context->bud_col_w - context->res_col_w - context->rel_col_w - (double)(CHART_SPACING*4);
	context->graph_height = context->h - context->title_zh - context->subtitle_zh - context->header_zh - bch;


	DB( g_print(" gfx_w = %.2f - %.2f - %.2f  - %.2f - %.2f \n",
		            context->w , context->cat_col_w , context->bud_col_w , context->res_col_w , (double)(CHART_SPACING*4)) );

	DB( g_print(" gfx_w = %.2f\n", context->graph_width) );

	//if expand : we compute available space
	//chart->barw = MAX(32, (chart->graph_width)/chart->nb_items);
	//chart->barw = 32; // usr setted or defaut to BARW

	blkw = context->barw + floor(context->barw * 0.2);
	context->blkw = blkw;

	context->visible = (context->graph_height - context->t) / blkw;
	context->visible = MIN(context->visible, chart->nb_items);

	g_object_unref (layout);

}


static void ui_chart_progress_recompute(ChartProgress *chart)
{
GtkWidget *drawarea = chart->drawarea;
HbtkDrawProgContext *context = &chart->context;
GtkAllocation allocation;
cairo_surface_t *surf;
cairo_t *cr;

	DB( g_print("\n[chartprogress] recompute \n") );

	if( !gtk_widget_get_realized(chart->drawarea) || chart->surface == NULL )
		return;

	gtk_widget_get_allocation(drawarea, &allocation);

	context->l = CHART_MARGIN;
	context->t = CHART_MARGIN;
	context->r = allocation.width  - CHART_MARGIN;
	context->b = allocation.height - CHART_MARGIN;
	context->w = allocation.width  - (CHART_MARGIN*2);
	context->h = allocation.height - (CHART_MARGIN*2);

	//todo: seems not working well...
	surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
	cr = cairo_create (surf);

	chart_progress_layout_area(cr, chart, context);

	cairo_destroy(cr);
	cairo_surface_destroy(surf);

	gtk_adjustment_set_value(chart->adjustment, 0);
	ui_chart_progress_scrollbar_setvalues(chart);
	gtk_widget_show(chart->scrollbar);

	//gtk_widget_queue_draw( chart->drawarea );
}


#if (DBGDRAW_RECT + DBGDRAW_TEXT + DBGDRAW_ITEM) > 0
static void ui_chart_progress_draw_help(cairo_t *cr, ChartProgress *chart, HbtkDrawProgContext *context)
{
double x, y, y2;
gint first = 0;
gint i;

	DB( g_print("\n[chartprogress] draw help\n") );

	DB( g_print(" rect is: %f %f %f %f, barw=%f, ctx=%p\n", context->l, context->t, context->w, context->h, context->barw, context) );


	cairo_set_line_width (cr, 1);

	#if DBGDRAW_RECT == 1
	//clip area
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); //green
	cairo_rectangle(cr, context->l+0.5, context->t+0.5, context->w, context->h);
	cairo_stroke(cr);

	//graph area
	y = context->header_y + context->header_zh;
	cairo_set_source_rgb(cr, 1.0, 0.5, 0.0); //orange
	cairo_rectangle(cr, context->l+context->cat_col_w+0.5, y+0.5, context->graph_width+0.5, context->graph_height+0.5);
	cairo_stroke(cr);
	#endif

	#if DBGDRAW_TEXT == 1
	//title rect
	cairo_set_source_rgb(cr, .0, .0, 1.0);
	cairo_rectangle(cr, context->l+0.5, context->t+0.5, context->w, context->title_zh);
	cairo_stroke(cr);

	//period rect
	cairo_set_source_rgb(cr, .0, 0, 1.0);
	cairo_rectangle(cr, context->l+0.5, context->subtitle_y+0.5, context->w, context->subtitle_zh);
	cairo_stroke(cr);

	//header rect
	cairo_set_source_rgb(cr, .0, 1.0, 1.0);
	cairo_rectangle(cr, context->l+0.5, context->header_y+0.5, context->w, context->header_zh);
	cairo_stroke(cr);

	//category column
	y = context->t + context->title_zh + context->header_zh + context->subtitle_zh;
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); //blue
	cairo_rectangle(cr, context->l+0.5, y+0.5, context->cat_col_w, context->h - y);
	cairo_stroke(cr);

	//budget column
	x = context->l + context->cat_col_w + context->graph_width + CHART_SPACING ;
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); //blue
	cairo_rectangle(cr, x+0.5, y+0.5, context->bud_col_w, context->h - y);
	cairo_stroke(cr);

	//result column
	x = context->l + context->cat_col_w + context->graph_width + context->bud_col_w + (CHART_SPACING*3);
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); //blue
	cairo_rectangle(cr, x+0.5, y+0.5, context->res_col_w, context->h - y);
	cairo_stroke(cr);
	#endif


	// draw item lines
	#if DBGDRAW_ITEM == 1
	y2 = y+0.5;
	cairo_set_line_width(cr, 1.0);
	double dashlength;
	dashlength = 4;
	cairo_set_dash (cr, &dashlength, 1, 0);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0); // violet
	for(i=first; i<=(first+context->visible) ;i++)
	{
		cairo_move_to(cr, context->l, y2);
		cairo_line_to(cr, context->r, y2);

		y2 += context->blkw;
	}
	cairo_stroke(cr);
	#endif

}
#endif


/*
** draw all visible bars
*/
static void ui_chart_progress_draw_bars(cairo_t *cr, ChartProgress *chart, HbtkDrawProgContext *context)
{
double x, y, x2, y2, h;
gint first;
gint i, idx;
gchar *valstr;
PangoLayout *layout;
int tw, th;

	DB( g_print("\n[chartprogress] draw bars\n") );

	DB( g_print(" print %d ctx:%p\n", context->isprint, context) );
	DB( g_print(" rect is: %f %f %f %f, barw=%f, ctx=%p\n", context->l, context->t, context->w, context->h, context->barw, context) );


	layout = pango_cairo_create_layout (cr);

	x = context->l + context->cat_col_w;
	y = context->t + context->title_zh + context->header_zh + context->subtitle_zh;
	first = context->first;

		if(!context->isprint)
			cairo_user_set_rgbcol(cr, &global_colors[THTEXT]);
		else
			cairo_user_set_rgbcol(cr, &global_colors[BLACK]);

	// draw title
	if(chart->title)
	{
		//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_TITLE * PANGO_SCALE);
		ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_TITLE);
		pango_layout_set_font_description (layout, chart->pfd);
		pango_layout_set_text (layout, chart->title, -1);
		pango_layout_get_size (layout, &tw, &th);

		cairo_move_to(cr, context->l, context->t);
		pango_cairo_show_layout (cr, layout);
	}

	// draw period
	if(chart->subtitle)
	{
		//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_PERIOD * PANGO_SCALE);
		ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_SUBTITLE);
		pango_layout_set_font_description (layout, chart->pfd);
		pango_layout_set_text (layout, chart->subtitle, -1);
		pango_layout_get_size (layout, &tw, &th);

		cairo_move_to(cr, context->l, context->subtitle_y);
		pango_cairo_show_layout (cr, layout);
	}

	// draw column title
	if(!context->isprint)
		cairo_user_set_rgbacol (cr, &global_colors[THTEXT], 0.78);
	else
		cairo_user_set_rgbacol (cr, &global_colors[BLACK], 0.78);

	//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_NORMAL * PANGO_SCALE);
	ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_NORMAL);
	pango_layout_set_font_description (layout, chart->pfd);

	pango_layout_set_text (layout, chart->budget_title, -1);
	pango_layout_get_size (layout, &tw, &th);
	cairo_move_to(cr, context->l + context->cat_col_w + context->graph_width + context->bud_col_w + CHART_SPACING - (tw /PANGO_SCALE), context->header_y);
	pango_cairo_show_layout (cr, layout);

	pango_layout_set_text (layout, chart->result_title, -1);
	pango_layout_get_size (layout, &tw, &th);
	cairo_move_to(cr, context->l + context->cat_col_w + context->graph_width + context->bud_col_w + context->res_col_w - (tw /PANGO_SCALE) + (CHART_SPACING*3), context->header_y);
	pango_cairo_show_layout (cr, layout);


	// draw items
	//pango_font_description_set_size(chart->pfd, CHART_FONT_SIZE_NORMAL * PANGO_SCALE);
	ui_chart_progress_set_font_size(chart, layout, CHART_FONT_SIZE_NORMAL);
	pango_layout_set_font_description (layout, chart->pfd);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

	for(i=0; i<context->visible ;i++)
	{
	StackItem *item;
	gint barw = context->barw;
	gint blkw = context->blkw;

		idx = i + first;

		if( (guint)idx > (chart->items->len - 1) )
			break;

		DB( g_print(" draw i:%d idx=%d", i, idx) );

		item = &g_array_index(chart->items, StackItem, idx);

		x2 = x;
		y2 = y + (CHART_SPACING/2) + (blkw * i);

		DB( g_print(" '%-32s' wrn=%d %.2f%% (%.2f%%) :: r=% 4.2f s=% 4.2f b=% 4.2f\n",
			item->label, item->warn, item->rawrate, item->rate, item->result, item->spent, item->budget) );

		valstr = item->label;
		pango_layout_set_text (layout, valstr, -1);
		pango_layout_get_size (layout, &tw, &th);

		double ytext = y2 + ((barw - (th / PANGO_SCALE))/2);

		if(!context->isprint)
			cairo_user_set_rgbacol (cr, &global_colors[THTEXT], 0.78);
		else
			cairo_user_set_rgbacol (cr, &global_colors[BLACK], 0.78);
		//cairo_move_to(cr, context->l + context->cat_col_w - (tw / PANGO_SCALE) - CHART_SPACING, ytext);
		cairo_move_to(cr, context->l, ytext);
		pango_layout_set_width(layout, context->cat_col_w * PANGO_SCALE);
		pango_cairo_show_layout (cr, layout);

		// bar background
		if(!context->isprint)
			cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.15);
		else
			cairo_user_set_rgbacol(cr, &global_colors[BLACK], 0.15);
		cairo_rectangle(cr, x2, y2, context->graph_width, barw);
		cairo_fill(cr);

		//bar with color :: todo migrate this
		h = floor(item->rate * context->graph_width);
		if(item->warn)
		{
			cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[chart->color_scheme.cs_red], idx == chart->hover);
		}
		else
		{
			if(item->rate > 0.8 && item->rate < 1.0)
				cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[chart->color_scheme.cs_orange], idx == chart->hover);
			else
				cairo_user_set_rgbcol_over(cr, &chart->color_scheme.colors[chart->color_scheme.cs_green], idx == chart->hover);
		}

		cairo_rectangle(cr, x2, y2, h, barw);
		cairo_fill(cr);

		// spent value
		if( item->result != 0)
		{
			valstr = ui_chart_progress_print_int(chart, item->spent);
			pango_layout_set_text (layout, valstr, -1);
			pango_layout_get_size (layout, &tw, &th);

			if( h  >= ( (tw / PANGO_SCALE) + (CHART_SPACING*2)) )
			{
				// draw inside
				cairo_user_set_rgbcol(cr, &global_colors[WHITE]);
				cairo_move_to(cr, x2 + h - (tw / PANGO_SCALE) - CHART_SPACING, ytext);
			}
			else
			{
				// draw outside
				//cairo_user_set_rgbacol(cr, &global_colors[THTEXT], 0.78);
				//#1897696 draw out of budget text in red
				if( item->warn )
					cairo_user_set_rgbcol (cr, &chart->color_scheme.colors[chart->color_scheme.cs_red]);				
				else
				{
					if(!context->isprint)						
						cairo_user_set_rgbacol (cr, &global_colors[THTEXT], 0.78);
					else
						cairo_user_set_rgbacol (cr, &global_colors[BLACK], 0.78);
				}
				cairo_move_to(cr, x2 + h + CHART_SPACING, ytext);
			}

			pango_cairo_show_layout (cr, layout);
		}

		// budget value
		valstr = ui_chart_progress_print_int(chart, item->budget);
		pango_layout_set_text (layout, valstr, -1);
		pango_layout_get_size (layout, &tw, &th);

		if(!context->isprint)
			cairo_user_set_rgbacol (cr, &global_colors[THTEXT], 0.78);
		else
			cairo_user_set_rgbacol (cr, &global_colors[BLACK], 0.78);
		cairo_move_to(cr, context->l + context->cat_col_w + context->graph_width + context->bud_col_w + CHART_SPACING - (tw / PANGO_SCALE), ytext);
		pango_cairo_show_layout (cr, layout);

		// result value

		if( item->result != 0)
		{
			valstr = ui_chart_progress_print_int(chart, item->result);

			if(item->warn)
				//cairo_set_source_rgb(cr, COLTOCAIRO(164), COLTOCAIRO(0), COLTOCAIRO(0));
				cairo_user_set_rgbcol(cr, &chart->color_scheme.colors[chart->color_scheme.cs_red]);
			else
			{
				if(!context->isprint)
					cairo_user_set_rgbacol (cr, &global_colors[THTEXT], 0.78);
				else
					cairo_user_set_rgbacol (cr, &global_colors[BLACK], 0.78);
			}

			pango_layout_set_text (layout, valstr, -1);
			pango_layout_get_size (layout, &tw, &th);
			cairo_move_to(cr, context->l + context->cat_col_w + context->graph_width + context->bud_col_w + context->res_col_w - (tw / PANGO_SCALE) + (CHART_SPACING*3), ytext);
			pango_cairo_show_layout (cr, layout);

			// status
			if( item->status )
			{
				pango_layout_set_text (layout, item->status, -1);
				pango_layout_get_size (layout, &tw, &th);
				cairo_move_to(cr, context->l + context->cat_col_w + context->graph_width + context->bud_col_w + context->res_col_w  + (CHART_SPACING*4), ytext);
				pango_cairo_show_layout (cr, layout);
			}
		}

		//y += blkw;

	}

	g_object_unref (layout);

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static gboolean drawarea_full_redraw(GtkWidget *widget, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
HbtkDrawProgContext *context = &chart->context;
cairo_t *cr;

	DB( g_print("\n[chartprogress] drawarea full redraw\n") );

	cr = cairo_create (chart->surface);

	/* fillin the back in white */
	if(!context->isprint)
		cairo_user_set_rgbcol(cr, &global_colors[THBASE]);
	else
		cairo_user_set_rgbcol(cr, &global_colors[WHITE]);

	cairo_paint(cr);

	if(chart->nb_items == 0)
	{
		cairo_destroy(cr);
		return FALSE;
	}

	cairo_rectangle(cr, context->l, context->t, context->w, context->h);
	cairo_clip(cr);


#if (DBGDRAW_RECT + DBGDRAW_TEXT + DBGDRAW_ITEM) > 0
	ui_chart_progress_draw_help(cr, chart, context);
#endif

	context->first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	ui_chart_progress_draw_bars(cr, chart, context);

	cairo_destroy(cr);

	return TRUE;
}


//static void drawarea_get_style(GtkWidget *widget, gpointer user_data)


static gboolean
drawarea_configure_event_callback (GtkWidget         *widget,
                          GdkEventConfigure *event,
                          gpointer           user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
HbtkDrawProgContext *context = &chart->context;
GtkAllocation allocation;
GtkStyleContext *stylctx;
PangoFontDescription *desc;
gboolean colfound;
GdkRGBA color;

	DB( g_print("\n[chartprogress] drawarea configure \n") );

	DB( g_print("w=%d h=%d\n", allocation.width, allocation.height) );

	gtk_widget_get_allocation (widget, &allocation);

	if (chart->surface)
		cairo_surface_destroy (chart->surface);

	chart->surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               allocation.width,
                                               allocation.height);

	stylctx = gtk_widget_get_style_context (widget);

	chart_color_global_default();

	// get base color
	colfound = gtk_style_context_lookup_color(stylctx, "theme_base_color", &color);
	if(!colfound)
		colfound = gtk_style_context_lookup_color(stylctx, "base_color", &color);

	if( colfound )
	{
		struct rgbcol *tcol = &global_colors[THBASE];
		tcol->r = color.red * 255;
		tcol->g = color.green * 255;
		tcol->b = color.blue * 255;
		DB( g_print(" - theme base col: %x %x %x\n", tcol->r, tcol->g, tcol->b) );
	}

	//get text color
	colfound = gtk_style_context_lookup_color(stylctx, "theme_text_color", &color);
	if(!colfound)
		//#1916932 colfound was not assigned
		colfound = gtk_style_context_lookup_color(stylctx, "text_color", &color);

	if( colfound )
	{
		struct rgbcol *tcol = &global_colors[THTEXT];
		tcol->r = color.red * 255;
		tcol->g = color.green * 255;
		tcol->b = color.blue * 255;
		DB( g_print(" - theme text (bg) col: %x %x %x\n", tcol->r, tcol->g, tcol->b) );
	}

	/* get and copy the font */
	gtk_style_context_get(stylctx, GTK_STATE_FLAG_NORMAL, "font", &desc, NULL);
	if(chart->pfd)
	{
		pango_font_description_free (chart->pfd);
		chart->pfd = NULL;
	}
	chart->pfd = pango_font_description_copy(desc);
	chart->pfd_size = pango_font_description_get_size (desc) / PANGO_SCALE;
	context->barw = (6 + chart->pfd_size) * PHI;

	//leak: we should free desc here ?
	//or no need to copy above ?
	//pango_font_description_free(desc);

	DB( g_print("family: %s\n", pango_font_description_get_family(chart->pfd) ) );
	DB( g_print("size  : %d (%d)\n", chart->pfd_size, chart->pfd_size/PANGO_SCALE ) );
	DB( g_print("isabs : %d\n", pango_font_description_get_size_is_absolute (chart->pfd) ) );

	if( gtk_widget_get_realized(widget) )
	{
		ui_chart_progress_recompute(chart);
		drawarea_full_redraw(widget, user_data);
	}

	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}


//state


static void drawarea_style_changed_callback(GtkWidget *widget, gpointer   user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);

	DB( g_print("\n[chartprogress] drawarea style changed\n") );

	if( gtk_widget_get_realized(widget))
	{
		drawarea_full_redraw(widget, chart);
	}
}


static gboolean drawarea_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
HbtkDrawProgContext *context = &chart->context;

	if( !gtk_widget_get_realized(widget) || chart->surface == NULL )
		return FALSE;

	DB( g_print("\n[chartprogress] drawarea draw cb\n") );

	cairo_set_source_surface (cr, chart->surface, 0, 0);
	cairo_paint (cr);

	/* always redraw directly the hover block */
	gint first;
	double ox, oy;

	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
	ox = context->l + context->cat_col_w;
	oy = context->t + context->title_zh + context->header_zh + context->subtitle_zh;


	if(chart->hover != -1)
	{
		DB( g_print(" draw hover\n") );

		cairo_rectangle(cr, context->l, context->t, context->w, context->h);
		cairo_clip(cr);

		oy += CHART_SPACING/2 + (chart->hover - first) * context->blkw;
		cairo_user_set_rgbacol(cr, &global_colors[WHITE], OVER_ALPHA);

		cairo_rectangle(cr, ox, oy, context->graph_width, context->barw);
		cairo_fill(cr);
	}

	return FALSE;
}


static gboolean
drawarea_cb_root_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);

	DB( g_print("\n[chartprogress] root breadcrumb clicked\n") );

	ui_chart_progress_clear_items(chart);
	ui_chart_progress_setup_with_model(chart, -1);
	ui_chart_progress_queue_redraw(chart);
    return TRUE;
}


static gboolean
drawarea_cb_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);

	if (chart->surface == NULL)
		return FALSE; /* paranoia check, in case we haven't gotten a configure event */

	DB( g_print("\n[chartprogress] mouse button press event\n") );

	if (event->button == GDK_BUTTON_PRIMARY)
	{
		DB( g_print(" x=%f, y=%f\n", event->x, event->y) );

		if( chart->hover >= 0 )
		{
		StackItem *item = &g_array_index(chart->items, StackItem, chart->hover);

			if( item->n_child > 1 )
			{		
				ui_chart_progress_clear_items(chart);
				ui_chart_progress_setup_with_model(chart, chart->hover);
				ui_chart_progress_queue_redraw(chart);
			}
		}
	}

	/* We've handled the event, stop processing */
	return TRUE;
}




static gboolean drawarea_motionnotifyevent_callback(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
HbtkDrawProgContext *context = &chart->context;
gint x, y;

	if(chart->surface == NULL || chart->nb_items == 0)
		return FALSE;

	DBD( g_print("\n[chartprogress] drawarea motion cb\n") );
	x = event->x;
	y = event->y;

	//todo see this
	if(event->is_hint)
	{
		//DB( g_print(" is hint\n") );

		gdk_window_get_device_position(event->window, event->device, &x, &y, NULL);
		//gdk_window_get_pointer(event->window, &x, &y, NULL);
		//return FALSE;
	}

	chart->hover = ui_chart_progress_get_hover(widget, x, y, chart);


	// rollover redraw ?
	DBD( g_print(" %d, %d :: hover: last=%d, curr=%d\n", x, y, chart->lasthover, chart->hover) );

	if(chart->lasthover != chart->hover)
	{
	GdkRectangle update_rect;
	gint first;
	double oy;

		DBD( g_print(" motion rollover redraw :: hover=%d\n", chart->hover) );

		first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
		oy = context->t + context->title_zh + context->header_zh + context->subtitle_zh;

		if(chart->lasthover != -1)
		{
			update_rect.x = context->l;
			update_rect.y = oy + (chart->lasthover - first) * context->blkw;
			update_rect.width = context->r;
			update_rect.height = context->blkw;

			/* Now invalidate the affected region of the drawing area. */
			gdk_window_invalidate_rect (gtk_widget_get_window (widget),
		                          &update_rect,
		                          FALSE);
		}

		update_rect.x = context->l;
		update_rect.y = oy + (chart->hover - first) * context->blkw;
		update_rect.width = context->r;
		update_rect.height = context->blkw;

		/* Now invalidate the affected region of the drawing area. */
		gdk_window_invalidate_rect (gtk_widget_get_window (widget),
	                          &update_rect,
	                          FALSE);

		//gtk_widget_queue_draw( widget );
		//retval = FALSE;
	}

	chart->lasthover = chart->hover;

	return TRUE;
}


static gboolean drawarea_scroll_event_callback( GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
ChartProgress *chart = GTK_CHARTPROGRESS(user_data);
GtkAdjustment *adj = chart->adjustment;
gdouble first, upper, pagesize;

	DB( g_print("\n[chartprogress] scroll\n") );

	first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));
	//lower = gtk_adjustment_get_lower(GTK_ADJUSTMENT(adj));
	upper = gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj));
	pagesize = gtk_adjustment_get_page_size(GTK_ADJUSTMENT(adj));

	DB( g_print("- pos is %.2f, [%.2f - %.2f]\n", first, 0.0, upper) );

	switch(event->direction)
	{
		case GDK_SCROLL_UP:
			gtk_adjustment_set_value(adj, first - 1);
			break;
		case GDK_SCROLL_DOWN:
			gtk_adjustment_set_value(adj, CLAMP(first + 1, 0, upper - pagesize) );
			break;
		default:
			break;
	}

	drawarea_full_redraw(widget, user_data);

	return TRUE;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* public functions */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
static void
gtk_chart_progress_begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             gpointer           user_data)
{
GtkChartProgPrintData *data = user_data;
HbtkDrawProgContext *drawctx;
gdouble t, l, w, h;
cairo_t *cr;

	DB( g_print("\n[chartprogress] begin print\n") );


	//gtk_print_context_get_hard_margins(context, &t, &b, &l, &r);
	t = 0;
	l = 0;
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



	//hack
	PangoContext *pctx = gtk_print_context_create_pango_context(context);
	PangoFontDescription *pfd = pango_context_get_font_description(pctx);
	drawctx->barw = (6 + (pango_font_description_get_size(pfd) / PANGO_SCALE )) * PHI;
	g_object_unref(pctx);


	cr = gtk_print_context_get_cairo_context (context);

	DB( g_print(" rect is: %f %f %f %f, barw=%f, ctx=%p\n", l, t, w, h, drawctx->barw, drawctx) );


	chart_progress_layout_area(cr, data->chart, drawctx);


	data->num_pages = ceil((gdouble)data->chart->nb_items / (gdouble)drawctx->visible);

	g_print(" nb pages: %d, nbitems %d / visi %d = %.2f\n", data->num_pages, data->chart->nb_items, drawctx->visible, (gdouble)data->chart->nb_items / (gdouble)drawctx->visible);

	gtk_print_operation_set_n_pages (operation, data->num_pages);


}


static void gtk_chart_progress_draw_page (GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data)
{
GtkChartProgPrintData *data = user_data;
HbtkDrawProgContext *drawctx;
cairo_t *cr;

	DB( g_print("\n[chartprogress] draw page\n") );


	cr = gtk_print_context_get_cairo_context (context);

	drawctx = &data->drawctx;

	//cairo_rectangle (cr, drawctx->l, drawctx->t, drawctx->w - drawctx->r, drawctx->h - drawctx->b);
	//cairo_clip(cr);

	cairo_set_line_width (cr, 1);

	//draw debug rectangle
	/*
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); //red
	cairo_rectangle(cr, 0, 0, drawctx->w-drawctx->r, drawctx->h-drawctx->b);
	cairo_stroke(cr);
	*/

	drawctx->first = page_nr * drawctx->visible;

	g_print(" draw page %d, with first=%d\n", page_nr, drawctx->first);


#if (DBGDRAW_RECT + DBGDRAW_TEXT + DBGDRAW_ITEM) > 0
	ui_chart_progress_draw_help(cr, data->chart, drawctx);
#endif

	ui_chart_progress_draw_bars(cr, data->chart, drawctx);

}


void gtk_chart_progress_print(ChartProgress *chart, GtkWindow *parent, gchar *dirname, gchar *filename)
{
GtkChartProgPrintData *data;
GtkPrintOperation *operation;
GtkPrintSettings *settings;
gchar *ext, *uri = NULL;
GError *error = NULL;

	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));


	g_print("\n[chartprogress] print\n");

	data = g_new0 (GtkChartProgPrintData, 1);

	data->chart = chart;

	settings = gtk_print_settings_new ();
	//TODO: this doesn't work for unknown reason...
	gtk_print_settings_set_orientation(settings, GTK_PAGE_ORIENTATION_PORTRAIT);

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

	g_signal_connect (G_OBJECT (operation), "begin-print", G_CALLBACK (gtk_chart_progress_begin_print), data);
	g_signal_connect (G_OBJECT (operation), "draw-page", G_CALLBACK (gtk_chart_progress_draw_page), data);
	//g_signal_connect (G_OBJECT (operation), "end-print", G_CALLBACK (end_print), data);


	gtk_print_operation_set_use_full_page (operation, FALSE);
	gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
	gtk_print_operation_set_embed_page_setup (operation, TRUE);

	gtk_print_operation_set_print_settings (operation, settings);

	//test forec pagfe
	//GtkPageSetup *ps = gtk_print_operation_get_default_page_setup(operation);

	GtkPageSetup *ps = gtk_page_setup_new();


	if( ps )
		gtk_page_setup_set_orientation(ps, GTK_PAGE_ORIENTATION_LANDSCAPE);
	else
		g_print("pagesetup fail\n");

	gtk_print_operation_set_default_page_setup(operation, ps);


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





static void ui_chart_progress_queue_redraw(ChartProgress *chart)
{

	DB( g_print("\n[chartprogress] queue redraw\n") );


	if( gtk_widget_get_realized(GTK_WIDGET(chart)) )
	{
		ui_chart_progress_recompute(chart);
		drawarea_full_redraw(chart->drawarea, chart);
		gtk_widget_queue_draw( chart->drawarea );
	}
}

/*
** change the model and/or column
*/
void ui_chart_progress_set_dualdatas(ChartProgress *chart, GtkTreeModel *model, gchar *coltitle1, gchar *coltitle2, gchar *title, gchar *subtitle)
{
	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));

	DB( g_print("\n[chartprogress] set dual datas\n") );

	ui_chart_progress_clear(chart);

	if( GTK_IS_TREE_MODEL(model) )
	{
		DB( g_print(" store model %p and columns=%s:%s\n", model, coltitle1, coltitle2) );
		chart->model = model;

		if(coltitle1)
			chart->budget_title = coltitle1;
		if(coltitle2)
			chart->result_title = coltitle2;

		if(title != NULL)
			chart->title = g_strdup(title);
		if(subtitle != NULL)
			chart->subtitle = g_strdup(subtitle);

		ui_chart_progress_setup_with_model(chart, -1);
		ui_chart_progress_queue_redraw(chart);
	}
}

/*
** change the tooltip title
*/
void ui_chart_progress_set_title(ChartProgress * chart, gchar *title)
{
	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));

	chart->title = g_strdup(title);

	DB( g_print("\n[chartprogress] set title = %s\n", chart->title) );

	ui_chart_progress_recompute(chart);

}

void ui_chart_progress_set_subtitle(ChartProgress * chart, gchar *subtitle)
{
	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));

	chart->subtitle = g_strdup(subtitle);

	DB( g_print("\n[chartprogress] set period = %s\n", chart->subtitle) );

	ui_chart_progress_recompute(chart);

}


/*
** change the minor visibility
*/
void ui_chart_progress_show_minor(ChartProgress * chart, gboolean minor)
{
	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));

	chart->minor = minor;

	ui_chart_progress_queue_redraw(chart);

}

void ui_chart_progress_set_color_scheme(ChartProgress * chart, gint index)
{
	colorscheme_init(&chart->color_scheme, index);
}


/*
** set the minor parameters
*/
/*void ui_chart_progress_set_minor_prefs(ChartProgress * chart, gdouble rate, gchar *symbol)
{
	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));

	chart->minor_rate   = rate;
	chart->minor_symbol = symbol;
}*/

void ui_chart_progress_set_currency(ChartProgress * chart, guint32 kcur)
{
	g_return_if_fail (GTK_IS_CHARTPROGRESS (chart));

	chart->kcur = kcur;
}


/* = = = = = = = = = = = = = = = = */


static void
ui_chart_progress_init (ChartProgress * chart)
{
GtkWidget *widget, *hbox, *frame, *overlay, *label;
HbtkDrawProgContext *context = &chart->context;


	DB( g_print("\n[chartprogress] init\n") );

	chart->surface = NULL;
 	chart->nb_items = 0;
	chart->hover = -1;
 	chart->title = NULL;
	chart->subtitle = NULL;
 	chart->pfd = NULL;

 	chart->budget_title = "Budget";
 	chart->result_title = "Result";

	context->barw = GTK_CHARTPROGRESS_BARW;
	ui_chart_progress_set_color_scheme(chart, CHART_COLMAP_HOMEBANK);

	widget=GTK_WIDGET(chart);
	gtk_box_set_homogeneous(GTK_BOX(widget), FALSE);

	frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (widget), frame, TRUE, TRUE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_frame_set_child(GTK_FRAME(frame), hbox);

	overlay = gtk_overlay_new ();
	chart->drawarea = gtk_drawing_area_new();
	gtk_widget_set_size_request(chart->drawarea, 150, 150 );
#if DYNAMICS == 1
	gtk_widget_set_has_tooltip(chart->drawarea, TRUE);
#endif

	gtk_widget_show(chart->drawarea);

	gtk_overlay_set_child (GTK_OVERLAY(overlay), chart->drawarea);
	gtk_box_pack_start (GTK_BOX (hbox), overlay, TRUE, TRUE, 0);


	label = gtk_label_new(NULL);
	chart->breadcrumb = label;
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_track_visited_links(GTK_LABEL(label), FALSE);
	gtk_overlay_add_overlay( GTK_OVERLAY(overlay), label );
	gtk_overlay_set_overlay_pass_through (GTK_OVERLAY (overlay), label, TRUE);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_valign (label, GTK_ALIGN_START);	
	gtk_widget_set_margin_start(label, SPACING_MEDIUM);
	gtk_widget_set_margin_top(label, SPACING_MEDIUM*4);


	/* scrollbar */
    chart->adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 1.0, 1.0));
    chart->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,GTK_ADJUSTMENT (chart->adjustment));
    gtk_box_pack_start (GTK_BOX (hbox), chart->scrollbar, FALSE, TRUE, 0);


	g_signal_connect( G_OBJECT(chart->drawarea), "configure-event", G_CALLBACK (drawarea_configure_event_callback), chart);
	//g_signal_connect( G_OBJECT(chart->drawarea), "realize", G_CALLBACK(drawarea_realize_callback), chart ) ;
	g_signal_connect( G_OBJECT(chart->drawarea), "draw", G_CALLBACK(drawarea_draw_callback), chart );
	//TODO check this to redraw when gtk theme chnage
	g_signal_connect( G_OBJECT(chart->drawarea), "style-updated", G_CALLBACK(drawarea_style_changed_callback), chart ) ;

#if DYNAMICS == 1
	gtk_widget_add_events(GTK_WIDGET(chart->drawarea),
		GDK_EXPOSURE_MASK |
		//GDK_POINTER_MOTION_MASK |
		//GDK_POINTER_MOTION_HINT_MASK |
		GDK_BUTTON_PRESS_MASK | 
		GDK_SCROLL_MASK
		);

	//g_signal_connect( G_OBJECT(chart->drawarea), "query-tooltip", G_CALLBACK(drawarea_querytooltip_callback), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "scroll-event", G_CALLBACK(drawarea_scroll_event_callback), chart ) ;
	g_signal_connect( G_OBJECT(chart->drawarea), "motion-notify-event", G_CALLBACK(drawarea_motionnotifyevent_callback), chart );
#endif

	g_signal_connect (G_OBJECT(chart->adjustment), "value-changed", G_CALLBACK (ui_chart_progress_first_changed), chart);

	g_signal_connect (G_OBJECT(chart->breadcrumb), "activate-link", G_CALLBACK (drawarea_cb_root_activate_link), chart);
	
	//g_signal_connect( G_OBJECT(chart->drawarea), "leave-notify-event", G_CALLBACK(ui_chart_progress_leave), chart );
	//g_signal_connect( G_OBJECT(chart->drawarea), "enter-notify-event", G_CALLBACK(ui_chart_progress_enter), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "button-press-event", G_CALLBACK(drawarea_cb_button_press_event), chart );
	//g_signal_connect( G_OBJECT(chart->drawarea), "button-release-event", G_CALLBACK(ui_chart_progress_button_release), chart );
}


void
ui_chart_progress_destroy (GtkWidget * object)
{
ChartProgress *chart = GTK_CHARTPROGRESS(object);

	g_return_if_fail (GTK_IS_CHARTPROGRESS (object));

	DB( g_print("\n[chartprogress] destroy\n") );


	ui_chart_progress_clear(GTK_CHARTPROGRESS (object));

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

	GTK_WIDGET_CLASS (parent_class)->destroy (object);
}


static void ui_chart_progress_class_init (ChartProgressClass * class)
{
//GObjectClass *gobject_class;
GtkWidgetClass *widget_class;

	DB( g_print("\n[chartprogress] class_init\n") );

	//gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	parent_class = g_type_class_peek_parent (class);

	//gobject_class->dispose = ui_chart_progress_dispose;
	//gobject_class->finalize = ui_chart_progress_finalize;
	//gobject_class->set_property = ui_chart_progress_set_property;
	//gobject_class->get_property = ui_chart_progress_get_property;

	widget_class->destroy = ui_chart_progress_destroy;


}

//gtk_chart_class_intern_init


GType ui_chart_progress_get_type ()
{
static GType ui_chart_progress_type = 0;

	if (G_UNLIKELY(ui_chart_progress_type == 0))
    {
		const GTypeInfo ui_chart_progress_info =
		{
		sizeof (ChartProgressClass),
		NULL,	/* base_init */
		NULL,	/* base_finalize */
		(GClassInitFunc) ui_chart_progress_class_init,
		NULL,	/* class_finalize */
		NULL,	/* class_init */
		sizeof (ChartProgress),
		0,		/* n_preallocs */
		(GInstanceInitFunc) ui_chart_progress_init,
		NULL	/* value_table */
		};

		ui_chart_progress_type = g_type_register_static (GTK_TYPE_BOX, "ChartProgress",
							 &ui_chart_progress_info, 0);

	}
	return ui_chart_progress_type;
}


GtkWidget *
ui_chart_progress_new (void)
{
GtkWidget *chart;

	DB( g_print("\n======================================================\n") );
	DB( g_print("\n[chartprogress] new\n") );

	chart = (GtkWidget *)g_object_new (GTK_TYPE_CHARTPROGRESS, NULL);

	return chart;
}



