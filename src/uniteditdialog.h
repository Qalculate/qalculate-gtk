/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNIT_EDIT_DIALOG_H
#define UNIT_EDIT_DIALOG_H

#include <gtk/gtk.h>

class Unit;

void edit_unit(const char *category = "", Unit *u = NULL, GtkWindow *win = NULL);

#endif /* UNIT_EDIT_DIALOG_H */
