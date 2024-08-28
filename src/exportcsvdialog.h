/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPORT_CSV_DIALOG_H
#define EXPORT_CSV_DIALOG_H

#include <gtk/gtk.h>

class KnownVariable;

void export_csv_file(GtkWindow *parent, KnownVariable *v = NULL);

#endif /* EXPORT_CSV_DIALOG_H */
