/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016-2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include <glib-unix.h>
#endif
#include <unistd.h>
#include <sys/stat.h>

#include "support.h"
#include "interface.h"
#include "callbacks.h"
#include "main.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

MathStructure *mstruct, *matrix_mstruct, *parsed_mstruct, *parsed_tostruct, *displayed_mstruct;
extern MathStructure mbak_convert;
KnownVariable *vans[5], *v_memory;
GtkWidget *functions_window;
string selected_function_category;
MathFunction *selected_function;
GtkWidget *variables_window;
string selected_variable_category;
Variable *selected_variable;
string result_text, parsed_text;
GtkWidget *units_window;
string selected_unit_category;
string selected_unit_selector_category;
Unit *selected_unit, *selected_to_unit;
bool load_global_defs, fetch_exchange_rates_at_startup, first_time, showing_first_time_message;
int allow_multiple_instances = -1;
cairo_surface_t *surface_result;
GdkPixbuf *pixbuf_result;
extern bool b_busy, b_busy_command, b_busy_result, b_busy_expression;
extern int block_expression_execution;
extern vector<string> recent_functions_pre;
extern vector<string> recent_variables_pre;
extern vector<string> recent_units_pre;
extern GtkWidget *expressiontext;
extern GtkWidget *resultview;
extern PrintOptions printops;
extern bool ignore_locale;
extern bool title_modified;
extern bool minimal_mode;
extern gint hidden_x, hidden_y, hidden_monitor;
extern bool hidden_monitor_primary;
bool check_version = false;
extern int version_numbers[3];
extern int unformatted_history;
string custom_title;
extern string custom_angle_unit;
extern EvaluationOptions evalops;

MathFunction *f_answer;
MathFunction *f_expression;

GtkBuilder *main_builder, *argumentrules_builder, *csvimport_builder, *csvexport_builder, *setbase_builder, *datasetedit_builder, *datasets_builder, *decimals_builder;
GtkBuilder *functionedit_builder, *functions_builder, *matrixedit_builder, *matrix_builder, *namesedit_builder, *nbases_builder, *plot_builder, *precision_builder;
GtkBuilder *shortcuts_builder, *preferences_builder, *unit_builder, *unitedit_builder, *units_builder, *unknownedit_builder, *variableedit_builder, *variables_builder, *buttonsedit_builder;
GtkBuilder *periodictable_builder, *simplefunctionedit_builder, *percentage_builder, *calendarconversion_builder, *floatingpoint_builder;

Thread *view_thread, *command_thread;
string calc_arg, file_arg;

bool check_expression_position;
gint expression_position;
bool do_imaginary_j = false;

QalculateDateTime last_version_check_date;

#define VERSION_BEFORE(i1, i2, i3) (version_numbers[0] < i1 || (version_numbers[0] == i1 && (version_numbers[1] < i2 || (version_numbers[1] == i2 && version_numbers[2] < i3))))

static GOptionEntry options[] = {
	{"file", 'f', 0, G_OPTION_ARG_STRING, NULL, N_("Execute expressions and commands from a file"), N_("FILE")},
	{"new-instance", 'n', 0, G_OPTION_ARG_NONE, NULL, N_("Start a new instance of the application"), NULL},
	{"version", 'v', 0, G_OPTION_ARG_NONE, NULL, N_("Display the application version"), NULL},
	{"title", 0, 0, G_OPTION_ARG_STRING, NULL, N_("Specify the window title"), N_("TITLE")},
	{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, N_("Expression to calculate"), N_("[EXPRESSION]")},
	{NULL}
};

void block_completion();
void unblock_completion();

gboolean create_menus_etc(gpointer) {

	test_border();

	//create button menus after definitions have been loaded
	block_expression_execution++;
	create_button_menus();
	block_expression_execution--;

	//create dynamic menus
	generate_units_tree_struct();

	update_unit_selector_tree();

	generate_functions_tree_struct();
	generate_variables_tree_struct();
	create_fmenu();
	create_vmenu();
	create_umenu();
	create_umenu2();
	create_pmenu2();
	add_custom_angles_to_menus();

	for(int i = ((int) recent_functions_pre.size()) - 1; i >= 0; i--) {
		function_inserted(CALCULATOR->getActiveFunction(recent_functions_pre[i]));
	}
	for(int i = ((int) recent_variables_pre.size()) - 1; i >= 0; i--) {
		variable_inserted(CALCULATOR->getActiveVariable(recent_variables_pre[i]));
	}
	for(int i = ((int) recent_units_pre.size()) - 1; i >= 0; i--) {
		Unit *u = CALCULATOR->getActiveUnit(recent_units_pre[i]);
		if(!u) u = CALCULATOR->getCompositeUnit(recent_units_pre[i]);
		unit_inserted(u);
	}

	update_completion();
	
	unblock_completion();

	test_border();

	return FALSE;

}

#ifdef G_OS_UNIX
static gboolean on_sigterm_received(gpointer) {
	on_gcalc_exit(NULL, NULL, NULL);
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

	b_busy = false;
	b_busy_result = false;
	b_busy_expression = false;
	b_busy_command = false;

	main_builder = NULL; argumentrules_builder = NULL;
	csvimport_builder = NULL; datasetedit_builder = NULL; datasets_builder = NULL; decimals_builder = NULL; functionedit_builder = NULL;
	functions_builder = NULL; matrixedit_builder = NULL; matrix_builder = NULL; namesedit_builder = NULL; nbases_builder = NULL; plot_builder = NULL;
	precision_builder = NULL; preferences_builder = NULL; unit_builder = NULL; percentage_builder = NULL; shortcuts_builder = NULL;
	unitedit_builder = NULL; units_builder = NULL; unknownedit_builder = NULL; variableedit_builder = NULL; buttonsedit_builder = NULL;
	variables_builder = NULL; csvexport_builder = NULL; setbase_builder = NULL; periodictable_builder = NULL, simplefunctionedit_builder = NULL; floatingpoint_builder = NULL;

	//create the almighty Calculator object
	new Calculator(ignore_locale);

	CALCULATOR->setExchangeRatesWarningEnabled(false);

	//load application specific preferences
	load_preferences();

	mstruct = new MathStructure();
	displayed_mstruct = NULL;
	parsed_mstruct = new MathStructure();
	parsed_tostruct = new MathStructure();
	parsed_tostruct->setUndefined();
	matrix_mstruct = new MathStructure();
	mbak_convert.setUndefined();

	bool canfetch = CALCULATOR->canFetch();

	//create main window
	create_main_window();

	if(!custom_title.empty()) {
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), custom_title.c_str());
		title_modified = true;
	} else {
		update_window_title();
		title_modified = false;
	}
	g_application_set_default(G_APPLICATION(app));
	gtk_window_set_application(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), app);

	while(gtk_events_pending()) gtk_main_iteration();
	test_border();

	showing_first_time_message = first_time;

	if(calc_arg.empty() && showing_first_time_message && file_arg.empty()) {
		gtk_widget_queue_draw(resultview);
	}

	while(gtk_events_pending()) gtk_main_iteration();

	//exchange rates

	if(fetch_exchange_rates_at_startup && canfetch) {
		fetch_exchange_rates(5);
		while(gtk_events_pending()) gtk_main_iteration();
	}
	CALCULATOR->loadExchangeRates();

	string ans_str = _("ans");
	vans[0] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str, m_undefined, _("Last Answer"), false));
	vans[0]->addName(_("answer"));
	vans[0]->addName(ans_str + "1");
	vans[1] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "2", m_undefined, _("Answer 2"), false));
	vans[2] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "3", m_undefined, _("Answer 3"), false));
	vans[3] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "4", m_undefined, _("Answer 4"), false));
	vans[4] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "5", m_undefined, _("Answer 5"), false));
	if(ans_str != "ans") {
		ans_str = "ans";
		vans[0]->addName(ans_str);
		vans[0]->addName(ans_str + "1");
		vans[1]->addName(ans_str + "2");
		vans[2]->addName(ans_str + "3");
		vans[3]->addName(ans_str + "4");
		vans[4]->addName(ans_str + "5");
	}
	v_memory = new KnownVariable(CALCULATOR->temporaryCategory(), "", m_zero, _("Memory"), true, true);
	ExpressionName ename;
	ename.name = "MR";
	ename.case_sensitive = true;
	ename.abbreviation = true;
	v_memory->addName(ename);
	ename.name = "MRC";
	v_memory->addName(ename);
	CALCULATOR->addVariable(v_memory);

	//load global definitions
	if(load_global_defs && !CALCULATOR->loadGlobalDefinitions()) {
		g_print(_("Failed to load global definitions!\n"));
	}

	f_answer = CALCULATOR->addFunction(new AnswerFunction());
	f_expression = CALCULATOR->addFunction(new ExpressionFunction());
	CALCULATOR->addFunction(new SetTitleFunction());

	//load local definitions
	CALCULATOR->loadLocalDefinitions();

	if(!custom_angle_unit.empty()) {
		CALCULATOR->setCustomAngleUnit(CALCULATOR->getActiveUnit(custom_angle_unit));
		if(CALCULATOR->customAngleUnit()) custom_angle_unit = CALCULATOR->customAngleUnit()->referenceName();
	}
	if(evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()) evalops.parse_options.angle_unit = ANGLE_UNIT_NONE;

	if(do_imaginary_j && CALCULATOR->v_i->hasName("j") == 0) {
		ExpressionName ename = CALCULATOR->v_i->getName(1);
		ename.name = "j";
		ename.reference = false;
		CALCULATOR->v_i->addName(ename, 1, true);
		CALCULATOR->v_i->setChanged(false);
	}

	if(unformatted_history == 1) {
		unformatted_history = 2;
		reload_history();
	}

	//reset
	functions_window = NULL;
	selected_function_category = _("All");
	selected_function = NULL;
	variables_window = NULL;
	selected_variable_category = _("All");
	selected_variable = NULL;
	units_window = NULL;
	selected_unit_category = _("All");
	selected_unit = NULL;
	selected_to_unit = NULL;
	result_text = "0";
	parsed_text = "0";
	surface_result = NULL;
	pixbuf_result = NULL;

	//check for calculation errros regularly
	g_timeout_add_seconds(1, on_display_errors_timeout, NULL);

	check_expression_position = true;
	expression_position = 1;

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object (main_builder, "menu_item_fetch_exchange_rates")), canfetch);

	view_thread = new ViewThread;
	view_thread->start();
	command_thread = new CommandThread;

	if(!file_arg.empty()) execute_from_file(file_arg);
	if(!calc_arg.empty()) {
		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_builder_get_object(main_builder, "expressionbuffer")), calc_arg.c_str(), -1);
		execute_expression();
	} else if(!showing_first_time_message && !minimal_mode && !file_arg.empty()) {
		int base = printops.base;
		printops.base = 10;
		setResult(NULL, false, false, false);
		printops.base = base;
	} else if(!showing_first_time_message) {
		gtk_widget_queue_draw(resultview);
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

}

static void qalculate_activate(GtkApplication *app) {

	GList *list;

	list = gtk_application_get_windows(app);

	if(list) {
		if(hidden_x >= 0) {
			gtk_widget_show(GTK_WIDGET(list->data));
			GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(list->data));
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
				gtk_window_get_size(GTK_WINDOW(list->data), &w, &h);
				if(hidden_x + w > area.width) hidden_x = area.width - w;
				if(hidden_y + h > area.height) hidden_y = area.height - h;
				gtk_window_move(GTK_WINDOW(list->data), hidden_x + area.x, hidden_y + area.y);
			} else {
				gtk_window_move(GTK_WINDOW(list->data), hidden_x, hidden_y);
			}
			hidden_x = -1;
		}
#ifdef _WIN32
		gtk_window_present_with_time(GTK_WINDOW(list->data), GDK_CURRENT_TIME);
#endif
		if(expressiontext) gtk_widget_grab_focus(expressiontext);
		gtk_window_present_with_time(GTK_WINDOW(list->data), GDK_CURRENT_TIME);

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
			gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), str);
			title_modified = true;
			g_free(str);
		}
		if(hidden_x >= 0) {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
			GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
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
				gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
				if(hidden_x + w > area.width) hidden_x = area.width - w;
				if(hidden_y + h > area.height) hidden_y = area.height - h;
				gtk_window_move(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), hidden_x + area.x, hidden_y + area.y);
			} else {
				gtk_window_move(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), hidden_x, hidden_y);
			}
			hidden_x = -1;
		}
#ifdef _WIN32
		gtk_window_present_with_time(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GDK_CURRENT_TIME);
#endif
		if(expressiontext) gtk_widget_grab_focus(expressiontext);
		gtk_window_present_with_time(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GDK_CURRENT_TIME);
		if(!file_arg.empty()) execute_from_file(file_arg);
		if(!calc_arg.empty()) {
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_builder_get_object(main_builder, "expressionbuffer")), calc_arg.c_str(), -1);
			execute_expression();
		} else if(allow_multiple_instances < 0 && file_arg.empty()) {
			GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, _("By default, only one instance (one main window) of %s is allowed.\n\nIf multiple instances are opened simultaneously, only the definitions (variables, functions, etc.), mode, preferences, and history of the last closed window will be saved.\n\nDo you, despite this, want to change the default behavior and allow multiple simultaneous instances?"), "Qalculate!");
			allow_multiple_instances = gtk_dialog_run(GTK_DIALOG(edialog)) == GTK_RESPONSE_YES;
			save_preferences(false);
			gtk_widget_destroy(edialog);
		}
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
					_putenv_s("LANG", lang.c_str());
#	else
					setenv("LANG", lang.c_str(), 1);
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
				WCHAR wlocale[n];
				if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, wlocale, &n)) {
					lang = utf8_encode(wlocale);
					gsub("-", "_", lang);
					if(lang.length() > 5) lang = lang.substr(0, 5);
					if(!lang.empty()) _putenv_s("LANG", lang.c_str());
				}
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



