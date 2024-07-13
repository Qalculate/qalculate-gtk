/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef VARIABLES_DIALOG_H
#define VARIABLES_DIALOG_H

#include <gtk/gtk.h>

void update_variables_tree();
void manage_variables(GtkWindow *parent, const gchar *str = NULL);
void update_variables_settings();
void variables_font_updated();

#endif /* VARIABLES_DIALOG_H */
