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
#include "plotdialog.h"
#include "expressionedit.h"
#include "insertfunctiondialog.h"
#include "keypad.h"
#include "historyview.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::deque;

#define EXPRESSION_YPAD 3

deque<string> inhistory;
deque<bool> inhistory_protected;
deque<int> inhistory_type;
deque<int> inhistory_value;
deque<time_t> inhistory_time;
int unformatted_history = 0;
vector<MathStructure*> history_parsed;
vector<MathStructure*> history_answer;
vector<string> history_bookmarks;
unordered_map<string, size_t> history_bookmark_titles;

int current_inhistory_index = -1;
int history_index = 0;
int history_index_bak = 0;
int initial_inhistory_index = 0;
int inhistory_index = 0;
int nr_of_new_expressions = 0;
bool b_editing_history = false;
bool fix_supsub_history = false;

bool clear_history_on_exit = false;
int max_history_lines = 300;
bool use_custom_history_font = false, save_custom_history_font = false;
string custom_history_font;
int history_expression_type = 2;

extern GtkBuilder *main_builder;

extern bool persistent_keypad;

GtkWidget *historyview = NULL;
GtkListStore *historystore = NULL;
GtkCssProvider *history_provider;

GtkCellRenderer *history_renderer, *history_index_renderer, *ans_renderer;
GtkTreeViewColumn *history_column, *history_index_column;

gchar history_error_color[8];
gchar history_warning_color[8];
gchar history_parse_color[8];
gchar history_bookmark_color[8];
GdkRGBA history_gray;

gint history_scroll_width = 16;
gint history_width_e = 0, history_width_a = 0;

MathFunction *f_answer;
MathFunction *f_expression;

#define HISTORY_IS_MESSAGE(x) (inhistory_type[x] == QALCULATE_HISTORY_MESSAGE || inhistory_type[x] == QALCULATE_HISTORY_ERROR || inhistory_type[x] == QALCULATE_HISTORY_WARNING)
#define HISTORY_IS_EXPRESSION(x) (inhistory_type[x] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[x] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[x] == QALCULATE_HISTORY_REGISTER_MOVED)
#define HISTORY_IS_PARSE(x) (inhistory_type[x] == QALCULATE_HISTORY_PARSE || inhistory_type[x] == QALCULATE_HISTORY_PARSE_APPROXIMATE || inhistory_type[x] == QALCULATE_HISTORY_PARSE_WITHEQUALS)
#define HISTORY_NOT_MESSAGE(x) (inhistory_type[x] != QALCULATE_HISTORY_MESSAGE && inhistory_type[x] != QALCULATE_HISTORY_ERROR && inhistory_type[x] != QALCULATE_HISTORY_WARNING)
#define HISTORY_NOT_EXPRESSION(x) (inhistory_type[x] != QALCULATE_HISTORY_EXPRESSION && inhistory_type[x] != QALCULATE_HISTORY_RPN_OPERATION && inhistory_type[x] != QALCULATE_HISTORY_REGISTER_MOVED)
#define HISTORY_NOT_PARSE(x) (inhistory_type[x] != QALCULATE_HISTORY_PARSE && inhistory_type[x] != QALCULATE_HISTORY_PARSE_APPROXIMATE && inhistory_type[x] != QALCULATE_HISTORY_PARSE_WITHEQUALS)
#define ITEM_IS_EXPRESSION(x) (HISTORY_IS_EXPRESSION(x) || ((size_t) x < inhistory_type.size() - 1 && HISTORY_IS_PARSE(x) && HISTORY_IS_EXPRESSION(x + 1)) || ((size_t) x < inhistory_type.size() - 2 && HISTORY_IS_MESSAGE(x) && HISTORY_IS_PARSE(x + 1) && HISTORY_IS_EXPRESSION(x + 2) && inhistory[x + 1].empty()))
#define ITEM_NOT_EXPRESSION(x) (HISTORY_NOT_EXPRESSION(x) && ((size_t) x >= inhistory_type.size() - 1 || HISTORY_NOT_PARSE(x) || HISTORY_NOT_EXPRESSION(x + 1)) && ((size_t) x >= inhistory_type.size() - 2 || HISTORY_NOT_MESSAGE(x) || HISTORY_NOT_PARSE(x + 1) || HISTORY_NOT_EXPRESSION(x + 2) || !inhistory[x + 1].empty()))

GtkWidget *history_view_widget() {
	if(!historyview) historyview = GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview"));
	return historyview;
}

DECLARE_BUILTIN_FUNCTION(AnswerFunction, 0)
DECLARE_BUILTIN_FUNCTION(ExpressionFunction, 0)

AnswerFunction::AnswerFunction() : MathFunction("answer", 1, 1, CALCULATOR->f_warning->category(), _("History Answer Value")) {
	ExpressionName name(_("answer"));
	if(name.name[0] <= 'Z' && name.name[0] >= 'A') name.name[0] += 32;
	if(name.name != "answer") addName(name, 1);
	VectorArgument *arg = new VectorArgument(_("History Index(es)"));
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO, true, true, INTEGER_TYPE_SINT));
	setArgumentDefinition(1, arg);
}
int AnswerFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].size() == 0) return 0;
	if(vargs[0].size() > 1) mstruct.clearVector();
	for(size_t i = 0; i < vargs[0].size(); i++) {
		int index = vargs[0][i].number().intValue();
		if(index < 0) index = (int) history_answer.size() + 1 + index;
		if(index <= 0 || index > (int) history_answer.size() || history_answer[(size_t) index - 1] == NULL) {
			CALCULATOR->error(true, _("History index %s does not exist."), vargs[0][i].print().c_str(), NULL);
			if(vargs[0].size() == 1) mstruct.setUndefined();
			else mstruct.addChild(m_undefined);
		} else {
			if(vargs[0].size() == 1) mstruct.set(*history_answer[(size_t) index - 1]);
			else mstruct.addChild(*history_answer[(size_t) index - 1]);
		}
	}
	return 1;
}
MathFunction *answer_function() {return f_answer;}
ExpressionFunction::ExpressionFunction() : MathFunction("expression", 1, 1, CALCULATOR->f_warning->category(), _("History Parsed Expression")) {
	ExpressionName name(_("expression"));
	if(name.name[0] <= 'Z' && name.name[0] >= 'A') name.name[0] += 32;
	if(name.name != "expression") addName(name, 1);
	VectorArgument *arg = new VectorArgument(_("History Index(es)"));
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO, true, true, INTEGER_TYPE_SINT));
	setArgumentDefinition(1, arg);
}
int ExpressionFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].size() == 0) return 0;
	if(vargs[0].size() > 1) mstruct.clearVector();
	for(size_t i = 0; i < vargs[0].size(); i++) {
		int index = vargs[0][i].number().intValue();
		if(index < 0) index = (int) history_parsed.size() + 1 + index;
		if(index <= 0 || index > (int) history_parsed.size() || history_parsed[(size_t) index - 1] == NULL) {
			CALCULATOR->error(true, _("History index %s does not exist."), vargs[0][i].print().c_str(), NULL);
			if(vargs[0].size() == 1) mstruct.setUndefined();
			else mstruct.addChild(m_undefined);
		} else {
			if(vargs[0].size() == 1) mstruct.set(*history_parsed[(size_t) index - 1]);
			else mstruct.addChild(*history_parsed[(size_t) index - 1]);
		}
	}
	return 1;
}
MathFunction *expression_function() {return f_expression;}

bool read_history_settings_line(string &svar, string &svalue, int &v) {
	if(svar == "clear_history_on_exit") {
		clear_history_on_exit = v;
	} else if(svar == "max_history_lines") {
		max_history_lines = v;
	} else if(svar == "history_expression_type") {
		history_expression_type = v;
	} else if(svar == "use_custom_history_font") {
		use_custom_history_font = v;
	} else if(svar == "custom_history_font") {
		custom_history_font = svalue;
		save_custom_history_font = true;
	} else {
		return false;
	}
	return true;
}
void write_history_settings(FILE *file) {
	fprintf(file, "clear_history_on_exit=%i\n", clear_history_on_exit);
	if(max_history_lines != 300) fprintf(file, "max_history_lines=%i\n", max_history_lines);
	fprintf(file, "history_expression_type=%i\n", history_expression_type);
	fprintf(file, "use_custom_history_font=%i\n", use_custom_history_font);
	if(use_custom_history_font || save_custom_history_font) fprintf(file, "custom_history_font=%s\n", custom_history_font.c_str());
}

size_t pref_bookmark_index = 0;
time_t pref_history_time = 0;
int old_history_format = -1;
bool read_history_line(string &svar, string &svalue) {
	if(old_history_format < 0) old_history_format = (version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 4)));
	if(svar == "history") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_OLD);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_old") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_OLD);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_expression") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_EXPRESSION);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_expression*") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_EXPRESSION);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(true);
		inhistory_value.push_front(0);
	} else if(svar == "history_transformation") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_TRANSFORMATION);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_result") {
		if(VERSION_BEFORE(4, 1, 1) && svalue.length() > 20 && svalue.substr(svalue.length() - 4, 4) == " …" && (unsigned char) svalue[svalue.length() - 5] >= 0xC0) svalue.erase(svalue.length() - 5, 1);
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_RESULT);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_result_approximate") {
		if(VERSION_BEFORE(4, 1, 1) && svalue.length() > 20 && svalue.substr(svalue.length() - 4, 4) == " …" && (unsigned char) svalue[svalue.length() - 5] >= 0xC0) svalue.erase(svalue.length() - 5, 1);
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_RESULT_APPROXIMATE);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_parse") {
		inhistory.push_front(svalue);
		if(old_history_format) inhistory_type.push_front(QALCULATE_HISTORY_PARSE_WITHEQUALS);
		else inhistory_type.push_front(QALCULATE_HISTORY_PARSE);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_parse_withequals") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_PARSE_WITHEQUALS);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_parse_approximate") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_PARSE_APPROXIMATE);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_register_moved") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_REGISTER_MOVED);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_rpn_operation") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_RPN_OPERATION);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_register_moved*") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_REGISTER_MOVED);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(true);
		inhistory_value.push_front(0);
	} else if(svar == "history_rpn_operation*") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_RPN_OPERATION);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(true);
		inhistory_value.push_front(0);
	} else if(svar == "history_warning") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_WARNING);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_message") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_MESSAGE);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_error") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_ERROR);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
	} else if(svar == "history_bookmark") {
		inhistory.push_front(svalue);
		inhistory_type.push_front(QALCULATE_HISTORY_BOOKMARK);
		inhistory_time.push_front(pref_history_time);
		inhistory_protected.push_front(false);
		inhistory_value.push_front(0);
		bool b = false;
		pref_bookmark_index = 0;
		for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
			if(string_is_less(svalue, *it)) {
				history_bookmarks.insert(it, svalue);
				b = true;
				break;
			}
			pref_bookmark_index++;
		}
		if(!b) history_bookmarks.push_back(svalue);
	} else if(svar == "history_continued") {
		if(inhistory.size() > 0) {
			inhistory[0] += "\n";
			inhistory[0] += svalue;
			if(inhistory_type[0] == QALCULATE_HISTORY_BOOKMARK) {
				history_bookmarks[pref_bookmark_index] += "\n";
				history_bookmarks[pref_bookmark_index] += svalue;
			}
		}
	} else if(svar == "history_time") {
		pref_history_time = (time_t) strtoll(svalue.c_str(), NULL, 10);
	} else {
		return false;
	}
	return true;
}

void write_history(FILE *file) {
	int lines = max_history_lines;
	bool end_after_result = false, end_before_expression = false;
	bool is_protected = false;
	bool is_bookmarked = false;
	bool doend = false;
	size_t hi = inhistory.size();
	time_t history_time_prev = 0;
	while(!doend && hi > 0) {
		hi--;
		if(inhistory_time[hi] != history_time_prev) {
			history_time_prev = inhistory_time[hi];
			fprintf(file, "history_time=%lli\n", (long long int) history_time_prev);
		}
		switch(inhistory_type[hi]) {
			case QALCULATE_HISTORY_EXPRESSION: {
				if(end_before_expression) {
					doend = true;
				} else {
					if(inhistory_protected[hi]) fprintf(file, "history_expression*=");
					else if(!clear_history_on_exit || is_bookmarked) fprintf(file, "history_expression=");
					is_protected = inhistory_protected[hi] || is_bookmarked;
					is_bookmarked = false;
				}
				break;
			}
			case QALCULATE_HISTORY_TRANSFORMATION: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_transformation=");
				break;
			}
			case QALCULATE_HISTORY_RESULT: {
				if(end_after_result) doend = true;
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_result=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_RESULT_APPROXIMATE: {
				if(end_after_result) doend = true;
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_result_approximate=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_PARSE: {}
			case QALCULATE_HISTORY_PARSE_WITHEQUALS: {}
			case QALCULATE_HISTORY_PARSE_APPROXIMATE: {
				if(clear_history_on_exit && !is_protected) break;
				if(inhistory_type[hi] == QALCULATE_HISTORY_PARSE) fprintf(file, "history_parse=");
				else if(inhistory_type[hi] == QALCULATE_HISTORY_PARSE_WITHEQUALS) fprintf(file, "history_parse_withequals=");
				else fprintf(file, "history_parse_approximate=");
				lines--;
				if(lines < 0) {
					if(hi + 1 < inhistory_protected.size() && inhistory_protected[hi + 1]) {
						end_before_expression = true;
					} else if(hi + 2 < inhistory_type.size() && inhistory_type[hi + 2] == QALCULATE_HISTORY_BOOKMARK) {
						end_before_expression = true;
					}
					if(!end_before_expression) end_after_result = true;
				}
				break;
			}
			case QALCULATE_HISTORY_REGISTER_MOVED: {
				if(end_before_expression) {
					doend = true;
				} else {
					if(inhistory_protected[hi]) fprintf(file, "history_register_moved*=");
					else if(!clear_history_on_exit || is_bookmarked) fprintf(file, "history_register_moved=");
					is_protected = inhistory_protected[hi] || is_bookmarked;
					is_bookmarked = false;
				}
				break;
			}
			case QALCULATE_HISTORY_RPN_OPERATION: {
				if(end_before_expression) {
					doend = true;
				} else {
					if(inhistory_protected[hi]) fprintf(file, "history_rpn_operation*=");
					else if(!clear_history_on_exit || is_bookmarked) fprintf(file, "history_rpn_operation=");
					is_protected = inhistory_protected[hi] || is_bookmarked;
					is_bookmarked = false;
				}
				break;
			}
			case QALCULATE_HISTORY_WARNING: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_warning=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_MESSAGE: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_message=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_ERROR: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_error=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_BOOKMARK: {
				if(end_before_expression && hi > 0 && (inhistory_type[hi - 1] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[hi - 1] == QALCULATE_HISTORY_REGISTER_MOVED || inhistory_type[hi - 1] == QALCULATE_HISTORY_RPN_OPERATION)) {
					doend = true;
				} else {
					fprintf(file, "history_bookmark=");
					is_bookmarked = true;
					is_protected = true;
					lines--;
					break;
				}
			}
			case QALCULATE_HISTORY_OLD: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_old=");
				lines--;
				if(lines < 0) doend = true;
				break;
			}
		}
		if(doend && end_before_expression) break;
		if(!clear_history_on_exit || is_protected) {
			size_t i3 = inhistory[hi].find('\n');
			if(i3 == string::npos) {
				size_t l = unformatted_length(inhistory[hi]);
				if(!is_protected && l > 5000) {
					string str = unhtmlize(inhistory[hi]);
					int index = 50;
					size_t i3 = str.find("\n", 40);
					if(i3 != string::npos && i3 < (size_t) index + 1) {
						fprintf(file,  "%s<br>…\n", fix_history_string(str.substr(0, i3)).c_str());
					} else {
						if(i3 != string::npos) gsub("\n", "<br>", str);
						while(index >= 0 && (signed char) str[index] < 0 && (unsigned char) str[index + 1] < 0xC0) index--;
						if(is_not_in(NUMBERS, str[index]) || is_not_in(NUMBERS, str[index + 1])) {
							str[index + 1] = ' ';
							index++;
						}
						fprintf(file,  "%s…\n", fix_history_string(str.substr(0, index + 1)).c_str());
					}
				} else {
					fprintf(file, "%s\n", inhistory[hi].c_str());
					if(l > 300) {
						if(l > 9000) lines -= 30;
						else lines -= l / 300;
					}
				}
			} else {
				fprintf(file, "%s\n", inhistory[hi].substr(0, i3).c_str());
				i3++;
				size_t i2 = inhistory[hi].find('\n', i3);
				while(i2 != string::npos) {
					fprintf(file, "history_continued=%s\n", inhistory[hi].substr(i3, i2 - i3).c_str());
					lines--;
					i3 = i2 + 1;
					i2 = inhistory[hi].find('\n', i3);
				}
				fprintf(file, "history_continued=%s\n", inhistory[hi].substr(i3, inhistory[hi].length() - i3).c_str());
				lines--;
			}
		}
	}
	while(hi >= 0 && inhistory.size() > 0) {
		if(inhistory_protected[hi] || (inhistory_type[hi] == QALCULATE_HISTORY_BOOKMARK && hi != 0 && inhistory_type[hi - 1] != QALCULATE_HISTORY_OLD)) {
			bool b_first = true;
			if(inhistory_time[hi] != history_time_prev) {
				history_time_prev = inhistory_time[hi];
				fprintf(file, "history_time=%lli\n", (long long int) history_time_prev);
			}
			while(hi >= 0) {
				bool do_end = false;
				switch(inhistory_type[hi]) {
					case QALCULATE_HISTORY_EXPRESSION: {
						if(!b_first) {
							do_end = true;
						} else {
							if(inhistory_protected[hi]) fprintf(file, "history_expression*=");
							else fprintf(file, "history_expression=");
							b_first = false;
						}
						break;
					}
					case QALCULATE_HISTORY_TRANSFORMATION: {
						fprintf(file, "history_transformation=");
						break;
					}
					case QALCULATE_HISTORY_RESULT: {
						fprintf(file, "history_result=");
						break;
					}
					case QALCULATE_HISTORY_RESULT_APPROXIMATE: {
						fprintf(file, "history_result_approximate=");
						break;
					}
					case QALCULATE_HISTORY_PARSE: {
						fprintf(file, "history_parse=");
						break;
					}
					case QALCULATE_HISTORY_PARSE_WITHEQUALS: {
						fprintf(file, "history_parse_withequals=");
						break;
					}
					case QALCULATE_HISTORY_PARSE_APPROXIMATE: {
						fprintf(file, "history_parse_approximate=");
						break;
					}
					case QALCULATE_HISTORY_REGISTER_MOVED: {
						if(!b_first) {
							do_end = true;
						} else {
							if(inhistory_protected[hi]) fprintf(file, "history_register_moved*=");
							else fprintf(file, "history_register_moved=");
							b_first = false;
						}
						break;
					}
					case QALCULATE_HISTORY_RPN_OPERATION: {
						if(!b_first) {
							do_end = true;
						} else {
							if(inhistory_protected[hi]) fprintf(file, "history_rpn_operation*=");
							else fprintf(file, "history_rpn_operation=");
							b_first = false;
						}
						break;
					}
					case QALCULATE_HISTORY_WARNING: {
						fprintf(file, "history_warning=");
						break;
					}
					case QALCULATE_HISTORY_MESSAGE: {
						fprintf(file, "history_message=");
						break;
					}
					case QALCULATE_HISTORY_ERROR: {
						fprintf(file, "history_error=");
						break;
					}
					case QALCULATE_HISTORY_BOOKMARK: {
						if(!b_first) {
							do_end = true;
							break;
						}
						fprintf(file, "history_bookmark=");
						break;
					}
					case QALCULATE_HISTORY_OLD: {
						do_end = true;
						break;
					}
				}
				if(do_end) {
					hi++;
					break;
				}
				size_t i3 = inhistory[hi].find('\n');
				if(i3 == string::npos) {
					fprintf(file, "%s\n", inhistory[hi].c_str());
				} else {
					fprintf(file, "%s\n", inhistory[hi].substr(0, i3).c_str());
					i3++;
					size_t i2 = inhistory[hi].find('\n', i3);
					while(i2 != string::npos) {
						fprintf(file, "history_continued=%s\n", inhistory[hi].substr(i3, i2 - i3).c_str());
						i3 = i2 + 1;
						i2 = inhistory[hi].find('\n', i3);
					}
					fprintf(file, "history_continued=%s\n", inhistory[hi].substr(i3, inhistory[hi].length() - i3).c_str());
				}
				if(hi == 0) break;
				hi--;
			}
			if(hi > inhistory_type.size()) break;
		}
		if(hi == 0) break;
		hi--;
	}
}

void add_line_breaks(string &str, int expr = false, size_t first_i = 0);

string ellipsize_result(const string &result_text, size_t length) {
	length /= 2;
	size_t index1 = result_text.find(SPACE, length);
	if(index1 == string::npos || index1 > length * 1.2) {
		index1 = result_text.find(THIN_SPACE, length);
		size_t index1b = result_text.find(NNBSP, length);
		if(index1b != string::npos && (index1 == string::npos || index1b < index1)) index1 = index1b;
	}
	if(index1 == string::npos || index1 > length * 1.2) {
		index1 = length;
		while(index1 > 0 && (signed char) result_text[index1] < 0 && (unsigned char) result_text[index1 + 1] < 0xC0) index1--;
	}
	size_t index2 = result_text.find(SPACE, result_text.length() - length);
	if(index2 == string::npos || index2 > result_text.length() - length * 0.8) {
		index2 = result_text.find(THIN_SPACE, result_text.length() - length);
		size_t index2b = result_text.find(NNBSP, result_text.length() - length);
		if(index2b != string::npos && (index2 == string::npos || index2b < index2)) index2 = index2b;
	}
	if(index2 == string::npos || index2 > result_text.length() - length * 0.8) {
		index2 = result_text.length() - length;
		while(index2 > index1 && (signed char) result_text[index2] < 0 && (unsigned char) result_text[index2 + 1] < 0xC0) index2--;
	}
	return result_text.substr(0, index1) + " (…) " + result_text.substr(index2, result_text.length() - index2);
}
string fix_history_string_new(const string &str2) {
	string str = str2;
	gsub("<sub class=\"nous\">", "<sub>", str);
	gsub("<i class=\"symbol\">", "<i>", str);
	return str;
}
void fix_history_string_new2(string &str) {
	gsub("<sub class=\"nous\">", "<sub>", str);
	gsub("<i class=\"symbol\">", "<i>", str);
}

void fix_history_string2(string &str) {
	gsub("&", "&amp;", str);
	gsub(">", "&gt;", str);
	gsub("<", "&lt;", str);
}
string fix_history_string(const string &str2) {
	string str = str2;
	gsub("&", "&amp;", str);
	gsub(">", "&gt;", str);
	gsub("<", "&lt;", str);
	return str;
}
void unfix_history_string(string &str) {
	gsub("&amp;", "&", str);
	gsub("&gt;", ">", str);
	gsub("&lt;", "<", str);
}
void improve_result_text(string &resstr) {
	size_t i1 = 0, i2 = 0, i3 = 0, i_prev = 0;
	size_t i_equals = resstr.find(_("approx.")) + strlen(_("approx."));
	while(i_prev + 2 < resstr.length()) {
		i1 = resstr.find_first_of("\"\'", i_prev);
		if(i1 == string::npos) break;
		i2 = resstr.find(resstr[i1], i1 + 1);
		if(i2 == string::npos) break;
		if(i2 - i1 > 2) {
			if(!text_length_is_one(resstr.substr(i1 + 1, i2 - i1 - 1))) {
				i_prev = i2 + 1;
				continue;
			}
		}

		if(i1 > 1 && resstr[i1 - 1] == ' ' && (i_equals == string::npos || i1 != i_equals + 1) && (is_in(NUMBERS, resstr[i1 - 2]) || i1 == i_prev + 1)) {
			if((signed char) resstr[i1 - 2] < 0) {
				i3 = i1 - 2;
				while(i3 > 0 && (signed char) resstr[i3] < 0 && (unsigned char) resstr[i3] < 0xC0) i3--;
				string str = resstr.substr(i3, i1 - i3 - 1);
				if(str != SIGN_DIVISION && str != SIGN_DIVISION_SLASH && str != SIGN_MULTIPLICATION && str != SIGN_MULTIDOT && str != SIGN_SMALLCIRCLE && str != SIGN_MULTIBULLET && str != SIGN_MINUS && str != SIGN_PLUS && str != SIGN_NOT_EQUAL && str != SIGN_GREATER_OR_EQUAL && str != SIGN_LESS_OR_EQUAL && str != SIGN_ALMOST_EQUAL && str != printops.comma()) {
					resstr.replace(i1 - 1, 2, "<i>");
					if(i_equals != string::npos && i1 < i_equals) i_equals += 1;
					i2 += 1;
				} else {
					resstr.replace(i1, 1, "<i>");
					if(i_equals != string::npos && i1 < i_equals) i_equals += 2;
					i2 += 2;
				}
			} else {
				resstr.replace(i1 - 1, 2, "<i>");
				if(i_equals != string::npos && i1 < i_equals) i_equals += 1;
				i2 += 1;
			}
		} else {
			resstr.replace(i1, 1, "<i>");
			if(i_equals != string::npos && i1 < i_equals) i_equals += 2;
			i2 += 2;
		}
		resstr.replace(i2, 1, "</i>");
		if(i_equals != string::npos && i1 < i_equals) i_equals += 3;
		i_prev = i2 + 4;
	}
	i1 = 1;
	while(i1 < resstr.length()) {
		i1 = resstr.find('_', i1);
		if(i1 == string::npos || i1 + 1 == resstr.length()) break;
		if(is_not_in(NOT_IN_NAMES, resstr[i1 + 1])) {
			i2 = resstr.find_last_of(NOT_IN_NAMES, i1 - 1);
			i3 = resstr.find_first_of(NOT_IN_NAMES, i1 + 1);
			if(i2 == string::npos) i2 = 0;
			else i2 = i2 + 1;
			if(i3 == string::npos) i3 = resstr.length();
			ExpressionItem *item = CALCULATOR->getActiveExpressionItem(resstr.substr(i2, i3 - i2));
			if(item) {
				size_t index = item->hasName(resstr.substr(i2, i3 - i2), true);
				if(index > 0 && item->getName(index).suffix) {
					resstr.replace(i2, i3 - i2, sub_suffix_html(resstr.substr(i2, i3 - i2)));
					i1 = i3 + 10;
				} else {
					i1 = i3 - 1;
				}
			}
		}
		i1++;
	}
}
gboolean history_row_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer) {
	gint hindex = -1;
	gtk_tree_model_get(model, iter, 1, &hindex, -1);
	return hindex < 0;
}
string history_display_errors(bool add_to_history, GtkWindow *win, int type, bool *implicit_warning, time_t history_time, int *mtype_highest_p) {
	if(!CALCULATOR->message() && (type != 1 || !inhistory_index || !CALCULATOR->exchangeRatesUsed())) return "";
	string str = "";
	MessageType mtype, mtype_highest = MESSAGE_INFORMATION;
	int index = 0;
	GtkTreeIter history_iter;
	int inhistory_added = 0;
	if(add_to_history && type != 1) inhistory_index = current_inhistory_index;
	while(CALCULATOR->message()) {
		if(CALCULATOR->message()->category() == MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION && (implicit_question_asked || implicit_warning)) {
			if(!implicit_question_asked) *implicit_warning = true;
		} else {
			mtype = CALCULATOR->message()->type();
			if(mtype == MESSAGE_INFORMATION && (type == 1 || type == 2) && win && CALCULATOR->message()->message().find("-------------------------------------\n") == 0) {
				GtkWidget *edialog = gtk_message_dialog_new(win,GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", CALCULATOR->message()->message().c_str());
				if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
				gtk_dialog_run(GTK_DIALOG(edialog));
				gtk_widget_destroy(edialog);
			} else {
				if(index > 0) {
					if(index == 1) str = "• " + str;
					str += "\n• ";
				}
				if(win != NULL && is_plot_dialog(win) && CALCULATOR->message()->message() == _("It took too long to generate the plot data.")) str += _("It took too long to generate the plot data. Please decrease the sampling rate or increase the time limit in preferences.");
				else str += CALCULATOR->message()->message();
				if(mtype == MESSAGE_ERROR || (mtype_highest != MESSAGE_ERROR && mtype == MESSAGE_WARNING)) {
					mtype_highest = mtype;
				}
				if(add_to_history && inhistory_index >= 0) {
					if(history_time == 0) history_time = time(NULL);
					if(mtype == MESSAGE_ERROR) {
						inhistory.insert(inhistory.begin() + inhistory_index, CALCULATOR->message()->message());
						inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_ERROR);
						inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
						inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
						inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
						string history_message = "- ";
						history_message += CALCULATOR->message()->message();
						fix_history_string2(history_message);
						add_line_breaks(history_message, false, 4);
						string history_str;
						if(pango_version() >= 15000) history_str = "<span font_size=\"90%\" foreground=\"";
						else history_str = "<span foreground=\"";
						history_str += history_error_color;
						history_str += "\">";
						history_str += history_message;
						history_str += "</span>";
						history_index++;
						gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
					} else if(mtype == MESSAGE_WARNING) {
						inhistory.insert(inhistory.begin() + inhistory_index, CALCULATOR->message()->message());
						inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_WARNING);
						inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
						inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
						inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
						string history_message = "- ";
						history_message += CALCULATOR->message()->message();
						fix_history_string2(history_message);
						add_line_breaks(history_message, false, 4);
						string history_str;
						if(pango_version() >= 15000) history_str = "<span font_size=\"90%\" foreground=\"";
						else history_str = "<span foreground=\"";
						history_str += history_warning_color;
						history_str += "\">";
						history_str += history_message;
						history_str += "</span>";
						history_index++;
						gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
					} else {
						inhistory.insert(inhistory.begin() + inhistory_index, CALCULATOR->message()->message());
						inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_MESSAGE);
						inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
						inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
						inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
						string history_message = "- ";
						history_message += CALCULATOR->message()->message();
						fix_history_string2(history_message);
						add_line_breaks(history_message, false, 4);
						string history_str;
						if(pango_version() >= 15000) {
							history_str = "<span font_size=\"90%\"><i>";
							history_str += history_message;
							history_str += "</i></span>";
						} else {
							history_str = "<i>";
							history_str += history_message;
							history_str += "</i>";
						}
						history_index++;
						gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
					}
					inhistory_added++;
				}
			}
			index++;
		}
		CALCULATOR->nextMessage();
	}
	if(inhistory_added > 0) {
		GtkTreeIter index_iter = history_iter;
		gint index_hi = -1;
		gint hi_add = 1;
		while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &index_iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &index_iter, 1, &index_hi, -1);
			if(index_hi >= 0) {
				gtk_list_store_set(historystore, &index_iter, 1, index_hi + hi_add, -1);
				if(inhistory_added > 1) {
					inhistory_added--;
					hi_add++;
				}
			}
		}
	}
	if(mtype_highest_p) *mtype_highest_p = mtype_highest;
	if(add_to_history && type != 1) current_inhistory_index = inhistory_index;
	return str;
}
void add_line_breaks(string &str, int expr, size_t first_i) {
	int history_width = (expr == 2 ? history_width_a : history_width_e);
	if(history_width == 0) return;
	string str_bak;
	bool markup = str.find('<') != string::npos;
	if(markup) str_bak = str;
	PangoLayout *layout = gtk_widget_create_pango_layout(history_view_widget(), "");
	PangoFontDescription *font_desc = NULL;
	if(expr == 3 || expr == 2 || expr == 4) {
		gtk_style_context_get(gtk_widget_get_style_context(history_view_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
		gint size = pango_font_description_get_size(font_desc);
		if(expr == 3) pango_font_description_set_style(font_desc, PANGO_STYLE_ITALIC);
		if(pango_version() >= 15000) {
			if(expr == 4) size *= 0.9;
			else if(expr == 2) size *= 1.1;
		} else {
			if(expr != 4) size *= 1.2;
		}
		if(pango_font_description_get_size_is_absolute(font_desc)) pango_font_description_set_absolute_size(font_desc, size);
		else pango_font_description_set_size(font_desc, size);
		pango_layout_set_font_description(layout, font_desc);
	}
	int r = 1;
	size_t i_row = 0;
	size_t indent = 0;
	size_t lb_point = string::npos;
	size_t c = 0;
	int b_or = 0;
	if(expr > 1 && str.find("||") != string::npos) b_or = 2;
	else if(expr > 1 && str.find(_("or")) != string::npos) b_or = 1;
	for(size_t i = first_i; i < str.length(); i++) {
		if(r != 1 && i - i_row <= indent) {
			if(str[i] == ' ') {
				str.erase(i, 1);
				if(i >= str.length()) i = str.length() - 1;
			} else if((signed char) str[i] == -30 && i + 2 < str.length() && (signed char) str[i + 1] == -128 && ((signed char) str[i + 2] == -119 || (signed char) str[i + 2] == -81)) {
				str.erase(i, 3);
				if(i >= str.length()) i = str.length() - 1;
			} else if((signed char) str[i] == -62 && i + 1 < str.length() && (signed char) str[i + 1] == -96) {
				str.erase(i, 2);
				if(i >= str.length()) i = str.length() - 1;
			}
		}
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0 || i == str.length() - 1) {
			if(str[i] == '\n') {
				r++;
				i_row = i + 1;
				lb_point = string::npos;
			} else if(str[i] == '<') {
				size_t i2 = str.find('>', i + 1);
				if(i2 != string::npos) {
					size_t i3 = str.find(str.substr(i + 1, i2 - i - 1), i2 + 1);
					if(i3 == string::npos) break;
					c += i3 - i2 - 1;
					i = i3 + (i2 - i - 1) - 1;
				}
				if(i != string::npos && i + 1 == str.length()) goto last_linebreak_test;
			} else if(str[i] == '&') {
				i = str.find(';', i + 1);
				if(i != string::npos && i + 1 == str.length()) goto last_linebreak_test;
				c++;
			} else {
				if(i - i_row > indent) {
					if(is_in(" \t", str[i]) && i + 1 < str.length() && (is_not_in("0123456789", str[i + 1]) || is_not_in("0123456789", str[i - 1]))) {
						if(b_or == 1 && str.length() > i + strlen("or") + 2 && str.substr(i + 1, strlen(_("or"))) == _("or") && str[i + strlen(_("or")) + 1] == ' ') {
							i = i + strlen(_("or")) + 1;
							str[i] = '\n';
							i_row = i + 1;
							lb_point = string::npos;
							c = 0;
						} else if(b_or == 2 && str.length() > i + 2 + 2 && str.substr(i + 1, 2) == "||" && str[i + 2 + 1] == ' ') {
							i = i + 2 + 1;
							str[i] = '\n';
							i_row = i + 1;
							lb_point = string::npos;
							c = 0;
						} else if(c > 10) {
							string teststr = str.substr(i_row, i - i_row);
							pango_layout_set_markup(layout, teststr.c_str(), -1);
							gint w = 0;
							pango_layout_get_pixel_size(layout, &w, NULL);
							if(w > history_width) {
								bool cbreak = lb_point == string::npos;
								if(!cbreak && expr) {
									teststr = str.substr(i_row, lb_point - i_row);
									pango_layout_set_markup(layout, teststr.c_str(), -1);
									pango_layout_get_pixel_size(layout, &w, NULL);
									cbreak = (w > history_width || w < history_width / 3);
									if(w <= history_width) teststr = str.substr(i_row, i - i_row);
								}
								if(cbreak) {
									size_t i_ts = 0;
									size_t i_nnbsp = 0;
									while(true) {
										if(i_ts != string::npos) i_ts = teststr.rfind(THIN_SPACE);
										if(i_nnbsp != string::npos) i_nnbsp = teststr.rfind(NNBSP);
										size_t i3 = i_ts;
										if(i_nnbsp > 0 && i_nnbsp != string::npos && (i3 == string::npos || i_nnbsp > i3)) {
											i3 = i_nnbsp;
										}
										if(i3 != string::npos && i3 != 0) {
											size_t i2 = teststr.find("</", i3);
											if(i2 != string::npos && teststr.find('<', i3) == i2) {
												i3 = teststr.find(">", i2);
												if(i3 != string::npos) {
													i2 = teststr.rfind(teststr.substr(i2 + 2, i3 - i2 - 1), i2 - 2);
													if(i2 != string::npos) {
														if(i2 == 0) {
															i = i_row + 1;
														} else {
															i2--;
															i -= teststr.length() - i2;
															teststr.erase(i2 + 1, teststr.length() - i2 - 1);
														}
													}
												}
											} else if(i3 + 3 < teststr.length()) {
												i3 += 3;
												i -= (teststr.length() - i3);
												teststr.erase(i3, teststr.length() - i3);
											}
										}
										while((signed char) teststr[teststr.length() - 1] <= 0 && (unsigned char) teststr[teststr.length() - 1] < 0xC0) {
											i--;
											teststr.erase(teststr.length() - 1, 1);
											if(i < i_row) {
												g_object_unref(layout);
												if(font_desc) pango_font_description_free(font_desc);
												return;
											}
										}
										if(teststr[teststr.length() - 1] == '>') {
											size_t i2 = teststr.rfind('/', teststr.length() - 2);
											if(i2 != string::npos) {
												i2 = teststr.rfind(teststr.substr(i2 + 1), i2 - 1);
												if(i2 != string::npos) {
													i2--;
													i -= teststr.length() - i2;
													teststr.erase(i2 + 1, teststr.length() - i2 - 1);
												}
											}
										} else if(teststr[teststr.length() - 1] == ';') {
											size_t i2 = teststr.rfind('&');
											if(i2 != string::npos && teststr.find(';', i2 + 1) == teststr.length() - 1) {
												i -= teststr.length() - i2;
												teststr.erase(i2 + 1, teststr.length() - i2 - 1);
											}
										}
										i--;
										teststr.erase(teststr.length() - 1, 1);
										if(i <= i_row) {
											g_object_unref(layout);
											if(font_desc) pango_font_description_free(font_desc);
											if(i < i_row && markup) {
												str = unhtmlize(str_bak);
												fix_history_string2(str);
												add_line_breaks(str, expr, first_i);
											}
											return;
										}
										pango_layout_set_markup(layout, teststr.c_str(), -1);
										pango_layout_get_pixel_size(layout, &w, NULL);
										if(w <= history_width) {
											i++;
											if(str[i - 1] == ' ') {
												i--;
											} else if((signed char) str[i - 1] == -30 && i + 1 < str.length() && (signed char) str[i] == -128 && ((signed char) str[i + 1] == -119 || (signed char) str[i + 1] == -81)) {
												i--;
											} else if(i > 3 && ((signed char) str[i - 1] == -119 || (signed char) str[i - 1] == -81) && (signed char) str[i - 2] == -128 && (signed char) str[i - 3] == -30) {
												i -= 3;
											} else if(i > 3 && str[i] <= '9' && str[i] >= '0' && str[i - 1] <= '9' && str[i - 1] >= '0') {
												if(str[i - 2] == ' ' && str[i - 3] <= '9' && str[i - 3] >= '0') i -= 2;
												else if(str[i - 3] == ' ' && str[i - 4] <= '9' && str[i - 4] >= '0') i -= 3;
												else if((str[i - 2] == '.' || str[i - 2] == ',') && str[i - 3] <= '9' && str[i - 3] >= '0') i--;
												else if((str[i - 3] == '.' || str[i - 3] == ',') && str[i - 4] <= '9' && str[i - 4] >= '0') i -= 2;
											} else if(i > 4 && (str[i] == '.' || str[i] == ',') && str[i - 1] <= '9' && str[i - 1] >= '0' && str[i - 4] == str[i] && str[i - 5] <= '9' && str[i - 5] >= '0') {
												i -= 3;
											}
											str.insert(i, "\n");
											i_row = i + 1;
											r++;
											lb_point = string::npos;
											break;
										}
									}
								} else {
									str[lb_point] = '\n';
									i = lb_point;
									i_row = i + 1;
									r++;
									lb_point = string::npos;
								}
								c = 0;
							} else {
								lb_point = i;
								c++;
							}
						}
					} else if(i + 1 == str.length() || (c >= 50 && c % 50 == 0)) {
						last_linebreak_test:
						string teststr;
						if((signed char) str[i] <= 0) {
							while(i + 1 < str.length() && (signed char) str[i + 1] <= 0 && (unsigned char) str[i + 1] < 0xC0) i++;
						}
						if(i + 1 == str.length()) teststr = str.substr(i_row);
						else teststr = str.substr(i_row, i - i_row + 1);
						pango_layout_set_markup(layout, teststr.c_str(), -1);
						gint w = 0;
						pango_layout_get_pixel_size(layout, &w, NULL);
						if(w > history_width) {
							bool cbreak = lb_point == string::npos;
							if(!cbreak && expr) {
								teststr = str.substr(i_row, lb_point - i_row);
								pango_layout_set_markup(layout, teststr.c_str(), -1);
								pango_layout_get_pixel_size(layout, &w, NULL);
								cbreak = (w > history_width || w < history_width / 3);
								if(w <= history_width) {
									if(i + 1 == str.length()) teststr = str.substr(i_row);
									else teststr = str.substr(i_row, i - i_row + 1);
								}
							}
							if(cbreak) {
								size_t i_ts = 0;
								size_t i_nnbsp = 0;
								while(true) {
									if(i_ts != string::npos) i_ts = teststr.rfind(THIN_SPACE);
									if(i_nnbsp != string::npos) i_nnbsp = teststr.rfind(NNBSP);
									size_t i3 = i_ts;
									if(i_nnbsp > 0 && i_nnbsp != string::npos && (i3 == string::npos || i_nnbsp > i3)) {
										i3 = i_nnbsp;
									}
									if(i3 != string::npos && i3 != 0) {
										size_t i2 = teststr.find("</", i3);
										if(i2 != string::npos && teststr.find('<', i3) == i2) {
											i3 = teststr.find(">", i2);
											if(i3 != string::npos) {
												i2 = teststr.rfind(teststr.substr(i2 + 2, i3 - i2 - 1), i2 - 2);
												if(i2 != string::npos) {
													if(i2 == 0) {
														i = i_row + 1;
													} else {
														i2--;
														i -= teststr.length() - i2;
														teststr.erase(i2 + 1, teststr.length() - i2 - 1);
													}
												}
											}
										} else if(i3 + 3 < teststr.length()) {
											i3 += 3;
											i -= (teststr.length() - i3);
											teststr.erase(i3, teststr.length() - i3);
										}
									}
									while((signed char) teststr[teststr.length() - 1] <= 0 && (unsigned char) teststr[teststr.length() - 1] < 0xC0) {
										i--;
										teststr.erase(teststr.length() - 1, 1);
										if(i < i_row) {
											g_object_unref(layout);
											if(font_desc) pango_font_description_free(font_desc);
											return;
										}
									}
									if(teststr[teststr.length() - 1] == '>') {
										size_t i2 = teststr.rfind("<", teststr.length() - 2);
										if(i2 != string::npos && teststr[i2 + 1] == '/') {
											i2 = teststr.rfind(teststr.substr(i2 + 2), i2 - 2);
										}
										if(i2 != string::npos) {
											if(i2 == 0) {
												i = i_row + 1;
											} else {
												i2--;
												i -= teststr.length() - i2;
												teststr.erase(i2 + 1, teststr.length() - i2 - 1);
											}
										}
									} else if(teststr[teststr.length() - 1] == ';') {
										size_t i2 = teststr.rfind('&');
										if(i2 != string::npos && teststr.find(';', i2 + 1) == teststr.length() - 1) {
											i -= teststr.length() - i2;
											teststr.erase(i2 + 1, teststr.length() - i2 - 1);
										}
									}
									i--;
									teststr.erase(teststr.length() - 1, 1);
									if(i <= i_row) {
										g_object_unref(layout);
										if(font_desc) pango_font_description_free(font_desc);
										if(i < i_row && markup) {
											str = unhtmlize(str_bak);
											fix_history_string2(str);
											add_line_breaks(str, expr, first_i);
										}
										return;
									}
									pango_layout_set_markup(layout, teststr.c_str(), -1);
									pango_layout_get_pixel_size(layout, &w, NULL);
									if(w <= history_width) {
										i++;
										if(str[i - 1] == ' ') {
											i--;
										} else if((signed char) str[i - 1] == -30 && i + 1 < str.length() && (signed char) str[i] == -128 && ((signed char) str[i + 1] == -119 || (signed char) str[i + 1] == -81)) {
											i--;
										} else if(i > 3 && ((signed char) str[i - 1] == -119 || (signed char) str[i - 1] == -81) && (signed char) str[i - 2] == -128 && (signed char) str[i - 3] == -30) {
											i -= 3;
										} else if(i > 3 && str[i] <= '9' && str[i] >= '0' && str[i - 1] <= '9' && str[i - 1] >= '0') {
											if(str[i - 2] == ' ' && str[i - 3] <= '9' && str[i - 3] >= '0') i -= 2;
											else if(str[i - 3] == ' ' && str[i - 4] <= '9' && str[i - 4] >= '0') i -= 3;
											else if((str[i - 2] == '.' || str[i - 2] == ',') && str[i - 3] <= '9' && str[i - 3] >= '0') i--;
											else if((str[i - 3] == '.' || str[i - 3] == ',') && str[i - 4] <= '9' && str[i - 4] >= '0') i -= 2;
										} else if(i > 4 && (str[i] == '.' || str[i] == ',') && str[i - 1] <= '9' && str[i - 1] >= '0' && str[i - 4] == str[i] && str[i - 5] <= '9' && str[i - 5] >= '0') {
											i -= 3;
										}
										str.insert(i, "\n");
										i_row = i + 1;
										r++;
										lb_point = string::npos;
										break;
									}
								}
							} else {
								str[lb_point] = '\n';
								i = lb_point;
								i_row = i + 1;
								r++;
								lb_point = string::npos;
							}
							c = 0;
						} else {
							c++;
						}
					} else {
						c++;
					}
				}
			}
		}
	}
	g_object_unref(layout);
	if(font_desc) pango_font_description_free(font_desc);
}
string remove_italic(string str) {
	gsub("<i>", "", str);
	gsub("</i>", "", str);
	gsub("<i class=\"symbol\">", "", str);
	gsub("<sup>2</sup>", SIGN_POWER_2, str);
	gsub("<sup>3</sup>", SIGN_POWER_3, str);
	gsub("<sup>4</sup>", SIGN_POWER_4, str);
	gsub("<sup>5</sup>", SIGN_POWER_5, str);
	gsub("<sup>6</sup>", SIGN_POWER_6, str);
	gsub("<sup>7</sup>", SIGN_POWER_7, str);
	gsub("<sup>8</sup>", SIGN_POWER_8, str);
	gsub("<sup>9</sup>", SIGN_POWER_9, str);
	gsub(" " SIGN_DIVISION_SLASH " ", "/", str);
	gsub("&amp;", "&", str);
	gsub("&gt;", ">", str);
	gsub("&lt;", "<", str);
	gsub("&quot;", "\"", str);
	gsub("&hairsp;", "", str);
	gsub("&nbsp;", " ", str);
	gsub("&thinsp;", THIN_SPACE, str);
	return str;
}
void reload_history(gint from_index) {
	if(from_index < 0) gtk_list_store_clear(historystore);
	string history_str;
	GtkTreeIter history_iter;
	size_t i = inhistory.size();
	gint pos = 0;
	FIX_SUPSUB_PRE(history_view_widget(), fix_supsub_history)
	while(i > 0 && (from_index < 0 || (i >= (size_t) from_index))) {
		i--;
		switch(inhistory_type[i]) {
			case QALCULATE_HISTORY_RESULT_APPROXIMATE: {}
			case QALCULATE_HISTORY_RESULT: {
				if(unformatted_history == 2) {
					fix_history_string2(inhistory[i]);
					improve_result_text(inhistory[i]);
				}
				history_str = "";
				size_t trans_l = 0;
				if(i + 1 < inhistory.size() && inhistory_type[i + 1] == QALCULATE_HISTORY_TRANSFORMATION) {
					history_str = fix_history_string(inhistory[i + 1]);
					history_str += ":  ";
					trans_l = history_str.length();
				}
				if(inhistory_type[i] == QALCULATE_HISTORY_RESULT_APPROXIMATE) {
					if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) history_view_widget())) {
						history_str += SIGN_ALMOST_EQUAL;
					} else {
						history_str += "= ";
						history_str += _("approx.");
					}
				} else {
					history_str += "=";
				}
				history_str += " ";
				size_t history_expr_i = history_str.length();
				history_str += fix_history_string_new(inhistory[i]);
				add_line_breaks(history_str, 2, history_expr_i);
				FIX_SUPSUB(history_str)
				if(trans_l > 0) {
					trans_l = history_str.find(":  ");
					if(trans_l != string::npos) {
						trans_l += 3;
						history_str.insert(trans_l, "</span>");
						history_str.insert(0, "<span font-style=\"italic\">");
					}
				}
				if(pango_version() >= 15000) history_str.insert(0, "<span font_size=\"110%\">");
				else history_str.insert(0, "<span font_size=\"larger\">");
				history_str += "</span>";
				gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 0, history_str.c_str(), 1, i, 3, inhistory_value[i], 4, 0, 5, history_scroll_width, 6, 1.0, 7, PANGO_ALIGN_RIGHT, -1);
				pos++;
				break;
			}
			case QALCULATE_HISTORY_PARSE_APPROXIMATE: {}
			case QALCULATE_HISTORY_PARSE: {
				if(unformatted_history == 2) {
					fix_history_string2(inhistory[i]);
				}
				if(i + 1 < inhistory.size() && (inhistory_type[i + 1] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[i + 1] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[i + 1] == QALCULATE_HISTORY_REGISTER_MOVED)) {
					if(i + 2 >= inhistory.size() || inhistory_type[i + 2] != QALCULATE_HISTORY_BOOKMARK) {
						if(i < inhistory.size() - 2) {gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 1, -1, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1); pos++;}
					}
					if(!inhistory[i].empty()) {
						string expr_str;
						if(inhistory_type[i + 1] == QALCULATE_HISTORY_RPN_OPERATION) expr_str = ("RPN Operation");
						else if(inhistory_type[i + 1] == QALCULATE_HISTORY_REGISTER_MOVED) expr_str = ("RPN Register Moved");
						else expr_str = inhistory[i + 1];
						if(history_expression_type == 0 && !inhistory[i].empty()) history_str = fix_history_string_new(inhistory[i]);
						else history_str = fix_history_string(expr_str);
						string str2;
						bool b_parse = history_expression_type > 1 && (history_str != remove_italic(inhistory[i]));
						if(b_parse) {
							history_str += "<span font-style=\"italic\" foreground=\"";
							history_str += history_parse_color;
							history_str += "\">  ";
							if(inhistory_type[i] == QALCULATE_HISTORY_PARSE) {
								str2 = "=";
							} else {
								if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) history_view_widget())) {
									str2 = SIGN_ALMOST_EQUAL;
								} else {
									str2 = _("approx.");
								}
							}
							history_str += str2;
							history_str += " ";
							history_str += fix_history_string_new(inhistory[i]);
							history_str += "</span>";
							FIX_SUPSUB(history_str)
						}
						PangoLayout *layout = gtk_widget_create_pango_layout(history_view_widget(), "");
						pango_layout_set_markup(layout, history_str.c_str(), -1);
						gint w = 0;
						pango_layout_get_pixel_size(layout, &w, NULL);
						if(w > history_width_e) {
							if(history_expression_type == 0 && !inhistory[i].empty()) history_str = fix_history_string_new(inhistory[i]);
							else history_str = fix_history_string(expr_str);
							add_line_breaks(history_str, 1, 0);
							if(b_parse) {
								str2 += " ";
								size_t history_expr_i = str2.length();
								str2 += inhistory[i];
								add_line_breaks(str2, 3, history_expr_i);
								FIX_SUPSUB(str2)
								history_str += '\n';
								history_str += "<span font-style=\"italic\" foreground=\"";
								history_str += history_parse_color;
								history_str += "\">";
								history_str += str2;
								history_str += "</span>";
							}
						}
						if(inhistory_protected[i + 1] || (i + 2 < inhistory.size() && inhistory_type[i + 2] == QALCULATE_HISTORY_BOOKMARK)) {
							if(can_display_unicode_string_function_exact("🔒", history_view_widget())) history_str += "<span size=\"small\"><sup> 🔒</sup></span>";
							else history_str += "<span size=\"x-small\"><sup> P</sup></span>";
						}
						gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 0, history_str.c_str(), 1, i, 2, inhistory_value[i] > 0 ? i2s(inhistory_value[i]).c_str() : "   ", 3, inhistory_value[i], 4, EXPRESSION_YPAD, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
						pos++;
						g_object_unref(layout);
					}
				}
				break;
			}
			case QALCULATE_HISTORY_ERROR: {}
			case QALCULATE_HISTORY_MESSAGE: {}
			case QALCULATE_HISTORY_WARNING: {
				string str = "- ";
				str += inhistory[i];
				fix_history_string2(str);
				add_line_breaks(str, false, 4);
				if(inhistory_type[i] == QALCULATE_HISTORY_MESSAGE) {
					if(pango_version() >= 15000) history_str = "<span font_size=\"90%\"><i>";
					else history_str = "<i>";
				} else {
					if(pango_version() >= 15000) history_str = "<span font_size=\"90%\" foreground=\"";
					else history_str = "<span foreground=\"";
					if(inhistory_type[i] == QALCULATE_HISTORY_WARNING) history_str += history_warning_color;
					else history_str += history_error_color;
					history_str += "\">";
				}
				history_str += str;
				if(inhistory_type[i] == QALCULATE_HISTORY_MESSAGE) history_str += "</i>";
				if(pango_version() >= 15000 || inhistory_type[i] != QALCULATE_HISTORY_MESSAGE) history_str += "</span>";
				if(i + 2 < inhistory.size() && inhistory_type[i + 2] == QALCULATE_HISTORY_EXPRESSION && inhistory_protected[i + 2]) {
					if(can_display_unicode_string_function_exact("🔒", history_view_widget())) history_str += "<span size=\"small\"><sup> 🔒</sup></span>";
					else history_str += "<span size=\"x-small\"><sup> P</sup></span>";
				}
				gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 0, history_str.c_str(), 1, i, 3, inhistory_value[i], 4, 0, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
				pos++;
				break;
			}
			case QALCULATE_HISTORY_BOOKMARK: {
				if(i > 0 && (inhistory_type[i - 1] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[i - 1] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[i - 1] == QALCULATE_HISTORY_REGISTER_MOVED)) {
					if(i < inhistory.size() - 1) {gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 1, -1, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1); pos++;}
				}
				string str = inhistory[i];
				fix_history_string2(str);
				add_line_breaks(str, false);
				history_str = "<span foreground=\"";
				history_str += history_bookmark_color;
				history_str += "\">";
				history_str += str;
				history_str += ":";
				history_str += "</span>";
				gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 0, history_str.c_str(), 1, i, 3, inhistory_value[i], 4, 0, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
				pos++;
				break;
			}
			default: {}
		}
	}
	if(inhistory.size() != 0) {gtk_list_store_insert_with_values(historystore, &history_iter, from_index < 0 ? -1 : pos, 1, -1, 2, "   ", 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1); pos++;}
	if(unformatted_history == 2) unformatted_history = 0;
}

time_t history_time;
GtkTreeIter history_iter;
bool add_result_to_history_pre(bool update_parse, bool update_history, bool register_moved, bool b_rpn_operation, bool *first_expression, string &result_text, string &transformation) {
	inhistory_index = 0;
	history_time = time(NULL);
	history_index_bak = history_index;
	*first_expression = current_inhistory_index < 0;
	if(update_history) {
		FIX_SUPSUB_PRE(history_view_widget(), fix_supsub_history)
		if(update_parse || register_moved || current_inhistory_index < 0) {
			if(register_moved) {
				inhistory_type.push_back(QALCULATE_HISTORY_REGISTER_MOVED);
				inhistory_time.push_back(history_time);
				inhistory_protected.push_back(false);
				inhistory.push_back("");
				inhistory_value.push_back(nr_of_new_expressions);
			} else {
				remove_blank_ends(result_text);
				gsub("\n", " ", result_text);
				if(b_rpn_operation) {
					inhistory_type.push_back(QALCULATE_HISTORY_RPN_OPERATION);
					inhistory_time.push_back(history_time);
					inhistory_protected.push_back(false);
					inhistory.push_back("");
					inhistory_value.push_back(nr_of_new_expressions);
				} else {
					inhistory_type.push_back(QALCULATE_HISTORY_EXPRESSION);
					inhistory_time.push_back(history_time);
					inhistory_protected.push_back(false);
					inhistory.push_back(result_text);
					inhistory_value.push_back(nr_of_new_expressions);
				}
			}
			nr_of_new_expressions++;
			string history_str = fix_history_string(result_text);
			FIX_SUPSUB(history_str)
			gtk_list_store_insert_with_values(historystore, &history_iter, 0, 0, history_str.c_str(), 1, inhistory.size() - 1, 2, i2s(nr_of_new_expressions).c_str(), 3, nr_of_new_expressions, 4, EXPRESSION_YPAD, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
			gtk_list_store_insert_with_values(historystore, NULL, 1, 1, -1, 5, history_scroll_width, 6, 1.0, 7, PANGO_ALIGN_RIGHT, -1);
			history_index = 0;
			inhistory_index = inhistory.size() - 1;
			history_parsed.push_back(NULL);
			history_answer.push_back(NULL);
		} else if(current_inhistory_index >= 0) {
			inhistory_index = current_inhistory_index;
			if(!transformation.empty()) {
				string history_str = fix_history_string(transformation);
				history_str += ":";
				add_line_breaks(history_str, 3, 0);
				FIX_SUPSUB(history_str)
				history_str.insert(0, "<span font-style=\"italic\">");
				history_str += "</span>";
				history_index++;
				gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, 5, history_scroll_width, 6, 1.0, 7, PANGO_ALIGN_RIGHT, -1);
				GtkTreeIter index_iter = history_iter;
				gint index_hi = -1;
				while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &index_iter)) {
					gtk_tree_model_get(GTK_TREE_MODEL(historystore), &index_iter, 1, &index_hi, -1);
					if(index_hi >= 0) {
						gtk_list_store_set(historystore, &index_iter, 1, index_hi + 1, -1);
					}
				}
				inhistory.insert(inhistory.begin() + inhistory_index, transformation);
				inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_TRANSFORMATION);
				inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
				inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
				inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
			}
		} else {
			return false;
		}
	}
	return true;
}
bool contains_rand_function(const MathStructure &m) {
	if(m.isFunction() && m.function()->category() == CALCULATOR->getFunctionById(FUNCTION_ID_RAND)->category()) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_rand_function(m[i])) return true;
	}
	return false;
}
bool contains_large_matrix(const MathStructure &m) {
	if((m.isVector() && m.size() > 500) || (m.isMatrix() && m.rows() * m.columns() > 500)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_large_matrix(m[i])) return true;
	}
	return false;
}
void add_result_to_history(bool &update_history, bool update_parse, bool register_moved, bool b_rpn_operation, string &result_text, bool b_approx, string &parsed_text, bool parsed_approx, string &transformation, GtkWindow *win, string *error_str, int *mtype_highest_p, bool *implicit_warning) {
	if(current_inhistory_index < 0) current_inhistory_index = 0;
	bool do_scroll = false;
	if(update_history) {
		if(unformatted_length(result_text) > (!current_result()->isMatrix() && contains_large_matrix(*current_result()) ? 10000 : 500000)) {
			if(current_result()->isMatrix()) {
				result_text = "matrix ("; result_text += i2s(current_result()->rows()); result_text += SIGN_MULTIPLICATION; result_text += i2s(current_result()->columns()); result_text += ")";
			} else {
				result_text = fix_history_string(ellipsize_result(unhtmlize(result_text), 5000));
			}
		}
		if(update_parse && unformatted_length(parsed_text) > 500000) {
			parsed_text = fix_history_string(ellipsize_result(unhtmlize(parsed_text), 5000));
		}
		if((!update_parse || (!rpn_mode && !register_moved && !b_rpn_operation && nr_of_new_expressions > 1 && !contains_rand_function(*current_parsed_result()))) && current_inhistory_index >= 0 && transformation.empty() && !CALCULATOR->message() && inhistory[current_inhistory_index] == result_text && inhistory_type[current_inhistory_index] == (b_approx ? QALCULATE_HISTORY_RESULT_APPROXIMATE : QALCULATE_HISTORY_RESULT)) {
			if(update_parse) {
				int b = 0;
				for(size_t i = current_inhistory_index; i < inhistory.size(); i++) {
					if(b == 1 && inhistory_type[i] == QALCULATE_HISTORY_EXPRESSION) {
						if(inhistory[i] == inhistory[inhistory_index - 1]) {
							b = 2;
						}
						break;
					} else if(b == 1) {
						break;
					} else if(inhistory_type[i] == QALCULATE_HISTORY_PARSE || inhistory_type[i] == QALCULATE_HISTORY_PARSE_APPROXIMATE) {
						if((inhistory_type[i] == QALCULATE_HISTORY_PARSE_APPROXIMATE) == parsed_approx && inhistory[i] == parsed_text) {
							b = 1;
						} else {
							break;
						}
					}
				}
				if(b == 2) {
					nr_of_new_expressions--;
					update_history = false;
					history_index = history_index_bak;
					gtk_list_store_remove(historystore, &history_iter);
					gtk_list_store_remove(historystore, &history_iter);
					inhistory_type.pop_back();
					inhistory_time.pop_back();
					inhistory_protected.pop_back();
					inhistory.pop_back();
					inhistory_value.pop_back();
					history_parsed.pop_back();
					history_answer.pop_back();
				}
			} else {
				update_history = false;
				history_index = history_index_bak;
			}
		}
	}
	if(update_history) {
		FIX_SUPSUB_PRE(history_view_widget(), fix_supsub_history)
		if(update_parse) {
			gchar *expr_str = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 0, &expr_str, -1);
			string str;
			if(history_expression_type == 0 && !parsed_text.empty()) str = fix_history_string_new(parsed_text);
			else str = expr_str;
			string str2;
			if(!parsed_approx) {
				str2 = "=";
				inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_PARSE);
				inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
				inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
				inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
			} else {
				if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) history_view_widget())) {
					str2 = SIGN_ALMOST_EQUAL;
				} else {
					str2 = _("approx.");
				}
				inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_PARSE_APPROXIMATE);
				inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
				inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
				inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
			}
			bool b_parse = history_expression_type > 1 && (expr_str != remove_italic(parsed_text));
			if(b_parse) {
				str += "<span font-style=\"italic\" foreground=\"";
				str += history_parse_color;
				str += "\">  ";
				str += str2;
				str += " ";
				str += fix_history_string_new(parsed_text);
				str += "</span>";
				FIX_SUPSUB(str)
			}
			inhistory.insert(inhistory.begin() + inhistory_index, parsed_text);
			if(nr_of_new_expressions > 0 && current_parsed_result() && !history_parsed[nr_of_new_expressions - 1]) {
				history_parsed[nr_of_new_expressions - 1] = new MathStructure(*current_parsed_result());
			}
			PangoLayout *layout = gtk_widget_create_pango_layout(history_view_widget(), "");
			pango_layout_set_markup(layout, str.c_str(), -1);
			gint w = 0;
			pango_layout_get_pixel_size(layout, &w, NULL);
			if(w > history_width_e) {
				if(history_expression_type == 0 && !parsed_text.empty()) str = fix_history_string_new(parsed_text);
				else str = expr_str;
				add_line_breaks(str, 1, 0);
				if(b_parse) {
					str2 += " ";
					size_t history_expr_i = str2.length();
					str2 += fix_history_string_new(parsed_text);
					add_line_breaks(str2, 3, history_expr_i);
					FIX_SUPSUB(str2)
					str += '\n';
					str += "<span font-style=\"italic\" foreground=\"";
					str += history_parse_color;
					str += "\">";
					str += str2;
					str += "</span>";
				}
			}
			gtk_list_store_set(historystore, &history_iter, 0, str.c_str(), -1);
			g_object_unref(layout);
			g_free(expr_str);
		}
		int history_index_bak = history_index;
		*error_str = history_display_errors(true, win, 1, win && update_parse && update_history && evalops.parse_options.parsing_mode <= PARSING_MODE_CONVENTIONAL ? implicit_warning : NULL, history_time, mtype_highest_p);

		string str;

		if(!b_approx) {
			str = "=";
		} else {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) history_view_widget())) {
				str = SIGN_ALMOST_EQUAL;
			} else {
				str = "= ";
				str += _("approx.");
			}
		}
		string history_str;
		size_t trans_l = 0;
		if(!update_parse && current_inhistory_index >= 0 && !transformation.empty() && history_index == history_index_bak) {
			history_str = fix_history_string(transformation);
			history_str += ":  ";
			trans_l = history_str.length();
		}
		history_str += str;
		history_str += " ";
		size_t history_expr_i = history_str.length();
		history_str += fix_history_string_new(result_text);
		add_line_breaks(history_str, 2, history_expr_i);
		FIX_SUPSUB(history_str)
		if(trans_l > 0) {
			trans_l = history_str.find(":  ");
			if(trans_l != string::npos) {
				trans_l += 3;
				history_str.insert(trans_l, "</span>");
				history_str.insert(0, "<span font-style=\"italic\">");
			}
		}
		if(pango_version() >= 15000) history_str.insert(0, "<span font_size=\"110%\">");
		else history_str.insert(0, "<span font_size=\"larger\">");
		history_str += "</span>";
		if(!update_parse && current_inhistory_index >= 0 && !transformation.empty() && history_index_bak == history_index) {
			gtk_list_store_set(historystore, &history_iter, 0, history_str.c_str(), 1, inhistory_index + 1, -1);
		} else {
			history_index++;
			gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, 5, history_scroll_width, 6, 1.0, 7, PANGO_ALIGN_RIGHT, -1);
		}
		inhistory.insert(inhistory.begin() + inhistory_index, result_text);
		current_inhistory_index = inhistory_index;
		if(b_approx) {
			inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_RESULT_APPROXIMATE);
		} else {
			inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_RESULT);
		}
		inhistory_time.insert(inhistory_time.begin() + inhistory_index, history_time);
		inhistory_protected.insert(inhistory_protected.begin() + inhistory_index, false);
		inhistory_value.insert(inhistory_value.begin() + inhistory_index, nr_of_new_expressions);
		if(nr_of_new_expressions > 0 && current_result() && nr_of_new_expressions <= (int) history_answer.size()) {
			if(!history_answer[nr_of_new_expressions - 1]) history_answer[nr_of_new_expressions - 1] = new MathStructure(*current_result());
			else history_answer[nr_of_new_expressions - 1]->set(*current_result());
		}

		GtkTreeIter index_iter = history_iter;
		gint index_hi = -1;
		while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &index_iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &index_iter, 1, &index_hi, -1);
			if(index_hi >= 0) {
				gtk_list_store_set(historystore, &index_iter, 1, index_hi + 1, -1);
			}
		}
		do_scroll = true;
	} else {
		int history_index_bak = history_index;
		*error_str = history_display_errors(true, win, 1, !win ? NULL : implicit_warning, history_time, mtype_highest_p);
		do_scroll = (history_index != history_index_bak);
	}
	if(do_scroll && gtk_widget_get_realized(history_view_widget())) {
		while(gtk_events_pending()) gtk_main_iteration();
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, history_index_column, FALSE, 0, 0);
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(history_view_widget()), 0, 0);
		gtk_tree_path_free(path);
	}
}
void add_message_to_history(string *error_str, int *mtype_highest_p) {
	history_time = time(NULL);
	history_index = -1;
	inhistory_type.push_back(QALCULATE_HISTORY_PARSE);
	inhistory_time.push_back(history_time);
	inhistory_protected.push_back(false);
	inhistory.push_back("");
	inhistory_value.push_back(-1);
	inhistory_type.push_back(QALCULATE_HISTORY_EXPRESSION);
	inhistory_time.push_back(history_time);
	inhistory_protected.push_back(false);
	inhistory.push_back("");
	inhistory_value.push_back(-1);
	int inhistory_index = inhistory.size() - 2;
	if(history_index >= 0) gtk_list_store_insert_with_values(historystore, NULL, history_index + 1, 1, -1, 5, history_scroll_width, 6, 1.0, 7, PANGO_ALIGN_RIGHT, -1);
	while(gtk_events_pending()) gtk_main_iteration();
	if(gtk_widget_get_realized(history_view_widget())) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, history_index_column, FALSE, 0, 0);
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(history_view_widget()), 0, 0);
		gtk_tree_path_free(path);
	}
	*error_str = history_display_errors(true, NULL, 1, NULL, history_time, mtype_highest_p);
	current_inhistory_index = inhistory_index;
}
string last_history_expression() {
	if(!inhistory_type.empty() && inhistory_type[inhistory_type.size() - 1] == QALCULATE_HISTORY_EXPRESSION) return inhistory[inhistory_type.size() - 1];
	return "";
}
bool history_activated() {
	return current_inhistory_index >= 0;
}
void set_history_activated() {
	if(current_inhistory_index < 0) current_inhistory_index = 0;
}
int history_new_expression_count() {
	return nr_of_new_expressions;
}
void history_free() {
	for(size_t i = 0; i < history_parsed.size(); i++) {
		if(history_parsed[i]) history_parsed[i]->unref();
		if(history_answer[i]) history_answer[i]->unref();
	}
}

void update_history_colors(bool initial) {
	GdkRGBA c;
	gtk_style_context_get_color(gtk_widget_get_style_context(history_view_widget()), GTK_STATE_FLAG_NORMAL, &c);
	GdkRGBA c_red = c;
	if(c_red.red >= 0.8) {
		c_red.green /= 1.5;
		c_red.blue /= 1.5;
		c_red.red = 1.0;
	} else {
		if(c_red.red >= 0.5) c_red.red = 1.0;
		else c_red.red += 0.5;
	}
	g_snprintf(history_error_color, 8, "#%02x%02x%02x", (int) (c_red.red * 255), (int) (c_red.green * 255), (int) (c_red.blue * 255));

	GdkRGBA c_blue = c;
	if(c_blue.blue >= 0.8) {
		c_blue.green /= 1.5;
		c_blue.red /= 1.5;
		c_blue.blue = 1.0;
	} else {
		if(c_blue.blue >= 0.4) c_blue.blue = 1.0;
		else c_blue.blue += 0.6;
	}
	g_snprintf(history_warning_color, 8, "#%02x%02x%02x", (int) (c_blue.red * 255), (int) (c_blue.green * 255), (int) (c_blue.blue * 255));

	GdkRGBA c_green = c;
	if(c_green.green >= 0.8) {
		c_green.blue /= 1.5;
		c_green.red /= 1.5;
		c_green.green = 0.8;
	} else {
		if(c_green.green >= 0.4) c_green.green = 0.8;
		else c_green.green += 0.4;
	}
	g_snprintf(history_bookmark_color, 8, "#%02x%02x%02x", (int) (c_green.red * 255), (int) (c_green.green * 255), (int) (c_green.blue * 255));

	history_gray = c;
	if(history_gray.blue + history_gray.green + history_gray.red > 1.5) {
		history_gray.green /= 1.5;
		history_gray.red /= 1.5;
		history_gray.blue /= 1.5;
	} else if(history_gray.blue + history_gray.green + history_gray.red > 0.3) {
		history_gray.green += 0.235;
		history_gray.red += 0.235;
		history_gray.blue += 0.235;
	} else if(history_gray.blue + history_gray.green + history_gray.red > 0.15) {
		history_gray.green += 0.3;
		history_gray.red += 0.3;
		history_gray.blue += 0.3;
	} else {
		history_gray.green += 0.4;
		history_gray.red += 0.4;
		history_gray.blue += 0.4;
	}
	g_snprintf(history_parse_color, 8, "#%02x%02x%02x", (int) (history_gray.red * 255), (int) (history_gray.green * 255), (int) (history_gray.blue * 255));
	if(initial) g_object_set(G_OBJECT(history_index_renderer), "ypad", 0, "yalign", 0.0, "xalign", 0.5, "foreground-rgba", &history_gray, NULL);
}
void history_font_modified() {
	fix_supsub_history = test_supsub(history_view_widget());
}
void update_history_font(bool initial) {
	if(use_custom_history_font) {
		gchar *gstr = font_name_to_css(custom_history_font.c_str());
		gtk_css_provider_load_from_data(history_provider, gstr, -1, NULL);
		g_free(gstr);
	} else if(initial) {
		if(custom_history_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(history_view_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_history_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	} else {
		gtk_css_provider_load_from_data(history_provider, "", -1, NULL);
	}
	history_font_modified();
}
void set_history_font(const char *str) {
	if(!str) {
		use_custom_history_font = false;
	} else {
		use_custom_history_font = true;
		if(custom_history_font != str) {
			save_custom_history_font = true;
			custom_history_font = str;
		}
	}
	update_history_font(false);
}
const char *history_font(bool return_default) {
	if(!return_default && !use_custom_history_font) return NULL;
	return custom_history_font.c_str();
}

#define INDEX_TYPE_ANS 0
#define INDEX_TYPE_XPR 1
#define INDEX_TYPE_TXT 2
void process_history_selection(vector<size_t> *selected_rows, vector<size_t> *selected_indeces, vector<int> *selected_index_type, bool ans_priority = false) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *selected_list, *current_selected_list;
	gint index = -1, hindex = -1;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
	selected_list = gtk_tree_selection_get_selected_rows(select, &model);
	current_selected_list = selected_list;
	while(current_selected_list) {
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) current_selected_list->data);
		gtk_tree_model_get(model, &iter, 1, &hindex, 3, &index, -1);
		if(hindex >= 0) {
			if(selected_rows) selected_rows->push_back((size_t) hindex);
			if(selected_indeces && (index <= 0 || !evalops.parse_options.functions_enabled || evalops.parse_options.base > BASE_DECIMAL || evalops.parse_options.base < 0)) {
				if(HISTORY_NOT_MESSAGE(hindex) && inhistory_type[hindex] != QALCULATE_HISTORY_BOOKMARK && (hindex < 1 || inhistory_type[hindex] != QALCULATE_HISTORY_TRANSFORMATION || inhistory_type[hindex - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[hindex - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) {
					selected_indeces->push_back((size_t) hindex);
					selected_index_type->push_back(INDEX_TYPE_TXT);
				}
			} else if(selected_indeces && index > 0) {
				bool index_found = false;
				size_t i = selected_indeces->size();
				for(; i > 0; i--) {
					if(selected_index_type->at(i - 1) != INDEX_TYPE_TXT && selected_indeces->at(i - 1) == (size_t) index) {
						index_found = true;
						break;
					}
				}
				if(!index_found) selected_indeces->push_back(index);
				switch(inhistory_type[hindex]) {
					case QALCULATE_HISTORY_EXPRESSION: {}
					case QALCULATE_HISTORY_REGISTER_MOVED: {}
					case QALCULATE_HISTORY_PARSE: {}
					case QALCULATE_HISTORY_PARSE_APPROXIMATE: {}
					case QALCULATE_HISTORY_RPN_OPERATION: {
						if(!index_found) selected_index_type->push_back(INDEX_TYPE_XPR);
						else if(!ans_priority) selected_index_type->at(i - 1) = INDEX_TYPE_XPR;
						break;
					}
					default: {
						if(!index_found) selected_index_type->push_back(INDEX_TYPE_ANS);
						else if(ans_priority) selected_index_type->at(i - 1) = INDEX_TYPE_ANS;
					}
				}
			}
		}
		current_selected_list = current_selected_list->next;
	}
	if(selected_list) g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
}
void history_operator(string str_sign) {
	if(calculator_busy()) return;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(NULL, &selected_indeces, &selected_index_type);
	if(rpn_mode && !expression_is_empty()) execute_expression();
	if(selected_indeces.empty()) {
		if(rpn_mode) {
			block_undo();
			insert_text(str_sign.c_str());
			unblock_undo();
			execute_expression();
			return;
		}
		if(evalops.parse_options.parsing_mode != PARSING_MODE_RPN) {
			if(do_chain_mode(expression_times_sign())) return;
			wrap_expression_selection();
		}
		insert_text(str_sign.c_str());
		return;
	}
	bool only_one_value = false;
	string str;
	if(selected_indeces.size() == 1) {
		str = get_selected_expression_text(true);
		if(str.empty()) {
			only_one_value = true;
		} else {
			string search_s = CALCULATOR->getDecimalPoint() + NUMBER_ELEMENTS;
			if((str.length() < 2 || str[0] != '(' || str[str.length() - 1] != ')') && str.find_first_not_of(search_s) != string::npos) {
				str.insert(str.begin(), '(');
				str += ')';
			}
			if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) str += ' ';
			else str += str_sign;
		}
	}

	for(size_t i = 0; i < selected_indeces.size(); i++) {
		if(i > 0) {
			if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) str += ' ';
			else str += str_sign;
		}
		if(selected_index_type[i] == INDEX_TYPE_TXT) {
			int index = selected_indeces[i];
			if(index > 0 && inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION) index--;
			string search_s = CALCULATOR->getDecimalPoint() + NUMBER_ELEMENTS;
			if((inhistory[index].length() >= 2 && inhistory[index][0] == '(' && inhistory[index][inhistory[index].length() - 1] == ')') || inhistory[index].find_first_not_of(search_s) == string::npos) {
				str += unhtmlize(inhistory[index]);
			} else {
				str += '(';
				str += unhtmlize(inhistory[index]);
				str += ')';
			}
		} else {
			const ExpressionName *ename = NULL;
			if(selected_index_type[i] == INDEX_TYPE_XPR) ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			else ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			str += ename->formattedName(TYPE_FUNCTION, true);
			str += '(';
			Number nr(selected_indeces[i], 1);
			str += print_with_evalops(nr);
			str += ')';
		}
	}
	if(only_one_value && evalops.parse_options.parsing_mode != PARSING_MODE_RPN && !rpn_mode) {
		str += str_sign;
	}
	if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
		str += ' ';
		if(selected_indeces.size() == 1) {
			str += str_sign;
		} else {
			for(size_t i = 0; i < selected_indeces.size() - 1; i++) {
				str += str_sign;
			}
		}
	}
	block_undo();
	gtk_text_buffer_set_text(expression_edit_buffer(), "", -1);
	unblock_undo();
	insert_text(str.c_str());
	if(!only_one_value && (!auto_calculate || parsed_in_result || rpn_mode)) {
		execute_expression();
	} else if(rpn_mode) {
		execute_expression();
		block_undo();
		insert_text(str_sign.c_str());
		unblock_undo();
		execute_expression();
	}

	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));

}

void on_button_history_add_clicked(GtkButton*, gpointer) {
	history_operator(expression_add_sign());
}
void on_button_history_sub_clicked(GtkButton*, gpointer) {
	history_operator(expression_sub_sign());
}
void on_button_history_times_clicked(GtkButton*, gpointer) {
	history_operator(expression_times_sign());
}
void on_button_history_divide_clicked(GtkButton*, gpointer) {
	history_operator(expression_divide_sign());
}
void on_button_history_xy_clicked(GtkButton*, gpointer) {
	history_operator("^");
}
void on_button_history_sqrt_clicked(GtkButton*, gpointer) {
	if(calculator_busy()) return;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(NULL, &selected_indeces, &selected_index_type);
	if(selected_indeces.empty()) {
		insert_button_function(CALCULATOR->f_sqrt);
		return;
	}
	const ExpressionName *ename2 = &CALCULATOR->f_sqrt->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
	string str = ename2->formattedName(TYPE_FUNCTION, true);
	str += "(";
	if(selected_index_type[0] == INDEX_TYPE_TXT) {
		int index = selected_indeces[0];
		if(index > 0 && inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION) index--;
		str += unhtmlize(inhistory[index]);
	} else {
		const ExpressionName *ename = NULL;
		if(selected_index_type[0] == INDEX_TYPE_XPR) ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
		else ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
		str += ename->formattedName(TYPE_FUNCTION, true);
		str += "(";
		Number nr(selected_indeces[0], 1);
		str += print_with_evalops(nr);
		str += ")";
	}
	str += ")";
	block_undo();
	gtk_text_buffer_set_text(expression_edit_buffer(), "", -1);
	unblock_undo();
	insert_text(str.c_str());
	execute_expression();
}
void on_button_history_insert_value_clicked(GtkButton*, gpointer) {
	if(calculator_busy()) return;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(NULL, &selected_indeces, &selected_index_type);
	if(selected_indeces.empty() || selected_index_type[0] == INDEX_TYPE_TXT) return;
	if(selected_indeces.size() > 1) {
		selected_indeces.clear();
		selected_index_type.clear();
		process_history_selection(NULL, &selected_indeces, &selected_index_type, true);
	}
	const ExpressionName *ename = NULL;
	if(selected_index_type[0] == INDEX_TYPE_XPR && (selected_indeces.size() == 1 || selected_index_type[1] == INDEX_TYPE_XPR)) ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
	else ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
	string str = ename->formattedName(TYPE_FUNCTION, true);
	str += "(";
	for(size_t i = 0; i < selected_indeces.size(); i++) {
		if(selected_index_type[i] != INDEX_TYPE_TXT) {
			if(i > 0) {str += CALCULATOR->getComma(); str += ' ';}
			Number nr(selected_indeces[i], 1);
			str += print_with_evalops(nr);
		}
	}
	str += ")";
	if(rpn_mode) {
		block_undo();
		insert_text(str.c_str());
		unblock_undo();
		execute_expression();
	} else {
		insert_text(str.c_str());
	}
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_button_history_insert_text_clicked(GtkButton*, gpointer) {
	if(calculator_busy()) return;
	vector<size_t> selected_rows;
	process_history_selection(&selected_rows, NULL, NULL);
	if(selected_rows.empty()) return;
	int index = selected_rows[0];
	if(index > 0 && ((inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION && (inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) || inhistory_type[index] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[index] == QALCULATE_HISTORY_REGISTER_MOVED)) index--;
	else if((size_t) index < inhistory_type.size() - 1 && HISTORY_IS_PARSE(index) && inhistory_type[index + 1] == QALCULATE_HISTORY_EXPRESSION) index++;
	insert_text(unhtmlize(inhistory[index]).c_str());
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_button_history_insert_parsed_text_clicked(GtkButton*, gpointer) {
	if(calculator_busy()) return;
	vector<size_t> selected_rows;
	process_history_selection(&selected_rows, NULL, NULL);
	if(selected_rows.empty()) return;
	int index = selected_rows[0];
	if(index > 0 && ((inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION && (inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) || inhistory_type[index] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[index] == QALCULATE_HISTORY_REGISTER_MOVED)) index--;
	else if(index > 0 && inhistory_type[index] == QALCULATE_HISTORY_EXPRESSION && HISTORY_IS_PARSE(index - 1)) index--;
	insert_text(unhtmlize(inhistory[index]).c_str());
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void history_copy(bool full_text, int ascii = -1) {
	if(calculator_busy()) return;
	vector<size_t> selected_rows;
	process_history_selection(&selected_rows, NULL, NULL);
	if(selected_rows.empty()) return;
	if(!full_text && selected_rows.size() == 1) {
		int index = selected_rows[0];
		if(index > 0 && ((inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION && (inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) || inhistory_type[index] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[index] == QALCULATE_HISTORY_REGISTER_MOVED)) index--;
		else if((size_t) index < inhistory_type.size() - 1 && (inhistory_type[index] == QALCULATE_HISTORY_PARSE || inhistory_type[index] == QALCULATE_HISTORY_PARSE_WITHEQUALS || inhistory_type[index] == QALCULATE_HISTORY_PARSE_APPROXIMATE) && inhistory_type[index + 1] == QALCULATE_HISTORY_EXPRESSION) index++;
		set_clipboard(inhistory[index], ascii, inhistory_type[index] == QALCULATE_HISTORY_PARSE || inhistory_type[index] == QALCULATE_HISTORY_PARSE_APPROXIMATE || inhistory_type[index] == QALCULATE_HISTORY_RESULT || inhistory_type[index] == QALCULATE_HISTORY_RESULT_APPROXIMATE, inhistory_type[index] == QALCULATE_HISTORY_RESULT || inhistory_type[index] == QALCULATE_HISTORY_RESULT_APPROXIMATE);
	} else {
		string str;
		int hindex = 0;
		for(size_t i = 0; i < selected_rows.size(); i++) {
			if(i > 0) str += '\n';
			hindex = selected_rows[i];
			if((size_t) hindex < inhistory_type.size() - 1 && (inhistory_type[hindex] == QALCULATE_HISTORY_PARSE || inhistory_type[hindex] == QALCULATE_HISTORY_PARSE_WITHEQUALS || inhistory_type[hindex] == QALCULATE_HISTORY_PARSE_APPROXIMATE) && (inhistory_type[hindex + 1] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[hindex + 1] == QALCULATE_HISTORY_REGISTER_MOVED || inhistory_type[hindex + 1] == QALCULATE_HISTORY_RPN_OPERATION)) hindex++;
			on_button_history_copy_add_hindex:
			bool add_parse = false;
			switch(inhistory_type[hindex]) {
				case QALCULATE_HISTORY_EXPRESSION: {
					if(i > 0) str += '\n';
					str += fix_history_string(inhistory[hindex]);
					add_parse = true;
					break;
				}
				case QALCULATE_HISTORY_REGISTER_MOVED: {
					if(i > 0) str += '\n';
					str += _("RPN Register Moved");
					add_parse = true;
					break;
				}
				case QALCULATE_HISTORY_RPN_OPERATION: {
					if(i > 0) str += '\n';
					str += _("RPN Operation");
					add_parse = true;
					break;
				}
				case QALCULATE_HISTORY_TRANSFORMATION: {
					str += fix_history_string(inhistory[hindex]);
					str += ": ";
					if(hindex > 0 && (inhistory_type[hindex - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[hindex - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) {
						hindex--;
						goto on_button_history_copy_add_hindex;
					}
					break;
				}
				case QALCULATE_HISTORY_PARSE: {str += " ";}
				case QALCULATE_HISTORY_RESULT: {
					str += "= ";
					str += inhistory[hindex];
					break;
				}
				case QALCULATE_HISTORY_PARSE_APPROXIMATE: {str += " ";}
				case QALCULATE_HISTORY_RESULT_APPROXIMATE: {
					if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) history_view_widget())) {
						str += SIGN_ALMOST_EQUAL " ";
					} else {
						str += "= ";
						str += _("approx.");
						str += " ";
					}
					str += inhistory[hindex];
					break;
				}
				case QALCULATE_HISTORY_PARSE_WITHEQUALS: {
					str += " ";
					str += fix_history_string(inhistory[hindex]);
					break;
				}
				case QALCULATE_HISTORY_MESSAGE: {}
				case QALCULATE_HISTORY_WARNING: {}
				case QALCULATE_HISTORY_ERROR: {}
				case QALCULATE_HISTORY_OLD: {
					str += fix_history_string(inhistory[hindex]);
					break;
				}
				case QALCULATE_HISTORY_BOOKMARK: {break;}
			}
			if(add_parse && hindex > 0 && (inhistory_type[hindex - 1] == QALCULATE_HISTORY_PARSE || inhistory_type[hindex - 1] == QALCULATE_HISTORY_PARSE_APPROXIMATE || inhistory_type[hindex - 1] == QALCULATE_HISTORY_PARSE_WITHEQUALS)) {
				hindex--;
				goto on_button_history_copy_add_hindex;
			}
		}
		set_clipboard(str, ascii, true, false);
	}
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_button_history_copy_clicked(GtkButton*, gpointer) {
	history_copy(false, -1);
}
bool history_protected_by_bookmark(size_t hi);
bool history_protected(size_t hi);
void on_popup_menu_item_history_clear_activate(GtkMenuItem*, gpointer) {
	history_clear();
}
void history_clear() {
	if(calculator_busy()) return;
	gtk_list_store_clear(historystore);
	bool b_protected = false;
	for(size_t i = inhistory.size(); i > 0;) {
		--i;
		if(inhistory_type[i] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[i] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[i] == QALCULATE_HISTORY_REGISTER_MOVED || inhistory_type[i] == QALCULATE_HISTORY_OLD) {
			b_protected = (inhistory_type[i] != QALCULATE_HISTORY_OLD && (inhistory_protected[i] || history_protected_by_bookmark(i)));
		}
		if(!b_protected && inhistory_type[i] != QALCULATE_HISTORY_BOOKMARK) {
			inhistory.erase(inhistory.begin() + i);
			inhistory_type.erase(inhistory_type.begin() + i);
			inhistory_time.erase(inhistory_time.begin() + i);
			inhistory_protected.erase(inhistory_protected.begin() + i);
			inhistory_value.erase(inhistory_value.begin() + i);
		}
	}
	current_inhistory_index = inhistory.size() - 1;
	history_index = -1;
	initial_inhistory_index = inhistory.size() - 1;
	set_expression_modified(true, true, false);
	reload_history();
}
void on_popup_menu_item_history_movetotop_activate(GtkMenuItem*, gpointer) {
	if(calculator_busy()) return;
	GtkTreeModel *model;
	GtkTreeIter iter, iter_first;
	GList *selected_list;
	gint hindex = -1, hindex2 = -1;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
	selected_list = gtk_tree_selection_get_selected_rows(select, &model);
	if(!selected_list) return;
	GList *list_i = g_list_last(selected_list);
	vector<int> indexes;
	while(list_i) {
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) list_i->data);
		gtk_tree_model_get(model, &iter, 1, &hindex, -1);
		list_i = list_i->prev;
		if(hindex >= 0) {
			if(inhistory_type[hindex] == QALCULATE_HISTORY_OLD) {
				indexes.push_back(hindex);
				gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			} else {
				iter_first = iter;
				while(gtk_tree_model_iter_next(model, &iter)) {
					gtk_tree_model_get(model, &iter, 1, &hindex, -1);
					if(hindex < 0 || ITEM_IS_EXPRESSION(hindex) || inhistory_type[hindex] == QALCULATE_HISTORY_OLD || inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK) {
						if(hindex < 0) {
							gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
						}
						break;
					}
					iter_first = iter;
				}
				iter = iter_first;
				bool b2 = true;
				do {
					if(list_i) {
						GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
						if(gtk_tree_path_compare(path, (GtkTreePath*) list_i->data) == 0) list_i = list_i->prev;
						gtk_tree_path_free(path);
					}
					gtk_tree_model_get(model, &iter, 1, &hindex, -1);
					if(inhistory_type[hindex] == QALCULATE_HISTORY_TRANSFORMATION) indexes.push_back(hindex - 1);
					indexes.push_back(hindex);
					if(HISTORY_IS_MESSAGE(hindex) && ITEM_IS_EXPRESSION(hindex)) {indexes.push_back(hindex + 1); hindex++;}
					if(HISTORY_IS_PARSE(hindex)) {indexes.push_back(hindex + 1); hindex++;}
					GtkTreeIter iter2 = iter;
					b2 = gtk_tree_model_iter_previous(model, &iter2);
					gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
					if(hindex < 0 || ITEM_IS_EXPRESSION(hindex) || inhistory_type[hindex] == QALCULATE_HISTORY_OLD || inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK) {
						if(hindex >= 0 && inhistory_type[hindex] != QALCULATE_HISTORY_BOOKMARK && b2) {
							gtk_tree_model_get(model, &iter2, 1, &hindex, -1);
							if(hindex < 0 || inhistory_type[hindex] != QALCULATE_HISTORY_BOOKMARK) {
								break;
							}
						} else {
							break;
						}
					}
					iter = iter2;
				} while(b2);
			}
		} else {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		}
	}
	unordered_map<int, int> new_indexes;
	hindex2 = -1;
	int n = 0;
	for(size_t i = 0; i < indexes.size(); i++) {
		hindex = indexes[i];
		if(hindex >= 0) {
			while(hindex2 >= 0 && hindex2 < hindex) {
				new_indexes[hindex2] = hindex2 - n;
				hindex2++;
			}
			n++;
			hindex2 = hindex + 1;
		}
	}
	while(hindex2 >= 0 && hindex2 < (gint) inhistory.size()) {
		new_indexes[hindex2] = hindex2 - n;
		hindex2++;
	}
	hindex2 = indexes[0];
	if(gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 1, &hindex, -1);
			if(hindex >= 0) {
				if(hindex < hindex2) break;
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, new_indexes[hindex], -1);
			}
		} while(gtk_tree_model_iter_next(model, &iter));
	}
	hindex2 = (gint) inhistory.size() - indexes.size() + 1;
	time_t history_time = time(NULL);
	for(size_t i = 0; i < indexes.size(); i++) {
		hindex = indexes[i];
		inhistory.push_back(inhistory[hindex]);
		inhistory_protected.push_back(inhistory_protected[hindex]);
		inhistory_type.push_back(inhistory_type[hindex]);
		inhistory_time.push_back(history_time);
		inhistory_value.push_back(inhistory_value[hindex]);
	}
	for(size_t i = indexes.size() - 1; ; i--) {
		hindex = indexes[i];
		inhistory.erase(inhistory.begin() + hindex);
		inhistory_protected.erase(inhistory_protected.begin() + hindex);
		inhistory_type.erase(inhistory_type.begin() + hindex);
		inhistory_time.erase(inhistory_time.begin() + hindex);
		inhistory_value.erase(inhistory_value.begin() + hindex);
		if(i == 0) break;
	}
	current_inhistory_index = inhistory.size() - 1;
	history_index = -1;
	initial_inhistory_index = inhistory.size() - 1;
	set_expression_modified(true, true, false);
	reload_history(hindex2);
	g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_popup_menu_item_history_delete_activate(GtkMenuItem*, gpointer) {
	if(calculator_busy()) return;
	GtkTreeModel *model;
	GtkTreeIter iter, iter2, iter3;
	GList *selected_list;
	gint hindex = -1, hindex2 = -1;
	bool del_prev = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
	selected_list = gtk_tree_selection_get_selected_rows(select, &model);
	if(!selected_list) return;
	GList *list_i = g_list_last(selected_list);
	vector<int> indexes;
	while(list_i || del_prev) {
		if(list_i) {
			gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) list_i->data);
			gtk_tree_model_get(model, &iter, 1, &hindex, -1);
		}
		if(del_prev && (!list_i || hindex != hindex2)) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter2);
			if(HISTORY_IS_EXPRESSION(hindex2)) indexes.push_back(hindex2 - 1);
			indexes.push_back(hindex2);
			if(HISTORY_IS_PARSE(hindex2)) {indexes.push_back(hindex2 + 1); hindex2++;}
			if(hindex2 + 1 != hindex && (size_t) hindex2 + 1 < inhistory.size() && inhistory_type[hindex2 + 1] == QALCULATE_HISTORY_BOOKMARK) {
				if(gtk_tree_model_iter_previous(model, &iter2)) {
					indexes.push_back(hindex2);
					gtk_list_store_remove(GTK_LIST_STORE(model), &iter2);
				}
			}
		}
		if(!list_i) break;
		del_prev = false;
		if(hindex >= 0 && (ITEM_IS_EXPRESSION(hindex) || inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK)) {
			iter3 = iter;
			if(inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK) gtk_tree_model_iter_next(model, &iter3);
			bool b = false;
			while(gtk_tree_model_iter_next(model, &iter3)) {
				gtk_tree_model_get(model, &iter3, 1, &hindex2, -1);
				if(hindex2 >= 0 && (ITEM_IS_EXPRESSION(hindex2) || inhistory_type[hindex2] == QALCULATE_HISTORY_OLD)) break;
				b = true;
				iter2 = iter3;
			}
			if(b) {
				while(true) {
					gtk_tree_model_get(model, &iter2, 1, &hindex2, -1);
					if(hindex2 == hindex) break;
					if(hindex2 >= 0) {
						if(inhistory_type[hindex2] == QALCULATE_HISTORY_TRANSFORMATION) indexes.push_back(hindex2 - 1);
						indexes.push_back(hindex2);
					}
					iter3 = iter2;
					gtk_tree_model_iter_previous(model, &iter2);
					gtk_list_store_remove(GTK_LIST_STORE(model), &iter3);
				}
			}
			if(HISTORY_IS_EXPRESSION(hindex)) indexes.push_back(hindex - 1);
		} else if(hindex >= 0 && inhistory_type[hindex] != QALCULATE_HISTORY_OLD) {
			iter2 = iter;
			if(gtk_tree_model_iter_next(model, &iter2)) {
				gtk_tree_model_get(model, &iter2, 1, &hindex2, -1);
				if(hindex2 < 0 || ITEM_IS_EXPRESSION(hindex2) || inhistory_type[hindex2] == QALCULATE_HISTORY_OLD) {
					iter2 = iter;
					if(gtk_tree_model_iter_previous(model, &iter2)) {
						gtk_tree_model_get(model, &iter2, 1, &hindex2, -1);
						if(hindex2 >= 0 && (ITEM_IS_EXPRESSION(hindex2) || inhistory_type[hindex2] == QALCULATE_HISTORY_OLD)) {
							del_prev = true;
							iter3 = iter;
							if(gtk_tree_model_iter_next(model, &iter3)) {
								gint hindex3 = 0;
								gtk_tree_model_get(model, &iter3, 1, &hindex3, -1);
								if(hindex3 < 0) gtk_list_store_remove(GTK_LIST_STORE(model), &iter3);
							}
						}
					}
				}
			}
		}
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		if(hindex >= 0) {
			if(inhistory_type[hindex] == QALCULATE_HISTORY_TRANSFORMATION) indexes.push_back(hindex - 1);
			indexes.push_back(hindex);
			if(HISTORY_IS_MESSAGE(hindex) && ITEM_IS_EXPRESSION(hindex)) {
				indexes.push_back(hindex + 1); hindex++;
				indexes.push_back(hindex + 1); hindex++;
			}
			if(HISTORY_IS_PARSE(hindex)) {indexes.push_back(hindex + 1); hindex++;}
			if(!del_prev && (size_t) hindex + 1 < inhistory.size() && inhistory_type[hindex + 1] == QALCULATE_HISTORY_BOOKMARK) {
				iter2 = iter;
				if(gtk_tree_model_iter_previous(model, &iter2)) {
					hindex2 = hindex + 1;
					del_prev = true;
				}
			}
		}
		list_i = list_i->prev;
	}
	unordered_map<int, int> new_indexes;
	hindex2 = -1;
	int n = 0;
	for(size_t i = 0; i < indexes.size(); i++) {
		hindex = indexes[i];
		if(hindex >= 0) {
			while(hindex2 >= 0 && hindex2 < hindex) {
				new_indexes[hindex2] = hindex2 - n;
				hindex2++;
			}
			n++;
			hindex2 = hindex + 1;
		}
	}
	while(hindex2 >= 0 && hindex2 < (gint) inhistory.size()) {
		new_indexes[hindex2] = hindex2 - n;
		hindex2++;
	}
	hindex2 = indexes[0];
	if(gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 1, &hindex, -1);
			if(hindex >= 0) {
				if(hindex < hindex2) break;
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, new_indexes[hindex], -1);
			}
		} while(gtk_tree_model_iter_next(model, &iter));
	}
	for(size_t i = indexes.size() - 1; ; i--) {
		hindex = indexes[i];
		if(inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK) {
			for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
				if(equalsIgnoreCase(inhistory[hindex], *it)) {
					history_bookmarks.erase(it);
					break;
				}
			}
		}
		inhistory.erase(inhistory.begin() + hindex);
		inhistory_protected.erase(inhistory_protected.begin() + hindex);
		inhistory_type.erase(inhistory_type.begin() + hindex);
		inhistory_time.erase(inhistory_time.begin() + hindex);
		inhistory_value.erase(inhistory_value.begin() + hindex);
		if(i == 0) break;
	}
	initial_inhistory_index = inhistory.size() - 1;
	if(new_indexes.count(current_inhistory_index) > 0) {
		current_inhistory_index = new_indexes[current_inhistory_index];
	} else {
		current_inhistory_index = inhistory.size() - 1;
		history_index = -1;
		set_expression_modified(true, true, false);
	}
	g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
}
void on_popup_menu_item_history_insert_value_activate(GtkMenuItem*, gpointer) {
	on_button_history_insert_value_clicked(NULL, NULL);
}
void on_popup_menu_item_history_insert_text_activate(GtkMenuItem*, gpointer) {
	on_button_history_insert_text_clicked(NULL, NULL);
}
void on_popup_menu_item_history_insert_parsed_text_activate(GtkMenuItem*, gpointer) {
	on_button_history_insert_parsed_text_clicked(NULL, NULL);
}
void on_popup_menu_item_history_copy_text_activate(GtkMenuItem*, gpointer) {
	history_copy(false, 0);
}
void on_popup_menu_item_history_copy_ascii_activate(GtkMenuItem*, gpointer) {
	history_copy(false, 1);
}
void on_popup_menu_item_history_copy_full_text_activate(GtkMenuItem*, gpointer) {
	history_copy(true, 0);
}
bool find_history_bookmark(string str, GtkTreeIter *iter2) {
	GtkTreeIter iter;
	gint hindex = -1;
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(historystore), &iter)) return false;
	while(true) {
		gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hindex, -1);
		if(hindex >= 0 && inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK && inhistory[hindex] == str) {
			*iter2 = iter;
			return true;
		}
		if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(historystore), &iter)) break;
	}
	return false;
}
void goto_history_bookmark(GtkMenuItem *w, gpointer) {
	string str = gtk_menu_item_get_label(w);
	if(history_bookmark_titles.count(str) > 0) str = history_bookmarks[history_bookmark_titles[str]];
	GtkTreeIter iter;
	if(find_history_bookmark(str, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(historystore), &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, history_index_column, TRUE, 0.0, 0.0);
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
		gtk_tree_selection_unselect_all(select);
		gtk_tree_selection_select_iter(select, &iter);
		gtk_tree_path_free(path);
	}
}
void remove_history_bookmark(string str) {
	for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
		if(equalsIgnoreCase(str, *it)) {
			history_bookmarks.erase(it);
			break;
		}
	}
	GtkTreeIter iter;
	gint hindex = 0;
	if(!find_history_bookmark(str, &iter)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hindex, -1);
	inhistory.erase(inhistory.begin() + hindex);
	inhistory_protected.erase(inhistory_protected.begin() + hindex);
	inhistory_type.erase(inhistory_type.begin() + hindex);
	inhistory_time.erase(inhistory_time.begin() + hindex);
	inhistory_value.erase(inhistory_value.begin() + hindex);
	GtkTreeIter history_iter = iter;
	if(gtk_tree_model_iter_next(GTK_TREE_MODEL(historystore), &history_iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 1, &hindex, -1);
		if(hindex >= 0 && !history_protected(hindex)) {
			gchar *gstr;
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 0, &gstr, -1);
			string str = gstr;
			size_t i = str.rfind("<span size=\"small\"><sup> ");
			if(i == string::npos) i = str.rfind("<span size=\"x-small\"><sup> ");
			if(i != string::npos) str = str.substr(0, i);
			gtk_list_store_set(historystore, &history_iter, 0, str.c_str(), -1);
			g_free(gstr);
		}
	}
	history_iter = iter;
	while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &history_iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 1, &hindex, -1);
		if(hindex >= 0) gtk_list_store_set(historystore, &history_iter, 1, hindex - 1, -1);
	}
	gtk_list_store_remove(historystore, &iter);
}
void add_history_bookmark(string history_message) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *selected_list;
	gint hindex = -1;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
	selected_list = gtk_tree_selection_get_selected_rows(select, &model);
	if(!selected_list) return;
	gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) selected_list->data);
	while(true) {
		gtk_tree_model_get(model, &iter, 1, &hindex, -1);
		if(hindex >= 0 && ITEM_IS_EXPRESSION(hindex)) break;
		if((hindex >= 0 && inhistory_type[hindex] == QALCULATE_HISTORY_OLD) || !gtk_tree_model_iter_previous(model, &iter)) {
			hindex = -1;
			break;
		}
	}
	if(hindex >= 0) {
		bool b = false;
		for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
			if(string_is_less(history_message, *it)) {
				history_bookmarks.insert(it, history_message);
				b = true;
				break;
			}
		}
		if(!b) history_bookmarks.push_back(history_message);
		if(HISTORY_IS_PARSE(hindex)) hindex++;
		hindex++;
		inhistory.insert(inhistory.begin() + hindex, history_message);
		inhistory_type.insert(inhistory_type.begin() + hindex, QALCULATE_HISTORY_BOOKMARK);
		inhistory_time.insert(inhistory_time.begin() + hindex, inhistory_time[hindex]);
		inhistory_protected.insert(inhistory_protected.begin() + hindex, false);
		inhistory_value.insert(inhistory_value.begin() + hindex, 0);
		fix_history_string2(history_message);
		add_line_breaks(history_message, false);
		string history_str = "<span foreground=\"";
		history_str += history_bookmark_color;
		history_str += "\">";
		history_str += history_message;
		history_str += ":";
		history_str += "</span>";
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 0, &gstr, -1);
		string str = gstr;
		if(str.find("<span size=\"x-small\"><sup> ") == string::npos && str.rfind("<span size=\"small\"><sup> ") == string::npos) {
			if(can_display_unicode_string_function_exact("🔒", history_view_widget())) str += "<span size=\"small\"><sup> 🔒</sup></span>";
			else str += "<span size=\"x-small\"><sup> P</sup></span>";
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, str.c_str(), -1);
		}
		g_free(gstr);
		gtk_list_store_insert_before(historystore, &iter, &iter);
		while(gtk_events_pending()) gtk_main_iteration();
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
		gtk_list_store_set(historystore, &iter, 0, history_str.c_str(), 1, hindex, 3, -1, 4, 0, 5, 6, 6, 0.0, 7, PANGO_ALIGN_LEFT, -1);
		while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hindex, -1);
			if(hindex >= 0) gtk_list_store_set(historystore, &iter, 1, hindex + 1, -1);
		}
	}
	set_expression_modified(true, true, true);
	g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
}
GtkWidget *history_search_dialog = NULL;
GtkWidget *history_search_entry = NULL;
void on_history_search_response(GtkDialog *w, gint reponse_id, gpointer) {
	if(reponse_id == GTK_RESPONSE_ACCEPT) {
		if(inhistory.empty()) return;
		char *cstr = utf8_strdown(gtk_entry_get_text(GTK_ENTRY(history_search_entry)));
		string str;
		if(cstr) {
			str = cstr;
			free(cstr);
		} else {
			str = gtk_entry_get_text(GTK_ENTRY(history_search_entry));
		}
		GtkTreeIter iter;
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
		GList *selected_list = gtk_tree_selection_get_selected_rows(select, NULL);
		GList *selected = NULL;
		if(selected_list) selected = g_list_last(selected_list);
		gint hi_first = inhistory.size() - 1;
		int b_wrap = -1;
		if(selected) {
			gtk_tree_model_get_iter(GTK_TREE_MODEL(historystore), &iter, (GtkTreePath*) selected->data);
			while(gtk_tree_model_iter_next(GTK_TREE_MODEL(historystore), &iter)) {
				gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hi_first, -1);
				if(hi_first >= 0) {
					b_wrap = 0;
					break;
				}
			}
			if(hi_first < 0) hi_first = inhistory.size() - 1;
		}
		if(!selected) {
			if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(historystore), &iter)) {
				g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
				return;
			}
		}
		string str2;
		for(gint i = hi_first; ; i--) {
			if(b_wrap == 1 && i == hi_first) {
				break;
			}
			cstr = utf8_strdown(inhistory[(size_t) i].c_str());
			if(cstr) {
				str2 = cstr;
				free(cstr);
			} else {
				str2 = inhistory[(size_t) i];
			}
			if(str2.find(str) != string::npos) {
				do {
					gint hi = -1;
					gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hi, -1);
					if(hi >= 0 && hi <= i) {
						GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(historystore), &iter);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, history_index_column, FALSE, 0.0, 0.0);
						gtk_tree_selection_unselect_all(select);
						gtk_tree_selection_select_iter(select, &iter);
						gtk_tree_path_free(path);
						break;
					}
				} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(historystore), &iter));
				break;
			} else if(i == 0) {
				if(b_wrap == 0) {
					b_wrap = 1;
					i = inhistory.size() - 1;
					gtk_tree_model_get_iter_first(GTK_TREE_MODEL(historystore), &iter);
				} else {
					break;
				}
			}
		}
		g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
	} else {
		history_search_dialog = NULL;
		gtk_widget_destroy(GTK_WIDGET(w));
	}
}
void on_history_search_activate(GtkEntry*, gpointer) {
	on_history_search_response(GTK_DIALOG(history_search_dialog), GTK_RESPONSE_ACCEPT, NULL);
}
void on_history_search_changed(GtkEditable*, gpointer) {
	gtk_widget_set_sensitive(gtk_dialog_get_widget_for_response(GTK_DIALOG(history_search_dialog), GTK_RESPONSE_ACCEPT), strlen(gtk_entry_get_text(GTK_ENTRY(history_search_entry))) > 0);
}

void on_popup_menu_item_history_search_activate(GtkMenuItem*, gpointer) {
	history_search();
}
void history_search() {
	if(history_search_dialog) {
		gtk_widget_show(history_search_dialog);
		gtk_window_present_with_time(GTK_WINDOW(history_search_dialog), GDK_CURRENT_TIME);
		gtk_widget_grab_focus(history_search_entry);
		return;
	}
	history_search_dialog = gtk_dialog_new_with_buttons(_("Search"), main_window(), (GtkDialogFlags) GTK_DIALOG_DESTROY_WITH_PARENT, _("_Close"), GTK_RESPONSE_REJECT, _("_Search"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(history_search_dialog), always_on_top);
	gtk_container_set_border_width(GTK_CONTAINER(history_search_dialog), 6);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(history_search_dialog))), hbox);
	history_search_entry = gtk_entry_new();
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(history_search_entry), GTK_ENTRY_ICON_PRIMARY, "edit-find");
	gtk_entry_set_icon_activatable(GTK_ENTRY(history_search_entry), GTK_ENTRY_ICON_PRIMARY, FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(history_search_entry), 35);
	gtk_box_pack_end(GTK_BOX(hbox), history_search_entry, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(gtk_dialog_get_widget_for_response(GTK_DIALOG(history_search_dialog), GTK_RESPONSE_ACCEPT), FALSE);
	g_signal_connect(G_OBJECT(history_search_entry), "activate", G_CALLBACK(on_history_search_activate), NULL);
	g_signal_connect(G_OBJECT(history_search_dialog), "response", G_CALLBACK(on_history_search_response), NULL);
	g_signal_connect(G_OBJECT(history_search_entry), "changed", G_CALLBACK(on_history_search_changed), NULL);
	gtk_widget_show_all(history_search_dialog);
	gtk_widget_grab_focus(history_search_entry);
}

void on_calendar_history_search_month_changed(GtkCalendar *date_w, gpointer) {
	guint year = 0, month = 0, day = 0;
	gtk_calendar_get_date(date_w, &year, &month, &day);
	gtk_calendar_clear_marks(date_w);
	struct tm tmdate;
	for(size_t i = inhistory_time.size(); i > 0; i--) {
		if(inhistory_time[i - 1] != 0) {
			tmdate = *localtime(&inhistory_time[i - 1]);
			if(tmdate.tm_year + 1900 == (int) year && tmdate.tm_mon == (int) month) {
				gtk_calendar_mark_day(date_w, tmdate.tm_mday);
			}
		}
	}

}
void on_popup_menu_item_history_search_date_activate(GtkMenuItem*, gpointer) {
	GtkWidget *d = gtk_dialog_new_with_buttons(_("Select date"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
	GtkWidget *date_w = gtk_calendar_new();
	on_calendar_history_search_month_changed(GTK_CALENDAR(date_w), NULL);
	g_signal_connect(G_OBJECT(date_w), "month-changed", G_CALLBACK(on_calendar_history_search_month_changed), NULL);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), date_w);
	gtk_widget_show_all(d);
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_OK) {
		guint year = 0, month = 0, day = 0;
		gtk_calendar_get_date(GTK_CALENDAR(date_w), &year, &month, &day);
		struct tm tmdate;
		for(size_t i = inhistory_time.size(); i > 0; i--) {
			if(inhistory_time[i - 1] != 0) {
				tmdate = *localtime(&inhistory_time[i - 1]);
				if(tmdate.tm_year + 1900 < (int) year || (tmdate.tm_year + 1900 == (int) year && (tmdate.tm_mon < (int) month ||  (tmdate.tm_mon == (int) month && tmdate.tm_mday <= (int) day)))) {
					GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
					GtkTreeIter iter;
					if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(historystore), &iter)) break;
					i--;
					gint hi = -1;
					do {
						gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hi, -1);
						if(hi >= 0 && (size_t) hi <= i) {
							GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(historystore), &iter);
							gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, history_index_column, FALSE, 0.0, 0.0);
							gtk_tree_selection_unselect_all(select);
							gtk_tree_selection_select_iter(select, &iter);
							gtk_tree_path_free(path);
							break;
						}
					} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(historystore), &iter));
					break;
				}
			}
		}
	}
	gtk_widget_destroy(d);
}
void on_popup_menu_item_history_bookmark_activate(GtkMenuItem *w, gpointer) {
	if(calculator_busy()) return;
	if(strcmp(gtk_menu_item_get_label(w), _("Remove Bookmark")) == 0) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		GList *selected_list;
		gint hindex = -1;
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
		selected_list = gtk_tree_selection_get_selected_rows(select, &model);
		if(!selected_list) return;
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) selected_list->data);
		while(true) {
			gtk_tree_model_get(model, &iter, 1, &hindex, -1);
			if(hindex >= 0 && inhistory_type[hindex] == QALCULATE_HISTORY_BOOKMARK) break;
			if(!gtk_tree_model_iter_previous(model, &iter)) {
				hindex = -1;
				break;
			}
		}
		if(hindex >= 0) {
			for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
				if(equalsIgnoreCase(inhistory[hindex], *it)) {
					history_bookmarks.erase(it);
					break;
				}
			}
			inhistory.erase(inhistory.begin() + hindex);
			inhistory_protected.erase(inhistory_protected.begin() + hindex);
			inhistory_type.erase(inhistory_type.begin() + hindex);
			inhistory_time.erase(inhistory_time.begin() + hindex);
			inhistory_value.erase(inhistory_value.begin() + hindex);
			GtkTreeIter history_iter = iter;
			if(gtk_tree_model_iter_next(GTK_TREE_MODEL(historystore), &history_iter)) {
				gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 1, &hindex, -1);
				if(!history_protected(hindex)) {
					gchar *gstr;
					gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 0, &gstr, -1);
					string str = gstr;
					size_t i = str.rfind("<span size=\"small\"><sup> ");
					if(i == string::npos) i = str.rfind("<span size=\"x-small\"><sup> ");
					if(i != string::npos) str = str.substr(0, i);
					gtk_list_store_set(historystore, &history_iter, 0, str.c_str(), -1);
					g_free(gstr);
				}
			}
			history_iter = iter;
			while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &history_iter)) {
				gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 1, &hindex, -1);
				if(hindex >= 0) gtk_list_store_set(historystore, &history_iter, 1, hindex - 1, -1);
			}
			gtk_list_store_remove(historystore, &iter);
			set_expression_modified(true, true, true);
			if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
		}
		g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
	} else {
		string history_message;
		GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Add Bookmark"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_REJECT, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
		gtk_widget_show(hbox);
		GtkWidget *label = gtk_label_new(_("Name"));
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_widget_show(label);
		GtkWidget *entry = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(entry), 35);
		gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
		gtk_widget_show(entry);
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			string history_message = gtk_entry_get_text(GTK_ENTRY(entry));
			remove_blank_ends(history_message);
			bool b = false;
			for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
				if(equalsIgnoreCase(history_message, *it)) {
					b = true;
					break;
				}
			}
			if(b) {
				if(ask_question(_("A bookmark with the selected name already exists.\nDo you want to overwrite it?"), GTK_WINDOW(dialog))) {
					remove_history_bookmark(history_message);
				} else {
					history_message = "";
				}
			}
			if(!history_message.empty()) {
				add_history_bookmark(history_message);
				if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
			}
		}
		gtk_widget_destroy(dialog);
	}
}
bool history_protected_by_bookmark(size_t hi) {
	if(inhistory_type[hi] == QALCULATE_HISTORY_BOOKMARK) return true;
	while(hi + 1 < inhistory_type.size() && HISTORY_NOT_EXPRESSION(hi)) {
		hi++;
		if(inhistory_type[hi] == QALCULATE_HISTORY_BOOKMARK) return true;
	}
	if(hi + 1 < inhistory_type.size() && inhistory_type[hi + 1] == QALCULATE_HISTORY_BOOKMARK) return true;
	return false;
}
bool history_protected(size_t hi) {
	if(inhistory_protected[hi]) return true;
	while(hi + 1 < inhistory_type.size() && HISTORY_NOT_EXPRESSION(hi) && inhistory_type[hi] != QALCULATE_HISTORY_OLD) {
		hi++;
	}
	return inhistory_protected[hi];
}
void on_popup_menu_item_history_protect_toggled(GtkCheckMenuItem *w, gpointer) {
	if(calculator_busy()) return;
	bool b = gtk_check_menu_item_get_active(w);
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *selected_list;
	gint hi = -1, hi_pre = 0, hi_pre_next;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
	selected_list = gtk_tree_selection_get_selected_rows(select, &model);
	GList *current_selected_list = selected_list;
	while(current_selected_list) {
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) current_selected_list->data);
		gtk_tree_model_get(model, &iter, 1, &hi, -1);
		hi_pre_next = hi;
		bool b2 = true;
		while(hi >= 0 && (size_t) hi + 1 < inhistory_type.size() && ITEM_NOT_EXPRESSION(hi)) {
			if(!gtk_tree_model_iter_previous(model, &iter)) {
				b2 = false;
				break;
			}
			gtk_tree_model_get(model, &iter, 1, &hi, -1);
			if(hi == hi_pre) {
				b2 = false;
				break;
			}
		}
		if(hi >= 0 && b2) {
			if(HISTORY_IS_MESSAGE(hi)) hi++;
			if(HISTORY_IS_PARSE(hi)) hi++;
			if(b != inhistory_protected[hi]) {
				inhistory_protected[hi] = b;
				gchar *gstr;
				gtk_tree_model_get(model, &iter, 0, &gstr, -1);
				string str = gstr;
				if((size_t) hi + 1 >= inhistory_type.size() || inhistory_type[hi + 1] != QALCULATE_HISTORY_BOOKMARK) {
					if(b) {
						if(str.find("<span size=\"x-small\"><sup> ") == string::npos && str.find("<span size=\"small\"><sup> ") == string::npos) {
							if(can_display_unicode_string_function_exact("🔒", history_view_widget())) str += "<span size=\"small\"><sup> 🔒</sup></span>";
							else str += "<span size=\"x-small\"><sup> P</sup></span>";
						}
					} else {
						size_t i = str.rfind("<span size=\"small\"><sup> ");
						if(i == string::npos) i = str.rfind("<span size=\"x-small\"><sup> ");
						if(i != string::npos) str = str.substr(0, i);
					}
					gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, str.c_str(), -1);
				}
				g_free(gstr);
			}
		}
		hi_pre = hi_pre_next;
		current_selected_list = current_selected_list->next;
	}
	g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_popup_menu_history_bookmark_update_activate(GtkMenuItem*, gpointer data) {
	string str = gtk_menu_item_get_label(GTK_MENU_ITEM(data));
	if(history_bookmark_titles.count(str) > 0) str = history_bookmarks[history_bookmark_titles[str]];
	remove_history_bookmark(str);
	add_history_bookmark(str);
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")));
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_popup_menu_history_bookmark_delete_activate(GtkMenuItem*, gpointer data) {
	string str = gtk_menu_item_get_label(GTK_MENU_ITEM(data));
	if(history_bookmark_titles.count(str) > 0) str = history_bookmarks[history_bookmark_titles[str]];
	remove_history_bookmark(str);
	gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")));
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}

gulong on_popup_menu_history_bookmark_update_activate_handler = 0, on_popup_menu_history_bookmark_delete_activate_handler = 0;

gboolean on_menu_history_bookmark_popup_menu(GtkWidget*, gpointer data) {
	if(calculator_busy()) return TRUE;
	vector<size_t> selected_rows;
	process_history_selection(&selected_rows, NULL, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark_update")), selected_rows.size() == 1 && inhistory_type[selected_rows[0]] != QALCULATE_HISTORY_OLD);
	if(on_popup_menu_history_bookmark_update_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark_update"), on_popup_menu_history_bookmark_update_activate_handler);
	if(on_popup_menu_history_bookmark_delete_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark_delete"), on_popup_menu_history_bookmark_delete_activate_handler);
	on_popup_menu_history_bookmark_update_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark_update"), "activate", G_CALLBACK(on_popup_menu_history_bookmark_update_activate), data);
	on_popup_menu_history_bookmark_delete_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark_delete"), "activate", G_CALLBACK(on_popup_menu_history_bookmark_delete_activate), data);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_history_bookmark")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_menu_history_bookmark_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	/* Ignore double-clicks and triple-clicks */
	if(gdk_event_triggers_context_menu((GdkEvent *) event) && gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS) {
		on_menu_history_bookmark_popup_menu(widget, data);
		return TRUE;
	}
	return FALSE;
}

void update_historyview_popup() {
	GtkTreeIter iter;
	vector<size_t> selected_rows;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	size_t hi = 0;
	process_history_selection(&selected_rows, &selected_indeces, &selected_index_type);
	if(selected_rows.size() == 1) {
		hi = selected_rows[0];
		if(HISTORY_IS_PARSE(hi)) {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_parsed_text")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_text")), hi < inhistory_type.size() - 1 && inhistory_type[hi + 1] == QALCULATE_HISTORY_EXPRESSION);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_text")), inhistory_type[hi] != QALCULATE_HISTORY_WARNING && inhistory_type[hi] != QALCULATE_HISTORY_ERROR && inhistory_type[hi] != QALCULATE_HISTORY_MESSAGE && inhistory_type[hi] != QALCULATE_HISTORY_BOOKMARK && inhistory_type[hi] != QALCULATE_HISTORY_RPN_OPERATION && inhistory_type[hi] != QALCULATE_HISTORY_REGISTER_MOVED);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_parsed_text")), hi > 0 && HISTORY_IS_EXPRESSION(hi) && HISTORY_IS_PARSE(hi - 1));
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_text")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_parsed_text")), FALSE);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_value")), selected_indeces.size() > 0 && selected_index_type[0] != INDEX_TYPE_TXT && selected_index_type.back() != INDEX_TYPE_TXT);
	bool default_insert_value = selected_indeces.size() > 0 && selected_index_type[0] != INDEX_TYPE_TXT && selected_index_type.back() != INDEX_TYPE_TXT && (selected_indeces.size() >= 2 || selected_index_type[0] == INDEX_TYPE_ANS);
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_value")))), default_insert_value ? GDK_KEY_Return : 0, (GdkModifierType) 0);
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_text")))), (!default_insert_value && selected_rows.size() > 0) ? GDK_KEY_Return : 0, (GdkModifierType) 0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_text")), selected_indeces.size() == 1 && inhistory_type[hi] != QALCULATE_HISTORY_BOOKMARK);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_ascii")), selected_indeces.size() == 1 && inhistory_type[hi] != QALCULATE_HISTORY_BOOKMARK);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_full_text")), !selected_rows.empty());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_movetotop")), !selected_rows.empty());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_delete")), !selected_rows.empty());
	bool protected_by_bookmark = true, b_protected = true, b_old = false;
	for(size_t i = 0; i < selected_rows.size(); i++) {
		if(!b_old && inhistory_type[selected_rows[i]] == QALCULATE_HISTORY_OLD) {b_old = true; b_protected = false; break;}
		if(b_protected) {
			if(history_protected(selected_rows[i])) {
				protected_by_bookmark = false;
			} else if(!history_protected_by_bookmark(selected_rows[i])) {
				protected_by_bookmark = false;
				b_protected = false;
			}
		}
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_protect")), selected_rows.size() > 0 && !b_old && !protected_by_bookmark);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_history_protect"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_history_protect_toggled, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_history_protect")), selected_rows.size() > 0 && b_protected);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_history_protect"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_history_protect_toggled, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_bookmark")), selected_rows.size() == 1 && hi >= 0 && inhistory_type[hi] != QALCULATE_HISTORY_OLD && (HISTORY_NOT_MESSAGE(hi) || ITEM_NOT_EXPRESSION(hi)));
	if(selected_rows.size() == 1 && history_protected_by_bookmark(hi)) gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_history_bookmark")), _("Remove Bookmark"));
	else gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_history_bookmark")), _("Add Bookmark…"));
	if(selected_rows.size() == 1 && inhistory_time[hi] != 0) {
		struct tm tmdate = *localtime(&inhistory_time[hi]);
		string str = _("Search by Date…");
		if(pango_version() >= 13800) str += "<span fgalpha=\"60%\">";
		str += " (";
		str += i2s(tmdate.tm_year + 1900);
		str += "-";
		if(tmdate.tm_mon < 10) str += "0";
		str += i2s(tmdate.tm_mon);
		str += "-";
		if(tmdate.tm_mday < 10) str += "0";
		str += i2s(tmdate.tm_mday);
		str += ")";
		if(pango_version() >= 13800) str+= "</span>";
		gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_search_date")))), true);
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_history_search_date")), str.c_str());
	} else {
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_history_search_date")), _("Search by Date…"));
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_clear")), gtk_tree_model_get_iter_first(GTK_TREE_MODEL(historystore), &iter));
	gtk_container_foreach(GTK_CONTAINER(gtk_builder_get_object(main_builder, "popup_menu_history_bookmarks")), (GtkCallback) gtk_widget_destroy, NULL);
	GtkWidget *item;
	GtkWidget *sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_history_bookmarks"));
	history_bookmark_titles.clear();
	for(size_t i = 0; i < history_bookmarks.size(); i++) {
		if(history_bookmarks[i].length() > 70) {
			string label = history_bookmarks[i].substr(0, 70);
			label += "…";
			MENU_ITEM(label.c_str(), goto_history_bookmark)
			history_bookmark_titles[label] = i;
		} else {
			MENU_ITEM(history_bookmarks[i].c_str(), goto_history_bookmark)
		}
		g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_history_bookmark_button_press), (gpointer) item);
		g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_history_bookmark_popup_menu), (gpointer) item);
	}
	if(history_bookmarks.empty()) {MENU_NO_ITEMS(_("No items found"))}
}
void on_historyview_item_edited(GtkCellRendererText*, gchar*, gchar*, gpointer) {
	b_editing_history = false;
}
void on_historyview_item_editing_started(GtkCellRenderer*, GtkCellEditable *editable, gchar*, gpointer) {
	gtk_editable_set_editable(GTK_EDITABLE(editable), FALSE);
	b_editing_history = true;
}
void on_historyview_item_editing_canceled(GtkCellRenderer*, gpointer) {
	b_editing_history = false;
}
bool editing_history() {
	return b_editing_history;
}
void on_historyview_row_activated(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn *column, gpointer);
bool do_history_edit = false;
guint historyedit_timeout_id = 0;
GtkTreePath *historyedit_path = NULL;
gboolean do_historyedit_timeout(gpointer) {
	historyedit_timeout_id = 0;
	if(gtk_tree_selection_path_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())), historyedit_path)) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(history_view_widget()), historyedit_path, history_column, TRUE);
	}
	gtk_tree_path_free(historyedit_path);
	historyedit_path = NULL;
	return FALSE;
}
gboolean on_historyview_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0; GdkModifierType state;
	gdouble x = 0, y = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	gdk_event_get_state((GdkEvent*) event, &state);
	gdk_event_get_coords((GdkEvent*) event, &x, &y);
	if(historyedit_timeout_id) {g_source_remove(historyedit_timeout_id); historyedit_timeout_id = 0; gtk_tree_path_free(historyedit_path); historyedit_path = NULL;}
	if(!do_history_edit) return FALSE;
	do_history_edit = false;
	if(button != 1 || b_editing_history || CLEAN_MODIFIERS(state) != 0) return FALSE;
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkTreeSelection *select = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(history_view_widget()), x, y, &path, &column, NULL, NULL)) {
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
		if(column == history_column && gtk_tree_selection_path_is_selected(select, path)) {
			historyedit_path = path;
			historyedit_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 250, do_historyedit_timeout, NULL, NULL);
		} else {
			gtk_tree_path_free(path);
		}
	}
	return FALSE;
}
gboolean on_historyview_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	GdkModifierType state; guint keyval = 0;
	gdk_event_get_state((GdkEvent*) event, &state);
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
	state = CLEAN_MODIFIERS(state);
	FIX_ALT_GR
	if(state == 0 && keyval == GDK_KEY_F2) {
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
		if(gtk_tree_selection_count_selected_rows(select) == 1) {
			GList *selected_list = gtk_tree_selection_get_selected_rows(select, NULL);
			if(historyedit_timeout_id) {g_source_remove(historyedit_timeout_id); historyedit_timeout_id = 0; gtk_tree_path_free(historyedit_path); historyedit_path = NULL;}
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(history_view_widget()), (GtkTreePath*) selected_list->data, gtk_tree_view_get_column(GTK_TREE_VIEW(history_view_widget()), 1), TRUE);
			g_list_free_full(selected_list, (GDestroyNotify) gtk_tree_path_free);
			return TRUE;
		}
	} else if(state == 0 && (keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_Return)) {
		vector<size_t> selected_rows;
		vector<size_t> selected_indeces;
		vector<int> selected_index_type;
		process_history_selection(&selected_rows, &selected_indeces, &selected_index_type);
		if(selected_rows.empty()) return FALSE;
		if(selected_indeces.size() > 0 && selected_index_type[0] != INDEX_TYPE_TXT && selected_index_type.back() != INDEX_TYPE_TXT && (selected_indeces.size() >= 2 || selected_index_type[0] == INDEX_TYPE_ANS)) {
			on_button_history_insert_value_clicked(NULL, NULL);
		} else {
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(history_view_widget()), &path, &column);
			if(path) {
				on_historyview_row_activated(GTK_TREE_VIEW(history_view_widget()), path, column, NULL);
				gtk_tree_path_free(path);
			}
		}
		return TRUE;
	} else if(state == GDK_CONTROL_MASK && keyval == GDK_KEY_c) {
		history_copy(false);
		return TRUE;
	} else if(state == GDK_SHIFT_MASK && keyval == GDK_KEY_Delete) {
		on_popup_menu_item_history_delete_activate(NULL, NULL);
		return TRUE;
	}
	return FALSE;
}
gboolean on_historyview_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	GdkModifierType state;
	gdouble x = 0, y = 0;
	gdk_event_get_state((GdkEvent*) event, &state);
	gdk_event_get_coords((GdkEvent*) event, &x, &y);
	do_history_edit = false;
	if(historyedit_timeout_id) {g_source_remove(historyedit_timeout_id); historyedit_timeout_id = 0; gtk_tree_path_free(historyedit_path); historyedit_path = NULL;}
	state = CLEAN_MODIFIERS(state);
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkTreeSelection *select = NULL;
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && gdk_event_get_event_type((GdkEvent*) event) == GDK_BUTTON_PRESS) {
		if(calculator_busy()) return TRUE;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(history_view_widget()), x, y, &path, NULL, NULL, NULL)) {
			select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
			if(!gtk_tree_selection_path_is_selected(select, path)) {
				gtk_tree_selection_unselect_all(select);
				gtk_tree_selection_select_path(select, path);
			}
			gtk_tree_path_free(path);
		}
		update_historyview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), (GdkEvent*) event);
#else
		guint button = 0;
		gdk_event_get_button((GdkEvent*) event, &button);
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), NULL, NULL, NULL, NULL, button, gdk_event_get_time((GdkEvent*) event));
#endif
		gtk_widget_grab_focus(history_view_widget());
		return TRUE;
	} else if(gdk_event_get_event_type((GdkEvent*) event) == GDK_2BUTTON_PRESS) {
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(history_view_widget()), x, y, &path, &column, NULL, NULL)) {
			on_historyview_row_activated(GTK_TREE_VIEW(history_view_widget()), path, column, NULL);
			gtk_tree_path_free(path);
			return TRUE;
		}
	} else {
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(history_view_widget()), x, y, &path, NULL, NULL, NULL)) {
			select = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
			if(gtk_tree_selection_path_is_selected(select, path)) {
				gtk_tree_path_free(path);
				if(state != 0) return FALSE;
				do_history_edit = true;
				return TRUE;
			} else if(state == 0) {
				GtkTreePath *path2;
				gtk_tree_view_get_cursor(GTK_TREE_VIEW(history_view_widget()), &path2, &column);
				if(path2 && gtk_tree_path_compare(path, path2) == 0) {
					gtk_tree_selection_unselect_all(select);
					gtk_tree_selection_select_path(select, path);
					gtk_tree_path_free(path);
					gtk_tree_path_free(path2);
					return TRUE;
				}
				if(path2) gtk_tree_path_free(path2);
			}
			gtk_tree_path_free(path);
		}
	}
	return FALSE;
}

gboolean on_historyview_popup_menu(GtkWidget*, gpointer) {
	if(calculator_busy()) return TRUE;
	update_historyview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}
void on_historyview_selection_changed(GtkTreeSelection*, gpointer) {
	do_history_edit = false;
	if(historyedit_timeout_id) {g_source_remove(historyedit_timeout_id); historyedit_timeout_id = 0; gtk_tree_path_free(historyedit_path); historyedit_path = NULL;}
	vector<size_t> selected_rows;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(&selected_rows, &selected_indeces, &selected_index_type);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_value")), selected_indeces.size() > 0 && selected_index_type[0] != INDEX_TYPE_TXT && selected_index_type.back() != INDEX_TYPE_TXT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_text")), selected_indeces.size() == 1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_copy")), !selected_rows.empty());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_sqrt")), selected_indeces.size() <= 1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_xy")), selected_indeces.size() <= 2);
}
void history_input_base_changed() {
	on_historyview_selection_changed(NULL, NULL);
}
void on_historyview_row_activated(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn *column, gpointer) {
	GtkTreeIter iter;
	gint index = -1, hindex = -1;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(historystore), &iter, path)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(historystore), &iter, 1, &hindex, 3, &index, -1);
	if(index > 0 && hindex >= 0 && evalops.parse_options.functions_enabled && evalops.parse_options.base <= BASE_DECIMAL && evalops.parse_options.base > 0) {
		const ExpressionName *ename = NULL;
		switch(inhistory_type[(size_t) hindex]) {
			case QALCULATE_HISTORY_RPN_OPERATION: {}
			case QALCULATE_HISTORY_REGISTER_MOVED: {
				if(hindex == 0 || column == history_index_column) {
					ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
				} else {
					insert_text(unhtmlize(inhistory[(size_t) hindex - 1]).c_str());
					return;
				}
				break;
			}
			case QALCULATE_HISTORY_PARSE: {}
			case QALCULATE_HISTORY_PARSE_APPROXIMATE: {
				if(column != history_index_column && (size_t) hindex < inhistory_type.size() - 1 && inhistory_type[hindex + 1] == QALCULATE_HISTORY_EXPRESSION) hindex++;
			}
			case QALCULATE_HISTORY_EXPRESSION: {
				if(column == history_index_column) {
					ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
				} else {
					insert_text(unhtmlize(inhistory[(size_t) hindex]).c_str());
					return;
				}
				break;
			}
			default: {
				ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			}
		}
		string str = ename->formattedName(TYPE_FUNCTION, true);
		str += "(";
		Number nr(index, 1);
		str += print_with_evalops(nr);
		str += ")";
		if(rpn_mode) {
			block_undo();
			insert_text(str.c_str());
			unblock_undo();
			execute_expression();
		} else {
			insert_text(str.c_str());
		}
	} else if(hindex >= 0) {
		if(hindex > 0 && (inhistory_type[hindex] == QALCULATE_HISTORY_TRANSFORMATION || inhistory_type[hindex] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[hindex] == QALCULATE_HISTORY_REGISTER_MOVED)) hindex--;
		else if((size_t) hindex < inhistory_type.size() - 1 && (inhistory_type[hindex] == QALCULATE_HISTORY_PARSE || inhistory_type[hindex] == QALCULATE_HISTORY_PARSE_WITHEQUALS || inhistory_type[hindex] == QALCULATE_HISTORY_PARSE_APPROXIMATE) && inhistory_type[hindex + 1] == QALCULATE_HISTORY_EXPRESSION) hindex++;
		if(HISTORY_NOT_MESSAGE(hindex) && inhistory_type[hindex] != QALCULATE_HISTORY_BOOKMARK) {
			if(rpn_mode && ITEM_NOT_EXPRESSION(hindex) && inhistory_type[hindex] != QALCULATE_HISTORY_OLD) {
				block_undo();
				insert_text(unhtmlize(inhistory[(size_t) hindex]).c_str());
				unblock_undo();
				execute_expression();
			} else {
				insert_text(unhtmlize(inhistory[(size_t) hindex]).c_str());
			}
		}
	}
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}

void history_scroll_on_realized() {
	if(nr_of_new_expressions > 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(history_view_widget()), path, history_index_column, FALSE, 0, 0);
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(history_view_widget()), 0, 0);
		gtk_tree_path_free(path);
	}
}

void on_history_resize(GtkWidget*, GdkRectangle *alloc, gpointer) {
	gint hsep = 0;
	gtk_widget_style_get(history_view_widget(), "horizontal-separator", &hsep, NULL);
	int prev_hw = history_width_a;
	history_width_a = alloc->width - gtk_tree_view_column_get_width(history_index_column) - hsep * 4;
	PangoLayout *layout = gtk_widget_create_pango_layout(history_view_widget(), "");
	if(can_display_unicode_string_function_exact("🔒", history_view_widget())) pango_layout_set_markup(layout, "<span size=\"small\"><sup> 🔒</sup></span>", -1);
	else pango_layout_set_markup(layout, "<span size=\"x-small\"><sup> P</sup></span>", -1);
	gint w = 0;
	pango_layout_get_pixel_size(layout, &w, NULL);
	g_object_unref(layout);
	history_width_e = history_width_a - 6 - history_scroll_width - w;
	history_width_a -= history_scroll_width * 2;
	if(prev_hw != history_width_a) {
		gtk_tree_view_column_set_max_width(history_column, history_width_a + history_scroll_width * 2);
		reload_history();
	}
}
void update_history_button_text() {
	if(printops.use_unicode_signs) {
		if(can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_history_sub"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sub")), SIGN_MINUS);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sub")), MINUS);
		if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_history_times"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_times")), SIGN_MULTIPLICATION);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_times")), MULTIPLICATION);
		if(can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_history_divide"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), SIGN_DIVISION_SLASH);
		else if(can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_history_divide"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), SIGN_DIVISION);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), DIVISION);
	} else {

		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sub")), MINUS);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_times")), MULTIPLICATION);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), DIVISION);
	}

	FIX_SUPSUB_PRE_W(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_history_xy")));
	string s_xy = "x<sup>y</sup>";
	FIX_SUPSUB(s_xy);
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_xy")), s_xy.c_str());

	if(can_display_unicode_string_function(SIGN_SQRT, (void*) gtk_builder_get_object(main_builder, "label_history_sqrt"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sqrt")), SIGN_SQRT);
	else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sqrt")), "sqrt");
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_value")), -1, -1);
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_copy")), -1, -1);
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_add")), -1, -1);
	GtkRequisition a;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_xy")), &a, NULL);
	gint w = a.width; gint h = a.height;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_sqrt")), &a, NULL);
	if(a.width > w) w = a.width;
	if(a.height > h) h = a.height;
	if(gtk_image_get_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_insert_value"))) != -1) {
		gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_insert_value")), -1);
		gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_insert_text")), -1);
		gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_copy")), -1);
	}
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_value")), &a, NULL);
	if(a.width > w) w = a.width;
	if(a.height > h) h = a.height;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_copy")), &a, NULL);
	gint h_i = -1;
	if(app_font()) {
		h_i = 16 + (h - a.height);
		if(h_i < 20) h_i = -1;
	}
	if(h_i != -1) {
		gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_insert_value")), h_i);
		gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_insert_text")), h_i);
		gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_history_copy")), h_i);
	}
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_value")), w, h);
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_copy")), w, h);
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_add")), w, h);
}

void update_history_accels(int type) {
	bool b = false;
	for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
		if(it->second.type.size() != 1 || (type >= 0 && it->second.type[0] != type)) continue;
		b = true;
		switch(it->second.type[0]) {
			case SHORTCUT_TYPE_HISTORY_SEARCH: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_search")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_HISTORY_CLEAR: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_clear")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
		}
		if(type >= 0) break;
	}
	if(!b) {
		switch(type) {
			case SHORTCUT_TYPE_HISTORY_SEARCH: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_search")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_HISTORY_CLEAR: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_clear")))), 0, (GdkModifierType) 0); break;}
		}
	}
	if(type == SHORTCUT_TYPE_COPY_RESULT) {
		if(copy_ascii) {
			gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_ascii")))), GDK_KEY_c, GDK_CONTROL_MASK);
			gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_text")))), 0, (GdkModifierType) 0);
		} else {
			gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_text")))), GDK_KEY_c, GDK_CONTROL_MASK);
			gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_ascii")))), 0, (GdkModifierType) 0);
		}
	}
}

void initialize_history_functions() {
	f_answer = CALCULATOR->addFunction(new AnswerFunction());
	f_expression = CALCULATOR->addFunction(new ExpressionFunction());
}

void create_history_view() {

	if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 22) || (version_numbers[0] == 3 && version_numbers[1] == 22 && version_numbers[2] < 1)) unformatted_history = 1;
	initial_inhistory_index = inhistory.size() - 1;

	history_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(history_view_widget()), GTK_STYLE_PROVIDER(history_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	update_history_font(true);

	update_history_button_text();

	GList *l;
	GList *list;
	CHILDREN_SET_FOCUS_ON_CLICK("historyactions")
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_history_copy"));
	CHILDREN_SET_FOCUS_ON_CLICK("box_ho")

	if(themestr == "Breeze" || themestr == "Breeze-Dark") {

		GtkCssProvider *link_style_top = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_top, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_bot = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bot, "* {border-top-left-radius: 0; border-top-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_mid = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_mid, "* {border-radius: 0;}", -1, NULL);

		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_hi"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ho"))), "linked");

		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_value"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_insert_text"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_add"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_sub"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_times"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_divide"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_xy"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_history_sqrt"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	}

#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION <= 18
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 18) && (themestr == "Ambiance" || themestr == "Radiance")) {
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ho"))), "linked");
	}
#endif

// Fix for breeze-gtk and Ubuntu theme
	if(RUNTIME_CHECK_GTK_VERSION(3, 20) && themestr.substr(0, 7) != "Adwaita" && themestr.substr(0, 5) != "oomox" && themestr.substr(0, 6) != "themix" && themestr != "Yaru") {
		GtkCssProvider *historyview_provider = gtk_css_provider_new();
		gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(history_view_widget()), GTK_TREE_VIEW_GRID_LINES_NONE);
		gtk_style_context_add_provider(gtk_widget_get_style_context(historyview), GTK_STYLE_PROVIDER(historyview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		if(themestr == "Breeze") {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		} else if(themestr == "Breeze-Dark") {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		} else {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		}
	}

	historystore = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(history_view_widget()), GTK_TREE_MODEL(historystore));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	history_index_renderer = gtk_cell_renderer_text_new();
	history_index_column = gtk_tree_view_column_new_with_attributes(_("Index"), history_index_renderer, "text", 2, "ypad", 4, NULL);
	gtk_tree_view_column_set_expand(history_index_column, FALSE);
	gtk_tree_view_column_set_min_width(history_index_column, 30);
	g_object_set(G_OBJECT(history_index_renderer), "ypad", 0, "yalign", 0.0, "xalign", 0.5, "foreground-rgba", &history_gray, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(history_view_widget()), history_index_column);
	history_renderer = gtk_cell_renderer_text_new();
	g_signal_connect((gpointer) history_renderer, "edited", G_CALLBACK(on_historyview_item_edited), NULL);
	g_signal_connect((gpointer) history_renderer, "editing-started", G_CALLBACK(on_historyview_item_editing_started), NULL);
	g_signal_connect((gpointer) history_renderer, "editing-canceled", G_CALLBACK(on_historyview_item_editing_canceled), NULL);
	g_object_set(G_OBJECT(history_renderer), "editable", true, NULL);
	history_column = gtk_tree_view_column_new_with_attributes(_("History"), history_renderer, "markup", 0, "ypad", 4, "xpad", 5, "xalign", 6, "alignment", 7, NULL);
	gtk_tree_view_column_set_expand(history_column, TRUE);
	GtkWidget *scrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "historyscrolled")));
	if(scrollbar) gtk_widget_get_preferred_width(scrollbar, NULL, &history_scroll_width);
	if(history_scroll_width == 0) history_scroll_width = 3;
	history_scroll_width += 1;
	gtk_tree_view_append_column(GTK_TREE_VIEW(history_view_widget()), history_column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_historyview_selection_changed), NULL);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(history_view_widget()), history_row_separator_func, NULL, NULL);
	g_signal_connect_after(gtk_builder_get_object(main_builder, "historyscrolled"), "size-allocate", G_CALLBACK(on_history_resize), NULL);

	if(!copy_ascii) gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_text")))), GDK_KEY_c, GDK_CONTROL_MASK);
	else gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_ascii")))), GDK_KEY_c, GDK_CONTROL_MASK);
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_delete")))), GDK_KEY_Delete, GDK_SHIFT_MASK);

	gtk_builder_add_callback_symbols(main_builder, "on_historyview_button_press_event", G_CALLBACK(on_historyview_button_press_event), "on_historyview_button_release_event", G_CALLBACK(on_historyview_button_release_event), "on_historyview_key_press_event", G_CALLBACK(on_historyview_key_press_event), "on_historyview_popup_menu", G_CALLBACK(on_historyview_popup_menu), "on_historyview_row_activated", G_CALLBACK(on_historyview_row_activated), "on_button_history_insert_value_clicked", G_CALLBACK(on_button_history_insert_value_clicked), "on_button_history_insert_text_clicked", G_CALLBACK(on_button_history_insert_text_clicked), "on_button_history_copy_clicked", G_CALLBACK(on_button_history_copy_clicked), "on_button_history_add_clicked", G_CALLBACK(on_button_history_add_clicked), "on_button_history_sub_clicked", G_CALLBACK(on_button_history_sub_clicked), "on_button_history_times_clicked", G_CALLBACK(on_button_history_times_clicked), "on_button_history_divide_clicked", G_CALLBACK(on_button_history_divide_clicked), "on_button_history_xy_clicked", G_CALLBACK(on_button_history_xy_clicked), "on_button_history_sqrt_clicked", G_CALLBACK(on_button_history_sqrt_clicked), "on_popup_menu_item_history_insert_value_activate", G_CALLBACK(on_popup_menu_item_history_insert_value_activate), "on_popup_menu_item_history_insert_text_activate", G_CALLBACK(on_popup_menu_item_history_insert_text_activate), "on_popup_menu_item_history_insert_parsed_text_activate", G_CALLBACK(on_popup_menu_item_history_insert_parsed_text_activate), "on_popup_menu_item_history_copy_text_activate", G_CALLBACK(on_popup_menu_item_history_copy_text_activate), "on_popup_menu_item_history_copy_ascii_activate", G_CALLBACK(on_popup_menu_item_history_copy_ascii_activate), "on_popup_menu_item_history_copy_full_text_activate", G_CALLBACK(on_popup_menu_item_history_copy_full_text_activate), "on_popup_menu_item_history_search_activate", G_CALLBACK(on_popup_menu_item_history_search_activate), "on_popup_menu_item_history_search_date_activate", G_CALLBACK(on_popup_menu_item_history_search_date_activate), "on_popup_menu_item_history_bookmark_activate", G_CALLBACK(on_popup_menu_item_history_bookmark_activate), "on_popup_menu_item_history_protect_toggled", G_CALLBACK(on_popup_menu_item_history_protect_toggled), "on_popup_menu_item_history_movetotop_activate", G_CALLBACK(on_popup_menu_item_history_movetotop_activate), "on_popup_menu_item_history_delete_activate", G_CALLBACK(on_popup_menu_item_history_delete_activate), "on_popup_menu_item_history_clear_activate", G_CALLBACK(on_popup_menu_item_history_clear_activate), NULL);

}
