/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FUNCTION_EDIT_DIALOG_H
#define FUNCTION_EDIT_DIALOG_H

#include <gtk/gtk.h>

class MathFunction;

void edit_function(const char *category = "", MathFunction *f = NULL, GtkWindow *win = NULL, const char *name = NULL, const char *expression = NULL, bool enable_ok = true);

#endif /* FUNCTION_EDIT_DIALOG_H */
