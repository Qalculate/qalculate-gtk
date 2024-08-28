/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef OPEN_HELP_H
#define OPEN_HELP_H

#include <gtk/gtk.h>

void show_help(const char *file, GtkWindow *parent);

bool read_help_settings_line(std::string &svar, std::string &svalue, int &v);
void write_help_settings(FILE *file);

#endif /* OPEN_HELP_H */
