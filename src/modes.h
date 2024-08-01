/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MODES_H
#define MODES_H

#include <libqalculate/qalculate.h>

struct mode_struct {
	PrintOptions po;
	EvaluationOptions eo;
	AssumptionType at;
	AssumptionSign as;
	Number custom_output_base;
	Number custom_input_base;
	int precision;
	std::string name;
	bool rpn_mode;
	bool interval;
	bool adaptive_interval_display;
	bool variable_units_enabled;
	int keypad;
	bool autocalc;
	bool chain_mode;
	bool complex_angle_form;
	bool implicit_question_asked;
	int simplified_percentage;
	bool concise_uncertainty_input;
	long int fixed_denominator;
	std::string custom_angle_unit;
};

size_t remove_mode(std::string name);
size_t save_mode_as(std::string name, bool *new_mode = NULL, bool set_as_current = false);
size_t initialize_mode_as(std::string name);
size_t mode_count(bool include_default = true);
mode_struct *get_mode(size_t index);
size_t mode_index(std::string name, bool case_sensitive = true);
void save_initial_modes();
void save_default_mode(const char *custom_angle_unit = NULL);
bool load_mode(std::string name);
std::string current_mode_name();

bool read_mode_line(size_t index, std::string &svar, std::string &svalue, int &v);
void write_mode(FILE *file, size_t index);

#endif /* MODES_H */
