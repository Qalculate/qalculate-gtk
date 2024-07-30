/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PLOT_DIALOG_H
#define PLOT_DIALOG_H

#include <gtk/gtk.h>
#include <stdio.h>

void hide_plot_dialog();
bool is_plot_dialog(GtkWidget *w);
void show_plot_dialog(GtkWindow *parent, const gchar *text = "");

bool read_plot_settings_line(std::string &svar, std::string &svalue, int &v);
void write_plot_settings(FILE *file);

#endif /* PLOT_DIALOG_H */
