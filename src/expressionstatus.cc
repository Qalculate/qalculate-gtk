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
#include "mainwindow.h"
#include "historyview.h"
#include "resultview.h"
#include "preferencesdialog.h"
#include "expressionedit.h"
#include "expressionstatus.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

extern GtkBuilder *main_builder;

GtkWidget *statuslabel_l = NULL, *statuslabel_r = NULL;
GtkCssProvider *statuslabel_l_provider = NULL, *statuslabel_r_provider = NULL, *statusframe_provider = NULL;
PangoLayout *status_layout = NULL;
string status_error_c = "#FF0000", status_warning_c = "#0000FF";
bool status_error_c_set = false, status_warning_c_set = false;
bool use_custom_status_font = false, save_custom_status_font = false;
string custom_status_font;
string sdot_s, saltdot_s, sdiv_s, sslash_s, stimes_s, sminus_s;
bool fix_supsub_status = false;
bool expression_has_changed2 = false;
string parsed_expression, parsed_expression_tooltip;
bool parsed_had_errors = false, parsed_had_warnings = false;
MathStructure *current_from_struct = NULL;
vector<Unit*> current_from_units;
MathStructure current_status_struct, mwhere, mfunc;
vector<MathStructure> displayed_parsed_to;
size_t current_function_index = 0, current_function_index_true = 0;
MathFunction *current_function = NULL;
int block_display_parse = 0;
extern bool parsed_in_result;
extern vector<CalculatorMessage> autocalc_messages;

enum {
	STATUS_TEXT_NONE,
	STATUS_TEXT_FUNCTION,
	STATUS_TEXT_ERROR,
	STATUS_TEXT_PARSED,
	STATUS_TEXT_AUTOCALC,
	STATUS_TEXT_OTHER
};
int status_text_source = STATUS_TEXT_NONE;

GtkWidget *parse_status_widget() {
	if(!statuslabel_l) statuslabel_l = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_status_left"));
	return statuslabel_l;
}

bool read_expression_status_settings_line(string &svar, string &svalue, int &v) {
	 if(svar == "use_custom_status_font") {
		use_custom_status_font = v;
	} else if(svar == "custom_status_font") {
		custom_status_font = svalue;
		save_custom_status_font = true;
	} else if(svar == "status_error_color") {
		status_error_c = svalue;
		status_error_c_set = true;
	} else if(svar == "status_warning_color") {
		status_warning_c = svalue;
		status_warning_c_set = true;
	} else {
		return false;
	}
	return true;
}
void write_expression_status_settings(FILE *file) {
	fprintf(file, "use_custom_status_font=%i\n", use_custom_status_font);
	if(use_custom_status_font || save_custom_status_font) fprintf(file, "custom_status_font=%s\n", custom_status_font.c_str());
	if(status_error_c_set) fprintf(file, "status_error_color=%s\n", status_error_c.c_str());
	if(status_warning_c_set) fprintf(file, "status_warning_color=%s\n", status_warning_c.c_str());
}

MathStructure &current_parsed_expression() {
	return current_status_struct;
}
MathStructure &current_parsed_function_struct() {
	return mfunc;
}
void clear_parsed_expression() {
	current_status_struct.setAborted();
}
MathStructure &current_parsed_where() {
	return mwhere;
}
vector<MathStructure> &current_parsed_to() {
	return displayed_parsed_to;
}
const string &current_parsed_expression_text() {
	return parsed_expression;
}
MathFunction *current_parsed_function() {
	return current_function;
}
size_t current_parsed_function_index() {
	return current_function_index_true;
}

void block_status() {
	block_display_parse++;
}
void unblock_status() {
	block_display_parse--;
}
bool status_blocked() {
	return block_display_parse;
}

void on_menu_item_expression_status_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)) == display_expression_status) return;
	display_expression_status = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	if(display_expression_status) {
		display_parse_status();
	} else {
		set_parsed_in_result(false);
		clear_status_text();
	}
	preferences_update_expression_status();
}
void on_menu_item_status_degrees_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_angle_unit(ANGLE_UNIT_DEGREES);
}
void on_menu_item_status_radians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_angle_unit(ANGLE_UNIT_RADIANS);
}
void on_menu_item_status_gradians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_angle_unit(ANGLE_UNIT_GRADIANS);
}
void on_menu_item_parsed_in_result_activate(GtkMenuItem *w, gpointer) {
	set_parsed_in_result(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_status_exact_activate(GtkMenuItem *w, gpointer) {
	set_approximation(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)) ? APPROXIMATION_EXACT : APPROXIMATION_TRY_EXACT);
}
void on_menu_item_status_read_precision_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_status_rpn_syntax_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
}
void on_menu_item_status_chain_syntax_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), TRUE);
}
void on_menu_item_status_adaptive_parsing_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
}
void on_menu_item_status_ignore_whitespace_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
}
void on_menu_item_status_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
}
void on_menu_item_copy_status_activate(GtkMenuItem*, gpointer) {
	copy_result(0, status_text_source == STATUS_TEXT_AUTOCALC ? 0 : 8);
}
void on_menu_item_copy_ascii_status_activate(GtkMenuItem*, gpointer) {
	copy_result(1, status_text_source == STATUS_TEXT_AUTOCALC ? 0 : 8);
}

gboolean on_status_left_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button == 3 && !calculator_busy()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_parsed_in_result_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result")), parsed_in_result && !rpn_mode);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_parsed_in_result_activate, NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result")), display_expression_status && !rpn_mode);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_expression_status"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_expression_status_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_expression_status")), display_expression_status);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_expression_status"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_expression_status_activate, NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_copy_status")), status_text_source == STATUS_TEXT_AUTOCALC || status_text_source == STATUS_TEXT_PARSED);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_copy_ascii_status")), status_text_source == STATUS_TEXT_AUTOCALC || status_text_source == STATUS_TEXT_PARSED);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_left")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_left")), NULL, NULL, NULL, NULL, button, gdk_event_get_time((GdkEvent*) event));
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_status_right_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button == 1) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), NULL, NULL, NULL, NULL, button, gdk_event_get_time((GdkEvent*) event));
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_status_right_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button != 1) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), NULL, NULL, NULL, NULL, button, gdk_event_get_time((GdkEvent*) event));
#endif
		return TRUE;
	}
	return FALSE;
}

void update_status_approximation() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_exact_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), evalops.approximation == APPROXIMATION_EXACT);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_exact_activate, NULL);
}
void update_status_angle() {
	switch(evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_degrees_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_degrees")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_degrees_activate, NULL);
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_gradians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_gradians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_gradians_activate, NULL);
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_radians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_radians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_radians_activate, NULL);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_other")), TRUE);
			break;
		}
	}
}
void update_status_syntax() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_read_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_read_precision_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_read_precision")), evalops.parse_options.read_precision != DONT_READ_PRECISION);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_read_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_read_precision_activate, NULL);
	switch(evalops.parse_options.parsing_mode) {
		case PARSING_MODE_RPN: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_rpn_syntax_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_rpn_syntax_activate, NULL);
			break;
		}
		case PARSING_MODE_ADAPTIVE: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_adaptive_parsing_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_adaptive_parsing_activate, NULL);
			break;
		}
		case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_ignore_whitespace_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_ignore_whitespace_activate, NULL);
			break;
		}
		case PARSING_MODE_CONVENTIONAL: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_no_special_implicit_multiplication_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_no_special_implicit_multiplication_activate, NULL);
			break;
		}
		case PARSING_MODE_CHAIN: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_chain_syntax_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_chain_syntax_activate, NULL);
			break;
		}
	}
}

#define STATUS_SPACE	if(b) str += "  "; else b = true;

bool status_selection_set = false;
void set_status_text(string text, bool break_begin = false, bool had_errors = false, bool had_warnings = false, string tooltip_text = "") {

	string str;
	if(had_errors) {
		str = "<span foreground=\"";
		str += status_error_c;
		str += "\">";
	} else if(had_warnings) {
		str = "<span foreground=\"";
		str += status_warning_c;
		str += "\">";
	}
	if(text.empty()) str += " ";
	else str += text;
	if(had_errors || had_warnings) str += "</span>";
	if(text.empty()) status_text_source = STATUS_TEXT_NONE;

	if(break_begin) gtk_label_set_ellipsize(GTK_LABEL(parse_status_widget()), PANGO_ELLIPSIZE_START);
	else gtk_label_set_ellipsize(GTK_LABEL(parse_status_widget()), PANGO_ELLIPSIZE_END);

	gtk_label_set_markup(GTK_LABEL(parse_status_widget()), str.c_str());
	gint w = 0;
	if(unformatted_length(str) > 500) {
		w = -1;
	} else if(unformatted_length(str) > 20) {
		if(!status_layout) status_layout = gtk_widget_create_pango_layout(parse_status_widget(), "");
		pango_layout_set_markup(status_layout, str.c_str(), -1);
		pango_layout_get_pixel_size(status_layout, &w, NULL);
	}
	if(((auto_calculate && !rpn_mode) || !had_errors || tooltip_text.empty()) && (w < 0 || w > gtk_widget_get_allocated_width(parse_status_widget())) && unformatted_length(text) < 5000) gtk_widget_set_tooltip_markup(parse_status_widget(), text.c_str());
	else gtk_widget_set_tooltip_text(parse_status_widget(), tooltip_text.c_str());
	status_selection_set = false;
}
void clear_status_text() {
	set_status_text("");
}
void set_status_selection_text(const std::string &str, bool had_errors, bool had_warnings) {
	set_status_text(str, false, had_errors, had_warnings);
	status_selection_set = true;
}
void clear_status_selection_text() {
	if(status_selection_set) {
		int bdp_bak = block_display_parse;
		block_display_parse = 0;
		display_parse_status();
		block_display_parse = bdp_bak;
		status_selection_set = false;
	}
}
void update_status_text() {

	string str = "<span size=\"small\">";

	bool b = false;
	if(evalops.approximation == APPROXIMATION_EXACT) {
		STATUS_SPACE
		str += _("EXACT");
	} else if(evalops.approximation == APPROXIMATION_APPROXIMATE) {
		STATUS_SPACE
		str += _("APPROX");
	}
	if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
		STATUS_SPACE
		str += _("RPN");
	}
	if(evalops.parse_options.parsing_mode == PARSING_MODE_CHAIN) {
		STATUS_SPACE
		// Chain mode
		str += _("CHN");
	}
	switch(evalops.parse_options.base) {
		case BASE_DECIMAL: {
			break;
		}
		case BASE_BINARY: {
			STATUS_SPACE
			str += _("BIN");
			break;
		}
		case BASE_OCTAL: {
			STATUS_SPACE
			str += _("OCT");
			break;
		}
		case 12: {
			STATUS_SPACE
			str += _("DUO");
			break;
		}
		case BASE_HEXADECIMAL: {
			STATUS_SPACE
			str += _("HEX");
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			STATUS_SPACE
			str += _("ROMAN");
			break;
		}
		case BASE_BIJECTIVE_26: {
			STATUS_SPACE
			str += "B26";
			break;
		}
		case BASE_BINARY_DECIMAL: {
			STATUS_SPACE
			str += "BCD";
			break;
		}
		case BASE_CUSTOM: {
			STATUS_SPACE
			str += CALCULATOR->customInputBase().print(CALCULATOR->messagePrintOptions());
			break;
		}
		case BASE_GOLDEN_RATIO: {
			STATUS_SPACE
			str += "φ";
			break;
		}
		case BASE_SUPER_GOLDEN_RATIO: {
			STATUS_SPACE
			str += "ψ";
			break;
		}
		case BASE_PI: {
			STATUS_SPACE
			str += "π";
			break;
		}
		case BASE_E: {
			STATUS_SPACE
			str += "e";
			break;
		}
		case BASE_SQRT2: {
			STATUS_SPACE
			str += "√2";
			break;
		}
		case BASE_UNICODE: {
			STATUS_SPACE
			str += "UNICODE";
			break;
		}
		default: {
			STATUS_SPACE
			str += i2s(evalops.parse_options.base);
			break;
		}
	}
	switch (evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			STATUS_SPACE
			str += _("DEG");
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			STATUS_SPACE
			str += _("RAD");
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			STATUS_SPACE
			str += _("GRA");
			break;
		}
		default: {}
	}
	if(evalops.parse_options.read_precision != DONT_READ_PRECISION) {
		STATUS_SPACE
		str += _("PREC");
	}
	if(!evalops.parse_options.functions_enabled) {
		STATUS_SPACE
		str += "<s>";
		str += _("FUNC");
		str += "</s>";
	}
	if(!evalops.parse_options.units_enabled) {
		STATUS_SPACE
		str += "<s>";
		str += _("UNIT");
		str += "</s>";
	}
	if(!evalops.parse_options.variables_enabled) {
		STATUS_SPACE
		str += "<s>";
		str += _("VAR");
		str += "</s>";
	}
	if(!evalops.allow_infinite) {
		STATUS_SPACE
		str += "<s>";
		str += _("INF");
		str += "</s>";
	}
	if(!evalops.allow_complex) {
		STATUS_SPACE
		str += "<s>";
		str += _("CPLX");
		str += "</s>";
	}

	remove_blank_ends(str);
	if(!b) str += " ";

	str += "</span>";

	if(str != gtk_label_get_label(GTK_LABEL(statuslabel_r))) {
		gtk_label_set_text(GTK_LABEL(parse_status_widget()), "");
		gtk_label_set_markup(GTK_LABEL(statuslabel_r), str.c_str());
		display_parse_status();
	}

}
bool display_function_hint(MathFunction *f, int arg_index = 1) {
	if(!f) return false;
	int iargs = f->maxargs();
	Argument *arg;
	Argument default_arg;
	string str, str2, str3;
	const ExpressionName *ename = &f->preferredName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) parse_status_widget());
	bool last_is_vctr = f->getArgumentDefinition(iargs) && f->getArgumentDefinition(iargs)->type() == ARGUMENT_TYPE_VECTOR;
	if(arg_index > iargs && iargs >= 0 && !last_is_vctr) {
		if(iargs == 1 && f->getArgumentDefinition(1) && f->getArgumentDefinition(1)->handlesVector()) {
			return false;
		}
		gchar *gstr = g_strdup_printf(_("Too many arguments for %s()."), ename->formattedName(TYPE_FUNCTION, true, true).c_str());
		set_status_text(gstr, false, false, true);
		g_free(gstr);
		status_text_source = STATUS_TEXT_FUNCTION;
		return true;
	}
	str += ename->formattedName(TYPE_FUNCTION, true, true);
	if(iargs < 0) {
		iargs = f->minargs() + 1;
		if((int) f->lastArgumentDefinitionIndex() > iargs) iargs = (int) f->lastArgumentDefinitionIndex();
		if(arg_index > iargs) arg_index = iargs;
	}
	if(arg_index > iargs && last_is_vctr) arg_index = iargs;
	str += "(";
	int i_reduced = 0;
	if(iargs != 0) {
		for(int i2 = 1; i2 <= iargs; i2++) {
			if(i2 > f->minargs() && arg_index < i2) {
				str += "[";
			}
			if(i2 > 1) {
				str += CALCULATOR->getComma();
				str += " ";
			}
			if(i2 == arg_index) str += "<b>";
			arg = f->getArgumentDefinition(i2);
			if(arg && !arg->name().empty()) {
				str2 = arg->name();
			} else {
				str2 = _("argument");
				if(i2 > 1 || f->maxargs() != 1) {
					str2 += " ";
					str2 += i2s(i2);
				}
			}
			if(i2 == arg_index) {
				if(arg) {
					if(i_reduced == 2) str3 = arg->print();
					else str3 = arg->printlong();
				} else {
					Argument arg_default;
					if(i_reduced == 2) str3 = arg_default.print();
					else str3 = arg_default.printlong();
				}
				if(i_reduced != 2 && printops.use_unicode_signs) {
					gsub(">=", SIGN_GREATER_OR_EQUAL, str3);
					gsub("<=", SIGN_LESS_OR_EQUAL, str3);
					gsub("!=", SIGN_NOT_EQUAL, str3);
				}
				if(!str3.empty()) {
					str2 += ": ";
					str2 += str3;
				}
				gsub("&", "&amp;", str2);
				gsub(">", "&gt;", str2);
				gsub("<", "&lt;", str2);
				str += str2;
				str += "</b>";
				if(i_reduced < 2) {
					PangoLayout *layout_test = gtk_widget_create_pango_layout(parse_status_widget(), NULL);
					pango_layout_set_markup(layout_test, str.c_str(), -1);
					gint w, h;
					pango_layout_get_pixel_size(layout_test, &w, &h);
					if(w > gtk_widget_get_allocated_width(parse_status_widget()) - 20) {
						str = ename->formattedName(TYPE_FUNCTION, true, true);
						str += "(";
						if(i2 != 1) {
							str += "…";
							i_reduced++;
						} else {
							i_reduced = 2;
						}
						i2--;
					}
					g_object_unref(layout_test);
				} else {
					i_reduced = 0;
				}
			} else {
				gsub("&", "&amp;", str2);
				gsub(">", "&gt;", str2);
				gsub("<", "&lt;", str2);
				str += str2;
				if(i2 > f->minargs() && arg_index < i2) {
					str += "]";
				}
			}
		}
		if(f->maxargs() < 0) {
			str += CALCULATOR->getComma();
			str += " …";
		}
	}
	str += ")";
	set_status_text(str);
	status_text_source = STATUS_TEXT_FUNCTION;
	return true;
}

void set_expression_output_updated(bool b) {
	expression_has_changed2 = b;
}
bool expression_output_updated() {
	return expression_has_changed2;
}

void replace_control_characters_gtk(string &str) {
	for(size_t i = 0; i < str.size();) {
		if((str[i] > 0 && str[i] < 9) || ((str[i] > 13 && str[i] < 32) && str[i] != '\e')) {
			str.erase(i, 1);
		} else {
			i++;
		}
	}
}

bool parse_status_error() {
	return display_expression_status && parsed_had_errors;
}

void display_parse_status() {
	current_function = NULL;
	mfunc.clear();
	if(expression_output_updated()) current_status_struct.setAborted();
	if(!display_expression_status) return;
	if(block_display_parse) return;
	GtkTextIter istart, iend, ipos;
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
	gchar *gtext = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
	string text = gtext, str_f;
	g_free(gtext);
	bool double_tag = false;
	string to_str = CALCULATOR->parseComments(text, evalops.parse_options, &double_tag);
	if(!to_str.empty() && text.empty() && double_tag) {
		text = CALCULATOR->f_message->referenceName();
		text += "(";
		if(to_str.find("\"") == string::npos) {text += "\""; text += to_str; text += "\"";}
		else if(to_str.find("\'") == string::npos) {text += "\'"; text += to_str; text += "\'";}
		else text += to_str;
		text += ")";
	}
	if(text.empty()) {
		set_status_text("", true, false, false);
		parsed_expression = "";
		parsed_expression_tooltip = "";
		set_expression_output_updated(false);
		clear_parsed_in_result();
		return;
	}
	if(text[0] == '/' && text.length() > 1) {
		size_t i = text.find_first_not_of(SPACES, 1);
		if(i != string::npos && (signed char) text[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, text[i])) {
			set_status_text("qalc command", true, false, false);
			status_text_source = STATUS_TEXT_OTHER;
			clear_parsed_in_result();
			return;
		}
	} else if(text == "MC") {
		set_status_text(_("MC (memory clear)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		clear_parsed_in_result();
		return;
	} else if(text == "MS") {
		set_status_text(_("MS (memory store)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		clear_parsed_in_result();
		return;
	} else if(text == "M+") {
		set_status_text(_("M+ (memory plus)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		clear_parsed_in_result();
		return;
	} else if(text == "M-" || text == "M−") {
		set_status_text(_("M− (memory minus)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		clear_parsed_in_result();
		return;
	}
	gsub(ID_WRAP_LEFT, LEFT_PARENTHESIS, text);
	gsub(ID_WRAP_RIGHT, RIGHT_PARENTHESIS, text);
	replace_control_characters_gtk(text);
	remove_duplicate_blanks(text);
	size_t i = text.find_first_of(SPACES);
	if(i != string::npos) {
		str_f = text.substr(0, i);
		if(str_f == "factor" || equalsIgnoreCase(str_f, "factorize") || equalsIgnoreCase(str_f, _("factorize"))) {
			text = text.substr(i + 1);
			str_f = _("factorize");
		} else if(equalsIgnoreCase(str_f, "expand") || equalsIgnoreCase(str_f, _("expand"))) {
			text = text.substr(i + 1);
			str_f = _("expand");
		} else {
			str_f = "";
		}
	}
	GtkTextMark *mark = gtk_text_buffer_get_insert(expression_edit_buffer());
	if(mark) gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mark);
	else ipos = iend;
	MathStructure mparse;
	bool full_parsed = false;
	string str_e, str_u, str_w;
	bool had_errors = false, had_warnings = false;
	if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	evalops.parse_options.preserve_format = true;
	if(!error_blocked()) display_errors();
	block_error();
	if(!gtk_text_iter_is_start(&ipos)) {
		evalops.parse_options.unended_function = &mfunc;
		if(!gtk_text_iter_is_end(&ipos)) {
			if(current_from_struct) {current_from_struct->unref(); current_from_struct = NULL; current_from_units.clear();}
			gtext = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &ipos, FALSE);
			str_e = CALCULATOR->unlocalizeExpression(gtext, evalops.parse_options);
			bool b = CALCULATOR->separateToExpression(str_e, str_u, evalops, false, !auto_calculate || rpn_mode || parsed_in_result);
			b = CALCULATOR->separateWhereExpression(str_e, str_w, evalops) || b;
			if(!b) {
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
				CALCULATOR->endTemporaryStopMessages();
			}
			g_free(gtext);
		} else {
			str_e = CALCULATOR->unlocalizeExpression(text, evalops.parse_options);
			transform_expression_for_equals_save(str_e, evalops.parse_options);
			bool b = CALCULATOR->separateToExpression(str_e, str_u, evalops, false, !auto_calculate || rpn_mode || parsed_in_result);
			b = CALCULATOR->separateWhereExpression(str_e, str_w, evalops) || b;
			if(!b) {
				CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
				if(current_from_struct) {current_from_struct->unref(); current_from_struct = NULL; current_from_units.clear();}
				full_parsed = true;
			}
		}
		evalops.parse_options.unended_function = NULL;
	}
	bool b_func = false;
	if(mfunc.isFunction()) {
		current_function = mfunc.function();
		current_function_index_true = mfunc.countChildren();
		if(mfunc.countChildren() == 0) {
			current_function_index = 1;
			b_func = display_function_hint(mfunc.function(), 1);
		} else {
			current_function_index = mfunc.countChildren();
			b_func = display_function_hint(mfunc.function(), mfunc.countChildren());
			if(mfunc.last().isZero()) {
				size_t i = str_e.find_last_not_of(SPACES);
				if(i != string::npos && (str_e[i] == COMMA_CH || str_e[i] == ';')) current_function_index_true--;
			}
		}
	}
	if(expression_output_updated()) {
		bool last_is_space = false;
		parsed_expression_tooltip = "";
		if(!full_parsed) {
			str_e = CALCULATOR->unlocalizeExpression(text, evalops.parse_options);
			transform_expression_for_equals_save(str_e, evalops.parse_options);
			last_is_space = is_in(SPACES, str_e[str_e.length() - 1]);
			bool b_to = CALCULATOR->separateToExpression(str_e, str_u, evalops, true, !auto_calculate || rpn_mode || parsed_in_result);
			CALCULATOR->separateWhereExpression(str_e, str_w, evalops);
			if(!str_e.empty()) CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
			if(b_to && !str_e.empty()) {
				if(!current_from_struct && test_autocalculatable(mparse)) {
					current_from_struct = new MathStructure;
					EvaluationOptions eo = evalops;
					eo.structuring = STRUCTURING_NONE;
					eo.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
					eo.auto_post_conversion = POST_CONVERSION_NONE;
					eo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					eo.expand = -2;
					if(!CALCULATOR->calculate(current_from_struct, str_w.empty() ? str_e : str_e + "/." + str_w, 20, eo)) current_from_struct->setAborted();
					find_matching_units(*current_from_struct, &mparse, current_from_units);
				}
			} else if(current_from_struct) {
				current_from_struct->unref();
				current_from_struct = NULL;
				current_from_units.clear();
			}
		}
		current_status_struct = mparse;
		PrintOptions po;
		po.preserve_format = true;
		po.show_ending_zeroes = evalops.parse_options.read_precision != DONT_READ_PRECISION && !CALCULATOR->usesIntervalArithmetic() && evalops.parse_options.base > BASE_CUSTOM;
		po.exp_display = printops.exp_display;
		po.lower_case_numbers = printops.lower_case_numbers;
		po.base_display = printops.base_display;
		po.twos_complement = printops.twos_complement;
		po.hexadecimal_twos_complement = printops.hexadecimal_twos_complement;
		po.round_halfway_to_even = printops.round_halfway_to_even;
		po.base = evalops.parse_options.base;
		Number nr_base;
		if(po.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
			nr_base = CALCULATOR->customOutputBase();
			CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
		} else if(po.base == BASE_CUSTOM || (po.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26 && po.base != BASE_BINARY_DECIMAL)) {
			po.base = 10;
			po.min_exp = 6;
			po.use_max_decimals = true;
			po.max_decimals = 5;
			po.preserve_format = false;
		}
		po.abbreviate_names = false;
		po.hide_underscore_spaces = true;
		po.use_unicode_signs = printops.use_unicode_signs;
		po.digit_grouping = printops.digit_grouping;
		po.multiplication_sign = printops.multiplication_sign;
		po.division_sign = printops.division_sign;
		po.short_multiplication = false;
		po.excessive_parenthesis = true;
		po.improve_division_multipliers = false;
		po.can_display_unicode_string_function = &can_display_unicode_string_function;
		po.can_display_unicode_string_arg = (void*) parse_status_widget();
		po.spell_out_logical_operators = printops.spell_out_logical_operators;
		po.restrict_to_parent_precision = false;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		size_t parse_l = 0;
		mwhere.clear();
		if(!str_w.empty()) {
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->parseExpressionAndWhere(&mparse, &mwhere, str_e, str_w, evalops.parse_options);
			mparse.format(po);
			parsed_expression = mparse.print(po, true, false, TAG_TYPE_HTML);
			parse_l = parsed_expression.length();
			parsed_expression += CALCULATOR->localWhereString();
			mwhere.format(po);
			parsed_expression += mwhere.print(po, true, false, TAG_TYPE_HTML);
			CALCULATOR->endTemporaryStopMessages();
		} else if(str_e.empty()) {
			parsed_expression = "";
		} else {
			CALCULATOR->beginTemporaryStopMessages();
			mparse.format(po);
			parsed_expression = mparse.print(po, true, false, TAG_TYPE_HTML);
			parse_l = parsed_expression.length();
			CALCULATOR->endTemporaryStopMessages();
		}
		displayed_parsed_to.clear();
		if(!str_u.empty()) {
			string str_u2;
			bool had_to_conv = false;
			MathStructure *mparse2 = NULL;
			while(true) {
				bool unit_struct = false;
				if(last_is_space) str_u += " ";
				if(!str_e.empty()) CALCULATOR->separateToExpression(str_u, str_u2, evalops, true, false);
				remove_blank_ends(str_u);
				if(parsed_expression.empty()) {
					parsed_expression += CALCULATOR->localToString(false);
					parsed_expression += " ";
				} else {
					parsed_expression += CALCULATOR->localToString();
				}
				size_t to_begin = parsed_expression.length();
				remove_duplicate_blanks(str_u);
				string to_str1, to_str2;
				size_t ispace = str_u.find_first_of(SPACES);
				if(ispace != string::npos) {
					to_str1 = str_u.substr(0, ispace);
					remove_blank_ends(to_str1);
					to_str2 = str_u.substr(ispace + 1);
					remove_blank_ends(to_str2);
				}
				if(equalsIgnoreCase(str_u, "hex") || equalsIgnoreCase(str_u, "hexadecimal") || equalsIgnoreCase(str_u, _("hexadecimal"))) {
					parsed_expression += _("hexadecimal number");
				} else if(equalsIgnoreCase(str_u, "oct") || equalsIgnoreCase(str_u, "octal") || equalsIgnoreCase(str_u, _("octal"))) {
					parsed_expression += _("octal number");
				} else if(equalsIgnoreCase(str_u, "dec") || equalsIgnoreCase(str_u, "decimal") || equalsIgnoreCase(str_u, _("decimal"))) {
					parsed_expression += _("decimal number");
				} else if(equalsIgnoreCase(str_u, "duo") || equalsIgnoreCase(str_u, "duodecimal") || equalsIgnoreCase(str_u, _("duodecimal"))) {
					parsed_expression += _("duodecimal number");
				} else if(equalsIgnoreCase(str_u, "doz") || equalsIgnoreCase(str_u, "dozenal")) {
					parsed_expression += _("duodecimal number");
				} else if(equalsIgnoreCase(str_u, "bin") || equalsIgnoreCase(str_u, "binary") || equalsIgnoreCase(str_u, _("binary"))) {
					parsed_expression += _("binary number");
				} else if(equalsIgnoreCase(str_u, "roman") || equalsIgnoreCase(str_u, _("roman"))) {
					parsed_expression += _("roman numerals");
				} else if(equalsIgnoreCase(str_u, "bijective") || equalsIgnoreCase(str_u, _("bijective"))) {
					parsed_expression += _("bijective base-26");
				} else if(equalsIgnoreCase(str_u, "bcd")) {
					parsed_expression += _("binary-coded decimal");
				} else if(equalsIgnoreCase(str_u, "sexa") || equalsIgnoreCase(str_u, "sexa2") || equalsIgnoreCase(str_u, "sexa3") || equalsIgnoreCase(str_u, "sexagesimal") || equalsIgnoreCase(str_u, _("sexagesimal")) || EQUALS_IGNORECASE_AND_LOCAL_NR(str_u, "sexagesimal", _("sexagesimal"), "2") || EQUALS_IGNORECASE_AND_LOCAL_NR(str_u, "sexagesimal", _("sexagesimal"), "3")) {
					parsed_expression += _("sexagesimal number");
				} else if(equalsIgnoreCase(str_u, "latitude") || equalsIgnoreCase(str_u, _("latitude")) || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", _("latitude"), "2")) {
					parsed_expression += _("latitude");
				} else if(equalsIgnoreCase(str_u, "longitude") || equalsIgnoreCase(str_u, _("longitude")) || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", _("longitude"), "2")) {
					parsed_expression += _("longitude");
				} else if(equalsIgnoreCase(str_u, "fp32") || equalsIgnoreCase(str_u, "binary32") || equalsIgnoreCase(str_u, "float")) {
					parsed_expression += _("32-bit floating point");
				} else if(equalsIgnoreCase(str_u, "fp64") || equalsIgnoreCase(str_u, "binary64") || equalsIgnoreCase(str_u, "double")) {
					parsed_expression += _("64-bit floating point");
				} else if(equalsIgnoreCase(str_u, "fp16") || equalsIgnoreCase(str_u, "binary16")) {
					parsed_expression += _("16-bit floating point");
				} else if(equalsIgnoreCase(str_u, "fp80")) {
					parsed_expression += _("80-bit (x86) floating point");
				} else if(equalsIgnoreCase(str_u, "fp128") || equalsIgnoreCase(str_u, "binary128")) {
					parsed_expression += _("128-bit floating point");
				} else if(equalsIgnoreCase(str_u, "time") || equalsIgnoreCase(str_u, _("time"))) {
					parsed_expression += _("time format");
				} else if(equalsIgnoreCase(str_u, "unicode")) {
					parsed_expression += _("Unicode");
				} else if(equalsIgnoreCase(str_u, "sci") || EQUALS_IGNORECASE_AND_LOCAL(str_u, "scientific", _("scientific"))) {
					parsed_expression += _("scientific notation");
				} else if(equalsIgnoreCase(str_u, "eng") || EQUALS_IGNORECASE_AND_LOCAL(str_u, "engineering", _("engineering"))) {
					parsed_expression += _("engineering notation");
				} else if(EQUALS_IGNORECASE_AND_LOCAL(str_u, "simple", _("simple"))) {
					parsed_expression += _("simple notation");
				} else if(equalsIgnoreCase(str_u, "bases") || equalsIgnoreCase(str_u, _("bases"))) {
					parsed_expression += _("number bases");
				} else if(equalsIgnoreCase(str_u, "calendars") || equalsIgnoreCase(str_u, _("calendars"))) {
					parsed_expression += _("calendars");
				} else if(equalsIgnoreCase(str_u, "optimal") || equalsIgnoreCase(str_u, _("optimal"))) {
					parsed_expression += _("optimal unit");
				} else if(equalsIgnoreCase(str_u, "prefix") || equalsIgnoreCase(str_u, _("prefix")) || str_u == "?" || (str_u.length() == 2 && str_u[1] == '?' && (str_u[0] == 'b' || str_u[0] == 'a' || str_u[0] == 'd'))) {
					parsed_expression += _("optimal prefix");
				} else if(equalsIgnoreCase(str_u, "base") || equalsIgnoreCase(str_u, _c("Units", "base"))) {
					parsed_expression += _("base units");
				} else if(equalsIgnoreCase(str_u, "mixed") || equalsIgnoreCase(str_u, _("mixed"))) {
					parsed_expression += _("mixed units");
				} else if(equalsIgnoreCase(str_u, "factors") || equalsIgnoreCase(str_u, _("factors")) || equalsIgnoreCase(str_u, "factor")) {
					parsed_expression += _("factors");
				} else if(equalsIgnoreCase(str_u, "partial fraction") || equalsIgnoreCase(str_u, _("partial fraction"))) {
					parsed_expression += _("expanded partial fractions");
				} else if(equalsIgnoreCase(str_u, "rectangular") || equalsIgnoreCase(str_u, "cartesian") || equalsIgnoreCase(str_u, _("rectangular")) || equalsIgnoreCase(str_u, _("cartesian"))) {
					parsed_expression += _("complex rectangular form");
				} else if(equalsIgnoreCase(str_u, "exponential") || equalsIgnoreCase(str_u, _("exponential"))) {
					parsed_expression += _("complex exponential form");
				} else if(equalsIgnoreCase(str_u, "polar") || equalsIgnoreCase(str_u, _("polar"))) {
					parsed_expression += _("complex polar form");
				} else if(str_u == "cis") {
					parsed_expression += _("complex cis form");
				} else if(equalsIgnoreCase(str_u, "angle") || equalsIgnoreCase(str_u, _("angle"))) {
					parsed_expression += _("complex angle notation");
				} else if(equalsIgnoreCase(str_u, "phasor") || equalsIgnoreCase(str_u, _("phasor"))) {
					parsed_expression += _("complex phasor notation");
				} else if(equalsIgnoreCase(str_u, "utc") || equalsIgnoreCase(str_u, "gmt")) {
					parsed_expression += _("UTC time zone");
				} else if(str_u.length() > 3 && (equalsIgnoreCase(str_u.substr(0, 3), "utc") || equalsIgnoreCase(str_u.substr(0, 3), "gmt"))) {
					str_u = str_u.substr(3);
					parsed_expression += "UTC";
					remove_blanks(str_u);
					bool b_minus = false;
					if(str_u[0] == '+') {
						str_u.erase(0, 1);
					} else if(str_u[0] == '-') {
						b_minus = true;
						str_u.erase(0, 1);
					} else if(str_u.find(SIGN_MINUS) == 0) {
						b_minus = true;
						str_u.erase(0, strlen(SIGN_MINUS));
					}
					unsigned int tzh = 0, tzm = 0;
					int itz = 0;
					if(!str_u.empty() && sscanf(str_u.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
						itz = tzh * 60 + tzm;
					} else {
						had_errors = true;
					}
					if(itz > 0) {
						if(b_minus) parsed_expression += '-';
						else parsed_expression += '+';
						if(itz < 60) {
							parsed_expression += "00";
						} else {
							if(itz < 60 * 10) parsed_expression += '0';
							parsed_expression += i2s(itz / 60);
						}
						if(itz % 60 > 0) {
							parsed_expression += ":";
							if(itz % 60 < 10) parsed_expression += '0';
							parsed_expression += i2s(itz % 60);
						}
					}
				} else if(str_u.length() > 3 && equalsIgnoreCase(str_u.substr(0, 3), "bin") && is_in(NUMBERS, str_u[3])) {
					unsigned int bits = s2i(str_u.substr(3));
					if(bits > 4096) bits = 4096;
					parsed_expression += i2s(bits);
					parsed_expression += "-bit ";
					parsed_expression += _("binary number");
				} else if(str_u.length() > 3 && equalsIgnoreCase(str_u.substr(0, 3), "hex") && is_in(NUMBERS, str_u[3])) {
					unsigned int bits = s2i(str_u.substr(3));
					if(bits > 4096) bits = 4096;
					parsed_expression += i2s(bits);
					parsed_expression += "-bit ";
					parsed_expression += _("hexadecimal number");
				} else if(str_u == "CET") {
					parsed_expression += "UTC";
					parsed_expression += "+01";
				} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, _c("Number base", "base"))) {
					gchar *gstr = g_strdup_printf(_("number base %s"), to_str2.c_str());
					parsed_expression += gstr;
					g_free(gstr);

				} else if(equalsIgnoreCase(str_u, "decimals") || equalsIgnoreCase(str_u, _("decimals"))) {
					parsed_expression += _("decimal fraction");
				} else {
					int tofr = 0;
					long int fden = get_fixed_denominator_gtk(unlocalize_expression(str_u), tofr);
					if(fden > 0) {
						parsed_expression += _("fraction");
						parsed_expression += " (";
						parsed_expression += "1/";
						parsed_expression += i2s(fden);
						parsed_expression += ")";
					} else if(fden < 0) {
						parsed_expression += _("fraction");
					} else {
						if(str_u[0] == '0' || str_u[0] == '?' || str_u[0] == '+' || str_u[0] == '-') {
							str_u = str_u.substr(1, str_u.length() - 1);
							remove_blank_ends(str_u);
						} else if(str_u.length() > 1 && str_u[1] == '?' && (str_u[0] == 'b' || str_u[0] == 'a' || str_u[0] == 'd')) {
							str_u = str_u.substr(2, str_u.length() - 2);
							remove_blank_ends(str_u);
						}
						MathStructure mparse_to;
						Unit *u = CALCULATOR->getActiveUnit(str_u);
						if(!u) u = CALCULATOR->getCompositeUnit(str_u);
						Variable *v = NULL;
						if(!u) v = CALCULATOR->getActiveVariable(str_u);
						if(v && !v->isKnown()) v = NULL;
						Prefix *p = NULL;
						if(!u && !v && CALCULATOR->unitNameIsValid(str_u)) p = CALCULATOR->getPrefix(str_u);
						CALCULATOR->startControl(100);
						if(u) {
							mparse_to = u;
							if(!had_to_conv && !str_e.empty()) {
								CALCULATOR->beginTemporaryStopMessages();
								MathStructure to_struct = get_units_for_parsed_expression(&mparse, u, evalops, current_from_struct && !current_from_struct->isAborted() ? current_from_struct : NULL, str_e);
								if(!to_struct.isZero()) {
									mparse2 = new MathStructure();
									CALCULATOR->parse(mparse2, str_e, evalops.parse_options);
									po.preserve_format = false;
									to_struct.format(po);
									po.preserve_format = true;
									if(to_struct.isMultiplication() && to_struct.size() >= 2) {
										if(to_struct[0].isOne()) to_struct.delChild(1, true);
										else if(to_struct[1].isOne()) to_struct.delChild(2, true);
									}
									mparse2->multiply(to_struct);
								}
								CALCULATOR->endTemporaryStopMessages();
							}
						} else if(v) {
							mparse_to = v;
						} else if(!p) {
							CALCULATOR->beginTemporaryStopMessages();
							CompositeUnit cu("", evalops.parse_options.limit_implicit_multiplication ? "01" : "00", "", str_u);
							int i_warn = 0, i_error = CALCULATOR->endTemporaryStopMessages(NULL, &i_warn);
							if(!had_to_conv && cu.countUnits() > 0 && !str_e.empty()) {
								CALCULATOR->beginTemporaryStopMessages();
								MathStructure to_struct = get_units_for_parsed_expression(&mparse, &cu, evalops, current_from_struct && !current_from_struct->isAborted() ? current_from_struct : NULL, str_e);
								if(!to_struct.isZero()) {
									mparse2 = new MathStructure();
									CALCULATOR->parse(mparse2, str_e, evalops.parse_options);
									po.preserve_format = false;
									to_struct.format(po);
									po.preserve_format = true;
									if(to_struct.isMultiplication() && to_struct.size() >= 2) {
										if(to_struct[0].isOne()) to_struct.delChild(1, true);
										else if(to_struct[1].isOne()) to_struct.delChild(2, true);
									}
									mparse2->multiply(to_struct);
								}
								CALCULATOR->endTemporaryStopMessages();
							}
							if(i_error) {
								ParseOptions pa = evalops.parse_options;
								pa.units_enabled = true;
								CALCULATOR->parse(&mparse_to, str_u, pa);
							} else {
								if(i_warn > 0) had_warnings = true;
								mparse_to = cu.generateMathStructure(true);
							}
							mparse_to.format(po);
						}
						CALCULATOR->stopControl();
						if(p) {
							parsed_expression += p->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, false, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).formattedName(-1, true, TAG_TYPE_HTML, 0, true, po.hide_underscore_spaces);
						} else {
							CALCULATOR->beginTemporaryStopMessages();
							parsed_expression += mparse_to.print(po, true, false, TAG_TYPE_HTML);
							CALCULATOR->endTemporaryStopMessages();
							if(parsed_in_result || show_parsed_instead_of_result) displayed_parsed_to.push_back(mparse_to);
							unit_struct = true;
						}
						had_to_conv = true;
					}
					if((parsed_in_result || show_parsed_instead_of_result) && !unit_struct && to_begin < parsed_expression.length()) {
						displayed_parsed_to.push_back(MathStructure(string("to expression:") + parsed_expression.substr(to_begin), true));
					}
				}
				if(str_u2.empty()) break;
				str_u = str_u2;
			}
			if(mparse2) {
				mparse2->format(po);
				parsed_expression.replace(0, parse_l, mparse2->print(po, true, false, TAG_TYPE_HTML));
				if(parsed_in_result || show_parsed_instead_of_result) mparse.set_nocopy(*mparse2);
				mparse2->unref();
			}
		}
		if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
		size_t message_n = 0;
		if(result_is_autocalculated() && ((parsed_in_result && !rpn_mode && !current_result_text().empty()) || show_parsed_instead_of_result)) {
			CALCULATOR->clearMessages();
			for(size_t i = 0; i < autocalc_messages.size(); i++) {
				MessageType mtype = autocalc_messages[i].type();
				if((mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) && (!implicit_question_asked || autocalc_messages[i].category() != MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION)) {
					if(mtype == MESSAGE_ERROR) had_errors = true;
					else had_warnings = true;
					if(message_n > 0) {
						if(message_n == 1) parsed_expression_tooltip = "• " + parsed_expression_tooltip;
						parsed_expression_tooltip += "\n• ";
					}
					parsed_expression_tooltip += autocalc_messages[i].message();
					message_n++;
				}
			}
		} else {
			while(CALCULATOR->message()) {
				MessageType mtype = CALCULATOR->message()->type();
				if((mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) && (!implicit_question_asked || CALCULATOR->message()->category() != MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION)) {
					if(mtype == MESSAGE_ERROR) had_errors = true;
					else had_warnings = true;
					if(message_n > 0) {
						if(message_n == 1) parsed_expression_tooltip = "• " + parsed_expression_tooltip;
						parsed_expression_tooltip += "\n• ";
					}
					parsed_expression_tooltip += CALCULATOR->message()->message();
					message_n++;
				}
				CALCULATOR->nextMessage();
			}
		}
		unblock_error();
		parsed_had_errors = had_errors; parsed_had_warnings = had_warnings;
		if(!str_f.empty()) {str_f += " "; parsed_expression.insert(0, str_f);}
		fix_history_string_new2(parsed_expression);
		gsub("&nbsp;", " ", parsed_expression);
		FIX_SUPSUB_PRE(parse_status_widget(), fix_supsub_status)
		FIX_SUPSUB(parsed_expression)
		if(show_parsed_instead_of_result || parsed_expression_is_displayed_instead_of_result()) {
			show_parsed_in_result(parse_l == 0 ? m_undefined : mparse, po);
			if((result_is_autocalculated() || show_parsed_instead_of_result) && !current_result_text().empty()) {
				string equalsstr;
				if(current_result_text_is_approximate()) {
					if(printops.use_unicode_signs && (!printops.can_display_unicode_string_function || (*printops.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, (void*) parse_status_widget()))) {
						equalsstr = SIGN_ALMOST_EQUAL " ";
					} else {
						equalsstr = "= ";
						equalsstr += _("approx.");
					}
				} else {
					equalsstr = "= ";
				}
				string str_autocalc = equalsstr;
				str_autocalc += current_result_text();
				FIX_SUPSUB(str_autocalc)
				set_status_text(str_autocalc, false);
				status_text_source = STATUS_TEXT_AUTOCALC;
			} else if((!autocalculation_stopped_at_operator() || !result_is_autocalculated()) && message_n >= 1 && !b_func && !last_is_operator(str_e)) {
				size_t i = parsed_expression_tooltip.rfind("\n• ");
				if(i == string::npos) set_status_text(parsed_expression_tooltip, false, had_errors, had_warnings);
				else set_status_text(parsed_expression_tooltip.substr(i + strlen("\n• ")), false, had_errors, had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_ERROR;
			} else if(!b_func) {
				set_status_text("");
			}
		} else {
			if(!b_func) {
				set_status_text(parsed_expression.c_str(), true, had_errors, had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_PARSED;
			}
		}
		set_expression_output_updated(false);
	} else if(!b_func) {
		CALCULATOR->clearMessages();
		unblock_error();
		if(show_parsed_instead_of_result || parsed_expression_is_displayed_instead_of_result()) {
			if(!current_result_text().empty() && (result_is_autocalculated() || show_parsed_instead_of_result)) {
				string equalsstr;
				if(current_result_text_is_approximate()) {
					if(printops.use_unicode_signs && (!printops.can_display_unicode_string_function || (*printops.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, (void*) parse_status_widget()))) {
						equalsstr = SIGN_ALMOST_EQUAL " ";
					} else {
						equalsstr = "= ";
						equalsstr += _("approx.");
					}
				} else {
					equalsstr = "= ";
				}
				string str_autocalc = equalsstr;
				str_autocalc += current_result_text();
				FIX_SUPSUB_PRE(parse_status_widget(), fix_supsub_status)
				FIX_SUPSUB(str_autocalc)
				set_status_text(str_autocalc, false);
				status_text_source = STATUS_TEXT_AUTOCALC;
			} else if((!autocalculation_stopped_at_operator() || !result_is_autocalculated()) && !parsed_expression_tooltip.empty()) {
				size_t i = parsed_expression_tooltip.rfind("\n• ");
				if(i == string::npos) set_status_text(parsed_expression_tooltip, false, parsed_had_errors, parsed_had_warnings);
				else set_status_text(parsed_expression_tooltip.substr(i + strlen("\n• ")), false, parsed_had_errors, parsed_had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_ERROR;
			} else {
				set_status_text("");
			}
		} else {
			set_status_text(parsed_expression.c_str(), true, parsed_had_errors, parsed_had_warnings, parsed_expression_tooltip);
			status_text_source = STATUS_TEXT_PARSED;
		}
	}
	if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	evalops.parse_options.preserve_format = false;
}

void set_status_bottom_border_visible(bool b) {
	gchar *gstr = gtk_css_provider_to_string(statusframe_provider);
	string status_css = gstr;
	g_free(gstr);
	if(b) {
		gsub("border-bottom-width: 0;", "", status_css);
	} else {
		gsub("}", "border-bottom-width: 0;}", status_css);
	}
	gtk_css_provider_load_from_data(statusframe_provider, status_css.c_str(), -1, NULL);
}
void set_status_size_request() {
	PangoLayout *layout_test = gtk_widget_create_pango_layout(parse_status_widget(), NULL);
	FIX_SUPSUB_PRE(parse_status_widget(), fix_supsub_status)
	string str = "Ä<sub>x</sub>y<sup>2</sup>";
	FIX_SUPSUB(str)
	pango_layout_set_markup(layout_test, str.c_str(), -1);
	gint h;
	pango_layout_get_pixel_size(layout_test, NULL, &h);
	g_object_unref(layout_test);
	gtk_widget_set_size_request(parse_status_widget(), -1, h);
}
void set_status_operator_symbols() {
	if(can_display_unicode_string_function_exact(SIGN_MINUS, (void*) parse_status_widget())) sminus_s = SIGN_MINUS;
	else sminus_s = "-";
	if(can_display_unicode_string_function(SIGN_DIVISION, (void*) parse_status_widget())) sdiv_s = SIGN_DIVISION;
	else sdiv_s = "/";
	if(can_display_unicode_string_function_exact(SIGN_DIVISION, (void*) parse_status_widget())) sslash_s = SIGN_DIVISION_SLASH;
	else sslash_s = "/";
	if(can_display_unicode_string_function(SIGN_MULTIDOT, (void*) parse_status_widget())) sdot_s = SIGN_MULTIDOT;
	else sdot_s = "*";
	if(can_display_unicode_string_function(SIGN_MIDDLEDOT, (void*) parse_status_widget())) saltdot_s = SIGN_MIDDLEDOT;
	else saltdot_s = "*";
	if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) parse_status_widget())) stimes_s = SIGN_MULTIPLICATION;
	else stimes_s = "*";
	if(status_layout) {
		g_object_unref(status_layout);
		status_layout = NULL;
	}
}
void update_status_font(bool initial) {
	gint h_old = 0;
	if(!initial) h_old = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
	if(use_custom_status_font) {
		gchar *gstr = font_name_to_css(custom_status_font.c_str());
		gtk_css_provider_load_from_data(statuslabel_l_provider, gstr, -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		if(initial && custom_status_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(parse_status_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			pango_font_description_set_size(font_desc, round(pango_font_description_get_size(font_desc) * 0.9 / PANGO_SCALE) * PANGO_SCALE);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_status_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
		gtk_css_provider_load_from_data(statuslabel_l_provider, "* {font-size: 90%;}", -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, "* {font-size: 90%;}", -1, NULL);
	}
	if(initial) {
		fix_supsub_status = test_supsub(parse_status_widget());
	} else {
		status_font_modified();
		while(gtk_events_pending()) gtk_main_iteration();
		gint h_new = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
		gint winh, winw;
		gtk_window_get_size(main_window(), &winw, &winh);
		winh += (h_new - h_old);
		gtk_window_resize(main_window(), winw, winh);
	}
}
void set_status_font(const char *str) {
	if(!str) {
		use_custom_status_font = false;
	} else {
		use_custom_status_font = true;
		if(custom_status_font != str) {
			save_custom_status_font = true;
			custom_status_font = str;
		}
	}
	update_status_font(false);
}
const char *status_font(bool return_default) {
	if(!return_default && !use_custom_status_font) return NULL;
	return custom_status_font.c_str();
}
void status_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	fix_supsub_status = test_supsub(parse_status_widget());
	set_status_size_request();
	set_status_operator_symbols();
}
void set_status_error_color(const char *color_str) {
	status_error_c = color_str;
	status_error_c_set = true;
	display_parse_status();
}
void set_status_warning_color(const char *color_str) {
	status_warning_c = color_str;
	status_warning_c_set = true;
	display_parse_status();
}
const char *status_error_color() {return status_error_c.c_str();}
const char *status_warning_color() {return status_warning_c.c_str();}
void update_status_menu(bool initial) {
	if(initial) {
		switch(evalops.approximation) {
			case APPROXIMATION_EXACT: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), TRUE);
				break;
			}
			case APPROXIMATION_TRY_EXACT: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), FALSE);
				break;
			}
			default: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), FALSE);
				break;
			}
		}
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_read_precision")), evalops.parse_options.read_precision != DONT_READ_PRECISION);
		switch(evalops.parse_options.parsing_mode) {
			case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace")), TRUE);
				break;
			}
			case PARSING_MODE_CONVENTIONAL: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication")), TRUE);
				break;
			}
			case PARSING_MODE_CHAIN: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax")), TRUE);
				break;
			}
			case PARSING_MODE_RPN: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax")), TRUE);
				break;
			}
			default: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing")), TRUE);
				break;
			}
		}
		switch(evalops.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_degrees")), TRUE);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_gradians")), TRUE);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_radians")), TRUE);
				break;
			}
			default: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_other")), TRUE);
				break;
			}
		}
	} else {
		update_status_approximation();
		update_status_angle();
		update_status_syntax();
	}
}
void update_status_colors(bool) {
	GdkRGBA c;
	gtk_style_context_get_color(gtk_widget_get_style_context(parse_status_widget()), GTK_STATE_FLAG_NORMAL, &c);
	if(!status_error_c_set) {
		GdkRGBA c_err = c;
		if(c_err.red >= 0.8) {
			c_err.green /= 1.5;
			c_err.blue /= 1.5;
			c_err.red = 1.0;
		} else {
			if(c_err.red >= 0.5) c_err.red = 1.0;
			else c_err.red += 0.5;
		}
		gchar ecs[8];
		g_snprintf(ecs, 8, "#%02x%02x%02x", (int) (c_err.red * 255), (int) (c_err.green * 255), (int) (c_err.blue * 255));
		status_error_c = ecs;
	}

	if(!status_warning_c_set) {
		GdkRGBA c_warn = c;
		if(c_warn.blue >= 0.8) {
			c_warn.green /= 1.5;
			c_warn.red /= 1.5;
			c_warn.blue = 1.0;
		} else {
			if(c_warn.blue >= 0.3) c_warn.blue = 1.0;
			else c_warn.blue += 0.7;
		}
		gchar wcs[8];
		g_snprintf(wcs, 8, "#%02x%02x%02x", (int) (c_warn.red * 255), (int) (c_warn.green * 255), (int) (c_warn.blue * 255));
		status_warning_c = wcs;
	}
}

void create_expression_status() {

	statuslabel_r = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_status_right"));

	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 3);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(statuslabel_r, 12);
	gtk_widget_set_margin_start(parse_status_widget(), 9);
#else
	gtk_widget_set_margin_right(statuslabel_r, 12);
	gtk_widget_set_margin_left(parse_status_widget(), 9);
#endif
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_label_set_xalign(GTK_LABEL(parse_status_widget()), 0.0);
	gtk_label_set_yalign(GTK_LABEL(parse_status_widget()), 0.5);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_r), 0.5);
#else
	gtk_misc_set_alignment(GTK_MISC(parse_status_widget()), 0.0, 0.5);
#endif

	statuslabel_l_provider = gtk_css_provider_new();
	statuslabel_r_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(parse_status_widget()), GTK_STYLE_PROVIDER(statuslabel_l_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_r), GTK_STYLE_PROVIDER(statuslabel_r_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	string topframe_css = "* {background-color: ";

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 16)) {
		GdkRGBA bg_color;
		gtk_style_context_get_background_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &bg_color);
		gchar *gstr = gdk_rgba_to_string(&bg_color);
		topframe_css += gstr;
		g_free(gstr);
	} else {
#endif
		topframe_css += "@theme_base_color;";
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	}
#endif
	topframe_css += "; border-left-width: 0; border-right-width: 0; border-radius: 0;}";
	statusframe_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusframe"))), GTK_STYLE_PROVIDER(statusframe_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_css_provider_load_from_data(statusframe_provider, topframe_css.c_str(), -1, NULL);

	update_status_font(true);
	update_status_text();
	set_status_operator_symbols();
	update_status_menu(true);
	set_status_size_request();

	gtk_builder_add_callback_symbols(main_builder, "on_status_left_button_press_event", G_CALLBACK(on_status_left_button_press_event), "on_status_right_button_press_event", G_CALLBACK(on_status_right_button_press_event), "on_status_right_button_release_event", G_CALLBACK(on_status_right_button_release_event), "on_menu_item_status_exact_activate", G_CALLBACK(on_menu_item_status_exact_activate), "on_menu_item_status_degrees_activate", G_CALLBACK(on_menu_item_status_degrees_activate), "on_menu_item_status_gradians_activate", G_CALLBACK(on_menu_item_status_gradians_activate), "on_menu_item_status_radians_activate", G_CALLBACK(on_menu_item_status_radians_activate), "on_menu_item_status_read_precision_activate", G_CALLBACK(on_menu_item_status_read_precision_activate), "on_menu_item_status_adaptive_parsing_activate", G_CALLBACK(on_menu_item_status_adaptive_parsing_activate), "on_menu_item_status_ignore_whitespace_activate", G_CALLBACK(on_menu_item_status_ignore_whitespace_activate), "on_menu_item_status_no_special_implicit_multiplication_activate", G_CALLBACK(on_menu_item_status_no_special_implicit_multiplication_activate), "on_menu_item_status_chain_syntax_activate", G_CALLBACK(on_menu_item_status_chain_syntax_activate), "on_menu_item_status_rpn_syntax_activate", G_CALLBACK(on_menu_item_status_rpn_syntax_activate), "on_menu_item_expression_status_activate", G_CALLBACK(on_menu_item_expression_status_activate), "on_menu_item_parsed_in_result_activate", G_CALLBACK(on_menu_item_parsed_in_result_activate), "on_menu_item_copy_status_activate", G_CALLBACK(on_menu_item_copy_status_activate), "on_menu_item_copy_ascii_status_activate", G_CALLBACK(on_menu_item_copy_ascii_status_activate), NULL);

}
