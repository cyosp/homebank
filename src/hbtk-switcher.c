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


//#include <time.h>
#include <stdlib.h>		/* atoi, atof, atol */
#include <libintl.h>	/* gettext */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ui-widgets.h"
#include "hbtk-switcher.h"

#define _(str) gettext (str)


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


static guint switcher_signals[LAST_SIGNAL] = {0,};


//G_DEFINE_TYPE(HbtkSwitcher, hbtk_radio_switcher, GTK_TYPE_BOX)
G_DEFINE_TYPE_WITH_CODE (HbtkSwitcher, hbtk_switcher, GTK_TYPE_BOX, G_ADD_PRIVATE (HbtkSwitcher))


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void
_radiotest_cb_button_toggled (GtkToggleButton *togglebutton, gpointer user_data);


static GtkWidget *
_radiotest_get_nth(GtkRadioButton *rbutton, gint nth)
{
GtkWidget *widget = NULL;
GSList *list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(rbutton));
gint nb = g_slist_length(list) - 1;

	if(nth <= nb)
	{
		widget = g_slist_nth_data(list, nb-nth);
	}

	return widget;
}


static void
_radiotest_set_active(GtkRadioButton *rbutton, gint nth)
{
GtkWidget *widget = _radiotest_get_nth(rbutton, nth);

	if(widget != NULL)
	{
		g_signal_handlers_block_by_func (widget, G_CALLBACK(_radiotest_cb_button_toggled), NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		g_signal_handlers_unblock_by_func (widget, G_CALLBACK(_radiotest_cb_button_toggled), NULL);
	}
}


static gint
_radiotest_get_active(GtkRadioButton *rbutton)
{
GSList *list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(rbutton));
gint nb, i = 0;
gint active = -1;

	//list is build with prepend 
	nb = g_slist_length(list) - 1;
	//g_print("-------- from %p, nb:%d\n", togglebutton, nb+1);

	while(nb >= 0 && list)
	{
	GtkToggleButton *tmp = g_slist_nth_data(list, nb);
	
		/*g_print("%d %d: %p %d %s\n", nb, i, 
			tmp, 
			gtk_toggle_button_get_active(tmp), 
			gtk_button_get_label(GTK_BUTTON(tmp))
		);*/

		if( gtk_toggle_button_get_active(tmp) == TRUE )
		{
			active = i;
			break;
		}

		i++;
		nb--;
	}

	//g_print(" get active:%d\n", active);


	return active;
}



static void
_radiotest_cb_button_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
HbtkSwitcher *switcher = HBTK_SWITCHER (user_data);
HbtkSwitcherPrivate *priv = switcher->priv;
gint active;

	if(gtk_toggle_button_get_active(togglebutton) == FALSE)
		return;

	//g_print("\n--------\n button toggled (%p)\n", togglebutton);

	active = _radiotest_get_active(GTK_RADIO_BUTTON(togglebutton));
	//g_print(" > active:%d\n", active);
	
	if(priv->active != active)
	{
		DB( g_print(" **emit 'changed' signal** %d\n", active) );
		priv->active = active;
		g_signal_emit_by_name (switcher, "changed", NULL, NULL);
	}
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void 
hbtk_switcher_destroy (GtkWidget *object)
{
//HbtkSwitcher *switcher = HBTK_SWITCHER (object);
//HbtkSwitcherPrivate *priv = switcher->priv;

	g_return_if_fail(object != NULL);
	g_return_if_fail(HBTK_IS_SWITCHER(object));

	DB( g_print("\n[switcher] destroy\n") );

	DB( g_print(" free switcher: %p\n", object) );



	GTK_WIDGET_CLASS (hbtk_switcher_parent_class)->destroy (object);
}



static void
hbtk_switcher_dispose (GObject *gobject)
{
//HbtkSwitcher *self = HBTK_SWITCHER (gobject);

	DB( g_print("\n[switcher] dispose\n") );

	
  //g_clear_object (&self->priv->an_object);

  G_OBJECT_CLASS (hbtk_switcher_parent_class)->dispose (gobject);
}




static void
hbtk_switcher_finalize (GObject *gobject)
{
//HbtkSwitcher *self = HBTK_SWITCHER (gobject);

	DB( g_print("\n[switcher] finalize\n") );

	
	//g_date_free(self->date);
  //g_free (self->priv->a_string);

  /* Always chain up to the parent class; as with dispose(), finalize()
   * is guaranteed to exist on the parent's class virtual function table
   */
	G_OBJECT_CLASS(hbtk_switcher_parent_class)->finalize (gobject);
}


static void
hbtk_switcher_class_init (HbtkSwitcherClass *class)
{
GObjectClass *object_class;
GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (class);
	widget_class = GTK_WIDGET_CLASS (class);

	DB( g_print("\n[switcher] class_init\n") );

	//object_class->constructor = hbtk_switcher_constructor;
	//object_class->set_property = hbtk_switcher_set_property;
	//object_class->get_property = hbtk_switcher_get_property;
	object_class->dispose  = hbtk_switcher_dispose;
	object_class->finalize = hbtk_switcher_finalize;

	widget_class->destroy  = hbtk_switcher_destroy;
	
	switcher_signals[CHANGED] =
		g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (HbtkSwitcherClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

	//g_type_class_add_private (object_class, sizeof (HbtkSwitcherPrivate));
	
}


static void
hbtk_switcher_init (HbtkSwitcher *switcher)
{
HbtkSwitcherPrivate *priv;

	DB( g_print("\n[switcher] init\n") );

	priv = switcher->priv = hbtk_switcher_get_instance_private(switcher);

	priv->active = 0;
	//TODO ??
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


GtkWidget *
hbtk_switcher_new (GtkOrientation orientation)
{
HbtkSwitcher *switcher;

	DB( g_print("\n[switcher] new\n") );

	switcher = g_object_new (HBTK_TYPE_SWITCHER, 
		"orientation", orientation,
		NULL);

	return GTK_WIDGET(switcher);
}


void 
hbtk_switcher_setup (HbtkSwitcher *switcher, gchar **items, gboolean buttonstyle)
{
HbtkSwitcherPrivate *priv = switcher->priv;
GtkWidget *button, *newbutton;
guint i;

    button = gtk_radio_button_new_with_label (NULL, _(items[0]));
    priv->first = GTK_RADIO_BUTTON(button);
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), !buttonstyle);
	g_signal_connect(button, "toggled", G_CALLBACK(_radiotest_cb_button_toggled), switcher);

    gtk_box_prepend (GTK_BOX (switcher), button);
	for (i = 1; items[i] != NULL; i++)
	{
		newbutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button), _(items[i]));
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (newbutton), !buttonstyle);
		g_signal_connect(newbutton, "toggled", G_CALLBACK(_radiotest_cb_button_toggled), switcher);
	    gtk_box_prepend (GTK_BOX (switcher), newbutton);
	}

	if(buttonstyle)
	{
	GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET(switcher));
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_LINKED);
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_RAISED);
	}
}


void
hbtk_switcher_setup_with_data (HbtkSwitcher *switcher, GtkWidget *label, HbKivData *kivdata, gboolean buttonstyle)
{
HbtkSwitcherPrivate *priv = switcher->priv;
GtkWidget *button, *image, *newbutton;
HbKivData *tmp = &kivdata[0];
guint i;

    //button = gtk_radio_button_new_with_label (NULL, _(items[0]));
	button = gtk_radio_button_new(NULL);
	priv->first = GTK_RADIO_BUTTON(button);
	image = hbtk_image_new_from_icon_name_16 (tmp->iconname);
	g_object_set (button, "image", image, "tooltip-text", _(tmp->name), NULL);
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), !buttonstyle);
	//#2065592
	g_signal_connect(button, "toggled", G_CALLBACK(_radiotest_cb_button_toggled), switcher);
    gtk_box_prepend (GTK_BOX (switcher), button);
	for (i = 1; ; i++)
	{
		tmp = &kivdata[i];
		if( tmp->name == NULL )
			break;

		//newbutton = gtk_radio_button_new_with_label_from_widget (GTK_BUTTON (button), _(items[i]));
		newbutton = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (button));
		image = hbtk_image_new_from_icon_name_16 (tmp->iconname);
		g_object_set (newbutton, "image", image, "tooltip-text", _(tmp->name), NULL);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (newbutton), !buttonstyle);
		g_signal_connect(newbutton, "toggled", G_CALLBACK(_radiotest_cb_button_toggled), switcher);
	    gtk_box_prepend (GTK_BOX (switcher), newbutton);
	}

	if(buttonstyle)
	{
	GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET(switcher));
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_LINKED);
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_RAISED);
	}
}



gint
hbtk_switcher_get_active (HbtkSwitcher *switcher)
{
HbtkSwitcherPrivate *priv = switcher->priv;

	//return _radiotest_get_active(
	return priv->active;
}


void
hbtk_switcher_set_active (HbtkSwitcher *switcher, gint active)
{
HbtkSwitcherPrivate *priv = switcher->priv;

	_radiotest_set_active(priv->first, active);
	priv->active = active;
}


void
hbtk_switcher_set_nth_sensitive (HbtkSwitcher *switcher, gint nth, gboolean sensitive)
{
HbtkSwitcherPrivate *priv = switcher->priv;
GtkWidget *widget = _radiotest_get_nth(priv->first, nth);

	if(widget != NULL)
	{
		gtk_widget_set_sensitive(widget, sensitive);
	}
}

