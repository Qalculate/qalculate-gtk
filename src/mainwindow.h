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

#ifdef _WIN32
void create_systray_icon();
void destroy_systray_icon();
#endif
bool has_systray_icon();
void test_border(void);
void create_main_window(void);

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

#ifdef __cplusplus
extern "C" {
#endif

void hide_tooltip(GtkWidget*);

void *view_proc(void*);
void *command_proc(void*);
void on_message_bar_response(GtkInfoBar *w, gint response_id, gpointer);
void on_expander_keypad_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_history_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_stack_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
void on_expander_convert_expanded(GObject *o, GParamSpec *param_spec, gpointer user_data);
gboolean on_gcalc_exit(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_popup_menu_item_persistent_keypad_toggled(GtkCheckMenuItem *w, gpointer user_data);
gboolean on_main_window_focus_in_event(GtkWidget *w, GdkEventFocus *e, gpointer user_data);

void on_button_functions_clicked(GtkButton *button, gpointer user_data);
void on_button_variables_clicked(GtkButton *button, gpointer user_data);
void on_button_units_clicked(GtkButton *button, gpointer user_data);

void on_type_label_date_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_file_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_vector_clicked(GtkEntry *w, gpointer user_data);
void on_type_label_matrix_clicked(GtkEntry *w, gpointer user_data);

gboolean on_menu_key_press(GtkWidget *widget, GdkEventKey *event);

#ifdef __cplusplus
}
#endif

#endif

