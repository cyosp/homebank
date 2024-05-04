/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 2018-2019 Adrien Dorsaz <adrien@adorsaz.ch>
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

#ifndef __HB_BUD_TABVIEW_H__
#define __HB_BUD_TABVIEW_H__

//by default Maxime don't want table budget view to add/remove/rename categories
#define HB_BUD_TABVIEW_EDIT_ENABLE 0

struct ui_bud_tabview_data
{
	GtkWidget *dialog;

	// Number of changes to notify globally
	gint change;

	// Tree view with budget
	GtkWidget *TV_budget;
	GtkTreeViewColumn *TVC_category;
	GtkTreeSelection *TV_selection;

	// Radio buttons of view mode
	GtkWidget *RA_mode;

	// Tool bar
#if HB_BUD_TABVIEW_EDIT_ENABLE
	GtkWidget *BT_category_add, *BT_category_delete, *BT_category_merge;
#endif
	GtkWidget *BT_category_reset, *BT_category_force_monitoring;
	gulong HID_category_monitoring_toggle;
	GtkWidget *BT_expand, *BT_collapse;

	// Should the tree be collapsed
	gboolean TV_is_expanded;

#if HB_BUD_TABVIEW_EDIT_ENABLE
	// Add Dialog
	GtkWidget *COMBO_add_parent, *EN_add_name, *BT_apply;

	// Merge Dialog
	GtkWidget *COMBO_merge_target, *CHECK_merge_delete;
	guint32 MERGE_source_category_key;
#endif
	// Search
	GtkWidget *EN_search;
};
typedef struct ui_bud_tabview_data ui_bud_tabview_data_t;

GtkWidget *ui_bud_tabview_manage_dialog(void);


#endif
