/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPRESSION_COMPLETION_H
#define EXPRESSION_COMPLETION_H

#include <gtk/gtk.h>

extern int completion_min, completion_min2;
extern bool enable_completion, enable_completion2;
extern int completion_delay;

void block_completion();
void unblock_completion();

bool completion_visible();
void hide_completion();
void toggle_completion_visible();
bool activate_first_completion();

bool completion_enter_pressed();
void completion_up_pressed();
void completion_down_pressed();

void update_completion();

void create_expression_completion();

void add_completion_timeout();
void stop_completion_timeout();

#endif /* EXPRESSION_COMPLETION_H */
