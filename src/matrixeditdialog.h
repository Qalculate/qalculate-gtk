/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MATRIX_EDIT_DIALOG_H
#define MATRIX_EDIT_DIALOG_H

#include <gtk/gtk.h>

class Variable;

void edit_matrix(const char *category = "", Variable *v = NULL, MathStructure *mstruct_ = NULL, GtkWindow *win = NULL, gboolean create_vector = false);

#endif /* MATRIX_EDIT_DIALOG_H */
