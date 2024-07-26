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
#include <libqalculate/qalculate.h>

#include "support.h"
#include "callbacks.h"
#include "interface.h"
#include "expressionedit.h"
#include "expressioncompletion.h"
#include "conversionview.h"
#include "historyview.h"
#include "stackview.h"
#include "keypad.h"
#include "menubar.h"
#include "settings.h"
#include "util.h"
#include "precisiondialog.h"
#include "decimalsdialog.h"
#include "setbasedialog.h"

#include <deque>

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::iterator;
using std::list;
using std::deque;

extern GtkBuilder *main_builder;

GtkWidget *mainwindow;

GtkWidget *tabs, *keypad, *historyview, *expander_keypad, *expander_history, *expander_stack, *expander_convert;

GtkWidget *resultview;
extern GtkWidget *expressiontext;
extern GtkTextBuffer *expressionbuffer;
GtkWidget *statuslabel_l, *statuslabel_r;
GtkAccelGroup *accel_group;

GtkCssProvider *topframe_provider = NULL, *resultview_provider = NULL, *statuslabel_l_provider = NULL, *statuslabel_r_provider = NULL, *app_provider = NULL, *app_provider_theme = NULL, *statusframe_provider = NULL, *color_provider = NULL;

extern bool show_keypad, show_history, show_stack, show_convert, persistent_keypad, minimal_mode;
extern string status_error_color, status_warning_color, text_color;
extern bool status_error_color_set, status_warning_color_set, text_color_set;
extern int enable_tooltips;
extern bool toe_changed;
string themestr;

extern std::vector<custom_button> custom_buttons;

extern PrintOptions printops;
extern EvaluationOptions evalops;

extern bool rpn_mode, rpn_keys, adaptive_interval_display;

extern vector<GtkWidget*> mode_items;
extern vector<GtkWidget*> popup_result_mode_items;

gint win_height, win_width, win_x, win_y, win_monitor, history_height;
bool win_monitor_primary;
extern bool remember_position, always_on_top, aot_changed;
extern gint minimal_width;

extern Unit *latest_button_unit, *latest_button_currency;
extern string latest_button_unit_pre, latest_button_currency_pre;

unordered_map<string, cairo_surface_t*> flag_surfaces;
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
		focus_expression();
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

	update_expression_colors(initial, text_color_set);

	if(initial || !text_color_set) {


		update_history_colors(initial);

		GdkRGBA c;
		gtk_style_context_get_color(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, &c);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
		if(gdk_rgba_equal(&c, &bg_color)) {
			gtk_style_context_get_color(gtk_widget_get_style_context(statuslabel_l), GTK_STATE_FLAG_NORMAL, &c);
		}
#endif
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

	keypad = GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"));
	historyview = GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview"));

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), accel_group);

	if(win_width > 0) gtk_window_set_default_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_width, win_height > 0 ? win_height : -1);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result")), false);
#endif

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14
	gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_swap")), "object-flip-vertical-symbolic", GTK_ICON_SIZE_BUTTON);
#endif

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

	statuslabel_l = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_status_left"));
	statuslabel_r = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_status_right"));
	tabs = GTK_WIDGET(gtk_builder_get_object(main_builder, "tabs"));

	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 3);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(statuslabel_r, 12);
	gtk_widget_set_margin_start(statuslabel_l, 9);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
#else
	gtk_widget_set_margin_right(statuslabel_r, 12);
	gtk_widget_set_margin_left(statuslabel_l, 9);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_label_set_xalign(GTK_LABEL(statuslabel_l), 0.0);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_l), 0.5);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_r), 0.5);
#else
	gtk_misc_set_alignment(GTK_MISC(statuslabel_l), 0.0, 0.5);
#endif

	resultview_provider = gtk_css_provider_new();
	statuslabel_l_provider = gtk_css_provider_new();
	statuslabel_r_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(resultview), GTK_STYLE_PROVIDER(resultview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_l), GTK_STYLE_PROVIDER(statuslabel_l_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_r), GTK_STYLE_PROVIDER(statuslabel_r_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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

	update_app_font(true);
	update_result_font(true);
	update_status_font(true);

	update_status_text();

	update_colors(true);

	set_status_operator_symbols();
	set_app_operator_symbols();

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

	gchar *theme_name = NULL;
	g_object_get(gtk_settings_get_default(), "gtk-theme-name", &theme_name, NULL);
	if(theme_name) {
		themestr = theme_name;
		g_free(theme_name);
	}

	GtkCssProvider *notification_style = gtk_css_provider_new(); gtk_css_provider_load_from_data(notification_style, "* {border-radius: 5px}", -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "overlaybox"))), GTK_STYLE_PROVIDER(notification_style), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	create_keypad();
	create_history_view();
	create_conversion_view();
	create_stack_view();
	create_expression_edit();
	create_menubar();
	update_status_menu(true);

	if(rpn_mode) {
		gtk_label_set_angle(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), 90.0);
		// RPN Enter (calculate and add to stack)
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), _("ENTER"));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), _("Calculate expression and add to stack"));
	} else {
		gtk_widget_hide(expander_stack);
	}

	gtk_builder_connect_signals(main_builder, NULL);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);

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

