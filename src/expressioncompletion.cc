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
#include <cairo/cairo-gobject.h>

#include "support.h"
#include "settings.h"
#include "util.h"
#include "mainwindow.h"
#include "resultview.h"
#include "expressionedit.h"
#include "expressionstatus.h"
#include "expressioncompletion.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

extern GtkBuilder *main_builder;

GtkEntryCompletion *completion;
GtkWidget *completion_view, *completion_window, *completion_scrolled;
GtkTreeModel *completion_filter, *completion_sort;
GtkListStore *completion_store;

unordered_map<const ExpressionName*, string> capitalized_names;

extern GtkCssProvider *expression_provider;

int completion_min = 1, completion_min2 = 1;
bool enable_completion = true, enable_completion2 = true;
guint completion_timeout_id = 0;
int completion_delay = 0;
bool fix_supsub_completion = false;

bool editing_to_expression = false, editing_to_expression1 = false, current_object_in_quotes = false;
gint current_object_start = -1, current_object_end = -1;
bool current_object_has_changed = false;
extern MathStructure *current_from_struct;
extern vector<Unit*> current_from_units;

GtkTreeIter tabbed_iter;
bool tabbed_completion = false;
gint tabbed_completion_start = -1, tabbed_completion_end = -1;
string tabbed_completion_object;

int completion_blocked = 0;

void get_expression_completion_settings(bool *enable1, bool *enable2, int *min1, int *min2, int *delay) {
	if(enable1) *enable1 = enable_completion;
	if(enable2) *enable2 = enable_completion2;
	if(min1) *min1 = completion_min;
	if(min2) *min2 = completion_min2;
	if(delay) *delay = completion_delay;
}
void set_expression_completion_settings(int enable1, int enable2, int min1, int min2, int delay) {
	if(enable1 >= 0) enable_completion = enable1;
	if(enable2 >= 0) enable_completion2 = enable2;
	if(min1 >= 0) completion_min = min1;
	if(min2 >= 0) completion_min2 = min2;
	if(delay >= 0) completion_delay = delay;
	if(completion_min2 < completion_min) {
		if(min1 >= 0) completion_min2 = completion_min;
		else completion_min = completion_min2;
	}
}

bool read_expression_completion_settings_line(string &svar, string&, int &v) {
	if(svar == "enable_completion") {
		enable_completion = v;
	} else if(svar == "enable_completion2") {
		enable_completion2 = v;
	} else if(svar == "completion_min") {
		if(v < 1) v = 1;
		completion_min = v;
	} else if(svar == "completion_min2") {
		if(v < 1) v = 1;
		completion_min2 = v;
	} else if(svar == "completion_delay") {
		if(v < 0) v = 0;
		completion_delay = v;
	} else {
		return false;
	}
	return true;
}
void write_expression_completion_settings(FILE *file) {
	fprintf(file, "enable_completion=%i\n", enable_completion);
	fprintf(file, "enable_completion2=%i\n", enable_completion2);
	fprintf(file, "completion_min=%i\n", completion_min);
	fprintf(file, "completion_min2=%i\n", completion_min2);
	fprintf(file, "completion_delay=%i\n", completion_delay);
}

gboolean completion_row_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer) {
	gint i;
	gtk_tree_model_get(model, iter, 4, &i, -1);
	return i == 3;
}

gint completion_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
	gint i1 = 0, i2 = 0;
	gtk_tree_model_get(model, a, 4, &i1, -1);
	gtk_tree_model_get(model, b, 4, &i2, -1);
	if(i1 < i2) return -1;
	if(i1 > i2) return 1;
	gchar *gstr1, *gstr2;
	gint retval;
	gint cid = GPOINTER_TO_INT(user_data);
	gtk_tree_model_get(model, a, cid, &gstr1, -1);
	gtk_tree_model_get(model, b, cid, &gstr2, -1);
	gchar *gstr1c = g_utf8_casefold(gstr1, -1);
	gchar *gstr2c = g_utf8_casefold(gstr2, -1);
	retval = g_utf8_collate(gstr1c, gstr2c);
	g_free(gstr1c);
	g_free(gstr2c);
	g_free(gstr1);
	g_free(gstr2);
	return retval;
}

bool title_matches(ExpressionItem *item, const string &str, size_t minlength) {
	bool big_A = false;
	if(minlength > 1 && str.length() == 1) {
		if(str[0] == 'a' || str[0] == 'x' || str[0] == 'y' || str[0] == 'X' || str[0] == 'Y') return false;
		big_A = (str[0] == 'A');
	}
	const string &title = item->title(false);
	size_t i = 0;
	while(true) {
		while(true) {
			if(i >= title.length()) return false;
			if(title[i] != ' ' && title[i] != '(') break;
			i++;
		}
		size_t i2 = title.find(' ', i);
		if(big_A && title[i] == str[0] && ((i2 == string::npos && i == title.length() - 1) || i2 - i == 1)) {
			return true;
		} else if(!big_A && equalsIgnoreCase(str, title, i, i2, minlength)) {
			return true;
		}
		if(i2 == string::npos) break;
		i = i2 + 1;
	}
	return false;
}
bool test_unicode_length_from(const string &str, size_t i, size_t l) {
	if(l == 0) return true;
	for(; i < str.length(); i++) {
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			l--;
			if(l == 0) return true;
		}
	}
	return false;
}
bool name_matches(ExpressionItem *item, const string &str) {
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		const ExpressionName *ename = &item->getName(i2);
		if(ename->case_sensitive) {
			if(str == ename->name.substr(0, str.length())) {
				return true;
			}
		} else {
			if(equalsIgnoreCase(str, ename->name, 0, str.length(), 0)) {
				return true;
			}
			unordered_map<const ExpressionName*, string>::iterator cap_it = capitalized_names.find(ename);
			if(cap_it != capitalized_names.end() && equalsIgnoreCase(str, cap_it->second, 0, str.length(), 0)) {
				return true;
			}
		}
		size_t i = 0;
		while(true) {
			i = ename->name.find("_", i);
			if(i == string::npos || !test_unicode_length_from(ename->name, i + 1, 2)) break;
			i++;
			if((ename->case_sensitive && str == ename->name.substr(i, str.length())) || (!ename->case_sensitive && equalsIgnoreCase(str, ename->name, i, str.length() + i, 0))) {
				return true;
			}
		}
	}
	return false;
}
int name_matches2(ExpressionItem *item, const string &str, size_t minlength, size_t *i_match = NULL) {
	if(minlength > 1 && !test_unicode_length_from(str, 0, 2)) return 0;
	bool b_match = false;
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		const ExpressionName *ename = &item->getName(i2);
		if(equalsIgnoreCase(str, ename->name, 0, str.length(), 0)) {
			if(!ename->case_sensitive && ename->name.length() == str.length()) {
				if(i_match) *i_match = i2;
				return 1;
			}
			if(i_match && *i_match == 0) *i_match = i2;
			b_match = true;
		}
		size_t i = 0;
		while(true) {
			i = ename->name.find("_", i);
			if(i == string::npos || !test_unicode_length_from(ename->name, i + 1, 2)) break;
			i++;
			if((ename->case_sensitive && str.length() <= ename->name.length() - i && str == ename->name.substr(i, str.length())) || (!ename->case_sensitive && equalsIgnoreCase(str, ename->name, i, str.length() + i, minlength))) {
				b_match = true;
				break;
			}
		}
	}
	return b_match ? 2 : 0;
}
bool country_matches(Unit *u, const string &str, size_t minlength) {
	const string &countries = u->countries();
	size_t i = 0;
	while(true) {
		while(true) {
			if(i >= countries.length()) return false;
			if(countries[i] != ' ') break;
			i++;
		}
		size_t i2 = countries.find(',', i);
		if(equalsIgnoreCase(str, countries, i, i2, minlength)) {
			return true;
		}
		if(i2 == string::npos) break;
		i = i2 + 1;
	}
	return false;
}
int completion_names_match(string name, const string &str, size_t minlength = 0, size_t *i_match = NULL) {
	size_t i = 0, n = 0;
	bool b_match = false;
	while(true) {
		size_t i2 = name.find(i == 0 ? " <i>" : "</i>", i);
		if(equalsIgnoreCase(str, name, i, i2, minlength)) {
			if((i2 == string::npos && name.length() - i == str.length()) || (i2 != string::npos && i2 - i == str.length())) {
				if(i_match) *i_match = n;
				return 1;
			}
			if(i_match && *i_match == 0) *i_match = n + 1;
			b_match = true;
		}
		if(i2 == string::npos) break;
		if(i == 0) {
			i = i2 + 4;
		} else {
			i = name.find("<i>", i2);
			if(i == string::npos) break;
			i += 3;
		}
		n++;
	}
	if(i_match && *i_match > 0) *i_match -= 1;
	return (b_match ? 2 : 0);
}

void block_completion() {
	gtk_widget_hide(completion_window);
	completion_blocked++;
}
void unblock_completion() {
	completion_blocked--;
}
bool completion_visible() {
	return gtk_widget_get_visible(completion_window);
}
gboolean hide_completion() {
	gtk_widget_hide(completion_window);
	return FALSE;
}

bool is_digit_q(char c, int base) {
	// 0-9 is always treated as a digit
	if(c >= '0' && c <= '9') return true;
	// in non standard bases every character might be a digit
	if(base < 0) return true;
	if(base <= 10) return false;
	// duodecimal bases uses 0-9, E, X
	if(base == 12) return c == 'E' || c == 'X' || c == 'A' || c == 'B' || c == 'a' || c == 'b';
	// bases 11-36 is case insensitive
	if(base <= 36) {
		if(c >= 'a' && c < 'a' + (base - 10)) return true;
		if(c >= 'A' && c < 'A' + (base - 10)) return true;
		return false;
	}
	// bases 37-62 is case sensitive
	if(base <= 62) {
		if(c >= 'a' && c < 'a' + (base - 36)) return true;
		if(c >= 'A' && c < 'Z') return true;
		return false;
	}
	return true;
}

void set_current_object() {
	if(!current_object_has_changed) return;
	while(gtk_events_pending()) gtk_main_iteration();
	GtkTextIter ipos, istart, iend;
	gint pos, pos2;
	g_object_get(expression_edit_buffer(), "cursor-position", &pos, NULL);
	pos2 = pos;
	current_object_in_quotes = false;
	if(pos == 0) {
		current_object_start = -1;
		current_object_end = -1;
		editing_to_expression = false;
		return;
	}
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &ipos, pos);
	gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &ipos, FALSE);
	gchar *p = gstr + strlen(gstr);
	size_t l_to = strlen(gstr);
	editing_to_expression = CALCULATOR->hasToExpression(gstr, !auto_calculate || rpn_mode || parsed_in_result, evalops);
	if(!editing_to_expression && ((evalops.parse_options.base > 10 && evalops.parse_options.base <= 36) || evalops.parse_options.base == BASE_UNICODE || evalops.parse_options.base == BASE_BIJECTIVE_26 || (evalops.parse_options.base == BASE_CUSTOM && (CALCULATOR->customInputBase() > 10 || CALCULATOR->customInputBase() < -10)))) {
		g_free(gstr);
		current_object_start = -1;
		current_object_end = -1;
		editing_to_expression = false;
		return;
	}
	if(l_to > 0) {
		if(gstr[0] == '/') {
			g_free(gstr);
			current_object_start = -1;
			current_object_end = -1;
			editing_to_expression = false;
			return;
		}
		bool cit1 = false, cit2 = false, hex = false, duo = false, after_space = false;
		for(size_t i = 0; i < l_to; i++) {
			if(!cit1 && gstr[i] == '\"') {
				cit2 = !cit2;
				hex = false;
				duo = false;
			} else if(!cit2 && gstr[i] == '\'') {
				cit1 = !cit1;
				hex = false;
				duo = false;
			} else if(!hex && !cit1 && !cit2 && i + 2 < l_to && gstr[i] == '0' && gstr[i + 1] == 'x' && is_digit_q(gstr[i + 2], 16)) {
				hex = true;
				duo = false;
				after_space = false;
				i += 2;
			} else if(!hex && !duo && !cit1 && !cit2 && i + 3 < l_to && gstr[i] == '0' && gstr[i + 1] == 'd' && is_digit_q(gstr[i + 2], 12) && is_digit_q(gstr[i + 3], 12)) {
				duo = true;
				after_space = false;
				i += 3;
			} else if(!cit1 && !cit2 && gstr[i] == '#') {
				g_free(gstr);
				current_object_start = -1;
				current_object_end = -1;
				editing_to_expression = false;
				return;
			} else if((hex || duo) && gstr[i] == ' ') {
				after_space = true;
			} else if((hex || duo) && after_space && (gstr[i] < '0' || gstr[i] < '9')) {
				after_space = false;
			} else if((hex || duo) && ((after_space && !is_digit_q(gstr[i], hex ? 16 : 12)) || !CALCULATOR->utf8_pos_is_valid_in_name(gstr + sizeof(gchar) * i))) {
				duo = false;
				hex = false;
			}
		}
		if(hex || duo) {
			g_free(gstr);
			current_object_start = -1;
			current_object_end = -1;
			editing_to_expression = false;
			return;
		}
		current_object_in_quotes = !editing_to_expression && (cit1 || cit2);
	}
	if(editing_to_expression) {
		string str = gstr, str_to;
		bool b_space = is_in(SPACES, str[str.length() - 1]);
		bool b_first = true;
		do {
			CALCULATOR->separateToExpression(str, str_to, evalops, true, !auto_calculate || rpn_mode || parsed_in_result);
			if(b_first && str.empty()) {
				if(current_from_struct) current_from_struct->unref();
				current_from_struct = current_result();
				if(current_from_struct) {
					current_from_struct->ref();
					find_matching_units(*current_from_struct, current_parsed_result(), current_from_units);
				}
			}
			b_first = false;
			str = str_to;
			if(!str_to.empty() && b_space) str += " ";
		} while(CALCULATOR->hasToExpression(str, !auto_calculate || rpn_mode || parsed_in_result, evalops));
		l_to = str_to.length();
	}
	bool non_number_before = false;
	while(pos2 > 0 && l_to > 0) {
		pos2--;
		l_to--;
		p = g_utf8_prev_char(p);
		if(!CALCULATOR->utf8_pos_is_valid_in_name(p)) {
			if(p[0] == '\\' || (current_object_in_quotes && (p[0] == '\"' || p[0] == '\''))) {
				pos = pos + 1;
			} else {
				pos2++;
			}
			break;
		} else if(is_in(NUMBERS, p[0])) {
			if(non_number_before) {
				pos2++;
				break;
			}
		} else {
			non_number_before = true;
		}
	}
	editing_to_expression1 = (l_to == 0);
	if(pos2 > pos) {
		current_object_start = -1;
		current_object_end = -1;
	} else {
		gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &ipos, pos);
		gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
		gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &ipos, &iend, FALSE);
		p = gstr2;
		while(p[0] != '\0') {
			if(!CALCULATOR->utf8_pos_is_valid_in_name(p)) {
				break;
			}
			pos++;
			p = g_utf8_next_char(p);
		}
		if(pos2 >= gtk_text_buffer_get_char_count(expression_edit_buffer())) {
			current_object_start = -1;
			current_object_end = -1;
		} else {
			current_object_start = pos2;
			current_object_end = pos;
		}
		g_free(gstr2);
	}
	g_free(gstr);
	current_object_has_changed = false;
}

bool completion_to_menu = false;

void on_completion_match_selected(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn*, gpointer) {
	GtkTreeIter iter;
	gtk_tree_model_get_iter(completion_sort, &iter, path);
	string str;
	ExpressionItem *item = NULL;
	Prefix *prefix = NULL;
	int p_type = 0;
	int exp = 1;
	void *p = NULL;
	const ExpressionName *ename = NULL, *ename_r = NULL, *ename_r2;
	gint i_type = 0;
	guint i_match = 0;
	gtk_tree_model_get(completion_sort, &iter, 2, &p, 4, &i_type, 7, &i_match, 8, &p_type, -1);
	if(i_type == 3) return;
	if(p_type == 1) item = (ExpressionItem*) p;
	else if(p_type == 2) prefix = (Prefix*) p;
	else if(p_type >= 100) p_type = 0;
	gint cos_bak = current_object_start;
	GtkTextIter object_start, object_end;
	gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_start, current_object_start);
	gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_end, current_object_end);
	if(item && item->isHidden() && item->type() == TYPE_UNIT && item->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) item)->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT && !((AliasUnit*) item)->firstBaseUnit()->isHidden() && ((AliasUnit*) item)->firstBaseUnit()->isActive()) {
		PrintOptions po = printops;
		po.can_display_unicode_string_arg = (void*) expression_edit_widget();
		po.abbreviate_names = true;
		str = ((AliasUnit*) item)->firstBaseUnit()->print(po, false, TAG_TYPE_HTML, true, false);
	} else if(item && item->type() == TYPE_UNIT && item->subtype() == SUBTYPE_COMPOSITE_UNIT && (((CompositeUnit*) item)->countUnits() > 1 || !((CompositeUnit*) item)->get(1, &exp, &prefix) || exp != 1)) {
		PrintOptions po = printops;
		po.can_display_unicode_string_arg = (void*) expression_edit_widget();
		po.abbreviate_names = true;
		str = ((Unit*) item)->print(po, false, TAG_TYPE_HTML, true, false);
	} else if(item) {
		CompositeUnit *cu = NULL;
		if(item->type() == TYPE_UNIT && item->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			cu = (CompositeUnit*) item;
			item = cu->get(1);
		}
		if(i_type > 2) {
			if(i_match > 0) ename = &item->getName(i_match);
			else ename = &item->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			if(!ename) return;
			if(cu && prefix) {
				str = prefix->preferredInputName(ename->abbreviation, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).name;
				str += ename->formattedName(TYPE_UNIT, true);
			} else {
				str = ename->formattedName(TYPE_UNIT, true);
			}
		} else if(cu && prefix) {
			string cmpstr;
			if(tabbed_completion) {
				cmpstr = tabbed_completion_object;
			} else {
				gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &object_start, &object_end, FALSE);
				cmpstr = gstr2;
				g_free(gstr2);
			}
			ename_r = &prefix->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			if(printops.abbreviate_names && ename_r->abbreviation) ename_r2 = &prefix->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			else ename_r2 = NULL;
			if(ename_r2 == ename_r) ename_r2 = NULL;
			const ExpressionName *ename_i;
			size_t l = 0;
			for(size_t name_i = 0; name_i <= (ename_r2 ? prefix->countNames() + 1 : prefix->countNames()) && l != cmpstr.length(); name_i++) {
				if(name_i == 0) {
					ename_i = ename_r;
				} else if(name_i == 1 && ename_r2) {
					ename_i = ename_r2;
				} else {
					ename_i = &prefix->getName(ename_r2 ? name_i - 1 : name_i);
					if(!ename_i || ename_i == ename_r || ename_i == ename_r2 || (ename_i->name.length() <= l && ename_i->name.length() != cmpstr.length()) || ename_i->plural || (ename_i->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename_i->name.c_str(), (void*) expression_edit_widget())))) {
						ename_i = NULL;
					}
				}
				if(ename_i) {
					if(!((Unit*)item)->useWithPrefixesByDefault() || ename_i->name.length() >= cmpstr.length()) {
						for(size_t i = 0; i < cmpstr.length() && i < ename_i->name.length(); i++) {
							if(ename_i->name[i] != cmpstr[i]) {
								if(i_type != 1 || !equalsIgnoreCase(ename_i->name, cmpstr)) {
									ename_i = NULL;
								}
								break;
							}
						}
					} else {
						ename_i = NULL;
					}
				}
				if(ename_i) {
					l = ename_i->name.length();
					ename = ename_i;
				}
			}
			for(size_t name_i = 1; name_i <= prefix->countNames() && l != cmpstr.length(); name_i++) {
				ename_i = &prefix->getName(name_i);
				if(!ename_i || ename_i == ename_r || ename_i == ename_r2 || (ename_i->name.length() <= l && ename_i->name.length() != cmpstr.length()) || (!ename_i->plural && !(ename_i->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename_i->name.c_str(), (void*) expression_edit_widget()))))) {
					ename_i = NULL;
				}
				if(ename_i) {
					if(!((Unit*)item)->useWithPrefixesByDefault() || ename_i->name.length() >= cmpstr.length()) {
						for(size_t i = 0; i < cmpstr.length() && i < ename_i->name.length(); i++) {
							if(ename_i->name[i] != cmpstr[i] && (ename_i->name[i] < 'A' || ename_i->name[i] > 'Z' || ename_i->name[i] != cmpstr[i] + 32) && (ename_i->name[i] < 'a' || ename_i->name[i] > '<' || ename_i->name[i] != cmpstr[i] - 32)) {
								if(i_type != 1 || !equalsIgnoreCase(ename_i->name, cmpstr)) {
									ename_i = NULL;
								}
								break;
							}
						}
					} else {
						ename_i = NULL;
					}
				}
				if(ename_i) {
					l = ename_i->name.length();
					ename = ename_i;
				}
			}
			if(ename && ename->completion_only) {
				ename = &prefix->preferredInputName(ename->abbreviation, printops.use_unicode_signs, ename->plural, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			}
			if(!ename) ename = ename_r;
			if(!ename) return;
			str = ename->name;
			str += item->preferredInputName(printops.abbreviate_names && ename->abbreviation, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_UNIT, true);
		} else {
			string cmpstr;
			if(tabbed_completion) {
				cmpstr = tabbed_completion_object;
			} else {
				gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &object_start, &object_end, FALSE);
				cmpstr = gstr2;
				if(i_match > 0) {
					gtk_text_iter_forward_chars(&object_start, unicode_length(cmpstr, i_match));
					current_object_start += unicode_length(cmpstr, i_match);
				}
				g_free(gstr2);
			}
			string cap_str;
			if(i_match > 0) cmpstr = cmpstr.substr(i_match);
			ename_r = &item->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			if(printops.abbreviate_names && ename_r->abbreviation) ename_r2 = &item->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			else ename_r2 = NULL;
			if(ename_r2 == ename_r) ename_r2 = NULL;
			for(size_t name_i = 0; name_i <= (ename_r2 ? item->countNames() + 1 : item->countNames()) && !ename; name_i++) {
				if(name_i == 0) {
					ename = ename_r;
				} else if(name_i == 1 && ename_r2) {
					ename = ename_r2;
				} else {
					ename = &item->getName(ename_r2 ? name_i - 1 : name_i);
					if(!ename || ename == ename_r || ename == ename_r2 || ename->plural || (ename->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget())))) {
						ename = NULL;
					}
				}
				if(ename) {
					bool b = false;
					unordered_map<const ExpressionName*, string>::iterator cap_it = capitalized_names.find(ename);
					if(cap_it != capitalized_names.end() && cmpstr.length() <= cap_it->second.length()) {
						b = true;
						for(size_t i = 0; i < cmpstr.length(); i++) {
							if(cap_it->second[i] != cmpstr[i]) {
								if(i_type != 1 || !equalsIgnoreCase(cap_it->second, cmpstr)) {
									b = false;
								}
								break;
							}
						}
						if(b && i_match > 0 && i_type != 1 && equalsIgnoreCase(cap_it->second, cmpstr)) b = false;
					}
					if(b) cap_str = cap_it->second;
					if(!b) {
						if(cmpstr.length() <= ename->name.length()) {
							b = true;
							for(size_t i = 0; i < cmpstr.length(); i++) {
								if(ename->name[i] != cmpstr[i]) {
									if(i_type != 1 || !equalsIgnoreCase(ename->name, cmpstr)) {
										b = false;
									}
									break;
								}
							}
						}
						if(b && i_match > 0 && i_type != 1 && equalsIgnoreCase(ename->name, cmpstr)) b = false;
					}
					if(!b) ename = NULL;
				}
			}
			for(size_t name_i = 1; name_i <= item->countNames() && !ename; name_i++) {
				ename = &item->getName(name_i);
				if(!ename || ename == ename_r || ename == ename_r2 || (!ename->plural && !(ename->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget()))))) {
					ename = NULL;
				}
				if(ename) {
					if(cmpstr.length() <= ename->name.length()) {
						for(size_t i = 0; i < cmpstr.length(); i++) {
							if(ename->name[i] != cmpstr[i] && (ename->name[i] < 'A' || ename->name[i] > 'Z' || ename->name[i] != cmpstr[i] + 32) && (ename->name[i] < 'a' || ename->name[i] > '<' || ename->name[i] != cmpstr[i] - 32)) {
								if(i_type != 1 || !equalsIgnoreCase(ename->name, cmpstr)) {
									ename = NULL;
								}
								break;
							}
						}
						if(ename && i_match > 0 && i_type != 1 && equalsIgnoreCase(ename->name, cmpstr)) ename = NULL;
					} else {
						ename = NULL;
					}
				}
			}
			if(!ename || ename->completion_only) ename = ename_r;
			if(!ename) return;
			if(!cap_str.empty()) str = cap_str;
			else str = ename->name;
			if(tabbed_completion && i_match > 0) str.insert(0, tabbed_completion_object.substr(0, i_match));
		}
	} else if(prefix) {
		string cmpstr;
		if(tabbed_completion) {
			cmpstr = tabbed_completion_object;
		} else {
			gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &object_start, &object_end, FALSE);
			cmpstr = gstr2;
			g_free(gstr2);
		}
		ename_r = &prefix->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
		if(printops.abbreviate_names && ename_r->abbreviation) ename_r2 = &prefix->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
		else ename_r2 = NULL;
		if(ename_r2 == ename_r) ename_r2 = NULL;
		for(size_t name_i = 0; name_i <= (ename_r2 ? prefix->countNames() + 1 : prefix->countNames()) && !ename; name_i++) {
			if(name_i == 0) {
				ename = ename_r;
			} else if(name_i == 1 && ename_r2) {
				ename = ename_r2;
			} else {
				ename = &prefix->getName(ename_r2 ? name_i - 1 : name_i);
				if(!ename || ename == ename_r || ename == ename_r2 || ename->plural || (ename->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget())))) {
					ename = NULL;
				}
			}
			if(ename) {
				if(cmpstr.length() <= ename->name.length()) {
					for(size_t i = 0; i < cmpstr.length(); i++) {
						if(ename->name[i] != cmpstr[i]) {
							if(i_type != 1 || !equalsIgnoreCase(ename->name, cmpstr)) {
								ename = NULL;
							}
							break;
						}
					}
				} else {
					ename = NULL;
				}
			}
		}
		for(size_t name_i = 1; name_i <= prefix->countNames() && !ename; name_i++) {
			ename = &prefix->getName(name_i);
			if(!ename || ename == ename_r || ename == ename_r2 || (!ename->plural && !(ename->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget()))))) {
				ename = NULL;
			}
			if(ename) {
				if(cmpstr.length() <= ename->name.length()) {
					for(size_t i = 0; i < cmpstr.length(); i++) {
						if(ename->name[i] != cmpstr[i] && (ename->name[i] < 'A' || ename->name[i] > 'Z' || ename->name[i] != cmpstr[i] + 32) && (ename->name[i] < 'a' || ename->name[i] > '<' || ename->name[i] != cmpstr[i] - 32)) {
							if(i_type != 1 || !equalsIgnoreCase(ename->name, cmpstr)) {
								ename = NULL;
							}
							break;
						}
					}
				} else {
					ename = NULL;
				}
			}
		}
		if(ename && (ename->completion_only || (printops.use_unicode_signs && ename->name == "u"))) {
			ename = &prefix->preferredInputName(ename->abbreviation, printops.use_unicode_signs, ename->plural, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
		}
		if(!ename) ename = ename_r;
		if(!ename) return;
		str = ename->name;
	} else {
		gchar *gstr;
		gtk_tree_model_get(completion_sort, &iter, 0, &gstr, -1);
		str = gstr;
		size_t i = 0;
		size_t i2 = str.find(" <i>");
		while(i_match > 0) {
			if(i == 0) i = i2 + 4;
			else i = i2 + 8;
			if(i >= str.length()) break;
			i2 = str.find("</i>", i);
			if(i2 == string::npos) break;
			i_match--;
			if(i == string::npos) break;
		}
		if(i2 == string::npos) i2 = str.length();
		if(i == string::npos) i = 0;
		str = str.substr(i, i2 - i);
		g_free(gstr);
	}
	if(completion_to_menu) {
		if(str[str.length() - 1] == ' ' || str[str.length() - 1] == '/') {
			if(printops.use_unicode_signs && can_display_unicode_string_function("➞", (void*) expression_edit_widget())) {
				str.insert(0, "➞");
			} else {
				if(!auto_calculate || rpn_mode || parsed_in_result) str.insert(0, " ");
				str.insert(0, CALCULATOR->localToString(auto_calculate));
			}
			if(auto_calculate && !rpn_mode && !parsed_in_result) {
				GtkTextIter iter;
				gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iter);
				gtk_text_buffer_select_range(expression_edit_buffer(), &iter, &iter);
			}
			insert_text(str.c_str());
		} else {
			str.insert(0, "➞");
			add_autocalculated_result_to_history();
			bool ac_bak = auto_calculate;
			auto_calculate = false;
			set_previous_expression(get_expression_text());
			execute_expression(true, false, OPERATION_ADD, NULL, false, 0, "", str);
			auto_calculate = ac_bak;
		}
		return;
	}
	block_completion();
	block_expression_modified();
	block_undo();
	gtk_text_buffer_delete(expression_edit_buffer(), &object_start, &object_end);
	unblock_undo();
	unblock_expression_modified();
	GtkTextIter ipos = object_start;
	if(item && item->type() == TYPE_FUNCTION) {
		GtkTextIter ipos2 = ipos;
		gtk_text_iter_forward_char(&ipos2);
		gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &ipos, &ipos2, FALSE);
		if(strlen(gstr) > 0 && gstr[0] == '(') {
			gtk_text_buffer_insert(expression_edit_buffer(), &ipos, str.c_str(), -1);
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &ipos);
		} else {
			str += "()";
			gtk_text_buffer_insert(expression_edit_buffer(), &ipos, str.c_str(), -1);
			gtk_text_iter_backward_char(&ipos);
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &ipos);
		}
		g_free(gstr);
	} else {
		gtk_text_buffer_insert(expression_edit_buffer(), &ipos, str.c_str(), -1);
		gtk_text_buffer_place_cursor(expression_edit_buffer(), &ipos);
	}
	current_object_end = current_object_start + unicode_length(str);
	current_object_start = cos_bak;
	gtk_widget_hide(completion_window);
	unblock_completion();
	if(!item && !prefix && editing_to_expression && gtk_text_iter_is_end(&ipos)) {
		string str = get_expression_text();
		if(str[str.length() - 1] != ' ' && str[str.length() - 1] != '/') execute_expression();
	}
}

bool completion_ignore_enter = false, completion_hover_blocked = false;

gboolean on_completionview_enter_notify_event(GtkWidget*, GdkEventCrossing*, gpointer) {
	return completion_ignore_enter;
}
gboolean on_completionview_motion_notify_event(GtkWidget*, GdkEventMotion*, gpointer) {
	completion_ignore_enter = FALSE;
	if(completion_hover_blocked) {
		gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(completion_view), TRUE);
		completion_hover_blocked = false;
	}
	return FALSE;
}
gboolean on_completionwindow_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	if(!gtk_widget_get_mapped(completion_window)) return FALSE;
	gtk_widget_event(expression_edit_widget(), (GdkEvent*) event);
	return TRUE;
}
gboolean on_completionwindow_button_press_event(GtkWidget*, GdkEventButton*, gpointer) {
	if(!gtk_widget_get_mapped(completion_window)) return FALSE;
	gtk_widget_hide(completion_window);
	return TRUE;
}

void completion_resize_popup(int matches) {

	gint x, y;
	gint items, height = 0, items_y = 0, height_diff;
	GdkDisplay *display;

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	GdkMonitor *monitor;
#endif
	GdkRectangle area, bufloc, rect;
	GdkWindow *window;
	GtkRequisition popup_req;
	GtkRequisition tree_req;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gboolean are_coords_global;

	GtkTextIter iter;
	if(current_object_start < 0) {
		GtkTextMark *miter = gtk_text_buffer_get_insert(expression_edit_buffer());
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, miter);
	} else {
		gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &iter, current_object_start);
	}
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(expression_edit_widget()), &iter, &bufloc);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(expression_edit_widget()), GTK_TEXT_WINDOW_WIDGET, bufloc.x, bufloc.y, &bufloc.x, &bufloc.y);
	window = gtk_text_view_get_window(GTK_TEXT_VIEW(expression_edit_widget()), GTK_TEXT_WINDOW_WIDGET);
	gdk_window_get_origin(window, &x, &y);

	x += bufloc.x;
	y += bufloc.y;

	gtk_widget_realize(completion_view);
	while(gtk_events_pending()) gtk_main_iteration();
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(completion_view));
	column = gtk_tree_view_get_column(GTK_TREE_VIEW(completion_view), 0);

	gtk_widget_get_preferred_size(completion_view, &tree_req, NULL);
	gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &height_diff);

	path = gtk_tree_path_new_from_indices(0, -1);
	gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
	gtk_tree_path_free(path);
	items_y = rect.y;
	height_diff -= rect.height;
	if(height_diff < 2) height_diff = 2;

	display = gtk_widget_get_display(GTK_WIDGET(expression_edit_widget()));
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	monitor = gdk_display_get_monitor_at_window(display, window);
	gdk_monitor_get_workarea(monitor, &area);
#else
	GdkScreen *screen = gdk_display_get_default_screen(display);
	gdk_screen_get_monitor_workarea(screen, gdk_screen_get_monitor_at_window(screen, window), &area);
#endif
	are_coords_global = !on_wayland();

	items = matches;
	if(items > (are_coords_global ? 20 : 15)) items = (are_coords_global ? 20 : 15);
	if(items > 0) {
		path = gtk_tree_path_new_from_indices(items - 1, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
		gtk_tree_path_free(path);
		height = rect.y + rect.height - items_y + height_diff;
	}
	if(are_coords_global) {
		while(items > 0 && ((y > area.height / 2 && area.y + y < height) || (y <= area.height / 2 && area.height - y < height))) {
			items--;
			path = gtk_tree_path_new_from_indices(items - 1, -1);
			gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
			gtk_tree_path_free(path);
			height = rect.y + rect.height - items_y + height_diff;
		}
	}

	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(completion_scrolled), height);

	if(items <= 0) gtk_widget_hide(completion_scrolled);
	else gtk_widget_show(completion_scrolled);

	gtk_widget_get_preferred_size(completion_window, &popup_req, NULL);

	if(popup_req.width < rect.width + 2) popup_req.width = rect.width + 2;

	if(are_coords_global) {
		if(x < area.x) x = area.x;
		else if(x + popup_req.width > area.x + area.width) x = area.x + area.width - popup_req.width;

		if(y + bufloc.height + popup_req.height <= area.y + area.height || y - area.y < (area.y + area.height) - (y + bufloc.height)) {
			y += bufloc.height;
		} else {
			path = gtk_tree_path_new_from_indices(matches - 1, -1);
			gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
			gtk_tree_path_free(path);
			height = rect.y + rect.height + height_diff;
			path = gtk_tree_path_new_from_indices(matches - items, -1);
			gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
			gtk_tree_path_free(path);
			height -= rect.y;
			gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(completion_scrolled), height);
			gtk_widget_get_preferred_size(completion_window, &popup_req, NULL);
			y -= popup_req.height;
		}
	} else {
		y += bufloc.height;
	}
	if(matches > 0) {
		path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(completion_view), path, NULL, FALSE, 0.0, 0.0);
		gtk_tree_path_free(path);
	}

	gtk_window_move(GTK_WINDOW(completion_window), x, y);

}

bool contains_related_unit(const MathStructure &m, Unit *u) {
	if(m.isUnit()) return u == m.unit() || u->containsRelativeTo(m.unit()) || m.unit()->containsRelativeTo(u);
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_related_unit(m[i], u)) return true;
	}
	return false;
}

GtkTreeIter completion_separator_iter;
extern bool display_expression_status;
extern int to_form, to_base;

const Number *from_struct_get_number(const MathStructure &m) {
	if(m.isNumber()) return &m.number();
	const Number *nr = NULL;
	for(size_t i = 0; i < m.size(); i++) {
		if(m[i].isNumber()) return &m[i].number();
		else if(!nr) nr = from_struct_get_number(m[i]);
	}
	return nr;
}

string current_completion_object() {
	set_current_object();
	if(current_object_start < 0 || current_object_in_quotes) return "";
	GtkTextIter object_start, object_end;
	gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_start, current_object_start);
	gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_end, current_object_end);
	gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &object_start, &object_end, FALSE);
	string str = gstr2;
	g_free(gstr2);
	return str;
}

extern bool function_differentiable(MathFunction*);
bool test_x_count_sub(const MathStructure &m, int &x, int &y, int &z) {
	if(m.isVariable() && !m.variable()->isKnown()) {
		if(x > 0 && m.variable()->referenceName() == "x") x--;
		else if(y > 0 && m.variable()->referenceName() == "y") y--;
		else if(z > 0 && m.variable()->referenceName() == "z") z--;
	}
	if(m.isFunction() && !function_differentiable(m.function())) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(test_x_count_sub(m[i], x, y, z)) return true;
	}
	return x == 0 || y == 0 || z == 0;
}
bool test_x_count(const MathStructure &m, int x = -1, int y = -1, int z = -1) {
	return test_x_count_sub(m, x, y, z);
}

void do_completion(bool to_menu, bool force) {
	if(!enable_completion && !to_menu) {gtk_widget_hide(completion_window); return;}
	Unit *exact_unit = NULL;
	if(to_menu) {
		editing_to_expression = true;
		editing_to_expression1 = true;
		current_from_struct = current_result();
		current_from_units.clear();
		Unit *u = NULL;
		if(current_displayed_result()) u = find_exact_matching_unit(*current_displayed_result());
		if(u) {
			current_from_units.push_back(u);
			if(u->subtype() == SUBTYPE_COMPOSITE_UNIT || !contains_prefix(*current_displayed_result())) {
				exact_unit = u;
			}
			Unit *u2 = CALCULATOR->findMatchingUnit(*current_parsed_result());
			int exp = 1;
			if(u2 && u2 != u && u2->category() != u->category() && ((u2->baseUnit() == u->baseUnit() && u2->baseExponent() == u->baseExponent()) || (u->subtype() == SUBTYPE_COMPOSITE_UNIT && ((CompositeUnit*) u)->countUnits() == 1 && u2->baseUnit() == ((CompositeUnit*) u)->get(1, &exp)->baseUnit() && u2->baseExponent() == ((CompositeUnit*) u)->get(1)->baseExponent(exp)))) {
				current_from_units.push_back(u2);
			}
		} else {
			find_matching_units(*current_result(), current_parsed_result(), current_from_units);
		}
		current_object_start = -1;
		current_object_has_changed = true;
		completion_to_menu = true;
	} else {
		set_current_object();
		completion_to_menu = false;
	}
	gint cos_bak = current_object_start, coe_bak = current_object_end;
	string str;
	int to_type = 0;
	if(editing_to_expression && current_from_struct && current_from_struct->isDateTime()) to_type = 3;
	if(current_object_start < 0) {
		if(editing_to_expression && editing_to_expression1 && current_from_struct && !current_from_units.empty()) {
			to_type = 4;
		} else if(editing_to_expression && editing_to_expression1 && current_from_struct) {
			to_type = 2;
		} else if(!editing_to_expression1 && current_parsed_function() && current_parsed_function()->subtype() == SUBTYPE_DATA_SET && current_parsed_function_index() > 1) {
			Argument *arg = current_parsed_function()->getArgumentDefinition(current_parsed_function_index());
			if(!arg || arg->type() != ARGUMENT_TYPE_DATA_PROPERTY) {
				gtk_widget_hide(completion_window);
				return;
			}
		} else if(to_type < 2) {
			gtk_widget_hide(completion_window);
			return;
		}
	} else {
		GtkTextIter object_start, object_end;
		gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_start, current_object_start);
		gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_end, current_object_end);
		gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &object_start, &object_end, FALSE);
		str = gstr2;
		g_free(gstr2);
		if(unicode_length(str) < (size_t) completion_min) {gtk_widget_hide(completion_window); return;}
	}
	if(!force && !str.empty() && !editing_to_expression && current_parsed_function() && current_parsed_function_index() > 0 && current_parsed_function()->getArgumentDefinition(current_parsed_function_index()) && current_parsed_function()->getArgumentDefinition(current_parsed_function_index())->type() == ARGUMENT_TYPE_TEXT && current_parsed_function_index() <= current_parsed_function_struct().size() && current_parsed_function_struct()[current_parsed_function_index() - 1].isSymbolic()) {
		gtk_widget_hide(completion_window);
		return;
	}
	if(!force && !editing_to_expression && ((str.length() == 1 && (str[0] == 'x' || str[0] == 'y' || str[0] == 'z')) || (str.length() == 2 && ((str[0] == 'x' && str[1] == 'y') || (str[0] == 'x' && str[1] == 'z') || (str[0] == 'y' && str[1] == 'z'))) || (str.length() == 3 && str[0] == 'x' && str[1] == 'y' && str[2] == 'z')) && (test_x_count(current_parsed_expression(), str.find("x") != string::npos ? 2 : 1, str.find("y") != string::npos ? 2 : 1, str.find("z") != string::npos ? 2 : 1) || (current_parsed_function() && current_parsed_function_index() > 0 && current_parsed_function_index() <= current_parsed_function_struct().size() && test_x_count(current_parsed_function_struct()[current_parsed_function_index() - 1], str.find("x") != string::npos ? 2 : 1, str.find("y") != string::npos ? 2 : 1, str.find("z") != string::npos ? 2 : 1)))) {
		gtk_widget_hide(completion_window);
		return;
	}
	GtkTreeIter iter;
	int matches = 0;
	int highest_match = 0;
	if(editing_to_expression && editing_to_expression1 && current_from_struct) {
		if((current_from_struct->isUnit() && current_from_struct->unit()->isCurrency()) || (current_from_struct->isMultiplication() && current_from_struct->size() == 2 && (*current_from_struct)[0].isNumber() && (*current_from_struct)[1].isUnit() && (*current_from_struct)[1].unit()->isCurrency())) {
			if(to_type == 4) to_type = 5;
			else to_type = 1;
		}
	}
	vector<string> current_from_categories;
	if(to_type == 4) {
		for(size_t i = 0; i < current_from_units.size(); i++) {
			bool b = false;
			for(size_t i2 = 0; i2 < alt_volcats.size(); i2++) {
				if(current_from_units[i]->category() == alt_volcats[i2]) {
					current_from_categories.push_back(volume_cat);
					b = true;
					break;
				}
			}
			if(!b) current_from_categories.push_back(current_from_units[i]->category());
		}
	}
	unordered_map<const ExpressionName*, string>::iterator cap_it;
	bool show_separator1 = false, show_separator2 = false;
	if(((str.length() > 0 && is_not_in(NUMBERS NOT_IN_NAMES "%", str[0])) || (str.empty() && !editing_to_expression1 && current_parsed_function() && current_parsed_function()->subtype() == SUBTYPE_DATA_SET) || to_type >= 2) && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_store), &iter)) {
		Argument *arg = NULL;
		if(!editing_to_expression1 && current_parsed_function() && current_parsed_function()->subtype() == SUBTYPE_DATA_SET) {
			arg = current_parsed_function()->getArgumentDefinition(current_parsed_function_index());
			if(arg && (arg->type() == ARGUMENT_TYPE_DATA_OBJECT || arg->type() == ARGUMENT_TYPE_DATA_PROPERTY)) {
				if(arg->type() == ARGUMENT_TYPE_DATA_OBJECT && (str.empty() || unicode_length(str) < (size_t) completion_min)) {gtk_widget_hide(completion_window); return;}
				if(current_parsed_function_index() == 1) {
					for(size_t i = 1; i <= current_parsed_function()->countNames(); i++) {
						if(str.find(current_parsed_function()->getName(i).name) != string::npos) {
							arg = NULL;
							break;
						}
					}
				}
			} else {
				arg = NULL;
			}
			if(arg) {
				DataSet *o = NULL;
				if(arg->type() == ARGUMENT_TYPE_DATA_OBJECT) o = ((DataObjectArgument*) arg)->dataSet();
				else if(arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) o = ((DataPropertyArgument*) arg)->dataSet();
				if(o) {
					while(true) {
						int p_type = 0;
						gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 8, &p_type, -1);
						if(p_type > 2 && p_type < 100) {
							if(!gtk_list_store_remove(completion_store, &iter)) break;
						} else {
							gtk_list_store_set(completion_store, &iter, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, -1);
							if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_store), &iter)) break;
						}
					}
					DataPropertyIter it;
					DataProperty *dp = o->getFirstProperty(&it);
					vector<DataObject*> found_objects;
					while(dp) {
						if(arg->type() == ARGUMENT_TYPE_DATA_OBJECT) {
							if(dp->isKey() && dp->propertyType() == PROPERTY_STRING) {
								DataObjectIter it2;
								DataObject *obj = o->getFirstObject(&it2);
								while(obj) {
									const string &name = obj->getProperty(dp);
									int b_match = 0;
									if(equalsIgnoreCase(str, name, 0, str.length(), 0)) b_match = name.length() == str.length() ? 1 : 2;
									for(size_t i = 0; b_match && i < found_objects.size(); i++) {
										if(found_objects[i] == obj) b_match = 0;
									}
									if(b_match) {
										found_objects.push_back(obj);
										DataPropertyIter it3;
										DataProperty *dp2 = o->getFirstProperty(&it3);
										string names = name;
										string title;
										while(dp2) {
											if(title.empty() && dp2->hasName("name")) {
												title = dp2->getDisplayString(obj->getProperty(dp2));
											}
											if(dp2 != dp && dp2->isKey()) {
												names += " <i>";
												names += dp2->getDisplayString(obj->getProperty(dp2));
												names += "</i>";
											}
											dp2 = o->getNextProperty(&it3);
										}
										gtk_list_store_append(completion_store, &iter); 										gtk_list_store_set(completion_store, &iter, 0, names.c_str(), 1, title.empty() ? _("Data object") : title.c_str(), 2, NULL, 3, TRUE, 4, b_match, 6, b_match == 1 ? PANGO_WEIGHT_BOLD : (b_match > 3 ? PANGO_WEIGHT_LIGHT : PANGO_WEIGHT_NORMAL), 7, 0, 8, 4, 9, NULL, -1);
										matches++;
									}
									obj = o->getNextObject(&it2);
								}
							}
						} else {
							int b_match = 0;
							size_t i_match = 0;
							if(str.empty()) {
								b_match = 2;
								i_match = 1;
							} else {
								for(size_t i = 1; i <= dp->countNames(); i++) {
									const string &name = dp->getName(i);
									if((i_match == 0 || name.length() == str.length()) && equalsIgnoreCase(str, name, 0, str.length(), 0)) {
										b_match = name.length() == str.length() ? 1 : 2;
										i_match = i;
										if(b_match == 1) break;
									}
								}
							}
							if(b_match) {
								string names = dp->getName(i_match);
								for(size_t i = 1; i <= dp->countNames(); i++) {
									if(i != i_match) {
										names += " <i>";
										names += dp->getName(i);
										names += "</i>";
									}
								}
								i_match = 0;
								gtk_list_store_append(completion_store, &iter);
								gtk_list_store_set(completion_store, &iter, 0, names.c_str(), 1, dp->title().c_str(), 2, NULL, 3, TRUE, 4, b_match, 6, b_match == 1 ? PANGO_WEIGHT_BOLD : (b_match > 3 ? PANGO_WEIGHT_LIGHT : PANGO_WEIGHT_NORMAL), 7, 0, 8, 3, 9, NULL, -1);
								if(b_match > highest_match) highest_match = b_match;
								matches++;
							}
						}
						dp = o->getNextProperty(&it);
					}
				} else {
					arg = NULL;
				}
			}
		}
		if(!arg) {
			vector<string> pstr;
			vector<Prefix*> prefixes;
			if(!str.empty()) {
				for(size_t pi = 1; ; pi++) {
					Prefix *prefix = CALCULATOR->getPrefix(pi);
					if(!prefix) break;
					for(size_t name_i = 1; name_i <= prefix->countNames(); name_i++) {
						const string *pname = &prefix->getName(name_i).name;
						if(!pname->empty() && pname->length() < str.length()) {
							bool pmatch = true;
							for(size_t i = 0; i < pname->length(); i++) {
								if((*pname)[i] != str[i]) {
									pmatch = false;
									break;
								}
							}
							if(pmatch) {
								prefixes.push_back(prefix);
								pstr.push_back(str.substr(pname->length()));
							}
						}
					}
				}
			}
			do {
				ExpressionItem *item = NULL;
				Prefix *prefix = NULL;
				void *p = NULL;
				int p_type = 0;
				gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 2, &p, 8, &p_type, -1);
				if(p_type == 1) item = (ExpressionItem*) p;
				else if(p_type == 2) prefix = (Prefix*) p;
				int b_match = false;
				size_t i_match = 0;
				if(item && to_type < 2) {
					if((editing_to_expression || !evalops.parse_options.functions_enabled) && item->type() == TYPE_FUNCTION) {}
					else if(item->type() == TYPE_VARIABLE && (!evalops.parse_options.variables_enabled || (editing_to_expression && !((Variable*) item)->isKnown()))) {}
					else if(!evalops.parse_options.units_enabled && item->type() == TYPE_UNIT) {}
					else {
						CompositeUnit *cu = NULL;
						int exp = 0;
						if(item->type() == TYPE_UNIT && ((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
							cu = (CompositeUnit*) item;
							item = cu->get(1, &exp, &prefix);
							if(item && prefix) {
								for(size_t name_i = 1; name_i <= prefix->countNames(); name_i++) {
									const ExpressionName *ename = &prefix->getName(name_i);
									if(!ename->name.empty() && ename->name.length() >= str.length() && (ename->abbreviation || str.length() >= 2)) {
										bool pmatch = true;
										for(size_t i = 0; i < str.length(); i++) {
											if(ename->name[i] != str[i]) {
												pmatch = false;
												break;
											}
										}
										if(pmatch) {
											b_match = 2;
											item = NULL;
											break;
										}
									}
								}
								if(item && exp == 1 && cu->countUnits() == 1 && ((Unit*) item)->useWithPrefixesByDefault()) {
									if(!b_match && enable_completion2 && title_matches(cu, str, completion_min2)) {
										b_match = 4;
									}
									item = NULL;
								}
							}
						}
						for(size_t name_i = 1; item && name_i <= item->countNames() && !b_match; name_i++) {
							const ExpressionName *ename = &item->getName(name_i);
							if(ename && (!cu || ename->abbreviation || str.length() >= 3 || str.length() == ename->name.length())) {
								if(item->isHidden() && (item->type() != TYPE_UNIT || !((Unit*) item)->isCurrency()) && ename) {
									b_match = (ename->name == str) ? 1 : 0;
								} else {
									for(size_t icap = 0; icap < 2; icap++) {
										const string *namestr;
										if(icap == 0) {
											namestr = &ename->name;
										} else {
											cap_it = capitalized_names.find(ename);
											if(cap_it == capitalized_names.end()) break;
											namestr = &cap_it->second;
										}
										for(size_t icmp = 0; icmp <= prefixes.size(); icmp++) {
											if(icmp == 1 && (item->type() != TYPE_UNIT || (cu && !prefix) || (!cu && !((Unit*) item)->useWithPrefixesByDefault()))) break;
											if(cu && prefix) {
												if(icmp == 0 || prefix != prefixes[icmp - 1]) continue;
											}
											const string *cmpstr;
											if(icmp == 0) cmpstr = &str;
											else cmpstr = &pstr[icmp - 1];
											if(cmpstr->empty()) break;
											if(cmpstr->length() <= namestr->length()) {
												b_match = 2;
												for(size_t i = 0; i < cmpstr->length(); i++) {
													if((*namestr)[i] != (*cmpstr)[i]) {
														b_match = false;
														break;
													}
												}
												if(b_match && (!cu || (exp == 1 && cu->countUnits() == 1)) && ((!ename->case_sensitive && equalsIgnoreCase(*namestr, *cmpstr)) || (ename->case_sensitive && *namestr == *cmpstr))) b_match = 1;
												if(b_match) {
													if(icmp > 0 && !cu) {
														prefix = prefixes[icmp - 1];
														if((prefix->type() != PREFIX_DECIMAL || ((Unit*) item)->minPreferredPrefix() > ((DecimalPrefix*) prefix)->exponent() || ((Unit*) item)->maxPreferredPrefix() < ((DecimalPrefix*) prefix)->exponent()) && (prefix->type() != PREFIX_BINARY || ((Unit*) item)->baseUnit()->referenceName() != "bit" || ((Unit*) item)->minPreferredPrefix() > ((BinaryPrefix*) prefix)->exponent() || ((Unit*) item)->maxPreferredPrefix() < ((BinaryPrefix*) prefix)->exponent())) {
															b_match = false;
															prefix = NULL;
															continue;
														}
														if(CALCULATOR->getActiveExpressionItem(str.substr(0, str.length() - cmpstr->length()) + *namestr)) {
															b_match = false;
															prefix = NULL;
															continue;
														}
														i_match = str.length() - cmpstr->length();
													} else if(b_match > 1 && !editing_to_expression && item->isHidden() && str.length() == 1) {
														b_match = 4;
														i_match = name_i;
													}
													break;
												}
											}
										}
										if(b_match) break;
									}
								}
							}
						}
						if(item && ((!cu && b_match >= 2) || (exp == 1 && cu->countUnits() == 1 && b_match == 2)) && item->countNames() > 1) {
							for(size_t icmp = 0; icmp <= prefixes.size() && b_match > 1; icmp++) {
								if(icmp == 1 && (item->type() != TYPE_UNIT  || (cu && !prefix) || (!cu && !((Unit*) item)->useWithPrefixesByDefault()))) break;
								if(cu && prefix) {
									if(icmp == 0 || prefix != prefixes[icmp - 1]) continue;
								}
								const string *cmpstr;
								if(icmp == 0) cmpstr = &str;
								else cmpstr = &pstr[icmp - 1];
								if(cmpstr->empty()) break;
								for(size_t name_i = 1; name_i <= item->countNames(); name_i++) {
									cap_it = capitalized_names.find(&item->getName(name_i));
									if(item->getName(name_i).name == *cmpstr || (cap_it != capitalized_names.end() && cap_it->second == *cmpstr)) {
										if(!cu) {
											if(icmp > 0) {
												if(CALCULATOR->getActiveExpressionItem(str.substr(0, str.length() - cmpstr->length()) + *cmpstr)) break;
												prefix = prefixes[icmp - 1];
											} else {
												prefix = NULL;
											}
										}
										b_match = 1; break;
									}
								}
							}
						}
						if(item && !b_match && enable_completion2 && (!item->isHidden() || (item->type() == TYPE_UNIT && str.length() > 1 && ((Unit*) item)->isCurrency()))) {
							int i_cinm = name_matches2(cu ? cu : item, str, to_type == 1 ? 1 : completion_min2, &i_match);
							if(i_cinm == 1) {b_match = 1; i_match = 0;}
							else if(i_cinm == 2) b_match = 4;
							else if(title_matches(cu ? cu : item, str, to_type == 1 ? 1 : completion_min2)) b_match = 4;
							else if(!cu && item->type() == TYPE_UNIT && ((Unit*) item)->isCurrency() && country_matches((Unit*) item, str, to_type == 1 ? 1 : completion_min2)) b_match = 5;
						}
						if(cu) prefix = NULL;
					}
					if(b_match > 1 && (
					(to_type == 1 && (!item || item->type() != TYPE_UNIT)) ||
					((b_match > 2 || str.length() < 3) && editing_to_expression && current_from_struct && !current_from_struct->isAborted() && item && item->type() == TYPE_UNIT && !contains_related_unit(*current_from_struct, (Unit*) item) && (!current_from_struct->isNumber() || !current_from_struct->number().isReal() || (!prefix && ((Unit*) item)->isSIUnit() && (Unit*) item != CALCULATOR->getRadUnit())))
					)) {
						b_match = 0;
						i_match = 0;
					}
					if(b_match) {
						gchar *gstr;
						gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 0, &gstr, -1);
						if(gstr && strlen(gstr) > 0) {
							string nstr;
							if(gstr[0] == '<') {
								nstr = gstr;
								size_t i = nstr.find("-) </small>");
								if(i != string::npos && i > 2) {
									if(prefix && prefix->longName() == nstr.substr(8, i - 8)) {
										prefix = NULL;
									} else {
										nstr = nstr.substr(i + 11);
										if(!prefix) gtk_list_store_set(completion_store, &iter, 0, nstr.c_str(), -1);
									}
								}
							}
							if(prefix) {
								if(nstr.empty()) nstr = gstr;
								nstr.insert(0, "-) </small>");
								nstr.insert(0, prefix->longName());
								nstr.insert(0, "<small>(");
								gtk_list_store_set(completion_store, &iter, 0, nstr.c_str(), -1);
							}
						}
						if(gstr) g_free(gstr);
						if(b_match > highest_match) highest_match = b_match;
					}
				} else if(item && to_type == 4) {
					if(item->type() == TYPE_UNIT && item != exact_unit) {
						for(size_t i = 0; i < current_from_categories.size(); i++) {
							if(item->category() == current_from_categories[i]) {
								b_match = 6;
								break;
							} else if(current_from_categories[i] == volume_cat && (((Unit*) item)->system() != "Imperial" || current_from_units[i]->system().find("Imperial") != string::npos)) {
								for(size_t i2 = 0; i2 < alt_volcats.size(); i2++) {
									if(item->category() == alt_volcats[i2]) {b_match = 6; break;}
								}
								if(b_match == 6) break;
							}
						}
						if(b_match == 6 && item->isHidden() && item != CALCULATOR->getLocalCurrency()) b_match = 0;
						if(b_match == 6) {
							gchar *gstr;
							gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 0, &gstr, -1);
							if(gstr && strlen(gstr) > 0 && gstr[0] == '<') {
								string nstr = gstr;
								size_t i = nstr.find("-) </small>");
								if(i != string::npos && i > 2) {
									nstr = nstr.substr(i + 11);
									gtk_list_store_set(completion_store, &iter, 0, nstr.c_str(), -1);
								}
							}
							if(gstr) g_free(gstr);
						}
					}
				} else if(item && to_type == 5) {
					if(item->type() == TYPE_UNIT && ((Unit*) item)->isCurrency() && (to_menu || !item->isHidden() || item == CALCULATOR->getLocalCurrency()) && item != exact_unit) b_match = 6;
				} else if(item && to_type == 2 && str.empty() && current_from_struct) {
					if(item->type() == TYPE_VARIABLE && (item == CALCULATOR->v_percent || item == CALCULATOR->v_permille) && current_from_struct->isNumber() && !current_from_struct->isInteger() && !current_from_struct->number().imaginaryPartIsNonZero()) b_match = 2;
				} else if(prefix && to_type < 2) {
					for(size_t name_i = 1; name_i <= prefix->countNames() && !b_match; name_i++) {
						const string *pname = &prefix->getName(name_i).name;
						if(!pname->empty() && str.length() <= pname->length()) {
							b_match = 2;
							for(size_t i = 0; i < str.length(); i++) {
								if(str[i] != (*pname)[i]) {
									b_match = false;
									break;
								}
							}
							if(b_match && *pname == str) b_match = 1;
						}
					}
					if(to_type == 1 && b_match > 1) b_match = 0;
					if(b_match > highest_match) highest_match = b_match;
					else if(b_match == 1 && highest_match < 2) highest_match = 2;
					prefix = NULL;
				} else if(p_type >= 100 && editing_to_expression && editing_to_expression1) {
					gchar *gstr;
					gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 0, &gstr, -1);
					if(to_type >= 2 && str.empty()) b_match = 2;
					else b_match = completion_names_match(gstr, str, completion_min, &i_match);
					if(b_match > 1) {
						if(current_from_struct && str.length() < 3) {
							if(p_type >= 100 && p_type < 200) {
								if(to_type == 5 || current_from_struct->containsType(STRUCT_UNIT) <= 0) b_match = 0;
							} else if((p_type == 294 || p_type == 295 || (p_type == 292 && to_type == 4)) && !current_from_units.empty()) {
								bool b = false;
								for(size_t i = 0; i < current_from_units.size(); i++) {
									if(current_from_units[i] == CALCULATOR->getDegUnit()) {
										b = true;
										break;
									}
								}
								if(!b) {
									b_match = 0;
								} else if(str.empty()) {
									int base = to_base;
									if(base == 0) base = printops.base;
									if((base == BASE_SEXAGESIMAL && p_type == 292) || (base == BASE_TIME && p_type == 293) || (base == BASE_LATITUDE && p_type == 294) || (base == BASE_LONGITUDE && p_type == 295)) {
										b_match = 0;
									}
								}
							} else if(p_type > 290 && p_type < 300 && (p_type != 292 || to_type >= 1)) {
								if(!current_from_struct->isNumber() || (p_type > 290 && str.empty() && current_from_struct->isInteger())) b_match = 0;
								if(str.empty()) {
									int base = to_base;
									if(base == 0) base = printops.base;
									if((base == BASE_SEXAGESIMAL && p_type == 292) || (base == BASE_TIME && p_type == 293) || (base == BASE_LATITUDE && p_type == 294) || (base == BASE_LONGITUDE && p_type == 295)) {
										b_match = 0;
									}
								}
							} else if(p_type >= 200 && p_type <= 290 && (p_type != 200 || to_type == 1 || to_type >= 3)) {
								if(!current_from_struct->isNumber()) {
									b_match = 0;
								} else if(str.empty() && p_type >= 202 && !current_from_struct->isInteger()) {
									b_match = 0;
								} else if(str.empty()) {
									int base = to_base;
									if(base == 0) base = printops.base;
									if((p_type >= 202 && p_type <= 236 && base == p_type - 200) || (base == BASE_UNICODE && p_type == 281) || (base == BASE_BINARY_DECIMAL && p_type == 285) || (base == BASE_BIJECTIVE_26 && p_type == 290) || (base == BASE_ROMAN_NUMERALS && p_type == 280)) {
										b_match = 0;
									}
								}
							} else if(p_type >= 300 && p_type < 400) {
								if(p_type == 300) {
									if(!contains_rational_number(to_menu && current_displayed_result() ? *current_displayed_result() : *current_from_struct)) b_match = 0;
								} else if(p_type == 302) {
									if((to_menu && current_displayed_result() && !contains_fraction(*current_displayed_result())) || (!to_menu && (printops.number_fraction_format == FRACTION_DECIMAL || !contains_rational_number(*current_from_struct)))) b_match = 0;
								} else if(p_type == 301) {
									if((to_menu || (!current_from_struct->isNumber() || current_from_struct->number().isInteger() || current_from_struct->number().hasImaginaryPart())) && (!to_menu || !current_displayed_result() || (!current_displayed_result()->isNumber() || current_displayed_result()->number().isInteger() || current_displayed_result()->number().hasImaginaryPart()))) {
										bool b = false;
										for(size_t i = 0; i < current_from_units.size(); i++) {
											if(current_from_units[i]->system().find("Imperial") != string::npos) {
												b = true;
												break;
											}
										}
										if(!b) b_match = 0;
									}
								} else {
									if(!current_from_struct->isNumber()) b_match = 0;
									if(p_type >= 310 && p_type <= 314) {
										int base = to_base;
										if(base == 0) base = printops.base;
										if((base == BASE_FP16 && p_type == 310) || (base == BASE_FP32 && p_type == 311) || (base == BASE_FP64 && p_type == 312) || (base == BASE_FP80 && p_type == 313) || (base == BASE_FP128 && p_type == 314)) {
											b_match = 0;
										}
									}
								}
							} else if(p_type >= 400 && p_type < 500) {
								if(!contains_imaginary_number(*current_from_struct)) b_match = 0;
								if(str.empty()) {
									if(p_type == 404 && evalops.complex_number_form == COMPLEX_NUMBER_FORM_RECTANGULAR) b_match = 0;
									else if(p_type == 403 && evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) b_match = 0;
									else if(p_type == 402 && evalops.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) b_match = 0;
									else if(p_type == 401 && evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS && !complex_angle_form) b_match = 0;
									else if(p_type == 400 && evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS && complex_angle_form) b_match = 0;
								}
							} else if(p_type >= 500 && p_type < 600) {
								if(!current_from_struct->isDateTime()) b_match = 0;
							} else if(p_type == 600) {
								if(!current_from_units.empty() || (!current_from_struct->isInteger() && current_from_struct->containsType(STRUCT_ADDITION) <= 0)) b_match = 0;
							} else if(p_type == 601) {
								if(current_from_units.empty() || current_from_struct->containsType(STRUCT_ADDITION) <= 0) b_match = 0;
							} else if(p_type >= 700 && p_type < 800) {
								int exp = to_form;
								if(exp == TO_FORM_OFF) exp = printops.min_exp;
								if((p_type == 703 && (exp == 0 || !current_from_struct->isNumber())) || (p_type == 702 && exp == -3) || (p_type == 701 && exp > 0)) {
									b_match = 0;
								} else {
									const Number *nr_p = from_struct_get_number(*current_from_struct);
									if(!nr_p) {
										b_match = 0;
									} else {
										int absexp = (exp < -1 ? -exp : exp);
										Number nr(nr_p->hasImaginaryPart() ? nr_p->realPart() : *nr_p);
										if(nr.isFraction()) {
											nr.recip();
											if(absexp == -1) absexp = PRECISION;
										} else if(absexp == -1) {
											absexp = PRECISION + 3;
										}
										nr.abs();
										if(nr > Number(1, 1, 100)) {
											if(p_type == 703 || (p_type == 701 && absexp <= 100 && exp > 0)) b_match = 0;
										} else {
											if((p_type == 703 && (absexp > 100 || nr < Number(1, 1, absexp))) || (p_type == 701 && absexp <= 100 && ((absexp > 0 && exp >= -1 && nr >= Number(1, 1, absexp)) || nr < Number(1, 1, 5))) || (p_type == 702 && nr < Number(1, 1, 5))) {
												b_match = 0;
											}
										}
									}
								}
							}
						}
						if(b_match > highest_match) highest_match = b_match;
					}
					g_free(gstr);
				}
				gtk_list_store_set(completion_store, &iter, 3, b_match > 0, 4, b_match, 6, b_match == 1 ? PANGO_WEIGHT_BOLD : (b_match == 4 || b_match == 5 ? PANGO_WEIGHT_LIGHT : PANGO_WEIGHT_NORMAL), 7, i_match, -1);
				if(b_match) {
					matches++;
					if(b_match > 3) show_separator2 = true;
					else if(b_match < 3) show_separator1 = true;
				}
			} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_store), &iter));
		}
	}
	if(to_menu) current_from_struct = NULL;
	if(!to_menu && (matches > 0 && (highest_match != 1 || completion_delay <= 0 || !display_expression_status))) {
		gtk_list_store_set(completion_store, &completion_separator_iter, 3, show_separator1 && show_separator2, 4, 3, -1);
		if(show_separator1 && show_separator2) matches++;
		completion_ignore_enter = TRUE;
		completion_resize_popup(matches);
		if(cos_bak != current_object_start || current_object_end != coe_bak) return;
		if(!gtk_widget_is_visible(completion_window)) {
			if(completion_delay > 0 && on_wayland()) hide_tooltips(GTK_WIDGET(main_window()));
			gtk_window_set_transient_for(GTK_WINDOW(completion_window), main_window());
			gtk_window_group_add_window(gtk_window_get_group(main_window()), GTK_WINDOW(completion_window));
			gtk_window_set_screen(GTK_WINDOW(completion_window), gtk_widget_get_screen(expression_edit_widget()));
			gtk_widget_show(completion_window);
		}
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)));
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)));
	} else {
		gtk_widget_hide(completion_window);
	}
}

gboolean do_completion_timeout(gpointer) {
	if(!completion_blocked) do_completion();
	completion_timeout_id = 0;
	return FALSE;
}
void add_completion_timeout() {
	if(!completion_blocked) {
		if(completion_delay <= 0 || completion_visible()) {
			completion_timeout_id = gdk_threads_add_idle(do_completion_timeout, NULL);
		} else {
			completion_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, completion_delay, do_completion_timeout, NULL, NULL);
		}
	}
}
void stop_completion_timeout() {
	if(completion_timeout_id != 0) {
		g_source_remove(completion_timeout_id);
		completion_timeout_id = 0;
	}
}
void toggle_completion_visible() {
	if(gtk_widget_get_visible(completion_window)) {
		gtk_widget_hide(completion_window);
	} else {
		stop_completion_timeout();
		int cm_bak = completion_min;
		bool ec_bak = enable_completion;
		completion_min = 1;
		enable_completion = true;
		do_completion(false, true);
		completion_min = cm_bak;
		enable_completion = ec_bak;
	}
}

void reset_tabbed_completion() {
	tabbed_completion = false;
}
bool activate_previous_completion() {
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
			current_object_start = tabbed_completion_start;
			current_object_end = tabbed_completion_end;
			tabbed_completion_end -= get_expression_text().length();
			on_completion_match_selected(GTK_TREE_VIEW(gtk_builder_get_object(main_builder, "completionview")), path, NULL, NULL);
			gtk_tree_path_free(path);
			tabbed_completion_end += get_expression_text().length();
			tabbed_completion = true;
			return true;
		}
	}
	return false;
}
bool activate_first_completion() {
	if(gtk_widget_get_visible(completion_window)) {
		tabbed_completion_start = current_object_start;
		tabbed_completion_end = current_object_end;
		tabbed_completion_end -= get_expression_text().length();
		GtkTextIter object_start, object_end;
		gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_start, current_object_start);
		gtk_text_buffer_get_iter_at_offset(expression_edit_buffer(), &object_end, current_object_end);
		gchar *gstr2 = gtk_text_buffer_get_text(expression_edit_buffer(), &object_start, &object_end, FALSE);
		tabbed_completion_object = gstr2;
		g_free(gstr2);
		if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)), NULL, &tabbed_iter)) {
			gtk_tree_model_get_iter_first(completion_sort, &tabbed_iter);
		}
		GtkTreePath *path = gtk_tree_model_get_path(completion_sort, &tabbed_iter);
		on_completion_match_selected(GTK_TREE_VIEW(completion_view), path, NULL, NULL);
		gtk_tree_path_free(path);
		tabbed_completion_end += get_expression_text().length();
		tabbed_completion = true;
		return true;
	} else if(tabbed_completion) {
		current_object_start = tabbed_completion_start;
		current_object_end = tabbed_completion_end;
		tabbed_completion_end -= get_expression_text().length();
		if(!gtk_tree_model_iter_next(completion_sort, &tabbed_iter)) gtk_tree_model_get_iter_first(completion_sort, &tabbed_iter);
		GtkTreePath *path = gtk_tree_model_get_path(completion_sort, &tabbed_iter);
		on_completion_match_selected(GTK_TREE_VIEW(completion_view), path, NULL, NULL);
		gtk_tree_path_free(path);
		tabbed_completion_end += get_expression_text().length();
		tabbed_completion = true;
		return true;
	}
	stop_completion_timeout();
	bool ec_bak = enable_completion;
	enable_completion = true;
	int cm_bak = completion_min;
	completion_min = 1;
	do_completion(false, true);
	completion_min = cm_bak;
	enable_completion = ec_bak;
	return gtk_widget_get_visible(completion_window);
}
bool completion_enter_pressed() {
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)), NULL, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(completion_sort, &iter);
		on_completion_match_selected(GTK_TREE_VIEW(completion_view), path, NULL, NULL);
		gtk_tree_path_free(path);
		return true;
	}
	return false;
}
void completion_up_pressed() {
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view));
	bool b = false;
	if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		if(gtk_tree_model_iter_previous(completion_sort, &iter)) {
			gint i_prio = 0;
			gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 4, &i_prio, -1);
			if(i_prio != 3 || gtk_tree_model_iter_previous(completion_sort, &iter)) b = true;
		} else {
			gtk_tree_selection_unselect_all(selection);
		}
	} else {
		gint rows = gtk_tree_model_iter_n_children(completion_sort, NULL);
		if(rows > 0) {
			GtkTreePath *path = gtk_tree_path_new_from_indices(rows - 1, -1);
			gtk_tree_model_get_iter(completion_sort, &iter, path);
			gtk_tree_path_free(path);
			b = true;
		}
	}
	if(b) {
		gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(completion_view), FALSE);
		completion_hover_blocked = true;
		GtkTreePath *path = gtk_tree_model_get_path(completion_sort, &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(completion_view), path, NULL, FALSE, 0.0, 0.0);
		gtk_tree_selection_select_iter(selection, &iter);
		gtk_tree_path_free(path);
	}
}
void completion_down_pressed() {
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view));
	bool b = false;
	if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		if(gtk_tree_model_iter_next(completion_sort, &iter)) {
			gint i_prio = 0;
			gtk_tree_model_get(GTK_TREE_MODEL(completion_sort), &iter, 4, &i_prio, -1);
			if(i_prio != 3 || gtk_tree_model_iter_next(completion_sort, &iter)) b = true;
		} else {
			gtk_tree_selection_unselect_all(selection);
		}
	} else {
		if(gtk_tree_model_get_iter_first(completion_sort, &iter)) b = true;
	}
	if(b) {
		gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(completion_view), FALSE);
		completion_hover_blocked = true;
		GtkTreePath *path = gtk_tree_model_get_path(completion_sort, &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(completion_view), path, NULL, FALSE, 0.0, 0.0);
		gtk_tree_selection_select_iter(selection, &iter);
		gtk_tree_path_free(path);
	}
}

bool ellipsize_completion_names(string &str) {
	if(str.length() < 50) return false;
	size_t l = 0, l_insub = 0, first_i = 0;
	bool insub = false;
	for(size_t i = 0; i < str.length(); i++) {
		if(str[i] == '<') {
			if(i + 1 == str.length()) break;
			if(str[i + 1] == 's') {insub = true; l_insub = l;}
			else if(insub && str[i + 1] == '/') {insub = false; l -= ((l - l_insub) * 6) / 10;}
			else if(first_i == 0 && str[i + 1] == 'i') first_i = i;
			i = str.find('>', i + 1);
			if(i == string::npos) break;
		} else if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			if(first_i > 0 && l >= 35 && (!insub || l - ((l - l_insub) * 6) / 10 >= 35) && i < str.length() - 2 && str[i + 1] != '<' && str[i + 1] != ')' && str[i + 1] != '(') {
				str = str.substr(0, i);
				str += "…";
				if(insub) str += "</sub>";
				str += "</i>";
				return true;
			}
			l++;
		}
	}
	return false;
}

bool name_has_formatting(const ExpressionName *ename) {
	if(ename->name.length() < 2) return false;
	if(ename->suffix) return true;
	if(ename->completion_only || ename->case_sensitive || ename->name.length() <= 4) return false;
	size_t i = ename->name.find('_');
	if(i == string::npos) return false;
	return unicode_length(ename->name, i) >= 3;
}
string format_name(const ExpressionName *ename, int type) {
	bool was_capitalized = false;
	string name = ename->formattedName(type, true, true, 0, false, false, NULL, &was_capitalized);
	if(was_capitalized) {
		if(ename->suffix) {
			string str = name;
			size_t i = str.find("<sub>");
			if(i != string::npos) {
				str.erase(str.length() - 6, 6);
				str.erase(i, 5);
				char *cap_str = utf8_strup(str.c_str() + sizeof(char) * i);
				if(cap_str) {
					str.replace(i, str.length() - i, cap_str);
					free(cap_str);
				}
			}
			capitalized_names[ename] = str;
		} else {
			capitalized_names[ename] = name;
		}
	}
	return name;
}

#define MAX_TITLE_LENGTH 45

void ellipsize_title(string &str) {
	if(unicode_length(str) > MAX_TITLE_LENGTH) {
		size_t l = 0;
		for(size_t i = 0; i < str.length(); i++) {
			if(str[i] == '(' && l > MAX_TITLE_LENGTH - 10) {
				str.resize(i);
				break;
			}
			if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
				l++;
				if(l >= MAX_TITLE_LENGTH - 4) {
					str.resize(i);
					str += "…";
					break;
				}
			}
		}
	}
}

void update_completion() {

	GtkTreeIter iter;
	gtk_list_store_clear(completion_store);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(completion_store), 1, GTK_SORT_ASCENDING);
	capitalized_names.clear();

	FIX_SUPSUB_PRE(completion_view, fix_supsub_completion)

	string str, title, title2;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(CALCULATOR->functions[i]->isActive()) {
			gtk_list_store_append(completion_store, &iter);
			const ExpressionName *ename, *ename_r;
			ename_r = &CALCULATOR->functions[i]->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			if(name_has_formatting(ename_r)) str = format_name(ename_r, TYPE_FUNCTION);
			else str = ename_r->name;
			str += "()";
			for(size_t name_i = 1; name_i <= CALCULATOR->functions[i]->countNames(); name_i++) {
				ename = &CALCULATOR->functions[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget()))) {
					str += " <i>";
					if(name_has_formatting(ename)) str += format_name(ename, TYPE_FUNCTION);
					else str += ename->name;
					str += "()</i>";
				}
			}
			ellipsize_completion_names(str);
			FIX_SUPSUB(str)
			title = CALCULATOR->functions[i]->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) completion_view);
			ellipsize_title(title);
			gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, CALCULATOR->functions[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, NULL, -1);
		}
	}
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isActive()) {
			gtk_list_store_append(completion_store, &iter);
			const ExpressionName *ename, *ename_r;
			bool b = false;
			ename_r = &CALCULATOR->variables[i]->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
			for(size_t name_i = 1; name_i <= CALCULATOR->variables[i]->countNames(); name_i++) {
				ename = &CALCULATOR->variables[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget()))) {
					if(!b) {
						if(name_has_formatting(ename_r)) str = format_name(ename_r, TYPE_VARIABLE);
						else str = ename_r->name;
						b = true;
					}
					str += " <i>";
					if(name_has_formatting(ename)) str += format_name(ename, TYPE_VARIABLE);
					else str += ename->name;
					str += "</i>";
				}
			}
			if(!b && name_has_formatting(ename_r)) {
				str = format_name(ename_r, TYPE_VARIABLE);
				b = true;
			}
			if(printops.use_unicode_signs && can_display_unicode_string_function("→", (void*) expression_edit_widget())) {
				size_t pos = 0;
				if(b) {
					pos = str.find("_to_");
				} else {
					pos = ename_r->name.find("_to_");
					if(pos != string::npos) {
						str = ename_r->name;
						b = true;
					}
				}
				if(b) {
					while(pos != string::npos) {
						if((pos == 1 && str[0] == 'm') || (pos > 1 && str[pos - 1] == 'm' && str[pos - 2] == '>')) {
							str.replace(pos, 4, "<span size=\"small\"><sup>-1</sup></span>→");
						} else {
							str.replace(pos, 4, "→");
						}
						pos = str.find("_to_", pos);
					}
				}
			}
			ellipsize_completion_names(str);
			FIX_SUPSUB(str)
			if(!CALCULATOR->variables[i]->title(false).empty()) {
				title = CALCULATOR->variables[i]->title();
				if(printops.use_unicode_signs) gsub("MeV/c^2", "MeV/c²", title);
				ellipsize_title(title);
				if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, NULL, -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, NULL, -1);
				if(CALCULATOR->variables[i] == CALCULATOR->v_percent) gtk_list_store_set(completion_store, &iter, 9, (CALCULATOR->variables[i]->title() + " (%)").c_str(), -1);
				else if(CALCULATOR->variables[i] == CALCULATOR->v_permille) gtk_list_store_set(completion_store, &iter, 9, (CALCULATOR->variables[i]->title() + " (‰)").c_str(), -1);
			} else {
				Variable *v = CALCULATOR->variables[i];
				string title;
				if(is_answer_variable(v)) {
					title = _("a previous result");
				} else if(v->isKnown()) {
					if(((KnownVariable*) v)->isExpression() && !v->isLocal()) {
						title = localize_expression(((KnownVariable*) v)->expression());
						if(unicode_length(title) > 30) {
							size_t n = 30;
							while(n > 0 && (signed char) title[n] < 0 && (unsigned char) title[n] < 0xC0) n--;
							title = title.substr(0, n); title += "…";
						} else if(!((KnownVariable*) v)->unit().empty() && ((KnownVariable*) v)->unit() != "auto") {
							title += " "; title += ((KnownVariable*) v)->unit();
						}
					} else {
						if(((KnownVariable*) v)->get().isMatrix()) {
							title = _("matrix");
						} else if(((KnownVariable*) v)->get().isVector()) {
							title = _("vector");
						} else {
							PrintOptions po = printops;
							po.can_display_unicode_string_arg = (void*) completion_view;
							po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;;
							po.base = 10;
							po.number_fraction_format = FRACTION_DECIMAL_EXACT;
							po.allow_non_usable = true;
							po.is_approximate = NULL;
							title = CALCULATOR->print(((KnownVariable*) v)->get(), 30, po);
							if(unicode_length(title) > 30) {
								size_t n = 30;
								while(n > 0 && (signed char) title[n] < 0 && (unsigned char) title[n] < 0xC0) n--;
								title = title.substr(0, n);
								title += "…";
							}
						}
					}
				} else {
					if(((UnknownVariable*) v)->assumptions()) {
						switch(((UnknownVariable*) v)->assumptions()->sign()) {
							case ASSUMPTION_SIGN_POSITIVE: {title = _("positive"); break;}
							case ASSUMPTION_SIGN_NONPOSITIVE: {title = _("non-positive"); break;}
							case ASSUMPTION_SIGN_NEGATIVE: {title = _("negative"); break;}
							case ASSUMPTION_SIGN_NONNEGATIVE: {title = _("non-negative"); break;}
							case ASSUMPTION_SIGN_NONZERO: {title = _("non-zero"); break;}
							default: {}
						}
						if(!title.empty() && ((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_NONE) title += " ";
						switch(((UnknownVariable*) v)->assumptions()->type()) {
							case ASSUMPTION_TYPE_BOOLEAN: {title += _("boolean"); break;}
							case ASSUMPTION_TYPE_INTEGER: {title += _("integer"); break;}
							case ASSUMPTION_TYPE_RATIONAL: {title += _("rational"); break;}
							case ASSUMPTION_TYPE_REAL: {title += _("real"); break;}
							case ASSUMPTION_TYPE_COMPLEX: {title += _("complex"); break;}
							case ASSUMPTION_TYPE_NUMBER: {title += _("number"); break;}
							case ASSUMPTION_TYPE_NONMATRIX: {title += _("(not matrix)"); break;}
							default: {}
						}
						if(title.empty()) title = _("unknown");
					} else {
						title = _("default assumptions");
					}
				}
				if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, NULL, -1);
				else if(ename_r) gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, NULL, -1);
			}
		}
	}
	PrintOptions po = printops;
	po.is_approximate = NULL;
	po.can_display_unicode_string_arg = (void*) expression_edit_widget();
	po.abbreviate_names = true;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		Unit *u = CALCULATOR->units[i];
		if(u->isActive()) {
			CompositeUnit *cu = (u->subtype() == SUBTYPE_COMPOSITE_UNIT ? (CompositeUnit*) u : NULL);
			if(!cu && u->isHidden() && u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT && !((AliasUnit*) u)->firstBaseUnit()->isHidden() && ((AliasUnit*) u)->firstBaseUnit()->isActive()) {
				cu = (CompositeUnit*) ((AliasUnit*) u)->firstBaseUnit();
			}
			if(!cu) {
				gtk_list_store_append(completion_store, &iter);
				const ExpressionName *ename, *ename_r;
				bool b = false;
				ename_r = &u->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
				for(size_t name_i = 1; name_i <= u->countNames(); name_i++) {
					ename = &u->getName(name_i);
					if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget()))) {
						if(!b) {
							if(name_has_formatting(ename_r)) str = format_name(ename_r, TYPE_UNIT);
							else str = ename_r->name;
							b = true;
						}
						str += " <i>";
						if(name_has_formatting(ename)) str += format_name(ename, TYPE_UNIT);
						else str += ename->name;
						str += "</i>";
					}
				}
				if(!b && name_has_formatting(ename_r)) {
					str = format_name(ename_r, TYPE_UNIT);
					b = true;
				} else {
					ellipsize_completion_names(str);
				}
				FIX_SUPSUB(str)
				unordered_map<string, cairo_surface_t*>::const_iterator it_flag = flag_surfaces.end();
				if(!u->title(false).empty()) {
					title2 = u->title(false);
					ename = &u->preferredInputName(true, printops.use_unicode_signs, false, u->isCurrency(), &can_display_unicode_string_function, (void*) expression_edit_widget());
					if(ename->abbreviation) {
						bool tp = title2[title2.length() - 1] == ')';
						title2 += " ";
						if(!tp) title2 += "(";
						if(name_has_formatting(ename)) title2 += format_name(ename, TYPE_UNIT);
						else title2 += ename->name;
						if(!tp) title2 += ")";
						if(title2.find("_") != string::npos) title2 = "";
						FIX_SUPSUB(title2)
					}
				}
				title = u->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) completion_view);
				if(u->isCurrency()) {
					it_flag = flag_surfaces.find(u->referenceName());
				} else if(u->isSIUnit() && !u->category().empty() && title[title.length() - 1] != ')') {
					size_t i_slash = string::npos;
					if(u->category().length() > 1) i_slash = u->category().rfind("/", u->category().length() - 2);
					if(i_slash != string::npos) i_slash++;
					if(title.length() + u->category().length() - (i_slash == string::npos ? 0 : i_slash) < 35) {
						title += " (";
						if(i_slash == string::npos) title += u->category();
						else title += u->category().substr(i_slash, u->category().length() - i_slash);
						title += ")";
					}
				}
				if(unicode_length(title) > MAX_TITLE_LENGTH) {
					if(title2.empty()) title2 = title;
					ellipsize_title(title);
				}
				if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, u, 3, FALSE, 4, 0, 5, it_flag == flag_surfaces.end() ? NULL : it_flag->second, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, title2.empty() ? NULL : title2.c_str(), -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, title.c_str(), 2, u, 3, FALSE, 4, 0, 5, it_flag == flag_surfaces.end() ? NULL : it_flag->second, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, title2.empty() ? NULL : title2.c_str(), -1);
			} else if(!cu->isHidden()) {
				Prefix *prefix = NULL;
				int exp = 1;
				title = cu->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) completion_view);
				title2 = title;
				bool tp = title2[title2.length() - 1] == ')';
				title2 += " ";
				if(!tp) title2 += "(";
				Unit *u_i = NULL;
				if(cu->countUnits() == 1 && (u_i = cu->get(1, &exp, &prefix)) != NULL && prefix != NULL && exp == 1) {
					str = "";
					for(size_t name_i = 0; name_i < 2; name_i++) {
						const ExpressionName *ename;
						ename = &prefix->preferredInputName(name_i != 1, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
						if(!ename->name.empty() && (ename->abbreviation == (name_i != 1))) {
							bool b_italic = !str.empty();
							if(b_italic) str += " <i>";
							str += ename->formattedName(-1, false, true);
							str += u_i->preferredInputName(name_i != 1, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_UNIT, true, true);
							if(b_italic) str += "</i>";
							if(!b_italic) title2 += str;
						}
					}
					ellipsize_completion_names(str);
				} else {
					str = cu->print(po, true, TAG_TYPE_HTML, true, false);
					title2 += str;
				}
				if(!tp) title2 += ")";
				FIX_SUPSUB(str)
				FIX_SUPSUB(title2)
				gtk_list_store_append(completion_store, &iter);
				size_t i_slash = string::npos;
				if(cu->category().length() > 1) i_slash = cu->category().rfind("/", cu->category().length() - 2);
				if(i_slash != string::npos) i_slash++;
				if(cu->isSIUnit() && !cu->category().empty()) {
					if(title.length() + cu->category().length() - (i_slash == string::npos ? 0 : i_slash) < 35 && title[title.length() - 1] != ')') {
						title += " (";
						if(i_slash == string::npos) title += cu->category();
						else title += cu->category().substr(i_slash, cu->category().length() - i_slash);
						title += ")";
					} else {
						if(i_slash == string::npos) title = cu->category();
						else title = cu->category().substr(i_slash, cu->category().length() - i_slash);
					}
				}
				ellipsize_title(title);
				gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, u, 3, FALSE, 4, 0, 5, NULL, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, 9, title2.c_str(), -1);
			}
		}
	}
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(completion_view), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
	for(size_t i = 1; ; i++) {
		Prefix *p = CALCULATOR->getPrefix(i);
		if(!p) break;
		gtk_list_store_append(completion_store, &iter);
		str = "";
		const ExpressionName *ename, *ename_r;
		ename_r = &p->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
		str = ename_r->formattedName(-1, false, true);
		for(size_t name_i = 1; name_i <= p->countNames(); name_i++) {
			ename = &p->getName(name_i);
			if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expression_edit_widget()))) {
				str += " <i>";
				str += ename->formattedName(-1, false, true);
				if(ename->name != "k" && ename->name != "M") str += "-";
				str += "</i>";
			}
		}
		ellipsize_completion_names(str);
		title = _("Prefix"); title += ": ";
		switch(p->type()) {
			case PREFIX_DECIMAL: {
				title +="10<sup>";
				title += i2s(((DecimalPrefix*) p)->exponent());
				title += "</sup>";
				break;
			}
			case PREFIX_BINARY: {
				title +=" 2<sup>";
				title += i2s(((BinaryPrefix*) p)->exponent());
				title += "</sup>";
				break;
			}
			case PREFIX_NUMBER: {
				title += ((NumberPrefix*) p)->value().print();
			}
		}
		FIX_SUPSUB(title);
		gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, p, 3, FALSE, 4, 0, 5, NULL, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 2, 9, NULL, -1);
	}
	pango_font_description_free(font_desc);
	string str2;
#define COMPLETION_CONVERT_STRING(x) str = _(x); if(str != x) {str += " <i>"; str += x; str += "</i>";}
#define COMPLETION_CONVERT_STRING2(x, y) str = _(x); if(str != x) {str += " <i>"; str += x; str += "</i>";} str2 = _(y);  str += " <i>"; str += str2; str += "</i>"; if(str2 != y) {str += " <i>"; str += y; str += "</i>";}
	COMPLETION_CONVERT_STRING2("angle", "phasor")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Complex Angle/Phasor Notation"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 400, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("bases")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Number Bases"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 201, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("base")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Base Units"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 101, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("base ")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Number Base"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 200, 9, NULL, -1);
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, "bcd", 1, _("Binary-Coded Decimal"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 285, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("bijective")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Bijective Base-26"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 290, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("binary") str += " <i>"; str += "bin"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Binary Number"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 202, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("calendars")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Calendars"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 500, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("cis")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Complex cis Form"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 401, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("decimals")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Decimal Fraction"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 302, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("decimal") str += " <i>"; str += "dec"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Decimal Number"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 210, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("duodecimal") str += " <i>"; str += "duo"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Duodecimal Number"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 212, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("engineering") str += " <i>"; str += "eng"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Engineering Notation"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 702, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("exponential")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Complex Exponential Form"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 402, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("factors")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Factors"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 600, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("fp16") str += " <i>"; str += "binary16"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("16-bit Floating Point Binary Format"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 310, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("fp32") str += " <i>"; str += "binary32"; str += "</i>"; str += " <i>"; str += "float"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("32-bit Floating Point Binary Format"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 311, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("fp64") str += " <i>"; str += "binary64"; str += "</i>"; str += " <i>"; str += "double"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("64-bit Floating Point Binary Format"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 312, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("fp80");
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("80-bit (x86) Floating Point Binary Format"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 313, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("fp128") str += " <i>"; str += "binary128"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("128-bit Floating Point Binary Format"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 314, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("fraction")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Fraction"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 300, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("1/")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, (string(_("Fraction")) + " (1/n)").c_str(), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 301, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("hexadecimal") str += " <i>"; str += "hex"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Hexadecimal Number"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 216, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("latitude") str += " <i>"; str += "latitude2"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Latitude"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 294, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("longitude") str += " <i>"; str += "longitude2"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Longitude"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 295, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("mixed")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Mixed Units"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 102, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("octal") str += " <i>"; str += "oct"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Octal Number"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 208, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("optimal")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Optimal Units"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 100, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("partial fraction")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Expanded Partial Fractions"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 601, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("polar")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Complex Polar Form"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 403, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("prefix")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Optimal Prefix"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 103, 9, NULL, -1);
	COMPLETION_CONVERT_STRING2("rectangular", "cartesian")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Complex Rectangular Form"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 404, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("roman")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Roman Numerals"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 280, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("scientific") str += " <i>"; str += "sci"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Scientific Notation"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 701, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("sexagesimal") str += " <i>"; str += "sexa"; str += "</i>"; str += " <i>"; str += "sexa2"; str += "</i>"; str += " <i>"; str += "sexa3"; str += "</i>";
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Sexagesimal Number"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 292, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("simple")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Simple Notation"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 703, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("time")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Time Format"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 293, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("unicode")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("Unicode"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 281, 9, NULL, -1);
	COMPLETION_CONVERT_STRING("utc")
	gtk_list_store_append(completion_store, &iter); gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, _("UTC Time Zone"), 2, NULL, 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 501, 9, NULL, -1);
	gtk_list_store_append(completion_store, &completion_separator_iter); gtk_list_store_set(completion_store, &completion_separator_iter, 0, "", 1, "", 2, NULL, 3, FALSE, 4, 3, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 0, 9, NULL, -1);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(completion_store), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
}
void completion_font_modified() {
	fix_supsub_completion = test_supsub(completion_view);
}

void create_expression_completion() {

	completion_view = GTK_WIDGET(gtk_builder_get_object(main_builder, "completionview"));
	gtk_style_context_add_provider(gtk_widget_get_style_context(completion_view), GTK_STYLE_PROVIDER(expression_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	completion_scrolled = GTK_WIDGET(gtk_builder_get_object(main_builder, "completionscrolled"));
	gtk_widget_set_size_request(gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(completion_scrolled)), -1, 0);
	completion_window = GTK_WIDGET(gtk_builder_get_object(main_builder, "completionwindow"));
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 20
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 20)) {
		GtkCssProvider *completion_provider = gtk_css_provider_new();
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "completionview"))), GTK_STYLE_PROVIDER(completion_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_css_provider_load_from_data(completion_provider, "* {font-size: medium;}", -1, NULL);
	}
#endif

	completion_store = gtk_list_store_new(10, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_INT, CAIRO_GOBJECT_TYPE_SURFACE, G_TYPE_INT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_STRING);
	completion_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(completion_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(completion_filter), 3);
	completion_sort = gtk_tree_model_sort_new_with_model(completion_filter);
	gtk_tree_view_set_model(GTK_TREE_VIEW(completion_view), completion_sort);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(completion_view), completion_row_separator_func, NULL, NULL);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkCellArea *area = gtk_cell_area_box_new();
	gtk_cell_area_box_set_spacing(GTK_CELL_AREA_BOX(area), 12);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_area(area);
	gtk_cell_area_box_pack_start(GTK_CELL_AREA_BOX(area), renderer, TRUE, TRUE, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "markup", 0, "weight", 6, NULL);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_padding(renderer, 2, 0);
	gtk_cell_area_box_pack_end(GTK_CELL_AREA_BOX(area), renderer, FALSE, TRUE, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "surface", 5, NULL);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "style", PANGO_STYLE_ITALIC, NULL);
	gtk_cell_area_box_pack_end(GTK_CELL_AREA_BOX(area), renderer, FALSE, TRUE, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "markup", 1, "weight", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(completion_view), column);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(completion_store), 1, string_sort_func, GINT_TO_POINTER(1), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(completion_store), 1, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(completion_sort), 1, completion_sort_func, GINT_TO_POINTER(1), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(completion_sort), 1, GTK_SORT_ASCENDING);

	completion_font_modified();

	gtk_builder_add_callback_symbols(main_builder, "on_completionwindow_button_press_event", G_CALLBACK(on_completionwindow_button_press_event), "on_completionwindow_key_press_event", G_CALLBACK(on_completionwindow_key_press_event), "on_completionview_enter_notify_event", G_CALLBACK(on_completionview_enter_notify_event), "on_completionview_motion_notify_event", G_CALLBACK(on_completionview_motion_notify_event), "on_completion_match_selected", G_CALLBACK(on_completion_match_selected), NULL);

}
