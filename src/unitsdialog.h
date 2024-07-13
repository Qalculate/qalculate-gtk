/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNITS_DIALOG_H
#define UNITS_DIALOG_H

#include <gtk/gtk.h>

void update_units_tree();
void manage_units(GtkWindow *parent, const gchar *str = NULL, bool show_currencies = false);
void update_units_settings();
void units_font_updated();

#endif /* UNITS_DIALOG_H */
