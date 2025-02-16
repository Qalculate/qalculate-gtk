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
#include "insertfunctiondialog.h"
#include "setbasedialog.h"
#include "variableeditdialog.h"
#include "functioneditdialog.h"
#include "functionsdialog.h"
#include "unitsdialog.h"
#include "variablesdialog.h"
#include "historyview.h"
#include "expressionedit.h"
#include "expressioncompletion.h"
#include "preferencesdialog.h"
#include "resultview.h"
#include "keypad.h"
#include "menubar.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;


extern GtkBuilder *main_builder;

GtkWidget *keypad;

vector<custom_button> custom_buttons;

GtkCssProvider *button_padding_provider = NULL, *keypad_provider;

Unit *latest_button_unit = NULL, *latest_button_currency = NULL;
std::string latest_button_unit_pre, latest_button_currency_pre;
GtkWidget *item_factorize, *item_simplify;

GtkWidget *result_bases;
string result_bin, result_oct, result_dec, result_hex;
bool result_bases_approx = false;
Number max_bases, min_bases;
int two_result_bases_rows = -1;

bool use_custom_keypad_font = false, save_custom_keypad_font = false;
string custom_keypad_font;
#ifdef _WIN32
int horizontal_button_padd = 6;
#else
int horizontal_button_padd = -1;
#endif
int vertical_button_padd = -1;
int visible_keypad = 0, previous_keypad = 0;
int programming_inbase = 0, programming_outbase = 0;
bool versatile_exact = false;
extern int to_base;
extern unsigned int to_bits;
extern GtkListStore *completion_store;
extern GtkTreeModel *completion_sort;
extern vector<MathStructure*> history_answer;

GtkWidget *keypad_widget() {
	if(!keypad) keypad = GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"));
	return keypad;
}

#define DO_CUSTOM_BUTTONS(i) \
	if(b2 && custom_buttons[i].type[2] != -1) {\
		if(custom_buttons[i].type[2] < 0) return FALSE;\
		do_shortcut(custom_buttons[i].type[2], custom_buttons[i].value[2]);\
		return TRUE;\
	} else if(!b2 && custom_buttons[i].type[1] != -1) {\
		if(custom_buttons[i].type[1] < 0) return FALSE;\
		do_shortcut(custom_buttons[i].type[1], custom_buttons[i].value[1]);\
		return TRUE;\
	}

#define DO_CUSTOM_BUTTONS_CX(i) DO_CUSTOM_BUTTONS(28 + i)

#define ADD_MB_TO_ITEM(i) \
	gchar *gstr = NULL;\
	cairo_surface_t *surface = NULL;\
	gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 5, &surface, 9, &gstr, -1);\
	if(!gstr) gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 1, &gstr, -1);\
	if(gstr) {\
		if(i > 1 && n2 == 0 && n > 0) {MENU_SEPARATOR}\
		if(surface) {\
			MENU_ITEM_MARKUP_WITH_INT_AND_FLAGIMAGE(gstr, surface, on_mb_to_activated, index)\
			cairo_surface_destroy(surface);\
		} else {\
			MENU_ITEM_MARKUP_WITH_INT(gstr, on_mb_to_activated, index)\
		}\
		g_free(gstr);\
		if(i == 1) n++;\
		else n2++;\
	}

void initialize_custom_buttons() {
	custom_buttons.resize(49);
	for(size_t i = 0; i < 49; i++) {
		custom_buttons[i].type[0] = -1;
		custom_buttons[i].type[1] = -1;
		custom_buttons[i].type[2] = -1;
		custom_buttons[i].value[0] = "";
		custom_buttons[i].value[1] = "";
		custom_buttons[i].value[2] = "";
		custom_buttons[i].text = "";
	}
}

bool read_keypad_settings_line(string &svar, string &svalue, int &v) {
	if(custom_buttons.empty()) initialize_custom_buttons();
	if(svar == "horizontal_button_padding") {
		horizontal_button_padd = v;
	} else if(svar == "vertical_button_padding") {
		vertical_button_padd = v;
	} else if(svar == "use_custom_keypad_font") {
		use_custom_keypad_font = v;
	} else if(svar == "custom_keypad_font") {
		custom_keypad_font = svalue;
		save_custom_keypad_font = true;
	} else if(svar == "programming_outbase") {
		programming_outbase = v;
	} else if(svar == "programming_inbase") {
		programming_inbase = v;
	} else if(svar == "general_exact") {
		versatile_exact = v;
	} else if(svar == "latest_button_unit") {
		latest_button_unit_pre = svalue;
	} else if(svar == "latest_button_currency") {
		latest_button_currency_pre = svalue;
	} else if(svar == "custom_button_label") {
		unsigned int index = 0;
		char str[20];
		int n = sscanf(svalue.c_str(), "%u:%19[^\n]", &index, str);
		if(n >= 2 && index < custom_buttons.size()) {
			custom_buttons[index].text = str;
			remove_blank_ends(custom_buttons[index].text);
		}
	} else if(svar == "custom_button") {
		unsigned int index = 0;
		unsigned int bi = 0;
		char str[20];
		int type = -1;
		int n = sscanf(svalue.c_str(), "%u:%u:%i:%19[^\n]", &index, &bi, &type, str);
		if(n >= 3 && index < custom_buttons.size()) {
			if(bi <= 2) {
				string value;
				if(n >= 4) {
					value = str;
					if(type != SHORTCUT_TYPE_TEXT) remove_blank_ends(value);
				}
				custom_buttons[index].type[bi] = type;
				custom_buttons[index].value[bi] = value;
			}
		}
	} else {
		return false;
	}
	return true;
}
void write_keypad_settings(FILE *file) {
	if(~visible_keypad & PROGRAMMING_KEYPAD && programming_outbase != 0 && programming_inbase != 0) {
		fprintf(file, "programming_outbase=%i\n", programming_outbase);
		fprintf(file, "programming_inbase=%i\n", programming_inbase);
	}
	if(visible_keypad & PROGRAMMING_KEYPAD && versatile_exact) {
		fprintf(file, "general_exact=%i\n", versatile_exact);
	}
	fprintf(file, "vertical_button_padding=%i\n", vertical_button_padd);
	fprintf(file, "horizontal_button_padding=%i\n", horizontal_button_padd);
	fprintf(file, "use_custom_keypad_font=%i\n", use_custom_keypad_font);
	if(use_custom_keypad_font || save_custom_keypad_font) fprintf(file, "custom_keypad_font=%s\n", custom_keypad_font.c_str());
	for(unsigned int i = 0; i < custom_buttons.size(); i++) {
		if(!custom_buttons[i].text.empty()) fprintf(file, "custom_button_label=%u:%s\n", i, custom_buttons[i].text.c_str());
		for(unsigned int bi = 0; bi <= 2; bi++) {
			if(custom_buttons[i].type[bi] != -1) {
				if(custom_buttons[i].value[bi].empty()) fprintf(file, "custom_button=%u:%u:%i\n", i, bi, custom_buttons[i].type[bi]);
				else fprintf(file, "custom_button=%u:%u:%i:%s\n", i, bi, custom_buttons[i].type[bi], custom_buttons[i].value[bi].c_str());
			}
		}
	}
	if(latest_button_unit) fprintf(file, "latest_button_unit=%s\n", latest_button_unit->referenceName().c_str());
	if(latest_button_currency) fprintf(file, "latest_button_currency=%s\n", latest_button_currency->referenceName().c_str());
}

void insert_left_shift() {
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(!rpn_mode && do_chain_mode("<<")) return;
		wrap_expression_selection();
	}
	insert_text("<<");
}
void insert_right_shift() {
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(!rpn_mode && do_chain_mode(">>")) return;
		wrap_expression_selection();
	}
	insert_text(">>");
}
void insert_bitwise_and() {
	if(rpn_mode) {calculateRPN(OPERATION_BITWISE_AND); return;}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode("&")) return;
		wrap_expression_selection();
	}
	insert_text("&");
}
void insert_bitwise_or() {
	if(rpn_mode) {calculateRPN(OPERATION_BITWISE_OR); return;}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode("|")) return;
		wrap_expression_selection();
	}
	insert_text("|");
}
void insert_bitwise_xor() {
	if(rpn_mode) {calculateRPN(OPERATION_BITWISE_XOR); return;}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode(" xor ")) return;
		wrap_expression_selection();
	}
	insert_text(" xor ");
}
void insert_bitwise_not() {
	if(rpn_mode) {
		if(expression_modified()) {
			if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
				execute_expression(true);
			}
		}
		execute_expression(true, false, OPERATION_ADD, NULL, false, 0, "~");
		return;
	}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN && wrap_expression_selection("~") > 0) return;
	insert_text("~");
}
void insert_button_function_default(GtkMenuItem*, gpointer user_data) {
	insert_button_function((MathFunction*) user_data);
}
void insert_button_function_save(GtkMenuItem*, gpointer user_data) {
	insert_button_function((MathFunction*) user_data, true);
}
void insert_button_function_norpn(GtkMenuItem*, gpointer user_data) {
	insert_button_function((MathFunction*) user_data, true, false);
}
void insert_function_operator(MathFunction *f) {
	if(rpn_mode || evalops.parse_options.parsing_mode == PARSING_MODE_RPN || is_at_beginning_of_expression()) {
		insert_button_function(f);
	} else if(f == CALCULATOR->f_mod) {
		if(wrap_expression_selection() >= 0) insert_text(" mod ");
		else insert_button_function(f);
	} else if(f == CALCULATOR->f_rem) {
		if(wrap_expression_selection() >= 0) insert_text(" rem ");
		else insert_button_function(f);
	} else {
		insert_button_function(f);
	}
}
void insert_function_operator_c(GtkWidget*, gpointer user_data) {
	insert_function_operator((MathFunction*) user_data);
}
void insert_button_variable(GtkWidget*, gpointer user_data) {
	insert_variable((Variable*) user_data);
}
void insert_button_unit(GtkMenuItem*, gpointer user_data) {
	insert_unit((Unit*) user_data);
	if((Unit*) user_data != latest_button_unit) {
		latest_button_unit = (Unit*) user_data;
		string si_label_str;
		if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			PrintOptions po = printops;
			po.is_approximate = NULL;
			po.can_display_unicode_string_arg = (void*) expression_edit_widget();
			po.abbreviate_names = true;
			si_label_str = ((CompositeUnit*) latest_button_unit)->print(po, true, TAG_TYPE_HTML, false, false);
		} else {
			si_label_str = latest_button_unit->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_UNIT, true, true);
		}
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_si")), si_label_str.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_si")), latest_button_unit->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str());
		if(enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_si")), FALSE);
	}
}
void insert_button_currency(GtkMenuItem*, gpointer user_data) {
	insert_unit((Unit*) user_data);
	if((Unit*) user_data != latest_button_currency) {
		latest_button_currency = (Unit*) user_data;
		string currency_label_str;
		if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			PrintOptions po = printops;
			po.is_approximate = NULL;
			po.can_display_unicode_string_arg = (void*) expression_edit_widget();
			po.abbreviate_names = true;
			currency_label_str = ((CompositeUnit*) latest_button_currency)->print(po, true, TAG_TYPE_HTML, false, false);
		} else {
			currency_label_str = latest_button_currency->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_UNIT, true, true);
		}
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_euro")), currency_label_str.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_euro")), latest_button_currency->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str());
		if(enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_euro")), FALSE);
	}
}

void base_button_alternative(int base) {
	if(printops.base != base) {
		set_output_base(base);
	} else if(evalops.parse_options.base != base) {
		set_output_base(evalops.parse_options.base);
	} else {
		set_output_base(BASE_DECIMAL);
	}
	focus_keeping_selection();
}

/*
	Button clicked -- insert text (1,2,3,... +,-,...)
*/
void button_pressed(GtkButton*, gpointer user_data) {
	insert_text((gchar*) user_data);
}

gboolean reenable_tooltip(GtkWidget *w, gpointer) {
	if(enable_tooltips == 1) gtk_widget_set_has_tooltip(w, TRUE);
	g_signal_handlers_disconnect_matched((gpointer) w, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) reenable_tooltip, NULL);
	return FALSE;
}

void hide_tooltip(GtkWidget *w) {
	if(!gtk_widget_get_has_tooltip(w)) return;
	gtk_widget_set_has_tooltip(w, FALSE);
	g_signal_connect(w, "leave-notify-event", G_CALLBACK(reenable_tooltip), NULL);
}

void set_keypad_tooltip(const gchar *w, const char *s1, const char *s2 = NULL, const char *s3 = NULL, bool b_markup = false, bool b_longpress = true) {
	string str;
	if(s1 && strlen(s1) == 0) s1 = NULL;
	if(s2 && strlen(s2) == 0) s2 = NULL;
	if(s3 && strlen(s3) == 0) s3 = NULL;
	if(s1) str += s1;
	if(s2) {
		if(s1) str += "\n\n";
		if(b_longpress) str += _("Right-click/long press: %s");
		else str += _("Right-click: %s");
		gsub("%s", s2, str);
	}
	if(s3) {
		if(s2) str += "\n";
		else if(s1) str += "\n\n";
		str += _("Middle-click: %s");
		gsub("%s", s3, str);
	}
	if(b_markup) gtk_widget_set_tooltip_markup(GTK_WIDGET(gtk_builder_get_object(main_builder, w)), str.c_str());
	else gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, w)), str.c_str());
	g_signal_connect(gtk_builder_get_object(main_builder, w), "clicked", G_CALLBACK(hide_tooltip), NULL);
}
void set_keypad_tooltip(const gchar *w, ExpressionItem *item1, ExpressionItem *item2 = NULL, ExpressionItem *item3 = NULL, bool b_markup = false, bool b_longpress = true) {
	set_keypad_tooltip(w, item1 ? item1->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str() : NULL, item2 ? item2->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str() : NULL, item3 ? item3->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str() : NULL, b_markup, b_longpress);
}

void update_keypad_caret_as_xor() {
	if(caret_as_xor) set_keypad_tooltip("button_xor", _("Bitwise Exclusive OR"));
	else set_keypad_tooltip("button_xor", (string(_("Bitwise Exclusive OR")) + " (Ctrl+^)").c_str());
	if(enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_xor")), FALSE);
}
void update_keypad_i() {
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_i")), (string("<i>") + CALCULATOR->v_i->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "label_i")).name + "</i>").c_str());
}

#define SET_LABEL_AND_TOOLTIP_2(i, w1, w2, l, t1, t2) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? (custom_buttons[i].type[1] == -1 ? NULL : t2) : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str());\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

#define SET_LABEL_AND_TOOLTIP_2NL(i, w2, t1, t2) \
	if(index == i || index < 0) {\
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? (custom_buttons[i].type[1] == -1 ? NULL : t2) : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str());\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

#define SET_LABEL_AND_TOOLTIP_3(i, w1, w2, l, t1, t2, t3) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? t3 : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str());\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

#define SET_LABEL_AND_TOOLTIP_3NP(i, w1, w2, l, t1, t2, t3) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? t3 :button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str(), false, custom_buttons[i].type[0] == -1);\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

#define SET_LABEL_AND_TOOLTIP_3M(i, w1, w2, l, t1, t2, t3) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? t3 : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str(), true);\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

#define SET_LABEL_AND_TOOLTIP_3NL(i, w2, t1, t2, t3) \
	if(index == i || index < 0) {\
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? t3 : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str());\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

#define SET_LABEL_AND_TOOLTIP_3C(i, w1, w2, l) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? NULL : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), custom_buttons[i].type[1] == -1 ? NULL : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), custom_buttons[i].type[2] == -1 ? NULL : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str());\
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, w2)), FALSE);\
	}

void update_custom_buttons(int index) {
	if(index == 0 || index < 0) {
		if(custom_buttons[0].text.empty()) gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_move")));
		else gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move")), GTK_WIDGET(gtk_builder_get_object(main_builder, "label_move")));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_move")), custom_buttons[0].text.c_str());
		set_keypad_tooltip("button_move", custom_buttons[0].type[0] == -1 ? _("Cycle through previous expression") : button_valuetype_text(custom_buttons[0].type[0], custom_buttons[0].value[0]).c_str(), custom_buttons[0].type[1] < 0 ? NULL : button_valuetype_text(custom_buttons[0].type[1], custom_buttons[0].value[1]).c_str(), custom_buttons[0].type[2] < 0 ? NULL : button_valuetype_text(custom_buttons[0].type[0], custom_buttons[0].value[2]).c_str(), false, custom_buttons[0].type[0] != -1);
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move")), FALSE);
	}
	if(index == 1 || index < 0) {
		if(custom_buttons[1].text.empty()) gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move2")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_move2")));
		else gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move2")), GTK_WIDGET(gtk_builder_get_object(main_builder, "label_move2")));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_move2")), custom_buttons[1].text.c_str());
		set_keypad_tooltip("button_move2", custom_buttons[1].type[0] == -1 ? _("Move cursor left or right") : button_valuetype_text(custom_buttons[1].type[0], custom_buttons[1].value[0]).c_str(), custom_buttons[1].type[1] == -1 ? _("Move cursor to beginning or end") : button_valuetype_text(custom_buttons[1].type[1], custom_buttons[1].value[1]).c_str(), custom_buttons[1].type[2] == -1 ? (custom_buttons[1].type[1] == -1 ? NULL : _("Move cursor to beginning or end")) : button_valuetype_text(custom_buttons[1].type[2], custom_buttons[1].value[2]).c_str(), false, custom_buttons[1].type[0] != -1);
		if(index >= 0 && enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move2")), FALSE);
	}
	SET_LABEL_AND_TOOLTIP_2(2, "label_percent", "button_percent", "%", CALCULATOR->v_percent->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str(), CALCULATOR->v_permille->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str())
	SET_LABEL_AND_TOOLTIP_3(3, "label_plusminus", "button_plusminus", "±", _("Uncertainty/interval"), _("Relative error"), _("Interval"))
	SET_LABEL_AND_TOOLTIP_3(4, "label_comma", "button_comma", CALCULATOR->getComma().c_str(), _("Argument separator"), _("Blank space"), _("New line"))
	SET_LABEL_AND_TOOLTIP_2(5, "label_brace_wrap", "button_brace_wrap", "(x)", _("Smart parentheses"), _("Vector brackets"))
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_brace_wrap")), custom_buttons[5].text.empty() ? "(x)" : custom_buttons[5].text.c_str());
	SET_LABEL_AND_TOOLTIP_2(6, "label_brace_open", "button_brace_open", "(", _("Left parenthesis"), _("Left vector bracket"))
	SET_LABEL_AND_TOOLTIP_2(7, "label_brace_close", "button_brace_close", ")", _("Right parenthesis"), _("Right vector bracket"))
	SET_LABEL_AND_TOOLTIP_3M(8, "label_zero", "button_zero", "0", NULL, "x<sup>0</sup>", CALCULATOR->getDegUnit()->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str())
	SET_LABEL_AND_TOOLTIP_3M(9, "label_one", "button_one", "1", NULL, "x<sup>1</sup>", "1/x")
	SET_LABEL_AND_TOOLTIP_3M(10, "label_two", "button_two", "2", NULL, "x<sup>2</sup>", "1/2")
	SET_LABEL_AND_TOOLTIP_3M(11, "label_three", "button_three", "3", NULL, "x<sup>3</sup>", "1/3")
	SET_LABEL_AND_TOOLTIP_3M(12, "label_four", "button_four", "4", NULL, "x<sup>4</sup>", "1/4")
	SET_LABEL_AND_TOOLTIP_3M(13, "label_five", "button_five", "5", NULL, "x<sup>5</sup>", "1/5")
	SET_LABEL_AND_TOOLTIP_3M(14, "label_six", "button_six", "6", NULL, "x<sup>6</sup>", "1/6")
	SET_LABEL_AND_TOOLTIP_3M(15, "label_seven", "button_seven", "7", NULL, "x<sup>7</sup>", "1/7")
	SET_LABEL_AND_TOOLTIP_3M(16, "label_eight", "button_eight", "8", NULL, "x<sup>8</sup>", "1/8")
	SET_LABEL_AND_TOOLTIP_3M(17, "label_nine", "button_nine", "9", NULL, "x<sup>9</sup>", "1/9")
	SET_LABEL_AND_TOOLTIP_3(18, "label_dot", "button_dot", CALCULATOR->getDecimalPoint().c_str(), _("Decimal point"), _("Blank space"), _("New line"))
	if(index < 0 || index == 19) {
		MathFunction *f = CALCULATOR->getActiveFunction("exp10");
		SET_LABEL_AND_TOOLTIP_3M(19, "label_exp", "button_exp", _("EXP"), "10<sup>x</sup> (Ctrl+Shift+E)", CALCULATOR->f_exp->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str(), (f ? f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str() : NULL))
	}
	if(index == 20 || (index < 0 && !custom_buttons[20].text.empty())) {
		if(custom_buttons[20].text.empty()) {
			FIX_SUPSUB_PRE_W(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_xy")));
			string s_xy = "x<sup>y</sup>";
			FIX_SUPSUB(s_xy);
			gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_xy")), s_xy.c_str());
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_xy")), custom_buttons[20].text.c_str());
		}
	}
	SET_LABEL_AND_TOOLTIP_2NL(20, "button_xy", _("Raise (Ctrl+*)"), CALCULATOR->f_sqrt->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str())
	if(index == 24 || (index < 0 && !custom_buttons[24].text.empty())) {
		if(custom_buttons[24].text.empty()) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_sub"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), SIGN_MINUS);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), MINUS);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), custom_buttons[24].text.c_str());
		}
	}
	if(index == 22 || (index < 0 && !custom_buttons[22].text.empty())) {
		if(custom_buttons[22].text.empty()) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_times"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), SIGN_MULTIPLICATION);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), MULTIPLICATION);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), custom_buttons[22].text.c_str());
		}
	}
	if(index == 21 || (index < 0 && !custom_buttons[21].text.empty())) {
		if(custom_buttons[21].text.empty()) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), SIGN_DIVISION_SLASH);
			else if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), SIGN_DIVISION);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), DIVISION);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), custom_buttons[21].text.c_str());
		}
	}
	SET_LABEL_AND_TOOLTIP_2NL(21, "button_divide", _("Divide"), "1/x")
	SET_LABEL_AND_TOOLTIP_2NL(22, "button_times", _("Multiply"), _("Bitwise Exclusive OR"))
	SET_LABEL_AND_TOOLTIP_3(23, "label_add", "button_add", "+", _c("Keypad", "Add"), _("M+ (memory plus)"), _("Bitwise AND"))
	if(index < 0 || index == 24) {
		MathFunction *f = CALCULATOR->getActiveFunction("neg");
		SET_LABEL_AND_TOOLTIP_3NL(24, "button_sub", _("Subtract"), f ? f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str() : NULL, _("Bitwise OR"));
	}
	SET_LABEL_AND_TOOLTIP_2(25, "label_ac", "button_ac", _("AC"), _("Clear"), _("MC (memory clear)"))
	SET_LABEL_AND_TOOLTIP_3NP(26, "label_del", "button_del", _("DEL"), _("Delete"), _("Backspace"), _("M− (memory minus)"))
	SET_LABEL_AND_TOOLTIP_2(27, "label_ans", "button_ans", _("ANS"), _("Previous result"), _("Previous result (static)"))
	if(index == 28 || (index < 0 && !custom_buttons[28].text.empty())) {
		if(custom_buttons[28].text.empty()) {
			gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), "<big>=</big>");
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), custom_buttons[28].text.c_str());
		}
	}
	SET_LABEL_AND_TOOLTIP_3NL(28, "button_equals", _("Calculate expression"), _("MR (memory recall)"), _("MS (memory store)"))
	SET_LABEL_AND_TOOLTIP_3C(29, "label_c1", "button_c1", "C1")
	SET_LABEL_AND_TOOLTIP_3C(30, "label_c2", "button_c2", "C2")
	SET_LABEL_AND_TOOLTIP_3C(31, "label_c3", "button_c3", "C3")
	SET_LABEL_AND_TOOLTIP_3C(32, "label_c4", "button_c4", "C4")
	SET_LABEL_AND_TOOLTIP_3C(33, "label_c5", "button_c5", "C5")
	SET_LABEL_AND_TOOLTIP_3C(34, "label_c6", "button_c6", "C6")
	SET_LABEL_AND_TOOLTIP_3C(35, "label_c7", "button_c7", "C7")
	SET_LABEL_AND_TOOLTIP_3C(36, "label_c8", "button_c8", "C8")
	SET_LABEL_AND_TOOLTIP_3C(37, "label_c9", "button_c9", "C9")
	SET_LABEL_AND_TOOLTIP_3C(38, "label_c10", "button_c10", "C10")
	SET_LABEL_AND_TOOLTIP_3C(39, "label_c11", "button_c11", "C11")
	SET_LABEL_AND_TOOLTIP_3C(40, "label_c12", "button_c12", "C12")
	SET_LABEL_AND_TOOLTIP_3C(41, "label_c13", "button_c13", "C13")
	SET_LABEL_AND_TOOLTIP_3C(42, "label_c14", "button_c14", "C14")
	SET_LABEL_AND_TOOLTIP_3C(43, "label_c15", "button_c15", "C15")
	SET_LABEL_AND_TOOLTIP_3C(44, "label_c16", "button_c16", "C16")
	SET_LABEL_AND_TOOLTIP_3C(45, "label_c17", "button_c17", "C17")
	SET_LABEL_AND_TOOLTIP_3C(46, "label_c18", "button_c18", "C18")
	SET_LABEL_AND_TOOLTIP_3C(47, "label_c19", "button_c19", "C19")
	SET_LABEL_AND_TOOLTIP_3C(48, "label_c20", "button_c20", "C20")
	if(index >= 29 && index <= 33) {
		bool b_show = false;
		for(size_t i = 29; i <= 33; i++) {
			if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
				b_show = true;
				break;
			}
		}
		if(b_show != gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons1")))) {
			if(!b_show && keypad_is_visible()) {
				gint w_c = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons1"))) + 6;
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons1")), b_show);
				while(gtk_events_pending()) gtk_main_iteration();
				gint w, h;
				gtk_window_get_size(main_window(), &w, &h);
				gtk_window_resize(main_window(), w - w_c, h);
			} else {
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons1")), b_show);
			}
		}
	}
	if(index >= 34 && index <= 38) {
		bool b_show = false;
		for(size_t i = 34; i <= 38; i++) {
			if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
				b_show = true;
				break;
			}
		}
		if(b_show != gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons2")))) {
			if(!b_show && keypad_is_visible()) {
				gint w_c = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons2"))) + 6;
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons2")), b_show);
				while(gtk_events_pending()) gtk_main_iteration();
				gint w, h;
				gtk_window_get_size(main_window(), &w, &h);
				gtk_window_resize(main_window(), w - w_c, h);
			} else {
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons2")), b_show);
			}
		}
	}
	if(index >= 39 && index <= 43) {
		bool b_show = false;
		for(size_t i = 39; i <= 43; i++) {
			if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
				b_show = true;
				break;
			}
		}
		if(b_show != gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons3")))) {
			if(!b_show && keypad_is_visible()) {
				gint w_c = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons3"))) + 6;
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons3")), b_show);
				while(gtk_events_pending()) gtk_main_iteration();
				gint w, h;
				gtk_window_get_size(main_window(), &w, &h);
				gtk_window_resize(main_window(), w - w_c, h);
			} else {
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons3")), b_show);
			}
		}
	}
	if(index >= 44 && index <= 48) {
		bool b_show = false;
		for(size_t i = 44; i <= 48; i++) {
			if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
				b_show = true;
				break;
			}
		}
		if(b_show != gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons4")))) {
			if(!b_show && keypad_is_visible()) {
				gint w_c = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons4"))) + 6;
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons4")), b_show);
				while(gtk_events_pending()) gtk_main_iteration();
				gint w, h;
				gtk_window_get_size(main_window(), &w, &h);
				gtk_window_resize(main_window(), w - w_c, h);
			} else {
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons4")), b_show);
			}
		}
	}
}

void set_custom_buttons() {
	if(!latest_button_unit_pre.empty()) {
		latest_button_unit = CALCULATOR->getActiveUnit(latest_button_unit_pre);
		if(!latest_button_unit) latest_button_unit = CALCULATOR->getCompositeUnit(latest_button_unit_pre);
	}
	PrintOptions po = printops;
	po.is_approximate = NULL;
	po.can_display_unicode_string_arg = (void*) expression_edit_widget();
	po.abbreviate_names = true;
	if(latest_button_unit) {
		string si_label_str;
		if(latest_button_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			si_label_str = ((CompositeUnit*) latest_button_unit)->print(po, true, TAG_TYPE_HTML, false, false);
		} else {
			si_label_str = latest_button_unit->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(STRUCT_UNIT, true, true);
		}
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_si")), si_label_str.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_si")), latest_button_unit->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str());
	}
	Unit *u_local_currency = CALCULATOR->getLocalCurrency();
	if(!latest_button_currency_pre.empty()) {
		latest_button_currency = CALCULATOR->getActiveUnit(latest_button_currency_pre);
	}
	if(!latest_button_currency && u_local_currency) latest_button_currency = u_local_currency;
	if(!latest_button_currency) latest_button_currency = CALCULATOR->u_euro;
	string unit_label_str;
	if(latest_button_currency->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		unit_label_str = ((CompositeUnit*) latest_button_currency)->print(po, true, TAG_TYPE_HTML, false, false);
	} else {
		unit_label_str = latest_button_currency->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(STRUCT_UNIT, true, true);
	}
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_euro")), unit_label_str.c_str());
	gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_euro")), latest_button_currency->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str());
}

void create_button_menus() {

	GtkWidget *item, *sub;
	MathFunction *f;

	Unit *u = CALCULATOR->getActiveUnit("bit");
	if(u && u->preferredDisplayName(true, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "combobox_bits")).formattedName(STRUCT_UNIT, true) != "bits" && unicode_length(u->preferredDisplayName(true, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "combobox_bits")).formattedName(STRUCT_UNIT, true)) <= 5) {
		string str = "? "; str += u->preferredDisplayName(true, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "combobox_bits")).formattedName(STRUCT_UNIT, true);
		gint i = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_bits")));
		gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(main_builder, "combobox_bits")), 0);
		gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(main_builder, "combobox_bits")), str.c_str());
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_bits")), i);
	}

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_bases"));
	MENU_ITEM(_("Bitwise Left Shift"), insert_left_shift)
	MENU_ITEM(_("Bitwise Right Shift"), insert_right_shift)
	MENU_ITEM(_("Bitwise AND"), insert_bitwise_and)
	MENU_ITEM(_("Bitwise OR"), insert_bitwise_or)
	MENU_ITEM(_("Bitwise Exclusive OR"), insert_bitwise_xor)
	MENU_ITEM(_("Bitwise NOT"), insert_bitwise_not)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_bitcmp, insert_button_function_default)
	f = CALCULATOR->getActiveFunction("bitrot");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default)}
	MENU_SEPARATOR
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_base, insert_button_function_default)
	MENU_SEPARATOR
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_ascii, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_char, insert_button_function_default)

	PangoFontDescription *font_desc;

	gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_xy"))), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);

	g_signal_connect(gtk_builder_get_object(main_builder, "button_e_var"), "clicked", G_CALLBACK(insert_button_variable), (gpointer) CALCULATOR->v_e);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_e"));
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_exp, insert_button_function_default)
	f = CALCULATOR->getActiveFunction("cis");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}

	pango_font_description_free(font_desc);

	g_signal_connect(gtk_builder_get_object(main_builder, "button_sqrt"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_sqrt);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_sqrt"));
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_cbrt, insert_button_function_default);
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_root, insert_button_function_default);
	f = CALCULATOR->getActiveFunction("sqrtpi");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}

	g_signal_connect(gtk_builder_get_object(main_builder, "button_ln"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_ln);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_ln"));
	f = CALCULATOR->getActiveFunction("log10");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("log2");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_logn, insert_button_function_default)

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_fac"));
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_factorial2, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_multifactorial, insert_button_function_default)
	f = CALCULATOR->getActiveFunction("hyperfactorial");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("superfactorial");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("perm");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("comb");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("derangements");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_binomial, insert_button_function_default)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_mod"), "clicked", G_CALLBACK(insert_function_operator_c), (gpointer) CALCULATOR->f_mod);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_mod"));
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_rem, insert_function_operator_c)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_abs, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_gcd, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_lcm, insert_button_function_default)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_sine"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_sin);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_sin"));
	MENU_SEPARATOR_PREPEND
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_asinh, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_asin, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_sinh, insert_button_function_default)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_cosine"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_cos);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_cos"));
	MENU_SEPARATOR_PREPEND
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_acosh, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_acos, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_cosh, insert_button_function_default)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_tan"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_tan);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_tan"));
	MENU_SEPARATOR_PREPEND
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_atanh, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_atan, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT_PREPEND(CALCULATOR->f_tanh, insert_button_function_default)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_sum"), "clicked", G_CALLBACK(insert_button_function_norpn), (gpointer) CALCULATOR->f_sum);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_sum"));
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_product, insert_button_function_norpn)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_for, insert_button_function_norpn)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_if, insert_button_function_norpn)


	g_signal_connect(gtk_builder_get_object(main_builder, "button_mean"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->getActiveFunction("mean"));
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_mean"));
	f = CALCULATOR->getActiveFunction("median");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("var");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("stdev");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("stderr");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("harmmean");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("geomean");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	MENU_SEPARATOR
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_rand, insert_button_function_default);

	g_signal_connect(gtk_builder_get_object(main_builder, "button_pi"), "clicked", G_CALLBACK(insert_button_variable), (gpointer) CALCULATOR->v_pi);

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_xequals"));
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_solve, insert_button_function_default)
	f = CALCULATOR->getActiveFunction("solve2");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("linearfunction");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_dsolve, insert_button_function_default)
	f = CALCULATOR->getActiveFunction("extremum");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	MENU_SEPARATOR
	MENU_ITEM(_("Set unknowns"), on_menu_item_set_unknowns_activate)

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_factorize"));
	MENU_ITEM(_("Expand"), on_menu_item_simplify_activate)
	item_simplify = item;
	MENU_ITEM(_("Factorize"), on_menu_item_factorize_activate)
	item_factorize = item;
	MENU_ITEM(_("Expand Partial Fractions"), on_menu_item_expand_partial_fractions_activate)
	MENU_SEPARATOR
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_diff, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_integrate, insert_button_function_default)
	gtk_widget_set_visible(item_factorize, evalops.structuring != STRUCTURING_SIMPLIFY);
	gtk_widget_set_visible(item_simplify, evalops.structuring != STRUCTURING_FACTORIZE);

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_i"));
	MENU_ITEM("∠ (Ctrl+Shift+A)", insert_angle_symbol)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_re, insert_button_function_default)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->f_im, insert_button_function_default)
	f = CALCULATOR->getActiveFunction("arg");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}
	f = CALCULATOR->getActiveFunction("conj");
	if(f) {MENU_ITEM_WITH_OBJECT(f, insert_button_function_default);}

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_si"));
	const char *si_units[] = {"m", "g", "s", "A", "K", "N", "Pa", "J", "W", "L", "V", "ohm", "oC", "cd", "mol", "C", "Hz", "F", "S", "Wb", "T", "H", "lm", "lx", "Bq", "Gy", "Sv", "kat"};
	vector<Unit*> to_us;
	size_t si_i = 0;
	size_t i_added = 0;
	for(; si_i < 28 && i_added < 12; si_i++) {
		Unit * u = CALCULATOR->getActiveUnit(si_units[si_i]);
		if(u && !u->isHidden()) {
			bool b = false;
			for(size_t i2 = 0; i2 < to_us.size(); i2++) {
				if(string_is_less(u->title(true, printops.use_unicode_signs), to_us[i2]->title(true, printops.use_unicode_signs))) {
					to_us.insert(to_us.begin() + i2, u);
					b = true;
					break;
				}
			}
			if(!b) to_us.push_back(u);
			i_added++;
		}
	}
	for(size_t i = 0; i < to_us.size(); i++) {
		MENU_ITEM_WITH_OBJECT(to_us[i], insert_button_unit)
	}

	// Show further items in a submenu
	if(si_i < 28) {SUBMENU_ITEM(_("more"), sub);}

	to_us.clear();
	for(; si_i < 28; si_i++) {
		Unit * u = CALCULATOR->getActiveUnit(si_units[si_i]);
		if(u && !u->isHidden()) {
			bool b = false;
			for(size_t i2 = 0; i2 < to_us.size(); i2++) {
				if(string_is_less(u->title(true, printops.use_unicode_signs), to_us[i2]->title(true, printops.use_unicode_signs))) {
					to_us.insert(to_us.begin() + i2, u);
					b = true;
					break;
				}
			}
			if(!b) to_us.push_back(u);
		}
	}
	for(size_t i = 0; i < to_us.size(); i++) {
		MENU_ITEM_WITH_OBJECT(to_us[i], insert_button_unit)
	}

	Unit *u_local_currency = CALCULATOR->getLocalCurrency();
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_euro"));
	const char *currency_units[] = {"USD", "GBP", "JPY"};
	to_us.clear();
	for(size_t i = 0; i < 5; i++) {
		Unit * u;
		if(i == 0) u = CALCULATOR->u_euro;
		else if(i == 1) u = u_local_currency;
		else u = CALCULATOR->getActiveUnit(currency_units[i - 2]);
		if(u && (i == 1 || !u->isHidden())) {
			bool b = false;
			for(size_t i2 = 0; i2 < to_us.size(); i2++) {
				if(u == to_us[i2]) {
					b = true;
					break;
				}
				if(string_is_less(u->title(true, printops.use_unicode_signs), to_us[i2]->title(true, printops.use_unicode_signs))) {
					to_us.insert(to_us.begin() + i2, u);
					b = true;
					break;
				}
			}
			if(!b) to_us.push_back(u);
		}
	}
	for(size_t i = 0; i < to_us.size(); i++) {
		MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us[i], insert_button_currency)
	}

	i_added = to_us.size();
	vector<Unit*> to_us2;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->baseUnit() == CALCULATOR->u_euro) {
			Unit *u = CALCULATOR->units[i];
			if(u->isActive()) {
				bool b = false;
				if(u->isHidden() && u != u_local_currency) {
					for(int i2 = to_us2.size() - 1; i2 >= 0; i2--) {
						if(u->title(true, printops.use_unicode_signs) > to_us2[(size_t) i2]->title(true, printops.use_unicode_signs)) {
							if((size_t) i2 == to_us2.size() - 1) to_us2.push_back(u);
							else to_us2.insert(to_us2.begin() + (size_t) i2 + 1, u);
							b = true;
							break;
						}
					}
					if(!b) to_us2.insert(to_us2.begin(), u);
				} else {
					for(size_t i2 = 0; i2 < i_added; i2++) {
						if(u == to_us[i2]) {
							b = true;
							break;
						}
					}
					for(size_t i2 = to_us.size() - 1; !b && i2 >= i_added; i2--) {
						if(u->title(true, printops.use_unicode_signs) > to_us[i2]->title(true, printops.use_unicode_signs)) {
							if(i2 == to_us.size() - 1) to_us.push_back(u);
							else to_us.insert(to_us.begin() + i2 + 1, u);
							b = true;
						}
					}
					if(!b) to_us.insert(to_us.begin() + i_added, u);
				}
			}
		}
	}
	for(size_t i = i_added; i < to_us.size(); i++) {
		// Show further items in a submenu
		if(i == i_added) {SUBMENU_ITEM(_("more"), sub);}
		MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us[i], insert_button_currency)
	}
	if(to_us2.size() > 0) {SUBMENU_ITEM(_("more"), sub);}
	for(size_t i = 0; i < to_us2.size(); i++) {
		// Show further items in a submenu
		MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us2[i], insert_button_currency)
	}

	set_keypad_tooltip("button_e_var", CALCULATOR->v_e);
	set_keypad_tooltip("button_pi", CALCULATOR->v_pi);
	set_keypad_tooltip("button_sine", CALCULATOR->f_sin);
	set_keypad_tooltip("button_cosine", CALCULATOR->f_cos);
	set_keypad_tooltip("button_tan", CALCULATOR->f_tan);
	f = CALCULATOR->getActiveFunction("mean");
	if(f) set_keypad_tooltip("button_mean", f);
	set_keypad_tooltip("button_sum", CALCULATOR->f_sum);
	set_keypad_tooltip("button_mod", CALCULATOR->f_mod);
	set_keypad_tooltip("button_fac", CALCULATOR->f_factorial);
	set_keypad_tooltip("button_ln", CALCULATOR->f_ln);
	set_keypad_tooltip("button_sqrt", CALCULATOR->f_sqrt);
	set_keypad_tooltip("button_i", CALCULATOR->v_i);
	update_keypad_i();

	g_signal_connect(gtk_builder_get_object(main_builder, "button_cmp"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_bitcmp);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_int"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_int);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_frac"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_frac);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_ln2"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_ln);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_sqrt2"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_sqrt);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_abs"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_abs);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_expf"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_exp);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_stamptodate"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_stamptodate);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_code"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_ascii);
	//g_signal_connect(gtk_builder_get_object(main_builder, "button_rnd"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) CALCULATOR->f_rand);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_mod2"), "clicked", G_CALLBACK(insert_function_operator_c), (gpointer) CALCULATOR->f_mod);

	f = CALCULATOR->getActiveFunction("exp2");
	set_keypad_tooltip("button_expf", CALCULATOR->f_exp, f);
	set_keypad_tooltip("button_mod2", CALCULATOR->f_mod, CALCULATOR->f_rem);
	set_keypad_tooltip("button_ln2", CALCULATOR->f_ln);
	set_keypad_tooltip("button_int", CALCULATOR->f_int);
	set_keypad_tooltip("button_frac", CALCULATOR->f_frac);
	set_keypad_tooltip("button_stamptodate", CALCULATOR->f_stamptodate, CALCULATOR->f_timestamp);
	set_keypad_tooltip("button_code", CALCULATOR->f_ascii, CALCULATOR->f_char);
	f = CALCULATOR->getActiveFunction("log2");
	MathFunction *f2 = CALCULATOR->getActiveFunction("log10");
	set_keypad_tooltip("button_log2", f, f2);
	if(f) g_signal_connect(gtk_builder_get_object(main_builder, "button_log2"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) f);
	set_keypad_tooltip("button_reciprocal", "1/x");
	f = CALCULATOR->getActiveFunction("div");
	if(f) set_keypad_tooltip("button_idiv", f);
	set_keypad_tooltip("button_sqrt2", CALCULATOR->f_sqrt, CALCULATOR->f_cbrt);
	set_keypad_tooltip("button_abs", CALCULATOR->f_abs);
	set_keypad_tooltip("button_fac2", CALCULATOR->f_factorial);
	//set_keypad_tooltip("button_rnd", CALCULATOR->f_rand);
	set_keypad_tooltip("button_cmp", CALCULATOR->f_bitcmp);
	f = CALCULATOR->getActiveFunction("bitrot");
	if(f) {
		set_keypad_tooltip("button_rot", f);
		g_signal_connect(gtk_builder_get_object(main_builder, "button_rot"), "clicked", G_CALLBACK(insert_button_function_default), (gpointer) f);
	}

	set_keypad_tooltip("button_and", _("Bitwise AND"), _("Logical AND"));
	set_keypad_tooltip("button_or", _("Bitwise OR"), _("Logical OR"));
	set_keypad_tooltip("button_not", _("Bitwise NOT"), _("Logical NOT"));

	set_keypad_tooltip("button_bin", _("Binary"), _("Toggle Result Base"));
	set_keypad_tooltip("button_oct", _("Octal"), _("Toggle Result Base"));
	set_keypad_tooltip("button_dec", _("Decimal"), _("Toggle Result Base"));
	set_keypad_tooltip("button_hex", _("Hexadecimal"), _("Toggle Result Base"));

	set_keypad_tooltip("button_store2", _("Store result as a variable"), _("Open menu with stored variables"));

	update_keypad_caret_as_xor();

	gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_percent")), CALCULATOR->v_percent->title().c_str());
	gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permille")), CALCULATOR->v_permille->title().c_str());
	gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permyriad")), CALCULATOR->v_permyriad->title().c_str());

	update_mb_fx_menu();
	update_mb_sto_menu();
	update_mb_units_menu();
	update_mb_pi_menu();
	update_mb_to_menu();

}

void update_button_padding(bool initial) {
	if(horizontal_button_padd >= 0 || vertical_button_padd >= 0) {
		if(!button_padding_provider) {
			button_padding_provider = gtk_css_provider_new();
			gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(button_padding_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
		}
		string padding_css;
		if(horizontal_button_padd >= 0) {
			padding_css += "#grid_buttons button, #button_exact, #button_fraction {";
			padding_css += "padding-left: "; padding_css += i2s(horizontal_button_padd);
			padding_css += "px; padding-right: "; padding_css += i2s(horizontal_button_padd); padding_css += "px}";
		}
		if(vertical_button_padd >= 0) {
			if(horizontal_button_padd >= 0) padding_css += "\n";
			padding_css += "#buttons button {";
			padding_css += "padding-top: "; padding_css += i2s(vertical_button_padd);
			padding_css += "px; padding-bottom: "; padding_css += i2s(vertical_button_padd); padding_css += "px}";
		}
		gtk_css_provider_load_from_data(button_padding_provider, padding_css.c_str(), -1, NULL);
	} else if(!initial) {
		if(button_padding_provider) gtk_css_provider_load_from_data(button_padding_provider, "", -1, NULL);
	}
}

gboolean on_keypad_button_alt(GtkWidget *w, bool b2) {
	hide_tooltip(w);
	if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_ac"))) {
		DO_CUSTOM_BUTTONS(25)
		memory_clear();
		show_notification("MC");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals"))) {
		DO_CUSTOM_BUTTONS(28)
		if(b2) {memory_store(); show_notification("MS");}
		else memory_recall();
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_divide"))) {
		DO_CUSTOM_BUTTONS(21)
		insert_button_function(CALCULATOR->getActiveFunction("inv"));
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_exp"))) {
		DO_CUSTOM_BUTTONS(19)
		if(b2) {
			insert_button_function(CALCULATOR->getActiveFunction("exp10"));
		} else {
			insert_button_function(CALCULATOR->f_exp);
		}
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_comma"))) {
		DO_CUSTOM_BUTTONS(4)
		if(b2) insert_text("\n");
		else insert_text(" ");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_dot"))) {
		DO_CUSTOM_BUTTONS(18)
		if(b2) insert_text("\n");
		else insert_text(" ");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_open"))) {
		DO_CUSTOM_BUTTONS(6)
		insert_text("[");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_close"))) {
		DO_CUSTOM_BUTTONS(7)
		insert_text("]");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_wrap"))) {
		DO_CUSTOM_BUTTONS(5)
		if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
			GtkTextIter istart, iend;
			gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &istart, &iend);
			gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
			string str = "[";
			str += gstr;
			str += "]";
			insert_text(str.c_str());
			g_free(gstr);
		} else {
			insert_text("[]");
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, gtk_text_buffer_get_insert(expression_edit_buffer()));
			gtk_text_iter_backward_char(&iter);
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
		}
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_add"))) {
		DO_CUSTOM_BUTTONS(23)
		if(b2) insert_bitwise_and();
		else {memory_add(); show_notification("M+");}
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_sub"))) {
		DO_CUSTOM_BUTTONS(24)
		if(b2) insert_bitwise_or();
		else insert_button_function(CALCULATOR->getActiveFunction("neg"));
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_times"))) {
		DO_CUSTOM_BUTTONS(22)
		insert_bitwise_xor();
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_xy"))) {
		DO_CUSTOM_BUTTONS(20)
		insert_button_function(CALCULATOR->f_sqrt);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_del"))) {
		DO_CUSTOM_BUTTONS(26)
		if(b2) {
			memory_subtract();
			show_notification("M−");
		} else {
			if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
				overwrite_expression_selection(NULL);
			} else {
				block_completion();
				GtkTextMark *mpos = gtk_text_buffer_get_insert(expression_edit_buffer());
				GtkTextIter ipos, iend;
				gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mpos);
				iend = ipos;
				if(gtk_text_iter_backward_char(&ipos)) {
					gtk_text_buffer_delete(expression_edit_buffer(), &ipos, &iend);
				}
				focus_keeping_selection();
				unblock_completion();
			}
		}
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_ans"))) {
		DO_CUSTOM_BUTTONS(27)
		if(history_answer.size() > 0) {
			string str = answer_function()->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_FUNCTION, true);
			Number nr(history_answer.size(), 1);
			str += '(';
			str += print_with_evalops(nr);
			str += ')';
			insert_text(str.c_str());
		}
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move"))) {
		DO_CUSTOM_BUTTONS(0)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move2"))) {
		DO_CUSTOM_BUTTONS(1)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_percent"))) {
		DO_CUSTOM_BUTTONS(2)
		insert_variable(CALCULATOR->v_permille);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_plusminus"))) {
		DO_CUSTOM_BUTTONS(3)
		if(b2) insert_button_function(CALCULATOR->f_interval);
		else insert_button_function(CALCULATOR->f_uncertainty);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_bin"))) {
		base_button_alternative(2);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_oct"))) {
		base_button_alternative(8);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_dec"))) {
		base_button_alternative(10);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_hex"))) {
		base_button_alternative(16);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_mod2"))) {
		if(expression_is_empty() || rpn_mode || evalops.parse_options.parsing_mode == PARSING_MODE_RPN || wrap_expression_selection() < 0) {
			insert_button_function(CALCULATOR->f_rem);
		} else {
			insert_text(" rem ");
		}
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_sqrt2"))) {
		insert_button_function(CALCULATOR->f_cbrt);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_expf"))) {
		insert_button_function(CALCULATOR->getActiveFunction("exp2"));
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_log2"))) {
		insert_button_function(CALCULATOR->getActiveFunction("log10"));
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_code"))) {
		insert_button_function(CALCULATOR->f_char);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_stamptodate"))) {
		insert_button_function(CALCULATOR->f_timestamp);
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_and"))) {
		if(rpn_mode) {calculateRPN(OPERATION_LOGICAL_AND); return TRUE;}
		if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) wrap_expression_selection();
		insert_text("&&");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_or"))) {
		if(rpn_mode) {calculateRPN(OPERATION_LOGICAL_OR); return TRUE;}
		if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) wrap_expression_selection();
		insert_text("||");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_not"))) {
		if(rpn_mode) {
			if(expression_modified()) {
				if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
					execute_expression(true);
				}
			}
			execute_expression(true, false, OPERATION_ADD, NULL, false, 0, "!");
			return TRUE;
		}
		if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN && wrap_expression_selection("!") > 0) return TRUE;
		insert_text("!");
		return TRUE;
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c1"))) {
		DO_CUSTOM_BUTTONS_CX(1)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c2"))) {
		DO_CUSTOM_BUTTONS_CX(2)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c3"))) {
		DO_CUSTOM_BUTTONS_CX(3)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c4"))) {
		DO_CUSTOM_BUTTONS_CX(4)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c5"))) {
		DO_CUSTOM_BUTTONS_CX(5)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c6"))) {
		DO_CUSTOM_BUTTONS_CX(6)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c7"))) {
		DO_CUSTOM_BUTTONS_CX(7)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c8"))) {
		DO_CUSTOM_BUTTONS_CX(8)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c9"))) {
		DO_CUSTOM_BUTTONS_CX(9)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c10"))) {
		DO_CUSTOM_BUTTONS_CX(10)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c11"))) {
		DO_CUSTOM_BUTTONS_CX(11)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c12"))) {
		DO_CUSTOM_BUTTONS_CX(12)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c13"))) {
		DO_CUSTOM_BUTTONS_CX(13)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c14"))) {
		DO_CUSTOM_BUTTONS_CX(14)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c15"))) {
		DO_CUSTOM_BUTTONS_CX(15)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c16"))) {
		DO_CUSTOM_BUTTONS_CX(16)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c17"))) {
		DO_CUSTOM_BUTTONS_CX(17)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c18"))) {
		DO_CUSTOM_BUTTONS_CX(18)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c19"))) {
		DO_CUSTOM_BUTTONS_CX(19)
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c20"))) {
		DO_CUSTOM_BUTTONS_CX(20)
	} else {
		int i = 0;
		if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_zero"))) i = 0;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_one"))) i = 1;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_two"))) i = 2;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_three"))) i = 3;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_four"))) i = 4;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_five"))) i = 5;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_six"))) i = 6;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_seven"))) i = 7;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_eight"))) i = 8;
		else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_nine"))) i = 9;
		else return FALSE;
		DO_CUSTOM_BUTTONS(i + 8)
		if(b2 && i == 1) {
			insert_button_function(CALCULATOR->getActiveFunction("inv"));
			return TRUE;
		} else if(b2 && i == 0) {
			insert_text(SIGN_DEGREE);
			return TRUE;
		}
		if(!b2) wrap_expression_selection();
		if(printops.use_unicode_signs && (evalops.parse_options.base > i || i == 0)) {
			if(b2) {
				if(i == 2 && can_display_unicode_string_function("½", (void*) expression_edit_widget())) {insert_text("½"); return TRUE;}
				if(i == 3 && can_display_unicode_string_function("⅓", (void*) expression_edit_widget())) {insert_text("⅓"); return TRUE;}
				if(i == 4 && can_display_unicode_string_function("¼", (void*) expression_edit_widget())) {insert_text("¼"); return TRUE;}
				if(i == 5 && can_display_unicode_string_function("⅕", (void*) expression_edit_widget())) {insert_text("⅕"); return TRUE;}
				if(i == 6 && can_display_unicode_string_function("⅙", (void*) expression_edit_widget())) {insert_text("⅙"); return TRUE;}
				if(i == 7 && can_display_unicode_string_function("⅐", (void*) expression_edit_widget())) {insert_text("⅐"); return TRUE;}
				if(i == 8 && can_display_unicode_string_function("⅛", (void*) expression_edit_widget())) {insert_text("⅛"); return TRUE;}
				if(i == 9 && can_display_unicode_string_function("⅑", (void*) expression_edit_widget())) {insert_text("⅑"); return TRUE;}
			} else {
				if(i == 0 && can_display_unicode_string_function(SIGN_POWER_0, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_0); return TRUE;}
				if(i == 1 && can_display_unicode_string_function(SIGN_POWER_1, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_1); return TRUE;}
				if(i == 2 && can_display_unicode_string_function(SIGN_POWER_2, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_2); return TRUE;}
				if(i == 3 && can_display_unicode_string_function(SIGN_POWER_3, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_3); return TRUE;}
				if(i == 4 && can_display_unicode_string_function(SIGN_POWER_4, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_4); return TRUE;}
				if(i == 5 && can_display_unicode_string_function(SIGN_POWER_5, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_5); return TRUE;}
				if(i == 6 && can_display_unicode_string_function(SIGN_POWER_6, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_6); return TRUE;}
				if(i == 7 && can_display_unicode_string_function(SIGN_POWER_7, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_7); return TRUE;}
				if(i == 8 && can_display_unicode_string_function(SIGN_POWER_8, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_8); return TRUE;}
				if(i == 9 && can_display_unicode_string_function(SIGN_POWER_9, (void*) expression_edit_widget())) {insert_text(SIGN_POWER_9); return TRUE;}
			}
		}
		if(b2) {
			string str = "(";
			str += print_with_evalops(Number(1, 1));
			str += "/";
			str += print_with_evalops(Number(i, 1));
			str += ")";
			insert_text(str.c_str());
		} else {
			string str = "^";
			str += print_with_evalops(Number(i, 1));
			insert_text(str.c_str());
		}
		return TRUE;
	}
	return FALSE;
}

guint button_press_timeout_id = 0;
GtkWidget *button_press_timeout_w = NULL;
int button_press_timeout_side = 0;
bool button_press_timeout_done = false;

void on_button_del_clicked(GtkButton*, gpointer);

GdkEventButton long_press_menu_event;

gboolean keypad_long_press_timeout(gpointer data) {
	if(!button_press_timeout_w) {
		button_press_timeout_id = 0;
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		button_press_timeout_done = false;
		return FALSE;
	}
	if(data) {
		hide_tooltip(GTK_WIDGET(data));
		if(GTK_WIDGET(data) == GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_to"))) {
			if(calculator_busy()) return TRUE;
			update_mb_to_menu();
		}
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_widget(GTK_MENU(data), button_press_timeout_w, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent*) &long_press_menu_event);
#else
		gtk_menu_popup(GTK_MENU(data), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	} else if(button_press_timeout_w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move2")) && button_press_timeout_side) {
		hide_tooltip(button_press_timeout_w);
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, gtk_text_buffer_get_insert(expression_edit_buffer()));
		if(button_press_timeout_side == 2) {
			if(gtk_text_iter_is_end(&iter)) gtk_text_buffer_get_start_iter(expression_edit_buffer(), &iter);
			else gtk_text_iter_forward_char(&iter);
		} else {
			if(gtk_text_iter_is_start(&iter)) gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iter);
			else gtk_text_iter_backward_char(&iter);
		}
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
		button_press_timeout_done = true;
		return TRUE;
	} else if(button_press_timeout_w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move")) && button_press_timeout_side) {
		hide_tooltip(button_press_timeout_w);
		if(button_press_timeout_side == 2) expression_history_down();
		else expression_history_up();
		button_press_timeout_done = true;
		return TRUE;
	} else if(button_press_timeout_w == GTK_WIDGET(gtk_builder_get_object(main_builder, "button_del")) && custom_buttons[26].type[0] == -1) {
		hide_tooltip(button_press_timeout_w);
		on_button_del_clicked(GTK_BUTTON(button_press_timeout_w), NULL);
		button_press_timeout_done = true;
		return TRUE;
	} else {
		on_keypad_button_alt(button_press_timeout_w, false);
	}
	button_press_timeout_done = true;
	button_press_timeout_id = 0;
	return FALSE;
}

gboolean keypad_unblock_timeout(gpointer w) {
	while(gtk_events_pending()) gtk_main_iteration();
	g_signal_handlers_unblock_matched(w, (GSignalMatchType) (G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_ID), g_signal_lookup("clicked", G_OBJECT_TYPE(w)), 0, NULL, NULL, NULL);
	g_signal_handlers_unblock_matched(w, (GSignalMatchType) (G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_ID), g_signal_lookup("toggled", G_OBJECT_TYPE(w)), 0, NULL, NULL, NULL);
	return FALSE;
}

gboolean on_keypad_button_button_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_id != 0) {
		g_source_remove(button_press_timeout_id);
		button_press_timeout_id = 0;
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		button_press_timeout_done = false;
	} else if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_done) {
		button_press_timeout_done = false;
		bool b_this = (button_press_timeout_w == w);
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		if(b_this) {
			g_signal_handlers_block_matched((gpointer) w, (GSignalMatchType) (G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_ID), g_signal_lookup("clicked", G_OBJECT_TYPE(w)), 0, NULL, NULL, NULL);
			g_signal_handlers_block_matched((gpointer) w, (GSignalMatchType) (G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_ID), g_signal_lookup("toggled", G_OBJECT_TYPE(w)), 0, NULL, NULL, NULL);
			g_timeout_add(50, keypad_unblock_timeout, (gpointer) w);
			return FALSE;
		}
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button == 1) {
		button_press_timeout_w = w;
		button_press_timeout_side = 0;
		button_press_timeout_id = g_timeout_add(500, keypad_long_press_timeout, NULL);
		return FALSE;
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && (button == 2 || button == 3)) {
		if(on_keypad_button_alt(w, button == 2)) return TRUE;
	}
	return FALSE;
}
bool block_del = false;
gboolean on_button_del_button_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if((button == 1 && custom_buttons[26].type[0] != -1) || (button == 3 && custom_buttons[26].type[1] != -1) || (button == 2 && custom_buttons[26].type[2] != -1)) return on_keypad_button_button_event(w, event, NULL);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_id != 0) {
		g_source_remove(button_press_timeout_id);
		bool b_this = (button_press_timeout_w == w);
		button_press_timeout_id = 0;
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		if(button_press_timeout_done) {
			button_press_timeout_done = false;
			if(b_this) {
				block_del = true;
				return FALSE;
			}
		}
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button == 1) {
		button_press_timeout_w = w;
		button_press_timeout_side = 0;
		button_press_timeout_id = g_timeout_add(250, keypad_long_press_timeout, NULL);
		return FALSE;
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && (button == 2 || button == 3)) {
		on_keypad_button_alt(w, button == 2);
	}
	return FALSE;
}
gboolean on_button_move2_button_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdouble x = 0, y = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	gdk_event_get_coords((GdkEvent*) event, &x, &y);
	if((button == 1 && custom_buttons[1].type[0] != -1) || (button == 3 && custom_buttons[1].type[1] != -1) || (button == 2 && custom_buttons[1].type[2] != -1)) return on_keypad_button_button_event(w, event, NULL);
	hide_tooltip(w);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_id != 0) {
		g_source_remove(button_press_timeout_id);
		bool b_this = (button_press_timeout_w == w);
		button_press_timeout_id = 0;
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		if(button_press_timeout_done) {
			button_press_timeout_done = false;
			if(b_this) return FALSE;
		}
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button == 1) {
		button_press_timeout_w = w;
		if(gdk_event_get_window((GdkEvent*) event) && x > gdk_window_get_width(gdk_event_get_window((GdkEvent*) event)) / 2) button_press_timeout_side = 2;
		else button_press_timeout_side = 1;
		button_press_timeout_id = g_timeout_add(250, keypad_long_press_timeout, NULL);
		return FALSE;
	}
	hide_tooltip(w);
	if(button == 2 || button == 3) {
		if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE) {
			GtkTextIter iter;
			if(gdk_event_get_window((GdkEvent*) event) && x > gdk_window_get_width(gdk_event_get_window((GdkEvent*) event)) / 2) {
				gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iter);
			} else {
				gtk_text_buffer_get_start_iter(expression_edit_buffer(), &iter);
			}
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
		}
	} else {
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, gtk_text_buffer_get_insert(expression_edit_buffer()));
		if(gdk_event_get_window((GdkEvent*) event) && x > gdk_window_get_width(gdk_event_get_window((GdkEvent*) event)) / 2) {
			if(gtk_text_iter_is_end(&iter)) gtk_text_buffer_get_start_iter(expression_edit_buffer(), &iter);
			else gtk_text_iter_forward_char(&iter);
		} else {
			if(gtk_text_iter_is_start(&iter)) gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iter);
			else gtk_text_iter_backward_char(&iter);
		}
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
	}
	return FALSE;
}
gboolean on_button_move_button_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdouble x = 0, y = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	gdk_event_get_coords((GdkEvent*) event, &x, &y);
	if((button == 1 && custom_buttons[0].type[0] != -1) || (button == 3 && custom_buttons[0].type[1] != -1) || (button == 2 && custom_buttons[0].type[2] != -1)) return on_keypad_button_button_event(w, event, NULL);
	hide_tooltip(w);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_id != 0) {
		g_source_remove(button_press_timeout_id);
		bool b_this = (button_press_timeout_w == w);
		button_press_timeout_id = 0;
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		if(button_press_timeout_done) {
			button_press_timeout_done = false;
			if(b_this) return FALSE;
		}
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button == 1) {
		button_press_timeout_w = w;
		if(gdk_event_get_window((GdkEvent*) event) && x > gdk_window_get_width(gdk_event_get_window((GdkEvent*) event)) / 2) button_press_timeout_side = 2;
		else button_press_timeout_side = 1;
		button_press_timeout_id = g_timeout_add(250, keypad_long_press_timeout, NULL);
		return FALSE;
	}
	hide_tooltip(w);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button == 1) {
		if(gdk_event_get_window((GdkEvent*) event) && (x < 0 || y < 0 || x > gdk_window_get_width(gdk_event_get_window((GdkEvent*) event)) || y > gdk_window_get_height(gdk_event_get_window((GdkEvent*) event)))) return FALSE;
		if(gdk_event_get_window((GdkEvent*) event) && x > gdk_window_get_width(gdk_event_get_window((GdkEvent*) event)) / 2) expression_history_down();
		else expression_history_up();
	}
	return FALSE;
}

gboolean on_keypad_menu_button_button_event(GtkWidget *w, GdkEventButton *event, gpointer data) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_id != 0) {
		g_source_remove(button_press_timeout_id);
		button_press_timeout_id = 0;
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		button_press_timeout_done = false;
	} else if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button_press_timeout_done) {
		button_press_timeout_done = false;
		bool b_this = (button_press_timeout_w == w);
		button_press_timeout_w = NULL;
		button_press_timeout_side = 0;
		if(b_this) return TRUE;
	}
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS && button == 1) {
		button_press_timeout_w = w;
		button_press_timeout_side = 0;
		long_press_menu_event = *event;
		button_press_timeout_id = g_timeout_add(500, keypad_long_press_timeout, data);
		return FALSE;
	}
	bool b = (gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && (button == 2 || button == 3));
	if(b) {
		if(GTK_WIDGET(data) == GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_to"))) {
			if(calculator_busy()) return TRUE;
			update_mb_to_menu();
		}
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(data), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(data), NULL, NULL, NULL, NULL, button, gdk_event_get_time((GdkEvent*) event));
#endif
		return TRUE;
	}
	return FALSE;
}

#define DO_CUSTOM_BUTTON_1(i) \
	if(custom_buttons[i].type[0] != -1) {\
		do_shortcut(custom_buttons[i].type[0], custom_buttons[i].value[0]);\
		return;\
	}

#define DO_CUSTOM_BUTTON_CX1(i) DO_CUSTOM_BUTTON_1(28 + i)

void on_button_fp_clicked(GtkWidget*, gpointer) {
	open_convert_floatingpoint();
}
void on_button_functions_clicked(GtkButton*, gpointer) {
	manage_functions(main_window());
}
void on_button_variables_clicked(GtkButton*, gpointer) {
	manage_variables(main_window());
}
void on_button_units_clicked(GtkButton*, gpointer) {
	manage_units(main_window());
}
void on_button_bases_clicked(GtkButton*, gpointer) {
	open_convert_number_bases();
}

/*
	Button clicked -- insert text (1,2,3,... +,-,...)
*/
void on_button_zero_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(8)
	insert_text("0");
}
void on_button_one_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(9)
	insert_text("1");
}
void on_button_two_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(10)
	insert_text("2");
}
void on_button_three_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(11)
	insert_text("3");
}
void on_button_four_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(12)
	insert_text("4");
}
void on_button_five_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(13)
	insert_text("5");
}
void on_button_six_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(14)
	insert_text("6");
}
void on_button_seven_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(15)
	insert_text("7");
}
void on_button_eight_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(16)
	insert_text("8");
}
void on_button_nine_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(17)
	insert_text("9");
}
void on_button_c1_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(1)
}
void on_button_c2_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(2)
}
void on_button_c3_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(3)
}
void on_button_c4_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(4)
}
void on_button_c5_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(5)
}
void on_button_c6_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(6)
}
void on_button_c7_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(7)
}
void on_button_c8_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(8)
}
void on_button_c9_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(9)
}
void on_button_c10_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(10)
}
void on_button_c11_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(11)
}
void on_button_c12_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(12)
}
void on_button_c13_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(13)
}
void on_button_c14_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(14)
}
void on_button_c15_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(15)
}
void on_button_c16_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(16)
}
void on_button_c17_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(17)
}
void on_button_c18_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(18)
}
void on_button_c19_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(19)
}
void on_button_c20_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_CX1(20)
}
void on_button_a_clicked(GtkButton*, gpointer) {
	insert_text(printops.lower_case_numbers ? "a" : "A");
}
void on_button_b_clicked(GtkButton*, gpointer) {
	insert_text(printops.lower_case_numbers ? "b" : "B");
}
void on_button_c_clicked(GtkButton*, gpointer) {
	insert_text(printops.lower_case_numbers ? "c" : "C");
}
void on_button_d_clicked(GtkButton*, gpointer) {
	insert_text(printops.lower_case_numbers ? "d" : "D");
}
void on_button_e_clicked(GtkButton*, gpointer) {
	insert_text(printops.lower_case_numbers ? "e" : "E");
}
void on_button_f_clicked(GtkButton*, gpointer) {
	insert_text(printops.lower_case_numbers ? "f" : "F");
}
void on_button_dot_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(18)
	insert_text(CALCULATOR->getDecimalPoint().c_str());
}
void on_button_brace_open_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(6)
	insert_text("(");
}
void on_button_brace_close_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(7)
	insert_text(")");
}
void on_button_brace_wrap_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(5)
	brace_wrap();
}
void on_button_i_clicked(GtkButton*, gpointer) {
	insert_variable(CALCULATOR->v_i);
}
void on_button_move_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(0)
}
void on_button_move2_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(1)
}
void on_button_percent_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(2)
	insert_text("%");
}
void on_button_si_clicked(GtkButton*, gpointer) {
	if(latest_button_unit) {
		insert_button_unit(NULL, (gpointer) latest_button_unit);
	} else {
		insert_text("kg");
	}
}
void on_button_euro_clicked(GtkButton*, gpointer) {
	if(latest_button_currency) {
		insert_button_currency(NULL, (gpointer) latest_button_currency);
	} else {
		insert_text("€");
	}
}

/*
	DEL button clicked -- delete in expression entry
*/
void on_button_del_clicked(GtkButton*, gpointer) {
	if(block_del) {block_del = false; return;}
	DO_CUSTOM_BUTTON_1(26)
	if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		overwrite_expression_selection(NULL);
		return;
	}
	block_completion();
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expression_edit_buffer());
	GtkTextIter ipos, iend;
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mpos);
	if(gtk_text_iter_is_end(&ipos)) {
		gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
		if(gtk_text_iter_backward_char(&ipos)) {
			gtk_text_buffer_delete(expression_edit_buffer(), &ipos, &iend);
		}
	} else {
		iend = ipos;
		if(!gtk_text_iter_forward_char(&iend)) {
			gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
		}
		gtk_text_buffer_delete(expression_edit_buffer(), &ipos, &iend);
	}
	focus_keeping_selection();
	unblock_completion();
}

/*
	"Execute" clicked
*/
void on_button_execute_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(28)
	execute_expression();
}

/*
	AC button clicked -- clear expression entry
*/
void on_button_ac_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(27)
	clear_expression_text();
	focus_keeping_selection();
}

void on_button_to_clicked(GtkButton*, gpointer) {
	if(calculator_busy()) return;
	string to_str;
	GtkTextIter istart, iend;
	gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
	gtk_text_buffer_select_range(expression_edit_buffer(), &iend, &iend);
	if(!gtk_widget_is_focus(expression_edit_widget())) gtk_widget_grab_focus(expression_edit_widget());
	if(printops.use_unicode_signs && can_display_unicode_string_function("➞", (void*) expression_edit_widget())) {
		to_str = "➞";
	} else {
		gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
		gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
		to_str = CALCULATOR->localToString();
		remove_blank_ends(to_str);
		to_str += ' ';
		if(strlen(gstr) > 0 && gstr[strlen(gstr) - 1] != ' ') to_str.insert(0, " ");
		g_free(gstr);
	}
	gtk_text_buffer_insert_at_cursor(expression_edit_buffer(), to_str.c_str(), -1);
}
void on_button_new_function_clicked(GtkButton*, gpointer) {
	edit_function("", NULL, main_window());
}
void on_button_fac_clicked(GtkButton*, gpointer) {
	if(rpn_mode || evalops.parse_options.parsing_mode == PARSING_MODE_RPN || is_at_beginning_of_expression()) {
		insert_button_function(CALCULATOR->f_factorial);
	}
	bool do_exec = wrap_expression_selection(NULL, true) > 0;
	insert_text("!");
	if(do_exec) execute_expression();
}
void on_button_comma_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(4)
	insert_text(CALCULATOR->getComma().c_str());
}
void on_button_x_clicked(GtkButton*, gpointer) {
	insert_text("x");
}
void on_button_y_clicked(GtkButton*, gpointer) {
	insert_text("y");
}
void on_button_z_clicked(GtkButton*, gpointer) {
	insert_text("z");
}
void on_button_xequals_clicked(GtkButton*, gpointer) {
	insert_text("=");
}
void on_button_plusminus_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(3)
	wrap_expression_selection();
	insert_text("±");
}
void on_button_factorize_clicked(GtkButton*, gpointer) {
	if(evalops.structuring == STRUCTURING_FACTORIZE) executeCommand(COMMAND_EXPAND);
	else executeCommand(COMMAND_FACTORIZE);
}
void on_button_factorize2_clicked(GtkButton*, gpointer) {
	executeCommand(COMMAND_FACTORIZE);
}

void on_button_add_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(23)
	if(use_keypad_buttons_for_history()) {
		history_operator(expression_add_sign());
		return;
	}
	if(rpn_mode) {
		calculateRPN(OPERATION_ADD);
		return;
	}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode(expression_add_sign())) return;
		wrap_expression_selection();
	}
	insert_text(expression_add_sign());
}

void on_button_sub_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(24)
	if(use_keypad_buttons_for_history()) {
		history_operator(expression_sub_sign());
		return;
	}
	if(rpn_mode) {
		calculateRPN(OPERATION_SUBTRACT);
		return;
	}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode(expression_sub_sign())) return;
		wrap_expression_selection();
	}
	insert_text(expression_sub_sign());
}
void on_button_times_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(22)
	if(use_keypad_buttons_for_history()) {
		history_operator(expression_times_sign());
		return;
	}
	if(rpn_mode) {
		calculateRPN(OPERATION_MULTIPLY);
		return;
	}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode(expression_times_sign())) return;
		wrap_expression_selection();
	}
	insert_text(expression_times_sign());
}
void on_button_divide_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(21)
	if(use_keypad_buttons_for_history()) {
		history_operator(expression_divide_sign());
		return;
	}
	if(rpn_mode) {
		calculateRPN(OPERATION_DIVIDE);
		return;
	}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode(expression_divide_sign())) return;
		wrap_expression_selection();
	}
	insert_text(expression_divide_sign());
}
void on_button_ans_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(27)
	insert_answer_variable();
}
void on_button_exp_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(19)
	if(rpn_mode) {
		calculateRPN(OPERATION_EXP10);
		return;
	}
	if((evalops.parse_options.parsing_mode != PARSING_MODE_RPN && wrap_expression_selection() > 0) || (evalops.parse_options.base != 10 && evalops.parse_options.base >= 2)) {
		insert_text((expression_times_sign() + i2s(evalops.parse_options.base) + "^").c_str());
	} else {
		if(printops.exp_display == EXP_LOWERCASE_E) insert_text("e");
		else insert_text("E");
	}
}
void on_button_xy_clicked(GtkButton*, gpointer) {
	DO_CUSTOM_BUTTON_1(20)
	if(use_keypad_buttons_for_history()) {
		history_operator("^");
		return;
	}
	if(rpn_mode) {
		calculateRPN(OPERATION_RAISE);
		return;
	}
	if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
		if(do_chain_mode("^")) return;
		wrap_expression_selection();
	}
	insert_text("^");
}
void on_button_square_clicked() {
	if(rpn_mode) {
		calculateRPN(CALCULATOR->f_sq);
		return;
	}
	if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN || chain_mode || wrap_expression_selection() < 0) {
		insert_button_function(CALCULATOR->f_sq);
	} else {
		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_POWER_2, (void*) expression_edit_widget())) insert_text(SIGN_POWER_2);
		else insert_text("^2");
	}
}

/*
	Button clicked -- insert corresponding function
*/
void on_button_sqrt_clicked(GtkButton*, gpointer) {
	insert_button_function(CALCULATOR->f_sqrt);
}
void on_button_log_clicked(GtkButton*, gpointer) {
	MathFunction *f = CALCULATOR->getActiveFunction("log10");
	if(f) {
		insert_button_function(f);
	} else {
		show_message(_("log10 function not found."));
	}
}
void on_button_ln_clicked(GtkButton*, gpointer) {
	insert_button_function(CALCULATOR->f_ln);
}

void on_button_reciprocal_clicked(GtkButton*, gpointer) {
	if(rpn_mode || evalops.parse_options.parsing_mode == PARSING_MODE_RPN || is_at_beginning_of_expression()) {
		insert_button_function(CALCULATOR->getActiveFunction("inv"));
	} else {
		bool do_exec = wrap_expression_selection(NULL, true) > 0;
		insert_text("^-1");
		if(do_exec) execute_expression();
	}
}
void on_button_idiv_clicked(GtkButton*, gpointer) {
	if(expression_is_empty() || rpn_mode || evalops.parse_options.parsing_mode == PARSING_MODE_RPN || is_at_beginning_of_expression() || wrap_expression_selection() < 0) {
		insert_button_function(CALCULATOR->getActiveFunction("div"));
	} else {
		insert_text("//");
	}
}

void on_button_bin_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		update_keypad_programming_base();
		return;
	}
	set_output_base(BASE_BINARY);
	set_input_base(BASE_BINARY, false, false);
	focus_keeping_selection();
}
void on_button_oct_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		update_keypad_programming_base();
		return;
	}
	set_output_base(BASE_OCTAL);
	set_input_base(BASE_OCTAL, false, false);
	focus_keeping_selection();
}
void on_button_dec_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		update_keypad_programming_base();
		return;
	}
	set_output_base(BASE_DECIMAL);
	set_input_base(BASE_DECIMAL, false, false);
	focus_keeping_selection();
}
void on_button_hex_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		update_keypad_programming_base();
		return;
	}
	set_output_base(BASE_HEXADECIMAL);
	set_input_base(BASE_HEXADECIMAL, false, false);
	update_setbase();
	focus_keeping_selection();
}

/*
	STO button clicked -- store result
*/
void on_button_store_clicked(GtkButton*, gpointer) {
	if(current_displayed_result() && current_result() && !current_result()->isZero()) add_as_variable();
	else edit_variable(NULL, NULL, NULL, main_window());
}

void set_type(const char *var, AssumptionType at) {
	if(calculation_blocked()) return;
	Variable *v = CALCULATOR->getActiveVariable(var);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	if(!uv->assumptions()) uv->setAssumptions(new Assumptions());
	uv->assumptions()->setType(at);
	expression_calculation_updated();
}
void set_sign(const char *var, AssumptionSign as) {
	if(calculation_blocked()) return;
	Variable *v = CALCULATOR->getActiveVariable(var);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	if(!uv->assumptions()) uv->setAssumptions(new Assumptions());
	uv->assumptions()->setSign(as);
	expression_calculation_updated();
}
void reset_assumptions(const char *var) {
	Variable *v = CALCULATOR->getActiveVariable(var);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	uv->setAssumptions(NULL);
	expression_calculation_updated();
}

void set_x_assumptions_items() {
	Variable *v = CALCULATOR->getActiveVariable("x");
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	block_calculation();
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	switch(ass->sign()) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_unknown")), TRUE);}
	}
	switch(ass->type()) {
		case ASSUMPTION_TYPE_BOOLEAN: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_boolean")), TRUE); break;}
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_none")), TRUE);}
	}
	unblock_calculation();
}
void on_mb_x_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	set_x_assumptions_items();
}

void on_menu_item_x_default_activate() {
	reset_assumptions("x");
}
void on_menu_item_x_boolean_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_BOOLEAN);
}
void on_menu_item_x_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_INTEGER);
}
void on_menu_item_x_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_RATIONAL);
}
void on_menu_item_x_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_REAL);
}
void on_menu_item_x_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_COMPLEX);
}
void on_menu_item_x_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_NUMBER);
}
void on_menu_item_x_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_NONE);
}
void on_menu_item_x_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_NONMATRIX);
}
void on_menu_item_x_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NONZERO);
}
void on_menu_item_x_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_POSITIVE);
}
void on_menu_item_x_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NONNEGATIVE);
}
void on_menu_item_x_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NEGATIVE);
}
void on_menu_item_x_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NONPOSITIVE);
}
void on_menu_item_x_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_UNKNOWN);
}

void set_y_assumptions_items() {
	Variable *v = CALCULATOR->getActiveVariable("y");
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	block_calculation();
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	switch(ass->sign()) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_unknown")), TRUE);}
	}
	switch(ass->type()) {
		case ASSUMPTION_TYPE_BOOLEAN: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_boolean")), TRUE); break;}
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_none")), TRUE);}
	}
	unblock_calculation();
}
void on_mb_y_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	set_y_assumptions_items();
}

void on_menu_item_y_default_activate() {
	reset_assumptions("y");
}
void on_menu_item_y_boolean_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_BOOLEAN);
}
void on_menu_item_y_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_INTEGER);
}
void on_menu_item_y_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_RATIONAL);
}
void on_menu_item_y_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_REAL);
}
void on_menu_item_y_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_COMPLEX);
}
void on_menu_item_y_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_NUMBER);
}
void on_menu_item_y_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_NONE);
}
void on_menu_item_y_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_NONMATRIX);
}
void on_menu_item_y_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NONZERO);
}
void on_menu_item_y_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_POSITIVE);
}
void on_menu_item_y_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NONNEGATIVE);
}
void on_menu_item_y_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NEGATIVE);
}
void on_menu_item_y_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NONPOSITIVE);
}
void on_menu_item_y_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_UNKNOWN);
}

void set_z_assumptions_items() {
	Variable *v = CALCULATOR->getActiveVariable("z");
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	block_calculation();
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	switch(ass->sign()) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_unknown")), TRUE);}
	}
	switch(ass->type()) {
		case ASSUMPTION_TYPE_BOOLEAN: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_boolean")), TRUE); break;}
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_none")), TRUE);}
	}
	unblock_calculation();
}
void on_mb_z_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	set_z_assumptions_items();
}

void on_menu_item_z_default_activate() {
	reset_assumptions("z");
}
void on_menu_item_z_boolean_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_BOOLEAN);
}
void on_menu_item_z_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_INTEGER);
}
void on_menu_item_z_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_RATIONAL);
}
void on_menu_item_z_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_REAL);
}
void on_menu_item_z_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_COMPLEX);
}
void on_menu_item_z_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_NUMBER);
}
void on_menu_item_z_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_NONE);
}
void on_menu_item_z_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_NONMATRIX);
}
void on_menu_item_z_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NONZERO);
}
void on_menu_item_z_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_POSITIVE);
}
void on_menu_item_z_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NONNEGATIVE);
}
void on_menu_item_z_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NEGATIVE);
}
void on_menu_item_z_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NONPOSITIVE);
}
void on_menu_item_z_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_UNKNOWN);
}

void on_mb_to_activated(GtkMenuItem*, gpointer p) {
	GtkTreePath *path = gtk_tree_path_new_from_indices(GPOINTER_TO_INT(p), -1);
	on_completion_match_selected(NULL, path, NULL, NULL);
	gtk_tree_path_free(path);
}

void update_mb_to_menu() {
	if(expression_modified() && !rpn_mode && (!result_is_autocalculated() || parsed_in_result)) execute_expression(true);
	GtkWidget *sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_to"));
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	do_completion(true);
	GtkWidget *item;
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_sort), &iter)) return;
	bool b_hidden = false;
	int p_type = 0;
	void *o = NULL;
	int n = 0, n2 = 0;
	gint index = 0;
	if(current_displayed_result() && contains_convertible_unit(*current_displayed_result())) {
		bool b = true;
		do {
			gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 2, &o, 8, &p_type, -1);
			if(p_type == 1 && ((ExpressionItem*) o)->type() == TYPE_UNIT) {
				b = false;
				break;
			}
		} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_sort), &iter));
		if(b) {
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_store), &iter);
			const char *si_units[] = {"m", "g", "s", "A", "K", "L", "J", "N"};
			size_t n_si = 8;
			do {
				gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 2, &o, 8, &p_type, -1);
				if(p_type == 1 && ((ExpressionItem*) o)->type() == TYPE_UNIT && ((Unit*) o)->subtype() != SUBTYPE_COMPOSITE_UNIT && ((Unit*) o)->isSIUnit()) {
					for(size_t i = 0; i < n_si; i++) {
						if(((ExpressionItem*) o)->referenceName() == si_units[i]) {
							gtk_list_store_set(GTK_LIST_STORE(completion_store), &iter, 3, TRUE, -1);
							break;
						}
					}
				}
			} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_store), &iter));
		}
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_sort), &iter);
	}
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 2, &o, 8, &p_type, -1);
		if(p_type != 1 || ((ExpressionItem*) o)->type() != TYPE_UNIT) {
			ADD_MB_TO_ITEM(1)
		}
		index++;
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_sort), &iter));
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_sort), &iter);
	index = 0;
	Unit *u_local_currency = CALCULATOR->getLocalCurrency();
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 2, &o, 8, &p_type, -1);
		if(p_type == 1 && ((ExpressionItem*) o)->type() == TYPE_UNIT) {
			if(((ExpressionItem*) o)->isHidden() && o != u_local_currency) {
				b_hidden = true;
			} else {
				ADD_MB_TO_ITEM(2)
			}
		}
		index++;
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_sort), &iter));
	if(b_hidden) {
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_sort), &iter);
		index = 0;
		if(n2 > 0) {SUBMENU_ITEM(_("more"), sub);}
		do {
			gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 2, &o, 8, &p_type, -1);
			if(p_type == 1 && ((ExpressionItem*) o)->type() == TYPE_UNIT && ((ExpressionItem*) o)->isHidden() && o != u_local_currency) {
				ADD_MB_TO_ITEM(3)
			}
			index++;
		} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_sort), &iter));
	}
}

gboolean on_mb_to_button_press_event(GtkWidget, GdkEventButton*, gpointer) {
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 22)) {
		if(calculator_busy()) return TRUE;
		update_mb_to_menu();
	}
	return FALSE;
}
gboolean on_mb_to_button_release_event(GtkWidget, GdkEventButton*, gpointer) {
	if(RUNTIME_CHECK_GTK_VERSION(3, 22)) {
		if(calculator_busy()) return TRUE;
		update_mb_to_menu();
	}
	return FALSE;
}

void update_mb_units_menu() {
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_units"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	const char *si_units[] = {"m", "g", "s", "A", "K"};
	size_t i_added = 0;
	for(size_t i = recent_units.size(); i > 0; i--) {
		if(!recent_units[i - 1]->isLocal() && CALCULATOR->stillHasUnit(recent_units[i - 1])) {
			MENU_ITEM_WITH_OBJECT(recent_units[i - 1], insert_unit_from_menu)
			i_added++;
		}
	}
	for(size_t i = 0; i_added < 5 && i < 5; i++) {
		Unit * u = CALCULATOR->getActiveUnit(si_units[i]);
		if(u && !u->isHidden()) {
			MENU_ITEM_WITH_OBJECT(u, insert_unit_from_menu)
			i_added++;
		}
	}

	MENU_SEPARATOR
	Prefix *p = CALCULATOR->getPrefix("giga");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix_from_menu, p);}
	p = CALCULATOR->getPrefix("mega");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix_from_menu, p);}
	p = CALCULATOR->getPrefix("kilo");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix_from_menu, p);}
	p = CALCULATOR->getPrefix("milli");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix_from_menu, p);}
	p = CALCULATOR->getPrefix("micro");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix_from_menu, p);}
}

void on_popup_menu_fx_edit_activate(GtkMenuItem*, gpointer data) {
	edit_function("", (MathFunction*) data, main_window());
}
void on_popup_menu_fx_delete_activate(GtkMenuItem*, gpointer data) {
	MathFunction *f = (MathFunction*) data;
	if(f && f->isLocal()) remove_function(f);
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "menu_fx")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "mb_fx")), FALSE);
	focus_keeping_selection();
}

gulong on_popup_menu_fx_edit_activate_handler = 0, on_popup_menu_fx_delete_activate_handler = 0;

gboolean on_menu_fx_popup_menu(GtkWidget*, gpointer data) {
	if(calculator_busy()) return TRUE;
	if(on_popup_menu_fx_edit_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_fx_edit"), on_popup_menu_fx_edit_activate_handler);
	if(on_popup_menu_fx_delete_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_fx_delete"), on_popup_menu_fx_delete_activate_handler);
	on_popup_menu_fx_edit_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_fx_edit"), "activate", G_CALLBACK(on_popup_menu_fx_edit_activate), data);
	on_popup_menu_fx_delete_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_fx_delete"), "activate", G_CALLBACK(on_popup_menu_fx_delete_activate), data);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_fx")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_fx")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_menu_fx_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	/* Ignore double-clicks and triple-clicks */
	if(gdk_event_triggers_context_menu((GdkEvent *) event) && gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS) {
		on_menu_fx_popup_menu(widget, data);
		return TRUE;
	}
	return FALSE;
}

void update_mb_fx_menu() {
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_fx"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	bool b = false;
	for(size_t i = 0; i < user_functions.size(); i++) {
		if(!user_functions[i]->isHidden()) {
			MENU_ITEM_WITH_OBJECT(user_functions[i], insert_button_function_default)
			g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_fx_button_press), user_functions[i]);
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_fx_popup_menu), (gpointer) user_functions[i]);
			b = true;
		}
	}
	bool b2 = false;
	for(size_t i = recent_functions.size(); i > 0; i--) {
		if(!recent_functions[i - 1]->isLocal() && CALCULATOR->stillHasFunction(recent_functions[i - 1])) {
			if(!b2 && b) {MENU_SEPARATOR}
			b2 = true;
			MENU_ITEM_WITH_OBJECT(recent_functions[i - 1], insert_button_function_save)
		}
	}
	if(b2 || b) {MENU_SEPARATOR}
	MENU_ITEM(_("All functions"), on_menu_item_manage_functions_activate);
}

void insert_button_sqrt2() {
	insert_text((CALCULATOR->f_sqrt->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_FUNCTION, true) + "(2)").c_str());
}

void update_mb_pi_menu() {

	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_pi"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);

	Variable *v = CALCULATOR->getActiveVariable("pythagoras");
	MENU_ITEM(v ? v->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str() : SIGN_SQRT "2", insert_button_sqrt2)
	MENU_ITEM_WITH_OBJECT(CALCULATOR->v_euler, insert_button_variable);
	v = CALCULATOR->getActiveVariable("golden");
	if(v) {MENU_ITEM_WITH_OBJECT(v, insert_button_variable);}
	MENU_SEPARATOR

	int i_added = 0;
	for(size_t i = recent_variables.size(); i > 0; i--) {
		if(!recent_variables[i - 1]->isLocal() && CALCULATOR->stillHasVariable(recent_variables[i - 1])) {
			MENU_ITEM_WITH_OBJECT(recent_variables[i - 1], insert_variable_from_menu)
			i_added++;
		}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("c");
		if(v) {MENU_ITEM_WITH_OBJECT(v, insert_button_variable); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("newtonian_constant");
		if(v) {MENU_ITEM_WITH_OBJECT(v, insert_button_variable); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("planck");
		if(v) {MENU_ITEM_WITH_OBJECT(v, insert_button_variable); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("boltzmann");
		if(v) {MENU_ITEM_WITH_OBJECT(v, insert_button_variable); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("avogadro");
		if(v) {MENU_ITEM_WITH_OBJECT(v, insert_button_variable); i_added++;}
	}

	MENU_SEPARATOR
	MENU_ITEM(_("All variables"), on_menu_item_manage_variables_activate);

}

void on_popup_menu_sto_set_activate(GtkMenuItem*, gpointer data) {
	KnownVariable *v = (KnownVariable*) data;
	v->set(*current_result());
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "menu_sto")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "mb_sto")), FALSE);
	variable_edited(v);
	focus_keeping_selection();
}
void on_popup_menu_sto_add_activate(GtkMenuItem*, gpointer data) {
	KnownVariable *v = (KnownVariable*) data;
	MathStructure m(v->get());
	m.calculateAdd(*current_result(), evalops);
	v->set(m);
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "menu_sto")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "mb_sto")), FALSE);
	variable_edited(v);
	focus_keeping_selection();
}
void on_popup_menu_sto_sub_activate(GtkMenuItem*, gpointer data) {
	KnownVariable *v = (KnownVariable*) data;
	MathStructure m(v->get());
	m.calculateSubtract(*current_result(), evalops);
	v->set(m);
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "menu_sto")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "mb_sto")), FALSE);
	variable_edited(v);
	focus_keeping_selection();
}
void on_popup_menu_sto_edit_activate(GtkMenuItem*, gpointer data) {
	edit_variable(NULL, (Variable*) data, NULL, main_window());
}
void on_popup_menu_sto_delete_activate(GtkMenuItem*, gpointer data) {
	Variable *v = (Variable*) data;
	if(v && !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."));
		update_vmenu();
	} else if(v && v->isLocal()) {
		remove_variable(v);
	}
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "menu_sto")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "mb_sto")), FALSE);
	focus_keeping_selection();
}

gulong on_popup_menu_sto_set_activate_handler = 0, on_popup_menu_sto_add_activate_handler = 0, on_popup_menu_sto_sub_activate_handler = 0, on_popup_menu_sto_edit_activate_handler = 0, on_popup_menu_sto_delete_activate_handler = 0;

gboolean on_menu_sto_popup_menu(GtkWidget*, gpointer data) {
	if(calculator_busy()) return TRUE;
	if(on_popup_menu_sto_set_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_set"), on_popup_menu_sto_set_activate_handler);
	if(on_popup_menu_sto_add_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_add"), on_popup_menu_sto_add_activate_handler);
	if(on_popup_menu_sto_sub_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_sub"), on_popup_menu_sto_sub_activate_handler);
	if(on_popup_menu_sto_edit_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_edit"), on_popup_menu_sto_edit_activate_handler);
	if(on_popup_menu_sto_delete_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_delete"), on_popup_menu_sto_delete_activate_handler);
	if(((Variable*) data)->isKnown() && current_result() && current_displayed_result()) {
		on_popup_menu_sto_set_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_set"), "activate", G_CALLBACK(on_popup_menu_sto_set_activate), data);
		on_popup_menu_sto_add_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_add"), "activate", G_CALLBACK(on_popup_menu_sto_add_activate), data);
		on_popup_menu_sto_sub_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_sub"), "activate", G_CALLBACK(on_popup_menu_sto_sub_activate), data);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_sto_set")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_sto_add")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_sto_sub")), TRUE);
	} else {
		on_popup_menu_sto_set_activate_handler = 0;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_sto_set")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_sto_add")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_sto_sub")), FALSE);
	}
	on_popup_menu_sto_edit_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_edit"), "activate", G_CALLBACK(on_popup_menu_sto_edit_activate), data);
	on_popup_menu_sto_delete_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_delete"), "activate", G_CALLBACK(on_popup_menu_sto_delete_activate), data);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_sto")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_sto")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_menu_sto_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	/* Ignore double-clicks and triple-clicks */
	if(gdk_event_triggers_context_menu((GdkEvent *) event) && gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS) {
		on_menu_sto_popup_menu(widget, data);
		return TRUE;
	}
	return FALSE;
}
void update_mb_sto_menu() {
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_sto"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	bool b = false;
	for(size_t i = 0; i < user_variables.size(); i++) {
		if(!user_variables[i]->isHidden()) {
			MENU_ITEM_WITH_OBJECT(user_variables[i], insert_button_variable)
			g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_sto_button_press), user_variables[i]);
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_sto_popup_menu), (gpointer) user_variables[i]);
			b = true;
		}
	}
	//if(!b) {MENU_NO_ITEMS(_("No items found"))}
	if(b) {MENU_SEPARATOR}
	MENU_ITEM(_("MC (memory clear)"), memory_clear);
	MENU_ITEM(_("MR (memory recall)"), memory_recall);
	MENU_ITEM(_("MS (memory store)"), memory_store);
	MENU_ITEM(_("M+ (memory plus)"), memory_add);
	MENU_ITEM(_("M− (memory minus)"), memory_subtract);
}

void on_menu_item_mb_degrees_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_angle_unit(ANGLE_UNIT_DEGREES);
}
void on_menu_item_mb_radians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_angle_unit(ANGLE_UNIT_RADIANS);
}
void on_menu_item_mb_gradians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_angle_unit(ANGLE_UNIT_GRADIANS);
}
void update_keypad_angle() {
	switch(evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sin_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_degrees_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sin_degrees")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sin_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_degrees_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_cos_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_degrees_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_cos_degrees")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_cos_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_degrees_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_tan_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_degrees_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_tan_degrees")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_tan_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_degrees_activate, NULL);
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sin_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_gradians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sin_gradians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sin_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_gradians_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_cos_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_gradians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_cos_gradians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_cos_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_gradians_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_tan_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_gradians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_tan_gradians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_tan_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_gradians_activate, NULL);
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sin_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_radians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sin_radians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sin_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_radians_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_cos_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_radians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_cos_radians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_cos_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_radians_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_tan_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_radians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_tan_radians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_tan_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_mb_radians_activate, NULL);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sin_other")), TRUE);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_cos_other")), TRUE);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_tan_other")), TRUE);
			break;
		}
	}
}
void on_combobox_bits_changed(GtkComboBox *w, gpointer) {
	set_binary_bits(combo_get_bits(w));
}

void on_button_twos_out_toggled(GtkToggleButton *w, gpointer) {
	if(printops.base == 16) set_twos_complement(-1, gtk_toggle_button_get_active(w));
	else if(printops.base == 2) set_twos_complement(gtk_toggle_button_get_active(w));
	focus_keeping_selection();
}
void on_button_twos_in_toggled(GtkToggleButton *w, gpointer) {
	if(evalops.parse_options.base == 16) set_twos_complement(-1, -1, -1, gtk_toggle_button_get_active(w));
	else if(evalops.parse_options.base == 2) set_twos_complement(-1, -1, gtk_toggle_button_get_active(w));
	focus_keeping_selection();
}

void update_keypad_programming_base() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_bin"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_bin_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_oct"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_oct_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_dec"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_dec_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_hex"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_hex_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_bin")), printops.base == 2 && evalops.parse_options.base == 2);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_oct")), printops.base == 8 && evalops.parse_options.base == 8);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_dec")), printops.base == 10 && evalops.parse_options.base == 10);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_hex")), printops.base == 16 && evalops.parse_options.base == 16);
	gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_bin")), (printops.base == 2) != (evalops.parse_options.base == 2));
	gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_oct")), (printops.base == 8) != (evalops.parse_options.base == 8));
	gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_dec")), (printops.base == 10) != (evalops.parse_options.base == 10));
	gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_hex")), (printops.base == 16) != (evalops.parse_options.base == 16));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_bin"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_bin_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_oct"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_oct_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_dec"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_dec_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_hex"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_hex_toggled, NULL);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_twos_out"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_twos_out_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_twos_out")), (printops.base == 16 && printops.hexadecimal_twos_complement) || (printops.base == 2 && printops.twos_complement));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_twos_out"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_twos_out_toggled, NULL);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_twos_in"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_twos_in_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_twos_in")), (evalops.parse_options.base == 16 && evalops.parse_options.hexadecimal_twos_complement) || (evalops.parse_options.base == 2 && evalops.parse_options.twos_complement));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_twos_in"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_twos_in_toggled, NULL);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_bits"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_bits_changed, NULL);
	combo_set_bits(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_bits")), printops.binary_bits);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_bits"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_bits_changed, NULL);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_a")), evalops.parse_options.base >= 13 || evalops.parse_options.base == 11);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_b")), evalops.parse_options.base >= 13);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c")), evalops.parse_options.base >= 13);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_d")), evalops.parse_options.base >= 14);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_e")), evalops.parse_options.base >= 15);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_f")), evalops.parse_options.base >= 16);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_twos_out")), printops.base == 2 || printops.base == 16);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_twos_in")), evalops.parse_options.base == 2 || evalops.parse_options.base == 16);

}

void on_button_programmers_keypad_toggled(GtkToggleButton *w, gpointer) {
	previous_keypad = visible_keypad;
	if(gtk_toggle_button_get_active(w)) {
		visible_keypad = visible_keypad | PROGRAMMING_KEYPAD;
		if(evalops.approximation == APPROXIMATION_EXACT) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
			versatile_exact = true;
		} else {
			versatile_exact = false;
		}
		bool b_expression = false;
		bool b_result = false;
		if(programming_inbase > 0 && programming_outbase != 0 && (((programming_inbase != 10 || (programming_outbase != 10 && programming_outbase > 0 && programming_outbase <= 36)) && evalops.parse_options.base == 10 && printops.base == 10) || evalops.parse_options.base < 2 || printops.base < 2 || evalops.parse_options.base > 36 || printops.base > 16)) {
			if(printops.base != programming_outbase) {
				set_output_base(programming_outbase);
				b_result = true;
			}
			if(evalops.parse_options.base != programming_inbase) {
				set_input_base(programming_inbase);
				b_expression = true;
			}
		}
		if(b_expression) expression_format_updated();
		else if(b_result) result_format_updated();
		programming_inbase = 0;
		programming_outbase = 0;
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_left_buttons")), GTK_WIDGET(gtk_builder_get_object(main_builder, "programmers_keypad")));
		if(current_displayed_result()) {
			set_result_bases(*current_displayed_result());
			update_result_bases();
		} else if(!rpn_mode) {
			autocalc_result_bases();
		}
		gtk_stack_set_visible_child_name(GTK_STACK(gtk_builder_get_object(main_builder, "stack_keypad_top")), "page1");
	} else {
		if(versatile_exact && evalops.approximation == APPROXIMATION_TRY_EXACT) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), TRUE);
		}
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_left_buttons")), GTK_WIDGET(gtk_builder_get_object(main_builder, "versatile_keypad")));
		gtk_stack_set_visible_child_name(GTK_STACK(gtk_builder_get_object(main_builder, "stack_keypad_top")), "page0");
		visible_keypad = visible_keypad & ~PROGRAMMING_KEYPAD;
		programming_inbase = evalops.parse_options.base;
		programming_outbase = printops.base;
		if(evalops.parse_options.base != 10) clear_expression_text();
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_dec")), TRUE);
		clear_result_bases();
	}
	focus_keeping_selection();
}

gboolean on_hide_left_buttons_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button == 1) {
		bool hide_left_keypad = gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_left_buttons")));
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_left_buttons")), !hide_left_keypad);
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_right_buttons")), !hide_left_keypad);
		if(hide_left_keypad) {
			visible_keypad = visible_keypad | HIDE_LEFT_KEYPAD;
			GtkRequisition req;
			gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), &req, NULL);
			gtk_window_resize(main_window(), req.width + 24, 1);
		} else {
			visible_keypad = visible_keypad & ~HIDE_LEFT_KEYPAD;
		}
		focus_keeping_selection();
		return TRUE;
	}
	return FALSE;
}
gboolean on_hide_right_buttons_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_RELEASE && button == 1) {
		bool hide_right_keypad = gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_right_buttons")));
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_right_buttons")), !hide_right_keypad);
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_left_buttons")), !hide_right_keypad);
		if(hide_right_keypad) {
			visible_keypad = visible_keypad | HIDE_RIGHT_KEYPAD;
			GtkRequisition req;
			gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), &req, NULL);
			gtk_window_resize(main_window(), req.width + 24, 1);
		} else {
			visible_keypad = visible_keypad & ~HIDE_RIGHT_KEYPAD;
		}
		focus_keeping_selection();
		return TRUE;
	}
	return FALSE;
}

void on_combobox_base_changed(GtkComboBox *w, gpointer) {
	switch(gtk_combo_box_get_active(w)) {
		case 0: {
			set_output_base(BASE_BINARY);
			break;
		}
		case 1: {
			set_output_base(BASE_OCTAL);
			break;
		}
		case 2: {
			set_output_base(BASE_DECIMAL);
			break;
		}
		case 3: {
			set_output_base(BASE_DUODECIMAL);
			break;
		}
		case 4: {
			set_output_base(BASE_HEXADECIMAL);
			break;
		}
		case 5: {
			set_output_base(BASE_SEXAGESIMAL);
			break;
		}
		case 6: {
			set_output_base(BASE_TIME);
			break;
		}
		case 7: {
			set_output_base(BASE_ROMAN_NUMERALS);
			break;
		}
		case 8: {
			open_setbase(main_window(), true, false);
			break;
		}
	}
	focus_keeping_selection();
}

void on_combobox_numerical_display_changed(GtkComboBox *w, gpointer) {
	switch(gtk_combo_box_get_active(w)) {
		case 0: {
			set_min_exp(EXP_PRECISION, true);
			break;
		}
		case 1: {
			set_min_exp(EXP_BASE_3, true);
			break;
		}
		case 2: {
			set_min_exp(EXP_SCIENTIFIC, true);
			break;
		}
		case 3: {
			set_min_exp(EXP_PURE, true);
			break;
		}
		case 4: {
			set_min_exp(EXP_NONE, true);
			break;
		}
	}
	focus_keeping_selection();
}

void on_button_exact_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		set_approximation(APPROXIMATION_EXACT);
	} else {
		set_approximation(APPROXIMATION_TRY_EXACT);
	}
	focus_keeping_selection();
}

void on_button_fraction_toggled(GtkToggleButton *w, gpointer) {
	toggle_fraction_format(gtk_toggle_button_get_active(w));
	focus_keeping_selection();
}

void update_keypad_fraction() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_fraction_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), printops.number_fraction_format == FRACTION_FRACTIONAL || printops.number_fraction_format == FRACTION_COMBINED || printops.number_fraction_format == FRACTION_FRACTIONAL_FIXED_DENOMINATOR || printops.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_fraction_toggled, NULL);
}
void update_keypad_exact() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "button_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_exact_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), evalops.approximation == APPROXIMATION_EXACT);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "button_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_button_exact_toggled, NULL);
}
void update_keypad_numerical_display() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	switch(printops.min_exp) {
		case EXP_PRECISION: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0); break;}
		case EXP_BASE_3: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 1); break;}
		case EXP_PURE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 3); break;}
		case EXP_NONE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 4); break;}
		default: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 2); break;}
	}
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
}
void update_keypad_base() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	switch(printops.base) {
		case 2: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 0); break;}
		case 8: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 1); break;}
		case 10: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 2); break;}
		case 12: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 3); break;}
		case 16: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 4); break;}
		case BASE_SEXAGESIMAL: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 5); break;}
		case BASE_TIME: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 6); break;}
		case BASE_ROMAN_NUMERALS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 7); break;}
		default: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 8); break;}
	}
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
}

void create_base_string(string &str1, int b_almost_equal, bool b_small) {
	if(b_small) str1 = "<small>";
	else str1 = "";
	if(b_almost_equal == 0) {
		str1 += "=";
	} else if(b_almost_equal == 1) {
		str1 += SIGN_ALMOST_EQUAL;
	} else {
		str1 += "= ";
		str1 += _("approx.");
	}
	str1 += " ";
	if(printops.base != 16) {
		str1 += result_hex;
		if(printops.hexadecimal_twos_complement && (current_result()->isNegate() || current_result()->number().isNegative())) str1 += "<sub>16-</sub>";
		else str1 += "<sub>16</sub>";
	}
	if(printops.base != 10) {
		if(printops.base != 16) {
			if(b_almost_equal) str1 += " " SIGN_ALMOST_EQUAL " ";
			else str1 += " = ";
		}
		str1 += result_dec;
		str1 += "<sub>10</sub>";
	}
	if(printops.base != 8) {
		if(b_almost_equal) str1 += " " SIGN_ALMOST_EQUAL " ";
		else str1 += " = ";
		str1 += result_oct;
		str1 += "<sub>8</sub>";
	}
	if(printops.base != 2) {
		if(b_almost_equal) str1 += " " SIGN_ALMOST_EQUAL " ";
		else str1 += " = ";
		str1 += result_bin;
		if(printops.twos_complement && (current_result()->isNegate() || current_result()->number().isNegative())) str1 += "<sub>2-</sub>";
		else str1 += "<sub>2</sub>";
	}
	if(b_small) str1 += "</small>";
	FIX_SUPSUB_PRE_W(result_bases);
	FIX_SUPSUB(str1);
}

void set_result_bases(const MathStructure &m) {
	result_bin = ""; result_oct = "", result_dec = "", result_hex = ""; result_bases_approx = false;
	if(max_bases.isZero()) {max_bases = 2; max_bases ^= 64; min_bases = -max_bases;}
	if(!CALCULATOR->aborted() && ((m.isNumber() && m.number() < max_bases && m.number() > min_bases) || (m.isNegate() && m[0].isNumber() && m[0].number() < max_bases && m[0].number() > min_bases))) {
		result_bases_approx = !m.isInteger() && (!m.isNegate() || !m[0].isInteger());
		Number nr;
		if(m.isNumber()) {
			nr = m.number();
		} else {
			nr = m[0].number();
			nr.negate();
		}
		nr.round(printops.rounding);
		PrintOptions po = printops;
		po.is_approximate = NULL;
		po.show_ending_zeroes = false;
		po.base_display = BASE_DISPLAY_NORMAL;
		po.min_exp = 0;
		if(printops.base != 2) {
			po.base = 2;
			if(!po.twos_complement || !nr.isNegative()) {
				po.binary_bits = 0;
			} else {
				po.binary_bits = printops.binary_bits;
			}
			result_bin = nr.print(po);
		}
		if(printops.base != 8) {
			po.base = 8;
			result_oct = nr.print(po);
			size_t i = result_oct.find_first_of(NUMBERS);
			if(i != string::npos && result_oct.length() > i + 1 && result_oct[i] == '0' && is_in(NUMBERS, result_oct[i + 1])) result_oct.erase(i, 1);
		}
		if(printops.base != 10) {
			po.base = 10;
			result_dec = nr.print(po);
		}
		if(printops.base != 16) {
			po.base = 16;
			if(!po.hexadecimal_twos_complement || !nr.isNegative()) {
				po.binary_bits = 0;
			} else {
				po.binary_bits = printops.binary_bits;
			}
			result_hex = nr.print(po);
			gsub("0x", "", result_hex);
			size_t l = result_hex.length();
			size_t i_after_minus = 0;
			if(nr.isNegative()) {
				if(l > 1 && result_hex[0] == '-') i_after_minus = 1;
				else if(result_hex.find("−") == 0) i_after_minus = strlen("−");
			}
			for(int i = (int) l - 2; i > (int) i_after_minus; i -= 2) {
				result_hex.insert(i, 1, ' ');
			}
			if(result_hex.length() > i_after_minus + 1 && result_hex[i_after_minus + 1] == ' ') result_hex.insert(i_after_minus, 1, '0');
		}
	}
}

void clear_result_bases() {
	result_bin = ""; result_oct = ""; result_dec = ""; result_hex = ""; result_bases_approx = false;
	update_result_bases();
}

void update_result_bases() {
	if(!result_hex.empty() || !result_dec.empty() || !result_oct.empty() || !result_bin.empty()) {
		string str1, str2;
		int b_almost_equal = -1;
		if(!result_bases_approx) {
			b_almost_equal = 0;
		} else if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) result_bases)) {
			b_almost_equal = 1;
		}
		create_base_string(str1, b_almost_equal, false);
		bool use_str2 = false;
		if(two_result_bases_rows != 0) {
			PangoLayout *layout = gtk_widget_create_pango_layout(result_bases, "");
			pango_layout_set_markup(layout, str1.c_str(), -1);
			gint w = 0;
			pango_layout_get_pixel_size(layout, &w, NULL);
			g_object_unref(layout);
			if(w + 12 > gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_keypad_top")))) {
				size_t i;
				if(two_result_bases_rows == 2) {
					create_base_string(str2, b_almost_equal, true);
					if(b_almost_equal == 1) i = str2.rfind(" " SIGN_ALMOST_EQUAL " ");
					else i = str2.rfind(" = ");
					if(i != string::npos) str2[i] = '\n';
					use_str2 = true;
				} else {
					if(b_almost_equal == 1) i = str1.rfind(" " SIGN_ALMOST_EQUAL " ");
					else i = str1.rfind(" = ");
					if(i != string::npos) str1[i] = '\n';
				}
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
				gtk_label_set_yalign(GTK_LABEL(result_bases), 0.0);
#else
				gtk_misc_set_alignment(GTK_MISC(result_bases), 1.0, 0.0);
#endif
				if(two_result_bases_rows < 0) {
					layout = gtk_widget_create_pango_layout(result_bases, "");
					pango_layout_set_markup(layout, str1.c_str(), -1);
					gint h = 0;
					pango_layout_get_pixel_size(layout, NULL, &h);
					if(h + 3 > gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_keypad_top")))) {
						create_base_string(str2, b_almost_equal, true);
						size_t i2;
						if(b_almost_equal == 1) i2 = str2.rfind(" " SIGN_ALMOST_EQUAL " ");
						else i2 = str2.rfind(" = ");
						if(i2 != string::npos) str2[i2] = '\n';
						pango_layout_set_markup(layout, str2.c_str(), -1);
						pango_layout_get_pixel_size(layout, NULL, &h);
						if(h + 3 > gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_keypad_top")))) {
							two_result_bases_rows = 0;
							if(i != string::npos) str1[i] = ' ';
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
							gtk_label_set_yalign(GTK_LABEL(result_bases), 0.5);
#else
							gtk_misc_set_alignment(GTK_MISC(result_bases), 1.0, 0.5);
#endif
						} else {
							use_str2 = true;
							two_result_bases_rows = 2;
						}
					} else {
						two_result_bases_rows = 1;
					}
					g_object_unref(layout);
				}
			} else {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
				gtk_label_set_yalign(GTK_LABEL(result_bases), 0.5);
#else
				gtk_misc_set_alignment(GTK_MISC(result_bases), 1.0, 0.5);
#endif
			}
		}
		gtk_label_set_markup(GTK_LABEL(result_bases), use_str2 ? str2.c_str() : str1.c_str());
		if(b_almost_equal) gsub(" " SIGN_ALMOST_EQUAL " ", "\n" SIGN_ALMOST_EQUAL " ", str1);
		else gsub(" = ", "\n= ", str1);
		gtk_widget_set_tooltip_markup(result_bases, str1.c_str());
	} else {
		gtk_label_set_text(GTK_LABEL(result_bases), "");
		gtk_widget_set_tooltip_markup(result_bases, "");
	}
}
void keypad_algebraic_mode_changed() {
	if(evalops.structuring == STRUCTURING_SIMPLIFY) {
		gtk_widget_hide(item_factorize);
		gtk_widget_show(item_simplify);
		FIX_SUPSUB_PRE_W(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_factorize")));
		string s_axb = "a(x)<sup>b</sup>";
		FIX_SUPSUB(s_axb);
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_factorize")), s_axb.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), _("Factorize"));
		if(!enable_tooltips) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), FALSE);
	} else if(evalops.structuring == STRUCTURING_FACTORIZE) {
		gtk_widget_show(item_factorize);
		gtk_widget_hide(item_simplify);
		FIX_SUPSUB_PRE_W(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_factorize")));
		string s_axb = "x+x<sup>b</sup>";
		FIX_SUPSUB(s_axb);
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_factorize")), s_axb.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), _("Expand"));
		if(!enable_tooltips) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), FALSE);
	}
}
void update_keypad_font(bool initial) {
	if(use_custom_keypad_font) {
		gchar *gstr = font_name_to_css(custom_keypad_font.c_str());
		gtk_css_provider_load_from_data(keypad_provider, gstr, -1, NULL);
		g_free(gstr);
	} else if(initial) {
		if(custom_keypad_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(keypad_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_keypad_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	} else {
		gtk_css_provider_load_from_data(keypad_provider, "", -1, NULL);
	}
	if(!initial) keypad_font_modified();
}
void set_keypad_font(const char *str) {
	if(!str) {
		use_custom_keypad_font = false;
	} else {
		use_custom_keypad_font = true;
		if(custom_keypad_font != str) {
			save_custom_keypad_font = true;
			custom_keypad_font = str;
		}
	}
	update_keypad_font(false);
}
const char *keypad_font(bool return_default) {
	if(!return_default && !use_custom_keypad_font) return NULL;
	return custom_keypad_font.c_str();
}
void set_vertical_button_padding(int i) {
	vertical_button_padd = i;
	update_button_padding();
	keypad_font_modified();
}
void set_horizontal_button_padding(int i) {
	horizontal_button_padd = i;
	if(horizontal_button_padd > 4) horizontal_button_padd = (horizontal_button_padd - 4) * 2 + 4;
	update_button_padding();
	keypad_font_modified();
}
int vertical_button_padding() {return vertical_button_padd;}
int horizontal_button_padding() {return horizontal_button_padd;}
void update_keypad_button_text() {
	if(custom_buttons.empty()) initialize_custom_buttons();
	if(printops.use_unicode_signs) {
		if(custom_buttons[24].text.empty()) {
			if(can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_sub"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), SIGN_MINUS);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), MINUS);
		}
		if(custom_buttons[22].text.empty()) {
			if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_times"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), SIGN_MULTIPLICATION);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), MULTIPLICATION);
		}
		if(custom_buttons[21].text.empty()) {
			if(can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), SIGN_DIVISION_SLASH);
			else if(can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), SIGN_DIVISION);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), DIVISION);
		}
		if(can_display_unicode_string_function("➞", (void*) gtk_builder_get_object(main_builder, "button_fraction"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_to")), "x ➞");
		else gtk_label_set_label(GTK_LABEL(gtk_builder_get_object(main_builder, "label_to")), "to");
		if(can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "button_fraction"))) gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), "a " SIGN_DIVISION_SLASH " b");
		else gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), "a " DIVISION " b");
		if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_factorize"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_factorize2")), "a" SIGN_MULTIPLICATION "b");
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_factorize2")), "a" MULTIPLICATION "b");
		if(can_display_unicode_string_function(SIGN_SQRT, (void*) gtk_builder_get_object(main_builder, "label_sqrt"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt")), SIGN_SQRT);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt")), "sqrt");
		if(can_display_unicode_string_function(SIGN_SQRT, (void*) gtk_builder_get_object(main_builder, "label_sqrt2"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt2")), SIGN_SQRT);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt2")), "sqrt");
		if(can_display_unicode_string_function("x̄", (void*) gtk_builder_get_object(main_builder, "label_mean"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_mean")), "x̄");
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_mean")), "mean");
		if(can_display_unicode_string_function("∑", (void*) gtk_builder_get_object(main_builder, "label_sum"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sum")), "∑");
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sum")), "sum");
		if(can_display_unicode_string_function("π", (void*) gtk_builder_get_object(main_builder, "label_pi"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_pi")), "π");
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_pi")), "pi");
	} else {
		if(custom_buttons[24].text.empty()) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), MINUS);
		if(custom_buttons[22].text.empty()) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), MULTIPLICATION);
		if(custom_buttons[21].text.empty()) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), DIVISION);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt")), "sqrt");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt2")), "sqrt");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_mean")), "mean");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sum")), "sum");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_pi")), "pi");
		gtk_label_set_label(GTK_LABEL(gtk_builder_get_object(main_builder, "label_factorize2")), "a" MULTIPLICATION "b");
		gtk_label_set_label(GTK_LABEL(gtk_builder_get_object(main_builder, "label_to")), "to");
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), "a " DIVISION " b");
	}
	if(custom_buttons[18].text.empty()) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_dot")), CALCULATOR->getDecimalPoint().c_str());
	if(custom_buttons[4].text.empty()) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_comma")), CALCULATOR->getComma().c_str());

	FIX_SUPSUB_PRE_W(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_xy")));
	if(custom_buttons[20].text.empty()) {
		string s_xy = "x<sup>y</sup>";
		FIX_SUPSUB(s_xy);
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_xy")), s_xy.c_str());
	}
	string s_axb;
	if(evalops.structuring != STRUCTURING_FACTORIZE) {
		s_axb = "a(x)<sup>b</sup>";
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), _("Factorize"));
	} else {
		s_axb = "x+x<sup>b</sup>";
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), _("Expand"));
	}
	FIX_SUPSUB(s_axb);
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_factorize")), s_axb.c_str());
	if(enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_factorize")), FALSE);
	string s_recip;
	if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_reciprocal"))) s_recip = "x<sup>" SIGN_MINUS "1</sup>";
	else s_recip = "x<sup>-1</sup>";
	FIX_SUPSUB(s_recip);
	string s_log2 = "log<sub>2</sub>";
	FIX_SUPSUB(s_log2);
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_log2")), s_log2.c_str());
}
void update_keypad_state(bool initial_update) {
	if(initial_update) {
		switch(evalops.approximation) {
			case APPROXIMATION_EXACT: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), TRUE);
				break;
			}
			case APPROXIMATION_TRY_EXACT: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
				break;
			}
		}
		update_keypad_angle();
		switch(printops.min_exp) {
			case EXP_PRECISION: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0);
				break;
			}
			case EXP_BASE_3: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 1);
				break;
			}
			case EXP_SCIENTIFIC: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 2);
				break;
			}
			case EXP_PURE: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 3);
				break;
			}
			case EXP_NONE: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 4);
				break;
			}
		}
		switch(printops.base) {
			case BASE_BINARY: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 0);
				break;
			}
			case BASE_OCTAL: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 1);
				break;
			}
			case BASE_DECIMAL: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 2);
				break;
			}
			case 12: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 3);
				break;
			}
			case BASE_HEXADECIMAL: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 4);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 7);
				break;
			}
			case BASE_SEXAGESIMAL: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 5);
				break;
			}
			case BASE_TIME: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 6);
				break;
			}
			default: {
				gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 8);
			}
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), printops.number_fraction_format == FRACTION_FRACTIONAL || printops.number_fraction_format == FRACTION_COMBINED || printops.number_fraction_format == FRACTION_FRACTIONAL_FIXED_DENOMINATOR || printops.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR);
	}
	update_keypad_programming_base();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_programmers_keypad")), visible_keypad & PROGRAMMING_KEYPAD);
	if(visible_keypad & PROGRAMMING_KEYPAD) {
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_left_buttons")), GTK_WIDGET(gtk_builder_get_object(main_builder, "programmers_keypad")));
		gtk_stack_set_visible_child_name(GTK_STACK(gtk_builder_get_object(main_builder, "stack_keypad_top")), "page1");
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_left_buttons")), !(visible_keypad & HIDE_LEFT_KEYPAD));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_right_buttons")), !(visible_keypad & HIDE_LEFT_KEYPAD));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_right_buttons")), !(visible_keypad & HIDE_RIGHT_KEYPAD));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_left_buttons")), !(visible_keypad & HIDE_RIGHT_KEYPAD));
	if(!initial_update && ((visible_keypad & HIDE_LEFT_KEYPAD) || (visible_keypad & HIDE_RIGHT_KEYPAD))) {
		gint h;
		gtk_window_get_size(main_window(), NULL, &h);
		gtk_window_resize(main_window(), 1, h);
	}
}
void keypad_rpn_mode_changed() {
	if(rpn_mode) {
		gtk_label_set_angle(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), 90.0);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), _("ENTER"));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), _("Calculate expression and add to stack"));
	} else {
		gtk_label_set_angle(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), 0.0);
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), "<big>=</big>");
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), _("Calculate expression"));
	}
}
#define SET_TOOLTIP_ACCEL(w, t) gtk_widget_set_tooltip_text(w, t); if(type >= 0 && enable_tooltips != 1) {gtk_widget_set_has_tooltip(w, FALSE);}
void update_keypad_accels(int type) {
	bool b = false;
	for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
		if(it->second.type.size() != 1 || (type >= 0 && it->second.type[0] != type)) continue;
		b = true;
		switch(it->second.type[0]) {
			case SHORTCUT_TYPE_SMART_PARENTHESES: {
				if(custom_buttons[5].type[0] == -1) {
					gchar *gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_wrap")));
					if(gstr) {
						string str = gstr;
						g_free(gstr);
						string str2 = _("Smart parentheses");
						str2 += " (";
						str2 += shortcut_to_text(it->second.key, it->second.modifier);
						str2 += ")";
						size_t i = str.find("\n");
						if(i == string::npos) {
							str = str2;
						} else {
							str.erase(0, i);
							str.insert(0, str2);
						}
						SET_TOOLTIP_ACCEL(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_wrap")), str.c_str());
					}
				}
				break;
			}
			case SHORTCUT_TYPE_PROGRAMMING: {
				string str = _("Show/hide programming keypad");
				str += " (";
				str += shortcut_to_text(it->second.key, it->second.modifier);
				str += ")";
				SET_TOOLTIP_ACCEL(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_programmers_keypad")), str.c_str());
				break;
			}
		}
		if(type >= 0) break;
	}
	if(!b) {
		switch(type) {
			case SHORTCUT_TYPE_SMART_PARENTHESES: {
				if(custom_buttons[5].type[0] == -1) {
					gchar *gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_wrap")));
					if(gstr) {
						string str = gstr;
						g_free(gstr);
						size_t i = str.find("\n");
						if(i == string::npos) {
							str = _("Smart parentheses");
						} else {
							str.erase(0, i);
							str.insert(0, _("Smart parentheses"));
						}
						SET_TOOLTIP_ACCEL(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_wrap")), str.c_str());
					}
				}
				break;
			}
			case SHORTCUT_TYPE_PROGRAMMING: {SET_TOOLTIP_ACCEL(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_programmers_keypad")), _("Show/hide programming keypad")); break;}
		}
	}
}

void create_keypad() {

	keypad_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(keypad_widget()), GTK_STYLE_PROVIDER(keypad_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	update_keypad_font(true);
	update_keypad_button_text();

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14
	if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "pan-start-symbolic")) {
		GtkWidget *arrow_left = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
		gtk_widget_set_size_request(GTK_WIDGET(arrow_left), 18, 18);
		gtk_widget_show(arrow_left);
		gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(main_builder, "image_hide_left_buttons")));
		gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(main_builder, "event_hide_left_buttons")), arrow_left);
	}
	if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "pan-end-symbolic")) {
		GtkWidget *arrow_right = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_OUT);
		gtk_widget_set_size_request(GTK_WIDGET(arrow_right), 18, 18);
		gtk_widget_show(arrow_right);
		gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(main_builder, "image_hide_right_buttons")));
		gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(main_builder, "event_hide_right_buttons")), arrow_right);
	}
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 14)) gtk_grid_set_column_spacing(GTK_GRID(gtk_builder_get_object(main_builder, "grid_buttons")), 0);
#endif

#ifdef _WIN32
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_down_image")), 12);
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_up_image")), 12);
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_left_image")), 12);
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_right_image")), 12);
#else
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_down_image")), 14);
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_up_image")), 14);
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_left_image")), 14);
	gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "button_right_image")), 14);
#endif

	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_sin")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_sin")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_cos")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_cos")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_tan")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tan")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_sqrt")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_sqrt")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_e")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_e")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_xequals")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_xequals")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_ln")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ln")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_sum")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_sum")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_mean")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_mean")));
	gtk_menu_button_set_align_widget(GTK_MENU_BUTTON(gtk_builder_get_object(main_builder, "mb_pi")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_pi")));


	if(themestr.substr(0, 7) == "Adwaita" || themestr.substr(0, 5) == "oomox" || themestr.substr(0, 6) == "themix" || themestr == "Breeze" || themestr == "Breeze-Dark" || themestr.substr(0, 4) == "Yaru") {

		GtkCssProvider *link_style_top = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_top, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_bot = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bot, "* {border-top-left-radius: 0; border-top-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_tl = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_tl, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0; border-top-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_tr = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_tr, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0; border-top-left-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_bl = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bl, "* {border-top-left-radius: 0; border-top-right-radius: 0; border-bottom-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_br = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_br, "* {border-top-left-radius: 0; border-top-right-radius: 0; border-bottom-left-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_mid = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_mid, "* {border-radius: 0;}", -1, NULL);

		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_zero"))), GTK_STYLE_PROVIDER(link_style_bl), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_dot"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_exp"))), GTK_STYLE_PROVIDER(link_style_br), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_one"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_two"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_three"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_four"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_five"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_six"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_seven"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_eight"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_nine"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_open"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_close"))), GTK_STYLE_PROVIDER(link_style_tr), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_brace_wrap"))), GTK_STYLE_PROVIDER(link_style_tl), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_comma"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_move2"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_percent"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_plusminus"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_xy"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_divide"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_times"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_sub"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_add"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_ac"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_del"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_ans"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c1"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c2"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c3"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c4"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_c5"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}

	GList *l, *l2;
	GList *list, *list2;
	GObject *obj;
	CHILDREN_SET_FOCUS_ON_CLICK_2("table_buttons", "grid_numbers")
	CHILDREN_SET_FOCUS_ON_CLICK("box_custom_buttons1")
	CHILDREN_SET_FOCUS_ON_CLICK("box_custom_buttons2")
	CHILDREN_SET_FOCUS_ON_CLICK("box_custom_buttons3")
	CHILDREN_SET_FOCUS_ON_CLICK("box_custom_buttons4")
	CHILDREN_SET_FOCUS_ON_CLICK("grid_numbers")
	CHILDREN_SET_FOCUS_ON_CLICK("grid_programmers_buttons")
	CHILDREN_SET_FOCUS_ON_CLICK("box_bases")
	CHILDREN_SET_FOCUS_ON_CLICK("box_twos")
	list = gtk_container_get_children(GTK_CONTAINER(gtk_builder_get_object(main_builder, "versatile_keypad")));
	for(l = list; l != NULL; l = l->next) {
		list2 = gtk_container_get_children(GTK_CONTAINER(l->data));
		for(l2 = list2; l2 != NULL; l2 = l2->next) {
			SET_FOCUS_ON_CLICK(l2->data);
		}
		g_list_free(list2);
	}
	g_list_free(list);

	update_button_padding(true);
	update_keypad_state(true);
	keypad_rpn_mode_changed();
	result_bases = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_result_bases"));
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_label_set_xalign(GTK_LABEL(result_bases), 1.0);
	gtk_label_set_yalign(GTK_LABEL(result_bases), 0.5);
#else
	gtk_misc_set_alignment(GTK_MISC(result_bases), 1.0, 0.5);
#endif
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_start(result_bases, 6);
	gtk_widget_set_margin_end(result_bases, 6);
#else
	gtk_widget_set_margin_left(result_bases, 6);
	gtk_widget_set_margin_right(result_bases, 6);
#endif
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons1")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons2")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons3")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons4")), FALSE);
	for(size_t i = 29; i <= 33; i++) {
		if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
			gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons1")), TRUE);
			break;
		}
	}
	for(size_t i = 34; i <= 38; i++) {
		if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
			gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons2")), TRUE);
			break;
		}
	}
	for(size_t i = 39; i <= 43; i++) {
		if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
			gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons3")), TRUE);
			break;
		}
	}
	for(size_t i = 44; i <= 48; i++) {
		if(custom_buttons[i].type[0] >= 0 || custom_buttons[i].type[1] >= 0 || custom_buttons[i].type[2] >= 0) {
			gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons4")), TRUE);
			break;
		}
	}

	// Fixes missing support for context in ui file translations
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_units")), _c("Manage units button", "u"));
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_to")), _c("Button for convert to operator", "to"));

	gtk_builder_add_callback_symbols(main_builder, "on_button_programmers_keypad_toggled", G_CALLBACK(on_button_programmers_keypad_toggled), "on_button_exact_toggled", G_CALLBACK(on_button_exact_toggled), "on_button_fraction_toggled", G_CALLBACK(on_button_fraction_toggled), "on_button_xequals_clicked", G_CALLBACK(on_button_xequals_clicked), "on_button_x_clicked", G_CALLBACK(on_button_x_clicked), "on_button_y_clicked", G_CALLBACK(on_button_y_clicked), "on_button_z_clicked", G_CALLBACK(on_button_z_clicked), "on_button_fac_clicked", G_CALLBACK(on_button_fac_clicked), "on_button_new_function_clicked", G_CALLBACK(on_button_new_function_clicked), "on_button_store_clicked", G_CALLBACK(on_button_store_clicked), "on_button_bases_clicked", G_CALLBACK(on_button_bases_clicked), "on_button_i_clicked", G_CALLBACK(on_button_i_clicked), "on_button_units_clicked", G_CALLBACK(on_button_units_clicked), "on_button_to_clicked", G_CALLBACK(on_button_to_clicked), "on_button_euro_clicked", G_CALLBACK(on_button_euro_clicked), "on_button_si_clicked", G_CALLBACK(on_button_si_clicked), "on_button_factorize_clicked", G_CALLBACK(on_button_factorize_clicked), "on_button_bin_toggled", G_CALLBACK(on_button_bin_toggled), "on_button_oct_toggled", G_CALLBACK(on_button_oct_toggled), "on_button_dec_toggled", G_CALLBACK(on_button_dec_toggled), "on_button_hex_toggled", G_CALLBACK(on_button_hex_toggled), "on_button_twos_in_toggled", G_CALLBACK(on_button_twos_in_toggled), "on_button_twos_out_toggled", G_CALLBACK(on_button_twos_out_toggled), "on_button_a_clicked", G_CALLBACK(on_button_a_clicked), "on_button_b_clicked", G_CALLBACK(on_button_b_clicked), "on_button_c_clicked", G_CALLBACK(on_button_c_clicked), "on_button_d_clicked", G_CALLBACK(on_button_d_clicked), "on_button_e_clicked", G_CALLBACK(on_button_e_clicked), "on_button_f_clicked", G_CALLBACK(on_button_f_clicked), "on_button_reciprocal_clicked", G_CALLBACK(on_button_reciprocal_clicked), "on_button_idiv_clicked", G_CALLBACK(on_button_idiv_clicked), "on_button_factorize2_clicked", G_CALLBACK(on_button_factorize2_clicked), "on_button_fp_clicked", G_CALLBACK(on_button_fp_clicked), "on_button_c16_clicked", G_CALLBACK(on_button_c16_clicked), "on_button_c17_clicked", G_CALLBACK(on_button_c17_clicked), "on_button_c18_clicked", G_CALLBACK(on_button_c18_clicked), "on_button_c19_clicked", G_CALLBACK(on_button_c19_clicked), "on_button_c20_clicked", G_CALLBACK(on_button_c20_clicked), "on_button_c11_clicked", G_CALLBACK(on_button_c11_clicked), "on_button_c12_clicked", G_CALLBACK(on_button_c12_clicked), "on_button_c13_clicked", G_CALLBACK(on_button_c13_clicked), "on_button_c14_clicked", G_CALLBACK(on_button_c14_clicked), "on_button_c15_clicked", G_CALLBACK(on_button_c15_clicked), "on_button_c6_clicked", G_CALLBACK(on_button_c6_clicked), "on_button_c7_clicked", G_CALLBACK(on_button_c7_clicked), "on_button_c8_clicked", G_CALLBACK(on_button_c8_clicked), "on_button_c9_clicked", G_CALLBACK(on_button_c9_clicked), "on_button_c10_clicked", G_CALLBACK(on_button_c10_clicked), "on_button_c1_clicked", G_CALLBACK(on_button_c1_clicked), "on_button_c2_clicked", G_CALLBACK(on_button_c2_clicked), "on_button_c3_clicked", G_CALLBACK(on_button_c3_clicked), "on_button_c4_clicked", G_CALLBACK(on_button_c4_clicked), "on_button_c5_clicked", G_CALLBACK(on_button_c5_clicked), "on_button_execute_clicked", G_CALLBACK(on_button_execute_clicked), "on_button_del_button_event", G_CALLBACK(on_button_del_button_event), "on_button_del_clicked", G_CALLBACK(on_button_del_clicked), "on_button_ac_clicked", G_CALLBACK(on_button_ac_clicked), "on_button_move_button_event", G_CALLBACK(on_button_move_button_event), "on_button_move_clicked", G_CALLBACK(on_button_move_clicked), "on_button_ans_clicked", G_CALLBACK(on_button_ans_clicked), "on_button_move2_button_event", G_CALLBACK(on_button_move2_button_event), "on_button_move2_clicked", G_CALLBACK(on_button_move2_clicked), "on_button_dot_clicked", G_CALLBACK(on_button_dot_clicked), "on_button_zero_clicked", G_CALLBACK(on_button_zero_clicked), "on_button_one_clicked", G_CALLBACK(on_button_one_clicked), "on_button_two_clicked", G_CALLBACK(on_button_two_clicked), "on_button_three_clicked", G_CALLBACK(on_button_three_clicked), "on_button_six_clicked", G_CALLBACK(on_button_six_clicked), "on_button_five_clicked", G_CALLBACK(on_button_five_clicked), "on_button_four_clicked", G_CALLBACK(on_button_four_clicked), "on_button_eight_clicked", G_CALLBACK(on_button_eight_clicked), "on_button_nine_clicked", G_CALLBACK(on_button_nine_clicked), "on_button_exp_clicked", G_CALLBACK(on_button_exp_clicked), "on_button_brace_close_clicked", G_CALLBACK(on_button_brace_close_clicked), "on_button_brace_open_clicked", G_CALLBACK(on_button_brace_open_clicked), "on_button_brace_wrap_clicked", G_CALLBACK(on_button_brace_wrap_clicked), "on_button_seven_clicked", G_CALLBACK(on_button_seven_clicked), "on_button_add_clicked", G_CALLBACK(on_button_add_clicked), "on_button_sub_clicked", G_CALLBACK(on_button_sub_clicked), "on_button_times_clicked", G_CALLBACK(on_button_times_clicked), "on_button_divide_clicked", G_CALLBACK(on_button_divide_clicked), "on_button_xy_clicked", G_CALLBACK(on_button_xy_clicked), "on_button_plusminus_clicked", G_CALLBACK(on_button_plusminus_clicked), "on_button_percent_clicked", G_CALLBACK(on_button_percent_clicked), "on_button_comma_clicked", G_CALLBACK(on_button_comma_clicked), NULL);
	gtk_builder_add_callback_symbols(main_builder, "on_menu_item_x_default_activate", G_CALLBACK(on_menu_item_x_default_activate), "on_menu_item_x_none_activate", G_CALLBACK(on_menu_item_x_none_activate), "on_menu_item_x_nonmatrix_activate", G_CALLBACK(on_menu_item_x_nonmatrix_activate), "on_menu_item_x_number_activate", G_CALLBACK(on_menu_item_x_number_activate), "on_menu_item_x_complex_activate", G_CALLBACK(on_menu_item_x_complex_activate), "on_menu_item_x_real_activate", G_CALLBACK(on_menu_item_x_real_activate), "on_menu_item_x_rational_activate", G_CALLBACK(on_menu_item_x_rational_activate), "on_menu_item_x_integer_activate", G_CALLBACK(on_menu_item_x_integer_activate), "on_menu_item_x_boolean_activate", G_CALLBACK(on_menu_item_x_boolean_activate), "on_menu_item_x_unknown_activate", G_CALLBACK(on_menu_item_x_unknown_activate), "on_menu_item_x_nonzero_activate", G_CALLBACK(on_menu_item_x_nonzero_activate), "on_menu_item_x_positive_activate", G_CALLBACK(on_menu_item_x_positive_activate), "on_menu_item_x_nonnegative_activate", G_CALLBACK(on_menu_item_x_nonnegative_activate), "on_menu_item_x_negative_activate", G_CALLBACK(on_menu_item_x_negative_activate), "on_menu_item_x_nonpositive_activate", G_CALLBACK(on_menu_item_x_nonpositive_activate), "on_menu_item_y_default_activate", G_CALLBACK(on_menu_item_y_default_activate), "on_menu_item_y_none_activate", G_CALLBACK(on_menu_item_y_none_activate), "on_menu_item_y_nonmatrix_activate", G_CALLBACK(on_menu_item_y_nonmatrix_activate), "on_menu_item_y_number_activate", G_CALLBACK(on_menu_item_y_number_activate), "on_menu_item_y_complex_activate", G_CALLBACK(on_menu_item_y_complex_activate), "on_menu_item_y_real_activate", G_CALLBACK(on_menu_item_y_real_activate), "on_menu_item_y_rational_activate", G_CALLBACK(on_menu_item_y_rational_activate), "on_menu_item_y_integer_activate", G_CALLBACK(on_menu_item_y_integer_activate), "on_menu_item_y_boolean_activate", G_CALLBACK(on_menu_item_y_boolean_activate), "on_menu_item_y_unknown_activate", G_CALLBACK(on_menu_item_y_unknown_activate), "on_menu_item_y_nonzero_activate", G_CALLBACK(on_menu_item_y_nonzero_activate), "on_menu_item_y_positive_activate", G_CALLBACK(on_menu_item_y_positive_activate), "on_menu_item_y_nonnegative_activate", G_CALLBACK(on_menu_item_y_nonnegative_activate), "on_menu_item_y_negative_activate", G_CALLBACK(on_menu_item_y_negative_activate), "on_menu_item_y_nonpositive_activate", G_CALLBACK(on_menu_item_y_nonpositive_activate), "on_menu_item_z_default_activate", G_CALLBACK(on_menu_item_z_default_activate), "on_menu_item_z_none_activate", G_CALLBACK(on_menu_item_z_none_activate), "on_menu_item_z_nonmatrix_activate", G_CALLBACK(on_menu_item_z_nonmatrix_activate), "on_menu_item_z_number_activate", G_CALLBACK(on_menu_item_z_number_activate), "on_menu_item_z_complex_activate", G_CALLBACK(on_menu_item_z_complex_activate), "on_menu_item_z_real_activate", G_CALLBACK(on_menu_item_z_real_activate), "on_menu_item_z_rational_activate", G_CALLBACK(on_menu_item_z_rational_activate), "on_menu_item_z_integer_activate", G_CALLBACK(on_menu_item_z_integer_activate), "on_menu_item_z_boolean_activate", G_CALLBACK(on_menu_item_z_boolean_activate), "on_menu_item_z_unknown_activate", G_CALLBACK(on_menu_item_z_unknown_activate), "on_menu_item_z_nonzero_activate", G_CALLBACK(on_menu_item_z_nonzero_activate), "on_menu_item_z_positive_activate", G_CALLBACK(on_menu_item_z_positive_activate), "on_menu_item_z_nonnegative_activate", G_CALLBACK(on_menu_item_z_nonnegative_activate), "on_menu_item_z_negative_activate", G_CALLBACK(on_menu_item_z_negative_activate), "on_menu_item_z_nonpositive_activate", G_CALLBACK(on_menu_item_z_nonpositive_activate), NULL);
	gtk_builder_add_callback_symbols(main_builder, "on_menu_item_mb_degrees_activate", G_CALLBACK(on_menu_item_mb_degrees_activate), "on_menu_item_mb_radians_activate", G_CALLBACK(on_menu_item_mb_radians_activate), "on_menu_item_mb_gradians_activate", G_CALLBACK(on_menu_item_mb_gradians_activate), "hide_tooltip", G_CALLBACK(hide_tooltip), "on_combobox_numerical_display_changed", G_CALLBACK(on_combobox_numerical_display_changed), "on_combobox_base_changed", G_CALLBACK(on_combobox_base_changed), "on_keypad_menu_button_button_event", G_CALLBACK(on_keypad_menu_button_button_event), "on_mb_x_toggled", G_CALLBACK(on_mb_x_toggled), "on_mb_y_toggled", G_CALLBACK(on_mb_y_toggled), "on_mb_z_toggled", G_CALLBACK(on_mb_z_toggled), "on_mb_to_button_release_event", G_CALLBACK(on_mb_to_button_release_event), "on_mb_to_button_press_event", G_CALLBACK(on_mb_to_button_press_event), "on_keypad_button_button_event", G_CALLBACK(on_keypad_button_button_event), "on_combobox_bits_changed", G_CALLBACK(on_combobox_bits_changed), "insert_bitwise_and", G_CALLBACK(insert_bitwise_and), "insert_bitwise_or", G_CALLBACK(insert_bitwise_or), "insert_bitwise_xor", G_CALLBACK(insert_bitwise_xor), "insert_bitwise_not", G_CALLBACK(insert_bitwise_not), "insert_left_shift", G_CALLBACK(insert_left_shift), "insert_right_shift", G_CALLBACK(insert_right_shift), "on_hide_left_buttons_button_release_event", G_CALLBACK(on_hide_left_buttons_button_release_event), "on_hide_right_buttons_button_release_event", G_CALLBACK(on_hide_right_buttons_button_release_event), NULL);

}
