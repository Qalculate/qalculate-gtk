/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef DATASET_EDIT_DIALOG_H
#define DATASET_EDIT_DIALOG_H

#include <gtk/gtk.h>

class DataSet;

void edit_dataset(DataSet *ds = NULL, GtkWindow *win = NULL);

#endif /* DATASET_EDIT_DIALOG_H */
