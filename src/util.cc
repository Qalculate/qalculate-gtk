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
#include "expressionedit.h"
#include "util.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::stack;

string unhtmlize(string str, bool b_ascii) {
	size_t i = 0, i2;
	while(true) {
		i = str.find("<", i);
		if(i == string::npos || i == str.length() - 1) break;
		i2 = str.find(">", i + 1);
		if(i2 == string::npos) break;
		if((i2 - i == 3 && str.substr(i + 1, 2) == "br") || (i2 - i == 4 && str.substr(i + 1, 3) == "/tr")) {
			str.replace(i, i2 - i + 1, "\n");
			continue;
		} else if(i2 - i == 4) {
			if(str.substr(i + 1, 3) == "sup") {
				size_t i3 = str.find("</sup>", i2 + 1);
				if(i3 != string::npos) {
					string str2 = unhtmlize(str.substr(i + 5, i3 - i - 5), b_ascii);
					if(!b_ascii && str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(!b_ascii && str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else if(str.length() == i3 + 6 && (unicode_length(str2) == 1 || str2.find_first_not_of(NUMBERS) == string::npos)) str.replace(i, i3 - i + 6, string("^") + str2);
					else str.replace(i, i3 - i + 6, string("^(") + str2 + ")");
					continue;
				}
			} else if(str.substr(i + 1, 3) == "sub") {
				size_t i3 = str.find("</sub>", i + 4);
				if(i3 != string::npos) {
					if(i3 - i2 > 16 && str.substr(i2 + 1, 7) == "<small>" && str.substr(i3 - 8, 8) == "</small>") str.erase(i, i3 - i + 6);
					else str.replace(i, i3 - i + 6, string("_") + unhtmlize(str.substr(i + 5, i3 - i - 5), b_ascii));
					continue;
				}
			}
		} else if(i2 - i == 17 && str.substr(i + 1, 16) == "i class=\"symbol\"") {
			size_t i3 = str.find("</i>", i2 + 1);
			if(i3 != string::npos) {
				string name = unhtmlize(str.substr(i2 + 1, i3 - i2 - 1), b_ascii);
				if(name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, 1, '\\');
				} else {
					name.insert(0, 1, '\"');
					name += '\"';
				}
				str.replace(i, i3 - i + 4, name);
				continue;
			}
		}
		str.erase(i, i2 - i + 1);
	}
	gsub(" " SIGN_DIVISION_SLASH " ", "/", str);
	gsub("&amp;", "&", str);
	gsub("&gt;", ">", str);
	gsub("&lt;", "<", str);
	gsub("&quot;", "\"", str);
	gsub("&hairsp;", "", str);
	gsub("&nbsp;", NBSP, str);
	gsub("&thinsp;", THIN_SPACE, str);
	gsub("&#8239;", NNBSP, str);
	return str;
}

void remove_separator(string &copy_text) {
	for(size_t i = ((CALCULATOR->local_digit_group_separator.empty() || CALCULATOR->local_digit_group_separator == " " || CALCULATOR->local_digit_group_separator == printops.decimalpoint()) ? 1 : 0); i < 4; i++) {
		string str_sep;
		if(i == 0) str_sep = CALCULATOR->local_digit_group_separator;
		else if(i == 1) str_sep = THIN_SPACE;
		else if(i == 2) str_sep = NNBSP;
		else str_sep = " ";
		size_t index = copy_text.find(str_sep);
		while(index != string::npos) {
			if(index > 0 && index + str_sep.length() < copy_text.length() && copy_text[index - 1] >= '0' && copy_text[index - 1] <= '9' && copy_text[index + str_sep.length()] >= '0' && copy_text[index + str_sep.length()] <= '9') {
				copy_text.erase(index, str_sep.length());
			} else {
				index++;
			}
			index = copy_text.find(str_sep, index);
		}
	}
}

string unformat(string str) {
	remove_separator(str);
	gsub(SIGN_MINUS, "-", str);
	gsub(SIGN_MULTIPLICATION, "*", str);
	gsub(SIGN_MULTIDOT, "*", str);
	gsub(SIGN_MIDDLEDOT, "*", str);
	gsub(THIN_SPACE, " ", str);
	gsub(NNBSP, " ", str);
	gsub(NBSP, " ", str);
	gsub(SIGN_DIVISION, "/", str);
	gsub(SIGN_DIVISION_SLASH, "/", str);
	gsub(SIGN_SQRT, "sqrt", str);
	gsub("Ω", "ohm", str);
	gsub("µ", "u", str);
	return str;
}

string print_with_evalops(const Number &nr) {
	PrintOptions po;
	po.is_approximate = NULL;
	po.base = evalops.parse_options.base;
	po.base_display = BASE_DISPLAY_NONE;
	po.twos_complement = evalops.parse_options.twos_complement;
	po.hexadecimal_twos_complement = evalops.parse_options.hexadecimal_twos_complement;
	if(((po.base == 2 && po.twos_complement) || (po.base == 16 && po.hexadecimal_twos_complement)) && nr.isNegative()) po.binary_bits = evalops.parse_options.binary_bits;
	po.min_exp = EXP_NONE;
	Number nr_base;
	if(po.base == BASE_CUSTOM) {
		nr_base = CALCULATOR->customOutputBase();
		CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
	}
	if(po.base == BASE_CUSTOM && CALCULATOR->customInputBase().isInteger() && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
		nr_base = CALCULATOR->customOutputBase();
		CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
	} else if((po.base < BASE_CUSTOM && po.base != BASE_BINARY_DECIMAL && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26 && po.base != BASE_BINARY_DECIMAL) || (po.base == BASE_CUSTOM && CALCULATOR->customInputBase() <= 12 && CALCULATOR->customInputBase() >= -12)) {
		po.base = 10;
		string str = "dec(";
		str += nr.print(po);
		str += ")";
		return str;
	} else if(po.base == BASE_CUSTOM) {
		po.base = 10;
	}
	string str = nr.print(po);
	if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
	return str;
}

bool equalsIgnoreCase(const string &str1, const string &str2, size_t i2, size_t i2_end, size_t minlength) {
	if(str1.empty() || str2.empty()) return false;
	size_t l = 0;
	if(i2_end == string::npos) i2_end = str2.length();
	for(size_t i1 = 0;; i1++, i2++) {
		if(i2 >= i2_end) {
			return i1 >= str1.length();
		}
		if(i1 >= str1.length()) break;
		if(((signed char) str1[i1] < 0 && i1 + 1 < str1.length()) || ((signed char) str2[i2] < 0 && i2 + 1 < str2.length())) {
			size_t iu1 = 1, iu2 = 1;
			size_t n1 = 1, n2 = 1;
			if((signed char) str1[i1] < 0) {
				while(iu1 + i1 < str1.length() && (signed char) str1[i1 + iu1] < 0) {
					if((unsigned char) str1[i1 + iu1] >= 0xC0) n1++;
					iu1++;
				}
			}
			if((signed char) str2[i2] < 0) {
				while(iu2 + i2 < str2.length() && (signed char) str2[i2 + iu2] < 0) {
					if((unsigned char) str2[i2 + iu2] >= 0xC0) {
						if(n1 == n2) break;
						n2++;
					}
					iu2++;
				}
			}
			if(n1 != n2) return false;
			bool isequal = (iu1 == iu2);
			if(isequal) {
				for(size_t i = 0; i < iu1; i++) {
					if(str1[i1 + i] != str2[i2 + i]) {
						isequal = false;
						break;
					}
				}
			}
			if(!isequal) {
				char *gstr1 = utf8_strdown(str1.c_str() + (sizeof(char) * i1), iu1);
				if(!gstr1) return false;
				char *gstr2 = utf8_strdown(str2.c_str() + (sizeof(char) * i2), iu2);
				if(!gstr2) {
					free(gstr1);
					return false;
				}
				bool b = strcmp(gstr1, gstr2) == 0;
				free(gstr1);
				free(gstr2);
				if(!b) return false;
			}
			i1 += iu1 - 1;
			i2 += iu2 - 1;
		} else if(str1[i1] != str2[i2] && !((str1[i1] >= 'a' && str1[i1] <= 'z') && str1[i1] - 32 == str2[i2]) && !((str1[i1] <= 'Z' && str1[i1] >= 'A') && str1[i1] + 32 == str2[i2])) {
			return false;
		}
		l++;
	}
	return l >= minlength;
}

bool entry_in_quotes(GtkEntry *w) {
	if(!w) return false;
	gint pos = -1;
	g_object_get(w, "cursor-position", &pos, NULL);
	if(pos >= 0) {
		const gchar *gtext = gtk_entry_get_text(GTK_ENTRY(w));
		bool in_cit1 = false, in_cit2 = false;
		for(gint i = 0; gtext && i < pos; i++) {
			if(!in_cit2 && gtext[0] == '\"') {
				in_cit1 = !in_cit1;
			} else if(!in_cit1 && gtext[0] == '\'') {
				in_cit2 = !in_cit2;
			}
			gtext = g_utf8_next_char(gtext);
		}
		return in_cit1 || in_cit2;
	}
	return false;
}

extern bool block_input;
const gchar *key_press_get_symbol(GdkEventKey *event, bool do_caret_as_xor, bool unit_expression) {
	if(block_input && (event->keyval == GDK_KEY_q || event->keyval == GDK_KEY_Q) && !(event->state & GDK_CONTROL_MASK)) {block_input = false; return "";}
	guint state = CLEAN_MODIFIERS(event->state);
	FIX_ALT_GR
	state = state & ~GDK_SHIFT_MASK;
	if(state == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_asciicircum: {}
			case GDK_KEY_dead_circumflex: {
				bool input_xor = !do_caret_as_xor || !caret_as_xor;
				return input_xor ? " xor " : "^";
			}
			case GDK_KEY_KP_Multiply: {}
			case GDK_KEY_asterisk: {
				return "^";
			}
		}
	}
	if(state != 0) return NULL;
	switch(event->keyval) {
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
			bool input_xor = !do_caret_as_xor && caret_as_xor;
			return input_xor ? " xor " : "^";
		}
		case GDK_KEY_KP_Multiply: {}
		case GDK_KEY_asterisk: {
			return times_sign(unit_expression);
		}
		case GDK_KEY_KP_Divide: {}
		case GDK_KEY_slash: {
			return divide_sign();
		}
		case GDK_KEY_KP_Subtract: {}
		case GDK_KEY_minus: {
			return sub_sign();
		}
		case GDK_KEY_KP_Add: {}
		case GDK_KEY_plus: {
			return "+";
		}
		case GDK_KEY_braceleft: {}
		case GDK_KEY_braceright: {
			return "";
		}
	}
	return NULL;
}

string localize_expression(string str, bool unit_expression) {
	ParseOptions pa = evalops.parse_options; pa.base = 10;
	str = CALCULATOR->localizeExpression(str, pa);
	gsub("*", times_sign(unit_expression), str);
	gsub("/", divide_sign(), str);
	gsub("-", sub_sign(), str);
	return str;
}

string unlocalize_expression(string str) {
	ParseOptions pa = evalops.parse_options; pa.base = 10;
	str = CALCULATOR->unlocalizeExpression(str, pa);
	CALCULATOR->parseSigns(str);
	return str;
}

unordered_map<void*, unordered_map<const char*, int> > coverage_map;
extern GtkWidget *historyview;

PangoCoverageLevel get_least_coverage(const gchar *gstr, GtkWidget *widget) {

	PangoCoverageLevel level = PANGO_COVERAGE_EXACT;
	PangoContext *context = gtk_widget_get_pango_context(widget);
	PangoLanguage *language = pango_context_get_language(context);
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(widget), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
	PangoFontset *fontset = pango_context_load_fontset(context, font_desc, language);
	pango_font_description_free(font_desc);
	while(gstr[0] != '\0') {
		if((signed char) gstr[0] < 0) {
			gunichar gu = g_utf8_get_char_validated(gstr, -1);
			if(gu != (gunichar) -1 && gu != (gunichar) -2) {
				PangoFont *font = pango_fontset_get_font(fontset, (guint) gu);
				if(font) {
					PangoCoverage *coverage = pango_font_get_coverage(font, language);
					if(pango_coverage_get(coverage, (int) gu) < level) {
						level = pango_coverage_get(coverage, gu);
					}
					g_object_unref(font);
#if PANGO_VERSION >= 15200
					g_object_unref(coverage);
#else
					pango_coverage_unref(coverage);
#endif
				} else {
					level = PANGO_COVERAGE_NONE;
				}
			}
		}
		gstr = g_utf8_find_next_char(gstr, NULL);
		if(!gstr) break;
	}
	g_object_unref(fontset);
	return level;

}
bool can_display_unicode_string_function(const char *str, void *w) {
	if(!w) w = (void*) historyview;
	unordered_map<void*, unordered_map<const char*, int> >::iterator it1 = coverage_map.find(w);
	if(it1 == coverage_map.end()) {
		coverage_map[w] = unordered_map<const char*, int>();
	} else {
		unordered_map<const char*, int>::iterator it = it1->second.find(str);
		if(it != it1->second.end()) return it->second;
	}
	coverage_map[w][str] = get_least_coverage(str, (GtkWidget*) w);
	return coverage_map[w][str] >= PANGO_COVERAGE_APPROXIMATE;
}
bool can_display_unicode_string_function_exact(const char *str, void *w) {
	if(!w) w = (void*) historyview;
	unordered_map<void*, unordered_map<const char*, int> >::iterator it1 = coverage_map.find(w);
	if(it1 == coverage_map.end()) {
		coverage_map[w] = unordered_map<const char*, int>();
	} else {
		unordered_map<const char*, int>::iterator it = it1->second.find(str);
		if(it != it1->second.end()) return it->second;
	}
	coverage_map[w][str] = get_least_coverage(str, (GtkWidget*) w);
	return coverage_map[w][str] >= PANGO_COVERAGE_EXACT;
}

bool test_supsub(GtkWidget *w) {
	PangoLayout *layout_test = gtk_widget_create_pango_layout(w, NULL);
	pango_layout_set_markup(layout_test, "x", -1);
	PangoRectangle rect;
	pango_layout_get_pixel_extents(layout_test, NULL, &rect);
	cairo_surface_t *tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rect.width, rect.height);
	cairo_t *cr = cairo_create(tmp_surface);
	pango_cairo_show_layout(cr, layout_test);
	cairo_destroy(cr);
	int y1, y2;
	get_image_blank_height(tmp_surface, &y1, &y2);
	cairo_surface_destroy(tmp_surface);
	pango_layout_set_markup(layout_test, "x<sup>1</sup>", -1);
	pango_layout_get_pixel_extents(layout_test, NULL, &rect);
	tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rect.width, rect.height);
	cr = cairo_create(tmp_surface);
	pango_cairo_show_layout(cr, layout_test);
	cairo_destroy(cr);
	int y3, y4;
	get_image_blank_height(tmp_surface, &y3, &y4);
	cairo_surface_destroy(tmp_surface);
	g_object_unref(layout_test);
	return y2 == y4;
}

void get_image_blank_width(cairo_surface_t *surface, int *x1, int *x2) {
	int w = cairo_image_surface_get_width(surface);
	int h = cairo_image_surface_get_height(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	int first_col = w;
	int last_col = -1;
	for(int i = 0; i < h; i++) {
		unsigned char *row = data + i * stride;
		if(x1) {
			for(int j = 0; j < first_col; j++) {
				for(int s_i = 0; s_i < 4; s_i++) {
					if(*(row + 4 * j + s_i) != 0) {
						first_col = j;
						if(first_col > last_col) last_col = first_col;
						break;
					}
				}
			}
		}
		if((first_col != w || !x1) && x2) {
			for(int j = w - 1; j > last_col; j--) {
				for(int s_i = 0; s_i < 4; s_i++) {
					if(*(row + 4 * j + s_i) != 0) {
						last_col = j;
						break;
					}
				}
			}
		}
	}
	if(x1) *x1 = first_col;
	if(x2) *x2 = last_col;
}
void get_image_blank_height(cairo_surface_t *surface, int *y1, int *y2) {
	int w = cairo_image_surface_get_width(surface);
	int h = cairo_image_surface_get_height(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	if(y1) {
		*y1 = 0;
		for(int i = 0; i < h - 1; i++) {
			unsigned char *row = data + i * stride;
			for(int j = 0; j < w; j++) {
				for(int s_i = 0; s_i < 4; s_i++) {
					if(*(row + 4 * j + s_i) != 0) {
						*y1 = i;
						j = w; i = h;
						break;
					}
				}
			}
		}
	}
	if(y2) {
		*y2 = h;
		for(int i = h - 1; i > 0; i--) {
			unsigned char *row = data + i * stride;
			for(int j = 0; j < w; j++) {
				for(int s_i = 0; s_i < 4; s_i++) {
					if(*(row + 4 * j + s_i) != 0) {
						*y2 = i;
						j = w; i = 0;
						break;
					}
				}
			}
		}
	}
}

void set_tooltips_enabled(GtkWidget *w, bool b) {
	if(!w) return;
	if(b) {
		gchar *gstr = gtk_widget_get_tooltip_text(w);
		if(gstr) {
			gtk_widget_set_has_tooltip(w, TRUE);
			g_free(gstr);
		} else {
			gstr = gtk_widget_get_tooltip_markup(w);
			if(gstr) {
				gtk_widget_set_has_tooltip(w, TRUE);
				g_free(gstr);
			} else if(GTK_IS_ENTRY(w)) {
				gstr = gtk_entry_get_icon_tooltip_text(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY);
				if(gstr) {
					gtk_widget_set_has_tooltip(w, TRUE);
					g_free(gstr);
				} else {
					gstr = gtk_entry_get_icon_tooltip_markup(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY);
					if(gstr) {
						gtk_widget_set_has_tooltip(w, TRUE);
						g_free(gstr);
					}
				}
			}
		}
	} else {
		gtk_widget_set_has_tooltip(w, FALSE);
	}
	if(GTK_IS_BIN(w)) {
		set_tooltips_enabled(gtk_bin_get_child(GTK_BIN(w)), b);
	} else if(GTK_IS_CONTAINER(w)) {
		GList *list = gtk_container_get_children(GTK_CONTAINER(w));
		for(GList *l = list; l != NULL; l = l->next) {
			if(GTK_IS_WIDGET(l->data)) set_tooltips_enabled(GTK_WIDGET(l->data), b);
		}
		g_list_free(list);
	}
}

void update_window_properties(GtkWidget *d, bool ignore_tooltips_setting) {
	if(!ignore_tooltips_setting && (!enable_tooltips || toe_changed)) set_tooltips_enabled(d, enable_tooltips);
	if(always_on_top || aot_changed) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
}

GtkBuilder *getBuilder(const char *filename) {
	string resstr = "/qalculate-gtk/ui/";
	resstr += filename;
	return gtk_builder_new_from_resource(resstr.c_str());
}

void show_message(const gchar *text, GtkWidget *win) {
	GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", text);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
	gtk_dialog_run(GTK_DIALOG(edialog));
	gtk_widget_destroy(edialog);
}
bool ask_question(const gchar *text, GtkWidget *win) {
	GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_YES_NO, "%s", text);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
	int question_answer = gtk_dialog_run(GTK_DIALOG(edialog));
	gtk_widget_destroy(edialog);
	return question_answer == GTK_RESPONSE_YES;
}


bool last_is_operator(string str, bool allow_exp) {
	remove_blank_ends(str);
	if(str.empty()) return false;
	if((signed char) str[str.length() - 1] > 0) {
		if(is_in(OPERATORS "\\" LEFT_PARENTHESIS LEFT_VECTOR_WRAP, str[str.length() - 1]) && (str[str.length() - 1] != '!' || str.length() == 1)) return true;
		if(allow_exp && is_in(EXP, str[str.length() - 1])) return true;
		if(str.length() >= 3 && str[str.length() - 1] == 'r' && str[str.length() - 2] == 'o' && str[str.length() - 3] == 'x') return true;
	} else {
		if(str.length() >= 3 && (signed char) str[str.length() - 2] < 0) {
			str = str.substr(str.length() - 3);
			if(str == "∧" || str == "∨" || str == "⊻" || str == "≤" || str == "≥" || str == "≠" || str == "∠" || str == expression_times_sign() || str == expression_divide_sign() || str == expression_add_sign() || str == expression_sub_sign()) {
				return true;
			}
		}
		if(str.length() >= 2) {
			str = str.substr(str.length() - 2);
			if(str == "¬" || str == expression_times_sign() || str == expression_divide_sign() || str == expression_add_sign() || str == expression_sub_sign()) return true;
		}
	}
	return false;
}

void base_from_string(string str, int &base, Number &nbase, bool input_base) {
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") base = BASE_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "roman") || equalsIgnoreCase(str, "roman")) base = BASE_ROMAN_NUMERALS;
	else if(!input_base && (equalsIgnoreCase(str, "time") || equalsIgnoreCase(str, "time"))) base = BASE_TIME;
	else if(str == "b26" || str == "B26") base = BASE_BIJECTIVE_26;
	else if(equalsIgnoreCase(str, "bcd")) base = BASE_BINARY_DECIMAL;
	else if(equalsIgnoreCase(str, "unicode")) base = BASE_UNICODE;
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") base = BASE_SUPER_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "pi") || str == "π") base = BASE_PI;
	else if(str == "e") base = BASE_E;
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") base = BASE_SQRT2;
	else {
		EvaluationOptions eo = evalops;
		eo.parse_options.base = 10;
		MathStructure m;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 350, eo);
		if(CALCULATOR->endTemporaryStopMessages()) {
			base = BASE_CUSTOM;
			nbase.clear();
		} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
			base = m.number().intValue();
		} else {
			base = BASE_CUSTOM;
			nbase = m.number();
		}
	}
}
