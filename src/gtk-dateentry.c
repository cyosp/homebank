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


//#include <time.h>
#include <stdlib.h>		/* atoi, atof, atol */
#include <libintl.h>	/* gettext */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gtk-dateentry.h"

//TODO: move this after GTK4
#include "ui-widgets.h"


#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif


enum {
  CHANGED,
  LAST_SIGNAL
};


enum {
	PROPERTY_DATE = 5,
};


static guint dateentry_signals[LAST_SIGNAL] = {0,};


//G_DEFINE_TYPE(GtkDateEntry, gtk_date_entry, GTK_TYPE_BOX)
G_DEFINE_TYPE_WITH_CODE(GtkDateEntry, gtk_date_entry, GTK_TYPE_BOX, G_ADD_PRIVATE (GtkDateEntry))



gboolean using_twodigit_years = FALSE;

/* order of these in the current locale
** https://calendars.fandom.com/wiki/Date_format_by_country
** YMD: hungary
** DMY: united-kingdom
** MDY: united-states
*/
static GDateDMY dmy_order[3] = 
{
   G_DATE_DAY, G_DATE_MONTH, G_DATE_YEAR
};

struct _GDateParseTokens {
  gint num_ints;
  gint n[3];
};

typedef struct _GDateParseTokens GDateParseTokens;

#define NUM_LEN 10


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

//https://en.wikipedia.org/wiki/Date_format_by_country

static void
hb_date_fill_parse_tokens (const gchar *str, GDateParseTokens *pt)
{
gchar num[4][NUM_LEN+1];
gint i;
const guchar *s;

	//DB( g_print("\n[dateentry] fill parse token\n") );

	/* We count 4, but store 3; so we can give an error
	* if there are 4.
	*/
	num[0][0] = num[1][0] = num[2][0] = num[3][0] = '\0';

	s = (const guchar *) str;
	pt->num_ints = 0;
	while (*s && pt->num_ints < 4) 
	{
		i = 0;
		while (*s && g_ascii_isdigit (*s) && i < NUM_LEN)
		{
			num[pt->num_ints][i] = *s;
			++s; 
			++i;
		}

		if (i > 0) 
		{
			num[pt->num_ints][i] = '\0';
			++(pt->num_ints);
		}

		if (*s == '\0') break;

		++s;
	}

	pt->n[0] = pt->num_ints > 0 ? atoi (num[0]) : 0;
	pt->n[1] = pt->num_ints > 1 ? atoi (num[1]) : 0;
	pt->n[2] = pt->num_ints > 2 ? atoi (num[2]) : 0;

}


static void
hb_date_determine_dmy_order(void)
{
GDateParseTokens testpt;
GDate d;
gchar buf[127];
gint i;

	DB( g_print("\n[dateentry] determine dmy order\n") );

	/* had to pick a random day - don't change this, some strftimes
	* are broken on some days, and this one is good so far. */
	g_date_set_dmy (&d, 4, 7, 1976);

	g_date_strftime (buf, 127, "%x", &d);

	hb_date_fill_parse_tokens (buf, &testpt);

	using_twodigit_years = FALSE;
	dmy_order[0] = G_DATE_DAY;
	dmy_order[1] = G_DATE_MONTH;
	dmy_order[2] = G_DATE_YEAR;

	i = 0;
	while (i < testpt.num_ints)
	{
		switch (testpt.n[i])
		{
		case 7:
			dmy_order[i] = G_DATE_MONTH;
			break;
		case 4:
			dmy_order[i] = G_DATE_DAY;
			break;
		case 76:
			using_twodigit_years = TRUE; /* FALL THRU */
		case 1976:
			dmy_order[i] = G_DATE_YEAR;
		break;
		}
		++i;
	}

	DB( g_print(" => dmy=%d mdy=%d ymd=%d\n", dmy_order[0]==G_DATE_DAY, dmy_order[0]==G_DATE_MONTH, dmy_order[0]==G_DATE_YEAR) );

}


static void
hb_date_parse_token_reorder(GDateParseTokens *pt, GDateDay *nday, GDateMonth *nmonth, GDateYear *nyear)
{
	//DMY
	if( dmy_order[0] == G_DATE_DAY )
	{
		*nday   = pt->n[0];
		*nmonth = pt->n[1];
		*nyear  = pt->n[2];
	}
	else
	//MDY
	if( dmy_order[0] == G_DATE_MONTH )
	{
		*nmonth = pt->n[0];
		*nday   = pt->n[1];
		*nyear  = pt->n[2];
	}
	else
	//YMD
	if( dmy_order[0] == G_DATE_YEAR )
	{
		*nyear  = pt->n[0];
		*nmonth = pt->n[1];
		*nday   = pt->n[2];
	}

	DB( g_print(" token converted dmy: %02d %02d %04d\n", *nday, *nmonth, *nyear) );

}


static void 
hb_date_parse_tokens(GDate *date, const gchar *str)
{
GDateParseTokens pt;
GDate tokendate;
GDateDay nday;
GDateMonth nmonth;
GDateYear nyear;

	// initialize with today's date
	g_date_set_time_t(&tokendate, time(NULL));
	nday   = g_date_get_day(&tokendate);
	nmonth = g_date_get_month(&tokendate);
	nyear  = g_date_get_year(&tokendate);

	hb_date_fill_parse_tokens(str, &pt);
	DB( g_print(" token raw %d values: %d %d %d\n", pt.num_ints, pt.n[0], pt.n[1], pt.n[2]) );

	// adjust necessary values
	if( pt.num_ints == 3 )
	{
		hb_date_parse_token_reorder(&pt, &nday, &nmonth, &nyear);

		//adjust 2digits year: windowing 40:60
		if( nyear < 100 )
		{
		GDateYear thisyear = g_date_get_year(&tokendate);
		GDateYear millenium = (gint)(thisyear/100) * 100;

			if( nyear <= (( thisyear % 100 ) + 60 ) )
				nyear += millenium;
			else
				nyear += (millenium - 100);
		
			DB( g_print(" token fixed year: %04d\n", nyear) );
		}

		g_date_set_day(&tokendate, nday);
		g_date_set_month(&tokendate, nmonth);
		g_date_set_year(&tokendate, nyear);
	}
	else
	//user input day/month or month/day
	if( pt.num_ints == 2 )
	{
		hb_date_parse_token_reorder(&pt, &nday, &nmonth, &nyear);

		g_date_set_day(&tokendate, nday);
		g_date_set_month(&tokendate, nmonth);
	}
	else
	//user input day
	if( pt.num_ints == 1 )
	{
		nday = pt.n[0];
		g_date_set_day(&tokendate, nday);
	}

	//update output date if tokendate is valid 
	if( g_date_valid(&tokendate) )
	{
		g_date_set_julian(date, g_date_get_julian(&tokendate));
	}
	else
		g_date_clear(date, 1);
}


static void
update_text(GtkDateEntry *self)
{
GtkDateEntryPrivate *priv = self->priv;
gchar label[127];

	DB( g_print("\n[dateentry] beg update text\n") );

	//%x : The preferred date representation for the current locale without the time.
	//5.7 added %a to display abbreviated weekday
	g_date_strftime (label, 127 - 1, "%a %x", priv->date);
	gtk_entry_set_text (GTK_ENTRY (priv->entry), label);
	DB( g_print(" = %s\n", label) );
	
	DB( g_print("\n[dateentry] end update text\n") );
}


static void
eval_date(GtkDateEntry *self)
{
GtkDateEntryPrivate *priv = self->priv;

	DB( g_print("\n[dateentry] beg eval date\n") );

	DB( g_print(" pre %02d %02d %04d\n", g_date_get_day(priv->date), g_date_get_month(priv->date), g_date_get_year(priv->date)) );

	g_date_clamp(priv->date, &priv->mindate, &priv->maxdate);
	//DB( g_print(" min %02d %02d %04d\n", g_date_get_day(&priv->mindate), g_date_get_month(&priv->mindate), g_date_get_year(&priv->mindate)) );
	//DB( g_print(" max %02d %02d %04d\n", g_date_get_day(&priv->maxdate), g_date_get_month(&priv->maxdate), g_date_get_year(&priv->maxdate)) );

	DB( g_print(" post %02d %02d %04d\n", g_date_get_day(priv->date), g_date_get_month(priv->date), g_date_get_year(priv->date)) );

	update_text(self);
	
	if(priv->lastdate != g_date_get_julian(priv->date))
	{
		DB( g_print(" **emit 'changed' signal**\n") );
		g_signal_emit_by_name (self, "changed", NULL, NULL);
	}

	priv->lastdate = g_date_get_julian(priv->date);

	DB( g_print("\n[dateentry] end eval date\n") );
}


static void
parse_date(GtkDateEntry *self)
{
GtkDateEntryPrivate *priv = self->priv;
const gchar *str;

	DB( g_print("\n[dateentry] parse date\n") );

	str = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	DB( g_print(" inputstr='%s'\n", str) );

	//1) my parse of d, dm, md, dmy, mdy, ymd 
	hb_date_parse_tokens(priv->date, str);

	//2) invalid: glib failover
	if(!g_date_valid(priv->date))
	{
		g_date_set_parse (priv->date, str);
	}

	//3) invalid: warn user put today's
	if(!g_date_valid(priv->date))
	{
		g_date_set_time_t(priv->date, time(NULL));
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(priv->entry)), GTK_STYLE_CLASS_WARNING);
	}
	else
	{
		gtk_style_context_remove_class (gtk_widget_get_style_context (GTK_WIDGET(priv->entry)), GTK_STYLE_CLASS_WARNING);
	}

	DB( g_print(" date is d:%02d m:%02d y:%04d\n", g_date_get_day(priv->date), g_date_get_month(priv->date), g_date_get_year(priv->date)) );

	eval_date(self);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
gtk_date_entry_set_calendar (GtkWidget * widget, GtkDateEntry * dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;
guint day, month;

	DB( g_print("\n[dateentry] set calendar\n") );

	/* GtkCalendar expects month to be in 0-11 range (inclusive) */
	day   = g_date_get_day (priv->date);
	month = g_date_get_month (priv->date) - 1;

	g_signal_handler_block(priv->calendar, priv->hid_dayselect);

	gtk_calendar_select_month (GTK_CALENDAR (priv->calendar),
			   month,
			   g_date_get_year (priv->date));
    gtk_calendar_select_day (GTK_CALENDAR (priv->calendar), day);
			 
	g_signal_handler_unblock(priv->calendar, priv->hid_dayselect);
}




static void
gtk_date_entry_cb_today_clicked (GtkWidget * widget, GtkDateEntry * dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;

	DB( g_print("\n[dateentry] today_clicked\n") );
	
	//revert to now (today)
	g_date_set_time_t(priv->date, time(NULL));
	eval_date(dateentry);

	gtk_date_entry_set_calendar(widget, dateentry);

	//1923368 keep the popover visible
	//gtk_widget_hide (priv->popover);
}


static void
gtk_date_entry_cb_calendar_day_selected(GtkWidget * calendar, GtkDateEntry * dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;
guint year, month, day;

	DB( g_print("\n[dateentry] calendar_day_selected\n") );

	gtk_calendar_get_date (GTK_CALENDAR (priv->calendar), &year, &month, &day);
	g_date_set_dmy (priv->date, day, month + 1, year);
	eval_date(dateentry);	
}


static gint
gtk_date_entry_cb_calendar_day_select_double_click(GtkWidget * calendar, gpointer user_data)
{
GtkDateEntry *dateentry = user_data;
GtkDateEntryPrivate *priv = dateentry->priv;

	DB( g_print("\n[dateentry] calendar_day_select_double_click\n") );

	gtk_widget_hide (priv->popover);	

	return FALSE;
}


static void
gtk_date_entry_cb_calendar_today_mark(GtkWidget *calendar, GtkDateEntry *dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;
guint year, month, day;

	DB( g_print("\n[dateentry] cb_calendar_mark_day\n") );

	gtk_calendar_get_date (GTK_CALENDAR (priv->calendar), &year, &month, &day);

	//maybe 1828914
	gtk_calendar_clear_marks(GTK_CALENDAR(priv->calendar));
	if( year == g_date_get_year (&priv->nowdate) && month == (g_date_get_month (&priv->nowdate)-1) )	
		gtk_calendar_mark_day(GTK_CALENDAR(priv->calendar), g_date_get_day (&priv->nowdate));

}


static void 
gtk_date_entry_cb_calendar_monthyear(GtkWidget *calendar, GtkDateEntry *dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;
guint year, month, day;

	DB( g_print("\n[dateentry] cb_calendar_monthyear\n") );

	gtk_calendar_get_date (GTK_CALENDAR (priv->calendar), &year, &month, &day);
	if( year < 1900)
		g_object_set(calendar, "year", 1900, NULL);

	if( year > 2200)
		g_object_set(calendar, "year", 2200, NULL);

	gtk_date_entry_cb_calendar_today_mark(calendar, dateentry);
}


static gint
gtk_date_entry_cb_entry_key_pressed (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
GtkDateEntry *dateentry = user_data;
GtkDateEntryPrivate *priv = dateentry->priv;
guint action;

	DB( g_print("\n[dateentry] entry key pressed") );


	DB( g_print(" state: %s %s\n", (event->state & GDK_SHIFT_MASK) ? "shift" : "", (event->state & GDK_CONTROL_MASK) ? "ctrl" : "" ) );
	DB( g_print(" kyval: %s %s\n", (event->keyval == GDK_KEY_Up) ? "up" : "",  (event->keyval == GDK_KEY_Down) ? "down" : "") );

	if( (event->type != GDK_KEY_PRESS) )
		return FALSE;

	//#1873643 preserve Up/Down (+ctrl) natural GTK focus change
	if( (event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK) )
		return FALSE;

	if( (event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_Down) )
	{
		//let's bitwise key to an action-id
		action = 0;
		action |= (event->state & GDK_SHIFT_MASK)   ? 2 : 0;
		action |= (event->state & GDK_CONTROL_MASK) ? 4 : 0;
		action |= (event->keyval == GDK_KEY_Down)   ? 1 : 0;

		DB( g_print(" action: %d\n", action) );

		switch(action)
		{
			case 0: g_date_add_days (priv->date, 1); break;
			case 1: g_date_subtract_days (priv->date, 1); break;
			case 2:	g_date_add_months (priv->date, 1); break;
			case 3:	g_date_subtract_months (priv->date, 1); break;
			case 6:	g_date_add_years (priv->date, 1); break;
			case 7:	g_date_subtract_years (priv->date, 1); break;
		}

		eval_date(dateentry);

		//stop handlers
		return TRUE;
	}
	//propagate
	return FALSE;
}


static void
gtk_date_entry_cb_entry_activate(GtkWidget *gtkentry, gpointer user_data)
{
GtkDateEntry *dateentry = user_data;

	DB( g_print("\n[dateentry] entry_activate\n") );

	parse_date(dateentry);
	eval_date(dateentry);
}


static gboolean 
gtk_date_entry_cb_entry_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
GtkDateEntry *dateentry = user_data;

	DB( g_print("\n[dateentry] entry focus-out-event %d\n", gtk_widget_is_focus(GTK_WIDGET(dateentry))) );

	parse_date(dateentry);
	eval_date(dateentry);
	return FALSE;
}


static void
gtk_date_entry_cb_button_clicked (GtkWidget * widget, GtkDateEntry * dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;

	DB( g_print("\n[dateentry] button_clicked\n") );

	gtk_date_entry_set_calendar(widget, dateentry);

	gtk_popover_set_relative_to (GTK_POPOVER (priv->popover), GTK_WIDGET (priv->entry));
	//gtk_widget_get_clip(priv->arrow, &rect);
	//gtk_popover_set_pointing_to (GTK_POPOVER (priv->popover), &rect);

	gtk_date_entry_cb_calendar_today_mark(widget, dateentry);
	
	gtk_widget_show_all (priv->popover);
}


static void 
gtk_date_entry_destroy (GtkWidget *object)
{
GtkDateEntry *dateentry = GTK_DATE_ENTRY (object);
GtkDateEntryPrivate *priv = dateentry->priv;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GTK_IS_DATE_ENTRY(object));

	DB( g_print("\n[dateentry] destroy\n") );

	DB( g_print(" free gtkentry: %p\n", priv->entry) );
	DB( g_print(" free arrow: %p\n", priv->button) );

	DB( g_print(" free dateentry: %p\n", dateentry) );

	if(priv->date)
		g_date_free(priv->date);
	priv->date = NULL;

	GTK_WIDGET_CLASS (gtk_date_entry_parent_class)->destroy (object);
}



static void
gtk_date_entry_dispose (GObject *gobject)
{
//GtkDateEntry *self = GTK_DATE_ENTRY (gobject);

	DB( g_print("\n[dateentry] dispose\n") );

	
  //g_clear_object (&self->priv->an_object);

  G_OBJECT_CLASS (gtk_date_entry_parent_class)->dispose (gobject);
}




static void
gtk_date_entry_finalize (GObject *gobject)
{
//GtkDateEntry *self = GTK_DATE_ENTRY (gobject);

	DB( g_print("\n[dateentry] finalize\n") );

	
	//g_date_free(self->date);
  //g_free (self->priv->a_string);

  /* Always chain up to the parent class; as with dispose(), finalize()
   * is guaranteed to exist on the parent's class virtual function table
   */
  G_OBJECT_CLASS(gtk_date_entry_parent_class)->finalize (gobject);
}



static void
gtk_date_entry_class_init (GtkDateEntryClass *class)
{
GObjectClass *object_class;
GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (class);
	widget_class = GTK_WIDGET_CLASS (class);

	DB( g_print("\n[dateentry] class_init\n") );

	//object_class->constructor = gtk_date_entry_constructor;
	//object_class->set_property = gtk_date_entry_set_property;
	//object_class->get_property = gtk_date_entry_get_property;
	object_class->dispose  = gtk_date_entry_dispose;
	object_class->finalize = gtk_date_entry_finalize;

	widget_class->destroy  = gtk_date_entry_destroy;
	
	dateentry_signals[CHANGED] =
		g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkDateEntryClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

	//g_type_class_add_private (object_class, sizeof (GtkDateEntryPrivate));
	
}

static void
gtk_date_entry_init (GtkDateEntry *dateentry)
{
GtkDateEntryPrivate *priv;
GtkWidget *vbox;

	DB( g_print("\n[dateentry] init\n") );

	/* yes, also priv, need to keep the code readable */
	/*dateentry->priv = G_TYPE_INSTANCE_GET_PRIVATE (dateentry,
                                                  GTK_TYPE_DATE_ENTRY,
                                                  GtkDateEntryPrivate);*/

	dateentry->priv = gtk_date_entry_get_instance_private(dateentry);
	
	priv = dateentry->priv;

	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(dateentry)), GTK_STYLE_CLASS_LINKED);

	priv->entry = gtk_entry_new ();
	//todo: see if really useful
	gtk_entry_set_width_chars(GTK_ENTRY(priv->entry), 16);
	gtk_entry_set_max_width_chars(GTK_ENTRY(priv->entry), 16);
	gtk_box_pack_start (GTK_BOX (dateentry), priv->entry, TRUE, TRUE, 0);

	priv->button = gtk_button_new ();
	priv->arrow = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(priv->button), priv->arrow);
	gtk_box_pack_end (GTK_BOX (dateentry), priv->button, FALSE, FALSE, 0);

	priv->popover = gtk_popover_new (priv->button);
	gtk_popover_set_position(GTK_POPOVER(priv->popover), GTK_POS_BOTTOM);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_popover_set_child (GTK_POPOVER(priv->popover), vbox);

	gtk_widget_set_margin_start (vbox, 10);
	gtk_widget_set_margin_end (vbox, 10);
	gtk_widget_set_margin_top (vbox, 10);
	gtk_widget_set_margin_bottom (vbox, 10);

	priv->calendar = gtk_calendar_new ();
	gtk_box_pack_start(GTK_BOX(vbox), priv->calendar, FALSE, FALSE, 0);

	priv->BT_today = gtk_button_new_with_mnemonic ( gettext("_Today"));
	gtk_box_pack_start(GTK_BOX(vbox), priv->BT_today, FALSE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET(dateentry));

	/* initialize datas */
	priv->date = g_date_new();
	g_date_set_time_t(priv->date, time(NULL));
	g_date_set_time_t(&priv->nowdate, time(NULL));
	g_date_set_dmy(&priv->mindate,  1,  1, 1900);	//693596
	g_date_set_dmy(&priv->maxdate, 31, 12, 2200);	//803533
	update_text(dateentry);


	g_signal_connect (priv->entry, "key-press-event",
				G_CALLBACK (gtk_date_entry_cb_entry_key_pressed), dateentry);

	g_signal_connect_after (priv->entry, "focus-out-event",
				G_CALLBACK (gtk_date_entry_cb_entry_focus_out), dateentry);

	g_signal_connect (priv->entry, "activate",
				G_CALLBACK (gtk_date_entry_cb_entry_activate), dateentry);


	g_signal_connect (priv->button, "clicked",
				G_CALLBACK (gtk_date_entry_cb_button_clicked), dateentry);


	g_signal_connect (priv->calendar, "prev-year",
				G_CALLBACK (gtk_date_entry_cb_calendar_monthyear), dateentry);
	g_signal_connect (priv->calendar, "next-year",
				G_CALLBACK (gtk_date_entry_cb_calendar_monthyear), dateentry);
	g_signal_connect (priv->calendar, "prev-month",
				G_CALLBACK (gtk_date_entry_cb_calendar_monthyear), dateentry);
	g_signal_connect (priv->calendar, "next-month",
				G_CALLBACK (gtk_date_entry_cb_calendar_monthyear), dateentry);

	priv->hid_dayselect = g_signal_connect (priv->calendar, "day-selected",
				G_CALLBACK (gtk_date_entry_cb_calendar_day_selected), dateentry);

	g_signal_connect (priv->calendar, "day-selected-double-click",
				G_CALLBACK (gtk_date_entry_cb_calendar_day_select_double_click), dateentry);

	g_signal_connect (priv->BT_today, "clicked",
				G_CALLBACK (gtk_date_entry_cb_today_clicked), dateentry);

}


GtkWidget *
gtk_date_entry_new (GtkWidget *label)
{
GtkDateEntry *dateentry;

	DB( g_print("\n[dateentry] new\n") );

	dateentry = g_object_new (GTK_TYPE_DATE_ENTRY, NULL);

	if(dateentry)
	{
	GtkDateEntryPrivate *priv = dateentry->priv;
	
		if(label)
			gtk_label_set_mnemonic_widget (GTK_LABEL(label), priv->entry);

		hb_date_determine_dmy_order();

	}
	return GTK_WIDGET(dateentry);
}


/*
**
*/
void 
gtk_date_entry_set_mindate(GtkDateEntry *dateentry, guint32 julian_days)
{
GtkDateEntryPrivate *priv = dateentry->priv;
	
	DB( g_print("\n[dateentry] set mindate\n") );

	g_return_if_fail (GTK_IS_DATE_ENTRY (dateentry));

	if(g_date_valid_julian(julian_days))
	{
		g_date_set_julian (&priv->mindate, julian_days);
	}
}


/*
**
*/
void 
gtk_date_entry_set_maxdate(GtkDateEntry *dateentry, guint32 julian_days)
{
GtkDateEntryPrivate *priv = dateentry->priv;
	
	DB( g_print("\n[dateentry] set maxdate\n") );

	g_return_if_fail (GTK_IS_DATE_ENTRY (dateentry));

	if(g_date_valid_julian(julian_days))
	{
		g_date_set_julian (&priv->maxdate, julian_days);
	}
}


/*
**
*/
void
gtk_date_entry_set_date(GtkDateEntry *dateentry, guint32 julian_days)
{
GtkDateEntryPrivate *priv = dateentry->priv;

	DB( g_print("\n[dateentry] set date\n") );

	g_return_if_fail (GTK_IS_DATE_ENTRY (dateentry));

	if(g_date_valid_julian(julian_days))
	{
		g_date_set_julian (priv->date, julian_days);
	}
	else
	{
		g_date_set_time_t(priv->date, time(NULL));
	}
	eval_date(dateentry);
}


/*
**
*/
guint32
gtk_date_entry_get_date(GtkDateEntry *dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;
	
	DB( g_print("\n[dateentry] get date\n") );

	g_return_val_if_fail (GTK_IS_DATE_ENTRY (dateentry), 0);

	return(g_date_get_julian(priv->date));
}


GDateWeekday
gtk_date_entry_get_weekday(GtkDateEntry *dateentry)
{
GtkDateEntryPrivate *priv = dateentry->priv;
	
	DB( g_print("\n[dateentry] get weekday\n") );

	g_return_val_if_fail (GTK_IS_DATE_ENTRY (dateentry), 0);

	return(g_date_get_weekday(priv->date));
}


