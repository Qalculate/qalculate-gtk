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
#include "setbasedialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *setbase_builder = NULL;

extern string prev_output_base, prev_input_base;

void on_set_base_entry_output_other_activate(GtkEntry *w, gpointer user_data);
void on_set_base_entry_input_other_activate(GtkEntry *w, gpointer user_data);
void on_set_base_radiobutton_input_other_toggled(GtkToggleButton *w, gpointer user_data);
void on_set_base_combo_input_other_changed(GtkComboBox *w, gpointer user_data);

void on_set_base_combo_output_other_changed(GtkComboBox*, gpointer) {
	string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")));
	remove_blank_ends(str);
	if(str == "φ" || str == "ψ" || str == "π" || str == "√2" || str == "e" || str == "-3" || str == "-2" || str == "-10" || str == "20" || str == "36" || str == "62" || str == "Unicode" || str == _("Bijective base-26") || str == "BCD" || str == "fp16" || str == "float" || str == "double" || str == "fp80" || str == "fp128") on_set_base_entry_output_other_activate(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), NULL);
}
void on_set_base_entry_output_other_activate(GtkEntry *w, gpointer) {
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	if(str.empty() || str == prev_output_base) {prev_output_base = str; return;}
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") {set_output_base(BASE_GOLDEN_RATIO); return;}
	else if(equalsIgnoreCase(str, "Bijective base-26") || equalsIgnoreCase(str, _("Bijective base-26")) || str == "b26" || str == "B26") {set_output_base(BASE_BIJECTIVE_26); return;}
	else if(equalsIgnoreCase(str, "BCD")) {set_output_base(BASE_BINARY_DECIMAL); return;}
	else if(equalsIgnoreCase(str, "unicode")) {set_output_base(BASE_UNICODE); return;}
	else if(equalsIgnoreCase(str, "fp16") || equalsIgnoreCase(str, "binary16")) {set_output_base(BASE_FP16); return;}
	else if(equalsIgnoreCase(str, "fp32") || equalsIgnoreCase(str, "binary32") || equalsIgnoreCase(str, "float")) {set_output_base(BASE_FP32); return;}
	else if(equalsIgnoreCase(str, "fp64") || equalsIgnoreCase(str, "binary64") || equalsIgnoreCase(str, "double")) {set_output_base(BASE_FP64); return;}
	else if(equalsIgnoreCase(str, "fp80")) {set_output_base(BASE_FP80); return;}
	else if(equalsIgnoreCase(str, "fp128") || equalsIgnoreCase(str, "binary128")) {set_output_base(BASE_FP128); return;}
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") {set_output_base(BASE_SUPER_GOLDEN_RATIO); return;}
	else if(equalsIgnoreCase(str, "pi") || str == "π") {set_output_base(BASE_PI); return;}
	else if(str == "e") {set_output_base(BASE_E); return;}
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") {set_output_base(BASE_SQRT2); return;}
	EvaluationOptions eo = evalops;
	eo.parse_options.base = 10;
	eo.approximation = APPROXIMATION_TRY_EXACT;
	int base;
	MathStructure m;
	CALCULATOR->beginTemporaryStopMessages();
	CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 500, eo);
	if(CALCULATOR->endTemporaryStopMessages() || !m.isNumber() || !m.number().isReal() || (m.number().isNegative() && !m.number().isInteger()) || !(m.number() > 1 || m.number() < -1)) {
		prev_output_base = str;
		show_message(_("Unsupported base."), GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog")));
		return;
	}
	if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
		base = m.number().intValue();
	} else {
		base = BASE_CUSTOM;
		CALCULATOR->setCustomOutputBase(m.number());
		prev_output_base = str;
	}
	set_output_base(base);
}
gboolean on_set_base_entry_output_other_focus_out_event(GtkWidget *w, GdkEvent*, gpointer data) {
	on_set_base_entry_output_other_activate(GTK_ENTRY(w), data);
	return FALSE;
}
void on_set_base_radiobutton_output_binary_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_BINARY);
}
void on_set_base_radiobutton_output_octal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_OCTAL);
}
void on_set_base_radiobutton_output_decimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_DECIMAL);
}
void on_set_base_radiobutton_output_duodecimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(12);
}
void on_set_base_radiobutton_output_hexadecimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_HEXADECIMAL);
}
void on_set_base_radiobutton_output_sexagesimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_SEXAGESIMAL);
}
void on_set_base_radiobutton_output_time_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_TIME);
}
void on_set_base_radiobutton_output_roman_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base(BASE_ROMAN_NUMERALS);
}
void on_set_base_radiobutton_output_other_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")));
	string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")));
	remove_blank_ends(str);
	if(str.empty()) {prev_output_base = str; return;}
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") {set_output_base(BASE_GOLDEN_RATIO); return;}
	else if(equalsIgnoreCase(str, "Bijective base-26") || equalsIgnoreCase(str, _("Bijective base-26")) || str == "b26" || str == "B26") {set_output_base(BASE_BIJECTIVE_26); return;}
	else if(equalsIgnoreCase(str, "bcd")) {set_output_base(BASE_BINARY_DECIMAL); return;}
	else if(equalsIgnoreCase(str, "unicode")) {set_output_base(BASE_UNICODE); return;}
	else if(equalsIgnoreCase(str, "fp16") || equalsIgnoreCase(str, "binary16")) {set_output_base(BASE_FP16); return;}
	else if(equalsIgnoreCase(str, "fp32") || equalsIgnoreCase(str, "binary32") || equalsIgnoreCase(str, "float")) {set_output_base(BASE_FP32); return;}
	else if(equalsIgnoreCase(str, "fp64") || equalsIgnoreCase(str, "binary64") || equalsIgnoreCase(str, "double")) {set_output_base(BASE_FP64); return;}
	else if(equalsIgnoreCase(str, "fp80")) {set_output_base(BASE_FP80); return;}
	else if(equalsIgnoreCase(str, "fp128") || equalsIgnoreCase(str, "binary128")) {set_output_base(BASE_FP128); return;}
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") {set_output_base(BASE_SUPER_GOLDEN_RATIO); return;}
	else if(equalsIgnoreCase(str, "pi") || str == "π") {set_output_base(BASE_PI); return;}
	else if(str == "e") {set_output_base(BASE_E); return;}
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") {set_output_base(BASE_SQRT2); return;}
	EvaluationOptions eo = evalops;
	eo.parse_options.base = 10;
	eo.approximation = APPROXIMATION_TRY_EXACT;
	int base;
	MathStructure m;
	CALCULATOR->beginTemporaryStopMessages();
	CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 500, eo);
	if(CALCULATOR->endTemporaryStopMessages() || !m.isNumber() || !m.number().isReal() || (m.number().isNegative() && !m.number().isInteger()) || !(m.number() > 1 || m.number() < -1)) {
		prev_output_base = str;
		show_message(_("Unsupported base."), GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog")));
		return;
	}
	if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
		base = m.number().intValue();
	} else {
		base = BASE_CUSTOM;
		CALCULATOR->setCustomOutputBase(m.number());
		prev_output_base = str;
	}
	set_output_base(base);
}
void on_set_base_radiobutton_input_binary_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_input_base(BASE_BINARY);
}
void on_set_base_radiobutton_input_octal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_input_base(BASE_OCTAL);
}
void on_set_base_radiobutton_input_decimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_input_base(BASE_DECIMAL);
}
void on_set_base_radiobutton_input_duodecimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_input_base(12);
}
void on_set_base_radiobutton_input_hexadecimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_input_base(BASE_HEXADECIMAL);
}
void on_set_base_radiobutton_input_other_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")));
	string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")));
	remove_blank_ends(str);
	int base = 10;
	if(str.empty() || str == prev_input_base) {prev_input_base = str; return;}
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") {base = BASE_GOLDEN_RATIO;}
	else if(equalsIgnoreCase(str, "Bijective base-26") || equalsIgnoreCase(str, _("Bijective base-26")) || str == "b26" || str == "B26") {base = BASE_BIJECTIVE_26;}
	else if(equalsIgnoreCase(str, "bcd")) {base = BASE_BINARY_DECIMAL;}
	else if(equalsIgnoreCase(str, "unicode")) {base = BASE_UNICODE;}
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") {base = BASE_SUPER_GOLDEN_RATIO;}
	else if(equalsIgnoreCase(str, "pi") || str == "π") {base = BASE_PI;}
	else if(str == "e") {base = BASE_E;}
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") {base = BASE_SQRT2;}
	else {
		EvaluationOptions eo = evalops;
		eo.parse_options.base = 10;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		MathStructure m;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 500, eo);
		if(CALCULATOR->endTemporaryStopMessages() || !m.isNumber()) {
			prev_input_base = str;
			show_message(_("Unsupported base."), GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog")));
			return;
		}
		if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
			base = m.number().intValue();
		} else {
			base = BASE_CUSTOM;
			CALCULATOR->setCustomInputBase(m.number());
		}
	}
	prev_input_base = str;
	set_input_base(base);
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")))) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_combo_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_combo_input_other_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "");
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_combo_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_combo_input_other_changed, NULL);
	}
}
void on_set_base_combo_input_other_changed(GtkComboBox*, gpointer) {
	string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")));
	remove_blank_ends(str);
	if(str == "φ" || str == "ψ" || str == "π" || str == "√2" || str == "e" || str == "-3" || str == "-2" || str == "-10" || str == "20" || str == "36" || str == "62" || str == "Unicode" || str == _("Bijective base-26") || str == "BCD") on_set_base_entry_input_other_activate(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), NULL);
}
void on_set_base_entry_input_other_activate(GtkEntry *w, gpointer) {
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	int base = 10;
	if(str.empty() || str == prev_input_base) {prev_input_base = str; return;}
	if(str.empty() || str == prev_input_base) {prev_input_base = str; return;}
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") {base = BASE_GOLDEN_RATIO;}
	else if(equalsIgnoreCase(str, "Bijective base-26") || equalsIgnoreCase(str, _("Bijective base-26")) || str == "b26" || str == "B26") {base = BASE_BIJECTIVE_26;}
	else if(equalsIgnoreCase(str, "bcd")) {base = BASE_BINARY_DECIMAL;}
	else if(equalsIgnoreCase(str, "unicode")) {base = BASE_UNICODE;}
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") {base = BASE_SUPER_GOLDEN_RATIO;}
	else if(equalsIgnoreCase(str, "pi") || str == "π") {base = BASE_PI;}
	else if(str == "e") {base = BASE_E;}
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") {base = BASE_SQRT2;}
	else {
		EvaluationOptions eo = evalops;
		eo.parse_options.base = 10;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		MathStructure m;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 500, eo);
		if(CALCULATOR->endTemporaryStopMessages() || !m.isNumber()) {
			prev_input_base = str;
			show_message(_("Unsupported base."), GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog")));
			return;
		}
		if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
			base = m.number().intValue();
		} else {
			base = BASE_CUSTOM;
			CALCULATOR->setCustomInputBase(m.number());
		}
	}
	prev_input_base = str;
	set_input_base(base);
}
gboolean on_set_base_entry_input_other_focus_out_event(GtkEntry *w, GdkEvent*, gpointer data) {
	on_set_base_entry_input_other_activate(w, data);
	return FALSE;
}
void on_set_base_radiobutton_input_roman_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_input_base(BASE_ROMAN_NUMERALS);
}

GtkWidget* get_set_base_dialog(void) {
	if(!setbase_builder) {

		setbase_builder = getBuilder("setbase.ui");
		g_assert(setbase_builder != NULL);

		g_assert(gtk_builder_get_object(setbase_builder, "set_base_dialog") != NULL);

		PrintOptions po = printops;
		po.number_fraction_format = FRACTION_DECIMAL_EXACT;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		po.preserve_precision = true;
		po.base = 10;
		if(printops.base >= BASE_CUSTOM && !CALCULATOR->customOutputBase().isZero()) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), CALCULATOR->customOutputBase().print(po).c_str());
		if(evalops.parse_options.base >= BASE_CUSTOM && !CALCULATOR->customInputBase().isZero()) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), CALCULATOR->customInputBase().print(po).c_str());
		switch(evalops.parse_options.base) {
			case BASE_BINARY: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_binary")), TRUE);
				break;
			}
			case BASE_OCTAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_octal")), TRUE);
				break;
			}
			case BASE_DECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_decimal")), TRUE);
				break;
			}
			case 12: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_duodecimal")), TRUE);
				break;
			}
			case BASE_HEXADECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_hexadecimal")), TRUE);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_roman")), TRUE);
				break;
			}
			case BASE_UNICODE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "Unicode");
				break;
			}
			case BASE_E: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "e");
				break;
			}
			case BASE_PI: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "π");
				break;
			}
			case BASE_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "φ");
				break;
			}
			case BASE_SUPER_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "ψ");
				break;
			}
			case BASE_SQRT2: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "√2");
				break;
			}
			case BASE_BIJECTIVE_26: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), _("Bijective base-26"));
				break;
			}
			case BASE_BINARY_DECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "BCD");
				break;
			}
			case BASE_CUSTOM: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), i2s(evalops.parse_options.base).c_str());
			}
		}
		switch(printops.base) {
			case BASE_BINARY: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary")), TRUE);
				break;
			}
			case BASE_OCTAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal")), TRUE);
				break;
			}
			case BASE_DECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal")), TRUE);
				break;
			}
			case 12: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_duodecimal")), TRUE);
				break;
			}
			case BASE_HEXADECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal")), TRUE);
				break;
			}
			case BASE_SEXAGESIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal")), TRUE);
				break;
			}
			case BASE_TIME: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time")), TRUE);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman")), TRUE);
				break;
			}
			case BASE_UNICODE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "Unicode");
				break;
			}
			case BASE_E: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "e");
				break;
			}
			case BASE_PI: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "π");
				break;
			}
			case BASE_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "φ");
				break;
			}
			case BASE_SUPER_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "ψ");
				break;
			}
			case BASE_SQRT2: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "sqrt(2)");
				break;
			}
			case BASE_BIJECTIVE_26: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), _("Bijective base-26"));
				break;
			}
			case BASE_BINARY_DECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "BCD");
				break;
			}
			case BASE_FP16: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp16");
				break;
			}
			case BASE_FP32: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "float");
				break;
			}
			case BASE_FP64: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "double");
				break;
			}
			case BASE_FP80: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp80");
				break;
			}
			case BASE_FP128: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp128");
				break;
			}
			case BASE_CUSTOM: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), i2s(printops.base).c_str());
			}
		}

		SET_FOCUS_ON_CLICK(gtk_builder_get_object(setbase_builder, "button_close"));

		gtk_builder_add_callback_symbols(setbase_builder, "on_set_base_radiobutton_output_binary_toggled", G_CALLBACK(on_set_base_radiobutton_output_binary_toggled), "on_set_base_radiobutton_output_octal_toggled", G_CALLBACK(on_set_base_radiobutton_output_octal_toggled), "on_set_base_radiobutton_output_decimal_toggled", G_CALLBACK(on_set_base_radiobutton_output_decimal_toggled), "on_set_base_radiobutton_output_duodecimal_toggled", G_CALLBACK(on_set_base_radiobutton_output_duodecimal_toggled), "on_set_base_radiobutton_output_hexadecimal_toggled", G_CALLBACK(on_set_base_radiobutton_output_hexadecimal_toggled), "on_set_base_radiobutton_output_sexagesimal_toggled", G_CALLBACK(on_set_base_radiobutton_output_sexagesimal_toggled), "on_set_base_radiobutton_output_time_toggled", G_CALLBACK(on_set_base_radiobutton_output_time_toggled), "on_set_base_radiobutton_output_roman_toggled", G_CALLBACK(on_set_base_radiobutton_output_roman_toggled), "on_set_base_radiobutton_output_other_toggled", G_CALLBACK(on_set_base_radiobutton_output_other_toggled), "on_set_base_combo_output_other_changed", G_CALLBACK(on_set_base_combo_output_other_changed), "on_set_base_entry_output_other_activate", G_CALLBACK(on_set_base_entry_output_other_activate), "on_set_base_entry_output_other_focus_out_event", G_CALLBACK(on_set_base_entry_output_other_focus_out_event), "on_set_base_radiobutton_input_binary_toggled", G_CALLBACK(on_set_base_radiobutton_input_binary_toggled), "on_set_base_radiobutton_input_octal_toggled", G_CALLBACK(on_set_base_radiobutton_input_octal_toggled), "on_set_base_radiobutton_input_decimal_toggled", G_CALLBACK(on_set_base_radiobutton_input_decimal_toggled), "on_set_base_radiobutton_input_duodecimal_toggled", G_CALLBACK(on_set_base_radiobutton_input_duodecimal_toggled), "on_set_base_radiobutton_input_hexadecimal_toggled", G_CALLBACK(on_set_base_radiobutton_input_hexadecimal_toggled), "on_set_base_radiobutton_input_roman_toggled", G_CALLBACK(on_set_base_radiobutton_input_roman_toggled), "on_set_base_radiobutton_input_other_toggled", G_CALLBACK(on_set_base_radiobutton_input_other_toggled), "on_set_base_combo_input_other_changed", G_CALLBACK(on_set_base_combo_input_other_changed), "on_set_base_entry_input_other_activate", G_CALLBACK(on_set_base_entry_input_other_activate), "on_set_base_entry_input_other_focus_out_event", G_CALLBACK(on_set_base_entry_input_other_focus_out_event), NULL);
		gtk_builder_connect_signals(setbase_builder, NULL);

	}
	prev_output_base = ""; prev_input_base = "";
	if(!enable_tooltips || toe_changed) set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog")), enable_tooltips);
	if(always_on_top || aot_changed) gtk_window_set_keep_above(GTK_WINDOW(gtk_builder_get_object(setbase_builder, "set_base_dialog")), always_on_top);
	return GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog"));
}

void bases_updated() {
	if(setbase_builder) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_combo_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_combo_output_other_changed, NULL);
		switch(printops.base) {
			case BASE_BINARY: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_binary_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_binary_toggled, NULL);
				break;
			}
			case BASE_OCTAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_octal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_octal_toggled, NULL);
				break;
			}
			case BASE_DECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_decimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_decimal_toggled, NULL);
				break;
			}
			case 12: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_duodecimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_duodecimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_duodecimal_toggled, NULL);
				break;
			}
			case BASE_HEXADECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_hexadecimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_hexadecimal_toggled, NULL);
				break;
			}
			case BASE_SEXAGESIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_sexagesimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_sexagesimal_toggled, NULL);
				break;
			}
			case BASE_TIME: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_time_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_time_toggled, NULL);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_roman_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_roman_toggled, NULL);
				break;
			}
			case BASE_UNICODE: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "Unicode");
				break;
			}
			case BASE_E: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "e");
				break;
			}
			case BASE_PI: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "π");
				break;
			}
			case BASE_GOLDEN_RATIO: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "φ");
				break;
			}
			case BASE_SUPER_GOLDEN_RATIO: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "ψ");
				break;
			}
			case BASE_SQRT2: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "√2");
				break;
			}
			case BASE_BIJECTIVE_26: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), _("Bijective base-26"));
				break;
			}
			case BASE_BINARY_DECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "BCD");
				break;
			}
			case BASE_FP16: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp16");
				break;
			}
			case BASE_FP32: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "float");
				break;
			}
			case BASE_FP64: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "double");
				break;
			}
			case BASE_FP80: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp80");
				break;
			}
			case BASE_FP128: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp128");
				break;
			}
			case BASE_CUSTOM: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				PrintOptions po = printops;
				po.is_approximate = NULL;
				po.number_fraction_format = FRACTION_DECIMAL_EXACT;
				po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
				po.preserve_precision = true;
				po.base = 10;
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), CALCULATOR->customOutputBase().print(po).c_str());
				break;
			}
			default: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), i2s(printops.base).c_str());
			}
		}
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_combo_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_combo_output_other_changed, NULL);
	}
	if(setbase_builder) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_combo_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_combo_input_other_changed, NULL);
		switch(evalops.parse_options.base) {
			case BASE_BINARY: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_binary_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_binary")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_binary_toggled, NULL);
				break;
			}
			case BASE_OCTAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_octal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_octal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_octal_toggled, NULL);
				break;
			}
			case BASE_DECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_decimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_decimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_decimal_toggled, NULL);
				break;
			}
			case 12: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_duodecimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_duodecimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_duodecimal_toggled, NULL);
				break;
			}
			case BASE_HEXADECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_hexadecimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_hexadecimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_hexadecimal_toggled, NULL);
				break;
			}
			case BASE_UNICODE: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "Unicode");
				break;
			}
			case BASE_E: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "e");
				break;
			}
			case BASE_PI: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "π");
				break;
			}
			case BASE_GOLDEN_RATIO: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "φ");
				break;
			}
			case BASE_SUPER_GOLDEN_RATIO: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "ψ");
				break;
			}
			case BASE_SQRT2: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "√2");
				break;
			}
			case BASE_BIJECTIVE_26: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), _("Bijective base-26"));
				break;
			}
			case BASE_BINARY_DECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "BCD");
				break;
			}
			case BASE_CUSTOM: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				PrintOptions po = printops;
				po.is_approximate = NULL;
				po.number_fraction_format = FRACTION_DECIMAL_EXACT;
				po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
				po.preserve_precision = true;
				po.base = 10;
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), CALCULATOR->customInputBase().print(po).c_str());
				break;
			}
			default: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_input_other_toggled, NULL);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), i2s(evalops.parse_options.base).c_str());
			}
		}
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_combo_input_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_combo_input_other_changed, NULL);
	}
}
void open_setbase(GtkWindow *parent, bool custom, bool input) {
	GtkWidget *dialog = get_set_base_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
	if(custom && input) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")));
	} else if(custom) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")));
	}
}
