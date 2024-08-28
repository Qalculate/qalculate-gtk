/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef DATASETS_DIALOG_H
#define DATASETS_DIALOG_H

#include <gtk/gtk.h>
#include <stdio.h>

void update_datasets_tree();
void manage_datasets(GtkWindow *parent);
void update_datasets_settings();

bool read_datasets_dialog_settings_line(std::string &svar, std::string &svalue, int &v);
void write_datasets_dialog_settings(FILE *file);

#endif /* DATASETS_DIALOG_H */
