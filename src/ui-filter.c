/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2024 Maxime DOYEN
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

#include "ui-filter.h"
#include "ui-account.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-tag.h"
#include "hbtk-switcher.h"
#include "gtk-dateentry.h"


/****************************************************************************/
/* Debug macros										 */
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


extern gchar *CYA_FLT_TYPE[];
extern gchar *CYA_FLT_STATUS[];
extern gchar *CYA_SELECT[];

extern gchar *RA_FILTER_MODE[];

extern HbKivData CYA_TXN_PAYMODE[NUM_PAYMODE_MAX];


/* = = = = = = = = = = = = = = = = = = = = */


static void
ui_flt_manage_cb_range_change(GtkWidget *widget, gpointer user_data);

static guint 
_gtkentry_to_filter(GtkEntry *entry, gchar **storage)
{
const gchar *txt;
guint change = 0;

	if(!GTK_IS_ENTRY(entry))
		return 0;

	txt = gtk_entry_get_text(GTK_ENTRY(entry));
	if( g_strcmp0(txt, *storage) != 0 )
	{
		change++;
		g_free(*storage);
		*storage = g_strdup(txt);
	}
	return change;
}


/* = = = = = = = = = = = = = = = = = = = = */


static void ui_flt_hub_tag_set(Filter *flt, struct ui_flt_manage_data *data)
{

	DB( g_print("(ui_flt_hub_tag) set\n") );

	if(data->filter != NULL)
	{
	GtkTreeModel *model;
	//GtkTreeSelection *selection;
	GtkTreeIter	iter;
	gboolean valid;
	gint i;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_tag));
		//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_tag));
		i=0; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
		while (valid)
		{
		Tag *tagitem;
		gboolean status;

			gtk_tree_model_get (model, &iter,
				LST_DEFTAG_DATAS, &tagitem,
				-1);

			status = da_flt_status_tag_get(flt, tagitem->key);
			DB( g_print(" set tag k:%3d = %d (%s)\n", tagitem->key, status, tagitem->name) );
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFTAG_TOGGLE, status, -1);

			/* Make iter point to the next row in the list store */
			i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
		}


	}
}


static gboolean ui_flt_hub_tag_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
	DB( g_print("(ui_flt_hub_tag) activate_link\n") );
	
	g_return_val_if_fail(GTK_IS_TREE_VIEW(user_data), TRUE);
	ui_tag_listview_quick_select(user_data, uri );
    return TRUE;
}


/* = = = = = = = = = = = = = = = = = = = = */


//#1828732 add expand/collapse all for categories in edit filter
static void ui_flt_hub_category_expand_all(GtkWidget *widget, gpointer user_data)
{
struct ui_flt_manage_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_flt_hub_category) expand all (data=%p)\n", data) );

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_cat));

}


static void ui_flt_hub_category_collapse_all(GtkWidget *widget, gpointer user_data)
{
struct ui_flt_manage_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n(ui_flt_hub_category) collapse all (data=%p)\n", data) );

	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data->LV_cat));

}


static void ui_flt_hub_category_set(Filter *flt, struct ui_flt_manage_data *data)
{

	DB( g_print("(ui_flt_hub_category) set\n") );

	if(data->filter != NULL)
	{
	GtkTreeModel *model;
	//GtkTreeSelection *selection;
	GtkTreeIter	iter, child;

	gint n_child;
	gboolean valid;
	gint i;


	// category
		DB( g_print(" category\n") );

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_cat));
		//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat));
		i=0; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
		while (valid)
		{
		Category *catitem;
		gboolean status;

			gtk_tree_model_get (model, &iter,
				LST_DEFCAT_DATAS, &catitem,
				-1);

			status = da_flt_status_cat_get(flt, catitem->key);
			gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LST_DEFCAT_TOGGLE, status, -1);

			DB( g_print("  set %d to '%s' %d\n", status, catitem->name, catitem->key) );

			n_child = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(model), &iter);
			gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
			while(n_child > 0)
			{
				i++;

				gtk_tree_model_get (model, &child,
					LST_DEFCAT_DATAS, &catitem,
					-1);

				status = da_flt_status_cat_get(flt, catitem->key);
				gtk_tree_store_set (GTK_TREE_STORE (model), &child, LST_DEFCAT_TOGGLE, status, -1);

				DB( g_print("  set %d to '%s' %d\n", status, catitem->name, catitem->key) );

				n_child--;
				gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
			}

			/* Make iter point to the next row in the list store */
			i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
		}


	}
}


static gboolean ui_flt_hub_category_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
	DB( g_print("(ui_flt_hub_category) activate_link\n") );
	
	g_return_val_if_fail(GTK_IS_TREE_VIEW(user_data), TRUE);
	ui_cat_listview_quick_select(user_data, uri );
    return TRUE;
}


/* = = = = = = = = = = = = = = = = */


static void ui_flt_hub_payee_set(Filter *flt, struct ui_flt_manage_data *data)
{

	DB( g_print("(ui_flt_hub_payee) set\n") );

	if(data->filter != NULL)
	{
	GtkTreeModel *model;
	//GtkTreeSelection *selection;
	GtkTreeIter	iter;
	gboolean valid;
	gint i;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_pay));
		//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_pay));
		i=0; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
		while (valid)
		{
		Payee *payitem;
		gboolean status;

			gtk_tree_model_get (model, &iter,
				LST_DEFPAY_DATAS, &payitem,
				-1);

			status = da_flt_status_pay_get(flt, payitem->key);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFPAY_TOGGLE, status, -1);

			DB( g_print("  set %d to '%s' %d\n", status, payitem->name, payitem->key) );

			/* Make iter point to the next row in the list store */
			i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
		}


	}
}


static gboolean ui_flt_hub_payee_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
	DB( g_print("(ui_flt_hub_payee) activate_link\n") );
	
	g_return_val_if_fail(GTK_IS_TREE_VIEW(user_data), TRUE);
	ui_pay_listview_quick_select(user_data, uri );
    return TRUE;
}


/* = = = = = = = = = = = = = = = = */




static void ui_flt_hub_account_set(Filter *flt, struct ui_flt_manage_data *data)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gint i;

	DB( g_print("(ui_flt_hub_account) set\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc));
	//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc));
	i=0; valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Account *accitem;
	gboolean status;

		gtk_tree_model_get (model, &iter,
			LST_DEFACC_DATAS, &accitem,
			-1);

		status = da_flt_status_acc_get(flt, accitem->key);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				LST_DEFACC_TOGGLE, status, -1);

		/* Make iter point to the next row in the list store */
		i++; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static gboolean ui_flt_hub_account_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
	DB( g_print("(ui_flt_hub_account) activate_link\n") );

	g_return_val_if_fail(GTK_IS_TREE_VIEW(user_data), TRUE);
	ui_acc_listview_quick_select(GTK_TREE_VIEW(user_data), uri);
    return TRUE;
}


/* = = = = = = = = = = = = = = = = */


static void ui_flt_manage_update_page(gint pageidx, const gchar *pagename, struct ui_flt_manage_data *data)
{
GtkWidget *child;
GValue gvalue = G_VALUE_INIT;
gboolean visible;

	g_value_init (&gvalue, G_TYPE_BOOLEAN);

	visible = (!gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[pageidx]))) ? FALSE : TRUE; 
	gtk_widget_set_sensitive(data->RA_matchmode[pageidx], visible);
	gtk_widget_set_sensitive(data->GR_page[pageidx], visible);

	g_value_set_boolean (&gvalue, visible);
	child = gtk_stack_get_child_by_name(GTK_STACK(data->stack), pagename);
	gtk_container_child_set_property(GTK_CONTAINER(data->stack), child, "needs-attention", &gvalue);
}


static void ui_flt_manage_update(GtkWidget *widget, gpointer user_data)
{
struct ui_flt_manage_data *data;
gboolean sensitive, visible, v1, v2;
GtkWidget *child;
GValue gvalue = G_VALUE_INIT;
gint range;


	DB( g_print("\n[ui_flt_manage] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));

	g_value_init (&gvalue, G_TYPE_BOOLEAN);

	// date
	gtk_widget_set_sensitive(data->SW_enabled[FLT_GRP_DATE], FALSE);
	ui_flt_manage_update_page(FLT_GRP_DATE, FLT_PAGE_NAME_DAT, data);
	sensitive = (range == FLT_RANGE_MISC_CUSTOM) ? TRUE : FALSE;
	gtk_widget_set_sensitive(GTK_WIDGET(data->LB_mindate), sensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(data->LB_maxdate), sensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(data->PO_mindate), sensitive);
	gtk_widget_set_sensitive(GTK_WIDGET(data->PO_maxdate), sensitive);

	// type
	visible = (!gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[FLT_GRP_TYPE]))) ? FALSE : TRUE; 
	gtk_widget_set_sensitive(data->RA_matchmode[FLT_GRP_TYPE], visible);
	gtk_widget_set_sensitive(data->GR_page[FLT_GRP_TYPE], visible);
	g_value_set_boolean (&gvalue, visible);
	child = gtk_stack_get_child_by_name(GTK_STACK(data->stack), FLT_PAGE_NAME_TYP);
	gtk_container_child_set_property(GTK_CONTAINER(data->stack), child, "needs-attention", &gvalue);

	// status
	visible = (!gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[FLT_GRP_STATUS]))) ? FALSE : TRUE;
	gtk_widget_set_sensitive(data->RA_matchmode[FLT_GRP_STATUS], visible);	
	gtk_widget_set_sensitive(data->GR_page[FLT_GRP_STATUS], visible);
	g_value_set_boolean (&gvalue, visible);
	child = gtk_stack_get_child_by_name(GTK_STACK(data->stack), FLT_PAGE_NAME_STA);
	gtk_container_child_set_property(GTK_CONTAINER(data->stack), child, "needs-attention", &gvalue);

	// account
	if(data->show_account == TRUE)
		ui_flt_manage_update_page(FLT_GRP_ACCOUNT, FLT_PAGE_NAME_ACC, data);

	// amount/text
	v1 = (!gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[FLT_GRP_AMOUNT]))) ? FALSE : TRUE; 
	gtk_widget_set_sensitive(data->RA_matchmode[FLT_GRP_AMOUNT], v1);
	gtk_widget_set_sensitive(data->GR_page[FLT_GRP_AMOUNT], v1);

	v2 = (!gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[FLT_GRP_TEXT]))) ? FALSE : TRUE; 
	gtk_widget_set_sensitive(data->RA_matchmode[FLT_GRP_TEXT], v2);
	gtk_widget_set_sensitive(data->GR_page[FLT_GRP_TEXT], v2);

	visible = v1 | v2;
	g_value_set_boolean (&gvalue, visible);
	child = gtk_stack_get_child_by_name(GTK_STACK(data->stack), FLT_PAGE_NAME_TXT);
	gtk_container_child_set_property(GTK_CONTAINER(data->stack), child, "needs-attention", &gvalue);

	// payee
	ui_flt_manage_update_page(FLT_GRP_PAYEE, FLT_PAGE_NAME_PAY, data);

	// category
	ui_flt_manage_update_page(FLT_GRP_CATEGORY, FLT_PAGE_NAME_CAT, data);

	// tag
	ui_flt_manage_update_page(FLT_GRP_TAG, FLT_PAGE_NAME_TAG, data);

	// payment
	ui_flt_manage_update_page(FLT_GRP_PAYMODE, FLT_PAGE_NAME_PMT, data);
	//v1 = (!gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[FLT_GRP_PAYMODE]))) ? FALSE : TRUE; 
	//gtk_widget_set_sensitive(data->GR_page[FLT_GRP_PAYMODE], v1);

}


static void
ui_flt_manage_get_option(struct ui_flt_manage_data *data, gint index)
{
gint newoption = gtk_switch_get_active(GTK_SWITCH(data->SW_enabled[index]));

	//option should be set: 0=off, 1=include, 2=exclude
	if( newoption == 1 )
	{
		if( hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_matchmode[index])) == 1)
			newoption++;
	}
	data->filter->option[index] = newoption;
}


static void ui_flt_manage_get(struct ui_flt_manage_data *data)
{
Filter *flt = data->filter;
gchar *olddigest, *newdigest;
guint i, length;
gboolean active;

	DB( g_print("\n[ui_flt_manage] get\n") );

	if(flt != NULL)
	{
		//TODO: 5.8 we should count changes into flt->nbchanges
		length = offsetof(Filter, exact);
		DB( g_print(" length: %d\n", length) );
		
		//use a checksum for non-pointer data
		olddigest = g_compute_checksum_for_data (G_CHECKSUM_MD5, (const guchar *)flt, length);
	
		DB( g_print(" option\n") );
		ui_flt_manage_get_option(data, FLT_GRP_DATE);
		ui_flt_manage_get_option(data, FLT_GRP_TYPE);
		ui_flt_manage_get_option(data, FLT_GRP_STATUS);
		ui_flt_manage_get_option(data, FLT_GRP_PAYEE);
		ui_flt_manage_get_option(data, FLT_GRP_CATEGORY);
		ui_flt_manage_get_option(data, FLT_GRP_TAG);
		if(data->show_account == TRUE)
			ui_flt_manage_get_option(data, FLT_GRP_ACCOUNT);
		ui_flt_manage_get_option(data, FLT_GRP_TEXT);
		ui_flt_manage_get_option(data, FLT_GRP_AMOUNT);
		ui_flt_manage_get_option(data, FLT_GRP_PAYMODE);

	//date
		DB( g_print(" date\n") );
		//5.8 date off means show all date
		if( data->filter->option[FLT_GRP_DATE] == 0 )
		{
			data->filter->option[FLT_GRP_DATE] = 1;
			data->filter->range = FLT_RANGE_MISC_ALLDATE;
		}

		flt->range   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));
		flt->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
		flt->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	//type
		DB( g_print(" type\n") );
		flt->typ_nexp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_typnexp));
		flt->typ_ninc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_typninc));
		flt->typ_xexp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_typxexp));
		flt->typ_xinc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_typxinc));

	//status
		DB( g_print(" status\n") );
		flt->sta_non = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_stanon));
		flt->sta_clr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_staclr));
		flt->sta_rec = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_starec));

		flt->forceadd = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forceadd));
		flt->forcechg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forcechg));
		flt->forceremind  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forceremind));
		flt->forcevoid  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forcevoid));

	//paymode
		DB( g_print(" paymode\n") );
		for(i=0;i<NUM_PAYMODE_MAX;i++)
		{
		gint uid;
			
			if( !GTK_IS_TOGGLE_BUTTON(data->CM_paymode[i] ))
				continue;

			uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(data->CM_paymode[i]), "uid")); 
			flt->paymode[uid] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_paymode[i]));
		}
			
	//amount
		flt->minamount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_minamount));
		flt->maxamount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_maxamount));

	//5.8 we compute new checksum here to detect changes
		newdigest = g_compute_checksum_for_data (G_CHECKSUM_MD5, (const guchar *)flt, length);
		DB( g_print(" checksum: '%s'\n", olddigest) );
		DB( g_print(" checksum: '%s'\n", newdigest) );

		if (strcmp(olddigest, newdigest) )
		{
			flt->nbchanges++;
			DB( g_print(" > checksum differs\n") );
		}

		DB( g_print(" changes: %d (post checksum)\n", flt->nbchanges) );

		g_free (olddigest);
		g_free (newdigest);

	// data below need to detect/count change on their own

	//text:memo
	//text:info
		flt->nbchanges += _gtkentry_to_filter(GTK_ENTRY(data->ST_memo), &flt->memo);
		flt->nbchanges += _gtkentry_to_filter(GTK_ENTRY(data->ST_number), &flt->number);
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_exact));
		if( flt->exact != active )
		{
			flt->nbchanges++;
			flt->exact = active;
		}

		DB( g_print(" changes: %d (post memo/info)\n", flt->nbchanges) );


	// account
		if(data->show_account == TRUE)
		{
			flt->nbchanges += ui_acc_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_acc), flt);
			DB( g_print(" changes: %d (post acc)\n", flt->nbchanges) );
		}

	// payee
		flt->nbchanges += ui_pay_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_pay), flt);
		DB( g_print(" changes: %d (post pay)\n", flt->nbchanges) );

	// category
		flt->nbchanges += ui_cat_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_cat), flt);
		DB( g_print(" changes: %d (post cat)\n", flt->nbchanges) );

	// tag
		flt->nbchanges += ui_tag_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_tag), flt);
		DB( g_print(" changes: %d (post tag)\n", flt->nbchanges) );


	// active tab
		g_strlcpy(flt->last_tab, gtk_stack_get_visible_child_name(GTK_STACK(data->stack)), 8);
		DB( g_print(" page is '%s'\n", flt->last_tab) );

		

	}
}


static void ui_flt_manage_set_option(struct ui_flt_manage_data *data, gint index)
{
//option: 0=off, 1=include, 2=exclude
gint tmpoption = data->filter->option[index];
	
	gtk_switch_set_active(GTK_SWITCH(data->SW_enabled[index]), tmpoption);
	if( tmpoption == 2 )
	{
		hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_matchmode[index]), 1);
	}
}


static void ui_flt_manage_set(struct ui_flt_manage_data *data)
{
Filter *flt = data->filter;

	DB( g_print("\n[ui_flt_manage] set\n") );

	if(flt != NULL)
	{
	gint i;

		DB( g_print(" options\n") );

		ui_flt_manage_set_option(data, FLT_GRP_DATE);
		ui_flt_manage_set_option(data, FLT_GRP_TYPE);
		ui_flt_manage_set_option(data, FLT_GRP_STATUS);
		ui_flt_manage_set_option(data, FLT_GRP_PAYEE);
		ui_flt_manage_set_option(data, FLT_GRP_CATEGORY);
		ui_flt_manage_set_option(data, FLT_GRP_TAG);
		if(data->show_account == TRUE)
			ui_flt_manage_set_option(data, FLT_GRP_ACCOUNT);
		ui_flt_manage_set_option(data, FLT_GRP_TEXT);
		ui_flt_manage_set_option(data, FLT_GRP_AMOUNT);
		ui_flt_manage_set_option(data, FLT_GRP_PAYMODE);

		//DB( g_print(" setdate %d to %x\n", flt->mindate, data->PO_mindate) );
		//DB( g_print(" setdate %d to %x\n", 0, data->PO_mindate) );
	//date
		DB( g_print(" date\n") );
		//g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), flt->range);
		//g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
		
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), flt->mindate);
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), flt->maxdate);

	//type
		DB( g_print(" type\n") );
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_typnexp), flt->typ_nexp);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_typninc), flt->typ_ninc);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_typxexp), flt->typ_xexp);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_typxinc), flt->typ_xinc);

	//status
		DB( g_print(" status/type\n") );
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_stanon), flt->sta_non);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_staclr), flt->sta_clr);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_starec), flt->sta_rec);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forceadd), flt->forceadd);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forcechg), flt->forcechg);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forceremind), flt->forceremind);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forcevoid), flt->forcevoid);

	//paymode
		DB( g_print(" paymode\n") );

		for(i=0;i<NUM_PAYMODE_MAX;i++)
		{
		gint uid;
			
			if( !GTK_IS_TOGGLE_BUTTON(data->CM_paymode[i] ))
				continue;

			uid = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(data->CM_paymode[i]), "uid")); 
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_paymode[i]), flt->paymode[uid]);

		}

	//amount
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_minamount), flt->minamount);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_maxamount), flt->maxamount);

	//text
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_exact), flt->exact);
		gtk_entry_set_text(GTK_ENTRY(data->ST_number), (flt->number != NULL) ? flt->number : "");
		gtk_entry_set_text(GTK_ENTRY(data->ST_memo), (flt->memo != NULL) ? flt->memo : "");

	//account
		if(data->show_account == TRUE)
		{
			DB( g_print(" account\n") );
			ui_flt_hub_account_set(flt, data);
		}

	// payee
		ui_flt_hub_payee_set(flt, data);

	// category
		ui_flt_hub_category_set(flt, data);

	// tag
		ui_flt_hub_tag_set(flt, data);

	}
}


static void ui_flt_manage_setup(struct ui_flt_manage_data *data)
{

	DB( g_print("\n[ui_flt_manage] setup\n") );

	if(data->show_account == TRUE && data->LV_acc != NULL)
	{
		//gtk_tree_selection_set_mode(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc))), GTK_SELECTION_MULTIPLE);

		ui_acc_listview_populate(data->LV_acc, ACC_LST_INSERT_REPORT, NULL);
		//populate_view_acc(data->LV_acc, GLOBALS->acc_list, FALSE);
	}

	if(data->LV_pay)
	{
		//gtk_tree_selection_set_mode(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_pay))), GTK_SELECTION_MULTIPLE);

		ui_pay_listview_populate(data->LV_pay, NULL, TRUE);
		//populate_view_pay(data->LV_pay, GLOBALS->pay_list, FALSE);
	}

	if(data->LV_cat)
	{
		//gtk_tree_selection_set_mode(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat))), GTK_SELECTION_MULTIPLE);

		//populate_view_cat(data->LV_cat, GLOBALS->cat_list, FALSE);
		ui_cat_listview_populate(data->LV_cat, CAT_TYPE_ALL, NULL, TRUE);
		gtk_tree_view_expand_all (GTK_TREE_VIEW(data->LV_cat));
	}
	
	if(data->LV_tag)
	{
		ui_tag_listview_populate(data->LV_tag, 0);
	}
	
}


static gboolean
ui_flt_page_paymode_activate_link (GtkWidget   *label,
               const gchar *uri,
               gpointer     user_data)
{
struct ui_flt_manage_data *data;
gint i;
	
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(label, GTK_TYPE_WINDOW)), "inst_data");
	
	for(i=0;i<NUM_PAYMODE_MAX;i++)
	{
		if( !GTK_IS_TOGGLE_BUTTON(data->CM_paymode[i] ))
			continue;

		if (g_strcmp0 (uri, "all") == 0)	
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_paymode[i]), TRUE);
		}
		else
		if (g_strcmp0 (uri, "non") == 0)	
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_paymode[i]), FALSE);
		}
		else
		if (g_strcmp0 (uri, "inv") == 0)	
		{
		gboolean act = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_paymode[i]));
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_paymode[i]), !act);
		}
	}

    return TRUE;
}


static void
_checkdate_valid(struct ui_flt_manage_data *data)
{
gboolean valid = (data->filter->mindate <= data->filter->maxdate) ? TRUE : FALSE;

	//5.8 check for error
	gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), !valid);
	gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), !valid);

	//disable use if invalid date
	gtk_dialog_set_response_sensitive(GTK_DIALOG(data->dialog), GTK_RESPONSE_ACCEPT, valid);

}





static void
ui_flt_manage_cb_date_change(GtkWidget *widget, gpointer user_data)
{
struct ui_flt_manage_data *data;

	DB( g_print("\n[ui_flt_manage] min/max/date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	g_signal_handlers_block_by_func(data->CY_range, G_CALLBACK(ui_flt_manage_cb_range_change), NULL);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handlers_unblock_by_func(data->CY_range, G_CALLBACK(ui_flt_manage_cb_range_change), NULL);

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	//5.8 check for error
	_checkdate_valid(data);
}


static void
ui_flt_manage_cb_range_change(GtkWidget *widget, gpointer user_data)
{
struct ui_flt_manage_data *data;
gint range;

	//DB( g_print("\n[repdist] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));

	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);


		g_signal_handlers_block_by_func(data->PO_mindate, G_CALLBACK(ui_flt_manage_cb_date_change), NULL);
		g_signal_handlers_block_by_func(data->PO_maxdate, G_CALLBACK(ui_flt_manage_cb_date_change), NULL);
		
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);
		
		g_signal_handlers_unblock_by_func(data->PO_mindate, G_CALLBACK(ui_flt_manage_cb_date_change), NULL);
		g_signal_handlers_unblock_by_func(data->PO_maxdate, G_CALLBACK(ui_flt_manage_cb_date_change), NULL);

		//#2046032 set min/max date for both widget
		_checkdate_valid(data);

	}

	ui_flt_manage_update(widget, NULL);
	
}


/* = = = = = = = = = = = = = = = = */


static GtkWidget *ui_flt_page_misc_header(gchar *title, gint index, struct ui_flt_manage_data *data)
{
GtkWidget *grid, *label, *widget;
gint row;

	//header
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);

	row = 0;
	label = make_label_group(title);
	gtk_grid_attach (GTK_GRID (grid), label, 1, row, 2, 1);

	row++;
	widget = gtk_switch_new();
	//gtk_widget_set_halign(widget, GTK_ALIGN_START);
	data->SW_enabled[index] = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 1, row, 1, 1);

	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), RA_FILTER_MODE, FALSE);
	data->RA_matchmode[index] = widget;
	//gtk_widget_set_halign(widget, GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (grid), widget, 2, row, 1, 1);

	return grid;
}


static GtkWidget *ui_flt_page_widget_toolbar(struct ui_flt_list_data *data)
{
GtkWidget *treebox, *label;

	treebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_widget_set_margin_bottom(treebox, SPACING_SMALL);

		label = make_label (_("Select:"), 0, 0.5);
		gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);

		label = make_clicklabel("all", _("All"));
		data->bt_all = label;
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);
		
		label = make_clicklabel("non", _("None"));
		data->bt_non = label;
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);

		label = make_clicklabel("inv", _("Invert"));
		data->bt_inv = label;
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);

	return treebox;
}


static GtkWidget *ui_flt_page_list_generic (gchar *title, GtkWidget *treeview, struct ui_flt_list_data *data)
{
GtkWidget *vbox, *treebox, *scrollwin;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	data->gr_criteria = vbox;

	treebox = ui_flt_page_widget_toolbar(data);
	gtk_box_pack_start (GTK_BOX (vbox), treebox, FALSE, FALSE, 0);

 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	//gtk_widget_set_size_request (scrollwin, HB_MINWIDTH_LIST, HB_MINHEIGHT_LIST);
	gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeview), GTK_TREE_VIEW_GRID_LINES_NONE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);


	//list toolbar
	//tbar = ui_flt_page_widget_toolbar(data);
	//gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	return vbox;
}


static GtkWidget *ui_flt_page_account (struct ui_flt_manage_data *data)
{
struct ui_flt_list_data tmp;
GtkWidget *part, *grid;
GtkWidget *treeview;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	//header
	grid = ui_flt_page_misc_header(_("Account"), FLT_GRP_ACCOUNT, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	treeview = ui_acc_listview_new(TRUE);
	grid = ui_flt_page_list_generic(_("Account"), treeview, &tmp);
	gtk_box_pack_start (GTK_BOX (part), grid, TRUE, TRUE, 0);

	//data->SW_enabled[FLT_GRP_ACCOUNT] = tmp.sw_enabled;
	data->GR_page[FLT_GRP_ACCOUNT] = tmp.gr_criteria;
	//data->RA_matchmode[FLT_GRP_ACCOUNT] = tmp.ra_mode;
	data->LV_acc = treeview;
	
	g_signal_connect (tmp.bt_all, "activate-link", G_CALLBACK (ui_flt_hub_account_activate_link), treeview);
	g_signal_connect (tmp.bt_non, "activate-link", G_CALLBACK (ui_flt_hub_account_activate_link), treeview);
	g_signal_connect (tmp.bt_inv, "activate-link", G_CALLBACK (ui_flt_hub_account_activate_link), treeview);

	return(part);
}


static GtkWidget *ui_flt_page_category (struct ui_flt_manage_data *data)
{
struct ui_flt_list_data tmp;
GtkWidget *part, *grid, *widget, *tbar, *bbox;
GtkWidget *treeview;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	//header
	grid = ui_flt_page_misc_header(_("Category"), FLT_GRP_CATEGORY, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	treeview = ui_cat_listview_new(TRUE, FALSE);
	grid = ui_flt_page_list_generic(_("Category"), treeview, &tmp);
	gtk_box_pack_start (GTK_BOX (part), grid, TRUE, TRUE, 0);

	//data->SW_enabled[FLT_GRP_CATEGORY] = tmp.sw_enabled;
	data->GR_page[FLT_GRP_CATEGORY] = tmp.gr_criteria;
	//data->RA_matchmode[FLT_GRP_CATEGORY] = tmp.ra_mode;
	data->LV_cat = treeview;

	g_signal_connect (tmp.bt_all, "activate-link", G_CALLBACK (ui_flt_hub_category_activate_link), treeview);
	g_signal_connect (tmp.bt_non, "activate-link", G_CALLBACK (ui_flt_hub_category_activate_link), treeview);
	g_signal_connect (tmp.bt_inv, "activate-link", G_CALLBACK (ui_flt_hub_category_activate_link), treeview);

	// expand/colapse
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (grid), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
		data->BT_expand = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
		data->BT_collapse = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (data->BT_expand), "clicked", G_CALLBACK (ui_flt_hub_category_expand_all), NULL);
	g_signal_connect (G_OBJECT (data->BT_collapse), "clicked", G_CALLBACK (ui_flt_hub_category_collapse_all), NULL);

	return(part);
}


static GtkWidget *ui_flt_page_payee (struct ui_flt_manage_data *data)
{
struct ui_flt_list_data tmp;
GtkWidget *part, *grid;
GtkWidget *treeview;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	//header
	grid = ui_flt_page_misc_header(_("Payee"), FLT_GRP_PAYEE, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	treeview = ui_pay_listview_new(TRUE, FALSE);
	grid = ui_flt_page_list_generic(_("Payee"), treeview, &tmp);
	gtk_box_pack_start (GTK_BOX (part), grid, TRUE, TRUE, 0);

	//data->SW_enabled[FLT_GRP_PAYEE] = tmp.sw_enabled;
	data->GR_page[FLT_GRP_PAYEE] = tmp.gr_criteria;
	//data->RA_matchmode[FLT_GRP_PAYEE] = tmp.ra_mode;
	data->LV_pay = treeview;
	
	g_signal_connect (tmp.bt_all, "activate-link", G_CALLBACK (ui_flt_hub_payee_activate_link), treeview);
	g_signal_connect (tmp.bt_non, "activate-link", G_CALLBACK (ui_flt_hub_payee_activate_link), treeview);
	g_signal_connect (tmp.bt_inv, "activate-link", G_CALLBACK (ui_flt_hub_payee_activate_link), treeview);

	return part;
}


static GtkWidget *ui_flt_page_tag (struct ui_flt_manage_data *data)
{
struct ui_flt_list_data tmp;
GtkWidget *part, *grid;
GtkWidget *treeview;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	//header
	grid = ui_flt_page_misc_header(_("Tag"), FLT_GRP_TAG, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	treeview = ui_tag_listview_new(TRUE, FALSE);
	grid = ui_flt_page_list_generic(_("Tag"), treeview, &tmp);
	gtk_box_pack_start (GTK_BOX (part), grid, TRUE, TRUE, 0);

	//data->SW_enabled[FLT_GRP_TAG] = tmp.sw_enabled;
	data->GR_page[FLT_GRP_TAG] = tmp.gr_criteria;
	//data->RA_matchmode[FLT_GRP_TAG] = tmp.ra_mode;
	data->LV_tag = treeview;
	
	g_signal_connect (tmp.bt_all, "activate-link", G_CALLBACK (ui_flt_hub_tag_activate_link), treeview);
	g_signal_connect (tmp.bt_non, "activate-link", G_CALLBACK (ui_flt_hub_tag_activate_link), treeview);
	g_signal_connect (tmp.bt_inv, "activate-link", G_CALLBACK (ui_flt_hub_tag_activate_link), treeview);

	return part;
}


static GtkWidget *ui_flt_page_date(struct ui_flt_manage_data *data)
{
GtkWidget *part, *grid, *label, *widget;
gint row;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	//header
	grid = ui_flt_page_misc_header(_("Date"), FLT_GRP_DATE, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	// criteria
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	data->GR_page[FLT_GRP_DATE] = grid;
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	row = 0;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	data->CY_range = make_daterange(label, DATE_RANGE_FLAG_NONE);
	gtk_grid_attach (GTK_GRID (grid), data->CY_range, 1, row, 1, 1);
	gtk_widget_set_margin_bottom(label, SPACING_MEDIUM);
	gtk_widget_set_margin_bottom(data->CY_range, SPACING_MEDIUM);

	row++;
	label = make_label_widget(_("_From:"));
	data->LB_mindate = label;
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	widget = gtk_date_entry_new(label);
	data->PO_mindate = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("_To:"));
	data->LB_maxdate = label;
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	widget = gtk_date_entry_new(label);
	data->PO_maxdate = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 1, row, 1, 1);

	return part;
}




static GtkWidget *ui_flt_page_amounttext(struct ui_flt_manage_data *data)
{
GtkWidget *part, *grid, *hbox, *widget, *label;
gint row;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	// header
	grid = ui_flt_page_misc_header(_("Amount"), FLT_GRP_AMOUNT, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	// criteria
    grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	data->GR_page[FLT_GRP_AMOUNT] = grid;
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	row = 0;
	label = make_label_widget(_("_From:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	data->ST_minamount = make_amount(label);
	gtk_grid_attach (GTK_GRID (grid), data->ST_minamount, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("_To:"));
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	data->ST_maxamount = make_amount(label);
	gtk_grid_attach (GTK_GRID (grid), data->ST_maxamount, 1, row, 1, 1);

	//#2051758 idiot-proof input help
	row++;
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	//gtk_widget_set_hexpand (hbox, TRUE);
	gtk_grid_attach (GTK_GRID (grid), hbox, 1, row, 2, 1);

		widget = gtk_image_new_from_icon_name (ICONNAME_INFO, GTK_ICON_SIZE_BUTTON);
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		label = make_label_widget(_("Input From -30 To -15 to filter on expense"));
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);


	// header
	grid = ui_flt_page_misc_header(_("Text"), FLT_GRP_TEXT, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	// criteria
    grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	data->GR_page[FLT_GRP_TEXT] = grid;
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);
	
	row = 0;
	label = make_label_widget(_("_Memo:"));
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	data->ST_memo = make_string(label);
	gtk_widget_set_hexpand (data->ST_memo, TRUE);
	gtk_grid_attach (GTK_GRID (grid), data->ST_memo, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("_Number:"));
	//----------------------------------------- l, r, t, b
	gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
	data->ST_number = make_string(label);
	gtk_widget_set_hexpand (data->ST_number, TRUE);
	gtk_grid_attach (GTK_GRID (grid), data->ST_number, 1, row, 1, 1);

	row++;
	data->CM_exact = gtk_check_button_new_with_mnemonic (_("Case _sensitive"));
	gtk_grid_attach (GTK_GRID (grid), data->CM_exact, 1, row, 1, 1);

	return part;
}


static GtkWidget *ui_flt_page_payment(struct ui_flt_manage_data *data)
{
struct ui_flt_list_data tmp;
GtkWidget *part, *grid, *widget, *image, *vbox2, *tbar;
HbKivData *tmpkv, *kvdata = CYA_TXN_PAYMODE;
gint i, row;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	// header
	grid = ui_flt_page_misc_header(_("Payment"), FLT_GRP_PAYMODE, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	data->GR_page[FLT_GRP_PAYMODE] = vbox2;
	gtk_box_pack_start (GTK_BOX (part), vbox2, FALSE, FALSE, 0);

	tbar = ui_flt_page_widget_toolbar(&tmp);
	gtk_box_pack_start (GTK_BOX (vbox2), tbar, TRUE, TRUE, 0);

	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

	grid = gtk_grid_new ();
	//gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_SMALL);
	//gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 0);
	gtk_frame_set_child(GTK_FRAME(frame), grid);
	
	hb_widget_set_margin(grid, 4);
	gtk_style_context_add_class (gtk_widget_get_style_context (frame), GTK_STYLE_CLASS_VIEW);

	
	for(i=0;i<NUM_PAYMODE_MAX;i++)
	{
	tmpkv = &kvdata[i];

		if( tmpkv->name == NULL )
			break;

		row = i;

		image = gtk_image_new_from_icon_name( tmpkv->iconname, GTK_ICON_SIZE_MENU);
		gtk_grid_attach (GTK_GRID (grid), image, 0, row, 1, 1);

		widget = gtk_check_button_new_with_mnemonic(_(tmpkv->name));
		data->CM_paymode[i] = widget;
		g_object_set_data(G_OBJECT(widget), "uid", GUINT_TO_POINTER(tmpkv->key));
		gtk_grid_attach (GTK_GRID (grid), data->CM_paymode[i], 1, row, 1, 1);
	}



	g_signal_connect (tmp.bt_all, "activate-link", G_CALLBACK (ui_flt_page_paymode_activate_link), NULL);
	g_signal_connect (tmp.bt_non, "activate-link", G_CALLBACK (ui_flt_page_paymode_activate_link), NULL);
	g_signal_connect (tmp.bt_inv, "activate-link", G_CALLBACK (ui_flt_page_paymode_activate_link), NULL);

	return part;
}


static GtkWidget *ui_flt_page_type(struct ui_flt_manage_data *data)
{
GtkWidget *part, *grid, *widget;
gint row;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	//header
	grid = ui_flt_page_misc_header(_("Type"), FLT_GRP_TYPE, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	// criteria
    grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	data->GR_page[FLT_GRP_TYPE] = grid;
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	row = 0;
	widget = gtk_toggle_button_new_with_label(_("Expense"));
	data->CM_typnexp = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	row++;
	widget = gtk_toggle_button_new_with_label(_("Income"));
	data->CM_typninc = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	row++;
	widget = gtk_toggle_button_new_with_label(_("Expense Transfer"));
	data->CM_typxexp = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	row++;
	widget = gtk_toggle_button_new_with_label(_("Income Transfer"));
	data->CM_typxinc = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	return part;
}


static GtkWidget *ui_flt_page_status(struct ui_flt_manage_data *data)
{
GtkWidget *part, *grid, *widget;
gint row;

	part = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_LARGE);

	// header
	grid = ui_flt_page_misc_header(_("Status"), FLT_GRP_STATUS, data);
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);


	// criteria
    grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	data->GR_page[FLT_GRP_STATUS] = grid;
	gtk_box_pack_start (GTK_BOX (part), grid, FALSE, FALSE, 0);

	row = 0;;
	widget = gtk_toggle_button_new_with_label(_("None"));
	data->CM_stanon = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	row++;
	widget = gtk_toggle_button_new_with_label(_("Cleared"));
	data->CM_staclr = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	row++;
	widget = gtk_toggle_button_new_with_label(_("Reconciled"));
	data->CM_starec = widget;
	gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);

	return part;
}


gint ui_flt_manage_dialog_new(GtkWindow *parentwindow, Filter *filter, gboolean show_account, gboolean txnmode)
{
struct ui_flt_manage_data *data;
GtkWidget *dialog, *content, *mainbox, *sidebar, *stack, *grid, *page, *label, *widget;
gchar *wintitle;
gint w, h, dw, dh;

	DB( g_print("\n\n------------------------\n") );
	DB( g_print("\n[ui-filter] new\n") );

	data = g_malloc0(sizeof(struct ui_flt_manage_data));

	DB( g_print(" key;%d name: '%s'\n", filter->key, filter->name) );
	DB( g_print(" show_account:%d, txnmode:%d\n", show_account, txnmode) );

	data->filter   = filter;
	data->saveable = filter->key > 0 ? TRUE : FALSE;

	data->dialog = dialog = gtk_dialog_new_with_buttons (NULL,
			GTK_WINDOW (parentwindow),
			0,	//no flags
			NULL, //no buttons
			NULL);

	if(!txnmode)
	{
		widget = gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Reset"),	HB_RESPONSE_FLT_RESET);
		gtk_widget_set_margin_end(widget, SPACING_LARGE);
	}

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Cancel"),	GTK_RESPONSE_REJECT);
	if( data->saveable )
		gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Save & Use"), HB_RESPONSE_FLT_SAVE_USE);

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Use"),    GTK_RESPONSE_ACCEPT);

	//window title
	wintitle = g_strdup_printf("%s - %s", _("Edit filter"), data->saveable ? filter->name : _("default") );
	gtk_window_set_title(GTK_WINDOW(dialog), wintitle );
	g_free(wintitle);

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(parentwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 2:3
	dw = (dh * 2) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, -1);

	//store our window private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" - window=%p, inst_data=%p\n", dialog, data) );

    g_signal_connect (dialog, "destroy",
			G_CALLBACK (gtk_widget_destroyed), &dialog);

	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (content), mainbox, TRUE, TRUE, 0);

	sidebar = gtk_stack_sidebar_new ();
	gtk_widget_set_margin_bottom(sidebar, SPACING_LARGE);
    gtk_box_pack_start (GTK_BOX (mainbox), sidebar, FALSE, FALSE, 0);


	stack = gtk_stack_new ();
	gtk_stack_set_transition_type (GTK_STACK (stack), GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
	//gtk_stack_set_transition_type (GTK_STACK (stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
	gtk_stack_sidebar_set_stack (GTK_STACK_SIDEBAR (sidebar), GTK_STACK (stack));
	hb_widget_set_margin(GTK_WIDGET(stack), SPACING_LARGE);


	data->stack = stack;
    gtk_box_pack_start (GTK_BOX (mainbox), stack, TRUE, TRUE, 0);


	//common (date + status + amount)
/*	label = gtk_label_new(_("General"));
	page = ui_flt_page_general(&data);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
*/

	//old: date/status/payments/amounts/texts/Cat/Payee

	//TODO: needs to keep this until we enable from/to into ledger
	page = ui_flt_page_date(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_DAT, _("Date"));

	page = ui_flt_page_type(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_TYP, _("Type"));

	page = ui_flt_page_status(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_STA, _("Status"));

	data->show_account = show_account;
	if(show_account == TRUE)
	{
		page = ui_flt_page_account(data);
		gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_ACC, _("Account"));
	}

	page = ui_flt_page_payee(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_PAY, _("Payee"));

	page = ui_flt_page_category(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_CAT, _("Category"));

	page = ui_flt_page_tag(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_TAG, _("Tag"));

	page = ui_flt_page_payment(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_PMT, _("Payment"));

	page = ui_flt_page_amounttext(data);
	gtk_stack_add_titled (GTK_STACK (stack), page, FLT_PAGE_NAME_TXT, _("Amount/Text"));

	//#xxxxxxx
	widget = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_set_margin_top(widget, SPACING_LARGE);
	gtk_widget_set_margin_bottom(widget, SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (mainbox), widget, FALSE, FALSE, 0);

	// force display
	grid = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	data->GR_force = grid;
	hb_widget_set_margin(grid, SPACING_LARGE);
	gtk_widget_set_valign(grid, GTK_ALIGN_END);
	gtk_box_pack_start (GTK_BOX (mainbox), grid, FALSE, TRUE, 0);

	label = make_label_group(_("Always show"));
	gtk_box_pack_start (GTK_BOX (grid), label, FALSE, FALSE, 0);

	widget = gtk_check_button_new_with_mnemonic (_("Remind"));
	data->CM_forceremind = widget;
	gtk_box_pack_start (GTK_BOX (grid), widget, FALSE, FALSE, 0);

	widget = gtk_check_button_new_with_mnemonic (_("Void"));
	data->CM_forcevoid = widget;
	gtk_box_pack_start (GTK_BOX (grid), widget, FALSE, FALSE, 0);

	widget = gtk_check_button_new_with_mnemonic (_("Added"));
	data->CM_forceadd = widget;
	gtk_box_pack_start (GTK_BOX (grid), widget, FALSE, FALSE, 0);

	widget = gtk_check_button_new_with_mnemonic (_("Edited"));
	data->CM_forcechg = widget;
	gtk_box_pack_start (GTK_BOX (grid), widget, FALSE, FALSE, 0);

	
	//setup, init and show window
	ui_flt_manage_setup(data);
	ui_flt_manage_set(data);

	/* signal connect */
	//g_signal_connect (data->SW_enabled[FLT_GRP_DATE]    , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_TYPE]    , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_STATUS]    , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_PAYEE]   , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_CATEGORY], "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_TAG]   , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	if(show_account == TRUE)
		g_signal_connect (data->SW_enabled[FLT_GRP_ACCOUNT] , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_TEXT]    , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_AMOUNT]  , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);
	g_signal_connect (data->SW_enabled[FLT_GRP_PAYMODE] , "notify::active", G_CALLBACK (ui_flt_manage_update), NULL);

	g_signal_connect (data->CY_range  , "changed", G_CALLBACK (ui_flt_manage_cb_range_change), NULL);
	g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (ui_flt_manage_cb_date_change), NULL);
	g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (ui_flt_manage_cb_date_change), NULL);


	gtk_widget_show_all (dialog);
	//gtk_widget_grab_focus(sidebar);

	ui_flt_manage_update(dialog, NULL);

	if(!txnmode)
	{
		hb_widget_visible (data->GR_force, FALSE);
	}

	if( *data->filter->last_tab != '\0' )
		gtk_stack_set_visible_child_name (GTK_STACK(data->stack), data->filter->last_tab);
	DB( g_print(" set page '%s'\n", data->filter->last_tab) );


	//wait for the user
	gint retval;	// = 55;

	//while( result == 55 )
	//{
		retval = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (retval)
	    {
		case HB_RESPONSE_FLT_SAVE_USE:
		case GTK_RESPONSE_ACCEPT:
		   //do_application_specific_something ();
			ui_flt_manage_get(data);
			da_flt_count_item(filter);
			break;
		//case 55:	reset will be treated in calling window
		}
	//}

	// cleanup and destroy
	//ui_flt_manage_cleanup(&data, result);

	DB( g_print(" destroy\n") );
	gtk_window_destroy (GTK_WINDOW(dialog));

	DB( g_print(" free\n") );
	g_free(data);
	
	DB( g_print(" end dialog filter all ok\n") );

	return retval;
}

