/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef CONVERSION_VIEW_H
#define CONVERSION_VIEW_H

#include <gtk/gtk.h>
#include <stdio.h>

class MathStructure;

void create_conversion_view();
void update_conversion_view_selection(const MathStructure*);
void update_unit_selector_tree();
std::string current_conversion_expression();
void focus_conversion_entry();
bool conversionview_continuous_conversion();

bool read_conversion_view_settings_line(std::string &svar, std::string &svalue, int &v);
void write_conversion_view_settings(FILE *file);

#endif /* CONVERSION_VIEW_H */
