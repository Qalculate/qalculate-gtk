/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include <gtk/gtk.h>
#include <time.h>
#include <stdio.h>

class MathFunction;

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

void create_history_view();
void initialize_history_functions();
void update_history_accels(int type);
void update_history_button_text();
void update_history_colors(bool initial);
void update_history_font(bool initial = false);
void set_history_font(const char *str);
const char *history_font(bool return_default = false);
void history_font_modified();
std::string history_display_errors(bool add_to_history, GtkWindow *win, int type, bool *implicit_warning, time_t history_time, int *mtype_highest_p);
void reload_history(gint from_index = -1);
bool add_result_to_history_pre(bool update_parse, bool update_history, bool register_moved, bool b_rpn_operation, bool *first_expression, std::string &result_text, std::string &transformation);
void add_result_to_history(bool &update_history, bool update_parse, bool register_moved, bool b_rpn_operation, std::string &result_text, bool b_approx, std::string &parsed_text, bool parsed_approx, std::string &transformation, GtkWindow *win, std::string *error_str, int *mtype_highest_p, bool *implicit_warning);
void add_message_to_history(std::string *error_str, int *mtype_highest_p);
void history_scroll_on_realized();
std::string last_history_expression();
bool history_activated();
void set_history_activated();
int history_new_expression_count();
void history_free();
void fix_history_string2(std::string &str);
void fix_history_string_new2(std::string &str);
void unfix_history_string(std::string &str);
std::string fix_history_string_new(const std::string &str2);
std::string fix_history_string(const std::string &str2);
std::string ellipsize_result(const std::string &result_text, size_t length);
bool editing_history();
void history_search();
void history_clear();
void history_input_base_changed();
void history_operator(std::string str_sign);

GtkWidget *history_view_widget();

MathFunction *answer_function();
MathFunction *expression_function();

bool read_history_settings_line(std::string &svar, std::string &svalue, int &v);
void write_history_settings(FILE *file);
bool read_history_line(std::string &svar, std::string &svalue);
void write_history(FILE *file);

#endif /* HISTORY_VIEW_H */
