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

#include "hbtk-decimalentry.h"
//#include "hb-currency.h"
//#include "hb-misc.h"

//TODO: move this after GTK4
//#include "ui-widgets.h"


/* = = = = = = = = = = = = = = = = */

#define DB(x)	//(x);
#define DBI(x)	//(x);


enum {
  VALUE_CHANGED,
  LAST_SIGNAL
};

static guint decimalentry_signals[LAST_SIGNAL] = {0,};

//G_DEFINE_TYPE(HbtkDecimalEntry, hbtk_decimal_entry, GTK_TYPE_BOX)
G_DEFINE_TYPE_WITH_CODE(HbtkDecimalEntry, hbtk_decimal_entry, GTK_TYPE_ENTRY, G_ADD_PRIVATE (HbtkDecimalEntry))


/* = = = = = = = = = = = = = = = = */


static const gdouble fac[9] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

static double
my_round(const gdouble d, guint digits)
{
gdouble out;

	digits = MIN(digits, 8);
	out = ((gint64) (d < 0 ? (d * fac[digits]) - 0.5 : (d * fac[digits]) + 0.5)) / fac[digits];
	return out;
}


static gboolean
my_is_decimal_str(const gchar* txt, guint digits)
{
const gchar *s;
guint idx = 0, dccnt=0, npcnt=0, dpcnt=0;
gboolean retval = TRUE;

	s = txt;
	while( *s )
	{
		if( (idx > 0 && (*s=='-' || *s=='+'))
		 || ( dccnt > 1)
		 || ( dpcnt > (digits-1))
		  )
		{
			retval = FALSE;
			break;
		}

		if( *s=='.' )
			dccnt++;

		//count number parts
		if( *s >= 0x30 && *s <= 0x39 )
		{
			if( dccnt == 0 )
				npcnt++;
			else
				dpcnt++;
		}

		s++; idx++;
	}

	return retval;
}


static gdouble
_amount_operation(gchar operator, gdouble prvval, gdouble curval)
{
gdouble outval = 0.0;

	switch( operator )
	{
		case '-': outval = prvval - curval; break;
		case '+': outval = prvval + curval; break;
		case '*': outval = prvval * curval; break;
		case '/':
			if( curval != 0.0 )
				outval = prvval / curval;
			break;
	}

	DB( g_print("compute: %g %c %g = %g\n", prvval, operator, curval, outval) );
	return outval;
}



static gdouble
_parse_amount_test (const gchar *string, guint digits, gboolean *isvalid, gboolean *iscalc)
{
gdouble newval, nxtval;
const gchar *s, *remainder;
gchar pc, operator;
gboolean tmpvalid = TRUE;
gboolean tmpcalc = FALSE;

	g_return_val_if_fail (string != NULL, 0.0);

	DB( g_print("\n[decimalentry] _parse_amount_test\n") );
	DB( g_print("rawtxt: '%s'\n", string) );

	newval = 0.0;
	pc = 0x20;
	operator = '?';
	s = remainder = string;
	while( *s )
	{
		// *=x2A +=x2B -=x2D /=x2F :: .=x2E ,=x2C
		if(	(*s=='.' && pc=='.' )	//forbid ..
		 ||	((*s=='/' || *s=='*') && (pc=='/' || pc=='*'))	//forbid // ** /* */ 
		 ||	((*s=='/' || *s=='*') && (pc=='+' || pc=='-'))	//forbid -/ -* +/ +*
 		)
		{
			tmpvalid = FALSE;
			goto abort;
		}

		if( pc >= 0x30 && pc <= 0x39 )
		{
			// reach an operator ?
			if(*s=='-' || *s=='+' || *s=='/' || *s=='*')
			{
			gchar *tmpbuf = g_strndup(remainder, (gsize)(s - remainder));

				tmpcalc = TRUE;
				tmpvalid = my_is_decimal_str(tmpbuf, digits);
				DB( g_print(" chknum: '%s' :: %d\n", tmpbuf, tmpvalid) );
				nxtval = g_strtod(tmpbuf, NULL);
				g_free(tmpbuf);
				if(!tmpvalid)
					goto abort;
				DB( g_print("nxtval: %g operator: '%c'\n", nxtval, operator) );
				if( operator == '?' )
					newval = nxtval;
				else
					newval = _amount_operation(operator, newval, nxtval);

				remainder = s + 1;
				operator = *s;
			}
		}
		pc = *s++;
	}

	if (*remainder)
	{
		//store rawnumber
		tmpvalid = my_is_decimal_str(remainder, digits);
		DB( g_print(" chknum: '%s' :: %d\n", remainder, tmpvalid) );
		if(!tmpvalid)
			goto abort;
		nxtval = g_strtod(remainder, NULL);
		DB( g_print("nxtval: %g operator: '%c'\n", nxtval, operator) );
		if( operator == '?' )
			newval = nxtval;
		else
			newval = _amount_operation(operator, newval, nxtval);
	}

abort:
	if( iscalc != NULL )
		*iscalc = tmpcalc;

	if( isvalid != NULL )
		*isvalid = tmpvalid;

	DB( g_print(" out > %g\n", newval) );
	return newval;
}




/* = = = = = = = = = = = = = = = = */



static void
hbtk_decimal_entry_default_output (HbtkDecimalEntry *decimalentry)
{
HbtkDecimalEntryPrivate *priv = decimalentry->priv;
gchar *buf;

	DB( g_print("--------\n[decimalentry] output (%d digits)\n", priv->digits) );

	g_signal_handler_block(G_OBJECT (priv->entry), priv->hid_insert);
	g_signal_handler_block(G_OBJECT (priv->entry), priv->hid_changed);

	buf = g_strdup_printf ("%0.*f", priv->digits, priv->value);
	DB( g_print(" replace with '%s'\n", buf) );		
	gtk_entry_set_text (GTK_ENTRY (priv->entry), buf);
	g_free (buf);

	g_signal_handler_unblock(G_OBJECT (priv->entry), priv->hid_changed);
	g_signal_handler_unblock(G_OBJECT (priv->entry), priv->hid_insert);
}


static void
hbtk_decimal_value_change (HbtkDecimalEntry *decimalentry, gdouble newval)
{
HbtkDecimalEntryPrivate *priv = decimalentry->priv;
gboolean doemit = FALSE;

	newval = my_round(newval, priv->digits);
	if(priv->value != newval)
		doemit = TRUE;

	priv->value = newval;
	hbtk_decimal_entry_default_output(decimalentry);

	if(doemit == TRUE)
	{
		DB( g_print("\n **emit 'value-changed' signal**\n") );
		g_signal_emit_by_name (decimalentry, "value-changed", NULL, NULL);
	}
}


static void
hbtk_decimal_validate (HbtkDecimalEntry *decimalentry)
{
HbtkDecimalEntryPrivate *priv = decimalentry->priv;
gdouble newval;
gboolean iscalc = FALSE;

	DB( g_print("\n[decimalentry] validate\n") );

	if( priv->valid == FALSE )
	{
		DB( g_print(" txt is invalid\n") );
		priv->value = 0;
		return;	
	}

	gchar *curtxt = (gchar *)gtk_entry_get_text(GTK_ENTRY(priv->entry));
	newval = _parse_amount_test(curtxt, priv->digits, NULL, &iscalc);

	//simple amount
	priv->forcedsign = FALSE;
	if(!iscalc)
	{
		DB( g_print(" simple amount\n") );
		//force sign 
		if( (*curtxt == '-') || (*curtxt == '+') )
		{
			DB( g_print(" force with sign\n") );
			priv->forcedsign = TRUE;
			if( (*curtxt == '-' && newval > 0) || (*curtxt == '+' && newval < 0) )
				newval = newval * -1;
		}
		//default sign
		/*else
		{
			DB( g_print(" force with privdata\n") );
			if( (priv->income == TRUE && newval < 0) || (priv->income == FALSE && newval > 0) )
				newval = newval * -1;
		}*/
	}

	hbtk_decimal_value_change(decimalentry, newval);
}


static void
hbtk_decimal_entry_insert_text_handler (GtkEntry *entry, gchar *nt, gint length, gint *position, gpointer data)
{
HbtkDecimalEntry *decimalentry = HBTK_DECIMAL_ENTRY(entry);
HbtkDecimalEntryPrivate *priv = decimalentry->priv;
GtkEditable *editable = GTK_EDITABLE(entry);

	DBI( g_print("\n[decimalentry] text-handler\n") );
	DBI( g_print(" len:%d pos:%d nt:'%s' 0x%x\n", length, *position, nt, *nt) );

	if( (length == 1) )
	{
		//replace , by .
		if(*nt == 0x2C)	
			*nt = 0x2E;
		if( (*nt < 0x2A) || (*nt > 0x39) )	//allow only: *+,-./0123456789
			goto stop;

		g_signal_handler_block(G_OBJECT (priv->entry), priv->hid_insert);
		gtk_editable_insert_text (editable, nt, length, position);
		g_signal_handler_unblock(G_OBJECT (priv->entry), priv->hid_insert);
	}
	else
	//sanitize pasted text
	//old: if( (*nt & 0xFF) >= 0x7F)	//disable utf-8
	{
	gchar *s, *d, *cleantxt;
	gsize cleanlen = 0;

		s = nt;
		d = cleantxt = g_strdup(nt);
		while( *s )
		{
			// *=x2A +=x2B ,=x2C .=x2E -=x2D /=x2F 
			//if( (*s >= 0x2A) && (*s <= 0x39) && (*s!=0x2C) )
			if( (*s >= 0x2A) && (*s <= 0x39) )
			{
				//replace , by .
				if( *s!=0x2C )
					*d++ = *s;
				else
					*d++ = 0x2E;	
				cleanlen++;
			}
			s++;
		}
		*s = 0;

		DBI( g_print(" raw: '%s' > cln: '%s'\n", nt, cleantxt) );

		g_signal_handler_block(G_OBJECT (priv->entry), priv->hid_insert);
		gtk_editable_insert_text (editable, cleantxt, cleanlen, position);
		g_signal_handler_unblock(G_OBJECT (priv->entry), priv->hid_insert);

		g_free(cleantxt);
	}

stop:
	g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");
}



static void
hbtk_decimal_entry_cb_changed(GtkWidget *widget, gpointer user_data)
{
HbtkDecimalEntry *decimalentry = HBTK_DECIMAL_ENTRY(widget);
HbtkDecimalEntryPrivate *priv = decimalentry->priv;
const gchar *curtxt;
gboolean isvalid;

	DB( g_print("\n[decimalentry] changed\n") );

	//check validity
	curtxt = gtk_entry_get_text(GTK_ENTRY(priv->entry));
	_parse_amount_test(curtxt, priv->digits, &isvalid, NULL);

	DBI( g_print(" check '%s' %d\n", curtxt, isvalid) );
	priv->valid = isvalid;

	//test error class
	if( !isvalid )
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(priv->entry)), GTK_STYLE_CLASS_ERROR);
	else
		gtk_style_context_remove_class (gtk_widget_get_style_context (GTK_WIDGET(priv->entry)), GTK_STYLE_CLASS_ERROR);
	
}


static void
hbtk_decimal_entry_cb_activate(GtkWidget *widget, gpointer user_data)
{
HbtkDecimalEntry *decimalentry = HBTK_DECIMAL_ENTRY(widget);

	DB( g_print("\n[decimalentry] entry_activate\n") );

	hbtk_decimal_validate(decimalentry);
}


static gboolean 
hbtk_decimal_entry_cb_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
HbtkDecimalEntry *decimalentry = HBTK_DECIMAL_ENTRY(widget);

	DB( g_print("\n[decimalentry] focus-out-event %d\n", gtk_widget_is_focus(GTK_WIDGET(decimalentry))) );

	hbtk_decimal_validate(decimalentry);

	return FALSE;
}


static gboolean 
hbtk_decimal_entry_cb_focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
HbtkDecimalEntry *decimalentry = HBTK_DECIMAL_ENTRY(widget);
HbtkDecimalEntryPrivate *priv = decimalentry->priv;

	DB( g_print("\n[decimalentry] focus-in-event %d\n", gtk_widget_is_focus(GTK_WIDGET(decimalentry))) );

	if( priv->valid == TRUE && priv->value == 0.0 )
	{
		g_signal_handler_block(G_OBJECT (priv->entry), priv->hid_insert);
		g_signal_handler_block(G_OBJECT (priv->entry), priv->hid_changed);

		gtk_editable_select_region(GTK_EDITABLE(priv->entry), 0, -1);
		gtk_entry_set_text (GTK_ENTRY (priv->entry), "");	

		g_signal_handler_unblock(G_OBJECT (priv->entry), priv->hid_changed);
		g_signal_handler_unblock(G_OBJECT (priv->entry), priv->hid_insert);
		return FALSE;
	}

	return TRUE;
}



static void
hbtk_decimal_entry_class_init (HbtkDecimalEntryClass *class)
{
	DB( g_print("\n[decimalentry] class_init\n") );

	decimalentry_signals[VALUE_CHANGED] =
		g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (HbtkDecimalEntryClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}


static void
hbtk_decimal_entry_init (HbtkDecimalEntry *decimalentry)
{
HbtkDecimalEntryPrivate *priv;

	DB( g_print("\n[decimalentry] init\n") );

	decimalentry->priv = hbtk_decimal_entry_get_instance_private(decimalentry);
	
	priv = decimalentry->priv;

	priv->valid = TRUE;
	priv->digits = 2;
	//priv->income = FALSE;

	priv->entry = GTK_WIDGET(decimalentry);
	//todo: see if really useful
	gtk_entry_set_width_chars(GTK_ENTRY(priv->entry), 16);
	gtk_entry_set_alignment(GTK_ENTRY(priv->entry), 1.0);
	gtk_entry_set_max_width_chars(GTK_ENTRY(priv->entry), 16);
	//gtk_box_pack_start (GTK_BOX (decimalentry), priv->entry, TRUE, TRUE, 0);

	priv->hid_insert = g_signal_connect(G_OBJECT(priv->entry), "insert-text",
			 G_CALLBACK(hbtk_decimal_entry_insert_text_handler), NULL);

	priv->hid_changed = g_signal_connect(G_OBJECT(priv->entry), "changed",
			 G_CALLBACK(hbtk_decimal_entry_cb_changed), NULL);

	g_signal_connect (priv->entry, "activate",
				G_CALLBACK (hbtk_decimal_entry_cb_activate), NULL);

	g_signal_connect_after (priv->entry, "focus-in-event",
				G_CALLBACK (hbtk_decimal_entry_cb_focus_in), NULL);

	g_signal_connect_after (priv->entry, "focus-out-event",
				G_CALLBACK (hbtk_decimal_entry_cb_focus_out), NULL);



}


/* = = = = = = = = public function = = = = = = = = */

//probably need get/set digit

//probably need _update method here


gdouble
hbtk_decimal_entry_get_value (HbtkDecimalEntry *decimalentry)
{
	g_return_val_if_fail (HBTK_IS_DECIMAL_ENTRY (decimalentry), 0.0);
	return decimalentry->priv->value;
}


gboolean
hbtk_decimal_entry_get_forcedsign (HbtkDecimalEntry *decimalentry)
{
	g_return_val_if_fail (HBTK_IS_DECIMAL_ENTRY (decimalentry), FALSE);
	return decimalentry->priv->forcedsign;
}


void
hbtk_decimal_entry_set_value (HbtkDecimalEntry *decimalentry, gdouble value)
{
	g_return_if_fail (HBTK_IS_DECIMAL_ENTRY (decimalentry));
	hbtk_decimal_value_change(decimalentry, value);
}


void
hbtk_decimal_entry_set_digits (HbtkDecimalEntry *decimalentry, guint value)
{
	g_return_if_fail (HBTK_IS_DECIMAL_ENTRY (decimalentry));
	decimalentry->priv->digits = value;
	hbtk_decimal_value_change(decimalentry, decimalentry->priv->value);
}


GtkWidget *
hbtk_decimal_entry_new (GtkWidget *label)
{
	DB( g_print("\n[decimalentry] new\n") );

	HbtkDecimalEntry *decimalentry = g_object_new (HBTK_TYPE_DECIMAL_ENTRY, NULL);
	if(decimalentry)
	{
	HbtkDecimalEntryPrivate *priv = decimalentry->priv;

		if(label)
			gtk_label_set_mnemonic_widget (GTK_LABEL(label), priv->entry);

		hbtk_decimal_entry_default_output(decimalentry);
	}
	return GTK_WIDGET(decimalentry);
}

