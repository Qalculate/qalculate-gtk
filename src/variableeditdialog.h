/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef VARIABLE_EDIT_DIALOG_H
#define VARIABLE_EDIT_DIALOG_H

#include <gtk/gtk.h>

class Variable;

void edit_variable(const char *category = "", Variable *v = NULL, MathStructure *mstruct_ = NULL, GtkWindow *win = NULL);

#endif /* VARIABLE_EDIT_DIALOG_H */
