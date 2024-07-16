/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MATRIX_DIALOG_H
#define MATRIX_DIALOG_H

#include <gtk/gtk.h>

class MathStructure;

std::string get_matrix(const MathStructure *initial_value = NULL, GtkWindow *win = NULL, gboolean create_vector = FALSE, bool is_text_struct = false, bool is_result = false);

#endif /* MATRIX_DIALOG_H */
