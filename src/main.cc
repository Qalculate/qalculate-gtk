/*
    Qalculate (GTK+ UI)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna_k@fmgirl.com)

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
#include <unistd.h>

#include "support.h"
#include "interface.h"
#include "callbacks.h"
#include "main.h"

MathStructure *mstruct, *matrix_mstruct, *parsed_mstruct, *parsed_tostruct, *displayed_mstruct;
bool prev_result_approx;
string *parsed_to_str;
KnownVariable *vans[5];
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
cairo_surface_t *surface_result;
GdkPixbuf *pixbuf_result;
extern bool b_busy, b_busy_command, b_busy_result, b_busy_expression;
extern vector<string> recent_functions_pre;
extern vector<string> recent_variables_pre;
extern vector<string> recent_units_pre;
extern GtkWidget *expression;
extern PrintOptions printops;

GtkBuilder *main_builder, *argumentrules_builder, *csvimport_builder, *csvexport_builder, *setbase_builder, *datasetedit_builder, *datasets_builder, *decimals_builder;
GtkBuilder *functionedit_builder, *functions_builder, *matrixedit_builder, *matrix_builder, *namesedit_builder, *nbases_builder, *plot_builder, *precision_builder;
GtkBuilder *preferences_builder, *unit_builder, *unitedit_builder, *units_builder, *unknownedit_builder, *variableedit_builder, *variables_builder;
GtkBuilder *periodictable_builder;

FILE *view_pipe_r, *view_pipe_w, *command_pipe_r, *command_pipe_w;
pthread_t view_thread, command_thread;
pthread_attr_t view_thread_attr, command_thread_attr;
bool command_thread_started;

bool do_timeout, check_expression_position;
gint expression_position;

static const char **args;
static GOptionEntry options[] = {
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
	&args, NULL},
	{NULL}
};
int main (int argc, char **argv) {

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	GError *error = NULL;

	gtk_init_with_args(&argc, &argv, NULL, options, GETTEXT_PACKAGE, &error);

	gtk_window_set_default_icon_from_file(PACKAGE_DATA_DIR "/pixmaps/qalculate.png", &error);

	string calc_arg;
	for(int i = 0; args && args[i]; i++) {
		if(i > 1) {
			calc_arg += " ";
		}
		if(strlen(args[i]) >= 2 && ((args[i][0] == '\"' && args[i][strlen(args[i]) - 1] == '\"') || (args[i][0] == '\'' && args[i][strlen(args[i]) - 1] == '\''))) {
			calc_arg += args[i] + 1;
			calc_arg.erase(calc_arg.length() - 1);
		} else {
			calc_arg += args[i];
		}
	}
	b_busy = false; 
	b_busy_result = false;
	b_busy_expression = false;
	b_busy_command = false;

	main_builder = NULL; argumentrules_builder = NULL; 
	csvimport_builder = NULL; datasetedit_builder = NULL; datasets_builder = NULL; decimals_builder = NULL; functionedit_builder = NULL; 
	functions_builder = NULL; matrixedit_builder = NULL; matrix_builder = NULL; namesedit_builder = NULL; nbases_builder = NULL; plot_builder = NULL; 
	precision_builder = NULL; preferences_builder = NULL; unit_builder = NULL; 
	unitedit_builder = NULL; units_builder = NULL; unknownedit_builder = NULL; variableedit_builder = NULL; 
	variables_builder = NULL;	csvexport_builder = NULL; setbase_builder = NULL; periodictable_builder = NULL;

	//create the almighty Calculator object
	new Calculator();
	
	CALCULATOR->setExchangeRatesWarningEnabled(false);
	
	//load application specific preferences
	load_preferences();

	mstruct = new MathStructure();
	displayed_mstruct = new MathStructure();
	parsed_mstruct = new MathStructure();
	parsed_tostruct = new MathStructure();
	parsed_tostruct->setUndefined();
	matrix_mstruct = new MathStructure();
	prev_result_approx = false;
	parsed_to_str = new string;

	bool canplot = CALCULATOR->canPlot();
	bool canfetch = CALCULATOR->canFetch();
	
	//create main window
	create_main_window();

	while(gtk_events_pending()) gtk_main_iteration();
	
	showing_first_time_message = first_time;

	if(!calc_arg.empty()) {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "expression")), calc_arg.c_str());
	} else if(first_time) {		
		PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview")), NULL);
		GdkRGBA rgba;
		cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))));
		pango_layout_set_markup(layout, _("<span font=\"10\">Type a mathematical expression above, e.g. \"5 + 2 / 3\",\nand press the enter key.</span>"), -1);
		gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))), gtk_widget_get_state_flags(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))), &rgba);
		cairo_move_to(cr, 6, 0);
		gdk_cairo_set_source_rgba(cr, &rgba);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);
		cairo_destroy(cr);
	}

	while(gtk_events_pending()) gtk_main_iteration();

	//exchange rates
	
	if(fetch_exchange_rates_at_startup && canfetch) {
		fetch_exchange_rates(5);
		while(gtk_events_pending()) gtk_main_iteration();
		CALCULATOR->loadExchangeRates();
	} else if(!CALCULATOR->loadExchangeRates() && first_time && canfetch) {
		GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object (main_builder, "main_window")), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You need to download exchange rates to be able to convert between different currencies. You can later get current exchange rates by selecting “Update Exchange Rates” under the File menu.\n\nDo you want to fetch exchange rates now from the Internet?"));
		int question_answer = gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
		while(gtk_events_pending()) gtk_main_iteration();
		if(question_answer == GTK_RESPONSE_YES) {
			fetch_exchange_rates(5);
			while(gtk_events_pending()) gtk_main_iteration();
			CALCULATOR->loadExchangeRates();
		}
	}

	

	string ans_str = _("ans");
	vans[0] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str, m_undefined, _("Last Answer"), false));
	vans[0]->addName(_("answer"));
	vans[0]->addName(ans_str + "1");
	vans[1] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "2", m_undefined, _("Answer 2"), false));
	vans[2] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "3", m_undefined, _("Answer 3"), false));
	vans[3] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "4", m_undefined, _("Answer 4"), false));
	vans[4] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "5", m_undefined, _("Answer 5"), false));

	//load global definitions
	if(load_global_defs && !CALCULATOR->loadGlobalDefinitions()) {
		g_print(_("Failed to load global definitions!\n"));
	}

	//load local definitions
	CALCULATOR->loadLocalDefinitions();

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
	do_timeout = true;
	g_timeout_add(100, on_display_errors_timeout, NULL);
	
	check_expression_position = true;
	expression_position = 1;
	g_timeout_add(50, on_check_expression_position_timeout, NULL);
	
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object (main_builder, "menu_item_plot_functions")), canplot);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object (main_builder, "menu_item_fetch_exchange_rates")), canfetch);

	//create dynamic menus
	generate_units_tree_struct();
	generate_functions_tree_struct();
	generate_variables_tree_struct();
	create_fmenu();
	create_vmenu();	
	create_umenu();
	//create_pmenu();	
	create_umenu2();
	create_pmenu2();
	
	update_unit_selector_tree();

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
	
	int pipe_wr[] = {0, 0};
	pipe(pipe_wr);
	view_pipe_r = fdopen(pipe_wr[0], "r");
	view_pipe_w = fdopen(pipe_wr[1], "w");
	pthread_attr_init(&view_thread_attr);
	pthread_create(&view_thread, &view_thread_attr, view_proc, view_pipe_r);
	
	int pipe_wr2[] = {0, 0};
	pipe(pipe_wr2);
	command_pipe_r = fdopen(pipe_wr2[0], "r");
	command_pipe_w = fdopen(pipe_wr2[1], "w");
	pthread_attr_init(&command_thread_attr);
	command_thread_started = false;
	
	if(!calc_arg.empty()) {
		execute_expression();
	} else if(!first_time) {
		int base = printops.base;
		printops.base = 10;
		setResult(NULL, false, false, false);
		printops.base = base;
	}

	gchar *gstr = g_build_filename(g_get_home_dir(), ".qalculate", "accelmap", NULL);
	gtk_accel_map_load(gstr);
	g_free(gstr);

	gtk_main();

	return 0;
}
