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

DECLARE_BUILTIN_FUNCTION(SetTitleFunction, 0)

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

cairo_surface_t *draw_structure(MathStructure &m, PrintOptions po = default_print_options, bool caf = false, InternalPrintStruct ips = top_ips, gint *point_central = NULL, int scaledown = 0, GdkRGBA *color = NULL, gint *x_offset = NULL, gint *w_offset = NULL, gint max_width = -1, bool for_result_widget = true, MathStructure *where_struct = NULL, std::vector<MathStructure> *to_structs = NULL);

void update_status_text();

void set_result_size_request();
void set_status_size_request();

void remove_old_my_variables_category();

void generate_functions_tree_struct();
void generate_variables_tree_struct();
void generate_units_tree_struct();

gboolean on_display_errors_timeout(gpointer data);
gboolean on_check_version_idle(gpointer data);

void execute_from_file(std::string file_name);

void set_rpn_mode(bool b);
void calculateRPN(int op);
void calculateRPN(MathFunction *f);

void function_inserted(MathFunction *object);
void variable_inserted(Variable *object);
void unit_inserted(Unit *object);

gint completion_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

void set_prefix(GtkMenuItem *w, gpointer user_data);

void apply_function(GtkMenuItem *w, gpointer user_data);

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

void set_status_bottom_border_visible(bool);

#ifdef __cplusplus
extern "C" {
#endif

void hide_tooltip(GtkWidget*);

void on_completion_match_selected(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn*, gpointer);

void *view_proc(void*);
void *command_proc(void*);
void on_message_bar_response(GtkInfoBar *w, gint response_id, gpointer);
void on_expressiontext_populate_popup(GtkTextView *w, GtkMenu *menu, gpointer user_data);
void on_expander_keypad_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_history_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_stack_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_convert_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
gboolean on_gcalc_exit(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_menu_item_expression_status_activate(GtkMenuItem *w, gpointer);
void on_menu_item_parsed_in_result_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_exact_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer user_data);
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
gboolean on_main_window_focus_in_event(GtkWidget *w, GdkEventFocus *e, gpointer user_data);

void on_popup_menu_item_calendarconversion_activate(GtkMenuItem *w, gpointer user_data);
void on_popup_menu_item_to_utc_activate(GtkMenuItem *w, gpointer user_data);

void on_button_functions_clicked(GtkButton *button, gpointer user_data);
void on_button_variables_clicked(GtkButton *button, gpointer user_data);
void on_button_units_clicked(GtkButton *button, gpointer user_data);

void on_precision_dialog_spinbutton_precision_value_changed(GtkSpinButton *w, gpointer user_data);
void on_precision_dialog_button_recalculate_clicked(GtkButton *w, gpointer user_data);
void on_decimals_dialog_spinbutton_max_value_changed(GtkSpinButton *w, gpointer user_data);
void on_decimals_dialog_spinbutton_min_value_changed(GtkSpinButton *w, gpointer user_data);
void on_decimals_dialog_checkbutton_max_toggled(GtkToggleButton *w, gpointer user_data);
void on_decimals_dialog_checkbutton_min_toggled(GtkToggleButton *w, gpointer user_data);

gboolean on_expressiontext_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer user_data);

gboolean on_resultview_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer user_data);
gboolean on_resultview_popup_menu(GtkWidget *w, gpointer user_data);

gboolean on_resultview_draw(GtkWidget *w, cairo_t *cr, gpointer user_data);

void on_type_label_date_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_file_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_vector_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_matrix_clicked(GtkEntry *w, gpointer user_data);

void on_unit_dialog_button_apply_clicked(GtkButton *w, gpointer user_data);
void on_unit_dialog_button_ok_clicked(GtkButton *w, gpointer user_data);
void on_unit_dialog_entry_unit_activate(GtkEntry *entry, gpointer user_data);

gboolean on_menu_key_press(GtkWidget *widget, GdkEventKey *event);

#ifdef __cplusplus
}
#endif

#endif

