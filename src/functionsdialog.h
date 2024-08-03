/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FUNCTIONS_DIALOG_H
#define FUNCTIONS_DIALOG_H

#include <gtk/gtk.h>
#include <stdio.h>

void update_functions_tree();
void manage_functions(GtkWindow *parent, const gchar *str = NULL);
void update_functions_settings();
void functions_font_updated();

bool read_functions_dialog_settings_line(std::string &svar, std::string &svalue, int &v);
void write_functions_dialog_settings(FILE *file);

#endif /* FUNCTIONS_DIALOG_H */
