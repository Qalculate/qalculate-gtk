/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef NAMES_EDIT_DIALOG_H
#define NAMES_EDIT_DIALOG_H

#include <gtk/gtk.h>

class DataProperty;
class ExpressionItem;

bool edit_names(ExpressionItem *item, int type, const gchar *namestr, GtkWindow *win, DataProperty *dp = NULL);
void set_name_label_and_entry(ExpressionItem *item, GtkWidget *entry, GtkWidget *label = NULL);
void correct_name_entry(GtkEditable *editable, ExpressionItemType etype, gpointer function);
void set_edited_names(ExpressionItem *item, std::string str);
void set_edited_names(DataProperty *dp, std::string str);
bool has_name();
std::string first_name();
void reset_names_status();
void set_names_status(int);
int names_status();
void name_entry_changed();

#endif /* NAMES_EDIT_DIALOG_H */
