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
void menu_enable_exchange_rates(bool b);
void set_assumptions_items(AssumptionType at, AssumptionSign as);
void set_mode_items(const PrintOptions &po, const EvaluationOptions &eo, AssumptionType at, AssumptionSign as, bool in_rpn_mode, int precision, bool interval, bool variable_units, bool id_adaptive, bool autocalc, bool chainmode, bool caf, bool simper, bool initial_update);
void add_custom_angles_to_menus();
void convert_to_unit(GtkMenuItem*, gpointer user_data);

void create_menubar();

#endif /* MENUBAR_H */
