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

#ifndef __HB_WIDGETS_GTK_H__
#define __HB_WIDGETS_GTK_H__


typedef struct _hbtk_kv_data		HbKvData;
typedef struct _hbtk_kiv_data		HbKivData;

struct _hbtk_kv_data {
	guint32			key;
	const gchar		*name;
};

struct _hbtk_kiv_data {
	guint32			key;
	const gchar		*iconname;
	const gchar		*name;
};


#define HBTK_IS_SEPARATOR -66


typedef enum {
	DATE_RANGE_BUDGET_MODE    = 1 << 1,
	DATE_RANGE_CUSTOM_HIDDEN  = 1 << 8,
	DATE_RANGE_CUSTOM_DISABLE = 1 << 9
} HbDateRangeFlags;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* GTK4 transitional anticipation */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#if( (GTK_MAJOR_VERSION < 4)  )
void gtk_window_set_child (GtkWindow* window, GtkWidget* child);
void gtk_popover_set_child (GtkPopover* popover, GtkWidget* child);

void gtk_frame_set_child (GtkFrame* frame, GtkWidget* child);
void gtk_overlay_set_child (GtkOverlay* overlay, GtkWidget* child);
void gtk_scrolled_window_set_child (GtkScrolledWindow* scrolled_window, GtkWidget* child);
void gtk_revealer_set_child (GtkRevealer* revealer, GtkWidget* child);
void gtk_expander_set_child (GtkExpander* expander, GtkWidget* child);

void gtk_window_destroy (GtkWindow* window);
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


GtkWidget *make_label(gchar *str, gfloat xalign, gfloat yalign);
GtkWidget *make_clicklabel(gchar *id, gchar *str);
GtkWidget *make_label_group(gchar *str);
GtkWidget *make_label_widget(gchar *str);
GtkWidget *make_text(gfloat xalign);
GtkWidget *make_search(void);
GtkWidget *make_string(GtkWidget *label);

GtkWidget *hbtk_menubar_add_menu(GtkWidget *menubar, gchar *label, GtkWidget **menuitem_ptr);
GtkWidget *hbtk_menu_add_menuitem(GtkWidget *menu, gchar *label);

GtkWidget *hbtk_toolbar_add_toolbutton(GtkToolbar *toolbar, gchar *icon_name, gchar *label, gchar *tooltip_text);

GtkWidget *make_image_toggle_button(gchar *icon_name, gchar *tooltip_text);
GtkWidget *make_image_button(gchar *icon_name, gchar *tooltip_text);

GtkWidget *make_memo_entry(GtkWidget *label);
GtkWidget *make_string_maxlength(GtkWidget *label, guint max_length);

GtkWidget *make_entry_numeric(GtkWidget *label, gint min, gint max);

GtkWidget *make_amount(GtkWidget *label);
GtkWidget *make_exchange_rate(GtkWidget *label);
GtkWidget *make_numeric(GtkWidget *label, gdouble min, gdouble max);
GtkWidget *make_scale(GtkWidget *label);
GtkWidget *make_long(GtkWidget *label);

guint32 hbtk_monthyear_getmin(GtkSpinButton *spin);
guint32 hbtk_monthyear_getmax(GtkSpinButton *spin);
void hbtk_monthyear_set(GtkSpinButton *spin, guint32 julian);
GtkWidget *make_monthyear(GtkWidget *label);
//GtkWidget *make_year(GtkWidget *label);
GtkWidget *make_cycle(GtkWidget *label, gchar **items);
GtkWidget *make_daterange(GtkWidget *label, HbDateRangeFlags flags);

GtkWidget *create_popover (GtkWidget *parent, GtkWidget *child, GtkPositionType pos);

void ui_label_set_integer(GtkLabel *label, gint value);

void hbtk_listview_redraw_selected_row(GtkTreeView *treeview);
gboolean hbtk_tree_store_get_top_level(GtkTreeModel *model, gint column_id, guint32 key, GtkTreeIter *return_iter);
void hbtk_tree_store_remove_iter_with_child(GtkTreeModel *model, GtkTreeIter *iter);
GtkTreeViewColumn *hbtk_treeview_get_column_by_id(GtkTreeView *treeview, gint search_id);

gchar *hbtk_get_label(HbKvData *kvdata, guint32 key);
guint32 hbtk_combo_box_get_active_id (GtkComboBoxText *combobox);
void hbtk_combo_box_set_active_id (GtkComboBoxText *combobox, guint32 active_id);
void hbtk_combo_box_text_append (GtkComboBoxText *combobox, guint32 key, gchar *text);
GtkWidget *hbtk_combo_box_new (GtkWidget *label);
GtkWidget *hbtk_combo_box_new_with_data (GtkWidget *label, HbKvData *kvdata);

gint hbtk_radio_button_get_active (GtkContainer *container);
void hbtk_radio_button_set_active (GtkContainer *container, gint active);
GtkWidget *hbtk_radio_button_get_nth (GtkContainer *container, gint nth);
void hbtk_radio_button_unblock_by_func(GtkContainer *container, GCallback c_handler, gpointer data);
void hbtk_radio_button_block_by_func(GtkContainer *container, GCallback c_handler, gpointer data);
void hbtk_radio_button_connect(GtkContainer *container, const gchar *detailed_signal, GCallback c_handler, gpointer data);
GtkWidget *hbtk_radio_button_new (GtkOrientation orientation, gchar **items, gboolean buttonstyle);
GtkWidget *hbtk_radio_button_new_with_data (HbKivData *kivdata, gboolean buttonstyle);

#ifdef G_OS_WIN32
void hbtk_assistant_hack_button_order(GtkAssistant *assistant);
#endif

void gimp_label_set_attributes (GtkLabel *label, ...);


void hb_widget_set_margins(GtkWidget *widget, gint top, gint right, gint bottom, gint left);
void hb_widget_set_margin(GtkWidget *widget, gint margin);
void hb_widget_visible(GtkWidget *widget, gboolean visible);
void hbtk_entry_tag_name_append(GtkEntry *entry, gchar *tagname);
void hbtk_entry_set_text(GtkEntry *entry, gchar *text);
void hbtk_entry_replace_text(GtkEntry *entry, gchar **storage);

/*
guint make_popaccount_populate(GtkComboBox *combobox, GList *srclist);
GtkWidget *make_popaccount(GtkWidget *label);

guint make_poppayee_populate(GtkComboBox *combobox, GList *srclist);
GtkWidget *make_poppayee(GtkWidget *label);

guint make_poparchive_populate(GtkComboBox *combobox, GList *srclist);
GtkWidget *make_poparchive(GtkWidget *label);

guint make_popcategory_populate(GtkComboBox *combobox, GList *srclist);
GtkWidget *make_popcategory(GtkWidget *label);
*/


gint hb_clicklabel_to_int(const gchar *uri);

const gchar *get_paymode_icon_name(guint32 key);

guint32 paymode_combo_box_get_active (GtkComboBox *combo_box);
void paymode_combo_box_set_active (GtkComboBox *combo_box, guint32 active_key);

GtkWidget *make_paymode(GtkWidget *label);
GtkWidget *make_paymode_nointxfer(GtkWidget *label);
GtkWidget *make_nainex(GtkWidget *label);

#endif
