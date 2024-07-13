/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef SETBASE_DIALOG_H
#define SETBASE_DIALOG_H

#include <gtk/gtk.h>

void open_setbase(GtkWindow *parent, bool custom = false, bool input = false);
void bases_updated();

#endif /* SETBASE_DIALOG_H */
