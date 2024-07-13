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
#include "percentagecalculationdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *percentage_builder = NULL;

bool updating_percentage_entries = false;
void update_percentage_entries();
vector<int> percentage_entries_changes;
void on_percentage_button_calculate_clicked(GtkWidget*, gpointer) {
	update_percentage_entries();
}
void on_percentage_button_clear_clicked(GtkWidget*, gpointer) {
	percentage_entries_changes.clear();
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_1")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_2")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_3")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_4")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_5")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_6")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_7")), "");
}
void percentage_entry_changed(int entry_id, GtkEntry *w) {
	for(size_t i = 0; i < percentage_entries_changes.size(); i++) {
		if(percentage_entries_changes[i] == entry_id) {
			percentage_entries_changes.erase(percentage_entries_changes.begin() + i);
			break;
		}
	}
	if(gtk_entry_get_text_length(w) == 0) return;
	percentage_entries_changes.push_back(entry_id);
}
void on_percentage_entry_1_changed(GtkEditable *w, gpointer) {percentage_entry_changed(1, GTK_ENTRY(w));}
void on_percentage_entry_2_changed(GtkEditable *w, gpointer) {percentage_entry_changed(2, GTK_ENTRY(w));}
void on_percentage_entry_3_changed(GtkEditable *w, gpointer) {percentage_entry_changed(4, GTK_ENTRY(w));}
void on_percentage_entry_4_changed(GtkEditable *w, gpointer) {percentage_entry_changed(8, GTK_ENTRY(w));}
void on_percentage_entry_5_changed(GtkEditable *w, gpointer) {percentage_entry_changed(16, GTK_ENTRY(w));}
void on_percentage_entry_6_changed(GtkEditable *w, gpointer) {percentage_entry_changed(32, GTK_ENTRY(w));}
void on_percentage_entry_7_changed(GtkEditable *w, gpointer) {percentage_entry_changed(64, GTK_ENTRY(w));}
void on_percentage_entry_1_activate(GtkEditable *w, gpointer) {percentage_entry_changed(1, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_2_activate(GtkEditable *w, gpointer) {percentage_entry_changed(2, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_3_activate(GtkEditable *w, gpointer) {percentage_entry_changed(4, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_4_activate(GtkEditable *w, gpointer) {percentage_entry_changed(8, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_5_activate(GtkEditable *w, gpointer) {percentage_entry_changed(16, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_6_activate(GtkEditable *w, gpointer) {percentage_entry_changed(32, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_7_activate(GtkEditable *w, gpointer) {percentage_entry_changed(64, GTK_ENTRY(w)); update_percentage_entries();}
void update_percentage_entries() {
	if(updating_percentage_entries) return;
	if(percentage_entries_changes.size() < 2) return;
	int variant = percentage_entries_changes[percentage_entries_changes.size() - 1];
	int variant2 = percentage_entries_changes[percentage_entries_changes.size() - 2];
	if(variant > 4) {
		for(int i = percentage_entries_changes.size() - 3; i >= 0 && variant2 > 4; i--) {
			variant2 = percentage_entries_changes[(size_t) i];
		}
		if(variant2 > 4) return;
	}
	variant += variant2;
	updating_percentage_entries = true;
	GtkWidget *w1 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_1"));
	GtkWidget *w2 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_2"));
	GtkWidget *w3 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_3"));
	GtkWidget *w4 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_4"));
	GtkWidget *w5 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_5"));
	GtkWidget *w6 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_6"));
	GtkWidget *w7 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_7"));
	g_signal_handlers_block_matched((gpointer) w1, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_1_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w2, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_2_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w3, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_3_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w4, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_4_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w5, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_5_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w6, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_6_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w7, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_7_changed, NULL);
	MathStructure m1, m2, m3, m4, m5, m6, m7, m1_pre, m2_pre;
	string str1, str2;
	switch(variant) {
		case 3: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w2)); break;}
		case 5: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w3)); break;}
		case 9: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w4)); break;}
		case 17: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w5)); break;}
		case 33: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w6)); break;}
		case 65: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w7)); break;}
		case 6: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w3)); break;}
		case 10: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w4)); break;}
		case 18: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w5)); break;}
		case 34: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w6)); break;}
		case 66: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w7)); break;}
		case 12: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w4)); break;}
		case 20: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w5)); break;}
		case 36: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w6)); break;}
		case 68: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w7)); break;}
		default: {variant = 0;}
	}
	block_error_timeout++;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = 10;
	eo.assume_denominators_nonzero = true;
	eo.warn_about_denominators_assumed_nonzero = false;
	if(variant != 0) {
		m1_pre.set(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str1, eo.parse_options), eo.parse_options));
		m2_pre.set(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str2, eo.parse_options), eo.parse_options));
	}
	bool b_divzero = false;
	MathStructure mtest;
	if(variant == 17 || variant == 65 || variant == 10 || variant == 34 || variant == 12 || variant == 20 || variant == 36 || variant == 68) {
		mtest = m2_pre;
		CALCULATOR->calculate(&mtest, 500, eo);
		if(!mtest.isNumber()) mtest = m_one;
	}
	switch(variant) {
		case 3: {m1 = m1_pre; m2 = m2_pre; break;}
		case 5: {m1 = m1_pre; m2 = m2_pre; m2 += m1; break;}
		case 9: {m1 = m1_pre; m2 = m2_pre; m2 /= 100; m2 += 1; m2 *= m1; break;}
		case 17: {
			ComparisonResult cr = mtest.number().compare(-100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m2_pre += 1; m2 = m1; m2 /= m2_pre;
			break;
		}
		case 33: {m1 = m1_pre; m2 = m2_pre; m2 /= 100; m2 *= m1; break;}
		case 65: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m2 = m1; m2 /= m2_pre;
			break;
		}
		case 6: {m2 = m1_pre; m1 = m1_pre; m1 -= m2_pre; break;}
		case 10: {
			ComparisonResult cr = mtest.number().compare(-100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m2 = m1_pre; m2_pre /= 100; m2_pre += 1; m1 = m2; m1 /= m2_pre;
			break;
		}
		case 18: {m2 = m1_pre; m2_pre /= 100; m2_pre += 1; m1 = m2; m1 *= m2_pre; break;}
		case 34: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m2 = m1_pre; m2_pre /= 100; m1 = m2; m1 /= m2_pre;
			break;
		}
		case 66: {m2 = m1_pre; m2_pre /= 100; m1 = m2; m1 *= m2_pre; break;}
		case 12: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m1 /= m2_pre; m2 = m1; m2 += m1_pre;
			break;
		}
		case 20: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m1_pre.negate(); m2 = m1_pre; m2_pre /= 100; m2 /= m2_pre; m1 = m2; m1 += m1_pre;
			break;
		}
		case 36: {
			ComparisonResult cr = mtest.number().compare(100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m2_pre -= 1; m1 /= m2_pre; m2 = m1; m2 += m1_pre;
			break;
		}
		case 68: {
			ComparisonResult cr = mtest.number().compare(100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m1_pre.negate(); m2 = m1_pre; m2_pre /= 100; m2_pre -= 1; m2 /= m2_pre; m1 = m2; m1 += m1_pre;
			break;
		}
		default: {variant = 0;}
	}
	if(b_divzero) {
		if(variant != 3 && variant != 5 && variant != 9 && variant != 17 && variant != 33 && variant != 65) gtk_entry_set_text(GTK_ENTRY(w1), "");
		if(variant != 3 && variant != 6 && variant != 10 && variant != 18 && variant != 34 && variant != 66) gtk_entry_set_text(GTK_ENTRY(w2), "");
		if(variant != 5 && variant != 6 && variant != 12 && variant != 20 && variant != 36 && variant != 68) gtk_entry_set_text(GTK_ENTRY(w3), "");
		if(variant != 9 && variant != 10 && variant != 12) gtk_entry_set_text(GTK_ENTRY(w4), "");
		if(variant != 17 && variant != 18 && variant != 20) gtk_entry_set_text(GTK_ENTRY(w5), "");
		if(variant != 33 && variant != 34 && variant != 36) gtk_entry_set_text(GTK_ENTRY(w6), "");
		if(variant != 65 && variant != 66 && variant != 68) gtk_entry_set_text(GTK_ENTRY(w7), "");
	} else if(variant != 0) {
		m3 = m2; m3 -= m1;
		m6 = m2; m6 /= m1;
		m7 = m1; m7 /= m2;
		m4 = m6; m4 -= 1;
		m5 = m7; m5 -= 1;
		m4 *= 100; m5 *= 100; m6 *= 100; m7 *= 100;
		CALCULATOR->calculate(&m1, 500, eo);
		CALCULATOR->calculate(&m2, 500, eo);
		CALCULATOR->calculate(&m3, 500, eo);
		if(!m1.isZero()) CALCULATOR->calculate(&m4, 500, eo);
		if(!m2.isZero()) CALCULATOR->calculate(&m5, 500, eo);
		if(!m1.isZero()) CALCULATOR->calculate(&m6, 500, eo);
		if(!m2.isZero()) CALCULATOR->calculate(&m7, 500, eo);
		PrintOptions po = printops;
		po.is_approximate = NULL;
		po.base = 10;
		po.number_fraction_format = FRACTION_DECIMAL;
		po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		gtk_entry_set_text(GTK_ENTRY(w1), m1.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m1, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w2), m2.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m2, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w3), m3.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m3, 200, po).c_str());
		po.max_decimals = 2;
		po.use_max_decimals = true;
		gtk_entry_set_text(GTK_ENTRY(w4), m1.isZero() ? "" : (m4.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m4, 200, po).c_str()));
		gtk_entry_set_text(GTK_ENTRY(w5), m2.isZero() ? "" : (m5.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m5, 200, po).c_str()));
		gtk_entry_set_text(GTK_ENTRY(w6), m1.isZero() ? "" : (m6.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m6, 200, po).c_str()));
		gtk_entry_set_text(GTK_ENTRY(w7), m2.isZero() ? "" : (m7.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m7, 200, po).c_str()));
	}
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_dialog")));
	block_error_timeout--;
	g_signal_handlers_unblock_matched((gpointer) w1, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_1_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w2, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_2_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w3, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_3_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w4, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_4_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w5, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_5_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w6, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_6_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w7, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_7_changed, NULL);
	updating_percentage_entries = false;
}

GtkWidget* get_percentage_dialog(void) {
	if(!percentage_builder) {

		percentage_builder = getBuilder("percentage.ui");
		g_assert(percentage_builder != NULL);

		g_assert(gtk_builder_get_object(percentage_builder, "percentage_dialog") != NULL);

		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_1")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_2")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_3")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_4")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_5")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_6")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_7")), 1.0);

		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
		gtk_text_view_set_right_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
		gtk_text_view_set_top_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
		gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
#else
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
#endif

		gtk_builder_add_callback_symbols(percentage_builder, "on_percentage_button_calculate_clicked", G_CALLBACK(on_percentage_button_calculate_clicked), "on_percentage_button_clear_clicked", G_CALLBACK(on_percentage_button_clear_clicked), "on_percentage_entry_1_activate", G_CALLBACK(on_percentage_entry_1_activate), "on_percentage_entry_1_changed", G_CALLBACK(on_percentage_entry_1_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_percentage_entry_3_activate", G_CALLBACK(on_percentage_entry_3_activate), "on_percentage_entry_3_changed", G_CALLBACK(on_percentage_entry_3_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_percentage_entry_4_activate", G_CALLBACK(on_percentage_entry_4_activate), "on_percentage_entry_4_changed", G_CALLBACK(on_percentage_entry_4_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_percentage_entry_2_activate", G_CALLBACK(on_percentage_entry_2_activate), "on_percentage_entry_2_changed", G_CALLBACK(on_percentage_entry_2_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_percentage_entry_6_activate", G_CALLBACK(on_percentage_entry_6_activate), "on_percentage_entry_6_changed", G_CALLBACK(on_percentage_entry_6_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_percentage_entry_7_activate", G_CALLBACK(on_percentage_entry_7_activate), "on_percentage_entry_7_changed", G_CALLBACK(on_percentage_entry_7_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_percentage_entry_5_activate", G_CALLBACK(on_percentage_entry_5_activate), "on_percentage_entry_5_changed", G_CALLBACK(on_percentage_entry_5_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), NULL);
		gtk_builder_connect_signals(percentage_builder, NULL);

	}

	if(!enable_tooltips || toe_changed) set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_dialog")), enable_tooltips);
	if(always_on_top || aot_changed) gtk_window_set_keep_above(GTK_WINDOW(gtk_builder_get_object(percentage_builder, "percentage_dialog")), always_on_top);
	return GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_dialog"));
}

void show_percentage_dialog(GtkWindow *parent, const gchar *initial_expression) {
	GtkWidget *dialog = get_percentage_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	on_percentage_button_clear_clicked(NULL, NULL);
	if(initial_expression && strlen(initial_expression) > 0 && strcmp(initial_expression, "0") != 0) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_1")), initial_expression);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_1")));
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
