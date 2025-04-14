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

#ifndef __HBTK_SWITCHER_H__
#define __HBTK_SWITCHER_H__

G_BEGIN_DECLS

#define HBTK_TYPE_SWITCHER            (hbtk_switcher_get_type ())
#define HBTK_SWITCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HBTK_TYPE_SWITCHER, HbtkSwitcher))
#define HBTK_SWITCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HBTK_TYPE_SWITCHER, HbtkSwitcherClass)
#define HBTK_IS_SWITCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HBTK_TYPE_SWITCHER))
#define HBTK_IS_SWITCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HBTK_TYPE_SWITCHER))
#define HBTK_SWITCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HBTK_TYPE_SWITCHER, HbtkSwitcherClass))

typedef struct _HbtkSwitcher		HbtkSwitcher;
typedef struct _HbtkSwitcherClass	HbtkSwitcherClass;
typedef struct _HbtkSwitcherPrivate	HbtkSwitcherPrivate;


struct _HbtkSwitcher
{
	GtkBox box;

	/*< private >*/
	HbtkSwitcherPrivate *priv;
};


struct _HbtkSwitcherClass
{
	GtkBoxClass parent_class;

	/* signals */
	void     (* changed)          (HbtkSwitcher *dateentry);

	/* Padding for future expansion */
	void (*_gtk_reserved0) (void);
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
};


struct _HbtkSwitcherPrivate
{
	GtkRadioButton	*first;
	gint			active;
};


GType		hbtk_switcher_get_type(void) G_GNUC_CONST;


GtkWidget *hbtk_switcher_new (GtkOrientation orientation);
void hbtk_switcher_setup (HbtkSwitcher *switcher, gchar **items, gboolean buttonstyle);
void hbtk_switcher_setup_with_data (HbtkSwitcher *switcher, GtkWidget *label, HbKivData *kivdata, gboolean buttonstyle);
gint hbtk_switcher_get_active (HbtkSwitcher *switcher);
void hbtk_switcher_set_active (HbtkSwitcher *switcher, gint active);
void hbtk_switcher_set_nth_sensitive (HbtkSwitcher *switcher, gint nth, gboolean sensitive);


G_END_DECLS

#endif /* __HBTK_SWITCHER_H__ */

