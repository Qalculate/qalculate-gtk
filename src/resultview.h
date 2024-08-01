/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef RESULT_VIEW_H
#define RESULT_VIEW_H

#include <gtk/gtk.h>
#include <libqalculate/qalculate.h>

void display_parsed_instead_of_result(bool b);
void result_display_updated();
void result_view_clear();
void result_view_clear_parsed();
bool result_view_empty();
bool parsed_expression_is_displayed_instead_of_result();
bool current_parsed_expression_is_displayed_in_result();
void start_result_spinner();
void stop_result_spinner();
bool draw_result(MathStructure *displayed_mstruct_pre);
void draw_result_temp(MathStructure &m);
bool draw_result_finalize();
void draw_result_failure(MathStructure &m, bool too_long);
void draw_result_pre();
void draw_result_post();
void draw_result_waiting();
void draw_result_check();
void draw_result_backup();
void draw_result_clear();
void draw_result_restore();
void draw_result_destroy();
void draw_parsed(MathStructure &mparse, const PrintOptions &po);
void redraw_result();
void save_as_image();
MathStructure *current_displayed_result();
bool result_did_not_fit(bool only_too_long = true);
void update_displayed_printops();
const PrintOptions &current_displayed_printops();
void update_result_font(bool initial = false);
void set_result_font(const char *str);
const char *result_font(bool return_default = false);
void result_font_modified();
void set_result_size_request();
void update_result_accels(int type);
void show_result_help();

void create_result_view();

GtkWidget *result_view_widget();

bool read_result_view_settings_line(std::string &svar, std::string &svalue, int &v);
void write_result_view_settings(FILE *file);

#endif /* RESULT_VIEW_H */
