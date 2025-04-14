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

#ifndef __GTK_CHART_H__
#define __GTK_CHART_H__

#include "gtk-chart-colors.h"

G_BEGIN_DECLS
#define GTK_TYPE_CHART            (gtk_chart_get_type ())
#define GTK_CHART(obj)			  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CHART, GtkChart))
#define GTK_CHART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CHART, GtkChartClass)
#define GTK_IS_CHART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CHART))
#define GTK_IS_CHART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CHART))
#define GTK_CHART_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CHART, GtkChartClass))

typedef struct _GtkChart		GtkChart;
typedef struct _GtkChartClass	GtkChartClass;
//typedef struct _GtkChartPrivate	GtkChartPrivate;

typedef struct _ChartItem	    ChartItem;
typedef struct _HbtkDrawContext		HbtkDrawContext;

typedef gchar (* GtkChartPrintIntFunc)    (gint value, gboolean minor);
typedef gchar (* GtkChartPrintDoubleFunc) (gdouble value, gboolean minor);

/* = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/* phi value */
#define PHI 1.61803399


/* default zoomx for charts */
#define GTK_CHART_BARW 			41	//24

//TODO: #2038623 maybe go lower to 3 pixels for lines
#define GTK_CHART_MINBARW 		 8	//4
#define GTK_CHART_MAXBARW 		41	//128
#define GTK_CHART_SPANBARW		GTK_CHART_MAXBARW+1

#define GTK_CHART_MINRADIUS 	64

#define CHART_BUFFER_LENGTH 128


// char draw options
#define CHART_PARAM_PIE_DONUT		TRUE
#define CHART_PARAM_PIE_LINE		FALSE
#define CHART_PARAM_PIE_MARK		FALSE

// 5.5
//#define CHART_PARAM_PIE_HOLEVALUE	0.5
#define CHART_PARAM_PIE_HOLEVALUE	0.61803399



/* new stuff */
#define CHART_MARGIN	12 //standard a4 margin is 18
#define CHART_SPACING   6

#define CHART_LINE_SPACING  1.25

//#define PROP_SHOW_MINOR		6
//#define PROP_SHOW_LEGEND	7

#define ROUNDHALF(x) floor(x *2) / 2


enum
{
	CHART_TYPE_NONE,
	CHART_TYPE_COL,

	CHART_TYPE_PIE,
	CHART_TYPE_LINE,

	CHART_TYPE_STACK,
	CHART_TYPE_STACK100,

	CHART_TYPE_MAX
};


enum
{
	LST_LEGEND_FAKE,
	LST_LEGEND_COLOR,
	LST_LEGEND_TITLE,
	LST_LEGEND_AMOUNT,
	LST_LEGEND_RATE,
	NUM_LST_LEGEND
};


struct _ChartItem
{
	/* data part */
	gchar		*label;
	//gchar		*xlabel;
	//gchar		*misclabel;
	//gshort		flags;
	//gshort		pad1;
	gdouble		serie1;
	gdouble		serie2;
	gdouble		rate;
	gint		n_child;

	/* draw stuffs */
	//gchar    *legend;
	double	 angle2;	  /* rate for pie */
	double	 height;   /* for column */ 
};


struct _HbtkDrawContext
{
	gboolean	isprint;


	gint	visible;
	double	range, max;
	double	min;
	double	unit;
	gint	div;
	double  barw, blkw;
//	double	posbarh, negbarh;

	/* drawing datas */
	double	font_h;
	int		l, t, w, h;
//	int		b, r;
	cairo_rectangle_t	graph;
	cairo_rectangle_t	legend;

	double	scale_w;
//	double scale_x, scale_y, scale_h;
//	double item_x, item_y, item_w;

	/* legend dimension */
	double  legend_font_h;
	double  legend_label_w;
	double	legend_value_w;
	double	legend_rate_w;

	/* zones height */
	double  title_zh;
	double  subtitle_zh, subtitle_y;
//	double  header_zh, header_y;
//	double  item_zh;

	double		ox, oy;
//	gint		lastx, lasty, ;
//	gint		lastpress_x, lastpress_y;
//	guint		timer_tag;
	gint		rayon, mark;
//	gint		left, top;

};


struct _GtkChart
{
	//own widget here

	/*< private >*/
	//GtkChartPrivate *priv;

	/* all below should be in priv normally */
	GtkBox			hbox;

	GtkWidget		*drawarea;
	GtkAdjustment	*adjustment;
	GtkWidget		*scrollbar;
	GtkWidget		*breadcrumb;

	/* data storage */
	GtkTreeModel   *totmodel;
	guint		column1, column2;
	gint		nb_items;
	GArray		*items;
	double		rawmin, rawmax;
	gdouble		total;

	//TODO: test of total, in waiting something else
	GtkTreeModel	*model;
	gdouble		*colsum;
	gchar       **collabel;
	DataCol		**cols;
	gint		nb_cols;
	gint		colindice;

	gchar		*title;
	gchar		*subtitle;

	/* chart properties */
	gint		type;	   // column/pie/line

	gboolean	dual;
	gboolean	abs;
	gboolean	show_breadcrumb;
	gboolean	show_legend;
	gboolean	show_legend_wide;
	gboolean	legend_visible;
	gboolean	legend_wide_visible;
	gboolean	smallfont;
	gboolean	show_over;
	gboolean	show_average;
	gboolean	show_xval;
	gboolean	show_mono;
	guint32		kcur;
	gboolean	minor;
	gdouble		minor_rate;
	gdouble		minimum, average;
	gint		usrbarw;
	gchar		*minor_symbol;

	/* color datas */
	GtkColorScheme color_scheme;
	
	cairo_surface_t	 *surface;

	//dynamics
	gint		hover, lasthover;
	gint		colhover, lastcolhover;
	gboolean	drillable;

	struct _HbtkDrawContext context;
	PangoFontDescription *pfd;

	gchar		buffer1[CHART_BUFFER_LENGTH];
	gchar		buffer2[CHART_BUFFER_LENGTH];

};


struct _GtkChartClass
{
	GtkBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


typedef struct
{
	GtkChart *chart;
	HbtkDrawContext drawctx;
} GtkChartPrintData;


GType gtk_chart_get_type (void) G_GNUC_CONST;

void gtk_chart_print(GtkChart *chart, GtkWindow *parent, gchar *dirname, gchar *filename);

/* public function */
GtkWidget *gtk_chart_new(gint type);

void gtk_chart_set_datas_none (GtkChart *chart);
void gtk_chart_set_datas_total(GtkChart *chart, GtkTreeModel *model, guint column1, guint column2, gchar *title, gchar *subtitle);
void gtk_chart_set_datas_time (GtkChart *chart, GtkTreeView *treeview, DataTable *dt, guint nbrows, guint nbcols, gchar *title, gchar *subtitle);

void gtk_chart_set_type(GtkChart *chart, gint type);
void gtk_chart_set_color_scheme(GtkChart * chart, gint colorscheme);

void gtk_chart_queue_redraw(GtkChart *chart);

void gtk_chart_set_minor_prefs(GtkChart * chart, gdouble rate, gchar *symbol);
void gtk_chart_set_currency(GtkChart * chart, guint32 kcur);

void gtk_chart_show_average(GtkChart * chart, gdouble value, gboolean visible);
void gtk_chart_set_overdrawn(GtkChart * chart, gdouble minimum);

void gtk_chart_show_xval(GtkChart * chart, gboolean visible);
void gtk_chart_set_barw(GtkChart * chart, gdouble barw);
void gtk_chart_set_showmono(GtkChart * chart, gboolean mono);
void gtk_chart_set_smallfont(GtkChart * chart, gboolean small);

void gtk_chart_show_legend(GtkChart * chart, gboolean visible, gboolean showextracol);
void gtk_chart_show_overdrawn(GtkChart * chart, gboolean visible);
void gtk_chart_show_minor(GtkChart * chart, gboolean minor);
void gtk_chart_set_absolute(GtkChart * chart, gboolean abs);

G_END_DECLS
#endif /* __GTK_CHART_H__ */


