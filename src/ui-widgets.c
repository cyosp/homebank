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


#include "homebank.h"

#include "gtk-chart.h"
#include "ui-widgets.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


extern HbKvData CYA_FLT_RANGE_DWF[];
extern HbKvData CYA_FLT_RANGE_MQY[];
extern HbKvData CYA_FLT_RANGE_YTO[];
extern HbKvData CYA_FLT_RANGE_LASTXXD[];
extern HbKvData CYA_FLT_RANGE_COMMON[];
extern HbKvData CYA_FLT_RANGE_CUSTOM[];

extern gchar *CYA_ABMONTHS[];
extern gchar *CYA_ARC_UNIT[];


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* GTK4 transitional anticipation */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#if( (GTK_MAJOR_VERSION < 4)  )

void gtk_window_set_child (GtkWindow* window, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(window), child); }

void gtk_popover_set_child (GtkPopover* popover, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(popover), child); }

void gtk_frame_set_child (GtkFrame* frame, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(frame), child); }

void gtk_overlay_set_child (GtkOverlay* overlay, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(overlay), child); }

void gtk_scrolled_window_set_child (GtkScrolledWindow* scrolled_window, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(scrolled_window), child); }

void gtk_revealer_set_child (GtkRevealer* revealer, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(revealer), child); }

void gtk_expander_set_child (GtkExpander* expander, GtkWidget* child)
{ gtk_container_add (GTK_CONTAINER(expander), child); }

void gtk_box_prepend (GtkBox* box, GtkWidget* child)
{ gtk_box_pack_start (GTK_BOX(box), child, FALSE, FALSE, 0); }
void gtk_box_append (GtkBox* box, GtkWidget* child)
{ gtk_box_pack_end (GTK_BOX(box), child, FALSE, FALSE, 0); }


void gtk_window_destroy (GtkWindow* window)
{ gtk_widget_destroy (GTK_WIDGET(window)); }

#endif


//TODO: to be removed when migrate
GtkWidget *hbtk_image_new_from_icon_name_16(const gchar *icon_name)
{
	return gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
}

GtkWidget *hbtk_image_new_from_icon_name_24(const gchar *icon_name)
{
	return gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
}

GtkWidget *hbtk_image_new_from_icon_name_32(const gchar *icon_name)
{
	return gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* GTK3 obsolete */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

GtkWidget *
hbtk_toolbar_add_toolbutton(GtkToolbar *toolbar, gchar *icon_name, gchar *label, gchar *tooltip_text)
{
GtkWidget *button = gtk_widget_new(GTK_TYPE_TOOL_BUTTON,
		"icon-name", icon_name,
        "label", label,
	    NULL);
	if(tooltip_text != NULL)
		gtk_widget_set_tooltip_text(button, tooltip_text);


	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	return button;
}



GtkWidget *
hbtk_menubar_add_menu(GtkWidget *menubar, gchar *label, GtkWidget **menuitem_ptr)
{
GtkWidget *menu, *menuitem;

	menu = gtk_menu_new();
	menuitem = gtk_menu_item_new_with_mnemonic(label);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menuitem);
	if(menuitem_ptr)
		*menuitem_ptr = menuitem;
	return menu;
}


GtkWidget *
hbtk_menu_add_menuitem(GtkWidget *menu, gchar *label)
{
GtkWidget *menuitem = gtk_menu_item_new_with_mnemonic(label);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	return menuitem;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


//TODO: only WEIGHT & SCALE are used for now
void
gimp_label_set_attributes (GtkLabel *label,
                           ...)
{
  PangoAttribute *attr  = NULL;
  PangoAttrList  *attrs;
  va_list         args;

  g_return_if_fail (GTK_IS_LABEL (label));

  attrs = pango_attr_list_new ();

  va_start (args, label);

  do
    {
      PangoAttrType attr_type = va_arg (args, PangoAttrType);

      if (attr_type <= 0)
        attr_type = PANGO_ATTR_INVALID;

      switch (attr_type)
        {
        case PANGO_ATTR_LANGUAGE:
          attr = pango_attr_language_new (va_arg (args, PangoLanguage *));
          break;

        case PANGO_ATTR_FAMILY:
          attr = pango_attr_family_new (va_arg (args, const gchar *));
          break;

        case PANGO_ATTR_STYLE:
          attr = pango_attr_style_new (va_arg (args, PangoStyle));
          break;

        case PANGO_ATTR_WEIGHT:
          attr = pango_attr_weight_new (va_arg (args, PangoWeight));
          break;

        case PANGO_ATTR_VARIANT:
          attr = pango_attr_variant_new (va_arg (args, PangoVariant));
          break;

        case PANGO_ATTR_STRETCH:
          attr = pango_attr_stretch_new (va_arg (args, PangoStretch));
          break;

        case PANGO_ATTR_SIZE:
          attr = pango_attr_size_new (va_arg (args, gint));
          break;

        case PANGO_ATTR_FONT_DESC:
          attr = pango_attr_font_desc_new (va_arg (args,
                                                   const PangoFontDescription *));
          break;

        case PANGO_ATTR_FOREGROUND:
          {
            const PangoColor *color = va_arg (args, const PangoColor *);

            attr = pango_attr_foreground_new (color->red,
                                              color->green,
                                              color->blue);
          }
          break;

        case PANGO_ATTR_BACKGROUND:
          {
            const PangoColor *color = va_arg (args, const PangoColor *);

            attr = pango_attr_background_new (color->red,
                                              color->green,
                                              color->blue);
          }
          break;

        case PANGO_ATTR_UNDERLINE:
          attr = pango_attr_underline_new (va_arg (args, PangoUnderline));
          break;

        case PANGO_ATTR_STRIKETHROUGH:
          attr = pango_attr_strikethrough_new (va_arg (args, gboolean));
          break;

        case PANGO_ATTR_RISE:
          attr = pango_attr_rise_new (va_arg (args, gint));
          break;

        case PANGO_ATTR_SCALE:
          attr = pango_attr_scale_new (va_arg (args, gdouble));
          break;

        default:
          //g_warning ("%s: invalid PangoAttribute type %d", G_STRFUNC, attr_type);
        case PANGO_ATTR_INVALID:
          attr = NULL;
          break;
        }

      if (attr)
        {
          attr->start_index = 0;
          attr->end_index   = -1;
          pango_attr_list_insert (attrs, attr);
        }
    }
  while (attr);

  va_end (args);

  gtk_label_set_attributes (label, attrs);
  pango_attr_list_unref (attrs);
}


// can't get rid of FILL|EXPAND for now
void hbtk_box_prepend (GtkBox* box, GtkWidget* child)
{
	gtk_box_pack_start (GTK_BOX(box), child, TRUE, TRUE, 0);
}



gint hb_clicklabel_to_int(const gchar *uri)
{
gint retval = HB_LIST_QUICK_SELECT_UNSET;

	if (g_strcmp0 (uri, "all") == 0)	
	{
		retval = HB_LIST_QUICK_SELECT_ALL;
	}
	else
	if (g_strcmp0 (uri, "non") == 0)	
	{
		retval = HB_LIST_QUICK_SELECT_NONE;
	}
	else
	if (g_strcmp0 (uri, "inv") == 0)	
	{
		retval = HB_LIST_QUICK_SELECT_INVERT;
	}

	return retval;
}


void hb_window_run_pending(void)
{
	while (gtk_events_pending ())
		gtk_main_iteration ();
}


void hb_widget_set_margins(GtkWidget *widget, gint top, gint right, gint bottom, gint left)
{
	gtk_widget_set_margin_top (widget, top);
	gtk_widget_set_margin_end (widget, right);
	gtk_widget_set_margin_bottom (widget, bottom);
	gtk_widget_set_margin_start (widget, left);
}


void hb_widget_set_margin(GtkWidget *widget, gint margin)
{
	hb_widget_set_margins (widget, margin, margin, margin, margin);
}


void hb_widget_visible(GtkWidget *widget, gboolean visible)
{
	if(!GTK_IS_WIDGET(widget))
		return;

	if(visible)
	{
		gtk_widget_show(widget);
	}
	else
	{
		gtk_widget_hide(widget);
	}
}


void ui_label_set_integer(GtkLabel *label, gint value)
{
gchar buf[16];

	g_snprintf(buf, 16, "%d", value);
	gtk_label_set_text (label, buf);
}


void hbtk_entry_tag_name_append(GtkEntry *entry, gchar *tagname)
{
GtkEntryBuffer *buffer;
const gchar *text;
guint len;

	text = gtk_entry_get_text(entry);
	if( g_strstr_len(text, -1, tagname) == NULL )
	{
		DB( g_print(" gtkentry append tagname '%s'\n", tagname) );
		buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
		if(buffer)
		{
			len = gtk_entry_buffer_get_length(buffer);
			DB( g_print("- add ' %s' %p %d\n", tagname, buffer, len) );
			if(len > 0)
				gtk_entry_buffer_insert_text(buffer, len, " ", 1);
			gtk_entry_buffer_insert_text(buffer, len+1, tagname, -1);
		}
	}

}


void hbtk_entry_set_text(GtkEntry *entry, gchar *text)
{
	//DB( g_print(" set text to '%s'\n", text) );
	gtk_entry_set_text(GTK_ENTRY(entry), ( text != NULL ) ? text : "");
}


gboolean hbtk_entry_replace_text(GtkEntry *entry, gchar **storage)
{
const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
gint tmpcmp = hb_string_ascii_compare(*storage, (gchar *)text);

	DB( g_print(" storage is '%p' at '%p'\n", *storage, storage) );
	if( tmpcmp != 0 )
	{
		// free any previous string
		g_free(*storage);
		*storage = g_strdup(text);
		DB( g_print(" replace with '%s'", text) );
		return TRUE;
	}
	return FALSE;
}


// redraw a single row of a listview (not work with GTK_SELECTION_MULTIPLE) 
void
hbtk_listview_redraw_selected_row(GtkTreeView *treeview)
{
GtkTreeModel *model;
GtkTreeSelection *selection;
GtkTreeIter iter;
GtkTreePath *path;

	selection = gtk_tree_view_get_selection(treeview);
	if( gtk_tree_selection_get_selected(selection, &model, &iter) )
	{
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free (path);
	}
}


gboolean
hbtk_tree_store_get_top_level(GtkTreeModel *model, gint column_id, guint32 key, GtkTreeIter *return_iter)
{
GtkTreeIter iter;
gboolean valid;
guint32 tmpkey;

    if( model != NULL && key > 0 )
    {
		valid = gtk_tree_model_get_iter_first(model, &iter);
		while (valid)
		{
			gtk_tree_model_get (model, &iter, column_id, &tmpkey, -1);
			if(tmpkey == key)
			{
				*return_iter = iter;
				return TRUE;
			}

		 valid = gtk_tree_model_iter_next(model, &iter);
		}
	}
	return FALSE;
}


void 
hbtk_tree_store_remove_iter_with_child(GtkTreeModel *model, GtkTreeIter *iter)
{
GtkTreeIter child;
gboolean valid;

	valid = gtk_tree_model_iter_children(model, &child, iter);
	while( valid )
	{
		valid = gtk_tree_store_remove(GTK_TREE_STORE(model), &child);
		if( valid )
			valid = gtk_tree_model_iter_next(model, &child);
	}

	gtk_tree_store_remove(GTK_TREE_STORE(model), iter);
}


GtkTreeViewColumn *
hbtk_treeview_get_column_by_id(GtkTreeView *treeview, gint search_id)
{
GtkTreeViewColumn *column = NULL;
GList *list, *tmp;
gint id;

	list = gtk_tree_view_get_columns( treeview );
	tmp = g_list_first(list);
	while (tmp != NULL)
	{
		id = gtk_tree_view_column_get_sort_column_id(tmp->data);
		if( search_id == id )
		{
			column = tmp->data;
			break;
		}
		tmp = g_list_next(tmp);
	}
	g_list_free(list);

	return column;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


GtkWidget *make_clicklabel(gchar *id, gchar *str)
{
GtkWidget *label;
gchar buffer[255];

	g_snprintf(buffer, 254, "<a href=\"%s\">%s</a>", id, str);
	label = gtk_label_new(buffer);
	gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_track_visited_links(GTK_LABEL(label), FALSE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);

	return GTK_WIDGET(label);
}


GtkWidget *make_label_group(gchar *str)
{
GtkWidget *label = gtk_label_new (str);

	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, -1);
	return label;
}

GtkWidget *make_label_left(char *str)
{
GtkWidget *label = gtk_label_new_with_mnemonic (str);

	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	return label;
}


GtkWidget *make_label_widget(char *str)
{
GtkWidget *label = gtk_label_new_with_mnemonic (str);

	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (label, GTK_ALIGN_END);
	return label;
}


GtkWidget *make_label(char *str, gfloat xalign, gfloat yalign)
{
GtkWidget *label = gtk_label_new_with_mnemonic (str);

	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION < 16) )
	gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
	#else
	gtk_label_set_xalign(GTK_LABEL(label), xalign);
	gtk_label_set_yalign(GTK_LABEL(label), yalign);
	#endif
	return label;
}


/*
**
*/
GtkWidget *make_text(gfloat xalign)
{
GtkWidget *entry;

	entry = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE(entry), FALSE);
	g_object_set(entry, "xalign", xalign, NULL);
	return entry;
}


GtkWidget *make_search(void)
{
GtkWidget *search;

	search = gtk_search_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(search), _("Search...") );
	gtk_entry_set_width_chars(GTK_ENTRY(search), 24);

	return search;
}



/*
**
*/
GtkWidget *make_string(GtkWidget *label)
{
GtkWidget *entry;

	entry = gtk_entry_new ();

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);

	return entry;
}


static GtkWidget *_raw_image_button(GType type, gchar *icon_name, gchar *tooltip_text, gboolean flat)
{
GtkWidget *image, *button;

	image = gtk_widget_new(GTK_TYPE_IMAGE, "icon-name", icon_name, /*"margin", 2,*/ NULL);
	button = gtk_widget_new(type, "image", image, "tooltip-text", tooltip_text, NULL);
	if( flat )
		gtk_style_context_add_class (gtk_widget_get_style_context (button), GTK_STYLE_CLASS_FLAT);
	return button;
}


GtkWidget *make_image_button(gchar *icon_name, gchar *tooltip_text)
{
	return _raw_image_button(GTK_TYPE_BUTTON, icon_name, tooltip_text, FALSE);
}


GtkWidget *make_image_toggle_button(gchar *icon_name, gchar *tooltip_text)
{
	return _raw_image_button(GTK_TYPE_TOGGLE_BUTTON, icon_name, tooltip_text, FALSE);
}


GtkWidget *make_image_radio_button(gchar *icon_name, gchar *tooltip_text)
{
GtkWidget * button = _raw_image_button(GTK_TYPE_RADIO_BUTTON, icon_name, tooltip_text, FALSE);

	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);

	return button;
}


/* toolbar stuff */


GtkWidget *make_tb(void)
{
GtkWidget *widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_style_context_add_class (gtk_widget_get_style_context (widget), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
	return widget;
}


GtkWidget *make_tb_separator(void)
{
GtkWidget *widget = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_set_margin_start(widget, SPACING_SMALL);
	gtk_widget_set_margin_end  (widget, SPACING_SMALL);
	return widget;
}


GtkWidget *make_tb_image_button(gchar *icon_name, gchar *tooltip_text)
{
GtkWidget *button = _raw_image_button(GTK_TYPE_BUTTON, icon_name, tooltip_text, TRUE);
GtkWidget *img = gtk_button_get_image(GTK_BUTTON(button));

	g_object_set(img, "icon-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	return button;
}


GtkWidget *make_tb_image_toggle_button(gchar *icon_name, gchar *tooltip_text)
{
GtkWidget *button = _raw_image_button(GTK_TYPE_TOGGLE_BUTTON, icon_name, tooltip_text, TRUE);
GtkWidget *img = gtk_button_get_image(GTK_BUTTON(button));

	g_object_set(img, "icon-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	return button;
}


GtkWidget *make_tb_image_radio_button(gchar *icon_name, gchar *tooltip_text)
{
GtkWidget * button = _raw_image_button(GTK_TYPE_RADIO_BUTTON, icon_name, tooltip_text, TRUE);
GtkWidget *img = gtk_button_get_image(GTK_BUTTON(button));

	g_object_set(img, "icon-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);

	return button;
}




/*
**
*/
GtkWidget *make_memo_entry(GtkWidget *label)
{
GtkListStore *store;
GtkWidget *entry;
GtkEntryCompletion *completion;
GList *lmem, *list;

	store = gtk_list_store_new (1, G_TYPE_STRING);

    completion = gtk_entry_completion_new ();
    gtk_entry_completion_set_model (completion, GTK_TREE_MODEL(store));
    gtk_entry_completion_set_text_column (completion, 0);

	entry = gtk_entry_new ();
	gtk_entry_set_completion (GTK_ENTRY (entry), completion);

	g_object_unref(store);

	//populate
	//gtk_list_store_clear (GTK_LIST_STORE(store));

	lmem = list = g_hash_table_get_keys(GLOBALS->h_memo);
	while (list != NULL)
	{
	GtkTreeIter  iter;

		//gtk_list_store_append (GTK_LIST_STORE(store), &iter);
		//gtk_list_store_set (GTK_LIST_STORE(store), &iter, 0, list->data, -1);
		gtk_list_store_insert_with_values(GTK_LIST_STORE(store), &iter, -1,
			0, list->data, 
			-1);

		list = g_list_next(list);
	}

	g_list_free(lmem);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);

	return entry;
}


/*
**
*/
GtkWidget *make_string_maxlength(GtkWidget *label, guint max_length)
{
GtkWidget *entry;

	entry = make_string(label);
	gtk_entry_set_width_chars(GTK_ENTRY(entry), max_length+2);
	gtk_entry_set_max_length(GTK_ENTRY(entry), max_length);

	return entry;
}


static void hb_amount_insert_text_handler (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data)
{
GtkEditable *editable = GTK_EDITABLE(entry);
gint i, digits, count=0, dcpos=-1;
gchar *clntxt;
	
	DB( g_print("-----\ninsert_text-handler: instxt:%s pos:%d len:%d\n", text, *position, length) );

	digits = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(entry));

	// most common : only 1 char to be inserted
	if( length == 1 )
	{
	const gchar *curtxt = gtk_entry_get_text(entry);

		for (i=0 ; curtxt[i]!='\0' ; i++)
		{
	 		if(curtxt[i]==',' || curtxt[i]=='.')
				dcpos = i;
		}
		DB( g_print(" dcpos:'%d'\n", dcpos) );

		clntxt = g_new0 (gchar, length+1);
		for (i=0 ; i < length ; i++)
		{
			if( g_ascii_isdigit(text[i]) && ( (*position <= dcpos + digits) || dcpos < 0) )
				goto doinsert;

			//5.4.3 + sign now authorized
			if( (text[i]=='-' || text[i]=='+') && *position==0 )	/* -/+ sign only at position 0 */
				goto doinsert;

			if( dcpos < 0 && (text[i]=='.' || text[i]==',') )	/* decimal separator if not in previous string */
				clntxt[count++] = '.';

			continue;

		doinsert:
			clntxt[count++] = text[i];
		}
	}
	// less common: paste a full text
	else
	{
		clntxt = hb_string_dup_raw_amount_clean(text, digits);
		count = strlen(clntxt);
	}
		
	if (count > 0)
	{
		DB( g_print(" insert %d char '%s' at %d\n", count, clntxt, *position) );
		g_signal_handlers_block_by_func (G_OBJECT (editable), G_CALLBACK (hb_amount_insert_text_handler), data);
		gtk_editable_insert_text (editable, clntxt, count, position);
		g_signal_handlers_unblock_by_func (G_OBJECT (editable), G_CALLBACK (hb_amount_insert_text_handler), data);
	}

	g_free (clntxt);

	g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");
}


GtkWidget *make_amount(GtkWidget *label)
{
GtkWidget *spinner;
GtkAdjustment *adj;

	//adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -G_MAXDOUBLE, G_MAXDOUBLE, 0.01, 1.0, 0.0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -8589934588, 8589934588, 0.01, 1.0, 0.0);
	spinner = gtk_spin_button_new (adj, 1.0, 2);
	g_object_set(spinner, "xalign", 1.0, NULL);

	//5.7
	gtk_entry_set_width_chars(GTK_ENTRY(spinner), 13);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), spinner);

	g_signal_connect(G_OBJECT(spinner), "insert-text",
			 G_CALLBACK(hb_amount_insert_text_handler),
			 NULL);

	return spinner;
}


GtkWidget *make_amount_pos(GtkWidget *label)
{
GtkWidget *widget = make_amount(label);

	gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), 0, 8589934588);
	return widget;
}


GtkWidget *make_exchange_rate(GtkWidget *label)
{
GtkWidget *spinner;
GtkAdjustment *adj;

	//#1871383 wish: increase exchange rate size
	//adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 99999, 0.01, 1.0, 0.0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 999999, 0.01, 1.0, 0.0);
	spinner = gtk_spin_button_new (adj, 1.0, 8);
	//gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
	g_object_set(spinner, "xalign", 1.0, NULL);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), spinner);

	return spinner;
}

/*
**
*/
GtkWidget *make_numeric(GtkWidget *label, gdouble min, gdouble max)
{
GtkWidget *spinner;
GtkAdjustment *adj;

	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, min, max, 1.0, 10.0, 0.0);
	spinner = gtk_spin_button_new (adj, 0, 0);
	//gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
	g_object_set(spinner, "xalign", 1.0, NULL);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), spinner);

	return spinner;
}


GtkWidget *
make_scrolled_window_ns(GtkPolicyType hscrollbar_policy, GtkPolicyType vscrollbar_policy)
{
GtkWidget *scrollwin;

	scrollwin = gtk_scrolled_window_new(NULL,NULL);
    //gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), hscrollbar_policy, vscrollbar_policy);
	
	return scrollwin;
}


GtkWidget *
make_scrolled_window(GtkPolicyType hscrollbar_policy, GtkPolicyType vscrollbar_policy)
{
GtkWidget *scrollwin;

	scrollwin = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), hscrollbar_policy, vscrollbar_policy);
	
	return scrollwin;
}


/*
**
*/
GtkWidget *make_scale(GtkWidget *label)
{
GtkWidget *scale;

	scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, GTK_CHART_MINBARW, GTK_CHART_SPANBARW, 1.0);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_range_set_value(GTK_RANGE(scale), GTK_CHART_BARW);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), scale);

	return scale;
}

/*
**
*/
GtkWidget *make_long(GtkWidget *label)
{
GtkWidget *spinner;

	spinner = make_numeric(label, 0.0, G_MAXINT);
	return spinner;
}


static gint
hbtk_monthyear_spin_input (GtkSpinButton *spin_button,
                 gdouble       *new_val)
{
  const gchar *text;
  gchar **str;
  gboolean found = FALSE;
  gint month = 0;
  gint year;
  gchar *endm;
   gint i;
  gchar *tmp1, *tmp2;

  text = gtk_entry_get_text (GTK_ENTRY (spin_button));
  str = g_strsplit (text, " ", 2);

  if (g_strv_length (str) == 2)
    {
      //month = strtol (str[0], &endh, 10);
	  for (i = 0; i < 12; i++)
		{
		  tmp1 = g_ascii_strup (_(CYA_ABMONTHS[i+1]), -1);
		  tmp2 = g_ascii_strup (str[0], -1);
		  if (strstr (tmp1, tmp2) == tmp1)
		  {
		    found = TRUE;
		    month = i;
		  }
		  g_free (tmp1);
		  g_free (tmp2);
		  if (found)
		    break;
		}


      year = strtol (str[1], &endm, 10) - 1900;
      
	//g_print(" input: m=%d y=%d => %d\n", month, year, month + year * 12);

      if (found && !*endm && 0 <= month && month < 12 )
        {
          *new_val = month + (year * 12);
          found = TRUE;
          //g_print("  affect newval %f\n", *new_val);
        }
    }

  g_strfreev (str);

  if (!found)
    {
      *new_val = 0.0;
      return GTK_INPUT_ERROR;
    }

  return TRUE;
}


static gint
hbtk_monthyear_spin_output (GtkSpinButton *spin_button)
{
  GtkAdjustment *adjustment;
  gchar *buf;
  gint month;
  gint year;
  gint retval = TRUE;

  adjustment = gtk_spin_button_get_adjustment (spin_button);
  month = ((gint)gtk_adjustment_get_value (adjustment) % 12);
  year = (gint)gtk_adjustment_get_value (adjustment) / 12.0;

  //g_print(" output: %d => m:%d y:%d\n", (gint)gtk_adjustment_get_value (adjustment), month, year);

  buf = g_strdup_printf ("%s %04d", _(CYA_ABMONTHS[month+1]), year+1900);

  //g_signal_handlers_block_by_func(spin_button, time_spin_input, NULL);

  if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
  {
	//g_print("  update text '%s'\n", buf);
    gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
    retval = TRUE;
	}

	//g_signal_handlers_unblock_by_func(spin_button, time_spin_input, NULL);

  g_free (buf);

  return retval;
}


static guint32 hbtk_monthyear_get_internal(GtkSpinButton *spin, guint type)
{
GDate date;
guint month, year;
gint val;

	if(!GTK_IS_SPIN_BUTTON(spin))
		return GLOBALS->today;

	val = gtk_spin_button_get_value_as_int(spin);
	year  = 1900 + (val / 12);
	month = (val % 12) + 1;
	
	g_date_clear(&date, 1);
	g_date_set_year(&date, year);
	g_date_set_month(&date, month);
	if(type == 0)
		g_date_set_day(&date, 1);
	else
		g_date_set_day(&date, g_date_get_days_in_month(month, year));

	return g_date_get_julian(&date);
}


guint32 hbtk_monthyear_getmin(GtkSpinButton *spin)
{
	return hbtk_monthyear_get_internal(spin, 0);
}

guint32 hbtk_monthyear_getmax(GtkSpinButton *spin)
{
	return hbtk_monthyear_get_internal(spin, 1);
}


void hbtk_monthyear_set(GtkSpinButton *spin, guint32 julian)
{
GDate date;
gdouble newval;

	if(!GTK_IS_SPIN_BUTTON(spin))
		return;

	g_date_set_julian(&date, julian);
	newval = (g_date_get_month(&date)-1) + (g_date_get_year(&date) - 1900) * 12;
	gtk_spin_button_set_value(spin, newval);
}


GtkWidget *make_monthyear(GtkWidget *label)
{
GtkWidget *spinner;
GtkAdjustment *adj;

	adj = (GtkAdjustment *) gtk_adjustment_new (0, 0, (2200-1900)*12, 1, 12, 0);
	//adj = (GtkAdjustment *) gtk_adjustment_new (0, 0, 14010, 30, 60, 0);
	spinner = gtk_spin_button_new (adj, 0, 0);
	//gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	//gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
	//g_object_set(spinner, "xalign", 1.0, NULL);

	//#2081574 width too small
	gtk_entry_set_width_chars(GTK_ENTRY(spinner), 10);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), spinner);

	g_signal_connect (spinner, "output", G_CALLBACK (hbtk_monthyear_spin_output), NULL);
	g_signal_connect (spinner, "input", G_CALLBACK (hbtk_monthyear_spin_input), NULL);

	return spinner;
}


/*
GtkWidget *make_year(GtkWidget *label)
{
GtkWidget *spinner;
GtkAdjustment *adj;

	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 1900, 2200, 1.0, 10.0, 0.0);
	spinner = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
	g_object_set(spinner, "xalign", 1.0, NULL);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), spinner);

	return spinner;
}*/


GtkWidget *
create_popover (GtkWidget       *parent,
                GtkWidget       *child,
                GtkPositionType  pos)
{
GtkWidget *popover;

	popover = gtk_popover_new (parent);
	gtk_popover_set_position (GTK_POPOVER (popover), pos);
	gtk_popover_set_child (GTK_POPOVER(popover), child);
	gtk_widget_show (child);

	hb_widget_set_margin(child, SPACING_POPOVER);
	
	return popover;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


#ifdef G_OS_WIN32

static GtkWidget *hbtk_container_get_children_named(GtkContainer *container, gchar *buildname)
{
GList *lchild, *list;
GtkWidget *widget = NULL;
gint i;

	if(!GTK_IS_CONTAINER(container))
		return NULL;
	
	lchild = list = gtk_container_get_children (container);
	for(i=0;list != NULL;i++)
	{
		if( hb_string_ascii_compare(buildname, (gchar *)gtk_buildable_get_name(list->data)) == 0 )
		{
			widget = list->data;
			break;
		}
		list = g_list_next(list);
	}
	g_list_free(lchild);

	return widget;
}


void hbtk_assistant_hack_button_order(GtkAssistant *assistant)
{
GtkWidget *bmain, *bchild, *barea, *bbutton;

	//main_box
	bmain = gtk_bin_get_child(GTK_BIN(assistant));
	if( !bmain ) return;
	DB( g_print(" got %s\n", gtk_buildable_get_name(bmain)) );
	
	bchild = hbtk_container_get_children_named(GTK_CONTAINER(bmain), "content_box");
	if( !bchild ) return;
	DB( g_print(" got %s\n", gtk_buildable_get_name(bchild)) );

	barea = hbtk_container_get_children_named(GTK_CONTAINER(bchild), "action_area");
	if( !barea ) return;
	DB( g_print(" got %s\n", gtk_buildable_get_name(barea)) );

	//assistant buttonbox is GTK_PACK_END
	bbutton = hbtk_container_get_children_named(GTK_CONTAINER(barea), "back");
	if(bbutton) gtk_box_reorder_child(GTK_BOX(barea), bbutton, 5);
	bbutton = hbtk_container_get_children_named(GTK_CONTAINER(barea), "forward");
	if(bbutton) gtk_box_reorder_child(GTK_BOX(barea), bbutton, 4);
	bbutton = hbtk_container_get_children_named(GTK_CONTAINER(barea), "apply");
	if(bbutton) gtk_box_reorder_child(GTK_BOX(barea), bbutton, 3);
	bbutton = hbtk_container_get_children_named(GTK_CONTAINER(barea), "last");
	if(bbutton) gtk_box_reorder_child(GTK_BOX(barea), bbutton, 2);
	bbutton = hbtk_container_get_children_named(GTK_CONTAINER(barea), "cancel");
	if(bbutton) gtk_box_reorder_child(GTK_BOX(barea), bbutton, 1);
	bbutton = hbtk_container_get_children_named(GTK_CONTAINER(barea), "close");
	if(bbutton) gtk_box_reorder_child(GTK_BOX(barea), bbutton, 0);
}

#endif


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


#define HB_KV_BUFFER_MAX_LEN	16
#define HB_KV_ITEMS_MAX_LEN		32

gchar *hbtk_get_label(HbKvData *kvdata, guint32 key)
{
gchar *retval = NULL;
guint32 i;

	for(i=0;i<HB_KV_ITEMS_MAX_LEN;i++)
	{
	HbKvData *tmp = &kvdata[i];
		if( tmp->name == NULL )
			break;
		if( tmp->key == key )
		{
			//#1820372
			retval = (gchar *)_(tmp->name);
			break;
		}
	}
	return retval;
}


static gboolean hbtk_combo_box_is_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
//GtkTreePath *path;
gboolean retval;
gchar *txt;

	gtk_tree_model_get (model, iter, 0, &txt, -1);
	retval = *txt == 0 ? TRUE : FALSE;
	//leak
	g_free(txt);

	return retval;
}


guint32 hbtk_combo_box_get_active_id (GtkComboBox *combobox)
{
const gchar* buf;
guint32 retval;
	
	buf = gtk_combo_box_get_active_id(GTK_COMBO_BOX(combobox));
	retval = buf != NULL ? atoi(buf) : 0;

	return retval;
}


void hbtk_combo_box_set_active_id (GtkComboBox *combobox, guint32 key)
{
gchar buf[HB_KV_BUFFER_MAX_LEN];

	g_snprintf(buf, HB_KV_BUFFER_MAX_LEN-1, "%d", key);
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(combobox), buf);
}


void hbtk_combo_box_text_append (GtkComboBox *combobox, guint32 key, gchar *text)
{
gchar buf[HB_KV_BUFFER_MAX_LEN];

	g_snprintf(buf, HB_KV_BUFFER_MAX_LEN-1, "%d", key);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combobox), buf, text);
}


GtkWidget *hbtk_combo_box_new (GtkWidget *label)
{
GtkWidget *combobox;

	combobox = gtk_combo_box_text_new();
	
	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), combobox);

	return combobox;
}


GtkWidget *hbtk_combo_box_new_with_data (GtkWidget *label, HbKvData *kvdata)
{
GtkWidget *combobox = hbtk_combo_box_new(label);
HbKvData *tmp;
gboolean hassep;
guint32 i;

	hassep = FALSE;
	for(i=0;i<HB_KV_ITEMS_MAX_LEN;i++)
	{
		tmp = &kvdata[i];
		if( tmp->name == NULL )
			break;
		if( *tmp->name != 0 )
		{
			hbtk_combo_box_text_append(GTK_COMBO_BOX(combobox), tmp->key, (gchar *)_(tmp->name));
		}
		else
		{
			hbtk_combo_box_text_append(GTK_COMBO_BOX(combobox), tmp->key, (gchar *)"");
			hassep = TRUE;
		}
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	if(hassep)
		gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combobox), hbtk_combo_box_is_separator, NULL, NULL);

	return combobox;
}


GtkWidget *hbtk_combo_box_new_with_array (GtkWidget *label, gchar **items)
{
GtkWidget *combobox = hbtk_combo_box_new(label);
guint32 i;

	for (i = 0; items[i] != NULL; i++)
	{
		if(*items[i] != 0)
			hbtk_combo_box_text_append(GTK_COMBO_BOX(combobox), i, (gchar *)_(items[i]));
		else
			hbtk_combo_box_text_append(GTK_COMBO_BOX(combobox), i, (gchar *)"");
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	return combobox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void
make_daterange_set_sensitive (GtkCellLayout   *cell_layout,
	       GtkCellRenderer *cell,
	       GtkTreeModel    *tree_model,
	       GtkTreeIter     *iter,
	       gpointer         data)
{
gint id_column = GPOINTER_TO_INT(data);
gboolean sensitive;
gchar *textid;
gint id;

	g_return_if_fail (id_column >= 0);
	gtk_tree_model_get (tree_model, iter, id_column, &textid, -1);
	id = atoi(textid);
	sensitive = (id != FLT_RANGE_MISC_CUSTOM);
	g_object_set (cell, "sensitive", sensitive, NULL);
	g_free(textid);
}


static void make_daterange_fill_items(GtkComboBox *combo_box, HbKvData *kvdata)
{
HbKvData *tmp;
guint32 i;

	for(i=0;i<HB_KV_ITEMS_MAX_LEN;i++)
	{
		tmp = &kvdata[i];
		if( tmp->name == NULL )
			break;
		hbtk_combo_box_text_append(GTK_COMBO_BOX(combo_box), tmp->key, (*tmp->name != 0) ? (gchar *)_(tmp->name) : (gchar *)"");
	}
}


GtkWidget *make_daterange(GtkWidget *label, HbDateRangeFlags flags)
{
GtkWidget *combo_box;
GList *renderers, *list;

	combo_box = hbtk_combo_box_new(label);

	if( !(flags & DATE_RANGE_FLAG_BUDGET_MODE) )
		make_daterange_fill_items(GTK_COMBO_BOX(combo_box), CYA_FLT_RANGE_DWF);

	make_daterange_fill_items(GTK_COMBO_BOX(combo_box), CYA_FLT_RANGE_MQY);

	make_daterange_fill_items(GTK_COMBO_BOX(combo_box), CYA_FLT_RANGE_YTO);

	if( !(flags & DATE_RANGE_FLAG_BUDGET_MODE) )
		make_daterange_fill_items(GTK_COMBO_BOX(combo_box), CYA_FLT_RANGE_LASTXXD);

	make_daterange_fill_items(GTK_COMBO_BOX(combo_box), CYA_FLT_RANGE_COMMON);

	if( !(flags & DATE_RANGE_FLAG_CUSTOM_HIDDEN) )
		make_daterange_fill_items(GTK_COMBO_BOX(combo_box), CYA_FLT_RANGE_CUSTOM);

	if( flags & DATE_RANGE_FLAG_CUSTOM_DISABLE )
	{
		renderers = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT(combo_box));
		if(g_list_length(renderers) == 1)
		{
		gint id_column = gtk_combo_box_get_id_column (GTK_COMBO_BOX (combo_box));
		
			list = g_list_first(renderers);
			gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo_box),
							list->data,
							make_daterange_set_sensitive,
							GINT_TO_POINTER(id_column),
							NULL);
		}	
		g_list_free(renderers);
	}

	//TODO: option removed into GTK4 ??
	gtk_combo_box_set_wrap_width (GTK_COMBO_BOX(combo_box), 3);

	//gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), FLT_RANGE_MISC_ALLDATE);
	//5.8.6 leave unset to trigger a change
	//hbtk_combo_box_set_active_id(GTK_COMBO_BOX(combo_box), FLT_RANGE_MISC_ALLDATE);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo_box), hbtk_combo_box_is_separator, NULL, NULL);

	return combo_box;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static HbKivData *hb_kvidata_get_by_key(HbKivData *kivdata, guint32 key)
{
HbKivData *tmp = NULL;
guint32 i;

	for(i=0;i<HB_KV_ITEMS_MAX_LEN;i++)
	{
		tmp = &kivdata[i];
		if( tmp->name == NULL )
			break;
		if( tmp->key == key )
			break;
	}
	return tmp;
}


HbKivData CYA_TXN_GRPFLAG[NUM_GRPFLAG] = 
{
//	{ GRPFLAG_NONE,			"hb-gf-none",		N_("(none)") },
	{ GRPFLAG_RED,			"hb-gf-red",		N_("Red") },
	{ GRPFLAG_ORANGE,		"hb-gf-orange",		N_("Orange") },
	{ GRPFLAG_YELLOW,		"hb-gf-yellow",		N_("Yellow") },
	{ GRPFLAG_GREEN,		"hb-gf-green",		N_("Green") },
	{ GRPFLAG_BLUE,			"hb-gf-blue",		N_("Blue") },
	{ GRPFLAG_PURPLE,		"hb-gf-purple",		N_("Purple") },

	{ 0, NULL , NULL }
};



const gchar *get_grpflag_icon_name(guint32 key)
{
HbKivData *tmp = hb_kvidata_get_by_key(CYA_TXN_GRPFLAG, key);
	return (tmp != NULL) ? tmp->iconname : NULL;
}


GtkWidget *make_fltgrpflag(GtkWidget *label)
{
GtkWidget *combo_box;
HbKivData *tmp = NULL;
guint32 i;

	combo_box = hbtk_combo_box_new(label);

	hbtk_combo_box_text_append(GTK_COMBO_BOX(combo_box), GRPFLAG_ANY, _("Any flag"));

	for(i=0;i<HB_KV_ITEMS_MAX_LEN;i++)
	{
		tmp = &CYA_TXN_GRPFLAG[i];
		if( tmp->name == NULL )
			break;
		hbtk_combo_box_text_append(GTK_COMBO_BOX(combo_box), tmp->key, (*tmp->name != 0) ? (gchar *)_(tmp->name) : (gchar *)"");
	}
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(combo_box), GRPFLAG_ANY);
	return combo_box;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/* OFX <TRNTYPE> transactiontype

id  ofx  			english                           french
 ---------------------------------------------------------------------
 0	--------      	(none)                            (aucun) 
 1	--------      	credit card                       carte de crédit
 2	OFX_CHECK     	Check                             cheque
 3  OFX_CASH      	Cash  	withdrawal                retrait espece
 4	OFX_XFER      	Transfer                          virement
 5	--------      	internal transfer                 virement compte 
 6	--------      	(debit card)                      (carte de paiement)
 7	OFX_REPEATPMT 	Repeating payment/standing order  Paiement recurrent/Virement auto.
		                                                   
 8	OFX_PAYMENT   	Electronic payment                télépaiement
 9	OFX_DEP 	    Deposit                           dépôt
10	OFX_FEE       	FI fee                            frais bancaires

		                                               
	 OFX_DIRECTDEBIT 	Merchant initiated debit     prelevement
	 OFX_OTHER 	Some other type of transaction      autre

other OFX values:

OFX_CREDIT 	Generic credit
OFX_DEBIT 	Generic debit
OFX_INT 	Interest earned or paid (Note: Depends on signage of amount)
OFX_DIV 	Dividend
OFX_SRVCHG 	Service charge
-OFX_DEP 	Deposit
OFX_ATM 	ATM debit or credit (Note: Depends on signage of amount)
OFX_POS 	Point of sale debit or credit (Note: Depends on signage of amount)
-OFX_XFER 	Transfer
-OFX_CHECK 	Check
-OFX_PAYMENT 	Electronic payment
-OFX_CASH 	Cash withdrawal
OFX_DIRECTDEP 	Direct deposit
OFX_DIRECTDEBIT 	Merchant initiated debit
-OFX_REPEATPMT 	Repeating payment/standing order
OFX_OTHER 	Somer other type of transaction 
*/

enum
{
	LST_PAYMODE_KEY,
	LST_PAYMODE_ICONNAME,
	LST_PAYMODE_LABEL,
	LST_PAYMODE_ONOFF,
	NUM_LST_PAYMODE
};


HbKivData CYA_TXN_PAYMODE[NUM_PAYMODE_MAX] = 
{
	{ PAYMODE_NONE,			"hb-pm-none",		N_("(none)") },
	{ PAYMODE_CCARD,		"hb-pm-ccard", 		N_("Credit card") },
	{ PAYMODE_CHECK,		"hb-pm-check", 		N_("Check") },
	{ PAYMODE_CASH,			"hb-pm-cash" , 		N_("Cash") },
	{ PAYMODE_XFER,			"hb-pm-transfer", 	N_("Bank Transfer") },
	{ PAYMODE_DCARD,		"hb-pm-dcard", 		N_("Debit card") },
	{ PAYMODE_REPEATPMT,	"hb-pm-standingorder",	N_("Standing order") },
	{ PAYMODE_EPAYMENT,		"hb-pm-epayment", 	N_("Electronic payment") },
	{ PAYMODE_DEPOSIT,		"hb-pm-deposit", 	N_("Deposit") },
	//TRANSLATORS: Financial institution fee
	{ PAYMODE_FEE,			"hb-pm-fifee", 		N_("FI fee") },
	{ PAYMODE_DIRECTDEBIT,	"hb-pm-directdebit", N_("Direct Debit") },
	//#1817274
	{ PAYMODE_MOBPHONE,		"hb-pm-mobphone",	N_("Mobile Phone") },
	
	{ 0, NULL , NULL }
};


/* nota: used in ui-filter */
const gchar *get_paymode_icon_name(guint32 key)
{
HbKivData *tmp = NULL;
/*HbKivData *kivdata = CYA_TXN_PAYMODE;
guint32 i;

	for(i=0;i<HB_KV_ITEMS_MAX_LEN;i++)
	{
	HbKivData *tmp = &kivdata[i];
		if( tmp->name == NULL )
			break;
		if( tmp->key == key )
		{
			retval = tmp->iconname;
			break;
		}
	}*/
	tmp = hb_kvidata_get_by_key(CYA_TXN_PAYMODE, key);
	return (tmp != NULL) ? tmp->iconname : NULL;
}


guint32 paymode_combo_box_get_active (GtkComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

	model = gtk_combo_box_get_model (combo_box);

	if (gtk_combo_box_get_active_iter (combo_box, &iter))
    {
	gint key;
		
      gtk_tree_model_get (model, &iter, LST_PAYMODE_KEY, &key, -1);

      return (guint32)key;
    }


	return 0;
}


void paymode_combo_box_set_active (GtkComboBox *combo_box, guint32 active_key)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

	model = gtk_combo_box_get_model (combo_box);
	
	  if (gtk_tree_model_get_iter_first (model, &iter))
    do {
	gint key;
		  
      gtk_tree_model_get (model, &iter, LST_PAYMODE_KEY, &key, -1);

      if (key == (gint)active_key)
        {
          gtk_combo_box_set_active_iter (combo_box, &iter);
          break;
        }
    } while (gtk_tree_model_iter_next (model, &iter));

}


void paymode_list_get_order(GtkTreeView *treeview)
{
GtkTreeModel *model;
GtkTreeIter iter;
gint i = 1;

	model = gtk_tree_view_get_model (treeview);

	PREFS->lst_paymode[0] = PAYMODE_NONE;
	if (gtk_tree_model_get_iter_first (model, &iter))
	do {
	guint32 key;
	gboolean active;

		gtk_tree_model_get (model, &iter, 
		LST_PAYMODE_KEY, &key,
		LST_PAYMODE_ONOFF, &active,
		-1);
		
		PREFS->lst_paymode[i++] = (active == TRUE) ? key : -key;

	} while (gtk_tree_model_iter_next (model, &iter));
}


static GtkListStore *
paymode_store_new(gboolean prefmode)
{
GtkListStore *store;
HbKivData *tmp;
GtkTreeIter iter;
guint i;

	store = gtk_list_store_new (
		NUM_LST_PAYMODE,
		G_TYPE_INT,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_BOOLEAN
		);

	//populate our combobox model
	for(i=0 ; i<NUM_PAYMODE_KEY ; i++)
	{
	gint key = PREFS->lst_paymode[i];

		//hide None for prefmode
		if( prefmode == TRUE && key == 0 )
			continue;

		tmp = hb_kvidata_get_by_key(CYA_TXN_PAYMODE, ABS(key));
		if( tmp != NULL )
		{
			if( prefmode == FALSE && key < 0 )
				continue;

			gtk_list_store_insert_with_values(store, &iter, -1,
				LST_PAYMODE_KEY, tmp->key,
				LST_PAYMODE_ICONNAME, tmp->iconname,
				LST_PAYMODE_LABEL, _(tmp->name),
				LST_PAYMODE_ONOFF, key > 0 ? TRUE : FALSE,
				-1);
		}
	}
	return store;
}


static void
fixed_toggled (GtkCellRendererToggle *cell,
               gchar                 *path_str,
               gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, LST_PAYMODE_ONOFF, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_PAYMODE_ONOFF, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}



GtkWidget *make_paymode_list(void)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer *renderer;
GtkTreeViewColumn *column;

	store = paymode_store_new(TRUE);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));

	/* column for fixed toggles */
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), store);

	column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "active", LST_PAYMODE_ONOFF,
                                                     NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
	//gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "icon-name",
                                                     LST_PAYMODE_ICONNAME,
                                                     NULL);
  //gtk_tree_view_column_set_sort_column_id (column, LST_PAYMODE_ICONNAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  /* column for description */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     LST_PAYMODE_LABEL,
                                                     NULL);
  //gtk_tree_view_column_set_sort_column_id (column, LST_PAYMODE_LABEL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (treeview), FALSE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW(treeview), TRUE);
	
	return GTK_WIDGET(treeview);
}



/*
** Make a paymode combobox widget
*/
GtkWidget *make_paymode(GtkWidget *label)
{
GtkListStore *store;
GtkWidget *combobox;
GtkCellRenderer *renderer, *r1, *r2;

	store = paymode_store_new(FALSE);

	combobox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	//leak
	g_object_unref(store);

	renderer = r1 = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combobox), renderer, "icon-name", LST_PAYMODE_ICONNAME);

	renderer = r2 = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xpad", SPACING_SMALL, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combobox), renderer, "text", LST_PAYMODE_LABEL);


	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), combobox);

	return combobox;
}

