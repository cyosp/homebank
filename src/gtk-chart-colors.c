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

#include <gtk/gtk.h>

#include "gtk-chart-colors.h"


#define MYDEBUG 0


#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif



char *chart_colors[] =
{
	"HomeBank",
	"Money",
	"SAP",
	"Quicken",
	"Office 2010",
	"Office 2013",
	"Analytics",
	"YNAB",
	"Quicken 2017",
	"Mint",
	"Material",
	"Nord",
	NULL
};


struct rgbcol nord_colors[] =
{
	{ 191,  97, 106 },
	{ 235, 203, 139 },
	{ 163, 190, 140 },
	{  94, 129, 172 },
	{ 180, 142, 173 },
	{ 193, 158, 102 },	/* added*/
	{ 208, 135, 112 },
	{ 129, 161, 193 },
	{ 238, 187, 119 },	/* added */
	{ 143, 188, 187 },
	{ 150, 150, 150 }	//grey/other
};
int nord_nbcolors = G_N_ELEMENTS(nord_colors);


struct rgbcol material_colors[] =
{
	{ 244,  67,  54 },
	{  63,  81, 181 },
	{   0, 150, 136 },
	{ 255, 235,  59 },
	{ 233,  30,  99 },
	{  33, 150, 243 },
	{  76, 175,  80 },
	{ 255, 193,   7 },
	{ 156,  39, 176 },
	{   3, 169, 244 },
	{ 139, 195,  74 },
	{ 255, 152,   0 },
	{ 103,  58, 183 },
	{   0, 188, 212 },
	{ 205, 220,  57 },
	{ 255,  87,  34 },

	//{ 121, 85, 72 },
	//{ 158, 158, 158 },
	//{ 96, 125, 139 },

};
int material_nbcolors = G_N_ELEMENTS(material_colors);


struct rgbcol mint_colors[] =
{
	{ 255, 207,  64 },
	{ 255, 171,  64 },
	{  69, 202, 230 },
	{  22, 126, 230 },
	{ 241, 132, 212 },
	{ 242,  72,  72 },
	{ 166, 237,  94 },
	{  10, 198, 117 },
	{  24, 158, 124 },
	{ 188, 146, 251 },
	{ 122,  71, 215 },
	{ 255, 255, 139 },
	{ 159, 159, 159 }	//grey/other

};
int mint_nbcolors = G_N_ELEMENTS(mint_colors);


struct rgbcol quicken2017_colors[] =
{
	{  64, 191, 117 },
	{ 119, 131, 255 },
	{ 214, 137, 251 },
	{ 255, 156,  78 },
	{  28, 168, 221 },
	{ 255, 116, 162 },
	{  78, 223, 225 },
	{  85, 120, 195 },
	{ 182, 198,  71 },
	{ 255,  85, 108 },
	{ 113, 113, 113 }	//grey/other

};
int quicken2017_nbcolors = G_N_ELEMENTS(quicken2017_colors);



struct rgbcol ynab_colors[] =
{
	{ 234,  84,  57 },
	{ 243, 173,  81 },
	{ 235, 229, 152 },
	{ 116, 169, 230 },
	{ 225, 248, 129 },
	{ 139, 161,  87 },
	{ 145, 197, 180 },
	{   0, 157, 174 },
	{ 203, 219,  60 },
	{ 228, 211,  84 },
	{ 128, 133, 233 },
	{ 247, 163,  92 },
	{ 164, 192, 208 },
	{ 124, 181, 236 },
	{ 199, 246, 190 },

	{ 105, 106, 105 }	  //grey/other
};
int ynab_nbcolors = G_N_ELEMENTS(ynab_colors);


struct rgbcol money_colors[] =
{
	{ 255, 193,  96 },
	{  92, 131, 180 },
	{ 165,  88, 124 },
	{ 108, 124, 101 },
	{ 230, 121,  99 },
	{  91, 160, 154 },
	{ 207,  93,  96 },
	{  70, 136, 106 },

	{ 245, 163,  97 },
	{ 158, 153,  88 },
	{ 255, 140,  90 },
	{ 122, 151, 173 },
	{  84, 142, 128 },
	{ 185, 201, 149 },
	{ 165,  99, 103 },
	{  77, 140, 172 },

	{ 251, 228, 128 },
	{  73,  99, 149 },
	{ 192,  80,  77 },
	{ 139, 180, 103 },
	{ 132, 165, 214 },
	{ 221, 216, 115 },
	{  77, 103, 137 },
	{ 165, 181, 156 }
	
	//grey/other
};
int money_nbcolors = G_N_ELEMENTS(money_colors);


struct rgbcol quicken_colors[] =
{
	{ 226,  73,  13 },
	{ 223, 180,   6 },
	{ 124, 179,   0 },
	{  44, 108, 182 },
	{ 184,  81, 186 },
	{ 165, 165, 165 },
	{ 122, 122, 122 },
	{ 137,  42,  40 },

	{  70, 161, 100 },
	{ 220, 106,   0 },
	{ 113, 113, 113 }	  //grey/other
};
int quicken_nbcolors = G_N_ELEMENTS(quicken_colors);


struct rgbcol analytics_colors[] =
{
	{   5, 141, 199 },	  //line color
	{  80, 180,  50 },
	{ 237,  86,  27 },
	{ 237, 239,   0 },
	{  36, 203, 229 },
	{ 100, 229, 114 },
	{ 255, 150,  85 },
	{ 255, 242,  99 },

	{ 106, 249, 196 },
	{ 178, 222, 255 },
	{ 204, 204, 204 }	  //grey/other
};
int analytics_nbcolors = G_N_ELEMENTS(analytics_colors);


struct rgbcol office2010_colors[] =
{
	{  60, 100, 149 },
	{ 150,  60,  59 },
	{ 120, 147,  68 },
	{  99,  75, 123 },
	{  61, 133, 157 },
	{ 196, 115,  49 },

	{  73, 120, 176 },
	{ 179,  74,  71 },
	{ 144, 178,  84 },
	{ 117,  93, 153 },
	{  73, 161, 185 },
	{ 232, 140,  65 },

	{ 126, 155, 199 },
	{ 202, 126, 126 },
	{ 174, 197, 129 },
	{ 156, 137, 182 },
	{ 123, 185, 206 },
	{ 248, 170, 121 }
	//grey/other
};
int office2010_nbcolors = G_N_ELEMENTS(office2010_colors);


struct rgbcol office2013_colors[] =
{
	{  91, 155, 213  },
	{ 237, 125,  49  },
	{ 165, 165, 165  },
	{ 255, 192,   0  },
	{  68, 114, 196  },
	{ 112, 173,  71  },

	{  37,  94, 145  },
	{ 158,  72,  14  },
	{  99,  99,  99  },
	{ 153, 115,   0  },
	{  38,  68, 120  },
	{  67, 104,  43  },

	{ 124, 175, 221  },
	{ 241, 151,  90  },
	{ 183, 183, 183  },
	{ 255, 205,  51  },
	{ 105, 142, 208  },
	{ 140, 193, 104  }
	//grey/other
};
int office2013_nbcolors = G_N_ELEMENTS(office2013_colors);

struct rgbcol sap_colors[] =
{
	{ 107, 148, 181  },
	{ 239, 205, 120  },
	{ 160, 117, 146  },
	{ 107, 181, 144  },
	{ 237, 164, 112  },
	{ 107, 106, 161  },
	{ 183, 213, 104  },
	{ 214, 128, 118  },

	{ 135, 115, 161  },
	{ 218, 217,  86  },
	{ 207, 111, 122  },
	{  85, 168, 161  },
	{ 253, 213,  65  },
	{ 146,  98, 148  },
	{ 115, 192,  59  },
	{ 205,  81,  96  },

	{  53, 180, 201  },
	{ 248, 175, 103  },
	{ 186,  97, 125  },
	{ 117, 202, 249  },
	{ 244, 131,  35  },
	{ 178,  45, 110  },
	{  87, 229, 151  },
	{ 204, 171,  68  },

	{ 172, 110, 145  },
	{  61, 132, 137  },
	{ 224, 117,  79  },
	{ 117,  84, 148  },
	{ 155, 206, 158  },
	{ 255, 133, 100  },
	{  60,  98, 153  },
	{ 128, 197, 122  }
};
int sap_nbcolors = G_N_ELEMENTS(sap_colors);

struct rgbcol homebank_colors[] =
{
	{  72, 118, 176  },
	{ 180, 198, 230  },
	{ 227, 126,  35  },
	{ 238, 186, 123  },
	{  97, 158,  58  },
	{ 175, 222, 142  },
	{ 184,  43,  44  },
	{ 231, 151, 149  },
	{ 136, 103, 185  },
	{ 190, 174, 210  },
	{ 127,  87,  77  },
	{ 184, 155, 147  },
	{ 202, 118, 190  },
	{ 230, 181, 208  },
	{ 126, 126, 126  },
	{ 198, 198, 198  },
	{ 187, 188,  56  },
	{ 218, 218, 144  },
	{ 109, 189, 205  },
	{ 176, 217, 228  },

	{ 237, 212,   0  },
	{ 255, 239, 101  },
	{ 207,  93,  96  },
	{ 234, 186, 187  },
	{ 193, 124,  17  },
	{ 240, 181,  90  },
	{ 186, 189, 182  },
	{ 225, 227, 223  },
	{ 115, 210,  22  },
	{ 175, 240, 112  },
	{ 255, 140,  90  },
	{ 255, 191, 165  }

};
int homebank_nbcolors = G_N_ELEMENTS(homebank_colors);



struct rgbcol global_colors[] =
{
	{  0,   0,   0},	// black
	{255, 255, 255},	// white
	{239, 239, 239},	// grey1	THTEXT 0.05
	{ 68,  68,  68},	// text		THTEXT 0.78
	{ 51,  51,  51},	// xyline   THTEXT 0.8


/*	{  255,   0,   0},	// fake
	{  255,   255,   0},	// fake
	{  255,   0,   255},	// fake
	{  0,   255,   0},	// fake
	{  0,   0,   255},	// fake
*/

	
	{255, 255, 255},	// theme base (bg)
	{ 46,  52,  54},	// theme fg
};



/*
struct rgbcol global_colors[] =
{
	{  0,   0,   0},	// black
	{255, 255, 255},	// white
	{238, 238, 238},	// #top/bottom lines
	{204, 204, 204},	// #dotted lines
	{102, 102, 102},	// #x-axis, scale text
	{153, 153, 153},	// # ??
	{  0, 119, 204},	// #line color

	//new
	{239, 239, 239},	// intermediate lines
	{ 68,  68,  68},	// text
	{ 51,  51,  51},	// x/y axis


};*/


void chart_color_global_default(void)
{
struct rgbcol *tcol;

	// set base color (adwaita)
	tcol = &global_colors[THBASE];
	tcol->r = 255;
	tcol->g = 255;
	tcol->b = 255;

	// set text(bg) color (adwaita)
	tcol = &global_colors[THTEXT];
	tcol->r = 46;
	tcol->g = 52;
	tcol->b = 54;
}


void cairo_user_set_rgbcol(cairo_t *cr, struct rgbcol *col)
{
	cairo_set_source_rgb(cr, COLTOCAIRO(col->r), COLTOCAIRO(col->g), COLTOCAIRO(col->b));
}


void cairo_user_set_rgbacol(cairo_t *cr, struct rgbcol *col, double alpha)
{
	cairo_set_source_rgba(cr, COLTOCAIRO(col->r), COLTOCAIRO(col->g), COLTOCAIRO(col->b), alpha);
}


void cairo_user_set_rgbacol_over(cairo_t *cr, struct rgbcol *col, gboolean over, double alpha)
{
	if( over )
		cairo_set_source_rgba(cr, COLTOCAIROOVER(col->r), COLTOCAIROOVER(col->g), COLTOCAIROOVER(col->b), alpha);
	else
		cairo_set_source_rgba(cr, COLTOCAIRO(col->r), COLTOCAIRO(col->g), COLTOCAIRO(col->b), alpha);
}


void cairo_user_set_rgbcol_over(cairo_t *cr, struct rgbcol *col, gboolean over)
{
	if( over )
		cairo_set_source_rgb(cr, COLTOCAIROOVER(col->r), COLTOCAIROOVER(col->g), COLTOCAIROOVER(col->b));
	else
		cairo_set_source_rgb(cr, COLTOCAIRO(col->r), COLTOCAIRO(col->g), COLTOCAIRO(col->b));
}


void colorsheme_col8_to_rgba(struct rgbcol *col8, GdkRGBA *rgba)
{

	DB( g_print(" set %d %d %d\n", col8->r, col8->g, col8->b) );

	rgba->red   = COLTOCAIRO((double)col8->r);
	rgba->green = COLTOCAIRO((double)col8->g);
	rgba->blue  = COLTOCAIRO((double)col8->b);
	rgba->alpha = 1.0;
	DB( g_print(" set %f %f %f\n", rgba->red, rgba->green, rgba->blue) );

}



void colorscheme_init(GtkColorScheme *scheme, gint index)
{

	DB( g_print("\n[chart] scheme init\n") );

	scheme->cs_blue = 0;

	switch(index)
	{
		default:
		case CHART_COLMAP_HOMEBANK:
			scheme->colors = homebank_colors;
			scheme->nb_cols = homebank_nbcolors;
			scheme->cs_green = 4;
			scheme->cs_red = 6;
			scheme->cs_orange = 2;
			break;
		case CHART_COLMAP_MSMONEY:
			scheme->colors = money_colors;
			scheme->nb_cols = money_nbcolors;
			scheme->cs_blue = 17;
			scheme->cs_green = 19;
			scheme->cs_red = 18;
			scheme->cs_orange = 8;
			break;
		case CHART_COLMAP_QUICKEN:
			scheme->colors = quicken_colors;
			scheme->nb_cols = quicken_nbcolors;
			scheme->cs_blue = 3;
			scheme->cs_green = 2;
			scheme->cs_red = 0;
			scheme->cs_orange = 9;
			break;
		case CHART_COLMAP_ANALYTICS:
			scheme->colors = analytics_colors;
			scheme->nb_cols = analytics_nbcolors;
			scheme->cs_green = 1;
			scheme->cs_red = 2;
			scheme->cs_orange = 6;
			break;
		case CHART_COLMAP_OFFICE2010:
			scheme->colors = office2010_colors;
			scheme->nb_cols = office2010_nbcolors;
			scheme->cs_green = 2;
			scheme->cs_red = 1;
			scheme->cs_orange = 5;
			break;
		case CHART_COLMAP_OFFICE2013:
			scheme->colors = office2013_colors;
			scheme->nb_cols = office2013_nbcolors;
			scheme->cs_green = 5;
			scheme->cs_red = 1;
			scheme->cs_orange = 1;
			break;
		case CHART_COLMAP_SAP:
			scheme->colors = sap_colors;
			scheme->nb_cols = sap_nbcolors;
			scheme->cs_green = 14;
			scheme->cs_red = 15;
			scheme->cs_orange = 20;
			break;
		case CHART_COLMAP_YNAB:
			scheme->colors = ynab_colors;
			scheme->nb_cols = ynab_nbcolors;
			scheme->cs_blue = 3;
			scheme->cs_green = 5;
			scheme->cs_red = 0;
			scheme->cs_orange = 1;
			break;
		case CHART_COLMAP_QUICKEN2017:
			scheme->colors = quicken2017_colors;
			scheme->nb_cols = quicken2017_nbcolors;
			scheme->cs_blue = 7;
			scheme->cs_green = 0;
			scheme->cs_red = 9;
			scheme->cs_orange = 3;
			break;
		case CHART_COLMAP_MINT:
			scheme->colors = mint_colors;
			scheme->nb_cols = mint_nbcolors;
			scheme->cs_blue = 3;
			scheme->cs_green = 7;
			scheme->cs_red = 5;
			scheme->cs_orange = 1;
			break;
		case CHART_COLMAP_MATERIAL:
			scheme->colors = material_colors;
			scheme->nb_cols = material_nbcolors;
			scheme->cs_blue = 5;
			scheme->cs_green = 6;
			scheme->cs_red = 0;
			scheme->cs_orange = 7;
			break;
		case CHART_COLMAP_NORD:
			scheme->colors = nord_colors;
			scheme->nb_cols = nord_nbcolors;
			scheme->cs_blue = 3;
			scheme->cs_green = 2;
			scheme->cs_red = 0;
			scheme->cs_orange = 6;
			break;
	}


}

