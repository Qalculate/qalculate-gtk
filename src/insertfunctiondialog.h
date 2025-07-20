/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef INSERT_FUNCTION_DIALOG_H
#define INSERT_FUNCTION_DIALOG_H

#include <gtk/gtk.h>
#include <stdio.h>

class MathFunction;

void insert_function(MathFunction *f, GtkWindow *parent = NULL, bool add_to_menu = true);
void insert_button_function(MathFunction *f, bool save_to_recent = false, bool apply_to_stack = true);

gint on_function_int_input(GtkSpinButton *entry, gpointer new_value, gpointer);

bool read_insert_function_dialog_settings_line(std::string &svar, std::string &svalue, int &v);
void write_insert_function_dialog_settings(FILE *file);

void insert_function_binary_bits_changed();
void insert_function_twos_complement_changed(bool bo, bool ho, bool bi, bool hi);

void update_insert_function_dialogs();

#endif /* INSERT_FUNCTION_DIALOG_H */
