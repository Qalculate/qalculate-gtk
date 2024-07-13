/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef QALCULATE_GTK_UTIL_H
#define QALCULATE_GTK_UTIL_H

#include <libqalculate/qalculate.h>
#include <gtk/gtk.h>

enum {
	SHORTCUT_TYPE_FUNCTION,
	SHORTCUT_TYPE_FUNCTION_WITH_DIALOG,
	SHORTCUT_TYPE_VARIABLE,
	SHORTCUT_TYPE_UNIT,
	SHORTCUT_TYPE_TEXT,
	SHORTCUT_TYPE_DATE,
	SHORTCUT_TYPE_VECTOR,
	SHORTCUT_TYPE_MATRIX,
	SHORTCUT_TYPE_SMART_PARENTHESES,
	SHORTCUT_TYPE_CONVERT,
	SHORTCUT_TYPE_CONVERT_ENTRY,
	SHORTCUT_TYPE_OPTIMAL_UNIT,
	SHORTCUT_TYPE_BASE_UNITS,
	SHORTCUT_TYPE_OPTIMAL_PREFIX,
	SHORTCUT_TYPE_TO_NUMBER_BASE,
	SHORTCUT_TYPE_FACTORIZE,
	SHORTCUT_TYPE_EXPAND,
	SHORTCUT_TYPE_PARTIAL_FRACTIONS,
	SHORTCUT_TYPE_SET_UNKNOWNS,
	SHORTCUT_TYPE_RPN_UP,
	SHORTCUT_TYPE_RPN_DOWN,
	SHORTCUT_TYPE_RPN_SWAP,
	SHORTCUT_TYPE_RPN_COPY,
	SHORTCUT_TYPE_RPN_LASTX,
	SHORTCUT_TYPE_RPN_DELETE,
	SHORTCUT_TYPE_RPN_CLEAR,
	SHORTCUT_TYPE_META_MODE,
	SHORTCUT_TYPE_OUTPUT_BASE,
	SHORTCUT_TYPE_INPUT_BASE,
	SHORTCUT_TYPE_EXACT_MODE,
	SHORTCUT_TYPE_DEGREES,
	SHORTCUT_TYPE_RADIANS,
	SHORTCUT_TYPE_GRADIANS,
	SHORTCUT_TYPE_FRACTIONS,
	SHORTCUT_TYPE_MIXED_FRACTIONS,
	SHORTCUT_TYPE_SCIENTIFIC_NOTATION,
	SHORTCUT_TYPE_SIMPLE_NOTATION,
	SHORTCUT_TYPE_RPN_MODE,
	SHORTCUT_TYPE_AUTOCALC,
	SHORTCUT_TYPE_PROGRAMMING,
	SHORTCUT_TYPE_KEYPAD,
	SHORTCUT_TYPE_HISTORY,
	SHORTCUT_TYPE_HISTORY_SEARCH,
	SHORTCUT_TYPE_CONVERSION,
	SHORTCUT_TYPE_STACK,
	SHORTCUT_TYPE_MINIMAL,
	SHORTCUT_TYPE_MANAGE_VARIABLES,
	SHORTCUT_TYPE_MANAGE_FUNCTIONS,
	SHORTCUT_TYPE_MANAGE_UNITS,
	SHORTCUT_TYPE_MANAGE_DATA_SETS,
	SHORTCUT_TYPE_STORE,
	SHORTCUT_TYPE_MEMORY_CLEAR,
	SHORTCUT_TYPE_MEMORY_RECALL,
	SHORTCUT_TYPE_MEMORY_STORE,
	SHORTCUT_TYPE_MEMORY_ADD,
	SHORTCUT_TYPE_MEMORY_SUBTRACT,
	SHORTCUT_TYPE_NEW_VARIABLE,
	SHORTCUT_TYPE_NEW_FUNCTION,
	SHORTCUT_TYPE_PLOT,
	SHORTCUT_TYPE_NUMBER_BASES,
	SHORTCUT_TYPE_FLOATING_POINT,
	SHORTCUT_TYPE_CALENDARS,
	SHORTCUT_TYPE_PERCENTAGE_TOOL,
	SHORTCUT_TYPE_PERIODIC_TABLE,
	SHORTCUT_TYPE_UPDATE_EXRATES,
	SHORTCUT_TYPE_COPY_RESULT,
	SHORTCUT_TYPE_SAVE_IMAGE,
	SHORTCUT_TYPE_HELP,
	SHORTCUT_TYPE_QUIT,
	SHORTCUT_TYPE_CHAIN_MODE,
	SHORTCUT_TYPE_ALWAYS_ON_TOP,
	SHORTCUT_TYPE_DO_COMPLETION,
	SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION,
	SHORTCUT_TYPE_INSERT_RESULT,
	SHORTCUT_TYPE_HISTORY_CLEAR,
	SHORTCUT_TYPE_PRECISION,
	SHORTCUT_TYPE_MIN_DECIMALS,
	SHORTCUT_TYPE_MAX_DECIMALS,
	SHORTCUT_TYPE_MINMAX_DECIMALS
};

#define LAST_SHORTCUT_TYPE SHORTCUT_TYPE_MINMAX_DECIMALS

bool string_is_less(std::string str1, std::string str2);
extern KnownVariable *v_memory;

struct tree_struct {
	std::string item;
	std::list<tree_struct> items;
	std::list<tree_struct>::iterator it;
	std::list<tree_struct>::reverse_iterator rit;
	std::vector<void*> objects;
	tree_struct *parent;
	void sort() {
		items.sort();
		for(std::list<tree_struct>::iterator it = items.begin(); it != items.end(); ++it) {
			it->sort();
		}
	}
	bool operator < (const tree_struct &s1) const {
		return string_is_less(item, s1.item);
	}
};

struct mode_struct {
	PrintOptions po;
	EvaluationOptions eo;
	AssumptionType at;
	AssumptionSign as;
	Number custom_output_base;
	Number custom_input_base;
	int precision;
	std::string name;
	bool rpn_mode;
	bool interval;
	bool adaptive_interval_display;
	bool variable_units_enabled;
	int keypad;
	bool autocalc;
	bool chain_mode;
	bool complex_angle_form;
	bool implicit_question_asked;
	int simplified_percentage;
	bool concise_uncertainty_input;
	long int fixed_denominator;
	std::string custom_angle_unit;
};

struct keyboard_shortcut {
	guint key;
	guint modifier;
	std::vector<int> type;
	std::vector<std::string> value;
};

struct custom_button {
	int type[3];
	std::string value[3], text;
	custom_button() {type[0] = -1; type[1] = -1; type[2] = -1;}
};

#define EXPAND_TO_ITER(model, view, iter)		GtkTreePath *path = gtk_tree_model_get_path(model, &iter); \
							gtk_tree_view_expand_to_path(GTK_TREE_VIEW(view), path); \
							gtk_tree_path_free(path);
#define SCROLL_TO_ITER(model, view, iter)		GtkTreePath *path2 = gtk_tree_model_get_path(model, &iter); \
							gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(view), path2, NULL, FALSE, 0, 0); \
							gtk_tree_path_free(path2);
#define EXPAND_ITER(model, view, iter)			GtkTreePath *path = gtk_tree_model_get_path(model, &iter); \
							gtk_tree_view_expand_row(GTK_TREE_VIEW(view), path, FALSE); \
							gtk_tree_path_free(path);

extern GtkWidget *mainwindow;

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
#	define CLEAN_MODIFIERS(x) (x & gdk_keymap_get_modifier_mask(gdk_keymap_get_for_display(gtk_widget_get_display(mainwindow)), GDK_MODIFIER_INTENT_DEFAULT_MOD_MASK))
#else
#	define CLEAN_MODIFIERS(x) (x & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_SUPER_MASK | GDK_HYPER_MASK | GDK_META_MASK))
#endif
#ifdef _WIN32
#	define FIX_ALT_GR if(state & GDK_MOD1_MASK && state & GDK_MOD2_MASK && state & GDK_CONTROL_MASK) state &= ~GDK_CONTROL_MASK;
#else
#	define FIX_ALT_GR
#endif

#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 20
#	define SET_FOCUS_ON_CLICK(x) gtk_button_set_focus_on_click(GTK_BUTTON(x), FALSE)
#else
#	define SET_FOCUS_ON_CLICK(x) gtk_widget_set_focus_on_click(GTK_WIDGET(x), FALSE)
#endif
#define CHILDREN_SET_FOCUS_ON_CLICK(x) list = gtk_container_get_children(GTK_CONTAINER(gtk_builder_get_object(main_builder, x))); \
	for(l = list; l != NULL; l = l->next) { \
		SET_FOCUS_ON_CLICK(l->data); \
	} \
	g_list_free(list);
#define CHILDREN_SET_FOCUS_ON_CLICK_2(x, y) list = gtk_container_get_children(GTK_CONTAINER(gtk_builder_get_object(main_builder, x))); \
	obj = gtk_builder_get_object(main_builder, y); \
	for(l = list; l != NULL; l = l->next) { \
		if(l->data != obj) SET_FOCUS_ON_CLICK(l->data); \
	} \
	g_list_free(list);

extern tree_struct function_cats, unit_cats, variable_cats;
extern std::string volume_cat;
extern std::vector<std::string> alt_volcats;
extern std::vector<void*> ia_units, ia_variables, ia_functions;
extern std::vector<Unit*> user_units;
extern std::vector<Variable*> user_variables;
extern std::vector<MathFunction*> user_functions;

extern int block_error_timeout;
extern bool b_busy, b_busy_command, b_busy_result, b_busy_expression, b_busy_fetch;
extern GtkWidget *expressiontext;

GtkBuilder *getBuilder(const char *filename);
void set_tooltips_enabled(GtkWidget *w, bool b);

bool last_is_operator(std::string str, bool allow_exp = false);

const char *sub_sign();
const char *times_sign(bool unit_expression = false);
const char *divide_sign();
const char *expression_add_sign();
const char *expression_sub_sign();
const char *expression_times_sign();
const char *expression_divide_sign();

gint string_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
gint category_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
gint int_string_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

bool can_display_unicode_string_function(const char *str, void *w);
bool can_display_unicode_string_function_exact(const char *str, void *w);

std::string localize_expression(std::string str, bool unit_expression = false);
std::string unlocalize_expression(std::string str);

void base_from_string(std::string str, int &base, Number &nbase, bool input_base = false);

bool entry_in_quotes(GtkEntry *w);
const gchar *key_press_get_symbol(GdkEventKey *event, bool do_caret_as_xor = true, bool unit_expression = false);
extern "C" {
gboolean on_math_entry_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer);
void entry_insert_text(GtkWidget *w, const gchar *text);
void update_keypad_bases();
void set_input_base(int base);
void set_output_base(int base);
}

void on_abort_display(GtkDialog*, gint, gpointer);
void on_abort_command(GtkDialog*, gint, gpointer);
void on_abort_calculation(GtkDialog*, gint, gpointer);

void show_message(const gchar *text, GtkWidget *win);
bool ask_question(const gchar *text, GtkWidget *win);
void show_help(const char *file, GtkWidget *win);
void insert_text(const gchar *text);
bool display_errors(int *history_index_p = NULL, GtkWidget *win = NULL, int *inhistory_index = NULL, int type = 0, bool *implicit_warning = NULL, time_t history_time = 0);

void result_display_updated();
void result_format_updated();
void result_action_executed();
void result_prefix_changed(Prefix *prefix = NULL);
void expression_calculation_updated();
void expression_format_updated(bool recalculate = false);
void execute_expression(bool force = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, std::string execute_str = std::string(), std::string str = std::string(), bool check_exrates = true);

void update_vmenu(bool update_compl = true);
void update_fmenu(bool update_compl = true);
void update_umenus(bool update_compl = true);
bool is_answer_variable(Variable *v);

void variable_removed(Variable *v);
void unit_removed(Unit *u);
void function_removed(MathFunction *f);

bool equalsIgnoreCase(const std::string &str1, const std::string &str2, size_t i2, size_t i2_end, size_t minlength);
bool title_matches(ExpressionItem *item, const std::string &str, size_t minlength = 0);
bool name_matches(ExpressionItem *item, const std::string &str);
bool country_matches(Unit *u, const std::string &str, size_t minlength = 0);

void import_csv_file(GtkWidget *win = NULL);
void export_csv_file(KnownVariable *v = NULL, GtkWidget *win = NULL);

void apply_function(MathFunction *f);
void insert_function(MathFunction *f, GtkWidget *parent = NULL, bool add_to_menu = true);

void edit_unknown(const char *category = "", Variable *v = NULL, GtkWidget *win = NULL);
void edit_variable(const char *category = "", Variable *v = NULL, MathStructure *mstruct_ = NULL, GtkWidget *win = NULL);
void edit_matrix(const char *category = "", Variable *v = NULL, MathStructure *mstruct_ = NULL, GtkWidget *win = NULL, gboolean create_vector = FALSE);
void edit_unit(const char *category = "", Unit *u = NULL, GtkWidget *win = NULL);
void edit_function(const char *category = "", MathFunction *f = NULL, GtkWidget *win = NULL, const char *name = NULL, const char *expression = NULL, bool enable_ok = true);
void edit_dataset(DataSet *ds = NULL, GtkWidget *win = NULL);

bool check_exchange_rates(GtkWidget *win = NULL, bool set_result = false);

void convert_result_to_unit(Unit *u);

void fix_deactivate_label_width(GtkWidget *w);

gboolean on_activate_link(GtkLabel*, gchar *uri, gpointer);

std::string shortcut_to_text(guint key, guint state);
const gchar *shortcut_type_text(int type, bool return_null = false);
std::string button_valuetype_text(int type, const std::string &value);
std::string shortcut_types_text(const std::vector<int> &type);
const char *shortcut_copy_value_text(int v);
std::string shortcut_values_text(const std::vector<std::string> &value, const std::vector<int> &type);
void update_accels(int type = -1);
void update_custom_buttons(int index = -1);

#endif /* QALCULATE_GTK_UTIL_H */
