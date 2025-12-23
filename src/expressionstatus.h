/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPRESSION_STATUS_H
#define EXPRESSION_STATUS_H

#include <libqalculate/qalculate.h>
#include <gtk/gtk.h>
#include <stdio.h>

void update_status_colors(bool initial);
void update_status_font(bool initial = false);
void set_status_font(const char *str);
const char *status_font(bool return_default = false);
void set_status_error_color(const char *color_str);
void set_status_warning_color(const char *color_str);
const char *status_error_color();
const char *status_warning_color();
void update_status_menu(bool initial = false);
void status_font_modified();
void set_status_operator_symbols();
void set_status_bottom_border_visible(bool b);
void update_status_approximation();
void update_status_angle();
void update_status_syntax();
void create_expression_status();

void update_status_text();
void display_parse_status();
void clear_status_text();
void set_status_selection_text(const std::string &str, bool had_errors = false, bool had_warnings = false);
void clear_status_selection_text();

bool parse_status_error();

void block_status();
void unblock_status();
bool status_blocked();

MathStructure &current_parsed_expression();
MathStructure &current_parsed_function_struct();
MathStructure &current_parsed_where();
std::vector<MathStructure> &current_parsed_to();
void clear_parsed_expression();
const std::string &current_parsed_expression_text();
MathFunction *current_parsed_function();
size_t current_parsed_function_index();

bool read_expression_status_settings_line(std::string &svar, std::string &svalue, int &v);
void write_expression_status_settings(FILE *file);

GtkWidget *parse_status_widget();

#endif /* EXPRESSION_STATUS_H */
