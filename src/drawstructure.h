/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef DRAW_STRUCTURE_H
#define DRAW_STRUCTURE_H

#include <gtk/gtk.h>
#include <libqalculate/qalculate.h>

cairo_surface_t *draw_structure(MathStructure &m, PrintOptions po = default_print_options, bool caf = false, InternalPrintStruct ips = top_ips, gint *point_central = NULL, int scaledown = 0, GdkRGBA *color = NULL, gint *x_offset = NULL, gint *w_offset = NULL, gint max_width = -1, bool for_result_widget = true, MathStructure *where_struct = NULL, std::vector<MathStructure> *to_structs = NULL);

void clear_draw_caches();
void calculate_par_width();
void draw_font_modified();
int get_binary_result_pos(int x, int y);

#endif /* DRAW_STRUCTURE_H */
