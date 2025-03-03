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

extern PrintOptions printops;
extern EvaluationOptions evalops;
extern bool complex_angle_form;
extern bool adaptive_interval_display;
extern int simplified_percentage;
extern bool copy_ascii, copy_ascii_without_units;
extern bool always_on_top;
extern bool aot_changed;
extern bool implicit_question_asked;
extern bool rpn_mode, chain_mode;
extern bool auto_calculate;
extern bool caret_as_xor;
extern int enable_tooltips;
extern bool toe_changed;
extern bool parsed_in_result;
extern bool load_global_defs, clear_history_on_exit;
extern bool display_expression_status;
extern std::string text_color;
extern bool text_color_set;
extern bool show_parsed_instead_of_result;
extern bool ignore_locale;
extern bool rpn_keys;
extern int close_with_esc;
extern bool check_version;
extern bool persistent_keypad;
extern bool minimal_mode;
extern bool repdeci_overline;
extern std::string themestr;
extern int version_numbers[3];

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

extern unordered_map<guint64, keyboard_shortcut> keyboard_shortcuts;

#endif /* QALCULATE_GTK_SETTINGS_H */
