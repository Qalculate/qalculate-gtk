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

extern PrintOptions printops, parsed_printops, displayed_printops;
extern EvaluationOptions evalops;
extern int simplified_percentage;
extern bool always_on_top;
extern bool implicit_question_asked;
extern bool rpn_mode, chain_mode;
extern bool auto_calculate;
extern int previous_precision;
extern bool caret_as_xor;
extern int enable_tooltips;
extern bool parsed_in_result;
extern int default_signed, default_bits;
extern bool automatic_fraction;
extern int default_fraction_fraction;
extern bool scientific_negexp;
extern bool scientific_notminuslast;
extern bool scientific_noprefix;
extern int auto_prefix;
extern bool fraction_fixed_combined;
extern bool use_custom_result_font, use_custom_expression_font, use_custom_status_font, use_custom_keypad_font, use_custom_app_font, use_custom_history_font;
extern std::string custom_result_font, custom_expression_font, custom_status_font, custom_keypad_font, custom_app_font, custom_history_font;
extern std::string themestr;

extern std::vector<MathFunction*> recent_functions;
extern std::vector<Variable*> recent_variables;
extern std::vector<Unit*> recent_units;

#endif /* QALCULATE_GTK_SETTINGS_H */
