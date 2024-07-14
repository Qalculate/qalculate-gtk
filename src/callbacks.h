/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <gtk/gtk.h>

#include "main.h"

enum {
	PROGRAMMING_KEYPAD = 1,
	HIDE_LEFT_KEYPAD = 2,
	HIDE_RIGHT_KEYPAD = 4
};

enum {
	QALCULATE_HISTORY_EXPRESSION,
	QALCULATE_HISTORY_TRANSFORMATION,
	QALCULATE_HISTORY_RESULT,
	QALCULATE_HISTORY_RESULT_APPROXIMATE,
	QALCULATE_HISTORY_PARSE_WITHEQUALS,
	QALCULATE_HISTORY_PARSE,
	QALCULATE_HISTORY_PARSE_APPROXIMATE,
	QALCULATE_HISTORY_WARNING,
	QALCULATE_HISTORY_ERROR,
	QALCULATE_HISTORY_OLD,
	QALCULATE_HISTORY_REGISTER_MOVED,
	QALCULATE_HISTORY_RPN_OPERATION,
	QALCULATE_HISTORY_BOOKMARK,
	QALCULATE_HISTORY_MESSAGE
};

DECLARE_BUILTIN_FUNCTION(AnswerFunction, 0)
DECLARE_BUILTIN_FUNCTION(ExpressionFunction, 0)
DECLARE_BUILTIN_FUNCTION(SetTitleFunction, 0)

#define RUNTIME_CHECK_GTK_VERSION(x, y) (gtk_get_minor_version() >= y)
#define RUNTIME_CHECK_GTK_VERSION_LESS(x, y) (gtk_get_minor_version() < y)

class ViewThread : public Thread {
protected:
	virtual void run();
};

class CommandThread : public Thread {
protected:
	virtual void run();
};

class FetchExchangeRatesThread : public Thread {
protected:
	virtual void run();
};

void set_unicode_buttons();
void set_operator_symbols();

bool update_window_title(const char *str = NULL, bool is_result = false);

cairo_surface_t *draw_structure(MathStructure &m, PrintOptions po = default_print_options, bool caf = false, InternalPrintStruct ips = top_ips, gint *point_central = NULL, int scaledown = 0, GdkRGBA *color = NULL, gint *x_offset = NULL, gint *w_offset = NULL, gint max_width = -1, bool for_result_widget = true, MathStructure *where_struct = NULL, std::vector<MathStructure> *to_structs = NULL);

void update_status_text();

void clearresult();

void set_result_size_request();
void set_status_size_request();
void set_expression_size_request();
void test_supsub();
bool test_supsub(GtkWidget *w);

void create_umenu(void);
void create_umenu2(void);
void create_vmenu(void);
void create_fmenu(void);
void create_pmenu(GtkWidget *item);
void create_pmenu2(void);

void add_custom_angles_to_menus();

void update_completion();

void remove_old_my_variables_category();

void generate_functions_tree_struct();
void generate_variables_tree_struct();
void generate_units_tree_struct();

gboolean on_display_errors_timeout(gpointer data);
gboolean on_check_version_idle(gpointer data);

void update_unit_selector_tree();
void on_tUnitSelector_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_tUnitSelectorCategories_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);

void setResult(Prefix *prefix = NULL, bool update_history = true, bool update_parse = false, bool force = false, std::string transformation = "", size_t stack_index = 0, bool register_moved = false, bool supress_dialog = false);
void execute_from_file(std::string file_name);

void set_rpn_mode(bool b);
void calculateRPN(int op);
void calculateRPN(MathFunction *f);
void RPNRegisterAdded(std::string text, gint index = 0);
void RPNRegisterRemoved(gint index);
void RPNRegisterChanged(std::string text, gint index);

void recreate_recent_functions();
void recreate_recent_variables();
void recreate_recent_units();
void function_inserted(MathFunction *object);
void variable_inserted(Variable *object);
void unit_inserted(Unit *object);

void on_tPlotFunctions_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);

void on_tNames_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);

void convert_to_unit(GtkMenuItem *w, gpointer user_data);

bool save_defs(bool allow_cancel = false);
void save_mode();

void load_preferences();
bool save_preferences(bool mode = false, bool allow_cancel = false);
bool save_history(bool allow_cancel = false);
void edit_preferences();

gint completion_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

void set_prefix(GtkMenuItem *w, gpointer user_data);

void set_clean_mode(GtkMenuItem *w, gpointer user_data);
void set_functions_enabled(GtkMenuItem *w, gpointer user_data);
void set_variables_enabled(GtkMenuItem *w, gpointer user_data);
void set_donot_calcvars(GtkMenuItem *w, gpointer user_data);
void set_unknownvariables_enabled(GtkMenuItem *w, gpointer user_data);
void set_units_enabled(GtkMenuItem *w, gpointer user_data);
void apply_function(GtkMenuItem *w, gpointer user_data);
void insert_menu_function(GtkMenuItem *w, gpointer user_data);
void insert_variable(GtkMenuItem *w, gpointer user_data);
void insert_prefix(GtkMenuItem *w, gpointer user_data);
void insert_unit(GtkMenuItem *w, gpointer user_data);

void insert_button_function(GtkMenuItem *w, gpointer user_data);
void insert_function_operator(GtkMenuItem *w, gpointer user_data);
void insert_button_function_norpn(GtkMenuItem *w, gpointer user_data);
void insert_button_variable(GtkWidget *w, gpointer user_data);
void insert_button_unit(GtkMenuItem *w, gpointer user_data);
void insert_button_currency(GtkMenuItem *w, gpointer user_data);

void new_function(GtkMenuItem *w, gpointer user_data);
void new_unknown(GtkMenuItem *w, gpointer user_data);
void new_variable(GtkMenuItem *w, gpointer user_data);
void new_matrix(GtkMenuItem *w, gpointer user_data);
void new_vector(GtkMenuItem *w, gpointer user_data);
void new_unit(GtkMenuItem *w, gpointer user_data);
void add_as_variable();

void fetch_exchange_rates(int timeout, int n = -1);

void insert_matrix(const MathStructure *initial_value = NULL, GtkWidget *win = NULL, gboolean create_vector = FALSE, bool is_text_struct = false, bool is_result = false, GtkEntry *entry = NULL);

gchar *font_name_to_css(const char *font_name, const char *w = "*");

void reload_history(gint from_index = -1);
void set_status_bottom_border_visible(bool);

#ifdef __cplusplus
extern "C" {
#endif

void memory_recall();
void memory_store();
void memory_add();
void memory_subtract();
void memory_clear();

void hide_tooltip(GtkWidget*);

void insert_left_shift();
void insert_right_shift();
void insert_bitwise_and();
void insert_bitwise_or();
void insert_bitwise_xor();
void insert_bitwise_not();
void insert_angle_symbol();

void update_mb_fx_menu();
void update_mb_sto_menu();
void update_mb_units_menu();
void update_mb_pi_menu();
void update_mb_to_menu();
void update_mb_angles(AngleUnit angle_unit);

void on_completion_match_selected(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn*, gpointer);

void *view_proc(void*);
void *command_proc(void*);
void on_history_resize(GtkWidget*, GdkRectangle*, gpointer);
void on_message_bar_response(GtkInfoBar *w, gint response_id, gpointer);
void on_expressiontext_populate_popup(GtkTextView *w, GtkMenu *menu, gpointer user_data);
void on_combobox_base_changed(GtkComboBox *w, gpointer user_data);
void on_combobox_numerical_display_changed(GtkComboBox *w, gpointer user_data);
void on_button_fraction_toggled(GtkToggleButton *w, gpointer user_data);
void on_button_exact_toggled(GtkToggleButton *w, gpointer user_data);
void on_expander_keypad_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_history_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_stack_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_convert_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
gboolean on_menu_item_meta_mode_popup_menu(GtkWidget*, gpointer data);
gboolean on_menu_item_meta_mode_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
void on_menu_item_meta_mode_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_meta_mode_save_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_meta_mode_delete_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_quit_activate(GtkMenuItem *w, gpointer user_data);
void on_colorbutton_status_error_color_color_set(GtkColorButton *w, gpointer user_data);
void on_colorbutton_status_warning_color_color_set(GtkColorButton *w, gpointer user_data);
void on_preferences_checkbutton_autocalc_history_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_parsed_in_result_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_combobox_bits_changed(GtkComboBox *w, gpointer user_data);
void on_preferences_checkbutton_twos_complement_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_hexadecimal_twos_complement_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_twos_complement_input_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_hexadecimal_twos_complement_input_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_persistent_keypad_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_copy_ascii_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_lower_case_numbers_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_e_notation_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_lower_case_e_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_alternative_base_prefixes_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_spell_out_logical_operators_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_unicode_signs_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_display_expression_status_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_fetch_exchange_rates_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_save_defs_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_save_mode_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_rpn_keys_only_toggled(GtkToggleButton *w, gpointer);
void on_preferences_checkbutton_dot_as_separator_toggled(GtkToggleButton *w, gpointer);
void on_preferences_checkbutton_load_defs_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_custom_result_font_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_custom_expression_font_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_checkbutton_custom_status_font_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_dot_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_ex_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_asterisk_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_slash_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_division_slash_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_division_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_digit_grouping_none_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_digit_grouping_standard_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_radiobutton_digit_grouping_locale_toggled(GtkToggleButton *w, gpointer user_data);
void on_preferences_button_result_font_toggled(GtkButton *w, gpointer user_data);
void on_preferences_button_expression_font_toggled(GtkButton *w, gpointer user_data);
void on_preferences_button_status_font_toggled(GtkButton *w, gpointer user_data);
void on_preferences_checkbutton_decimal_comma_toggled(GtkToggleButton *w, gpointer);
void on_preferences_checkbutton_dot_as_separator_toggled(GtkToggleButton *w, gpointer);
void on_preferences_checkbutton_comma_as_separator_toggled(GtkToggleButton *w, gpointer);
void on_preferences_radiobutton_temp_rel_toggled(GtkToggleButton *w, gpointer);
void on_preferences_radiobutton_temp_abs_toggled(GtkToggleButton *w, gpointer);
void on_preferences_radiobutton_temp_hybrid_toggled(GtkToggleButton *w, gpointer);
void on_radiobutton_radians_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_radiobutton_degrees_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_radiobutton_gradians_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_radiobutton_no_default_angle_unit_toggled(GtkToggleButton *togglebutton, gpointer user_data);
gboolean on_gcalc_exit(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_button_execute_clicked(GtkButton *button, gpointer user_data);
void on_button_del_clicked(GtkButton *w, gpointer user_data);
void on_button_ac_clicked(GtkButton *w, gpointer user_data);
void on_button_hyp_toggled(GtkToggleButton *w, gpointer user_data);
void on_button_inv_toggled(GtkToggleButton *w, gpointer user_data);
void on_button_tan_clicked(GtkButton *w, gpointer user_data);
void on_button_sine_clicked(GtkButton *w, gpointer user_data);
void on_button_cosine_clicked(GtkButton *w, gpointer user_data);
void on_button_store_clicked(GtkButton *w, gpointer user_data);
void on_button_mod_clicked(GtkButton *w, gpointer user_data);
void on_button_reciprocal_clicked(GtkButton *w, gpointer user_data);
void on_togglebutton_expression_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_togglebutton_result_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_expression_changed(GtkEditable *w, gpointer user_data);
void on_expression_move_cursor(GtkEntry*, GtkMovementStep, gint, gboolean, gpointer);
void on_button_zero_clicked(GtkButton *w, gpointer user_data);
void on_button_one_clicked(GtkButton *w, gpointer user_data);
void on_button_two_clicked(GtkButton *w, gpointer user_data);
void on_button_three_clicked(GtkButton *w, gpointer user_data);
void on_button_four_clicked(GtkButton *w, gpointer user_data);
void on_button_five_clicked(GtkButton *w, gpointer user_data);
void on_button_six_clicked(GtkButton *w, gpointer user_data);
void on_button_seven_clicked(GtkButton *w, gpointer user_data);
void on_button_eight_clicked(GtkButton *w, gpointer user_data);
void on_button_nine_clicked(GtkButton *w, gpointer user_data);
void on_button_dot_clicked(GtkButton *w, gpointer user_data);
void on_button_brace_open_clicked(GtkButton *w, gpointer user_data);
void on_button_brace_close_clicked(GtkButton *w, gpointer user_data);
void on_button_times_clicked(GtkButton *w, gpointer user_data);
void on_button_add_clicked(GtkButton *w, gpointer user_data);
void on_button_sub_clicked(GtkButton *w, gpointer user_data);
void on_button_divide_clicked(GtkButton *w, gpointer user_data);
void on_button_ans_clicked(GtkButton *w, gpointer user_data);
void on_button_exp_clicked(GtkButton *w, gpointer user_data);
void on_button_xy_clicked(GtkButton *w, gpointer user_data);
void on_button_square_clicked();
void on_button_sqrt_clicked(GtkButton *w, gpointer user_data);
void on_button_log_clicked(GtkButton *w, gpointer user_data);
void on_button_ln_clicked(GtkButton *w, gpointer user_data);
void on_button_twos_in_toggled(GtkToggleButton *w, gpointer);
void on_menu_item_manage_variables_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_manage_functions_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_manage_units_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_datasets_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_import_csv_file_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_export_csv_file_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_convert_to_unit_expression_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_convert_to_base_units_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_convert_to_best_unit_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_set_prefix_activate(GtkMenuItem*, gpointer user_data);
void on_menu_item_insert_date_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_insert_matrix_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_insert_vector_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_enable_variables_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_enable_functions_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_enable_units_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_enable_unknown_variables_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_calculate_variables_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_enable_variable_units_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_allow_complex_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_allow_infinite_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_unknown_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_variable_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_matrix_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_vector_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_function_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_dataset_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_new_unit_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_autocalc_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_chain_mode_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_rpn_mode_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_rpn_syntax_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_limit_implicit_multiplication_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_adaptive_parsing_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_ignore_whitespace_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_fetch_exchange_rates_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_save_defs_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_save_mode_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_edit_prefs_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_degrees_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_radians_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_gradians_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_custom_angle_unit_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_no_default_angle_unit_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_read_precision_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_rpn_syntax_activate(GtkMenuItem *w, gpointer user_data);
void output_base_updated_from_menu();
void input_base_updated_from_menu();
void update_menu_base();
void on_menu_item_binary_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_octal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_decimal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_duodecimal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_custom_base_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_roman_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_sexagesimal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_time_format_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_set_base_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_convert_number_bases_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_periodic_table_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_integer_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_rational_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_real_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_complex_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_number_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_nonmatrix_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_none_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_nonzero_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_positive_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_nonnegative_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_negative_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_nonpositive_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assumptions_unknown_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_expression_status_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_parsed_in_result_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_exact_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_abort_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_clear_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_clear_history_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_history_clear_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_normal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_no_prefixes_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_prefixes_for_selected_units_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_prefixes_for_currencies_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_display_prefixes_for_all_units_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_mixed_units_conversion_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_fraction_decimal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_fraction_decimal_exact_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_fraction_combined_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_fraction_fraction_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_binary_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_roman_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_sexagesimal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_time_format_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_octal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_decimal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_duodecimal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_custom_base_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_abbreviate_names_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_all_prefixes_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_denominator_prefixes_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_view_matrix_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_view_vector_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_complex_rectangular_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_complex_exponential_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_complex_polar_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_complex_angle_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_persistent_keypad_toggled(GtkCheckMenuItem *w, gpointer user_data);
void on_menu_item_complex_rectangular_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_complex_exponential_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_complex_polar_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_complex_angle_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_normal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_no_prefixes_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_prefixes_for_selected_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_prefixes_for_currencies_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_display_prefixes_for_all_units_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_indicate_infinite_series_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_show_ending_zeroes_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_negative_exponents_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_sort_minus_last_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_fraction_decimal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_fraction_decimal_exact_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_fraction_combined_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_fraction_fraction_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_interval_adaptive_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_interval_significant_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_interval_interval_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_interval_plusminus_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_interval_midpoint_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_all_prefixes_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_denominator_prefixes_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_place_units_separately_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_post_conversion_none_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_post_conversion_base_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_post_conversion_optimal_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_post_conversion_optimal_si_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_abbreviate_names_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_always_exact_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_try_exact_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_approximate_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_interval_arithmetic_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_ic_none(GtkMenuItem *w, gpointer user_data);
void on_menu_item_ic_variance_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_ic_interval_arithmetic_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_ic_simple(GtkMenuItem *w, gpointer user_data);
void on_menu_item_save_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_save_image_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_copy_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_copy_ascii_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_precision_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_decimals_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_plot_functions_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_factorize_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_simplify_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_expand_partial_fractions_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_set_unknowns_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_algebraic_mode_simplify_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_algebraic_mode_factorize_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_algebraic_mode_hybrid_activate(GtkMenuItem *w, gpointer user_data);
gboolean on_main_window_focus_in_event(GtkWidget *w, GdkEventFocus *e, gpointer user_data);

void on_button_registerup_clicked(GtkButton *button, gpointer user_data);
void on_button_registerdown_clicked(GtkButton *button, gpointer user_data);
void on_button_registerswap_clicked(GtkButton *button, gpointer user_data);
void on_button_editregister_clicked(GtkButton *button, gpointer user_data);
void on_button_deleteregister_clicked(GtkButton *button, gpointer user_data);
void on_button_copyregister_clicked(GtkButton *button, gpointer user_data);
void on_button_clearstack_clicked(GtkButton *button, gpointer user_data);
void on_stackview_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_stackview_item_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data);
void on_stackview_item_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data);
void on_stackview_item_editing_canceled(GtkCellRenderer *renderer, gpointer user_data);
void on_stackstore_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data);
void on_stackstore_row_deleted(GtkTreeModel *model, GtkTreePath *path, gpointer user_data);

void on_historyview_selection_changed(GtkTreeSelection *select, gpointer);
void on_historyview_item_edited(GtkCellRendererText*, gchar*, gchar*, gpointer);
void on_historyview_item_editing_started(GtkCellRenderer*, GtkCellEditable*, gchar*, gpointer);
void on_historyview_item_editing_canceled(GtkCellRenderer*, gpointer);

void on_menu_item_show_percentage_dialog_activate(GtkMenuItem *w, gpointer user_data);

void on_menu_item_show_calendarconversion_dialog_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_calendarconversion_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_to_utc_activate(GtkMenuItem *w, gpointer user_data);

void on_button_functions_clicked(GtkButton *button, gpointer user_data);
void on_button_variables_clicked(GtkButton *button, gpointer user_data);
void on_button_units_clicked(GtkButton *button, gpointer user_data);
void on_button_convert_clicked(GtkButton *button, gpointer user_data);

void on_menu_item_about_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_help_activate(GtkMenuItem *w, gpointer user_data);

void on_precision_dialog_spinbutton_precision_value_changed(GtkSpinButton *w, gpointer user_data);
void on_precision_dialog_button_recalculate_clicked(GtkButton *w, gpointer user_data);
void on_decimals_dialog_spinbutton_max_value_changed(GtkSpinButton *w, gpointer user_data);
void on_decimals_dialog_spinbutton_min_value_changed(GtkSpinButton *w, gpointer user_data);
void on_decimals_dialog_checkbutton_max_toggled(GtkToggleButton *w, gpointer user_data);
void on_decimals_dialog_checkbutton_min_toggled(GtkToggleButton *w, gpointer user_data);

gboolean on_expressiontext_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer user_data);

gboolean on_resultview_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer user_data);
gboolean on_resultview_popup_menu(GtkWidget *w, gpointer user_data);

gboolean on_expressiontext_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer user_data);

void on_expressionbuffer_cursor_position_notify();

gboolean on_resultview_draw(GtkWidget *w, cairo_t *cr, gpointer user_data);

gboolean on_tMatrix_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer user_data);
gboolean on_tMatrix_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer user_data);
gboolean on_tMatrix_cursor_changed(GtkTreeView *w, gpointer user_data);

void on_matrix_spinbutton_columns_value_changed(GtkSpinButton *w, gpointer user_data);
void on_matrix_spinbutton_rows_value_changed(GtkSpinButton *w, gpointer user_data);

void on_matrix_radiobutton_matrix_toggled(GtkToggleButton *w, gpointer user_data);
void on_matrix_radiobutton_vector_toggled(GtkToggleButton *w, gpointer user_data);

void on_type_label_date_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_file_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_vector_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_matrix_clicked(GtkEntry *w, gpointer user_data);

void on_plot_button_save_clicked(GtkButton *w, gpointer user_data);
void on_plot_button_add_clicked(GtkButton *w, gpointer user_data);
void on_plot_button_modify_clicked(GtkButton *w, gpointer user_data);
void on_plot_button_remove_clicked(GtkButton *w, gpointer user_data);
void on_plot_checkbutton_xlog_toggled(GtkToggleButton *w, gpointer user_data);
void on_plot_checkbutton_ylog_toggled(GtkToggleButton *w, gpointer user_data);
void on_plot_radiobutton_step_toggled(GtkToggleButton *w, gpointer user_data);
void on_plot_radiobutton_steps_toggled(GtkToggleButton *w, gpointer user_data);
void on_plot_entry_expression_activate(GtkEntry *entry, gpointer user_data);

void on_unit_dialog_button_apply_clicked(GtkButton *w, gpointer user_data);
void on_unit_dialog_button_ok_clicked(GtkButton *w, gpointer user_data);
void on_unit_dialog_entry_unit_activate(GtkEntry *entry, gpointer user_data);

void convert_from_convert_entry_unit();
void on_convert_button_convert_clicked(GtkButton *w, gpointer user_data);
void on_convert_entry_unit_activate(GtkEntry *entry, gpointer user_data);
void on_convert_entry_search_changed(GtkEntry *w, gpointer user_data);

gboolean on_menu_key_press(GtkWidget *widget, GdkEventKey *event);

#ifdef __cplusplus
}
#endif

#endif

