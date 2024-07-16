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
#include <gtk/gtk.h>
#include <cairo/cairo-gobject.h>

#include "support.h"
#include "callbacks.h"
#include "interface.h"
#include "main.h"
#include "keypad.h"
#include "settings.h"
#include "util.h"
#include "precisiondialog.h"
#include "decimalsdialog.h"
#include "setbasedialog.h"

#include <deque>

#include "unordered_map_define.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::iterator;
using std::list;
using std::deque;

extern GtkBuilder *main_builder;
extern GtkBuilder *preferences_builder;

GtkWidget *mainwindow;

GtkWidget *tUnitSelectorCategories;
GtkWidget *tUnitSelector;
GtkListStore *tUnitSelector_store;
GtkTreeModel *tUnitSelector_store_filter;
GtkTreeStore *tUnitSelectorCategories_store;

GtkWidget *tabs, *expander_keypad, *expander_history, *expander_stack, *expander_convert;
GtkEntryCompletion *completion;
GtkWidget *completion_view, *completion_window, *completion_scrolled;
GtkTreeModel *completion_filter, *completion_sort;
GtkListStore *completion_store;

GtkCellRenderer *history_renderer, *history_index_renderer, *ans_renderer, *register_renderer, *register_index_renderer;
GtkTreeViewColumn *register_column, *history_column, *history_index_column, *flag_column;

GtkWidget *expressiontext;
GtkTextBuffer *expressionbuffer;
GtkTextTag *expression_par_tag;
GtkWidget *resultview;
GtkWidget *historyview;
GtkListStore *historystore;
GtkWidget *stackview;
GtkListStore *stackstore;
GtkWidget *statuslabel_l, *statuslabel_r, *keypad;
GtkWidget *f_menu ,*v_menu, *u_menu, *u_menu2, *recent_menu;
GtkAccelGroup *accel_group;

gint history_scroll_width = 16;

GtkCssProvider *topframe_provider, *expression_provider, *resultview_provider, *statuslabel_l_provider, *statuslabel_r_provider, *keypad_provider, *box_rpnl_provider, *app_provider, *app_provider_theme, *statusframe_provider, *color_provider, *history_provider;

extern bool show_keypad, show_history, show_stack, show_convert, continuous_conversion, set_missing_prefixes, persistent_keypad, minimal_mode;
extern bool save_mode_on_exit, save_defs_on_exit, load_global_defs, hyp_is_on, inv_is_on, fetch_exchange_rates_at_startup, clear_history_on_exit, save_history_separately;
extern int max_history_lines;
extern bool fraction_fixed_combined;
extern int allow_multiple_instances;
extern int title_type;
extern int history_expression_type;
extern bool display_expression_status, parsed_in_result;
extern int expression_lines;
extern int gtk_theme;
extern string custom_lang;
extern bool use_custom_result_font, use_custom_expression_font, use_custom_status_font, use_custom_keypad_font, use_custom_app_font, use_custom_history_font;
extern string custom_result_font, custom_expression_font, custom_status_font, custom_keypad_font, custom_app_font, custom_history_font;
extern string status_error_color, status_warning_color, text_color;
extern bool status_error_color_set, status_warning_color_set, text_color_set;
extern int auto_update_exchange_rates;
extern bool copy_ascii, copy_ascii_without_units;
extern bool ignore_locale;
extern bool caret_as_xor;
extern int close_with_esc;
extern int visible_keypad;
extern bool auto_calculate, chain_mode;
extern int simplified_percentage;
extern bool complex_angle_form;
extern bool check_version;
extern int max_plot_time;
extern gint autocalc_history_delay;
extern int default_fraction_fraction;
extern bool use_systray_icon, hide_on_startup;
extern int horizontal_button_padding, vertical_button_padding;
extern string custom_angle_unit;
extern int previous_precision;
extern int enable_tooltips;
extern bool toe_changed;

extern std::vector<custom_button> custom_buttons;
extern std::vector<mode_struct> modes;

extern PrintOptions printops;
extern EvaluationOptions evalops;

extern bool rpn_mode, rpn_keys, adaptive_interval_display;

extern vector<GtkWidget*> mode_items;
extern vector<GtkWidget*> popup_result_mode_items;

extern deque<string> expression_undo_buffer;

gint win_height, win_width, win_x, win_y, win_monitor, history_height;
bool win_monitor_primary;
extern bool remember_position, always_on_top, aot_changed;
extern gint minimal_width;

extern Unit *latest_button_unit, *latest_button_currency;
extern string latest_button_unit_pre, latest_button_currency_pre;

extern int completion_min, completion_min2;
extern bool enable_completion, enable_completion2;
extern int completion_delay;

gchar history_error_color[8];
gchar history_warning_color[8];
gchar history_parse_color[8];
gchar history_bookmark_color[8];

extern unordered_map<string, cairo_surface_t*> flag_surfaces;
int flagheight;

extern string fix_history_string(const string &str);

gint compare_categories(gconstpointer a, gconstpointer b) {
	gchar *gstr1c = g_utf8_casefold((const char*) a, -1);
	gchar *gstr2c = g_utf8_casefold((const char*) b, -1);
	gint retval = g_utf8_collate(gstr1c, gstr2c);
	g_free(gstr1c);
	g_free(gstr2c);
	return retval;
}

bool border_tested = false;
gint hidden_x = -1, hidden_y = -1, hidden_monitor = 1;
bool hidden_monitor_primary = false;

#ifdef _WIN32
#	include <gdk/gdkwin32.h>
#	define WIN_TRAY_ICON_ID 1000
#	define WIN_TRAY_ICON_MESSAGE WM_APP + WIN_TRAY_ICON_ID
static NOTIFYICONDATA nid;
static HWND hwnd = NULL;

INT_PTR CALLBACK tray_window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WIN_TRAY_ICON_MESSAGE && (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)) {
		if(hidden_x >= 0) {
			gtk_widget_show(mainwindow);
			GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(mainwindow));
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
			GdkMonitor *monitor = NULL;
			if(hidden_monitor_primary) monitor = gdk_display_get_primary_monitor(display);
			if(!monitor && hidden_monitor > 0) gdk_display_get_monitor(display, hidden_monitor - 1);
			if(monitor) {
				GdkRectangle area;
				gdk_monitor_get_workarea(monitor, &area);
#else
			GdkScreen *screen = gdk_display_get_default_screen(display);
			int i = -1;
			if(hidden_monitor_primary) i = gdk_screen_get_primary_monitor(screen);
			if(i < 0 && hidden_monitor > 0 && hidden_monitor < gdk_screen_get_n_monitors(screen)) i = hidden_monitor;
			if(i >= 0) {
				GdkRectangle area;
				gdk_screen_get_monitor_workarea(screen, i, &area);
#endif
				gint w = 0, h = 0;
				gtk_window_get_size(GTK_WINDOW(mainwindow), &w, &h);
				if(hidden_x + w > area.width) hidden_x = area.width - w;
				if(hidden_y + h > area.height) hidden_y = area.height - h;
				gtk_window_move(GTK_WINDOW(mainwindow), hidden_x + area.x, hidden_y + area.y);
			} else {
				gtk_window_move(GTK_WINDOW(mainwindow), hidden_x, hidden_y);
			}
			hidden_x = -1;
		}
		gtk_window_present_with_time(GTK_WINDOW(mainwindow), GDK_CURRENT_TIME);
		if(expressiontext) gtk_widget_grab_focus(expressiontext);
		gtk_window_present_with_time(GTK_WINDOW(mainwindow), GDK_CURRENT_TIME);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
void destroy_systray_icon() {
	if(hwnd == NULL) return;
	Shell_NotifyIcon(NIM_DELETE, &nid);
	DestroyWindow(hwnd);
	hwnd = NULL;
}
void create_systray_icon() {

	if(hwnd != NULL) return;

	WNDCLASSEX wcex;
	TCHAR wname[32];
	strcpy(wname, "QalculateTrayWin");
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = 0;
	wcex.lpfnWndProc = (WNDPROC) tray_window_proc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = NULL;
	wcex.hCursor = NULL,
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = wname;
	wcex.hIconSm = NULL;

	if(RegisterClassEx(&wcex)) {
		hwnd = CreateWindow(wname, "", 0, 0, 0, 0, 0, (HWND) gdk_win32_window_get_handle(gtk_widget_get_window(mainwindow)), NULL, GetModuleHandle(NULL), 0);
	}
	if(hwnd != NULL) {
		UpdateWindow(hwnd);
		memset(&nid, 0, sizeof(nid));
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwnd;
		nid.uID = WIN_TRAY_ICON_ID;
		nid.uFlags = NIF_ICON | NIF_MESSAGE;
		nid.uCallbackMessage = WIN_TRAY_ICON_MESSAGE;
		strcpy(nid.szTip, "Qalculate!");
		nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(100));
		Shell_NotifyIcon(NIM_ADD, &nid);
	}
}
bool has_systray_icon() {
	return hwnd != NULL;
}
#else
bool has_systray_icon() {
	return false;
}
#endif

void test_border() {
#ifndef _WIN32
	if(border_tested) return;
	GdkWindow *window = gtk_widget_get_window(mainwindow);
	GdkRectangle rect;
	gdk_window_get_frame_extents(window, &rect);
	gint window_border = (rect.width - gtk_widget_get_allocated_width(mainwindow)) / 2;
	if(window_border > 0) {
		gchar *gstr = gtk_css_provider_to_string(topframe_provider);
		string topframe_css = gstr;
		g_free(gstr);
		gsub("border-left-width: 0;", "", topframe_css);
		gsub("border-right-width: 0;", "", topframe_css);
		gtk_css_provider_load_from_data(topframe_provider, topframe_css.c_str(), -1, NULL);
		border_tested = true;
	} else if(rect.x != 0 || rect.y != 0) {
		border_tested = true;
	}
#endif
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

GdkRGBA c_gray;

void update_colors(bool initial) {

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	GdkRGBA bg_color;
	gtk_style_context_get_background_color(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, &bg_color);
	if(!initial && RUNTIME_CHECK_GTK_VERSION_LESS(3, 16)) {
		gchar *gstr = gtk_css_provider_to_string(topframe_provider);
		string topframe_css = gstr;
		g_free(gstr);
		size_t i1 = topframe_css.find("background-color:");
		if(i1 != string::npos) {
			i1 += 18;
			size_t i2 = topframe_css.find(";", i1);
			if(i2 != string::npos) {
				gchar *gstr = gdk_rgba_to_string(&bg_color);
				topframe_css.replace(i1, i2 - i1 - 1, gstr);
				g_free(gstr);
				gtk_css_provider_load_from_data(topframe_provider, topframe_css.c_str(), -1, NULL);
			}
		}
	}
#endif

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14

	gint scalefactor = gtk_widget_get_scale_factor(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")));
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 16 * scalefactor, 16 * scalefactor);
	cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
	cairo_t *cr = cairo_create(surface);
	GdkRGBA rgba;
	gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals"))), GTK_STATE_FLAG_NORMAL, &rgba);

	PangoLayout *layout_equals = gtk_widget_create_pango_layout(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), "=");
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
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

	if(initial || !text_color_set) {

		GdkRGBA c;

		gtk_style_context_get_color(gtk_widget_get_style_context(historyview), GTK_STATE_FLAG_NORMAL, &c);
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

		c_gray = c;
		if(c_gray.blue + c_gray.green + c_gray.red > 1.5) {
			c_gray.green /= 1.5;
			c_gray.red /= 1.5;
			c_gray.blue /= 1.5;
		} else if(c_gray.blue + c_gray.green + c_gray.red > 0.3) {
			c_gray.green += 0.235;
			c_gray.red += 0.235;
			c_gray.blue += 0.235;
		} else if(c_gray.blue + c_gray.green + c_gray.red > 0.15) {
			c_gray.green += 0.3;
			c_gray.red += 0.3;
			c_gray.blue += 0.3;
		} else {
			c_gray.green += 0.4;
			c_gray.red += 0.4;
			c_gray.blue += 0.4;
		}
		g_snprintf(history_parse_color, 8, "#%02x%02x%02x", (int) (c_gray.red * 255), (int) (c_gray.green * 255), (int) (c_gray.blue * 255));
		if(!initial) g_object_set(G_OBJECT(history_index_renderer), "ypad", 0, "yalign", 0.0, "xalign", 0.5, "foreground-rgba", &c_gray, NULL);

		gtk_style_context_get_color(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, &c);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
		if(gdk_rgba_equal(&c, &bg_color)) {
			gtk_style_context_get_color(gtk_widget_get_style_context(statuslabel_l), GTK_STATE_FLAG_NORMAL, &c);
		}
#endif
		GdkRGBA c_par = c;
		if(c_par.green >= 0.8) {
			c_par.red /= 1.5;
			c_par.blue /= 1.5;
			c_par.green = 1.0;
		} else {
			if(c_par.green >= 0.5) c_par.green = 1.0;
			else c_par.green += 0.5;
		}
		if(initial) {
			PangoLayout *layout_par = gtk_widget_create_pango_layout(expressiontext, "()");
			gint w1 = 0, w2 = 0;
			pango_layout_get_pixel_size(layout_par, &w1, NULL);
			pango_layout_set_markup(layout_par, "<b>()</b>", -1);
			pango_layout_get_pixel_size(layout_par, &w2, NULL);
			g_object_unref(layout_par);
			if(w1 == w2) expression_par_tag = gtk_text_buffer_create_tag(expressionbuffer, "curpar", "foreground-rgba", &c_par, "weight", PANGO_WEIGHT_BOLD, NULL);
			else expression_par_tag = gtk_text_buffer_create_tag(expressionbuffer, "curpar", "foreground-rgba", &c_par, NULL);
		} else {
			g_object_set(G_OBJECT(expression_par_tag), "foreground-rgba", &c_par, NULL);
		}

		gchar tcs[8];
		g_snprintf(tcs, 8, "#%02x%02x%02x", (int) (c.red * 255), (int) (c.green * 255), (int) (c.blue * 255));
		if(initial && text_color == tcs) text_color_set = false;
		if(!text_color_set) {
			text_color = tcs;
			if(initial) color_provider = NULL;
		} else if(initial) {
			color_provider = gtk_css_provider_new();
			string css_str = "* {color: "; css_str += text_color; css_str += "}";
			gtk_css_provider_load_from_data(color_provider, css_str.c_str(), -1, NULL);
			gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(color_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}

		gtk_style_context_get_color(gtk_widget_get_style_context(statuslabel_l), GTK_STATE_FLAG_NORMAL, &c);

		if(!status_error_color_set) {
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
			status_error_color = ecs;
		}

		if(!status_warning_color_set) {
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
			status_warning_color = wcs;
		}

	}

}

void set_assumptions_items(AssumptionType at, AssumptionSign as) {
	switch(as) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_unknown")), TRUE);}
	}
	switch(at) {
		case ASSUMPTION_TYPE_BOOLEAN: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_boolean")), TRUE); break;}
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_none")), TRUE);}
	}
}

extern unordered_map<Unit*, GtkWidget*> angle_unit_items;

void set_mode_items(const PrintOptions &po, const EvaluationOptions &eo, AssumptionType at, AssumptionSign as, bool in_rpn_mode, int precision, bool interval, bool variable_units, bool id_adaptive, int keypad, bool autocalc, bool chainmode, bool caf, bool simper, bool initial_update) {

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_autocalc")), autocalc && (!initial_update || !in_rpn_mode));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), chainmode && (!initial_update || !in_rpn_mode));
	auto_calculate = autocalc;
	chain_mode = chainmode;
	if(initial_update) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_autocalc")), !in_rpn_mode);
	if(initial_update) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), !in_rpn_mode);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), in_rpn_mode);
	switch(eo.approximation) {
		case APPROXIMATION_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), TRUE);
			if(initial_update) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), TRUE);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), TRUE);
			}
			break;
		}
		case APPROXIMATION_TRY_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), TRUE);
			if(initial_update) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), FALSE);
			}
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_approximate")), TRUE);
			if(initial_update) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), FALSE);
			}
			break;
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_arithmetic")), interval);

	switch(eo.interval_calculation) {
		case INTERVAL_CALCULATION_VARIANCE_FORMULA: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_variance")), TRUE);
			break;
		}
		case INTERVAL_CALCULATION_INTERVAL_ARITHMETIC: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_interval_arithmetic")), TRUE);
			break;
		}
		case INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_simple")), TRUE);
			break;
		}
		case INTERVAL_CALCULATION_NONE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_none")), TRUE);
			break;
		}
	}

	switch(eo.auto_post_conversion) {
		case POST_CONVERSION_OPTIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_optimal")), TRUE);
			break;
		}
		case POST_CONVERSION_OPTIMAL_SI: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_optimal_si")), TRUE);
			break;
		}
		case POST_CONVERSION_BASE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_base")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_none")), TRUE);
			break;
		}
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_mixed_units_conversion")), eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE);

	switch(eo.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_degrees")), TRUE);
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_radians")), TRUE);
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_gradians")), TRUE);
			break;
		}
		case ANGLE_UNIT_CUSTOM: {
			Unit *u = initial_update ? NULL : CALCULATOR->getActiveUnit(custom_angle_unit);
			if(!u) {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_default_angle_unit")), TRUE);
			} else {
				unordered_map<Unit*, GtkWidget*>::iterator it = angle_unit_items.find(u);
				if(it != angle_unit_items.end()) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(it->second), TRUE);
				else if(u->hasName("rad")) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_radians")), TRUE);
				else if(u->hasName("gra")) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_gradians")), TRUE);
				else if(u->hasName("deg")) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_degrees")), TRUE);
			}
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_default_angle_unit")), TRUE);
			break;
		}
	}
	if(initial_update) update_mb_angles(eo.parse_options.angle_unit);

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), eo.parse_options.read_precision != DONT_READ_PRECISION);
	if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_read_precision")), eo.parse_options.read_precision != DONT_READ_PRECISION);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication")), eo.parse_options.limit_implicit_multiplication);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_simplified_percentage")), simper);
	switch(eo.parse_options.parsing_mode) {
		case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace")), TRUE);
			break;
		}
		case PARSING_MODE_CONVENTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication")), TRUE);
			break;
		}
		case PARSING_MODE_CHAIN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), TRUE);
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax")), TRUE);
			break;
		}
		case PARSING_MODE_RPN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing")), TRUE);
			break;
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assume_nonzero_denominators")), eo.assume_denominators_nonzero);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_warn_about_denominators_assumed_nonzero")), eo.warn_about_denominators_assumed_nonzero);

	switch(eo.structuring) {
		case STRUCTURING_FACTORIZE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_factorize")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_simplify")), TRUE);
			break;
		}
	}

	switch(po.base) {
		case BASE_BINARY: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 0);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_binary")), TRUE);
			break;
		}
		case BASE_OCTAL: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 1);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_octal")), TRUE);
			break;
		}
		case BASE_DECIMAL: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 2);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_decimal")), TRUE);
			break;
		}
		case 12: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 3);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_duodecimal")), TRUE);
			break;
		}
		case BASE_HEXADECIMAL: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 4);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 7);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_roman")), TRUE);
			break;
		}
		case BASE_SEXAGESIMAL: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 5);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sexagesimal")), TRUE);
			break;
		}
		case BASE_TIME: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 6);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_time_format")), TRUE);
			break;
		}
		default: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 8);
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")), TRUE);
			printops.base = po.base;
			output_base_updated_from_menu();
		}
	}

	switch(po.min_exp) {
		case EXP_PRECISION: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_normal")), TRUE);
			break;
		}
		case EXP_BASE_3: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 1);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_engineering")), TRUE);
			break;
		}
		case EXP_SCIENTIFIC: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 2);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_scientific")), TRUE);
			break;
		}
		case EXP_PURE: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 3);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific")), TRUE);
			break;
		}
		case EXP_NONE: {
			if(initial_update) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 4);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_non_scientific")), TRUE);
			break;
		}
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_indicate_infinite_series")), po.indicate_infinite_series);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_show_ending_zeroes")), po.show_ending_zeroes);
	switch(po.rounding) {
		case ROUNDING_HALF_AWAY_FROM_ZERO: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_away_from_zero")), TRUE);
			break;
		}
		case ROUNDING_HALF_TO_EVEN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_to_even")), TRUE);
			break;
		}
		case ROUNDING_HALF_TO_ODD: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_to_odd")), TRUE);
			break;
		}
		case ROUNDING_HALF_TOWARD_ZERO: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_toward_zero")), TRUE);
			break;
		}
		case ROUNDING_HALF_RANDOM: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_random")), TRUE);
			break;
		}
		case ROUNDING_HALF_UP: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_up")), TRUE);
			break;
		}
		case ROUNDING_HALF_DOWN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_down")), TRUE);
			break;
		}
		case ROUNDING_TOWARD_ZERO: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_toward_zero")), TRUE);
			break;
		}
		case ROUNDING_AWAY_FROM_ZERO: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_away_from_zero")), TRUE);
			break;
		}
		case ROUNDING_UP: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_up")), TRUE);
			break;
		}
		case ROUNDING_DOWN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_down")), TRUE);
			break;
		}
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), po.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), po.sort_options.minus_last);

	if(!po.use_unit_prefixes) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_no_prefixes")), TRUE);
	} else if(po.use_prefixes_for_all_units) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_all_units")), TRUE);
	} else if(po.use_prefixes_for_currencies) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_currencies")), TRUE);
	} else {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_selected_units")), TRUE);
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_all_prefixes")), po.use_all_prefixes);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_denominator_prefixes")), po.use_denominator_prefix);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_place_units_separately")), po.place_units_separately);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_abbreviate_names")), po.abbreviate_names);

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_variables")), eo.parse_options.variables_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_functions")), eo.parse_options.functions_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_units")), eo.parse_options.units_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_unknown_variables")), eo.parse_options.unknowns_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_variable_units")), variable_units);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_calculate_variables")), eo.calculate_variables);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_allow_complex")), eo.allow_complex);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_allow_infinite")), eo.allow_infinite);
	
	int dff = default_fraction_fraction;
	if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")), fraction_fixed_combined);
	switch(po.number_fraction_format) {
		case FRACTION_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), FALSE);
			break;
		}
		case FRACTION_DECIMAL_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), FALSE);
			break;
		}
		case FRACTION_COMBINED: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), TRUE);
			break;
		}
		case FRACTION_FRACTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), TRUE);
			break;
		}
		case FRACTION_COMBINED_FIXED_DENOMINATOR: {}
		case FRACTION_FRACTIONAL_FIXED_DENOMINATOR: {
			switch(CALCULATOR->fixedDenominator()) {
				case 2: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_halves")), TRUE); break;}
				case 3: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_3rds")), TRUE); break;}
				case 4: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_4ths")), TRUE); break;}
				case 5: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_5ths")), TRUE); break;}
				case 6: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_6ths")), TRUE); break;}
				case 8: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_8ths")), TRUE); break;}
				case 10: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_10ths")), TRUE); break;}
				case 12: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_12ths")), TRUE); break;}
				case 16: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_16ths")), TRUE); break;}
				case 32: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_32ths")), TRUE); break;}
				default: {
					if(po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
						printops.number_fraction_format = FRACTION_COMBINED_FIXED_DENOMINATOR;
					} else {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
						printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
					}
					break;
				}
			}
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")), po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), TRUE);
			break;
		}
		case FRACTION_PERCENT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_percent")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), FALSE);
			break;
		}
		case FRACTION_PERMILLE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permille")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), FALSE);
			break;
		}
		case FRACTION_PERMYRIAD: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permyriad")), TRUE);
			if(initial_update) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_fraction")), FALSE);
			break;
		}
	}
	default_fraction_fraction = dff;

	switch(eo.complex_number_form) {
		case COMPLEX_NUMBER_FORM_RECTANGULAR: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_rectangular")), TRUE);
			break;
		}
		case COMPLEX_NUMBER_FORM_EXPONENTIAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_exponential")), TRUE);
			break;
		}
		case COMPLEX_NUMBER_FORM_POLAR: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
			break;
		}
		case COMPLEX_NUMBER_FORM_CIS: {
			if(caf) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_angle")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
			break;
		}
	}

	if(id_adaptive) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_adaptive")), TRUE);
	} else {
		switch(po.interval_display) {
			case INTERVAL_DISPLAY_INTERVAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_interval")), TRUE); break;}
			case INTERVAL_DISPLAY_PLUSMINUS: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_plusminus")), TRUE); break;}
			case INTERVAL_DISPLAY_MIDPOINT: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_midpoint")), TRUE); break;}
			case INTERVAL_DISPLAY_SIGNIFICANT_DIGITS: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_significant")), TRUE); break;}
			default: {}
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_concise_uncertainty_input")), CALCULATOR->conciseUncertaintyInputEnabled());

	set_assumptions_items(at, as);

	if(!initial_update) {
		printops.max_decimals = po.max_decimals;
		printops.use_max_decimals = po.use_max_decimals;
		printops.max_decimals = po.min_decimals;
		printops.use_min_decimals = po.use_min_decimals;
		decimals_updated();
		CALCULATOR->setPrecision(precision);
		previous_precision = 0;
		precision_updated();
		printops.spacious = po.spacious;
		printops.short_multiplication = po.short_multiplication;
		printops.excessive_parenthesis = po.excessive_parenthesis;
		evalops.calculate_functions = eo.calculate_functions;
		evalops.parse_options.base = eo.parse_options.base;
		bases_updated();
	}
	update_keypad_programming_base();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_programmers_keypad")), keypad & PROGRAMMING_KEYPAD);
	visible_keypad = keypad;
	
	if(!initial_update) {
		if(keypad & HIDE_LEFT_KEYPAD) {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_left_buttons")));
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_right_buttons")));
			gint h;
			gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), NULL, &h);
			gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), 1, 1);
		} else if(keypad & HIDE_RIGHT_KEYPAD) {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_right_buttons")));
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_left_buttons")));
			gint h;
			gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), NULL, &h);
			gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), 1, 1);
		}
	}
}

gboolean completion_row_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer) {
	gint i;
	gtk_tree_model_get(model, iter, 4, &i, -1);
	return i == 3;
}
gboolean history_row_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer) {
	gint hindex = -1;
	gtk_tree_model_get(model, iter, 1, &hindex, -1);
	return hindex < 0;
}

GtkBuilder *getBuilder(const char *filename) {
	string resstr = "/qalculate-gtk/ui/";
	resstr += filename;
	return gtk_builder_new_from_resource(resstr.c_str());
}

void create_main_window(void) {

	main_builder = getBuilder("main.ui");
	g_assert(main_builder != NULL);

	/* make sure we get a valid main window */
	g_assert(gtk_builder_get_object(main_builder, "main_window") != NULL);

	mainwindow = GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window"));

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), accel_group);

	if(win_width > 0) gtk_window_set_default_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_width, win_height > 0 ? win_height : -1);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result")), false);
#endif

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
	gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_swap")), "object-flip-vertical-symbolic", GTK_ICON_SIZE_BUTTON);
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

	char **flags_r = g_resources_enumerate_children("/qalculate-gtk/flags", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	if(flags_r) {
		PangoFontDescription *font_desc;
		gtk_style_context_get(gtk_widget_get_style_context(mainwindow), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
		PangoFontset *fontset = pango_context_load_fontset(gtk_widget_get_pango_context(mainwindow), font_desc, pango_context_get_language(gtk_widget_get_pango_context(mainwindow)));
		PangoFontMetrics *metrics = pango_fontset_get_metrics(fontset);
		flagheight = (pango_font_metrics_get_ascent(metrics) + pango_font_metrics_get_descent(metrics)) / PANGO_SCALE;
		pango_font_metrics_unref(metrics);
		g_object_unref(fontset);
		pango_font_description_free(font_desc);
		gint scalefactor = gtk_widget_get_scale_factor(mainwindow);
		for(size_t i = 0; flags_r[i] != NULL; i++) {
			string flag_s = flags_r[i];
			size_t i_ext = flag_s.find(".", 1);
			if(i_ext != string::npos) {
				GdkPixbuf *flagbuf = gdk_pixbuf_new_from_resource_at_scale((string("/qalculate-gtk/flags/") + flag_s).c_str(), -1, flagheight * scalefactor, TRUE, NULL);
				if(flagbuf) {
					cairo_surface_t *s = gdk_cairo_surface_create_from_pixbuf(flagbuf, scalefactor, NULL);
					flag_surfaces[flag_s.substr(0, i_ext)] = s;
					g_object_unref(flagbuf);
				}
			}
		}
		g_strfreev(flags_r);
	}

	expressiontext = GTK_WIDGET(gtk_builder_get_object(main_builder, "expressiontext"));
	expressionbuffer = GTK_TEXT_BUFFER(gtk_builder_get_object(main_builder, "expressionbuffer"));
	resultview = GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"));
	historyview = GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview"));

	expression_undo_buffer.push_back("");

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(expressiontext), 12);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(expressiontext), 6);
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(expressiontext), 6);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(expressiontext), 6);
#else
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(expressiontext), 12);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(expressiontext), 6);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(expressiontext), GTK_TEXT_WINDOW_TOP, 6);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(expressiontext), GTK_TEXT_WINDOW_BOTTOM, 6);
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION > 22 || (GTK_MINOR_VERSION == 22 && GTK_MICRO_VERSION >= 20)
	gtk_text_view_set_input_hints(GTK_TEXT_VIEW(expressiontext), GTK_INPUT_HINT_NO_EMOJI);
#endif

	stackview = GTK_WIDGET(gtk_builder_get_object(main_builder, "stackview"));
	statuslabel_l = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_status_left"));
	statuslabel_r = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_status_right"));
	keypad = GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"));
	tabs = GTK_WIDGET(gtk_builder_get_object(main_builder, "tabs"));

	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 3);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(statuslabel_r, 12);
	gtk_widget_set_margin_start(statuslabel_l, 9);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")), 6);
#else
	gtk_widget_set_margin_right(statuslabel_r, 12);
	gtk_widget_set_margin_left(statuslabel_l, 9);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")), 6);
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_label_set_xalign(GTK_LABEL(statuslabel_l), 0.0);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_l), 0.5);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_r), 0.5);
#else
	gtk_misc_set_alignment(GTK_MISC(statuslabel_l), 0.0, 0.5);
#endif

	expression_provider = gtk_css_provider_new();
	resultview_provider = gtk_css_provider_new();
	statuslabel_l_provider = gtk_css_provider_new();
	statuslabel_r_provider = gtk_css_provider_new();
	keypad_provider = gtk_css_provider_new();
	history_provider = gtk_css_provider_new();
	box_rpnl_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(expressiontext), GTK_STYLE_PROVIDER(expression_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(resultview), GTK_STYLE_PROVIDER(resultview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_l), GTK_STYLE_PROVIDER(statuslabel_l_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_r), GTK_STYLE_PROVIDER(statuslabel_r_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(keypad), GTK_STYLE_PROVIDER(keypad_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(historyview), GTK_STYLE_PROVIDER(history_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl"))), GTK_STYLE_PROVIDER(box_rpnl_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	topframe_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "topframe"))), GTK_STYLE_PROVIDER(topframe_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	string topframe_css = "* {background-color: ";

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 16)) {
		GdkRGBA bg_color;
		gtk_style_context_get_background_color(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, &bg_color);
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
	gtk_style_context_add_provider(gtk_widget_get_style_context(expressiontext), GTK_STYLE_PROVIDER(expressionborder_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	string border_css = topframe_css; border_css += "}";
	gsub("*", "textview.view > border", border_css);
	gtk_css_provider_load_from_data(expressionborder_provider, border_css.c_str(), -1, NULL);
#endif
	GtkCssProvider *expression_provider2 = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(expressiontext), GTK_STYLE_PROVIDER(expression_provider2), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	string expression_css = topframe_css; expression_css += "}";
	gsub("*", "textview.view > text", expression_css);
	gtk_css_provider_load_from_data(expression_provider2, expression_css.c_str(), -1, NULL);
	topframe_css += "; border-left-width: 0; border-right-width: 0; border-radius: 0;}";
	gtk_css_provider_load_from_data(topframe_provider, topframe_css.c_str(), -1, NULL);
	statusframe_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusframe"))), GTK_STYLE_PROVIDER(statusframe_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_css_provider_load_from_data(statusframe_provider, topframe_css.c_str(), -1, NULL);

	if(RUNTIME_CHECK_GTK_VERSION(3, 16)) {
		GtkCssProvider *result_color_provider = gtk_css_provider_new();
		gtk_css_provider_load_from_data(result_color_provider, "* {color: @theme_text_color;}", -1, NULL);
		gtk_style_context_add_provider(gtk_widget_get_style_context(resultview), GTK_STYLE_PROVIDER(result_color_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 1);
	}

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	if(gtk_theme < 0) {
		app_provider_theme = NULL;
	} else {
		app_provider_theme = gtk_css_provider_new();
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(app_provider_theme), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		switch(gtk_theme) {
			case 0: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/Adwaita/gtk-contained.css"); break;}
			case 1: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/Adwaita/gtk-contained-dark.css"); break;}
			case 2: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/HighContrast/gtk-contained.css"); break;}
			case 3: {gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/HighContrast/gtk-contained-inverse.css"); break;}
		}
	}
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_label_unit")), 12);
	gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
#else
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_label_unit")), 12);
	gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
#endif
	gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 9);
	gtk_widget_set_margin_bottom(tabs, 3);
	gtk_widget_set_margin_bottom(keypad, 3);

	if(visible_keypad & PROGRAMMING_KEYPAD) {
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_left_buttons")), GTK_WIDGET(gtk_builder_get_object(main_builder, "programmers_keypad")));
		gtk_stack_set_visible_child_name(GTK_STACK(gtk_builder_get_object(main_builder, "stack_keypad_top")), "page1");
	}
	
	if(visible_keypad & HIDE_LEFT_KEYPAD) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "stack_left_buttons")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_right_buttons")));
	} else if(visible_keypad & HIDE_RIGHT_KEYPAD) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_right_buttons")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "event_hide_left_buttons")));
	}

	set_mode_items(printops, evalops, CALCULATOR->defaultAssumptions()->type(), CALCULATOR->defaultAssumptions()->sign(), rpn_mode, CALCULATOR->getPrecision(), CALCULATOR->usesIntervalArithmetic(), CALCULATOR->variableUnitsEnabled(), adaptive_interval_display, visible_keypad, auto_calculate, chain_mode, complex_angle_form, simplified_percentage, true);

	if(use_custom_app_font) {
		app_provider = gtk_css_provider_new();
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(app_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gchar *gstr = font_name_to_css(custom_app_font.c_str());
		gtk_css_provider_load_from_data(app_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		app_provider = NULL;
		if(custom_app_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(mainwindow), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_app_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(use_custom_result_font) {
		gchar *gstr = font_name_to_css(custom_result_font.c_str());
		gtk_css_provider_load_from_data(resultview_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(resultview_provider, "* {font-size: larger;}", -1, NULL);
		if(custom_result_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(resultview), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_result_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(use_custom_expression_font) {
		gchar *gstr;
		if(RUNTIME_CHECK_GTK_VERSION(3, 20)) gstr = font_name_to_css(custom_expression_font.c_str(), "textview.view");
		else gstr = font_name_to_css(custom_expression_font.c_str());
		gtk_css_provider_load_from_data(expression_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		PangoFontDescription *font_desc;
		gtk_style_context_get(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
		pango_font_description_set_size(font_desc, pango_font_description_get_size(font_desc) * 1.2);
		char *gstr_l = pango_font_description_to_string(font_desc);
		if(custom_expression_font.empty()) custom_expression_font = gstr_l;
		pango_font_description_free(font_desc);
		gchar *gstr;
		if(RUNTIME_CHECK_GTK_VERSION(3, 20)) gstr = font_name_to_css(gstr_l, "textview.view");
		else gstr = font_name_to_css(gstr_l);
		gtk_css_provider_load_from_data(expression_provider, gstr, -1, NULL);
		g_free(gstr);
		g_free(gstr_l);
	}
	if(use_custom_status_font) {
		gchar *gstr = font_name_to_css(custom_status_font.c_str());
		gtk_css_provider_load_from_data(statuslabel_l_provider, gstr, -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(statuslabel_l_provider, "* {font-size: 90%;}", -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, "* {font-size: 90%;}", -1, NULL);
		if(custom_status_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(statuslabel_l), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_status_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(use_custom_keypad_font) {
		gchar *gstr = font_name_to_css(custom_keypad_font.c_str());
		gtk_css_provider_load_from_data(keypad_provider, gstr, -1, NULL);
		gtk_css_provider_load_from_data(box_rpnl_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		if(custom_keypad_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(keypad), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_keypad_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(use_custom_history_font) {
		gchar *gstr = font_name_to_css(custom_history_font.c_str());
		gtk_css_provider_load_from_data(history_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		if(custom_history_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(historyview), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_history_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}

	update_status_text();

	update_colors(true);

	set_unicode_buttons();

	set_operator_symbols();

	gtk_widget_grab_focus(expressiontext);
	gtk_widget_set_can_default(expressiontext, TRUE);
	gtk_widget_grab_default(expressiontext);

	expander_keypad = GTK_WIDGET(gtk_builder_get_object(main_builder, "expander_keypad"));
	expander_history = GTK_WIDGET(gtk_builder_get_object(main_builder, "expander_history"));
	expander_stack = GTK_WIDGET(gtk_builder_get_object(main_builder, "expander_stack"));
	expander_convert = GTK_WIDGET(gtk_builder_get_object(main_builder, "expander_convert"));

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_hi")), !persistent_keypad);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl")), !persistent_keypad || (show_stack && rpn_mode));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnr")), !persistent_keypad || (show_stack && rpn_mode));
	if(history_height > 0) gtk_widget_set_size_request(tabs, -1, history_height);
	if(show_stack && rpn_mode) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), TRUE);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 1);
		gtk_widget_show(tabs);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
	} else if(show_keypad && !persistent_keypad) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), TRUE);
		gtk_widget_hide(tabs);
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
	} else if(show_history) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_history), TRUE);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 0);
		gtk_widget_show(tabs);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
	} else if(show_convert) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 2);
		gtk_widget_show(tabs);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
	} else {
		gtk_widget_hide(tabs);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		gtk_widget_set_vexpand(resultview, TRUE);
	}
	if(persistent_keypad) {
		if(show_keypad) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), TRUE);
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
			gtk_widget_set_vexpand(resultview, FALSE);
		}
		gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_keypad_lock")), "changes-prevent-symbolic", GTK_ICON_SIZE_BUTTON);
		if(show_convert) gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), 6);
	}
	GtkRequisition req;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_keypad")), &req, NULL);
	if(req.height < 20) gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_keypad_lock")), req.height * 0.8);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_persistent_keypad")), persistent_keypad);
	gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), !persistent_keypad || !gtk_widget_get_visible(tabs));

	if(minimal_mode) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		set_status_bottom_border_visible(false);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), TRUE);
		gtk_widget_set_vexpand(resultview, FALSE);
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")), minimal_mode);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")), continuous_conversion);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes")), set_missing_prefixes);

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
	CHILDREN_SET_FOCUS_ON_CLICK("historyactions")
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_history_copy"));
	CHILDREN_SET_FOCUS_ON_CLICK("box_ho")
	CHILDREN_SET_FOCUS_ON_CLICK("box_rm")
	CHILDREN_SET_FOCUS_ON_CLICK("box_re")
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_clearstack"));
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_editregister"));
	CHILDREN_SET_FOCUS_ON_CLICK("box_ro1")
	CHILDREN_SET_FOCUS_ON_CLICK("box_ro2")
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_rpn_sum"));
	list = gtk_container_get_children(GTK_CONTAINER(gtk_builder_get_object(main_builder, "versatile_keypad")));
	for(l = list; l != NULL; l = l->next) {
		list2 = gtk_container_get_children(GTK_CONTAINER(l->data));
		for(l2 = list2; l2 != NULL; l2 = l2->next) {
			SET_FOCUS_ON_CLICK(l2->data);
		}
		g_list_free(list2);
	}
	g_list_free(list);

	gchar *theme_name = NULL;
	g_object_get(gtk_settings_get_default(), "gtk-theme-name", &theme_name, NULL);
	string themestr;
	if(theme_name) themestr = theme_name;

	GtkCssProvider *notification_style = gtk_css_provider_new(); gtk_css_provider_load_from_data(notification_style, "* {border-radius: 5px}", -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "overlaybox"))), GTK_STYLE_PROVIDER(notification_style), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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

		if(themestr == "Breeze" || themestr == "Breeze-Dark") {

			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro1"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro2"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rm"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_re"))), "linked");
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

			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_negate"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}
	}

	create_keypad();

	if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "document-edit-symbolic")) {
		gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_edit")), "gtk-edit", GTK_ICON_SIZE_BUTTON);
#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION <= 18
		if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 18) && (themestr == "Ambiance" || themestr == "Radiance")) {
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_re"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rm"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro1"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro2"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions"))), "linked");
			gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ho"))), "linked");
		}
#endif
	}

	gtk_builder_connect_signals(main_builder, NULL);

	historystore = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(historyview), GTK_TREE_MODEL(historystore));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(historyview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	history_index_renderer = gtk_cell_renderer_text_new();
	history_index_column = gtk_tree_view_column_new_with_attributes(_("Index"), history_index_renderer, "text", 2, "ypad", 4, NULL);
	gtk_tree_view_column_set_expand(history_index_column, FALSE);
	gtk_tree_view_column_set_min_width(history_index_column, 30);
	g_object_set(G_OBJECT(history_index_renderer), "ypad", 0, "yalign", 0.0, "xalign", 0.5, "foreground-rgba", &c_gray, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(historyview), history_index_column);
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
	gtk_tree_view_append_column(GTK_TREE_VIEW(historyview), history_column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_historyview_selection_changed), NULL);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(historyview), history_row_separator_func, NULL, NULL);

	if(!copy_ascii) gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_text")))), GDK_KEY_c, GDK_CONTROL_MASK);
	else gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_ascii")))), GDK_KEY_c, GDK_CONTROL_MASK);
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_history_delete")))), GDK_KEY_Delete, GDK_SHIFT_MASK);

	completion_view = GTK_WIDGET(gtk_builder_get_object(main_builder, "completionview"));
	gtk_style_context_add_provider(gtk_widget_get_style_context(completion_view), GTK_STYLE_PROVIDER(expression_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	// Fix for breeze-gtk and Ubuntu theme
	if(RUNTIME_CHECK_GTK_VERSION(3, 20) && themestr.substr(0, 7) != "Adwaita" && themestr.substr(0, 5) != "oomox" && themestr.substr(0, 6) != "themix" && themestr != "Yaru") {
		GtkCssProvider *historyview_provider = gtk_css_provider_new();
		gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(historyview), GTK_TREE_VIEW_GRID_LINES_NONE);
		gtk_style_context_add_provider(gtk_widget_get_style_context(historyview), GTK_STYLE_PROVIDER(historyview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		if(themestr == "Breeze") {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		} else if(themestr == "Breeze-Dark") {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		} else {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		}
	}

	if(theme_name) g_free(theme_name);

	stackstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(stackview), GTK_TREE_MODEL(stackstore));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	register_index_renderer = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT(register_index_renderer), "xalign", 0.5, NULL);
	if(use_custom_history_font) g_object_set(G_OBJECT(register_index_renderer), "font", custom_history_font.c_str(), NULL);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Index"), register_index_renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(stackview), column);
	register_renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(register_renderer), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, "xalign", 1.0, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
	if(use_custom_history_font) g_object_set(G_OBJECT(register_renderer), "font", custom_history_font.c_str(), NULL);
	g_signal_connect((gpointer) register_renderer, "edited", G_CALLBACK(on_stackview_item_edited), NULL);
	g_signal_connect((gpointer) register_renderer, "editing-started", G_CALLBACK(on_stackview_item_editing_started), NULL);
	g_signal_connect((gpointer) register_renderer, "editing-canceled", G_CALLBACK(on_stackview_item_editing_canceled), NULL);
	register_column = gtk_tree_view_column_new_with_attributes(_("Value"), register_renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(stackview), register_column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_stackview_selection_changed), NULL);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(stackview), TRUE);
	g_signal_connect((gpointer) stackstore, "row-deleted", G_CALLBACK(on_stackstore_row_deleted), NULL);
	g_signal_connect((gpointer) stackstore, "row-inserted", G_CALLBACK(on_stackstore_row_inserted), NULL);

	if(rpn_mode) {
		gtk_label_set_angle(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), 90.0);
		// RPN Enter (calculate and add to stack)
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), _("ENTER"));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), _("Calculate expression and add to stack"));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Calculate expression and add to stack"));
	} else {
		gtk_widget_hide(expander_stack);
	}

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);

/*	Completion	*/
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

	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkCellArea *area = gtk_cell_area_box_new();
	gtk_cell_area_box_set_spacing(GTK_CELL_AREA_BOX(area), 12);
	column = gtk_tree_view_column_new_with_area(area);
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

	for(size_t i = 0; i < modes.size(); i++) {
		GtkWidget *item = gtk_menu_item_new_with_label(modes[i].name.c_str());
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) modes[i].name.c_str());
		g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) modes[i].name.c_str());
		g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) modes[i].name.c_str());
		gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_meta_modes")), item, (gint) i);
		mode_items.push_back(item);
		item = gtk_menu_item_new_with_label(modes[i].name.c_str());
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) modes[i].name.c_str());
		g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) modes[i].name.c_str());
		g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) modes[i].name.c_str());
		gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_result_popup_meta_modes")), item, (gint) i);
		popup_result_mode_items.push_back(item);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), modes.size() > 2);

	tUnitSelectorCategories = GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_category"));
	tUnitSelector = GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_unit"));

	tUnitSelector_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_POINTER, CAIRO_GOBJECT_TYPE_SURFACE, G_TYPE_BOOLEAN);
	tUnitSelector_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnitSelector_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tUnitSelector_store_filter), 3);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitSelector_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitSelector_store), 0, GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitSelector), GTK_TREE_MODEL(tUnitSelector_store_filter));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_padding(renderer, 4, 0);
	flag_column = gtk_tree_view_column_new_with_attributes(_("Flag"), renderer, "surface", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitSelector), flag_column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitSelector), column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnitSelector_selection_changed), NULL);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tUnitSelector), FALSE);

	tUnitSelectorCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitSelectorCategories), GTK_TREE_MODEL(tUnitSelectorCategories_store));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitSelectorCategories), column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnitSelectorCategories_selection_changed), NULL);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitSelectorCategories_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitSelectorCategories_store), 0, GTK_SORT_ASCENDING);

	test_supsub();
	set_result_size_request();
	set_expression_size_request();
	set_status_size_request();
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), FALSE);

	if(win_height <= 0) gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), NULL, &win_height);
	if(minimal_mode && minimal_width > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), minimal_width, win_height);
	else if(win_width > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_width, win_height);

	if(remember_position) {
		GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		GdkMonitor *monitor = NULL;
		if(win_monitor_primary) monitor = gdk_display_get_primary_monitor(display);
		if(!monitor && win_monitor > 0) gdk_display_get_monitor(display, win_monitor - 1);
		if(monitor) {
			GdkRectangle area;
			gdk_monitor_get_workarea(monitor, &area);
#else
			GdkScreen *screen = gdk_display_get_default_screen(display);
			int i = -1;
			if(hidden_monitor_primary) i = gdk_screen_get_primary_monitor(screen);
			if(i < 0 && hidden_monitor > 0 && hidden_monitor < gdk_screen_get_n_monitors(screen)) i = hidden_monitor;
			if(i >= 0) {
				GdkRectangle area;
				gdk_screen_get_monitor_workarea(screen, i, &area);
#endif
			gint w = 0, h = 0;
			gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
			if(win_x + w > area.width) win_x = area.width - w;
			if(win_y + h > area.height) win_y = area.height - h;
			gtk_window_move(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_x + area.x, win_y + area.y);
		} else {
			gtk_window_move(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_x, win_y);
		}
	}
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), always_on_top);

	g_signal_connect_after(gtk_builder_get_object(main_builder, "historyscrolled"), "size-allocate", G_CALLBACK(on_history_resize), NULL);

	gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 18
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 18)) set_expression_size_request();
#endif

	if(history_height > 0) gtk_widget_set_size_request(tabs, -1, -1);

#ifdef _WIN32
	if(use_systray_icon) create_systray_icon();
#endif
	if(hide_on_startup) {
		if(remember_position) {
			hidden_x = win_x;
			hidden_y = win_y;
			hidden_monitor = win_monitor;
			hidden_monitor_primary = win_monitor_primary;
		}
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
	}

}

GtkWidget* get_preferences_dialog(void) {
	if(!preferences_builder) {

		preferences_builder = getBuilder("preferences.ui");
		g_assert(preferences_builder != NULL);

		g_assert(gtk_builder_get_object(preferences_builder, "preferences_dialog") != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_display_expression_status")), display_expression_status);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), parsed_in_result && !rpn_mode);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_parsed_in_result")), display_expression_status && !rpn_mode);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_expression_lines_spin_button")), (double) (expression_lines < 1 ? 3 : expression_lines));
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_vertical_padding_combo")), vertical_button_padding > 9 ? 9 : vertical_button_padding + 1);
		if(horizontal_button_padding > 4) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_horizontal_padding_combo")), horizontal_button_padding > 12 ? 9 : (horizontal_button_padding - 4) / 2 + 4 + 1);
		else gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_horizontal_padding_combo")), horizontal_button_padding + 1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_fetch_exchange_rates")), fetch_exchange_rates_at_startup);
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement")), printops.twos_complement);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement")), printops.hexadecimal_twos_complement);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement_input")), evalops.parse_options.twos_complement);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement_input")), evalops.parse_options.hexadecimal_twos_complement);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combobox_bits")), gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_bits"))));
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_history_expression")), history_expression_type);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_spell_out_logical_operators")), printops.spell_out_logical_operators);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_caret_as_xor")), caret_as_xor);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_close_with_esc")), close_with_esc > 0 || (close_with_esc < 0 && use_systray_icon));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_binary_prefixes")), CALCULATOR->usesBinaryPrefixes() > 0);
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_save_defs")), save_defs_on_exit);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_rpn_keys")), rpn_keys);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma")), CALCULATOR->getDecimalPoint() == COMMA);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")), evalops.parse_options.dot_as_separator);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")), evalops.parse_options.comma_as_separator);
		if(CALCULATOR->getDecimalPoint() == DOT) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")));
		if(CALCULATOR->getDecimalPoint() == COMMA) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_result_font")), use_custom_result_font);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_expression_font")), use_custom_expression_font);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_status_font")), use_custom_status_font);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_keypad_font")), use_custom_keypad_font);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_history_font")), use_custom_history_font);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_app_font")), use_custom_app_font);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), use_custom_result_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), custom_result_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), use_custom_expression_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), custom_expression_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), use_custom_status_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), custom_status_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), use_custom_keypad_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), custom_keypad_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_history_font")), use_custom_history_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_history_font")), custom_history_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), use_custom_app_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), custom_app_font.c_str());
		GdkRGBA c;
		gdk_rgba_parse(&c, text_color.c_str());
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_text_color")), &c);
		gdk_rgba_parse(&c, status_error_color.c_str());
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtk_builder_get_object(preferences_builder, "colorbutton_status_error_color")), &c);
		gdk_rgba_parse(&c, status_warning_color.c_str());
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
#ifdef _WIN32
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_language")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_language")));
#else
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_language")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_language")));
#endif
		if(custom_lang == "ca") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 1);
		else if(custom_lang == "de") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 2);
		else if(custom_lang == "en") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 3);
		else if(custom_lang == "es") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 4);
		else if(custom_lang == "fr") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 5);
		else if(custom_lang == "ka") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 6);
		else if(custom_lang == "nl") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 7);
		else if(custom_lang == "pt_BR") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 8);
		else if(custom_lang == "ru") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 9);
		else if(custom_lang == "sl") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 10);
		else if(custom_lang == "sv") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 11);
		else if(custom_lang == "zh_CN") gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 12);
		else gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), 0);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_language")), !ignore_locale);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_theme")), gtk_theme < 0 ? 0 : gtk_theme + 1);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_theme")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_combo_theme")));
#endif
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_use_systray_icon")), use_systray_icon);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), hide_on_startup);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")), use_systray_icon);
#ifdef _WIN32
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_use_systray_icon")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hide_on_startup")));
#endif

		gtk_builder_connect_signals(preferences_builder, NULL);

	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_update_exchange_rates_spin_button")), (double) auto_update_exchange_rates);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion")), enable_completion);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2")), enable_completion2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min")), enable_completion);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min")), enable_completion);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min")), (double) completion_min);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_enable_completion2")), enable_completion);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_min2")), enable_completion && enable_completion2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), enable_completion && enable_completion2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_min2")), (double) completion_min2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_label_completion_delay")), enable_completion);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay")), enable_completion);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_spin_completion_delay")), (double) completion_delay);

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_dialog"));
}
