/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "support.h"
#include "settings.h"
#include "util.h"
#include "calendarconversiondialog.h"

#include "unordered_map_define.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *calendarconversion_builder = NULL;

unordered_map<size_t, GtkWidget*> cal_year, cal_month, cal_day, cal_label;
GtkWidget *chinese_stem, *chinese_branch;
bool block_calendar_conversion = false;

void calendar_changed(GtkWidget*, gpointer data) {
	if(block_calendar_conversion) return;
	block_calendar_conversion = true;
	gint i = GPOINTER_TO_INT(data);
	long int y;
	if(i == CALENDAR_CHINESE) {
		long int cy = chineseStemBranchToCycleYear((gtk_combo_box_get_active(GTK_COMBO_BOX(chinese_stem)) * 2) + 1, gtk_combo_box_get_active(GTK_COMBO_BOX(chinese_branch)) + 1);
		if(cy <= 0) {
			show_message(_("The selected Chinese year does not exist."), GTK_WINDOW(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog")));
			block_calendar_conversion = false;
			return;
		}
		y = chineseCycleYearToYear(79, cy);
	} else {
		y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cal_year[(size_t) i]));
	}
	long int m = gtk_combo_box_get_active(GTK_COMBO_BOX(cal_month[(size_t) i])) + 1;
	long int d = gtk_combo_box_get_active(GTK_COMBO_BOX(cal_day[(size_t) i])) + 1;
	QalculateDateTime date;
	if(!calendarToDate(date, y, m, d, (CalendarSystem) i)) {
		show_message(_("Conversion to Gregorian calendar failed."), GTK_WINDOW(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog")));
		block_calendar_conversion = false;
		return;
	}
	string failed_str;
	for(size_t i2 = 0; i2 < NUMBER_OF_CALENDARS; i2++) {
		if(cal_day.count(i2) > 0) {
			if(dateToCalendar(date, y, m, d, (CalendarSystem) i2) && y <= G_MAXINT && y >= G_MININT && m <= numberOfMonths((CalendarSystem) i2) && d <= 31) {
				if(i2 == CALENDAR_CHINESE) {
					long int cy, yc, st, br;
					chineseYearInfo(y, cy, yc, st, br);
					gtk_combo_box_set_active(GTK_COMBO_BOX(chinese_stem), (st - 1) / 2);
					gtk_combo_box_set_active(GTK_COMBO_BOX(chinese_branch), br - 1);
				} else {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal_year[i2]), y);
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(cal_month[i2]), m - 1);
				gtk_combo_box_set_active(GTK_COMBO_BOX(cal_day[i2]), d - 1);
			} else {
				if(!failed_str.empty()) failed_str += ", ";
				failed_str += gtk_label_get_text(GTK_LABEL(cal_label[i2]));
			}
		}
	}
	if(!failed_str.empty()) {
		gchar *gstr = g_strdup_printf(_("Calendar conversion failed for: %s."), failed_str.c_str());
		show_message(gstr, GTK_WINDOW(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog")));
		g_free(gstr);
	}
	block_calendar_conversion = false;
}

GtkWidget* get_calendarconversion_dialog(void) {
	if(!calendarconversion_builder) {

		calendarconversion_builder = getBuilder("calendarconversion.ui");
		g_assert(calendarconversion_builder != NULL);

		g_assert(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog") != NULL);

		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_1")), _("Gregorian"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_8")), _("Revised Julian (Milanković)"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_7")), _("Julian"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_3")), _("Islamic (Hijri)"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_2")), _("Hebrew"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_6")), _("Chinese"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_4")), _("Persian (Solar Hijri)"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_9")), _("Coptic"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_10")), _("Ethiopian"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_5")), _("Indian (National)"));

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_1")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_2")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_3")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_4")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_5")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_6")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_7")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_8")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_9")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_10")), 12);
#else
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_1")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_2")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_3")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_4")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_5")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_6")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_7")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_8")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_9")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_10")), 12);
#endif

		cal_year[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_1"));
		cal_month[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_1"));
		cal_day[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_1"));
		cal_label[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_1"));
		cal_year[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_2"));
		cal_month[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_2"));
		cal_day[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_2"));
		cal_label[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_2"));
		cal_year[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_3"));
		cal_month[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_3"));
		cal_day[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_3"));
		cal_label[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_3"));
		cal_year[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_4"));
		cal_month[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_4"));
		cal_day[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_4"));
		cal_label[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_4"));
		cal_year[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_5"));
		cal_month[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_5"));
		cal_day[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_5"));
		cal_label[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_5"));
		cal_year[CALENDAR_CHINESE] = NULL;
		chinese_stem = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "stem_6"));
		chinese_branch = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "branch_6"));
		cal_month[CALENDAR_CHINESE] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_6"));
		cal_day[CALENDAR_CHINESE] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_6"));
		cal_label[CALENDAR_CHINESE] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_6"));
		cal_year[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_7"));
		cal_month[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_7"));
		cal_day[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_7"));
		cal_label[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_7"));
		cal_year[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_8"));
		cal_month[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_8"));
		cal_day[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_8"));
		cal_label[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_8"));
		cal_year[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_9"));
		cal_month[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_9"));
		cal_day[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_9"));
		cal_label[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_9"));
		cal_year[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_10"));
		cal_month[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_10"));
		cal_day[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_10"));
		cal_label[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_10"));

		for(size_t i = 0; i < NUMBER_OF_CALENDARS; i++) {
			if(cal_day.count(i) > 0) {
				if(i == CALENDAR_CHINESE) {
					for(size_t i2 = 1; i2 <= 5; i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(chinese_stem), chineseStemName(i2 * 2).c_str());
					for(size_t i2 = 1; i2 <= 12; i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(chinese_branch), chineseBranchName(i2).c_str());
				} else {
					gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal_year[i]), G_MININT, G_MAXINT);
					gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(cal_year[i]), TRUE);
					gtk_spin_button_set_increments(GTK_SPIN_BUTTON(cal_year[i]), 1.0, 10.0);
					gtk_spin_button_set_digits(GTK_SPIN_BUTTON(cal_year[i]), 0);
					gtk_entry_set_alignment(GTK_ENTRY(cal_year[i]), 1.0);
				}
				for(size_t i2 = 1; i2 <= (size_t) numberOfMonths((CalendarSystem) i); i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cal_month[i]), monthName(i2, (CalendarSystem) i, true).c_str());
				for(size_t i2 = 1; i2 <= 31; i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cal_day[i]), i2s(i2).c_str());
			}
		}

		QalculateDateTime date;
		date.setToCurrentDate();
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal_year[CALENDAR_GREGORIAN]), date.year());
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_month[CALENDAR_GREGORIAN]), date.month() - 1);

		for(size_t i = 0; i < NUMBER_OF_CALENDARS; i++) {
			if(cal_day.count(i) > 0) {
				if(i == CALENDAR_CHINESE) {
					g_signal_connect(chinese_stem, "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
					g_signal_connect(chinese_branch, "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
				} else {
					g_signal_connect(cal_year[i], "value-changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
				}
				g_signal_connect(cal_month[i], "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
				g_signal_connect(cal_day[i], "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
			}
		}

		gtk_builder_connect_signals(calendarconversion_builder, NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_day[CALENDAR_GREGORIAN]), date.day() - 1);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog"));
}

void show_calendarconversion_dialog(GtkWindow *parent, QalculateDateTime *datetime) {
	GtkWidget *dialog = get_calendarconversion_dialog();
	if(datetime) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal_year[CALENDAR_GREGORIAN]), datetime->year());
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_month[CALENDAR_GREGORIAN]), datetime->month() - 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_day[CALENDAR_GREGORIAN]), datetime->day() - 1);
	}
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_1")));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}

void calendarconversion_dialog_result_has_changed(const MathStructure *value) {
	if(calendarconversion_builder && gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog"))) && value && value->isDateTime()) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal_year[CALENDAR_GREGORIAN]), value->datetime()->year());
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_month[CALENDAR_GREGORIAN]), value->datetime()->month() - 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_day[CALENDAR_GREGORIAN]), value->datetime()->day() - 1);
	}
}
