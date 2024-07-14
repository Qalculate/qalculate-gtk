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
#include "numberbasesdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *nbases_builder = NULL;
bool show_bases_keypad = true;
bool changing_in_nbases_dialog = false;
string nbases_error_color, nbases_warning_color;

void on_nbases_button_close_clicked(GtkButton *button, gpointer user_data);
void on_nbases_entry_decimal_changed(GtkEditable *editable, gpointer user_data);
void on_nbases_entry_binary_changed(GtkEditable *editable, gpointer user_data);
void on_nbases_entry_octal_changed(GtkEditable *editable, gpointer user_data);
void on_nbases_entry_hexadecimal_changed(GtkEditable *editable, gpointer user_data);
void on_nbases_entry_duo_changed(GtkEditable *editable, gpointer user_data);
void on_nbases_entry_roman_changed(GtkEditable *editable, gpointer user_data);
void on_nbases_entry_sexa_changed(GtkEditable *editable, gpointer user_data);

gboolean on_nbases_event_hide_buttons_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(event->type == GDK_BUTTON_RELEASE && event->button == 1) {
		show_bases_keypad = !gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "box_keypad")));
		if(show_bases_keypad) {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "box_keypad")));
		} else {
			gint w, h;
			gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(nbases_builder, "nbases_dialog")), &w, &h);
			w -= gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "box_keypad")));
			w--;
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "box_keypad")));
			gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(nbases_builder, "nbases_dialog")), w, h);
		}
		return TRUE;
	}
	return FALSE;
}
GtkWidget *nbases_get_entry() {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_bin")))) return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_binary"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_oct")))) return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_octal"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_duo")))) return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_duo"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_hex")))) return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_rom")))) return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_roman"));
	return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal"));
}
int nbases_get_base() {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_bin")))) return 2;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_oct")))) return 8;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_duo")))) return 12;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_hex")))) return 16;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_rom")))) return BASE_ROMAN_NUMERALS;
	return 10;
}

void update_nbases_entries(const MathStructure &value, int base) {
	GtkWidget *w_dec, *w_bin, *w_oct, *w_hex, *w_duo, *w_roman;
	w_dec = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal"));
	w_bin = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_binary"));
	w_oct = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_octal"));
	w_hex = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal"));
	w_duo = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_duo"));
	w_roman = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_roman"));
	g_signal_handlers_block_matched((gpointer) w_dec, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_decimal_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_bin, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_binary_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_oct, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_octal_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_hex, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_hexadecimal_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_duo, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_duo_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_roman, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_roman_changed, NULL);
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	po.twos_complement = printops.twos_complement;
	po.hexadecimal_twos_complement = printops.hexadecimal_twos_complement;
	po.use_unicode_signs = printops.use_unicode_signs;
	po.exp_display = printops.exp_display;
	po.lower_case_numbers = printops.lower_case_numbers;
	po.base_display = BASE_DISPLAY_NONE;
	po.abbreviate_names = printops.abbreviate_names;
	po.digit_grouping = printops.digit_grouping;
	po.multiplication_sign = printops.multiplication_sign;
	po.division_sign = printops.division_sign;
	po.short_multiplication = printops.short_multiplication;
	po.excessive_parenthesis = printops.excessive_parenthesis;
	po.can_display_unicode_string_function = &can_display_unicode_string_function;
	po.can_display_unicode_string_arg = (void*) w_dec;
	po.spell_out_logical_operators = printops.spell_out_logical_operators;
	po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	po.round_halfway_to_even = printops.round_halfway_to_even;
	string str;
	if(base != 10) {po.base = 10; str = value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po); if(str.length() > 1000) {str = _("result is too long");} gtk_entry_set_text(GTK_ENTRY(w_dec), str.c_str());}
	if(base != 8) {po.base = 8; str = value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po); if(str.length() > 1000) {str = _("result is too long");} gtk_entry_set_text(GTK_ENTRY(w_oct), str.c_str());}
	if(base != 12) {po.base = 12; str = value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po); if(str.length() > 1000) {str = _("result is too long");} gtk_entry_set_text(GTK_ENTRY(w_duo), str.c_str());}
	if(base != 16) {po.base = 16; str = value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po); if(str.length() > 1000) {str = _("result is too long");} gtk_entry_set_text(GTK_ENTRY(w_hex), str.c_str());}
	if(base != BASE_ROMAN_NUMERALS) {
		if(value.isAborted()) {
			gtk_entry_set_text(GTK_ENTRY(w_roman), CALCULATOR->timedOutString().c_str());
		} else if(!value.isNumber() || !value.number().isReal() || !(value.number() <= 9999) || !(value.number() >= -9999)) {
			gtk_entry_set_text(GTK_ENTRY(w_roman), "-");
		} else {
			Number nr = value.number();
			nr.round(printops.rounding);
			po.base = BASE_ROMAN_NUMERALS;
			gtk_entry_set_text(GTK_ENTRY(w_roman), nr.print(po).c_str());
		}
	}
	if(base != 2) {po.base = 2; po.base_display = BASE_DISPLAY_NORMAL; str = value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po); if(str.length() > 1000) {str = _("result is too long");} gtk_entry_set_text(GTK_ENTRY(w_bin), str.c_str());}
	g_signal_handlers_unblock_matched((gpointer) w_dec, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_decimal_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_bin, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_binary_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_oct, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_octal_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_hex, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_hexadecimal_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_duo, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_duo_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_roman, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_roman_changed, NULL);
	gtk_widget_set_tooltip_text(w_dec, "");
	gtk_widget_set_tooltip_text(w_bin, "");
	gtk_widget_set_tooltip_text(w_oct, "");
	gtk_widget_set_tooltip_text(w_duo, "");
	gtk_widget_set_tooltip_text(w_hex, "");
	gtk_widget_set_tooltip_text(w_roman, "");
	if(base == 2) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_binary")), "");
	if(base == 8) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_octal")), "");
	if(base == 10) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_decimal")), "");
	if(base == 12) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_duodecimal")), "");
	if(base == 16) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_hexadecimal")), "");
	if(base == BASE_ROMAN_NUMERALS) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_roman")), "");
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_binary")), _("Binary"));
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_octal")), _("Octal"));
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_decimal")), _("Decimal"));
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_duodecimal")), _("Duodecimal"));
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_hexadecimal")), _("Hexadecimal"));
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_roman")), _("Roman numerals"));
	if(CALCULATOR->message()) {
		string sfull;
		int index = 0;
		MessageType mtype_highest = MESSAGE_INFORMATION;
		while(true) {
			if(!implicit_question_asked || CALCULATOR->message()->category() != MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION) {
				MessageType mtype = CALCULATOR->message()->type();
				if(index > 0) {
					if(index == 1) sfull = "• " + sfull;
					sfull += "\n• ";
				}
				sfull += CALCULATOR->message()->message();
				if(mtype > mtype_highest) {
					mtype_highest = mtype;
				}
				index++;
			}
			if(!CALCULATOR->nextMessage()) break;
		}
		if(base == 2) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_binary")), sfull.c_str());
		else if(base == 8) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_octal")), sfull.c_str());
		else if(base == 10) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_decimal")), sfull.c_str());
		else if(base == 12) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_duodecimal")), sfull.c_str());
		else if(base == 16) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_hexadecimal")), sfull.c_str());
		else if(base == BASE_ROMAN_NUMERALS) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_roman")), sfull.c_str());
		if(base == 10) gtk_widget_set_tooltip_text(w_dec, sfull.c_str());
		else if(base == 2) gtk_widget_set_tooltip_text(w_bin, sfull.c_str());
		else if(base == 8) gtk_widget_set_tooltip_text(w_oct, sfull.c_str());
		else if(base == 12) gtk_widget_set_tooltip_text(w_duo, sfull.c_str());
		else if(base == 16) gtk_widget_set_tooltip_text(w_hex, sfull.c_str());
		else if(base == BASE_ROMAN_NUMERALS) gtk_widget_set_tooltip_text(w_roman, sfull.c_str());
		if(mtype_highest != MESSAGE_INFORMATION) {
			string str = "<span foreground=\"";
			if(mtype_highest == MESSAGE_ERROR) str += nbases_error_color;
			else str += nbases_warning_color;
			str += "\">";
			if(base == 2) str += _("Binary");
			else if(base == 8) str += _("Octal");
			else if(base == 10) str += _("Decimal");
			else if(base == 12) str += _("Duodecimal");
			else if(base == 16) str += _("Hexadecimal");
			else if(base == BASE_ROMAN_NUMERALS) str += _("Roman numerals");
			str += "</span>";
			if(base == 2) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_binary")), str.c_str());
			else if(base == 8) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_octal")), str.c_str());
			else if(base == 10) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_decimal")), str.c_str());
			else if(base == 12) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_duodecimal")), str.c_str());
			else if(base == 16) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_hexadecimal")), str.c_str());
			else if(base == BASE_ROMAN_NUMERALS) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_roman")), str.c_str());
		}
	}
}
void on_nbases_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));
}
void on_nbases_entry_decimal_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str, true)) return;
	changing_in_nbases_dialog = true;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = 10;
	MathStructure value;
	block_error_timeout++;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options), 1500, eo);
	update_nbases_entries(value, 10);
	block_error_timeout--;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_binary_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str)) return;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = BASE_BINARY;
	changing_in_nbases_dialog = true;
	MathStructure value;
	block_error_timeout++;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options), 1500, eo);
	update_nbases_entries(value, 2);
	block_error_timeout--;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_octal_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str)) return;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = BASE_OCTAL;
	changing_in_nbases_dialog = true;
	MathStructure value;
	block_error_timeout++;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options), 1500, eo);
	update_nbases_entries(value, 8);
	block_error_timeout--;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_hexadecimal_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str)) return;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = BASE_HEXADECIMAL;
	changing_in_nbases_dialog = true;
	MathStructure value;
	block_error_timeout++;
	str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options);
	CALCULATOR->calculate(&value, str, 1500, eo);
	update_nbases_entries(value, 16);
	block_error_timeout--;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_duo_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str)) return;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = BASE_DUODECIMAL;
	changing_in_nbases_dialog = true;
	MathStructure value;
	block_error_timeout++;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options), 1500, eo);
	update_nbases_entries(value, 12);
	block_error_timeout--;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_roman_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str) && (str[str.length() - 1] != '|' || str.find('|') == str.length() - 1)) return;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = BASE_ROMAN_NUMERALS;
	changing_in_nbases_dialog = true;
	MathStructure value;
	block_error_timeout++;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options), 1500, eo);
	update_nbases_entries(value, BASE_ROMAN_NUMERALS);
	block_error_timeout--;
	changing_in_nbases_dialog = false;
}

void on_nbases_button_bin_toggled(GtkToggleButton *w, gpointer);
void on_nbases_button_oct_toggled(GtkToggleButton *w, gpointer);
void on_nbases_button_dec_toggled(GtkToggleButton *w, gpointer);
void on_nbases_button_duo_toggled(GtkToggleButton *w, gpointer);
void on_nbases_button_hex_toggled(GtkToggleButton *w, gpointer);
void on_nbases_button_rom_toggled(GtkToggleButton *w, gpointer);

void update_nbases_keypad(int base) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_bin"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_bin_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_oct"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_oct_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_dec"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_dec_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_duo"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_duo_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_hex"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_hex_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_rom"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_rom_toggled, NULL);
	if(base != 2) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_bin")), FALSE);
	if(base != 8) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_oct")), FALSE);
	if(base != 10) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_dec")), FALSE);
	if(base != 12) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_duo")), FALSE);
	if(base != 16) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_hex")), FALSE);
	if(base != BASE_ROMAN_NUMERALS) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_rom")), FALSE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_bin"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_bin_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_oct"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_oct_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_dec"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_dec_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_duo"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_duo_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_hex"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_hex_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_rom"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_rom_toggled, NULL);

	if(base == BASE_ROMAN_NUMERALS && strcmp(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_one"))), "1") != 0) return;

	if(base == 12 && printops.duodecimal_symbols) {
		if(strcmp(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_a"))), "A") == 0) {
			if(can_display_unicode_string_function("↊", (void*) gtk_builder_get_object(nbases_builder, "nbases_label_a"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_a")), "↊");
			else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_a")), "X");
			if(can_display_unicode_string_function("↋", (void*) gtk_builder_get_object(nbases_builder, "nbases_label_b"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_b")), "↋");
			else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_b")), "E");
		}
	} else if(strcmp(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_a"))), "A") != 0) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_a")), "A");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_b")), "B");
	}
	bool uni_roman = (base == BASE_ROMAN_NUMERALS) && printops.use_unicode_signs && can_display_unicode_string_function("Ɔ", (void*) gtk_builder_get_object(nbases_builder, "nbases_label_9"));
	if(base == BASE_ROMAN_NUMERALS) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_zero")), "I");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_one")), "V");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_two")), "X");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_three")), "L");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_four")), "C");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_five")), "D");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_six")), "M");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_eight")), "|");
		if(uni_roman) {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_nine")), "Ɔ");
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_seven")), "(");
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_nine")), ")");
		}
	} else if(strcmp(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_one"))), "1") != 0) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_zero")), "0");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_one")), "1");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_two")), "2");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_three")), "3");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_four")), "4");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_five")), "5");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_six")), "6");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_seven")), "7");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_eight")), "8");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_nine")), "9");
	}

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_two")), base != 2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_three")), base != 2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_four")), base != 2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_five")), base != 2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_six")), base != 2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_seven")), base != 2 && (base != BASE_ROMAN_NUMERALS || !uni_roman));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_eight")), base != 2 && base != 8);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_nine")), base != 2 && base != 8);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_a")), base >= 12);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_b")), base >= 12);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_c")), base == 16);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_d")), base == 16);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_e")), base == 16);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_f")), base == 16);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_and")), base != BASE_ROMAN_NUMERALS);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_or")), base != BASE_ROMAN_NUMERALS);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_xor")), base != BASE_ROMAN_NUMERALS);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_not")), base != BASE_ROMAN_NUMERALS);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_left_shift")), base != BASE_ROMAN_NUMERALS);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_right_shift")), base != BASE_ROMAN_NUMERALS);

}

gboolean on_nbases_entry_binary_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer);
gboolean on_nbases_entry_octal_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer);
gboolean on_nbases_entry_decimal_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer);
gboolean on_nbases_entry_duo_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer);
gboolean on_nbases_entry_hexadecimal_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer);
gboolean on_nbases_entry_roman_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer);

void on_nbases_button_bin_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		g_signal_handlers_block_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_bin_toggled, NULL);
		gtk_toggle_button_set_active(w, TRUE);
		g_signal_handlers_unblock_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_bin_toggled, NULL);
		return;
	}
	update_nbases_keypad(2);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_binary_focus_in_event, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_binary")));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_binary_focus_in_event, NULL);
}
void on_nbases_button_oct_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		g_signal_handlers_block_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_oct_toggled, NULL);
		gtk_toggle_button_set_active(w, TRUE);
		g_signal_handlers_unblock_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_oct_toggled, NULL);
		return;
	}
	update_nbases_keypad(8);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_octal_focus_in_event, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_octal")));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_octal_focus_in_event, NULL);
}
void on_nbases_button_dec_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		g_signal_handlers_block_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_dec_toggled, NULL);
		gtk_toggle_button_set_active(w, TRUE);
		g_signal_handlers_unblock_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_dec_toggled, NULL);
		return;
	}
	update_nbases_keypad(10);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_decimal_focus_in_event, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal")));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_decimal_focus_in_event, NULL);
}
void on_nbases_button_duo_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		g_signal_handlers_block_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_duo_toggled, NULL);
		gtk_toggle_button_set_active(w, TRUE);
		g_signal_handlers_unblock_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_duo_toggled, NULL);
		return;
	}
	update_nbases_keypad(12);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_duo"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_duo_focus_in_event, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_duo")));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_duo"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_duo_focus_in_event, NULL);
}
void on_nbases_button_hex_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		g_signal_handlers_block_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_hex_toggled, NULL);
		gtk_toggle_button_set_active(w, TRUE);
		g_signal_handlers_unblock_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_hex_toggled, NULL);
		return;
	}
	update_nbases_keypad(16);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_hexadecimal_focus_in_event, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal")));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_hexadecimal_focus_in_event, NULL);
}
void on_nbases_button_rom_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		g_signal_handlers_block_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_rom_toggled, NULL);
		gtk_toggle_button_set_active(w, TRUE);
		g_signal_handlers_unblock_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_rom_toggled, NULL);
		return;
	}
	update_nbases_keypad(BASE_ROMAN_NUMERALS);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_roman_focus_in_event, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_roman")));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_entry_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_roman_focus_in_event, NULL);
}

gboolean on_nbases_entry_binary_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_bin"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_bin_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_bin")), TRUE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_bin"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_bin_toggled, NULL);
	update_nbases_keypad(2);
	return FALSE;
}
gboolean on_nbases_entry_octal_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_oct"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_oct_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_oct")), TRUE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_oct"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_oct_toggled, NULL);
	update_nbases_keypad(8);
	return FALSE;
}
gboolean on_nbases_entry_decimal_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_dec"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_dec_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_dec")), TRUE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_dec"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_dec_toggled, NULL);
	update_nbases_keypad(10);
	return FALSE;
}
gboolean on_nbases_entry_duo_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_duo"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_duo_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_duo")), TRUE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_duo"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_duo_toggled, NULL);
	update_nbases_keypad(12);
	return FALSE;
}
gboolean on_nbases_entry_hexadecimal_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_hex"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_hex_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_hex")), TRUE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_hex"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_hex_toggled, NULL);
	update_nbases_keypad(16);
	return FALSE;
}
gboolean on_nbases_entry_roman_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_rom"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_rom_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(nbases_builder, "nbases_button_rom")), TRUE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(nbases_builder, "nbases_button_rom"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_button_rom_toggled, NULL);
	update_nbases_keypad(BASE_ROMAN_NUMERALS);
	return FALSE;
}

void nbases_insert_text(GtkWidget *w, const gchar *text) {
	changing_in_nbases_dialog = true;
	gtk_editable_delete_selection(GTK_EDITABLE(w));
	changing_in_nbases_dialog = false;
	gint pos = gtk_editable_get_position(GTK_EDITABLE(w));
	gtk_editable_insert_text(GTK_EDITABLE(w), text, -1, &pos);
	gtk_editable_set_position(GTK_EDITABLE(w), pos);
	gtk_widget_grab_focus(w);
	gtk_editable_select_region(GTK_EDITABLE(w), pos, pos);
}

void on_nbases_button_zero_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "I" : "0");
}
void on_nbases_button_one_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "V" : "1");
}
void on_nbases_button_two_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "X" : "2");
}
void on_nbases_button_three_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "L" : "3");
}
void on_nbases_button_four_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "C" : "4");
}
void on_nbases_button_five_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "D" : "5");
}
void on_nbases_button_six_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "M" : "6");
}
void on_nbases_button_seven_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "(" : "7");
}
void on_nbases_button_eight_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? "|" : "8");
}
void on_nbases_button_nine_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == BASE_ROMAN_NUMERALS ? (can_display_unicode_string_function("Ɔ", (void*) gtk_builder_get_object(nbases_builder, "nbases_entry_roman")) ? "Ɔ" : ")") : "9");
}
void on_nbases_button_a_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == 12 && printops.duodecimal_symbols ? (can_display_unicode_string_function("↊", (void*) gtk_builder_get_object(nbases_builder, "nbases_entry_duo")) ? "↊" : "X") : (printops.lower_case_numbers ? "a" : "A"));
}
void on_nbases_button_b_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), nbases_get_base() == 12 && printops.duodecimal_symbols ? (can_display_unicode_string_function("↊", (void*) gtk_builder_get_object(nbases_builder, "nbases_entry_duo")) ? "↋" : "E") : (printops.lower_case_numbers ? "b" : "B"));
}
void on_nbases_button_c_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), printops.lower_case_numbers ? "c" : "C");
}
void on_nbases_button_d_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), printops.lower_case_numbers ? "d" : "D");
}
void on_nbases_button_e_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), printops.lower_case_numbers ? "e" : "E");
}
void on_nbases_button_f_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), printops.lower_case_numbers ? "f" : "F");
}
void on_nbases_button_add_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), expression_add_sign());
}
void on_nbases_button_sub_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), expression_sub_sign());
}
void on_nbases_button_times_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), expression_times_sign());
}
void on_nbases_button_divide_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), expression_divide_sign());
}
void on_nbases_button_and_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), "&");
}
void on_nbases_button_or_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), "|");
}
void on_nbases_button_xor_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), " xor ");
}
void on_nbases_button_not_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), "~");
}
void on_nbases_button_left_shift_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), "<<");
}
void on_nbases_button_right_shift_clicked(GtkToggleButton*, gpointer) {
	nbases_insert_text(nbases_get_entry(), ">>");
}
void on_nbases_button_del_clicked(GtkToggleButton*, gpointer) {
	gint i1, i2;
	GtkWidget *w = nbases_get_entry();
	if(!gtk_editable_get_selection_bounds(GTK_EDITABLE(w), &i1, &i2)) {
		i1 = gtk_editable_get_position(GTK_EDITABLE(w));
		i2 = i1 + 1;
	}
	string str = gtk_entry_get_text(GTK_ENTRY(w));
	gtk_editable_delete_text(GTK_EDITABLE(w), i1, i2);
	if(str == gtk_entry_get_text(GTK_ENTRY(w))) gtk_editable_delete_text(GTK_EDITABLE(w), i1 - 1, i2 - 1);
	gtk_widget_grab_focus(w);
	gtk_editable_select_region(GTK_EDITABLE(w), i1, i1);
}
void on_nbases_button_ac_clicked(GtkToggleButton*, gpointer) {
	gtk_entry_set_text(GTK_ENTRY(nbases_get_entry()), "");
	gtk_widget_grab_focus(nbases_get_entry());
}

gboolean on_nbases_dialog_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(b_busy) {
		if(event->keyval == GDK_KEY_Escape) {
			if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
			else if(b_busy_result) on_abort_display(NULL, 0, NULL);
			else if(b_busy_command) on_abort_command(NULL, 0, NULL);
		}
		return TRUE;
	}
	if(entry_in_quotes(GTK_ENTRY(nbases_get_entry()))) return FALSE;
	const gchar *key = key_press_get_symbol(event);
	if(!key) return FALSE;
	if(strlen(key) > 0) nbases_insert_text(nbases_get_entry(), key);
	return TRUE;
}

GtkWidget* get_nbases_dialog(void) {
	if(!nbases_builder) {

		nbases_builder = getBuilder("nbases.ui");
		g_assert(nbases_builder != NULL);

		g_assert(gtk_builder_get_object(nbases_builder, "nbases_dialog") != NULL);

		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_binary")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_octal")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_duo")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_roman")), 1.0);

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14
		if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "pan-start-symbolic")) {
			GtkWidget *arrow_left = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
			gtk_widget_set_size_request(GTK_WIDGET(arrow_left), 18, 18);
			gtk_widget_show(arrow_left);
			gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_image_hide_buttons")));
			gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(nbases_builder, "nbases_event_hide_buttons")), arrow_left);
		}
		if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 14)) gtk_box_set_spacing(GTK_BOX(gtk_builder_get_object(nbases_builder, "grid_nbases")), 0);
#endif

		if(!show_bases_keypad) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "box_keypad")));

		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_sub"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_sub")), SIGN_MINUS);
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_sub")), MINUS);
		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_times"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_times")), SIGN_MULTIPLICATION);
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_times")), MULTIPLICATION);
		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_divide")), SIGN_DIVISION_SLASH);
		else if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_divide")), SIGN_DIVISION);
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_divide")), DIVISION);

		gchar *theme_name = NULL;
		g_object_get(gtk_settings_get_default(), "gtk-theme-name", &theme_name, NULL);
		string themestr;
		if(theme_name) {
			themestr = theme_name;
			g_free(theme_name);
		}

		if(themestr.substr(0, 7) == "Adwaita" || themestr.substr(0, 5) == "oomox" || themestr.substr(0, 6) == "themix" || themestr == "Breeze" || themestr == "Breeze-Dark" || themestr == "Yaru") {
			GtkCssProvider *link_style_top = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_top, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0}", -1, NULL);
			GtkCssProvider *link_style_bot = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bot, "* {border-top-left-radius: 0; border-top-right-radius: 0}", -1, NULL);
			GtkCssProvider *link_style_tl = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_tl, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0; border-top-right-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_tr = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_tr, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0; border-top-left-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_bl = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bl, "* {border-top-left-radius: 0; border-top-right-radius: 0; border-bottom-right-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_br = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_br, "* {border-top-left-radius: 0; border-top-right-radius: 0; border-bottom-left-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_mid = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_mid, "* {border-radius: 0;}", -1, NULL);

			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_zero"))), GTK_STYLE_PROVIDER(link_style_bl), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_one"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_two"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_three"))), GTK_STYLE_PROVIDER(link_style_br), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_four"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_five"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_six"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_seven"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_eight"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_nine"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_a"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_b"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_c"))), GTK_STYLE_PROVIDER(link_style_tl), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_d"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_e"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_f"))), GTK_STYLE_PROVIDER(link_style_tr), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_and"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_or"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_xor"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_not"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_divide"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_times"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_sub"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_add"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_left_shift"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_right_shift"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_del"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_ac"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}

		GdkRGBA c;
		gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_decimal"))), GTK_STATE_FLAG_NORMAL, &c);
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
		nbases_error_color = ecs;

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
		nbases_warning_color = wcs;

		gtk_builder_add_callback_symbols(nbases_builder, "on_nbases_dialog_key_press_event", G_CALLBACK(on_nbases_dialog_key_press_event), "on_nbases_entry_binary_changed", G_CALLBACK(on_nbases_entry_binary_changed), "on_nbases_entry_binary_focus_in_event", G_CALLBACK(on_nbases_entry_binary_focus_in_event), "on_nbases_entry_decimal_changed", G_CALLBACK(on_nbases_entry_decimal_changed), "on_nbases_entry_decimal_focus_in_event", G_CALLBACK(on_nbases_entry_decimal_focus_in_event), "on_nbases_entry_octal_changed", G_CALLBACK(on_nbases_entry_octal_changed), "on_nbases_entry_octal_focus_in_event", G_CALLBACK(on_nbases_entry_octal_focus_in_event), "on_nbases_entry_roman_changed", G_CALLBACK(on_nbases_entry_roman_changed), "on_nbases_entry_roman_focus_in_event", G_CALLBACK(on_nbases_entry_roman_focus_in_event), "on_nbases_entry_duo_changed", G_CALLBACK(on_nbases_entry_duo_changed), "on_nbases_entry_duo_focus_in_event", G_CALLBACK(on_nbases_entry_duo_focus_in_event), "on_nbases_entry_hexadecimal_changed", G_CALLBACK(on_nbases_entry_hexadecimal_changed), "on_nbases_entry_hexadecimal_focus_in_event", G_CALLBACK(on_nbases_entry_hexadecimal_focus_in_event), "on_nbases_event_hide_buttons_button_release_event", G_CALLBACK(on_nbases_event_hide_buttons_button_release_event), "on_nbases_button_bin_toggled", G_CALLBACK(on_nbases_button_bin_toggled), "on_nbases_button_oct_toggled", G_CALLBACK(on_nbases_button_oct_toggled), "on_nbases_button_dec_toggled", G_CALLBACK(on_nbases_button_dec_toggled), "on_nbases_button_duo_toggled", G_CALLBACK(on_nbases_button_duo_toggled), "on_nbases_button_hex_toggled", G_CALLBACK(on_nbases_button_hex_toggled), "on_nbases_button_rom_toggled", G_CALLBACK(on_nbases_button_rom_toggled), "on_nbases_button_d_clicked", G_CALLBACK(on_nbases_button_d_clicked), "on_nbases_button_e_clicked", G_CALLBACK(on_nbases_button_e_clicked), "on_nbases_button_f_clicked", G_CALLBACK(on_nbases_button_f_clicked), "on_nbases_button_nine_clicked", G_CALLBACK(on_nbases_button_nine_clicked), "on_nbases_button_a_clicked", G_CALLBACK(on_nbases_button_a_clicked), "on_nbases_button_b_clicked", G_CALLBACK(on_nbases_button_b_clicked), "on_nbases_button_five_clicked", G_CALLBACK(on_nbases_button_five_clicked), "on_nbases_button_six_clicked", G_CALLBACK(on_nbases_button_six_clicked), "on_nbases_button_seven_clicked", G_CALLBACK(on_nbases_button_seven_clicked), "on_nbases_button_one_clicked", G_CALLBACK(on_nbases_button_one_clicked), "on_nbases_button_two_clicked", G_CALLBACK(on_nbases_button_two_clicked), "on_nbases_button_three_clicked", G_CALLBACK(on_nbases_button_three_clicked), "on_nbases_button_c_clicked", G_CALLBACK(on_nbases_button_c_clicked), "on_nbases_button_eight_clicked", G_CALLBACK(on_nbases_button_eight_clicked), "on_nbases_button_four_clicked", G_CALLBACK(on_nbases_button_four_clicked), "on_nbases_button_zero_clicked", G_CALLBACK(on_nbases_button_zero_clicked), "on_nbases_button_add_clicked", G_CALLBACK(on_nbases_button_add_clicked), "on_nbases_button_sub_clicked", G_CALLBACK(on_nbases_button_sub_clicked), "on_nbases_button_times_clicked", G_CALLBACK(on_nbases_button_times_clicked), "on_nbases_button_divide_clicked", G_CALLBACK(on_nbases_button_divide_clicked), "on_nbases_button_and_clicked", G_CALLBACK(on_nbases_button_and_clicked), "on_nbases_button_or_clicked", G_CALLBACK(on_nbases_button_or_clicked), "on_nbases_button_xor_clicked", G_CALLBACK(on_nbases_button_xor_clicked), "on_nbases_button_not_clicked", G_CALLBACK(on_nbases_button_not_clicked), "on_nbases_button_del_clicked", G_CALLBACK(on_nbases_button_del_clicked), "on_nbases_button_ac_clicked", G_CALLBACK(on_nbases_button_ac_clicked), "on_nbases_button_left_shift_clicked", G_CALLBACK(on_nbases_button_left_shift_clicked), "on_nbases_button_right_shift_clicked", G_CALLBACK(on_nbases_button_right_shift_clicked), NULL);
		gtk_builder_connect_signals(nbases_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog"));
}

void convert_number_bases(GtkWindow *parent, const gchar *initial_expression, int base) {
	GtkWidget *dialog = get_nbases_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	switch(base) {
		case BASE_BINARY: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_binary")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_binary")));
			break;
		}
		case BASE_OCTAL: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_octal")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_octal")));
			break;
		}
		case BASE_HEXADECIMAL: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal")));
			break;
		}
		case BASE_DUODECIMAL: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_duo")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_duo")));
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_roman")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_roman")));
			break;
		}
		default: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal")));
		}
	}
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
