/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef QALCULATE_GTK_SETTINGS_H
#define QALCULATE_GTK_SETTINGS_H

#include <libqalculate/qalculate.h>
#include "unordered_map_define.h"

extern PrintOptions printops, parsed_printops, displayed_printops;
extern EvaluationOptions evalops;
extern bool complex_angle_form;
extern std::string custom_angle_unit;
extern bool displayed_caf;
extern bool adaptive_interval_display;
extern int simplified_percentage;
extern bool copy_ascii, copy_ascii_without_units;
extern bool use_systray_icon, hide_on_startup;
extern int gtk_theme;
extern bool always_on_top;
extern bool aot_changed;
extern bool implicit_question_asked;
extern int b_decimal_comma;
extern bool dot_question_asked;
extern bool rpn_mode, chain_mode;
extern bool auto_calculate;
extern int previous_precision;
extern bool caret_as_xor;
extern int enable_tooltips;
extern bool toe_changed;
extern bool parsed_in_result;
extern int default_signed, default_bits;
extern bool automatic_fraction;
extern int default_fraction_fraction;
extern bool scientific_negexp;
extern bool scientific_notminuslast;
extern bool scientific_noprefix;
extern int auto_prefix;
extern bool fraction_fixed_combined;
extern bool save_mode_on_exit, save_defs_on_exit, load_global_defs, fetch_exchange_rates_at_startup, clear_history_on_exit, save_history_separately;
extern int max_history_lines;
extern int allow_multiple_instances;
extern int title_type;
extern bool title_modified;
extern int history_expression_type;
extern bool display_expression_status;
extern int expression_lines;
extern std::string custom_lang;
extern std::string status_error_color, status_warning_color, text_color;
extern bool status_error_color_set, status_warning_color_set, text_color_set;
extern int auto_update_exchange_rates;
extern bool ignore_locale;
extern bool rpn_keys;
extern int close_with_esc;
extern bool check_version;
extern int max_plot_time;
extern int autocalc_history_delay;
extern bool use_systray_icon, hide_on_startup;
extern bool tc_set;
extern int horizontal_button_padding, vertical_button_padding;
extern bool persistent_keypad;
extern bool remember_position;
extern bool use_custom_result_font, use_custom_expression_font, use_custom_status_font, use_custom_keypad_font, use_custom_app_font, use_custom_history_font;
extern bool save_custom_result_font, save_custom_expression_font, save_custom_status_font, save_custom_keypad_font, save_custom_app_font, save_custom_history_font;
extern std::string custom_result_font, custom_expression_font, custom_status_font, custom_keypad_font, custom_app_font, custom_history_font;
extern std::string current_mode;
extern std::string themestr;

extern std::vector<MathFunction*> recent_functions;
extern std::vector<Variable*> recent_variables;
extern std::vector<Unit*> recent_units;

extern unordered_map<std::string, cairo_surface_t*> flag_surfaces;
extern int flagheight;

enum {
	SHORTCUT_TYPE_FUNCTION,
	SHORTCUT_TYPE_FUNCTION_WITH_DIALOG,
	SHORTCUT_TYPE_VARIABLE,
	SHORTCUT_TYPE_UNIT,
	SHORTCUT_TYPE_TEXT,
	SHORTCUT_TYPE_DATE,
	SHORTCUT_TYPE_VECTOR,
	SHORTCUT_TYPE_MATRIX,
	SHORTCUT_TYPE_SMART_PARENTHESES,
	SHORTCUT_TYPE_CONVERT,
	SHORTCUT_TYPE_CONVERT_ENTRY,
	SHORTCUT_TYPE_OPTIMAL_UNIT,
	SHORTCUT_TYPE_BASE_UNITS,
	SHORTCUT_TYPE_OPTIMAL_PREFIX,
	SHORTCUT_TYPE_TO_NUMBER_BASE,
	SHORTCUT_TYPE_FACTORIZE,
	SHORTCUT_TYPE_EXPAND,
	SHORTCUT_TYPE_PARTIAL_FRACTIONS,
	SHORTCUT_TYPE_SET_UNKNOWNS,
	SHORTCUT_TYPE_RPN_UP,
	SHORTCUT_TYPE_RPN_DOWN,
	SHORTCUT_TYPE_RPN_SWAP,
	SHORTCUT_TYPE_RPN_COPY,
	SHORTCUT_TYPE_RPN_LASTX,
	SHORTCUT_TYPE_RPN_DELETE,
	SHORTCUT_TYPE_RPN_CLEAR,
	SHORTCUT_TYPE_META_MODE,
	SHORTCUT_TYPE_OUTPUT_BASE,
	SHORTCUT_TYPE_INPUT_BASE,
	SHORTCUT_TYPE_EXACT_MODE,
	SHORTCUT_TYPE_DEGREES,
	SHORTCUT_TYPE_RADIANS,
	SHORTCUT_TYPE_GRADIANS,
	SHORTCUT_TYPE_FRACTIONS,
	SHORTCUT_TYPE_MIXED_FRACTIONS,
	SHORTCUT_TYPE_SCIENTIFIC_NOTATION,
	SHORTCUT_TYPE_SIMPLE_NOTATION,
	SHORTCUT_TYPE_RPN_MODE,
	SHORTCUT_TYPE_AUTOCALC,
	SHORTCUT_TYPE_PROGRAMMING,
	SHORTCUT_TYPE_KEYPAD,
	SHORTCUT_TYPE_HISTORY,
	SHORTCUT_TYPE_HISTORY_SEARCH,
	SHORTCUT_TYPE_CONVERSION,
	SHORTCUT_TYPE_STACK,
	SHORTCUT_TYPE_MINIMAL,
	SHORTCUT_TYPE_MANAGE_VARIABLES,
	SHORTCUT_TYPE_MANAGE_FUNCTIONS,
	SHORTCUT_TYPE_MANAGE_UNITS,
	SHORTCUT_TYPE_MANAGE_DATA_SETS,
	SHORTCUT_TYPE_STORE,
	SHORTCUT_TYPE_MEMORY_CLEAR,
	SHORTCUT_TYPE_MEMORY_RECALL,
	SHORTCUT_TYPE_MEMORY_STORE,
	SHORTCUT_TYPE_MEMORY_ADD,
	SHORTCUT_TYPE_MEMORY_SUBTRACT,
	SHORTCUT_TYPE_NEW_VARIABLE,
	SHORTCUT_TYPE_NEW_FUNCTION,
	SHORTCUT_TYPE_PLOT,
	SHORTCUT_TYPE_NUMBER_BASES,
	SHORTCUT_TYPE_FLOATING_POINT,
	SHORTCUT_TYPE_CALENDARS,
	SHORTCUT_TYPE_PERCENTAGE_TOOL,
	SHORTCUT_TYPE_PERIODIC_TABLE,
	SHORTCUT_TYPE_UPDATE_EXRATES,
	SHORTCUT_TYPE_COPY_RESULT,
	SHORTCUT_TYPE_SAVE_IMAGE,
	SHORTCUT_TYPE_HELP,
	SHORTCUT_TYPE_QUIT,
	SHORTCUT_TYPE_CHAIN_MODE,
	SHORTCUT_TYPE_ALWAYS_ON_TOP,
	SHORTCUT_TYPE_DO_COMPLETION,
	SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION,
	SHORTCUT_TYPE_INSERT_RESULT,
	SHORTCUT_TYPE_HISTORY_CLEAR,
	SHORTCUT_TYPE_PRECISION,
	SHORTCUT_TYPE_MIN_DECIMALS,
	SHORTCUT_TYPE_MAX_DECIMALS,
	SHORTCUT_TYPE_MINMAX_DECIMALS
};

#define LAST_SHORTCUT_TYPE SHORTCUT_TYPE_MINMAX_DECIMALS

struct keyboard_shortcut {
	guint key;
	guint modifier;
	std::vector<int> type;
	std::vector<std::string> value;
};

struct custom_button {
	int type[3];
	std::string value[3], text;
	custom_button() {type[0] = -1; type[1] = -1; type[2] = -1;}
};

extern unordered_map<guint64, keyboard_shortcut> keyboard_shortcuts;
extern bool default_shortcuts;
extern std::vector<custom_button> custom_buttons;

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
extern std::vector<mode_struct> modes;

size_t save_mode_as(std::string name, bool *new_mode = NULL);
void load_mode(std::string name);

bool save_defs(bool allow_cancel = false);
void save_mode();

void load_preferences();
bool save_preferences(bool mode = false, bool allow_cancel = false);
bool save_history(bool allow_cancel = false);

#endif /* QALCULATE_GTK_SETTINGS_H */
