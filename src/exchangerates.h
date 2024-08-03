/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXCHANGE_RATES_H
#define EXCHANGE_RATES_H

#include <gtk/gtk.h>
#include <stdio.h>

bool check_exchange_rates(GtkWindow *win = NULL, bool set_result = false);
void fetch_exchange_rates(int timeout, int n = -1);

int exchange_rates_frequency();
void set_exchange_rates_frequency(int v);

bool read_exchange_rates_settings_line(std::string &svar, std::string &svalue, int &v);
void write_exchange_rates_settings(FILE *file);

#endif /* EXCHANGE_RATES_H */
