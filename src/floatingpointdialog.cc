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
#include "floatingpointdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *floatingpoint_builder = NULL;
bool changing_in_fp_dialog = false;

unsigned int get_fp_bits() {
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(floatingpoint_builder, "fp_combo_bits")))) {
		case 0: return 16;
		case 1: return 32;
		case 2: return 64;
		case 3: return 80;
		case 4: return 128;
		case 5: return 24;
		case 6: return 32;
	}
	return 32;
}
unsigned int get_fp_expbits() {
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(floatingpoint_builder, "fp_combo_bits")));
	if(i == 5) return 8;
	return standard_expbits(get_fp_bits());
}
unsigned int get_fp_sgnpos() {
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(floatingpoint_builder, "fp_combo_bits")));
	if(i == 5 || i == 6) return 8;
	return 0;
}

void update_fp_entries(string sbin, int base, Number *decnum = NULL);
void on_fp_entry_dec_changed(GtkEditable *editable, gpointer) {
	if(changing_in_fp_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(last_is_operator(str, true)) return;
	unsigned int bits = get_fp_bits();
	unsigned int expbits = get_fp_expbits();
	unsigned int sgnpos = get_fp_sgnpos();
	changing_in_fp_dialog = true;
	EvaluationOptions eo;
	eo.parse_options = evalops.parse_options;
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.base = 10;
	MathStructure value;
	block_error();
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), eo.parse_options), 1500, eo);
	if(value.isNumber()) {
		string sbin = to_float(value.number(), bits, expbits, sgnpos);
		update_fp_entries(sbin, 10, &value.number());
	} else if(value.isUndefined()) {
		string sbin = to_float(nr_one_i, bits, expbits, sgnpos);
		update_fp_entries(sbin, 10);
	} else {
		update_fp_entries("", 10);
	}
	changing_in_fp_dialog = false;
	CALCULATOR->clearMessages();
	unblock_error();
}
void on_fp_combo_bits_changed(GtkComboBox*, gpointer) {
	on_fp_entry_dec_changed(GTK_EDITABLE(gtk_builder_get_object(floatingpoint_builder, "fp_entry_dec")), NULL);
}
void on_fp_buffer_bin_changed(GtkTextBuffer *w, gpointer) {
	if(changing_in_fp_dialog) return;
	GtkTextIter istart, iend;
	gtk_text_buffer_get_start_iter(w, &istart);
	gtk_text_buffer_get_end_iter(w, &iend);
	gchar *gtext = gtk_text_buffer_get_text(w, &istart, &iend, FALSE);
	string str = gtext;
	g_free(gtext);
	remove_blanks(str);
	if(str.empty()) return;
	changing_in_fp_dialog = true;
	block_error();
	unsigned int bits = get_fp_bits();
	if(str.find_first_not_of("01") == string::npos && str.length() <= bits) {
		update_fp_entries(str, 2);
	} else {
		update_fp_entries("", 2);
	}
	changing_in_fp_dialog = false;
	CALCULATOR->clearMessages();
	unblock_error();
}
void on_fp_entry_hex_changed(GtkEditable *editable, gpointer) {
	if(changing_in_fp_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blanks(str);
	if(str.empty()) return;
	changing_in_fp_dialog = true;
	unsigned int bits = get_fp_bits();
	block_error();
	ParseOptions pa;
	pa.base = BASE_HEXADECIMAL;
	Number nr(str, pa);
	PrintOptions po;
	po.base = BASE_BINARY;
	po.binary_bits = bits;
	po.max_decimals = 0;
	po.use_max_decimals = true;
	po.base_display = BASE_DISPLAY_NONE;
	string sbin = nr.print(po);
	if(sbin.length() < bits) sbin.insert(0, bits - sbin.length(), '0');
	if(sbin.length() <= bits) {
		update_fp_entries(sbin, 16);
	} else {
		update_fp_entries("", 16);
	}
	changing_in_fp_dialog = false;
	CALCULATOR->clearMessages();
	unblock_error();
}
void update_fp_entries(string sbin, int base, Number *decnum) {
	unsigned int bits = get_fp_bits();
	unsigned int expbits = get_fp_expbits();
	unsigned int sgnpos = get_fp_sgnpos();
	GtkWidget *w_dec, *w_hex, *w_float, *w_floathex, *w_value, *w_error;
	GtkTextBuffer *w_bin;
	w_dec = GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_dec"));
	w_bin = GTK_TEXT_BUFFER(gtk_builder_get_object(floatingpoint_builder, "fp_buffer_bin"));
	w_hex = GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_hex"));
	w_float = GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_float"));
	w_floathex = GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_floathex"));
	w_value = GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_value"));
	w_error = GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_error"));
	g_signal_handlers_block_matched((gpointer) w_dec, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_fp_entry_dec_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_bin, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_fp_buffer_bin_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_hex, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_fp_entry_hex_changed, NULL);
	if(sbin.empty()) {
		if(base != 10) gtk_entry_set_text(GTK_ENTRY(w_dec), "");
		if(base != 16) gtk_entry_set_text(GTK_ENTRY(w_hex), "");
		if(base != 2) gtk_text_buffer_set_text(w_bin, "", -1);
		gtk_entry_set_text(GTK_ENTRY(w_float), "");
		gtk_entry_set_text(GTK_ENTRY(w_floathex), "");
		gtk_entry_set_text(GTK_ENTRY(w_value), "");
		gtk_entry_set_text(GTK_ENTRY(w_error), "");
	} else {
		PrintOptions po;
		po.number_fraction_format = FRACTION_DECIMAL;
		po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		po.use_unicode_signs = printops.use_unicode_signs;
		po.exp_display = printops.exp_display;
		po.lower_case_numbers = printops.lower_case_numbers;
		po.rounding = printops.rounding;
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
		po.binary_bits = bits;
		po.show_ending_zeroes = false;
		po.min_exp = 0;
		int prec_bak = CALCULATOR->getPrecision();
		CALCULATOR->setPrecision(100);
		ParseOptions pa;
		pa.base = BASE_BINARY;
		Number nr(sbin, pa);
		if(base != 16) {po.base = 16; gtk_entry_set_text(GTK_ENTRY(w_hex), nr.print(po).c_str());}
		if(base != 2) {
			string str = sbin;
			if(bits > 32) {
				for(size_t i = expbits + 5; i < str.length() - 1; i += 4) {
					if((bits == 80 && str.length() - i == 32) || (bits == 128 && (str.length() - i == 56))) str.insert(i, "\n");
					else str.insert(i, " ");
					i++;
				}
			}
			str.insert(expbits + 1, bits > 32 ? "\n" : " ");
			str.insert(1, " ");
			gtk_text_buffer_set_text(w_bin, str.c_str(), -1);
		}
		if(printops.min_exp == -1 || printops.min_exp == 0) po.min_exp = 8;
		else po.min_exp = printops.min_exp;
		po.base = 10;
		po.max_decimals = 50;
		po.use_max_decimals = true;
		Number value;
		int ret = from_float(value, sbin, bits, expbits, sgnpos);
		if(ret <= 0) {
			gtk_entry_set_text(GTK_ENTRY(w_float), ret < 0 ? "NaN" : "");
			gtk_entry_set_text(GTK_ENTRY(w_floathex), ret < 0 ? "NaN" : "");
			gtk_entry_set_text(GTK_ENTRY(w_value), ret < 0 ? "NaN" : "");
			gtk_entry_set_text(GTK_ENTRY(w_error), "");
			if(base != 10) gtk_entry_set_text(GTK_ENTRY(w_dec), m_undefined.print(po).c_str());
		} else {
			if(sbin.length() < bits) sbin.insert(0, bits - sbin.length(), '0');
			Number exponent, significand;
			exponent.set(sbin.substr(1, expbits), pa);
			Number expbias(2);
			expbias ^= (expbits - 1);
			expbias--;
			bool subnormal = exponent.isZero();
			exponent -= expbias;
			string sfloat, sfloathex;
			bool b_approx = false;
			po.is_approximate = &b_approx;
			if(exponent > expbias) {
				if(sbin[0] != '0') sfloat = nr_minus_inf.print(po);
				else sfloat = nr_plus_inf.print(po);
				sfloathex = sfloat;
			} else {
				if(subnormal) exponent++;
				if(subnormal) significand.set(string("0.") + sbin.substr(1 + expbits), pa);
				else significand.set(string("1.") + sbin.substr(1 + expbits), pa);
				if(sbin[0] != '0') significand.negate();
				int exp_bak = po.min_exp;
				po.min_exp = 0;
				sfloat = significand.print(po);
				if(!subnormal || !significand.isZero()) {
					sfloat += " ";
					sfloat += expression_times_sign();
					sfloat += " ";
					sfloat += "2^";
					sfloat += exponent.print(po);
				}
				if(b_approx) sfloat.insert(0, SIGN_ALMOST_EQUAL " ");
				b_approx = false;
				po.base = 16;
				po.lower_case_numbers = true;
				po.decimalpoint_sign = ".";
				if(significand.isNegative()) {
					significand.negate();
					sfloathex = "-";
				}
				po.use_unicode_signs = false;
				sfloathex += "0x";
				sfloathex += significand.print(po);
				po.base = 10;
				sfloathex += 'p';
				if(sfloathex == "0") sfloathex += "-1";
				else sfloathex += exponent.print(po);
				if(b_approx) sfloat.insert(0, SIGN_ALMOST_EQUAL " ");
				po.decimalpoint_sign = printops.decimalpoint_sign;
				po.lower_case_numbers = false;
				po.min_exp = exp_bak;
				po.use_unicode_signs = printops.use_unicode_signs;
			}
			gtk_entry_set_text(GTK_ENTRY(w_float), sfloat.c_str());
			gtk_entry_set_text(GTK_ENTRY(w_floathex), sfloathex.c_str());
			b_approx = false;
			string svalue = value.print(po);
			if(base != 10) gtk_entry_set_text(GTK_ENTRY(w_dec), svalue.c_str());
			if(b_approx) svalue.insert(0, SIGN_ALMOST_EQUAL " ");
			gtk_entry_set_text(GTK_ENTRY(w_value), svalue.c_str());
			Number nr_error;
			if(decnum && (!decnum->isInfinite() || !value.isInfinite())) {
				nr_error = value;
				nr_error -= *decnum;
				nr_error.abs();
				if(decnum->isApproximate() && prec_bak < CALCULATOR->getPrecision()) CALCULATOR->setPrecision(prec_bak);
			}
			b_approx = false;
			string serror = nr_error.print(po);
			if(b_approx) serror.insert(0, SIGN_ALMOST_EQUAL " ");
			gtk_entry_set_text(GTK_ENTRY(w_error), serror.c_str());
		}
		CALCULATOR->setPrecision(prec_bak);
	}
	g_signal_handlers_unblock_matched((gpointer) w_dec, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_fp_entry_dec_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_bin, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_fp_buffer_bin_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_hex, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_fp_entry_hex_changed, NULL);
}
void fp_insert_text(GtkWidget *w, const gchar *text) {
	changing_in_fp_dialog = true;
	gtk_editable_delete_selection(GTK_EDITABLE(w));
	changing_in_fp_dialog = false;
	gint pos = gtk_editable_get_position(GTK_EDITABLE(w));
	gtk_editable_insert_text(GTK_EDITABLE(w), text, -1, &pos);
	gtk_editable_set_position(GTK_EDITABLE(w), pos);
	gtk_widget_grab_focus(w);
	gtk_editable_select_region(GTK_EDITABLE(w), pos, pos);
}

gboolean on_floatingpoint_dialog_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(b_busy) {
		if(event->keyval == GDK_KEY_Escape) {
			if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
			else if(b_busy_result) on_abort_display(NULL, 0, NULL);
			else if(b_busy_command) on_abort_command(NULL, 0, NULL);
		}
		return TRUE;
	}
	return FALSE;
}
gboolean on_fp_entry_dec_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(entry_in_quotes(GTK_ENTRY(o))) return FALSE;
	const gchar *key = key_press_get_symbol(event);
	if(!key) return FALSE;
	if(strlen(key) > 0) fp_insert_text(o, key);
	return TRUE;
}
GtkWidget* get_floatingpoint_dialog(void) {
	if(!floatingpoint_builder) {

		floatingpoint_builder = getBuilder("floatingpoint.ui");
		g_assert(floatingpoint_builder != NULL);

		g_assert(gtk_builder_get_object(floatingpoint_builder, "floatingpoint_dialog") != NULL);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
		gtk_text_view_set_top_margin(GTK_TEXT_VIEW(gtk_builder_get_object(floatingpoint_builder, "fp_textedit_bin")), 6);
		gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(gtk_builder_get_object(floatingpoint_builder, "fp_textedit_bin")), 6);
#else
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(gtk_builder_get_object(floatingpoint_builder, "fp_textedit_bin")), 6);
#endif

		gtk_builder_add_callback_symbols(floatingpoint_builder, "on_fp_buffer_bin_changed", G_CALLBACK(on_fp_buffer_bin_changed), "on_floatingpoint_dialog_key_press_event", G_CALLBACK(on_floatingpoint_dialog_key_press_event), "on_fp_entry_dec_changed", G_CALLBACK(on_fp_entry_dec_changed), "on_fp_entry_dec_key_press_event", G_CALLBACK(on_fp_entry_dec_key_press_event), "on_fp_entry_hex_changed", G_CALLBACK(on_fp_entry_hex_changed), "on_fp_combo_bits_changed", G_CALLBACK(on_fp_combo_bits_changed), NULL);
		gtk_builder_connect_signals(floatingpoint_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "floatingpoint_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "floatingpoint_dialog"));
}
void convert_floatingpoint(const gchar *initial_expression, bool b_result, GtkWindow *parent) {
	changing_in_fp_dialog = false;
	GtkWidget *dialog = get_floatingpoint_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	switch(b_result ? displayed_printops.base : evalops.parse_options.base) {
		case BASE_BINARY: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(floatingpoint_builder, "fp_entry_bin")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_bin")));
			break;
		}
		case BASE_HEXADECIMAL: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(floatingpoint_builder, "fp_entry_hex")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_hex")));
			break;
		}
		default: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(floatingpoint_builder, "fp_entry_dec")), initial_expression);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "fp_entry_dec")));
		}
	}
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
