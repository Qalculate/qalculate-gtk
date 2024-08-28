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
#include <stdio.h>

enum {
	PROGRAMMING_KEYPAD = 1,
	HIDE_LEFT_KEYPAD = 2,
	HIDE_RIGHT_KEYPAD = 4
};

struct custom_button {
	int type[3];
	std::string value[3], text;
	custom_button() {type[0] = -1; type[1] = -1; type[2] = -1;}
};

void set_custom_buttons();
void create_button_menus();
void update_button_padding(bool initial = false);
void update_keypad_state(bool initial = false);
void update_keypad_accels(int type);
void update_custom_buttons(int index = -1);
void create_keypad();

GtkWidget *keypad_widget();

bool read_keypad_settings_line(std::string &svar, std::string &svalue, int &v);
void write_keypad_settings(FILE *file);

void update_keypad_caret_as_xor();
void update_keypad_i();
void update_keypad_button_text();
void update_keypad_font(bool initial = false);
void set_keypad_font(const char *str);
const char *keypad_font(bool return_default = false);
void set_vertical_button_padding(int i);
void set_horizontal_button_padding(int i);
int vertical_button_padding();
int horizontal_button_padding();
void update_keypad_programming_base();
void update_keypad_fraction();
void update_keypad_exact();
void update_keypad_numerical_display();
void update_keypad_base();
void update_keypad_angle();
void update_result_bases();
void keypad_rpn_mode_changed();
void set_result_bases(const MathStructure &m);
void clear_result_bases();
void update_mb_fx_menu();
void update_mb_sto_menu();
void update_mb_units_menu();
void update_mb_pi_menu();
void update_mb_to_menu();
void keypad_algebraic_mode_changed();

#endif /* KEYPAD_DIALOG_H */
