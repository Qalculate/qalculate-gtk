/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FLOATINGPOINT_DIALOG_H
#define FLOATINGPOINT_DIALOG_H

#include <gtk/gtk.h>

class MathStructure;

void convert_floatingpoint(const MathStructure *initial_value, GtkWindow *parent);
void convert_floatingpoint(const gchar *initial_expression, int base, GtkWindow *parent);

#endif /* FLOATINGPOINT_DIALOG_H */
