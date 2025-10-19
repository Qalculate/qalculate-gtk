/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <gtk/gtk.h>

void edit_preferences(GtkWindow *parent, int tab = -1);

GtkWidget* get_preferences_dialog();

void preferences_update_twos_complement(bool initial = false);
void preferences_update_temperature_calculation(bool initial = false);
void preferences_update_dot(bool initial = false);
void preferences_update_persistent_keypad();
void preferences_update_keep_above();
void preferences_update_expression_status();
void preferences_update_exchange_rates();
void preferences_update_completion(bool initial = false);
void preferences_rpn_mode_changed();
void preferences_parsing_mode_changed();

void preferences_dialog_set(const gchar *obj, gboolean b);
void preferences_dialog_set_combo(const gchar *obj, int i);

#endif /* PREFERENCES_DIALOG_H */
