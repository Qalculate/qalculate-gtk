/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNKNOWN_EDIT_DIALOG_H
#define UNKNOWN_EDIT_DIALOG_H

#include <gtk/gtk.h>

class Variable;

void edit_unknown(const char *category = "", Variable *v = NULL, GtkWindow *win = NULL);

#endif /* UNKNOWN_EDIT_DIALOG_H */
