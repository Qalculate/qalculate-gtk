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
#include <sstream>
#include <fstream>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#ifdef USE_WEBKITGTK
#	include <webkit2/webkit2.h>
#endif

#include "support.h"
#include "settings.h"
#include "mainwindow.h"
#include "openhelp.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

gint help_width = -1, help_height = -1;
gdouble help_zoom = -1.0;

GtkWidget *button_zoomout = NULL;

bool read_help_settings_line(string &svar, string &svalue, int &v) {
	if(svar == "help_width") {
		help_width = v;
	} else if(svar == "help_height") {
		help_height = v;
	} else if(svar == "help_zoom") {
		help_zoom = strtod(svalue.c_str(), NULL);
	} else {
		return false;
	}
	return true;
}
void write_help_settings(FILE *file) {
	if(help_width != -1) fprintf(file, "help_width=%i\n", help_width);
	if(help_height != -1) fprintf(file, "help_height=%i\n", help_height);
	if(help_zoom > 0.0) fprintf(file, "help_zoom=%f\n", help_zoom);
}

string get_doc_uri(string file, bool with_proto = true) {
	string surl;
#ifndef LOCAL_HELP
	surl = "https://qalculate.github.io/manual/";
	surl += file;
#else
	if(with_proto) surl += "file://";
#	ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	surl += exepath;
	surl.resize(surl.find_last_of('\\'));
	if(surl.substr(surl.length() - 4) == "\\bin") {
		surl.resize(surl.find_last_of('\\'));
		surl += "\\share\\doc\\";
		surl += PACKAGE;
		surl += "\\html\\";
	} else if(surl.substr(surl.length() - 6) == "\\.libs") {
		surl.resize(surl.find_last_of('\\'));
		surl.resize(surl.find_last_of('\\'));
		surl += "\\doc\\html\\";
	} else {
		surl += "\\doc\\";
	}
	gsub("\\", "/", surl);
	surl += file;
#	else
	surl += PACKAGE_DOC_DIR "/html/";
	surl += file;
#	endif
#endif
	return surl;
}

#ifdef USE_WEBKITGTK
unordered_map<GtkWidget*, GtkWidget*> help_find_entries;
bool backwards_search;
void on_help_stop_search(GtkSearchEntry *w, gpointer view) {
	webkit_find_controller_search_finish(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)));
	gtk_entry_set_text(GTK_ENTRY(w), "");
}
void on_help_search_found(WebKitFindController*, guint, gpointer) {
	backwards_search = false;
}
void string_strdown(const string &str, string &strnew) {
	char *cstr = utf8_strdown(str.c_str());
	if(cstr) {
		strnew = cstr;
		free(cstr);
	} else {
		strnew = str;
	}
}
vector<string> help_files;
vector<string> help_contents;
void on_help_search_failed(WebKitFindController *f, gpointer w) {
	g_signal_handlers_disconnect_matched((gpointer) f, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_help_search_failed, NULL);
	string str = gtk_entry_get_text(GTK_ENTRY(help_find_entries[GTK_WIDGET(w)]));
	remove_blank_ends(str);
	remove_duplicate_blanks(str);
	if(str.empty()) return;
	string strl;
	string_strdown(str, strl);
	gsub("&", "&amp;", strl);
	gsub(">", "&gt;", strl);
	gsub("<", "&lt;", strl);
	if(!webkit_web_view_get_uri(WEBKIT_WEB_VIEW(w))) return;
	string file = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(w));
	size_t i = file.rfind("/");
	if(i != string::npos) file = file.substr(i + 1);
	i = file.find("#");
	if(i != string::npos) file = file.substr(0, i);
	size_t help_i = 0;
	if(help_files.empty()) {
		std::ifstream ifile(get_doc_uri("index.html", false).c_str());
		if(!ifile.is_open()) return;
		std::stringstream ssbuffer;
		ssbuffer << ifile.rdbuf();
		string sbuffer;
		string_strdown(ssbuffer.str(), sbuffer);
		ifile.close();
		help_files.push_back("index.html");
		help_contents.push_back(sbuffer);
		i = sbuffer.find(".html\"");
		while(i != string::npos) {
			size_t i2 = sbuffer.rfind("\"", i);
			if(i2 != string::npos) {
				string sfile = sbuffer.substr(i2 + 1, (i + 5) - (i2 + 1));
				if(sfile.find("/") == string::npos) {
					for(i2 = 0; i2 < help_files.size(); i2++) {
						if(help_files[i2] == sfile) break;
					}
					if(i2 == help_files.size()) {
						help_files.push_back(sfile);
						std::ifstream ifile_i(get_doc_uri(sfile, false).c_str());
						string sbuffer_i;
						if(ifile_i.is_open()) {
							std::stringstream ssbuffer_i;
							ssbuffer_i << ifile_i.rdbuf();
							string_strdown(ssbuffer_i.str(), sbuffer_i);
							ifile_i.close();
						}
						help_contents.push_back(sbuffer_i);
					}
				}
			}
			i = sbuffer.find(".html\"", i + 1);
		}
	}
	for(i = 0; i < help_files.size(); i++) {
		if(file == help_files[i]) {
			help_i = i;
			break;
		}
	}
	size_t help_cur = help_i;
	while(true) {
		if(backwards_search) {
			if(help_i == 0) help_i = help_files.size() - 1;
			else help_i--;
		} else {
			help_i++;
			if(help_i == help_files.size()) help_i = 0;
		}
		if(help_i == help_cur) {
			webkit_find_controller_search(f, str.c_str(), backwards_search ? WEBKIT_FIND_OPTIONS_BACKWARDS | WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE : WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE, 10000);
			backwards_search = false;
			break;
		}
		string sbuffer = help_contents[help_i];
		i = sbuffer.find("<body");
		while(i != string::npos) {
			i = sbuffer.find(strl, i + 1);
			if(i == string::npos) break;
			size_t i2 = sbuffer.find_last_of("<>", i);
			if(i2 != string::npos && sbuffer[i2] == '>') {
				webkit_web_view_load_uri(WEBKIT_WEB_VIEW(w), get_doc_uri(help_files[help_i]).c_str());
				break;
			}
			i = sbuffer.find(">", i);
		}
		if(i != string::npos) break;
	}
}
void on_help_search_changed(GtkSearchEntry *w, gpointer view) {
	string str = gtk_entry_get_text(GTK_ENTRY(w));
	remove_blank_ends(str);
	remove_duplicate_blanks(str);
	if(str.empty()) {
		webkit_find_controller_search_finish(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)));
	} else {
		g_signal_handlers_disconnect_matched((gpointer) webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_help_search_failed, NULL);
		webkit_find_controller_search(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)), str.c_str(), WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE, 10000);
	}
}
void on_help_next_match(GtkWidget*, gpointer view) {
	backwards_search = false;
	g_signal_handlers_disconnect_matched((gpointer) webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_help_search_failed, NULL);
	g_signal_connect(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)), "failed-to-find-text", G_CALLBACK(on_help_search_failed), view);
	webkit_find_controller_search_next(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)));
}
void on_help_previous_match(GtkWidget*, gpointer view) {
	backwards_search = true;
	g_signal_handlers_disconnect_matched((gpointer) webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_help_search_failed, NULL);
	g_signal_connect(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)), "failed-to-find-text", G_CALLBACK(on_help_search_failed), view);
	webkit_find_controller_search_previous(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(view)));
}
gboolean on_help_configure_event(GtkWidget*, GdkEventConfigure *event, gpointer) {
	int w = gdk_window_get_width(gdk_event_get_window((GdkEvent*) event));
	int h = gdk_window_get_height(gdk_event_get_window((GdkEvent*) event));
	if(help_width != -1 || w != 800 || h != 600) {
		help_width = w;
		help_height = h;
	}
	return FALSE;
}
gboolean on_help_key_press_event(GtkWidget *d, GdkEventKey *event, gpointer w) {
	GdkModifierType state; guint keyval = 0;
	gdk_event_get_state((GdkEvent*) event, &state);
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
	GtkWidget *entry_find = help_find_entries[GTK_WIDGET(w)];
	switch(keyval) {
		case GDK_KEY_Escape: {
			string str = gtk_entry_get_text(GTK_ENTRY(entry_find));
			remove_blank_ends(str);
			remove_duplicate_blanks(str);
			if(str.empty()) {
				gtk_widget_destroy(d);
			} else {
				on_help_stop_search(GTK_SEARCH_ENTRY(entry_find), w);
				return TRUE;
			}
			return TRUE;
		}
		case GDK_KEY_BackSpace: {
			if(gtk_widget_has_focus(entry_find)) return FALSE;
			webkit_web_view_go_back(WEBKIT_WEB_VIEW(w));
			return TRUE;
		}
		case GDK_KEY_Left: {
			if(state & GDK_CONTROL_MASK || state & GDK_MOD1_MASK) {
				webkit_web_view_go_back(WEBKIT_WEB_VIEW(w));
				return TRUE;
			}
			break;
		}
		case GDK_KEY_Right: {
			if(state & GDK_CONTROL_MASK || state & GDK_MOD1_MASK) {
				webkit_web_view_go_forward(WEBKIT_WEB_VIEW(w));
				return TRUE;
			}
			break;
		}
		case GDK_KEY_KP_Add: {}
		case GDK_KEY_plus: {
			if(state & GDK_CONTROL_MASK || state & GDK_MOD1_MASK) {
				help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) + 0.1;
				webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
				gtk_widget_set_sensitive(button_zoomout, TRUE);
				return TRUE;
			}
			break;
		}
		case GDK_KEY_KP_Subtract: {}
		case GDK_KEY_minus: {
			if((state & GDK_CONTROL_MASK || state & GDK_MOD1_MASK) && webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) > 0.11) {
				help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) - 0.1;
				webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
				gtk_widget_set_sensitive(button_zoomout, help_zoom > 0.11);
				return TRUE;
			}
			break;
		}
		case GDK_KEY_Home: {
			if(state & GDK_CONTROL_MASK || state & GDK_MOD1_MASK) {
				webkit_web_view_load_uri(WEBKIT_WEB_VIEW(w), get_doc_uri("index.html").c_str());
				return TRUE;
			}
			break;
		}
		case GDK_KEY_f: {
			if(state & GDK_CONTROL_MASK) {
				gtk_widget_grab_focus(GTK_WIDGET(entry_find));
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}
void on_help_button_home_clicked(GtkButton*, gpointer w) {
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(w), get_doc_uri("index.html").c_str());
}
void on_help_button_zoomin_clicked(GtkButton*, gpointer w) {
	help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) + 0.1;
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
	gtk_widget_set_sensitive(button_zoomout, TRUE);
}
void on_help_button_zoomout_clicked(GtkButton*, gpointer w) {
	if(webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) > 0.11) {
		help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) - 0.1;
		webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
		gtk_widget_set_sensitive(button_zoomout, help_zoom > 0.11);
	}
}
gboolean on_help_context_menu(WebKitWebView*, WebKitContextMenu*, GdkEvent*, WebKitHitTestResult *hit_test_result, gpointer) {
	return webkit_hit_test_result_context_is_image(hit_test_result) || webkit_hit_test_result_context_is_link(hit_test_result) || webkit_hit_test_result_context_is_media(hit_test_result);
}

void on_help_load_changed_b(WebKitWebView *w, WebKitLoadEvent load_event, gpointer button) {
	if(load_event == WEBKIT_LOAD_FINISHED) gtk_widget_set_sensitive(GTK_WIDGET(button), webkit_web_view_can_go_back(w));
}
void on_help_load_changed_f(WebKitWebView *w, WebKitLoadEvent load_event, gpointer button) {
	if(load_event == WEBKIT_LOAD_FINISHED) gtk_widget_set_sensitive(GTK_WIDGET(button), webkit_web_view_can_go_forward(w));
}
void on_help_load_changed(WebKitWebView *w, WebKitLoadEvent load_event, gpointer) {
	if(load_event == WEBKIT_LOAD_FINISHED) {
		string str = gtk_entry_get_text(GTK_ENTRY(help_find_entries[GTK_WIDGET(w)]));
		remove_blank_ends(str);
		remove_duplicate_blanks(str);
		if(!str.empty()) webkit_find_controller_search(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(w)), str.c_str(), backwards_search ? WEBKIT_FIND_OPTIONS_BACKWARDS | WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE : WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE, 10000);
		backwards_search = false;
	}
}
gboolean on_help_decide_policy(WebKitWebView*, WebKitPolicyDecision *d, WebKitPolicyDecisionType t, gpointer window) {
	if(t == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
		const gchar *uri = webkit_uri_request_get_uri(webkit_navigation_action_get_request(webkit_navigation_policy_decision_get_navigation_action (WEBKIT_NAVIGATION_POLICY_DECISION(d))));
		if(uri[0] == 'h' && (uri[4] == ':' || uri[5] == ':')) {
			GError *error = NULL;
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
			gtk_show_uri_on_window(GTK_WINDOW(window), uri, gtk_get_current_event_time(), &error);
#else
			gtk_show_uri(NULL, uri, gtk_get_current_event_time(), &error);
#endif
			if(error) {
				gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
				GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Failed to open %s.\n%s"), uri, error_str);
				if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
				g_free(error_str);
				g_error_free(error);
			}
			webkit_policy_decision_ignore(d);
			return TRUE;
		}
	}
	return FALSE;
}
#endif

void show_help(const char *file, GtkWindow *parent) {
#ifdef _WIN32
	if(ShellExecuteA(NULL, "open", get_doc_uri("index.html").c_str(), NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE) 32) {
		GtkWidget *dialog = gtk_message_dialog_new(parent, (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not display help for Qalculate!."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
#elif USE_WEBKITGTK
	GtkWidget *dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_window_set_title(GTK_WINDOW(dialog), "Qalculate! Manual");
	if(parent) {
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
		gtk_window_set_modal(GTK_WINDOW(dialog), gtk_window_get_modal(parent));
	}
	gtk_window_set_default_size(GTK_WINDOW(dialog), help_width > 0 ? help_width : 800, help_height > 0 ? help_height : 600);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *hbox_l = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *hbox_c = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *hbox_r = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *button_back = gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
	GtkWidget *button_home = gtk_button_new_from_icon_name("go-home-symbolic", GTK_ICON_SIZE_BUTTON);
	GtkWidget *button_forward = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_BUTTON);
	GtkWidget *entry_find = gtk_search_entry_new();
	GtkWidget *button_previous_match = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_BUTTON);
	GtkWidget *button_next_match = gtk_button_new_from_icon_name("go-down-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_entry_set_width_chars(GTK_ENTRY(entry_find), 25);
	GtkWidget *button_zoomin = gtk_button_new_from_icon_name("zoom-in-symbolic", GTK_ICON_SIZE_BUTTON);
	button_zoomout = gtk_button_new_from_icon_name("zoom-out-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_sensitive(button_back, FALSE);
	gtk_widget_set_sensitive(button_forward, FALSE);
	gtk_container_add(GTK_CONTAINER(hbox_l), button_back);
	gtk_container_add(GTK_CONTAINER(hbox_l), button_home);
	gtk_container_add(GTK_CONTAINER(hbox_l), button_forward);
	gtk_container_add(GTK_CONTAINER(hbox_c), entry_find);
	gtk_container_add(GTK_CONTAINER(hbox_c), button_previous_match);
	gtk_container_add(GTK_CONTAINER(hbox_c), button_next_match);
	gtk_container_add(GTK_CONTAINER(hbox_r), button_zoomout);
	gtk_container_add(GTK_CONTAINER(hbox_r), button_zoomin);
	gtk_box_pack_start(GTK_BOX(hbox), hbox_l, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), hbox_c, TRUE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), hbox_r, FALSE, FALSE, 0);
	gtk_style_context_add_class(gtk_widget_get_style_context(hbox_l), "linked");
	gtk_style_context_add_class(gtk_widget_get_style_context(hbox_c), "linked");
	gtk_style_context_add_class(gtk_widget_get_style_context(hbox_r), "linked");
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	gtk_container_add(GTK_CONTAINER(dialog), vbox);
	GtkWidget *scrolledWeb = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand(scrolledWeb, TRUE);
	gtk_widget_set_vexpand(scrolledWeb, TRUE);
	gtk_container_add(GTK_CONTAINER(vbox), scrolledWeb);
	GtkWidget *webView = webkit_web_view_new();
	help_find_entries[webView] = entry_find;
	WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView));
#	if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 32
	webkit_settings_set_enable_plugins(settings, FALSE);
#	endif
	webkit_settings_set_zoom_text_only(settings, FALSE);
	gtk_widget_set_sensitive(button_zoomout, help_zoom > 0.11 || help_zoom < 0.0);
	if(help_zoom > 0.0) webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webView), help_zoom);
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(main_window())), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
	webkit_settings_set_default_font_family(settings, pango_font_description_get_family(font_desc));
	webkit_settings_set_default_font_size(settings, webkit_settings_font_size_to_pixels(pango_font_description_get_size(font_desc) / PANGO_SCALE));
	pango_font_description_free(font_desc);
	g_signal_connect(G_OBJECT(dialog), "key-press-event", G_CALLBACK(on_help_key_press_event), (gpointer) webView);
	g_signal_connect(G_OBJECT(webView), "context-menu", G_CALLBACK(on_help_context_menu), NULL);
	g_signal_connect(G_OBJECT(webView), "load-changed", G_CALLBACK(on_help_load_changed_b), (gpointer) button_back);
	g_signal_connect(G_OBJECT(webView), "load-changed", G_CALLBACK(on_help_load_changed_f), (gpointer) button_forward);
	g_signal_connect(G_OBJECT(webView), "load-changed", G_CALLBACK(on_help_load_changed), NULL);
	g_signal_connect(G_OBJECT(webView), "decide-policy", G_CALLBACK(on_help_decide_policy), dialog);
	g_signal_connect_swapped(G_OBJECT(button_back), "clicked", G_CALLBACK(webkit_web_view_go_back), (gpointer) webView);
	g_signal_connect_swapped(G_OBJECT(button_forward), "clicked", G_CALLBACK(webkit_web_view_go_forward), (gpointer) webView);
	g_signal_connect(G_OBJECT(button_home), "clicked", G_CALLBACK(on_help_button_home_clicked), (gpointer) webView);
	g_signal_connect(G_OBJECT(button_zoomin), "clicked", G_CALLBACK(on_help_button_zoomin_clicked), (gpointer) webView);
	g_signal_connect(G_OBJECT(button_zoomout), "clicked", G_CALLBACK(on_help_button_zoomout_clicked), (gpointer) webView);
	g_signal_connect(G_OBJECT(entry_find), "search-changed", G_CALLBACK(on_help_search_changed), (gpointer) webView);
	g_signal_connect(G_OBJECT(entry_find), "next-match", G_CALLBACK(on_help_next_match), (gpointer) webView);
	g_signal_connect(G_OBJECT(entry_find), "previous-match", G_CALLBACK(on_help_previous_match), (gpointer) webView);
	g_signal_connect(G_OBJECT(button_next_match), "clicked", G_CALLBACK(on_help_next_match), (gpointer) webView);
	g_signal_connect(G_OBJECT(button_previous_match), "clicked", G_CALLBACK(on_help_previous_match), (gpointer) webView);
	g_signal_connect(G_OBJECT(entry_find), "stop-search", G_CALLBACK(on_help_stop_search), (gpointer) webView);
	g_signal_connect(G_OBJECT(entry_find), "activate", G_CALLBACK(on_help_next_match), (gpointer) webView);
	g_signal_connect(webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webView)), "found-text", G_CALLBACK(on_help_search_found), NULL);
	gtk_container_add(GTK_CONTAINER(scrolledWeb), GTK_WIDGET(webView));
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), get_doc_uri(file).c_str());
	g_signal_connect(G_OBJECT(dialog), "configure-event", G_CALLBACK(on_help_configure_event), NULL);
	gtk_widget_grab_focus(GTK_WIDGET(webView));
	gtk_widget_show_all(dialog);
#else
	GError *error = NULL;
#	if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_show_uri_on_window(parent, get_doc_uri(file).c_str(), gtk_get_current_event_time(), &error);
#	else
	gtk_show_uri(NULL, get_doc_uri(file).c_str(), gtk_get_current_event_time(), &error);
#	endif
	if(error) {
		gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
		GtkWidget *dialog = gtk_message_dialog_new(parent, (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not display help for Qalculate!.\n%s"), error_str);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(error_str);
		g_error_free(error);
	}
#endif
}
