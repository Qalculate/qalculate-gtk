/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef STACK_VIEW_H
#define STACK_VIEW_H

#include <gtk/gtk.h>

void create_stack_view();
std::string get_register_text(int index);
void stack_view_swap(int index = -1);
void stack_view_copy(int index = -1);
void stack_view_pop(int index = -1);
void stack_view_rotate(bool up = false);
void stack_view_lastx();
void stack_view_clear();
void RPNStackCleared();
void updateRPNIndexes();
void RPNRegisterAdded(std::string text, gint index = 0);
void RPNRegisterRemoved(gint index);
void RPNRegisterChanged(std::string text, gint index);
void update_stack_font(bool initial = false);
void update_stack_button_font();
void update_lastx();
bool editing_stack();


#endif /* STACK_VIEW_H */
