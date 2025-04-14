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

#ifndef __HBTK_DECIMAL_ENTRY_H__
#define __HBTK_DECIMAL_ENTRY_H__

G_BEGIN_DECLS

#define HBTK_TYPE_DECIMAL_ENTRY            (hbtk_decimal_entry_get_type ())
#define HBTK_DECIMAL_ENTRY(obj)			   (G_TYPE_CHECK_INSTANCE_CAST ((obj), HBTK_TYPE_DECIMAL_ENTRY, HbtkDecimalEntry))
#define HBTK_DECIMAL_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HBTK_TYPE_DECIMAL_ENTRY, HbtkDecimalEntryClass)
#define HBTK_IS_DECIMAL_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HBTK_TYPE_DECIMAL_ENTRY))
#define HBTK_IS_DECIMAL_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HBTK_TYPE_DECIMAL_ENTRY))
#define HBTK_DECIMAL_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HBTK_TYPE_DECIMAL_ENTRY, HbtkDecimalEntryClass))

typedef struct _HbtkDecimalEntry		HbtkDecimalEntry;
typedef struct _HbtkDecimalEntryClass	HbtkDecimalEntryClass;
typedef struct _HbtkDecimalEntryPrivate	HbtkDecimalEntryPrivate;

typedef enum {
	OPERATION_OFF,
	OPERATION_ADD,
	OPERATION_SUB,
	OPERATION_MUL,
	OPERATION_DIV
} HbtkOperation;



struct _HbtkDecimalEntry
{
	GtkBox box;

	/*< private >*/
	HbtkDecimalEntryPrivate *priv;
};


struct _HbtkDecimalEntryClass
{
	GtkBoxClass parent_class;

	/* signals */
	void     (* changed)          (HbtkDecimalEntry *dateentry);

	/* Padding for future expansion */
	void (*_gtk_reserved0) (void);
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
};


struct _HbtkDecimalEntryPrivate
{
	GtkWidget *entry;
    
	gboolean	forcedsign;
	gboolean	valid;
	gdouble		value;

	guint		digits		: 10;
	gulong		hid_insert;
	gulong		hid_changed;

};


GType		hbtk_decimal_entry_get_type(void) G_GNUC_CONST;

GtkWidget	*hbtk_decimal_entry_new(GtkWidget *label);

gdouble hbtk_decimal_entry_get_value (HbtkDecimalEntry *decimalentry);
gboolean hbtk_decimal_entry_get_forcedsign (HbtkDecimalEntry *decimalentry);

void hbtk_decimal_entry_set_value (HbtkDecimalEntry *decimalentry, gdouble value);
void hbtk_decimal_entry_set_digits (HbtkDecimalEntry *decimalentry, guint value);

G_END_DECLS

#endif /* __HBTK_DECIMAL_ENTRY_H__ */


