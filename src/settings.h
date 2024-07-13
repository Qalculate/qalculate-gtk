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
extern int enable_tooltips;
extern bool toe_changed;
extern bool always_on_top, aot_changed;
extern bool implicit_question_asked;
extern bool rpn_mode;
extern int previous_precision;

#endif /* QALCULATE_GTK_SETTINGS_H */
