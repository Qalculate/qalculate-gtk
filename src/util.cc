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
#include "historyview.h"
#include "mainwindow.h"
#include "menubar.h"
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

size_t unformatted_length(const string &str) {
	size_t l = 0;
	bool intag = false;
	for(size_t i = 0; i < str.length(); i++) {
		if(intag) {
			if(str[i] == '>') intag = false;
		} else if(str[i] == '<') {
			intag = true;
		} else if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			l++;
		}
	}
	return l;
}

string replace_result_separators(string str) {
	if(printops.digit_grouping == DIGIT_GROUPING_LOCALE && !evalops.parse_options.comma_as_separator && CALCULATOR->local_digit_group_separator == COMMA && printops.comma() == ";" && printops.decimalpoint() == ".") {
		gsub(COMMA, "", str);
	} else if(printops.digit_grouping == DIGIT_GROUPING_LOCALE && !evalops.parse_options.dot_as_separator && CALCULATOR->local_digit_group_separator == DOT && printops.decimalpoint() != DOT) {
		gsub(DOT, "", str);
	}
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
		if(i2 >= i2_end) return i1 >= str1.length();
		if(i1 >= str1.length()) break;
		if(i2 >= str2.length()) return false;
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
	GdkModifierType state; guint keyval = 0;
	gdk_event_get_state((GdkEvent*) event, &state);
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
	if(block_input && (keyval == GDK_KEY_q || keyval == GDK_KEY_Q) && !(state & GDK_CONTROL_MASK)) {block_input = false; return "";}
	state = CLEAN_MODIFIERS(state);
	FIX_ALT_GR
	state = (GdkModifierType) (state & ~GDK_SHIFT_MASK);
	if(state == GDK_CONTROL_MASK) {
		switch(keyval) {
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
	switch(keyval) {
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
	if(!w) w = (void*) history_view_widget();
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
	if(!w) w = (void*) history_view_widget();
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
	if(GTK_IS_MENU_ITEM(w)) {
		set_tooltips_enabled(gtk_menu_item_get_submenu(GTK_MENU_ITEM(w)), b);
	} else if(GTK_IS_BIN(w)) {
		set_tooltips_enabled(gtk_bin_get_child(GTK_BIN(w)), b);
	} else if(GTK_IS_CONTAINER(w)) {
		GList *list = gtk_container_get_children(GTK_CONTAINER(w));
		for(GList *l = list; l != NULL; l = l->next) {
			if(GTK_IS_WIDGET(l->data)) set_tooltips_enabled(GTK_WIDGET(l->data), b);
		}
		g_list_free(list);
	}
}

bool hide_tooltips(GtkWidget *w) {
	if(!w) return false;
	if(gtk_widget_get_has_tooltip(w)) {
		gtk_widget_set_has_tooltip(w, FALSE);
		gtk_widget_trigger_tooltip_query(w);
		gtk_widget_set_has_tooltip(w, TRUE);
	}
	if(GTK_IS_BIN(w)) {
		return hide_tooltips(gtk_bin_get_child(GTK_BIN(w)));
	} else if(GTK_IS_CONTAINER(w)) {
		GList *list = gtk_container_get_children(GTK_CONTAINER(w));
		for(GList *l = list; l != NULL; l = l->next) {
			if(GTK_IS_WIDGET(l->data)) {
				if(hide_tooltips(GTK_WIDGET(l->data))) {
					g_list_free(list);
					return true;
				}
			}
		}
		g_list_free(list);
	}
	return false;
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

void show_message(const gchar *text, GtkWindow *win) {
	if(!win) win = main_window();
	GtkWidget *edialog = gtk_message_dialog_new(win, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", text);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
	gtk_dialog_run(GTK_DIALOG(edialog));
	gtk_widget_destroy(edialog);
}
bool ask_question(const gchar *text, GtkWindow *win) {
	if(!win) win = main_window();
	GtkWidget *edialog = gtk_message_dialog_new(win, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", text);
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

bool string_is_less(string str1, string str2) {
	size_t i = 0;
	bool b_uni = false;
	while(i < str1.length() && i < str2.length()) {
		if(str1[i] == str2[i]) i++;
		else if((signed char) str1[i] < 0 || (signed char) str2[i] < 0) {b_uni = true; break;}
		else return str1[i] < str2[i];
	}
	if(b_uni) return g_utf8_collate(str1.c_str(), str2.c_str()) < 0;
	return str1 < str2;
}

bool contains_plot_or_save(const string &str) {
	if(expression_contains_save_function(str, evalops.parse_options, false)) return true;
	for(size_t f_i = 0; f_i < 4; f_i++) {
		int id = 0;
		if(f_i == 0) id = FUNCTION_ID_PLOT;
		else if(f_i == 1) id = FUNCTION_ID_EXPORT;
		else if(f_i == 2) id = FUNCTION_ID_LOAD;
		else if(f_i == 3) id = FUNCTION_ID_COMMAND;
		MathFunction *f = CALCULATOR->getFunctionById(id);
		for(size_t i = 1; f && i <= f->countNames(); i++) {
			if(str.find(f->getName(i).name) != string::npos) {
				MathStructure mtest;
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&mtest, str, evalops.parse_options);
				CALCULATOR->endTemporaryStopMessages();
				if(mtest.containsFunctionId(FUNCTION_ID_PLOT) || mtest.containsFunctionId(FUNCTION_ID_EXPORT) || mtest.containsFunctionId(FUNCTION_ID_LOAD) || mtest.containsFunctionId(FUNCTION_ID_COMMAND)) return true;
				return false;
			}
		}
	}
	return false;
}

long int get_fixed_denominator_gtk2(const string &str, int &to_fraction, char sgn, bool qalc_command) {
	long int fden = 0;
	if(!qalc_command && (equalsIgnoreCase(str, "fraction") || equalsIgnoreCase(str, _("fraction")) || str == "1/n" || str == "/n")) {
		fden = -1;
	} else {
		if(str.length() > 2 && str[0] == '1' && str[1] == '/' && str.find_first_not_of(NUMBERS SPACES, 2) == string::npos) {
			fden = s2i(str.substr(2, str.length() - 2));
		} else if(str.length() > 1 && str[0] == '/' && str.find_first_not_of(NUMBERS SPACES, 1) == string::npos) {
			fden = s2i(str.substr(1, str.length() - 1));
		} else if(str == "3rds") {
			fden = 3;
		} else if(str == "halves") {
			fden = 2;
		} else if(str.length() > 3 && str.find("ths", str.length() - 3) != string::npos && str.find_first_not_of(NUMBERS SPACES) == str.length() - 3) {
			fden = s2i(str.substr(0, str.length() - 3));
		}
	}
	if(fden == 1) fden = 0;
	if(fden != 0) {
		if(sgn == '-' || (fden > 0 && !qalc_command && sgn != '+' && !combined_fixed_fraction_set())) to_fraction = 2;
		else if(fden > 0 && sgn == 0) to_fraction = -1;
		else to_fraction = 1;
	}
	return fden;
}
long int get_fixed_denominator_gtk(const string &str, int &to_fraction, bool qalc_command) {
	size_t n = 0;
	if(str[0] == '-' || str[0] == '+') n = 1;
	if(n > 0) return get_fixed_denominator_gtk2(str.substr(n, str.length() - n), to_fraction, str[0], qalc_command);
	return get_fixed_denominator_gtk2(str, to_fraction, 0, qalc_command);
}

bool contains_polynomial_division(MathStructure &m) {
	if(m.isPower() && m[0].containsType(STRUCT_ADDITION) && m[1].representsNegative()) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_polynomial_division(m[i])) return true;
	}
	return false;
}
bool contains_imaginary_number(MathStructure &m) {
	if(m.isNumber() && m.number().hasImaginaryPart()) return true;
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_CIS) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_imaginary_number(m[i])) return true;
	}
	return false;
}
bool contains_rational_number(MathStructure &m) {
	if(m.isNumber() && ((m.number().realPartIsRational() && !m.number().realPart().isInteger()) || (m.number().hasImaginaryPart() && m.number().imaginaryPart().isRational() && !m.number().imaginaryPart().isInteger()))) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_rational_number(m[i])) {
			return i != 1 || !m.isPower() || !m[1].isNumber() || m[1].number().denominatorIsGreaterThan(9) || (m[1].number().numeratorIsGreaterThan(9) && !m[1].number().denominatorIsTwo() && !m[0].representsNonNegative(true));
		}
	}
	return false;
}
bool contains_fraction(MathStructure &m, bool in_div) {
	if(in_div) {
		if(m.isInteger()) return true;
		if(!m.isMultiplication()) in_div = false;
	}
	if(!in_div) {
		if(m.isInverse()) return contains_fraction(m[0], true);
		if(m.isDivision()) {
			return contains_fraction(m[1], true) || contains_fraction(m[0], false);
		}
		if(m.isPower() && m[1].isNumber() && m[1].number().isMinusOne()) return contains_fraction(m[0], true);
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_fraction(m[i], in_div)) return true;
	}
	return false;
}

bool contains_convertible_unit(MathStructure &m) {
	if(m.type() == STRUCT_UNIT) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(!m.isFunction() || !m.function()->getArgumentDefinition(i + 1) || m.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
			if(contains_convertible_unit(m[i])) return true;
		}
	}
	return false;
}

bool contains_prefix(const MathStructure &m) {
	if(m.isUnit() && (m.prefix() && m.prefix() != CALCULATOR->decimal_null_prefix && m.prefix() != CALCULATOR->binary_null_prefix)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_prefix(m[i])) return true;
	}
	return false;
}

void entry_insert_text(GtkWidget *w, const gchar *text) {
	if(gtk_entry_get_overwrite_mode(GTK_ENTRY(w)) && !gtk_editable_get_selection_bounds(GTK_EDITABLE(w), NULL, NULL)) {
		gint pos = gtk_editable_get_position(GTK_EDITABLE(w));
		gtk_editable_delete_text(GTK_EDITABLE(w), pos, pos + 1);
	} else {
		gtk_editable_delete_selection(GTK_EDITABLE(w));
	}
	gint pos = gtk_editable_get_position(GTK_EDITABLE(w));
	gtk_editable_insert_text(GTK_EDITABLE(w), text, -1, &pos);
	gtk_editable_set_position(GTK_EDITABLE(w), pos);
	gtk_widget_grab_focus(w);
	gtk_editable_select_region(GTK_EDITABLE(w), pos, pos);
}

bool textview_in_quotes(GtkTextView *w) {
	if(!w) return false;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(w);
	if(!buffer) return false;
	GtkTextIter ipos, istart;
	if(gtk_text_buffer_get_has_selection(buffer)) {
		gtk_text_buffer_get_selection_bounds(buffer, &ipos, &istart);
	} else {
		gtk_text_buffer_get_iter_at_mark(buffer, &ipos, gtk_text_buffer_get_insert(buffer));
	}
	gtk_text_buffer_get_start_iter(buffer, &istart);
	gchar *gtext = gtk_text_buffer_get_text(buffer, &istart, &ipos, FALSE);
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
gboolean on_math_entry_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(entry_in_quotes(GTK_ENTRY(o))) return FALSE;
	const gchar *key = key_press_get_symbol(event);
	if(!key) return FALSE;
	if(strlen(key) > 0) entry_insert_text(o, key);
	return TRUE;
}
gboolean on_unit_entry_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(entry_in_quotes(GTK_ENTRY(o))) return FALSE;
	const gchar *key = key_press_get_symbol(event, false, true);
	if(!key) return FALSE;
	if(strlen(key) > 0) entry_insert_text(o, key);
	return TRUE;
}

gchar *font_name_to_css(const char *font_name, const char *w) {
	gchar *gstr = NULL;
	PangoFontDescription *font_desc = pango_font_description_from_string(font_name);
	gint size = pango_font_description_get_size(font_desc) / PANGO_SCALE;
	switch(pango_font_description_get_style(font_desc)) {
		case PANGO_STYLE_NORMAL: {
			gstr = g_strdup_printf("%s {font-family: %s; font-weight: %i; font-size: %ipt;}", w, pango_font_description_get_family(font_desc), pango_font_description_get_weight(font_desc), size);
			break;
		}
		case PANGO_STYLE_OBLIQUE: {
			gstr = g_strdup_printf("%s {font-family: %s; font-weight: %i; font-size: %ipt; font-style: oblique;}", w, pango_font_description_get_family(font_desc), pango_font_description_get_weight(font_desc), size);
			break;
		}
		case PANGO_STYLE_ITALIC: {
			gstr = g_strdup_printf("%s {font-family: %s; font-weight: %i; font-size: %ipt; font-style: italic;}", w, pango_font_description_get_family(font_desc), pango_font_description_get_weight(font_desc), size);
			break;
		}
	}
	pango_font_description_free(font_desc);
	return gstr;
}

#ifdef _WIN32
gboolean on_activate_link(GtkLabel*, gchar *uri, gpointer) {
	ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
	return TRUE;
}
#else
gboolean on_activate_link(GtkLabel*, gchar*, gpointer) {
	return FALSE;
}
#endif

gint compare_categories(gconstpointer a, gconstpointer b) {
	gchar *gstr1c = g_utf8_casefold((const char*) a, -1);
	gchar *gstr2c = g_utf8_casefold((const char*) b, -1);
	gint retval = g_utf8_collate(gstr1c, gstr2c);
	g_free(gstr1c);
	g_free(gstr2c);
	return retval;
}

string shortcut_to_text(guint key, guint state) {
	string str;
#ifdef GDK_WINDOWING_QUARTZ
	if(state & GDK_LOCK_MASK) {str += "Lock";}
	if(state & GDK_CONTROL_MASK) {str += "\xe2\x8c\x83";}
	if(state & GDK_SUPER_MASK) {str += "Super";}
	if(state & GDK_HYPER_MASK) {str += "Hyper";}
	if(state & GDK_META_MASK) {str += "\xe2\x8c\x98";}
	if(state & GDK_MOD1_MASK) {str += "\xe2\x8c\xa5";}
	if(state & GDK_SHIFT_MASK) {str += "\xe2\x87\xa7";}
	if(state & GDK_MOD2_MASK) {str += "Mod2";}
	if(state & GDK_MOD3_MASK) {str += "Mod3";}
	if(state & GDK_MOD4_MASK) {str += "Mod4";}
	if(state & GDK_MOD5_MASK) {str += "Mod5";}
#else
	if(state & GDK_LOCK_MASK) {if(!str.empty()) str += "+"; str += "Lock";}
	if(state & GDK_CONTROL_MASK) {if(!str.empty()) str += "+"; str += "Ctrl";}
	if(state & GDK_SUPER_MASK) {if(!str.empty()) str += "+"; str += "Super";}
	if(state & GDK_HYPER_MASK) {if(!str.empty()) str += "+"; str += "Hyper";}
	if(state & GDK_META_MASK) {if(!str.empty()) str += "+"; str += "Meta";}
	if(state & GDK_MOD1_MASK) {if(!str.empty()) str += "+"; str += "Alt";}
	if(state & GDK_SHIFT_MASK) {if(!str.empty()) str += "+"; str += "Shift";}
	if(state & GDK_MOD2_MASK) {if(!str.empty()) str += "+"; str += "Mod2";}
	if(state & GDK_MOD3_MASK) {if(!str.empty()) str += "+"; str += "Mod3";}
	if(state & GDK_MOD4_MASK) {if(!str.empty()) str += "+"; str += "Mod4";}
	if(state & GDK_MOD5_MASK) {if(!str.empty()) str += "+"; str += "Mod5";}
	if(!str.empty()) str += "+";
#endif
	gunichar uni = gdk_keyval_to_unicode(key);
	if(uni == 0 || !g_unichar_isprint(uni) || g_unichar_isspace(uni)) {
		str += gdk_keyval_name(key);
	} else {
		uni = g_unichar_toupper(uni);
		char s[7];
		s[g_unichar_to_utf8(uni, s)] = '\0';
		str += s;
	}
	return str;
}
const gchar *shortcut_type_text(int type, bool return_null) {
	switch(type) {
		case SHORTCUT_TYPE_FUNCTION: {return _("Insert function"); break;}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {return _("Insert function (dialog)"); break;}
		case SHORTCUT_TYPE_VARIABLE: {return _("Insert variable"); break;}
		case SHORTCUT_TYPE_UNIT: {return _("Insert unit"); break;}
		case SHORTCUT_TYPE_TEXT: {return _("Insert text"); break;}
		case SHORTCUT_TYPE_DATE: {return _("Insert date"); break;}
		case SHORTCUT_TYPE_VECTOR: {return _("Insert vector"); break;}
		case SHORTCUT_TYPE_MATRIX: {return _("Insert matrix"); break;}
		case SHORTCUT_TYPE_SMART_PARENTHESES: {return _("Insert smart parentheses"); break;}
		case SHORTCUT_TYPE_CONVERT: {return _("Convert to unit"); break;}
		case SHORTCUT_TYPE_CONVERT_ENTRY: {return _("Convert to unit (entry)"); break;}
		case SHORTCUT_TYPE_OPTIMAL_UNIT: {return _("Convert to optimal unit"); break;}
		case SHORTCUT_TYPE_BASE_UNITS: {return _("Convert to base units"); break;}
		case SHORTCUT_TYPE_OPTIMAL_PREFIX: {return _("Convert to optimal prefix"); break;}
		case SHORTCUT_TYPE_TO_NUMBER_BASE: {return _("Convert to number base"); break;}
		case SHORTCUT_TYPE_FACTORIZE: {return _("Factorize result"); break;}
		case SHORTCUT_TYPE_EXPAND: {return _("Expand result"); break;}
		case SHORTCUT_TYPE_PARTIAL_FRACTIONS: {return _("Expand partial fractions"); break;}
		case SHORTCUT_TYPE_SET_UNKNOWNS: {return _("Set unknowns"); break;}
		case SHORTCUT_TYPE_RPN_DOWN: {return _("RPN: down"); break;}
		case SHORTCUT_TYPE_RPN_UP: {return _("RPN: up"); break;}
		case SHORTCUT_TYPE_RPN_SWAP: {return _("RPN: swap"); break;}
		case SHORTCUT_TYPE_RPN_COPY: {return _("RPN: copy"); break;}
		case SHORTCUT_TYPE_RPN_LASTX: {return _("RPN: lastx"); break;}
		case SHORTCUT_TYPE_RPN_DELETE: {return _("RPN: delete register"); break;}
		case SHORTCUT_TYPE_RPN_CLEAR: {return _("RPN: clear stack"); break;}
		case SHORTCUT_TYPE_META_MODE: {return _("Load meta mode"); break;}
		case SHORTCUT_TYPE_INPUT_BASE: {return _("Set expression base"); break;}
		case SHORTCUT_TYPE_OUTPUT_BASE: {return _("Set result base"); break;}
		case SHORTCUT_TYPE_EXACT_MODE: {return _("Toggle exact mode"); break;}
		case SHORTCUT_TYPE_DEGREES: {return _("Set angle unit to degrees"); break;}
		case SHORTCUT_TYPE_RADIANS: {return _("Set angle unit to radians"); break;}
		case SHORTCUT_TYPE_GRADIANS: {return _("Set angle unit to gradians"); break;}
		case SHORTCUT_TYPE_FRACTIONS: {return _("Toggle simple fractions"); break;}
		case SHORTCUT_TYPE_MIXED_FRACTIONS: {return _("Toggle mixed fractions"); break;}
		case SHORTCUT_TYPE_SCIENTIFIC_NOTATION: {return _("Toggle scientific notation"); break;}
		case SHORTCUT_TYPE_SIMPLE_NOTATION: {return _("Toggle simple notation"); break;}
		case SHORTCUT_TYPE_PRECISION: {return _("Toggle precision");}
		case SHORTCUT_TYPE_MAX_DECIMALS: {return _("Toggle max decimals");}
		case SHORTCUT_TYPE_MIN_DECIMALS: {return _("Toggle min decimals");}
		case SHORTCUT_TYPE_MINMAX_DECIMALS: {return _("Toggle max/min decimals");}
		case SHORTCUT_TYPE_RPN_MODE: {return _("Toggle RPN mode"); break;}
		case SHORTCUT_TYPE_AUTOCALC: {return _("Toggle calculate as you type"); break;}
		case SHORTCUT_TYPE_PROGRAMMING: {return _("Toggle programming keypad"); break;}
		case SHORTCUT_TYPE_KEYPAD: {return _("Show keypad"); break;}
		case SHORTCUT_TYPE_HISTORY: {return _("Show history"); break;}
		case SHORTCUT_TYPE_HISTORY_SEARCH: {return _("Search history"); break;}
		case SHORTCUT_TYPE_HISTORY_CLEAR: {return _("Clear history"); break;}
		case SHORTCUT_TYPE_CONVERSION: {return _("Show conversion"); break;}
		case SHORTCUT_TYPE_STACK: {return _("Show RPN stack"); break;}
		case SHORTCUT_TYPE_MINIMAL: {return _("Toggle minimal window"); break;}
		case SHORTCUT_TYPE_MANAGE_VARIABLES: {return _("Manage variables"); break;}
		case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {return _("Manage functions"); break;}
		case SHORTCUT_TYPE_MANAGE_UNITS: {return _("Manage units"); break;}
		case SHORTCUT_TYPE_MANAGE_DATA_SETS: {return _("Manage data sets"); break;}
		case SHORTCUT_TYPE_STORE: {return _("Store result"); break;}
		case SHORTCUT_TYPE_MEMORY_CLEAR: {return _("MC (memory clear)"); break;}
		case SHORTCUT_TYPE_MEMORY_RECALL: {return _("MR (memory recall)"); break;}
		case SHORTCUT_TYPE_MEMORY_STORE: {return _("MS (memory store)"); break;}
		case SHORTCUT_TYPE_MEMORY_ADD: {return _("M+ (memory plus)"); break;}
		case SHORTCUT_TYPE_MEMORY_SUBTRACT: {return _("M− (memory minus)"); break;}
		case SHORTCUT_TYPE_NEW_VARIABLE: {return _("New variable"); break;}
		case SHORTCUT_TYPE_NEW_FUNCTION: {return _("New function"); break;}
		case SHORTCUT_TYPE_PLOT: {return _("Open plot functions/data"); break;}
		case SHORTCUT_TYPE_NUMBER_BASES: {return _("Open convert number bases"); break;}
		case SHORTCUT_TYPE_FLOATING_POINT: {return _("Open floating point conversion"); break;}
		case SHORTCUT_TYPE_CALENDARS: {return _("Open calendar conversion"); break;}
		case SHORTCUT_TYPE_PERCENTAGE_TOOL: {return _("Open percentage calculation tool"); break;}
		case SHORTCUT_TYPE_PERIODIC_TABLE: {return _("Open periodic table"); break;}
		case SHORTCUT_TYPE_UPDATE_EXRATES: {return _("Update exchange rates"); break;}
		case SHORTCUT_TYPE_COPY_RESULT: {return _("Copy result"); break;}
		case SHORTCUT_TYPE_INSERT_RESULT: {return _("Insert result"); break;}
		case SHORTCUT_TYPE_SAVE_IMAGE: {return _("Save result image"); break;}
		case SHORTCUT_TYPE_HELP: {return _("Help"); break;}
		case SHORTCUT_TYPE_QUIT: {return _("Quit"); break;}
		case SHORTCUT_TYPE_CHAIN_MODE: {return _("Toggle chain mode"); break;}
		case SHORTCUT_TYPE_ALWAYS_ON_TOP: {return _("Toggle keep above"); break;}
		case SHORTCUT_TYPE_DO_COMPLETION: {return _("Show/hide completion"); break;}
		case SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION: {return _("Perform completion (activate first item)"); break;}
	}
	if(return_null) return NULL;
	return "-";
}
string button_valuetype_text(int type, const string &value) {
	switch(type) {
		case SHORTCUT_TYPE_FUNCTION: {
			MathFunction *f = CALCULATOR->getActiveFunction(value);
			if(!f) break;
			return f->title(true, printops.use_unicode_signs);
		}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
			MathFunction *f = CALCULATOR->getActiveFunction(value);
			if(!f) break;
			return f->title(true, printops.use_unicode_signs);
		}
		case SHORTCUT_TYPE_VARIABLE: {
			Variable *v = CALCULATOR->getActiveVariable(value);
			if(!v) break;
			return v->title(true, printops.use_unicode_signs);
		}
		case SHORTCUT_TYPE_UNIT: {
			Unit *u = CALCULATOR->getActiveUnit(value);
			if(!u) {
				CALCULATOR->beginTemporaryStopMessages();
				CompositeUnit cu("", "", "", value);
				CALCULATOR->endTemporaryStopMessages();
				if(cu.countUnits() == 0) break;
				for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
					if(CALCULATOR->units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						CompositeUnit *cu2 = (CompositeUnit*) CALCULATOR->units[i];
						if(cu2->countUnits() == cu.countUnits()) {
							bool b = true;
								for(size_t i2 = 1; i2 <= cu.countUnits(); i2++) {
								int exp1 = 1, exp2 = 1;
								Prefix *p1 = NULL, *p2 = NULL;
								if(cu.get(i2, &exp1, &p1) != cu2->get(i2, &exp2, &p2) || exp1 != exp2 || p1 != p2) {
									b = false;
									break;
								}
							}
							if(b) return cu2->title(true, printops.use_unicode_signs);
						}
					}
				}
				return cu.print(false, true, printops.use_unicode_signs);
			}
			return u->title(true, printops.use_unicode_signs);
		}
		default: {}
	}
	if(value.empty() || type == SHORTCUT_TYPE_COPY_RESULT) return shortcut_type_text(type);
	string str = shortcut_type_text(type);
	str += " ("; str += value; str += ")";
	return str;
}
string shortcut_types_text(const vector<int> &type) {
	if(type.size() == 1) return shortcut_type_text(type[0]);
	string str;
	for(size_t i = 0; i < type.size(); i++) {
		if(!str.empty()) str += ", ";
		str += shortcut_type_text(type[i]);
	}
	return str;
}
const char *shortcut_copy_value_text(int v) {
	switch(v) {
		case 1: {return _("Formatted result");}
		case 2: {return _("Unformatted ASCII result");}
		case 3: {return _("Unformatted ASCII result without units");}
		case 4: {return _("Formatted expression");}
		case 5: {return _("Unformatted ASCII expression");}
		case 6: {return _("Formatted expression + result");}
		case 7: {return _("Unformatted ASCII expression + result");}
	}
	return _("Default");
}
string shortcut_values_text(const vector<string> &value, const vector<int> &type) {
	if(value.size() == 1 && type[0] != SHORTCUT_TYPE_COPY_RESULT) return value[0];
	string str;
	for(size_t i = 0; i < value.size(); i++) {
		if(!str.empty() && !value[i].empty()) str += ", ";
		if(type[i] == SHORTCUT_TYPE_COPY_RESULT) str += shortcut_copy_value_text(s2i(value[i]));
		else str += value[i];
	}
	return str;
}

string get_value_string(const MathStructure &mstruct_, int type, Prefix *prefix) {
	PrintOptions po = printops;
	po.is_approximate = NULL;
	po.allow_non_usable = false;
	po.prefix = prefix;
	po.base = 10;
	if(type > 0) {
		po.preserve_precision = true;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		if(type == 1) po.show_ending_zeroes = false;
		else po.restrict_to_parent_precision = false;
		if(po.number_fraction_format == FRACTION_DECIMAL) po.number_fraction_format = FRACTION_DECIMAL_EXACT;
	}
	string str = CALCULATOR->print(mstruct_, 100, po);
	return str;
}

Unit *find_exact_matching_unit2(const MathStructure &m) {
	switch(m.type()) {
		case STRUCT_POWER: {
			if(m.base()->isUnit() && (!m.base()->prefix() || m.base()->prefix()->value().isOne()) && m.base()->unit()->subtype() != SUBTYPE_COMPOSITE_UNIT && m.exponent()->isNumber() && m.exponent()->number().isInteger() && m.exponent()->number() < 10 && m.exponent()->number() > -10) {
				Unit *u_base = m.base()->unit();
				int exp = m.exponent()->number().intValue();
				for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
					if(CALCULATOR->units[i]->subtype() == SUBTYPE_ALIAS_UNIT) {
						AliasUnit *u = (AliasUnit*) CALCULATOR->units[i];
						if(u->firstBaseUnit() == u_base && u->firstBaseExponent() == exp) return u;
					}
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			if(m.prefix() && !m.prefix()->value().isOne()) {
				for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
					if(CALCULATOR->units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						CompositeUnit *u = (CompositeUnit*) CALCULATOR->units[i];
						int exp = 0;
						Prefix *p = NULL;
						if(u->countUnits() == 1 && u->get(1, &exp, &p) == m.unit() && exp == 1 && p == m.prefix()) return u;
					}
				}
			}
			return m.unit();
		}
		case STRUCT_MULTIPLICATION: {
			if(m.size() == 2 && !m[0].containsType(STRUCT_UNIT, false)) {
				return find_exact_matching_unit2(m[1]);
			}
			CompositeUnit *cu = new CompositeUnit("", "temporary_find_matching_unit");
			for(size_t i = 1; i <= m.countChildren(); i++) {
				if(m.getChild(i)->isUnit()) {
					cu->add(m.getChild(i)->unit(), 1, m.getChild(i)->prefix() && !m.getChild(i)->prefix()->value().isOne() ? m.getChild(i)->prefix() : NULL);
				} else if(m.getChild(i)->isPower() && m.getChild(i)->base()->isUnit() && m.getChild(i)->exponent()->isInteger() && m.getChild(i)->exponent()->number() < 10 && m.getChild(i)->exponent()->number() > -10) {
					cu->add(m.getChild(i)->base()->unit(), m.getChild(i)->exponent()->number().intValue(), m.getChild(i)->base()->prefix() && !m.getChild(i)->base()->prefix()->value().isOne() ? m.getChild(i)->base()->prefix() : NULL);
				} else if(m.getChild(i)->containsType(STRUCT_UNIT, false)) {
					delete cu;
					return NULL;
				}
			}
			if(cu->countUnits() == 1) {
				int exp = 1;
				Prefix *p = NULL;
				Unit *u = cu->get(1, &exp, &p);
				MathStructure m2(u, p);
				if(exp != 1) m2.raise(exp);
				return find_exact_matching_unit2(m2);
			}
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				Unit *u = CALCULATOR->units[i];
				if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					if(((CompositeUnit*) u)->countUnits() == cu->countUnits()) {
						bool b = true;
						for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
							int exp1 = 1, exp2 = 1;
							Prefix *p1 = NULL, *p2 = NULL;
							Unit *ui1 = cu->get(i2, &exp1, &p1);
							b = false;
							for(size_t i3 = 1; i3 <= cu->countUnits(); i3++) {
								Unit *ui2 = ((CompositeUnit*) u)->get(i3, &exp2, &p2);
								if(ui1 == ui2) {
									b = (exp1 == exp2 && p1 == p2);
									break;
								}
							}
							if(!b) break;
						}
						if(b) {
							delete cu;
							return u;
						}
					}
				}
			}
			delete cu;
			break;
		}
		default: {}
	}
	return NULL;
}

void find_match_unformat(MathStructure &m) {
	for(size_t i = 0; i < m.size(); i++) {
		find_match_unformat(m[i]);
	}
	switch(m.type()) {
		case STRUCT_INVERSE: {
			m.setToChild(1, true);
			if(m.isPower() && m[1].isNumber()) m[1].number().negate();
			else m.raise(nr_minus_one);
			break;
		}
		case STRUCT_NEGATE: {
			m.setToChild(1);
			if(m.type() != STRUCT_MULTIPLICATION) m.transform(STRUCT_MULTIPLICATION);
			m.insertChild(m_minus_one, 1);
			break;
		}
		case STRUCT_DIVISION: {
			m.setType(STRUCT_MULTIPLICATION);
			if(m[1].isPower() && m[1][1].isNumber()) m[1][1].number().negate();
			else m[1].raise(nr_minus_one);
			find_match_unformat(m);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < m.size();) {
				if(m[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < m[i].size(); i2++) {
						m[i][i2].ref();
						m.insertChild_nocopy(&m[i][i2], i + i2 + 2);
					}
					m.delChild(i + 1);
				} else {
					i++;
				}
			}
			break;
		}
		default: {}
	}
}

Unit *find_exact_matching_unit(const MathStructure &m) {
	MathStructure m2(m);
	find_match_unformat(m2);
	return find_exact_matching_unit2(m2);
}
void remove_non_units(MathStructure &m) {
	if(m.isPower() && m[0].isUnit()) return;
	if(m.size() > 0) {
		for(size_t i = m.size(); i > 0; i--) {
			if(m[i - 1].isFunction() || m[i - 1].containsType(STRUCT_UNIT, true) <= 0) {
				m.delChild(i);
			} else {
				remove_non_units(m[i - 1]);
			}
		}
		if(m.size() == 0) m.clear();
		else if(m.size() == 1) m.setToChild(1);
	}
}
void find_matching_units(const MathStructure &m, const MathStructure *mparse, vector<Unit*> &v, bool top) {
	Unit *u = CALCULATOR->findMatchingUnit(m);
	if(u) {
		for(size_t i = 0; i < v.size(); i++) {
			if(v[i] == u) return;
		}
		v.push_back(u);
		if(top && mparse) {
			Unit *u2 = CALCULATOR->findMatchingUnit(*mparse);
			if(u2 && u2 != u && u2->baseUnit() == u->baseUnit() && u2->baseExponent() == u->baseExponent() && u2->category() != u->category()) v.push_back(u2);
		}
		return;
	}
	if(top && mparse && !m.containsType(STRUCT_UNIT, true) && (mparse->containsFunctionId(FUNCTION_ID_ASIN) || mparse->containsFunctionId(FUNCTION_ID_ACOS) || mparse->containsFunctionId(FUNCTION_ID_ATAN))) {
		v.push_back(CALCULATOR->getRadUnit());
		return;
	}
	if(top && m.containsType(STRUCT_UNIT, true) > 0 && m.size() < 100) {
		MathStructure m2(m);
		remove_non_units(m2);
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->startControl(50);
		m2 = CALCULATOR->convertToOptimalUnit(m2);
		CALCULATOR->stopControl();
		CALCULATOR->endTemporaryStopMessages();
		find_matching_units(m2, mparse, v, false);
	} else {
		for(size_t i = 0; i < m.size() && i < 100; i++) {
			if(!m.isFunction() || !m.function()->getArgumentDefinition(i + 1) || m.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
				find_matching_units(m[i], mparse, v, false);
			}
		}
	}
}

/*
	tree text sort function
*/
gint string_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
	gint cid = GPOINTER_TO_INT(user_data);
	gchar *gstr1, *gstr2;
	gint retval;
	gtk_tree_model_get(model, a, cid, &gstr1, -1);
	gtk_tree_model_get(model, b, cid, &gstr2, -1);
	gchar *gstr1c = g_utf8_casefold(gstr1, -1);
	gchar *gstr2c = g_utf8_casefold(gstr2, -1);
#ifdef _WIN32
	for(size_t i = 2; i < strlen(gstr1c); i++) {
		if((unsigned char) gstr1c[i - 2] == 0xE2 && (unsigned char) gstr1c[i - 1] == 0x88) {
			gstr1c[i - 2] = ' '; gstr1c[i - 1] = ' '; gstr1c[i] = ' ';
		}
	}
	for(size_t i = 2; i < strlen(gstr2c); i++) {
		if((unsigned char) gstr2c[i - 2] == 0xE2 && (unsigned char) gstr2c[i - 1] == 0x88) {
			gstr2c[i - 2] = ' '; gstr2c[i - 1] = ' '; gstr2c[i] = ' ';
		}
	}
#endif
	retval = g_utf8_collate(gstr1c, gstr2c);
	g_free(gstr1c);
	g_free(gstr2c);
	g_free(gstr1);
	g_free(gstr2);
	return retval;
}

gint category_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
	gint cid = GPOINTER_TO_INT(user_data);
	gchar *gstr1, *gstr2;
	gint retval;
	gtk_tree_model_get(model, a, cid, &gstr1, -1);
	gtk_tree_model_get(model, b, cid, &gstr2, -1);
	if(g_strcmp0(gstr1, _("User variables")) == 0) retval = -1;
	else if(g_strcmp0(gstr2, _("User variables")) == 0) retval = 1;
	else if(g_strcmp0(gstr1, _("User units")) == 0) retval = -1;
	else if(g_strcmp0(gstr2, _("User units")) == 0) retval = 1;
	else if(g_strcmp0(gstr1, _("User functions")) == 0) retval = -1;
	else if(g_strcmp0(gstr2, _("User functions")) == 0) retval = 1;
	else if(g_strcmp0(gstr1, _("Inactive")) == 0) retval = -1;
	else if(g_strcmp0(gstr2, _("Inactive")) == 0) retval = 1;
	else {
		gchar *gstr1c = g_utf8_casefold(gstr1, -1);
		gchar *gstr2c = g_utf8_casefold(gstr2, -1);
		retval = g_utf8_collate(gstr1c, gstr2c);
		g_free(gstr1c);
		g_free(gstr2c);
	}
	g_free(gstr1);
	g_free(gstr2);
	return retval;
}

/*
	tree sort function for number strings
*/
gint int_string_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
	gint cid = GPOINTER_TO_INT(user_data);
	gchar *gstr1, *gstr2;
	bool b1 = false, b2 = false;
	gint retval;
	gtk_tree_model_get(model, a, cid, &gstr1, -1);
	gtk_tree_model_get(model, b, cid, &gstr2, -1);
	if(gstr1[0] == '-') {
		b1 = true;
	}
	if(gstr2[0] == '-') {
		b2 = true;
	}
	if(b1 == b2) {
		gchar *gstr1c = g_utf8_casefold(gstr1, -1);
		gchar *gstr2c = g_utf8_casefold(gstr2, -1);
		retval = g_utf8_collate(gstr1c, gstr2c);
		g_free(gstr1c);
		g_free(gstr2c);
	} else if(b1) {
		retval = -1;
	} else {
		retval = 1;
	}
	g_free(gstr1);
	g_free(gstr2);
	return retval;
}

void fix_deactivate_label_width(GtkWidget *w) {
	gint w1, w2;
	string str = _("Deac_tivate");
	size_t i = str.find("_"); if(i != string::npos) str.erase(i, 1);
	PangoLayout *layout_test = gtk_widget_create_pango_layout(w, str.c_str());
	pango_layout_get_pixel_size(layout_test, &w1, NULL);
	str = _("Ac_tivate");
	i = str.find("_"); if(i != string::npos) str.erase(i, 1);
	pango_layout_set_text(layout_test, str.c_str(), -1);
	pango_layout_get_pixel_size(layout_test, &w2, NULL);
	g_object_unref(layout_test);
	g_object_set(w, "width-request", w2 > w1 ? w2 : w1, NULL);
}

void combo_set_bits(GtkComboBox *w, unsigned int bits, bool has_auto) {
	int i = 0;
	switch(bits) {
		case 0: {i = 0; break;}
		case 8: {i = 1; break;}
		case 16: {i = 2; break;}
		case 32: {i = 3; break;}
		case 64: {i = 4; break;}
		case 128: {i = 5; break;}
		case 256: {i = 6; break;}
		case 512: {i = 7; break;}
		case 1024: {i = 8; break;}
	}
	if(!has_auto) {
		if(i == 0) i = 2;
		else i--;
	}
	gtk_combo_box_set_active(w, i);
}
unsigned int combo_get_bits(GtkComboBox *w, bool has_auto) {
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
	unsigned int bits = 0;
	if(!has_auto) {
		if(i >= 0) i++;
		bits = 32;
	}
	switch(i) {
		case 1: {bits = 8; break;}
		case 2: {bits = 16; break;}
		case 3: {bits = 32; break;}
		case 4: {bits = 64; break;}
		case 5: {bits = 128; break;}
		case 6: {bits = 256; break;}
		case 7: {bits = 512; break;}
		case 8: {bits = 1024; break;}
	}
	return bits;
}

extern MathFunction *f_title;
bool test_autocalculatable(const MathStructure &m, bool where, bool top) {
	if(m.isFunction()) {
		if(m.size() < (size_t) m.function()->minargs() && (!where || m.size() != 0) && (m.size() != 1 || m[0].representsScalar())) {
			MathStructure mfunc(m);
			mfunc.calculateFunctions(evalops, false);
			return false;
		}
		if(m.function()->id() == FUNCTION_ID_SAVE || m.function()->id() == FUNCTION_ID_PLOT || m.function()->id() == FUNCTION_ID_EXPORT || m.function()->id() == FUNCTION_ID_LOAD || m.function()->id() == FUNCTION_ID_COMMAND || m.function() == f_title || (m.function()->subtype() == SUBTYPE_USER_FUNCTION && ((UserFunction*) m.function())->formula().find("plot(") != string::npos)) return false;
		if(m.size() > 0 && (m.function()->id() == FUNCTION_ID_FACTORIAL || m.function()->id() == FUNCTION_ID_DOUBLE_FACTORIAL || m.function()->id() == FUNCTION_ID_MULTI_FACTORIAL) && m[0].isInteger() && m[0].number().integerLength() > 17) {
			return false;
		}
		if(m.function()->id() == FUNCTION_ID_LOGN && m.size() == 2 && m[0].isUndefined() && m[1].isNumber()) return false;
		if(top && m.function()->subtype() == SUBTYPE_DATA_SET && m.size() >= 2 && m[1].isSymbolic() && equalsIgnoreCase(m[1].symbol(), "info")) return false;
	} else if(m.isSymbolic() && contains_plot_or_save(m.symbol())) {
		return false;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(!test_autocalculatable(m[i], where || (m.isFunction() && m.function()->id() == FUNCTION_ID_REPLACE && i > 0), false)) return false;
	}
	return true;
}
