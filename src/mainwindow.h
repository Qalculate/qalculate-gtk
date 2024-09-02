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
#include <libqalculate/qalculate.h>

struct mode_struct;

void set_system_tray_icon_enabled(bool b);
bool system_tray_icon_enabled();
bool has_systray_icon();
void test_border(void);
void restore_window(GtkWindow *win = NULL);

void create_main_window(void);

GtkWindow *main_window();

void update_accels(int type = -1);
void set_app_font(const char *str);
const char *app_font(bool return_default = false);
void keypad_font_modified();
void update_colors(bool initial = false);
void set_app_operator_symbols();
bool update_window_title(const char *str = NULL, bool is_result = false);
void set_custom_window_title(const char *str);

bool keypad_is_visible();
bool use_keypad_buttons_for_history();
void update_persistent_keypad(bool showhide_buttons = false);

void check_for_new_version(bool do_not_show_again = false);

void mainwindow_cursor_moved();

void show_notification(std::string text);

void hide_tooltip(GtkWidget*);

void set_minimal_mode(bool b);
void minimal_mode_show_resultview(bool b = true);

gboolean on_display_errors_timeout(gpointer data);
gboolean on_check_version_idle(gpointer data);

bool do_keyboard_shortcut(GdkEventKey *event);

void copy_result(int ascii = -1, int type = 0);

void set_clipboard(std::string str, int ascii, bool html, bool is_result, int copy_without_units = -1);

void result_format_updated();
void result_prefix_changed(Prefix *prefix = NULL);
void expression_calculation_updated();
void expression_format_updated(bool recalculate = false);
void update_message_print_options();

void set_expression_output_updated(bool);

void execute_expression(bool force = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, std::string execute_str = std::string(), std::string str = std::string(), bool check_exrates = true);
void executeCommand(int command_type, bool show_result = true, bool force = false, std::string ceu_str = "", Unit *u = NULL, int run = 1);

void abort_calculation();
bool calculator_busy();
void set_busy(bool b = true);

void execute_from_file(std::string file_name);

void calculateRPN(int op);
void calculateRPN(MathFunction *f);
void set_rpn_mode(bool b);

bool do_chain_mode(const gchar *op);

void toggle_binary_pos(int pos);

void convert_result_to_unit(Unit *u);
void convert_result_to_unit_expression(std::string str);

void setResult(Prefix *prefix = NULL, bool update_history = true, bool update_parse = false, bool force = false, std::string transformation = "", size_t stack_index = 0, bool register_moved = false, bool supress_dialog = false);

void clearresult();

void set_parsed_in_result(bool b);
void show_parsed(bool);
void show_parsed_in_result(MathStructure &mparse, const PrintOptions &po);
void clear_parsed_in_result();

void set_autocalculate(bool b);
void add_autocalculated_result_to_history();
bool autocalculation_stopped_at_operator();
void stop_autocalculate_history_timeout();
void autocalc_result_bases();

bool display_errors(GtkWindow *win = NULL, int type = 0, bool add_to_history = false);

void handle_expression_modified(bool autocalc);

MathStructure *current_result();
void replace_current_result(MathStructure*);
MathStructure *current_parsed_result();
const std::string &current_result_text();
bool current_result_text_is_approximate();

void memory_recall();
void memory_store();
void memory_add();
void memory_subtract();
void memory_clear();

void generate_functions_tree_struct();
void generate_variables_tree_struct();
void generate_units_tree_struct();

void definitions_loaded();
void initialize_variables_and_functions();

void update_vmenu(bool update_compl = true);
void update_fmenu(bool update_compl = true);
void update_umenus(bool update_compl = true);

void add_recent_items();

void function_inserted(MathFunction *object);
void variable_inserted(Variable *object);
void unit_inserted(Unit *object);

void variable_removed(Variable *v);
void unit_removed(Unit *u);
void function_removed(MathFunction *f);
void variable_edited(Variable *v);
void function_edited(MathFunction *f);
void unit_edited(Unit *u);
void dataset_edited(DataSet *ds);

bool is_answer_variable(Variable *v);
bool is_memory_variable(Variable *v);
void insert_answer_variable(size_t index = 0);

void insert_variable(Variable *v, bool add_to_menu = true);
void insert_unit(Unit *u, bool add_to_recent = false);
void insert_matrix(const MathStructure *initial_value = NULL, GtkWindow *win = NULL, gboolean create_vector = FALSE, bool is_text_struct = false, bool is_result = false, GtkEntry *entry = NULL);
void apply_function(MathFunction *f);

void add_as_variable();

void set_input_base(int base, bool opendialog = false, bool recalculate = true);
void set_output_base(int base);
void set_twos_complement(int bo = -1, int ho = -1, int bi = -1, int hi = -1, bool recalculate = true);
void set_binary_bits(unsigned int i, bool recalculate = true);
void set_fraction_format(int nff);
void toggle_fraction_format(bool b);
void set_fixed_fraction(long int v, bool combined);
void set_min_exp(int min_exp, bool extended);
void set_prefix_mode(int i);
void set_approximation(ApproximationMode approx);
void set_angle_unit(AngleUnit au);
void set_custom_angle_unit(Unit *u);
void set_precision(int v, int recalc = -1);

void update_exchange_rates();
void import_definitions_file();
void show_about();
void report_bug();
void set_unknowns();
void open_convert_number_bases();
void open_convert_floatingpoint();
void open_percentage_tool();
void open_calendarconversion();
void show_unit_conversion();
void open_plot();

bool qalculate_quit();

void block_calculation();
void unblock_calculation();
bool calculation_blocked();
void block_error();
void unblock_error();
bool error_blocked();
void block_result();
void unblock_result();
bool result_blocked();

void load_mode(const mode_struct *mode);

bool save_defs(bool allow_cancel = false);
void save_mode();
void load_preferences();
bool save_preferences(bool mode = false, bool allow_cancel = false);
bool save_history(bool allow_cancel = false);

#endif

