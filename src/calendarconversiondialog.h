/*
    Qalculate (GTK UI)

    Copyright (C) 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef CALENDAR_CONVERSION_DIALOG_H
#define CALENDAR_CONVERSION_DIALOG_H

#include <gtk/gtk.h>

class QalculateDateTime;

void show_calendarconversion_dialog(GtkWindow *parent, QalculateDateTime *datetime = NULL);

void calendarconversion_dialog_result_has_changed(const MathStructure *value);

#endif /* CALENDAR_CONVERSION_DIALOG_H */
