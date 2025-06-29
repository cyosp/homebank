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

#ifndef __HB_GROUP_GTK_H__
#define __HB_GROUP_GTK_H__


/* = = = = = = = = = = */

struct grpPopContext
{
	GtkTreeModel *model;
	guint	except_key;
};

/* = = = = = = = = = = */

guint32 ui_grp_comboboxentry_get_key(GtkComboBox *entry_box);
guint32 ui_grp_comboboxentry_get_key_add_new(GtkComboBox *entry_box);
Group *ui_grp_comboboxentry_get(GtkComboBox *entry_box);
gboolean ui_grp_comboboxentry_set_active(GtkComboBox *entry_box, guint32 key);
void ui_grp_comboboxentry_add(GtkComboBox *entry_box, Group *pay);
void ui_grp_comboboxentry_populate(GtkComboBox *entry_box, GHashTable *hash);
void ui_grp_comboboxentry_populate_except(GtkComboBox *entry_box, GHashTable *hash, guint except_key);
GtkWidget *ui_grp_comboboxentry_new(GtkWidget *label);


#endif

