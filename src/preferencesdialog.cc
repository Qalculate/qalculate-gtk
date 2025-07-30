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
#include "keypad.h"
#include "stackview.h"
#include "exchangerates.h"
#include "functionsdialog.h"
#include "unitsdialog.h"
#include "variablesdialog.h"
#include "resultview.h"
#include "expressionedit.h"
#include "expressionstatus.h"
#include "expressioncompletion.h"
#include "preferencesdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *preferences_builder = NULL;

extern GtkCssProvider *app_provider_theme, *color_provider;

void preferences_dialog_set(const gchar *obj, gboolean b) {
	if(!preferences_builder) get_preferences_dialog();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, obj)), b);
}
void on_colorbutton_text_color_color_set(GtkColorButton *w, gpointer) {
	GdkRGBA c;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &c);
	gchar color_str[8];
	g_snprintf(color_str, 8, "#%02x%02x%02x", (int) (c.red * 255), (int) (c.green * 255), (int) (c.blue * 255));
	text_color = color_str;
	text_color_set = true;
	if(!color_provider) {
		color_provider = gtk_css_provider_new();
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(color_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
	string css_str = "* {color: "; css_str += text_color; css_str += "}";
	gtk_css_provider_load_from_data(color_provider, css_str.c_str(), -1, NULL);
}
void on_colorbutton_status_error_color_color_set(GtkColorButton *w, gpointer) {
	GdkRGBA c;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &c);
	gchar color_str[8];
	g_snprintf(color_str, 8, "#%02x%02x%02x", (int) (c.red * 255), (int) (c.green * 255), (int) (c.blue * 255));
	set_status_error_color(color_str);
}
void on_colorbutton_status_warning_color_color_set(GtkColorButton *w, gpointer) {
	GdkRGBA c;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &c);
	gchar color_str[8];
	g_snprintf(color_str, 8, "#%02x%02x%02x", (int) (c.red * 255), (int) (c.green * 255), (int) (c.blue * 255));
	set_status_warning_color(color_str);
}
extern int expression_lines;
void on_preferences_expression_lines_spin_button_value_changed(GtkSpinButton *spin, gpointer) {
	expression_lines = gtk_spin_button_get_value_as_int(spin);
	gint h_old = gtk_widget_get_allocated_height(expression_edit_widget());
	gint winw = 0, winh = 0;
	gtk_window_get_size(main_window(), &winw, &winh);
	set_expression_size_request();
	while(gtk_events_pending()) gtk_main_iteration();
	gint h_new = gtk_widget_get_allocated_height(expression_edit_widget());
	winh += (h_new - h_old);
	gtk_window_resize(main_window(), winw, winh);
}
void on_preferences_vertical_padding_combo_changed(GtkComboBox *w, gpointer) {
	set_vertical_button_padding(gtk_combo_box_get_active(w) - 1);
}
void on_preferences_horizontal_padding_combo_changed(GtkComboBox *w, gpointer) {
	set_horizontal_button_padding(gtk_combo_box_get_active(w) - 1);
}
void on_preferences_update_exchange_rates_spin_button_value_changed(GtkSpinButton *spin, gpointer) {
	set_exchange_rates_frequency(gtk_spin_button_get_value_as_int(spin));
}
gint on_preferences_update_exchange_rates_spin_button_input(GtkSpinButton *spin, gdouble *new_value, gpointer) {
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(spin));
	if(g_strcmp0(text, _("never")) == 0) *new_value = 0.0;
	else if(g_strcmp0(text, _("ask")) == 0) *new_value = -1.0;
	else *new_value = g_strtod(text, NULL);
	return TRUE;
}
gboolean on_preferences_update_exchange_rates_spin_button_output(GtkSpinButton *spin, gpointer) {
	int value = gtk_spin_button_get_value_as_int(spin);
	if(value > 0) {
		gchar *text;
		text = g_strdup_printf(_n("%i day","%i days", value), value);
		gtk_entry_set_text(GTK_ENTRY(spin), text);
		g_free(text);
	} else if(value == 0) {
		gtk_entry_set_text(GTK_ENTRY(spin), _("never"));
	} else {
		gtk_entry_set_text(GTK_ENTRY(spin), _("ask"));
	}
	return TRUE;
}
void preferences_update_exchange_rates() {
	if(!preferences_builder) return;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_update_exchange_rates_spin_button")), (double) exchange_rates_frequency());
}
extern int max_plot_time;
void on_preferences_scale_plot_time_value_changed(GtkRange *w, gpointer) {
	Number nr; nr.setFloat(gtk_range_get_value(w) + 0.322); nr.exp2(); nr.round();
	max_plot_time = nr.intValue();
}
void on_preferences_checkbutton_persistent_keypad_toggled(GtkToggleButton *w, gpointer) {
	persistent_keypad = gtk_toggle_button_get_active(w);
	update_persistent_keypad(true);
}
void preferences_update_persistent_keypad() {
	if(!preferences_builder) return;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_persistent_keypad"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_persistent_keypad_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_persistent_keypad")), persistent_keypad);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_persistent_keypad"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_persistent_keypad_toggled, NULL);
}
void on_preferences_checkbutton_clear_history_toggled(GtkToggleButton *w, gpointer) {
	clear_history_on_exit = gtk_toggle_button_get_active(w);
}
extern bool save_history_separately;
void on_preferences_checkbutton_save_history_separately_toggled(GtkToggleButton *w, gpointer) {
	save_history_separately = gtk_toggle_button_get_active(w);
}
extern int max_history_lines;
void on_preferences_max_history_lines_spin_button_value_changed(GtkSpinButton *spin, gpointer) {
	max_history_lines = gtk_spin_button_get_value_as_int(spin);
}
void on_preferences_checkbutton_check_version_toggled(GtkToggleButton *w, gpointer) {
	check_version = gtk_toggle_button_get_active(w);
	if(check_version) check_for_new_version(true);
}
extern bool remember_position;
void on_preferences_checkbutton_remember_position_toggled(GtkToggleButton *w, gpointer) {
	remember_position = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_keep_above_toggled(GtkToggleButton *w, gpointer) {
	always_on_top = gtk_toggle_button_get_active(w);
	aot_changed = true;
	gtk_window_set_keep_above(main_window(), always_on_top);
}
void preferences_update_keep_above() {
	if(!preferences_builder) return;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_keep_above"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_keep_above_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_keep_above")), always_on_top);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_keep_above"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_keep_above_toggled, NULL);
}
void on_preferences_combo_tooltips_changed(GtkComboBox *w, gpointer) {
	int i = gtk_combo_box_get_active(w);
	if(i == 2) enable_tooltips = 0;
	else if(i == 1) enable_tooltips = 2;
	else enable_tooltips = 1;
	toe_changed = true;
	set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_dialog")), enable_tooltips);
	update_tooltips_enabled();
}
void on_preferences_checkbutton_local_currency_conversion_toggled(GtkToggleButton *w, gpointer) {
	evalops.local_currency_conversion = gtk_toggle_button_get_active(w);
	expression_calculation_updated();
}
void on_preferences_checkbutton_binary_prefixes_toggled(GtkToggleButton *w, gpointer) {
	CALCULATOR->useBinaryPrefixes(gtk_toggle_button_get_active(w) ? 1 : 0);
	result_format_updated();
}
extern bool tc_set;
void on_preferences_radiobutton_temp_rel_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	CALCULATOR->setTemperatureCalculationMode(TEMPERATURE_CALCULATION_RELATIVE);
	tc_set = true;
	expression_calculation_updated();
}
void on_preferences_radiobutton_temp_abs_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	CALCULATOR->setTemperatureCalculationMode(TEMPERATURE_CALCULATION_ABSOLUTE);
	tc_set = true;
	expression_calculation_updated();
}
void on_preferences_radiobutton_temp_hybrid_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	CALCULATOR->setTemperatureCalculationMode(TEMPERATURE_CALCULATION_HYBRID);
	tc_set = true;
	expression_calculation_updated();
}
void preferences_update_temperature_calculation(bool initial) {
	if(!preferences_builder) return;
	if(!initial) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_abs_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_rel_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_hybrid_toggled, NULL);
	}
	switch(CALCULATOR->getTemperatureCalculationMode()) {
		case TEMPERATURE_CALCULATION_ABSOLUTE: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs")), TRUE);
			break;
		}
		case TEMPERATURE_CALCULATION_RELATIVE: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel")), TRUE);
			break;
		}
		default: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid")), TRUE);
			break;
		}
	}
	if(!initial) {
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_abs_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_rel_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_hybrid_toggled, NULL);
	}
}
extern std::string custom_lang;
void on_preferences_combo_language_changed(GtkComboBox *w, gpointer) {
#ifdef _WIN32
	switch(gtk_combo_box_get_active(w)) {
		case 0: {custom_lang = ""; break;}
		case 1: {custom_lang = "ca"; break;}
		case 2: {custom_lang = "de"; break;}
		case 3: {custom_lang = "en"; break;}
		case 4: {custom_lang = "es"; break;}
		case 5: {custom_lang = "fr"; break;}
		case 6: {custom_lang = "hu"; break;}
		case 7: {custom_lang = "ka"; break;}
		case 8: {custom_lang = "nl"; break;}
		case 9: {custom_lang = "pt_PT"; break;}
		case 10: {custom_lang = "pt_BR"; break;}
		case 11: {custom_lang = "ru"; break;}
		case 12: {custom_lang = "sl"; break;}
		case 13: {custom_lang = "sv"; break;}
		case 14: {custom_lang = "zh_CN"; break;}
		case 15: {custom_lang = "zh_TW"; break;}
	}
#else
	switch(gtk_combo_box_get_active(w)) {
		case 0: {custom_lang = ""; break;}
		case 1: {custom_lang = "ca_ES.UTF8"; break;}
		case 2: {custom_lang = "de_DE.UTF8"; break;}
		case 3: {custom_lang = "en_US.UTF8"; break;}
		case 4: {custom_lang = "es_ES.UTF8"; break;}
		case 5: {custom_lang = "fr_FR.UTF8"; break;}
		case 6: {custom_lang = "hu_HU.UTF8"; break;}
		case 7: {custom_lang = "ka_GE.UTF8"; break;}
		case 8: {custom_lang = "nl_NL.UTF8"; break;}
		case 9: {custom_lang = "pt_PT.UTF8"; break;}
		case 10: {custom_lang = "pt_BR.UTF8"; break;}
		case 11: {custom_lang = "ru_RU.UTF8"; break;}
		case 12: {custom_lang = "sl_SI.UTF8"; break;}
		case 13: {custom_lang = "sv_SE.UTF8"; break;}
		case 14: {custom_lang = "zh_CN.UTF8"; break;}
		case 15: {custom_lang = "zh_TW.UTF8"; break;}
	}
#endif
	if(!custom_lang.empty()) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_ignore_locale")), false);
		ignore_locale = false;
	}
	show_message(_("Please restart the program for the language change to take effect."), GTK_WINDOW(gtk_builder_get_object(preferences_builder, "preferences_dialog")));
}
void on_preferences_checkbutton_ignore_locale_toggled(GtkToggleButton *w, gpointer) {
	ignore_locale = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), !ignore_locale);
	if(ignore_locale) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_combo_language"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_combo_language_changed, NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 0);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_combo_language"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_combo_language_changed, NULL);
		custom_lang = "";
	}
}
extern int title_type;
extern bool title_modified;
void on_preferences_combo_title_changed(GtkComboBox *w, gpointer) {
	title_type = gtk_combo_box_get_active(w);
	title_modified = false;
	update_window_title();
}
extern int history_expression_type;
void on_preferences_combo_history_expression_changed(GtkComboBox *w, gpointer) {
	history_expression_type = gtk_combo_box_get_active(w);
	reload_history();
}
void on_preferences_checkbutton_copy_ascii_toggled(GtkToggleButton *w, gpointer) {
	copy_ascii = gtk_toggle_button_get_active(w);
	update_accels(SHORTCUT_TYPE_COPY_RESULT);
}
void on_preferences_checkbutton_copy_ascii_without_units_toggled(GtkToggleButton *w, gpointer) {
	copy_ascii_without_units = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_repdeci_overline_toggled(GtkToggleButton *w, gpointer) {
	if(printops.indicate_infinite_series) {
		if(gtk_toggle_button_get_active(w)) printops.indicate_infinite_series = REPEATING_DECIMALS_OVERLINE;
		else printops.indicate_infinite_series = REPEATING_DECIMALS_ELLIPSIS;
		result_format_updated();
	} else {
		repdeci_overline = gtk_toggle_button_get_active(w);
	}
}
void on_preferences_checkbutton_lower_case_numbers_toggled(GtkToggleButton *w, gpointer) {
	printops.lower_case_numbers = gtk_toggle_button_get_active(w);
	result_format_updated();
}
void on_preferences_checkbutton_lower_case_e_toggled(GtkToggleButton *w, gpointer) {
	if(printops.exp_display != EXP_POWER_OF_10) {
		if(gtk_toggle_button_get_active(w)) printops.exp_display = EXP_LOWERCASE_E;
		else printops.exp_display = EXP_UPPERCASE_E;
		result_format_updated();
	}
}
void on_preferences_checkbutton_imaginary_j_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w) != (CALCULATOR->v_i->hasName("j") > 0)) {
		if(gtk_toggle_button_get_active(w)) {
			ExpressionName ename = CALCULATOR->v_i->getName(1);
			ename.name = "j";
			ename.reference = false;
			CALCULATOR->v_i->addName(ename, 1, true);
			CALCULATOR->v_i->setChanged(false);
		} else {
			CALCULATOR->v_i->clearNonReferenceNames();
			CALCULATOR->v_i->setChanged(false);
		}
		update_keypad_i();
		expression_format_updated();
	}
}
void on_preferences_checkbutton_e_notation_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		printops.exp_display = EXP_POWER_OF_10;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_e")))) {
		printops.exp_display = EXP_LOWERCASE_E;
	} else {
		printops.exp_display = EXP_UPPERCASE_E;
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_e")), printops.exp_display != EXP_POWER_OF_10);
	result_format_updated();
}
void on_preferences_checkbutton_alternative_base_prefixes_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) printops.base_display = BASE_DISPLAY_ALTERNATIVE;
	else printops.base_display = BASE_DISPLAY_NORMAL;
	result_format_updated();
}
void on_preferences_checkbutton_duodecimal_symbols_toggled(GtkToggleButton *w, gpointer) {
	printops.duodecimal_symbols = gtk_toggle_button_get_active(w);
	result_format_updated();
}
void on_preferences_checkbutton_twos_complement_toggled(GtkToggleButton *w, gpointer) {
	set_twos_complement(gtk_toggle_button_get_active(w), -1, -1, -1, false);
}
void on_preferences_checkbutton_hexadecimal_twos_complement_toggled(GtkToggleButton *w, gpointer) {
	set_twos_complement(-1, gtk_toggle_button_get_active(w), -1, -1, false);
}
void on_preferences_checkbutton_twos_complement_input_toggled(GtkToggleButton *w, gpointer) {
	set_twos_complement(-1, -1, gtk_toggle_button_get_active(w), -1, false);
}
void on_preferences_checkbutton_hexadecimal_twos_complement_input_toggled(GtkToggleButton *w, gpointer) {
	set_twos_complement(-1, -1, -1, gtk_toggle_button_get_active(w), false);
}
void on_preferences_combobox_bits_changed(GtkComboBox *w, gpointer) {
	set_binary_bits(combo_get_bits(w), false);
}
void preferences_update_twos_complement(bool initial) {
	if(!preferences_builder) return;
	if(!initial) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_twos_complement_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_hexadecimal_twos_complement_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_combobox_bits"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_combobox_bits_changed, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement_input"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_twos_complement_input_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement_input"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_hexadecimal_twos_complement_input_toggled, NULL);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement")), printops.twos_complement);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement")), printops.hexadecimal_twos_complement);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement_input")), evalops.parse_options.twos_complement);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement_input")), evalops.parse_options.hexadecimal_twos_complement);
	combo_set_bits(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combobox_bits")), printops.binary_bits);
	if(!initial) {
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement_input"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_twos_complement_input_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement_input"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_hexadecimal_twos_complement_input_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_combobox_bits"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_combobox_bits_changed, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_twos_complement_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_hexadecimal_twos_complement_toggled, NULL);
	}
}
void on_preferences_checkbutton_spell_out_logical_operators_toggled(GtkToggleButton *w, gpointer) {
	printops.spell_out_logical_operators = gtk_toggle_button_get_active(w);
	result_display_updated();
}
void on_preferences_checkbutton_caret_as_xor_toggled(GtkToggleButton *w, gpointer) {
	caret_as_xor = gtk_toggle_button_get_active(w);
	update_keypad_caret_as_xor();
}
void on_preferences_checkbutton_close_with_esc_toggled(GtkToggleButton *w, gpointer) {
	close_with_esc = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_unicode_signs_toggled(GtkToggleButton *w, gpointer) {
	printops.use_unicode_signs = gtk_toggle_button_get_active(w);
	set_expression_operator_symbols();
	set_status_operator_symbols();
	set_app_operator_symbols();
	update_keypad_button_text();
	update_stack_button_text();
	update_history_button_text();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_asterisk")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_ex")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_dot")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_altdot")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_slash")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division_slash")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division")), printops.use_unicode_signs);
	result_display_updated();
}

extern bool save_defs_on_exit;
void on_preferences_checkbutton_save_defs_toggled(GtkToggleButton *w, gpointer) {
	save_defs_on_exit = gtk_toggle_button_get_active(w);
}
extern bool save_mode_on_exit;
void on_preferences_checkbutton_save_mode_toggled(GtkToggleButton *w, gpointer) {
	save_mode_on_exit = gtk_toggle_button_get_active(w);
}
extern int allow_multiple_instances;
void on_preferences_checkbutton_allow_multiple_instances_toggled(GtkToggleButton *w, gpointer) {
	allow_multiple_instances = gtk_toggle_button_get_active(w);
	save_preferences(false);
}
void on_preferences_checkbutton_rpn_keys_toggled(GtkToggleButton *w, gpointer) {
	rpn_keys = gtk_toggle_button_get_active(w);
}
extern bool dot_question_asked;
extern int b_decimal_comma;
void on_preferences_checkbutton_decimal_comma_toggled(GtkToggleButton *w, gpointer) {
	b_decimal_comma = gtk_toggle_button_get_active(w);
	if(b_decimal_comma) {
		CALCULATOR->useDecimalComma();
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")));
	} else {
		CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")));
	}
	dot_question_asked = true;
	expression_format_updated(false);
	result_display_updated();
	update_keypad_button_text();
}
void on_preferences_checkbutton_dot_as_separator_toggled(GtkToggleButton *w, gpointer) {
	evalops.parse_options.dot_as_separator = gtk_toggle_button_get_active(w);
	dot_question_asked = true;
	expression_format_updated(false);
}
void on_preferences_checkbutton_comma_as_separator_toggled(GtkToggleButton *w, gpointer) {
	evalops.parse_options.comma_as_separator = gtk_toggle_button_get_active(w);
	CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
	update_keypad_button_text();
	dot_question_asked = true;
	expression_format_updated(false);
}
void preferences_update_dot(bool initial) {
	if(!preferences_builder) return;
	if(!initial) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_dot_as_separator_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_comma_as_separator_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_decimal_comma_toggled, NULL);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma")), CALCULATOR->getDecimalPoint() == COMMA);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")), evalops.parse_options.dot_as_separator);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")), evalops.parse_options.comma_as_separator);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")), CALCULATOR->getDecimalPoint() != DOT);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")), CALCULATOR->getDecimalPoint() != COMMA);
	if(!initial) {
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_dot_as_separator_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_comma_as_separator_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_decimal_comma_toggled, NULL);
	}
}
void on_preferences_checkbutton_load_defs_toggled(GtkToggleButton *w, gpointer) {
	load_global_defs = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_display_expression_status_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		display_expression_status = true;
		display_parse_status();
	} else {
		display_expression_status = false;
		if(parsed_in_result) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), false);
		}
		clear_status_text();
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), display_expression_status);
}
void on_preferences_checkbutton_parsed_in_result_toggled(GtkToggleButton *w, gpointer) {
	set_parsed_in_result(gtk_toggle_button_get_active(w));
}
extern int autocalc_history_delay;
void on_preferences_scale_autocalc_history_value_changed(GtkRange *w, gpointer) {
	autocalc_history_delay = (gint) ::round(::pow(gtk_range_get_value(w), 3.0));
}
void on_preferences_checkbutton_autocalc_history_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		autocalc_history_delay = (gint) ::round(::pow(gtk_range_get_value(GTK_RANGE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history"))), 3.0));
	} else {
		autocalc_history_delay = -1;
		stop_autocalculate_history_timeout();
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), autocalc_history_delay >= 0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "label_autocalc_history")), autocalc_history_delay >= 0);
}
void preferences_update_expression_status() {
	if(!preferences_builder) return;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_display_expression_status"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_display_expression_status_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_display_expression_status")), display_expression_status);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_display_expression_status"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_display_expression_status_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_parsed_in_result_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), parsed_in_result);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_parsed_in_result_toggled, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), display_expression_status);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_autocalc_history"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_autocalc_history_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_autocalc_history")), autocalc_history_delay >= 0 && !parsed_in_result);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_autocalc_history"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_autocalc_history_toggled, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_autocalc_history")), !parsed_in_result);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), autocalc_history_delay >= 0 && !parsed_in_result);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "label_autocalc_history")), autocalc_history_delay >= 0 && !parsed_in_result);
}
extern int gtk_theme;
void on_preferences_combo_theme_changed(GtkComboBox *w, gpointer) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	if(!app_provider_theme) {
		app_provider_theme = gtk_css_provider_new();
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(app_provider_theme), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
	gtk_theme = gtk_combo_box_get_active(w) - 1;
	switch(gtk_theme) {
		case 0: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/Adwaita/gtk-contained.css"); break;}
		case 1: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/Adwaita/gtk-contained-dark.css"); break;}
		case 2: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/HighContrast/gtk-contained.css"); break;}
		case 3: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/HighContrast/gtk-contained-inverse.css"); break;}
		default: {gtk_css_provider_load_from_data(app_provider_theme, "", -1, NULL);}
	}
	update_colors(false);
	reload_history();
	GdkRGBA c;
	gdk_rgba_parse(&c, text_color.c_str());
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_text_color")), &c);
	gdk_rgba_parse(&c, status_error_color());
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_status_error_color")), &c);
	gdk_rgba_parse(&c, status_warning_color());
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_status_warning_color")), &c);
#endif
}
void on_preferences_checkbutton_disable_cursor_blinking_toggled(GtkToggleButton *w, gpointer) {
	set_disable_cursor_blinking(gtk_toggle_button_get_active(w));
}
#ifdef _WIN32
void on_preferences_checkbutton_use_systray_icon_toggled(GtkToggleButton *w, gpointer) {
	bool b = gtk_toggle_button_get_active(w);
	set_system_tray_icon_enabled(b);
	if(b) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), FALSE);
	}
	if(close_with_esc < 0) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_close_with_esc"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_close_with_esc_toggled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_close_with_esc")), b);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_close_with_esc"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_close_with_esc_toggled, NULL);
	}
}
#else
void on_preferences_checkbutton_use_systray_icon_toggled(GtkToggleButton*, gpointer) {}
#endif
extern bool hide_on_startup;
void on_preferences_checkbutton_hide_on_startup_toggled(GtkToggleButton *w, gpointer) {
	hide_on_startup = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_custom_result_font_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_result_font(result_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), TRUE);
	} else {
		set_result_font(NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), FALSE);
	}
}
void on_preferences_checkbutton_custom_expression_font_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_expression_font(expression_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), TRUE);
	} else {
		set_expression_font(NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), FALSE);
	}
}
void on_preferences_checkbutton_custom_status_font_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_status_font(status_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), TRUE);
	} else {
		set_status_font(NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), FALSE);
	}
}
void on_preferences_checkbutton_custom_history_font_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_history_font(history_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_history_font")), TRUE);
	} else {
		set_history_font(NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_history_font")), FALSE);
	}
	update_stack_font();
}
void on_preferences_checkbutton_custom_keypad_font_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_keypad_font(keypad_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_app_font")), FALSE);
	} else {
		set_keypad_font(NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), FALSE);
	}
	update_stack_button_font();
}
void on_preferences_checkbutton_custom_app_font_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_app_font(app_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_keypad_font")), FALSE);
	} else {
		set_app_font(NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), FALSE);
	}
}
void on_preferences_radiobutton_dot_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_DOT;
		result_display_updated();
	}
}
void on_preferences_radiobutton_altdot_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_ALTDOT;
		result_display_updated();
	}
}
void on_preferences_radiobutton_ex_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_X;
		result_display_updated();
	}
}
void on_preferences_radiobutton_asterisk_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_ASTERISK;
		result_display_updated();
	}
}
void on_preferences_radiobutton_slash_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.division_sign = DIVISION_SIGN_SLASH;
		result_display_updated();
	}
}
void on_preferences_radiobutton_division_slash_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.division_sign = DIVISION_SIGN_DIVISION_SLASH;
		result_display_updated();
	}
}
void on_preferences_radiobutton_division_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.division_sign = DIVISION_SIGN_DIVISION;
		result_display_updated();
	}
}
void on_preferences_radiobutton_digit_grouping_none_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.digit_grouping = DIGIT_GROUPING_NONE;
		result_format_updated();
	}
}
void on_preferences_radiobutton_digit_grouping_standard_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.digit_grouping = DIGIT_GROUPING_STANDARD;
		result_format_updated();
	}
}
void on_preferences_radiobutton_digit_grouping_locale_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.digit_grouping = DIGIT_GROUPING_LOCALE;
		if((!evalops.parse_options.comma_as_separator || CALCULATOR->getDecimalPoint() == COMMA) && CALCULATOR->local_digit_group_separator == COMMA) {
			evalops.parse_options.comma_as_separator = true;
			evalops.parse_options.dot_as_separator = false;
			CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
			update_keypad_button_text();
			dot_question_asked = true;
			expression_format_updated(false);
			preferences_update_dot();
		} else if((!evalops.parse_options.dot_as_separator || CALCULATOR->getDecimalPoint() == DOT) && CALCULATOR->local_digit_group_separator == DOT) {
			evalops.parse_options.dot_as_separator = true;
			evalops.parse_options.comma_as_separator = false;
			CALCULATOR->useDecimalComma();
			update_keypad_button_text();
			dot_question_asked = true;
			expression_format_updated(false);
			preferences_update_dot();
		} else {
			result_format_updated();
		}
	}
}

void on_preferences_checkbutton_enable_completion_toggled(GtkToggleButton *w, gpointer) {
	bool b = gtk_toggle_button_get_active(w);
	set_expression_completion_settings(b);
	bool b2 = false;
	get_expression_completion_settings(NULL, &b2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min")), b);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min")), b);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2")), b);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min2")), b && b2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), b && b2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_delay")), b);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay")), b);
}
void on_preferences_checkbutton_enable_completion2_toggled(GtkToggleButton *w, gpointer) {
	bool b2 = gtk_toggle_button_get_active(w);
	set_expression_completion_settings(-1, b2);
	bool b = false;
	get_expression_completion_settings(&b);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min2")), b && b2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), b && b2);
}
void on_preferences_spin_completion_min_value_changed(GtkSpinButton *spin, gpointer) {
	set_expression_completion_settings(-1, -1, gtk_spin_button_get_value_as_int(spin));
	int i2 = 0;
	get_expression_completion_settings(NULL, NULL, NULL, &i2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), (double) i2);
}
void on_preferences_spin_completion_min2_value_changed(GtkSpinButton *spin, gpointer) {
	set_expression_completion_settings(-1, -1, -1, gtk_spin_button_get_value_as_int(spin));
	int i = 0;
	get_expression_completion_settings(NULL, NULL, &i);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min")), (double) i);
}
void on_preferences_spin_completion_delay_value_changed(GtkSpinButton *spin, gpointer) {
	set_expression_completion_settings(-1, -1, -1, -1, gtk_spin_button_get_value_as_int(spin));
}
void preferences_update_completion(bool initial) {
	bool c1 = false, c2 = false;
	int m1 = 0, m2 = 0, d = 0;
	get_expression_completion_settings(&c1, &c2, &m1, &m2, &d);
	if(!initial) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_enable_completion_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_enable_completion2_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_spin_completion_min_value_changed, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_spin_completion_min2_value_changed, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_spin_completion_delay_value_changed, NULL);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion")), c1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2")), c2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min")), c1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min")), c1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min")), (double) m1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2")), c1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min2")), c1 && c2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), c1 && c2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), (double) m2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_delay")), c1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay")), c2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay")), (double) d);
	if(!initial) {
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_enable_completion_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_enable_completion2_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_spin_completion_min_value_changed, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_spin_completion_min2_value_changed, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_spin_completion_delay_value_changed, NULL);
	}
}

void preferences_rpn_mode_changed() {
	if(!preferences_builder) return;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_parsed_in_result_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), parsed_in_result && !rpn_mode);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_parsed_in_result_toggled, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), display_expression_status && !rpn_mode);
}
void preferences_parsing_mode_changed() {
	if(!preferences_builder) return;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_rpn_keys_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys")), rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_rpn_keys_toggled, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys")), evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
}
void on_preferences_button_result_font_font_set(GtkFontButton *w, gpointer) {
	set_result_font(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w)));
}
void on_preferences_button_expression_font_font_set(GtkFontButton *w, gpointer) {
	set_expression_font(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w)));
}
void on_preferences_button_status_font_font_set(GtkFontButton *w, gpointer) {
	set_status_font(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w)));
}
void on_preferences_button_keypad_font_font_set(GtkFontButton *w, gpointer) {
	set_keypad_font(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w)));
	update_stack_button_font();
}
void on_preferences_button_history_font_font_set(GtkFontButton *w, gpointer) {
	set_history_font(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w)));
	update_stack_font();
}
void on_preferences_button_app_font_font_set(GtkFontButton *w, gpointer) {
	set_app_font(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w)));
}

GtkWidget* get_preferences_dialog() {
	if(!preferences_builder) {

		preferences_builder = getBuilder("preferences.ui");
		g_assert(preferences_builder != NULL);

		g_assert(gtk_builder_get_object(preferences_builder, "preferences_dialog") != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_display_expression_status")), display_expression_status);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), parsed_in_result && !rpn_mode);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), display_expression_status && !rpn_mode);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_expression_lines_spin_button")), (double) (expression_lines < 1 ? 3 : expression_lines));
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_vertical_padding_combo")), vertical_button_padding() > 9 ? 9 : vertical_button_padding() + 1);
		if(horizontal_button_padding() > 4) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_horizontal_padding_combo")), horizontal_button_padding() > 12 ? 9 : (horizontal_button_padding() - 4) / 2 + 4 + 1);
		else gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_horizontal_padding_combo")), horizontal_button_padding() + 1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_local_currency_conversion")), evalops.local_currency_conversion);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_save_mode")), save_mode_on_exit);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_clear_history")), clear_history_on_exit);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_max_history_lines_spin_button")), (double) max_history_lines);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_save_history_separately")), save_history_separately);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_allow_multiple_instances")), allow_multiple_instances > 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_ignore_locale")), ignore_locale);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_check_version")), check_version);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_remember_position")), remember_position);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_keep_above")), always_on_top);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_persistent_keypad")), persistent_keypad);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_tooltips")), enable_tooltips == 0 ? 2 : (enable_tooltips == 1 ? 0 : 1));
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_title")), title_type);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_history_expression")), history_expression_type);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_unicode_signs")), printops.use_unicode_signs);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_copy_ascii")), copy_ascii);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_copy_ascii_without_units")), copy_ascii_without_units);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_numbers")), printops.lower_case_numbers);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_duodecimal_symbols")), printops.duodecimal_symbols);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_e_notation")), printops.exp_display == EXP_UPPERCASE_E || printops.exp_display == EXP_LOWERCASE_E);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_e")), printops.exp_display == EXP_LOWERCASE_E);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_e")), printops.exp_display != EXP_POWER_OF_10);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_imaginary_j")), CALCULATOR->v_i->hasName("j") > 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_alternative_base_prefixes")), printops.base_display == BASE_DISPLAY_ALTERNATIVE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_repdeci_overline")), repdeci_overline || printops.indicate_infinite_series == REPEATING_DECIMALS_OVERLINE);
		if(pango_version() < 14600) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_repdeci_overline")));
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_history_expression")), history_expression_type);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_spell_out_logical_operators")), printops.spell_out_logical_operators);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_caret_as_xor")), caret_as_xor);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_close_with_esc")), close_with_esc > 0 || (close_with_esc < 0 && system_tray_icon_enabled()));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_binary_prefixes")), CALCULATOR->usesBinaryPrefixes() > 0);
		preferences_update_temperature_calculation(true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_save_defs")), save_defs_on_exit);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys")), rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys")), evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
		preferences_update_dot(true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_result_font")), result_font() != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_expression_font")), expression_font() != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_status_font")), status_font() != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_keypad_font")), keypad_font() != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_history_font")), history_font() != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_app_font")), app_font() != NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), result_font() != NULL);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), result_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), expression_font() != NULL);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), expression_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), status_font() != NULL);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), status_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), keypad_font() != NULL);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), keypad_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_history_font")), history_font() != NULL);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_history_font")), history_font(true));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), app_font() != NULL);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), app_font(true));
		GdkRGBA c;
		gdk_rgba_parse(&c, text_color.c_str());
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_text_color")), &c);
		gdk_rgba_parse(&c, status_error_color());
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_status_error_color")), &c);
		gdk_rgba_parse(&c, status_warning_color());
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_status_warning_color")), &c);
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_dot")), SIGN_MULTIDOT);
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_altdot")), SIGN_MIDDLEDOT);
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_ex")), SIGN_MULTIPLICATION);
		switch(printops.multiplication_sign) {
			case MULTIPLICATION_SIGN_DOT: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_dot")), TRUE);
				break;
			}
			case MULTIPLICATION_SIGN_ALTDOT: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_altdot")), TRUE);
				break;
			}
			case MULTIPLICATION_SIGN_X: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_ex")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_asterisk")), TRUE);
				break;
			}
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_asterisk")), printops.use_unicode_signs);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_ex")), printops.use_unicode_signs);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_dot")), printops.use_unicode_signs);
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division_slash")), " " SIGN_DIVISION_SLASH " ");
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division")), SIGN_DIVISION);
		switch(printops.division_sign) {
			case DIVISION_SIGN_DIVISION_SLASH: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division_slash")), TRUE);
				break;
			}
			case DIVISION_SIGN_DIVISION: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_slash")), TRUE);
				break;
			}
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_slash")), printops.use_unicode_signs);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division_slash")), printops.use_unicode_signs);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division")), printops.use_unicode_signs);
		switch(printops.digit_grouping) {
			case DIGIT_GROUPING_STANDARD: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_digit_grouping_standard")), TRUE);
				break;
			}
			case DIGIT_GROUPING_LOCALE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_digit_grouping_locale")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_digit_grouping_none")), TRUE);
				break;
			}
		}
		preferences_update_twos_complement(true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_autocalc_history")), autocalc_history_delay >= 0 && !parsed_in_result);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_autocalc_history")), !parsed_in_result);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), autocalc_history_delay >= 0 && !parsed_in_result);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "label_autocalc_history")), autocalc_history_delay >= 0 && !parsed_in_result);
		Number nr(autocalc_history_delay); nr.cbrt();
		gtk_range_set_value(GTK_RANGE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), autocalc_history_delay < 0 ? 12.599 : nr.floatValue());
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 6.3, GTK_POS_BOTTOM, NULL);
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 7.937, GTK_POS_BOTTOM, (string("0") + CALCULATOR->getDecimalPoint() + "5 s").c_str());
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 10.0, GTK_POS_BOTTOM, "1 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 11.447, GTK_POS_BOTTOM, NULL);
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 12.599, GTK_POS_BOTTOM, "2 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 14.422, GTK_POS_BOTTOM, NULL);
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 15.874, GTK_POS_BOTTOM, NULL);
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 17.1, GTK_POS_BOTTOM, "5 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_autocalc_history")), 21.544, GTK_POS_BOTTOM, "10 s");

		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 0.0, GTK_POS_BOTTOM, "1 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 2.0, GTK_POS_BOTTOM, "5 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 3.0, GTK_POS_BOTTOM, "10 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 4.0, GTK_POS_BOTTOM, "20 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 5.58, GTK_POS_BOTTOM, "60 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 7.17, GTK_POS_BOTTOM, "180 s");
		gtk_scale_add_mark(GTK_SCALE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), 8.91, GTK_POS_BOTTOM, "600 s");
		nr.set(max_plot_time); nr.log(2);
		gtk_range_set_value(GTK_RANGE(gtk_builder_get_object(preferences_builder, "preferences_scale_plot_time")), nr.floatValue() - 0.322);
		string lang = custom_lang;
		if(lang.length() > 2) lang = lang.substr(0, 2);
		if(lang == "ca") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 1);
		else if(lang == "de") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 2);
		else if(lang == "en") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 3);
		else if(lang == "es") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 4);
		else if(lang == "fr") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 5);
		else if(lang == "hu") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 6);
		else if(lang == "ka") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 7);
		else if(lang == "nl") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 8);
		else if(lang == "pt" && custom_lang.length() >= 5 && custom_lang.substr(0, 5) == "pt_PT") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 9);
		else if(lang == "pt") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 10);
		else if(lang == "ru") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 11);
		else if(lang == "sl") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 12);
		else if(lang == "sv") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 13);
		else if(custom_lang.length() >= 5 && custom_lang.substr(0, 5) == "zh_CN") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 14);
		else if(custom_lang.length() >= 5 && custom_lang.substr(0, 5) == "zh_TW") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 15);
		else gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 0);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), !ignore_locale);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_theme")), gtk_theme < 0 ? 0 : gtk_theme + 1);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_theme")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_theme")));
#endif
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_use_systray_icon")), system_tray_icon_enabled());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), hide_on_startup);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), system_tray_icon_enabled());
#ifdef _WIN32
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_use_systray_icon")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")));
#endif
		preferences_update_completion(true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_update_exchange_rates_spin_button")), (double) exchange_rates_frequency());

		gtk_builder_add_callback_symbols(preferences_builder, "on_preferences_checkbutton_save_defs_toggled", G_CALLBACK(on_preferences_checkbutton_save_defs_toggled), "on_preferences_checkbutton_clear_history_toggled", G_CALLBACK(on_preferences_checkbutton_clear_history_toggled), "on_preferences_max_history_lines_spin_button_value_changed",
		G_CALLBACK(on_preferences_max_history_lines_spin_button_value_changed), "on_preferences_checkbutton_save_history_separately_toggled",
		G_CALLBACK(on_preferences_checkbutton_save_history_separately_toggled), "on_preferences_checkbutton_allow_multiple_instances_toggled",
		G_CALLBACK(on_preferences_checkbutton_allow_multiple_instances_toggled), "on_preferences_checkbutton_check_version_toggled", G_CALLBACK(on_preferences_checkbutton_check_version_toggled), "on_preferences_checkbutton_save_mode_toggled", G_CALLBACK(on_preferences_checkbutton_save_mode_toggled), "on_preferences_checkbutton_close_with_esc_toggled",
		G_CALLBACK(on_preferences_checkbutton_close_with_esc_toggled), "on_preferences_checkbutton_rpn_keys_toggled", G_CALLBACK(on_preferences_checkbutton_rpn_keys_toggled), "on_preferences_checkbutton_caret_as_xor_toggled", G_CALLBACK(on_preferences_checkbutton_caret_as_xor_toggled), "on_preferences_combo_history_expression_changed",
		G_CALLBACK(on_preferences_combo_history_expression_changed), "on_preferences_checkbutton_autocalc_history_toggled", G_CALLBACK(on_preferences_checkbutton_autocalc_history_toggled), "on_preferences_scale_autocalc_history_value_changed", G_CALLBACK(on_preferences_scale_autocalc_history_value_changed), "on_preferences_scale_plot_time_value_changed",
		G_CALLBACK(on_preferences_scale_plot_time_value_changed), "on_preferences_checkbutton_unicode_signs_toggled", G_CALLBACK(on_preferences_checkbutton_unicode_signs_toggled), "on_preferences_checkbutton_ignore_locale_toggled", G_CALLBACK(on_preferences_checkbutton_ignore_locale_toggled), "on_preferences_checkbutton_use_systray_icon_toggled",
		G_CALLBACK(on_preferences_checkbutton_use_systray_icon_toggled), "on_preferences_checkbutton_hide_on_startup_toggled", G_CALLBACK(on_preferences_checkbutton_hide_on_startup_toggled), "on_preferences_checkbutton_remember_position_toggled", G_CALLBACK(on_preferences_checkbutton_remember_position_toggled), "on_preferences_checkbutton_keep_above_toggled",
		G_CALLBACK(on_preferences_checkbutton_keep_above_toggled), "on_preferences_horizontal_padding_combo_changed", G_CALLBACK(on_preferences_horizontal_padding_combo_changed), "on_preferences_vertical_padding_combo_changed", G_CALLBACK(on_preferences_vertical_padding_combo_changed), "on_preferences_combo_title_changed", G_CALLBACK(on_preferences_combo_title_changed),
		"on_preferences_combo_theme_changed", G_CALLBACK(on_preferences_combo_theme_changed), "on_preferences_combo_language_changed", G_CALLBACK(on_preferences_combo_language_changed), "on_preferences_combo_tooltips_changed", G_CALLBACK(on_preferences_combo_tooltips_changed), "on_preferences_expression_lines_spin_button_value_changed",
		G_CALLBACK(on_preferences_expression_lines_spin_button_value_changed), "on_preferences_checkbutton_display_expression_status_toggled", G_CALLBACK(on_preferences_checkbutton_display_expression_status_toggled), "on_preferences_checkbutton_parsed_in_result_toggled", G_CALLBACK(on_preferences_checkbutton_parsed_in_result_toggled),
		"on_preferences_checkbutton_persistent_keypad_toggled", G_CALLBACK(on_preferences_checkbutton_persistent_keypad_toggled), "on_preferences_checkbutton_twos_complement_toggled", G_CALLBACK(on_preferences_checkbutton_twos_complement_toggled), "on_preferences_checkbutton_hexadecimal_twos_complement_toggled",
		G_CALLBACK(on_preferences_checkbutton_hexadecimal_twos_complement_toggled), "on_preferences_checkbutton_twos_complement_input_toggled", G_CALLBACK(on_preferences_checkbutton_twos_complement_input_toggled), "on_preferences_checkbutton_hexadecimal_twos_complement_input_toggled",
		G_CALLBACK(on_preferences_checkbutton_hexadecimal_twos_complement_input_toggled), "on_preferences_combobox_bits_changed", G_CALLBACK(on_preferences_combobox_bits_changed),
		"on_preferences_checkbutton_lower_case_numbers_toggled", G_CALLBACK(on_preferences_checkbutton_lower_case_numbers_toggled), "on_preferences_checkbutton_duodecimal_symbols_toggled", G_CALLBACK(on_preferences_checkbutton_duodecimal_symbols_toggled), "on_preferences_checkbutton_alternative_base_prefixes_toggled",
		G_CALLBACK(on_preferences_checkbutton_alternative_base_prefixes_toggled), "on_preferences_checkbutton_spell_out_logical_operators_toggled", G_CALLBACK(on_preferences_checkbutton_spell_out_logical_operators_toggled), "on_preferences_checkbutton_e_notation_toggled", G_CALLBACK(on_preferences_checkbutton_e_notation_toggled),
		"on_preferences_checkbutton_lower_case_e_toggled", G_CALLBACK(on_preferences_checkbutton_lower_case_e_toggled), "on_preferences_checkbutton_repdeci_overline_toggled", G_CALLBACK(on_preferences_checkbutton_repdeci_overline_toggled), "on_preferences_checkbutton_decimal_comma_toggled", G_CALLBACK(on_preferences_checkbutton_decimal_comma_toggled), "on_preferences_checkbutton_imaginary_j_toggled", G_CALLBACK(on_preferences_checkbutton_imaginary_j_toggled),
		"on_preferences_checkbutton_comma_as_separator_toggled", G_CALLBACK(on_preferences_checkbutton_comma_as_separator_toggled), "on_preferences_checkbutton_copy_ascii_toggled", G_CALLBACK(on_preferences_checkbutton_copy_ascii_toggled), "on_preferences_checkbutton_dot_as_separator_toggled", G_CALLBACK(on_preferences_checkbutton_dot_as_separator_toggled),
		"on_preferences_radiobutton_digit_grouping_none_toggled", G_CALLBACK(on_preferences_radiobutton_digit_grouping_none_toggled), "on_preferences_radiobutton_digit_grouping_standard_toggled", G_CALLBACK(on_preferences_radiobutton_digit_grouping_standard_toggled), "on_preferences_radiobutton_digit_grouping_locale_toggled",
		G_CALLBACK(on_preferences_radiobutton_digit_grouping_locale_toggled), "on_preferences_radiobutton_dot_toggled", G_CALLBACK(on_preferences_radiobutton_dot_toggled), "on_preferences_radiobutton_ex_toggled", G_CALLBACK(on_preferences_radiobutton_ex_toggled), "on_preferences_radiobutton_altdot_toggled", G_CALLBACK(on_preferences_radiobutton_altdot_toggled),
		"on_preferences_radiobutton_asterisk_toggled", G_CALLBACK(on_preferences_radiobutton_asterisk_toggled), "on_preferences_radiobutton_division_slash_toggled", G_CALLBACK(on_preferences_radiobutton_division_slash_toggled), "on_preferences_radiobutton_division_toggled", G_CALLBACK(on_preferences_radiobutton_division_toggled),
		"on_preferences_radiobutton_slash_toggled", G_CALLBACK(on_preferences_radiobutton_slash_toggled), "on_preferences_checkbutton_binary_prefixes_toggled", G_CALLBACK(on_preferences_checkbutton_binary_prefixes_toggled), "on_preferences_checkbutton_copy_ascii_without_units_toggled",
		G_CALLBACK(on_preferences_checkbutton_copy_ascii_without_units_toggled), "on_preferences_checkbutton_local_currency_conversion_toggled", G_CALLBACK(on_preferences_checkbutton_local_currency_conversion_toggled), "on_preferences_update_exchange_rates_spin_button_input", G_CALLBACK(on_preferences_update_exchange_rates_spin_button_input),
		"on_preferences_update_exchange_rates_spin_button_output", G_CALLBACK(on_preferences_update_exchange_rates_spin_button_output), "on_preferences_update_exchange_rates_spin_button_value_changed", G_CALLBACK(on_preferences_update_exchange_rates_spin_button_value_changed), "on_preferences_radiobutton_temp_abs_toggled", G_CALLBACK(on_preferences_radiobutton_temp_abs_toggled),
		"on_preferences_radiobutton_temp_rel_toggled", G_CALLBACK(on_preferences_radiobutton_temp_rel_toggled), "on_preferences_radiobutton_temp_hybrid_toggled", G_CALLBACK(on_preferences_radiobutton_temp_hybrid_toggled), "on_preferences_checkbutton_enable_completion_toggled", G_CALLBACK(on_preferences_checkbutton_enable_completion_toggled),
		"on_preferences_checkbutton_enable_completion2_toggled", G_CALLBACK(on_preferences_checkbutton_enable_completion2_toggled), "on_preferences_spin_completion_min_value_changed", G_CALLBACK(on_preferences_spin_completion_min_value_changed), "on_preferences_spin_completion_min2_value_changed", G_CALLBACK(on_preferences_spin_completion_min2_value_changed),
		"on_preferences_spin_completion_delay_value_changed", G_CALLBACK(on_preferences_spin_completion_delay_value_changed), "on_preferences_checkbutton_custom_status_font_toggled", G_CALLBACK(on_preferences_checkbutton_custom_status_font_toggled), "on_preferences_button_status_font_font_set", G_CALLBACK(on_preferences_button_status_font_font_set),
		"on_preferences_button_expression_font_font_set", G_CALLBACK(on_preferences_button_expression_font_font_set), "on_preferences_checkbutton_custom_expression_font_toggled", G_CALLBACK(on_preferences_checkbutton_custom_expression_font_toggled), "on_preferences_checkbutton_custom_result_font_toggled", G_CALLBACK(on_preferences_checkbutton_custom_result_font_toggled),
		"on_preferences_button_result_font_font_set", G_CALLBACK(on_preferences_button_result_font_font_set), "on_preferences_checkbutton_custom_keypad_font_toggled", G_CALLBACK(on_preferences_checkbutton_custom_keypad_font_toggled), "on_preferences_button_keypad_font_font_set", G_CALLBACK(on_preferences_button_keypad_font_font_set),
		"on_colorbutton_status_warning_color_color_set", G_CALLBACK(on_colorbutton_status_warning_color_color_set), "on_colorbutton_status_error_color_color_set", G_CALLBACK(on_colorbutton_status_error_color_color_set), "on_colorbutton_text_color_color_set", G_CALLBACK(on_colorbutton_text_color_color_set), "on_preferences_button_app_font_font_set",
		G_CALLBACK(on_preferences_button_app_font_font_set), "on_preferences_button_history_font_font_set", G_CALLBACK(on_preferences_button_history_font_font_set), "on_preferences_checkbutton_custom_app_font_toggled", G_CALLBACK(on_preferences_checkbutton_custom_app_font_toggled), "on_preferences_checkbutton_custom_history_font_toggled",
		G_CALLBACK(on_preferences_checkbutton_custom_history_font_toggled), "on_preferences_checkbutton_disable_cursor_blinking_toggled",
		G_CALLBACK(on_preferences_checkbutton_disable_cursor_blinking_toggled), NULL);

		gtk_builder_connect_signals(preferences_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_dialog"));
}

void edit_preferences(GtkWindow *win, int tab) {
	GtkWidget *dialog = get_preferences_dialog();
	if(tab >= 0) gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(preferences_builder, "preferences_tabs")), tab);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), win);
	gtk_widget_show(dialog);
}

