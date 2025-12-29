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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#ifdef G_OS_UNIX
#	include <glib-unix.h>
#endif
#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <sys/stat.h>
#include <libqalculate/qalculate.h>

#include "support.h"
#include "mainwindow.h"
#include "conversionview.h"
#include "historyview.h"
#include "keypad.h"
#include "resultview.h"
#include "menubar.h"
#include "expressioncompletion.h"
#include "expressionedit.h"
#include "settings.h"
#include "util.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool load_global_defs, first_time;
int allow_multiple_instances = -1;
bool check_version = false;
extern int unformatted_history;
string custom_title;
bool exrates = false;

extern GtkBuilder *main_builder;

string calc_arg, file_arg;

QalculateDateTime last_version_check_date;

static GOptionEntry options[] = {
	{"file", 'f', 0, G_OPTION_ARG_STRING, NULL, N_("Execute expressions and commands from a file"), N_("FILE")},
	{"new-instance", 'n', 0, G_OPTION_ARG_NONE, NULL, N_("Start a new instance of the application"), NULL},
	{"version", 'v', 0, G_OPTION_ARG_NONE, NULL, N_("Display the application version"), NULL},
	{"title", 0, 0, G_OPTION_ARG_STRING, NULL, N_("Specify the window title"), N_("TITLE")},
	{"update-exchange-rates", 0, 0, G_OPTION_ARG_NONE, NULL, N_("Update exchange rates"), NULL},
	{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, N_("Expression to calculate"), N_("[EXPRESSION]")},
	{NULL}
};

gboolean create_menus_etc(gpointer) {

	test_border();

	generate_units_tree_struct();
	update_unit_selector_tree();
	generate_functions_tree_struct();
	generate_variables_tree_struct();

	//create button menus after definitions have been loaded
	block_calculation();
	create_button_menus();
	unblock_calculation();

	//create dynamic menus
	create_fmenu();
	create_vmenu();
	create_umenu();
	create_umenu2();
	create_pmenu2();
	add_custom_angles_to_menus();
	add_recent_items();

	if(!enable_tooltips) set_tooltips_enabled(GTK_WIDGET(main_window()), FALSE);
	else if(enable_tooltips > 1) set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), FALSE);

	update_completion();

	unblock_completion();

	test_border();

	return FALSE;

}

#ifdef G_OS_UNIX
static gboolean on_sigterm_received(gpointer) {
	qalculate_quit();
	return G_SOURCE_REMOVE;
}
#endif

void create_application(GtkApplication *app) {

#ifdef _WIN32
	AllowSetForegroundWindow(0);
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 14
	gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(), "/qalculate-gtk/icons");
#endif

	gtk_window_set_default_icon_name("qalculate");

	//create the almighty Calculator object
	new Calculator(ignore_locale);

	CALCULATOR->setExchangeRatesWarningEnabled(!CALCULATOR->canFetch());

	//load application specific preferences
	load_preferences();

	//create main window
	create_main_window();

	set_custom_window_title(custom_title.empty() ? NULL : custom_title.c_str());

	g_application_set_default(G_APPLICATION(app));
	gtk_window_set_application(main_window(), app);

	while(gtk_events_pending()) gtk_main_iteration();
	test_border();

	if(calc_arg.empty() && first_time && file_arg.empty()) {
		show_result_help();
	}

	while(gtk_events_pending()) gtk_main_iteration();

	CALCULATOR->loadExchangeRates();

	//load global definitions
	if(load_global_defs && !CALCULATOR->loadGlobalDefinitions()) {
		g_print(_("Failed to load global definitions!\n"));
	}

	initialize_variables_and_functions();

	//load local definitions
	CALCULATOR->loadLocalDefinitions();

	definitions_loaded();

	if(unformatted_history == 1) {
		unformatted_history = 2;
		reload_history();
	}

	//check for calculation errros regularly
	g_timeout_add_seconds(1, on_display_errors_timeout, NULL);

	if(!file_arg.empty()) execute_from_file(file_arg);
	if(!calc_arg.empty()) {
		block_undo();
		set_expression_text(calc_arg.c_str());
		unblock_undo();
		execute_expression();
	} else if(!first_time) {
		redraw_result();
	}

	block_completion();
	set_custom_buttons();
	update_custom_buttons();
	update_accels();
	test_border();
	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 50, create_menus_etc, NULL, NULL);

	if(check_version) {
		QalculateDateTime next_version_check_date(last_version_check_date);
		next_version_check_date.addDays(14);
		if(!next_version_check_date.isFutureDate()) {
			g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 50, on_check_version_idle, NULL, NULL);
		}
	}

#ifdef G_OS_UNIX
	GSource *source = g_unix_signal_source_new(SIGTERM);
	g_source_set_callback(source, on_sigterm_received, NULL, NULL);
	g_source_attach(source, NULL);
	g_source_unref(source);
#endif

	//start_test();

}

static void qalculate_activate(GtkApplication *app) {

	GList *list;

	list = gtk_application_get_windows(app);

	if(list) {
		restore_window(GTK_WINDOW(list->data));
		return;
	}

	create_application(app);

}

static gint qalculate_handle_local_options(GtkApplication *app, GVariantDict *options_dict) {
	gboolean b = false;
	g_variant_dict_lookup(options_dict, "version", "b", &b);
	if(b) {
		g_printf(VERSION "\n");
		return 0;
	}
	gchar *str = NULL;
	g_variant_dict_lookup(options_dict, "title", "s", &str);
	if(str) {
		custom_title = str;
		g_free(str);
	}
	g_variant_dict_lookup(options_dict, "new-instance", "b", &b);
	if(b) {
		g_application_set_flags(G_APPLICATION(app), G_APPLICATION_NON_UNIQUE);
		return -1;
	}
	allow_multiple_instances = -1;
	FILE *file = NULL;
	gchar *gstr_oldfile = NULL;
	gchar *gstr_file = g_build_filename(getLocalDir().c_str(), "qalculate-gtk.cfg", NULL);
	file = fopen(gstr_file, "r");
	if(!file) {
#ifndef _WIN32
		gstr_oldfile = g_build_filename(getOldLocalDir().c_str(), "qalculate-gtk.cfg", NULL);
		file = fopen(gstr_oldfile, "r");
		if(!file) {
			g_free(gstr_oldfile);
#endif
			g_free(gstr_file);
			return -1;
#ifndef _WIN32
		}
#endif
	}
	if(file) {
		char line[100];
		string stmp, svar;
		size_t i;
		while(true) {
			if(fgets(line, 100, file) == NULL) break;
			stmp = line;
			remove_blank_ends(stmp);
			if((i = stmp.find_first_of("=")) != string::npos) {
				svar = stmp.substr(0, i);
				remove_blank_ends(svar);
				if(svar == "allow_multiple_instances") {
					string svalue = stmp.substr(i + 1, stmp.length() - (i + 1));
					remove_blank_ends(svalue);
					allow_multiple_instances = s2i(svalue);
					break;
				}
			}
		}
		fclose(file);
		if(gstr_oldfile) {
			recursiveMakeDir(getLocalDir());
			move_file(gstr_oldfile, gstr_file);
			g_free(gstr_oldfile);
		}
	}
	g_free(gstr_file);
	if(allow_multiple_instances > 0) g_application_set_flags(G_APPLICATION(app), G_APPLICATION_NON_UNIQUE);
#ifdef _WIN32
	else AllowSetForegroundWindow(ASFW_ANY);
#endif
	return -1;
}

static gint qalculate_command_line(GtkApplication *app, GApplicationCommandLine *cmd_line) {
	GVariantDict *options_dict = g_application_command_line_get_options_dict(cmd_line);
	gchar *str = NULL;
	file_arg = "";
	g_variant_dict_lookup(options_dict, "file", "s", &str);
	if(str) {
		file_arg = str;
		g_free(str);
	}
	g_variant_dict_lookup(options_dict, "update-exchange-rates", "b", &exrates);
	gchar **remaining = NULL;
	g_variant_dict_lookup(options_dict, G_OPTION_REMAINING, "^as", &remaining);
	calc_arg = "";
	for(int i = 0; remaining && i < (int) g_strv_length(remaining); i++) {
		if(i > 1) {
			calc_arg += " ";
		}
		if(strlen(remaining[i]) >= 2 && ((remaining[i][0] == '\"' && remaining[i][strlen(remaining[i]) - 1] == '\"') || (remaining[i][0] == '\'' && remaining[i][strlen(remaining[i]) - 1] == '\''))) {
			calc_arg += remaining[i] + 1;
			calc_arg.erase(calc_arg.length() - 1);
		} else {
			calc_arg += remaining[i];
		}
	}
	if(main_builder) {
		str = NULL;
		g_variant_dict_lookup(options_dict, "title", "s", &str);
		if(str) {
			set_custom_window_title(str);
			g_free(str);
		}
		restore_window();
		if(!file_arg.empty()) execute_from_file(file_arg);
		if(!calc_arg.empty()) {
			block_undo();
			set_expression_text(calc_arg.c_str());
			unblock_undo();
			execute_expression();
		} else if(allow_multiple_instances < 0 && file_arg.empty()) {
			GtkWidget *edialog = gtk_message_dialog_new(main_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, _("By default, only one instance (one main window) of %s is allowed.\n\nIf multiple instances are opened simultaneously, only the definitions (variables, functions, etc.), mode, preferences, and history of the last closed window will be saved.\n\nDo you, despite this, want to change the default behavior and allow multiple simultaneous instances?"), "Qalculate!");
			allow_multiple_instances = gtk_dialog_run(GTK_DIALOG(edialog)) == GTK_RESPONSE_YES;
			save_preferences(false);
			gtk_widget_destroy(edialog);
		}
		if(exrates) update_exchange_rates();
	} else if(exrates) {
		new Calculator();
		return !CALCULATOR->fetchExchangeRates();
	} else {
		create_application(app);
	}
	return 0;
}

#ifdef _WIN32
#	include <winsock2.h>
#	include <windows.h>
#	include <shlobj.h>
#	include <direct.h>
#	include <knownfolders.h>
#	include <initguid.h>
#	include <shlobj.h>
#	include <unordered_set>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <dirent.h>
using std::unordered_set;
#endif

int main (int argc, char *argv[]) {

	GtkApplication *app;
	gint status;

#ifdef ENABLE_NLS
	gchar *gstr_file = g_build_filename(getLocalDir().c_str(), "qalculate-gtk.cfg", NULL);
	FILE *file = fopen(gstr_file, "r");
	char line[10000];
	string stmp, lang;
	if(file) {
		while(true) {
			if(fgets(line, 10000, file) == NULL) break;
			if(strcmp(line, "ignore_locale=1\n") == 0) {
				ignore_locale = true;
				break;
			} else if(strcmp(line, "ignore_locale=0\n") == 0) {
				break;
			} else if(strncmp(line, "language=", 9) == 0) {
				lang = line + sizeof(char) * 9;
				remove_blank_ends(lang);
				if(!lang.empty()) {
#	ifdef _WIN32
					_putenv_s("LANGUAGE", lang.c_str());
#	else
					setenv("LANGUAGE", lang.c_str(), 1);
					if(lang.find(".") == string::npos && lang.find("_") != string::npos) lang += ".utf8";
					if(lang.find(".") != string::npos) setenv("LC_MESSAGES", lang.c_str(), 1);
#	endif
				}
				break;
			}
		}
		fclose(file);
	}
	if(!ignore_locale) {
#	ifdef _WIN32
		if(lang.empty()) {
			ULONG nlang = 0;
			DWORD n = 0;
			if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, NULL, &n)) {
				WCHAR* wlocale = new WCHAR[n];
				if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, wlocale, &n)) {
					for(size_t i = 2; nlang > 1 && i < n - 1; i++) {
						if(wlocale[i] == '\0') {
							if(wlocale[i + 1] == '\0') break;
							wlocale[i] = ':';
							nlang--;
						}
					}
					lang = utf8_encode(wlocale);
					gsub("-", "_", lang);
					if(!lang.empty()) _putenv_s("LANGUAGE", lang.c_str());
				}
				delete[] wlocale;
			}
		}
		bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
#	else
		bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#	endif
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
		textdomain(GETTEXT_PACKAGE);
	}
#endif

	if(!ignore_locale) setlocale(LC_ALL, "");

	app = gtk_application_new("io.github.Qalculate", G_APPLICATION_HANDLES_COMMAND_LINE);
	g_application_add_main_option_entries(G_APPLICATION(app), options);
	g_signal_connect(app, "activate", G_CALLBACK(qalculate_activate), NULL);
	g_signal_connect(app, "handle_local_options", G_CALLBACK(qalculate_handle_local_options), NULL);
	g_signal_connect(app, "command_line", G_CALLBACK(qalculate_command_line), NULL);

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

#ifdef _WIN32
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path);
	string tmpdir = buildPath(path, "Temp"), str, newest_file;
	unordered_set<string> tmpfiles;
	struct dirent *ep;
	struct stat stats;
	time_t newest_time = 0;
	DIR *dp = opendir(tmpdir.c_str());
	if(dp) {
		while((ep = readdir(dp))) {
			str = ep->d_name;
			if(str.find("gdbus-nonce-file-") == 0) {
				str = buildPath(tmpdir, str);
				if(stat(str.c_str(), &stats) == 0) {
					if(stats.st_mtime > newest_time) {
						newest_time = stats.st_mtime;
						newest_file = str;
					}
					tmpfiles.insert(str);
				}
			}
		}
		closedir(dp);
		if(!newest_file.empty()) tmpfiles.erase(newest_file);
		for(unordered_set<string>::iterator it = tmpfiles.begin(); it != tmpfiles.end(); ++it) {
			remove(it->c_str());
		}
	}
#endif

	return status;

}



