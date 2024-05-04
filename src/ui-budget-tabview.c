/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 2018-2019 Adrien Dorsaz <adrien@adorsaz.ch>
 *  Copyright (C) 2019-2023 Maxime DOYEN
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
#include "hb-misc.h"
#include "dsp-mainwindow.h"
#include "hb-category.h"
#include "ui-budget-tabview.h"

/****************************************************************************/
/* Implementation notes */
/****************************************************************************/
/*
 * This dialog allows user to manage its budget within a GtkTreeView.
 *
 * The view rows are separated in three main tree roots:
 *   - Income: contains all Homebank categories of income type (see GF_INCOME)
 *   - Expense: contains all Homebank categories of expense type
 *   - Total: contains 3 sub-rows:
 *      - Income: sum all amounts of the Income root
 *      - Expense: sum all amounts of the Expense root
 *      - Summary: difference between the two above sub-rows
 *
 * The view columns contain:
 *   - Category: Homebank categories organised in hierarchy
 *     according to the main tree roots above and the categories hierarchy
 *
 *   - Annual Total: sum all amounts of the year for the category
 *
 *   - Monthly Average: average of the amounts for the category
 *
 *   - Monthly: set the monthly amount when the Same flag is active
 *     - That column contains a toggle check box to enable or not monthly values
 *       Check it to disable the GF_CUSTOM flag of Homebank categories
 *       "Does this category has same amount planned every month ?"
 *
 *   - 12 columns for each month of the year containing their specific amount
 *
 * The dialog shows 3 radio buttons on top to choose between 3 viewing modes:
 *   - Summary: show Homebank categories with budget set or set with GF_FORCED
  *  - Expense: show all available Homebank categories of expense type
 *   - Income: show all available Homebank categories of income type
 *
 */

/****************************************************************************/
/* Debug macros */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* Global data */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;

static gchar *UI_BUD_TABVIEW_MONTHS[] = {
	N_("Jan"), N_("Feb"), N_("Mar"),
	N_("Apr"), N_("May"), N_("Jun"),
	N_("Jul"), N_("Aug"), N_("Sept"),
	N_("Oct"), N_("Nov"), N_("Dec"),
	NULL};

/* The different view mode available */
static gchar *UI_BUD_TABVIEW_VIEW_MODE[] = {
	N_("Summary"),
	N_("Expense"),
	N_("Income"),
	NULL
};

/* These values has to correspond to UI_BUD_TABVIEW_VIEW_MODE[] */
enum ui_bud_tabview_view_mode
{
	UI_BUD_TABVIEW_VIEW_SUMMARY = 0,
	UI_BUD_TABVIEW_VIEW_EXPENSE,
	UI_BUD_TABVIEW_VIEW_INCOME
};
typedef enum ui_bud_tabview_view_mode ui_bud_tabview_view_mode_t;

/* These values corresponds to the return of category_type_get from hb-category */
enum ui_bud_tabview_cat_type
{
	UI_BUD_TABVIEW_CAT_TYPE_EXPENSE = -1,
	UI_BUD_TABVIEW_CAT_TYPE_NONE = 0, // Not real category type: used to retrieve tree roots
	UI_BUD_TABVIEW_CAT_TYPE_INCOME = 1
};
typedef enum ui_bud_tabview_cat_type ui_bud_tabview_cat_type_t;

/* enum for the Budget Tree Store model */
enum ui_bud_tabview_store
{
	UI_BUD_TABVIEW_CATEGORY_KEY = 0,
	UI_BUD_TABVIEW_CATEGORY_NAME,
	UI_BUD_TABVIEW_CATEGORY_FULLNAME,
	UI_BUD_TABVIEW_CATEGORY_TYPE,
	UI_BUD_TABVIEW_IS_ROOT, // To retrieve easier the 3 main tree roots
	UI_BUD_TABVIEW_IS_TOTAL, // To retrieve rows inside the Total root
	UI_BUD_TABVIEW_IS_CHILD_HEADER, // The row corresponds to the head child which is shown before the separator
	UI_BUD_TABVIEW_IS_SEPARATOR, // Row to just display a separator in Tree View
	UI_BUD_TABVIEW_IS_MONITORING_FORCED,
	UI_BUD_TABVIEW_IS_SAME_AMOUNT,
	UI_BUD_TABVIEW_IS_SUB_CATEGORY,
	UI_BUD_TABVIEW_HAS_BUDGET,
	UI_BUD_TABVIEW_SAME_AMOUNT,
	UI_BUD_TABVIEW_JANUARY,
	UI_BUD_TABVIEW_FEBRUARY,
	UI_BUD_TABVIEW_MARCH,
	UI_BUD_TABVIEW_APRIL,
	UI_BUD_TABVIEW_MAY,
	UI_BUD_TABVIEW_JUNE,
	UI_BUD_TABVIEW_JULY,
	UI_BUD_TABVIEW_AUGUST,
	UI_BUD_TABVIEW_SEPTEMBER,
	UI_BUD_TABVIEW_OCTOBER,
	UI_BUD_TABVIEW_NOVEMBER,
	UI_BUD_TABVIEW_DECEMBER,
	UI_BUD_TABVIEW_NUMBER_COLOMNS
};
typedef enum ui_bud_tabview_store ui_bud_tabview_store_t;

// Retrieve a row iterator according to specific criterias
const struct ui_bud_tabview_search_criteria
{
	// Search by non-zero category key
	guint32 row_category_key;
	// Search by other criterias
	ui_bud_tabview_cat_type_t row_category_type;
	gboolean row_is_root;
	gboolean row_is_total;
	// Found iterator, NULL if not found
	GtkTreeIter *iterator;
} ui_bud_tabview_search_criteria_default = {0, UI_BUD_TABVIEW_CAT_TYPE_NONE, FALSE, FALSE, NULL} ;
typedef struct ui_bud_tabview_search_criteria ui_bud_tabview_search_criteria_t;

/*
 * Local headers
 **/

// GtkTreeStore model
static gboolean ui_bud_tabview_model_search_iterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ui_bud_tabview_search_criteria_t *search);
static void ui_bud_tabview_model_add_category_with_lineage(GtkTreeStore *budget, GtkTreeIter *balanceIter, guint32 *key_category);
static void ui_bud_tabview_model_collapse (GtkTreeView *view);
static void ui_bud_tabview_model_insert_roots(GtkTreeStore* budget);
static void ui_bud_tabview_model_update_monthly_total(GtkTreeStore* budget);
static gboolean ui_bud_tabview_model_row_filter (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
#if HB_BUD_TABVIEW_EDIT_ENABLE
static gboolean ui_bud_tabview_model_row_merge_filter_with_headers (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static gboolean ui_bud_tabview_model_row_filter_parents (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
#endif
static gint ui_bud_tabview_model_row_sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
static GtkTreeModel * ui_bud_tabview_model_new ();

// GtkTreeView widget
static void ui_bud_tabview_view_display_category_name (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_bud_tabview_view_display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_bud_tabview_view_display_is_same_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_bud_tabview_view_display_annual_total (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_bud_tabview_view_display_monthly_average(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_bud_tabview_view_toggle (gpointer user_data, ui_bud_tabview_view_mode_t view_mode);
static gboolean ui_bud_tabview_view_search (GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer data);
#if HB_BUD_TABVIEW_EDIT_ENABLE
static gboolean ui_bud_tabview_view_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
#endif
static void ui_bud_tabview_view_on_select(GtkTreeSelection *treeselection, gpointer user_data);
static GtkWidget *ui_bud_tabview_view_new (gpointer user_data);

// UI actions
#if HB_BUD_TABVIEW_EDIT_ENABLE
static void ui_bud_tabview_cell_update_category(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data);
#endif
static void ui_bud_tabview_cell_update_amount(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data);
static void ui_bud_tabview_cell_update_is_same_amount(GtkCellRendererText *renderer, gchar *filter_path, gpointer user_data);
static void ui_bud_tabview_view_update_mode (GtkToggleButton *button, gpointer user_data);
static void ui_bud_tabview_view_expand (GtkButton *button, gpointer user_data);
static void ui_bud_tabview_view_collapse (GtkButton *button, gpointer user_data);
static gboolean ui_bud_tabview_get_selected_category (GtkTreeModel **budget, GtkTreeIter *iter, Category **category, ui_bud_tabview_data_t *data);
#if HB_BUD_TABVIEW_EDIT_ENABLE
static gboolean ui_bud_tabview_get_selected_root_iter (GtkTreeModel **budget, GtkTreeIter *iter, ui_bud_tabview_data_t *data);
static void ui_bud_tabview_category_add_full_filled (GtkWidget *source, gpointer user_data);
static void ui_bud_tabview_category_add (GtkButton *button, gpointer user_data);
static void ui_bud_tabview_category_delete (GtkButton *button, gpointer user_data);
static void ui_bud_tabview_category_merge_full_filled (GtkWidget *source, gpointer user_data);
static void ui_bud_tabview_category_merge (GtkButton *button, gpointer user_data);
#endif
static void ui_bud_tabview_category_reset (GtkButton *button, gpointer user_data);
static gboolean ui_bud_tabview_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void ui_bud_tabview_dialog_close(ui_bud_tabview_data_t *data, gint response);

/**
 * GtkTreeStore model
 **/

// Look for category by deterministic characteristics
// Only categories with specific characteristics can be easily found
// like roots, total rows and categories with real key id
// You are responsible to g_free iterator
static gboolean ui_bud_tabview_model_search_iterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ui_bud_tabview_search_criteria_t *search)
{
guint32 category_key;
ui_bud_tabview_cat_type_t category_type;
gboolean is_found = FALSE, is_root, is_total, is_separator;

	search->iterator = NULL;

	gtk_tree_model_get (model, iter,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &category_type,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		UI_BUD_TABVIEW_IS_SEPARATOR, &is_separator,
		-1);

	if (search->row_category_key > 0 // Look for iter of real category row
		&& category_key == search->row_category_key
		&& !(is_total)
		&& !(is_root)
	)
	{
		DB(g_print("\tFound row with key %d\n", category_key));
		is_found = TRUE;
	}
	else if (search->row_category_key == 0 // Look for iter of fake category row
		&& is_root == search->row_is_root
		&& is_total == search->row_is_total
		&& category_type == search->row_category_type
		&& !is_separator
	)
	{
		DB(g_print("\tFound row with is_root = %d, is_total %d, type = %d\n", is_root, is_total, category_type));
		is_found = TRUE;
	}

	// If found, save result to struct
	if (is_found)
	{
		search->iterator = g_malloc0(sizeof(GtkTreeIter));
		*search->iterator = *iter;
	}

	return is_found;
}

/* Recursive function which add a new row in the budget model with all its ancestors */
static void ui_bud_tabview_model_add_category_with_lineage(GtkTreeStore *budget, GtkTreeIter *balanceIter, guint32 *key_category)
{
GtkTreeIter child;
GtkTreeIter *parent;
Category *bdg_category;
gboolean cat_is_same_amount;
ui_bud_tabview_search_criteria_t parent_search = ui_bud_tabview_search_criteria_default;

	bdg_category = da_cat_get(*key_category);

	if (bdg_category == NULL)
	{
		return;
	}

	cat_is_same_amount = (! (bdg_category->flags & GF_CUSTOM));

	/* Check if parent category already exists */
	parent_search.row_category_key = bdg_category->parent;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&parent_search);

	if (bdg_category->parent == 0)
	{
		// If we are one of the oldest parent, stop recursion
		gtk_tree_store_insert (
			budget,
			&child,
			balanceIter,
			-1);
	}
	else
	{
		if (parent_search.iterator)
		{
			DB(g_print("\tRecursion optimisation: parent key %d already exists\n", parent_search.row_category_key));
			// If parent already exists, stop recursion
			parent = parent_search.iterator;
		}
		else
		{
			// Parent has not been found, ask to create it first
			ui_bud_tabview_model_add_category_with_lineage(budget, balanceIter, &(bdg_category->parent));

			// Now, we are sure parent exists, look for it again
			gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
				(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
				&parent_search);

			parent = parent_search.iterator;
		}

		gtk_tree_store_insert (
			budget,
			&child,
			parent,
			-1);
	}

	DB(g_print("insert new category %s (key: %d, type: %d)\n",
		bdg_category->name, bdg_category->key, category_type_get (bdg_category)));

	gtk_tree_store_set(
		budget,
		&child,
		UI_BUD_TABVIEW_CATEGORY_KEY, bdg_category->key,
		UI_BUD_TABVIEW_CATEGORY_NAME, bdg_category->typename,
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, bdg_category->fullname,
		UI_BUD_TABVIEW_CATEGORY_TYPE, category_type_get (bdg_category),
		UI_BUD_TABVIEW_IS_MONITORING_FORCED, (bdg_category->flags & GF_FORCED),
		UI_BUD_TABVIEW_IS_ROOT, FALSE,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, cat_is_same_amount,
		UI_BUD_TABVIEW_IS_TOTAL, FALSE,
		UI_BUD_TABVIEW_IS_SUB_CATEGORY, bdg_category->parent != 0,
		UI_BUD_TABVIEW_HAS_BUDGET, (bdg_category->flags & GF_BUDGET),
		UI_BUD_TABVIEW_SAME_AMOUNT, bdg_category->budget[0],
		UI_BUD_TABVIEW_JANUARY, bdg_category->budget[1],
		UI_BUD_TABVIEW_FEBRUARY, bdg_category->budget[2],
		UI_BUD_TABVIEW_MARCH, bdg_category->budget[3],
		UI_BUD_TABVIEW_APRIL, bdg_category->budget[4],
		UI_BUD_TABVIEW_MAY, bdg_category->budget[5],
		UI_BUD_TABVIEW_JUNE, bdg_category->budget[6],
		UI_BUD_TABVIEW_JULY, bdg_category->budget[7],
		UI_BUD_TABVIEW_AUGUST, bdg_category->budget[8],
		UI_BUD_TABVIEW_SEPTEMBER, bdg_category->budget[9],
		UI_BUD_TABVIEW_OCTOBER, bdg_category->budget[10],
		UI_BUD_TABVIEW_NOVEMBER, bdg_category->budget[11],
		UI_BUD_TABVIEW_DECEMBER, bdg_category->budget[12],
		-1);

	// Always add child header and separator
	parent = gtk_tree_iter_copy(&child);
	gtk_tree_store_insert_with_values(
		budget,
		&child,
		parent,
		-1,
		UI_BUD_TABVIEW_CATEGORY_KEY, bdg_category->key,
		UI_BUD_TABVIEW_CATEGORY_NAME, bdg_category->name,
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, bdg_category->fullname,
		UI_BUD_TABVIEW_CATEGORY_TYPE, category_type_get (bdg_category),
		UI_BUD_TABVIEW_IS_CHILD_HEADER, TRUE,
		-1);

	gtk_tree_store_insert_with_values(
		budget,
		&child,
		parent,
		-1,
		UI_BUD_TABVIEW_CATEGORY_KEY, bdg_category->key,
		UI_BUD_TABVIEW_CATEGORY_TYPE, category_type_get (bdg_category),
		UI_BUD_TABVIEW_IS_SEPARATOR, TRUE,
		-1);

	gtk_tree_iter_free(parent);

	g_free(parent_search.iterator);

	return;
}

// Collapse all categories except root
static void ui_bud_tabview_model_collapse (GtkTreeView *view)
{
GtkTreeModel *budget;
GtkTreePath *path;
ui_bud_tabview_search_criteria_t root_search = ui_bud_tabview_search_criteria_default;

	budget = gtk_tree_view_get_model (view);

	gtk_tree_view_collapse_all(view);

	// Keep root categories expanded

	// Retrieve income root
	root_search.row_is_root = TRUE;
	root_search.row_is_total = FALSE;
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_INCOME;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (root_search.iterator != NULL)
	{
		path = gtk_tree_model_get_path(budget, root_search.iterator);
		gtk_tree_view_expand_row(view, path, FALSE);
	}

	// Retrieve expense root
	root_search.row_is_root = TRUE;
	root_search.row_is_total = FALSE;
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_EXPENSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (root_search.iterator != NULL)
	{
		path = gtk_tree_model_get_path(budget, root_search.iterator);
		gtk_tree_view_expand_row(view, path, FALSE);
	}

	// Retrieve total root
	root_search.row_is_root = TRUE;
	root_search.row_is_total = FALSE;
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_NONE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (root_search.iterator != NULL)
	{
		path = gtk_tree_model_get_path(budget, root_search.iterator);
		gtk_tree_view_expand_row(view, path, FALSE);
	}

	g_free(root_search.iterator);

	return;
}

// Create tree roots for the store
static void ui_bud_tabview_model_insert_roots(GtkTreeStore* budget)
{
GtkTreeIter iter, root;

	gtk_tree_store_insert_with_values (
		budget,
		&root,
		NULL,
		-1,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_INCOME]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_INCOME]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_INCOME,
		UI_BUD_TABVIEW_IS_ROOT, TRUE,
		UI_BUD_TABVIEW_IS_TOTAL, FALSE,
		-1);

	// For add category dialog: copy of the root to be able to select it
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_INCOME]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_INCOME]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_INCOME,
		UI_BUD_TABVIEW_CATEGORY_KEY, 0,
		UI_BUD_TABVIEW_IS_ROOT, FALSE,
		UI_BUD_TABVIEW_IS_TOTAL, FALSE,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, TRUE,
		-1);

	// For add category dialog: add a separator to distinguish root with children
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		UI_BUD_TABVIEW_IS_SEPARATOR, TRUE,
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_INCOME,
		-1);

	gtk_tree_store_insert_with_values (
		budget,
		&root,
		NULL,
		-1,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_EXPENSE]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_EXPENSE]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_EXPENSE,
		UI_BUD_TABVIEW_IS_ROOT, TRUE,
		UI_BUD_TABVIEW_IS_TOTAL, FALSE,
		-1);

	// For add category dialog: copy of the root to be able to select it
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_EXPENSE]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_EXPENSE]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_EXPENSE,
		UI_BUD_TABVIEW_CATEGORY_KEY, 0,
		UI_BUD_TABVIEW_IS_ROOT, FALSE,
		UI_BUD_TABVIEW_IS_TOTAL, FALSE,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, TRUE,
		-1);

	// For add category dialog: add a separator to distinguish root with children
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		UI_BUD_TABVIEW_IS_SEPARATOR, TRUE,
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_EXPENSE,
		-1);

	gtk_tree_store_insert_with_values (
		budget,
		&root,
		NULL,
		-1,
		UI_BUD_TABVIEW_CATEGORY_NAME, _("Totals"),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _("Totals"),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_NONE,
		UI_BUD_TABVIEW_IS_ROOT, TRUE,
		UI_BUD_TABVIEW_IS_TOTAL, FALSE,
		-1);

	return;
}

// Update (or insert) total rows for a budget according to the view mode
// This function will is used to initiate model and to refresh it after change by user
static void ui_bud_tabview_model_update_monthly_total(GtkTreeStore* budget)
{
ui_bud_tabview_search_criteria_t root_search = ui_bud_tabview_search_criteria_default;
GtkTreeIter total_root, child;
double total_income[12] = {0}, total_expense[12] = {0};
gboolean cat_is_same_amount;
guint32 n_category;

	// Go through all categories to compute totals
	n_category = da_cat_get_max_key();

	for(guint32 i=1; i<=n_category; ++i)
	{
	Category *bdg_category;
	gboolean cat_is_income;

		bdg_category = da_cat_get(i);

		if (bdg_category == NULL)
		{
			continue;
		}

		cat_is_income = (category_type_get (bdg_category) == 1);
		cat_is_same_amount = (! (bdg_category->flags & GF_CUSTOM));

		for (gint j=0; j<=11; ++j)
		{
			if (cat_is_income)
			{
				if (cat_is_same_amount)
				{
					total_income[j] += bdg_category->budget[0];
				}
				else
				{
					total_income[j] += bdg_category->budget[j+1];
				}
			}
			else
			{
				if (cat_is_same_amount)
				{
					total_expense[j] += bdg_category->budget[0];
				}
				else
				{
					total_expense[j] += bdg_category->budget[j+1];
				}
			}
		}
	}

	// Retrieve total root and insert required total rows
	root_search.row_is_root = TRUE;
	root_search.row_is_total = FALSE;
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_NONE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (!root_search.iterator)
	{
		return;
	}

	total_root = *root_search.iterator;

	// Retrieve and set totals
	root_search.row_is_root = FALSE;
	root_search.row_is_total = TRUE;

	// First, look for Incomes
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_INCOME;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (root_search.iterator)
	{
		child = *root_search.iterator;
	}
	else
	{
		gtk_tree_store_insert(budget, &child, &total_root, -1);
	}

	gtk_tree_store_set (
		budget,
		&child,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_INCOME]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_INCOME]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_INCOME,
		UI_BUD_TABVIEW_IS_TOTAL, TRUE,
		UI_BUD_TABVIEW_JANUARY, total_income[0],
		UI_BUD_TABVIEW_FEBRUARY, total_income[1],
		UI_BUD_TABVIEW_MARCH, total_income[2],
		UI_BUD_TABVIEW_APRIL, total_income[3],
		UI_BUD_TABVIEW_MAY, total_income[4],
		UI_BUD_TABVIEW_JUNE, total_income[5],
		UI_BUD_TABVIEW_JULY, total_income[6],
		UI_BUD_TABVIEW_AUGUST, total_income[7],
		UI_BUD_TABVIEW_SEPTEMBER, total_income[8],
		UI_BUD_TABVIEW_OCTOBER, total_income[9],
		UI_BUD_TABVIEW_NOVEMBER, total_income[10],
		UI_BUD_TABVIEW_DECEMBER, total_income[11],
		-1);

	// Then look for Expenses
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_EXPENSE;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (root_search.iterator)
	{
		child = *root_search.iterator;
	}
	else
	{
		gtk_tree_store_insert(budget, &child, &total_root, -1);
	}

	gtk_tree_store_set (
		budget,
		&child,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_EXPENSE]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_EXPENSE]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_EXPENSE,
		UI_BUD_TABVIEW_IS_TOTAL, TRUE,
		UI_BUD_TABVIEW_JANUARY, total_expense[0],
		UI_BUD_TABVIEW_FEBRUARY, total_expense[1],
		UI_BUD_TABVIEW_MARCH, total_expense[2],
		UI_BUD_TABVIEW_APRIL, total_expense[3],
		UI_BUD_TABVIEW_MAY, total_expense[4],
		UI_BUD_TABVIEW_JUNE, total_expense[5],
		UI_BUD_TABVIEW_JULY, total_expense[6],
		UI_BUD_TABVIEW_AUGUST, total_expense[7],
		UI_BUD_TABVIEW_SEPTEMBER, total_expense[8],
		UI_BUD_TABVIEW_OCTOBER, total_expense[9],
		UI_BUD_TABVIEW_NOVEMBER, total_expense[10],
		UI_BUD_TABVIEW_DECEMBER, total_expense[11],
		-1);

	// Finally, set Balance total row
	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_NONE;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);

	if (root_search.iterator)
	{
		child = *root_search.iterator;
	}
	else
	{
		gtk_tree_store_insert(budget, &child, &total_root, -1);
	}

	gtk_tree_store_set (
		budget,
		&child,
		UI_BUD_TABVIEW_CATEGORY_NAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_SUMMARY]),
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, _(UI_BUD_TABVIEW_VIEW_MODE[UI_BUD_TABVIEW_VIEW_SUMMARY]),
		UI_BUD_TABVIEW_CATEGORY_TYPE, UI_BUD_TABVIEW_CAT_TYPE_NONE,
		UI_BUD_TABVIEW_IS_TOTAL, TRUE,
		UI_BUD_TABVIEW_JANUARY, total_income[0] + total_expense[0],
		UI_BUD_TABVIEW_FEBRUARY, total_income[1] + total_expense[1],
		UI_BUD_TABVIEW_MARCH, total_income[2] + total_expense[2],
		UI_BUD_TABVIEW_APRIL, total_income[3] + total_expense[3],
		UI_BUD_TABVIEW_MAY, total_income[4] + total_expense[4],
		UI_BUD_TABVIEW_JUNE, total_income[5] + total_expense[5],
		UI_BUD_TABVIEW_JULY, total_income[6] + total_expense[6],
		UI_BUD_TABVIEW_AUGUST, total_income[7] + total_expense[7],
		UI_BUD_TABVIEW_SEPTEMBER, total_income[8] + total_expense[8],
		UI_BUD_TABVIEW_OCTOBER, total_income[9] + total_expense[9],
		UI_BUD_TABVIEW_NOVEMBER, total_income[10] + total_expense[10],
		UI_BUD_TABVIEW_DECEMBER, total_income[11] + total_expense[11],
		-1);

	g_free(root_search.iterator);

	return;
}

// Filter shown rows according to VIEW mode
static gboolean ui_bud_tabview_model_row_filter (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_visible, is_root, is_total, is_separator, is_childheader;
ui_bud_tabview_data_t* data;
ui_bud_tabview_view_mode_t view_mode;
guint32 category_key;
ui_bud_tabview_cat_type_t category_type;
Category *bdg_category;

	is_visible = TRUE;
	data = user_data;
	view_mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		UI_BUD_TABVIEW_IS_SEPARATOR, &is_separator,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, &is_childheader,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &category_type,
		-1);

	// On specific mode, hide categories of opposite type
	if (!is_total
		&& category_type == UI_BUD_TABVIEW_CAT_TYPE_INCOME
		&& view_mode == UI_BUD_TABVIEW_VIEW_EXPENSE)
	{
		is_visible = FALSE;
	}

	if (!is_total
		&& category_type == UI_BUD_TABVIEW_CAT_TYPE_EXPENSE
		&& view_mode == UI_BUD_TABVIEW_VIEW_INCOME)
	{
		is_visible = FALSE;
	}

	// Hide fake first child root used for add dialog
	if (is_childheader
		|| is_separator)
	{
		is_visible = FALSE;
	}

	// On balance mode, hide not forced empty categories
	if (!is_total
		&& !is_root
		&& !is_childheader
		&& !is_separator
		&& view_mode == UI_BUD_TABVIEW_VIEW_SUMMARY)
	{
		bdg_category = da_cat_get(category_key);

		if (bdg_category != NULL)
		{
			// Either the category has some budget, or its display is forced
			is_visible = (bdg_category->flags & (GF_BUDGET|GF_FORCED));

			// Force display if one of its children should be displayed
			if (!is_visible)
			{
			GtkTreeIter child;
			Category *subcat;
			guint32 subcat_key;
			gint child_id=0;

				while (gtk_tree_model_iter_nth_child(model,
					&child,
					iter,
					child_id))
				{
					gtk_tree_model_get(model, &child,
						UI_BUD_TABVIEW_CATEGORY_KEY, &subcat_key,
						-1);

					if (subcat_key != 0)
					{
						subcat = da_cat_get (subcat_key);

						if (subcat != NULL)
						{
							is_visible = (subcat->flags & (GF_BUDGET|GF_FORCED));
						}
					}

					// Stop loop on first visible children
					if (is_visible)
					{
						break;
					}

					++child_id;
				}
			}
		}
	}

	return is_visible;
}

#if HB_BUD_TABVIEW_EDIT_ENABLE
// Filter rows to show only parent categories
static gboolean ui_bud_tabview_model_row_filter_parents (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
gboolean is_visible, is_root, is_total, is_separator, is_childheader;
Category *bdg_category;
guint32 category_key;
ui_bud_tabview_cat_type_t category_type;
ui_bud_tabview_view_mode_t view_mode = UI_BUD_TABVIEW_VIEW_SUMMARY;

	view_mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	is_visible = TRUE;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &category_type,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, &is_childheader,
		UI_BUD_TABVIEW_IS_SEPARATOR, &is_separator,
		-1);

	// Show root according to view_mode
	if (is_root)
	{
		// Always hide total root
		if(category_type == UI_BUD_TABVIEW_CAT_TYPE_NONE)
		{
			is_visible = FALSE;
		}
		else if(view_mode == UI_BUD_TABVIEW_VIEW_EXPENSE && category_type == UI_BUD_TABVIEW_CAT_TYPE_INCOME)
		{
			is_visible = FALSE;
		}
		else if(view_mode == UI_BUD_TABVIEW_VIEW_INCOME && category_type == UI_BUD_TABVIEW_CAT_TYPE_EXPENSE)
		{
			is_visible = FALSE;
		}
	}

	// Hide Total rows
	if (is_total) {
		is_visible = FALSE;
	}

	if (category_key > 0 && (is_separator || is_childheader))
	{
		is_visible = FALSE;
	}
	else if (category_key > 0)
	{
		// Hide rows according to currently view mode
		if(view_mode == UI_BUD_TABVIEW_VIEW_EXPENSE && category_type == UI_BUD_TABVIEW_CAT_TYPE_INCOME)
		{
			is_visible = FALSE;
		}
		else if(view_mode == UI_BUD_TABVIEW_VIEW_INCOME && category_type == UI_BUD_TABVIEW_CAT_TYPE_EXPENSE)
		{
			is_visible = FALSE;
		}
		else
		{
			// Show categories without parents
			bdg_category = da_cat_get(category_key);

			if (bdg_category != NULL)
			{
				if (bdg_category->parent > 0)
				{
					is_visible = FALSE;
				}
			}
		}
	}

	return is_visible;
}

// Filter rows to show only mergeable categories
static gboolean ui_bud_tabview_model_row_merge_filter_with_headers (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
gboolean is_visible, is_root, is_total, is_separator, is_childheader;
guint32 category_key;
ui_bud_tabview_cat_type_t category_type;

	is_visible = TRUE;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &category_type,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, &is_childheader,
		UI_BUD_TABVIEW_IS_SEPARATOR, &is_separator,
		-1);

	// Hide source merge row
	if (data->MERGE_source_category_key == category_key)
	{
		is_visible = FALSE;
	}

	// Hide Total root
	if (is_root
		&& category_type == UI_BUD_TABVIEW_CAT_TYPE_NONE )
	{
		is_visible = FALSE;
	}

	// Hide Total rows
	if (is_total)
	{
		is_visible = FALSE;
	}

	if ((is_separator || is_childheader))
	{
	GtkTreeIter parent;
		gtk_tree_model_iter_parent(model, &parent, iter);

		// Show child header and separator if parent has more than 2 children
		is_visible = (gtk_tree_model_iter_n_children(model, &parent) > 2);
	}

	return is_visible;
}
#endif

static gint ui_bud_tabview_model_row_sort (GtkTreeModel *model, GtkTreeIter *cat_a, GtkTreeIter *cat_b, gpointer user_data)
{
const gchar* cat_a_name;
const gchar* cat_b_name;
ui_bud_tabview_cat_type_t cat_a_type, cat_b_type;
guint32 cat_a_key, cat_b_key;
gboolean cat_a_is_childheader, cat_a_is_separator, cat_b_is_childheader, cat_b_is_separator;
gint order = 0;

	gtk_tree_model_get(model, cat_a,
		UI_BUD_TABVIEW_CATEGORY_NAME, &cat_a_name,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &cat_a_type,
		UI_BUD_TABVIEW_CATEGORY_KEY, &cat_a_key,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, &cat_a_is_childheader,
		UI_BUD_TABVIEW_IS_SEPARATOR, &cat_a_is_separator,
		-1);

	gtk_tree_model_get(model, cat_b,
		UI_BUD_TABVIEW_CATEGORY_NAME, &cat_b_name,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &cat_b_type,
		UI_BUD_TABVIEW_CATEGORY_KEY, &cat_b_key,
		UI_BUD_TABVIEW_IS_CHILD_HEADER, &cat_b_is_childheader,
		UI_BUD_TABVIEW_IS_SEPARATOR, &cat_b_is_separator,
		-1);

	// Sort first by category type
	if (cat_a_type != cat_b_type)
	{
		switch (cat_a_type)
		{
			case UI_BUD_TABVIEW_CAT_TYPE_INCOME:
				order = -1;
				break;
			case UI_BUD_TABVIEW_CAT_TYPE_EXPENSE:
				order = 0;
				break;
			case UI_BUD_TABVIEW_CAT_TYPE_NONE:
				order = 1;
				break;
		}
	}
	else
	{
		// On standard categories, just order by name
		if (!cat_a_is_childheader && !cat_a_is_separator
		    && !cat_b_is_childheader && !cat_b_is_separator)
		{
			order = g_utf8_collate(g_utf8_casefold(cat_a_name, -1),
				g_utf8_casefold(cat_b_name, -1)
				);
		}
		// Otherwise, fake categories have to be first (header and separator)
		else if (cat_a_is_childheader || cat_a_is_separator)
		{
			if (!cat_b_is_separator && !cat_b_is_childheader)
			{
				order = -1;
			}
			// When both are fake, header has to be first
			else
			{
				order = (cat_a_is_childheader ? -1 : 1);
			}
		}
		else
		{
			// Same idea for fake categories when cat_b is fake, but
			// with reversed result, because sort function return
			// result according to cat_a

			if (!cat_a_is_separator && !cat_a_is_childheader)
			{
				order = 1;
			}
			else
			{
				order = (cat_b_is_childheader ? 1 : -1);
			}
		}
	}

	return order;
}

// the budget model creation
static GtkTreeModel * ui_bud_tabview_model_new ()
{
GtkTreeStore *budget;
GtkTreeIter *iter_income, *iter_expense;
guint32 n_category;
ui_bud_tabview_search_criteria_t root_search = ui_bud_tabview_search_criteria_default;

	// Create Tree Store
	budget = gtk_tree_store_new ( UI_BUD_TABVIEW_NUMBER_COLOMNS,
		G_TYPE_UINT, // UI_BUD_TABVIEW_CATEGORY_KEY
		G_TYPE_STRING, // UI_BUD_TABVIEW_CATEGORY_NAME
		G_TYPE_STRING, // UI_BUD_TABVIEW_CATEGORY_FULLNAME
		G_TYPE_INT, // UI_BUD_TABVIEW_CATEGORY_TYPE
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_ROOT
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_TOTAL
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_CHILD_HEADER
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_SEPARATOR
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_MONITORING_FORCED
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_SAME_AMOUNT
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_IS_SUB_CATEGORY
		G_TYPE_BOOLEAN, // UI_BUD_TABVIEW_HAS_BUDGET
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_SAME_AMOUNT
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_JANUARY
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_FEBRUARY
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_MARCH
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_APRIL
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_MAY
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_JUNE
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_JULY
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_AUGUST
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_SEPTEMBER
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_OCTOBER
		G_TYPE_DOUBLE, // UI_BUD_TABVIEW_NOVEMBER
		G_TYPE_DOUBLE  // UI_BUD_TABVIEW_DECEMBER
	);

	// Populate the store

	/* Create tree roots */
	ui_bud_tabview_model_insert_roots (budget);

	// Retrieve required root
	root_search.row_is_root = TRUE;
	root_search.row_is_total = FALSE;

	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_INCOME;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);
	iter_income = root_search.iterator;

	root_search.row_category_type = UI_BUD_TABVIEW_CAT_TYPE_EXPENSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
		&root_search);
	iter_expense = root_search.iterator;

	/* Create rows for real categories */
	n_category = da_cat_get_max_key();

	for(guint32 i=1; i<=n_category; ++i)
	{
	Category *bdg_category;
	gboolean cat_is_income;

		bdg_category = da_cat_get(i);

		if (bdg_category == NULL)
		{
			continue;
		}

		cat_is_income = (category_type_get (bdg_category) == 1);

		DB(g_print(" category %d:'%s' isincome=%d, issub=%d hasbudget=%d parent=%d\n",
			bdg_category->key, bdg_category->name,
			cat_is_income, (bdg_category->flags & GF_SUB),
			(bdg_category->flags & GF_BUDGET), bdg_category->parent));

		// Compute totals and initiate category in right tree root
		if (cat_is_income)
		{
			ui_bud_tabview_model_add_category_with_lineage(budget, iter_income, &(bdg_category->key));
		}
		else if (!cat_is_income)
		{
			ui_bud_tabview_model_add_category_with_lineage(budget, iter_expense, &(bdg_category->key));
		}
	}

	/* Create rows for total root */
	ui_bud_tabview_model_update_monthly_total(GTK_TREE_STORE(budget));

	/* Sort categories on same node level */
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(budget),
		UI_BUD_TABVIEW_CATEGORY_NAME, ui_bud_tabview_model_row_sort,
		NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (budget),
		UI_BUD_TABVIEW_CATEGORY_NAME, GTK_SORT_ASCENDING);

	g_free(root_search.iterator);

	return GTK_TREE_MODEL(budget);
}

/**
 * GtkTreeView functions
 **/

static void ui_bud_tabview_icon_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
gchar *iconname = NULL;
gboolean has_budget, is_monitoring_forced;
ui_bud_tabview_view_mode_t view_mode = UI_BUD_TABVIEW_VIEW_SUMMARY;
	
	gtk_tree_model_get(model, iter,
	UI_BUD_TABVIEW_IS_MONITORING_FORCED, &is_monitoring_forced,
	UI_BUD_TABVIEW_HAS_BUDGET, &has_budget,
	-1);

	view_mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	//5.3 added
	if( is_monitoring_forced )
		iconname = ICONNAME_HB_OPE_FORCED;
	else
		if (view_mode != UI_BUD_TABVIEW_VIEW_SUMMARY )
		{
			if( has_budget )
				iconname = ICONNAME_HB_OPE_BUDGET;
		}

	g_object_set(renderer, "icon-name", iconname, NULL);
}



// Display category name in bold if it has budget
static void ui_bud_tabview_view_display_category_name (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
gboolean has_budget, is_sub_category;
PangoWeight weight = PANGO_WEIGHT_NORMAL;
ui_bud_tabview_view_mode_t view_mode = UI_BUD_TABVIEW_VIEW_SUMMARY;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_IS_SUB_CATEGORY, &is_sub_category,
		UI_BUD_TABVIEW_HAS_BUDGET, &has_budget,
		-1);

	view_mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	if (view_mode != UI_BUD_TABVIEW_VIEW_SUMMARY && has_budget)
	{
		weight = PANGO_WEIGHT_BOLD;
	}

	g_object_set(renderer,
		//"style", is_sub_category ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
		"weight", weight,
		NULL);
}

// to enable or not edition on month columns
static void ui_bud_tabview_view_display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GtkAdjustment *adjustment;
gboolean is_same_amount, is_root, is_total, is_visible, is_editable;
ui_bud_tabview_cat_type_t row_category_type;
gdouble amount = 0.0;
gchar *text;
gchar *fgcolor;
const ui_bud_tabview_store_t column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_CATEGORY_TYPE, &row_category_type,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, &is_same_amount,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		-1);

	// Text to display
	if (is_same_amount)
	{
		gtk_tree_model_get(model, iter, UI_BUD_TABVIEW_SAME_AMOUNT, &amount, -1);
	}
	else if (column_id >= UI_BUD_TABVIEW_JANUARY && column_id <= UI_BUD_TABVIEW_DECEMBER)
	{
		gtk_tree_model_get(model, iter, column_id, &amount, -1);
	}

	text = g_strdup_printf("%.2f", amount);
	fgcolor = get_normal_color_amount(amount);

	// Default styling values
	is_visible = TRUE;
	is_editable = FALSE;

	if (is_root)
	{
		is_visible = FALSE;
		is_editable = FALSE;
	}
	else if (is_total)
	{
		is_visible = TRUE;
		is_editable = FALSE;

		if (column_id == UI_BUD_TABVIEW_SAME_AMOUNT)
		{
			is_visible = FALSE;
		}
	}
	else if (is_same_amount)
	{
		is_visible = TRUE;
		is_editable = FALSE;

		if (column_id == UI_BUD_TABVIEW_SAME_AMOUNT)
		{
			is_editable = TRUE;
		}
	}
	else if (! is_same_amount)
	{
		is_visible = TRUE;
		is_editable = TRUE;

		if (column_id == UI_BUD_TABVIEW_SAME_AMOUNT)
		{
			is_editable = FALSE;
		}
	}

	// Finally, visibility depends on set amount
	is_visible = (is_visible && (is_editable || amount != 0.0));

	adjustment = gtk_adjustment_new(
		0.0, // initial-value
		-G_MAXDOUBLE, // minmal-value
		G_MAXDOUBLE, // maximal-value
		0.5, // step increment
		10, // page increment
		0); // page size (0 because irrelevant for GtkSpinButton)

	g_object_set(renderer,
		"text", text,
		"visible", is_visible,
		"editable", is_editable,
		"foreground", fgcolor,
		"xalign", 1.0f,
		"adjustment", adjustment,
		"digits", 2,
		NULL);

	g_free(text);
}

// to enable or not edition on month columns
static void ui_bud_tabview_view_display_is_same_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_same_amount, is_root, is_total, is_visible, is_sensitive;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, &is_same_amount,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		-1);

	// Default values
	is_visible = TRUE;
	is_sensitive = TRUE;

	if (is_root || is_total)
	{
		is_visible = FALSE;
		is_sensitive = FALSE;
	}

	g_object_set(renderer,
		"activatable", TRUE,
		"active", is_same_amount,
		"visible", is_visible,
		"sensitive", is_sensitive,
		NULL);
}

// Compute dynamically the annual total
static void ui_bud_tabview_view_display_annual_total (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_same_amount = FALSE, is_total = FALSE, is_root = FALSE;
gdouble amount = 0.0;
gdouble total = 0.0;
gchar *text;
gchar *fgcolor;
gboolean is_visible = TRUE;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, &is_same_amount,
		UI_BUD_TABVIEW_SAME_AMOUNT, &amount,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		-1);

	if (is_same_amount)
	{
		total = 12.0 * amount;
	}
	else
	{
		for (int i = UI_BUD_TABVIEW_JANUARY ; i <= UI_BUD_TABVIEW_DECEMBER ; ++i)
		{
			gtk_tree_model_get(model, iter, i, &amount, -1);
			total += amount;
		}
	}

	text = g_strdup_printf("%.2f", total);
	fgcolor = get_normal_color_amount(total);

	if (is_root)
	{
		is_visible = FALSE;
	}

	// Finally, visibility depends on set amount
	//is_visible = (is_visible && amount != 0.0);
	//#1859275 visibility to be tested on total
	is_visible = (is_visible && total != 0.0);

	g_object_set(renderer,
		"text", text,
		"foreground", fgcolor,
		"visible", is_visible,
		"xalign", 1.0f,
		NULL);

	g_free(text);
}

// Compute monthly average
static void ui_bud_tabview_view_display_monthly_average(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_same_amount = FALSE, is_total = FALSE, is_root = FALSE;
gdouble amount = 0.0;
gdouble average = 0.0;
gchar *text;
gchar *fgcolor;
gboolean is_visible = TRUE;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, &is_same_amount,
		UI_BUD_TABVIEW_SAME_AMOUNT, &amount,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		-1);

	if (is_same_amount)
	{
		average = amount;
	}
	else
	{
		for (int i = UI_BUD_TABVIEW_JANUARY ; i <= UI_BUD_TABVIEW_DECEMBER ; ++i)
		{
			gtk_tree_model_get(model, iter, i, &amount, -1);
			average += amount;
		}
		average = hb_amount_round(average / 12.0, 2);
	}

	text = g_strdup_printf("%.2f", average);
	fgcolor = get_normal_color_amount(average);

	if (is_root)
	{
		is_visible = FALSE;
	}

	// Finally, visibility depends on set amount
	is_visible = (is_visible && average != 0.0);

	g_object_set(renderer,
		"text", text,
		"foreground", fgcolor,
		"visible", is_visible,
		"xalign", 1.0f,
		NULL);

	g_free(text);
}

// When view mode is toggled:
// - recreate the view to update columns rendering
static void ui_bud_tabview_view_toggle (gpointer user_data, ui_bud_tabview_view_mode_t view_mode)
{
ui_bud_tabview_data_t *data = user_data;
GtkTreeModel *budget;
GtkWidget *view;
GtkTreePath* firstRow;

	view = data->TV_budget;

	budget = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(budget));

	if (data->TV_is_expanded)
	{
		gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
	}
	else
	{
		ui_bud_tabview_model_collapse(GTK_TREE_VIEW(view));
	}

	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)));

	firstRow = gtk_tree_path_new_from_string("0");
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(view), firstRow, data->TVC_category, TRUE, 0, 0);

	DB(g_print("[ui_bud_tabview] : button state changed to: %d\n", view_mode));

	return;
}

static gboolean ui_bud_tabview_view_search (GtkTreeModel *filter, gint column, const gchar *key, GtkTreeIter *filter_iter, gpointer data)
{
gboolean is_matching = FALSE, is_root, is_total;
GtkTreeModel *budget;
GtkTreeIter iter;
gchar *category_name;

	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		filter_iter);

	gtk_tree_model_get(budget, &iter,
		UI_BUD_TABVIEW_CATEGORY_NAME, &category_name,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		-1);

	if (!is_root && !is_total
		&& g_strstr_len(g_utf8_casefold(category_name, -1), -1,
			g_utf8_casefold(key, -1)))
	{
		is_matching = TRUE;
	}

	// GtkTreeViewSearchEqualFunc has to return FALSE only if iter matches.
	return !is_matching;
}

#if HB_BUD_TABVIEW_EDIT_ENABLE
static gboolean ui_bud_tabview_view_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
gboolean is_separator;

	gtk_tree_model_get(model, iter,
		UI_BUD_TABVIEW_IS_SEPARATOR, &is_separator,
		-1);

	return is_separator;
}
#endif

// the budget view creation which run the model creation tool

static GtkWidget *ui_bud_tabview_view_new (gpointer user_data)
{
GtkTreeViewColumn *col;
GtkCellRenderer *renderer, *cat_name_renderer;
GtkWidget *view;
ui_bud_tabview_data_t *data = user_data;

	view = gtk_tree_view_new();

	/* icon column */
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_bud_tabview_icon_cell_data_function, (gpointer) data, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), col);

	
	/* --- Category column --- */
	col = gtk_tree_view_column_new();
	data->TVC_category = col;

	gtk_tree_view_column_set_title(col, _("Category"));
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment(col, 0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	gtk_tree_view_set_expander_column(GTK_TREE_VIEW(view), col);
	
	// Category Name
	cat_name_renderer = gtk_cell_renderer_text_new();

	//#2004053 + add 5.6.2
	g_object_set(cat_name_renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	gtk_tree_view_column_set_min_width(col, HB_MINWIDTH_LIST);
	gtk_tree_view_column_set_resizable(col, TRUE);

	gtk_tree_view_column_pack_start (col, cat_name_renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, cat_name_renderer, "markup", UI_BUD_TABVIEW_CATEGORY_NAME);
	gtk_tree_view_column_set_cell_data_func(col, cat_name_renderer, ui_bud_tabview_view_display_category_name, (gpointer) data, NULL);
#if HB_BUD_TABVIEW_EDIT_ENABLE
	g_object_set(cat_name_renderer, "editable", TRUE, NULL);
	g_signal_connect(cat_name_renderer, "edited", G_CALLBACK(ui_bud_tabview_cell_update_category), (gpointer) data);
#endif

	/* --- Annual Total --- */
	col = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(col, _("Annual Total"));
	gtk_tree_view_column_set_title(col, _("Annual\nTotal"));
	//gtk_tree_view_column_set_title(col, _("Total"));
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment(col, 1.0);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_bud_tabview_view_display_annual_total, NULL, NULL);

	/* --- Monthly average --- */
	col = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(col, _("Monthly Average"));
	gtk_tree_view_column_set_title(col, _("Monthly\nAverage"));
	//gtk_tree_view_column_set_title(col, _("Average"));
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment(col, 1.0);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_bud_tabview_view_display_monthly_average, NULL, NULL);

	/* --- Monthly column --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Monthly"));
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment(col, 1.0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	// Monthly toggler
	renderer = gtk_cell_renderer_toggle_new();
	//5.7 fix memhit because value was nor float...
	g_object_set(renderer,
		"xalign", 0.0f,
		NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_bud_tabview_view_display_is_same_amount, NULL, NULL);

	g_signal_connect (renderer, "toggled", G_CALLBACK(ui_bud_tabview_cell_update_is_same_amount), (gpointer) data);

	// Monthly amount
	renderer = gtk_cell_renderer_spin_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_bud_tabview_view_display_amount, GINT_TO_POINTER(UI_BUD_TABVIEW_SAME_AMOUNT), NULL);

	g_object_set_data(G_OBJECT(renderer), "ui_bud_tabview_column_id", GINT_TO_POINTER(UI_BUD_TABVIEW_SAME_AMOUNT));

	g_signal_connect(renderer, "edited", G_CALLBACK(ui_bud_tabview_cell_update_amount), (gpointer) data);

	/* --- Each month amount --- */
	for (int i = UI_BUD_TABVIEW_JANUARY ; i <= UI_BUD_TABVIEW_DECEMBER ; ++i)
	{
		int month = i - UI_BUD_TABVIEW_JANUARY ;
		col = gtk_tree_view_column_new();

		gtk_tree_view_column_set_title(col, _(UI_BUD_TABVIEW_MONTHS[month]));
		//#2004631 date and column title alignement
		gtk_tree_view_column_set_alignment(col, 1.0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		renderer = gtk_cell_renderer_spin_new();

		gtk_tree_view_column_pack_start(col, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(col, renderer, ui_bud_tabview_view_display_amount, GINT_TO_POINTER(i), NULL);

		g_object_set_data(G_OBJECT(renderer), "ui_bud_tabview_column_id", GINT_TO_POINTER(i));

		g_signal_connect(renderer, "edited", G_CALLBACK(ui_bud_tabview_cell_update_amount), (gpointer) data);

	}

	/* --- Empty column to expand according to the window width --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), UI_BUD_TABVIEW_CATEGORY_NAME);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view), (GtkTreeViewSearchEqualFunc) ui_bud_tabview_view_search, NULL, NULL);

	g_object_set(view,
		"enable-grid-lines", PREFS->grid_lines,
		"enable-tree-lines", FALSE,
		NULL);

	return view;
}

/*
 * UI actions
 **/
#if HB_BUD_TABVIEW_EDIT_ENABLE
// Update homebank category on user change
static void ui_bud_tabview_cell_update_category(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
guint32 category_key;
gboolean is_root, is_total;

	DB(g_print("\n[ui_bud_tabview] category name updated with new name '%s'\n", new_text));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		UI_BUD_TABVIEW_IS_ROOT, &is_root,
		UI_BUD_TABVIEW_IS_TOTAL, &is_total,
		-1);

	category = da_cat_get (category_key);

	if (! category || is_root || is_total)
	{
		return;
	}

	// Update category name
	category_rename(category, new_text);

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		UI_BUD_TABVIEW_CATEGORY_NAME, category->name,
		UI_BUD_TABVIEW_CATEGORY_FULLNAME, category->fullname,
		-1);

	return;
}
#endif


// Update amount in budget model and homebank category on user change
static void ui_bud_tabview_cell_update_amount(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data)
{
const ui_bud_tabview_store_t column_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer), "ui_bud_tabview_column_id"));
ui_bud_tabview_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
gdouble amount;
guint32 category_key;

	DB(g_print("\n[ui_bud_tabview] amount updated:\n"));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		-1);

	category = da_cat_get (category_key);

	if (! category)
	{
		return;
	}

	amount = g_strtod(new_text, NULL);

	DB(g_print("\tcolumn: %d (month: %d), category key: %d, amount %.2f\n", column_id, column_id - UI_BUD_TABVIEW_JANUARY + 1, category_key, amount));

	// Update Category
	category->budget[column_id - UI_BUD_TABVIEW_JANUARY + 1] = amount;

	// Reset Budget Flag
	category->flags &= ~(GF_BUDGET);
	if (category->flags & GF_FORCED)
	{
		category->flags |= GF_BUDGET;
	}
	else
	{
		for(gint budget_id = 0; budget_id <=12; ++budget_id)
		{
			if( category->budget[budget_id] != 0.0)
			{
				category->flags |= GF_BUDGET;
				break;
			}
		}
	}

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		UI_BUD_TABVIEW_HAS_BUDGET, (category->flags & GF_BUDGET),
		column_id, amount,
		-1);

	// Refresh total rows
	ui_bud_tabview_model_update_monthly_total (GTK_TREE_STORE(budget));

	return;
}

// Update the row to (dis/enable) same amount for this category
static void ui_bud_tabview_cell_update_is_same_amount(GtkCellRendererText *renderer, gchar *filter_path, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
gboolean issame;
guint32 category_key;

	DB(g_print("\n[ui_bud_tabview] Is same amount updated:\n"));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		UI_BUD_TABVIEW_CATEGORY_KEY, &category_key,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, &issame,
		-1);

	category = da_cat_get (category_key);

	if (! category)
	{
		return;
	}

	// Value has been toggled !
	issame = !(issame);

	DB(g_print("\tcategory key: %d, issame: %d (before: %d)\n", category_key, issame, !(issame)));

	// Update Category

	// Reset Forced Flag
	category->flags &= ~(GF_CUSTOM);

	if (issame == FALSE)
	{
		category->flags |= (GF_CUSTOM);
	}

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		UI_BUD_TABVIEW_IS_SAME_AMOUNT, issame,
		-1);

	// Refresh total rows
	ui_bud_tabview_model_update_monthly_total (GTK_TREE_STORE(budget));

	return;
}


// Update budget view and model according to the new view mode selected
static void ui_bud_tabview_view_update_mode (GtkToggleButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
ui_bud_tabview_view_mode_t view_mode = UI_BUD_TABVIEW_VIEW_SUMMARY;

	// Only run once the view update, so only run on the activated button signal
	if(!gtk_toggle_button_get_active(button))
	{
		return;
	}

	// Mode is directly set by radio button, because the UI_BUD_TABVIEW_VIEW_MODE and enum
	// for view mode are constructed to correspond (manually)
	view_mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	DB(g_print("\n[ui_bud_tabview] view mode toggled to: %d\n", view_mode));

	ui_bud_tabview_view_toggle((gpointer) data, view_mode);

	return;
}

// Expand all categories inside the current view
static void ui_bud_tabview_view_expand (GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *view;

	view = data->TV_budget;

	data->TV_is_expanded = TRUE;

	gtk_tree_view_expand_all(GTK_TREE_VIEW(view));

	return;
}

// Collapse all categories inside the current view
static void ui_bud_tabview_view_collapse (GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *view;

	view = data->TV_budget;

	data->TV_is_expanded = FALSE;

	ui_bud_tabview_model_collapse (GTK_TREE_VIEW(view));

	return;
}


// From TreeView, retrieve the category, the budget TreeStore and the iter inside that store.
//   * budget: a GtkTreeModel
//   * iter: an unintialized GtkTreeIter
//   * category: a pointer of Category which will point to the found Category
// If category is not retrieved, return FALSE and reset budget, iter and category.
// That's especillaly useful for buttons outside the GtkTreeView.
// Warning: iter has to be already allocated
static gboolean ui_bud_tabview_get_selected_category (GtkTreeModel **budget, GtkTreeIter *iter, Category **category, ui_bud_tabview_data_t *data)
{
GtkWidget *view;
GtkTreeModel *filter;
GtkTreeSelection *selection;
GtkTreeIter filter_iter;
guint32 item_key;

	DB( g_print("[ui_bud_tabview_get_selected_category] retrieve category from selected row\n") );

	view = data->TV_budget;

	// Read filter to retrieve the currently selected row
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	*budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));

	// Retrieve selected row from filter if possible
	if (gtk_tree_selection_get_selected(selection, &filter, &filter_iter))
	{
		// Convert data to budget model
		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
			iter,
			&filter_iter);

		// Avoid to return fake categories
		gtk_tree_model_get (*budget, iter,
			UI_BUD_TABVIEW_CATEGORY_KEY, &item_key,
			-1);

		if (item_key != 0)
		{
			DB(g_print("look category with key: %d\n", item_key));

			*category = da_cat_get(item_key);

			return (*category != NULL);
		}
	}

	return FALSE;
}

#if HB_BUD_TABVIEW_EDIT_ENABLE
// From TreeView, retrieve the the budget TreeStore and the root iter inside that store.
//   * budget: a GtkTreeModel
//   * iter: an unintialized GtkTreeIter
// Return true if selected row is a root row.
// Warning: iter has to be already allocated
static gboolean ui_bud_tabview_get_selected_root_iter (GtkTreeModel **budget, GtkTreeIter *iter, ui_bud_tabview_data_t *data)
{
GtkWidget *view;
GtkTreeModel *filter;
GtkTreeSelection *selection;
GtkTreeIter filter_iter;
gboolean is_root;

	DB( g_print("[ui_bud_tabview_get_selected_root_iter] retrieve root iter from selected row\n") );

	view = data->TV_budget;

	// Read filter to retrieve the currently selected row
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	*budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));

	// Retrieve selected row from filter if possible
	if (gtk_tree_selection_get_selected(selection, &filter, &filter_iter))
	{
		// Convert data to budget model
		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
			iter,
			&filter_iter);

		gtk_tree_model_get (*budget, iter,
			UI_BUD_TABVIEW_IS_ROOT, &is_root,
			-1);

		return (is_root);
	}

	return FALSE;
}


// Check if add category dialog is full filled
static void ui_bud_tabview_category_add_full_filled (GtkWidget *source, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
const gchar* new_raw_name;
gchar* new_name;
gboolean is_name_filled = FALSE, is_parent_choosen = FALSE;

	// Check a name for the new category is given:
	new_raw_name = gtk_entry_get_text(GTK_ENTRY(data->EN_add_name));

	if (new_raw_name && *new_raw_name)
	{
		new_name = g_strdup(new_raw_name);
		g_strstrip(new_name);

		if (strlen(new_name) > 0)
		{
			is_name_filled = TRUE;
		}

		g_free(new_name);
	}

	// Check an entry has been selected in parent combobox
	is_parent_choosen = (gtk_combo_box_get_active(GTK_COMBO_BOX(data->COMBO_add_parent)) > -1);

	// Dis/Enable apply dialog button
	gtk_widget_set_sensitive(data->BT_apply, is_name_filled && is_parent_choosen);

	return;
}

// Add a category according to the current selection
static void ui_bud_tabview_category_add (GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *apply;
ui_bud_tabview_view_mode_t view_mode;
GtkTreeModel *budget, *categories;
GtkTreeIter default_parent_iter, categories_iter;
GtkWidget *dialog, *content, *grid, *combobox, *textentry, *widget;
GtkCellRenderer *renderer;
Category *category;
gint gridrow, response;
gboolean exists_default_select = FALSE;

	view_mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	// Setup budget and retrieve default selection from budget dialog
	if (ui_bud_tabview_get_selected_category (&budget, &default_parent_iter, &category, data))
	{
		exists_default_select = TRUE;

		if (category->parent != 0)
		{
		ui_bud_tabview_search_criteria_t parent_search = ui_bud_tabview_search_criteria_default;
			parent_search.row_category_key = category->parent;

			gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
				(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
				&parent_search);

			if (!parent_search.iterator) {
				DB(g_print(" -> error: not found good parent iterator !\n"));
				return;
			}

			default_parent_iter = *parent_search.iterator;

			g_free(parent_search.iterator);
		}
	}
	else if (ui_bud_tabview_get_selected_root_iter (&budget, &default_parent_iter, data))
	{
		// If currently selected row is a root row, use it as default value
		exists_default_select = TRUE;
	}

	// Selectable categories from original model
	categories = gtk_tree_model_filter_new(budget, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(categories),
		ui_bud_tabview_model_row_filter_parents, data, NULL);

	if (exists_default_select)
	{
		gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(categories),
			&categories_iter,
			&default_parent_iter);
	}

	DB( g_print("[ui_bud_tabview] open sub-dialog to add a category\n") );

	dialog = gtk_dialog_new_with_buttons (_("Add a category"),
		GTK_WINDOW(data->dialog),
		GTK_DIALOG_MODAL,
		_("_Cancel"),
		GTK_RESPONSE_CANCEL,
		NULL);

	// Apply button will be enabled only when parent category and name are choosen
	apply = gtk_dialog_add_button(GTK_DIALOG(dialog),
		_("_Apply"),
		GTK_RESPONSE_APPLY);
	data->BT_apply = apply;
	gtk_widget_set_sensitive(apply, FALSE);

	//window contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	// design content
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	hb_widget_set_margin(GTK_WIDGET(grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content), grid, FALSE, FALSE, 0);

	// First row display parent selector
	gridrow = 0;

	widget = gtk_label_new(_("Parent category"));
	gtk_grid_attach (GTK_GRID (grid), widget, 0, gridrow, 1, 1);

	combobox = gtk_combo_box_new_with_model(categories);
	data->COMBO_add_parent = combobox;
	gtk_grid_attach (GTK_GRID (grid), combobox, 1, gridrow, 1, 1);

	gtk_combo_box_set_row_separator_func(
		GTK_COMBO_BOX(combobox),
		ui_bud_tabview_view_separator,
		data,
		NULL
	);

	gtk_combo_box_set_id_column(GTK_COMBO_BOX(combobox), UI_BUD_TABVIEW_CATEGORY_KEY);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combobox), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "text", UI_BUD_TABVIEW_CATEGORY_FULLNAME);

	// Next row displays the new category entry
	gridrow++;

	widget = gtk_label_new(_("Category name"));
	gtk_grid_attach (GTK_GRID (grid), widget, 0, gridrow, 1, 1);

	textentry = gtk_entry_new();
	data->EN_add_name = textentry;
	gtk_grid_attach (GTK_GRID (grid), textentry, 1, gridrow, 1, 1);

	// Signals to enable Apply button
	g_signal_connect (data->COMBO_add_parent, "changed", G_CALLBACK(ui_bud_tabview_category_add_full_filled), (gpointer)data);
	g_signal_connect (data->EN_add_name, "changed", G_CALLBACK(ui_bud_tabview_category_add_full_filled), (gpointer)data);

	if (exists_default_select)
	{
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combobox), &categories_iter);
	}

	gtk_widget_show_all (dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	// When the response is APPLY, the form was full filled
	if (response == GTK_RESPONSE_APPLY) {
	Category *new_item;
	const gchar *new_name;
	gchar *parent_name;
	guint32 parent_key;
	ui_bud_tabview_cat_type_t parent_type;
	ui_bud_tabview_search_criteria_t root_search = ui_bud_tabview_search_criteria_default;
	GtkTreeIter parent_iter, *root_iter;

		DB( g_print("[ui_bud_tabview] applying creation of a new category\n") );

		// Retrieve info from dialog
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox), &categories_iter);

		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(categories),
			&parent_iter,
			&categories_iter);

		gtk_tree_model_get (budget, &parent_iter,
			UI_BUD_TABVIEW_CATEGORY_NAME, &parent_name,
			UI_BUD_TABVIEW_CATEGORY_KEY, &parent_key,
			UI_BUD_TABVIEW_CATEGORY_TYPE, &parent_type,
			-1);

		DB( g_print(" -> from parent cat: %s (key: %d, type: %d)\n",
			parent_name, parent_key ,parent_type) );

		// Retrieve required root
		root_search.row_is_root = TRUE;
		root_search.row_is_total = FALSE;
		root_search.row_category_type = parent_type;
		gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
			(GtkTreeModelForeachFunc) ui_bud_tabview_model_search_iterator,
			&root_search);

		if (!root_search.iterator) {
			DB(g_print(" -> error: not found good tree root !\n"));
			return;
		}

		root_iter = root_search.iterator;

		// Build new category from name and parent iterator
		new_name = gtk_entry_get_text(GTK_ENTRY(textentry));

		data->change++;
		new_item = da_cat_malloc();
		new_item->name = g_strdup(new_name);
		g_strstrip(new_item->name);

		new_item->parent = parent_key;

		if (parent_key)
		{
			new_item->flags |= GF_SUB;
		}

		if (parent_type == UI_BUD_TABVIEW_CAT_TYPE_INCOME)
		{
			new_item->flags |= GF_INCOME;
		}

		// On balance mode, enable forced display too to render it to user
		if (view_mode == UI_BUD_TABVIEW_VIEW_SUMMARY)
		{
			new_item->flags |= GF_FORCED;
		}

		if(da_cat_append(new_item))
		{
		GtkWidget *view;
		GtkTreeModel *filter;
		GtkTreeIter filter_iter;
		GtkTreePath *path;

			DB( g_print(" => add cat: %p (%d), type=%d\n", new_item->name, new_item->key, category_type_get(new_item)) );

			// Finally add it to model
			ui_bud_tabview_model_add_category_with_lineage (GTK_TREE_STORE(budget), root_iter, &(new_item->key));

			// Expand view up to the newly added item, so expand its parent which is already known as iter

			view = data->TV_budget;
			filter = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

			if(gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(filter),
				&filter_iter,
				&parent_iter)
			)
			{
				path = gtk_tree_model_get_path(filter, &filter_iter);
				gtk_tree_view_expand_row(GTK_TREE_VIEW(view), path, TRUE);
			}
		}
	}

	gtk_window_destroy (GTK_WINDOW(dialog));

	return;
}

// Delete a category according to the current selection
static void ui_bud_tabview_category_delete (GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkTreeModel *budget;
GtkTreeIter iter;
Category* category;
gint response;

	DB( g_print("[ui_bud_tabview] open sub-dialog to delete a category\n") );

	// Retrieve selected row from filter if possible
	if (ui_bud_tabview_get_selected_category (&budget, &iter, &category, data))
	{
	gchar *title = NULL;
	gchar *secondtext = NULL;

		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), category->name);

		if( category->usage_count > 0 )
		{
			secondtext = _("This category is used.\n"
				"Any transaction using that category will be set to (no category)");
		}

		response = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->dialog),
				title,
				secondtext,
				_("_Delete"),
				TRUE
			);

		g_free(title);

		if( response == GTK_RESPONSE_OK )
		{
			gtk_tree_store_remove(GTK_TREE_STORE(budget),
				&iter);

			category_move(category->key, 0);
			da_cat_delete(category->key);
			data->change++;
		}
	}

	return;
}

// Check if add category dialog is full filled
static void ui_bud_tabview_category_merge_full_filled (GtkWidget *source, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
gboolean is_target_choosen = FALSE;

	is_target_choosen = (gtk_combo_box_get_active(GTK_COMBO_BOX(data->COMBO_merge_target)) > -1);

	// Dis/Enable apply dialog button
	gtk_widget_set_sensitive(data->BT_apply, is_target_choosen);

	return;
}

static void ui_bud_tabview_category_merge (GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *apply;
GtkTreeModel *budget, *categories;
GtkTreeIter iter_source, iter, categories_iter;
GtkWidget *dialog, *content, *grid, *combobox, *widget, *checkbutton;
GtkCellRenderer *renderer;
gint gridrow, response;
Category *merge_source;
gchar *label_source, *label_delete;

	// Retrieve source of merge
	if (ui_bud_tabview_get_selected_category(&budget, &iter_source, &merge_source, data))
	{
		DB( g_print("[ui_bud_tabview] open sub-dialog to merge category: %s\n", merge_source->name) );

		dialog = gtk_dialog_new_with_buttons (_("Merge categories"),
			GTK_WINDOW(data->dialog),
			GTK_DIALOG_MODAL,
			_("_Cancel"),
			GTK_RESPONSE_CANCEL,
			NULL);

		// Apply button will be enabled only when a target merge category is choosen
		apply = gtk_dialog_add_button(GTK_DIALOG(dialog),
			_("_Apply"),
			GTK_RESPONSE_APPLY);
		data->BT_apply = apply;
		gtk_widget_set_sensitive(apply, FALSE);

		//window contents
		content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

		// design content
		grid = gtk_grid_new ();
		gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_MEDIUM);
		gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
		hb_widget_set_margin(GTK_WIDGET(grid), SPACING_MEDIUM);
		gtk_box_pack_start (GTK_BOX (content), grid, FALSE, FALSE, 0);

		// First row display parent selector
		gridrow = 0;

		label_source = g_strdup_printf(_("Transactions assigned to category '%s', will be moved to the category selected below."), merge_source->name);
		widget = gtk_label_new (label_source);
		gtk_grid_attach (GTK_GRID (grid), widget, 0, gridrow, 4, 1);

		// Line to select merge target
		gridrow++;

		widget = gtk_label_new(_("Target category"));
		gtk_grid_attach (GTK_GRID (grid), widget, 1, gridrow, 1, 1);

		// Target category list is built from original model with a filter
		categories = gtk_tree_model_filter_new(budget, NULL);

		data->MERGE_source_category_key = merge_source->key;

		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(categories),
			ui_bud_tabview_model_row_merge_filter_with_headers, data, NULL);

		combobox = gtk_combo_box_new_with_model(categories);
		data->COMBO_merge_target = combobox;
		gtk_grid_attach (GTK_GRID (grid), combobox, 2, gridrow, 1, 1);

		gtk_combo_box_set_row_separator_func(
			GTK_COMBO_BOX(combobox),
			ui_bud_tabview_view_separator,
			data,
			NULL
		);

		gtk_combo_box_set_id_column(GTK_COMBO_BOX(combobox), UI_BUD_TABVIEW_CATEGORY_KEY);

		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combobox), renderer, TRUE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "text", UI_BUD_TABVIEW_CATEGORY_FULLNAME);

		// Next row displays the automatic delete option
		gridrow++;

		label_delete = g_strdup_printf (
			_("_Delete the category '%s'"), merge_source->name);
		checkbutton = gtk_check_button_new_with_mnemonic(label_delete);
		gtk_grid_attach (GTK_GRID (grid), checkbutton, 0, gridrow, 4, 1);

		// Signals to enable Apply button
		g_signal_connect (data->COMBO_merge_target, "changed", G_CALLBACK(ui_bud_tabview_category_merge_full_filled), (gpointer)data);

		gtk_widget_show_all (dialog);

		response = gtk_dialog_run (GTK_DIALOG (dialog));

		// When the response is APPLY, the form was full filled
		if (response == GTK_RESPONSE_APPLY)
		{
		Category *merge_target, *parent_source;
		gint target_key;

			DB( g_print("[ui_bud_tabview] applying merge\n") );

			// Retrieve info from dialog
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox), &categories_iter);

			gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(categories),
				&iter,
				&categories_iter);

			gtk_tree_model_get (budget, &iter,
				UI_BUD_TABVIEW_CATEGORY_KEY, &target_key,
				-1);

			merge_target = da_cat_get(target_key);

			DB( g_print(" -> to target category: %s (key: %d)\n",
				merge_target->name, target_key) );

			// Merge categories (according to ui-category.c)
			category_move(merge_source->key, merge_target->key);

			merge_target->usage_count += merge_source->usage_count;
			merge_source->usage_count = 0;

			// Keep the income type with us
			parent_source = da_cat_get(merge_source->parent);
			if(parent_source != NULL && (parent_source->flags & GF_INCOME))
			{
				merge_target->flags |= GF_INCOME;
			}

			// Clean source merge
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
			{
				da_cat_delete(merge_source->key);

				gtk_tree_store_remove(GTK_TREE_STORE(budget),
					&iter_source);
			}

			data->change++;
		}

		data->MERGE_source_category_key = 0;

		gtk_window_destroy (GTK_WINDOW(dialog));

		g_free(label_source);
		g_free(label_delete);
	}

	return;
}
#endif


// Reset inputs done for the currently selected category
static void ui_bud_tabview_category_reset (GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkTreeModel *budget;
GtkTreeIter iter;
Category* category;
gint response;

	DB( g_print("[ui_bud_tabview] open sub-dialog to confirm category reset\n") );

	// Retrieve selected row from filter if possible
	if (ui_bud_tabview_get_selected_category (&budget, &iter, &category, data))
	{
	gchar *title = NULL;
	gchar *secondtext = NULL;

		title = g_strdup_printf (
			_("Are you sure you want to clear inputs for '%s'?"), category->name);
		secondtext = _("If you proceed, every amount will be set to 0.");

		response = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->dialog),
				title,
				secondtext,
				_("_Clear"),
				TRUE
			);

		g_free(title);

		if( response == GTK_RESPONSE_OK )
		{
			// Update data
			for(int i=0;i<=12;i++)
			{
				category->budget[i] = 0;
			}

			// Reset budget flag according to GF_FORCED
			category->flags &= ~(GF_BUDGET);

			//mdo: no
			/*if (category->flags & GF_FORCED)
			{
				category->flags |= GF_BUDGET;
			}*/

			data->change++;

			// Update GtkTreeStore
			gtk_tree_store_set(
				GTK_TREE_STORE(budget),
				&iter,
				UI_BUD_TABVIEW_HAS_BUDGET, (category->flags & GF_BUDGET),
				UI_BUD_TABVIEW_SAME_AMOUNT, category->budget[0],
				UI_BUD_TABVIEW_JANUARY, category->budget[1],
				UI_BUD_TABVIEW_FEBRUARY, category->budget[2],
				UI_BUD_TABVIEW_MARCH, category->budget[3],
				UI_BUD_TABVIEW_APRIL, category->budget[4],
				UI_BUD_TABVIEW_MAY, category->budget[5],
				UI_BUD_TABVIEW_JUNE, category->budget[6],
				UI_BUD_TABVIEW_JULY, category->budget[7],
				UI_BUD_TABVIEW_AUGUST, category->budget[8],
				UI_BUD_TABVIEW_SEPTEMBER, category->budget[9],
				UI_BUD_TABVIEW_OCTOBER, category->budget[10],
				UI_BUD_TABVIEW_NOVEMBER, category->budget[11],
				UI_BUD_TABVIEW_DECEMBER, category->budget[12],
				-1);

			// Refresh total rows
			ui_bud_tabview_model_update_monthly_total (GTK_TREE_STORE(budget));
		}
	}

	return;
}



static void ui_bud_tabview_category_toggle_monitoring(GtkButton *button, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkTreeModel *budget;
GtkTreeIter iter;
Category* category;

	DB( g_print("[ui_bud_tabview] open sub-dialog to confirm category reset\n") );

	// Retrieve selected row from filter if possible
	if (ui_bud_tabview_get_selected_category (&budget, &iter, &category, data))
	{
	gboolean is_monitoring_forced;

		// Toggle monitoring
		is_monitoring_forced = !(category->flags & GF_FORCED);

		// Update Category

		// Reset Forced and Budget Flags
		category->flags &= ~(GF_FORCED);
		category->flags &= ~(GF_BUDGET);

		if (is_monitoring_forced == TRUE)
		{
			category->flags |= (GF_FORCED);
			category->flags |= (GF_BUDGET);
		}
		else
		{
			for(gint budget_id = 0; budget_id <=12; ++budget_id)
			{
				if( category->budget[budget_id] != 0.0)
				{
					category->flags |= GF_BUDGET;
					break;
				}
			}
		}

		// Notify of changes
		data->change++;

		// Update budget model

		// Current row
		gtk_tree_store_set(
			GTK_TREE_STORE(budget),
			&iter,
			UI_BUD_TABVIEW_IS_MONITORING_FORCED, is_monitoring_forced,
			UI_BUD_TABVIEW_HAS_BUDGET, (category->flags & GF_BUDGET),
			-1);

		// Refresh total rows
		ui_bud_tabview_model_update_monthly_total (GTK_TREE_STORE(budget));
	}

	return;
}



static gboolean ui_bud_tabview_on_key_press(GtkWidget *source, GdkEventKey *event, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;

	// On Control-f enable search entry
	if (event->state & GDK_CONTROL_MASK
		&& event->keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->EN_search);
	}

	return GDK_EVENT_PROPAGATE;
}


static void ui_bud_tabview_view_on_select(GtkTreeSelection *treeselection, gpointer user_data)
{
ui_bud_tabview_data_t *data = user_data;
GtkWidget *view;
GtkTreeModel *filter, *budget;
GtkTreeIter filter_iter, iter;
GtkTreeSelection *selection;
gboolean is_root, is_total, is_monitoring_forced;
ui_bud_tabview_cat_type_t category_type;

	view = data->TV_budget;
	selection = data->TV_selection;

	// Block signals
	g_signal_handler_block(data->BT_category_force_monitoring, data->HID_category_monitoring_toggle);

	// Reset buttons
	#if HB_BUD_TABVIEW_EDIT_ENABLE
	gtk_widget_set_sensitive(data->BT_category_add, FALSE);
	gtk_widget_set_sensitive(data->BT_category_delete, FALSE);
	gtk_widget_set_sensitive(data->BT_category_merge, FALSE);
	#endif
	gtk_widget_set_sensitive(data->BT_category_reset, FALSE);
	gtk_widget_set_sensitive(data->BT_category_force_monitoring, FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->BT_category_force_monitoring), FALSE);

	// Read filter to retrieve the currently selected row in real model
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));

	// Activate buttons if selected row is editable
	if (gtk_tree_selection_get_selected(selection, &filter, &filter_iter))
	{
		// Convert data to budget model
		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
			&iter,
			&filter_iter);

		// Check the iter is an editable one
		gtk_tree_model_get (budget, &iter,
			UI_BUD_TABVIEW_CATEGORY_TYPE, &category_type,
			UI_BUD_TABVIEW_IS_ROOT, &is_root,
			UI_BUD_TABVIEW_IS_TOTAL, &is_total,
			UI_BUD_TABVIEW_IS_MONITORING_FORCED, &is_monitoring_forced,
			-1);

		// If category is neither a root, neither a total row, every operations can be applied
		if (!is_root && !is_total)
		{
			#if HB_BUD_TABVIEW_EDIT_ENABLE
			gtk_widget_set_sensitive(data->BT_category_add, TRUE);
			gtk_widget_set_sensitive(data->BT_category_delete, TRUE);
			gtk_widget_set_sensitive(data->BT_category_merge, TRUE);
			#endif
			gtk_widget_set_sensitive(data->BT_category_reset, TRUE);
			gtk_widget_set_sensitive(data->BT_category_force_monitoring, TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->BT_category_force_monitoring), is_monitoring_forced);
		}
		// If category is a root (except the Total one), we can use it to add a child row
		#if HB_BUD_TABVIEW_EDIT_ENABLE
		else if (category_type != UI_BUD_TABVIEW_CAT_TYPE_NONE
			&& is_root
			&& !is_total)
		{
			gtk_widget_set_sensitive(data->BT_category_add, TRUE);
		}
		#endif
	}

	// Unblock signals
	g_signal_handler_unblock(data->BT_category_force_monitoring, data->HID_category_monitoring_toggle);

	return;
}


static void ui_bud_tabview_dialog_close(ui_bud_tabview_data_t *data, gint response)
{
	DB( g_print("[ui_bud_tabview] dialog close\n") );

	GLOBALS->changes_count += data->change;

	return;
}

// Open / create the main dialog, the budget view and the budget model
GtkWidget *ui_bud_tabview_manage_dialog(void)
{
ui_bud_tabview_data_t *data;
struct WinGeometry *wg;
GtkWidget *dialog, *content, *grid;
GtkWidget *radiomode;
GtkWidget *widget;
GtkWidget *vbox, *hbox, *bbox;
GtkWidget *search_entry;
GtkWidget *scrollwin, *treeview;
GtkWidget *tbar;
//GtkWidget *menu, *menuitem, *image;
GtkTreeModel *model, *filter;
gint response;
gint w, h, dw, dh;
gint gridrow;

	data = g_malloc0(sizeof(ui_bud_tabview_data_t));
	data->change = 0;
	if(!data) return NULL;

	DB( g_print("\n[ui_bud_tabview] open dialog\n") );

	// create window
	dialog = gtk_dialog_new_with_buttons (_("Manage Budget"),
		GTK_WINDOW(GLOBALS->mainwindow),
		GTK_DIALOG_MODAL,
		_("_Close"),
		GTK_RESPONSE_ACCEPT,
		NULL);

	data->dialog = dialog;

	//#2007714 keep dimension
	wg = &PREFS->dbud_wg;
	if( wg->w == 0 && wg->h == 0 )
	{

		//set a nice dialog size
		gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
		dh = (h*1.33/PHI);
		//ratio 3:2
		dw = (dh * 3) / 2;
		DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
		gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);
	}
	else
		gtk_window_set_default_size(GTK_WINDOW(dialog), wg->w, wg->h);


	// store data inside dialog property to retrieve them easily in callbacks
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)&data);
	DB( g_print(" - new dialog=%p, inst_data=%p\n", dialog, data) );

	//window contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog)); // return a vbox

	// design content
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	hb_widget_set_margin(GTK_WIDGET(grid), SPACING_MEDIUM);
	gtk_box_pack_start (GTK_BOX (content), grid, TRUE, TRUE, 0);

	// First row displays radio button to change mode (edition / view) and search entry
	gridrow = 0;

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (grid), hbox, 0, gridrow, 1, 1);

	// edition mode radio buttons
	radiomode = hbtk_radio_button_new(GTK_ORIENTATION_HORIZONTAL, UI_BUD_TABVIEW_VIEW_MODE, TRUE);
	data->RA_mode = radiomode;
	gtk_box_set_center_widget(GTK_BOX (hbox), radiomode);

	// Search
	search_entry = make_search();
	data->EN_search = search_entry;
	gtk_box_pack_end (GTK_BOX (hbox), search_entry, FALSE, FALSE, 0);

	// Next row displays the budget tree with its toolbar
	gridrow++;

	// We use a Vertical Box to link tree with searchbar and toolbar
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_grid_attach (GTK_GRID (grid), vbox, 0, gridrow, 1, 1);

	// Scrolled Window will permit to display budgets with a lot of active categories
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);

	treeview = ui_bud_tabview_view_new ((gpointer) data);
	data->TV_budget = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	data->TV_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	// Toolbar to add, remove categories, expand and collapse categorie
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

#if HB_BUD_TABVIEW_EDIT_ENABLE
	// Add / Remove / Merge
	widget = make_image_button(ICONNAME_LIST_ADD, _("Add category"));
	data->BT_category_add = widget;
	gtk_widget_set_sensitive(widget, FALSE);
	gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	widget = make_image_button(ICONNAME_LIST_DELETE, _("Remove category"));
	data->BT_category_delete = widget;
	gtk_widget_set_sensitive(widget, FALSE);
	gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_with_label (_("Merge"));
	data->BT_category_merge = widget;
	gtk_widget_set_sensitive(widget, FALSE);
	gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

#endif

	// Clear Input
	widget = gtk_button_new_with_label (_("Clear input"));
	data->BT_category_reset = widget;
	gtk_widget_set_sensitive(widget, FALSE);
	gtk_box_pack_start (GTK_BOX (tbar), widget, FALSE, FALSE, 0);

	// Force monitoring
	widget = gtk_check_button_new_with_mnemonic (_("_Force monitoring this category"));
	data->BT_category_force_monitoring = widget;
	gtk_widget_set_sensitive (widget, FALSE);
	gtk_box_pack_start (GTK_BOX (tbar), widget, FALSE, FALSE, 0);
	g_object_set(widget,
		"draw-indicator", TRUE,
		NULL);

	// Expand / Collapse
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

	widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
	data->BT_expand = widget;
	gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
	data->BT_collapse = widget;
	gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	/* signal connect */

	// connect every radio button to the toggled signal to correctly update the view
	for (int i=0; UI_BUD_TABVIEW_VIEW_MODE[i] != NULL; i++)
	{
		widget = hbtk_radio_button_get_nth (GTK_CONTAINER(radiomode), i);

		if (widget)
		{
			g_signal_connect (widget, "toggled", G_CALLBACK (ui_bud_tabview_view_update_mode), (gpointer)data);
		}
	}

	// Connect to key press to handle some events like Control-f
	gtk_tree_view_set_search_entry(GTK_TREE_VIEW(treeview), GTK_ENTRY(search_entry));
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_bud_tabview_on_key_press), (gpointer)data);


	// Tree View
	g_signal_connect (data->TV_selection, "changed", G_CALLBACK(ui_bud_tabview_view_on_select), (gpointer)data);

	
	// toolbar buttons
#if HB_BUD_TABVIEW_EDIT_ENABLE
	g_signal_connect (data->BT_category_add, "clicked", G_CALLBACK(ui_bud_tabview_category_add), (gpointer)data);
	g_signal_connect (data->BT_category_delete, "clicked", G_CALLBACK (ui_bud_tabview_category_delete), (gpointer)data);
	g_signal_connect (data->BT_category_merge, "clicked", G_CALLBACK (ui_bud_tabview_category_merge), (gpointer)data);
#endif
	g_signal_connect (data->BT_category_reset, "clicked", G_CALLBACK (ui_bud_tabview_category_reset), (gpointer)data);
	data->HID_category_monitoring_toggle = g_signal_connect (data->BT_category_force_monitoring, "toggled", G_CALLBACK (ui_bud_tabview_category_toggle_monitoring), (gpointer)data);
	g_signal_connect (data->BT_expand, "clicked", G_CALLBACK (ui_bud_tabview_view_expand), (gpointer)data);
	g_signal_connect (data->BT_collapse, "clicked", G_CALLBACK (ui_bud_tabview_view_collapse), (gpointer)data);

	// dialog
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);

	// tree model to map HomeBank categories to the tree view
	model = ui_bud_tabview_model_new();

	filter = gtk_tree_model_filter_new(model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), ui_bud_tabview_model_row_filter, data, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), filter);

	g_object_unref(model); // Remove model with filter
	g_object_unref(filter); // Remove filter with view

	// By default, show the balance mode with all categories expanded
	data->TV_is_expanded = TRUE;
	ui_bud_tabview_view_toggle((gpointer) data, UI_BUD_TABVIEW_VIEW_SUMMARY);

	gtk_widget_show_all (dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	//#2007714 keep dimension
	wg = &PREFS->dbud_wg;
	gtk_window_get_size(GTK_WINDOW(dialog), &wg->w, &wg->h);

	ui_bud_tabview_dialog_close(data, response);
	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);

	return NULL;
}

