/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef KEYPAD_DIALOG_H
#define KEYPAD_DIALOG_H

#include <gtk/gtk.h>
#include <libqalculate/qalculate.h>

enum {
	PROGRAMMING_KEYPAD = 1,
	HIDE_LEFT_KEYPAD = 2,
	HIDE_RIGHT_KEYPAD = 4
};

void set_custom_buttons();
void create_button_menus();
void update_button_padding(bool initial = false);
void create_keypad();

void update_keypad_programming_base();
void update_keypad_fraction();
void update_keypad_exact();
void update_keypad_numerical_display();
void update_keypad_base();
void update_result_bases();
void set_result_bases(const MathStructure &m);
void clear_result_bases();
void update_mb_fx_menu();
void update_mb_sto_menu();
void update_mb_units_menu();
void update_mb_pi_menu();
void update_mb_to_menu();
void update_mb_angles(AngleUnit angle_unit);

#endif /* KEYPAD_DIALOG_H */
