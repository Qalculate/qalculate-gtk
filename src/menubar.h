/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MENUBAR_H
#define MENUBAR_H

#include <libqalculate/qalculate.h>
#include <gtk/gtk.h>

struct mode_struct;

void create_umenu(void);
void create_umenu2(void);
void create_vmenu(void);
void create_fmenu(void);
void create_pmenu(GtkWidget *item);
void create_pmenu2(void);
void recreate_recent_functions();
void recreate_recent_variables();
void recreate_recent_units();

void add_recent_function(MathFunction *object);
void add_recent_variable(Variable *object);
void add_recent_unit(Unit *object);
void remove_from_recent_functions(MathFunction *f);
void remove_from_recent_variables(Variable *v);
void remove_from_recent_units(Unit *u);

void update_menu_accels(int type);
void update_menu_base();
void update_menu_approximation();
void update_menu_numerical_display();
void update_menu_fraction();
void update_menu_calculator_mode();
void update_menu_angle();
void set_assumptions_items(AssumptionType at, AssumptionSign as);
void set_mode_items(const mode_struct *mode, bool initial_update);
void add_custom_angles_to_menus();
void convert_to_unit(GtkMenuItem*, gpointer user_data);

bool combined_fixed_fraction_set();

void create_menubar();

bool read_menubar_settings_line(std::string &svar, std::string &svalue, int &v);
void write_menubar_settings(FILE *file);

void on_menu_item_set_unknowns_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_simplify_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_factorize_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_expand_partial_fractions_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_manage_functions_activate(GtkMenuItem *w, gpointer user_data);
void on_menu_item_manage_variables_activate(GtkMenuItem *w, gpointer user_data);
void insert_variable_from_menu(GtkMenuItem *w, gpointer user_data);
void insert_prefix_from_menu(GtkMenuItem *w, gpointer user_data);
void insert_unit_from_menu(GtkMenuItem *w, gpointer user_data);

#endif /* MENUBAR_H */
