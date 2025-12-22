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
#include "modes.h"
#include "mainwindow.h"
#include "historyview.h"
#include "stackview.h"
#include "keypad.h"
#include "preferencesdialog.h"
#include "insertfunctiondialog.h"
#include "expressionstatus.h"
#include "expressioncompletion.h"
#include "expressionedit.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::deque;

extern GtkBuilder *main_builder;

GtkWidget *expressiontext;
GtkTextBuffer *expressionbuffer;
GtkTextTag *expression_par_tag, *expression_par_u_tag;
int expression_lines = -1;
size_t undo_index = 0;
int block_add_to_undo = 0;
int block_emodified = 0;
vector<string> expression_history;
string current_history_expression;
int expression_history_index = -1;
bool dont_change_index = false;
int block_add_to_expression_history = 0;
bool expression_has_changed = false, expression_has_changed_pos = false;
extern bool current_object_has_changed;
string previous_expression;
bool cursor_has_moved = false;
extern bool minimal_mode;
bool use_custom_expression_font = false, save_custom_expression_font = false;
string custom_expression_font;
int replace_expression = KEEP_EXPRESSION;
bool highlight_background = false;

string sdot, saltdot, sdiv, sslash, stimes, sminus;

GtkCssProvider *expression_provider;

deque<string> expression_undo_buffer;

GtkMenu *popup_menu_expressiontext;
vector<GtkWidget*> popup_expression_mode_items;

GtkWidget *expression_edit_widget() {
	if(!expressiontext) expressiontext = GTK_WIDGET(gtk_builder_get_object(main_builder, "expressiontext"));
	return expressiontext;
}

GtkTextBuffer *expression_edit_buffer() {
	if(!expressionbuffer) expressionbuffer = GTK_TEXT_BUFFER(gtk_builder_get_object(main_builder, "expressionbuffer"));
	return expressionbuffer;
}

bool read_expression_edit_settings_line(string &svar, string &svalue, int &v) {
	if(svar == "expression_lines") {
		expression_lines = v;
	} else if(svar == "use_custom_expression_font") {
		use_custom_expression_font = v;
	} else if(svar == "custom_expression_font") {
		custom_expression_font = svalue;
		save_custom_expression_font = true;
	} else if(svar == "replace_expression") {
		replace_expression = v;
	} else if(svar == "highlight_parenthesis_background") {
		highlight_background = v;
	} else if(!read_expression_completion_settings_line(svar, svalue, v)) {
		return false;
	}
	return true;
}
void write_expression_edit_settings(FILE *file) {
	if(expression_lines > 0) fprintf(file, "expression_lines=%i\n", expression_lines);
	fprintf(file, "use_custom_expression_font=%i\n", use_custom_expression_font);
	if(use_custom_expression_font || save_custom_expression_font) fprintf(file, "custom_expression_font=%s\n", custom_expression_font.c_str());
	fprintf(file, "replace_expression=%i\n", replace_expression);
	if(highlight_background) fprintf(file, "highlight_parenthesis_background=%i\n", highlight_background);
	write_expression_completion_settings(file);
}
bool read_expression_history_line(string &svar, string &svalue) {
	if(svar == "expression_history") {
		expression_history.push_back(svalue);
	} else {
		return false;
	}
	return true;
}
void write_expression_history(FILE *file) {
	if(clear_history_on_exit) return;
	for(size_t i = 0; i < expression_history.size(); i++) {
		gsub("\n", " ", expression_history[i]);
		fprintf(file, "expression_history=%s\n", expression_history[i].c_str());
	}
}

void start_expression_spinner() {
	gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
}
void stop_expression_spinner() {
	gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
}

GtkTextIter selstore_start, selstore_end;
void store_expression_selection() {
	gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &selstore_start, &selstore_end);
}
void restore_expression_selection() {
	if(!gtk_text_iter_equal(&selstore_start, &selstore_end)) {
		block_undo();
		gtk_text_buffer_select_range(expression_edit_buffer(), &selstore_start, &selstore_end);
		unblock_undo();
	}
}

int wrap_expression_selection(const char *insert_before, bool return_true_if_whole_selected) {
	if(!gtk_text_buffer_get_has_selection(expression_edit_buffer())) return false;
	GtkTextMark *mstart = gtk_text_buffer_get_selection_bound(expression_edit_buffer());
	if(!mstart) return false;
	GtkTextMark *mend = gtk_text_buffer_get_insert(expression_edit_buffer());
	if(!mend) return false;
	GtkTextIter istart, iend;
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &istart, mstart);
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iend, mend);
	gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
	string str = gstr;
	g_free(gstr);
	if(!insert_before && ((gtk_text_iter_is_start(&iend) && gtk_text_iter_is_end(&istart)) || (gtk_text_iter_is_start(&istart) && gtk_text_iter_is_end(&iend)))) {
		if(str.find_first_not_of(NUMBER_ELEMENTS SPACE) == string::npos) {
			if(gtk_text_iter_is_end(&istart)) gtk_text_buffer_place_cursor(expression_edit_buffer(), &istart);
			else gtk_text_buffer_place_cursor(expression_edit_buffer(), &iend);
			return true;
		} else if((str.length() > 1 && str[0] == '/' && str.find_first_not_of(NUMBER_ELEMENTS SPACES, 1) != string::npos) || CALCULATOR->hasToExpression(str, true, evalops) || CALCULATOR->hasWhereExpression(str, evalops)) {
			return -1;
		}
	}
	CALCULATOR->parseSigns(str);
	if(!str.empty() && is_in(OPERATORS, str[str.length() - 1]) && str[str.length() - 1] != '!') {
		if(gtk_text_iter_is_start(&iend) || gtk_text_iter_is_start(&istart)) return -1;
		return false;
	}
	bool b_ret = (!return_true_if_whole_selected || (gtk_text_iter_is_start(&istart) && gtk_text_iter_is_end(&iend)) || (gtk_text_iter_is_start(&iend) && gtk_text_iter_is_end(&istart)));
	if(gtk_text_iter_compare(&istart, &iend) > 0) {
		block_undo();
		if(auto_calculate && !rpn_mode) block_result();
		if(insert_before) gtk_text_buffer_insert(expression_edit_buffer(), &iend, insert_before, -1);
		gtk_text_buffer_insert(expression_edit_buffer(), &iend, "(", -1);
		if(auto_calculate && !rpn_mode) unblock_result();
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &istart, mstart);
		unblock_undo();
		gtk_text_buffer_insert(expression_edit_buffer(), &istart, ")", -1);
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &istart);
	} else {
		block_undo();
		if(auto_calculate && !rpn_mode) block_result();
		if(insert_before) gtk_text_buffer_insert(expression_edit_buffer(), &istart, insert_before, -1);
		gtk_text_buffer_insert(expression_edit_buffer(), &istart, "(", -1);
		if(auto_calculate && !rpn_mode) unblock_result();
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iend, mend);
		unblock_undo();
		gtk_text_buffer_insert(expression_edit_buffer(), &iend, ")", -1);
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &iend);
	}
	return b_ret;
}
string get_expression_text() {
	GtkTextIter istart, iend;
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
	gchar *gtext = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
	string text = gtext;
	g_free(gtext);
	return text;
}
bool expression_modified() {
	return expression_has_changed;
}
void block_undo() {
	block_add_to_undo++;
}
void unblock_undo() {
	block_add_to_undo--;
}
bool undo_blocked() {
	return block_add_to_undo;
}
void block_expression_history() {
	block_add_to_expression_history++;
}
void unblock_expression_history() {
	block_add_to_expression_history--;
}
bool expression_history_blocked() {
	return block_add_to_expression_history;
}
string get_selected_expression_text(bool return_all_if_no_sel) {
	if(!gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		if(return_all_if_no_sel) {
			string str = get_expression_text();
			remove_blank_ends(str);
			return str;
		}
		return "";
	}
	GtkTextIter istart, iend;
	gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &istart, &iend);
	gchar *gtext = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
	string text = gtext;
	g_free(gtext);
	return text;
}
void add_expression_to_undo() {
	if(expression_undo_buffer.size() > 100) expression_undo_buffer.pop_front();
	else undo_index++;
	while(undo_index < expression_undo_buffer.size()) {
		expression_undo_buffer.pop_back();
	}
	expression_undo_buffer.push_back(get_expression_text());
}

void overwrite_expression_selection(const gchar *text) {
	block_completion();
	block_undo();
	if(gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget())) && !gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		GtkTextIter ipos;
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, gtk_text_buffer_get_insert(expression_edit_buffer()));
		if(!gtk_text_iter_is_end(&ipos)) {
			GtkTextIter ipos2 = ipos;
			gtk_text_iter_forward_char(&ipos2);
			gtk_text_buffer_delete(expression_edit_buffer(), &ipos, &ipos2);
		}
	} else {
		gtk_text_buffer_delete_selection(expression_edit_buffer(), FALSE, TRUE);
	}
	unblock_undo();
	if(text) gtk_text_buffer_insert_at_cursor(expression_edit_buffer(), text, -1);
	unblock_completion();
}
void set_expression_text(const gchar *text) {
	block_undo();
	gtk_text_buffer_set_text(expression_edit_buffer(), text, -1);
	unblock_undo();
	if(!block_add_to_undo) add_expression_to_undo();
}
void insert_text(const gchar *name) {
	if(calculator_busy()) return;
	block_completion();
	overwrite_expression_selection(name);
	focus_expression();
	unblock_completion();
}
void clear_expression_text() {
	gtk_text_buffer_set_text(expression_edit_buffer(), "", -1);
}
bool expression_is_empty() {
	GtkTextIter istart;
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	return gtk_text_iter_is_end(&istart);
}
bool is_at_beginning_of_expression(bool allow_selection) {
	if(!allow_selection && gtk_text_buffer_get_has_selection(expression_edit_buffer())) return false;
	GtkTextIter ipos;
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, gtk_text_buffer_get_insert(expression_edit_buffer()));
	return gtk_text_iter_is_start(&ipos);
}

void add_to_expression_history(string str) {
	if(expression_history_blocked()) return;
	for(size_t i = 0; i < expression_history.size(); i++) {
		if(expression_history[i] == str) {
			expression_history.erase(expression_history.begin() + i);
			break;
		}
	}
	if(expression_history.size() >= 100) {
		expression_history.pop_back();
	}
	expression_history.insert(expression_history.begin(), str);
	expression_history_index = 0;
}
bool expression_history_down() {
	if(expression_history_index == -1) current_history_expression = get_expression_text();
	if(expression_history_index >= -1) expression_history_index--;
	dont_change_index = true;
	block_completion();
	if(expression_history_index < 0) {
		if(expression_history_index == -1 && current_history_expression != get_expression_text()) set_expression_text(current_history_expression.c_str());
		else clear_expression_text();
	} else {
		set_expression_text(expression_history[expression_history_index].c_str());
	}
	unblock_completion();
	dont_change_index = false;
	return true;
}
bool expression_history_up() {
	if(expression_history_index + 1 < (int) expression_history.size()) {
		if(expression_history_index == -1) current_history_expression = get_expression_text();
		expression_history_index++;
		dont_change_index = true;
		block_completion();
		if(expression_history_index == -1 && current_history_expression == get_expression_text()) expression_history_index = 0;
		if(expression_history_index == -1) set_expression_text(current_history_expression.c_str());
		else if(expression_history.empty()) expression_history_index = -1;
		else set_expression_text(expression_history[expression_history_index].c_str());
		unblock_completion();
		dont_change_index = false;
		return true;
	}
	return false;
}

void set_expression_operator_symbols() {
	if(can_display_unicode_string_function_exact(SIGN_MINUS, (void*) expression_edit_widget())) sminus = SIGN_MINUS;
	else sminus = "-";
	if(can_display_unicode_string_function(SIGN_DIVISION, (void*) expression_edit_widget())) sdiv = SIGN_DIVISION;
	else sdiv = "/";
	sslash = "/";
	if(can_display_unicode_string_function(SIGN_MULTIDOT, (void*) expression_edit_widget())) sdot = SIGN_MULTIDOT;
	else sdot = "*";
	if(can_display_unicode_string_function(SIGN_MIDDLEDOT, (void*) expression_edit_widget())) saltdot = SIGN_MIDDLEDOT;
	else saltdot = "*";
	if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) expression_edit_widget())) stimes = SIGN_MULTIPLICATION;
	else stimes = "*";
}

const char *expression_add_sign() {
	return "+";
}
const char *expression_sub_sign() {
	if(!printops.use_unicode_signs) return "-";
	return sminus.c_str();
}
const char *expression_times_sign() {
	if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_DOT) return sdot.c_str();
	else if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) return saltdot.c_str();
	else if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_X) return stimes.c_str();
	return "*";
}
const char *expression_divide_sign() {
	if(!printops.use_unicode_signs) return "/";
	if(printops.division_sign == DIVISION_SIGN_DIVISION) return sdiv.c_str();
	return sslash.c_str();
}
void set_previous_expression(const string &str) {
	previous_expression = str;
}
const string &get_previous_expression() {
	return previous_expression;
}
void restore_previous_expression() {
	block_expression_icon_update();
	if(rpn_mode) {
		clear_expression_text();
	} else {
		rpn_mode = true;
		gtk_text_buffer_set_text(expression_edit_buffer(), previous_expression.c_str(), -1);
		rpn_mode = false;
		focus_expression();
		expression_select_all();
	}
	cursor_has_moved = false;
	expression_has_changed = false;
	unblock_expression_icon_update();
	if(gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) != GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon"))) {
		if(rpn_mode) update_expression_icons();
		else update_expression_icons(EXPRESSION_CLEAR);
	}
}

void focus_keeping_selection() {
	if(gtk_widget_is_focus(expression_edit_widget())) return;
	gtk_widget_grab_focus(expression_edit_widget());
}
void focus_expression() {
	if(expression_edit_widget() && !gtk_widget_is_focus(expression_edit_widget())) gtk_widget_grab_focus(expression_edit_widget());
}
void expression_select_all() {
	GtkTextIter istart, iend;
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
	gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iend);
	gtk_text_buffer_remove_tag(expression_edit_buffer(), expression_par_tag, &istart, &iend);
	gtk_text_buffer_remove_tag(expression_edit_buffer(), expression_par_u_tag, &istart, &iend);
}
void brace_wrap() {
	string expr = get_expression_text();
	GtkTextIter istart, iend, ipos;
	gint il = expr.length();
	if(il == 0) {
		set_expression_text("()");
		gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
		gtk_text_iter_forward_char(&istart);
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &istart);
		return;
	}
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expression_edit_buffer());
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mpos);
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	iend = istart;
	bool goto_start = false;
	if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		GtkTextMark *mstart = gtk_text_buffer_get_selection_bound(expression_edit_buffer());
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &istart, mstart);
		if(gtk_text_iter_compare(&istart, &ipos) > 0) {
			iend = istart;
			istart = ipos;
		} else {
			iend = ipos;
		}
	} else {
		iend = ipos;
		if(!gtk_text_iter_is_start(&iend)) {
			gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
			string str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
			g_free(gstr);
			CALCULATOR->parseSigns(str);
			if(str.empty() || is_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1])) {
				istart = iend;
				gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
				if(gtk_text_iter_compare(&istart, &iend) < 0) {
					gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
					str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
					g_free(gstr);
					CALCULATOR->parseSigns(str);
					if(str.empty() || (is_in(OPERATORS SPACES SEXADOT DOT RIGHT_VECTOR_WRAP LEFT_PARENTHESIS RIGHT_PARENTHESIS COMMAS, str[0]) && str[0] != MINUS_CH)) {
						iend = istart;
					}
				}
			}
		} else {
			goto_start = true;
			gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
			gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
			string str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
			g_free(gstr);
			CALCULATOR->parseSigns(str);
			if(str.empty() || (is_in(OPERATORS SPACES SEXADOT DOT RIGHT_VECTOR_WRAP LEFT_PARENTHESIS RIGHT_PARENTHESIS COMMAS, str[0]) && str[0] != MINUS_CH)) {
				iend = istart;
			}
		}
	}
	if(gtk_text_iter_compare(&istart, &iend) >= 0) {
		gtk_text_buffer_insert(expression_edit_buffer(), &istart, "()", -1);
		gtk_text_iter_backward_char(&istart);
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &istart);
		return;
	}
	gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
	GtkTextMark *mstart = gtk_text_buffer_create_mark(expression_edit_buffer(), "istart", &istart, TRUE);
	GtkTextMark *mend = gtk_text_buffer_create_mark(expression_edit_buffer(), "iend", &iend, FALSE);
	block_undo();
	gtk_text_buffer_insert(expression_edit_buffer(), &istart, "(", -1);
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iend, mend);
	unblock_undo();
	gtk_text_buffer_insert(expression_edit_buffer(), &iend, ")", -1);
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &istart, mstart);
	gtk_text_buffer_delete_mark(expression_edit_buffer(), mstart);
	gtk_text_buffer_delete_mark(expression_edit_buffer(), mend);
	string str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
	g_free(gstr);
	CALCULATOR->parseSigns(str);
	if(str.empty() || is_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1])) {
		gtk_text_iter_backward_char(&iend);
		goto_start = false;
	}
	gtk_text_buffer_place_cursor(expression_edit_buffer(), goto_start ? &istart : &iend);
}

void highlight_parentheses() {
	GtkTextMark *mcur = gtk_text_buffer_get_insert(expression_edit_buffer());
	if(!mcur) return;
	GtkTextIter icur, istart, iend;
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &icur, mcur);
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
	bool at_end = gtk_text_iter_equal(&icur, &iend);
	gtk_text_buffer_remove_tag(expression_edit_buffer(), expression_par_tag, &istart, &iend);
	gtk_text_buffer_remove_tag(expression_edit_buffer(), expression_par_u_tag, &istart, &iend);
	bool b = false;
	b = (gtk_text_iter_get_char(&icur) == ')');
	if(!b && gtk_text_iter_backward_char(&icur)) {
		b = (gtk_text_iter_get_char(&icur) == ')');
		if(!b) gtk_text_iter_forward_char(&icur);
	}
	if(b) {
		GtkTextIter ipar2 = icur;
		int pars = 1;
		while(gtk_text_iter_backward_char(&ipar2)) {
			if(gtk_text_iter_get_char(&ipar2) == ')') {
				pars++;
			} else if(gtk_text_iter_get_char(&ipar2) == '(') {
				pars--;
				if(pars == 0) break;
			}
		}
		if(pars == 0) {
			GtkTextIter inext = icur;
			gtk_text_iter_forward_char(&inext);
			gtk_text_buffer_apply_tag(expression_edit_buffer(), expression_par_tag, &icur, &inext);
			inext = ipar2;
			gtk_text_iter_forward_char(&inext);
			gtk_text_buffer_apply_tag(expression_edit_buffer(), expression_par_tag, &ipar2, &inext);
		} else {
			GtkTextIter inext = icur;
			gtk_text_iter_forward_char(&inext);
			gtk_text_buffer_apply_tag(expression_edit_buffer(), expression_par_u_tag, &icur, &inext);
		}
	} else {
		b = (gtk_text_iter_get_char(&icur) == '(');
		if(!b && gtk_text_iter_backward_char(&icur)) {
			b = (gtk_text_iter_get_char(&icur) == '(');
			if(!b) gtk_text_iter_forward_char(&icur);
		}
		if(b) {
			GtkTextIter ipar2 = icur;
			int pars = 1;
			while(gtk_text_iter_forward_char(&ipar2)) {
				if(gtk_text_iter_get_char(&ipar2) == '(') {
					pars++;
				} else if(gtk_text_iter_get_char(&ipar2) == ')') {
					pars--;
					if(pars == 0) break;
				}
			}
			if(pars == 0) {
				GtkTextIter inext = icur;
				gtk_text_iter_forward_char(&inext);
				gtk_text_buffer_apply_tag(expression_edit_buffer(), expression_par_tag, &icur, &inext);
				inext = ipar2;
				gtk_text_iter_forward_char(&inext);
				gtk_text_buffer_apply_tag(expression_edit_buffer(), expression_par_tag, &ipar2, &inext);
			} else if(!at_end) {
				GtkTextIter inext = icur;
				gtk_text_iter_forward_char(&inext);
				gtk_text_buffer_apply_tag(expression_edit_buffer(), expression_par_u_tag, &icur, &inext);
			}
		}
	}
}

guint autocalc_selection_timeout_id = 0;
gboolean do_autocalc_selection_timeout(gpointer) {
	autocalc_selection_timeout_id = 0;
	string expression = get_selected_expression_text();
	if(expression.length() > 2000 || expression.find_first_not_of(NUMBER_ELEMENTS COMMA SPACES) == string::npos || CALCULATOR->hasToExpression(expression, false, evalops) || contains_plot_or_save(CALCULATOR->unlocalizeExpression(expression, evalops.parse_options))) return FALSE;
	string str;
	int had_errors = false, had_warnings = false;
	CALCULATOR->beginTemporaryStopMessages();
	string result = CALCULATOR->calculateAndPrint(CALCULATOR->unlocalizeExpression(expression, evalops.parse_options), 100, evalops, printops, AUTOMATIC_FRACTION_OFF, AUTOMATIC_APPROXIMATION_OFF, &str, -1, NULL, true);
	had_errors = CALCULATOR->endTemporaryStopMessages(NULL, &had_warnings);
	if(had_errors || result.empty() || result == str || result.find(CALCULATOR->abortedMessage()) != string::npos || result.find(CALCULATOR->timedOutString()) != string::npos) {
		if(str.empty()) {
			CALCULATOR->endTemporaryStopMessages();
			return FALSE;
		}
	} else {
		if(*printops.is_approximate) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) parse_status_widget())) {
				str += " " SIGN_ALMOST_EQUAL " ";
			} else {
				str += "= ";
				str += _("approx.");
				str += " ";
			}
		} else {
			str += " = ";
		}
		str += result;
	}
	set_status_selection_text(str, had_errors, had_warnings);
	return FALSE;
}

extern bool tabbed_completion;
void on_expressionbuffer_cursor_position_notify() {
	if(autocalc_selection_timeout_id != 0) {
		g_source_remove(autocalc_selection_timeout_id);
		autocalc_selection_timeout_id = 0;
	}
	clear_status_selection_text();
	selstore_end = selstore_start;
	tabbed_completion = false;
	cursor_has_moved = true;
	if(expression_has_changed_pos) {
		expression_has_changed_pos = false;
		return;
	}
	hide_completion();
	highlight_parentheses();
	display_parse_status();
	mainwindow_cursor_moved();
	GtkTextIter istart, iend;
	if(auto_calculate && !rpn_mode && !status_blocked() && display_expression_status && gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &istart, &iend) && (!gtk_text_iter_is_start(&istart) || !gtk_text_iter_is_end(&iend))) {
		autocalc_selection_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 700, do_autocalc_selection_timeout, NULL, NULL);
	}
}

extern bool tabbed_completion;
void block_expression_modified() {
	block_emodified++;
}
void unblock_expression_modified() {
	block_emodified--;
}
void set_expression_modified(bool b, bool handle, bool autocalc) {
	if(!b || !handle) {
		expression_has_changed = b;
		return;
	}
	tabbed_completion = false;
	stop_completion_timeout();
	if(!undo_blocked()) add_expression_to_undo();
	if(!expression_has_changed || (rpn_mode && gtk_text_buffer_get_char_count(expression_edit_buffer()) == 1)) {
		expression_has_changed = true;
		update_expression_icons();
	}
	set_expression_output_updated(true);
	current_object_has_changed = true;
	expression_has_changed_pos = true;
	highlight_parentheses();
	showhide_expression_button();
	if(!dont_change_index) expression_history_index = -1;
	handle_expression_modified(autocalc);
	add_completion_timeout();
}

void on_expressionbuffer_changed(GtkTextBuffer *o, gpointer) {
	if(block_emodified) return;
	selstore_end = selstore_start;
	set_expression_modified(true, true, o != NULL);
}

void on_expressionbuffer_paste_done(GtkTextBuffer*, GtkClipboard *cb, gpointer) {
	if(!printops.use_unicode_signs || expression_undo_buffer.size() < 2 || gtk_text_buffer_get_has_selection(expression_edit_buffer())) return;
	gchar *cb_gtext = gtk_clipboard_wait_for_text(cb);
	if(!cb_gtext) return;
	string cb_text = cb_gtext;
	if(cb_text.empty() || cb_text.length() == 1) {
		g_free(cb_gtext);
		return;
	}
	GtkTextIter ipos;
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, gtk_text_buffer_get_insert(expression_edit_buffer()));
	gint index = gtk_text_iter_get_offset(&ipos);
	string text = expression_undo_buffer[expression_undo_buffer.size() - 2];
	bool in_cit1 = false, in_cit2 = false;
	gint iu = unicode_length(cb_text);
	for(size_t i = 0; iu < index && i < text.length(); i++) {
		if(!in_cit2 && text[i] == '\"') {
			in_cit1 = !in_cit1;
		} else if(!in_cit1 && text[i] == '\'') {
			in_cit2 = !in_cit2;
		}
		if((signed char) text[i] > 0 || (unsigned char) text[i] > 0xC0) iu++;
	}
	for(size_t i = 0; i < cb_text.length(); i++) {
		if(!in_cit2 && cb_text[i] == '\"') {
			in_cit1 = !in_cit1;
		} else if(!in_cit1 && cb_text[i] == '\'') {
			in_cit2 = !in_cit2;
		} else if(!in_cit1 && !in_cit2) {
			if(cb_text[i] == '*') cb_text.replace(i, 1, expression_times_sign());
			else if(cb_text[i] == '/') cb_text.replace(i, 1, expression_divide_sign());
			else if(cb_text[i] == '-') cb_text.replace(i, 1, expression_sub_sign());
		}
	}
	if(cb_text == cb_gtext) {
		g_free(cb_gtext);
		return;
	}
	gint pos = unicode_length(cb_gtext);
	bool b = false;
	string test_text = text;
	for(size_t i = 0; i < text.length(); i++) {
		if(pos == index) {
			test_text.insert(i, cb_gtext);
			text.insert(i, cb_text);
			b = true;
			break;
		}
		while(i + 1 < text.length() && text[i + 1] < 0 && (signed char) text[i + 1] < 0 && (unsigned char) text[i + 1] < 0xC0) {
			i++;
		}
		pos++;
	}
	if(!b) {
		text += cb_text;
		test_text += cb_gtext;
	}
	if(test_text != get_expression_text()) {
		g_free(cb_gtext);
		return;
	}
	block_undo();
	gtk_text_buffer_set_text(expression_edit_buffer(), text.c_str(), -1);
	gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &ipos, index);
	gtk_text_buffer_place_cursor(expression_edit_buffer(), &ipos);
	unblock_undo();
	expression_undo_buffer.pop_back();
	expression_undo_buffer.push_back(text);
	g_free(cb_gtext);
}

bool expression_in_quotes() {
	GtkTextIter ipos, istart;
	if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &ipos, &istart);
	} else {
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, gtk_text_buffer_get_insert(expression_edit_buffer()));
	}
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gchar *gtext = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &ipos, FALSE);
	bool in_cit1 = false, in_cit2 = false;
	for(size_t i = 0; i < strlen(gtext); i++) {
		if(!in_cit2 && gtext[i] == '\"') {
			in_cit1 = !in_cit1;
		} else if(!in_cit1 && gtext[i] == '\'') {
			in_cit2 = !in_cit2;
		}
	}
	g_free(gtext);
	return in_cit1 || in_cit2;
}

gboolean on_expressiontext_focus_out_event(GtkWidget*, GdkEvent*, gpointer) {
	hide_completion();
	return FALSE;
}

int block_update_expression_icons = 0;
void block_expression_icon_update() {block_update_expression_icons++;}
void unblock_expression_icon_update() {block_update_expression_icons--;}
GtkWidget *prev_eb = NULL;
bool prev_ebv = false;
string prev_ebtext;

void showhide_expression_button() {
	if(block_update_expression_icons) return;
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), !expression_is_empty() || (gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) != GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")) && gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) != GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear"))));
}
void hide_expression_spinner() {
	if(prev_eb) {
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), prev_eb);
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stack")), prev_ebv);
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), prev_ebtext.c_str());
	}
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionspinnerbox")));
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultspinnerbox")));
}
void update_expression_icons(int id) {
	if(block_update_expression_icons) return;
	if(auto_calculate && !parsed_in_result && !rpn_mode && id == 0) id = EXPRESSION_CLEAR;
	switch(id) {
		case RESULT_SPINNER: {}
		case EXPRESSION_SPINNER: {
			prev_eb = gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")));
			prev_ebv = gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")));
			gchar *gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")));
			if(gstr) {
				prev_ebtext = gstr;
				g_free(gstr);
			}
		}
		case EXPRESSION_STOP: {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")));
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Stop process"));
			break;
		}
		case EXPRESSION_INFO: {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")));
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), gtk_widget_get_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon"))));
			break;
		}
		case EXPRESSION_CLEAR: {
			if(!rpn_mode) {
				gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear")));
				gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Clear expression"));
				break;
			}
		}
		default: {
			if(gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) != GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals"))) {
				gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")));
				gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), rpn_mode ? _("Calculate expression and add to stack") : _("Calculate expression"));
			}
		}
	}
	if(!enable_tooltips && id != EXPRESSION_INFO) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), "");
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionspinnerbox")), id == EXPRESSION_SPINNER);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultspinnerbox")), id == RESULT_SPINNER);
	showhide_expression_button();
}

void insert_angle_symbol() {
	if(!rpn_mode && do_chain_mode("∠")) return;
	insert_text("∠");
}

extern bool disable_history_arrow_keys;
extern GtkTreeModel *completion_sort;
extern GtkTreeIter tabbed_iter;
extern bool block_input;

#ifdef EVENT_CONTROLLER_TEST
gboolean on_expressiontext_key_press_event(GtkEventControllerKey*, guint keyval, guint, GdkModifierType state, gpointer) {
#else
gboolean on_expressiontext_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	GdkModifierType state; guint keyval = 0;
	gdk_event_get_state((GdkEvent*) event, &state);
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
#endif
	if(block_input && (keyval == GDK_KEY_q || keyval == GDK_KEY_Q) && !(state & GDK_CONTROL_MASK)) {block_input = false;
return TRUE;}
	if(calculator_busy()) {
		if(keyval == GDK_KEY_Escape) {
			abort_calculation();
		}
		return TRUE;
	}
	if(do_keyboard_shortcut(keyval, state)) return TRUE;
	FIX_ALT_GR
	switch(keyval) {
		case GDK_KEY_Escape: {
			if(completion_visible()) {
				hide_completion();
				return TRUE;
			} else if((close_with_esc > 0 || (close_with_esc < 0 && has_systray_icon())) && expression_is_empty()) {
				gtk_window_close(main_window());
				return TRUE;
			} else {
				clear_expression_text();
				return TRUE;
			}
			break;
		}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_Return: {
			if(completion_visible()) {
				if(completion_enter_pressed()) return TRUE;
			}
			if(rpn_mode || !expression_is_empty()) execute_expression();
			return TRUE;
		}
		case GDK_KEY_dead_circumflex: {
#ifdef _WIN32
			// fix dead key
			block_input = true;
			INPUT ip; ip.type = INPUT_KEYBOARD; ip.ki.wScan = 0; ip.ki.time = 0; ip.ki.dwExtraInfo = 0;
			ip.ki.wVk = 0x51; ip.ki.dwFlags = 0; SendInput(1, &ip, sizeof(INPUT));
			ip.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &ip, sizeof(INPUT));
#endif
		}
		case GDK_KEY_asciicircum: {
			bool input_xor = (caret_as_xor != ((state & GDK_CONTROL_MASK) > 0));
			if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				calculateRPN(input_xor ? OPERATION_BITWISE_XOR : OPERATION_RAISE);
				return TRUE;
			}
			if(expression_in_quotes()) break;
			if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				if(do_chain_mode(input_xor ? " xor " : "^")) return TRUE;
				if(!gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget()))) wrap_expression_selection();
			}
			overwrite_expression_selection(input_xor ? " xor " : "^");
			return TRUE;
		}
		case GDK_KEY_parenright: {
			if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
				brace_wrap();
				return true;
			}
			GtkTextMark *mpos = gtk_text_buffer_get_insert(expression_edit_buffer());
			if(mpos) {
				GtkTextIter ipos;
				gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mpos);
				if(gtk_text_iter_is_start(&ipos)) {
					brace_wrap();
					return true;
				}
			}
			break;
		}
		case GDK_KEY_slash: {}
		case GDK_KEY_KP_Divide: {
			if(state & GDK_CONTROL_MASK) {
				overwrite_expression_selection("/");
				return TRUE;
			}
			if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				calculateRPN(OPERATION_DIVIDE);
				return TRUE;
			}
			if(expression_in_quotes()) break;
			if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				if(do_chain_mode(expression_divide_sign())) return TRUE;
				if(!gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget()))) wrap_expression_selection();
			}
			overwrite_expression_selection(expression_divide_sign());
			return TRUE;
		}
		case GDK_KEY_plus: {}
		case GDK_KEY_KP_Add: {
			if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				calculateRPN(OPERATION_ADD);
				return TRUE;
			}
			if(expression_in_quotes()) break;
			if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				if(do_chain_mode(expression_add_sign())) return TRUE;
				if(!gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget()))) wrap_expression_selection();
			}
			overwrite_expression_selection(expression_add_sign());
			return TRUE;
		}
		case GDK_KEY_minus: {}
		case GDK_KEY_KP_Subtract: {
			if(rpn_mode && state & GDK_CONTROL_MASK) {
				insert_button_function(CALCULATOR->getActiveFunction("neg"));
				return TRUE;
			}
			if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				if(!expression_is_empty()) {
					GtkTextIter icur;
					if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
						gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &icur, NULL);
					} else {
						GtkTextMark *mcur = gtk_text_buffer_get_insert(expression_edit_buffer());
						if(mcur) gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &icur, mcur);
					}
					if(gtk_text_iter_backward_char(&icur) && (gtk_text_iter_get_char(&icur) == 'E' ||gtk_text_iter_get_char(&icur) == 'e') && gtk_text_iter_backward_char(&icur) && is_in(NUMBERS, gtk_text_iter_get_char(&icur))) {
						insert_text(expression_sub_sign());
						return TRUE;
					}
				}
				calculateRPN(OPERATION_SUBTRACT);
				return TRUE;
			}
			if(expression_in_quotes()) break;
			if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				if(do_chain_mode(expression_sub_sign())) return TRUE;
				if(!gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget()))) wrap_expression_selection();
			}
			overwrite_expression_selection(expression_sub_sign());
			return TRUE;
		}
		case GDK_KEY_KP_Multiply: {}
		case GDK_KEY_asterisk: {
			if(state & GDK_CONTROL_MASK) {
				if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
					calculateRPN(OPERATION_RAISE);
					return TRUE;
				}
				if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
					if(do_chain_mode("^")) return TRUE;
					if(!gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget()))) wrap_expression_selection();
				}
				overwrite_expression_selection("^");
				return TRUE;
			}
			if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				calculateRPN(OPERATION_MULTIPLY);
				return TRUE;
			}
			if(expression_in_quotes()) break;
			if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
				if(do_chain_mode(expression_times_sign())) return TRUE;
				if(!gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget()))) wrap_expression_selection();
			}
			overwrite_expression_selection(expression_times_sign());
			return TRUE;
		}
		case GDK_KEY_E: {
			if(state & GDK_CONTROL_MASK) {
				if(rpn_mode && rpn_keys && evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
					calculateRPN(OPERATION_EXP10);
					return TRUE;
				}
				if((evalops.parse_options.parsing_mode != PARSING_MODE_RPN && !gtk_text_view_get_overwrite(GTK_TEXT_VIEW(expression_edit_widget())) && wrap_expression_selection() > 0) || (evalops.parse_options.base != 10 && evalops.parse_options.base >= 2)) {
					insert_text((expression_times_sign() + i2s(evalops.parse_options.base) + "^").c_str());
				} else {
					if(printops.exp_display == EXP_LOWERCASE_E) insert_text("e");
					else insert_text("E");
				}
				return TRUE;
			}
			break;
		}
		case GDK_KEY_A: {
			if(state & GDK_CONTROL_MASK) {
				insert_angle_symbol();
				return TRUE;
			}
			break;
		}
		case GDK_KEY_End: {
			GtkTextIter iend;
			gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
			if(state & GDK_SHIFT_MASK) {
				GtkTextIter iselstart, iselend, ipos;
				GtkTextMark *mark = gtk_text_buffer_get_insert(expression_edit_buffer());
				if(mark) gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mark);
				if(!gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &iselstart, &iselend)) gtk_text_buffer_select_range(expression_edit_buffer(), &iend, &ipos);
				else if(gtk_text_iter_equal(&iselstart, &ipos)) gtk_text_buffer_select_range(expression_edit_buffer(), &iend, &iselend);
				else gtk_text_buffer_select_range(expression_edit_buffer(), &iend, &iselstart);
			} else {
				gtk_text_buffer_place_cursor(expression_edit_buffer(), &iend);
			}
			return TRUE;
		}
		case GDK_KEY_Home: {
			GtkTextIter istart;
			gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
			if(state & GDK_SHIFT_MASK) {
				GtkTextIter iselstart, iselend, ipos;
				GtkTextMark *mark = gtk_text_buffer_get_insert(expression_edit_buffer());
				if(mark) gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mark);
				if(!gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &iselstart, &iselend)) gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &ipos);
				else if(gtk_text_iter_equal(&iselend, &ipos)) gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iselstart);
				else gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iselend);
			} else {
				gtk_text_buffer_place_cursor(expression_edit_buffer(), &istart);
			}
			return TRUE;
		}
		case GDK_KEY_Up: {
			key_up:
			if(completion_visible()) {
				completion_up_pressed();
				return TRUE;
			}
			if(disable_history_arrow_keys || state & GDK_SHIFT_MASK || state & GDK_CONTROL_MASK) break;
			GtkTextIter ipos;
			GtkTextMark *mark = gtk_text_buffer_get_insert(expression_edit_buffer());
			if(mark) {
				gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mark);
				if((cursor_has_moved && (!gtk_text_iter_is_start(&ipos) || gtk_text_buffer_get_has_selection(expression_edit_buffer()))) || (!gtk_text_iter_is_end(&ipos) && !gtk_text_iter_is_start(&ipos)) || gtk_text_view_backward_display_line(GTK_TEXT_VIEW(expression_edit_widget()), &ipos)) {
					disable_history_arrow_keys = true;
					break;
				}
			}
			if(!expression_history_up()) break;
			cursor_has_moved = false;
			return TRUE;
		}
		case GDK_KEY_KP_Page_Up: {}
		case GDK_KEY_Page_Up: {
			if(expression_history_up()) return TRUE;
			break;
		}
		case GDK_KEY_ISO_Left_Tab: {
			if(tabbed_completion) {
				GtkTreePath *path = NULL;
				if(gtk_tree_model_iter_previous(completion_sort, &tabbed_iter)) {
					path = gtk_tree_model_get_path(completion_sort, &tabbed_iter);
				} else {
					gint rows = gtk_tree_model_iter_n_children(completion_sort, NULL);
					if(rows > 0) {
						path = gtk_tree_path_new_from_indices(rows - 1, -1);
					}
				}
				if(path) {
					on_completion_match_selected(GTK_TREE_VIEW(gtk_builder_get_object(main_builder, "completionview")), path, NULL, NULL);
					gtk_tree_path_free(path);
					tabbed_completion = true;
					return TRUE;
				}
			}
		}
		case GDK_KEY_Tab: {
			if(!completion_visible()) break;
			if(state & GDK_SHIFT_MASK) goto key_up;
		}
		case GDK_KEY_Down: {
			if(completion_visible()) {
				completion_down_pressed();
				return TRUE;
			}
			if(disable_history_arrow_keys || state & GDK_SHIFT_MASK || state & GDK_CONTROL_MASK) break;
			GtkTextIter ipos;
			GtkTextMark *mark = gtk_text_buffer_get_insert(expression_edit_buffer());
			if(mark) {
				gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mark);
				if((cursor_has_moved && (!gtk_text_iter_is_end(&ipos) || gtk_text_buffer_get_has_selection(expression_edit_buffer()))) || (!gtk_text_iter_is_end(&ipos) && !gtk_text_iter_is_start(&ipos)) || gtk_text_view_forward_display_line(GTK_TEXT_VIEW(expression_edit_widget()), &ipos)) {
					disable_history_arrow_keys = true;
					break;
				}
			}
			expression_history_down();
			cursor_has_moved = false;
			return TRUE;
		}
		case GDK_KEY_KP_Page_Down: {}
		case GDK_KEY_Page_Down: {
			expression_history_down();
			return TRUE;
		}
		case GDK_KEY_KP_Separator: {
			overwrite_expression_selection(CALCULATOR->getDecimalPoint().c_str());
			return TRUE;
		}
	}
	if(state & GDK_CONTROL_MASK && keyval == GDK_KEY_c && !gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		copy_result();
		return TRUE;
	}
	if(state & GDK_CONTROL_MASK && (keyval == GDK_KEY_z || keyval == GDK_KEY_Z)) {
		if(state & GDK_SHIFT_MASK || keyval == GDK_KEY_Z) expression_redo();
		else expression_undo();
		return TRUE;
	}
	return FALSE;
}

void expression_set_from_undo_buffer() {
	if(undo_index < expression_undo_buffer.size()) {
		string str_old = get_expression_text();
		string str_new = expression_undo_buffer[undo_index];
		if(str_old == str_new) return;
		size_t i;
		block_undo();
		GtkTextIter istart, iend;
		if(str_old.length() > str_new.length()) {
			if((i = str_old.find(str_new)) != string::npos) {
				if(i != 0) {
					gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &iend, g_utf8_strlen(str_old.c_str(), i));
					gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
					gtk_text_buffer_delete(expression_edit_buffer(), &istart, &iend);
				}
				if(i + str_new.length() < str_old.length()) {
					gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_new.c_str(), -1));
					gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
					gtk_text_buffer_delete(expression_edit_buffer(), &istart, &iend);
				}
				unblock_undo();
				return;
			}
			for(i = 0; i < str_new.length(); i++) {
				if(str_new[i] != str_old[i]) {
					if(i == 0) break;
					string str_test = str_old.substr(0, i);
					str_test += str_old.substr(i + str_old.length() - str_new.length());
					if(str_test == str_new) {
						gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_old.c_str(), i));
						gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &iend, g_utf8_strlen(str_old.c_str(), i + str_old.length() - str_new.length()));
						gtk_text_buffer_delete(expression_edit_buffer(), &istart, &iend);
						unblock_undo();
						return;
					}
					if(str_new.length() + 1 == str_old.length()) break;
					str_test = str_old.substr(0, i);
					str_test += str_old.substr(i + str_old.length() - str_new.length() - 1);
					size_t i2 = i;
					while((i2 = str_test.find(')', i2 + 1)) != string::npos) {
						string str_test2 = str_test;
						str_test2.erase(str_test2.begin() + i2);
						if(str_test2 == str_new) {
							gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_old.c_str(), i));
							gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &iend, g_utf8_strlen(str_old.c_str(), i + str_old.length() - str_new.length() - 1));
							gtk_text_buffer_delete(expression_edit_buffer(), &istart, &iend);
							gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_old.c_str(), i2));
							iend = istart;
							gtk_text_iter_forward_char(&iend);
							gtk_text_buffer_delete(expression_edit_buffer(), &istart, &iend);
							unblock_undo();
							return;
						}
					}
					break;
				}
			}
		} else if(str_new.length() > str_old.length()) {
			if((i = str_new.find(str_old)) != string::npos) {
				if(i + str_old.length() < str_new.length()) {
					gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
					gtk_text_buffer_insert(expression_edit_buffer(), &iend, str_new.substr(i + str_old.length(), str_new.length() - (i + str_old.length())).c_str(), -1);
				}
				if(i > 0) {
					gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
					gtk_text_buffer_insert(expression_edit_buffer(), &istart, str_new.substr(0, i).c_str(), -1);
				}
				unblock_undo();
				return;
			}
			for(i = 0; i < str_old.length(); i++) {
				if(str_old[i] != str_new[i]) {
					if(i == 0) break;
					string str_test = str_new.substr(0, i);
					str_test += str_new.substr(i + str_new.length() - str_old.length());
					if(str_test == str_old) {
						gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_new.c_str(), i));
						gtk_text_buffer_insert(expression_edit_buffer(), &istart, str_new.substr(i, str_new.length() - str_old.length()).c_str(), -1);
						unblock_undo();
						return;
					}
					if(str_old.length() + 1 == str_new.length()) break;
					str_test = str_new.substr(0, i);
					str_test += str_new.substr(i + str_new.length() - str_old.length() - 1);
					size_t i2 = i;
					while((i2 = str_test.find(')', i2 + 1)) != string::npos) {
						string str_test2 = str_test;
						str_test2.erase(str_test2.begin() + i2);
						if(str_test2 == str_old) {
							gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_new.c_str(), i));
							gtk_text_buffer_insert(expression_edit_buffer(), &istart, str_new.substr(i, str_new.length() - str_old.length() - 1).c_str(), -1);
							gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &istart, g_utf8_strlen(str_new.c_str(), i2 + str_new.length() - str_old.length() - 1));
							gtk_text_buffer_insert(expression_edit_buffer(), &istart, ")", -1);
							unblock_undo();
							return;
						}
					}
					break;
				}
			}
		}
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(expression_edit_widget()), FALSE);
		gtk_text_buffer_set_text(expression_edit_buffer(), str_new.c_str(), -1);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(expression_edit_widget()), TRUE);
		unblock_undo();
	}
}
void expression_undo() {
	if(undo_index == 0) return;
	undo_index--;
	expression_set_from_undo_buffer();
}
void expression_redo() {
	if(undo_index >= expression_undo_buffer.size() - 1) return;
	undo_index++;
	expression_set_from_undo_buffer();
}

void on_popup_menu_item_completion_level_toggled(GtkCheckMenuItem *w, gpointer p) {
	if(!gtk_check_menu_item_get_active(w)) return;
	int completion_level = GPOINTER_TO_INT(p);
	set_expression_completion_settings(completion_level > 0, completion_level > 2, completion_level == 2 || completion_level > 3 ? 1 : 2, completion_level > 3 ? 1 : 2);
	preferences_update_completion();
}
void on_popup_menu_item_completion_delay_toggled(GtkCheckMenuItem *w, gpointer) {
	set_expression_completion_settings(-1, -1, -1, -1, gtk_check_menu_item_get_active(w) ? 500 : 0);
	preferences_update_completion();
}
void on_popup_menu_item_custom_completion_activated(GtkMenuItem*, gpointer) {
	edit_preferences(main_window(), 4);
}
void on_popup_menu_item_read_precision_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_limit_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_adaptive_parsing_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_chain_syntax_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_ignore_whitespace_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_rpn_syntax_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_rpn_mode_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_abort_activate(GtkMenuItem*, gpointer) {
	abort_calculation();
}
void on_popup_menu_item_clear_activate(GtkMenuItem*, gpointer) {
	clear_expression_text();
	focus_keeping_selection();
}
void clear_expression_history() {
	expression_history.clear();
	expression_history_index = -1;
	current_history_expression = "";
}
void on_popup_menu_item_clear_history_activate(GtkMenuItem*, gpointer) {
	clear_expression_history();
}

extern void on_menu_item_meta_mode_activate(GtkMenuItem *w, gpointer user_data);
extern void on_menu_item_meta_mode_save_activate(GtkMenuItem *w, gpointer user_data);
extern gboolean on_menu_item_meta_mode_popup_menu(GtkWidget*, gpointer data);
extern gboolean on_menu_item_meta_mode_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);

void expression_insert_date() {
	GtkWidget *d = gtk_dialog_new_with_buttons(_("Select date"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
	GtkWidget *date_w = gtk_calendar_new();
	string str = get_selected_expression_text(), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	int b_quote = -1;
	if(str.length() > 2 && ((str[0] == '\"' && str[str.length() - 1] == '\"') || (str[0] == '\'' && str[str.length() - 1] == '\''))) {
		str = str.substr(1, str.length() - 2);
		remove_blank_ends(str);
		b_quote = 1;
	}
	if(!str.empty()) {
		QalculateDateTime date;
		if(date.set(str)) {
			if(b_quote < 0 && is_in(NUMBERS, str[0])) b_quote = 0;
			gtk_calendar_select_month(GTK_CALENDAR(date_w), date.month() - 1, date.year());
			gtk_calendar_select_day(GTK_CALENDAR(date_w), date.day());
		}
	}
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), date_w);
	gtk_widget_show_all(d);
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_OK) {
		guint year = 0, month = 0, day = 0;
		gtk_calendar_get_date(GTK_CALENDAR(date_w), &year, &month, &day);
		gchar *gstr;
		if(b_quote == 0) gstr = g_strdup_printf("%i-%02i-%02i", year, month + 1, day);
		else gstr = g_strdup_printf("\"%i-%02i-%02i\"", year, month + 1, day);
		insert_text(gstr);
		g_free(gstr);
	}
	gtk_widget_destroy(d);
}
void expression_insert_matrix() {
	string str = get_selected_expression_text(), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	if(!str.empty()) {
		MathStructure mstruct_sel;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_sel, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct_sel.isMatrix() && mstruct_sel[0].size() > 0) {
			insert_matrix(&mstruct_sel, main_window(), false);
			return;
		}
	}
	insert_matrix(NULL, main_window(), false);
}
void expression_insert_vector() {
	string str = get_selected_expression_text(), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	if(!str.empty()) {
		MathStructure mstruct_sel;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_sel, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct_sel.isVector() && !mstruct_sel.isMatrix()) {
			insert_matrix(&mstruct_sel, main_window(), true);
			return;
		}
	}
	insert_matrix(NULL, main_window(), true);
}

GtkMenu *expression_edit_popup_menu() {return popup_menu_expressiontext;}

void on_popup_menu_item_insert_date_activate(GtkMenuItem*, gpointer) {
	expression_insert_date();
}
void on_popup_menu_item_insert_matrix_activate(GtkMenuItem*, gpointer) {
	expression_insert_matrix();
}
void on_popup_menu_item_insert_vector_activate(GtkMenuItem*, gpointer) {
	expression_insert_vector();
}

bool block_popup_input_base = false;
void on_popup_menu_item_input_base(GtkMenuItem *w, gpointer data) {
	if(block_popup_input_base) return;
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_input_base(GPOINTER_TO_INT(data), true, false);
}
GtkCssProvider *expression_popup_provider = NULL;
extern string custom_app_font;
void on_expressiontext_populate_popup(GtkTextView*, GtkMenu *menu, gpointer) {
	popup_menu_expressiontext = menu;
	GtkWidget *item, *sub, *sub2;
	GSList *group = NULL;
	sub = GTK_WIDGET(menu);
	if(!expression_popup_provider) expression_popup_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(sub), GTK_STYLE_PROVIDER(expression_popup_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gchar *gstr = font_name_to_css(custom_app_font.c_str());
	gtk_css_provider_load_from_data(expression_popup_provider, gstr, -1, NULL);
	g_free(gstr);
	MENU_ITEM(_("Clear"), on_popup_menu_item_clear_activate)
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(item))), GDK_KEY_Escape, (GdkModifierType) 0);
	if(expression_is_empty()) gtk_widget_set_sensitive(item, FALSE);
	MENU_ITEM(_("Clear History"), on_popup_menu_item_clear_history_activate)
	if(expression_history.empty()) gtk_widget_set_sensitive(item, FALSE);
	MENU_SEPARATOR
	if(calculator_busy()) {
		MENU_ITEM(_("Abort"), on_popup_menu_item_abort_activate)
		return;
	}
	MENU_ITEM(_("Undo"), expression_undo)
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(item))), GDK_KEY_z, (GdkModifierType) GDK_CONTROL_MASK);
	if(undo_index == 0) gtk_widget_set_sensitive(item, FALSE);
	MENU_ITEM(_("Redo"), expression_redo)
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(item))), GDK_KEY_z, (GdkModifierType) (GDK_SHIFT_MASK | GDK_CONTROL_MASK));
	if(undo_index >= expression_undo_buffer.size() - 1) gtk_widget_set_sensitive(item, FALSE);
	MENU_SEPARATOR
	sub2 = sub;
	SUBMENU_ITEM(_("Completion Mode"), sub2);
	int completion_level = 0;
	if(enable_completion) {
		if(enable_completion2) {
			if(completion_min2 > 1) completion_level = 3;
			else completion_level = 4;
		} else {
			if(completion_min > 1) completion_level = 1;
			else completion_level = 2;
		}
	}
	for(gint i = 0; i < 5; i++) {
		switch(i) {
			case 1: {item = gtk_radio_menu_item_new_with_label(group, _("Limited strict completion")); break;}
			case 2: {item = gtk_radio_menu_item_new_with_label(group, _("Strict completion")); break;}
			case 3: {item = gtk_radio_menu_item_new_with_label(group, _("Limited full completion")); break;}
			case 4: {item = gtk_radio_menu_item_new_with_label(group, _("Full completion")); break;}
			default: {item = gtk_radio_menu_item_new_with_label(group, _("No completion"));}
		}
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
		gtk_widget_show(item);
		if(i == completion_level) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(on_popup_menu_item_completion_level_toggled), GINT_TO_POINTER(i));
		gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
	}
	MENU_SEPARATOR
	CHECK_MENU_ITEM(_("Delayed completion"), on_popup_menu_item_completion_delay_toggled, completion_delay > 0)
	MENU_SEPARATOR
	MENU_ITEM(_("Customize completion…"), on_popup_menu_item_custom_completion_activated)
	group = NULL;
	SUBMENU_ITEM(_("Parsing Mode"), sub2);
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_adaptive_parsing_activate, gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing"))
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_ignore_whitespace_activate, gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace"))
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_no_special_implicit_multiplication_activate, gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication"))
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_chain_syntax_activate, gtk_builder_get_object(main_builder, "menu_item_chain_syntax"))
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_rpn_syntax_activate, gtk_builder_get_object(main_builder, "menu_item_rpn_syntax"))
	MENU_SEPARATOR
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_limit_implicit_multiplication_activate, gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication"))
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_read_precision_activate, gtk_builder_get_object(main_builder, "menu_item_read_precision"))
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_rpn_mode_activate, gtk_builder_get_object(main_builder, "menu_item_rpn_mode"))
	SUBMENU_ITEM(_("Number Base"), sub2);
	group = NULL;
	block_popup_input_base = true;
	RADIO_MENU_ITEM_WITH_INT(_("Binary"), on_popup_menu_item_input_base, evalops.parse_options.base == 2, 2)
	RADIO_MENU_ITEM_WITH_INT(_("Octal"), on_popup_menu_item_input_base, evalops.parse_options.base == 8, 8)
	RADIO_MENU_ITEM_WITH_INT(_("Decimal"), on_popup_menu_item_input_base, evalops.parse_options.base == 10, 10)
	RADIO_MENU_ITEM_WITH_INT(_("Duodecimal"), on_popup_menu_item_input_base, evalops.parse_options.base == 12, 12)
	RADIO_MENU_ITEM_WITH_INT(_("Hexadecimal"), on_popup_menu_item_input_base, evalops.parse_options.base == 16, 16)
	RADIO_MENU_ITEM_WITH_INT(_("Roman Numerals"), on_popup_menu_item_input_base, evalops.parse_options.base == BASE_ROMAN_NUMERALS, BASE_ROMAN_NUMERALS)
	RADIO_MENU_ITEM_WITH_INT(_("Other…"), on_popup_menu_item_input_base, evalops.parse_options.base != 2 && evalops.parse_options.base != 8 && evalops.parse_options.base != 10 && evalops.parse_options.base != 12 && evalops.parse_options.base != 16 && evalops.parse_options.base != BASE_ROMAN_NUMERALS, BASE_CUSTOM)
	block_popup_input_base = false;
	SUBMENU_ITEM(_("Meta Modes"), sub2)
	popup_expression_mode_items.clear();
	for(size_t i = 0; ; i++) {
		mode_struct *mode = get_mode(i);
		if(!mode) break;
		item = gtk_menu_item_new_with_label(mode->name.c_str());
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) mode->name.c_str());
		g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) mode->name.c_str());
		g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) mode->name.c_str());
		popup_expression_mode_items.push_back(item);
		gtk_menu_shell_insert(GTK_MENU_SHELL(sub), item, (gint) i);
	}
	MENU_SEPARATOR
	MENU_ITEM(_("Save Mode…"), on_menu_item_meta_mode_save_activate)
	sub = sub2;
	MENU_SEPARATOR
	MENU_ITEM(_("Insert Date…"), on_popup_menu_item_insert_date_activate)
	MENU_ITEM(_("Insert Matrix…"), on_popup_menu_item_insert_matrix_activate)
	MENU_ITEM(_("Insert Vector…"), on_popup_menu_item_insert_vector_activate)
}

gboolean epxression_tooltip_timeout(gpointer) {
	gtk_widget_trigger_tooltip_query(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")));
	return FALSE;
}
gboolean on_button_minimal_mode_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(button != 1) return FALSE;
	set_minimal_mode(false);
	return TRUE;
}
gboolean on_expression_button_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(button != 1) return FALSE;
	GtkWidget *w = gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")));
	if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals"))) {
		execute_expression();
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear"))) {
		clear_expression_text();
		focus_expression();
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon"))) {
		g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 0, epxression_tooltip_timeout, NULL, NULL);
	} else {
		abort_calculation();
	}
	return TRUE;
}
gboolean on_expression_button_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	if(button != 1) return FALSE;
	if(gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) == GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon"))) {
		g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 0, epxression_tooltip_timeout, NULL, NULL);
		return TRUE;
	}
	return FALSE;
}
gboolean on_expressiontext_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS) {
		if(calculator_busy()) return TRUE;
	}
	return FALSE;
}

void set_expression_highlight_background(bool b) {
	highlight_background = b;
	update_expression_colors(false, false);
}
bool get_expression_highlight_background() {
	return highlight_background && get_expression_enable_highlight_background();
}
bool get_expression_enable_highlight_background() {
	GdkRGBA c;
	gtk_style_context_get_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &c);
	return c.green < 0.8;
}

void update_expression_colors(bool initial, bool keep_pars) {
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14

	gint scalefactor = gtk_widget_get_scale_factor(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")));
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 16 * scalefactor, 16 * scalefactor);
	cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
	cairo_t *cr = cairo_create(surface);
	GdkRGBA rgba;
	gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals"))), GTK_STATE_FLAG_NORMAL, &rgba);

	PangoLayout *layout_equals = gtk_widget_create_pango_layout(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), "=");
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
	pango_font_description_set_weight(font_desc, PANGO_WEIGHT_MEDIUM);
	pango_font_description_set_absolute_size(font_desc, PANGO_SCALE * 26);
	pango_layout_set_font_description(layout_equals, font_desc);

	PangoRectangle rect, lrect;
	pango_layout_get_pixel_extents(layout_equals, &rect, &lrect);
	if(rect.width >= 16) {
		pango_font_description_set_absolute_size(font_desc, PANGO_SCALE * 18);
		pango_layout_set_font_description(layout_equals, font_desc);
		pango_layout_get_pixel_extents(layout_equals, &rect, &lrect);
	}

	pango_font_description_free(font_desc);

	double offset_x = (rect.x - (lrect.width - (rect.x + rect.width))) / 2.0, offset_y = (rect.y - (lrect.height - (rect.y + rect.height))) / 2.0;
	cairo_move_to(cr, (16 - lrect.width) / 2.0 - offset_x, (16 - lrect.height) / 2.0 - offset_y);
	gdk_cairo_set_source_rgba(cr, &rgba);
	pango_cairo_show_layout(cr, layout_equals);
	g_object_unref(layout_equals);
	cairo_destroy(cr);
	gtk_image_set_from_surface(GTK_IMAGE(gtk_builder_get_object(main_builder, "expression_button_equals")), surface);
	cairo_surface_destroy(surface);

#endif

	if(initial || !keep_pars) {
		GdkRGBA c;
		gtk_style_context_get_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &c);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
		GdkRGBA bg_color;
		gtk_style_context_get_background_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &bg_color);
		if(gdk_rgba_equal(&c, &bg_color)) {
			gtk_style_context_get_color(gtk_widget_get_style_context(parse_status_widget()), GTK_STATE_FLAG_NORMAL, &c);
		}
#endif
		GdkRGBA c_par = c;
		if(c.green >= 0.8) {
			c_par.red /= 2;
			c_par.blue /= 2;
			c_par.green = 1.0;
		} else {
			if(highlight_background) {
				c_par.red = 0.7;
				c_par.green = 0.95;
				c_par.blue = 0.75;
			} else {
				if(c_par.green >= 0.5) c_par.green = 1.0;
				else if(c_par.green > 0.1) c_par.green += 0.5;
				else c_par.green = 0.6;
			}
		}
		GdkRGBA c_par_u = c;
		if(c.green >= 0.8) {
			c_par_u.red = 1.0;
			c_par_u.blue /= 2;
			c_par_u.green /= 2;
		} else {
			if(highlight_background) {
				c_par_u.red = 1;
				c_par_u.green = 0.8;
				c_par_u.blue = 0.8;
			} else {
				if(c_par_u.red >= 0.5) c_par_u.red = 1.0;
				else if(c_par_u.red > 0.2) c_par_u.red += 0.5;
				else c_par_u.red = 0.7;
			}
		}
		if(initial) {
			PangoLayout *layout_par = gtk_widget_create_pango_layout(expression_edit_widget(), "(1)");
			gint w1 = 0, w2 = 0;
			pango_layout_get_pixel_size(layout_par, &w1, NULL);
			pango_layout_set_markup(layout_par, "<b>(</b>1<b>)</b>", -1);
			pango_layout_get_pixel_size(layout_par, &w2, NULL);
			g_object_unref(layout_par);
			if(c.green >= 0.8 || !highlight_background) {
				if(w1 == w2) expression_par_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar", "foreground-rgba", &c_par, "weight", PANGO_WEIGHT_BOLD, NULL);
				else expression_par_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar", "foreground-rgba", &c_par, NULL);
				if(w1 == w2) expression_par_u_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar_u", "foreground-rgba", &c_par_u, "weight", PANGO_WEIGHT_BOLD, NULL);
				else expression_par_u_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar_u", "foreground-rgba", &c_par_u, NULL);
			} else {
				if(w1 == w2) expression_par_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar", "background-rgba", &c_par, "weight", PANGO_WEIGHT_BOLD, NULL);
				else expression_par_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar", "background-rgba", &c_par, NULL);
				if(w1 == w2) expression_par_u_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar_u", "background-rgba", &c_par_u, "weight", PANGO_WEIGHT_BOLD, NULL);
				else expression_par_u_tag = gtk_text_buffer_create_tag(expression_edit_buffer(), "curpar_u", "background-rgba", &c_par_u, NULL);
			}
		} else {
			if(c.green >= 0.8 || !highlight_background) g_object_set(G_OBJECT(expression_par_tag), "foreground-rgba", &c_par, "background-rgba", NULL, NULL);
			else g_object_set(G_OBJECT(expression_par_tag), "background-rgba", &c_par, "foreground-rgba", NULL, NULL);
			if(c.green >= 0.8 || !highlight_background) g_object_set(G_OBJECT(expression_par_u_tag), "foreground-rgba", &c_par_u, "background-rgba", NULL, NULL);
			else g_object_set(G_OBJECT(expression_par_u_tag), "background-rgba", &c_par_u, "foreground-rgba", NULL, NULL);
		}
	}
}

void set_expression_size_request() {
	string test_str = "Äy";
	for(int i = 1; i < (expression_lines < 1 ? 3 : expression_lines); i++) test_str += "\nÄy";
	PangoLayout *layout_test = gtk_widget_create_pango_layout(expression_edit_widget(), test_str.c_str());
	gint h;
	pango_layout_get_pixel_size(layout_test, NULL, &h);
	g_object_unref(layout_test);
	h += 12;
	bool show_eb = gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")));
	gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")));
	gint h2 = 0;
	gtk_widget_get_preferred_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_expression_buttons")), NULL, &h2);
	if(!show_eb) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")));
	if(h2 <= 0) h2 = minimal_mode ? 58 : 34;
	if(minimal_mode) h2 += 2;
	if(h < h2) h = h2;
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), -1, h);
	layout_test = gtk_widget_create_pango_layout(expression_edit_widget(), "Äy");
	pango_layout_get_pixel_size(layout_test, NULL, &h);
	g_object_unref(layout_test);
	h = h / 2 - 4;
	if(h < 0) h = 0;
	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), h);
	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear")), h);
	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")), h);
	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), h);
}
void expression_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	set_expression_size_request();
	set_expression_operator_symbols();
	PangoLayout *layout_par = gtk_widget_create_pango_layout(expression_edit_widget(), "(1)");
	gint w1 = 0, w2 = 0;
	pango_layout_get_pixel_size(layout_par, &w1, NULL);
	pango_layout_set_markup(layout_par, "<b>(</b>1<b>)</b>", -1);
	pango_layout_get_pixel_size(layout_par, &w2, NULL);
	if(w1 == w2) g_object_set(expression_par_tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	else g_object_set(expression_par_tag, "weight", PANGO_WEIGHT_NORMAL, NULL);
	if(w1 == w2) g_object_set(expression_par_u_tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	else g_object_set(expression_par_u_tag, "weight", PANGO_WEIGHT_NORMAL, NULL);
}

void update_expression_font(bool initial) {
	gint h_old = 0, h_new = 0;
	if(!initial) gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h_old);
	if(use_custom_expression_font) {
		gchar *gstr;
		if(RUNTIME_CHECK_GTK_VERSION(3, 20)) gstr = font_name_to_css(custom_expression_font.c_str(), "textview.view");
		else gstr = font_name_to_css(custom_expression_font.c_str());
		gtk_css_provider_load_from_data(expression_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		if(initial && custom_expression_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			pango_font_description_set_size(font_desc, round(pango_font_description_get_size(font_desc) * 1.2 / PANGO_SCALE) * PANGO_SCALE);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_expression_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
		if(RUNTIME_CHECK_GTK_VERSION(3, 20)) gtk_css_provider_load_from_data(expression_provider, "textview.view {font-size: 120%;}", -1, NULL);
		else gtk_css_provider_load_from_data(expression_provider, "* {font-size: 120%;}", -1, NULL);
	}
	if(!initial) {
		expression_font_modified();
		gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h_new);
		gint winh, winw;
		gtk_window_get_size(main_window(), &winw, &winh);
		winh += (h_new - h_old);
		gtk_window_resize(main_window(), winw, winh);
	}
}
void set_expression_font(const char *str) {
	if(!str) {
		use_custom_expression_font = false;
	} else {
		use_custom_expression_font = true;
		if(custom_expression_font != str) {
			save_custom_expression_font = true;
			custom_expression_font = str;
		}
	}
	update_expression_font(false);
}
const char *expression_font(bool return_default) {
	if(!return_default && !use_custom_expression_font) return NULL;
	return custom_expression_font.c_str();
}


void create_expression_edit() {

	expression_undo_buffer.push_back("");
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(expression_edit_widget()), 12);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(expression_edit_widget()), 6);
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(expression_edit_widget()), 6);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(expression_edit_widget()), 6);
#else
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(expression_edit_widget()), 12);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(expression_edit_widget()), 6);
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 22)) {
		gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(expression_edit_widget()), GTK_TEXT_WINDOW_TOP, 6);
		gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(expression_edit_widget()), GTK_TEXT_WINDOW_BOTTOM, 6);
	}
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION > 22 || (GTK_MINOR_VERSION == 22 && GTK_MICRO_VERSION >= 20)
	gtk_text_view_set_input_hints(GTK_TEXT_VIEW(expression_edit_widget()), GTK_INPUT_HINT_NO_EMOJI);
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")), 6);
#else
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")), 6);
#endif
	expression_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(expression_edit_widget()), GTK_STYLE_PROVIDER(expression_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 18
	GtkCssProvider *expressionborder_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(expression_edit_widget()), GTK_STYLE_PROVIDER(expressionborder_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	string border_css = topframe_css; border_css += "}";
	gsub("*", "textview.view > border", border_css);
	if(RUNTIME_CHECK_GTK_VERSION(3, 22)) border_css += "\ntextview.view {padding-top: 6px; padding-bottom: 6px;}";
	gtk_css_provider_load_from_data(expressionborder_provider, border_css.c_str(), -1, NULL);
#endif
	GtkCssProvider *expression_provider2 = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(expression_edit_widget()), GTK_STYLE_PROVIDER(expression_provider2), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	string expression_css = topframe_css; expression_css += "}";
	gsub("*", "textview.view > text", expression_css);
	gtk_css_provider_load_from_data(expression_provider2, expression_css.c_str(), -1, NULL);

	update_expression_font(true);
	set_expression_operator_symbols();

	gtk_widget_grab_focus(expression_edit_widget());
	gtk_widget_set_can_default(expression_edit_widget(), TRUE);
	gtk_widget_grab_default(expression_edit_widget());

	if(rpn_mode) gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Calculate expression and add to stack"));

	create_expression_completion();

	set_expression_size_request();

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), FALSE);

#ifdef EVENT_CONTROLLER_TEST
	GtkEventController *controller = gtk_event_controller_key_new(expression_edit_widget());
	gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
	g_signal_connect(G_OBJECT(controller), "key-pressed", G_CALLBACK(on_expressiontext_key_press_event), NULL);
#else
	g_signal_connect(G_OBJECT(expression_edit_widget()), "key-press-event", G_CALLBACK(on_expressiontext_key_press_event), NULL);
#endif

	gtk_builder_add_callback_symbols(main_builder, "on_expressionbuffer_changed", G_CALLBACK(on_expressionbuffer_changed), "on_expressionbuffer_cursor_position_notify", G_CALLBACK(on_expressionbuffer_cursor_position_notify), "on_expressionbuffer_paste_done", G_CALLBACK(on_expressionbuffer_paste_done), "on_expressiontext_button_press_event", G_CALLBACK(on_expressiontext_button_press_event), "on_expressiontext_focus_out_event", G_CALLBACK(on_expressiontext_focus_out_event), "on_expressiontext_populate_popup", G_CALLBACK(on_expressiontext_populate_popup), "on_expression_button_button_press_event", G_CALLBACK(on_expression_button_button_press_event), "on_expression_button_button_release_event", G_CALLBACK(on_expression_button_button_release_event), "on_button_minimal_mode_button_press_event", G_CALLBACK(on_button_minimal_mode_button_press_event), NULL);

}
