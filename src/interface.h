/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef _WIN32
void create_systray_icon();
void destroy_systray_icon();
#endif
bool has_systray_icon();
void test_border(void);
void create_main_window(void);

#endif /* INTERFACE_H */
