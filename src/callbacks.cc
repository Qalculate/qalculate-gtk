/*
    Qalculate (GTK+ UI)

    Copyright (C) 2003-2007, 2008, 2016-2017  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <limits>

#include "support.h"
#include "callbacks.h"
#include "interface.h"
#include "main.h"
#include <stack>
#include <deque>

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
#elif 	defined(__GNUC__)

#	ifndef __has_include
#	define __has_include(x) 0
#	endif

#	if (defined(__clang__) && __has_include(<tr1/unordered_map>)) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#		include <tr1/unordered_map>
		namespace Sgi = std;
#		define unordered_map std::tr1::unordered_map
#	else
#		if __GNUC__ < 3
#			include <hash_map.h>
			namespace Sgi { using ::hash_map; }; // inherit globals
#		else
#			include <ext/hash_map>
#			if __GNUC__ == 3 && __GNUC_MINOR__ == 0
				namespace Sgi = std;               // GCC 3.0
#			else
				namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
#			endif
#		endif
#		define unordered_map Sgi::hash_map
#	endif
#else      // ...  there are other compilers, right?
	namespace Sgi = std;
#	define unordered_map Sgi::hash_map
#endif

extern bool do_timeout, check_expression_position;
extern gint expression_position;

extern GtkBuilder *main_builder, *argumentrules_builder, *csvimport_builder, *csvexport_builder, *setbase_builder, *datasetedit_builder, *datasets_builder, *decimals_builder;
extern GtkBuilder *functionedit_builder, *functions_builder, *matrixedit_builder, *matrix_builder, *namesedit_builder, *nbases_builder, *plot_builder, *precision_builder;
extern GtkBuilder *preferences_builder, *unit_builder, *unitedit_builder, *units_builder, *unknownedit_builder, *variableedit_builder, *variables_builder;
extern GtkBuilder *periodictable_builder, *simplefunctionedit_builder, *percentage_builder;

extern GtkWidget *mainwindow;

bool changing_in_nbases_dialog;

extern GtkWidget *tabs, *expander_keypad, *expander_history, *expander_stack, *expander_convert;
extern GtkEntryCompletion *completion;
extern GtkWidget *completion_view, *completion_window, *completion_scrolled;
extern GtkListStore *completion_store;
extern GtkTreeModel *completion_filter;

extern GtkCssProvider *expression_provider, *resultview_provider, *statuslabel_l_provider, *statuslabel_r_provider;

extern GtkWidget *expressiontext, *statuslabel_l, *statuslabel_r;
extern GtkTextBuffer *expressionbuffer;
extern GtkTextTag *expression_par_tag;
extern GtkWidget *f_menu, *v_menu, *u_menu, *u_menu2, *recent_menu;
extern KnownVariable *vans[5];
extern GtkWidget *tPlotFunctions;
extern GtkListStore *tPlotFunctions_store;
extern GtkWidget *tFunctionArguments;
extern GtkListStore *tFunctionArguments_store;
extern GtkWidget *tSubfunctions;
extern GtkListStore *tSubfunctions_store;
extern GtkWidget *tFunctions, *tFunctionCategories;
extern GtkListStore *tFunctions_store;
extern GtkTreeStore *tFunctionCategories_store;
extern GtkWidget *tVariables, *tVariableCategories;
extern GtkListStore *tVariables_store;
extern GtkTreeStore *tVariableCategories_store;
extern GtkWidget *tUnits, *tUnitCategories;
extern GtkListStore *tUnits_store;
extern GtkTreeStore *tUnitCategories_store;
extern GtkWidget *tUnitSelectorCategories;
extern GtkWidget *tUnitSelector;
extern GtkListStore *tUnitSelector_store;
extern GtkTreeStore *tUnitSelectorCategories_store;
extern GtkWidget *tDataObjects, *tDatasets;
extern GtkListStore *tDataObjects_store, *tDatasets_store;
extern GtkWidget *tDataProperties;
extern GtkListStore *tDataProperties_store;
extern GtkWidget *tNames;
extern GtkListStore *tNames_store;
extern GtkAccelGroup *accel_group;
extern string selected_function_category;
extern MathFunction *selected_function;
DataObject *selected_dataobject = NULL;
DataSet *selected_dataset = NULL;
DataProperty *selected_dataproperty = NULL;
MathFunction *edited_function = NULL;
KnownVariable *edited_variable = NULL;
UnknownVariable *edited_unknown = NULL;
KnownVariable *edited_matrix = NULL;
Unit *edited_unit = NULL;
DataSet *edited_dataset = NULL;
DataProperty *edited_dataproperty = NULL;
bool editing_variable = false, editing_unknown = false, editing_matrix = false, editing_unit = false, editing_function = false, editing_dataset = false, editing_dataproperty = false;
size_t selected_subfunction;
size_t last_subfunction_index;
Argument *selected_argument;
Argument *edited_argument;
extern string selected_variable_category;
extern Variable *selected_variable;
extern string selected_unit_category;
extern string selected_unit_selector_category;
extern Unit *selected_unit;
extern Unit *selected_to_unit;
bool save_mode_on_exit;
bool save_defs_on_exit;
bool use_custom_result_font, use_custom_expression_font, use_custom_status_font;
bool save_custom_result_font = false, save_custom_expression_font = false, save_custom_status_font = false;
string custom_result_font, custom_expression_font, custom_status_font;
int scale_n = 0;
bool hyp_is_on, inv_is_on;
bool show_keypad, show_history, show_stack, show_convert, continuous_conversion, set_missing_prefixes;
extern bool load_global_defs, fetch_exchange_rates_at_startup, first_time, showing_first_time_message, allow_multiple_instances;
int auto_update_exchange_rates;
bool first_error;
bool display_expression_status, enable_completion;
bool block_unit_convert, block_unit_selector_convert;
extern MathStructure *mstruct, *matrix_mstruct, *parsed_mstruct, *parsed_tostruct, *displayed_mstruct;
extern string result_text, parsed_text;
bool result_text_approximate = false;
string result_text_long;
extern GtkWidget *resultview;
extern GtkWidget *historyview;
extern GtkWidget *stackview;
extern GtkListStore *stackstore, *historystore;
extern GtkCellRenderer *register_renderer;
extern GtkTreeViewColumn *register_column, *history_column, *history_index_column;
extern cairo_surface_t *surface_result;
vector<vector<GtkWidget*> > insert_element_entries;
bool b_busy, b_busy_command, b_busy_result, b_busy_expression, b_busy_fetch;
cairo_surface_t *tmp_surface;
bool expression_has_changed = false, current_object_has_changed = false, expression_has_changed2 = false, expression_has_changed_pos = false;
bool block_result_update = false, block_expression_execution = false, block_display_parse = false;
string parsed_expression;
bool parsed_had_errors = false, parsed_had_warnings = false;
vector<DataProperty*> tmp_props;
vector<DataProperty*> tmp_props_orig;

string command_convert_units_string;
Unit *command_convert_unit;

extern GtkWidget *tMatrixEdit, *tMatrix;
extern GtkListStore *tMatrixEdit_store, *tMatrix_store;
vector<GtkTreeViewColumn*> matrix_edit_columns, matrix_columns;

extern GtkAccelGroup *accel_group;

extern gint win_height, win_width, history_height, keypad_height, variables_width, variables_height, variables_position, units_width, units_height, units_position, functions_width, functions_height, functions_hposition, functions_vposition, datasets_width, datasets_height, datasets_hposition, datasets_vposition1, datasets_vposition2;

vector<string> expression_history;
int expression_history_index = -1;
bool dont_change_index = false;
bool result_font_updated = false;
bool first_draw_of_result = true;

PlotLegendPlacement default_plot_legend_placement = PLOT_LEGEND_TOP_RIGHT;
bool default_plot_display_grid = true;
bool default_plot_full_border = false;
string default_plot_min = "0";
string default_plot_max = "10";
string default_plot_step = "1";
int default_plot_sampling_rate = 100;
bool default_plot_use_sampling_rate = true;
bool default_plot_rows = false;
int default_plot_type = 0;
PlotStyle default_plot_style = PLOT_STYLE_LINES;
PlotSmoothing default_plot_smoothing = PLOT_SMOOTHING_NONE;
string default_plot_variable = "x";
bool default_plot_color = true;

bool b_editing_stack = false;

string status_error_color, status_warning_color;

bool names_edited = false;

int message_type = 1;

GtkTextIter current_object_start, current_object_end;
bool editing_to_expression = false;
bool stop_timeouts = false;

PrintOptions printops, parse_printops;
EvaluationOptions evalops;

bool rpn_mode, rpn_keys;

extern Thread *view_thread, *command_thread;
bool exit_in_progress = false, command_aborted = false, display_aborted = false, result_too_long = false;

vector<mode_struct> modes;
vector<GtkWidget*> mode_items;
vector<GtkWidget*> popup_result_mode_items;

deque<string> inhistory;
deque<int> inhistory_type;
vector<MathStructure*> history_parsed;
vector<MathStructure*> history_answer;

deque<string> expression_undo_buffer;
size_t undo_index = 0;
bool add_to_undo = true;

int current_inhistory_index = -1;
int history_index = 0;
int initial_inhistory_index = 0;
int nr_of_new_expressions = 0;

unordered_map<void*, string> number_map;
unordered_map<void*, bool> number_approx_map;
unordered_map<void*, string> number_exp_map;
unordered_map<void*, bool> number_exp_minus_map;

extern MathFunction *f_answer;
extern MathFunction *f_expression;

unordered_map<string, GtkTreeIter> convert_category_map;

extern gchar history_error_color[8];
extern gchar history_warning_color[8];
extern gchar history_parse_color[8];

bool status_error_color_set;
bool status_warning_color_set;

string old_fromValue, old_toValue;

extern QalculateDate last_version_check_date;
string last_found_version;

#define TEXT_TAGS			"<span size=\"xx-large\">"
#define TEXT_TAGS_END			"</span>"
#define TEXT_TAGS_SMALL			"<span size=\"large\">"
#define TEXT_TAGS_SMALL_END		"</span>"
#define TEXT_TAGS_XSMALL		"<span size=\"medium\">"
#define TEXT_TAGS_XSMALL_END		"</span>"

#define TTB(str)			if(scaledown <= 0) {str += "<span size=\"xx-large\">";} else if(scaledown == 1) {str += "<span size=\"x-large\">";} else if(scaledown == 2) {str += "<span size=\"large\">";} else {str += "<span size=\"medium\">";}
#define TTB_SMALL(str)			if(scaledown <= 0) {str += "<span size=\"large\">";} else if(scaledown == 1) {str += "<span size=\"medium\">";} else if(scaledown == 2) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";}
#define TTB_XSMALL(str)			if(scaledown <= 0) {str += "<span size=\"medium\">";} else if(scaledown == 1) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";}
#define TTBP(str)			if(ips.power_depth > 0) {TTB_SMALL(str);} else {TTB(str);}
#define TTBP_SMALL(str)			if(ips.power_depth > 0) {TTB_XSMALL(str);} else {TTB_SMALL(str);}
#define TTE(str)			str += "</span>";
#define TT(str, x)			{if(scaledown <= 0) {str += "<span size=\"xx-large\">";} else if(scaledown == 1) {str += "<span size=\"x-large\">";} else if(scaledown == 2) {str += "<span size=\"large\">";} else {str += "<span size=\"medium\">";} str += x; str += "</span>";}
#define TT_SMALL(str, x)		{if(scaledown <= 0) {str += "<span size=\"large\">";} else if(scaledown == 1) {str += "<span size=\"medium\">";} else if(scaledown == 2) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";} str += x; str += "</span>";}
#define TT_XSMALL(str, x)		{if(scaledown <= 0) {str += "<span size=\"medium\">";} else if(scaledown == 1) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";} str += x; str += "</span>";}
#define TTP(str, x)			if(ips.power_depth > 0) {TT_SMALL(str, x);} else {TT(str, x);}
#define TTP_SMALL(str, x)		if(ips.power_depth > 0) {TT_XSMALL(str, x);} else {TT_SMALL(str, x);}

#define PANGO_TT(layout, x)		if(scaledown <= 0) {pango_layout_set_markup(layout, "<span size=\"xx-large\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"x-large\">" x "</span>", -1);} else if(scaledown == 2) {pango_layout_set_markup(layout, "<span size=\"large\">" x "</span>", -1);} else {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);}
#define PANGO_TT_SMALL(layout, x)	if(scaledown <= 0) {pango_layout_set_markup(layout, "<span size=\"large\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);} else {pango_layout_set_markup(layout, "<span size=\"x-small\">" x "</span>", -1);}
#define PANGO_TT_XSMALL(layout, x)	if(scaledown <= 0) {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"small\">" x "</span>", -1);} else {pango_layout_set_markup(layout, "<span size=\"x-small\">" x "</span>", -1);}
#define PANGO_TTP(layout, x)		if(ips.power_depth > 0) {PANGO_TT_SMALL(layout, x);} else {PANGO_TT(layout, x);}
#define PANGO_TTP_SMALL(layout, x)	if(ips.power_depth > 0) {PANGO_TT_XSMALL(layout, x);} else {PANGO_TT_SMALL(layout, x);}

#define CALCULATE_SPACE_W		gint space_w, space_h; PangoLayout *layout_space = gtk_widget_create_pango_layout(resultview, NULL); PANGO_TTP(layout_space, " "); pango_layout_get_pixel_size(layout_space, &space_w, &space_h); g_object_unref(layout_space);

AnswerFunction::AnswerFunction() : MathFunction(_("answer"), 1, 1, _("Utilities"), _("History Answer Value")) {
	if(strcmp(_("answer"), "answer")) addName("answer");
	VectorArgument *arg = new VectorArgument(_("History Index(es)"));
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO, true, true, INTEGER_TYPE_SINT));
	setArgumentDefinition(1, arg);
	setHidden(true);
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
ExpressionFunction::ExpressionFunction() : MathFunction(_("expression"), 1, 1, _("Utilities"), _("History Parsed Expression")) {
	if(strcmp(_("expression"), "expression")) addName("expression");
	VectorArgument *arg = new VectorArgument(_("History Index(es)"));
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO, true, true, INTEGER_TYPE_SINT));
	setArgumentDefinition(1, arg);
	setHidden(true);
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

enum {
	COMMAND_FACTORIZE,
	COMMAND_SIMPLIFY,
	COMMAND_TRANSFORM,
	COMMAND_CONVERT_UNIT,
	COMMAND_CONVERT_STRING,
	COMMAND_CONVERT_BASE,
	COMMAND_CONVERT_OPTIMAL
};

void show_help(const char *file, GObject *parent) {
	string surl;
#ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	surl = "file://";
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
	if((int) ShellExecuteA(NULL, "open", surl.c_str(), NULL, NULL, SW_SHOWNORMAL) <= 32) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not display help for Qalculate!."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
#else
	GError *error = NULL;
	surl = "file://" PACKAGE_DOC_DIR "/html/";
	surl += file;
#	if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_show_uri_on_window(GTK_WINDOW(parent), surl.c_str(), gtk_get_current_event_time(), &error);
#	else
	gtk_show_uri(NULL, surl.c_str(), gtk_get_current_event_time(), &error);
#	endif
	if(error) {
		gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not display help for Qalculate!.\n%s"), error_str);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(error_str);
		g_error_free(error);
	}
#endif
}

string fix_history_string(const string &str) {
	string str2 = str;
	gsub("&", "&amp;", str2);
	gsub(">", "&gt;", str2);
	gsub("<", "&lt;", str2);
	return str2;
}

bool completion_blocked = false;
void block_completion() {
	gtk_widget_hide(completion_window);
	completion_blocked = true;
}
void unblock_completion() {
	completion_blocked = false;
}

string get_expression_text() {
	GtkTextIter istart, iend;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	string text = gtext;
	g_free(gtext);
	return text;
}
string get_selected_expression_text(bool return_all_if_no_sel = false) {
	if(!gtk_text_buffer_get_has_selection(expressionbuffer)) {
		if(return_all_if_no_sel) {
			string str = get_expression_text();
			remove_blank_ends(str);
			return str;
		}
		return "";
	}
	GtkTextIter istart, iend;
	gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);
	gchar *gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	string text = gtext;
	g_free(gtext);
	return text;
}
void add_expression_to_undo() {
	if(expression_undo_buffer.size() > 100) expression_undo_buffer.pop_front();
	else undo_index++;
	while(undo_index < expression_undo_buffer.size()) {
		expression_undo_buffer.pop_back();
	}
	expression_undo_buffer.push_back(get_expression_text());
}

void overwrite_expression_selection(const gchar *text) {
	block_completion();
	add_to_undo = false;
	gtk_text_buffer_delete_selection(expressionbuffer, FALSE, TRUE);
	add_to_undo = true;
	if(text) gtk_text_buffer_insert_at_cursor(expressionbuffer, text, -1);
	unblock_completion();
}
void set_expression_text(const gchar *text) {
	bool b = add_to_undo;
	add_to_undo = false;
	gtk_text_buffer_set_text(expressionbuffer, text, -1);
	add_to_undo = b;
	if(add_to_undo) add_expression_to_undo();
}
void clear_expression_text() {
	gtk_text_buffer_set_text(expressionbuffer, "", -1);
}
bool expression_is_empty() {
	GtkTextIter istart;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	return gtk_text_iter_is_end(&istart);
}

void set_assumptions_items(AssumptionType, AssumptionSign);
void set_mode_items(const PrintOptions&, const EvaluationOptions&, AssumptionType, AssumptionSign, bool, int, bool);

string sdot, saltdot, sdiv, sslash, stimes, sminus;
string sdot_s, saltdot_s, sdiv_s, sslash_s, stimes_s, sminus_s;

void set_operator_symbols() {
	if(can_display_unicode_string_function_exact(SIGN_MINUS, (void*) expressiontext)) sminus = SIGN_MINUS;
	else sminus = "-";
	if(can_display_unicode_string_function(SIGN_DIVISION, (void*) expressiontext)) sdiv = SIGN_DIVISION;
	else sdiv = "/";
	sslash = "/";
	if(can_display_unicode_string_function(SIGN_MULTIDOT, (void*) expressiontext)) sdot = SIGN_MULTIDOT;
	else sdot = "*";
	if(can_display_unicode_string_function(SIGN_MIDDLEDOT, (void*) expressiontext)) saltdot = SIGN_MIDDLEDOT;
	else saltdot = "*";
	if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) expressiontext)) stimes = SIGN_MULTIPLICATION;
	else stimes = "*";
	
	if(can_display_unicode_string_function_exact(SIGN_MINUS, (void*) statuslabel_l)) sminus_s = SIGN_MINUS;
	else sminus_s = "-";
	if(can_display_unicode_string_function(SIGN_DIVISION, (void*) statuslabel_l)) sdiv_s = SIGN_DIVISION;
	else sdiv_s = "/";
	if(can_display_unicode_string_function_exact(SIGN_DIVISION, (void*) statuslabel_l)) sslash_s = SIGN_DIVISION_SLASH;
	else sslash_s = "/";
	if(can_display_unicode_string_function(SIGN_MULTIDOT, (void*) statuslabel_l)) sdot_s = SIGN_MULTIDOT;
	else sdot_s = "*";
	if(can_display_unicode_string_function(SIGN_MIDDLEDOT, (void*) statuslabel_l)) saltdot_s = SIGN_MIDDLEDOT;
	else saltdot_s = "*";
	if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) statuslabel_l)) stimes_s = SIGN_MULTIPLICATION;
	else stimes_s = "*";
}

const char *expression_add_sign() {
	//if(printops.use_unicode_signs) return SIGN_PLUS;
	return "+";
}
const char *expression_sub_sign() {
	if(!printops.use_unicode_signs) return "-";
	return sminus.c_str();
}
const char *expression_times_sign() {
	if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_DOT) return sdot.c_str();
	else if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) return saltdot.c_str();
	else if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_X) return stimes.c_str();
	return "*";
}
const char *expression_divide_sign() {
	if(!printops.use_unicode_signs) return "/";
	if(printops.division_sign == DIVISION_SIGN_DIVISION) return sdiv.c_str();
	return sslash.c_str();
}

#define EXPRESSION_STOP 1
#define EXPRESSION_SPINNER 2
#define EXPRESSION_INFO 3
void update_expression_icons(int id = 0) {
	switch(id) {
		case EXPRESSION_STOP: {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_stop")));
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Stop process"));
			break;
		}
		case EXPRESSION_SPINNER: {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionspinner")));
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Stop process"));
			break;
		}
		case EXPRESSION_INFO: {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")));
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), gtk_widget_get_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon"))));
			break;
		}
		default: {
			if(gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) != GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals"))) {
				gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")), GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals")));
				gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), rpn_mode ? _("Calculate expression and add to stack") : _("Calculate expression"));
			}
		}
	}
}

void result_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	set_result_size_request();
	result_font_updated = TRUE;
	set_operator_symbols();
	result_display_updated();
}
void expression_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	set_expression_size_request();
	set_operator_symbols();
}


PangoCoverageLevel get_least_coverage(const gchar *gstr, GtkWidget *widget) {

	PangoCoverageLevel level = PANGO_COVERAGE_EXACT;
	PangoContext *context = gtk_widget_get_pango_context(widget);
	PangoLanguage *language = pango_context_get_language(context);
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(widget), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
	PangoFontset *fontset = pango_context_load_fontset(context, font_desc, language);
	pango_font_description_free(font_desc);
	while(gstr[0] != '\0') {
		if(gstr[0] < 0) {
			gunichar gu = g_utf8_get_char_validated(gstr, -1);
			if(gu != (gunichar) -1 && gu != (gunichar) -2) {
				PangoFont *font = pango_fontset_get_font(fontset, (guint) gu);
				if(font) {
					PangoCoverage *coverage = pango_font_get_coverage(font, language);
					if(pango_coverage_get(coverage, (int) gu) < level) {
						level = pango_coverage_get(coverage, gu);
					}
					g_object_unref(font);
					pango_coverage_unref(coverage);
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
	return get_least_coverage(str, (GtkWidget*) w) >= PANGO_COVERAGE_APPROXIMATE;
}
bool can_display_unicode_string_function_exact(const char *str, void *w) {
	if(!w) w = (void*) historyview;
	return get_least_coverage(str, (GtkWidget*) w) >= PANGO_COVERAGE_EXACT;
}

void set_result_size_request() {
	MathStructure mtest, mtest2;
	mtest.setType(STRUCT_DIVISION);
	mtest.addChild(1);
	mtest2.setType(STRUCT_POWER);
	mtest2.addChild(1);
	mtest2.addChild(1);
	mtest.addChild(mtest2);
	mtest.format();
	PrintOptions po;
	po.can_display_unicode_string_function = &can_display_unicode_string_function;
	po.can_display_unicode_string_arg = (void*) resultview;
	cairo_surface_t *tmp_surface2 = draw_structure(mtest, po);
	if(tmp_surface2) {
		cairo_surface_flush(tmp_surface2);
		int h = cairo_image_surface_get_height(tmp_surface2) / gtk_widget_get_scale_factor(resultview);
		h += 8;
		cairo_surface_destroy(tmp_surface2);
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), -1, h);
	}
}

void set_expression_size_request() {
	PangoLayout *layout_test = gtk_widget_create_pango_layout(expressiontext, "Äy\nÄy\nÄy");
	gint w, h;
	pango_layout_get_pixel_size(layout_test, &w, &h);
	gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), -1, h + 12);
}

void set_unicode_buttons() {
	if(printops.use_unicode_signs) {
		if(can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_sub"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), "<b>" SIGN_MINUS "</b>");
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), "<b>" MINUS "</b>");
		if(can_display_unicode_string_function(SIGN_PLUS, (void*) gtk_builder_get_object(main_builder, "label_add"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_add")), "<b>" SIGN_PLUS "</b>");
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_add")), "<b>" PLUS "</b>");
		if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_times"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), "<b>" SIGN_MULTIPLICATION "</b>");
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), "<b>" MULTIPLICATION "</b>");
		if(can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), "<b>" SIGN_DIVISION_SLASH "</b>");
		else if(can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), "<b>" SIGN_DIVISION "</b>");
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), DIVISION);
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_dot")), (string("<b>") + CALCULATOR->getDecimalPoint() + "</b>").c_str());
		
		if(can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_history_sub"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sub")), SIGN_MINUS);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sub")), MINUS);
		if(can_display_unicode_string_function(SIGN_PLUS, (void*) gtk_builder_get_object(main_builder, "label_history_add"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_add")), SIGN_PLUS);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_add")), PLUS);
		if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_history_times"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_times")), SIGN_MULTIPLICATION);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_times")), MULTIPLICATION);
		if(can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_history_divide"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), SIGN_DIVISION_SLASH);
		else if(can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_history_divide"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), SIGN_DIVISION);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), DIVISION);
		if(can_display_unicode_string_function(SIGN_SQRT, (void*) gtk_builder_get_object(main_builder, "label_sqrt"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sqrt")), SIGN_SQRT);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sqrt")), "sqrt");
		
		
		if(can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_rpn_sub"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_sub")), SIGN_MINUS);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_sub")), MINUS);
		if(can_display_unicode_string_function(SIGN_PLUS, (void*) gtk_builder_get_object(main_builder, "label_rpn_add"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_add")), SIGN_PLUS);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_add")), PLUS);
		if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_rpn_times"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_times")), SIGN_MULTIPLICATION);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_times")), MULTIPLICATION);
		if(can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_rpn_divide"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_divide")), SIGN_DIVISION_SLASH);
		else if(can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_rpn_divide"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_divide")), SIGN_DIVISION);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_divide")), DIVISION);	
		if(can_display_unicode_string_function(SIGN_SQRT, (void*) gtk_builder_get_object(main_builder, "label_rpn_sqrt"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_sqrt")), SIGN_SQRT);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_sqrt")), "sqrt");
		
		if(can_display_unicode_string_function(SIGN_SQRT, (void*) gtk_builder_get_object(main_builder, "label_sqrt"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt")), SIGN_SQRT);
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt")), "sqrt");
		if(can_display_unicode_string_function("x̄", (void*) gtk_builder_get_object(main_builder, "label_mean"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_mean")), "x̄");
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_mean")), "mean");
		if(can_display_unicode_string_function("∑", (void*) gtk_builder_get_object(main_builder, "label_sum"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sum")), "∑");
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sum")), "sum");
		if(can_display_unicode_string_function("π", (void*) gtk_builder_get_object(main_builder, "label_pi"))) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_pi")), "π");
		else gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_pi")), "pi");
	} else {
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), "<b>" MINUS "</b>");
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_add")), "<b>" PLUS "/<b>");
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), "<b>" MULTIPLICATION "</b>");
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), "<b>" DIVISION "</b>");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sqrt")), "sqrt");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_mean")), "mean");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sum")), "sum");
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_pi")), "pi");
		gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_dot")), (string("<b>") + CALCULATOR->getDecimalPoint() + "</b>").c_str());
		
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sub")), MINUS);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_add")), PLUS);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_times")), MULTIPLICATION);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_divide")), DIVISION);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_history_sqrt")), "sqrt");
		
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_sub")), MINUS);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_add")), PLUS);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_times")), MULTIPLICATION);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_divide")), DIVISION);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_rpn_sqrt")), "sqrt");
	}
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_comma")), CALCULATOR->getComma().c_str());
}


struct tree_struct {
	string item;
	list<tree_struct> items;
	list<tree_struct>::iterator it;
	list<tree_struct>::reverse_iterator rit;
	vector<void*> objects;	
	tree_struct *parent;
	void sort() {
		items.sort();
		for(list<tree_struct>::iterator it = items.begin(); it != items.end(); ++it) {
			it->sort();
		}
	}
	bool operator < (const tree_struct &s1) const {
		return item < s1.item;	
	}	
};

tree_struct function_cats, unit_cats, variable_cats;
vector<void*> ia_units, ia_variables, ia_functions;	
vector<string> recent_functions_pre;
vector<string> recent_variables_pre;
vector<string> recent_units_pre;
vector<GtkWidget*> recent_function_items;
vector<GtkWidget*> recent_variable_items;
vector<GtkWidget*> recent_unit_items;
vector<MathFunction*> recent_functions;
vector<Variable*> recent_variables;
vector<Unit*> recent_units;
Unit *latest_button_unit = NULL, *latest_button_currency = NULL;
string latest_button_unit_pre, latest_button_currency_pre;

bool is_answer_variable(Variable *v) {
	return v == vans[0] || v == vans[1] || v == vans[2] || v == vans[3] || v == vans[4];
}

void wrap_expression_selection() {
	if(!gtk_text_buffer_get_has_selection(expressionbuffer)) return;
	GtkTextMark *mstart = gtk_text_buffer_get_selection_bound(expressionbuffer);
	if(!mstart) return;
	GtkTextMark *mend = gtk_text_buffer_get_insert(expressionbuffer);
	if(!mend) return;
	GtkTextIter istart, iend;
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &istart, mstart);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iend, mend);
	if(gtk_text_iter_compare(&istart, &iend) > 0) {
		add_to_undo = false;
		gtk_text_buffer_insert(expressionbuffer, &iend, "(", -1);
		gtk_text_buffer_get_iter_at_mark(expressionbuffer, &istart, mstart);
		add_to_undo = true;
		gtk_text_buffer_insert(expressionbuffer, &istart, ")", -1);
		gtk_text_buffer_place_cursor(expressionbuffer, &istart);
	} else {
		add_to_undo = false;
		gtk_text_buffer_insert(expressionbuffer, &istart, "(", -1);
		gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iend, mend);
		add_to_undo = true;
		gtk_text_buffer_insert(expressionbuffer, &iend, ")", -1);
		gtk_text_buffer_place_cursor(expressionbuffer, &iend);
	}
}

void show_message(const gchar *text, GtkWidget *win) {
	GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", text);
	gtk_dialog_run(GTK_DIALOG(edialog));
	gtk_widget_destroy(edialog);
}
bool ask_question(const gchar *text, GtkWidget *win) {
	GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_YES_NO, "%s", text);
	int question_answer = gtk_dialog_run(GTK_DIALOG(edialog));
	gtk_widget_destroy(edialog);
	return question_answer == GTK_RESPONSE_YES;
}

#define STATUS_SPACE	if(b) str += "  "; else b = true;

void set_status_text(string text, bool break_begin = false, bool had_errors = false, bool had_warnings = false) {

	string str;
	if(had_errors) {
		str = "<span foreground=\"";
		str += status_error_color;
		str += "\">";
	} else if(had_warnings) {
		str = "<span foreground=\"";
		str += status_warning_color;
		str += "\">";
	}
	if(text.empty()) str += " ";
	else str += text;
	if(had_errors || had_warnings) str += "</span>";

	if(break_begin) gtk_label_set_ellipsize(GTK_LABEL(statuslabel_l), PANGO_ELLIPSIZE_START);
	else gtk_label_set_ellipsize(GTK_LABEL(statuslabel_l), PANGO_ELLIPSIZE_END);
	
	gtk_label_set_markup(GTK_LABEL(statuslabel_l), str.c_str());
	
}

void display_parse_status();

void update_status_text() {

	string str = "<span size=\"small\">";
	
	bool b = false;
	if(evalops.approximation == APPROXIMATION_EXACT) {
		STATUS_SPACE
		str += _("EXACT");
	} else if(evalops.approximation == APPROXIMATION_APPROXIMATE) {
		STATUS_SPACE
		str += _("APPROX");
	}
	if(evalops.parse_options.rpn) {
		STATUS_SPACE
		str += _("RPN");
	}
	switch(evalops.parse_options.base) {
		case BASE_DECIMAL: {
			break;
		}
		case BASE_BINARY: {
			STATUS_SPACE
			str += _("BIN");
			break;
		}
		case BASE_OCTAL: {
			STATUS_SPACE
			str += _("OCT");
			break;
		}
		case BASE_HEXADECIMAL: {
			STATUS_SPACE
			str += _("HEX");
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			STATUS_SPACE
			str += _("ROMAN");
			break;
		}
		default: {
			STATUS_SPACE
			str += i2s(evalops.parse_options.base);
			break;
		}
	}
	switch (evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			STATUS_SPACE
			str += _("DEG");
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			STATUS_SPACE
			str += _("RAD");
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			STATUS_SPACE
			str += _("GRA");
			break;
		}
		default: {}
	}
	if(evalops.parse_options.read_precision != DONT_READ_PRECISION) {
		STATUS_SPACE
		str += _("PREC");
	}
	if(!evalops.parse_options.functions_enabled) {
		STATUS_SPACE
		str += "<s>";
		str += _("FUNC");
		str += "</s>";
	}
	if(!evalops.parse_options.units_enabled) {
		STATUS_SPACE
		str += "<s>";
		str += _("UNIT");
		str += "</s>";
	}
	if(!evalops.parse_options.variables_enabled) {
		STATUS_SPACE
		str += "<s>";
		str += _("VAR");
		str += "</s>";
	}
	if(!evalops.allow_infinite) {
		STATUS_SPACE
		str += "<s>";
		str += _("INF");
		str += "</s>";
	}
	if(!evalops.allow_complex) {
		STATUS_SPACE
		str += "<s>";
		str += _("CPLX");
		str += "</s>";
	}

	remove_blank_ends(str);
	if(!b) str += " ";
	
	str += "</span>";

	if(str != gtk_label_get_label(GTK_LABEL(statuslabel_r))) {
		gtk_label_set_text(GTK_LABEL(statuslabel_l), "");	
		gtk_label_set_markup(GTK_LABEL(statuslabel_r), str.c_str());	
		display_parse_status();
	}
	
}

bool check_exchange_rates(GtkWidget *win = NULL) {
	if(!CALCULATOR->exchangeRatesUsed()) return false;
	if(auto_update_exchange_rates == 0 && win != NULL) return false;
	if(CALCULATOR->checkExchangeRatesDate(auto_update_exchange_rates > 0 ? auto_update_exchange_rates : 7, false, auto_update_exchange_rates == 0)) return false;
	if(auto_update_exchange_rates == 0) return false;
	bool b = false;
	if(auto_update_exchange_rates < 0) {
		GtkWidget *edialog = gtk_message_dialog_new(win == NULL ? GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")) : GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you wish to update the exchange rates now?"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(edialog), _("It has been %s day(s) since the exchange rates last were updated."), i2s((int) floor(difftime(time(NULL), CALCULATOR->getExchangeRatesTime()) / 86400)).c_str());
		GtkWidget *w = gtk_check_button_new_with_label(_("Do not ask again"));
		gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(edialog))), w);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
		gtk_widget_show(w);
		switch(gtk_dialog_run(GTK_DIALOG(edialog))) {
			case GTK_RESPONSE_YES: {
				b = true;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
					auto_update_exchange_rates = 7;
				}
				break;
			}
			case GTK_RESPONSE_NO: {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
					auto_update_exchange_rates = 0;
				}
				break;
			}
			default: {}
		}
		gtk_widget_destroy(edialog);
	}
	if(b || auto_update_exchange_rates > 0) {
		fetch_exchange_rates(15);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}

/*
	display errors generated under calculation
*/
void display_errors(int *history_index_p = NULL, GtkWidget *win = NULL, int *inhistory_index = NULL, int type = 0) {
	if(!CALCULATOR->message()) return;
	int index = 0;
	MessageType mtype, mtype_highest = MESSAGE_INFORMATION;
	string str = "";
	GtkTreeIter history_iter;
	int inhistory_added = 0;
	while(true) {
		mtype = CALCULATOR->message()->type();
		if(index > 0) {
			if(index == 1) str = "• " + str;
			str += "\n• ";
		}
		str += CALCULATOR->message()->message();
		if(mtype == MESSAGE_ERROR || (mtype_highest != MESSAGE_ERROR && mtype == MESSAGE_WARNING)) {
			mtype_highest = mtype;
		}
		if((mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) && history_index_p && inhistory_index) {
			if(mtype == MESSAGE_ERROR) {
				inhistory.insert(inhistory.begin() + *inhistory_index, CALCULATOR->message()->message());
				inhistory_type.insert(inhistory_type.begin() + *inhistory_index, QALCULATE_HISTORY_ERROR);
				string history_str = "<span foreground=\"";
				history_str += history_error_color;
				history_str += "\">- ";
				history_str += fix_history_string(CALCULATOR->message()->message());
				history_str += "</span>";
				(*history_index_p)++;
				gtk_list_store_insert_with_values(historystore, &history_iter, *history_index_p, 0, history_str.c_str(), 1, *inhistory_index, 3, nr_of_new_expressions, 4, 0, -1);
			} else if(mtype == MESSAGE_WARNING) {
				inhistory.insert(inhistory.begin() + *inhistory_index, CALCULATOR->message()->message());
				inhistory_type.insert(inhistory_type.begin() + *inhistory_index, QALCULATE_HISTORY_WARNING);
				string history_str = "<span foreground=\"";
				history_str += history_warning_color;
				history_str += "\">- ";
				history_str += fix_history_string(CALCULATOR->message()->message());
				history_str += "</span>";
				(*history_index_p)++;
				gtk_list_store_insert_with_values(historystore, &history_iter, *history_index_p, 0, history_str.c_str(), 1, *inhistory_index, 3, nr_of_new_expressions, 4, 0, -1);
			}
			inhistory_added++;
		}
		index++;
		if(!CALCULATOR->nextMessage()) break;
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
	if(!str.empty()) {
		if(type == 1) {
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), str.c_str());
			if(mtype_highest == MESSAGE_ERROR) {
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_tooltip_icon")), "dialog-error", GTK_ICON_SIZE_BUTTON);
			} else if(mtype_highest == MESSAGE_WARNING) {
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_tooltip_icon")), "dialog-warning", GTK_ICON_SIZE_BUTTON);
			} else {
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_tooltip_icon")), "dialog-information", GTK_ICON_SIZE_BUTTON);
			}
			update_expression_icons(EXPRESSION_INFO);
			if(first_error) {
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "message_label")), _("When errors, warnings and other information are generated during calculation the button to the right of the expression entry changes to reflect this. If you hold the pointer over or click the button, the message will be shown."));
				gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_icon")));
				gtk_info_bar_set_message_type(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_MESSAGE_INFO);
				gtk_info_bar_set_show_close_button(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), TRUE);
				gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), TRUE);
				first_error = FALSE;
			}
		} else if(type == 2) {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "message_label")), str.c_str());
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_icon")));
			if(mtype_highest == MESSAGE_ERROR) {
				gtk_info_bar_set_message_type(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_MESSAGE_ERROR);				
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_icon")), "dialog-error-symbolic", GTK_ICON_SIZE_BUTTON);
			} else if(mtype_highest == MESSAGE_WARNING) {
				gtk_info_bar_set_message_type(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_MESSAGE_WARNING);
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_icon")), "dialog-warning-symbolic", GTK_ICON_SIZE_BUTTON);
			} else {
				gtk_info_bar_set_message_type(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_MESSAGE_INFO);
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_icon")), "dialog-information-symbolic", GTK_ICON_SIZE_BUTTON);
			}
			gtk_info_bar_set_show_close_button(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), TRUE);
			/*gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_yes_button")));
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_no_button")));*/
			gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), TRUE);
		} else if(mtype_highest != MESSAGE_INFORMATION) {
			GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(win),GTK_DIALOG_DESTROY_WITH_PARENT, mtype_highest == MESSAGE_ERROR ? GTK_MESSAGE_ERROR : (mtype_highest == MESSAGE_WARNING ? GTK_MESSAGE_WARNING : GTK_MESSAGE_INFO), GTK_BUTTONS_CLOSE, "%s", str.c_str());
			gtk_dialog_run(GTK_DIALOG(edialog));
			gtk_widget_destroy(edialog);
		}

	}
}

gboolean on_display_errors_timeout(gpointer) {
	if(stop_timeouts) return false;
	if(!do_timeout) return true;
	if(CALCULATOR->checkSaveFunctionCalled()) {
		update_vmenu();
	}
	display_errors();
	return true;
}

gboolean on_activate_link(GtkLabel*, gchar *uri, gpointer) {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL)
	return TRUE;
#else
	cout << "A" << endl;
	return FALSE;
#endif
}

gboolean on_check_version_idle(gpointer) {
	string new_version;
#ifdef _WIN32
	int ret = checkAvailableVersion("windows", VERSION, &new_version, 1);
#else
	int ret = checkAvailableVersion("qalculate-gtk", VERSION, &new_version, 1);
#endif
	if(ret > 0 && new_version != last_found_version) {
		last_found_version = new_version;
		GtkWidget *dialog = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Close"), GTK_RESPONSE_REJECT, NULL);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
		GtkWidget *label = gtk_label_new(NULL);
		gchar *gstr = g_strdup_printf(_("A new version of %s is available.\n\nYou can get version %s at %s."), "Qalculate!", new_version.c_str(), "<a href=\"http://qalculate.github.io/downloads.html\">qalculate.github.io</a>");
		gtk_label_set_markup(GTK_LABEL(label), gstr);
		g_free(gstr);
		gtk_container_add(GTK_CONTAINER(hbox), label);
		g_signal_connect(G_OBJECT(label), "activate-link", G_CALLBACK(on_activate_link), NULL);
		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	last_version_check_date.setToCurrentDate();
	return FALSE;
}

void display_function_hint(MathFunction *f, int arg_index = 1) {
	if(!f) return;
	int iargs = f->maxargs();
	Argument *arg;
	Argument default_arg;
	string str, str2, str3;
	const ExpressionName *ename = &f->preferredName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) statuslabel_l);
	bool last_is_vctr = f->getArgumentDefinition(iargs) && f->getArgumentDefinition(iargs)->type() == ARGUMENT_TYPE_VECTOR;
	if(arg_index > iargs && iargs >= 0 && !last_is_vctr) {
		gchar *gstr = g_strdup_printf(_("Too many arguments for %s()."), ename->name.c_str());
		set_status_text(gstr, false, false, true);
		g_free(gstr);
		return;
	}
	str += ename->name;		
	if(iargs < 0) {
		iargs = f->minargs() + 1;
		if(arg_index > iargs) arg_index = iargs;
	}
	if(arg_index > iargs && last_is_vctr) arg_index = iargs;
	str += "(";
	int i_reduced = 0;
	if(iargs != 0) {
		for(int i2 = 1; i2 <= iargs; i2++) {			
			if(i2 > f->minargs() && arg_index < i2) {
				str += "[";
			}
			if(i2 > 1) {
				str += CALCULATOR->getComma();
				str += " ";
			}
			if(i2 == arg_index) str += "<b>";
			arg = f->getArgumentDefinition(i2);
			if(arg && !arg->name().empty()) {
				str2 = arg->name();
			} else {
				str2 = _("argument");
				str2 += " ";
				str2 += i2s(i2);
			}
			if(i2 == arg_index) {
				if(arg) {
					if(i_reduced == 2) str3 = arg->print();
					else str3 = arg->printlong();
				} else {
					Argument arg_default;
					if(i_reduced == 2) str3 = arg_default.print();
					else str3 = arg_default.printlong();
				}
				if(!str3.empty()) {
					str2 += ": ";
					str2 += str3;
				}
				gsub("&", "&amp;", str2);
				gsub(">", "&gt;", str2);
				gsub("<", "&lt;", str2);
				str += str2;					
				str += "</b>";
				if(i_reduced < 2) {
					PangoLayout *layout_test = gtk_widget_create_pango_layout(statuslabel_l, NULL);
					pango_layout_set_markup(layout_test, str.c_str(), -1);
					gint w, h;
					pango_layout_get_pixel_size(layout_test, &w, &h);
					if(w > gtk_widget_get_allocated_width(statuslabel_l) - 20) {
						str = ename->name;	
						str += "(";
						if(i2 != 1) {
							str += "…";
							i_reduced++;
						} else {
							i_reduced = 2;
						}
						i2--;
					}
					g_object_unref(layout_test);	
				} else {
					i_reduced = 0;
				}
			} else {
				gsub("&", "&amp;", str2);
				gsub(">", "&gt;", str2);
				gsub("<", "&lt;", str2);
				str += str2;
				if(i2 > f->minargs() && arg_index < i2) {
					str += "]";
				}
			}
		}
		if(f->maxargs() < 0) {
			str += CALCULATOR->getComma();
			str += " …";
		}
	}
	str += ")";
	set_status_text(str);
}

void display_parse_status() {
	if(!display_expression_status) return;
	if(block_display_parse) return;
	GtkTextIter istart, iend, ipos;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	string text = gtext;
	g_free(gtext);
	if(text.empty()) {
		set_status_text("", true, false, false);
		parsed_expression = "";
		expression_has_changed2 = false;
		return;
	}
	GtkTextMark *mark = gtk_text_buffer_get_insert(expressionbuffer);
	if(mark) gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mark);
	else ipos = iend;
	MathStructure mparse, mfunc;
	bool full_parsed = false;
	string str_e, str_u;
	bool had_errors = false, had_warnings = false;
	evalops.parse_options.preserve_format = true;
	if(!gtk_text_iter_is_start(&ipos)) {
		evalops.parse_options.unended_function = &mfunc;
		CALCULATOR->beginTemporaryStopMessages();
		if(!gtk_text_iter_is_end(&ipos)) {
			gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &ipos, FALSE);
			str_e = CALCULATOR->unlocalizeExpression(gtext, evalops.parse_options);
			if(!CALCULATOR->separateToExpression(str_e, str_u, evalops)) {
				CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
			}
			g_free(gtext);
		} else {
			str_e = CALCULATOR->unlocalizeExpression(text, evalops.parse_options);
			CALCULATOR->separateToExpression(str_e, str_u, evalops);
			CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
			full_parsed = true;
		}
		int warnings_count;
		had_errors = CALCULATOR->endTemporaryStopMessages(NULL, &warnings_count) > 0;
		had_warnings = warnings_count > 0;
		evalops.parse_options.unended_function = NULL;
	}
	if(mfunc.isFunction()) {
		if(mfunc.countChildren() == 0) {
			display_function_hint(mfunc.function(), 1);
		} else {
			display_function_hint(mfunc.function(), mfunc.countChildren());
		}
	}
	if(expression_has_changed2) {
		if(!full_parsed) {
			CALCULATOR->beginTemporaryStopMessages();
			str_e = CALCULATOR->unlocalizeExpression(text, evalops.parse_options);
			CALCULATOR->separateToExpression(str_e, str_u, evalops);			
			CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
			int warnings_count;
			had_errors = CALCULATOR->endTemporaryStopMessages(NULL, &warnings_count) > 0;
			had_warnings = warnings_count > 0;
		}
		PrintOptions po;
		po.preserve_format = true;
		po.show_ending_zeroes = true;
		po.lower_case_e = printops.lower_case_e;
		po.lower_case_numbers = printops.lower_case_numbers;
		po.base_display = printops.base_display;
		po.abbreviate_names = false;
		po.hide_underscore_spaces = true;
		po.use_unicode_signs = printops.use_unicode_signs;
		po.multiplication_sign = printops.multiplication_sign;
		po.division_sign = printops.division_sign;
		po.short_multiplication = false;
		po.excessive_parenthesis = true;
		po.improve_division_multipliers = false;
		po.can_display_unicode_string_function = &can_display_unicode_string_function;
		po.can_display_unicode_string_arg = (void*) statuslabel_l;
		po.spell_out_logical_operators = printops.spell_out_logical_operators;
		po.restrict_to_parent_precision = false;
		mparse.format(po);
		parsed_expression = mparse.print(po);
		if(!str_u.empty()) {
			parsed_expression += CALCULATOR->localToString();
			if(equalsIgnoreCase(str_u, "hex") || equalsIgnoreCase(str_u, "hexadecimal") || equalsIgnoreCase(str_u, _("hexadecimal"))) {
				parsed_expression += _("hexadecimal number");
			} else if(equalsIgnoreCase(str_u, "oct") || equalsIgnoreCase(str_u, "octal") || equalsIgnoreCase(str_u, _("octal"))) {
				parsed_expression += _("octal number");
			} else if(equalsIgnoreCase(str_u, "bin") || equalsIgnoreCase(str_u, "binary") || equalsIgnoreCase(str_u, _("binary"))) {
				parsed_expression += _("binary number");
			} else if(equalsIgnoreCase(str_u, "bases") || equalsIgnoreCase(str_u, _("bases"))) {
				parsed_expression += _("number bases");
			} else if(equalsIgnoreCase(str_u, "optimal") || equalsIgnoreCase(str_u, _("optimal"))) {
				parsed_expression += _("optimal unit");
			} else if(equalsIgnoreCase(str_u, "base") || equalsIgnoreCase(str_u, _("base"))) {
				parsed_expression += _("base units");
			} else if(equalsIgnoreCase(str_u, "mixed") || equalsIgnoreCase(str_u, _("mixed"))) {
				parsed_expression += _("mixed units");
			} else if(equalsIgnoreCase(str_u, "fraction") || equalsIgnoreCase(str_u, _("fraction"))) {
				parsed_expression += _("fraction");
			} else if(equalsIgnoreCase(str_u, "factors") || equalsIgnoreCase(str_u, _("factors"))) {
				parsed_expression += _("factors");
			} else {			
				CALCULATOR->beginTemporaryStopMessages();
				CompositeUnit cu("", "temporary_composite_parse", "", str_u);
				int warnings_count;
				had_errors = CALCULATOR->endTemporaryStopMessages(NULL, &warnings_count) > 0 || had_errors;
				had_warnings = had_warnings || warnings_count > 0;
				mparse = cu.generateMathStructure(!printops.negative_exponents);
				mparse.format(po);
				parsed_expression += mparse.print(po);
			}
		}
		parsed_had_errors = had_errors; parsed_had_warnings = had_warnings;
		gsub("&", "&amp;", parsed_expression);
		gsub(">", "&gt;", parsed_expression);
		gsub("<", "&lt;", parsed_expression);
		if(!mfunc.isFunction()) set_status_text(parsed_expression.c_str(), true, had_errors, had_warnings);
		expression_has_changed2 = false;
	} else if(!mfunc.isFunction()) {
		set_status_text(parsed_expression.c_str(), true, parsed_had_errors, parsed_had_warnings);
	}
	evalops.parse_options.preserve_format = false;
}


void highlight_parentheses() {
	GtkTextMark *mcur = gtk_text_buffer_get_insert(expressionbuffer);
	if(!mcur) return;
	GtkTextIter icur, istart, iend;
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &icur, mcur);
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gtk_text_buffer_remove_tag(expressionbuffer, expression_par_tag, &istart, &iend);
	bool b = false;
	b = (gtk_text_iter_get_char(&icur) == ')');
	if(!b && gtk_text_iter_backward_char(&icur)) {
		b = (gtk_text_iter_get_char(&icur) == ')');
		if(!b) gtk_text_iter_forward_char(&icur);
	}
	if(b) {
		GtkTextIter ipar2 = icur;
		int pars = 1;
		while(gtk_text_iter_backward_char(&ipar2)) {
			if(gtk_text_iter_get_char(&ipar2) == ')') {
				pars++;
			} else if(gtk_text_iter_get_char(&ipar2) == '(') {
				pars--;
				if(pars == 0) break;
			}
		}
		if(pars == 0) {
			GtkTextIter inext = icur;
			gtk_text_iter_forward_char(&inext);
			gtk_text_buffer_apply_tag(expressionbuffer, expression_par_tag, &icur, &inext);
			inext = ipar2;
			gtk_text_iter_forward_char(&inext);
			gtk_text_buffer_apply_tag(expressionbuffer, expression_par_tag, &ipar2, &inext);
		}
	} else {
		b = (gtk_text_iter_get_char(&icur) == '(');
		if(!b && gtk_text_iter_backward_char(&icur)) {
			b = (gtk_text_iter_get_char(&icur) == '(');
			if(!b) gtk_text_iter_forward_char(&icur);
		}
		if(b) {
			GtkTextIter ipar2 = icur;
			int pars = 1;
			while(gtk_text_iter_forward_char(&ipar2)) {
				if(gtk_text_iter_get_char(&ipar2) == '(') {
					pars++;
				} else if(gtk_text_iter_get_char(&ipar2) == ')') {
					pars--;
					if(pars == 0) break;
				}
			}
			if(pars == 0) {
				GtkTextIter inext = icur;
				gtk_text_iter_forward_char(&inext);
				gtk_text_buffer_apply_tag(expressionbuffer, expression_par_tag, &icur, &inext);
				inext = ipar2;
				gtk_text_iter_forward_char(&inext);
				gtk_text_buffer_apply_tag(expressionbuffer, expression_par_tag, &ipar2, &inext);
			}
		}
	}
}

void on_expressionbuffer_cursor_position_notify() {
	if(expression_has_changed_pos) {
		expression_has_changed_pos = false;
		return;
	}
	gtk_widget_hide(completion_window);
	if(!check_expression_position) return;
	highlight_parentheses();
	display_parse_status();
}

/*
	set focus on expression entry without losing selection
*/
void focus_keeping_selection() {
	if(gtk_widget_is_focus(expressiontext)) return;
	gtk_widget_grab_focus(expressiontext);
}

MathFunction *get_selected_function() {
	return selected_function;
}

MathFunction *get_edited_function() {
	return edited_function;
}
Unit *get_edited_unit() {
	return edited_unit;
}
DataSet *get_edited_dataset() {
	return edited_dataset;
}
DataProperty *get_edited_dataproperty() {
	return edited_dataproperty;
}
KnownVariable *get_edited_variable() {
	return edited_variable;
}
UnknownVariable *get_edited_unknown() {
	return edited_unknown;
}
KnownVariable *get_edited_matrix() {
	return edited_matrix;
}

Argument *get_edited_argument() {
	return edited_argument;
}
Argument *get_selected_argument() {
	return selected_argument;
}
size_t get_selected_subfunction() {
	return selected_subfunction;
}

Variable *get_selected_variable() {
	return selected_variable;
}

Unit *get_selected_unit() {
	return selected_unit;
}

Unit *get_selected_to_unit() {
	return selected_to_unit;
}

void generate_units_tree_struct() {
	size_t cat_i, cat_i_prev; 
	bool b;	
	string str, cat, cat_sub;
	Unit *u = NULL;
	unit_cats.items.clear();
	unit_cats.objects.clear();
	unit_cats.parent = NULL;	
	ia_units.clear();
	list<tree_struct>::iterator it;	
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(!CALCULATOR->units[i]->isActive()) {
			b = false;
			for(size_t i3 = 0; i3 < ia_units.size(); i3++) {
				u = (Unit*) ia_units[i3];
				if(CALCULATOR->units[i]->title() < u->title()) {
					b = true;
					ia_units.insert(ia_units.begin() + i3, (void*) CALCULATOR->units[i]);
					break;
				}
			}
			if(!b) ia_units.push_back((void*) CALCULATOR->units[i]);						
		} else {
			tree_struct *item = &unit_cats;
			if(!CALCULATOR->units[i]->category().empty()) {
				cat = CALCULATOR->units[i]->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;		
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						it->parent = item;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			b = false;
			for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
				u = (Unit*) item->objects[i3];
				if(CALCULATOR->units[i]->title() < u->title()) {
					b = true;
					item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->units[i]);
					break;
				}
			}
			if(!b) item->objects.push_back((void*) CALCULATOR->units[i]);		
		}
	}
	
	unit_cats.sort();

}
void generate_variables_tree_struct() {

	size_t cat_i, cat_i_prev; 
	bool b;	
	string str, cat, cat_sub;
	Variable *v = NULL;
	variable_cats.items.clear();
	variable_cats.objects.clear();
	variable_cats.parent = NULL;
	ia_variables.clear();
	list<tree_struct>::iterator it;	
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->isActive()) {
			//deactivated variable
			b = false;
			for(size_t i3 = 0; i3 < ia_variables.size(); i3++) {
				v = (Variable*) ia_variables[i3];
				if(CALCULATOR->variables[i]->title() < v->title()) {
					b = true;
					ia_variables.insert(ia_variables.begin() + i3, (void*) CALCULATOR->variables[i]);
					break;
				}
			}
			if(!b) ia_variables.push_back((void*) CALCULATOR->variables[i]);											
		} else {
			tree_struct *item = &variable_cats;
			if(!CALCULATOR->variables[i]->category().empty()) {
				cat = CALCULATOR->variables[i]->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;		
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						it->parent = item;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			b = false;
			for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
				v = (Variable*) item->objects[i3];
				if(CALCULATOR->variables[i]->title() < v->title()) {
					b = true;
					item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->variables[i]);
					break;
				}
			}
			if(!b) item->objects.push_back((void*) CALCULATOR->variables[i]);		
		}
	}
	
	variable_cats.sort();

}
void generate_functions_tree_struct() {

	size_t cat_i, cat_i_prev; 
	bool b;	
	string str, cat, cat_sub;
	MathFunction *f = NULL;
	function_cats.items.clear();
	function_cats.objects.clear();
	function_cats.parent = NULL;
	ia_functions.clear();
	list<tree_struct>::iterator it;

	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(!CALCULATOR->functions[i]->isActive()) {
			//deactivated function
			b = false;
			for(size_t i3 = 0; i3 < ia_functions.size(); i3++) {
				f = (MathFunction*) ia_functions[i3];
				if(CALCULATOR->functions[i]->title() < f->title()) {
					b = true;
					ia_functions.insert(ia_functions.begin() + i3, (void*) CALCULATOR->functions[i]);
					break;
				}
			}
			if(!b) ia_functions.push_back((void*) CALCULATOR->functions[i]);								
		} else {
			tree_struct *item = &function_cats;
			if(!CALCULATOR->functions[i]->category().empty()) {
				cat = CALCULATOR->functions[i]->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;		
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						it->parent = item;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			b = false;
			for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
				f = (MathFunction*) item->objects[i3];
				if(CALCULATOR->functions[i]->title() < f->title()) {
					b = true;
					item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->functions[i]);
					break;
				}
			}
			if(!b) item->objects.push_back((void*) CALCULATOR->functions[i]);
		}
	}
	
	function_cats.sort();
	
}

/*
	generate the function categories tree in manage functions dialog
*/
void update_functions_tree() {
	if(!functions_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tFunctionCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctionCategories_selection_changed, NULL);
	gtk_tree_store_clear(tFunctionCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctionCategories_selection_changed, NULL);
	gtk_tree_store_append(tFunctionCategories_store, &iter3, NULL);
	gtk_tree_store_set(tFunctionCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	function_cats.it = function_cats.items.begin();
	if(function_cats.it != function_cats.items.end()) {
		item = &*function_cats.it;
		++function_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	while(item) {
		gtk_tree_store_append(tFunctionCategories_store, &iter, &iter2);
		str += "/";
		str += item->item;
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_function_category) {
			EXPAND_TO_ITER(model, tFunctionCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
		while(item && item->it == item->items.end()) {
			size_t str_i = str.rfind("/");
			if(str_i == string::npos) {
				str = "";
			} else {
				str = str.substr(0, str_i);
			}
			item = item->parent;
			gtk_tree_model_iter_parent(model, &iter2, &iter);
			iter = iter2;
		}
		if(item) {
			item2 = &*item->it;
			if(item->it == item->items.begin()) iter2 = iter;
			++item->it;
			item = item2;
			item->it = item->items.begin();	
		}
	}
	if(!function_cats.objects.empty()) {
		//add "Uncategorized" category if there are functions without category
		gtk_tree_store_append(tFunctionCategories_store, &iter, &iter3);
		EXPAND_TO_ITER(model, tFunctionCategories, iter)
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		if(selected_function_category == _("Uncategorized")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
	}	
	if(!ia_functions.empty()) {
		//add "Inactive" category if there are inactive functions
		gtk_tree_store_append(tFunctionCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tFunctionCategories, iter)
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, _("Inactive"), 1, _("Inactive"), -1);
		if(selected_function_category == _("Inactive")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
	}		
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_function_category = _("All");
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionCategories_store), &iter);
		EXPAND_ITER(model, tFunctionCategories, iter)
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
	}
}

void setFunctionTreeItem(GtkTreeIter &iter2, MathFunction *f) {
	gtk_list_store_append(tFunctions_store, &iter2);
	gtk_list_store_set(tFunctions_store, &iter2, 0, f->title(true).c_str(), 1, (gpointer) f, -1);
	if(f == selected_function) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &iter2);
	}
}

/*
	generate the function tree in manage functions dialog when category selection has changed
*/
void on_tFunctionCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	bool no_cat = false, b_all = false, b_inactive = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctions_selection_changed, NULL);
	gtk_list_store_clear(tFunctions_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctions_selection_changed, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_edit")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_insert")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_delete")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_deactivate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_apply")), FALSE);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_function_category = gstr;
		if(selected_function_category == _("All")) {
			b_all = true;
		} else if(selected_function_category == _("Uncategorized")) {
			no_cat = true;
		} else if(selected_function_category == _("Inactive")) {
			b_inactive = true;
		}
		if(!b_all && !no_cat && !b_inactive && selected_function_category[0] == '/') {
			string str = selected_function_category.substr(1, selected_function_category.length() - 1);
			for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
				if(CALCULATOR->functions[i]->isActive() && CALCULATOR->functions[i]->category().substr(0, selected_function_category.length() - 1) == str) {
					setFunctionTreeItem(iter2, CALCULATOR->functions[i]);
				}
			}
		} else {			
			for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
				if((b_inactive && !CALCULATOR->functions[i]->isActive()) || (CALCULATOR->functions[i]->isActive() && (b_all || (no_cat && CALCULATOR->functions[i]->category().empty()) || (!b_inactive && CALCULATOR->functions[i]->category() == selected_function_category)))) {
					setFunctionTreeItem(iter2, CALCULATOR->functions[i]);
				}
			}
		}
		if(!selected_function || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &model2, &iter2)) {
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctions_store), &iter2);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &iter2);
		}
		g_free(gstr);
	} else {
		selected_function_category = "";
	}
}

/*
	function selection has changed
*/
void on_tFunctions_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		MathFunction *f;
		gtk_tree_model_get(model, &iter, 1, &f, -1);
		//remember the new selection
		selected_function = f;
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			if(CALCULATOR->functions[i] == selected_function) {
				GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functions_builder, "functions_textview_description")));
				gtk_text_buffer_set_text(buffer, "", -1);
				GtkTextIter iter;
				f = CALCULATOR->functions[i];
				Argument *arg;
				Argument default_arg;
				string str, str2;
				const ExpressionName *ename = &f->preferredName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(functions_builder, "functions_textview_description"));
				str += ename->name;
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", "italic", NULL);
				str = "";
				int iargs = f->maxargs();
				if(iargs < 0) {
					iargs = f->minargs() + 1;
				}
				str += "(";				
				if(iargs != 0) {
					for(int i2 = 1; i2 <= iargs; i2++) {	
						if(i2 > f->minargs()) {
							str += "[";
						}
						if(i2 > 1) {
							str += CALCULATOR->getComma();
							str += " ";
						}
						arg = f->getArgumentDefinition(i2);
						if(arg && !arg->name().empty()) {
							str2 = arg->name();
						} else {
							str2 = _("argument");
							str2 += " ";
							str2 += i2s(i2);
						}
						str += str2;
						if(i2 > f->minargs()) {
							str += "]";
						}
					}
					if(f->maxargs() < 0) {
						str += CALCULATOR->getComma();
						str += " …";
					}
				}
				str += ")";
				for(size_t i2 = 1; i2 <= f->countNames(); i2++) {
					if(&f->getName(i2) != ename) {
						str += "\n";
						str += f->getName(i2).name;
					}
				}
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
				str = "";
				str += "\n";
				if(f->subtype() == SUBTYPE_DATA_SET) {
					str += "\n";
					gchar *gstr = g_strdup_printf(_("Retrieves data from the %s data set for a given object and property. If \"info\" is typed as property, a dialog window will pop up with all properties of the object."), f->title().c_str());
					str += gstr;
					g_free(gstr);
					str += "\n";
				}
				if(!f->description().empty()) {
					str += "\n";
					str += f->description();
					str += "\n";
				}
				if(!f->example(true).empty()) {
					str += "\n";
					str += _("Example:");
					str += " ";
					str += f->example(false, ename->name);
					str += "\n";
				}
				if(f->subtype() == SUBTYPE_DATA_SET && !((DataSet*) f)->copyright().empty()) {
					str += "\n";
					str += ((DataSet*) f)->copyright();
					str += "\n";
				}
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
				if(iargs) {
					str = "\n";
					str += _("Arguments");
					str += "\n";
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
					for(int i2 = 1; i2 <= iargs; i2++) {	
						arg = f->getArgumentDefinition(i2);
						if(arg && !arg->name().empty()) {
							str = arg->name();
						} else {
							str = i2s(i2);	
						}
						str += ": ";
						if(arg) {
							str2 = arg->printlong();
						} else {
							str2 = default_arg.printlong();
						}
						if(i2 > f->minargs()) {
							str2 += " (";
							//optional argument
							str2 += _("optional");
							if(!f->getDefaultValue(i2).empty()) {
								str2 += ", ";
								//argument default, in description
								str2 += _("default: ");
								str2 += f->getDefaultValue(i2);
							}
							str2 += ")";
						}
						str2 += "\n";
						gtk_text_buffer_get_end_iter(buffer, &iter);
						gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
						gtk_text_buffer_get_end_iter(buffer, &iter);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str2.c_str(), -1, "italic", NULL);
					}
				}
				if(!f->condition().empty()) {
					str = "\n";
					str += _("Requirement");
					str += ": ";
					str += f->printCondition();
					str += "\n";
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
				}
				if(f->subtype() == SUBTYPE_DATA_SET) {
					DataSet *ds = (DataSet*) f;
					str = "\n";
					str += _("Properties");
					str += "\n";
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
					DataPropertyIter it;
					DataProperty *dp = ds->getFirstProperty(&it);
					while(dp) {	
						if(!dp->isHidden()) {
							if(!dp->title(false).empty()) {
								str = dp->title();	
								str += ": ";
							}
							for(size_t i = 1; i <= dp->countNames(); i++) {
								if(i > 1) str += ", ";
								str += dp->getName(i);
							}
							if(dp->isKey()) {
								str += " (";
								//indicating that the property is a data set key
								str += _("key");
								str += ")";
							}
							str += "\n";
							gtk_text_buffer_get_end_iter(buffer, &iter);
							gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
							if(!dp->description().empty()) {
								str = dp->description();
								str += "\n";
								gtk_text_buffer_get_end_iter(buffer, &iter);
								gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
							}
						}
						dp = ds->getNextProperty(&it);
					}
				}
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_edit")), !CALCULATOR->functions[i]->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_deactivate")), TRUE);
				if(CALCULATOR->functions[i]->isActive()) {
					gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(functions_builder, "functions_buttonlabel_deactivate")), _("Deacti_vate"));
				} else {
					gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(functions_builder, "functions_buttonlabel_deactivate")), _("Acti_vate"));
				}
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_insert")), CALCULATOR->functions[i]->isActive());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_apply")), CALCULATOR->functions[i]->isActive() && (CALCULATOR->functions[i]->minargs() <= 1 || rpn_mode));
				//user cannot delete global definitions
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_delete")), CALCULATOR->functions[i]->isLocal());
			}
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_edit")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_insert")), FALSE);		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_delete")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_deactivate")), FALSE);
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functions_builder, "functions_textview_description"))), "", -1);
		selected_function = NULL;
	}
}

/*
	generate the variable categories tree in manage variables dialog
*/
void update_variables_tree() {
	if(!variables_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tVariableCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariableCategories_selection_changed, NULL);
	gtk_tree_store_clear(tVariableCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariableCategories_selection_changed, NULL);
	gtk_tree_store_append(tVariableCategories_store, &iter3, NULL);
	gtk_tree_store_set(tVariableCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	variable_cats.it = variable_cats.items.begin();
	if(variable_cats.it != variable_cats.items.end()) {
		item = &*variable_cats.it;
		++variable_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	while(item) {
		gtk_tree_store_append(tVariableCategories_store, &iter, &iter2);
		str += "/";
		str += item->item;
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_variable_category) {
			EXPAND_TO_ITER(model, tVariableCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}

		while(item && item->it == item->items.end()) {
			size_t str_i = str.rfind("/");
			if(str_i == string::npos) {
				str = "";
			} else {
				str = str.substr(0, str_i);
			}
			item = item->parent;
			gtk_tree_model_iter_parent(model, &iter2, &iter);
			iter = iter2;
		}
		if(item) {
			item2 = &*item->it;
			if(item->it == item->items.begin()) iter2 = iter;
			++item->it;
			item = item2;
			item->it = item->items.begin();	
		}
	}

	if(!variable_cats.objects.empty()) {	
		//add "Uncategorized" category if there are variables without category
		gtk_tree_store_append(tVariableCategories_store, &iter, &iter3);
		EXPAND_TO_ITER(model, tVariableCategories, iter)
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		if(selected_variable_category == _("Uncategorized")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}
	}
	if(!ia_variables.empty()) {
		//add "Inactive" category if there are inactive variables
		gtk_tree_store_append(tVariableCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tVariableCategories, iter)
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, _("Inactive"), 1, _("Inactive"), -1);
		if(selected_variable_category == _("Inactive")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}
	}	
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_variable_category = _("All");
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariableCategories_store), &iter);
		EXPAND_ITER(model, tVariableCategories, iter)
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
	}
}

void setVariableTreeItem(GtkTreeIter &iter2, Variable *v) {
	gtk_list_store_append(tVariables_store, &iter2);
	string value = "";
	if(is_answer_variable(v)) {
		value = _("a previous result");
	} else if(v->isKnown()) {
		if(((KnownVariable*) v)->isExpression()) {
			value = CALCULATOR->localizeExpression(((KnownVariable*) v)->expression());
		} else {
			if(((KnownVariable*) v)->get().isMatrix()) {
				value = _("matrix");
			} else if(((KnownVariable*) v)->get().isVector()) {
				value = _("vector");
			} else {
				value = CALCULATOR->print(((KnownVariable*) v)->get(), 30);
			}
		}
	} else {
		if(((UnknownVariable*) v)->assumptions()) {
			switch(((UnknownVariable*) v)->assumptions()->sign()) {
				case ASSUMPTION_SIGN_POSITIVE: {value = _("positive"); break;}
				case ASSUMPTION_SIGN_NONPOSITIVE: {value = _("non-positive"); break;}
				case ASSUMPTION_SIGN_NEGATIVE: {value = _("negative"); break;}
				case ASSUMPTION_SIGN_NONNEGATIVE: {value = _("non-negative"); break;}
				case ASSUMPTION_SIGN_NONZERO: {value = _("non-zero"); break;}
				default: {}
			}
			if(!value.empty() && ((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_NONE) value += " ";
			switch(((UnknownVariable*) v)->assumptions()->type()) {
				case ASSUMPTION_TYPE_INTEGER: {value += _("integer"); break;}
				case ASSUMPTION_TYPE_RATIONAL: {value += _("rational"); break;}
				case ASSUMPTION_TYPE_REAL: {value += _("real"); break;}
				case ASSUMPTION_TYPE_COMPLEX: {value += _("complex"); break;}
				case ASSUMPTION_TYPE_NUMBER: {value += _("number"); break;}
				case ASSUMPTION_TYPE_NONMATRIX: {value += _("(not matrix)"); break;}
				default: {}
			}
			if(value.empty()) value = _("unknown");
		} else {
			value = _("default assumptions");
		}		
	}
	gtk_list_store_set(tVariables_store, &iter2, 0, v->title(true).c_str(), 1, value.c_str(), 2, (gpointer) v, -1);
	if(v == selected_variable) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &iter2);
	}
}

/*
	generate the variable tree in manage variables dialog when category selection has changed
*/
void on_tVariableCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	bool no_cat = false, b_all = false, b_inactive = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariables_selection_changed, NULL);
	gtk_list_store_clear(tVariables_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariables_selection_changed, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_edit")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_insert")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_delete")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_deactivate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_export")), FALSE);

	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_variable_category = gstr;		
		if(selected_variable_category == _("All")) {
			b_all = true;
		} else if(selected_variable_category == _("Uncategorized")) {
			no_cat = true;
		} else if(selected_variable_category == _("Inactive")) {
			b_inactive = true;
		}

		if(!b_all && !no_cat && !b_inactive && selected_variable_category[0] == '/') {
			string str = selected_variable_category.substr(1, selected_variable_category.length() - 1);
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				if(CALCULATOR->variables[i]->isActive() && CALCULATOR->variables[i]->category().substr(0, selected_variable_category.length() - 1) == str) {
					setVariableTreeItem(iter2, CALCULATOR->variables[i]);
				}
			}			
		} else {			
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				if((b_inactive && !CALCULATOR->variables[i]->isActive()) || (CALCULATOR->variables[i]->isActive() && (b_all || (no_cat && CALCULATOR->variables[i]->category().empty()) || (!b_inactive && CALCULATOR->variables[i]->category() == selected_variable_category)))) {
					setVariableTreeItem(iter2, CALCULATOR->variables[i]);
				}
			}
		}

		if(!selected_variable || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &model2, &iter2)) {
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariables_store), &iter2);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &iter2);
		}
		g_free(gstr);

	} else {
		selected_variable_category = "";
	}

}

/*
	variable selection has changed
*/
void on_tVariables_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Variable *v;
		gtk_tree_model_get(model, &iter, 2, &v, -1);
		if(!CALCULATOR->stillHasVariable(v)) {
			show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
			selected_variable = NULL;
			update_vmenu();
			return;
		}
		//remember selection
		selected_variable = v;
		for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			if(CALCULATOR->variables[i] == selected_variable) {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_edit")), !CALCULATOR->variables[i]->isBuiltin() && !is_answer_variable(CALCULATOR->variables[i]));
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_insert")), CALCULATOR->variables[i]->isActive());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_deactivate")), !is_answer_variable(CALCULATOR->variables[i]));
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_export")), CALCULATOR->variables[i]->isKnown());
				if(CALCULATOR->variables[i]->isActive()) {
					gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(variables_builder, "variables_buttonlabel_deactivate")), _("Deacti_vate"));
				} else {
					gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(variables_builder, "variables_buttonlabel_deactivate")), _("Acti_vate"));
				}
				//user cannot delete global definitions
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_delete")), CALCULATOR->variables[i]->isLocal() && !is_answer_variable(CALCULATOR->variables[i]) && CALCULATOR->variables[i] != CALCULATOR->v_x && CALCULATOR->variables[i] != CALCULATOR->v_y && CALCULATOR->variables[i] != CALCULATOR->v_z);
			}
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_edit")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_insert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_delete")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_deactivate")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_export")), FALSE);
		selected_variable = NULL;
	}
}


/*
	generate the unit categories tree in manage units dialog
*/
void update_units_tree() {
	if(!units_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitCategories_selection_changed, NULL);
	gtk_tree_store_clear(tUnitCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitCategories_selection_changed, NULL);
	gtk_tree_store_append(tUnitCategories_store, &iter3, NULL);
	gtk_tree_store_set(tUnitCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	unit_cats.it = unit_cats.items.begin();
	if(unit_cats.it != unit_cats.items.end()) {
		item = &*unit_cats.it;
		++unit_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	while(item) {
		gtk_tree_store_append(tUnitCategories_store, &iter, &iter2);
		str += "/";
		str += item->item;
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_unit_category) {
			EXPAND_TO_ITER(model, tUnitCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}
		while(item && item->it == item->items.end()) {
			size_t str_i = str.rfind("/");
			if(str_i == string::npos) {
				str = "";
			} else {
				str = str.substr(0, str_i);
			}
			item = item->parent;
			gtk_tree_model_iter_parent(model, &iter2, &iter);
			iter = iter2;
		}
		if(item) {
			item2 = &*item->it;
			if(item->it == item->items.begin()) iter2 = iter;
			++item->it;
			item = item2;
			item->it = item->items.begin();	
		}
	}
	if(!unit_cats.objects.empty()) {	
		//add "Uncategorized" category if there are units without category
		gtk_tree_store_append(tUnitCategories_store, &iter, &iter3);
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		if(selected_unit_category == _("Uncategorized")) {
			EXPAND_TO_ITER(model, tUnitCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}
	}	
	if(!ia_units.empty()) {
		gtk_tree_store_append(tUnitCategories_store, &iter, NULL);
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, _("Inactive"), 1, _("Inactive"), -1);
		if(selected_unit_category == _("Inactive")) {
			EXPAND_TO_ITER(model, tUnitCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}	
	}
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_unit_category = _("All");
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter);
		EXPAND_ITER(model, tUnitCategories, iter)
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
	}
}

void setUnitTreeItem(GtkTreeIter &iter2, Unit *u) {
	gtk_list_store_append(tUnits_store, &iter2);
	string snames, sbase;
	//display name, plural name and short name in the second column
	AliasUnit *au;
	for(size_t i = 1; i <= u->countNames(); i++) {
		if(i > 1) snames += " / ";
		snames += u->getName(i).name;
	}
	//depending on unit type display relation to base unit(s)
	switch(u->subtype()) {
		case SUBTYPE_COMPOSITE_UNIT: {
			snames = "";
			sbase = ((CompositeUnit*) u)->print(false, true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tUnits);
			break;
		}
		case SUBTYPE_ALIAS_UNIT: {
			au = (AliasUnit*) u;
			sbase = au->firstBaseUnit()->preferredDisplayName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) tUnits).name;
			if(au->firstBaseExponent() != 1) {
				sbase += POWER;
				sbase += i2s(au->firstBaseExponent());
			}
			break;
		}
		case SUBTYPE_BASE_UNIT: {
			sbase = "";
			break;
		}
	}
	//display descriptive name (title), or name if no title defined
	gtk_list_store_set(tUnits_store, &iter2, UNITS_TITLE_COLUMN, u->title(true).c_str(), UNITS_NAMES_COLUMN, snames.c_str(), UNITS_BASE_COLUMN, sbase.c_str(), UNITS_POINTER_COLUMN, (gpointer) u, -1);
	if(u == selected_unit) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &iter2);
	}
}

/*
	generate the unit tree and units conversion menu in manage units dialog when category selection has changed
*/
void on_tUnitCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	//make sure that no unit conversion is done in the dialog until everthing is updated
	block_unit_convert = true;
	bool no_cat = false, b_all = false, b_inactive = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnits_selection_changed, NULL);
	gtk_list_store_clear(tUnits_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnits_selection_changed, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_edit")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_insert")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_delete")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_deactivate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_convert_to")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_frame_convert")), FALSE);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_unit_category = gstr;
		if(selected_unit_category == _("All")) {
			b_all = true;
		} else if(selected_unit_category == _("Uncategorized")) {
			no_cat = true;
		} else if(selected_unit_category == _("Inactive")) {
			b_inactive = true;
		}
		if(!b_all && !no_cat && !b_inactive && selected_unit_category[0] == '/') {
			string str = selected_unit_category.substr(1, selected_unit_category.length() - 1);
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {	
				if(CALCULATOR->units[i]->isActive() && CALCULATOR->units[i]->category().substr(0, selected_unit_category.length() - 1) == str) {
					setUnitTreeItem(iter2, CALCULATOR->units[i]);
				}
			}
		} else {
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if((b_inactive && !CALCULATOR->units[i]->isActive()) || (CALCULATOR->units[i]->isActive() && (b_all || (no_cat && CALCULATOR->units[i]->category().empty()) || (!b_inactive && CALCULATOR->units[i]->category() == selected_unit_category)))) {
					setUnitTreeItem(iter2, CALCULATOR->units[i]);			
				}
			}
		}
		if(!selected_unit || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &model2, &iter2)) {
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store), &iter2);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &iter2);
		}
		g_free(gstr);
	} else {
		selected_unit_category = "";
	}
	int i = 0, h = -1;
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(units_builder, "units_combobox_to_unit")));
	//add all units in units tree to menu
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store), &iter2);
	Unit *u;
	while(b) {
		gchar *gstr;
		gtk_tree_model_get(GTK_TREE_MODEL(tUnits_store), &iter2, UNITS_TITLE_COLUMN, &gstr, UNITS_POINTER_COLUMN, &u, -1);
		if(!selected_to_unit) {
			selected_to_unit = u;	
		}
		if(u) {
			
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(units_builder, "units_combobox_to_unit")), gstr);
			if(selected_to_unit == u) {
				h = i;
			}
		}
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnits_store), &iter2);
		i++;
		g_free(gstr);
	}
	//if no items were added to the menu, reset selected unit
	if(i == 0)
		selected_to_unit = NULL;
	else {
		//if no menu item was selected, select the first
		if(h < 0) {
			h = 0;
			b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store), &iter2);
			if(b) {
				gtk_tree_model_get(GTK_TREE_MODEL(tUnits_store), &iter2, UNITS_POINTER_COLUMN, &u, -1);
				selected_to_unit = u;
			}
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(units_builder, "units_combobox_to_unit")), h);
	}

	block_unit_convert = false;		
	//update conversion display
	convert_in_wUnits();
}

/*
	unit selection has changed
*/
void on_tUnits_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Unit *u;
		gtk_tree_model_get(model, &iter, UNITS_POINTER_COLUMN, &u, -1);
		selected_unit = u;
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i] == selected_unit) {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_frame_convert")), TRUE);				
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_from_unit")), CALCULATOR->units[i]->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) gtk_builder_get_object(units_builder, "units_label_from_unit")).c_str());
				//user cannot delete global definitions
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_delete")), CALCULATOR->units[i]->isLocal());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_convert_to")), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_insert")), CALCULATOR->units[i]->isActive());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_edit")), !CALCULATOR->units[i]->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_deactivate")), TRUE);
				if(CALCULATOR->units[i]->isActive()) {
					gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(units_builder, "units_buttonlabel_deactivate")), _("Deacti_vate"));
				} else {
					gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(units_builder, "units_buttonlabel_deactivate")), _("Acti_vate"));
				}
			}
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_edit")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_insert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_delete")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_deactivate")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_convert_to")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_frame_convert")), FALSE);
		selected_unit = NULL;
	}
	if(!block_unit_convert) convert_in_wUnits();
}

void update_unit_selector_tree() {
	//if(!unit_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitSelectorCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelectorCategories_selection_changed, NULL);
	gtk_tree_store_clear(tUnitSelectorCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelectorCategories_selection_changed, NULL);
	gtk_tree_store_append(tUnitSelectorCategories_store, &iter3, NULL);
	gtk_tree_store_set(tUnitSelectorCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	unit_cats.it = unit_cats.items.begin();
	if(unit_cats.it != unit_cats.items.end()) {
		item = &*unit_cats.it;
		++unit_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	convert_category_map.clear();
	while(item) {
		gtk_tree_store_append(tUnitSelectorCategories_store, &iter, &iter2);
		if(!str.empty()) str += "/";
		str += item->item;
		gtk_tree_store_set(tUnitSelectorCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_unit_category) {
			EXPAND_TO_ITER(model, tUnitSelectorCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
		}
		convert_category_map[str] = iter;
		while(item && item->it == item->items.end()) {
			size_t str_i = str.rfind("/");
			if(str_i == string::npos) {
				str = "";
			} else {
				str = str.substr(0, str_i);
			}
			item = item->parent;
			gtk_tree_model_iter_parent(model, &iter2, &iter);
			iter = iter2;
		}
		if(item) {
			item2 = &*item->it;
			if(item->it == item->items.begin()) iter2 = iter;
			++item->it;
			item = item2;
			item->it = item->items.begin();	
		}
	}
	if(!unit_cats.objects.empty()) {	
		//add "Uncategorized" category if there are units without category
		gtk_tree_store_append(tUnitSelectorCategories_store, &iter, &iter3);
		gtk_tree_store_set(tUnitSelectorCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		convert_category_map[_("Uncategorized")] = iter;
		if(selected_unit_category == _("Uncategorized")) {
			EXPAND_TO_ITER(model, tUnitSelectorCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
		}
	}	
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_unit_category = _("All");
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitSelectorCategories_store), &iter);
		EXPAND_ITER(model, tUnitSelectorCategories, iter)
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
	}
}

void setUnitSelectorTreeItem(GtkTreeIter &iter2, Unit *u) {
	gtk_list_store_append(tUnitSelector_store, &iter2);
	string snames, sbase;
	gtk_list_store_set(tUnitSelector_store, &iter2, 0, u->title(true).c_str(), 1, (gpointer) u, -1);
}

/*
	generate the unit tree in conversion tab when category selection has changed
*/
void on_tUnitSelectorCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {

	block_unit_selector_convert = true;

	GtkTreeModel *model;
	GtkTreeIter iter, iter2;

	bool no_cat = false, b_all = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelector_selection_changed, NULL);
	gtk_list_store_clear(tUnitSelector_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelector_selection_changed, NULL);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_unit_selector_category = gstr;
		if(selected_unit_selector_category == _("All")) {
			b_all = true;
		} else if(selected_unit_selector_category == _("Uncategorized")) {
			no_cat = true;
		}
		if(!b_all && !no_cat && selected_unit_selector_category[0] == '/') {
			string str = selected_unit_selector_category.substr(1, selected_unit_selector_category.length() - 1);
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {	
				if(CALCULATOR->units[i]->isActive() && !CALCULATOR->units[i]->isHidden() && CALCULATOR->units[i]->category().substr(0, selected_unit_selector_category.length() - 1) == str) {
					setUnitSelectorTreeItem(iter2, CALCULATOR->units[i]);
				}
			}
		} else {
			bool list_empty = true;
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if(CALCULATOR->units[i]->isActive() && !CALCULATOR->units[i]->isHidden() && (b_all || (no_cat && CALCULATOR->units[i]->category().empty()) || CALCULATOR->units[i]->category() == selected_unit_selector_category)) {
					setUnitSelectorTreeItem(iter2, CALCULATOR->units[i]);
					list_empty = false;
				}
			}
			bool collapse_all = true;
			if(list_empty && !b_all && !no_cat) {
				for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
					if(CALCULATOR->units[i]->isActive() && !CALCULATOR->units[i]->isHidden() && CALCULATOR->units[i]->category().length() > selected_unit_selector_category.length() && CALCULATOR->units[i]->category()[selected_unit_selector_category.length()] == '/' && CALCULATOR->units[i]->category().substr(0, selected_unit_selector_category.length()) == selected_unit_selector_category) {
						setUnitSelectorTreeItem(iter2, CALCULATOR->units[i]);
					}
				}
			} else if(!b_all && !no_cat) {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				collapse_all = !gtk_tree_view_expand_row(GTK_TREE_VIEW(tUnitSelectorCategories), path, FALSE);
				gtk_tree_path_free(path);
			}
			if(collapse_all) {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				if(gtk_tree_path_get_depth(path) == 2) {
					GtkTreeIter iter3;
					gtk_tree_model_get_iter_first(model, &iter3);
					if(gtk_tree_model_iter_children(model, &iter2, &iter3)) {
						do {
							GtkTreePath *path2 = gtk_tree_model_get_path(model, &iter2);
							if(gtk_tree_path_compare(path, path2) != 0) gtk_tree_view_collapse_row(GTK_TREE_VIEW(tUnitSelectorCategories), path2);
							gtk_tree_path_free(path2);
						} while(gtk_tree_model_iter_next(model, &iter2));
					}
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tUnitSelectorCategories), path, NULL, FALSE, 0, 0);
				}
				gtk_tree_path_free(path);
			}
		}
		g_free(gstr);
	} else {
		selected_unit_selector_category = "";
	}
	
	block_unit_selector_convert = false;
	
}

void on_tUnitSelector_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Unit *u;
		gtk_tree_model_get(model, &iter, 1, &u, -1);
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i] == u) {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")), u->print(false, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit")).c_str());
				if(!block_unit_selector_convert) convert_from_convert_entry_unit();
			}
		}
	}
}


void update_datasets_tree() {
	if(!datasets_builder) return;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
	gtk_list_store_clear(tDatasets_store);
	DataSet *ds;
	bool b = false;
	for(size_t i = 1; ; i++) {
		ds = CALCULATOR->getDataSet(i);
		if(!ds) break;
		gtk_list_store_append(tDatasets_store, &iter);
		gtk_list_store_set(tDatasets_store, &iter, 0, ds->title().c_str(), 1, (gpointer) ds, -1);
		if(ds == selected_dataset) {
			g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
			gtk_tree_selection_select_iter(select, &iter);
			g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
			b = true;
		}
	}
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
	if(!b) {
		gtk_tree_selection_unselect_all(select);
		selected_dataset = NULL;
	}
}

void on_tDatasets_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects));
	gtk_tree_selection_unselect_all(select);
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDataObjects_selection_changed, NULL);
	gtk_list_store_clear(tDataObjects_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDataObjects_selection_changed, NULL);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		DataSet *ds = NULL;
		gtk_tree_model_get(model, &iter, 1, &ds, -1);
		selected_dataset = ds;
		if(!ds) return;
		DataObjectIter it;
		DataPropertyIter pit;
		DataProperty *dp;
		DataObject *o = ds->getFirstObject(&it);
		bool b = false;
		while(o) {
			b = true;
			gtk_list_store_append(tDataObjects_store, &iter2);
			dp = ds->getFirstProperty(&pit);
			size_t index = 0;
			while(dp) {
				if(!dp->isHidden() && dp->isKey()) {
					gtk_list_store_set(tDataObjects_store, &iter2, index, o->getPropertyDisplayString(dp).c_str(), -1);
					index++;
					if(index > 2) break;
				}
				dp = ds->getNextProperty(&pit);
			}
			while(index < 3) {
				gtk_list_store_set(tDataObjects_store, &iter2, index, "", -1);
				index++;
			}
			gtk_list_store_set(tDataObjects_store, &iter2, 3, (gpointer) o, -1);
			if(o == selected_dataobject) {
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects)), &iter2);
			}
			o = ds->getNextObject(&it);
		}
		if(b && (!selected_dataobject || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects)), &model2, &iter2))) {
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tDataObjects_store), &iter2);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects)), &iter2);
		}
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasets_builder, "datasets_textview_description")));
		gtk_text_buffer_set_text(buffer, "", -1);
		GtkTextIter iter;
		string str, str2;
		if(!ds->description().empty()) {
			str = ds->description();
			str += "\n";
			str += "\n";
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
		}	
		str = _("Properties");
		str += "\n";
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
		dp = ds->getFirstProperty(&pit);
		while(dp) {	
			if(!dp->isHidden()) {
				str = "";
				if(!dp->title(false).empty()) {
					str += dp->title();	
					str += ": ";
				}
				for(size_t i = 1; i <= dp->countNames(); i++) {
					if(i > 1) str += ", ";
					str += dp->getName(i);
				}
				if(dp->isKey()) {
					str += " (";
					str += _("key");
					str += ")";
				}
				str += "\n";
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
				if(!dp->description().empty()) {
					str = dp->description();
					str += "\n";
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
				}
			}
			dp = ds->getNextProperty(&pit);
		}
		str = "\n";
		str += _("Data Retrieval Function");
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
		Argument *arg;
		Argument default_arg;
		const ExpressionName *ename = &ds->preferredName(false, true, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(datasets_builder, "datasets_textview_description"));
		str = "\n";
		str += ename->name;
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", "italic", NULL);
		str = "";
		int iargs = ds->maxargs();
		if(iargs < 0) {
			iargs = ds->minargs() + 1;
		}
		str += "(";				
		if(iargs != 0) {
			for(int i2 = 1; i2 <= iargs; i2++) {	
				if(i2 > ds->minargs()) {
					str += "[";
				}
				if(i2 > 1) {
					str += CALCULATOR->getComma();
					str += " ";
				}
				arg = ds->getArgumentDefinition(i2);
				if(arg && !arg->name().empty()) {
					str2 = arg->name();
				} else {
					str2 = _("argument");
					str2 += " ";
					str2 += i2s(i2);
				}
				str += str2;
				if(i2 > ds->minargs()) {
					str += "]";
				}
			}
			if(ds->maxargs() < 0) {
				str += CALCULATOR->getComma();
				str += " …";
			}
		}
		str += ")";
		for(size_t i2 = 1; i2 <= ds->countNames(); i2++) {
			if(&ds->getName(i2) != ename) {
				str += "\n";
				str += ds->getName(i2).name;
			}
		}
		str += "\n\n";
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
		if(!ds->copyright().empty()) {
			str = "\n";
			str = ds->copyright();
			str += "\n";
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editset")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delset")), ds->isLocal());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_newobject")), TRUE);
	} else {
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasets_builder, "datasets_textview_description"))), "", -1);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editset")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delset")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_newobject")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editobject")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delobject")), FALSE);
		selected_dataset = NULL;
	}
}

void update_dataobjects() {
	on_tDatasets_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets)), NULL);
}

void on_dataset_button_function_clicked(GtkButton *w, gpointer user_data) {
	DataProperty *dp = (DataProperty*) user_data;
	DataObject *o = selected_dataobject;
	DataSet *ds = NULL;
	if(o) ds = dp->parentSet();
	if(ds && o) {
		string str = ds->preferredDisplayName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) w).name;
		str += "(";
		str += o->getProperty(ds->getPrimaryKeyProperty());
		str += CALCULATOR->getComma();
		str += " ";
		str += dp->getName();
		str += ")";
		insert_text(str.c_str());
	}
}
void on_tDataObjects_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *ptable = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_grid_properties"));
	GList *childlist = gtk_container_get_children(GTK_CONTAINER(ptable));
	for(guint i = 0; ; i++) {
		GtkWidget *w = (GtkWidget*) g_list_nth_data(childlist, i);
		if(!w) break;
		gtk_widget_destroy(w);
	}
	g_list_free(childlist);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		DataObject *o = NULL;
		gtk_tree_model_get(model, &iter, 3, &o, -1);
		selected_dataobject = o;
		if(!o) return;
		DataSet *ds = o->parentSet();
		if(!ds) return;
		DataPropertyIter it;
		DataProperty *dp = ds->getFirstProperty(&it);
		string sval;
		int rows = 1;
		gtk_grid_remove_column(GTK_GRID(ptable), 0);
		gtk_grid_remove_column(GTK_GRID(ptable), 1);
		gtk_grid_remove_column(GTK_GRID(ptable), 2);
		GtkWidget *button, *label;
		string str;
		while(dp) {
			if(!dp->isHidden()) {
				sval = o->getPropertyDisplayString(dp);
				if(!sval.empty()) {
					label = gtk_label_new(NULL);
					str = "<span weight=\"bold\">"; str += dp->title(); str += ":"; str += "</span>";
					gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), FALSE);
					gtk_widget_set_margin_end(label, 20);
					gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1 , 1);
					label = gtk_label_new(NULL);
					gtk_widget_set_hexpand(label, TRUE);
					gtk_label_set_markup(GTK_LABEL(label), sval.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
					gtk_grid_attach(GTK_GRID(ptable), label, 1, rows - 1, 1, 1);
					button = gtk_button_new();
					gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("edit-paste", GTK_ICON_SIZE_BUTTON));
					gtk_widget_set_halign(button, GTK_ALIGN_END);
					//gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
					gtk_grid_attach(GTK_GRID(ptable), button, 2, rows - 1, 1, 1);
					g_signal_connect((gpointer) button, "clicked", G_CALLBACK(on_dataset_button_function_clicked), (gpointer) dp);
					rows++;
				}
			}
			dp = ds->getNextProperty(&it);
		}
		gtk_widget_show_all(ptable);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editobject")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delobject")), o->isUserModified());
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editobject")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delobject")), FALSE);
		selected_dataobject = NULL;
	}
}

void on_tDataProperties_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_dataproperty = NULL;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 3, &selected_dataproperty, -1);
	}
	if(selected_dataproperty) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), selected_dataproperty->isUserModified());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), selected_dataproperty->isUserModified());
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), FALSE);
	}
}


void on_tPlotFunctions_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_argument = NULL;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr1, *gstr2, *gstr3;
		gint type, smoothing, style, axis, rows;
		gtk_tree_model_get(model, &iter, 0, &gstr1, 1, &gstr2, 2, &style, 3, &smoothing, 4, &type, 5, &axis, 6, &rows, 9, &gstr3, -1);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), gstr2);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")), gstr3);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")), gstr1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), style);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), smoothing);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")), type == 1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")), type == 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis1")), axis != 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis2")), axis == 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), rows);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_remove")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_modify")), TRUE);		
		g_free(gstr1);
		g_free(gstr2);
		g_free(gstr3);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_remove")), FALSE);
	}
}

void on_tSubfunctions_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_subfunction = 0;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gboolean g_b = FALSE;
		guint index = 0;
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, 3, &index, 4, &g_b, -1);
		selected_subfunction = index;
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression")), gstr);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate")), g_b);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_subfunction")), TRUE);
		g_free(gstr);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_subfunction")), FALSE);
	}
}

void on_tFunctionArguments_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_argument = NULL;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Argument *arg;
		gtk_tree_model_get(model, &iter, 2, &arg, -1);
		selected_argument = arg;
		int menu_index = MENU_ARGUMENT_TYPE_FREE;
		if(selected_argument) {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name")), selected_argument->name().c_str());
			switch(selected_argument->type()) {
				case ARGUMENT_TYPE_TEXT: {
					menu_index = MENU_ARGUMENT_TYPE_TEXT;
					break;
				}
				case ARGUMENT_TYPE_SYMBOLIC: {
					menu_index = MENU_ARGUMENT_TYPE_SYMBOLIC;
					break;
				}
				case ARGUMENT_TYPE_DATE: {
					menu_index = MENU_ARGUMENT_TYPE_DATE;				
					break;
				}
				case ARGUMENT_TYPE_INTEGER: {
					menu_index = MENU_ARGUMENT_TYPE_INTEGER;
					break;
				}
				case ARGUMENT_TYPE_NUMBER: {
					menu_index = MENU_ARGUMENT_TYPE_NUMBER;
					break;
				}
				case ARGUMENT_TYPE_VECTOR: {
					menu_index = MENU_ARGUMENT_TYPE_VECTOR;
					break;
				}
				case ARGUMENT_TYPE_MATRIX: {
					menu_index = MENU_ARGUMENT_TYPE_MATRIX;
					break;
				}
				case ARGUMENT_TYPE_EXPRESSION_ITEM: {
					menu_index = MENU_ARGUMENT_TYPE_EXPRESSION_ITEM;
					break;
				}					
				case ARGUMENT_TYPE_FUNCTION: {
					menu_index = MENU_ARGUMENT_TYPE_FUNCTION;
					break;
				}
				case ARGUMENT_TYPE_UNIT: {
					menu_index = MENU_ARGUMENT_TYPE_UNIT;
					break;
				}
				case ARGUMENT_TYPE_VARIABLE: {
					menu_index = MENU_ARGUMENT_TYPE_VARIABLE;
					break;
				}
				case ARGUMENT_TYPE_FILE: {
					menu_index = MENU_ARGUMENT_TYPE_FILE;
					break;
				}					
				case ARGUMENT_TYPE_BOOLEAN: {
					menu_index = MENU_ARGUMENT_TYPE_BOOLEAN;
					break;
				}
				case ARGUMENT_TYPE_ANGLE: {
					menu_index = MENU_ARGUMENT_TYPE_ANGLE;
					break;
				}
				case ARGUMENT_TYPE_DATA_OBJECT: {
					menu_index = MENU_ARGUMENT_TYPE_DATA_OBJECT;
					break;
				}
				case ARGUMENT_TYPE_DATA_PROPERTY: {
					menu_index = MENU_ARGUMENT_TYPE_DATA_PROPERTY;
					break;
				}
			}			
		} else {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name")), "");
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(functionedit_builder, "function_edit_combobox_argument_type")), menu_index);
		if(!(get_edited_function() && get_edited_function()->isBuiltin())) {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_rules")), TRUE);	
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_argument")), TRUE);
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), TRUE);		
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_argument")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_rules")), FALSE);	
	}
}
void update_function_arguments_list(MathFunction *f) {
	if(!functionedit_builder) return;
	selected_argument = NULL;
	gtk_list_store_clear(tFunctionArguments_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_argument")), FALSE);	
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_rules")), FALSE);
	if(f) {
		GtkTreeIter iter;
		Argument *arg;
		int args = f->maxargs();
		if(args < 0) {
			args = f->minargs() + 1;	
		}
		Argument defarg;
		string str, str2;
		for(int i = 1; i <= args; i++) {
			gtk_list_store_append(tFunctionArguments_store, &iter);
			arg = f->getArgumentDefinition(i);
			if(arg) {
				arg = arg->copy();
				str = arg->printlong();
				str2 = arg->name();
			} else {
				str = defarg.printlong();
				str2 = "";
			}			
			gtk_list_store_set(tFunctionArguments_store, &iter, 0, str2.c_str(), 1, str.c_str(), 2, (gpointer) arg, -1);
		}
	}
}

void on_tNames_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_subfunction = 0;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gboolean abbreviation = FALSE, suffix = FALSE, unicode = FALSE, plural = FALSE, reference = FALSE, avoid_input = FALSE, case_sensitive = FALSE;
		gchar *name;
		gtk_tree_model_get(model, &iter, NAMES_NAME_COLUMN, &name, NAMES_ABBREVIATION_COLUMN, &abbreviation, NAMES_SUFFIX_COLUMN, &suffix, NAMES_UNICODE_COLUMN, &unicode, NAMES_PLURAL_COLUMN, &plural, NAMES_REFERENCE_COLUMN, &reference, NAMES_AVOID_INPUT_COLUMN, &avoid_input, NAMES_CASE_SENSITIVE_COLUMN, &case_sensitive, -1);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")), name);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation")), abbreviation);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix")), suffix);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode")), unicode);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural")), plural);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference")), reference);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input")), avoid_input);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive")), case_sensitive);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_remove")), TRUE);
		g_free(name);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_remove")), FALSE);
	}
}



/*
	generate unit submenu in expression menu
*/
void create_umenu() {
	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "units_menu"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	
	u_menu = sub;
	sub2 = sub;
	Unit *u;
	tree_struct *titem, *titem2;
	unit_cats.rit = unit_cats.items.rbegin();
	if(unit_cats.rit != unit_cats.items.rend()) {
		titem = &*unit_cats.rit;
		++unit_cats.rit;
		titem->rit = titem->items.rbegin();
	} else {
		titem = NULL;
	}
	stack<GtkWidget*> menus;
	menus.push(sub);
	sub3 = sub;
	while(titem) {
		bool b_empty = titem->items.size() == 0;
		if(b_empty) {
			for(size_t i = 0; i < titem->objects.size(); i++) {
				u = (Unit*) titem->objects[i];
				if(u->isActive() && !u->isHidden()) {
					b_empty = false;
					break;
				}
			}
		}
		if(!b_empty) {
			SUBMENU_ITEM_PREPEND(titem->item.c_str(), sub3)
			menus.push(sub);
			sub3 = sub;
			bool is_currencies = false;
			for(size_t i = 0; i < titem->objects.size(); i++) {
				u = (Unit*) titem->objects[i];
				if(!is_currencies && u == CALCULATOR->u_euro) is_currencies = true;
				if(u->isActive() && !u->isHidden()) {
					MENU_ITEM_WITH_POINTER(u->title(true).c_str(), insert_unit, u)
				}
			}
			if(is_currencies) {
				SUBMENU_ITEM_PREPEND(_("more"), sub3)
				for(size_t i = 0; i < titem->objects.size(); i++) {
					u = (Unit*) titem->objects[i];
					if(u->isActive() && u->isHidden()) {
						MENU_ITEM_WITH_POINTER(u->title(true).c_str(), insert_unit, u)
					}
				}
			}
		} else {
			titem = titem->parent;
		}
		while(titem && titem->rit == titem->items.rend()) {
			titem = titem->parent;
			menus.pop();
			if(menus.size() > 0) sub3 = menus.top();
		}	
		if(titem) {
			titem2 = &*titem->rit;
			++titem->rit;
			titem = titem2;
			titem->rit = titem->items.rbegin();	
		}
	}
	sub = sub2;
	for(size_t i = 0; i < unit_cats.objects.size(); i++) {
		u = (Unit*) unit_cats.objects[i];
		if(u->isActive() && !u->isHidden()) {
			MENU_ITEM_WITH_POINTER(u->title(true).c_str(), insert_unit, u)
		}
	}		
	
	MENU_SEPARATOR	
	item = gtk_menu_item_new_with_label(_("Prefixes"));
	gtk_widget_show (item);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
	create_pmenu(item);		
	
}

/*
	generate unit submenu in result menu
*/
void create_umenu2() {
	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_result_units"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);	
	u_menu2 = sub;
	sub2 = sub;
	Unit *u;
	tree_struct *titem, *titem2;
	unit_cats.rit = unit_cats.items.rbegin();
	if(unit_cats.rit != unit_cats.items.rend()) {
		titem = &*unit_cats.rit;
		++unit_cats.rit;
		titem->rit = titem->items.rbegin();
	} else {
		titem = NULL;
	}
	stack<GtkWidget*> menus;
	menus.push(sub);
	sub3 = sub;
	while(titem) {
		bool b_empty = titem->items.size() == 0;
		if(b_empty) {
			for(size_t i = 0; i < titem->objects.size(); i++) {
				u = (Unit*) titem->objects[i];
				if(u->isActive() && !u->isHidden()) {
					b_empty = false;
					break;
				}
			}
		}
		if(!b_empty) {
			SUBMENU_ITEM_PREPEND(titem->item.c_str(), sub3)
			menus.push(sub);
			sub3 = sub;
			bool is_currencies = false;
			for(size_t i = 0; i < titem->objects.size(); i++) {
				u = (Unit*) titem->objects[i];
				if(!is_currencies && u == CALCULATOR->u_euro) is_currencies = true;
				if(u->isActive() && !u->isHidden()) {
					MENU_ITEM_WITH_POINTER(u->title(true).c_str(), convert_to_unit, u)
				}
			}
			if(is_currencies) {
				SUBMENU_ITEM_PREPEND(_("more"), sub3)
				for(size_t i = 0; i < titem->objects.size(); i++) {
					u = (Unit*) titem->objects[i];
					if(u->isActive() && u->isHidden()) {
						MENU_ITEM_WITH_POINTER(u->title(true).c_str(), convert_to_unit, u)
					}
				}
			}
		} else {
			titem = titem->parent;
		}
		while(titem && titem->rit == titem->items.rend()) {
			titem = titem->parent;
			menus.pop();
			if(menus.size() > 0) sub3 = menus.top();
		}	
		if(titem) {
			titem2 = &*titem->rit;
			++titem->rit;
			titem = titem2;
			titem->rit = titem->items.rbegin();	
		}
	}
	sub = sub2;
	for(size_t i = 0; i < unit_cats.objects.size(); i++) {
		u = (Unit*) unit_cats.objects[i];
		if(u->isActive() && !u->isHidden()) {
			MENU_ITEM_WITH_POINTER(u->title(true).c_str(), convert_to_unit, u)
		}
	}		
}

/*
	recreate unit menus and update unit manager (when units have changed)
*/
void update_umenus() {
	gtk_widget_destroy(u_menu);
	gtk_widget_destroy(u_menu2);
	generate_units_tree_struct();
	create_umenu();
	recreate_recent_units();
	create_umenu2();
	update_units_tree();
	update_unit_selector_tree();
	update_completion();
}

/*
	generate variables submenu in expression menu
*/
void create_vmenu() {

	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "variables_menu"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	
	v_menu = sub;
	sub2 = sub;
	Variable *v;
	tree_struct *titem, *titem2;
	variable_cats.rit = variable_cats.items.rbegin();
	if(variable_cats.rit != variable_cats.items.rend()) {
		titem = &*variable_cats.rit;
		++variable_cats.rit;
		titem->rit = titem->items.rbegin();
	} else {
		titem = NULL;
	}

	stack<GtkWidget*> menus;
	menus.push(sub);
	sub3 = sub;
	while(titem) {
		bool b_empty = titem->items.size() == 0;
		if(b_empty) {
			for(size_t i = 0; i < titem->objects.size(); i++) {
				v = (Variable*) titem->objects[i];
				if(v->isActive() && !v->isHidden()) {
					b_empty = false;
					break;
				}
			}
		}
		if(!b_empty) {
			SUBMENU_ITEM_PREPEND(titem->item.c_str(), sub3)
			menus.push(sub);
			sub3 = sub;			
			for(size_t i = 0; i < titem->objects.size(); i++) {
				v = (Variable*) titem->objects[i];
				if(v->isActive() && !v->isHidden()) {
					MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_variable, v);
				}
			}
		} else {
			titem = titem->parent;
		}
		while(titem && titem->rit == titem->items.rend()) {
			titem = titem->parent;
			menus.pop();
			if(menus.size() > 0) sub3 = menus.top();
		}	
		if(titem) {
			titem2 = &*titem->rit;
			++titem->rit;
			titem = titem2;
			titem->rit = titem->items.rbegin();	
		}
	}
	sub = sub2;

	for(size_t i = 0; i < variable_cats.objects.size(); i++) {
		v = (Variable*) variable_cats.objects[i];
		if(v->isActive() && !v->isHidden()) {
			MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_variable, v);
		}
	}		

}


/*
	generate prefixes submenu in expression menu
*/
void create_pmenu(GtkWidget *item) {
//	GtkWidget *item;
	GtkWidget *sub;
//	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_expression_prefixes"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(item), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
	int index = 0;
	Prefix *p = CALCULATOR->getPrefix(index);
	while(p) {
		gchar *gstr = NULL;
		switch(p->type()) {
			case PREFIX_DECIMAL: {
				gstr = g_strdup_printf("%s (10<span size=\"x-small\" rise=\"%i\">%i</span>)", p->name(false, true, &can_display_unicode_string_function, (void*) item).c_str(), (int) (pango_font_description_get_size(font_desc) / 1.5), ((DecimalPrefix*) p)->exponent());
				break;
			}
			case PREFIX_BINARY: {
				gstr = g_strdup_printf("%s (2<span size=\"x-small\" rise=\"%i\">%i</span>)", p->name(false, true, &can_display_unicode_string_function, (void*) item).c_str(), (int) (pango_font_description_get_size(font_desc) / 1.5), ((BinaryPrefix*) p)->exponent());
				break;
			}
			case PREFIX_NUMBER: {				
				gstr = g_strdup_printf("%s", p->name(false, true, &can_display_unicode_string_function, (void*) item).c_str());
				break;
			}			
		}
		MENU_ITEM_WITH_POINTER(gstr, insert_prefix, p)
		gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), TRUE);
		g_free(gstr);			
		index++;
		p = CALCULATOR->getPrefix(index);
	}
	pango_font_description_free(font_desc);
}

/*
	generate prefixes submenu in result menu
*/
void create_pmenu2() {
	GtkWidget *item;
	GtkWidget *sub;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_result_prefixes"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);	
	int index = 0;
	MENU_ITEM_WITH_POINTER(_("No Prefix"), on_menu_item_set_prefix_activate, CALCULATOR->decimal_null_prefix)
	MENU_ITEM_WITH_POINTER(_("Optimal Prefix"), on_menu_item_set_prefix_activate, NULL)
	Prefix *p = CALCULATOR->getPrefix(index);
	while(p) {
		gchar *gstr = NULL;
		switch(p->type()) {
			case PREFIX_DECIMAL: {
				gstr = g_strdup_printf("%s (10<sup>%i</sup>)", p->name(false, true, &can_display_unicode_string_function, (void*) item).c_str(), ((DecimalPrefix*) p)->exponent());
				break;
			}
			case PREFIX_BINARY: {
				gstr = g_strdup_printf("%s (2<sup>%i</sup>)", p->name(false, true, &can_display_unicode_string_function, (void*) item).c_str(), ((BinaryPrefix*) p)->exponent());
				break;
			}
			case PREFIX_NUMBER: {				
				gstr = g_strdup_printf("%s", p->name(false, true, &can_display_unicode_string_function, (void*) item).c_str());
				break;
			}			
		}
		MENU_ITEM_WITH_POINTER(gstr, on_menu_item_set_prefix_activate, p)
		gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), TRUE);
		g_free(gstr);			
		index++;
		p = CALCULATOR->getPrefix(index);
	}	
}

/*
	recreate variables menu and update variable manager (when variables have changed)
*/
void update_vmenu() {
	gtk_widget_destroy(v_menu);
	generate_variables_tree_struct();
	create_vmenu();
	recreate_recent_variables();
	update_variables_tree();
	update_completion();
}

/*
	generate functions submenu in expression menu
*/
void create_fmenu() {
	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "functions_menu"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	f_menu = sub;
	sub2 = sub;
	MathFunction *f;
	tree_struct *titem, *titem2;
	function_cats.rit = function_cats.items.rbegin();
	if(function_cats.rit != function_cats.items.rend()) {
		titem = &*function_cats.rit;
		++function_cats.rit;
		titem->rit = titem->items.rbegin();
	} else {
		titem = NULL;
	}
	stack<GtkWidget*> menus;
	menus.push(sub);
	sub3 = sub;
	while(titem) {
		bool b_empty = titem->items.size() == 0;
		if(b_empty) {
			for(size_t i = 0; i < titem->objects.size(); i++) {
				f = (MathFunction*) titem->objects[i];
				if(f->isActive() && !f->isHidden()) {
					b_empty = false;
					break;
				}
			}
		}
		if(!b_empty) {
			SUBMENU_ITEM_PREPEND(titem->item.c_str(), sub3)
			for(size_t i = 0; i < titem->objects.size(); i++) {
				f = (MathFunction*) titem->objects[i];
				if(f->isActive() && !f->isHidden()) {
					MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_function, f)
				}
			}
			menus.push(sub);
			sub3 = sub;
		} else {
			titem = titem->parent;
		}		
		while(titem && titem->rit == titem->items.rend()) {
			titem = titem->parent;
			menus.pop();
			if(menus.size() > 0) sub3 = menus.top();
		}	
		if(titem) {
			titem2 = &*titem->rit;
			++titem->rit;
			titem = titem2;
			titem->rit = titem->items.rbegin();	
		}
	}
	sub = sub2;
	for(size_t i = 0; i < function_cats.objects.size(); i++) {
		f = (MathFunction*) function_cats.objects[i];
		if(f->isActive() && !f->isHidden()) {
			MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_function, f)
		}
	}		
}

string sub_suffix(const ExpressionName *ename) {
	size_t i = ename->name.rfind('_');
	bool b = i == string::npos || i == ename->name.length() - 1 || i == 0;
	size_t i2 = 1;
	string str;
	if(b) {
		if(is_in(NUMBERS, ename->name[ename->name.length() - 1])) {
			while(ename->name.length() > i2 + 1 && is_in(NUMBERS, ename->name[ename->name.length() - 1 - i2])) {
					i2++;
			}
		}
		str += ename->name.substr(0, ename->name.length() - i2);
	} else {
		str += ename->name.substr(0, i);
	}
	str += "<span size=\"small\"><sub>";
	if(b) str += ename->name.substr(ename->name.length() - i2, i2);
	else str += ename->name.substr(i + 1, ename->name.length() - (i + 1));
	str += "</sub></span>";
	return str;
}

void update_completion() {

	GtkTreeIter iter;
	
	gtk_list_store_clear(completion_store);
	
	if(!enable_completion) return;
	
	string str;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(CALCULATOR->functions[i]->isActive()) {
			gtk_list_store_append(completion_store, &iter);
			const ExpressionName *ename, *ename_r;
			ename_r = &CALCULATOR->functions[i]->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
			if(ename_r->suffix && ename_r->name.length() > 1) {
				str = sub_suffix(ename_r);
			} else {
				str = ename_r->name;
			}
			str += "()";
			for(size_t name_i = 1; name_i <= CALCULATOR->functions[i]->countNames(); name_i++) {
				ename = &CALCULATOR->functions[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expressiontext))) {
					str += " <i>";
					if(ename->suffix && ename->name.length() > 1) {
						str += sub_suffix(ename);
					} else {
						str += ename->name;
					}
					str += "()</i>";
				}
			}			
			gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, CALCULATOR->functions[i]->title().c_str(), 2, CALCULATOR->functions[i], -1);
		}
	}
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isActive()) {
			gtk_list_store_append(completion_store, &iter);
			const ExpressionName *ename, *ename_r;
			bool b = false;
			ename_r = &CALCULATOR->variables[i]->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
			for(size_t name_i = 1; name_i <= CALCULATOR->variables[i]->countNames(); name_i++) {
				ename = &CALCULATOR->variables[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expressiontext))) {
					if(!b) {
						if(ename_r->suffix && ename_r->name.length() > 1) {
							str = sub_suffix(ename_r);
						} else {
							str = ename_r->name;
						}
						b = true;
					}
					str += " <i>";
					if(ename->suffix && ename->name.length() > 1) {
						str += sub_suffix(ename);
					} else {
						str += ename->name;
					}
					str += "</i>";
				}
			}
			if(!CALCULATOR->variables[i]->title(false).empty()) {
				if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, CALCULATOR->variables[i]->title().c_str(), 2, CALCULATOR->variables[i], -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, CALCULATOR->variables[i]->title().c_str(), 2, CALCULATOR->variables[i], -1);
			} else {
				Variable *v = CALCULATOR->variables[i];
				string title;
				if(is_answer_variable(v)) {
					title = _("a previous result");
				} else if(v->isKnown()) {
					if(((KnownVariable*) v)->isExpression()) {
						title = CALCULATOR->localizeExpression(((KnownVariable*) v)->expression());
					} else {
						if(((KnownVariable*) v)->get().isMatrix()) {
							title = _("matrix");
						} else if(((KnownVariable*) v)->get().isVector()) {
							title = _("vector");
						} else {
							title = CALCULATOR->print(((KnownVariable*) v)->get(), 30);
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
				if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], -1);
			}
		}
	}
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->isActive() && CALCULATOR->units[i]->subtype() != SUBTYPE_COMPOSITE_UNIT) {
			gtk_list_store_append(completion_store, &iter);
			const ExpressionName *ename, *ename_r;
			bool b = false;
			ename_r = &CALCULATOR->units[i]->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
			for(size_t name_i = 1; name_i <= CALCULATOR->units[i]->countNames(); name_i++) {
				ename = &CALCULATOR->units[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), (void*) expressiontext))) {
					if(!b) {
						if(ename_r->suffix && ename_r->name.length() > 1) {
							str = sub_suffix(ename_r);
						} else {
							str = ename_r->name;
						}
						b = true;
					}
					str += " <i>";
					if(ename->suffix && ename->name.length() > 1) {
						str += sub_suffix(ename);
					} else {
						str += ename->name;
					}
					str += "</i>";
				}
			}
			if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, CALCULATOR->units[i]->title().c_str(), 2, CALCULATOR->units[i], -1);
			else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, CALCULATOR->units[i]->title().c_str(), 2, CALCULATOR->units[i], -1);
		}
	}	
}

/*
	recreate functions menu and update function manager (when functions have changed)
*/
void update_fmenu() {
	gtk_widget_destroy(f_menu);
	generate_functions_tree_struct();
	create_fmenu();
	recreate_recent_functions();
	update_completion();
	update_functions_tree();
}


string get_value_string(const MathStructure &mstruct_, bool rlabel = false, Prefix *prefix = NULL) {
	printops.allow_non_usable = rlabel;
	printops.prefix = prefix;
	string str = CALCULATOR->print(mstruct_, 100, printops);
	printops.allow_non_usable = false;
	printops.prefix = NULL;
	return str;
}


void draw_background(cairo_t *cr, gint w, gint h) {
/*	GdkRGBA rgba;
	gtk_style_context_get_background_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview);, &rgba);
	gdk_cairo_set_source_rgba(cr, &rgba);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);*/
}

cairo_surface_t *get_left_parenthesis(gint arc_w, gint arc_h, int, GdkRGBA *color) {
	gint scalefactor = gtk_widget_get_scale_factor(expressiontext);
	gint par_h = 0, par_w = 0;
	PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
	pango_layout_set_markup(layout, "<span font=\"100\">(</span>", -1);
	pango_layout_get_pixel_size(layout, &par_w, &par_h);
	cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, arc_w * scalefactor, arc_h * scalefactor);
	cairo_surface_set_device_scale(s, scalefactor, scalefactor);
	cairo_t *cr = cairo_create(s);
	cairo_scale(cr, (double) arc_w / (double) par_w, (double) arc_h / (double) par_h);
	gdk_cairo_set_source_rgba(cr, color);
	pango_cairo_show_layout(cr, layout);
	cairo_destroy(cr);
	return s;
}
cairo_surface_t *get_right_parenthesis(gint arc_w, gint arc_h, int, GdkRGBA *color) {
	gint scalefactor = gtk_widget_get_scale_factor(expressiontext);
	gint par_h = 0, par_w = 0;
	PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
	pango_layout_set_markup(layout, "<span font=\"100\">)</span>", -1);
	pango_layout_get_pixel_size(layout, &par_w, &par_h);
	cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, arc_w * scalefactor, arc_h * scalefactor);
	cairo_surface_set_device_scale(s, scalefactor, scalefactor);
	cairo_t *cr = cairo_create(s);
	cairo_scale(cr, (double) arc_w / (double) par_w, (double) arc_h / (double) par_h);
	gdk_cairo_set_source_rgba(cr, color);
	pango_cairo_show_layout(cr, layout);
	cairo_destroy(cr);
	return s;
}

#define SHOW_WITH_ROOT_SIGN(x) (x.isFunction() && ((x.function() == CALCULATOR->f_sqrt && x.size() == 1) || (x.function() == CALCULATOR->f_cbrt && x.size() == 1) || (x.function() == CALCULATOR->f_root && x.size() == 2 && x[1].isNumber() && x[1].number().isInteger() && x[1].number().isPositive() && x[1].number().isLessThan(10))))

cairo_surface_t *draw_structure(MathStructure &m, PrintOptions po, InternalPrintStruct ips, gint *point_central, int scaledown, GdkRGBA *color) {

	if(CALCULATOR->aborted()) return NULL;
	
	gint scalefactor = gtk_widget_get_scale_factor(expressiontext);

	if(ips.depth == 0 && po.is_approximate) *po.is_approximate = false;

	cairo_surface_t *surface = NULL;
	cairo_t *cr = NULL;
	GdkRGBA rgba;
	if(!color) {
		gtk_style_context_get_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview), &rgba);
		color = &rgba;
	}
	gint w, h;
	gint central_point = 0;

	InternalPrintStruct ips_n = ips;
	if(m.isApproximate()) ips_n.parent_approximate = true;
	if(m.precision() > 0 && (ips_n.parent_precision < 1 || m.precision() < ips_n.parent_precision)) ips_n.parent_precision = m.precision();

	switch(m.type()) {
		case STRUCT_NUMBER: {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			string exp = "";
			bool exp_minus;
			ips_n.exp = &exp;
			ips_n.exp_minus = &exp_minus;
			TTBP(str)
			unordered_map<void*, string>::iterator it = number_map.find((void*) &m.number());
			if(it != number_map.end()) {
				if(po.is_approximate && !(*po.is_approximate) && number_approx_map[(void*) &m.number()]) *po.is_approximate = true;
				str += it->second;
				exp = number_exp_map[(void*) &m.number()];
				exp_minus = number_exp_minus_map[(void*) &m.number()];
			} else {
				number_map[(void*) &m.number()] = m.number().print(po, ips_n);
				str += number_map[(void*) &m.number()];
				number_exp_map[(void*) &m.number()] = exp;
				number_exp_minus_map[(void*) &m.number()] = exp_minus;
				if(po.is_approximate) {
					number_approx_map[(void*) &m.number()] = *po.is_approximate;
				} else {
					number_approx_map[(void*) &m.number()] = FALSE;
				}
			}
			if(!exp.empty()) {
				if(po.lower_case_e) {TTP(str, "e");}
				else {TTP_SMALL(str, "E");}
				if(exp_minus) {
					str += "-";
				}
				str += exp;
			} else if(po.base == BASE_SEXAGESIMAL || po.base == BASE_TIME) {
				string estr;
				if(po.lower_case_e) {TTP(estr, "e");}
				else {TTP_SMALL(estr, "E");}
				if(po.lower_case_e) gsub("e", estr, str);
				else gsub("E", estr, str);
			}
			if(po.base != BASE_DECIMAL && po.base != BASE_HEXADECIMAL && po.base > 0 && po.base <= 36) {
				TTBP_SMALL(str)
				str += "<sub>";
				str += i2s(po.base);
				str += "</sub>";
				TTE(str)
			}
			TTE(str)
			pango_layout_set_markup(layout, str.c_str(), -1);
			PangoRectangle rect;
			pango_layout_get_pixel_size(layout, &w, &h);
			pango_layout_get_pixel_extents(layout, &rect, NULL);
			w = rect.width + rect.x;
			w += 1;
			central_point = h / 2;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, 1, 0);
			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			str = "<i>";
			TTBP(str)
			str += m.symbol();
			TTE(str)
			str += "</i>";
			pango_layout_set_markup(layout, str.c_str(), -1);
			PangoRectangle rect;
			pango_layout_get_pixel_size(layout, &w, &h);
			pango_layout_get_pixel_extents(layout, &rect, NULL);
			w = rect.width + rect.x;
			w += 1;
			central_point = h / 2;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, 1, 0);
			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);
			break;
		}
		case STRUCT_ADDITION: {
			ips_n.depth++;
			
			vector<cairo_surface_t*> surface_terms;
			vector<gint> hpt;
			vector<gint> wpt;
			vector<gint> cpt;
			gint plus_w, plus_h, minus_w, minus_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0;
			
			CALCULATE_SPACE_W
			PangoLayout *layout_plus = gtk_widget_create_pango_layout(resultview, NULL);
			PANGO_TTP(layout_plus, "+");
			pango_layout_get_pixel_size(layout_plus, &plus_w, &plus_h);
			PangoLayout *layout_minus = gtk_widget_create_pango_layout(resultview, NULL);
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) {
				PANGO_TTP(layout_minus, SIGN_MINUS);
			} else {
				PANGO_TTP(layout_minus, "-");
			}
			pango_layout_get_pixel_size(layout_minus, &minus_w, &minus_h);
			for(size_t i = 0; i < m.size(); i++) {
				hetmp = 0;		
				if(m[i].type() == STRUCT_NEGATE && i > 0) {
					ips_n.wrap = m[i][0].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure(m[i][0], po, ips_n, &hetmp, scaledown, color));
				} else {
					ips_n.wrap = m[i].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure(m[i], po, ips_n, &hetmp, scaledown, color));
				}
				if(CALCULATOR->aborted()) {
					for(size_t i = 0; i < surface_terms.size(); i++) {
						if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
					}
					g_object_unref(layout_minus);
					g_object_unref(layout_plus);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_terms[i]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_terms[i]) / scalefactor;
				hpt.push_back(htmp);
				cpt.push_back(hetmp);
				wpt.push_back(wtmp);
				w += wtmp;
				if(m[i].type() == STRUCT_NEGATE && i > 0) {
					w += minus_w;
					if(minus_h / 2 > dh) {
						dh = minus_h / 2;
					}
					if(minus_h / 2 + minus_h % 2 > uh) {
						uh = minus_h / 2 + minus_h % 2;
					}						
				} else if(i > 0) {
					w += plus_w;
					if(plus_h / 2 > dh) {
						dh = plus_h / 2;
					}
					if(plus_h / 2 + plus_h % 2 > uh) {
						uh = plus_h / 2 + plus_h % 2;
					}					
				}
				if(htmp - hetmp > uh) {
					uh = htmp - hetmp;
				}
				if(hetmp > dh) {
					dh = hetmp;
				}				
			}
			w += space_w * (surface_terms.size() - 1) * 2;
			central_point = dh;
			h = dh + uh;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			w = 0;
			for(size_t i = 0; i < surface_terms.size(); i++) {
				if(!CALCULATOR->aborted()) {
					gdk_cairo_set_source_rgba(cr, color);
					if(i > 0) {
						w += space_w;
						if(m[i].type() == STRUCT_NEGATE) {
							cairo_move_to(cr, w, uh - minus_h / 2 - minus_h % 2);
							pango_cairo_show_layout(cr, layout_minus);
							w += minus_w;
						} else {
							cairo_move_to(cr, w, uh - plus_h / 2 - plus_h % 2);
							pango_cairo_show_layout(cr, layout_plus);
							w += plus_w;
						}
						w += space_w;
					}
					cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
					cairo_paint(cr);
					w += wpt[i];
				}
				cairo_surface_destroy(surface_terms[i]);
			}
			g_object_unref(layout_minus);
			g_object_unref(layout_plus);
			break;
		}
		case STRUCT_NEGATE: {
			ips_n.depth++;

			gint minus_w, minus_h, uh, dh, h, w, ctmp, htmp, wtmp, hpa, cpa;
			//gint wpa;
			
			PangoLayout *layout_minus = gtk_widget_create_pango_layout(resultview, NULL);
			
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) {
				PANGO_TTP(layout_minus, SIGN_MINUS);
			} else {
				PANGO_TTP(layout_minus, "-");
			}
			pango_layout_get_pixel_size(layout_minus, &minus_w, &minus_h);

			w = minus_w + 1;
			uh = minus_h / 2 + minus_h % 2;
			dh = minus_h / 2;
			
			ips_n.wrap = m[0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
			cairo_surface_t *surface_arg = draw_structure(m[0], po, ips_n, &ctmp, scaledown, color);
			if(!surface_arg) {
				g_object_unref(layout_minus);
				return NULL;
			}
			wtmp = cairo_image_surface_get_width(surface_arg) / scalefactor;
			htmp = cairo_image_surface_get_height(surface_arg) / scalefactor;
			hpa = htmp;
			cpa = ctmp;
			//wpa = wtmp;
			w += wtmp;
			if(ctmp > dh) {
				dh = ctmp;
			}
			if(htmp - ctmp > uh) {
				uh = htmp - ctmp;
			}				
			
			h = uh + dh;
			central_point = dh;

			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			
			w = 0;
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, w, uh - minus_h / 2 - minus_h % 2);
			pango_cairo_show_layout(cr, layout_minus);
			w += minus_w + 1;
			cairo_set_source_surface(cr, surface_arg, w, uh - (hpa - cpa));
			cairo_paint(cr);
			cairo_surface_destroy(surface_arg);

			g_object_unref(layout_minus);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			ips_n.depth++;
			
			vector<cairo_surface_t*> surface_terms;
			vector<gint> hpt;
			vector<gint> wpt;
			vector<gint> cpt;
			gint mul_w, mul_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0;
			
			CALCULATE_SPACE_W
			PangoLayout *layout_mul = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) {
				TTP_SMALL(str, SIGN_MULTIDOT);
			} else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) {
				TTP_SMALL(str, SIGN_MIDDLEDOT);
			} else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) {
				TTP_SMALL(str, SIGN_MULTIPLICATION);
			} else {
				TTP(str, "*");
			}
			pango_layout_set_markup(layout_mul, str.c_str(), -1);
			pango_layout_get_pixel_size(layout_mul, &mul_w, &mul_h);
			bool par_prev = false;
			vector<int> nm;
			for(size_t i = 0; i < m.size(); i++) {
				hetmp = 0;		
				ips_n.wrap = m[i].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_terms.push_back(draw_structure(m[i], po, ips_n, &hetmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					for(size_t i = 0; i < surface_terms.size(); i++) {
						if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
					}
					g_object_unref(layout_mul);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_terms[i]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_terms[i]) / scalefactor;
				hpt.push_back(htmp);
				cpt.push_back(hetmp);
				wpt.push_back(wtmp);
				w += wtmp;
				if(!po.short_multiplication && i > 0) {
					w += mul_w + space_w * 2;
					if(mul_h / 2 > dh) {
						dh = mul_h / 2;
					}
					if(mul_h / 2 + mul_h % 2 > uh) {
						uh = mul_h / 2 + mul_h % 2;
					}
					nm.push_back(-1);
				} else if(i > 0) {
					nm.push_back(m[i].neededMultiplicationSign(po, ips_n, m, i + 1, ips_n.wrap || (m[i].isPower() && m[i][0].needsParenthesis(po, ips_n, m[i], 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0)), par_prev, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0));
					switch(nm[i]) {
						case MULTIPLICATION_SIGN_SPACE: {
							w += space_w;
							break;
						}
						case MULTIPLICATION_SIGN_OPERATOR: {
							w += mul_w + space_w * 2;
							if(mul_h / 2 > dh) {
								dh = mul_h / 2;
							}
							if(mul_h / 2 + mul_h % 2 > uh) {
								uh = mul_h / 2 + mul_h % 2;
							}
							break;
						}
						case MULTIPLICATION_SIGN_OPERATOR_SHORT: {
							w += mul_w;
							if(mul_h / 2 > dh) {
								dh = mul_h / 2;
							}
							if(mul_h / 2 + mul_h % 2 > uh) {
								uh = mul_h / 2 + mul_h % 2;
							}
							break;
						}
						default: {
							if(m[i - 1].isNumber()) w++;
						}
					}
				} else {
					nm.push_back(-1);
				}
				if(htmp - hetmp > uh) {
					uh = htmp - hetmp;
				}
				if(hetmp > dh) {
					dh = hetmp;
				}
				par_prev = ips_n.wrap;		
			}
			central_point = dh;
			h = dh + uh;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			w = 0;
			for(size_t i = 0; i < surface_terms.size(); i++) {
				if(!CALCULATOR->aborted()) {
					gdk_cairo_set_source_rgba(cr, color);
					if(!po.short_multiplication && i > 0) {
						w += space_w;
						cairo_move_to(cr, w, uh - mul_h / 2 - mul_h % 2);
						pango_cairo_show_layout(cr, layout_mul);
						w += mul_w;
						w += space_w;
					} else if(i > 0) {
						switch(nm[i]) {
							case MULTIPLICATION_SIGN_SPACE: {
								w += space_w;
								break;
							}
							case MULTIPLICATION_SIGN_OPERATOR: {
								w += space_w;
								cairo_move_to(cr, w, uh - mul_h / 2 - mul_h % 2);
								pango_cairo_show_layout(cr, layout_mul);
								w += mul_w;
								w += space_w;
								break;
							}
							case MULTIPLICATION_SIGN_OPERATOR_SHORT: {
								cairo_move_to(cr, w, uh - mul_h / 2 - mul_h % 2);
								pango_cairo_show_layout(cr, layout_mul);
								w += mul_w;
								break;
							}
							default: {
								if(m[i - 1].isNumber()) w++;
							}
						}
					}
					cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
					cairo_paint(cr);
					w += wpt[i];
				}
				cairo_surface_destroy(surface_terms[i]);
			}
			g_object_unref(layout_mul);
			break;
		}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {

			ips_n.depth++;
			ips_n.division_depth++;
			
			gint den_uh, den_w, den_dh, num_w, num_dh, num_uh, dh = 0, uh = 0, w = 0, h = 0, one_w = 0, one_h = 0;
			
			bool flat = ips.division_depth > 0 || ips.power_depth > 0;
			if(!flat && po.place_units_separately) {
				flat = true;
				size_t i = 0;
				if(m.isDivision()) {
					i = 1;
				}
				if(m[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < m[i].size(); i2++) {
						if(!m[i][i2].isUnit_exp()) {
							flat = false;
							break;
						}
					}
				} else if(!m[i].isUnit_exp()) {
					flat = false;
				}
				if(flat) {
					ips_n.division_depth--;
				}
			}
			cairo_surface_t *num_surface = NULL, *den_surface = NULL, *surface_one = NULL;
			if(m.type() == STRUCT_DIVISION) {
				ips_n.wrap = (!m[0].isDivision() || !flat || ips.division_depth > 0 || ips.power_depth > 0) && m[0].needsParenthesis(po, ips_n, m, 1, flat, ips.power_depth > 0);
				num_surface = draw_structure(m[0], po, ips_n, &num_dh, scaledown, color);
				if(!num_surface) {
					return NULL;
				}
				num_w = cairo_image_surface_get_width(num_surface) / scalefactor;
				h = cairo_image_surface_get_height(num_surface) / scalefactor;
				num_uh = h - num_dh;
			} else {
				MathStructure onestruct(1, 1);
				ips_n.wrap = false;
				surface_one = draw_structure(onestruct, po, ips_n, NULL, scaledown, color);
				if(!surface_one) {
					return NULL;
				}
				one_w = cairo_image_surface_get_width(surface_one) / scalefactor;
				one_h = cairo_image_surface_get_height(surface_one) / scalefactor;
				num_w = one_w; num_dh = one_h / 2; num_uh = one_h - num_dh;
			}
			if(m.type() == STRUCT_DIVISION) {
				ips_n.wrap = m[1].needsParenthesis(po, ips_n, m, 2, flat, ips.power_depth > 0);
				den_surface = draw_structure(m[1], po, ips_n, &den_dh, scaledown, color);
			} else {
				ips_n.wrap = m[0].needsParenthesis(po, ips_n, m, 2, flat, ips.power_depth > 0);
				den_surface = draw_structure(m[0], po, ips_n, &den_dh, scaledown, color);
			}
			if(!den_surface) {
				if(num_surface) cairo_surface_destroy(num_surface);
				if(surface_one) cairo_surface_destroy(surface_one);
				return NULL;
			}
			den_w = cairo_image_surface_get_width(den_surface) / scalefactor;
			h = cairo_image_surface_get_height(den_surface) / scalefactor;
			den_uh = h - den_dh;
			h = 0;
			if(flat) {
				gint div_w, div_h;
				PangoLayout *layout_div = gtk_widget_create_pango_layout(resultview, NULL);
				CALCULATE_SPACE_W
				if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
					PANGO_TTP(layout_div, SIGN_DIVISION);
				} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
					PANGO_TTP(layout_div, SIGN_DIVISION_SLASH);
				} else {
					PANGO_TTP(layout_div, "/");
				}
				pango_layout_get_pixel_size(layout_div, &div_w, &div_h);
				w = num_w + den_w + space_w + space_w + div_w;
				dh = num_dh; uh = num_uh;
				if(den_dh > dh) h = den_dh;
				if(den_uh > uh) uh = den_uh;
				if(div_h / 2 > dh) {
					dh = div_h / 2;
				}
				if(div_h / 2 + div_h % 2 > uh) {
					uh = div_h / 2 + div_h % 2;
				}
				h = uh + dh;
				central_point = dh;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				w = 0;
				if(m.type() == STRUCT_DIVISION) {
					cairo_set_source_surface(cr, num_surface, w, uh - num_uh);
					cairo_paint(cr);
				} else {
					cairo_set_source_surface(cr, surface_one, w, uh - one_h / 2 - one_h % 2);
					cairo_paint(cr);
				}
				w += num_w;
				w += space_w;
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, w, uh - div_h / 2 - div_h % 2);
				pango_cairo_show_layout(cr, layout_div);
				w += div_w;
				w += space_w;
				cairo_set_source_surface(cr, den_surface, w, uh - den_uh);
				cairo_paint(cr);
				g_object_unref(layout_div);
			} else {
				gint wfr;
				dh = den_dh + den_uh + 3;
				uh = num_dh + num_uh + 3;
				wfr = den_w;
				if(num_w > wfr) wfr = num_w;
				wfr += 2;
				w = wfr;
				h = uh + dh;
				central_point = dh;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				w = 0;
				if(m.type() == STRUCT_DIVISION) {
					cairo_set_source_surface(cr, num_surface, w + (wfr - num_w) / 2, uh - 3 - num_uh - num_dh);
					cairo_paint(cr);
				} else {
					cairo_set_source_surface(cr, surface_one, w + (wfr - one_w) / 2, uh - 3 - one_h);
					cairo_paint(cr);
				}
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to (cr, w, uh - 1);
				cairo_line_to (cr, w + wfr, uh - 1);
				cairo_set_line_width(cr, 2);
				cairo_stroke(cr);
				cairo_set_source_surface(cr, den_surface, w + (wfr - den_w) / 2, uh + 3);
				cairo_paint(cr);
			}
			if(num_surface) cairo_surface_destroy(num_surface);
			if(den_surface) cairo_surface_destroy(den_surface);
			if(surface_one) cairo_surface_destroy(surface_one);
			break;
		}
		case STRUCT_POWER: {

			ips_n.depth++;
			
			gint base_w, base_h, exp_w, exp_h, w = 0, h = 0, ctmp = 0;
			CALCULATE_SPACE_W
			ips_n.wrap = m[0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0, false);
			cairo_surface_t *surface_base = draw_structure(m[0], po, ips_n, &central_point, scaledown, color);
			if(!surface_base) {
				return NULL;
			}
			base_w = cairo_image_surface_get_width(surface_base) / scalefactor;
			base_h = cairo_image_surface_get_height(surface_base) / scalefactor;
			
			ips_n.power_depth++;
			ips_n.wrap = false;
			PrintOptions po2 = po;
			po2.show_ending_zeroes = false;
			cairo_surface_t *surface_exp = draw_structure(m[1], po2, ips_n, &ctmp, scaledown, color);
			if(!surface_exp) {
				cairo_surface_destroy(surface_base);
				return NULL;
			}
			exp_w = cairo_image_surface_get_width(surface_exp) / scalefactor;
			exp_h = cairo_image_surface_get_height(surface_exp) / scalefactor;
			h = base_h;
			w = base_w;
			if(exp_h < h) {
				h += exp_h / 3;
			} else {
				h += exp_h - base_h / 2;
			}
			w += exp_w;
			
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			gdk_cairo_set_source_rgba(cr, color);
			w = 0;
			cairo_set_source_surface(cr, surface_base, w, h - base_h);
			cairo_paint(cr);
			cairo_surface_destroy(surface_base);
			w += base_w;
			gdk_cairo_set_source_rgba(cr, color);
			cairo_set_source_surface(cr, surface_exp, w, 0);
			cairo_paint(cr);
			cairo_surface_destroy(surface_exp);

			break;
		}
		case STRUCT_LOGICAL_AND: {
			if(!po.preserve_format && m.size() == 2 && m[0].isComparison() && m[1].isComparison() && m[0].comparisonType() != COMPARISON_EQUALS && m[0].comparisonType() != COMPARISON_NOT_EQUALS && m[1].comparisonType() != COMPARISON_EQUALS && m[1].comparisonType() != COMPARISON_NOT_EQUALS && m[0][0] == m[1][0]) {
				ips_n.depth++;
			
				vector<cairo_surface_t*> surface_terms;
				vector<gint> hpt;
				vector<gint> wpt;
				vector<gint> cpt;
				gint sign_w, sign_h, sign2_w, sign2_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0;
				CALCULATE_SPACE_W
			
				hetmp = 0;		
				ips_n.wrap = m[0][1].needsParenthesis(po, ips_n, m[0], 2, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_terms.push_back(draw_structure(m[0][1], po, ips_n, &hetmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_terms[0]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_terms[0]) / scalefactor;
				hpt.push_back(htmp);
				cpt.push_back(hetmp);
				wpt.push_back(wtmp);
				w += wtmp;
				if(htmp - hetmp > uh) {
					uh = htmp - hetmp;
				}
				if(hetmp > dh) {
					dh = hetmp;
				}
				hetmp = 0;		
				ips_n.wrap = m[0][0].needsParenthesis(po, ips_n, m[0], 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_terms.push_back(draw_structure(m[0][0], po, ips_n, &hetmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					cairo_surface_destroy(surface_terms[0]);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_terms[1]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_terms[1]) / scalefactor;
				hpt.push_back(htmp);
				cpt.push_back(hetmp);
				wpt.push_back(wtmp);
				w += wtmp;
				if(htmp - hetmp > uh) {
					uh = htmp - hetmp;
				}
				if(hetmp > dh) {
					dh = hetmp;
				}
				hetmp = 0;		
				ips_n.wrap = m[1][1].needsParenthesis(po, ips_n, m[1], 2, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_terms.push_back(draw_structure(m[1][1], po, ips_n, &hetmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					cairo_surface_destroy(surface_terms[0]);
					cairo_surface_destroy(surface_terms[1]);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_terms[2]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_terms[2]) / scalefactor;
				hpt.push_back(htmp);
				cpt.push_back(hetmp);
				wpt.push_back(wtmp);
				w += wtmp;
				if(htmp - hetmp > uh) {
					uh = htmp - hetmp;
				}
				if(hetmp > dh) {
					dh = hetmp;
				}
			
				PangoLayout *layout_sign = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				TTBP(str);
				switch(m[0].comparisonType()) {
					case COMPARISON_LESS: {
						str += "&gt;";
						break;
					}
					case COMPARISON_GREATER: {
						str += "&lt;";
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_GREATER_OR_EQUAL;
						} else {
							str += "&gt;=";
						}
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_LESS_OR_EQUAL;
						} else {
							str += "&lt;=";
						}
						break;
					}
					default: {}
				}
				TTE(str);
				pango_layout_set_markup(layout_sign, str.c_str(), -1);
				pango_layout_get_pixel_size(layout_sign, &sign_w, &sign_h);
				if(sign_h / 2 > dh) {
					dh = sign_h / 2;
				}
				if(sign_h / 2 + sign_h % 2 > uh) {
					uh = sign_h / 2 + sign_h % 2;
				}
				w += sign_w;
				
				PangoLayout *layout_sign2 = gtk_widget_create_pango_layout(resultview, NULL);
				str = "";
				TTBP(str);
				switch(m[1].comparisonType()) {
					case COMPARISON_GREATER: {
						str += "&gt;";
						break;
					}
					case COMPARISON_LESS: {
						str += "&lt;";
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_GREATER_OR_EQUAL;
						} else {
							str += "&gt;=";
						}
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_LESS_OR_EQUAL;
						} else {
							str += "&lt;=";
						}
						break;
					}
					default: {}
				}
				TTE(str);
				pango_layout_set_markup(layout_sign2, str.c_str(), -1);
				pango_layout_get_pixel_size(layout_sign2, &sign2_w, &sign2_h);
				if(sign2_h / 2 > dh) {
					dh = sign2_h / 2;
				}
				if(sign2_h / 2 + sign2_h % 2 > uh) {
					uh = sign2_h / 2 + sign2_h % 2;
				}
				w += sign2_w;
				
			
				w += space_w * (surface_terms.size() - 1) * 2;
			
				central_point = dh;
				h = dh + uh;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cr = cairo_create(surface);
				w = 0;
				for(size_t i = 0; i < surface_terms.size(); i++) {
					gdk_cairo_set_source_rgba(cr, color);
					if(i > 0) {
						w += space_w;
						if(i == 1) {
							cairo_move_to(cr, w, uh - sign_h / 2 - sign_h % 2);
							pango_cairo_show_layout(cr, layout_sign);
							w += sign_w;
						} else {
							cairo_move_to(cr, w, uh - sign2_h / 2 - sign2_h % 2);
							pango_cairo_show_layout(cr, layout_sign2);
							w += sign2_w;
						}
						w += space_w;			
					}
					cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
					cairo_paint(cr);
					w += wpt[i];
					cairo_surface_destroy(surface_terms[i]);
				}
				g_object_unref(layout_sign);
				g_object_unref(layout_sign2);
				break;
			}
		}
		case STRUCT_COMPARISON: {}		
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_XOR: {}
		case STRUCT_BITWISE_OR: {

			ips_n.depth++;
			
			vector<cairo_surface_t*> surface_terms;
			vector<gint> hpt;
			vector<gint> wpt;
			vector<gint> cpt;
			gint sign_w, sign_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0;
			CALCULATE_SPACE_W
			
			for(size_t i = 0; i < m.size(); i++) {
				hetmp = 0;		
				ips_n.wrap = m[i].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_terms.push_back(draw_structure(m[i], po, ips_n, &hetmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					for(size_t i = 0; i < surface_terms.size(); i++) {
						if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
					}
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_terms[i]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_terms[i]) / scalefactor;
				hpt.push_back(htmp);
				cpt.push_back(hetmp);
				wpt.push_back(wtmp);
				w += wtmp;
				if(htmp - hetmp > uh) {
					uh = htmp - hetmp;
				}
				if(hetmp > dh) {
					dh = hetmp;
				}				
			}
			
			PangoLayout *layout_sign = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			TTBP(str);
			if(m.type() == STRUCT_COMPARISON) {
				switch(m.comparisonType()) {
					case COMPARISON_EQUALS: {
						if(ips.depth == 0 && po.use_unicode_signs && ((po.is_approximate && *po.is_approximate) || m.isApproximate()) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_ALMOST_EQUAL;
						} else {
							str += "=";
						}
						break;
					}
					case COMPARISON_NOT_EQUALS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_NOT_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_NOT_EQUAL;
						} else {
							str += "!=";
						}
						break;
					}
					case COMPARISON_GREATER: {
						str += "&gt;";
						break;
					}
					case COMPARISON_LESS: {
						str += "&lt;";
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_GREATER_OR_EQUAL;
						} else {
							str += "&gt;=";
						}
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_LESS_OR_EQUAL;
						} else {
							str += "&lt;=";
						}
						break;
					}
				}
			} else if(m.type() == STRUCT_LOGICAL_AND) {
				if(po.spell_out_logical_operators) str += _("and");
				else str += "&amp;&amp;";
			} else if(m.type() == STRUCT_LOGICAL_OR) {
				if(po.spell_out_logical_operators) str += _("or");
				else str += "||";
			} else if(m.type() == STRUCT_LOGICAL_XOR) {
				str += "XOR";
			} else if(m.type() == STRUCT_BITWISE_AND) {
				str += "&amp;";
			} else if(m.type() == STRUCT_BITWISE_OR) {
				str += "|";
			} else if(m.type() == STRUCT_BITWISE_XOR) {
				str += "XOR";
			}
			
			TTE(str);
			pango_layout_set_markup(layout_sign, str.c_str(), -1);
			pango_layout_get_pixel_size(layout_sign, &sign_w, &sign_h);
			if(sign_h / 2 > dh) {
				dh = sign_h / 2;
			}
			if(sign_h / 2 + sign_h % 2 > uh) {
				uh = sign_h / 2 + sign_h % 2;
			}
			w += sign_w * (m.size() - 1);
			
			w += space_w * (surface_terms.size() - 1) * 2;
			
			central_point = dh;
			h = dh + uh;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			w = 0;
			for(size_t i = 0; i < surface_terms.size(); i++) {
				if(!CALCULATOR->aborted()) {
					gdk_cairo_set_source_rgba(cr, color);
					if(i > 0) {
						w += space_w;
						cairo_move_to(cr, w, uh - sign_h / 2 - sign_h % 2);
						pango_cairo_show_layout(cr, layout_sign);
						w += sign_w;	
						w += space_w;			
					}
					cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
					cairo_paint(cr);
					w += wpt[i];
				}
				cairo_surface_destroy(surface_terms[i]);
			}
			g_object_unref(layout_sign);
			break;
		}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_BITWISE_NOT: {

			ips_n.depth++;

			gint not_w, not_h, uh, dh, h, w, ctmp, htmp, wtmp, hpa, cpa;
			//gint wpa;
			
			PangoLayout *layout_not = gtk_widget_create_pango_layout(resultview, NULL);
			
			if(m.type() == STRUCT_LOGICAL_NOT) {
				PANGO_TTP(layout_not, "!");
			} else {
				PANGO_TTP(layout_not, "~");
			}
			pango_layout_get_pixel_size(layout_not, &not_w, &not_h);

			w = not_w + 1;
			uh = not_h / 2 + not_h % 2;
			dh = not_h / 2;
			
			ips_n.wrap = m[0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
			cairo_surface_t *surface_arg = draw_structure(m[0], po, ips_n, &ctmp, scaledown, color);
			if(!surface_arg) {
				g_object_unref(layout_not);
				return NULL;
			}
			wtmp = cairo_image_surface_get_width(surface_arg) / scalefactor;
			htmp = cairo_image_surface_get_height(surface_arg) / scalefactor;
			hpa = htmp;
			cpa = ctmp;
			//wpa = wtmp;
			w += wtmp;
			if(ctmp > dh) {
				dh = ctmp;
			}
			if(htmp - ctmp > uh) {
				uh = htmp - ctmp;
			}				
			
			h = uh + dh;
			central_point = dh;

			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			
			w = 0;
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, w, uh - not_h / 2 - not_h % 2);
			pango_cairo_show_layout(cr, layout_not);
			w += not_w + 1;
			cairo_set_source_surface(cr, surface_arg, w, uh - (hpa - cpa));
			cairo_paint(cr);
			cairo_surface_destroy(surface_arg);
			
			g_object_unref(layout_not);
			break;
		}
		case STRUCT_VECTOR: {

			ips_n.depth++;
			
			if(m.isMatrix()) {
				if(m[0].size() == 0) {
					PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
					string str;
					TTBP(str)
					str += "[ ]";
					TTE(str)
					pango_layout_set_markup(layout, str.c_str(), -1);
					PangoRectangle rect;
					pango_layout_get_pixel_size(layout, &w, &h);
					pango_layout_get_pixel_extents(layout, &rect, NULL);
					w = rect.width + rect.x;
					w += 1;
					central_point = h / 2;
					surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, 1, 0);
					pango_cairo_show_layout(cr, layout);
					g_object_unref(layout);
					break;
				}
				gint wtmp, htmp, ctmp = 0, w = 0, h = 0;
				CALCULATE_SPACE_W
				vector<gint> col_w;
				vector<gint> row_h;
				vector<gint> row_uh;
				vector<gint> row_dh;									
				vector<vector<gint> > element_w;
				vector<vector<gint> > element_h;
				vector<vector<gint> > element_c;			
				vector<vector<cairo_surface_t*> > surface_elements;
				element_w.resize(m.size());
				element_h.resize(m.size());
				element_c.resize(m.size());						
				surface_elements.resize(m.size());
				PangoLayout *layout_comma = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				gint comma_w = 0, comma_h = 0;
				TTP(str, po.comma())
				pango_layout_set_markup(layout_comma, str.c_str(), -1);		
				pango_layout_get_pixel_size(layout_comma, &comma_w, &comma_h);
				for(size_t index_r = 0; index_r < m.size(); index_r++) {
					for(size_t index_c = 0; index_c < m[index_r].size(); index_c++) {
						ctmp = 0;
						ips_n.wrap = m[index_r][index_c].needsParenthesis(po, ips_n, m, index_r + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
						surface_elements[index_r].push_back(draw_structure(m[index_r][index_c], po, ips_n, &ctmp, scaledown, color));
						if(CALCULATOR->aborted()) {
							break;
						}
						wtmp = cairo_image_surface_get_width(surface_elements[index_r][index_c]) / scalefactor;
						htmp = cairo_image_surface_get_height(surface_elements[index_r][index_c]) / scalefactor;
						element_w[index_r].push_back(wtmp);
						element_h[index_r].push_back(htmp);
						element_c[index_r].push_back(ctmp);					
						if(index_r == 0) {
							col_w.push_back(wtmp);
						} else if(wtmp > col_w[index_c]) {
							col_w[index_c] = wtmp;
						}
						if(index_c == 0) {
							row_uh.push_back(htmp - ctmp);
							row_dh.push_back(ctmp);
						} else {
							if(ctmp > row_dh[index_r]) {
								row_dh[index_r] = ctmp;
							}
							if(htmp - ctmp > row_uh[index_r]) {
								row_uh[index_r] = htmp - ctmp;
							}						
						}
					}
					if(CALCULATOR->aborted()) {
						break;
					}
					row_h.push_back(row_uh[index_r] + row_dh[index_r]);
					h += row_h[index_r];
					if(index_r != 0) {
						h += 4;
					}
				}	
				h += 4;
				for(size_t i = 0; i < col_w.size(); i++) {
					w += col_w[i];
					if(i != 0) {
						w += space_w * 2;
					}
				}
	
				gint wlr, wll;
				wll = 10;
				wlr = 10;
			
				w += wlr + 1;
				w += wll + 3;
				central_point = h / 2;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				w = 1;
				cairo_move_to(cr, w, 1);
				cairo_line_to(cr, w, h - 1);
				cairo_move_to(cr, w, 1);
				cairo_line_to(cr, w + 7, 1);
				cairo_move_to(cr, w, h - 1);
				cairo_line_to(cr, w + 7, h - 1);
				cairo_set_line_width(cr, 2);
				cairo_stroke(cr);
				h = 2;
				for(size_t index_r = 0; index_r < surface_elements.size(); index_r++) {
					if(!CALCULATOR->aborted()) {
						gdk_cairo_set_source_rgba(cr, color);
						w = wll + 1;
					}
					for(size_t index_c = 0; index_c < surface_elements[index_r].size(); index_c++) {
						if(!CALCULATOR->aborted()) {
							cairo_set_source_surface(cr, surface_elements[index_r][index_c], w + (col_w[index_c] - element_w[index_r][index_c]), h + row_uh[index_r] - (element_h[index_r][index_c] - element_c[index_r][index_c]));
							cairo_paint(cr);
							w += col_w[index_c];
							if(index_c != m[index_r].size() - 1) {
								w += space_w * 2;
							}
						}
						if(surface_elements[index_r][index_c]) {
							cairo_surface_destroy(surface_elements[index_r][index_c]);
						}
					}
					if(!CALCULATOR->aborted()) {
						h += row_h[index_r];
						h += 4;
					}
				}
				h -= 4;
				h += 2;
				w += wll - 7;
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, w + 7, 1);
				cairo_line_to(cr, w + 7, h - 1);
				cairo_move_to(cr, w, 1);
				cairo_line_to(cr, w + 7, 1);
				cairo_move_to(cr, w, h - 1);
				cairo_line_to(cr, w + 7, h - 1);
				cairo_set_line_width(cr, 2);
				cairo_stroke(cr);
				g_object_unref(layout_comma);
				break;
			}
			
			gint comma_w, comma_h, uh = 0, dh = 0, h = 0, w = 0, ctmp, htmp, wtmp, arc_w, arc_h;
			vector<cairo_surface_t*> surface_args;
			vector<gint> hpa;
			vector<gint> cpa;			
			vector<gint> wpa;
			
			CALCULATE_SPACE_W
			PangoLayout *layout_comma = gtk_widget_create_pango_layout(resultview, NULL);
			string str, func_str;
			TTP(str, CALCULATOR->getComma())
			pango_layout_set_markup(layout_comma, str.c_str(), -1);		
			pango_layout_get_pixel_size(layout_comma, &comma_w, &comma_h);

			if(m.size() == 0) {
				PangoLayout *layout_one = gtk_widget_create_pango_layout(resultview, NULL);
				TTP(str, "1")
				pango_layout_set_markup(layout_one, str.c_str(), -1);
				pango_layout_get_pixel_size(layout_one, &w, &h);
				uh = h / 2 + h % 2;
				dh = h / 2;
				w = 2;
				g_object_unref(layout_one);
			}
			for(size_t index = 0; index < m.size(); index++) {
				ips_n.wrap = m[index].needsParenthesis(po, ips_n, m, index + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_args.push_back(draw_structure(m[index], po, ips_n, &ctmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					for(size_t i = 0; i < surface_args.size(); i++) {
						if(surface_args[i]) cairo_surface_destroy(surface_args[i]);
					}
					g_object_unref(layout_comma);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_args[index]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_args[index]) / scalefactor;
				hpa.push_back(htmp);
				cpa.push_back(ctmp);				
				wpa.push_back(wtmp);
				if(index > 0) {
					w += comma_w;
					w += space_w;
				}				
				w += wtmp;
				if(ctmp > dh) {
					dh = ctmp;
				}
				if(htmp - ctmp > uh) {
					uh = htmp - ctmp;
				}				
			}
		
			if(dh > uh) uh = dh;
			h = uh + dh;
			central_point = dh;
			arc_h = dh * 2;
			arc_w = (int) ::sqrt((double) arc_h * 3);
			w += arc_w * 2;
			w += 1;

			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);

			w = 0;
			cairo_set_source_surface(cr, get_left_parenthesis(arc_w, arc_h, scaledown, color), w, uh - arc_h / 2 - arc_h % 2);
			cairo_paint(cr);
			w += arc_w;
			if(m.size() == 0) w += 2;
			for(size_t index = 0; index < surface_args.size(); index++) {
				if(!CALCULATOR->aborted()) {
					gdk_cairo_set_source_rgba(cr, color);
					if(index > 0) {
						cairo_move_to(cr, w, uh - comma_h / 2 - comma_h % 2);
						pango_cairo_show_layout(cr, layout_comma);
						w += comma_w;
						w += space_w;
					}
					cairo_set_source_surface(cr, surface_args[index], w, uh - (hpa[index] - cpa[index]));
					cairo_paint(cr);
					w += wpa[index];
				}
				cairo_surface_destroy(surface_args[index]);
			}
			cairo_set_source_surface(cr, get_right_parenthesis(arc_w, arc_h, scaledown, color), w, uh - arc_h / 2 - arc_h % 2);
			cairo_paint(cr);
			
			g_object_unref(layout_comma);

			break;
		}
		case STRUCT_UNIT: {

			string str, str2;
			TTBP(str);
			
			const ExpressionName *ename = &m.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, m.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			if(m.prefix()) {
				str += m.prefix()->name(po.abbreviate_names && ename->abbreviation && (ename->suffix || ename->name.find("_") == string::npos), po.use_unicode_signs, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			}
			if(ename->suffix && ename->name.length() > 1) {
				size_t i = ename->name.rfind('_');
				bool b = i == string::npos || i == ename->name.length() - 1 || i == 0;
				size_t i2 = 1;
				if(b) {
					if(is_in(NUMBERS, ename->name[ename->name.length() - 1])) {
						while(ename->name.length() > i2 + 1 && is_in(NUMBERS, ename->name[ename->name.length() - 1 - i2])) {
							i2++;
						}
					}
					str += ename->name.substr(0, ename->name.length() - i2);
				} else {
					str += ename->name.substr(0, i);
				}
				TTBP_SMALL(str);
				str += "<sub>";
				if(b) str += ename->name.substr(ename->name.length() - i2, i2);
				else str += ename->name.substr(i + 1, ename->name.length() - (i + 1));
				str += "</sub>";
				TTE(str);
			} else {
				str += ename->name;
			}
			gsub("_", " ", str);

			TTE(str);
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			pango_layout_set_markup(layout, str.c_str(), -1);
			pango_layout_get_pixel_size(layout, &w, &h);
			central_point = h / 2;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, 0, 0);
			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);
			break;
		}
		case STRUCT_VARIABLE: {

			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			
			if(m.variable() != CALCULATOR->v_i) {
				str = "<i>";
			}
			TTBP(str);
			
			const ExpressionName *ename = &m.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			if(ename->suffix && ename->name.length() > 1) {
				size_t i = ename->name.rfind('_');
				bool b = i == string::npos || i == ename->name.length() - 1 || i == 0;
				size_t i2 = 1;
				if(b) {
					if(is_in(NUMBERS, ename->name[ename->name.length() - 1])) {
						while(ename->name.length() > i2 + 1 && is_in(NUMBERS, ename->name[ename->name.length() - 1 - i2])) {
							i2++;
						}
					}
					str += ename->name.substr(0, ename->name.length() - i2);
				} else {
					str += ename->name.substr(0, i);
				}
				TTBP_SMALL(str);
				str += "<sub>";
				if(b) str += ename->name.substr(ename->name.length() - i2, i2);
				else str += ename->name.substr(i + 1, ename->name.length() - (i + 1));
				str += "</sub>";
				TTE(str);
			} else {
				str += ename->name;
			}
			gsub("_", " ", str);
			
			TTE(str);
			if(m.variable() != CALCULATOR->v_i) {
				str += "</i>";
			} 
			pango_layout_set_markup(layout, str.c_str(), -1);
			PangoRectangle rect;
			pango_layout_get_pixel_size(layout, &w, &h);
			pango_layout_get_pixel_extents(layout, &rect, NULL);
			w = rect.width + rect.x;
			w += 1;
			if(m.variable() == CALCULATOR->v_i) w += 1;
			central_point = h / 2;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, 1, 0);
			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);
			break;
		}
		case STRUCT_FUNCTION: {
		
			if(SHOW_WITH_ROOT_SIGN(m)) {

				ips_n.depth++;
				gint arg_w, arg_h, root_w, root_h, sign_w, sign_h, h, w, ctmp;
				
				int i_root = 2;
				if(m.function() == CALCULATOR->f_root) i_root = m[1].number().intValue();
				else if(m.function() == CALCULATOR->f_cbrt) i_root = 3;
				string root_str;
				TT_XSMALL(root_str, i2s(i_root));
				PangoLayout *layout_root = gtk_widget_create_pango_layout(resultview, NULL);
				pango_layout_set_markup(layout_root, root_str.c_str(), -1);		
				pango_layout_get_pixel_size(layout_root, &root_w, &root_h);

				ips_n.wrap = false;
				cairo_surface_t *surface_arg = draw_structure(m[0], po, ips_n, &ctmp, scaledown, color);
				if(!surface_arg) return NULL;

				arg_w = cairo_image_surface_get_width(surface_arg) / scalefactor;
				arg_h = cairo_image_surface_get_height(surface_arg) / scalefactor;
				
				double divider = 1.0;
				if(ips.power_depth >= 1) divider = 1.5;
				
				gint extra_space = 5;
				if(scaledown == 1) extra_space = 3;
				else if(scaledown > 1) extra_space = 1;
				
				central_point = ctmp + extra_space / divider;
				
				root_w = root_w / divider;
				root_h = root_h / divider;
				sign_w = root_w * 2.6;
				
				if(i_root == 2) {
					sign_h = arg_h + extra_space / divider;
				} else {
					sign_h = root_h * 2.0;
					if(sign_h < arg_h + extra_space / divider) sign_h = arg_h + extra_space / divider;
				}
				
				h = sign_h + extra_space * 2.0 / divider;
				w = arg_w + sign_w * 1.25;
				
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);

				cairo_move_to(cr, 0, h / 2.0 + h / 15.0);
				cairo_line_to(cr, sign_w / 6.0, h / 2.0);
				cairo_line_to(cr, sign_w / 2.2, h - extra_space / divider);
				cairo_line_to(cr, sign_w,  extra_space / divider);
				cairo_line_to(cr, w,  extra_space / divider);
				cairo_set_line_width(cr, 2 / divider);
				cairo_stroke(cr);
				
				if(i_root != 2) {
					cairo_move_to(cr, (sign_w - root_w) / 3.0, ((h / 2.0) - root_h) / 2.0);
					cairo_surface_set_device_scale(surface, scalefactor / divider, scalefactor / divider);
					pango_cairo_show_layout(cr, layout_root);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				}
				
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, 0, 0);
				cairo_set_source_surface(cr, surface_arg, sign_w + 1, h - arg_h - extra_space / divider);
				cairo_paint(cr);
				
				cairo_surface_destroy(surface_arg);
				g_object_unref(layout_root);
				
				break;
				
			} else if(m.function() == CALCULATOR->f_abs && m.size() == 1) {
			
				ips_n.depth++;
				gint arg_w, arg_h, h, w, ctmp;
				
				ips_n.wrap = false;
				cairo_surface_t *surface_arg = draw_structure(m[0], po, ips_n, &ctmp, scaledown, color);
				if(!surface_arg) return NULL;

				arg_w = cairo_image_surface_get_width(surface_arg) / scalefactor;
				arg_h = cairo_image_surface_get_height(surface_arg) / scalefactor;
				
				double divider = 1.0;
				if(ips.power_depth >= 1) divider = 1.5;
				
				gint extra_space = 5;
				if(scaledown == 1) extra_space = 3;
				else if(scaledown > 1) extra_space = 1;
				
				central_point = ctmp + extra_space / divider;
				h = arg_h + extra_space * 2 / divider;
				w = arg_w + extra_space * 2 + extra_space * 2 / divider;
				
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);

				cairo_move_to(cr, (extra_space / divider), extra_space / divider);
				cairo_line_to(cr, (extra_space / divider), h - extra_space / divider);
				cairo_move_to(cr, w - (extra_space / divider), extra_space / divider);
				cairo_line_to(cr, w - (extra_space / divider), h - extra_space / divider);
				cairo_set_line_width(cr, 2 / divider);
				cairo_stroke(cr);
				
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, 0, 0);
				cairo_set_source_surface(cr, surface_arg, (w - arg_w) / 2.0, (h - arg_h) / 2.0);
				cairo_paint(cr);
				
				cairo_surface_destroy(surface_arg);
				
				break;
			}

			ips_n.depth++;
			
			gint comma_w, comma_h, function_w, function_h, uh, dh, h, w, ctmp, htmp, wtmp, arc_w, arc_h;
			vector<cairo_surface_t*> surface_args;
			vector<gint> hpa;
			vector<gint> cpa;			
			vector<gint> wpa;
			
			CALCULATE_SPACE_W
			PangoLayout *layout_comma = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			TTP(str, po.comma())
			pango_layout_set_markup(layout_comma, str.c_str(), -1);		
			pango_layout_get_pixel_size(layout_comma, &comma_w, &comma_h);
			PangoLayout *layout_function = gtk_widget_create_pango_layout(resultview, NULL);
		
			str = "";	
			TTBP(str);
			
			const ExpressionName *ename = &m.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			if(ename->suffix && ename->name.length() > 1) {
				size_t i = ename->name.rfind('_');
				bool b = i == string::npos || i == ename->name.length() - 1 || i == 0;
				size_t i2 = 1;
				if(b) {
					if(is_in(NUMBERS, ename->name[ename->name.length() - 1])) {
						while(ename->name.length() > i2 + 1 && is_in(NUMBERS, ename->name[ename->name.length() - 1 - i2])) {
							i2++;
						}
					}
					str += ename->name.substr(0, ename->name.length() - i2);
				} else {
					str += ename->name.substr(0, i);
				}
				TTBP_SMALL(str);
				str += "<sub>";
				if(b) str += ename->name.substr(ename->name.length() - i2, i2);
				else str += ename->name.substr(i + 1, ename->name.length() - (i + 1));
				str += "</sub>";
				TTE(str);
			} else {
				str += ename->name;
			}
			gsub("_", " ", str);
			
			TTE(str);
			
			pango_layout_set_markup(layout_function, str.c_str(), -1);			
			pango_layout_get_pixel_size(layout_function, &function_w, &function_h);
			w = function_w + 1;
			uh = function_h / 2 + function_h % 2;
			dh = function_h / 2;

			for(size_t index = 0; index < m.size(); index++) {
				ips_n.wrap = m[index].needsParenthesis(po, ips_n, m, index + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				surface_args.push_back(draw_structure(m[index], po, ips_n, &ctmp, scaledown, color));
				if(CALCULATOR->aborted()) {
					for(size_t i = 0; i < surface_args.size(); i++) {
						if(surface_args[i]) cairo_surface_destroy(surface_args[i]);
					}
					g_object_unref(layout_function);
					g_object_unref(layout_comma);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_args[index]) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_args[index]) / scalefactor;
				hpa.push_back(htmp);
				cpa.push_back(ctmp);				
				wpa.push_back(wtmp);
				if(index > 0) {
					w += comma_w;
					w += space_w;
				}				
				w += wtmp;
				if(ctmp > dh) {
					dh = ctmp;
				}
				if(htmp - ctmp > uh) {
					uh = htmp - ctmp;
				}				
			}
			
			if(dh > uh) uh = dh;
			h = uh + dh;
			central_point = dh;
			arc_h = dh * 2;
			arc_w = (int) ::sqrt((double) arc_h * 3);
			w += arc_w * 2;
			w += 1;

			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			
			w = 0;
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, w, uh - function_h / 2 - function_h % 2);
			pango_cairo_show_layout(cr, layout_function);
			w += function_w;
			cairo_set_source_surface(cr, get_left_parenthesis(arc_w, arc_h, scaledown, color), w, uh - arc_h / 2 - arc_h % 2);
			cairo_paint(cr);
			w += arc_w;
			for(size_t index = 0; index < surface_args.size(); index++) {
				if(!CALCULATOR->aborted()) {
					gdk_cairo_set_source_rgba(cr, color);
					if(index > 0) {
						cairo_move_to(cr, w, uh - comma_h / 2 - comma_h % 2);
						pango_cairo_show_layout(cr, layout_comma);
						w += comma_w;
						w += space_w;
					}
					cairo_set_source_surface(cr, surface_args[index], w, uh - (hpa[index] - cpa[index]));
					cairo_paint(cr);
					w += wpa[index];
				}
				cairo_surface_destroy(surface_args[index]);
			}
			cairo_set_source_surface(cr, get_right_parenthesis(arc_w, arc_h, scaledown, color), w, uh - arc_h / 2 - arc_h % 2);
			cairo_paint(cr);
						
			g_object_unref(layout_comma);
			g_object_unref(layout_function);

			break;
		}
		case STRUCT_UNDEFINED: {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			TTP(str, _("undefined"));
			pango_layout_set_markup(layout, str.c_str(), -1);
			PangoRectangle rect;
			pango_layout_get_pixel_size(layout, &w, &h);
			pango_layout_get_pixel_extents(layout, &rect, NULL);
			w = rect.width + rect.x;
			w += 1;
			central_point = h / 2;
			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
			cr = cairo_create(surface);
			gdk_cairo_set_source_rgba(cr, color);
			cairo_move_to(cr, 1, 0);
			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);
			break;
		}
		default: {}
	}
	if(ips.wrap && surface) {
		gint w, h, base_h, base_w;
		base_w = cairo_image_surface_get_width(surface) / scalefactor;
		base_h = cairo_image_surface_get_height(surface) / scalefactor;
		h = base_h;
		w = base_w;
		gint arc_base_h = central_point * 2;
		if(h < arc_base_h) h = arc_base_h;
		gint arc_base_w = (int) ::sqrt((double) arc_base_h * 3);
		//base_h += 4;
		//central_point += 2;
		w += arc_base_w * 2;
		cairo_surface_t *surface_old = surface;
		cairo_destroy(cr);
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cr = cairo_create(surface);
		gdk_cairo_set_source_rgba(cr, color);
		w = 0;
		cairo_set_source_surface(cr, get_left_parenthesis(arc_base_w, arc_base_h, scaledown, color), w, h - arc_base_h);
		cairo_paint(cr);
		w += arc_base_w;
		cairo_set_source_surface(cr, surface_old, w, (h - base_h) / 2);
		cairo_paint(cr);
		cairo_surface_destroy(surface_old);
		w += base_w;
		cairo_set_source_surface(cr, get_right_parenthesis(arc_base_w, arc_base_h, scaledown, color), w, h - arc_base_h);
		cairo_paint(cr);
	}
	if(ips.depth == 0 && !(m.isComparison() && (!((po.is_approximate && *po.is_approximate) || m.isApproximate()) || (m.comparisonType() == COMPARISON_EQUALS && po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))))) && surface) {
		gint w, h, wle, hle, w_new, h_new;
		w = cairo_image_surface_get_width(surface) / scalefactor;
		h = cairo_image_surface_get_height(surface) / scalefactor;
		cairo_surface_t *surface_old = surface;
		PangoLayout *layout_equals = gtk_widget_create_pango_layout(resultview, NULL);
		if((po.is_approximate && *po.is_approximate) || m.isApproximate()) {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) {
				PANGO_TT(layout_equals, SIGN_ALMOST_EQUAL);
			} else {
				
				string str;
				TT(str, _("approx."));
				pango_layout_set_markup(layout_equals, str.c_str(), -1);
			}
		} else {
			PANGO_TT(layout_equals, "=");
		}
		CALCULATE_SPACE_W
		pango_layout_get_pixel_size(layout_equals, &wle, &hle);
		w_new = w + wle + 1 + space_w;
		h_new = h;
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w_new * scalefactor, h_new * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cr = cairo_create(surface);
		gdk_cairo_set_source_rgba(cr, color);
		cairo_move_to(cr, 1, h - central_point - hle / 2 - hle % 2);
		pango_cairo_show_layout(cr, layout_equals);
		cairo_set_source_surface(cr, surface_old, wle + 1 + space_w, 0);
		cairo_paint(cr);
		cairo_surface_destroy(surface_old);
		g_object_unref(layout_equals);
	}
	if(!surface) {
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1 * scalefactor, 1 * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cr = cairo_create(surface);
	}
	if(cr) cairo_destroy(cr);
	if(point_central) *point_central = central_point;
	return surface;
}

void clearresult() {
	showing_first_time_message = false;
	if(displayed_mstruct) {
		displayed_mstruct->unref();
		displayed_mstruct = NULL;
		if(!surface_result) gtk_widget_queue_draw(resultview);
	}	
	number_map.clear();
	number_exp_map.clear();
	number_exp_minus_map.clear();
	number_approx_map.clear();
	if(gtk_revealer_get_child_revealed(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")))) {
		gtk_info_bar_response(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_RESPONSE_CLOSE);
	}
	update_expression_icons();
	if(surface_result) {
		cairo_surface_destroy(surface_result);
		surface_result = NULL;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);
		gtk_widget_queue_draw(resultview);
	}
	gtk_widget_set_tooltip_text(resultview, "");
}

void on_abort_display(GtkDialog*, gint, gpointer) {
	CALCULATOR->abort();
}

void ViewThread::run() {

	while(true) {
		int scale_tmp = 0;
		if(!read(&scale_tmp)) break;
		void *x = NULL;
		if(!read(&x)) break;
		MathStructure m(*((MathStructure*) x));
		bool b_stack = false;
		if(!read(&b_stack)) break;
		if(!read(&x)) break;
		MathStructure *mm = (MathStructure*) x;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		printops.can_display_unicode_string_arg = (void*) historyview;
		bool b_puup = printops.use_unit_prefixes;
		if(x) {
			PrintOptions po;
			po.preserve_format = true;
			po.show_ending_zeroes = true;
			po.lower_case_e = printops.lower_case_e;
			po.lower_case_numbers = printops.lower_case_numbers;
			po.base_display = printops.base_display;
			po.abbreviate_names = false;
			po.use_unicode_signs = printops.use_unicode_signs;
			po.multiplication_sign = printops.multiplication_sign;
			po.division_sign = printops.division_sign;
			po.short_multiplication = false;
			po.excessive_parenthesis = true;
			po.improve_division_multipliers = false;
			po.can_display_unicode_string_function = &can_display_unicode_string_function;
			po.can_display_unicode_string_arg = (void*) statuslabel_l;
			po.spell_out_logical_operators = printops.spell_out_logical_operators;
			po.restrict_to_parent_precision = false;
			MathStructure mp(*((MathStructure*) x));
			if(!read(&po.is_approximate)) break;
			mp.format(po);
			parsed_text = mp.print(po);
			if(!read(&x)) break;
			mp.set(*((MathStructure*) x));
			if(!mp.isUndefined()) {
				parsed_text += CALCULATOR->localToString();
				mp.format(po);
				parsed_text += mp.print(po); 
				printops.use_unit_prefixes = true;
			}
		}			
		printops.allow_non_usable = false;

		if(mm && m.isMatrix()) {
			mm->set(m);
			MathStructure mm2(m);
			string mstr;
			int c = mm->columns(), r = mm->rows();
			for(int index_r = 0; index_r < r; index_r++) {
				for(int index_c = 0; index_c < c; index_c++) {
					mm->getElement(index_r + 1, index_c + 1)->setAborted();
				}
			}
			for(int index_r = 0; index_r < r; index_r++) {
				for(int index_c = 0; index_c < c; index_c++) {
					mm2.getElement(index_r + 1, index_c + 1)->format(printops);
					mstr = mm2.getElement(index_r + 1, index_c + 1)->print(printops);
					mm->getElement(index_r + 1, index_c + 1)->set(mstr);
				}
			}
		}

		m.format(printops);
		gint64 time1 = g_get_monotonic_time();
		result_text = m.print(printops);
		result_text_approximate = *printops.is_approximate;
		if(!b_stack && g_get_monotonic_time() - time1 < 200000) {
			PrintOptions printops_long = printops;
			printops_long.abbreviate_names = false; 
			printops_long.short_multiplication = false;
			printops_long.excessive_parenthesis = true;
			printops_long.is_approximate = NULL;
			result_text_long = m.print(printops_long);
		} else if(!b_stack) {
			result_text_long = "";
		}
		printops.can_display_unicode_string_arg = NULL;
		
		result_too_long = false;
		if(!b_stack && result_text.length() > 900) {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			int index = 0;
			//GtkTextView can not display very very long lines
			while(result_text.length() - index > 2500) {
				size_t index_utf8 = (index == 0 ? 2498 : 2500) + index;
				size_t space_index = 0;
				while(space_index < 100 && result_text[index_utf8 - space_index] != ' ') {
					space_index++;
				}
				if(space_index < 100) {
					index_utf8 -= space_index;
				} else {
					while(result_text[index_utf8] < 0 && (unsigned char) result_text[index_utf8] < 0xC2) {
						index_utf8--;
					}
				}
				result_text.insert(index_utf8, "\n");
				index = index_utf8;
			}
			result_too_long = true;
			pango_layout_set_markup(layout, _("result is too long\nsee history"), -1);
			gint w = 0, h = 0;
			pango_layout_get_pixel_size(layout, &w, &h);
			gint scalefactor = gtk_widget_get_scale_factor(expressiontext);
			tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(tmp_surface, scalefactor, scalefactor);
			cairo_t *cr = cairo_create(tmp_surface);
			GdkRGBA rgba;
			gtk_style_context_get_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview), &rgba);
			gdk_cairo_set_source_rgba(cr, &rgba);
			pango_cairo_show_layout(cr, layout);
			cairo_destroy(cr);
			g_object_unref(layout);
			*printops.is_approximate = false;
			if(displayed_mstruct) {
				displayed_mstruct->unref();
				displayed_mstruct = NULL;
			}
		} else if(!b_stack && m.isAborted()) {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			pango_layout_set_markup(layout, _("calculation was aborted"), -1);
			gint w = 0, h = 0;
			pango_layout_get_pixel_size(layout, &w, &h);
			gint scalefactor = gtk_widget_get_scale_factor(expressiontext);
			tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_t *cr = cairo_create(tmp_surface);
			GdkRGBA rgba;
			gtk_style_context_get_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview), &rgba);
			gdk_cairo_set_source_rgba(cr, &rgba);
			pango_cairo_show_layout(cr, layout);
			cairo_destroy(cr);
			g_object_unref(layout);
			*printops.is_approximate = false;
			if(displayed_mstruct) displayed_mstruct->unref();
			displayed_mstruct = new MathStructure(m);
		} else if(!b_stack) {
			if(!CALCULATOR->aborted()) {
				printops.allow_non_usable = true;
				printops.can_display_unicode_string_arg = (void*) resultview;

				MathStructure *displayed_mstruct_pre = new MathStructure(m);
				tmp_surface = draw_structure(*displayed_mstruct_pre, printops, top_ips, NULL, scale_tmp);
				if(displayed_mstruct) displayed_mstruct->unref();
				displayed_mstruct = displayed_mstruct_pre;
				if(tmp_surface && CALCULATOR->aborted()) {
					cairo_surface_destroy(tmp_surface);
					tmp_surface = NULL;
				}
			
				printops.can_display_unicode_string_arg = NULL;
				printops.allow_non_usable = false;
			}
			if(!tmp_surface && displayed_mstruct) {
				displayed_mstruct->unref();
				displayed_mstruct = NULL;
			}
		}
		printops.use_unit_prefixes = b_puup;
		b_busy = false;
		CALCULATOR->stopControl();
	}
}
gboolean on_event(GtkWidget*, GdkEvent *e, gpointer) {
	if(e->type == GDK_EXPOSE || e->type == GDK_PROPERTY_NOTIFY || e->type == GDK_CONFIGURE || e->type == GDK_FOCUS_CHANGE || e->type == GDK_VISIBILITY_NOTIFY) {
		return FALSE;
	}
	return TRUE;
}

/*
	set result in result widget and add to history widget
*/
void setResult(Prefix *prefix, bool update_history, bool update_parse, bool force, string transformation, size_t stack_index, bool register_moved) {

	if(block_result_update || exit_in_progress) return;
	
	if(expression_has_changed && (!rpn_mode || CALCULATOR->RPNStackSize() == 0)) {
		if(!force) return;
		execute_expression();
		if(!prefix) return;
	}
	
	if(rpn_mode && CALCULATOR->RPNStackSize() == 0) return;
	
	if(nr_of_new_expressions == 0 && !update_parse && !register_moved && update_history) {
		update_history = false;
	}	
	
	if(b_busy || b_busy_result || b_busy_expression || b_busy_command) return;

	if(!rpn_mode) stack_index = 0;
	if(stack_index != 0) {
		update_history = true;
		update_parse = false;
	}
	if(register_moved) {
		update_history = true;
		update_parse = false;
	}

	do_timeout = false;
	b_busy = true;
	b_busy_result = true;
	display_aborted = false;
	
	if(!view_thread->running && !view_thread->start()) {
		b_busy = false; 
		b_busy_result = false;
		do_timeout = true;
		return;
	}

	GtkTreeIter history_iter;

	int inhistory_index = 0;
	if(update_history) {
		if(update_parse || register_moved || current_inhistory_index < 0) {
			if(register_moved) {
				result_text = _("RPN Register Moved");
				inhistory_type.push_back(QALCULATE_HISTORY_REGISTER_MOVED);
				inhistory.push_back("");
			} else {
				remove_blank_ends(result_text);
				gsub("\n", " ", result_text);
				if(result_text == _("RPN Operation")) {
					inhistory_type.push_back(QALCULATE_HISTORY_RPN_OPERATION);
					inhistory.push_back("");
				} else {
					inhistory_type.push_back(QALCULATE_HISTORY_EXPRESSION);
					inhistory.push_back(result_text);
				}
			}
			nr_of_new_expressions++;
			gtk_list_store_insert_with_values(historystore, &history_iter, 0, 0, fix_history_string(result_text).c_str(), 1, inhistory.size() - 1, 2, i2s(nr_of_new_expressions).c_str(), 3, nr_of_new_expressions, 4, EXPRESSION_YPAD, -1);
			gtk_list_store_insert_with_values(historystore, NULL, 1, 1, -1, -1);
			history_index = 0;			
			inhistory_index = inhistory.size() - 1;
			history_parsed.push_back(NULL);
			history_answer.push_back(NULL);
		} else if(current_inhistory_index >= 0) {
			inhistory_index = current_inhistory_index;
			if(!transformation.empty()) {
				string history_str = "<span font-style=\"italic\">";
				history_str += fix_history_string(transformation);
				history_str += ":  </span>";				
				history_index++;
				gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, -1);
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
			}
		} else {
			b_busy = false;
			b_busy_result = false;
			do_timeout = true;
			return;
		}
		result_text = "?";
	}
	
	if(update_parse) {
		parsed_text = "aborted";
	}

	if(stack_index == 0) {
		clearresult();
	}
	
	update_expression_icons(EXPRESSION_STOP);

	scale_n = 0;

	gint w = 0, h = 0;
	bool parsed_approx = false;
	bool title_set = FALSE;
	
	if(stack_index == 0) {
		if(surface_result) {
			cairo_surface_destroy(surface_result);
		}
		surface_result = NULL;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);
	}

	printops.prefix = prefix;
	tmp_surface = NULL;

	if(!view_thread->write(scale_n)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	if(stack_index == 0) {
		if(!view_thread->write((void *) mstruct)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void *) mreg)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	}
	bool b_stack = stack_index != 0;
	if(!view_thread->write(b_stack)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	if(b_stack) {
		if(!view_thread->write(NULL)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	} else {
		matrix_mstruct->clear();
		if(!view_thread->write((void *) matrix_mstruct)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	}
	if(update_parse) {
		if(!view_thread->write((void *) parsed_mstruct)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write((void *) parsed_approx_p)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
		if(!view_thread->write((void *) parsed_tostruct)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	} else {
		if(!view_thread->write(NULL)) {b_busy = false; b_busy_result = false; do_timeout = true; return;}
	}

	int i = 0;
	while(b_busy && view_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(b_busy && view_thread->running) {
		g_application_mark_busy(g_application_get_default());
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultspinner")));
		gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), _("Processing…"));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		if(unit_builder) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unit_builder, "unit_action_area")), FALSE);
		title_set = TRUE;
	}
	while(b_busy && view_thread->running) {
		while(gtk_events_pending()) gtk_main_iteration();
		sleep_ms(100);
	}
	b_busy = true;
	b_busy_result = true;
	
	if(stack_index == 0) {
		display_aborted = !tmp_surface;
		if(display_aborted) {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			pango_layout_set_markup(layout, _("result processing was aborted"), -1);
			pango_layout_get_pixel_size(layout, &w, &h);
			gint scalefactor = gtk_widget_get_scale_factor(expressiontext);
			tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(tmp_surface, scalefactor, scalefactor);
			cairo_t *cr = cairo_create(tmp_surface);
			GdkRGBA rgba;
			gtk_style_context_get_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview), &rgba);
			gdk_cairo_set_source_rgba(cr, &rgba);
			pango_cairo_show_layout(cr, layout);
			cairo_destroy(cr);
			g_object_unref(layout);
			*printops.is_approximate = false;
		}
	}
	
	update_expression_icons();
	if(title_set) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(unit_builder) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unit_builder, "unit_action_area")), TRUE);
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), _("Qalculate!"));
	}

	if(stack_index == 0) {
		if(title_set) {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultspinner")));
			gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
			g_application_unmark_busy(g_application_get_default());
			while(gtk_events_pending()) gtk_main_iteration();
		}
		surface_result = NULL;
		if(tmp_surface) {
			showing_first_time_message = FALSE;
			first_draw_of_result = TRUE;
			surface_result = tmp_surface;
			gtk_widget_queue_draw(resultview);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), displayed_mstruct && !display_aborted);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), displayed_mstruct && !display_aborted);
		}
	}
	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}
	if(current_inhistory_index < 0) {
		update_parse = true;
		current_inhistory_index = 0;
	}
	bool do_scroll = false;
	if(stack_index != 0) {
		if(result_text.length() > 500000) {
			result_text = "(…)";
		}		
		RPNRegisterChanged(result_text, stack_index);
		display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), NULL, message_type == 0 ? 0 : 2);
	} else if(update_history) {
		if(result_text.length() > 500000) {
			result_text = "(…)";
		}
		if(parsed_text.length() > 500000) {
			parsed_text = "(…)";
		}
		if(update_parse) {
			gchar *expr_str = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 0, &expr_str, -1);
			string str = expr_str;
			g_free(expr_str);
			str += "<span font-style=\"italic\" foreground=\"";
			str += history_parse_color;
			str += "\">  ";
			if(!parsed_approx) {
				str += "=";
				inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_PARSE);
			} else {
				if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) historyview)) {
					str += SIGN_ALMOST_EQUAL;
				} else {
					str += _("approx.");
				}
				inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_PARSE_APPROXIMATE);
			}
			str += " ";
			str += fix_history_string(parsed_text);
			str += "</span>";
			
			inhistory.insert(inhistory.begin() + inhistory_index, parsed_text);
			if(nr_of_new_expressions > 0 && parsed_mstruct && !history_parsed[nr_of_new_expressions - 1]) {
				history_parsed[nr_of_new_expressions - 1] = new MathStructure(*parsed_mstruct);
			}
			
			gtk_list_store_set(historystore, &history_iter, 0, str.c_str(), 1, inhistory_index + 1, -1);
		}
		int history_index_bak = history_index;
		display_errors(&history_index, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), &inhistory_index, message_type);
		if(rpn_mode && !register_moved) {
			RPNRegisterChanged(result_text, stack_index);
		}

		string str;

		bool b_approx = result_text_approximate || mstruct->isApproximate();		
		if(!b_approx) {
			str = "=";	
		} else {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) historyview)) {
				str = SIGN_ALMOST_EQUAL;		
			} else {
				str = "= ";
				str += _("approx.");
			}
		}
		string history_str;
		if(!update_parse && current_inhistory_index >= 0 && !transformation.empty() && history_index == history_index_bak) {
			gchar *expr_str = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &history_iter, 0, &expr_str, -1);
			history_str = expr_str;
			g_free(expr_str);
		}
		history_str += str;
		history_str += " <span font-weight=\"bold\">";
		history_str += fix_history_string(result_text);
		history_str += "</span>";		
		if(!update_parse && current_inhistory_index >= 0 && !transformation.empty() && history_index_bak == history_index) {
			gtk_list_store_set(historystore, &history_iter, 0, history_str.c_str(), 1, inhistory_index + 1, -1);
		} else {
			history_index++;
			gtk_list_store_insert_with_values(historystore, &history_iter, history_index, 0, history_str.c_str(), 1, inhistory_index, 3, nr_of_new_expressions, 4, 0, -1);
		}
		inhistory.insert(inhistory.begin() + inhistory_index, result_text);
		current_inhistory_index = inhistory_index;
		if(b_approx) {
			inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_RESULT_APPROXIMATE);
		} else {
			inhistory_type.insert(inhistory_type.begin() + inhistory_index, QALCULATE_HISTORY_RESULT);
		}		
		if(nr_of_new_expressions > 0 && mstruct && nr_of_new_expressions <= (int) history_answer.size()) {
			if(!history_answer[nr_of_new_expressions - 1]) history_answer[nr_of_new_expressions - 1] = new MathStructure(*mstruct);
			else history_answer[nr_of_new_expressions - 1]->set(*mstruct);
		}
		
		GtkTreeIter index_iter = history_iter;
		gint index_hi = -1;
		while(gtk_tree_model_iter_previous(GTK_TREE_MODEL(historystore), &index_iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(historystore), &index_iter, 1, &index_hi, -1);
			if(index_hi >= 0) {
				gtk_list_store_set(historystore, &index_iter, 1, index_hi + 1, -1);
			}
		}		
		
		if(result_text.length() < 1000) {
			str += " ";
			if(result_text_long.empty()) {
				str += result_text;
			} else {
				str += result_text_long;
			}
			gtk_widget_set_tooltip_text(resultview, str.length() < 1000 ? str.c_str() : "");
		}
		do_scroll = true;
	} else {
		int history_index_bak = history_index;
		display_errors(&history_index, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), &inhistory_index, message_type);
		do_scroll = (history_index != history_index_bak);
	}
	printops.prefix = NULL;
	b_busy = false;
	b_busy_result = false;
	
	while(gtk_events_pending()) gtk_main_iteration();
	if(do_scroll && gtk_widget_get_realized(historyview)) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(historyview), path, history_index_column, FALSE, 0, 0);
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(historyview), 0, 0);
		gtk_tree_path_free(path);
	}

	if(!register_moved && stack_index == 0 && mstruct->isMatrix() && (mstruct->rows() > 4 || mstruct->columns() > 4) && matrix_mstruct->isMatrix() && matrix_mstruct->columns() < 200) {
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_widget_grab_focus(expressiontext);
		if(update_history && update_parse && force) {
			GtkTextIter istart, iend;
			gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
		}
		insert_matrix(matrix_mstruct, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), false, true, true);
	}
	
	do_timeout = true;

}

void on_abort_command(GtkDialog*, gint, gpointer) {
	CALCULATOR->abort();
	int msecs = 5000;
	while(b_busy && msecs > 0) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(b_busy) {
		command_thread->cancel();
		b_busy = false;
		CALCULATOR->stopControl();
		command_aborted = true;
	}
}

void CommandThread::run() {

	enableAsynchronousCancel();

	while(true) {
		int command_type = 0;
		if(!read(&command_type)) break;
		void *x = NULL;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->factorize(evalops, true, -1, 0);
				}
				break;
			}
			case COMMAND_SIMPLIFY: {
				((MathStructure*) x)->simplify(evalops);
				break;
			}
			case COMMAND_TRANSFORM: {
				string ceu_str;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")))) {
					ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), evalops.parse_options);
					remove_blank_ends(ceu_str);
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {			
						if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-') {
							ceu_str = "?" + ceu_str;
						}
					}
				}
				((MathStructure*) x)->set(CALCULATOR->calculate(*((MathStructure*) x), evalops, ceu_str));
				break;
			}
			case COMMAND_CONVERT_STRING: {
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_units_string, evalops));
				break;
			}
			case COMMAND_CONVERT_UNIT: {
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_unit, evalops, false));
				break;
			}
			case COMMAND_CONVERT_OPTIMAL: {
				((MathStructure*) x)->set(CALCULATOR->convertToBestUnit(*((MathStructure*) x), evalops, true));
				break;
			}
			case COMMAND_CONVERT_BASE: {
				((MathStructure*) x)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x), evalops));
				break;
			}
		}
		b_busy = false;
		CALCULATOR->stopControl();
	}
}

void executeCommand(int command_type, bool show_result = true, string ceu_str = "", Unit *u = NULL, int run = 1) {

	if(exit_in_progress) return;

	if(run == 1) {
		if(expression_has_changed && !rpn_mode && command_type != COMMAND_TRANSFORM) {
			execute_expression();
		}

		if(b_busy || b_busy_result || b_busy_expression || b_busy_command) return;
	
		do_timeout = false;
		b_busy = true;
		b_busy_command = true;
		command_aborted = false;
	
		if(command_type >= COMMAND_CONVERT_UNIT) {
			CALCULATOR->resetExchangeRatesUsed();
			command_convert_units_string = ceu_str;
			command_convert_unit = u;
		}
	}
	
	bool title_set = FALSE;
	update_expression_icons(EXPRESSION_STOP);
	int i = 0;
	
	MathStructure *mfactor = new MathStructure(*mstruct);
	
	rerun_command:

	if(!command_thread->running && !command_thread->start()) {do_timeout = true; b_busy = false; b_busy_command = false; return;}

	if(!command_thread->write(command_type)) {do_timeout = true; b_busy = false; b_busy_command = false; return;}
	if(!command_thread->write((void *) mfactor)) {do_timeout = true; b_busy = false; b_busy_command = false; return;}

	while(b_busy && command_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;
	
	if(b_busy && command_thread->running) {
		string progress_str;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				progress_str = _("Factorizing…");
				break;
			}
			case COMMAND_SIMPLIFY: {
				progress_str = _("Simplifying…");
				break;
			}
			case COMMAND_TRANSFORM: {
				progress_str = _("Calculating…");
				break;
			}
			default: {
				progress_str = _("Converting…");
				break;
			}
		}
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), progress_str.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		if(unit_builder) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unit_builder, "unit_action_area")), FALSE);
		update_expression_icons(EXPRESSION_SPINNER);
		gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_mark_busy(g_application_get_default());
		title_set = TRUE;
	}
	while(b_busy && command_thread->running) {
		while(gtk_events_pending()) gtk_main_iteration();
		sleep_ms(100);
	}
	if(!command_thread->running) command_aborted = true;
	
	if(!command_aborted && run == 1 && command_type >= COMMAND_CONVERT_UNIT && check_exchange_rates()) {
		b_busy = true;
		mfactor->set(*mstruct);
		run = 2;
		goto rerun_command;
	}

	b_busy = false;
	b_busy_command = false;

	update_expression_icons();
	if(title_set) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(unit_builder) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unit_builder, "unit_action_area")), TRUE);
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), _("Qalculate!"));
		gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_unmark_busy(g_application_get_default());
	}

	if(!command_aborted) {
		mstruct->set(*mfactor);
		mfactor->unref();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				printops.allow_factorization = true;
				break;
			}
			case COMMAND_SIMPLIFY: {
				printops.allow_factorization = false;
				break;
			}
			default: {
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
			}
		}
		if(show_result) setResult(NULL, true, false, true, command_type == COMMAND_TRANSFORM ? ceu_str : "");
	}
		
	do_timeout = true;

}

void fetch_exchange_rates(int timeout) {
	FetchExchangeRatesThread fetch_thread;
	if(fetch_thread.start() && fetch_thread.write(timeout)) {
		int i = 0;
		while(fetch_thread.running && i < 50) {
			while(gtk_events_pending()) gtk_main_iteration();
			sleep_ms(10);
			i++;
		}
		if(fetch_thread.running) {
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL), GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, _("Fetching exchange rates."));
			gtk_widget_show(dialog);
			while(fetch_thread.running) {
				while(gtk_events_pending()) gtk_main_iteration();
				sleep_ms(10);
			}
			gtk_widget_destroy(dialog);
		}
	}
}

void FetchExchangeRatesThread::run() {
	int timeout = 15;
	if(!read(&timeout)) return;
	CALCULATOR->fetchExchangeRates(timeout);
}

void result_display_updated() {
	gtk_widget_queue_draw(resultview);
	update_status_text();
}
void result_format_updated() {
	setResult(NULL, true, false, false);
	update_status_text();
}
void result_action_executed() {
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	setResult(NULL, true, false, true);
}
void result_prefix_changed(Prefix *prefix) {
	bool b_use_unit_prefixes = printops.use_unit_prefixes;
	bool b_use_prefixes_for_all_units = printops.use_prefixes_for_all_units;
	if(!prefix) {
		mstruct->unformat(evalops);
		printops.use_unit_prefixes = true; 
		printops.use_prefixes_for_all_units = true;
	}
	setResult(prefix, true, false, true);
	printops.use_unit_prefixes = b_use_unit_prefixes;
	printops.use_prefixes_for_all_units = b_use_prefixes_for_all_units;
	
}
void expression_calculation_updated() {
	expression_has_changed2 = true;
	display_parse_status();
	if(!rpn_mode) execute_expression(false);
	update_status_text();
}
void expression_format_updated(bool recalculate) {
	expression_has_changed2 = true;
	if(rpn_mode) recalculate = false;
	display_parse_status();
	if(!expression_has_changed && !recalculate && !rpn_mode) {
		clearresult();
	}
	if(recalculate) {
		execute_expression(false);
	}
	update_status_text();
}


void on_abort_calculation(GtkDialog*, gint, gpointer) {
	CALCULATOR->abort();
}


void add_to_expression_history(string str) {
	for(size_t i = 0; i < expression_history.size(); i++) {
		if(expression_history[i] == str) {
			expression_history.erase(expression_history.begin() + i);
			break;
		}
	}
	if(expression_history.size() >= 100) {
		expression_history.pop_back();
	}
	expression_history.insert(expression_history.begin(), str);
	expression_history_index = 0;
}

/*
	calculate entered expression and display result
*/
void execute_expression(bool force, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, string execute_str, string str) {

	if(block_expression_execution || exit_in_progress) return;

	string saved_execute_str = execute_str;
	
	if(b_busy || b_busy_result || b_busy_expression || b_busy_command) return;
	
	b_busy = true;
	b_busy_expression = true;

	bool do_factors = false, do_fraction = false;
	if(do_stack && !rpn_mode) do_stack = false;

	if(str.empty()) {
		if(do_stack) {
	  		GtkTreeIter iter;
			gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, stack_index);
			gchar *gstr;
			gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
			str = gstr;
			g_free(gstr);
			do_timeout = false;
		} else {
			GtkTextIter istart, iend;
			gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
			str = gstr;
			g_free(gstr);
			if(!force && (expression_has_changed || str.find_first_not_of(SPACES) == string::npos)) {
				b_busy = false;
				b_busy_expression = false;
				return;
			}
			do_timeout = false;
			expression_has_changed = false;
			if(!do_mathoperation && !str.empty()) add_to_expression_history(str);
		}
	}
	
	string from_str = str, to_str;
	bool b_units_saved = evalops.parse_options.units_enabled;
	evalops.parse_options.units_enabled = true;
	if(execute_str.empty() && CALCULATOR->separateToExpression(from_str, to_str, evalops, true)) {
		if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, _("hexadecimal"))) {
			int save_base = printops.base;
			printops.base = BASE_HEXADECIMAL;
			evalops.parse_options.units_enabled = b_units_saved;
			b_busy = false;
			b_busy_expression = false;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			printops.base = save_base;
			return;
		} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, _("octal"))) {
			int save_base = printops.base;
			printops.base = BASE_OCTAL;
			evalops.parse_options.units_enabled = b_units_saved;
			b_busy = false;
			b_busy_expression = false;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			printops.base = save_base;
			return;
		} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, _("binary"))) {
			int save_base = printops.base;
			printops.base = BASE_BINARY;
			evalops.parse_options.units_enabled = b_units_saved;
			b_busy = false;
			b_busy_expression = false;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			printops.base = save_base;
			return;
		} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, _("bases"))) {
			b_busy = false;
			b_busy_expression = false;
			evalops.parse_options.units_enabled = b_units_saved;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			convert_number_bases(from_str.c_str());
			return;
		} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, _("optimal"))) {
			AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
			evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
			b_busy = false;
			b_busy_expression = false;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			evalops.auto_post_conversion = save_auto_post_conversion;
			evalops.parse_options.units_enabled = b_units_saved;
			return;
		} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, _("base"))) {
			AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
			evalops.auto_post_conversion = POST_CONVERSION_BASE;
			b_busy = false;
			b_busy_expression = false;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			evalops.auto_post_conversion = save_auto_post_conversion;
			evalops.parse_options.units_enabled = b_units_saved;
			return;
		} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, _("mixed"))) {
			AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
			MixedUnitsConversion save_mixed_units_conversion = evalops.mixed_units_conversion;
			evalops.auto_post_conversion = POST_CONVERSION_NONE;
			evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
			b_busy = false;
			b_busy_expression = false;
			execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, from_str);
			evalops.auto_post_conversion = save_auto_post_conversion;
			evalops.mixed_units_conversion = save_mixed_units_conversion;
			evalops.parse_options.units_enabled = b_units_saved;
			return;
		} else if(equalsIgnoreCase(to_str, "fraction") || equalsIgnoreCase(to_str, _("fraction"))) {
			do_fraction = true;
			execute_str = from_str;
		} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, _("factors"))) {
			do_factors = true;
			execute_str = from_str;
		}
	}
	evalops.parse_options.units_enabled = b_units_saved;

	size_t stack_size = 0;
	
	
	update_expression_icons(EXPRESSION_STOP);

	if(execute_str.empty() && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")))) {
		string ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), evalops.parse_options);
		remove_blank_ends(ceu_str);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {			
			if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-') {
				ceu_str = "?" + ceu_str;
			}
		}
		if(ceu_str.empty()) parsed_tostruct->setUndefined();
		else parsed_tostruct->set(ceu_str);
	} else {
		parsed_tostruct->setUndefined();
	}
	CALCULATOR->resetExchangeRatesUsed();
	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, evalops.parse_options), 0, evalops, parsed_mstruct, parsed_tostruct);
	} else if(rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			if(f) CALCULATOR->calculateRPN(f, 0, evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, evalops, parsed_mstruct);
		} else {
			string str2 = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, evalops.parse_options);
			CALCULATOR->parseSigns(str2);
			remove_blank_ends(str2);
			if(str2.length() == 1) {
				do_mathoperation = true;
				switch(str2[0]) {
					case '^': {CALCULATOR->calculateRPN(OPERATION_RAISE, 0, evalops, parsed_mstruct); break;}
					case '+': {CALCULATOR->calculateRPN(OPERATION_ADD, 0, evalops, parsed_mstruct); break;}
					case '-': {CALCULATOR->calculateRPN(OPERATION_SUBTRACT, 0, evalops, parsed_mstruct); break;}
					case '*': {CALCULATOR->calculateRPN(OPERATION_MULTIPLY, 0, evalops, parsed_mstruct); break;}
					case '/': {CALCULATOR->calculateRPN(OPERATION_DIVIDE, 0, evalops, parsed_mstruct); break;}
					case '&': {CALCULATOR->calculateRPN(OPERATION_BITWISE_AND, 0, evalops, parsed_mstruct); break;}
					case '|': {CALCULATOR->calculateRPN(OPERATION_BITWISE_OR, 0, evalops, parsed_mstruct); break;}
					case '~': {CALCULATOR->calculateRPNBitwiseNot(0, evalops, parsed_mstruct); break;}
					case '!': {CALCULATOR->calculateRPN(CALCULATOR->f_factorial, 0, evalops, parsed_mstruct); break;}
					case '>': {CALCULATOR->calculateRPN(OPERATION_GREATER, 0, evalops, parsed_mstruct); break;}
					case '<': {CALCULATOR->calculateRPN(OPERATION_LESS, 0, evalops, parsed_mstruct); break;}
					case '=': {CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, evalops, parsed_mstruct); break;}
					default: {do_mathoperation = false;}
				}
			} else if(str2.length() == 2) {
				if(str2 == "**") {
					CALCULATOR->calculateRPN(OPERATION_RAISE, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!!") {
					CALCULATOR->calculateRPN(CALCULATOR->f_factorial2, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!=" || str == "=!" || str == "<>") {
					CALCULATOR->calculateRPN(OPERATION_NOT_EQUALS, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "<=" || str == "=<") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_LESS, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == ">=" || str == "=>") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_GREATER, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "==") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				}
			}
			if(!do_mathoperation) {
				bool had_nonnum = false, test_function = true;
				int in_par = 0;
				for(size_t i = 0; i < str2.length(); i++) {
					if(is_in(NUMBERS, str2[i])) {
						if(!had_nonnum || in_par) {
							test_function = false;
							break;
						}
					} else if(str2[i] == '(') {
						if(in_par || !had_nonnum) {
							test_function = false;
							break;
						}
						in_par = i;
					} else if(str2[i] == ')') {
						if(i != str2.length() - 1) {
							test_function = false;
							break;
						}
					} else if(str2[i] == ' ') {
						if(!in_par) {
							test_function = false;
							break;
						}
					} else if(is_in(NOT_IN_NAMES, str2[i])) {
						test_function = false;
						break;
					} else {
						if(in_par) {
							test_function = false;
							break;
						}
						had_nonnum = true;
					}
				}
				f = NULL;
				if(test_function) {
					if(in_par) f = CALCULATOR->getActiveFunction(str2.substr(0, in_par));
					else f = CALCULATOR->getActiveFunction(str2);
				}
				if(f && f->minargs() > 0) {
					do_mathoperation = true;
					CALCULATOR->calculateRPN(f, 0, evalops, parsed_mstruct);
				} else {
					CALCULATOR->RPNStackEnter(str2, 0, evalops, parsed_mstruct, parsed_tostruct);
				}
			}			
		}
	} else {
		CALCULATOR->calculate(mstruct, CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, evalops.parse_options), 0, evalops, parsed_mstruct, parsed_tostruct);
	}

	int i = 0;
	while(CALCULATOR->busy() && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;
	//GtkWidget *dialog = NULL;
	bool title_set = FALSE;
	//bool progress_set = FALSE;
	if(CALCULATOR->busy()) {
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), _("Calculating…"));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		if(unit_builder) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unit_builder, "unit_action_area")), FALSE);
		update_expression_icons(EXPRESSION_SPINNER);
		gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_mark_busy(g_application_get_default());
		title_set = TRUE;
	}
	while(CALCULATOR->busy()) {
		while(gtk_events_pending()) gtk_main_iteration();
		sleep_ms(100);
	}
	update_expression_icons();
	if(title_set) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(unit_builder) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unit_builder, "unit_action_area")), TRUE);
		gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), _("Qalculate!"));
		gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_unmark_busy(g_application_get_default());
	}

	b_busy = false;
	b_busy_expression = false;

	if(rpn_mode && (!do_stack || stack_index == 0)) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
	}
	
	//update "ans" variables
	if(!do_stack || stack_index == 0) {
		MathStructure m4(vans[3]->get());
		m4.replace(vans[4], vans[4]->get());
		vans[4]->set(m4);
		MathStructure m3(vans[2]->get());
		m3.replace(vans[3], vans[4]);
		vans[3]->set(m3);
		MathStructure m2(vans[1]->get());
		m2.replace(vans[2], vans[3]);
		vans[2]->set(m2);
		MathStructure m1(vans[0]->get());
		m1.replace(vans[1], vans[2]);
		vans[1]->set(m1);
		mstruct->replace(vans[0], vans[1]);
		vans[0]->set(*mstruct);
	}
	
	if(do_stack && stack_index > 0) {
	} else if(rpn_mode && do_mathoperation) {
		result_text = _("RPN Operation");
	} else {
		result_text = str;
	}
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	if(rpn_mode && (!do_stack || stack_index == 0)) {
		clear_expression_text();
		while(CALCULATOR->RPNStackSize() < stack_size) {
			RPNRegisterRemoved(1);
			stack_size--;
		}
		if(CALCULATOR->RPNStackSize() > stack_size) {
			RPNRegisterAdded("");
		}
	}
	
	
	if(rpn_mode && do_mathoperation && parsed_tostruct && !parsed_tostruct->isUndefined() && parsed_tostruct->isSymbolic()) {
		mstruct->set(CALCULATOR->convert(*mstruct, parsed_tostruct->symbol(), evalops));
	}

	if(!do_mathoperation && check_exchange_rates()) {
		execute_expression(force, do_mathoperation, op, f, rpn_mode, do_stack ? stack_index : 0, saved_execute_str, str);
		return;
	}
	
	if(do_factors) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			executeCommand(COMMAND_FACTORIZE, false);
			mstruct = save_mstruct;
		} else {
			executeCommand(COMMAND_FACTORIZE, false);
		}
	}
	
	if(do_fraction) {
		NumberFractionFormat save_format = printops.number_fraction_format;
		if(((!do_stack || stack_index == 0) && mstruct->isNumber()) || (do_stack && stack_index != 0 && CALCULATOR->getRPNRegister(stack_index + 1)->isNumber())) printops.number_fraction_format = FRACTION_COMBINED;
		else printops.number_fraction_format = FRACTION_FRACTIONAL;
		setResult(NULL, true, (!do_stack || stack_index == 0), true, "", do_stack ? stack_index : 0);
		printops.number_fraction_format = save_format;
	} else {
		setResult(NULL, true, (!do_stack || stack_index == 0), true, "", do_stack ? stack_index : 0);
	}

	if(!do_stack || stack_index == 0) {
		Unit *u = CALCULATOR->findMatchingUnit(*mstruct);
		if(u && !u->category().empty()) {
			string s_cat = u->category();
			if(s_cat.empty()) s_cat = _("Uncategorized");
			if(s_cat != selected_unit_category) {
				GtkTreeIter iter = convert_category_map[s_cat];
				GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
				gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tUnitSelectorCategories), path);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tUnitSelectorCategories), path, NULL, TRUE, 0.5, 0);
				gtk_tree_path_free(path);							
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
			}
		}
		if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")))) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)));
		}
		gtk_widget_grab_focus(expressiontext);
		GtkTextIter istart, iend;
		gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
		gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
		gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);

	}
	do_timeout = true;

}
void set_rpn_mode(bool b) {
	if(b == rpn_mode) return;
	rpn_mode = b;
	update_expression_icons();
	if(rpn_mode) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), _("Ent"));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), _("Calculate expression and add to stack"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "expression_button_equals")), _("Ent"));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Calculate expression and add to stack"));
		gtk_widget_show(expander_stack);
		show_history = gtk_expander_get_expanded(GTK_EXPANDER(expander_history));
		show_keypad = gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad));
		show_convert = gtk_expander_get_expanded(GTK_EXPANDER(expander_convert));
		if(show_stack) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), TRUE);
		}
		expression_has_changed = true;
		expression_has_changed2 = true;
		expression_history_index = -1;
		clearresult();
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), _("="));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), _("Calculate expression"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "expression_button_equals")), _("="));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), _("Calculate expression"));
		gtk_widget_hide(expander_stack);
		show_stack = gtk_expander_get_expanded(GTK_EXPANDER(expander_stack));
		if(show_stack) {
			if(show_history) gtk_expander_set_expanded(GTK_EXPANDER(expander_history), TRUE);
			else if(show_keypad) gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), TRUE);
			else if(show_convert) gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
			else gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), FALSE);
		}
		CALCULATOR->clearRPNStack();
		gtk_list_store_clear(stackstore);
		clearresult();
	}
}

void updateRPNIndexes() {
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(stackstore), &iter)) return;
	for(int i = 1; ; i++) {
		gtk_list_store_set(stackstore, &iter, 0, i2s(i).c_str(), -1);
		if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(stackstore), &iter)) break;
	}
}

void calculateRPN(int op) {
	if(expression_has_changed) {
		if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
			execute_expression(true);
		}
	}
	execute_expression(true, true, (MathOperation) op, NULL);
}
void calculateRPN(MathFunction *f) {
	if(expression_has_changed) {
		if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
			execute_expression(true);
		}
	}
	execute_expression(true, true, OPERATION_ADD, f);
}
void RPNRegisterAdded(string text, gint index) {
	GtkTreeIter iter;
	gtk_list_store_insert(stackstore, &iter, index);
	gtk_list_store_set(stackstore, &iter, 0, i2s(index + 1).c_str(), 1, text.c_str(), -1);
	updateRPNIndexes();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_clearstack")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy")), TRUE);
	if(CALCULATOR->RPNStackSize() >= 2) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap")), TRUE);
	}
	on_stackview_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), NULL);
}
void RPNRegisterRemoved(gint index) {
  	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index);
	gtk_list_store_remove(stackstore, &iter);
	updateRPNIndexes();
	if(CALCULATOR->RPNStackSize() == 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_clearstack")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy")), FALSE);
	}
	if(CALCULATOR->RPNStackSize() < 2) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap")), FALSE);
	}
	on_stackview_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), NULL);
}
void RPNRegisterChanged(string text, gint index) {
  	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index);
	gtk_list_store_set(stackstore, &iter, 1, text.c_str(), -1);
}

/*
	general function used to insert text in expression entry
*/
void insert_text(const gchar *name) {
	if(b_busy) return;
	block_completion();
	overwrite_expression_selection(name);
	if(!gtk_widget_is_focus(expressiontext)) {
		gtk_widget_grab_focus(expressiontext);
	}
	unblock_completion();
}

void recreate_recent_functions() {
	GtkWidget *item, *sub;
	sub = f_menu;
	recent_function_items.clear();
	bool b = false;
	for(size_t i = 0; i < recent_functions.size(); i++) {
		if(!CALCULATOR->stillHasFunction(recent_functions[i])) {
			recent_functions.erase(recent_functions.begin() + i);
			i--;
		} else {
			if(!b) {
				MENU_SEPARATOR_PREPEND
				b = true;
			}
			item = gtk_menu_item_new_with_label(recent_functions[i]->title(true).c_str()); 
			recent_function_items.push_back(item);
			gtk_widget_show(item); 
			gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_function), (gpointer) recent_functions[i]);
		}
	}
}
void recreate_recent_variables() {
	GtkWidget *item, *sub;
	sub = v_menu;
	recent_variable_items.clear();
	bool b = false;
	for(size_t i = 0; i < recent_variables.size(); i++) {
		if(!CALCULATOR->stillHasVariable(recent_variables[i])) {
			recent_variables.erase(recent_variables.begin() + i);
			i--;
		} else {
			if(!b) {
				MENU_SEPARATOR_PREPEND
				b = true;
			}
			item = gtk_menu_item_new_with_label(recent_variables[i]->title(true).c_str()); 
			recent_variable_items.push_back(item);
			gtk_widget_show(item); 
			gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_variable), (gpointer) recent_variables[i]);
		}
	}
}
void recreate_recent_units() {
	GtkWidget *item, *sub;
	sub = u_menu;
	recent_unit_items.clear();
	bool b = false;
	for(size_t i = 0; i < recent_units.size(); i++) {
		if(!CALCULATOR->stillHasUnit(recent_units[i])) {
			recent_units.erase(recent_units.begin() + i);
			i--;
		} else {
			if(!b) {
				MENU_SEPARATOR_PREPEND
				b = true;
			}
			item = gtk_menu_item_new_with_label(recent_units[i]->title(true).c_str()); 
			recent_unit_items.push_back(item);
			gtk_widget_show(item); 
			gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_unit), (gpointer) recent_units[i]);
		}
	}
}

void function_inserted(MathFunction *object) {
	if(!object) {
		return;
	}
	GtkWidget *item, *sub;
	sub = f_menu;
	if(recent_function_items.size() <= 0) {
		MENU_SEPARATOR_PREPEND
	}
	for(size_t i = 0; i < recent_functions.size(); i++) {
		if(recent_functions[i] == object) {
			recent_functions.erase(recent_functions.begin() + i);
			gtk_widget_destroy(recent_function_items[i]);
			recent_function_items.erase(recent_function_items.begin() + i);
			break;
		}
	}
	if(recent_function_items.size() >= 5) {
		recent_functions.erase(recent_functions.begin());
		gtk_widget_destroy(recent_function_items[0]);
		recent_function_items.erase(recent_function_items.begin());
	}
	item = gtk_menu_item_new_with_label(object->title(true).c_str()); 
	recent_function_items.push_back(item);
	recent_functions.push_back(object);
	gtk_widget_show(item); 
	gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_function), (gpointer) object);
}
void variable_inserted(Variable *object) {
	if(!object) {
		return;
	}
	GtkWidget *item, *sub;
	sub = v_menu;
	if(recent_variable_items.size() <= 0) {
		MENU_SEPARATOR_PREPEND
	}
	for(size_t i = 0; i < recent_variables.size(); i++) {
		if(recent_variables[i] == object) {
			recent_variables.erase(recent_variables.begin() + i);
			gtk_widget_destroy(recent_variable_items[i]);
			recent_variable_items.erase(recent_variable_items.begin() + i);
			break;
		}
	}
	if(recent_variable_items.size() >= 5) {
		recent_variables.erase(recent_variables.begin());
		gtk_widget_destroy(recent_variable_items[0]);
		recent_variable_items.erase(recent_variable_items.begin());
	}
	item = gtk_menu_item_new_with_label(object->title(true).c_str()); 
	recent_variable_items.push_back(item);
	recent_variables.push_back(object);
	gtk_widget_show(item); 
	gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_variable), (gpointer) object);
}
void unit_inserted(Unit *object) {
	if(!object) {
		return;
	}
	GtkWidget *item, *sub;
	sub = u_menu;
	if(recent_unit_items.size() <= 0) {
		MENU_SEPARATOR_PREPEND
	}
	for(size_t i = 0; i < recent_units.size(); i++) {
		if(recent_units[i] == object) {
			recent_units.erase(recent_units.begin() + i);
			gtk_widget_destroy(recent_unit_items[i]);
			recent_unit_items.erase(recent_unit_items.begin() + i);
			break;
		}
	}
	if(recent_unit_items.size() >= 5) {
		recent_units.erase(recent_units.begin());
		gtk_widget_destroy(recent_unit_items[0]);
		recent_unit_items.erase(recent_unit_items.begin());
	}
	item = gtk_menu_item_new_with_label(object->title(true).c_str()); 
	recent_unit_items.push_back(item);
	recent_units.push_back(object);
	gtk_widget_show(item); 
	gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_unit), (gpointer) object);
}

void apply_function(MathFunction *f, GtkWidget* = NULL) {
	if(b_busy) return;
	if(rpn_mode) {
		calculateRPN(f);
		return;
	}
	string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionbuffer).name;
	if(f->args() == 0) {
		str += "()";
	} else {
		str += "(";
		str += get_expression_text();
		str += ")";
	}
	add_to_undo = false;
	gtk_text_buffer_set_text(expressionbuffer, "", -1);
	add_to_undo = true;
	insert_text(str.c_str());
	execute_expression();
	function_inserted(f);
}

gint on_function_int_input(GtkSpinButton *entry, gpointer new_value, gpointer) {
	string str = gtk_entry_get_text(GTK_ENTRY(entry));
	remove_blank_ends(str);
	if(str.find_first_not_of(NUMBERS) != string::npos) {
		MathStructure value;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 200, evalops);
		CALCULATOR->endTemporaryStopMessages();
		if(!value.isNumber()) return GTK_INPUT_ERROR;
		bool overflow = false;
		*((gdouble*) new_value) = value.number().intValue(&overflow);
		if(overflow) return GTK_INPUT_ERROR;
		return TRUE;
	}
	return FALSE;
}

/*
	insert function
	pops up an argument entry dialog and inserts function into expression entry
	parent is parent window
*/
void insert_function(MathFunction *f, GtkWidget *parent = NULL, bool add_to_menu = true) {
	if(!f) {
		return;
	}
	
	//if function takes no arguments, do not display dialog and insert function directly
	if(f->args() == 0) {
		string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name + "()";
		gchar *gstr = g_strdup(str.c_str());
		function_inserted(f);
		insert_text(gstr);
		g_free(gstr);
		return;
	}
	
	GtkTextIter istart, iend;
	gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);
	
	string f_title = f->title(true);
	GtkWidget *dialog = gtk_dialog_new_with_buttons(f_title.c_str(), GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

	GtkWidget *b_exec = gtk_button_new_with_mnemonic(_("_Execute"));
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), b_exec, GTK_RESPONSE_APPLY);

	GtkWidget *b_insert = gtk_button_new_with_mnemonic(_("_Insert"));
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), b_insert, GTK_RESPONSE_ACCEPT);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	GtkWidget *vbox_pre = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_pre), 12);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox_pre);
	f_title.insert(0, "<b>");
	f_title += "</b>";
	GtkWidget *title_label = gtk_label_new(f_title.c_str());
	gtk_label_set_use_markup(GTK_LABEL(title_label), TRUE);
	gtk_widget_set_halign(title_label, GTK_ALIGN_START);

	gtk_container_add(GTK_CONTAINER(vbox_pre), title_label);
	int args = 0;
	bool has_vector = false;
	if(f->args() > 0) {
		args = f->args();
	} else if(f->minargs() > 0) {
		args = f->minargs() + 1;
		has_vector = true;
	} else {
		args = 1;
		has_vector = true;
	}

	GtkWidget *table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 6);
	gtk_grid_set_column_spacing(GTK_GRID(table), 12);
	gtk_container_add(GTK_CONTAINER(vbox_pre), table);
	gtk_widget_set_hexpand(table, TRUE);
	vector<GtkWidget*> label;
	vector<GtkWidget*> entry;
	vector<GtkWidget*> type_label;
	label.resize(args, NULL);
	entry.resize(args, NULL);
	type_label.resize(args, NULL);
	vector<GtkWidget*> boolean_buttons;
	vector<int> boolean_index;
	boolean_index.resize(args, 0);
	int bindex = 0;
	string argstr, typestr, defstr; 
	string argtype;
	//create argument entries
	Argument *arg;
	GtkListStore *properties_store = NULL;
	for(int i = 0; i < args; i++) {
		arg = f->getArgumentDefinition(i + 1);
		if(!arg || arg->name().empty()) {
			if(args == 1) {
				argstr = _("Value");
			} else {
				argstr = _("Argument");
				argstr += " ";
				argstr += i2s(i + 1);
			}
		} else {
			argstr = arg->name();
		}
		typestr = "";
		argtype = "";
		defstr = f->getDefaultValue(i + 1);
		if(arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && defstr.length() >= 2 && defstr[0] == '\"' && defstr[defstr.length() - 1] == '\"') {
			defstr = defstr.substr(1, defstr.length() - 2);
		}
		label[i] = gtk_label_new(argstr.c_str());
		gtk_widget_set_halign(label[i], GTK_ALIGN_END);
		gtk_widget_set_hexpand(label[i], FALSE);
		if(arg) {
			switch(arg->type()) {
				case ARGUMENT_TYPE_INTEGER: {
					IntegerArgument *iarg = (IntegerArgument*) arg;
					glong min = LONG_MIN, max = LONG_MAX;
					if(iarg->min()) {
						min = iarg->min()->lintValue();
					}
					if(iarg->max()) {
						max = iarg->max()->lintValue();
					}
					entry[i] = gtk_spin_button_new_with_range(min, max, 1);
					gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry[i]), FALSE);
					gtk_entry_set_alignment(GTK_ENTRY(entry[i]), 1.0);
					g_signal_connect(GTK_SPIN_BUTTON(entry[i]), "input", G_CALLBACK(on_function_int_input), NULL);
					if(!f->getDefaultValue(i + 1).empty()) {
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry[i]), s2i(f->getDefaultValue(i + 1)));
					} else if(!arg->zeroForbidden() && min <= 0 && max >= 0) {
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry[i]), 0);
					} else {
						if(max < 0) {
							gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry[i]), max);
						} else if(min <= 1) {
							gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry[i]), 1);
						} else {
							gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry[i]), min);
						}
					}
					break;
				}
				case ARGUMENT_TYPE_BOOLEAN: {
					boolean_index[i] = bindex;
					bindex += 2;
					entry[i] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
					gtk_box_set_homogeneous(GTK_BOX(entry[i]), TRUE);
					gtk_widget_set_halign(entry[i], GTK_ALIGN_START);
					boolean_buttons.push_back(gtk_radio_button_new_with_label(NULL, _("True")));
					gtk_box_pack_start(GTK_BOX(entry[i]), boolean_buttons[boolean_buttons.size() - 1], TRUE, TRUE, 0);
					boolean_buttons.push_back(gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(boolean_buttons[boolean_buttons.size() - 1]), _("False")));
					gtk_box_pack_end(GTK_BOX(entry[i]), boolean_buttons[boolean_buttons.size() - 1], TRUE, TRUE, 0);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(boolean_buttons[boolean_buttons.size() - 1]), TRUE);
					break;
				}
				case ARGUMENT_TYPE_DATA_PROPERTY: {
					if(f->subtype() == SUBTYPE_DATA_SET) {
						properties_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
						gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(properties_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
						gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(properties_store), 0, GTK_SORT_ASCENDING);
						entry[i] = gtk_combo_box_new_with_model(GTK_TREE_MODEL(properties_store));
						GtkCellRenderer *cell = gtk_cell_renderer_text_new();
						gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(entry[i]), cell, TRUE);
						gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(entry[i]), cell, "text", 0);
						DataPropertyIter it;
						DataSet *ds = (DataSet*) f;
						DataProperty *dp = ds->getFirstProperty(&it);
						GtkTreeIter iter;
						bool active_set = false;
						while(dp) {
							if(!dp->isHidden()) {
								gtk_list_store_append(properties_store, &iter);
								if(!active_set && defstr == dp->getName()) {
									gtk_combo_box_set_active_iter(GTK_COMBO_BOX(entry[i]), &iter);
									active_set = true;
								}
								gtk_list_store_set(properties_store, &iter, 0, dp->title().c_str(), 1, (gpointer) dp, -1);
							}
							dp = ds->getNextProperty(&it);
						}
						gtk_list_store_append(properties_store, &iter);
						if(!active_set) {
							gtk_combo_box_set_active_iter(GTK_COMBO_BOX(entry[i]), &iter);
						}
						gtk_list_store_set(properties_store, &iter, 0, _("Info"), 1, (gpointer) NULL, -1);
						break;
					}
				}
				default: {
					if(i >= f->minargs() && !has_vector) {
						typestr = "(";
						typestr += _("optional");
					}
					argtype = arg->print();		
					if(typestr.empty()) {
						typestr = "(";
					} else if(!argtype.empty()) {
						typestr += " ";
					}
					if(!argtype.empty()) {
						typestr += argtype;
					}
					typestr += ")";		
					if(typestr.length() == 2) {
						typestr = "";
					}
					entry[i] = gtk_entry_new();
					gtk_entry_set_alignment(GTK_ENTRY(entry[i]), 1.0);
				}
			}
		} else {
			entry[i] = gtk_entry_new();
			gtk_entry_set_alignment(GTK_ENTRY(entry[i]), 1.0);
		}
		gtk_widget_set_hexpand(entry[i], TRUE);
		if(typestr.empty() && i >= f->minargs() && !has_vector) {
			typestr = "(";
			typestr += _("optional");
			typestr += ")";			
		}
		if(arg) {
			switch(arg->type()) {		
				case ARGUMENT_TYPE_DATE: {
					typestr = typestr.substr(1, typestr.length() - 2);
					type_label[i] = gtk_button_new_with_label(typestr.c_str());
					g_signal_connect((gpointer) type_label[i], "clicked", G_CALLBACK(on_type_label_date_clicked), (gpointer) entry[i]);
					break;
				}
				case ARGUMENT_TYPE_FILE: {
					typestr = typestr.substr(1, typestr.length() - 2);
					type_label[i] = gtk_button_new_with_label(typestr.c_str());
					g_signal_connect((gpointer) type_label[i], "clicked", G_CALLBACK(on_type_label_file_clicked), (gpointer) entry[i]);
					break;
				}
				default: {
					type_label[i] = gtk_label_new(typestr.c_str());
				}
			}
		} else if(!typestr.empty()) {
			type_label[i] = gtk_label_new(typestr.c_str());
		} else {
			type_label[i] = NULL;
		}
		if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(defstr == "1") {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(boolean_buttons[boolean_buttons.size() - 2]), TRUE);
			}
		} else if(properties_store && arg && arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
		} else {
			gtk_entry_set_text(GTK_ENTRY(entry[i]), defstr.c_str());
			//insert selection in expression entry into the first argument entry
			if(i == 0) {
				gtk_entry_set_text(GTK_ENTRY(entry[i]), get_selected_expression_text(true).c_str());
				if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
					gtk_spin_button_update(GTK_SPIN_BUTTON(entry[i]));
				}
			}
		}
		gtk_grid_attach(GTK_GRID(table), label[i], 0, i, 1, 1);
		gtk_grid_attach(GTK_GRID(table), entry[i], 1, i, 1, 1);
		if(type_label[i]) {
			gtk_widget_set_hexpand(type_label[i], FALSE);
			gtk_widget_set_halign(type_label[i], GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(table), type_label[i], 2, i, 1, 1);
		}
	}
	//display function description
	if(!f->description().empty() || !f->example(true).empty()) {
		GtkWidget *descr_frame = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(vbox_pre), descr_frame);
		GtkWidget *descr = gtk_text_view_new();
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(descr), GTK_WRAP_WORD);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(descr), FALSE);
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(descr));
		string str;
		if(!f->description().empty()) str += f->description();
		if(!f->example(true).empty()) {
			if(!str.empty()) str += "\n\n";
			str += _("Example:");
			str += " ";
			str += f->example(false);
		}
		gtk_text_buffer_set_text(buffer, str.c_str(), -1);
		gtk_container_add(GTK_CONTAINER(descr_frame), descr);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(descr), 12);
		gtk_text_view_set_right_margin(GTK_TEXT_VIEW(descr), 12);
		gtk_text_view_set_top_margin(GTK_TEXT_VIEW(descr), 12);
		gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(descr), 12);
#else 
		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(descr), 6);
		gtk_text_view_set_right_margin(GTK_TEXT_VIEW(descr), 6);
#endif
		gtk_widget_show_all(vbox_pre);
		gint nw, mw, nh, mh;
		gtk_widget_get_preferred_width(vbox_pre, &mw, &nw);
		gtk_widget_get_preferred_height(vbox_pre, &mh, &nh);
		PangoLayout *layout_test = gtk_widget_create_pango_layout(descr, NULL);
		pango_layout_set_text(layout_test, str.c_str(), -1);
		pango_layout_set_width(layout_test, (nw - 24) * PANGO_SCALE);
		pango_layout_set_wrap(layout_test, PANGO_WRAP_WORD);
		gint w, h;
		pango_layout_get_pixel_size(layout_test, &w, &h);
		h *= 1.2;
		if(h > nh) h = nh;
		if(h < 100) h = 100;
		gtk_widget_set_size_request(descr_frame, -1, h);
	}
	gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_APPLY) {
		
		string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name + "(", str2;
		for(int i = 0; i < args; i++) {
			if(f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_BOOLEAN) {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(boolean_buttons[boolean_index[i]]))) {
					str2 = "1";
				} else {
					str2 = "0";
				}
			} else if(evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_INTEGER) {
				Number nr(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry[i])), 1);
				PrintOptions po;
				po.base = evalops.parse_options.base;
				po.base_display = BASE_DISPLAY_NONE;
				str2 = nr.print(po);
			} else if(properties_store && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
				GtkTreeIter iter;
				DataProperty *dp = NULL;
				if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(entry[i]), &iter)) {
					gtk_tree_model_get(GTK_TREE_MODEL(properties_store), &iter, 1, &dp, -1);
				}	
				if(dp) {
					str2 = dp->getName();
				} else {
					str2 = "info";
				}
			} else {
				str2 = gtk_entry_get_text(GTK_ENTRY(entry[i]));
			}

			//if the minimum number of function arguments have been filled, do not add anymore if entry is empty
			if(i >= f->minargs()) {
				remove_blank_ends(str2);
			}
			if((i < f->minargs() || !str2.empty()) && f->getArgumentDefinition(i + 1) && (f->getArgumentDefinition(i + 1)->suggestsQuotes() || (f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_TEXT && str2.find(CALCULATOR->getComma()) != string::npos))) {
				if(str2.length() < 1 || (str2[0] != '\"' && str[0] != '\'')) { 
					str2.insert(0, "\"");
					str2 += "\"";
				}
			}
			if(i > 0) {
				str += CALCULATOR->getComma();
				str += " ";
			}
			str += str2;
		}
		str += ")";
		
		add_to_undo = false;
		//redo selection if "OK" was clicked, clear expression entry "Execute"
		if(response == GTK_RESPONSE_ACCEPT) {
			gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
		} else {
			gtk_text_buffer_set_text(expressionbuffer, "", -1);
		}
		add_to_undo = true;
		insert_text(str.c_str());
		//Calculate directly when "Execute" was clicked
		if(response == GTK_RESPONSE_APPLY) {
			execute_expression();
		}
		if(add_to_menu) function_inserted(f);
	}
	gtk_widget_destroy(dialog);
}

/*
	called from function menu
*/
void insert_function(GtkMenuItem*, gpointer user_data) {
	insert_function((MathFunction*) user_data, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

/*
	called from variable menu
	just insert text data stored in menu item
*/
void insert_variable(GtkMenuItem*, gpointer user_data) {
	Variable *v = (Variable*) user_data;
	if(!CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
		update_vmenu();
		return;
	}
	insert_text(v->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
	variable_inserted((Variable*) user_data);
}
void insert_button_variable(GtkWidget*, gpointer user_data) {
	Variable *v = (Variable*) user_data;
	if(!CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
		update_vmenu();
		return;
	}
	insert_text(v->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
}

//from prefix menu
void insert_prefix(GtkMenuItem*, gpointer user_data) {
	insert_text(((Prefix*) user_data)->name(printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext).c_str());
}
//from unit menu
void insert_unit(GtkMenuItem*, gpointer user_data) {
	if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		insert_text(((CompositeUnit*) user_data)->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext).c_str());
	} else {
		insert_text(((Unit*) user_data)->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
	}
	unit_inserted((Unit*) user_data);
}

void insert_button_unit(GtkMenuItem*, gpointer user_data) {
	if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		insert_text(((CompositeUnit*) user_data)->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext).c_str());
	} else {
		insert_text(((Unit*) user_data)->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
	}
	if((Unit*) user_data != latest_button_unit) {
		latest_button_unit = (Unit*) user_data;
		string si_label_str;
		if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			si_label_str = ((CompositeUnit*) latest_button_unit)->print(false, true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext);
		} else {
		
			si_label_str = latest_button_unit->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name;
		}
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_si")), si_label_str.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_si")), latest_button_unit->title(true).c_str());
	}
}
void insert_button_currency(GtkMenuItem*, gpointer user_data) {
	if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		insert_text(((CompositeUnit*) user_data)->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext).c_str());
	} else {
		insert_text(((Unit*) user_data)->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
	}
	if((Unit*) user_data != latest_button_currency) {
		latest_button_currency = (Unit*) user_data;
		string currency_label_str;
		if(((Unit*) user_data)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			currency_label_str = ((CompositeUnit*) latest_button_currency)->print(false, true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext);
		} else {
		
			currency_label_str = latest_button_currency->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name;
		}
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_euro")), currency_label_str.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_euro")), latest_button_currency->title(true).c_str());
	}
}

void set_name_label_and_entry(ExpressionItem *item, GtkWidget *entry, GtkWidget *label) {
	const ExpressionName *ename = &item->getName(1);
	gtk_entry_set_text(GTK_ENTRY(entry), ename->name.c_str());
	if(item->countNames() > 1) {
		string str = "+ ";
		for(size_t i = 2; i <= item->countNames(); i++) {
			if(i > 2) str += ", ";
			str += item->getName(i).name;
		}
		gtk_label_set_text(GTK_LABEL(label), str.c_str());
	}
}
void set_edited_names(ExpressionItem *item, string str) {
	if(item->isBuiltin() && !(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_DATA_SET)) return;
	if(names_edited) {
		item->clearNames();
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {
			ExpressionName ename;
			gchar *gstr;
			while(true) {	
				gboolean abbreviation = FALSE, suffix = FALSE, unicode = FALSE, plural = FALSE;
				gboolean reference = FALSE, avoid_input = FALSE, case_sensitive = FALSE;
				gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, NAMES_ABBREVIATION_COLUMN, &abbreviation, NAMES_SUFFIX_COLUMN, &suffix, NAMES_UNICODE_COLUMN, &unicode, NAMES_PLURAL_COLUMN, &plural, NAMES_REFERENCE_COLUMN, &reference, NAMES_AVOID_INPUT_COLUMN, &avoid_input, NAMES_CASE_SENSITIVE_COLUMN, &case_sensitive, -1);
				ename.name = gstr; ename.abbreviation = abbreviation; ename.suffix = suffix;
				ename.unicode = unicode; ename.plural = plural; ename.reference = reference; 
				ename.avoid_input = avoid_input; ename.case_sensitive = case_sensitive;
				item->addName(ename);
				g_free(gstr);
				if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tNames_store), &iter)) break;
			}
		} else {
			item->addName(str);
		}
	} else {
		if(item->countNames() == 0) {
			ExpressionName ename(str);
			ename.reference = true;
			item->setName(ename, 1);
		} else {
			item->setName(str, 1);
		}
		
	}
}

/*
	display edit/new unit dialog
	creates new unit if u == NULL, win is parent window
*/
void edit_unit(const char *category = "", Unit *u = NULL, GtkWidget *win = NULL) {

	edited_unit = u;
	names_edited = false;
	editing_unit = true;
	GtkWidget *dialog = get_unit_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	
	if(u) {
		if(u->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Unit"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Unit (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Unit"));
	}

	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category")))), category);

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), "");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), 1);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system")))), "");
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(unitedit_builder, "unit_edit_label_names")), "");

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_button_ok")), TRUE);

	if(u) {
		//fill in original parameters
		if(u->subtype() == SUBTYPE_BASE_UNIT) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), UNIT_CLASS_BASE_UNIT);
		} else if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), UNIT_CLASS_ALIAS_UNIT);
		} else if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), UNIT_CLASS_COMPOSITE_UNIT);
		}
		on_unit_edit_combobox_class_changed(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), NULL);	
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), !u->isBuiltin());

		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), u->isLocal() && !u->isBuiltin());

		set_name_label_and_entry(u, GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_names")));
		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), !u->isBuiltin());
		
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system")))), u->system().c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system")), !u->isBuiltin());

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_hidden")), u->isHidden());
		
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_use_prefixes")), u->useWithPrefixesByDefault());
		
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category")))), u->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc")), u->title(false).c_str());
		
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), 1);		

		switch(u->subtype()) {
			case SUBTYPE_ALIAS_UNIT: {
				AliasUnit *au = (AliasUnit*) u;
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), ((CompositeUnit*) (au->firstBaseUnit()))->preferredDisplayName(printops.abbreviate_names, true, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")).name.c_str());
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), au->firstBaseExponent());
				if(au->firstBaseExponent() != 1) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_frame_mix")), FALSE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), CALCULATOR->localizeExpression(au->expression()).c_str());
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), CALCULATOR->localizeExpression(au->inverseExpression()).c_str());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_box_reversed")), au->hasComplexExpression());
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_exact")), !au->isApproximate());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_exact")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_frame_mix")), !u->isBuiltin());
				if(au->mixWithBase() > 0) {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), TRUE);
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), au->mixWithBase());
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), au->mixWithBaseMinimum() > 1 ? au->mixWithBaseMinimum() : 1);
					on_unit_edit_checkbutton_mix_toggled(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), NULL);
				}
				break;
			}
			case SUBTYPE_COMPOSITE_UNIT: {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), ((CompositeUnit*) u)->print(false, printops.abbreviate_names, true, &can_display_unicode_string_function, (void*) gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")).c_str());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), !u->isBuiltin());
			}
			default: {				
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_frame_mix")), FALSE);
			}
		}
	} else {
		//default values
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_hidden")), false);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_exact")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_box_reversed")), false);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), UNIT_CLASS_ALIAS_UNIT);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), "1");
		on_unit_edit_combobox_class_changed(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), NULL);
	}	

run_unit_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str;
		str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));
		remove_blank_ends(str);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name given
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_unit_edit_dialog;
		}

		//unit with the same name exists -- overwrite or open the dialog again
		if((!u || !u->hasName(str)) && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) && CALCULATOR->unitNameTaken(str, u) && !ask_question(_("A variable or unit with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));
			goto run_unit_edit_dialog;
		}
		bool add_unit = false;
		if(u) {
			//edited an existing unit -- update unit
			u->setLocal(true);
			gint i1 = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")));
			switch(u->subtype()) {
				case SUBTYPE_ALIAS_UNIT: {
					if(i1 != UNIT_CLASS_ALIAS_UNIT) {
						u->destroy();
						u = NULL;
						break;
					}
					if(!u->isBuiltin()) {
						AliasUnit *au = (AliasUnit*) u;
						Unit *bu = CALCULATOR->getUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
						if(!bu) bu = CALCULATOR->getCompositeUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
						if(!bu) {
							gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")));
							show_message(_("Base unit does not exist."), dialog);
							goto run_unit_edit_dialog;
						}
						au->setBaseUnit(bu);
						au->setExpression(CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation"))), evalops.parse_options));
						au->setInverseExpression(au->hasComplexExpression() ? CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed"))), evalops.parse_options) : "");
						au->setExponent(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp"))));
						au->setApproximate(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_exact"))));
						if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")))) {
							au->setMixWithBase(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority"))));
							au->setMixWithBaseMinimum(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min"))));
						} else {
							au->setMixWithBase(0);
						}
					}
					break;
				}
				case SUBTYPE_COMPOSITE_UNIT: {
					if(i1 != UNIT_CLASS_COMPOSITE_UNIT) {
						u->destroy();
						u = NULL;
						break;
					}
					if(!u->isBuiltin()) {
						((CompositeUnit*) u)->setBaseExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
					}
					break;
				}
				case SUBTYPE_BASE_UNIT: {
					if(i1 != UNIT_CLASS_BASE_UNIT) {
						u->destroy();
						u = NULL;
						break;
					}
					break;
				}
			}
			if(u) {
				u->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))));
				u->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))));
			}
		}
		if(!u) {
			//new unit
			switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")))) {
				case UNIT_CLASS_ALIAS_UNIT: {
					Unit *bu = CALCULATOR->getUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
					if(!bu) bu = CALCULATOR->getCompositeUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
					if(!bu) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")));
						show_message(_("Base unit does not exist."), dialog);
						goto run_unit_edit_dialog;
					}
					u = new AliasUnit(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))), "", "", "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))), bu, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation"))), evalops.parse_options), gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp"))), CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed"))), evalops.parse_options), true);
					((AliasUnit*) u)->setApproximate(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_exact"))));
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")))) {
						((AliasUnit*) u)->setMixWithBase(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority"))));
						((AliasUnit*) u)->setMixWithBaseMinimum(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min"))));
					}
					break;
				}
				case UNIT_CLASS_COMPOSITE_UNIT: {
					CompositeUnit *cu = new CompositeUnit(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))), "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))), true);
					u = cu;
					break;
				}
				default: {
					u = new Unit(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))), "", "", "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))), true);
					break;
				}
			}
			add_unit = true;
		}
		if(u) {
			u->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_hidden"))));			
			if(!u->isBuiltin()) {
				u->setSystem(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system"))));
			}
			if(u->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				u->setUseWithPrefixesByDefault(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_use_prefixes"))));
			}
			set_edited_names(u, str);
			if(add_unit) {
				CALCULATOR->addUnit(u);
			}
			//select the new unit
			selected_unit = u;
			if(!u->isActive()) {
				selected_unit_category = _("Inactive");
			} else if(u->category().empty()) {
				selected_unit_category = _("Uncategorized");
			} else {
				selected_unit_category = "/";
				selected_unit_category += u->category();
			}
		}
		update_umenus();
		unit_inserted(u);
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-units.html#qalculate-unit-creation", gtk_builder_get_object(unitedit_builder, "unit_edit_dialog")); 
		goto run_unit_edit_dialog;
	}
	edited_unit = NULL;
	names_edited = false;
	editing_unit = false;
	gtk_widget_hide(dialog);
}

void edit_argument(Argument *arg) {
	if(!arg) {
		arg = new Argument();
	}
	edited_argument = arg;
	GtkWidget *dialog = get_argument_rules_dialog();	
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_test")), arg->tests());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), arg->matrixAllowed());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), TRUE);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")), CALCULATOR->localizeExpression(arg->getCustomCondition()).c_str());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_condition")), !arg->getCustomCondition().empty());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")), !arg->getCustomCondition().empty());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_forbid_zero")), arg->zeroForbidden());
	switch(arg->type()) {
		case ARGUMENT_TYPE_NUMBER: {
			NumberArgument *farg = (NumberArgument*) arg;
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_box_min")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), farg->min() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), farg->includeEqualsMin());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), farg->min() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), TRUE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 8);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
			if(farg->min()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), farg->min()->floatValue());
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			}
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_box_max")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), farg->max() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), farg->includeEqualsMax());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), farg->max() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), TRUE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 8);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);
			if(farg->max()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), farg->max()->floatValue());
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			}			
			break;
		}
		case ARGUMENT_TYPE_INTEGER: {
			IntegerArgument *iarg = (IntegerArgument*) arg;
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_box_min")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), iarg->min() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), iarg->min() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), FALSE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
			if(iarg->min()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), iarg->min()->intValue());		
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			}
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_box_max")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), iarg->max() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), iarg->max() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), FALSE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);
			if(iarg->max()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), iarg->max()->intValue());		
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			}	
			break;
		}
		case ARGUMENT_TYPE_FREE: {}
		case ARGUMENT_TYPE_MATRIX: {		
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), FALSE);	
		}
		default: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), FALSE);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_box_min")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_box_max")), FALSE);
		}
	}

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		arg->setTests(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_test"))));
		arg->setMatrixAllowed(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix"))));
		arg->setZeroForbidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_forbid_zero"))));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_condition")))) {
			arg->setCustomCondition(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition"))));
		} else {
			arg->setCustomCondition("");
		}
		if(arg->type() == ARGUMENT_TYPE_NUMBER) {
			NumberArgument *farg = (NumberArgument*) arg;
			farg->setIncludeEqualsMin(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals"))));
			farg->setIncludeEqualsMax(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals"))));
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")))) {
				Number nr;
				nr.setFloat(gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min"))));
				farg->setMin(&nr);
			} else {
				farg->setMin(NULL);
			}
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")))) {
				Number nr;
				nr.setFloat(gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max"))));
				farg->setMax(&nr);
			} else {
				farg->setMax(NULL);
			}			
		} else if(arg->type() == ARGUMENT_TYPE_INTEGER) {
			IntegerArgument *iarg = (IntegerArgument*) arg;
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")))) {
				Number integ(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min"))), 1);
				iarg->setMin(&integ);
			} else {
				iarg->setMin(NULL);
			}
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")))) {
				Number integ(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max"))), 1);
				iarg->setMax(&integ);
			} else {
				iarg->setMax(NULL);
			}			
		}
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
		if(gtk_tree_selection_get_selected(select, &model, &iter)) {
			gtk_list_store_set(tFunctionArguments_store, &iter, 0, arg->name().c_str(), 1, arg->printlong().c_str(), 2, (gpointer) arg, -1);
		}
	}
	edited_argument = NULL;
	gtk_widget_hide(dialog);
}


void delete_function(MathFunction *f) {
	if(f && f->isLocal()) {
		for(size_t i = 0; i < recent_functions.size(); i++) {
			if(recent_functions[i] == f) {
				recent_functions.erase(recent_functions.begin() + i);
				gtk_widget_destroy(recent_function_items[i]);
				recent_function_items.erase(recent_function_items.begin() + i);
				break;
			}
		}
		//ensure removal of all references in Calculator
		f->destroy();
		//update menus and trees
		update_fmenu();
	}
}

/*
	display edit/new function dialog
	creates new function if f == NULL, win is parent window
*/
void edit_function(const char *category = "", MathFunction *f = NULL, GtkWidget *win = NULL, const char *name = NULL, const char *expression = NULL) {

	if(f && f->subtype() == SUBTYPE_DATA_SET) {
		edit_dataset((DataSet*) f, win);
		return;
	}

	GtkWidget *dialog = get_function_edit_dialog();	
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	edited_function = f;
	names_edited = false;
	editing_function = true;
	
	if(f) {
		if(f->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Function"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Function (global)"));		
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Function"));
	}

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_description")));
	GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression")));	

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), "");
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(functionedit_builder, "function_edit_label_names")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition")), "");
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition")), !f || !f->isBuiltin());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), !f || !f->isBuiltin());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression")), !f || !f->isBuiltin());
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")))), category);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc")), "");
	gtk_text_buffer_set_text(description_buffer, "", -1);
	gtk_text_buffer_set_text(expression_buffer, "", -1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_hidden")), false);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name")), "");

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_ok")), TRUE);	
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_combobox_argument_type")), !f || !f->isBuiltin());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_argument")), !f || !f->isBuiltin());

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_subfunctions")), !f || !f->isBuiltin());
	gtk_list_store_clear(tSubfunctions_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_subfunction")), FALSE);		
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_subfunction")), !f || !f->isBuiltin());
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression")), "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate")), TRUE);
	selected_subfunction = 0;
	last_subfunction_index = 0;
	if(f) {
		//fill in original paramaters
		set_name_label_and_entry(f, GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_label_names")));
		if(!f->isBuiltin()) {
			gtk_text_buffer_set_text(expression_buffer, CALCULATOR->localizeExpression(((UserFunction*) f)->formula()).c_str(), -1);
		}
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")))), f->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc")), f->title(false).c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition")), CALCULATOR->localizeExpression(f->condition()).c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_hidden")), f->isHidden());
		gtk_text_buffer_set_text(description_buffer, f->description().c_str(), -1);
		
		if(!f->isBuiltin()) {
			GtkTreeIter iter;
			string str, str2;
			for(size_t i = 1; i <= ((UserFunction*) f)->countSubfunctions(); i++) {
				gtk_list_store_append(tSubfunctions_store, &iter);
				if(((UserFunction*) f)->subfunctionPrecalculated(i)) {
					str = _("Yes");
				} else {
					str = _("No");
				}
				str2 = "\\";
				str2 += i2s(i);
				gtk_list_store_set(tSubfunctions_store, &iter, 0, str2.c_str(), 1, ((UserFunction*) f)->getSubfunction(i).c_str(), 2, str.c_str(), 3, i, 4, ((UserFunction*) f)->subfunctionPrecalculated(i), -1);
				last_subfunction_index = i;
			}
		}
	}
	if(name) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), name);
	if(expression) gtk_text_buffer_set_text(expression_buffer, expression, -1);
	update_function_arguments_list(f);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);
	
run_function_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
		remove_blank_ends(str);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_function_edit_dialog;
		}
		GtkTextIter e_iter_s, e_iter_e;
		gtk_text_buffer_get_start_iter(expression_buffer, &e_iter_s);
		gtk_text_buffer_get_end_iter(expression_buffer, &e_iter_e);
		gchar *gstr = gtk_text_buffer_get_text(expression_buffer, &e_iter_s, &e_iter_e, FALSE);
		string str2 = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
		g_free(gstr);
		remove_blank_ends(str2);
		gsub("\n", " ", str2);
		if(!(f && f->isBuiltin()) && str2.empty()) {
			//no expression/relation -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 1);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression")));
			show_message(_("Empty expression field."), dialog);
			goto run_function_edit_dialog;
		}
		GtkTextIter d_iter_s, d_iter_e;
		gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
		gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
		gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
		//function with the same name exists -- overwrite or open the dialog again
		if((!f || !f->hasName(str)) && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) && CALCULATOR->functionNameTaken(str, f) && !ask_question(_("A function with the same name already exists.\nDo you want to overwrite the function?"), dialog)) {
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
			goto run_function_edit_dialog;
		}
		bool add_func = false;
		if(f) {
			f->setLocal(true);
			//edited an existing function
			f->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category"))));
			f->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc"))));
			f->setDescription(gstr_descr);
			if(!f->isBuiltin()) {				
				f->clearArgumentDefinitions();
			}	
		} else {
			//new function
			f = new UserFunction(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT((gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")))), "", "", true, -1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc"))), gstr_descr);
			add_func = true;
		}
		g_free(gstr_descr);
		if(f) {
			f->setCondition(CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition"))), evalops.parse_options));
			GtkTreeIter iter;
			bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionArguments_store), &iter);
			int i = 1;
			Argument *arg;
			while(b) {
				gtk_tree_model_get(GTK_TREE_MODEL(tFunctionArguments_store), &iter, 2, &arg, -1);
				if(arg && f->isBuiltin() && f->getArgumentDefinition(i)) {
					f->getArgumentDefinition(i)->setName(arg->name());
					delete arg;
				} else if(arg) {
					f->setArgumentDefinition(i, arg);
				}
				b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tFunctionArguments_store), &iter);
				i++;
			}
			if(!f->isBuiltin()) {
				((UserFunction*) f)->clearSubfunctions();
				b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tSubfunctions_store), &iter);
				while(b) {
					gchar *gstr;
					gboolean g_b = FALSE;
					gtk_tree_model_get(GTK_TREE_MODEL(tSubfunctions_store), &iter, 1, &gstr, 4, &g_b, -1);
					((UserFunction*) f)->addSubfunction(gstr, g_b);
					b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tSubfunctions_store), &iter);
					g_free(gstr);
				}
				((UserFunction*) f)->setFormula(str2);
			}
			f->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_hidden"))));
			set_edited_names(f, str);
			if(add_func) {
				CALCULATOR->addFunction(f);
			}
			if(!f->isActive()) {
				selected_function_category = _("Inactive");
			} else if(f->category().empty()) {
				selected_function_category = _("Uncategorized");
			} else {
				selected_function_category = "/";
				selected_function_category += f->category();
			}
			//select the new function
			selected_function = f;
		}
		update_fmenu();	
		function_inserted(f);
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-functions.html#qalculate-function-creation", gtk_builder_get_object(functionedit_builder, "function_edit_dialog")); 
		goto run_function_edit_dialog;
	}
	edited_function = NULL;
	names_edited = false;
	editing_function = false;
	gtk_widget_hide(dialog);
}

/*
	display edit/new function dialog
	creates new function if f == NULL, win is parent window
*/
void edit_function_simple(const char *category = "", MathFunction *f = NULL, GtkWidget *win = NULL) {

	if(f && f->subtype() == SUBTYPE_DATA_SET) {
		edit_dataset((DataSet*) f, win);
		return;
	}

	GtkWidget *dialog = get_simple_function_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	edited_function = f;
	editing_function = true;
	
	if(f) {
		if(f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Function"));
		else return edit_function(category, f, win);
		if(((UserFunction*) f)->countSubfunctions() > 0 || f->countNames() > 1 || !f->condition().empty() || f->lastArgumentDefinitionIndex() > 0 || !f->description().empty() || !f->title(false).empty() || !f->category().empty()) return edit_function(category, f, win);
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Function"));
	}

	GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_textview_expression")));

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name")), "");
	gtk_text_buffer_set_text(expression_buffer, "", -1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_button_ok")), TRUE);

	if(f) {
		//fill in original paramaters
		gtk_text_buffer_set_text(expression_buffer, CALCULATOR->localizeExpression(((UserFunction*) f)->formula()).c_str(), -1);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name")), f->getName(1).name.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_radiobutton_slash")), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_radiobutton_noslash")), TRUE);
	}

	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name")));
	
run_simple_function_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name")));
		remove_blank_ends(str);
		if(str.empty()) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_simple_function_edit_dialog;
		}
		GtkTextIter e_iter_s, e_iter_e;
		gtk_text_buffer_get_start_iter(expression_buffer, &e_iter_s);
		gtk_text_buffer_get_end_iter(expression_buffer, &e_iter_e);;
		gchar *gstr = gtk_text_buffer_get_text(expression_buffer, &e_iter_s, &e_iter_e, FALSE);
		string str2 = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
		g_free(gstr);
		remove_blank_ends(str2);
		gsub("\n", " ", str2);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_radiobutton_noslash")))) {
			gsub("x", "\\x", str2);
			gsub("y", "\\y", str2);
			gsub("z", "\\z", str2);
		}
		if(str2.empty()) {
			//no expression/relation -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_textview_expression")));
			show_message(_("Empty expression field."), dialog);
			goto run_simple_function_edit_dialog;
		}
		//function with the same name exists -- overwrite or open the dialog again
		if((!f || !f->hasName(str)) && CALCULATOR->functionNameTaken(str, f) && !ask_question(_("A function with the same name already exists.\nDo you want to overwrite the function?"), dialog)) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name")));
			goto run_simple_function_edit_dialog;
		}
		if(f) {
			//edited an existing function
			f->setLocal(true);
			((UserFunction*) f)->setFormula(str2);
			f->setName(str, 1);
		} else {
			//new function
			f = new UserFunction(category, str, str2);
			CALCULATOR->addFunction(f);
		}
		update_fmenu();
		function_inserted(f);
	}
	edited_function = NULL;
	names_edited = false;
	editing_function = false;
	gtk_widget_hide(dialog);
	if(response == 1) {
		GtkTextIter e_iter_s, e_iter_e;
		gtk_text_buffer_get_start_iter(expression_buffer, &e_iter_s);
		gtk_text_buffer_get_end_iter(expression_buffer, &e_iter_e);;
		gchar *gstr = gtk_text_buffer_get_text(expression_buffer, &e_iter_s, &e_iter_e, FALSE);
		string str2 = gstr;
		g_free(gstr);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_radiobutton_noslash")))) {
			gsub("x", "\\x", str2);
			gsub("y", "\\y", str2);
			gsub("z", "\\z", str2);
		}
		edit_function(category, f, win, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_entry_name"))), str2.c_str());
	}
}

/*
	"New function" menu item selected
*/
void new_function(GtkMenuItem*, gpointer)
{
	edit_function("", NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
/*
	"New unit" menu item selected
*/
void new_unit(GtkMenuItem*, gpointer)
{
	edit_unit("", NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

/*
	a unit selected in result menu, convert result
*/
void convert_to_unit(GtkMenuItem*, gpointer user_data)
{
	GtkWidget *edialog;
	Unit *u = (Unit*) user_data;
	if(!u) {
		edialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Unit does not exist"));
		gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
	}
	//result is stored in MathStructure *mstruct
	executeCommand(COMMAND_CONVERT_UNIT, true, "", u);
	focus_keeping_selection();
}

void edit_unknown(const char *category, Variable *var, GtkWidget *win) {

	if(var != NULL && var->isKnown()) {
		edit_variable(category, var, NULL, win);
		return;
	}
	
	UnknownVariable *v = (UnknownVariable*) var;
	edited_unknown = v;
	names_edited = false;
	editing_unknown = true;
	
	GtkWidget *dialog = get_unknown_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	
	if(v) {
		if(v->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Unknown Variable"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Unknown Variable (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Unknown Variable"));
	}
	
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_type_changed, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_sign_changed, NULL);
	if(v) {
		//fill in original parameters
		set_name_label_and_entry(v, GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_names")));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), !v->isBuiltin());
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combo_category")))), v->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_desc")), v->title(false).c_str());
		if(v->assumptions()) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_type")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_sign")), TRUE);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), v->assumptions()->type() - 2);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), v->assumptions()->sign());
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_type")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_sign")), FALSE);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), CALCULATOR->defaultAssumptions()->type() - 2);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), CALCULATOR->defaultAssumptions()->sign());
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), TRUE);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_type")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_sign")), TRUE);

		//fill in default values
		string v_name = CALCULATOR->getName();
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), v_name.c_str());
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_names")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combo_category")))), category);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_desc")), "");
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), CALCULATOR->defaultAssumptions()->type() - 2);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), CALCULATOR->defaultAssumptions()->sign());

	}
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_type_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_sign_changed, NULL);
	
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));

run_unknown_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));
		remove_blank_ends(str);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_unknown_edit_dialog;
		}

		//unknown with the same name exists -- overwrite or open dialog again
		if((!v || !v->hasName(str)) && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) && CALCULATOR->variableNameTaken(str, v) && !ask_question(_("An unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));
			goto run_unknown_edit_dialog;
		}
		if(!v) {
			//no need to create a new unknown when a unknown with the same name exists
			var = CALCULATOR->getActiveVariable(str);
			if(var && var->isLocal() && !var->isKnown()) v = (UnknownVariable*) var;
		}
		bool add_var = false;
		if(v) {
			//update existing unknown
			v->setLocal(true);
		} else {
			//new unknown
			v = new UnknownVariable("", "", "", true);
			add_var = true;
		}
		if(v) {
			if(!v->isBuiltin()) {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")))) {
					if(!v->assumptions()) v->setAssumptions(new Assumptions());
					v->assumptions()->setType((AssumptionType) (gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"))) + 2));
					v->assumptions()->setSign((AssumptionSign) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"))));
				} else {
					v->setAssumptions(NULL);
				}
			}
			v->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combo_category"))));
			v->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_desc"))));
			set_edited_names(v, str);
			if(add_var) {
				CALCULATOR->addVariable(v);
			}
			//select the new unknown
			selected_variable = v;
			if(!v->isActive()) {
				selected_variable_category = _("Inactive");
			} else if(v->category().empty()) {
				selected_variable_category = _("Uncategorized");
			} else {
				selected_variable_category = "/";
				selected_variable_category += v->category();
			}
		}
		update_vmenu();
		variable_inserted(v);
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-variables.html#qalculate-variable-creation", gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog")); 
		goto run_unknown_edit_dialog;
	}
	edited_unknown = NULL;
	names_edited = false;
	editing_unknown = false;
	gtk_widget_hide(dialog);
}

void delete_variable(Variable *v) {
	if(v && !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
		update_vmenu();
		return;
	}
	if(v && v->isLocal()) {
		for(size_t i = 0; i < recent_variables.size(); i++) {
			if(recent_variables[i] == v) {
				recent_variables.erase(recent_variables.begin() + i);
				gtk_widget_destroy(recent_variable_items[i]);
				recent_variable_items.erase(recent_variable_items.begin() + i);
				break;
			}
		}
		//ensure that all references are removed in Calculator
		v->destroy();
		update_vmenu();
	}
}


/*
	display edit/new variable dialog
	creates new variable if v == NULL, mstruct_ is forced value, win is parent window
*/
void edit_variable(const char *category, Variable *var, MathStructure *mstruct_, GtkWidget *win) {

	if(var != NULL && !var->isKnown()) {
		edit_unknown(category, var, win);
		return;
	}
	KnownVariable *v = (KnownVariable*) var;
	
	CALCULATOR->beginTemporaryStopMessages();
	if(v != NULL && v->get().isVector() && (!mstruct_ || mstruct_->isVector()) && (v->get().size() != 1 || !v->get()[0].isVector() || v->get()[0].size() > 0)) {
		CALCULATOR->endTemporaryStopMessages();
		edit_matrix(category, v, mstruct_, win);
		return;
	}

	edited_variable = v;
	names_edited = false;
	editing_variable = true;
	GtkWidget *dialog = get_variable_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	if(v) {
		if(v->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Variable"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Variable (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Variable"));
	}

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_button_ok")), TRUE);

	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(variableedit_builder, "variable_edit_label_names")), "");

	if(mstruct_) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_box_names")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_box_value")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact")));
	} else {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_box_names")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_box_value")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact")));
	}

	if(v) {
		//fill in original parameters
		set_name_label_and_entry(v, GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_label_names")));
		if(v->isExpression()) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value")), CALCULATOR->localizeExpression(v->expression()).c_str());
		else gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value")), get_value_string(v->get(), false, NULL).c_str());
		bool b_approx = *printops.is_approximate || v->isApproximate();
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact")), !b_approx);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact")), !v->isBuiltin());
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category")))), v->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_desc")), v->title(false).c_str());
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact")), TRUE);

		//fill in default values
		string v_name = CALCULATOR->getName();
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), v_name.c_str());
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(variableedit_builder, "variable_edit_label_names")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value")), displayed_mstruct ? get_value_string(*mstruct).c_str() : get_expression_text().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category")))), category);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_desc")), "");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact")), !mstruct_ || !mstruct_->isApproximate());
	}
	
	CALCULATOR->endTemporaryStopMessages();
	
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));

run_variable_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
		string str2 = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value"))), evalops.parse_options);
		remove_blank_ends(str);
		remove_blank_ends(str2);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_variable_edit_dialog;
		}
		if(str2.empty() && !mstruct_) {
			//no value -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_value")));
			show_message(_("Empty value field."), dialog);
			goto run_variable_edit_dialog;
		}
		//variable with the same name exists -- overwrite or open dialog again
		if((!v || !v->hasName(str)) && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) && CALCULATOR->variableNameTaken(str, v) && !ask_question(_("An unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
			goto run_variable_edit_dialog;
		}
		if(!v) {
			//no need to create a new variable when a variable with the same name exists
			var = CALCULATOR->getActiveVariable(str);
			if(var && var->isLocal() && var->isKnown()) v = (KnownVariable*) var;
		}
		bool add_var = false;
		if(v) {
			//update existing variable
			v->setLocal(true);
			if(!v->isBuiltin()) {
				if(mstruct_) {
					v->set(*mstruct_);
				} else {
					v->set(str2);
				}
				v->setApproximate(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact"))));
			}
		} else {
			//new variable
			if(mstruct_) {
				//forced value
				v = new KnownVariable("", "", *mstruct_, "", true);
			} else {
				v = new KnownVariable("", "", str2, "", true);
				v->setApproximate(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_exact"))));
			}
			add_var = true;
		}
		if(v) {
			v->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category"))));
			v->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_desc"))));
			set_edited_names(v, str);
			if(add_var) {
				CALCULATOR->addVariable(v);
			}
			//select the new variable
			selected_variable = v;
			if(!v->isActive()) {
				selected_variable_category = _("Inactive");
			} else if(v->category().empty()) {
				selected_variable_category = _("Uncategorized");
			} else {
				selected_variable_category = "/";
				selected_variable_category += v->category();
			}
		}
		update_vmenu();
		variable_inserted(v);
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-variables.html#qalculate-variable-creation", gtk_builder_get_object(variableedit_builder, "variable_edit_dialog")); 
		goto run_variable_edit_dialog;
	}
	edited_variable = NULL;
	names_edited = false;
	editing_variable = false;
	gtk_widget_hide(dialog);
}

/*
	display edit/new matrix dialog
	creates new matrix if v == NULL, mstruct_ is forced value, win is parent window
*/
void edit_matrix(const char *category, Variable *var, MathStructure *mstruct_, GtkWidget *win, gboolean create_vector) {

	if(var != NULL && !var->isKnown()) {
		edit_unknown(category, var, win);
		return;
	}
	
	KnownVariable *v = (KnownVariable*) var;

	if((v && !v->get().isVector()) || (mstruct_ && !mstruct_->isVector())) {
		edit_variable(category, v, mstruct_, win);
		return;
	}
	
	edited_matrix = v;
	names_edited = false;
	editing_matrix = true;

	GtkWidget *dialog = get_matrix_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	if(mstruct_) {
		create_vector = !mstruct_->isMatrix();
	} else if(v) {
		create_vector = !v->get().isMatrix();	
	}
	if(create_vector) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), TRUE);

	if(create_vector) {
		if(v) {
			if(v->isLocal())
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Vector"));
			else
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Vector (global)"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("New Vector"));
		}
	} else {
		if(v) {
			if(v->isLocal())
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Matrix"));
			else
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Matrix (global)"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("New Matrix"));
		}	
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_button_ok")), TRUE);		

	int r = 4, c = 4;
	const MathStructure *old_vctr = NULL;
	if(v) {
		if(create_vector) {
			old_vctr = &v->get();
		} else {
			c = v->get().columns();
			r = v->get().rows();
		}	
		//fill in original parameters
		set_name_label_and_entry(v, GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_names")));
		//can only change name and value of user variable
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), !v->isBuiltin());		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), !v->isBuiltin());				
		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_table_elements")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), !v->isBuiltin());						
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), !v->isBuiltin());								
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")))), v->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_desc")), v->title(false).c_str());	
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), TRUE);		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), TRUE);				
		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_table_elements")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), TRUE);						
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), TRUE);								
	
		//fill in default values
		string v_name = CALCULATOR->getName();
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), v_name.c_str());
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_names")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")))), category);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_desc")), "");		
	}
	if(mstruct_) {
		//forced value
		if(create_vector) {
			old_vctr = mstruct_;
		} else {
			c = mstruct_->columns();
			r = mstruct_->rows();
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), FALSE);		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), FALSE);				
		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_table_elements")), FALSE);						
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), FALSE);		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), FALSE);		
	}

	if(create_vector) {
		if(old_vctr) {
			r = old_vctr->countChildren();
			c = (int) ::sqrt(::sqrt((double) r)) + 8;
			if(c < 10) c = 10;
			if(r % c > 0) {
				r = r / c + 1;
			} else {
				r = r / c;
			}
			if(r < 100) r = 100;
		} else {
			c = 10;
			r = 100;
		}
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), r);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), c);
	on_matrix_edit_spinbutton_columns_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), NULL);
	on_matrix_edit_spinbutton_rows_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), NULL);		

	CALCULATOR->startControl(2000);
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL_EXACT;
	while(gtk_events_pending()) gtk_main_iteration();
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
	for(size_t index_r = 0; b && index_r < (size_t) r; index_r++) {
		for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
			if(create_vector) {
				if(old_vctr && index_r * c + index_c < old_vctr->countChildren()) {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, old_vctr->getChild(index_r * c + index_c + 1)->print(po).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "", -1);
				}
			} else {
				if(v) {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, v->get().getElement(index_r + 1, index_c + 1)->print(po).c_str(), -1);
				} else if(mstruct_) {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, mstruct_->getElement(index_r + 1, index_c + 1)->print(po).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "0", -1);
				}
			}
		}
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
	}
	CALCULATOR->stopControl();
	if(r > 0 && c > 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], TRUE);
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], FALSE, 0.0, 0.0);
		on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
		gtk_tree_path_free(path);
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_position")), "");
	}
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
	
run_matrix_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
		remove_blank_ends(str);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_matrix_edit_dialog;
		}

		//variable with the same name exists -- overwrite or open dialog again
		if((!v || !v->hasName(str)) && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) && CALCULATOR->variableNameTaken(str) && !ask_question(_("An unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
			goto run_matrix_edit_dialog;
		}
		if(!v) {
			//no need to create a new variable when a variable with the same name exists
			var = CALCULATOR->getActiveVariable(str);
			if(var && var->isLocal() && var->isKnown()) v = (KnownVariable*) var;
		}
		MathStructure mstruct_new;
		if(!mstruct_) {
			b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
			c = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")));
			r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")));
			gchar *gstr = NULL;
			string mstr;
			do_timeout = false;
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")))) {
				mstruct_new.clearVector();
				for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
					for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
						gtk_tree_model_get(GTK_TREE_MODEL(tMatrixEdit_store), &iter, index_c, &gstr, -1);
						mstr = gstr;
						g_free(gstr);
						remove_blank_ends(mstr);
						if(!mstr.empty()) {
							mstruct_new.addChild(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(mstr, evalops.parse_options), evalops.parse_options));
						}
					}
					b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
				}
			} else {
				mstruct_new.clearMatrix();
				mstruct_new.resizeMatrix((size_t) r, (size_t) c, m_undefined);
				for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
					for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
						gtk_tree_model_get(GTK_TREE_MODEL(tMatrixEdit_store), &iter, index_c, &gstr, -1);
						mstr = gstr;
						g_free(gstr);
						remove_blank_ends(mstr);
						mstruct_new.setElement(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(mstr, evalops.parse_options), evalops.parse_options), index_r + 1, index_c + 1);
					}
					b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
				}
			}
			display_errors(NULL, dialog);
			do_timeout = true;
		}
		bool add_var = false;
		if(v) {
			v->setLocal(true);
			//update existing variable
			if(!v->isBuiltin()) {
				if(mstruct_) {
					v->set(*mstruct_);
				} else {
					v->set(mstruct_new);
				}
			}
		} else {
			//new variable
			if(mstruct_) {
				v = new KnownVariable("", "", *mstruct_, "", true);
			} else {
				v = new KnownVariable("", "", mstruct_new, "", true);
			}
			add_var = true;
		}
		if(v) {
			v->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category"))));
			v->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_desc"))));
			set_edited_names(v, str);
			if(add_var) {
				CALCULATOR->addVariable(v);
			}
			//select the new variable
			selected_variable = v;
			if(!v->isActive()) {
				selected_variable_category = _("Inactive");
			} else if(v->category().empty()) {
				selected_variable_category = _("Uncategorized");
			} else {
				selected_variable_category = "/";
				selected_variable_category += v->category();
			}
		}
		update_vmenu();
		variable_inserted(v);
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-variables.html#qalculate-vectors-matrices", gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog")); 
		goto run_matrix_edit_dialog;
	}
	edited_matrix = NULL;
	names_edited = false;
	editing_matrix = false;
	gtk_widget_hide(dialog);
}

void insert_matrix(const MathStructure *initial_value, GtkWidget *win, gboolean create_vector, bool is_text_struct, bool is_result) {

	GtkWidget *dialog = get_matrix_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	if(initial_value && !initial_value->isVector()) {
		return;
	}
	
	GtkTextIter istart, iend;
	gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);

	if(initial_value) {
		create_vector = !initial_value->isMatrix();
	}
	if(create_vector) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_vector")), TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")), TRUE);

	if(is_result) {
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_button_cancel")), _("_Close"));
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_button_cancel")));
		if(create_vector) {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Vector Result"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Matrix Result"));
		}
	} else {
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_button_cancel")), _("_Cancel"));
		gtk_widget_grab_focus(tMatrix);
		if(create_vector) {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Vector"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Matrix"));
		}
	}

	int r = 4, c = 4;
	if(create_vector) {
		if(initial_value) {
			r = initial_value->countChildren();
			c = (int) sqrt(::sqrt(r)) + 8;
			if(c < 10) c = 10;
			if(r % c > 0) {
				r = r / c + 1;
			} else {
				r = r / c;
			}
			if(r < 100) r = 100;
		} else {
			c = 10;
			r = 100;
		}
	} else if(initial_value) {
		c = initial_value->columns();
		r = initial_value->rows();
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_rows")), r);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_columns")), c);
	on_matrix_spinbutton_columns_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_columns")), NULL);
	on_matrix_spinbutton_rows_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_rows")), NULL);


	printops.can_display_unicode_string_arg = (void*) tMatrix;
	while(gtk_events_pending()) gtk_main_iteration();
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrix_store), &iter);
	CALCULATOR->startControl(5000);
	for(size_t index_r = 0; b && index_r < (size_t) r; index_r++) {
		for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
			if(create_vector) {
				if(initial_value && index_r * c + index_c < initial_value->countChildren()) {
					if(is_text_struct) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getChild(index_r * c + index_c + 1)->symbol().c_str(), -1);
					else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getChild(index_r * c + index_c + 1)->print(printops).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "", -1);
				}
			} else {
				if(initial_value) {
					if(is_text_struct) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getElement(index_r + 1, index_c + 1)->symbol().c_str(), -1);
					else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getElement(index_r + 1, index_c + 1)->print(printops).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "0", -1);
				}
			}
		}
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter);
	}
	CALCULATOR->stopControl();

	if(r > 0 && c > 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], TRUE);
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], FALSE, 0.0, 0.0);
		on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
		gtk_tree_path_free(path);
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_position")), "");
	}
	printops.can_display_unicode_string_arg = NULL;

	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string matrixstr, str;
		bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrix_store), &iter);
		c = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_columns")));
		r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_rows")));
		gchar *gstr = NULL;
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_vector")))) {
			bool b1 = false;
			matrixstr = "[";
			for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
				for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
					gtk_tree_model_get(GTK_TREE_MODEL(tMatrix_store), &iter, index_c, &gstr, -1);
					str = gstr;
					g_free(gstr);
					remove_blank_ends(str);
					if(!str.empty()) {
						if(b1) {
							matrixstr += CALCULATOR->getComma();
							matrixstr += " ";
						} else {
							b1 = true;
						}
						matrixstr += str;
					}
				}
				b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter);
			}
			matrixstr += "]";
		} else {
			matrixstr = "[";
			bool b1 = false;
			for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
				if(b1) {
					matrixstr += CALCULATOR->getComma();
					matrixstr += " ";
				} else {
					b1 = true;
				}
				matrixstr += "[";
				bool b2 = false;
				for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
					if(b2) {
						matrixstr += CALCULATOR->getComma();
						matrixstr += " ";
					} else {
						b2 = true;
					}
					gtk_tree_model_get(GTK_TREE_MODEL(tMatrix_store), &iter, index_c, &gstr, -1);
					str = gstr;
					remove_blank_ends(str);
					g_free(gstr);
					matrixstr += str;
				}
				matrixstr += "]";
				b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter);
			}
			matrixstr += "]";
		}
		gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
		insert_text(matrixstr.c_str());
	}
	gtk_widget_hide(dialog);
}


void edit_dataobject(DataSet *ds, DataObject *o, GtkWidget *win) {
	if(!ds) return;
	GtkWidget *dialog = get_dataobject_edit_dialog();
	if(o) {
		gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Data Object"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Data Object"));
	}
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	GtkWidget *ptable = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_grid"));
	GList *childlist = gtk_container_get_children(GTK_CONTAINER(ptable));
	for(guint i = 0; ; i++) {
		GtkWidget *w = (GtkWidget*) g_list_nth_data(childlist, i);
		if(!w) break;
		gtk_widget_destroy(w);
	}
	g_list_free(childlist);
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	string sval;
	int rows = 1;
	gtk_grid_remove_column(GTK_GRID(ptable), 0);
	gtk_grid_remove_column(GTK_GRID(ptable), 1);
	gtk_grid_remove_column(GTK_GRID(ptable), 2);
	gtk_grid_remove_column(GTK_GRID(ptable), 3);
	gtk_grid_set_column_spacing(GTK_GRID(ptable), 20);
	GtkWidget *label, *entry, *om;
	vector<GtkWidget*> value_entries;
	vector<GtkWidget*> approx_menus;
	string str;
	while(dp) {
		label = gtk_label_new(dp->title().c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);
		
		entry = gtk_entry_new();
		value_entries.push_back(entry);
		int iapprox = -1;
		if(o) {
			gtk_entry_set_text(GTK_ENTRY(entry), o->getProperty(dp, &iapprox).c_str());
		}
		gtk_grid_attach(GTK_GRID(ptable), entry, 1, rows - 1, 1, 1);
		
		label = gtk_label_new(dp->getUnitString().c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(ptable), label, 2, rows - 1, 1, 1);
		
		om = gtk_combo_box_text_new();
		approx_menus.push_back(om);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(om), NULL, _("Default"));
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(om), NULL, _("Approximate"));
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(om), NULL, _("Exact"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(om), iapprox + 1);

		gtk_grid_attach(GTK_GRID(ptable), om, 3, rows - 1, 1, 1);

		rows++;
		dp = ds->getNextProperty(&it);
	}
	gtk_widget_show_all(ptable);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		bool new_object = (o == NULL);
		if(new_object) {
			o = new DataObject(ds);
			ds->addObject(o);
		}
		dp = ds->getFirstProperty(&it);
		size_t i = 0;
		string val;
		while(dp) {
			val = gtk_entry_get_text(GTK_ENTRY(value_entries[i]));
			remove_blank_ends(val);
			if(!val.empty()) {
				o->setProperty(dp, val, gtk_combo_box_get_active(GTK_COMBO_BOX(approx_menus[i])) - 1);
			} else if(!new_object) {
				o->eraseProperty(dp);
			}
			dp = ds->getNextProperty(&it);
			i++;
		}
		o->setUserModified();
		selected_dataobject = o;
		update_dataobjects();
	}
	/*for(size_t i = 0; i < approx_menus.size(); i++) {
		menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(approx_menus[i]));
		gtk_widget_destroy(menu);
	}*/
	gtk_widget_hide(dialog);
}

void update_dataset_property_list(DataSet*) {
	if(!datasetedit_builder) return;
	selected_dataproperty = NULL;
	gtk_list_store_clear(tDataProperties_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), FALSE);
	GtkTreeIter iter;
	string str;
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i]) {
			gtk_list_store_append(tDataProperties_store, &iter);
			str = "";
			switch(tmp_props[i]->propertyType()) {
				case PROPERTY_STRING: {
					str += _("text");
					break;
				}
				case PROPERTY_NUMBER: {
					if(tmp_props[i]->isApproximate()) {
						str += _("approximate");
						str += " ";
					}
					str += _("number");
					break;
				}
				case PROPERTY_EXPRESSION: {
					if(tmp_props[i]->isApproximate()) {
						str += _("approximate");
						str += " ";
					}
					str += _("expression");
					break;
				}
			}
			if(tmp_props[i]->isKey()) {
				str += " (";
				str += _("key");
				str += ")";
			}
			gtk_list_store_set(tDataProperties_store, &iter, 0, tmp_props[i]->title(false).c_str(), 1, tmp_props[i]->getName().c_str(), 2, str.c_str(), 3, (gpointer) tmp_props[i], -1);
		}
	}
}

bool edit_dataproperty(DataProperty *dp) {

	GtkWidget *dialog = get_dataproperty_edit_dialog();	
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog")));

	edited_dataproperty = dp;	
	names_edited = false;
	editing_dataproperty = true;

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")), dp->getName().c_str());
	if(dp->countNames() > 1) {
		string str = "+ ";
		for(size_t i = 2; i <= dp->countNames(); i++) {
			if(i > 2) str += ", ";
			str += dp->getName(i);
		}
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_label_names")), str.c_str());
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_label_names")), "");
	}

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_title")), dp->title(false).c_str());
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_unit")), dp->getUnitString().c_str());

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_hide")), dp->isHidden());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_key")), dp->isKey());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_approximate")), dp->isApproximate());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_case")), dp->isCaseSensitive());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_brackets")), dp->usesBrackets());
	
	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_textview_description")));
	gtk_text_buffer_set_text(description_buffer, dp->description().c_str(), -1);

	switch(dp->propertyType()) {	
		case PROPERTY_STRING: {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 0);
			break;
		}
		case PROPERTY_NUMBER: {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 1);
			break;
		}
		case PROPERTY_EXPRESSION: {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 2);
			break;
		}
	}
	
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")));
	
	bool return_val = false;

	run_dataproperty_edit_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
	
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")));
		remove_blank_ends(str);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_dataproperty_edit_dialog;
		}
	
		dp->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_title"))));
		dp->setUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_unit"))));
		
		dp->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_hide"))));
		dp->setKey(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_key"))));
		dp->setApproximate(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_approximate"))));
		dp->setCaseSensitive(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_case"))));
		dp->setUsesBrackets(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_brackets"))));
	
		GtkTextIter e_iter_s, e_iter_e;
		gtk_text_buffer_get_start_iter(description_buffer, &e_iter_s);
		gtk_text_buffer_get_end_iter(description_buffer, &e_iter_e);
		gchar *gstr = gtk_text_buffer_get_text(description_buffer, &e_iter_s, &e_iter_e, FALSE);
		dp->setDescription(gstr);
		g_free(gstr);
		
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")))) {
			case 0: {
				dp->setPropertyType(PROPERTY_STRING);
				break;
			}
			case 1: {
				dp->setPropertyType(PROPERTY_NUMBER);
				break;
			}
			case 2: {
				dp->setPropertyType(PROPERTY_EXPRESSION);
				break;
			}
		}
		if(names_edited) {
			dp->clearNames();
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {
				gchar *gstr;
				while(true) {	
					gboolean reference = FALSE;
					gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, NAMES_REFERENCE_COLUMN, &reference, -1);
					dp->addName(gstr, reference);
					g_free(gstr);
					if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tNames_store), &iter)) break;
				}
			} else {
				dp->addName(str);
			}
		} else {
			dp->setName(str, 1);
		}
		
		return_val = true;
			
	}
	
	names_edited = false;
	editing_dataproperty = false;
	edited_dataproperty = NULL;
	
	gtk_widget_hide(dialog);
	
	return return_val;
	
}


void edit_dataset(DataSet *ds, GtkWidget *win) {
	GtkWidget *dialog = get_dataset_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	
	edited_dataset = ds;
	names_edited = false;
	editing_dataset = true;
	
	if(ds) {
		if(ds->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Data Set"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Data Set (global)"));		
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Data Set"));
	}

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_description")));
	GtkTextBuffer *copyright_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_copyright")));

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_copyright")), !ds || ds->isLocal());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), !ds || ds->isLocal());

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), "");
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(datasetedit_builder, "dataset_edit_label_names")), "");
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), TRUE);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_object_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_property_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_default_property")), _("info"));
	gtk_text_buffer_set_text(description_buffer, "", -1);
	gtk_text_buffer_set_text(copyright_buffer, "", -1);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_ok")), TRUE);	

	gtk_list_store_clear(tDataProperties_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), FALSE);		
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_new_property")), TRUE);
	
	if(ds) {
		//fill in original paramaters
		set_name_label_and_entry(ds, GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_label_names")));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc")), ds->title(false).c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), ds->defaultDataFile().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_default_property")), ds->defaultProperty().c_str());
		Argument *arg = ds->getArgumentDefinition(1);
		if(arg) {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_object_name")), arg->name().c_str());
		}
		arg = ds->getArgumentDefinition(2);
		if(arg) {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_property_name")), arg->name().c_str());
		}
		gtk_text_buffer_set_text(description_buffer, ds->description().c_str(), -1);
		gtk_text_buffer_set_text(copyright_buffer, ds->copyright().c_str(), -1);
		DataPropertyIter it;
		DataProperty *dp = ds->getFirstProperty(&it);
		while(dp) {
			tmp_props.push_back(new DataProperty(*dp));
			tmp_props_orig.push_back(dp);
			dp = ds->getNextProperty(&it);
		}
	}
	update_dataset_property_list(ds);
	
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(datasetedit_builder, "dataset_edit_tabs")), 0);
	
	run_dataset_edit_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
		remove_blank_ends(str);
		GtkTreeIter iter;
		if(str.empty() && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter))) {
			//no name -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(datasetedit_builder, "dataset_edit_tabs")), 2);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_dataset_edit_dialog;
		}
		GtkTextIter d_iter_s, d_iter_e;
		gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
		gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
		GtkTextIter c_iter_s, c_iter_e;
		gtk_text_buffer_get_start_iter(copyright_buffer, &c_iter_s);
		gtk_text_buffer_get_end_iter(copyright_buffer, &c_iter_e);
		//dataset with the same name exists -- overwrite or open the dialog again
		if((!ds || !ds->hasName(str)) && (!names_edited || !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) && CALCULATOR->functionNameTaken(str, ds) && !ask_question(_("A function with the same name already exists.\nDo you want to overwrite the function?"), dialog)) {
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(datasetedit_builder, "dataset_edit_tabs")), 2);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
			goto run_dataset_edit_dialog;
		}
		bool add_func = false;
		gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
		if(ds) {
			//edited an existing dataset
			ds->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc"))));
			if(ds->isLocal()) ds->setDefaultDataFile(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file"))));
			ds->setDescription(gstr_descr);
		} else {
			//new dataset
			ds = new DataSet(_("Data Sets"), "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file"))), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc"))), gstr_descr, true);
			add_func = true;
		}
		g_free(gstr_descr);
		string str2;
		if(ds) {
			str2 = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_object_name")));
			remove_blank_ends(str2);
			if(str2.empty()) str2 = _("Object");
			Argument *arg = ds->getArgumentDefinition(1);
			if(arg) {
				arg->setName(str2);
			}
			str2 = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_property_name")));
			remove_blank_ends(str2);
			if(str2.empty()) str2 = _("Property");
			arg = ds->getArgumentDefinition(2);
			if(arg) {
				arg->setName(str2);
			}
			ds->setDefaultProperty(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_default_property"))));
			gchar *gstr = gtk_text_buffer_get_text(copyright_buffer, &c_iter_s, &c_iter_e, FALSE);
			ds->setCopyright(gstr);
			g_free(gstr);
			DataPropertyIter it;
			for(size_t i = 0; i < tmp_props.size();) {
				if(!tmp_props[i]) {
					if(tmp_props_orig[i]) ds->delProperty(tmp_props_orig[i]);
					i++;
				} else if(tmp_props[i]->isUserModified()) {
					if(tmp_props_orig[i]) {
						tmp_props_orig[i]->set(*tmp_props[i]);
						i++;
					} else {
						ds->addProperty(tmp_props[i]);
						tmp_props.erase(tmp_props.begin() + i);
					}
				} else {
					i++;
				}
			}
			set_edited_names(ds, str);
			if(add_func) {
				CALCULATOR->addDataSet(ds);
				ds->loadObjects();
				ds->setObjectsLoaded(true);
			}
			selected_dataset = ds;
		}
		update_fmenu();	
		function_inserted(ds);
		update_datasets_tree();
	}
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i]) delete tmp_props[i];
	}
	tmp_props.clear();
	tmp_props_orig.clear();
	edited_dataset = NULL;
	editing_dataset = false;
	names_edited = false;
	gtk_widget_hide(dialog);
}

void import_csv_file(GtkWidget *win) {

	GtkWidget *dialog = get_csv_import_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")), "");	
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_desc")), "");
	
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));

run_csv_import_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));
		remove_blank_ends(str);
		if(str.empty()) {
			//no filename -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));
			show_message(_("No file name entered."), dialog);
			goto run_csv_import_dialog;
		}
		string name_str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
		remove_blank_ends(name_str);
		if(name_str.empty()) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_csv_import_dialog;
		}
		//variable with the same name exists -- overwrite or open dialog again
		if(CALCULATOR->variableNameTaken(name_str) && !ask_question(_("An unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
			goto run_csv_import_dialog;
		}
		string delimiter = "";
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(csvimport_builder, "csv_import_combobox_delimiter")))) {
			case DELIMITER_COMMA: {
				delimiter = ",";
				break;
			}
			case DELIMITER_TABULATOR: {
				delimiter = "\t";
				break;
			}			
			case DELIMITER_SEMICOLON: {
				delimiter = ";";
				break;
			}		
			case DELIMITER_SPACE: {
				delimiter = " ";
				break;
			}				
			case DELIMITER_OTHER: {
				delimiter = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_delimiter_other")));
				break;
			}			
		}
		if(delimiter.empty()) {
			//no filename -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_delimiter_other")));
			show_message(_("No delimiter selected."), dialog);
			goto run_csv_import_dialog;
		}
		do_timeout = false;
		if(!CALCULATOR->importCSV(str.c_str(), gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(csvimport_builder, "csv_import_spinbutton_first_row"))), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvimport_builder, "csv_import_checkbutton_headers"))), delimiter, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvimport_builder, "csv_import_radiobutton_matrix"))), name_str, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_desc"))), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(csvimport_builder, "csv_import_combo_category"))))) {
			GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not import from file \n%s"), str.c_str());
			gtk_dialog_run(GTK_DIALOG(edialog));
			gtk_widget_destroy(edialog);
		}
		display_errors(NULL, dialog);
		do_timeout = true;
		update_vmenu();
	}
	gtk_widget_hide(dialog);
}

void export_csv_file(KnownVariable *v, GtkWidget *win) {

	GtkWidget *dialog = get_csv_export_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	if(v) {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")), v->preferredDisplayName(false, false, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")).name.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), v->preferredDisplayName(false, false, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")).name.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_matrix")), TRUE);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), "");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")), TRUE);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_matrix")), !v);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")), !v);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), FALSE);
	
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));

run_csv_export_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
		remove_blank_ends(str);
		if(str.empty()) {
			//no filename -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
			show_message(_("No file name entered."), dialog);
			goto run_csv_export_dialog;
		}
		string delimiter = "";
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(csvexport_builder, "csv_export_combobox_delimiter")))) {
			case DELIMITER_COMMA: {
				delimiter = ",";
				break;
			}
			case DELIMITER_TABULATOR: {
				delimiter = "\t";
				break;
			}			
			case DELIMITER_SEMICOLON: {
				delimiter = ";";
				break;
			}		
			case DELIMITER_SPACE: {
				delimiter = " ";
				break;
			}				
			case DELIMITER_OTHER: {
				delimiter = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_delimiter_other")));
				break;
			}			
		}
		if(delimiter.empty()) {
			//no delimiter -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_delimiter_other")));
			show_message(_("No delimiter selected."), dialog);
			goto run_csv_export_dialog;
		}
		MathStructure *matrix_struct;
		if(v) {
			matrix_struct = (MathStructure*) &v->get();
		} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")))) {
			matrix_struct = mstruct;
		} else {
			string str2 = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
			remove_blank_ends(str2);
			if(str2.empty()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
				show_message(_("No variable name entered."), dialog);
				goto run_csv_export_dialog;
			}
			Variable *var = CALCULATOR->getActiveVariable(str2);
			if(!var || !var->isKnown()) {
				var = CALCULATOR->getVariable(str2);
				while(var && !var->isKnown()) {
					var = CALCULATOR->getVariable(str2);
				}
			}
			if(!var || !var->isKnown()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
				show_message(_("No known variable with entered name found."), dialog);
				goto run_csv_export_dialog;
			}
			matrix_struct = (MathStructure*) &((KnownVariable*) var)->get();
		}
		CALCULATOR->startControl(600000);
		if(!CALCULATOR->exportCSV(*matrix_struct, str.c_str(), delimiter) && CALCULATOR->aborted()) {
			GtkWidget *edialog = gtk_message_dialog_new(
				GTK_WINDOW(
					gtk_builder_get_object(main_builder, "main_window")
				),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Could not export to file \n%s"),
				str.c_str()
			);
			gtk_dialog_run(GTK_DIALOG(edialog));
			gtk_widget_destroy(edialog);
		}
		CALCULATOR->stopControl();
	}
	gtk_widget_hide(dialog);
	
}

void edit_names(ExpressionItem *item, const gchar *namestr, GtkWidget *win, bool is_dp, DataProperty *dp) {

	GtkWidget *dialog = get_names_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	
	GtkTreeIter iter;
	
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_editbox1")), !(item->isBuiltin() && !(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_DATA_SET)));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_editbox2")), !(item->isBuiltin() && !(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_DATA_SET)));
	
	if(!names_edited) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_remove")), FALSE);
	
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")), "");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode")), FALSE);
		
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation")), !is_dp);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural")), !is_dp);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix")), !is_dp);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input")), !is_dp);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive")), !is_dp);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode")), !is_dp);

		gtk_list_store_clear(tNames_store);
		
		if(!is_dp && item && item->countNames() > 0) {
			for(size_t i = 1; i <= item->countNames(); i++) {
				const ExpressionName *ename = &item->getName(i);
				gtk_list_store_append(tNames_store, &iter);
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, ename->name.c_str(), NAMES_ABBREVIATION_STRING_COLUMN, b2yn(ename->abbreviation), NAMES_PLURAL_STRING_COLUMN, b2yn(ename->plural), NAMES_REFERENCE_STRING_COLUMN, b2yn(ename->reference), NAMES_ABBREVIATION_COLUMN, ename->abbreviation, NAMES_PLURAL_COLUMN, ename->plural, NAMES_UNICODE_COLUMN, ename->unicode, NAMES_REFERENCE_COLUMN, ename->reference, NAMES_SUFFIX_COLUMN, ename->suffix, NAMES_AVOID_INPUT_COLUMN, ename->avoid_input, NAMES_CASE_SENSITIVE_COLUMN, ename->case_sensitive, -1);
				if(i == 1 && namestr && strlen(namestr) > 0) {
					gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, -1);
				}
			}
		} else if(is_dp && dp && dp->countNames() > 0) {
			for(size_t i = 1; i <= dp->countNames(); i++) {
				gtk_list_store_append(tNames_store, &iter);
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, dp->getName(i).c_str(), NAMES_ABBREVIATION_STRING_COLUMN, "-", NAMES_PLURAL_STRING_COLUMN, "-", NAMES_REFERENCE_STRING_COLUMN, b2yn(dp->nameIsReference(i)), NAMES_ABBREVIATION_COLUMN, FALSE, NAMES_PLURAL_COLUMN, FALSE, NAMES_UNICODE_COLUMN, FALSE, NAMES_REFERENCE_COLUMN, dp->nameIsReference(i), NAMES_SUFFIX_COLUMN, FALSE, NAMES_AVOID_INPUT_COLUMN, FALSE, NAMES_CASE_SENSITIVE_COLUMN, FALSE, -1);
				if(i == 1 && namestr && strlen(namestr) > 0) {
					gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, -1);
				}
			}
		} else if(namestr && strlen(namestr) > 0) {
			gtk_list_store_append(tNames_store, &iter);
			if(is_dp) {
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, NAMES_ABBREVIATION_STRING_COLUMN, "-", NAMES_PLURAL_STRING_COLUMN, "-", NAMES_REFERENCE_STRING_COLUMN, b2yn(true), NAMES_ABBREVIATION_COLUMN, FALSE, NAMES_PLURAL_COLUMN, FALSE, NAMES_UNICODE_COLUMN, FALSE, NAMES_REFERENCE_COLUMN, TRUE, NAMES_SUFFIX_COLUMN, FALSE, NAMES_AVOID_INPUT_COLUMN, FALSE, NAMES_CASE_SENSITIVE_COLUMN, FALSE, -1);
			} else {
				ExpressionName ename(namestr);	
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, ename.name.c_str(), NAMES_ABBREVIATION_STRING_COLUMN, b2yn(ename.abbreviation), NAMES_PLURAL_STRING_COLUMN, b2yn(ename.plural), NAMES_REFERENCE_STRING_COLUMN, b2yn(ename.reference), NAMES_ABBREVIATION_COLUMN, ename.abbreviation, NAMES_PLURAL_COLUMN, ename.plural, NAMES_UNICODE_COLUMN, ename.unicode, NAMES_REFERENCE_COLUMN, ename.reference, NAMES_SUFFIX_COLUMN, ename.suffix, NAMES_AVOID_INPUT_COLUMN, ename.avoid_input, NAMES_CASE_SENSITIVE_COLUMN, ename.case_sensitive, -1);
			}
		}
	} else if(namestr && strlen(namestr) > 0) {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {
			gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, -1);
		}
		on_tNames_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames)), NULL);
	}
	
	gtk_dialog_run(GTK_DIALOG(dialog));
	names_edited = true;
	
	gtk_widget_hide(dialog);
}


/*
	add a new variable (from menu) with the value of result
*/
void add_as_variable()
{
	edit_variable(CALCULATOR->temporaryCategory().c_str(), NULL, mstruct, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

void new_unknown(GtkMenuItem*, gpointer)
{
	edit_unknown(_("My Variables"), NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

/*
	add a new variable (from menu)
*/
void new_variable(GtkMenuItem*, gpointer)
{
	edit_variable(_("My Variables"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

/*
	add a new matrix (from menu)
*/
void new_matrix(GtkMenuItem*, gpointer)
{
	edit_matrix(_("Matrices"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), FALSE);
}
/*
	add a new vector (from menu)
*/
void new_vector(GtkMenuItem*, gpointer)
{
	edit_matrix(_("Vectors"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), TRUE);
}

bool is_number(const gchar *expr) {
	string str = CALCULATOR->unlocalizeExpression(expr, evalops.parse_options);
	CALCULATOR->parseSigns(str);
	for(size_t i = 0; i < str.length(); i++) {
		if(is_not_in(NUMBER_ELEMENTS, str[i]) && (i > 0 || str.length() == 1 || is_not_in(MINUS PLUS, str[0]))) return false;
	}
	return true;
}
bool last_is_number(const gchar *expr) {
	string str = CALCULATOR->unlocalizeExpression(expr, evalops.parse_options);
	CALCULATOR->parseSigns(str);
	if(str.empty()) return false;
	return is_not_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1]);
}

/*
	insert function when button clicked
*/
void insertButtonFunction(MathFunction *f, bool save_to_recent = false, bool apply_to_stack = true) {
	if(!f) return;
	if(rpn_mode && apply_to_stack && (f->minargs() <= 1 || (int) CALCULATOR->RPNStackSize() >= f->minargs())) {
		calculateRPN(f);
		return;
	}
	if(f->minargs() > 1) return insert_function(f, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), save_to_recent);
	
	//insert one-argument function
	const ExpressionName *ename = &f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
	GtkTextIter istart, iend, ipos;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *expr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expressionbuffer);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mpos);
	// special case: the user just entered a number, then select all, so that it gets executed
	if(f != CALCULATOR->f_factorial && gtk_text_iter_is_end(&ipos) && last_is_number(expr)) {
		gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
	}
	if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
		gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);
		// execute expression, if the whole expression was selected, no need for additional enter
		bool do_exec = !rpn_mode && gtk_text_iter_is_start(&istart) && gtk_text_iter_is_end(&iend);
		//set selection as argument
		gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
		gchar *gstr2;
		if(f == CALCULATOR->f_factorial) {
			gstr2 = g_strdup_printf("(%s)!", gstr);
		} else {
			gstr2 = g_strdup_printf("%s(%s)", ename->name.c_str(), gstr);
		}
		insert_text(gstr2);
		if(do_exec) execute_expression();
		g_free(gstr);
		g_free(gstr2);
	} else {
		if(f == CALCULATOR->f_factorial) {
			insert_text("!");
		} else {
			gchar *gstr2;
			//one-argument functions do not need parenthesis
			/*if(!text_length_is_one(ename->name)) {
				gstr2 = g_strdup_printf("%s ", text);
			} else {
				gstr2 = g_strdup_printf("%s", text);
			}*/
			gstr2 = g_strdup_printf("%s()", ename->name.c_str());
			insert_text(gstr2);
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iter, gtk_text_buffer_get_insert(expressionbuffer));
			gtk_text_iter_backward_char(&iter);
			gtk_text_buffer_place_cursor(expressionbuffer, &iter);
			g_free(gstr2);
		}
	}
	g_free(expr);
	if(save_to_recent) function_inserted(f);
}
void insert_button_function(GtkMenuItem*, gpointer user_data) {
	insertButtonFunction((MathFunction*) user_data);
}
void insert_button_function_save(GtkMenuItem*, gpointer user_data) {
	insertButtonFunction((MathFunction*) user_data, true);
}
void insert_button_function_norpn(GtkMenuItem*, gpointer user_data) {
	insertButtonFunction((MathFunction*) user_data, true, false);
}


/*
	Button clicked -- insert text (1,2,3,... +,-,...)
*/
void button_pressed(GtkButton*, gpointer user_data) {
	insert_text((gchar*) user_data);
}


/*
	Update angle menu
*/
void set_angle_item() {
	GtkWidget *mi = NULL;
	switch(evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_RADIANS: {
			mi = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_radians"));
			g_signal_handlers_block_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_radians_activate, NULL);	
			if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), TRUE);
			g_signal_handlers_unblock_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_radians_activate, NULL);						
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			mi = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_gradians"));
			g_signal_handlers_block_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_gradians_activate, NULL);	
			if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), TRUE);
			g_signal_handlers_unblock_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_gradians_activate, NULL);						
			break;
		}
		case ANGLE_UNIT_DEGREES: {
			mi = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_degrees"));
			g_signal_handlers_block_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_degrees_activate, NULL);	
			if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), TRUE);
			g_signal_handlers_unblock_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_degrees_activate, NULL);
			break;
		}
		default: {
			mi = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_no_default_angle_unit"));
			g_signal_handlers_block_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_no_default_angle_unit_activate, NULL);
			if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), TRUE);
			g_signal_handlers_unblock_matched((gpointer) mi, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_no_default_angle_unit_activate, NULL);
		}
	}
}

/*
	Update angle radio buttons
*/
void set_angle_button() {
	GtkWidget *tb = NULL;
	switch(evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_RADIANS: {
			tb = GTK_WIDGET(gtk_builder_get_object(main_builder, "radiobutton_radians"));
			g_signal_handlers_block_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_radians_toggled, NULL);		
			if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb)))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb), TRUE);
			g_signal_handlers_unblock_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_radians_toggled, NULL);							
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			tb = GTK_WIDGET(gtk_builder_get_object(main_builder, "radiobutton_gradians"));
			g_signal_handlers_block_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_gradians_toggled, NULL);		
			if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb)))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb), TRUE);
			g_signal_handlers_unblock_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_gradians_toggled, NULL);							
			break;
		}
		case ANGLE_UNIT_DEGREES: {
			tb = GTK_WIDGET(gtk_builder_get_object(main_builder, "radiobutton_degrees"));
			g_signal_handlers_block_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_degrees_toggled, NULL);		
			if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb)))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb), TRUE);
			g_signal_handlers_unblock_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_degrees_toggled, NULL);
			break;
		}
		default: {
			tb = GTK_WIDGET(gtk_builder_get_object(main_builder, "radiobutton_no_default_angle_unit"));
			g_signal_handlers_block_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_no_default_angle_unit_toggled, NULL);
			if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb)))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb), TRUE);
			g_signal_handlers_unblock_matched((gpointer) tb, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_radiobutton_no_default_angle_unit_toggled, NULL);
			break;
		}
	}

}

/*
	variables, functions and units enabled/disabled from menu
*/
void set_clean_mode(GtkMenuItem *w, gpointer) {
	gboolean b = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	evalops.parse_options.functions_enabled = !b;
	evalops.parse_options.variables_enabled = !b;
	evalops.parse_options.units_enabled = !b;
	expression_format_updated(true);
}

/*
	Open variable manager
*/
void manage_variables() {
	GtkWidget *dialog = get_variables_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
}

/*
	Open function manager
*/
void manage_functions() {
	GtkWidget *dialog = get_functions_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));	
}

/*
	Open unit manager
*/
void manage_units() {
	GtkWidget *dialog = get_units_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
}
/*
	selected item in unit conversion menu in unit manager has changed -- update conversion
*/
void on_units_combobox_to_unit_changed(GtkComboBox *w, gpointer) {
	Unit *u;
	GtkTreeIter iter;
	if(gtk_combo_box_get_active(w) < 0) return;
	if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(tUnits_store), &iter, NULL, gtk_combo_box_get_active(w))) {
		gtk_tree_model_get(GTK_TREE_MODEL(tUnits_store), &iter, UNITS_POINTER_COLUMN, &u, -1);
		selected_to_unit = u;
		convert_in_wUnits();
	}
}

/*
	do the conversion in unit manager
*/
void convert_in_wUnits(int toFrom) {
	//units
	Unit *uFrom = get_selected_unit();
	Unit *uTo = get_selected_to_unit();
	
	if(uFrom && uTo) {
		//values
		const gchar *fromValue = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")));
		const gchar *toValue = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")));
		old_fromValue = fromValue;
		old_toValue = toValue;
		//determine conversion direction
		bool b = false;
		if(toFrom > 0) {
			if(CALCULATOR->timedOutString() == toValue) return;
			if(uFrom == uTo) {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")), toValue);
			} else {
				EvaluationOptions eo;
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.parse_options.angle_unit = evalops.parse_options.angle_unit;
				PrintOptions po;
				po.is_approximate = &b;
				po.number_fraction_format = FRACTION_DECIMAL;
				CALCULATOR->resetExchangeRatesUsed();
				do_timeout = false;
				MathStructure v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(toValue, evalops.parse_options), uTo, uFrom, 1500, eo);
				if(!v_mstruct.isAborted() && check_exchange_rates(get_units_dialog())) v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(toValue, evalops.parse_options), uTo, uFrom, 1500, eo);
				if(v_mstruct.isAborted()) {
					old_fromValue = CALCULATOR->timedOutString();
				} else {
					old_fromValue = CALCULATOR->print(v_mstruct, 300, po);
				}
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")), old_fromValue.c_str());
				b = b || v_mstruct.isApproximate();
				display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
				do_timeout = true;
			}
		} else {
			if(CALCULATOR->timedOutString() == fromValue) return;
			if(uFrom == uTo) {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")), fromValue);
			} else {
				EvaluationOptions eo;
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.parse_options.angle_unit = evalops.parse_options.angle_unit;
				PrintOptions po;
				po.is_approximate = &b;
				po.number_fraction_format = FRACTION_DECIMAL;
				CALCULATOR->resetExchangeRatesUsed();
				do_timeout = false;
				MathStructure v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(fromValue, evalops.parse_options), uFrom, uTo, 1500, eo);
				if(!v_mstruct.isAborted() && check_exchange_rates(get_units_dialog())) v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(fromValue, evalops.parse_options), uFrom, uTo, 1500, eo);
				if(v_mstruct.isAborted()) {
					old_toValue = CALCULATOR->timedOutString();
				} else {
					old_toValue = CALCULATOR->print(v_mstruct, 300, po);
				}
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")), old_toValue.c_str());
				b = b || v_mstruct.isApproximate();
				display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
				do_timeout = true;
			}
		}
		if(b && printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) gtk_builder_get_object(units_builder, "units_label_equals"))) {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_equals")), SIGN_ALMOST_EQUAL);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_equals")), "=");
		}
	}
}

/*
	save definitions to ~/.conf/qalculate/qalculate.cfg
	the hard work is done in the Calculator class
*/
void save_defs() {
	if(!CALCULATOR->saveDefinitions()) {
		GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Couldn't write definitions"));
		gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
	}
}

/*
	save mode to file
*/
void save_mode() {
	save_preferences(true);
}

/*
	remember current mode
*/
void set_saved_mode() {
	modes[1].precision = CALCULATOR->getPrecision();
	modes[1].po = printops;
	modes[1].po.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	modes[1].eo = evalops;
	modes[1].at = CALCULATOR->defaultAssumptions()->type();
	modes[1].as = CALCULATOR->defaultAssumptions()->sign();
	modes[1].rpn_mode = rpn_mode;
}

size_t save_mode_as(string name, bool *new_mode = NULL) {
	remove_blank_ends(name);
	size_t index = 0;
	for(; index < modes.size(); index++) {
		if(modes[index].name == name) {
			if(new_mode) *new_mode = false;
			break;
		}
	}
	if(index >= modes.size()) {
		modes.resize(modes.size() + 1);
		index = modes.size() - 1;
		if(new_mode) *new_mode = true;
	}
	modes[index].po = printops;
	modes[index].po.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	modes[index].eo = evalops;
	modes[index].precision = CALCULATOR->getPrecision();
	modes[index].at = CALCULATOR->defaultAssumptions()->type();
	modes[index].as = CALCULATOR->defaultAssumptions()->sign();
	modes[index].name = name;
	modes[index].rpn_mode = rpn_mode;
	return index;
}

void load_mode(const mode_struct &mode) {
	block_result_update = true;
	block_expression_execution = true;
	block_display_parse = true;
	set_mode_items(mode.po, mode.eo, mode.at, mode.as, mode.rpn_mode, mode.precision, false);
	block_result_update = false;
	block_expression_execution = false;
	block_display_parse = false;
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	set_rpn_mode(mode.rpn_mode);
	GtkTextIter istart, iend;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	string str = gtext;
	g_free(gtext);
	if(expression_has_changed || str.find_first_not_of(SPACES) == string::npos) {
		setResult(NULL, true, false, false);
	} else {
		execute_expression(false);
	}
	expression_has_changed2 = true;
	display_parse_status();
}
void load_mode(string name) {
	for(size_t i = 0; i < modes.size(); i++) {
		if(modes[i].name == name) {
			load_mode(modes[i]);
			return;
		}
	}
}
void load_mode(size_t index) {
	if(index < modes.size()) {
		load_mode(modes[index]);
	}
}

void on_popup_menu_item_disable_completion_activate(GtkMenuItem*, gpointer) {
	enable_completion = false;
	update_completion();
}
void on_popup_menu_item_enable_completion_activate(GtkMenuItem*, gpointer) {
	enable_completion = true;
	update_completion();
}
void on_popup_menu_item_read_precision_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_limit_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_adaptive_parsing_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_ignore_whitespace_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_rpn_syntax_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_rpn_mode_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void expression_set_from_undo_buffer() {
	if(undo_index < expression_undo_buffer.size()) {
		string str_old = get_expression_text();
		string str_new = expression_undo_buffer[undo_index];
		if(str_old == str_new) return;
		size_t i;
		add_to_undo = false;
		GtkTextIter istart, iend;
		if(str_old.length() > str_new.length()) {
			if((i = str_old.find(str_new)) != string::npos) {
				if(i != 0) {
					gtk_text_buffer_get_iter_at_offset(expressionbuffer, &iend, g_utf8_strlen(str_old.c_str(), i));
					gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
					gtk_text_buffer_delete(expressionbuffer, &istart, &iend);
				}
				if(i + str_new.length() < str_old.length()) {
					gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_new.c_str(), -1));
					gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
					gtk_text_buffer_delete(expressionbuffer, &istart, &iend);
				}
				add_to_undo = true;
				return;
			}
			for(i = 0; i < str_new.length(); i++) {
				if(str_new[i] != str_old[i]) {
					if(i == 0) break;
					string str_test = str_old.substr(0, i);
					str_test += str_old.substr(i + str_old.length() - str_new.length());
					if(str_test == str_new) {
						gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_old.c_str(), i));
						gtk_text_buffer_get_iter_at_offset(expressionbuffer, &iend, g_utf8_strlen(str_old.c_str(), i + str_old.length() - str_new.length()));
						gtk_text_buffer_delete(expressionbuffer, &istart, &iend);
						add_to_undo = true;
						return;
					}
					if(str_new.length() + 1 == str_old.length()) break;
					str_test = str_old.substr(0, i);
					str_test += str_old.substr(i + str_old.length() - str_new.length() - 1);
					size_t i2 = i;
					while((i2 = str_test.find(')', i2 + 1)) != string::npos) {
						string str_test2 = str_test;
						str_test2.erase(str_test2.begin() + i2);
						if(str_test2 == str_new) {
							gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_old.c_str(), i));
							gtk_text_buffer_get_iter_at_offset(expressionbuffer, &iend, g_utf8_strlen(str_old.c_str(), i + str_old.length() - str_new.length() - 1));
							gtk_text_buffer_delete(expressionbuffer, &istart, &iend);
							gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_old.c_str(), i2));
							iend = istart;
							gtk_text_iter_forward_char(&iend);
							gtk_text_buffer_delete(expressionbuffer, &istart, &iend);
							add_to_undo = true;
							return;
						}
					}
					break;
				}
			}
		} else if(str_new.length() > str_old.length()) {
			if((i = str_new.find(str_old)) != string::npos) {
				if(i + str_old.length() < str_new.length()) {
					gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
					gtk_text_buffer_insert(expressionbuffer, &iend, str_new.substr(i + str_old.length(), str_new.length() - (i + str_old.length())).c_str(), -1);
				}
				if(i > 0) {
					gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
					gtk_text_buffer_insert(expressionbuffer, &istart, str_new.substr(0, i).c_str(), -1);
				}
				add_to_undo = true;
				return;
			}
			for(i = 0; i < str_old.length(); i++) {
				if(str_old[i] != str_new[i]) {
					if(i == 0) break;
					string str_test = str_new.substr(0, i);
					str_test += str_new.substr(i + str_new.length() - str_old.length());
					if(str_test == str_old) {
						gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_new.c_str(), i));
						gtk_text_buffer_insert(expressionbuffer, &istart, str_new.substr(i, str_new.length() - str_old.length()).c_str(), -1);
						add_to_undo = true;
						return;
					}
					if(str_old.length() + 1 == str_new.length()) break;
					str_test = str_new.substr(0, i);
					str_test += str_new.substr(i + str_new.length() - str_old.length() - 1);
					size_t i2 = i;
					while((i2 = str_test.find(')', i2 + 1)) != string::npos) {
						string str_test2 = str_test;
						str_test2.erase(str_test2.begin() + i2);
						if(str_test2 == str_old) {
							gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_new.c_str(), i));
							gtk_text_buffer_insert(expressionbuffer, &istart, str_new.substr(i, str_new.length() - str_old.length() - 1).c_str(), -1);
							gtk_text_buffer_get_iter_at_offset(expressionbuffer, &istart, g_utf8_strlen(str_new.c_str(), i2 + str_new.length() - str_old.length() - 1));
							gtk_text_buffer_insert(expressionbuffer, &istart, ")", -1);
							add_to_undo = true;
							return;
						}
					}
					break;
				}
			}
		}
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(expressiontext), FALSE);
		gtk_text_buffer_set_text(expressionbuffer, str_new.c_str(), -1);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(expressiontext), TRUE);
		add_to_undo = true;
	}
}
void expression_undo() {
	if(undo_index == 0) return;
	undo_index--;
	expression_set_from_undo_buffer();
}
void expression_redo() {
	if(undo_index >= expression_undo_buffer.size() - 1) return;
	undo_index++;
	expression_set_from_undo_buffer();
}
void on_expressiontext_populate_popup(GtkTextView*, GtkMenu *menu, gpointer) {
	GtkWidget *item, *sub, *sub2;
	GSList *group = NULL;
	gchar *gstr;
	sub = GTK_WIDGET(menu);
	MENU_SEPARATOR
	if(b_busy) {
		MENU_ITEM(_("Abort"), on_popup_menu_item_abort_activate)
		return;
	}
	MENU_ITEM(_("Undo"), expression_undo)
	if(undo_index == 0) gtk_widget_set_sensitive(item, FALSE);
	MENU_ITEM(_("Redo"), expression_redo)
	if(undo_index >= expression_undo_buffer.size() - 1) gtk_widget_set_sensitive(item, FALSE);
	MENU_SEPARATOR
	MENU_ITEM(_("Clear"), on_popup_menu_item_clear_activate)
	if(expression_is_empty()) gtk_widget_set_sensitive(item, FALSE);
	if(enable_completion) {
		MENU_ITEM(_("Disable Completion"), on_popup_menu_item_disable_completion_activate)
	} else {
		MENU_ITEM(_("Enable Completion"), on_popup_menu_item_enable_completion_activate)
	}
	MENU_SEPARATOR
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_adaptive_parsing_activate, gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing"))
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_ignore_whitespace_activate, gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace"))
	POPUP_RADIO_MENU_ITEM(on_popup_menu_item_no_special_implicit_multiplication_activate, gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication"))
	MENU_SEPARATOR
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_limit_implicit_multiplication_activate, gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication"))
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_read_precision_activate, gtk_builder_get_object(main_builder, "menu_item_read_precision"))
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_rpn_syntax_activate, gtk_builder_get_object(main_builder, "menu_item_rpn_syntax"))
	POPUP_CHECK_MENU_ITEM(on_popup_menu_item_rpn_mode_activate, gtk_builder_get_object(main_builder, "menu_item_rpn_mode"))
	MENU_SEPARATOR
	sub2 = sub;
	SUBMENU_ITEM(_("Meta Modes"), sub2)
	for(size_t i = 0; i < modes.size(); i++) {
		item = gtk_menu_item_new_with_label(modes[i].name.c_str()); 
		gtk_widget_show(item); 
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) modes[i].name.c_str());
		gtk_menu_shell_insert(GTK_MENU_SHELL(sub), item, (gint) i);
	}
	MENU_SEPARATOR
	MENU_ITEM(_("Save Mode…"), on_menu_item_meta_mode_save_activate)
	MENU_ITEM(_("Delete Mode…"), on_menu_item_meta_mode_delete_activate)
	gtk_widget_set_sensitive(item, modes.size() > 2);
	sub = sub2;
	MENU_SEPARATOR
	MENU_ITEM(_("Insert Matrix…"), on_menu_item_insert_matrix_activate)
	MENU_ITEM(_("Insert Vector…"), on_menu_item_insert_vector_activate)
}

void on_combobox_base_changed(GtkComboBox *w, gpointer) {
	switch(gtk_combo_box_get_active(w)) {
		case 0: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_binary")), TRUE);
			break;
		}
		case 1: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_octal")), TRUE);
			break;
		}
		case 2: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_decimal")), TRUE);
			break;
		}
		case 3: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
			break;
		}
		case 4: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sexagesimal")), TRUE);
			break;
		}
		case 5: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_time_format")), TRUE);
			break;
		}
		case 6: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_roman")), TRUE);
			break;
		}
		case 7: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")), TRUE);
			break;
		}
	}
}
void on_combobox_numerical_display_changed(GtkComboBox *w, gpointer) {
	switch(gtk_combo_box_get_active(w)) {
		case 0: {
			printops.negative_exponents = false;
			printops.sort_options.minus_last = true;
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_normal")), TRUE);
			break;
		}
		case 1: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_engineering")), TRUE);
			break;
		}
		case 2: {
			printops.negative_exponents = true;
			printops.sort_options.minus_last = false;
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_scientific")), TRUE);
			break;
		}
		case 3: {
			printops.negative_exponents = true;
			printops.sort_options.minus_last = false;
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific")), TRUE);
			break;
		}
		case 4: {
			printops.negative_exponents = false;
			printops.sort_options.minus_last = true;
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_non_scientific")), TRUE);
			break;
		}
	}
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), printops.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), printops.sort_options.minus_last);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
}

void on_combobox_fraction_mode_changed(GtkComboBox *w, gpointer) {
	switch(gtk_combo_box_get_active(w)) {
		case 0: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
			break;
		}
		case 2: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
			break;
		}
		case 1: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
			break;
		}
		case 3: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
			break;
		}
	}
}
void on_combobox_approximation_changed(GtkComboBox *w, gpointer) {
	switch(gtk_combo_box_get_active(w)) {
		case 0: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), TRUE);
			break;
		}
		case 1: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), TRUE);
			break;
		}
		case 2: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_approximate")), TRUE);
			break;
		}
	}
}

void show_tabs(bool do_show) {
	if(do_show == gtk_widget_get_visible(tabs)) return;
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
	if(gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")))) h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))) + 6;
	if(do_show) {		
		gtk_widget_show(tabs);
		gint a_h = gtk_widget_get_allocated_height(tabs);
		if(a_h > 10) h += a_h + 6;
		else h += history_height + 6;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), w, h);
	} else {
		h -= gtk_widget_get_allocated_height(tabs) + 6;
		gtk_widget_hide(tabs);
		set_result_size_request();
		set_expression_size_request();
		gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), w, h);
	}
}
void show_keypad_widget(bool do_show) {
	if(do_show == gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")))) return;
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
	if(gtk_widget_get_visible(tabs)) h -= gtk_widget_get_allocated_height(tabs) + 6;
	if(do_show) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		gint a_h = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		if(a_h > 10) h += a_h + 6;
		else h += keypad_height + 6;
		gtk_widget_hide(tabs);
		gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), w, h);
	} else {
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))) + 6;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		set_result_size_request();
		set_expression_size_request();
		gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), w, h);
	}
}

void on_expander_keypad_expanded(GObject *o, GParamSpec*, gpointer) {	
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		show_keypad_widget(true);
		if(gtk_expander_get_expanded(GTK_EXPANDER(expander_history))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_history), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), FALSE);
		}		
	} else {
		show_keypad_widget(false);		
	}
}
void on_expander_history_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		bool history_was_realized = gtk_widget_get_realized(historyview);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 0);
		show_tabs(true);
		while(!history_was_realized && gtk_events_pending()) gtk_main_iteration();
		if(!history_was_realized) {
			GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(historyview), path, history_index_column, FALSE, 0, 0);
			gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(historyview), 0, 0);
			gtk_tree_path_free(path);
		}
		if(gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), FALSE);
		}
	} else if(!gtk_expander_get_expanded(GTK_EXPANDER(expander_stack)) && !gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
		show_tabs(false);
	}
}
void on_expander_stack_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 1);
		show_tabs(true);
		if(gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_history))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_history), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), FALSE);
		}		
	} else if(!gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) && !gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
		show_tabs(false);
	}
}
void on_expander_convert_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 2);
		show_tabs(true);
		if(gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_history))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_history), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), FALSE);
		}		
	} else if(!gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) && !gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) {
		show_tabs(false);
	}
}

void on_menu_item_meta_mode_activate(GtkMenuItem*, gpointer user_data) {
	const char *name = (const char*) user_data;
	load_mode(name);
}
void on_menu_item_meta_mode_save_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Save Mode"), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Save"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
	gtk_widget_show(hbox);
	GtkWidget *label = gtk_label_new(_("Name"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	GtkWidget *entry = gtk_combo_box_text_new_with_entry();
	for(size_t i = 2; i < modes.size(); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), modes[i].name.c_str());
	}
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_widget_show(entry);
run_meta_mode_save_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT) {
		bool new_mode = true;
		string name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry));
		remove_blank_ends(name);
		if(name.empty()) {
			show_message(_("Empty name field."), dialog);
			goto run_meta_mode_save_dialog;
		}
		if(name == modes[0].name) {
			show_message(_("Preset mode cannot be overwritten."), dialog);
			goto run_meta_mode_save_dialog;
		}
		size_t index = save_mode_as(name, &new_mode);
		if(new_mode) {
			GtkWidget *item = gtk_menu_item_new_with_label(modes[index].name.c_str()); 
			gtk_widget_show(item); 
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) modes[index].name.c_str());
			gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_meta_modes")), item, (gint) index);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), TRUE);
			mode_items.push_back(item);
			item = gtk_menu_item_new_with_label(modes[index].name.c_str()); 
			gtk_widget_show(item); 
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) modes[index].name.c_str());
			gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_result_popup_meta_modes")), item, (gint) index);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_result_popup_meta_mode_delete")), TRUE);
			popup_result_mode_items.push_back(item);
		}
	}
	gtk_widget_destroy(dialog);
}
void on_menu_item_meta_mode_delete_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Delete Mode"), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Delete"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
	gtk_widget_show(hbox);
	GtkWidget *label = gtk_label_new(_("Mode"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	GtkWidget *menu = gtk_combo_box_text_new();
	for(size_t i = 2; i < modes.size(); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(menu), modes[i].name.c_str());
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu), 0);
	gtk_box_pack_end(GTK_BOX(hbox), menu, FALSE, TRUE, 0);
	gtk_widget_show(menu);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT && gtk_combo_box_get_active(GTK_COMBO_BOX(menu)) >= 0) {
		size_t index = gtk_combo_box_get_active(GTK_COMBO_BOX(menu)) + 2;
		gtk_widget_destroy(mode_items[index]);
		gtk_widget_destroy(popup_result_mode_items[index]);
		modes.erase(modes.begin() + index);
		mode_items.erase(mode_items.begin() + index);
		popup_result_mode_items.erase(popup_result_mode_items.begin() + index);
		if(modes.size() < 3) {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_result_popup_meta_mode_delete")), FALSE);
		}
	}
	gtk_widget_destroy(dialog);
}

/*
	load preferences from ~/.conf/qalculate/qalculate-gtk.cfg
*/
void load_preferences() {

	default_plot_legend_placement = PLOT_LEGEND_TOP_RIGHT;
	default_plot_display_grid = true;
	default_plot_full_border = false;
	default_plot_min = "0";
	default_plot_max = "10";
	default_plot_step = "1";
	default_plot_sampling_rate = 100;
	default_plot_rows = false;
	default_plot_type = 0;
	default_plot_style = PLOT_STYLE_LINES;
	default_plot_smoothing = PLOT_SMOOTHING_NONE;
	default_plot_variable = "x";
	default_plot_color = true;
	default_plot_use_sampling_rate = true;

	printops.multiplication_sign = MULTIPLICATION_SIGN_X;
	printops.division_sign = DIVISION_SIGN_DIVISION_SLASH;
	printops.is_approximate = new bool(false);
	printops.prefix = NULL;
	printops.use_min_decimals = false;
	printops.use_denominator_prefix = true;
	printops.min_decimals = 0;
	printops.use_max_decimals = false;
	printops.max_decimals = 2;
	printops.base = 10;
	printops.min_exp = EXP_PRECISION;
	printops.negative_exponents = false;
	printops.sort_options.minus_last = true;
	printops.indicate_infinite_series = false;
	printops.show_ending_zeroes = false;
	printops.round_halfway_to_even = false;
	printops.number_fraction_format = FRACTION_DECIMAL;
	printops.abbreviate_names = true;
	printops.use_unicode_signs = true;
	printops.use_unit_prefixes = true;
	printops.use_prefixes_for_currencies = false;
	printops.use_prefixes_for_all_units = false;
	printops.spacious = true;
	printops.short_multiplication = true;
	printops.place_units_separately = true;
	printops.use_all_prefixes = false;
	printops.excessive_parenthesis = false;
	printops.allow_non_usable = false;
	printops.lower_case_numbers = false;
	printops.lower_case_e = false;
	printops.base_display = BASE_DISPLAY_NORMAL;
	printops.limit_implicit_multiplication = false;
	printops.can_display_unicode_string_function = &can_display_unicode_string_function;
	printops.allow_factorization = false;
	printops.spell_out_logical_operators = true;
	printops.exp_to_root = true;
	
	evalops.approximation = APPROXIMATION_TRY_EXACT;
	evalops.sync_units = true;
	evalops.structuring = STRUCTURING_HYBRID;
	evalops.parse_options.unknowns_enabled = false;
	evalops.parse_options.read_precision = DONT_READ_PRECISION;
	evalops.parse_options.base = BASE_DECIMAL;
	evalops.allow_complex = true;
	evalops.allow_infinite = true;
	evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL;
	evalops.assume_denominators_nonzero = true;
	evalops.warn_about_denominators_assumed_nonzero = true;
	evalops.parse_options.limit_implicit_multiplication = false;
	evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	evalops.parse_options.comma_as_separator = false;
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	
	rpn_mode = false;
	rpn_keys = true;
	
	save_mode_as(_("Preset"));
	save_mode_as(_("Default"));
	size_t mode_index = 1;

	win_width = -1;
	win_height = -1;
	variables_width = -1;
	variables_height = -1;
	variables_position = -1;
	units_width = -1;
	units_height = -1;
	units_position = -1;
	functions_width = -1;
	functions_height = -1;
	functions_hposition = -1;
	functions_vposition = -1;
	datasets_width = -1;
	datasets_height = -1;
	datasets_hposition = -1;
	datasets_vposition1 = -1;
	datasets_vposition2 = -1;
	keypad_height = 0;
	history_height = 0;
	save_mode_on_exit = true;
	save_defs_on_exit = true;
	hyp_is_on = false;
	inv_is_on = false;
	use_custom_result_font = false;
	use_custom_expression_font = false;
	use_custom_status_font = false;
	custom_result_font = "";
	custom_expression_font = "";
	custom_status_font = "";
	status_error_color = "#FF0000";
	status_warning_color = "#0000FF";
	status_error_color_set = false;
	status_warning_color_set = false;
	show_keypad = true;
	show_history = false;
	show_stack = true;
	show_convert = false;
	continuous_conversion = true;
	set_missing_prefixes = false;
	load_global_defs = true;
	fetch_exchange_rates_at_startup = false;
	auto_update_exchange_rates = -1;
	display_expression_status = true;
	enable_completion = true;
	first_time = false;
	first_error = true;
	expression_history.clear();
	expression_history_index = -1;
	
#ifdef _WIN32
	last_version_check_date.setToCurrentDate();
#endif
	
	latest_button_unit = NULL;
	latest_button_currency = NULL;

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
			first_time = true;
			return;
#ifndef _WIN32
		}
#endif
	}

	int version_numbers[] = {2, 0, 0};
	bool old_history_format = false;
			
	if(file) {
		char line[10000];
		string stmp, svalue, svar;
		size_t i;
		int v;
		while(true) {
			if(fgets(line, 10000, file) == NULL)
				break;
			stmp = line;
			remove_blank_ends(stmp);
			if((i = stmp.find_first_of("=")) != string::npos) {
				svar = stmp.substr(0, i);
				remove_blank_ends(svar);
				svalue = stmp.substr(i + 1, stmp.length() - (i + 1));
				remove_blank_ends(svalue);
				v = s2i(svalue);
				if(svar == "version") {
					parse_qalculate_version(svalue, version_numbers);
					old_history_format = (version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 4)));
				} else if(svar == "width") {
					win_width = v;
				/*} else if(svar == "height") {
					win_height = v;*/
				} else if(svar == "variables_width") {
					variables_width = v;
				} else if(svar == "variables_height") {
					variables_height = v;
				} else if(svar == "variables_panel_position") {
					variables_position = v;
				} else if(svar == "units_width") {
					units_width = v;
				} else if(svar == "units_height") {
					units_height = v;
				} else if(svar == "units_panel_position") {
					units_position = v;
				} else if(svar == "functions_width") {
					functions_width = v;
				} else if(svar == "functions_height") {
					functions_height = v;
				} else if(svar == "functions_hpanel_position") {
					functions_hposition = v;
				} else if(svar == "functions_vpanel_position") {
					functions_vposition = v;
				} else if(svar == "datasets_width") {
					datasets_width = v;
				} else if(svar == "datasets_height") {
					datasets_height = v;
				} else if(svar == "datasets_hpanel_position") {
					datasets_hposition = v;
				} else if(svar == "datasets_vpanel1_position") {
					datasets_vposition1 = v;
				} else if(svar == "datasets_vpanel2_position") {
					datasets_vposition2 = v;
				} else if(svar == "error_info_shown") {
					first_error = !v;
				} else if(svar == "save_mode_on_exit") {
					save_mode_on_exit = v;
				} else if(svar == "save_definitions_on_exit") {
					save_defs_on_exit = v;
				} else if(svar == "fetch_exchange_rates_at_startup") {
					if(auto_update_exchange_rates < 0 && v) auto_update_exchange_rates = 1;
					//fetch_exchange_rates_at_startup = v;
				} else if(svar == "auto_update_exchange_rates") {
					auto_update_exchange_rates = v;
#ifdef _WIN32
				} else if(svar == "last_version_check") {
					last_version_check_date.set(svalue);
				} else if(svar == "last_found_version") {
					last_found_version = svalue;
#endif
				} else if(svar == "show_keypad") {
					show_keypad = v;
				/*} else if(svar == "keypad_height") {
					keypad_height = v;*/
				} else if(svar == "show_history") {
					show_history = v;
				} else if(svar == "history_height") {
					history_height = v;
				} else if(svar == "show_stack") {
					show_stack = v;
				} else if(svar == "show_convert") {
					show_convert = v;
				} else if(svar == "continuous_conversion") {
					continuous_conversion = v;
				} else if(svar == "set_missing_prefixes") {
					set_missing_prefixes = v;
				} else if(svar == "display_expression_status") {
					display_expression_status = v;
				} else if(svar == "enable_completion") {
					enable_completion = v;
				} else if(svar == "min_deci") {
					if(mode_index == 1) printops.min_decimals = v;
					else modes[mode_index].po.min_decimals = v;
				} else if(svar == "use_min_deci") {
					if(mode_index == 1) printops.use_min_decimals = v;
					else modes[mode_index].po.use_min_decimals = v;
				} else if(svar == "max_deci") {
					if(mode_index == 1) printops.max_decimals = v;
					else modes[mode_index].po.max_decimals = v;
				} else if(svar == "use_max_deci") {
					if(mode_index == 1) printops.use_max_decimals = v;
					else modes[mode_index].po.use_max_decimals = v;
				} else if(svar == "precision") {
					if(mode_index == 1) CALCULATOR->setPrecision(v);
					else modes[mode_index].precision = v;
				} else if(svar == "min_exp") {
					if(mode_index == 1) printops.min_exp = v;
					else modes[mode_index].po.min_exp = v;
				} else if(svar == "negative_exponents") {
					if(mode_index == 1) printops.negative_exponents = v;
					else modes[mode_index].po.negative_exponents = v;
				} else if(svar == "sort_minus_last") {
					if(mode_index == 1) printops.sort_options.minus_last = v;
					else modes[mode_index].po.sort_options.minus_last = v;
				} else if(svar == "place_units_separately") {
					if(mode_index == 1) printops.place_units_separately = v;
					else modes[mode_index].po.place_units_separately = v;
				} else if(svar == "display_mode") {	//obsolete
					switch(v) {
						case 1: {
							if(mode_index == 1) {
								printops.min_exp = EXP_PRECISION;
								printops.negative_exponents = false;
								printops.sort_options.minus_last = true;
							} else {
								modes[mode_index].po.min_exp = EXP_PRECISION;
								modes[mode_index].po.negative_exponents = false;
								modes[mode_index].po.sort_options.minus_last = true;
							}
							break;
						}
						case 2: {
							if(mode_index == 1) {
								printops.min_exp = EXP_SCIENTIFIC;
								printops.negative_exponents = true;
								printops.sort_options.minus_last = false;
							} else {
								modes[mode_index].po.min_exp = EXP_SCIENTIFIC;
								modes[mode_index].po.negative_exponents = true;
								modes[mode_index].po.sort_options.minus_last = false;
							}
							break;
						}
						case 3: {
							if(mode_index == 1) {
								printops.min_exp = EXP_PURE;
								printops.negative_exponents = true;
								printops.sort_options.minus_last = false;
							} else {
								modes[mode_index].po.min_exp = EXP_PURE;
								modes[mode_index].po.negative_exponents = true;
								modes[mode_index].po.sort_options.minus_last = false;
							}
							break;
						}
						case 4: {
							if(mode_index == 1) {
								printops.min_exp = EXP_NONE;
								printops.negative_exponents = false;
								printops.sort_options.minus_last = true;
							} else {
								modes[mode_index].po.min_exp = EXP_NONE;
								modes[mode_index].po.negative_exponents = false;
								modes[mode_index].po.sort_options.minus_last = true;
							}
							break;
						}
					}
				} else if(svar == "use_prefixes") {
					if(mode_index == 1) printops.use_unit_prefixes = v;
					else modes[mode_index].po.use_unit_prefixes = v;
				} else if(svar == "use_prefixes_for_all_units") {
					if(mode_index == 1) printops.use_prefixes_for_all_units = v;
					else modes[mode_index].po.use_prefixes_for_all_units = v;
				} else if(svar == "use_prefixes_for_currencies") {
					if(mode_index == 1) printops.use_prefixes_for_currencies = v;
					else modes[mode_index].po.use_prefixes_for_currencies = v;
				} else if(svar == "fractional_mode") {	//obsolete
					switch(v) {
						case 1: {
							if(mode_index == 1) printops.number_fraction_format = FRACTION_DECIMAL;
							else modes[mode_index].po.number_fraction_format = FRACTION_DECIMAL;
							break;
						}
						case 2: {
							if(mode_index == 1) printops.number_fraction_format = FRACTION_COMBINED;
							else modes[mode_index].po.number_fraction_format = FRACTION_COMBINED;
							break;
						}
						case 3: {
							if(mode_index == 1) printops.number_fraction_format = FRACTION_FRACTIONAL;
							else modes[mode_index].po.number_fraction_format = FRACTION_FRACTIONAL;
							break;
						}
					}
				} else if(svar == "number_fraction_format") {
					if(v >= FRACTION_DECIMAL && v <= FRACTION_COMBINED) {
						if(mode_index == 1) printops.number_fraction_format = (NumberFractionFormat) v;
						else modes[mode_index].po.number_fraction_format = (NumberFractionFormat) v;
					}
				} else if(svar == "number_base") {
					if(mode_index == 1) printops.base = v;
					else modes[mode_index].po.base = v;
				} else if(svar == "number_base_expression") {
					if(mode_index == 1) evalops.parse_options.base = v;	
					else modes[mode_index].eo.parse_options.base = v;	
				} else if(svar == "read_precision") {
					if(v >= DONT_READ_PRECISION && v <= READ_PRECISION_WHEN_DECIMALS) {
						if(mode_index == 1) evalops.parse_options.read_precision = (ReadPrecisionMode) v;
						else modes[mode_index].eo.parse_options.read_precision = (ReadPrecisionMode) v;
					}
				} else if(svar == "assume_denominators_nonzero") {
					if(version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
						v = true;
					}
					if(mode_index == 1) evalops.assume_denominators_nonzero = v;
					else modes[mode_index].eo.assume_denominators_nonzero = v;
				} else if(svar == "warn_about_denominators_assumed_nonzero") {
					if(mode_index == 1) evalops.warn_about_denominators_assumed_nonzero = v;
					else modes[mode_index].eo.warn_about_denominators_assumed_nonzero = v;
				} else if(svar == "structuring") {
					if(v >= STRUCTURING_NONE && v <= STRUCTURING_HYBRID) {
						if((v == STRUCTURING_NONE || v == STRUCTURING_SIMPLIFY) && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 12))) {
							v = STRUCTURING_HYBRID;
						}
						if(mode_index == 1) {
							evalops.structuring = (StructuringMode) v;
							printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
						} else {
							modes[mode_index].eo.structuring = (StructuringMode) v;
							modes[mode_index].po.allow_factorization = (modes[mode_index].eo.structuring == STRUCTURING_FACTORIZE);
						}
					}
				} else if(svar == "angle_unit") {
					if(version_numbers[0] == 0 && (version_numbers[1] < 7 || (version_numbers[1] == 7 && version_numbers[2] == 0))) {
						v++;
					}
					if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_GRADIANS) {
						if(mode_index == 1) evalops.parse_options.angle_unit = (AngleUnit) v;
						else modes[mode_index].eo.parse_options.angle_unit = (AngleUnit) v;
					}
				} else if(svar == "functions_enabled") {
					if(mode_index == 1) evalops.parse_options.functions_enabled = v;
					else modes[mode_index].eo.parse_options.functions_enabled = v;
				} else if(svar == "variables_enabled") {
					if(mode_index == 1) evalops.parse_options.variables_enabled = v;
					else modes[mode_index].eo.parse_options.variables_enabled = v;
				} else if(svar == "donot_calculate_variables") {
					if(mode_index == 1) evalops.calculate_variables = !v;
					else modes[mode_index].eo.calculate_variables = !v;
				} else if(svar == "calculate_variables") {
					if(mode_index == 1) evalops.calculate_variables = v;
					else modes[mode_index].eo.calculate_variables = v;
				} else if(svar == "calculate_functions") {
					if(mode_index == 1) evalops.calculate_functions = v;
					else modes[mode_index].eo.calculate_functions = v;
				} else if(svar == "sync_units") {
					if(mode_index == 1) evalops.sync_units = v;
					else modes[mode_index].eo.sync_units = v;
				} else if(svar == "unknownvariables_enabled") {
					if(mode_index == 1) evalops.parse_options.unknowns_enabled = v;
					else modes[mode_index].eo.parse_options.unknowns_enabled = v;
				} else if(svar == "units_enabled") {
					if(mode_index == 1) evalops.parse_options.units_enabled = v;
					else modes[mode_index].eo.parse_options.units_enabled = v;
				} else if(svar == "allow_complex") {
					if(mode_index == 1) evalops.allow_complex = v;
					else modes[mode_index].eo.allow_complex = v;
				} else if(svar == "allow_infinite") {
					if(mode_index == 1) evalops.allow_infinite = v;
					else modes[mode_index].eo.allow_infinite = v;
				} else if(svar == "use_short_units") {
					if(mode_index == 1) printops.abbreviate_names = v;
					else modes[mode_index].po.abbreviate_names = v;
				} else if(svar == "abbreviate_names") {
					if(mode_index == 1) printops.abbreviate_names = v;
					else modes[mode_index].po.abbreviate_names = v;
				} else if(svar == "all_prefixes_enabled") {
					if(mode_index == 1) printops.use_all_prefixes = v;
					else modes[mode_index].po.use_all_prefixes = v;
				} else if(svar == "denominator_prefix_enabled") {
					if(mode_index == 1) printops.use_denominator_prefix = v;
					else modes[mode_index].po.use_denominator_prefix = v;
				} else if(svar == "auto_post_conversion") {
					if(v >= POST_CONVERSION_NONE && v <= POST_CONVERSION_OPTIMAL) {
						if(v == POST_CONVERSION_NONE && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 12))) {
							v = POST_CONVERSION_OPTIMAL;
						}
						if(mode_index == 1) evalops.auto_post_conversion = (AutoPostConversion) v;
						else modes[mode_index].eo.auto_post_conversion = (AutoPostConversion) v;
					}
				} else if(svar == "mixed_units_conversion") {
					if(v >= MIXED_UNITS_CONVERSION_NONE || v <= MIXED_UNITS_CONVERSION_FORCE_ALL) {
						if(mode_index == 1) evalops.mixed_units_conversion = (MixedUnitsConversion) v;
						else modes[mode_index].eo.mixed_units_conversion = (MixedUnitsConversion) v;
					}
				} else if(svar == "indicate_infinite_series") {
					if(mode_index == 1) printops.indicate_infinite_series = v;
					else modes[mode_index].po.indicate_infinite_series = v;
				} else if(svar == "show_ending_zeroes") {
					if(mode_index == 1) printops.show_ending_zeroes = v;
					else modes[mode_index].po.show_ending_zeroes = v;
				} else if(svar == "round_halfway_to_even") {
					if(mode_index == 1) printops.round_halfway_to_even = v;	
					else modes[mode_index].po.round_halfway_to_even = v;	
				} else if(svar == "always_exact") {		//obsolete
					if(mode_index == 1) evalops.approximation = APPROXIMATION_EXACT;
					else modes[mode_index].eo.approximation = APPROXIMATION_EXACT;
				} else if(svar == "approximation") {
					if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
						if(mode_index == 1) evalops.approximation = (ApproximationMode) v;
						else modes[mode_index].eo.approximation = (ApproximationMode) v;
					}
				} else if(svar == "in_rpn_mode") {
					if(mode_index == 1) rpn_mode = v;
					else modes[mode_index].rpn_mode = v;
				} else if(svar == "rpn_keys") {
					rpn_keys = v;
				} else if(svar == "rpn_syntax") {
					if(mode_index == 1) evalops.parse_options.rpn = v;
					else modes[mode_index].eo.parse_options.rpn = v;
				} else if(svar == "limit_implicit_multiplication") {
					if(mode_index == 1) {
						evalops.parse_options.limit_implicit_multiplication = v;
						printops.limit_implicit_multiplication = v;
					} else {
						modes[mode_index].eo.parse_options.limit_implicit_multiplication = v;
						modes[mode_index].po.limit_implicit_multiplication = v;
					}
				} else if(svar == "parsing_mode") {
					if(v >= PARSING_MODE_ADAPTIVE && v <= PARSING_MODE_CONVENTIONAL) {
						if(mode_index == 1) {
							evalops.parse_options.parsing_mode = (ParsingMode) v;
						} else {
							modes[mode_index].eo.parse_options.parsing_mode = (ParsingMode) v;
						}
					}
				} else if(svar == "default_assumption_type") {
					if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_INTEGER) {
						if(v < ASSUMPTION_TYPE_NUMBER && version_numbers[0] < 1) v = ASSUMPTION_TYPE_NUMBER;
						if(v == ASSUMPTION_TYPE_COMPLEX && version_numbers[0] < 2) v = ASSUMPTION_TYPE_NUMBER;
						if(mode_index == 1) CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
						else modes[mode_index].at = (AssumptionType) v;
					}
				} else if(svar == "default_assumption_sign") {
					if(v >= ASSUMPTION_SIGN_UNKNOWN && v <= ASSUMPTION_SIGN_NONZERO) {
						if(v == ASSUMPTION_SIGN_NONZERO && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
							v = ASSUMPTION_SIGN_UNKNOWN;
						}
						if(mode_index == 1) CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
						else modes[mode_index].as = (AssumptionSign) v;
					}
				} else if(svar == "spacious") {
					if(mode_index == 1) printops.spacious = v;
					else modes[mode_index].po.spacious = v;
				} else if(svar == "excessive_parenthesis") {
					if(mode_index == 1) printops.excessive_parenthesis = v;
					else modes[mode_index].po.excessive_parenthesis = v;
				} else if(svar == "short_multiplication") {
					if(mode_index == 1) printops.short_multiplication = v;
					else modes[mode_index].po.short_multiplication = v;
				/*} else if(svar == "hyp_is_on") {
					hyp_is_on = v;
				} else if(svar == "inv_is_on") {
					inv_is_on = v;*/
				} else if(svar == "use_unicode_signs" && (version_numbers[0] > 0 || version_numbers[1] > 7 || (version_numbers[1] == 7 && version_numbers[2] > 0))) {
					printops.use_unicode_signs = v;	
				} else if(svar == "lower_case_numbers") {
					printops.lower_case_numbers = v;	
				} else if(svar == "lower_case_e") {
					printops.lower_case_e = v;	
				} else if(svar == "base_display") {
					if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_ALTERNATIVE) printops.base_display = (BaseDisplay) v;
				} else if(svar == "spell_out_logical_operators") {
					printops.spell_out_logical_operators = v;	
				} else if(svar == "dot_as_separator") {
					evalops.parse_options.dot_as_separator = v;
				} else if(svar == "comma_as_separator") {
					evalops.parse_options.comma_as_separator = v;
					if(CALCULATOR->getDecimalPoint() != ",") {						
						CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
					}
				} else if(svar == "use_custom_result_font") {
					use_custom_result_font = v;
				} else if(svar == "use_custom_expression_font") {
					use_custom_expression_font = v;
				} else if(svar == "use_custom_status_font") {
					use_custom_status_font = v;
				} else if(svar == "custom_result_font") {
					custom_result_font = svalue;
					save_custom_result_font = true;
				} else if(svar == "custom_expression_font") {
					custom_expression_font = svalue;
					save_custom_expression_font = true;
				} else if(svar == "custom_status_font") {
					custom_status_font = svalue;
					save_custom_status_font = true;
				} else if(svar == "status_error_color") {
					status_error_color = svalue;
					status_error_color_set = true;
				} else if(svar == "status_warning_color") {
					status_warning_color = svalue;
					status_warning_color_set = true;
				} else if(svar == "multiplication_sign") {
					if(svalue == "*") {
						printops.multiplication_sign = MULTIPLICATION_SIGN_ASTERISK;
					} else if(svalue == SIGN_MULTIDOT) {
						printops.multiplication_sign = MULTIPLICATION_SIGN_DOT;
					} else if(svalue == SIGN_MIDDLEDOT) {
						printops.multiplication_sign = MULTIPLICATION_SIGN_ALTDOT;
					} else if(svalue == SIGN_MULTIPLICATION) {
						printops.multiplication_sign = MULTIPLICATION_SIGN_X;
					} else if(v >= MULTIPLICATION_SIGN_ASTERISK && v <= MULTIPLICATION_SIGN_ALTDOT) {
						printops.multiplication_sign = (MultiplicationSign) v;
					}
					if(printops.multiplication_sign == MULTIPLICATION_SIGN_DOT && version_numbers[0] < 2) {
						printops.multiplication_sign = MULTIPLICATION_SIGN_X;
					}
				} else if(svar == "division_sign") {
					if(v >= DIVISION_SIGN_SLASH && v <= DIVISION_SIGN_DIVISION) printops.division_sign = (DivisionSign) v;
				} else if(svar == "recent_functions") {
					size_t v_i = 0;
					while(true) {
						v_i = svalue.find(',');
						if(v_i == string::npos) {
							svar = svalue.substr(0, svalue.length());
							remove_blank_ends(svar);
							if(!svar.empty()) {
								recent_functions_pre.push_back(svar);	
							}
							break;
						} else {
							svar = svalue.substr(0, v_i);
							svalue = svalue.substr(v_i + 1, svalue.length() - (v_i + 1));
							remove_blank_ends(svar);
							if(!svar.empty()) {
								recent_functions_pre.push_back(svar);	
							}
						}
					}
				} else if(svar == "recent_variables") {
					size_t v_i = 0;
					while(true) {
						v_i = svalue.find(',');
						if(v_i == string::npos) {
							svar = svalue.substr(0, svalue.length());
							remove_blank_ends(svar);
							if(!svar.empty()) {
								recent_variables_pre.push_back(svar);	
							}
							break;
						} else {
							svar = svalue.substr(0, v_i);
							svalue = svalue.substr(v_i + 1, svalue.length() - (v_i + 1));
							remove_blank_ends(svar);
							if(!svar.empty()) {
								recent_variables_pre.push_back(svar);	
							}
						}
					}
				} else if(svar == "recent_units") {
					size_t v_i = 0;
					while(true) {
						v_i = svalue.find(',');
						if(v_i == string::npos) {
							svar = svalue.substr(0, svalue.length());
							remove_blank_ends(svar);
							if(!svar.empty()) {
								recent_units_pre.push_back(svar);	
							}
							break;
						} else {
							svar = svalue.substr(0, v_i);
							svalue = svalue.substr(v_i + 1, svalue.length() - (v_i + 1));
							remove_blank_ends(svar);
							if(!svar.empty()) {
								recent_units_pre.push_back(svar);	
							}
						}
					}
				} else if(svar == "latest_button_unit") {
					latest_button_unit_pre = svalue;
				} else if(svar == "latest_button_currency") {
					latest_button_currency_pre = svalue;
				} else if(svar == "plot_legend_placement") {
					if(v >= PLOT_LEGEND_NONE && v <= PLOT_LEGEND_OUTSIDE) default_plot_legend_placement = (PlotLegendPlacement) v;
				} else if(svar == "plot_style") {
					if(v >= PLOT_STYLE_LINES && v <= PLOT_STYLE_DOTS) default_plot_style = (PlotStyle) v;
				} else if(svar == "plot_smoothing") {
					if(v >= PLOT_SMOOTHING_NONE && v <= PLOT_SMOOTHING_SBEZIER) default_plot_smoothing = (PlotSmoothing) v;		
				} else if(svar == "plot_display_grid") {
					default_plot_display_grid = v;
				} else if(svar == "plot_full_border") {
					default_plot_full_border = v;
				} else if(svar == "plot_min") {
					default_plot_min = svalue;	
				} else if(svar == "plot_max") {
					default_plot_max = svalue;
				} else if(svar == "plot_step") {
					default_plot_step = svalue;
				} else if(svar == "plot_sampling_rate") {
					default_plot_sampling_rate = v;	
				} else if(svar == "plot_use_sampling_rate") {
					default_plot_use_sampling_rate = v;		
				} else if(svar == "plot_variable") {
					default_plot_variable = svalue;
				} else if(svar == "plot_rows") {
					default_plot_rows = v;	
				} else if(svar == "plot_type") {
					default_plot_type = v;	
				} else if(svar == "plot_color") {
					default_plot_color = v;
				} else if(svar == "expression_history") {
					expression_history.push_back(svalue);		
				} else if(svar == "history") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_OLD);
				} else if(svar == "history_old") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_OLD);
				} else if(svar == "history_expression") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_EXPRESSION);
				} else if(svar == "history_transformation") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_TRANSFORMATION);
				} else if(svar == "history_result") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_RESULT);
				} else if(svar == "history_result_approximate") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_RESULT_APPROXIMATE);
				} else if(svar == "history_parse") {					
					inhistory.push_front(svalue);
					if(old_history_format) inhistory_type.push_front(QALCULATE_HISTORY_PARSE_WITHEQUALS);
					else inhistory_type.push_front(QALCULATE_HISTORY_PARSE);
				} else if(svar == "history_parse_withequals") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_PARSE_WITHEQUALS);
				} else if(svar == "history_parse_approximate") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_PARSE_APPROXIMATE);
				} else if(svar == "history_register_moved") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_REGISTER_MOVED);
				} else if(svar == "history_rpn_operation") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_RPN_OPERATION);
				} else if(svar == "history_warning") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_WARNING);
				} else if(svar == "history_error") {
					inhistory.push_front(svalue);
					inhistory_type.push_front(QALCULATE_HISTORY_ERROR);
				} else if(svar == "history_continued") {
					if(inhistory.size() > 0) {
						inhistory[0] += "\n";
						inhistory[0] += svalue;
					}
				}
			} else if(stmp.length() > 2 && stmp[0] == '[' && stmp[stmp.length() - 1] == ']') {
				stmp = stmp.substr(1, stmp.length() - 2);
				remove_blank_ends(stmp);
				if(stmp == "Mode") {
					mode_index = 1;
				} else if(stmp.length() > 5 && stmp.substr(0, 4) == "Mode") {
					mode_index = save_mode_as(stmp.substr(5, stmp.length() - 5));
				}
			}
		}
		fclose(file);
		if(gstr_oldfile) {
			g_mkdir(getLocalDir().c_str(), S_IRWXU);
			move_file(gstr_oldfile, gstr_file);
			g_free(gstr_oldfile);
		}
	} else {
		first_time = true;
	}
	initial_inhistory_index = inhistory.size() - 1;
	g_free(gstr_file);
	show_history = show_history && !show_keypad;
	show_convert = show_convert && !show_history && !show_keypad;
	set_saved_mode();

}

/*
	save preferences to ~/.config/qalculate/qalculate-gtk.cfg
	set mode to true to save current calculator mode
*/

void save_preferences(bool mode) {
	FILE *file = NULL;
	string homedir = getLocalDir();
	g_mkdir(homedir.c_str(), S_IRWXU);
	gchar *gstr2 = g_build_filename(homedir.c_str(), "qalculate-gtk.cfg", NULL);
	file = fopen(gstr2, "w+");
	if(file == NULL) {
		GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Couldn't write preferences to\n%s"), gstr2);
		gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
		g_free(gstr2);
		return;
	}
	g_free(gstr2);
	gint w, h;
	if(variables_builder) {
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")), &w, &h);
		variables_width = w;
		variables_height = h;
		variables_position = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(variables_builder, "variables_hpaned")));
	}
	if(units_builder) {
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")), &w, &h);
		units_width = w;
		units_height = h;
		units_position = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(units_builder, "units_hpaned")));
	}
	if(functions_builder) {
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")), &w, &h);
		functions_width = w;
		functions_height = h;
		functions_hposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_hpaned")));
		functions_vposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_vpaned")));
	}
	if(datasets_builder) {
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(datasets_builder, "datasets_dialog")), &w, &h);
		datasets_width = w;
		datasets_height = h;
		datasets_hposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_hpaned")));
		datasets_vposition1 = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned1")));
		datasets_vposition2 = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned2")));
	}
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", VERSION);
	fprintf(file, "allow_multiple_instances=%i\n", allow_multiple_instances);
	gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
	fprintf(file, "width=%i\n", w);
	//fprintf(file, "height=%i\n", h);
	if(variables_width > -1) fprintf(file, "variables_width=%i\n", variables_width);
	if(variables_height > -1) fprintf(file, "variables_height=%i\n", variables_height);
	if(variables_position > -1) fprintf(file, "variables_panel_position=%i\n", variables_position);
	if(units_width > -1) fprintf(file, "units_width=%i\n", units_width);
	if(units_height > -1) fprintf(file, "units_height=%i\n", units_height);
	if(units_position > -1) fprintf(file, "units_panel_position=%i\n", units_position);
	if(functions_width > -1) fprintf(file, "functions_width=%i\n", functions_width);
	if(functions_height > -1) fprintf(file, "functions_height=%i\n", functions_height);
	if(functions_hposition > -1) fprintf(file, "functions_hpanel_position=%i\n", functions_hposition);
	if(functions_vposition > -1) fprintf(file, "functions_vpanel_position=%i\n", functions_vposition);
	if(datasets_width > -1) fprintf(file, "datasets_width=%i\n", datasets_width);
	if(datasets_height > -1) fprintf(file, "datasets_height=%i\n", datasets_height);
	if(datasets_hposition > -1) fprintf(file, "datasets_hpanel_position=%i\n", datasets_hposition);
	if(datasets_vposition1 > -1) fprintf(file, "datasets_vpanel1_position=%i\n", datasets_vposition1);
	if(datasets_vposition2 > -1) fprintf(file, "datasets_vpanel2_position=%i\n", datasets_vposition2);
	fprintf(file, "error_info_shown=%i\n", !first_error);
	fprintf(file, "save_mode_on_exit=%i\n", save_mode_on_exit);
	fprintf(file, "save_definitions_on_exit=%i\n", save_defs_on_exit);
	fprintf(file, "load_global_definitions=%i\n", load_global_defs);
	//fprintf(file, "fetch_exchange_rates_at_startup=%i\n", fetch_exchange_rates_at_startup);
	fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
#ifdef _WIN32
	fprintf(file, "last_version_check=%s\n", last_version_check_date.toISOString().c_str());
	if(!last_found_version.empty()) fprintf(file, "last_found_version=%s\n", last_found_version.c_str());
#endif
	fprintf(file, "show_keypad=%i\n", (rpn_mode && show_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) || gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad)));
	//h = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
	//fprintf(file, "keypad_height=%i\n", h > 10 ? h : keypad_height);
	fprintf(file, "show_history=%i\n", (rpn_mode && show_history && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) || gtk_expander_get_expanded(GTK_EXPANDER(expander_history)));
	h = gtk_widget_get_allocated_height(tabs);
	fprintf(file, "history_height=%i\n", h > 10 ? h : history_height);
	fprintf(file, "show_stack=%i\n", rpn_mode ? gtk_expander_get_expanded(GTK_EXPANDER(expander_stack)) : show_stack);
	fprintf(file, "show_convert=%i\n", (rpn_mode && show_convert && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) || gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)));
	fprintf(file, "continuous_conversion=%i\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion"))));
	fprintf(file, "set_missing_prefixes=%i\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))));
	fprintf(file, "rpn_keys=%i\n", rpn_keys);
	fprintf(file, "display_expression_status=%i\n", display_expression_status);
	fprintf(file, "enable_completion=%i\n", enable_completion);
	fprintf(file, "use_unicode_signs=%i\n", printops.use_unicode_signs);
	fprintf(file, "lower_case_numbers=%i\n", printops.lower_case_numbers);
	fprintf(file, "lower_case_e=%i\n", printops.lower_case_e);
	fprintf(file, "base_display=%i\n", printops.base_display);
	fprintf(file, "spell_out_logical_operators=%i\n", printops.spell_out_logical_operators);
	fprintf(file, "dot_as_separator=%i\n", evalops.parse_options.dot_as_separator);
	fprintf(file, "comma_as_separator=%i\n", evalops.parse_options.comma_as_separator);
	fprintf(file, "use_custom_result_font=%i\n", use_custom_result_font);	
	fprintf(file, "use_custom_expression_font=%i\n", use_custom_expression_font);
	fprintf(file, "use_custom_status_font=%i\n", use_custom_status_font);
	if(use_custom_result_font || save_custom_result_font) fprintf(file, "custom_result_font=%s\n", custom_result_font.c_str());	
	if(use_custom_expression_font || save_custom_expression_font) fprintf(file, "custom_expression_font=%s\n", custom_expression_font.c_str());
	if(use_custom_status_font || save_custom_status_font) fprintf(file, "custom_status_font=%s\n", custom_status_font.c_str());
	if(status_error_color_set) fprintf(file, "status_error_color=%s\n", status_error_color.c_str());
	if(status_warning_color_set) fprintf(file, "status_warning_color=%s\n", status_warning_color.c_str());
	fprintf(file, "multiplication_sign=%i\n", printops.multiplication_sign);
	fprintf(file, "division_sign=%i\n", printops.division_sign);
	for(size_t i = 0; i < expression_history.size(); i++) {
		fprintf(file, "expression_history=%s\n", expression_history[i].c_str()); 
	}	
	int lines = 300;
	bool end_after_result = false;
	bool doend = false;
	size_t hi = inhistory.size();
	while(!doend && hi > 0) {
		hi--;
		switch(inhistory_type[hi]) {
			case QALCULATE_HISTORY_EXPRESSION: {
				fprintf(file, "history_expression=");
				break;
			}
			case QALCULATE_HISTORY_TRANSFORMATION: {
				fprintf(file, "history_transformation=");
				break;
			}
			case QALCULATE_HISTORY_RESULT: {
				fprintf(file, "history_result=");
				lines--;
				if(end_after_result) doend = true;
				break;
			}
			case QALCULATE_HISTORY_RESULT_APPROXIMATE: {
				fprintf(file, "history_result_approximate=");
				lines--;
				if(end_after_result) doend = true;
				break;
			}
			case QALCULATE_HISTORY_PARSE: {
				fprintf(file, "history_parse=");
				lines--;
				if(lines < 0) end_after_result = true;
				break;
			}
			case QALCULATE_HISTORY_PARSE_WITHEQUALS: {
				fprintf(file, "history_parse_withequals=");
				lines--;
				if(lines < 0) end_after_result = true;
				break;
			}
			case QALCULATE_HISTORY_PARSE_APPROXIMATE: {
				fprintf(file, "history_parse_approximate=");
				lines--;
				if(lines < 0) end_after_result = true;
				break;
			}
			case QALCULATE_HISTORY_REGISTER_MOVED: {
				fprintf(file, "history_register_moved=");
				break;
			}
			case QALCULATE_HISTORY_RPN_OPERATION: {
				fprintf(file, "history_rpn_operation=");
				break;
			}
			case QALCULATE_HISTORY_WARNING: {
				fprintf(file, "history_warning=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_ERROR: {
				fprintf(file, "history_error=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_OLD: {
				fprintf(file, "history_old=");
				lines--;
				if(lines < 0) doend = true;
				break;
			}
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
				lines--;
				i3 = i2 + 1;
				i2 = inhistory[hi].find('\n', i3);
			}
			fprintf(file, "history_continued=%s\n", inhistory[hi].substr(i3, inhistory[hi].length() - i3).c_str());
			lines--;
		}
	}
	fprintf(file, "recent_functions="); 
	for(int i = (int) (recent_functions.size()) - 1; i >= 0; i--) {
		fprintf(file, "%s", recent_functions[i]->referenceName().c_str()); 
		if(i != 0) fprintf(file, ","); 
	}
	fprintf(file, "\n");
	fprintf(file, "recent_variables="); 
	for(int i = (int) (recent_variables.size()) - 1; i >= 0; i--) {
		fprintf(file, "%s", recent_variables[i]->referenceName().c_str()); 
		if(i != 0) fprintf(file, ","); 
	}
	fprintf(file, "\n"); 	
	fprintf(file, "recent_units="); 
	for(int i = (int) (recent_units.size()) - 1; i >= 0; i--) {
		fprintf(file, "%s", recent_units[i]->referenceName().c_str()); 
		if(i != 0) fprintf(file, ","); 
	}
	fprintf(file, "\n"); 
	if(latest_button_unit) fprintf(file, "latest_button_unit=%s\n", latest_button_unit->referenceName().c_str());
	if(latest_button_currency) fprintf(file, "latest_button_currency=%s\n", latest_button_currency->referenceName().c_str());
	if(mode)
		set_saved_mode();
	for(size_t i = 1; i < modes.size(); i++) {
		if(i == 1) {
			fprintf(file, "\n[Mode]\n");
		} else {
			fprintf(file, "\n[Mode %s]\n", modes[i].name.c_str());
		}
		fprintf(file, "min_deci=%i\n", modes[i].po.min_decimals);
		fprintf(file, "use_min_deci=%i\n", modes[i].po.use_min_decimals);
		fprintf(file, "max_deci=%i\n", modes[i].po.max_decimals);
		fprintf(file, "use_max_deci=%i\n", modes[i].po.use_max_decimals);	
		fprintf(file, "precision=%i\n", modes[i].precision);
		fprintf(file, "min_exp=%i\n", modes[i].po.min_exp);
		fprintf(file, "negative_exponents=%i\n", modes[i].po.negative_exponents);
		fprintf(file, "sort_minus_last=%i\n", modes[i].po.sort_options.minus_last);
		fprintf(file, "number_fraction_format=%i\n", modes[i].po.number_fraction_format);	
		fprintf(file, "use_prefixes=%i\n", modes[i].po.use_unit_prefixes);
		fprintf(file, "use_prefixes_for_all_units=%i\n", modes[i].po.use_prefixes_for_all_units);
		fprintf(file, "use_prefixes_for_currencies=%i\n", modes[i].po.use_prefixes_for_currencies);
		fprintf(file, "abbreviate_names=%i\n", modes[i].po.abbreviate_names);
		fprintf(file, "all_prefixes_enabled=%i\n", modes[i].po.use_all_prefixes);
		fprintf(file, "denominator_prefix_enabled=%i\n", modes[i].po.use_denominator_prefix);
		fprintf(file, "place_units_separately=%i\n", modes[i].po.place_units_separately);
		fprintf(file, "auto_post_conversion=%i\n", modes[i].eo.auto_post_conversion);
		fprintf(file, "mixed_units_conversion=%i\n", modes[i].eo.mixed_units_conversion);
		fprintf(file, "number_base=%i\n", modes[i].po.base);
		fprintf(file, "number_base_expression=%i\n", modes[i].eo.parse_options.base);
		fprintf(file, "read_precision=%i\n", modes[i].eo.parse_options.read_precision);
		fprintf(file, "assume_denominators_nonzero=%i\n", modes[i].eo.assume_denominators_nonzero);
		fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", modes[i].eo.warn_about_denominators_assumed_nonzero);
		fprintf(file, "structuring=%i\n", modes[i].eo.structuring);
		fprintf(file, "angle_unit=%i\n", modes[i].eo.parse_options.angle_unit);
		fprintf(file, "functions_enabled=%i\n", modes[i].eo.parse_options.functions_enabled);
		fprintf(file, "variables_enabled=%i\n", modes[i].eo.parse_options.variables_enabled);
		fprintf(file, "calculate_functions=%i\n", modes[i].eo.calculate_functions);	
		fprintf(file, "calculate_variables=%i\n", modes[i].eo.calculate_variables);	
		fprintf(file, "sync_units=%i\n", modes[i].eo.sync_units);	
		fprintf(file, "unknownvariables_enabled=%i\n", modes[i].eo.parse_options.unknowns_enabled);
		fprintf(file, "units_enabled=%i\n", modes[i].eo.parse_options.units_enabled);
		fprintf(file, "allow_complex=%i\n", modes[i].eo.allow_complex);
		fprintf(file, "allow_infinite=%i\n", modes[i].eo.allow_infinite);
		fprintf(file, "indicate_infinite_series=%i\n", modes[i].po.indicate_infinite_series);
		fprintf(file, "show_ending_zeroes=%i\n", modes[i].po.show_ending_zeroes);
		fprintf(file, "round_halfway_to_even=%i\n", modes[i].po.round_halfway_to_even);
		fprintf(file, "approximation=%i\n", modes[i].eo.approximation);	
		fprintf(file, "in_rpn_mode=%i\n", modes[i].rpn_mode);
		fprintf(file, "rpn_syntax=%i\n", modes[i].eo.parse_options.rpn);
		fprintf(file, "limit_implicit_multiplication=%i\n", modes[i].eo.parse_options.limit_implicit_multiplication);
		fprintf(file, "parsing_mode=%i\n", modes[i].eo.parse_options.parsing_mode);
		fprintf(file, "spacious=%i\n", modes[i].po.spacious);
		fprintf(file, "excessive_parenthesis=%i\n", modes[i].po.excessive_parenthesis);
		fprintf(file, "short_multiplication=%i\n", modes[i].po.short_multiplication);
		fprintf(file, "default_assumption_type=%i\n", modes[i].at);
		fprintf(file, "default_assumption_sign=%i\n", modes[i].as);
	}
	fprintf(file, "\n[Plotting]\n");
	fprintf(file, "plot_legend_placement=%i\n", default_plot_legend_placement);
	fprintf(file, "plot_style=%i\n", default_plot_style);
	fprintf(file, "plot_smoothing=%i\n", default_plot_smoothing);
	fprintf(file, "plot_display_grid=%i\n", default_plot_display_grid);
	fprintf(file, "plot_full_border=%i\n", default_plot_full_border);
	fprintf(file, "plot_min=%s\n", default_plot_min.c_str());
	fprintf(file, "plot_max=%s\n", default_plot_max.c_str());
	fprintf(file, "plot_step=%s\n", default_plot_step.c_str());
	fprintf(file, "plot_sampling_rate=%i\n", default_plot_sampling_rate);
	fprintf(file, "plot_use_sampling_rate=%i\n", default_plot_use_sampling_rate);
	fprintf(file, "plot_variable=%s\n", default_plot_variable.c_str());
	fprintf(file, "plot_rows=%i\n", default_plot_rows);
	fprintf(file, "plot_type=%i\n", default_plot_type);
	fprintf(file, "plot_color=%i\n", default_plot_color);

	fclose(file);
	
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
	retval = g_ascii_strcasecmp(gstr1, gstr2);
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
	if(b1 == b2)
		retval = g_ascii_strcasecmp(gstr1, gstr2);
	else if(b1)
		retval = -1;
	else
		retval = 1;
	g_free(gstr1);
	g_free(gstr2);
	return retval;
}
/*
	display preferences dialog
*/
void edit_preferences() {
	GtkWidget *dialog = get_preferences_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	//save_preferences();
}

gchar *font_name_to_css(const char *font_name) {
	gchar *gstr = NULL;
	PangoFontDescription *font_desc = pango_font_description_from_string(font_name);
	gint size = pango_font_description_get_size(font_desc) / PANGO_SCALE;
	switch(pango_font_description_get_style(font_desc)) {
		case PANGO_STYLE_NORMAL: {
			gstr = g_strdup_printf("* {font-family: %s; font-weight: %i; font-size: %ipt;}", pango_font_description_get_family(font_desc), pango_font_description_get_weight(font_desc), size);
			break;
		}
		case PANGO_STYLE_OBLIQUE: {
			gstr = g_strdup_printf("* {font-family: %s; font-weight: %i; font-size: %ipt; font-style: oblique;}", pango_font_description_get_family(font_desc), pango_font_description_get_weight(font_desc), size);
			break;
		}
		case PANGO_STYLE_ITALIC: {
			gstr = g_strdup_printf("* {font-family: %s; font-weight: %i; font-size: %ipt; font-style: italic;}", pango_font_description_get_family(font_desc), pango_font_description_get_weight(font_desc), size);
			break;
		}
	}
	pango_font_description_free(font_desc);
	return gstr;
}

#ifdef __cplusplus
extern "C" {
#endif

void on_convert_treeview_category_row_expanded(GtkTreeView *tree_view, GtkTreeIter*, GtkTreePath *path, gpointer) {
	if(gtk_tree_path_get_depth(path) != 2) return;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter2, iter3;
	gtk_tree_model_get_iter_first(model, &iter3);
	if(gtk_tree_model_iter_children(model, &iter2, &iter3)) {
		do {
			GtkTreePath *path2 = gtk_tree_model_get_path(model, &iter2);
			if(gtk_tree_path_compare(path, path2) != 0) gtk_tree_view_collapse_row(GTK_TREE_VIEW(tUnitSelectorCategories), path2);
			gtk_tree_path_free(path2);
		} while(gtk_tree_model_iter_next(model, &iter2));
	}
	gtk_tree_view_scroll_to_cell(tree_view, path, NULL, FALSE, 0, 0);
}

void on_message_bar_response(GtkInfoBar *w, gint response_id, gpointer) {
	if(response_id == GTK_RESPONSE_CLOSE) {
		gint w, h, dur;
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "message_label")), "");
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_bar")));
		dur = gtk_revealer_get_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")));
		gtk_revealer_set_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), 0);
		gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), FALSE);
		gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), w, h);
		gtk_revealer_set_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), dur);
	}
}

void set_current_object() {
	if(!current_object_has_changed) return;
	while(gtk_events_pending()) gtk_main_iteration();
	GtkTextIter ipos, ipos2, istart, iend;
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expressionbuffer);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mpos);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos2, mpos);
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	if(gtk_text_iter_is_start(&ipos)) {
		current_object_start = iend;
		current_object_end = iend;
		editing_to_expression = false;
		return;
	}
	gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &ipos, FALSE);
	gchar *p = gstr + strlen(gstr);
	editing_to_expression = CALCULATOR->hasToExpression(gstr);
	bool non_number_before = false;
	while(gtk_text_iter_backward_char(&ipos2)) {
		p = g_utf8_prev_char(p);
		if(!CALCULATOR->utf8_pos_is_valid_in_name(p)) {
			gtk_text_iter_forward_char(&ipos2);
			break;
		} else if(is_in(NUMBERS, p[0])) {
			if(non_number_before) {
				gtk_text_iter_forward_char(&ipos2);
				break;
			}
		} else {
			non_number_before = true;
		}
	}
	g_free(gstr);
	if(gtk_text_iter_compare(&ipos2, &ipos) >= 0) {
		current_object_start = iend;
		current_object_end = iend;
	} else {
		current_object_start = ipos2;
		current_object_end = ipos;
		gstr = gtk_text_buffer_get_text(expressionbuffer, &ipos, &iend, FALSE);
		p = gstr;
		while(p[0] != '\0') {
			if(!CALCULATOR->utf8_pos_is_valid_in_name(p)) {
				break;
			}
			gtk_text_iter_forward_char(&current_object_end);
			p = g_utf8_next_char(p);
		}
		g_free(gstr);
	}
	current_object_has_changed = false;
}
void on_completion_match_selected(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn*, gpointer) {
	set_current_object();
	GtkTreeIter iter;
	gtk_tree_model_get_iter(completion_filter, &iter, path);
	ExpressionItem *item = NULL;
	const ExpressionName *ename = NULL, *ename_r = NULL;
	gtk_tree_model_get(completion_filter, &iter, 2, &item, -1);
	if(!item) return;
	ename_r = &item->preferredInputName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);	
	gchar *gstr2 = gtk_text_buffer_get_text(expressionbuffer, &current_object_start, &current_object_end, FALSE);
	for(size_t name_i = 0; name_i <= item->countNames() && !ename; name_i++) {
		if(name_i == 0) {
			ename = ename_r;
		} else {
			ename = &item->getName(name_i);
			if(!ename || ename == ename_r || ename->plural || (ename->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) expressiontext)))) {
				ename = NULL;
			}
		}
		if(ename) {
			if(strlen(gstr2) <= ename->name.length()) {
				for(size_t i = 0; i < strlen(gstr2); i++) {
					if(ename->name[i] != gstr2[i]) {
						ename = NULL;
						break;
					}
				}
			} else {
				ename = NULL;
			}
		}
	}
	for(size_t name_i = 1; name_i <= item->countNames() && !ename; name_i++) {
		ename = &item->getName(name_i);
		if(!ename || ename == ename_r || (!ename->plural && !(ename->unicode && (!printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) expressiontext))))) {
			ename = NULL;
		}
		if(ename) {
			if(strlen(gstr2) <= ename->name.length()) {
				for(size_t i = 0; i < strlen(gstr2); i++) {
					if(ename->name[i] != gstr2[i]) {
						ename = NULL;
						break;
					}
				}
			} else {
				ename = NULL;
			}
		}
	}
	if(!ename) ename = ename_r;
	g_free(gstr2);
	if(!ename) return;
	block_completion();
	add_to_undo = false;
	gtk_text_buffer_delete(expressionbuffer, &current_object_start, &current_object_end);
	add_to_undo = true;
	GtkTextIter ipos = current_object_start;
	if(item->type() == TYPE_FUNCTION) {
		GtkTextIter ipos2 = ipos;
		gtk_text_iter_forward_char(&ipos2);
		gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &ipos, &ipos2, FALSE);
		if(strlen(gstr) > 0 && gstr[0] == '(') {
			gtk_text_buffer_insert(expressionbuffer, &ipos, ename->name.c_str(), -1);
			gtk_text_buffer_place_cursor(expressionbuffer, &ipos);
		} else {
			string str = ename->name;
			str += "()";
			gtk_text_buffer_insert(expressionbuffer, &ipos, str.c_str(), -1);
			gtk_text_iter_backward_char(&ipos);
			gtk_text_buffer_place_cursor(expressionbuffer, &ipos);
		}
		g_free(gstr);
	} else {
		gtk_text_buffer_insert(expressionbuffer, &ipos, ename->name.c_str(), -1);
		gtk_text_buffer_place_cursor(expressionbuffer, &ipos);
	}
	gtk_widget_hide(completion_window);
	unblock_completion();
}

void on_menu_item_quit_activate(GtkMenuItem*, gpointer user_data) {
	on_gcalc_exit(NULL, NULL, user_data);
}

/*
	change preferences
*/
void on_colorbutton_status_error_color_color_set(GtkColorButton *w, gpointer) {
	GdkRGBA c;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &c);
	gchar color_str[8];
	g_snprintf(color_str, 8, "#%02x%02x%02x", (int) (c.red * 255), (int) (c.green * 255), (int) (c.blue * 255));
	status_error_color = color_str;
	status_error_color_set = true;
	display_parse_status();
}
void on_colorbutton_status_warning_color_color_set(GtkColorButton *w, gpointer) {
	GdkRGBA c;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &c);
	gchar color_str[8];
	g_snprintf(color_str, 8, "#%02x%02x%02x", (int) (c.red * 255), (int) (c.green * 255), (int) (c.blue * 255));
	status_warning_color = color_str;
	status_warning_color_set = true;
	display_parse_status();
}
void on_preferences_update_exchange_rates_spin_button_value_changed(GtkSpinButton *spin, gpointer) {
	auto_update_exchange_rates = gtk_spin_button_get_value_as_int(spin);
}
gint on_preferences_update_exchange_rates_spin_button_input(GtkSpinButton *spin, gdouble *new_value, gpointer) {
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(spin));
	if(g_strcmp0(text, _("never")) == 0) *new_value = 0.0;
	else if(g_strcmp0(text, _("ask")) == 0) *new_value = -1.0;
	else *new_value = g_strtod(text, NULL);
	return TRUE;
}
gboolean on_preferences_update_exchange_rates_spin_button_output(GtkSpinButton *spin, gpointer) {
	int value = gtk_spin_button_get_value_as_int(spin);
	if(value > 0) {
		gchar *text;
		text = g_strdup_printf(_("%i days"), value);
		gtk_entry_set_text(GTK_ENTRY(spin), text);
		g_free(text);
	} else if(value == 0) {
		gtk_entry_set_text(GTK_ENTRY(spin), _("never"));
	} else {
		gtk_entry_set_text(GTK_ENTRY(spin), _("ask"));
	}
	return TRUE;
}
void on_preferences_checkbutton_lower_case_numbers_toggled(GtkToggleButton *w, gpointer) {
	printops.lower_case_numbers = gtk_toggle_button_get_active(w);
	result_format_updated();
}
void on_preferences_checkbutton_lower_case_e_toggled(GtkToggleButton *w, gpointer) {
	printops.lower_case_e = gtk_toggle_button_get_active(w);
	result_format_updated();
}
void on_preferences_checkbutton_alternative_base_prefixes_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) printops.base_display = BASE_DISPLAY_ALTERNATIVE;
	else printops.base_display = BASE_DISPLAY_NORMAL;
	result_format_updated();
}
void on_preferences_checkbutton_spell_out_logical_operators_toggled(GtkToggleButton *w, gpointer) {
	printops.spell_out_logical_operators = gtk_toggle_button_get_active(w);
	result_display_updated();
}
void on_preferences_checkbutton_unicode_signs_toggled(GtkToggleButton *w, gpointer) {
	printops.use_unicode_signs = gtk_toggle_button_get_active(w);
	set_operator_symbols();
	set_unicode_buttons();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_asterisk")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_ex")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_dot")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_altdot")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_slash")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division_slash")), printops.use_unicode_signs);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division")), printops.use_unicode_signs);
	result_display_updated();
}

void on_preferences_checkbutton_save_defs_toggled(GtkToggleButton *w, gpointer) {
	save_defs_on_exit = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_save_mode_toggled(GtkToggleButton *w, gpointer) {
	save_mode_on_exit = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_allow_multiple_instances_toggled(GtkToggleButton *w, gpointer) {
	allow_multiple_instances = gtk_toggle_button_get_active(w);
	save_preferences(false);
}
void on_preferences_checkbutton_rpn_keys_toggled(GtkToggleButton *w, gpointer) {
	rpn_keys = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_dot_as_separator_toggled(GtkToggleButton *w, gpointer) {
	evalops.parse_options.dot_as_separator = gtk_toggle_button_get_active(w);
	expression_format_updated(false);
}
void on_preferences_checkbutton_comma_as_separator_toggled(GtkToggleButton *w, gpointer) {
	evalops.parse_options.comma_as_separator = gtk_toggle_button_get_active(w);
	CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
	set_unicode_buttons();
	expression_format_updated(false);
}
void on_preferences_checkbutton_load_defs_toggled(GtkToggleButton *w, gpointer) {
	load_global_defs = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_fetch_exchange_rates_toggled(GtkToggleButton *w, gpointer) {
	fetch_exchange_rates_at_startup = gtk_toggle_button_get_active(w);
}
void on_preferences_checkbutton_display_expression_status_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		display_expression_status = true;
		display_parse_status();
	} else {
		display_expression_status = false;
		set_status_text("");
	}
}
void on_preferences_checkbutton_enable_completion_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		enable_completion = true;
		update_completion();
	} else {
		enable_completion = false;
		update_completion();
	}
}
void on_preferences_checkbutton_custom_result_font_toggled(GtkToggleButton *w, gpointer) {
	use_custom_result_font = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), use_custom_result_font);
	gint h_old, h_new;
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_old);
	if(use_custom_result_font) {
		gchar *gstr = font_name_to_css(custom_result_font.c_str());
		gtk_css_provider_load_from_data(resultview_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(resultview_provider, "* {font-size: larger;}", -1, NULL);
	}
	result_font_modified();
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_new);
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	winh += (h_new - h_old);
	gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
}
void on_preferences_checkbutton_custom_expression_font_toggled(GtkToggleButton *w, gpointer) {
	use_custom_expression_font = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), use_custom_expression_font);
	gint h_old, h_new;
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h_old);
	if(use_custom_expression_font) {
		gchar *gstr = font_name_to_css(custom_expression_font.c_str());
		gtk_css_provider_load_from_data(expression_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(expression_provider, "", -1, NULL);
	}
	expression_font_modified();
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h_new);
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	winh += (h_new - h_old);
	gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
}
void on_preferences_checkbutton_custom_status_font_toggled(GtkToggleButton *w, gpointer) {
	use_custom_status_font = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), use_custom_status_font);
	gint h_old = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
	if(use_custom_status_font) {
		gchar *gstr = font_name_to_css(custom_status_font.c_str());
		gtk_css_provider_load_from_data(statuslabel_l_provider, gstr, -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(statuslabel_l_provider, "* {font-size: smaller;}", -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, "* {font-size: smaller;}", -1, NULL);
	}
	set_operator_symbols();
	while(gtk_events_pending()) gtk_main_iteration();
	gint h_new = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	winh += (h_new - h_old);
	gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
}
void on_preferences_radiobutton_dot_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_DOT;
		result_display_updated();
	}
}
void on_preferences_radiobutton_altdot_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_ALTDOT;
		result_display_updated();
	}
}
void on_preferences_radiobutton_ex_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_X;
		result_display_updated();
	}
}
void on_preferences_radiobutton_asterisk_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.multiplication_sign = MULTIPLICATION_SIGN_ASTERISK;
		result_display_updated();
	}
}
void on_preferences_radiobutton_slash_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.division_sign = DIVISION_SIGN_SLASH;
		result_display_updated();
	}
}
void on_preferences_radiobutton_division_slash_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.division_sign = DIVISION_SIGN_DIVISION_SLASH;
		result_display_updated();
	}
}
void on_preferences_radiobutton_division_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.division_sign = DIVISION_SIGN_DIVISION;
		result_display_updated();
	}
}

void on_preferences_button_result_font_font_set(GtkFontButton *w, gpointer) {
	save_custom_result_font = true;
	custom_result_font = gtk_font_button_get_font_name(w);
	gint h_old, h_new;
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_old);
	gchar *gstr = font_name_to_css(custom_result_font.c_str());
	gtk_css_provider_load_from_data(resultview_provider, gstr, -1, NULL);
	g_free(gstr);
	result_font_modified();
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_new);
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	winh += (h_new - h_old);
	gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
}
void on_preferences_button_expression_font_font_set(GtkFontButton *w, gpointer) {
	save_custom_expression_font = true;
	custom_expression_font = gtk_font_button_get_font_name(w);
	gint h_old, h_new;
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h_old);
	gchar *gstr = font_name_to_css(custom_expression_font.c_str());
	gtk_css_provider_load_from_data(expression_provider, gstr, -1, NULL);
	g_free(gstr);
	expression_font_modified();
	gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h_new);
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	winh += (h_new - h_old);
	gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
}
void on_preferences_button_status_font_font_set(GtkFontButton *w, gpointer) {
	save_custom_status_font = true;
	custom_status_font = gtk_font_button_get_font_name(w);
	gint h_old = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
	gchar *gstr = font_name_to_css(custom_status_font.c_str());
	gtk_css_provider_load_from_data(statuslabel_l_provider, gstr, -1, NULL);
	gtk_css_provider_load_from_data(statuslabel_r_provider, gstr, -1, NULL);
	g_free(gstr);
	set_operator_symbols();
	while(gtk_events_pending()) gtk_main_iteration();
	gint h_new = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	winh += (h_new - h_old);
	gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
}

/*
	hide unit manager when "Close" clicked
*/
void on_units_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
}

/*
	change conversion direction in unit manager on user request
*/
void on_units_toggle_button_from_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_toggle_button_to")), FALSE);
		convert_in_wUnits();
	}
}

/*
	convert button clicked
*/
void on_units_button_convert_clicked(GtkButton*, gpointer) {
	convert_in_wUnits();
}

/*
	change conversion direction in unit manager on user request
*/
void on_units_toggle_button_to_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_toggle_button_from")), FALSE);
		convert_in_wUnits();
	}
}

/*
	enter in conversion field
*/
void on_units_entry_from_val_activate(GtkEntry*, gpointer) {
	convert_in_wUnits(0);
}
void on_units_entry_to_val_activate(GtkEntry*, gpointer) {
	convert_in_wUnits(1);
}

void update_resultview_popup() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_octal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_decimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_hexadecimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_binary_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_roman_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_sexagesimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_time_format"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_time_format_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_custom_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_custom_base_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_normal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_normal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_engineering"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_engineering_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_scientific_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_purely_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_purely_scientific_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_non_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_non_scientific_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_no_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_no_prefixes_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_selected_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_prefixes_for_selected_units_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_currencies"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_prefixes_for_currencies_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_all_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_prefixes_for_all_units_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_abbreviate_names_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_denominator_prefixes_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_denominator_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_all_prefixes_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_exact_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_combined_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_fraction_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_assume_nonzero_denominators_activate, NULL);
	bool b_unit = mstruct && mstruct->containsType(STRUCT_UNIT);
	
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names")), !b_busy);	
	
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_display_prefixes")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_no_prefixes")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_selected_units")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_currencies")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_all_units")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_denominator_prefixes")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_unit_settings")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_convert_to_unit")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_convert_to_base_units")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_convert_to_best_unit")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_optimal_prefix")), b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_units")), b_unit);
	
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_octal")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_decimal")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_hexadecimal")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_binary")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_roman")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_sexagesimal")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_time_format")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_custom_base")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_base")), !b_unit);
	
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_normal")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_engineering")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_scientific")), !b_unit);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_purely_scientific")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_non_scientific")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_display")), !b_unit);	
	
	if(mstruct && mstruct->containsUnknowns()) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_unknowns")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_unknowns")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	}
	if(mstruct && mstruct->containsType(STRUCT_ADDITION)) {
		if(evalops.structuring == STRUCTURING_FACTORIZE) {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
		} else {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
		}
		if(evalops.structuring == STRUCTURING_SIMPLIFY) {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_simplify")));
		} else {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_simplify")));
		}
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	} else {
		if(mstruct && mstruct->isNumber() && mstruct->number().isInteger() && !mstruct->number().isZero()) {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
		} else {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
		}
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_simplify")));
	}
	if(mstruct && mstruct->containsDivision()) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_nonzero")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_nonzero")));
	}
	if(mstruct->isVector() && (mstruct->size() != 1 || !(*mstruct)[0].isVector() || (*mstruct)[0].size() > 0)) {
		if(mstruct->isMatrix()) {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_view_matrix")));
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_view_vector")));
		} else {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_view_matrix")));
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_view_vector")));
		}
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_view_matrixvector")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_view_matrix")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_view_vector")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_view_matrixvector")));
	}
	switch (printops.base) {
		case BASE_OCTAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_octal")), TRUE);
			break;
		}
		case BASE_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_decimal")), TRUE);
			break;
		}
		case BASE_HEXADECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_hexadecimal")), TRUE);
			break;
		}
		case BASE_BINARY: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_binary")), TRUE);
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_roman")), TRUE);
			break;
		}
		case BASE_SEXAGESIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_sexagesimal")), TRUE);
			break;
		}
		case BASE_TIME: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_time_format")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_custom_base")), TRUE);
			break;
		}
	}
	switch(printops.min_exp) {
		case EXP_PRECISION: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_normal")), TRUE);
			break;
		}
		case EXP_BASE_3: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_engineering")), TRUE);
			break;
		}
		case EXP_SCIENTIFIC: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_scientific")), TRUE);
			break;
		}
		case EXP_PURE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_purely_scientific")), TRUE);
			break;
		}
		case EXP_NONE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_non_scientific")), TRUE);
			break;
		}
	}
	if(!printops.use_unit_prefixes) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_no_prefixes")), TRUE);
	} else if(printops.use_prefixes_for_all_units) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_all_units")), TRUE);
	} else if(printops.use_prefixes_for_currencies) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_currencies")), TRUE);
	} else {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_selected_units")), TRUE);
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names")), printops.abbreviate_names);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes")), printops.use_all_prefixes);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_denominator_prefixes")), printops.use_denominator_prefix);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")), evalops.assume_denominators_nonzero);
	switch(printops.number_fraction_format) {
		case FRACTION_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal")), TRUE);
			break;
		}
		case FRACTION_DECIMAL_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal_exact")), TRUE);
			break;
		}
		case FRACTION_COMBINED: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_combined")), TRUE);
			break;		
		}
		case FRACTION_FRACTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_fraction")), TRUE);
			break;		
		}
	}
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_octal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_decimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_hexadecimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_binary_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_roman_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_sexagesimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_time_format"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_time_format_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_custom_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_custom_base_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_normal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_normal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_engineering"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_engineering_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_scientific_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_purely_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_purely_scientific_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_non_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_non_scientific_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_no_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_no_prefixes_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_selected_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_prefixes_for_selected_units_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_all_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_prefixes_for_all_units_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_display_prefixes_for_currencies"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_display_prefixes_for_currencies_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_abbreviate_names_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_all_prefixes_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_denominator_prefixes_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_exact_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_combined_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_fraction_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_assume_nonzero_denominators_activate, NULL);
}
void on_expression_button_clicked(GtkButton*, gpointer) {
	GtkWidget *w = gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack")));
	if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_equals"))) {
		execute_expression();
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button_clear"))) {
		clear_expression_text();
		gtk_widget_grab_focus(expressiontext);
	} else if(w == GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon"))) {
		gtk_widget_trigger_tooltip_query(w);
	} else {
		if(b_busy_command) on_abort_command(NULL, 0, NULL);
		else if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
		else if(b_busy_result) on_abort_display(NULL, 0, NULL);
	}
}
gboolean on_expressiontext_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		if(b_busy) return TRUE;
	}
	return FALSE;
}
gboolean on_main_window_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	gtk_widget_hide(completion_window);
	return FALSE;
}
gboolean on_resultview_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		if(b_busy) return TRUE;
		update_resultview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_resultview_popup_menu(GtkWidget*, gpointer) {
	if(b_busy) return TRUE;
	update_resultview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_units_entry_from_val_focus_out_event(GtkEntry*, GdkEventFocus*, gpointer) {
	if(old_fromValue != gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")))) convert_in_wUnits(0);
	return FALSE;
}
gboolean on_units_entry_to_val_focus_out_event(GtkEntry*, GdkEventFocus*, gpointer) {
	if(old_toValue != gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")))) convert_in_wUnits(1);
	return FALSE;
}

/*
	angle mode radio buttons toggled
*/
void on_radiobutton_radians_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
		set_angle_item();
		expression_format_updated(true);
	}
	focus_keeping_selection();
}
void on_radiobutton_degrees_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_DEGREES;
		set_angle_item();
		expression_format_updated(true);
	}
	focus_keeping_selection();
}
void on_radiobutton_gradians_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_GRADIANS;
		set_angle_item();
		expression_format_updated(true);
	}
	focus_keeping_selection();
}
void on_radiobutton_no_default_angle_unit_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_NONE;
		set_angle_item();
		expression_format_updated(true);
	}
	focus_keeping_selection();
}

/*
	enter in expression entry does the same as clicking "Execute" button
*/
void on_expression_activate(GtkEntry*, gpointer) {
	execute_expression();
}

void on_convert_entry_unit_icon_release(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent*, gpointer) {
	switch(icon_pos) {
		case GTK_ENTRY_ICON_PRIMARY: {
			break;
		}
		case GTK_ENTRY_ICON_SECONDARY: {
			gtk_editable_delete_text(GTK_EDITABLE(entry), 0, -1);
			break;
		}
	}
}

/*
	"Execute" clicked
*/
void on_button_execute_clicked(GtkButton*, gpointer) {
	execute_expression();
}
/*
	save preferences, mode and definitions and then quit
*/
gboolean on_gcalc_exit(GtkWidget*, GdkEvent*, gpointer) {
	stop_timeouts = true;
	exit_in_progress = true;
	CALCULATOR->abort();
	if(plot_builder && gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")))) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
	}
	//save_accels();
	if(save_mode_on_exit) {
		save_mode();
	} else {
		save_preferences();
	}
	if(save_defs_on_exit) {
		save_defs();
	}
	for(size_t i = 0; i < history_parsed.size(); i++) {
		if(history_parsed[i]) history_parsed[i]->unref();
		if(history_answer[i]) history_answer[i]->unref();
	}
	view_thread->cancel();
	CALCULATOR->terminateThreads();
	g_application_quit(g_application_get_default());
	return TRUE;
}

void save_accels() {
	g_mkdir(getLocalDir().c_str(), S_IRWXU);
	gchar *gstr = g_build_filename(getLocalDir().c_str(), "accelmap", NULL);
	gtk_accel_map_save(gstr);
	g_free(gstr);
}


/*
	DEL button clicked -- delete in expression entry
*/
void on_button_del_clicked(GtkButton*, gpointer) {
	if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
		overwrite_expression_selection(NULL);
		return;
	}
	block_completion();
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expressionbuffer);
	GtkTextIter ipos, iend;
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mpos);
	if(gtk_text_iter_is_end(&ipos)) {
		gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
		if(gtk_text_iter_backward_char(&ipos)) {
			gtk_text_buffer_delete(expressionbuffer, &ipos, &iend);
		}
	} else {
		iend = ipos;
		if(!gtk_text_iter_forward_char(&iend)) {
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
		}
		gtk_text_buffer_delete(expressionbuffer, &ipos, &iend);
	}
	gtk_widget_grab_focus(expressiontext);
	unblock_completion();
}

/*
	AC button clicked -- clear expression entry
*/
void on_button_ac_clicked(GtkButton*, gpointer) {
	clear_expression_text();
	gtk_widget_grab_focus(expressiontext);
}

/*
	HYP button toggled -- enable/disable hyperbolic functions
*/
void on_button_hyp_toggled(GtkToggleButton *w, gpointer) {
	hyp_is_on = gtk_toggle_button_get_active(w);
	focus_keeping_selection();
}

/*
	INV button toggled -- enable/disable inverse functions
*/

void on_button_inv_toggled(GtkToggleButton *w, gpointer) {
	inv_is_on = gtk_toggle_button_get_active(w);
	focus_keeping_selection();
}

/*
	fraction button toggled -- enable/disable fractional display
*/
void on_button_fraction_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		printops.number_fraction_format = FRACTION_FRACTIONAL;
		GtkWidget *w_fraction = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction"));
		g_signal_handlers_block_matched((gpointer) w_fraction, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_fraction_activate, NULL);		
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w_fraction), TRUE);		
		g_signal_handlers_unblock_matched((gpointer) w_fraction, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_fraction_activate, NULL);		
	} else {
		printops.number_fraction_format = FRACTION_DECIMAL;
		GtkWidget *w_fraction = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal"));
		g_signal_handlers_block_matched((gpointer) w_fraction, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_decimal_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w_fraction), TRUE);
		g_signal_handlers_unblock_matched((gpointer) w_fraction, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_decimal_activate, NULL);
	}
	result_format_updated();
	focus_keeping_selection();
}

/*
	exact button toggled
*/
void on_button_exact_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		evalops.approximation = APPROXIMATION_EXACT;
		GtkWidget *w_exact = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_always_exact"));
		g_signal_handlers_block_matched((gpointer) w_exact, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_always_exact_activate, NULL);		
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w_exact), TRUE);		
		g_signal_handlers_unblock_matched((gpointer) w_exact, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_always_exact_activate, NULL);		
	} else {
		evalops.approximation = APPROXIMATION_TRY_EXACT;
		GtkWidget *w_exact = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_try_exact"));
		g_signal_handlers_block_matched((gpointer) w_exact, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_try_exact_activate, NULL);		
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w_exact), TRUE);		
		g_signal_handlers_unblock_matched((gpointer) w_exact, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_try_exact_activate, NULL);		
	}
	expression_calculation_updated();
	focus_keeping_selection();
}

/*
	Tan/Sin/Cos button clicked -- insert corresponding function
*/
void on_button_tan_clicked(GtkButton*, gpointer) {
	if(hyp_is_on) {
		if(inv_is_on) {
			insertButtonFunction(CALCULATOR->f_atanh);
			inv_is_on = false;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_inv")), FALSE);
		} else {
			insertButtonFunction(CALCULATOR->f_tanh);
		}
		hyp_is_on = false;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_hyp")), FALSE);
	} else if(inv_is_on) {
		insertButtonFunction(CALCULATOR->f_atan);
		inv_is_on = false;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_inv")), FALSE);
	} else {
		insertButtonFunction(CALCULATOR->f_tan);
	}
}
void on_button_sine_clicked(GtkButton*, gpointer) {
	if(hyp_is_on) {
		if(inv_is_on) {
			insertButtonFunction(CALCULATOR->f_asinh);
			inv_is_on = false;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_inv")), FALSE);
		} else {
			insertButtonFunction(CALCULATOR->f_sinh);
		}
		hyp_is_on = false;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_hyp")), FALSE);
	} else if(inv_is_on) {
		insertButtonFunction(CALCULATOR->f_asin);
		inv_is_on = false;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_inv")), FALSE);
	} else {
		insertButtonFunction(CALCULATOR->f_sin);
	}
}
void on_button_cosine_clicked(GtkButton*, gpointer) {
	if(hyp_is_on) {
		if(inv_is_on) {
			insertButtonFunction(CALCULATOR->f_acosh);
			inv_is_on = false;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_inv")), FALSE);
		} else {
			insertButtonFunction(CALCULATOR->f_cosh);
		}
		hyp_is_on = false;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_hyp")), FALSE);
	} else if(inv_is_on) {
		insertButtonFunction(CALCULATOR->f_acos);
		inv_is_on = false;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_inv")), FALSE);
	} else {
		insertButtonFunction(CALCULATOR->f_cos);
	}
}

void on_button_mod_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->f_mod);
}

void on_button_reciprocal_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->getActiveFunction("inv"));
}

/*
	STO button clicked -- store result
*/
void on_button_store_clicked(GtkButton*, gpointer) {
	if(displayed_mstruct && mstruct && !mstruct->isZero()) add_as_variable();
	else edit_variable(_("My Variables"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

bool completion_ignore_enter = false, completion_hover_blocked = false;

gboolean on_completionview_enter_notify_event(GtkWidget*, GdkEventCrossing*, gpointer data) {
	return completion_ignore_enter;
}
gboolean on_completionview_motion_notify_event(GtkWidget*, GdkEventMotion*, gpointer data) {
	completion_ignore_enter = FALSE;
	if(completion_hover_blocked) {
		gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(completion_view), TRUE);
		completion_hover_blocked = false;
	}
	return FALSE;
}
gboolean on_completionwindow_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	if(!gtk_widget_get_mapped(completion_window)) return FALSE;
	gtk_widget_event(expressiontext, (GdkEvent*) event);
	return TRUE;
}
gboolean on_completionwindow_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
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
	gboolean above;
	GtkTreeViewColumn *column;

	GtkTextMark *miter = gtk_text_buffer_get_insert(expressionbuffer);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iter, miter);
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(expressiontext), &iter, &bufloc);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(expressiontext), GTK_TEXT_WINDOW_WIDGET, bufloc.x, bufloc.y, &bufloc.x, &bufloc.y);
	window = gtk_text_view_get_window (GTK_TEXT_VIEW(expressiontext), GTK_TEXT_WINDOW_WIDGET);
	gdk_window_get_origin(window, &x, &y);

	x += bufloc.x;
	y += bufloc.y;

	gtk_widget_realize(completion_view);
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

	display = gtk_widget_get_display(GTK_WIDGET(expressiontext));
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	monitor = gdk_display_get_monitor_at_window(display, window);
	gdk_monitor_get_workarea(monitor, &area);
#else
	GdkScreen *screen = gdk_display_get_default_screen(display);
	gdk_screen_get_monitor_workarea(screen, gdk_screen_get_monitor_at_window(screen, window), &area);
#endif
	
	items = matches;
	if(items > 20) items = 20;
	if(items > 0) {
		path = gtk_tree_path_new_from_indices(items - 1, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
		gtk_tree_path_free(path);
		height = rect.y + rect.height - items_y + height_diff;
	}
	while(items > 0 && ((y > area.height / 2 && area.y + y < height) || (y <= area.height / 2 && area.height - y < height))) {
		items--;
		path = gtk_tree_path_new_from_indices(items - 1, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(completion_view), path, column, &rect);
		gtk_tree_path_free(path);
		height = rect.y + rect.height - items_y + height_diff;
	}

	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(completion_scrolled), height);

	if(items <= 0) gtk_widget_hide(completion_scrolled);
	else gtk_widget_show(completion_scrolled);

	gtk_widget_get_preferred_size(completion_window, &popup_req, NULL);
	
	if(popup_req.width < rect.width + 2) popup_req.width = rect.width + 2;

	if(x < area.x) x = area.x;
	else if(x + popup_req.width > area.x + area.width) x = area.x + area.width - popup_req.width;

	if(y + bufloc.height + popup_req.height <= area.y + area.height || y - area.y < (area.y + area.height) - (y + bufloc.height)) {
		y += bufloc.height;
		above = FALSE;
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
		y -= popup_req.height;
		above = TRUE;
	}

	if(matches > 0) {
		path = gtk_tree_path_new_from_indices(above ? matches - 1 : 0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(completion_view), path, NULL, FALSE, 0.0, 0.0);
		gtk_tree_path_free(path);
	}

	gtk_window_move(GTK_WINDOW(completion_window), x, y);

}

void do_completion() {
	set_current_object();
	if(gtk_text_iter_is_end(&current_object_start)) {
		gtk_widget_hide(completion_window); 
		return;
	}
	gchar *gstr2 = gtk_text_buffer_get_text(expressionbuffer, &current_object_start, &current_object_end, FALSE);
	GtkTreeIter iter;
	int matches = 0;
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(completion_store), &iter)) {
		do {
			ExpressionItem *item = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(completion_store), &iter, 2, &item, -1);
			bool b_match = false;
			if(item) {
				if((editing_to_expression || !evalops.parse_options.functions_enabled) && item->type() == TYPE_FUNCTION) {}
				else if((editing_to_expression || !evalops.parse_options.variables_enabled) && item->type() == TYPE_VARIABLE) {}
				else if(!evalops.parse_options.units_enabled && item->type() == TYPE_UNIT) {}
				else {
					for(size_t name_i = 1; name_i <= item->countNames() && !b_match; name_i++) {
						const ExpressionName *ename = &item->getName(name_i);
						if(ename && strlen(gstr2) <= ename->name.length()) {
							b_match = true;
							for(size_t i = 0; i < strlen(gstr2); i++) {
								if(ename->name[i] != gstr2[i]) {
									b_match = false;
									break;
								}
							}
						}
					}
				}
			}
			gtk_list_store_set(completion_store, &iter, 3, b_match, -1);
			if(b_match) matches++;
		} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(completion_store), &iter));
	}
	g_free(gstr2);
	if(matches > 0) {
		completion_ignore_enter = TRUE;
		completion_resize_popup(matches);
		if(!gtk_widget_is_visible(completion_window)) {
			gtk_window_set_transient_for(GTK_WINDOW(completion_window), GTK_WINDOW(mainwindow));
			gtk_window_group_add_window(gtk_window_get_group(GTK_WINDOW(mainwindow)), GTK_WINDOW(completion_window));
			gtk_window_set_screen(GTK_WINDOW(completion_window), gtk_widget_get_screen(expressiontext));
			gtk_widget_show(completion_window);
		}
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)));
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)));
	} else {
		gtk_widget_hide(completion_window);
	}
}
void on_expressionbuffer_changed(GtkTextBuffer*, gpointer) {
	if(add_to_undo) add_expression_to_undo();
	expression_has_changed = true;
	expression_has_changed2 = true;
	current_object_has_changed = true;
	expression_has_changed_pos = true;
	highlight_parentheses();
	display_parse_status();
	if(!completion_blocked) {
		do_completion();
	}
	if(result_text.empty()) return;
	if(!dont_change_index) expression_history_index = -1;
	if(!rpn_mode) clearresult();
}

void on_convert_entry_unit_changed(GtkEditable *w, gpointer) {
	bool b = gtk_entry_get_text_length(GTK_ENTRY(w)) > 0;
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, b ? "edit-clear-symbolic" : NULL);
	gtk_entry_set_icon_tooltip_text(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, b ? _("Clear expression") : NULL);
}


/*
	Button clicked -- insert text (1,2,3,... +,-,...)
*/
void on_button_zero_clicked(GtkButton*, gpointer) {
	insert_text("0");
}
void on_button_one_clicked(GtkButton*, gpointer) {
	insert_text("1");
}
void on_button_two_clicked(GtkButton*, gpointer) {
	insert_text("2");
}
void on_button_three_clicked(GtkButton*, gpointer) {
	insert_text("3");
}
void on_button_four_clicked(GtkButton*, gpointer) {
	insert_text("4");
}
void on_button_five_clicked(GtkButton*, gpointer) {
	insert_text("5");
}
void on_button_six_clicked(GtkButton*, gpointer) {
	insert_text("6");
}
void on_button_seven_clicked(GtkButton*, gpointer) {
	insert_text("7");
}
void on_button_eight_clicked(GtkButton*, gpointer) {
	insert_text("8");
}
void on_button_nine_clicked(GtkButton*, gpointer) {
	insert_text("9");
}
void on_button_dot_clicked(GtkButton*, gpointer) {
	insert_text(CALCULATOR->getDecimalPoint().c_str());
}
void on_button_brace_open_clicked(GtkButton*, gpointer) {
	insert_text("(");
}
void on_button_brace_close_clicked(GtkButton*, gpointer) {
	insert_text(")");
}
void on_button_brace_wrap_clicked(GtkButton*, gpointer) {
	string expr = get_expression_text();
	GtkTextIter istart, iend, ipos;
	gint il = expr.length();
	if(il == 0) {
		set_expression_text("()");
		gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
		gtk_text_iter_forward_char(&istart);
		gtk_text_buffer_place_cursor(expressionbuffer, &istart);
		return;
	}
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expressionbuffer);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mpos);
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	iend = istart;
	bool goto_start = false;
	if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
		GtkTextMark *mstart = gtk_text_buffer_get_selection_bound(expressionbuffer);
		gtk_text_buffer_get_iter_at_mark(expressionbuffer, &istart, mstart);
		if(gtk_text_iter_compare(&istart, &ipos) > 0) {
			iend = istart;
			istart = ipos;
		} else {
			iend = ipos;
		}
	} else {
		iend = ipos;
		if(!gtk_text_iter_is_start(&iend)) {
			gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
			string str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
			g_free(gstr);
			CALCULATOR->parseSigns(str);
			if(str.empty() || is_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1])) {
				istart = iend;
				gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
				if(gtk_text_iter_compare(&istart, &iend) < 0) {
					gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
					str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
					g_free(gstr);
					CALCULATOR->parseSigns(str);
					if(str.empty() || (is_in(OPERATORS SPACES SEXADOT DOT RIGHT_VECTOR_WRAP LEFT_PARENTHESIS RIGHT_PARENTHESIS COMMAS, str[0]) && str[0] != MINUS_CH)) {
						iend = istart;
					}
				}
			}
		} else {
			goto_start = true;
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
			string str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
			g_free(gstr);
			CALCULATOR->parseSigns(str);
			if(str.empty() || (is_in(OPERATORS SPACES SEXADOT DOT RIGHT_VECTOR_WRAP LEFT_PARENTHESIS RIGHT_PARENTHESIS COMMAS, str[0]) && str[0] != MINUS_CH)) {
				iend = istart;
			}
		}
	}
	if(gtk_text_iter_compare(&istart, &iend) >= 0) {
		gtk_text_buffer_insert(expressionbuffer, &istart, "()", -1);
		gtk_text_iter_backward_char(&istart);
		gtk_text_buffer_place_cursor(expressionbuffer, &istart);
		return;
	}
	gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	GtkTextMark *mstart = gtk_text_buffer_create_mark(expressionbuffer, "istart", &istart, TRUE);
	GtkTextMark *mend = gtk_text_buffer_create_mark(expressionbuffer, "iend", &iend, FALSE);
	add_to_undo = false;
	gtk_text_buffer_insert(expressionbuffer, &istart, "(", -1);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iend, mend);
	add_to_undo = true;
	gtk_text_buffer_insert(expressionbuffer, &iend, ")", -1);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &istart, mstart);
	gtk_text_buffer_delete_mark(expressionbuffer, mstart);
	gtk_text_buffer_delete_mark(expressionbuffer, mend);
	string str = CALCULATOR->unlocalizeExpression(gstr, evalops.parse_options);
	g_free(gstr);
	CALCULATOR->parseSigns(str);
	if(str.empty() || is_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1])) {
		gtk_text_iter_backward_char(&iend);
		goto_start = false;
	}
	gtk_text_buffer_place_cursor(expressionbuffer, goto_start ? &istart : &iend);
}
void on_button_i_clicked(GtkButton*, gpointer) {
	insert_text("i");
}
void on_button_percent_clicked(GtkButton*, gpointer) {
	insert_text("%");
}
void on_button_si_clicked(GtkButton*, gpointer) {
	if(latest_button_unit) {
		insert_button_unit(NULL, (gpointer) latest_button_unit);
	} else {
		insert_text("kg");
	}
}
void on_button_euro_clicked(GtkButton*, gpointer) {
	if(latest_button_currency) {
		insert_button_currency(NULL, (gpointer) latest_button_currency);
	} else {
		insert_text("€");
	}
}

void on_button_to_clicked(GtkButton*, gpointer) {
	GtkTextIter istart, iend;
	if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
		gtk_text_buffer_get_bounds(expressionbuffer, &istart, &iend);
		gtk_text_buffer_select_range(expressionbuffer, &iend, &iend);
	}
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	string to_str = CALCULATOR->localToString();
	remove_blank_ends(to_str);
	to_str += ' ';
	if(strlen(gstr) > 0 && gstr[strlen(gstr) - 1] != ' ') to_str.insert(0, " ");
	insert_text(to_str.c_str());
	g_free(gstr);
}
void on_button_new_function_clicked(GtkButton*, gpointer) {
	edit_function_simple("", NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_button_fac_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->f_factorial);
}
void on_button_comma_clicked(GtkButton*, gpointer) {
	insert_text(CALCULATOR->getComma().c_str());
}
void on_button_x_clicked(GtkButton*, gpointer) {
	insert_text("x");
}
void on_button_y_clicked(GtkButton*, gpointer) {
	insert_text("y");
}
void on_button_z_clicked(GtkButton*, gpointer) {
	insert_text("z");
}
void on_button_factorize_clicked(GtkButton*, gpointer) {
	executeCommand(COMMAND_FACTORIZE);
}
void on_button_add_clicked(GtkButton*, gpointer) {
	if(rpn_mode) {
		calculateRPN(OPERATION_ADD);
		return;
	}
	if(!evalops.parse_options.rpn) {
		wrap_expression_selection();
	}
	insert_text(expression_add_sign());
}
void on_button_sub_clicked(GtkButton*, gpointer) {
	if(rpn_mode) {
		calculateRPN(OPERATION_SUBTRACT);
		return;
	}
	if(!evalops.parse_options.rpn) {
		wrap_expression_selection();
	}
	insert_text(expression_sub_sign());
}
void on_button_times_clicked(GtkButton*, gpointer) {
	if(rpn_mode) {
		calculateRPN(OPERATION_MULTIPLY);
		return;
	}
	if(!evalops.parse_options.rpn) {
		wrap_expression_selection();
	}
	insert_text(expression_times_sign());
}
void on_button_divide_clicked(GtkButton*, gpointer) {
	if(rpn_mode) {
		calculateRPN(OPERATION_DIVIDE);
		return;
	}
	if(!evalops.parse_options.rpn) {
		wrap_expression_selection();
	}
	insert_text(expression_divide_sign());
}
void on_button_ans_clicked(GtkButton*, gpointer) {
	insert_text(vans[0]->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
}
void on_button_exp_clicked(GtkButton*, gpointer) {
	if(rpn_mode) {
		calculateRPN(OPERATION_EXP10);
		return;
	}
	if(printops.lower_case_e) insert_text("e");
	else insert_text("E");	
}
void on_button_xy_clicked(GtkButton*, gpointer) {
	if(rpn_mode) {
		calculateRPN(OPERATION_RAISE);
		return;
	}
	if(!evalops.parse_options.rpn) {
		wrap_expression_selection();
	}
	insert_text("^");
}
void on_button_square_clicked() {
	if(rpn_mode) {
		calculateRPN(CALCULATOR->f_sq);
		return;
	}
	if(evalops.parse_options.rpn) {
		insertButtonFunction(CALCULATOR->f_sq);
	} else {
		wrap_expression_selection();
		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_POWER_2, (void*) expressiontext)) insert_text(SIGN_POWER_2);
		else insert_text("^2");
	}
}

/*
	Button clicked -- insert corresponding function
*/
void on_button_sqrt_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->f_sqrt);
}
void on_button_log_clicked(GtkButton*, gpointer) {
	MathFunction *f = CALCULATOR->getActiveFunction("log10");
	if(f) {
		insertButtonFunction(f);
	} else {
		show_message(_("log10 function not found."), GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
	}
}
void on_button_ln_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->f_ln);
}

/*
	Buttons to the left of the RPN stack clicked
*/
void on_button_rpn_add_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_ADD);
}
void on_button_rpn_sub_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_SUBTRACT);
}
void on_button_rpn_times_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_MULTIPLY);
}
void on_button_rpn_divide_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_DIVIDE);
}
void on_button_rpn_xy_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_RAISE);
}
void on_button_rpn_sqrt_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->f_sqrt);
}
void on_button_rpn_reciprocal_clicked(GtkButton*, gpointer) {
	insertButtonFunction(CALCULATOR->getActiveFunction("inv"));
}
#define INDEX_TYPE_ANS 0
#define INDEX_TYPE_XPR 1
#define INDEX_TYPE_TXT 2
void process_history_selection(vector<size_t> *selected_rows, vector<size_t> *selected_indeces, vector<int> *selected_index_type, bool ans_priority = false) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *selected_list, *current_selected_list;
	gint index = -1, hindex = -1;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(historyview));
	selected_list = gtk_tree_selection_get_selected_rows(select, &model);
	current_selected_list = selected_list;
	while(current_selected_list) {
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) current_selected_list->data);
		gtk_tree_model_get(model, &iter, 1, &hindex, 3, &index, -1);
		if(hindex >= 0) {
			if(selected_rows) selected_rows->push_back((size_t) hindex);
			if(selected_indeces && (index <= 0 || !evalops.parse_options.functions_enabled || evalops.parse_options.base > BASE_DECIMAL || evalops.parse_options.base < 0)) {
				if(inhistory_type[hindex] != QALCULATE_HISTORY_WARNING && inhistory_type[hindex] != QALCULATE_HISTORY_ERROR && (hindex < 1 || inhistory_type[hindex] != QALCULATE_HISTORY_TRANSFORMATION || inhistory_type[hindex - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[hindex - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) {
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
	if(b_busy) return;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(NULL, &selected_indeces, &selected_index_type);
	if(rpn_mode && selected_indeces.size() == 1 && expression_is_empty() && nr_of_new_expressions > 0) {
		selected_indeces.insert(selected_indeces.begin(), nr_of_new_expressions);
		selected_index_type.insert(selected_index_type.begin(), INDEX_TYPE_ANS);
	}
	if(selected_indeces.empty()) {
		if(!evalops.parse_options.rpn) {
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
			if(evalops.parse_options.rpn) str += ' ';
			else str += str_sign;
		}
	}
	
	for(size_t i = 0; i < selected_indeces.size(); i++) {
		if(i > 0) {
			if(evalops.parse_options.rpn) str += ' ';
			else str += str_sign;
		}
		if(selected_index_type[i] == INDEX_TYPE_TXT) {
			int index = selected_indeces[i];
			if(index > 0 && inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION) index--;
			string search_s = CALCULATOR->getDecimalPoint() + NUMBER_ELEMENTS;
			if((inhistory[index].length() >= 2 && inhistory[index][0] == '(' && inhistory[index][inhistory[index].length() - 1] == ')') || inhistory[index].find_first_not_of(search_s) == string::npos) {
				str += inhistory[index];
			} else {
				str += '(';
				str += inhistory[index];
				str += ')';
			}
		} else {
			const ExpressionName *ename = NULL;
			if(selected_index_type[i] == INDEX_TYPE_XPR) ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
			else ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
			str += ename->name;
			str += '(';
			if(evalops.parse_options.base != BASE_DECIMAL) {
				Number nr(selected_indeces[i], 1);
				PrintOptions po;
				po.base = evalops.parse_options.base;
				po.base_display = BASE_DISPLAY_NONE;
				str += nr.print(po);
			} else {
				str += i2s(selected_indeces[i]);
			}
			str += ')';
		}
	}
	if(only_one_value && !evalops.parse_options.rpn) {
		str += str_sign;
	}
	if(evalops.parse_options.rpn) {
		str += ' ';
		if(selected_indeces.size() == 1) {
			str += str_sign;
		} else {
			for(size_t i = 0; i < selected_indeces.size() - 1; i++) {
				str += str_sign;
			}
		}
	}
	add_to_undo = false;
	gtk_text_buffer_set_text(expressionbuffer, "", -1);
	add_to_undo = true;
	insert_text(str.c_str());
	if(!only_one_value) execute_expression();

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
	if(b_busy) return;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(NULL, &selected_indeces, &selected_index_type);
	if(selected_indeces.empty()) {
		insertButtonFunction(CALCULATOR->f_sqrt);
		return;
	}	
	const ExpressionName *ename2 = &CALCULATOR->f_sqrt->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
	string str = ename2->name;
	str += "(";
	if(selected_index_type[0] == INDEX_TYPE_TXT) {
		int index = selected_indeces[0];
		if(index > 0 && inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION) index--;
		str += inhistory[index];
	} else {
		const ExpressionName *ename = NULL;
		if(selected_index_type[0] == INDEX_TYPE_XPR) ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
		else ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
		str += ename->name;
		str += "(";
		if(evalops.parse_options.base != BASE_DECIMAL) {
			Number nr(selected_indeces[0], 1);
			PrintOptions po;
			po.base = evalops.parse_options.base;
			po.base_display = BASE_DISPLAY_NONE;
			str += nr.print(po);
		} else {
			str += i2s(selected_indeces[0]);
		}
		str += ")";
	}
	str += ")";
	add_to_undo = false;
	gtk_text_buffer_set_text(expressionbuffer, "", -1);
	add_to_undo = true;
	insert_text(str.c_str());
	execute_expression();
}
void on_button_history_insert_value_clicked(GtkButton*, gpointer) {
	if(b_busy) return;
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
	if(selected_index_type[0] == INDEX_TYPE_XPR && (selected_indeces.size() == 1 || selected_index_type[1] == INDEX_TYPE_XPR)) ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
	else ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
	string str = ename->name;
	str += "(";
	for(size_t i = 0; i < selected_indeces.size(); i++) {
		if(selected_index_type[i] != INDEX_TYPE_TXT) {
			if(i > 0) {str += CALCULATOR->getComma(); str += ' ';}
			if(evalops.parse_options.base != BASE_DECIMAL) {
				Number nr(selected_indeces[i], 1);
				PrintOptions po;
				po.base = evalops.parse_options.base;
				po.base_display = BASE_DISPLAY_NONE;
				str += nr.print(po);
			} else {
				str += i2s(selected_indeces[i]);
			}
		}
	}
	str += ")";
	insert_text(str.c_str());
}
void on_button_history_insert_text_clicked(GtkButton*, gpointer) {
	if(b_busy) return;
	vector<size_t> selected_rows;
	process_history_selection(&selected_rows, NULL, NULL);
	if(selected_rows.empty()) return;
	int index = selected_rows[0];
	if(index > 0 && ((inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION && (inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) || inhistory_type[index] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[index] == QALCULATE_HISTORY_REGISTER_MOVED)) index--;
	insert_text(inhistory[index].c_str());
}
void history_copy(bool full_text) {
	if(b_busy) return;
	vector<size_t> selected_rows;
	process_history_selection(&selected_rows, NULL, NULL);
	if(selected_rows.empty()) return;
	if(!full_text && selected_rows.size() == 1) {
		int index = selected_rows[0];
		if(index > 0 && ((inhistory_type[index] == QALCULATE_HISTORY_TRANSFORMATION && (inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT || inhistory_type[index - 1] == QALCULATE_HISTORY_RESULT_APPROXIMATE)) || inhistory_type[index] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[index] == QALCULATE_HISTORY_REGISTER_MOVED)) index--;;
		gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), inhistory[index].c_str(), -1);
	} else {
		string str;
		int hindex = 0;
		for(size_t i = 0; i < selected_rows.size(); i++) {
			if(i > 0) str += '\n';
			hindex = selected_rows[i];
			on_button_history_copy_add_hindex:
			bool add_parse = false;
			switch(inhistory_type[hindex]) {
				case QALCULATE_HISTORY_EXPRESSION: {
					if(i > 0) str += '\n';
					str += inhistory[hindex];
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
					str += inhistory[hindex];
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
					if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) historyview)) {
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
					str += inhistory[hindex];
					break;
				}
				case QALCULATE_HISTORY_WARNING: {}
				case QALCULATE_HISTORY_ERROR: {}
				case QALCULATE_HISTORY_OLD: {
					str += inhistory[hindex];
					break;
				}
			}
			if(add_parse && hindex > 0 && (inhistory_type[hindex - 1] == QALCULATE_HISTORY_PARSE || inhistory_type[hindex - 1] == QALCULATE_HISTORY_PARSE_APPROXIMATE || inhistory_type[hindex - 1] == QALCULATE_HISTORY_PARSE_WITHEQUALS)) {
				hindex--;
				goto on_button_history_copy_add_hindex;
			}
		}
		gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), str.c_str(), -1);
	}
}
void on_button_history_copy_clicked(GtkButton*, gpointer) {
	history_copy(false);
}
void on_popup_menu_item_history_clear_activate(GtkMenuItem*, gpointer) {
	if(b_busy) return;
	gtk_list_store_clear(historystore);
	inhistory.clear();
	inhistory_type.clear();
	for(size_t i = 0; i < history_parsed.size(); i++) {
		if(history_parsed[i]) history_parsed[i]->unref();
		if(history_answer[i]) history_answer[i]->unref();
	}
	history_parsed.clear();
	history_answer.clear();
	current_inhistory_index = -1;
	history_index = 0;
	initial_inhistory_index = 0;
	nr_of_new_expressions = 0;
}
void on_popup_menu_item_history_insert_value_activate(GtkMenuItem*, gpointer) {
	on_button_history_insert_value_clicked(NULL, NULL);
}
void on_popup_menu_item_history_insert_text_activate(GtkMenuItem*, gpointer) {
	on_button_history_insert_text_clicked(NULL, NULL);
}
void on_popup_menu_item_history_copy_text_activate(GtkMenuItem*, gpointer) {
	history_copy(false);
}
void on_popup_menu_item_history_copy_full_text_activate(GtkMenuItem*, gpointer) {
	history_copy(true);
}
void update_historyview_popup() {
	GtkTreeIter iter;
	vector<size_t> selected_rows;
	vector<size_t> selected_indeces;
	vector<int> selected_index_type;
	process_history_selection(&selected_rows, &selected_indeces, &selected_index_type);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_value")), selected_indeces.size() > 0 && selected_index_type[0] != INDEX_TYPE_TXT && selected_index_type.back() != INDEX_TYPE_TXT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_insert_text")), selected_indeces.size() == 1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_text")), selected_indeces.size() == 1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_copy_full_text")), !selected_rows.empty());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_history_clear")), gtk_tree_model_get_iter_first(GTK_TREE_MODEL(historystore), &iter));
}
gboolean on_historyview_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	GtkTreePath *path = NULL;
	GtkTreeSelection *select = NULL;
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		if(b_busy) return TRUE;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(historyview), event->x, event->y, &path, NULL, NULL, NULL)) {
			select = gtk_tree_view_get_selection(GTK_TREE_VIEW(historyview));
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
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_historyview_popup_menu(GtkWidget*, gpointer) {
	if(b_busy) return TRUE;
	update_historyview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_historyview")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}
void on_historyview_selection_changed(GtkTreeSelection*, gpointer) {
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
					ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
				} else {
					insert_text(inhistory[(size_t) hindex - 1].c_str());
					return;
				}
				break;
			}
			case QALCULATE_HISTORY_PARSE: {}
			case QALCULATE_HISTORY_PARSE_APPROXIMATE: {}
			case QALCULATE_HISTORY_EXPRESSION: {
				if(column == history_index_column) {
					ename = &f_expression->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
				} else {
					insert_text(inhistory[(size_t) hindex].c_str());
					return;
				}
				break;
			}
			default: {
				ename = &f_answer->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
			}
		}		
		string str = ename->name;
		str += "(";
		if(evalops.parse_options.base != BASE_DECIMAL) {
			Number nr(index, 1);
			PrintOptions po;
			po.base = evalops.parse_options.base;
			po.base_display = BASE_DISPLAY_NONE;
			str += nr.print(po);
		} else {
			str += i2s(index);
		}
		str += ")";
		insert_text(str.c_str());
	} else if(hindex >= 0) {
		if(hindex > 0 && (inhistory_type[hindex] == QALCULATE_HISTORY_TRANSFORMATION || inhistory_type[hindex] == QALCULATE_HISTORY_RPN_OPERATION || inhistory_type[hindex] == QALCULATE_HISTORY_REGISTER_MOVED)) hindex--;
		if(inhistory_type[hindex] != QALCULATE_HISTORY_WARNING && inhistory_type[hindex] != QALCULATE_HISTORY_ERROR) {
			insert_text(inhistory[(size_t) hindex].c_str());
		}
	}
}

void on_menu_item_manage_variables_activate(GtkMenuItem*, gpointer) {
	manage_variables();
}
void on_menu_item_manage_functions_activate(GtkMenuItem*, gpointer) {
	manage_functions();
}
void on_menu_item_manage_units_activate(GtkMenuItem*, gpointer) {
	manage_units();
}

void on_menu_item_datasets_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = get_datasets_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));	
}

void on_menu_item_import_csv_file_activate(GtkMenuItem*, gpointer) {
	import_csv_file(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

void on_menu_item_export_csv_file_activate(GtkMenuItem*, gpointer) {
	export_csv_file(NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

void on_expander_convert_activate(GtkExpander *o, gpointer) {
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_unit"))); 
}
void on_menu_item_convert_to_unit_expression_activate(GtkMenuItem*, gpointer) {
	gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_unit"))); 
	/*GtkWidget *dialog = get_unit_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	if(gtk_widget_get_visible(dialog)) {
		gtk_window_present(GTK_WINDOW(dialog));
	} else {
		gtk_widget_show(dialog);
	}*/
}
void on_menu_item_convert_to_best_unit_activate(GtkMenuItem*, gpointer) {
	executeCommand(COMMAND_CONVERT_OPTIMAL);
}
void on_menu_item_convert_to_base_units_activate(GtkMenuItem*, gpointer) {
	executeCommand(COMMAND_CONVERT_BASE);
}

void on_menu_item_set_prefix_activate(GtkMenuItem*, gpointer user_data) {
	result_prefix_changed((Prefix*) user_data);
	focus_keeping_selection();
}

void on_menu_item_insert_matrix_activate(GtkMenuItem*, gpointer) {
	string str = get_selected_expression_text();
	remove_blank_ends(str);
	if(!str.empty()) {
		MathStructure mstruct_sel;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_sel, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct_sel.isMatrix() && mstruct_sel[0].size() > 0) {
			insert_matrix(&mstruct_sel, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), false);
			return;
		}
	}
	insert_matrix(NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), false);
}
void on_menu_item_insert_vector_activate(GtkMenuItem*, gpointer) {
	string str = get_selected_expression_text();
	remove_blank_ends(str);
	if(!str.empty()) {
		MathStructure mstruct_sel;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_sel, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct_sel.isVector() && !mstruct_sel.isMatrix()) {
			insert_matrix(&mstruct_sel, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), true);
			return;
		}
	}
	insert_matrix(NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), true);
}

void update_assumptions_items() {
	if(!block_expression_execution) {
		block_expression_execution = true;
		set_assumptions_items(CALCULATOR->defaultAssumptions()->type(), CALCULATOR->defaultAssumptions()->sign());
		block_expression_execution = false;
	}
}

void on_menu_item_assumptions_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_INTEGER);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_RATIONAL);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_REAL);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_COMPLEX);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NUMBER);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NONE);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NONMATRIX);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONZERO);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_POSITIVE);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NEGATIVE);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_UNKNOWN);
	update_assumptions_items();
	expression_calculation_updated();
}

void set_type(const char *var, AssumptionType at) {
	if(block_expression_execution) return;
	Variable *v = CALCULATOR->getActiveVariable(var);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	if(!uv->assumptions()) uv->setAssumptions(new Assumptions()); 
	uv->assumptions()->setType(at);
	expression_calculation_updated();
}
void set_sign(const char *var, AssumptionSign as) {
	if(block_expression_execution) return;
	Variable *v = CALCULATOR->getActiveVariable(var);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	if(!uv->assumptions()) uv->setAssumptions(new Assumptions()); 
	uv->assumptions()->setSign(as);
	expression_calculation_updated();
}
void reset_assumptions(const char *var) {
	Variable *v = CALCULATOR->getActiveVariable(var);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	uv->setAssumptions(NULL);
	expression_calculation_updated();
}

void set_x_assumptions_items() {
	Variable *v = CALCULATOR->getActiveVariable("x");
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	bool b_bak = block_expression_execution;
	block_expression_execution = true;
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	switch(ass->sign()) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_unknown")), TRUE);}
	}
	switch(ass->type()) {
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_x_none")), TRUE);}
	}
	block_expression_execution = b_bak;
}
void on_mb_x_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	set_x_assumptions_items();
}

void on_menu_item_x_default_activate() {
	reset_assumptions("x");
}
void on_menu_item_x_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_INTEGER);
}
void on_menu_item_x_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_RATIONAL);
}
void on_menu_item_x_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_REAL);
}
void on_menu_item_x_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_COMPLEX);
}
void on_menu_item_x_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_NUMBER);
}
void on_menu_item_x_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_NONE);
}
void on_menu_item_x_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("x", ASSUMPTION_TYPE_NONMATRIX);
}
void on_menu_item_x_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NONZERO);
}
void on_menu_item_x_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_POSITIVE);
}
void on_menu_item_x_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NONNEGATIVE);
}
void on_menu_item_x_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NEGATIVE);
}
void on_menu_item_x_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_NONPOSITIVE);
}
void on_menu_item_x_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("x", ASSUMPTION_SIGN_UNKNOWN);
}

void set_y_assumptions_items() {
	Variable *v = CALCULATOR->getActiveVariable("y");
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	bool b_bak = block_expression_execution;
	block_expression_execution = true;
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	switch(ass->sign()) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_unknown")), TRUE);}
	}
	switch(ass->type()) {
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_y_none")), TRUE);}
	}
	block_expression_execution = b_bak;
}
void on_mb_y_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	set_y_assumptions_items();
}

void on_menu_item_y_default_activate() {
	reset_assumptions("y");
}
void on_menu_item_y_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_INTEGER);
}
void on_menu_item_y_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_RATIONAL);
}
void on_menu_item_y_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_REAL);
}
void on_menu_item_y_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_COMPLEX);
}
void on_menu_item_y_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_NUMBER);
}
void on_menu_item_y_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_NONE);
}
void on_menu_item_y_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("y", ASSUMPTION_TYPE_NONMATRIX);
}
void on_menu_item_y_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NONZERO);
}
void on_menu_item_y_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_POSITIVE);
}
void on_menu_item_y_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NONNEGATIVE);
}
void on_menu_item_y_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NEGATIVE);
}
void on_menu_item_y_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_NONPOSITIVE);
}
void on_menu_item_y_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("y", ASSUMPTION_SIGN_UNKNOWN);
}

void set_z_assumptions_items() {
	Variable *v = CALCULATOR->getActiveVariable("z");
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	bool b_bak = block_expression_execution;
	block_expression_execution = true;
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	switch(ass->sign()) {
		case ASSUMPTION_SIGN_POSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_positive")), TRUE); break;}
		case ASSUMPTION_SIGN_NONPOSITIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonpositive")), TRUE); break;}
		case ASSUMPTION_SIGN_NEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_negative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONNEGATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonnegative")), TRUE); break;}
		case ASSUMPTION_SIGN_NONZERO: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonzero")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_unknown")), TRUE);}
	}
	switch(ass->type()) {
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_z_none")), TRUE);}
	}
	block_expression_execution = b_bak;
}
void on_mb_z_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	set_z_assumptions_items();
}

void on_menu_item_z_default_activate() {
	reset_assumptions("z");
}
void on_menu_item_z_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_INTEGER);
}
void on_menu_item_z_rational_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_RATIONAL);
}
void on_menu_item_z_real_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_REAL);
}
void on_menu_item_z_complex_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_COMPLEX);
}
void on_menu_item_z_number_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_NUMBER);
}
void on_menu_item_z_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_NONE);
}
void on_menu_item_z_nonmatrix_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_type("z", ASSUMPTION_TYPE_NONMATRIX);
}
void on_menu_item_z_nonzero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NONZERO);
}
void on_menu_item_z_positive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_POSITIVE);
}
void on_menu_item_z_nonnegative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NONNEGATIVE);
}
void on_menu_item_z_negative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NEGATIVE);
}
void on_menu_item_z_nonpositive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_NONPOSITIVE);
}
void on_menu_item_z_unknown_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_sign("z", ASSUMPTION_SIGN_UNKNOWN);
}

void menu_to_bin(GtkMenuItem*, gpointer) {
	int save_base = printops.base;
	printops.base = BASE_BINARY;
	result_format_updated();
	printops.base = save_base;
}
void menu_to_oct(GtkMenuItem*, gpointer) {
	int save_base = printops.base;
	printops.base = BASE_OCTAL;
	result_format_updated();
	printops.base = save_base;
}
void menu_to_hex(GtkMenuItem*, gpointer) {
	int save_base = printops.base;
	printops.base = BASE_HEXADECIMAL;
	result_format_updated();
	printops.base = save_base;
}
void menu_to_fraction(GtkMenuItem*, gpointer) {
	NumberFractionFormat save_format = printops.number_fraction_format;
	if(mstruct && mstruct->isNumber()) printops.number_fraction_format = FRACTION_COMBINED;
	else printops.number_fraction_format = FRACTION_FRACTIONAL;
	result_format_updated();
	printops.number_fraction_format = save_format;
}
void on_mb_to_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	GtkWidget *sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_to"));
	if(expression_has_changed && !rpn_mode) execute_expression(true);
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	MENU_ITEM(_("Base units"), on_menu_item_convert_to_base_units_activate);
	MENU_ITEM(_("Optimal unit"), on_menu_item_convert_to_best_unit_activate);
	MENU_ITEM(_("Optimal prefix"), on_menu_item_set_prefix_activate);
	MENU_SEPARATOR
	if(!mstruct || !displayed_mstruct || !mstruct->containsType(STRUCT_UNIT, true)) {
		MENU_ITEM(_("Number bases"), on_menu_item_convert_number_bases_activate)
		MENU_ITEM(_("Binary"), menu_to_bin)
		MENU_ITEM(_("Octal"), menu_to_oct)
		MENU_ITEM(_("Hexadecimal"), menu_to_hex)
		MENU_ITEM(_("Factors"), on_menu_item_factorize_activate)
		MENU_ITEM(_("Fraction"), menu_to_fraction)
		return;
	}
	string s_cat;
	Unit *u = CALCULATOR->findMatchingUnit(*mstruct);
	if(u) s_cat = u->category();
	vector<Unit*> to_us;
	size_t i_added = 0;
	if(!s_cat.empty()) {
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i]->category() == s_cat) {
				u = CALCULATOR->units[i];
				if(u->isActive() && !u->isHidden()) {
					bool b = false;
					for(size_t i2 = 0; i2 < to_us.size(); i2++) {
						if(u->title(true) < to_us[i2]->title(true)) {
							to_us.insert(to_us.begin() + i2, u);
							b = true;
							break;
						}
					}
					if(!b) to_us.push_back(u);
					i_added++;
				}
			}
		}
		for(size_t i = 0; i < to_us.size(); i++) {
			if((i + 4) % 9 == 0 && i + 1 != to_us.size()) {SUBMENU_ITEM(_("more"), sub);}
			MENU_ITEM_WITH_POINTER(to_us[i]->title(true).c_str(), convert_to_unit, to_us[i])
			// Show further items in a submenu
		}
	}
	if(i_added + 3 < 9) {
		const char *si_units[] = {"m", "g", "s", "A", "K", "L"};
		vector<Unit*> to_us2;
		for(size_t i = 0; i < 6 && i_added + 3 < 9; i++) {
			Unit * u = CALCULATOR->getActiveUnit(si_units[i]);
			if(u && !u->isHidden()) {
				bool b = false;
				for(size_t i2 = 0; i2 < to_us.size(); i2++) {
					if(u == to_us[i2]) {
						b = true;
						break;
					}
				}
				for(size_t i2 = 0; !b && i2 < to_us2.size(); i2++) {
					if(u->title(true) < to_us2[i2]->title(true)) {
						to_us2.insert(to_us2.begin() + i2, u);
						b = true;
						break;
					}
				}
				if(!b) {
					to_us2.push_back(u);
					i_added++;
				}
			}
		}
		for(size_t i = 0; i < to_us2.size(); i++) {
			MENU_ITEM_WITH_POINTER(to_us2[i]->title(true).c_str(), convert_to_unit, to_us2[i])
		}
	}
}

void on_mb_units_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_units"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	const char *si_units[] = {"m", "g", "s", "A", "K"};
	size_t i_added = 0;
	for(size_t i = 0; i < recent_units.size(); i++) {
		if(!recent_units[i]->isLocal() && CALCULATOR->stillHasUnit(recent_units[i])) {
			MENU_ITEM_WITH_POINTER(recent_units[i]->title(true).c_str(), insert_unit, recent_units[i])
			i_added++;
		}
	}
	for(size_t i = 0; i_added < 5 && i < 5; i++) {
		Unit * u = CALCULATOR->getActiveUnit(si_units[i]);
		if(u && !u->isHidden()) {
			MENU_ITEM_WITH_POINTER(u->title(true).c_str(), insert_unit, u)
			i_added++;
		}
	}
	
	MENU_SEPARATOR
	Prefix *p = CALCULATOR->getPrefix("giga");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix, p);}
	p = CALCULATOR->getPrefix("mega");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix, p);}
	p = CALCULATOR->getPrefix("kilo");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix, p);}
	p = CALCULATOR->getPrefix("milli");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix, p);}
	p = CALCULATOR->getPrefix("micro");
	if(p) {MENU_ITEM_WITH_POINTER(p->longName(true, printops.use_unicode_signs).c_str(), insert_prefix, p);}
}

void on_popup_menu_fx_edit_activate(GtkMenuItem *w, gpointer data) {
	edit_function_simple("", (MathFunction*) data, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_popup_menu_fx_delete_activate(GtkMenuItem *w, gpointer data) {
	// For some reason a dialog is required to close/update the menu with the deleted function
	if(ask_question(_("Are you sure you want to delete the function?"), GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")))) {
		delete_function((MathFunction*) data);
	}
}

gulong on_popup_menu_fx_edit_activate_handler = 0, on_popup_menu_fx_delete_activate_handler = 0;

gboolean on_menu_fx_popup_menu(GtkWidget*, gpointer data) {
	if(b_busy) return TRUE;
	if(on_popup_menu_fx_edit_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_fx_edit"), on_popup_menu_fx_edit_activate_handler);
	if(on_popup_menu_fx_delete_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_fx_delete"), on_popup_menu_fx_delete_activate_handler);
	on_popup_menu_fx_edit_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_fx_edit"), "activate", G_CALLBACK(on_popup_menu_fx_edit_activate), data);
	on_popup_menu_fx_delete_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_fx_delete"), "activate", G_CALLBACK(on_popup_menu_fx_delete_activate), data);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_fx")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_fx")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_menu_fx_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	/* Ignore double-clicks and triple-clicks */
	if(gdk_event_triggers_context_menu((GdkEvent *) event) && event->type == GDK_BUTTON_PRESS) {
		on_menu_fx_popup_menu(widget, data);
		return TRUE;
	}
	return FALSE;
}

void on_mb_fx_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_fx"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	bool b = false;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(CALCULATOR->functions[i]->isLocal() && !CALCULATOR->functions[i]->isBuiltin() && CALCULATOR->functions[i]->isActive() && !CALCULATOR->functions[i]->isHidden()) {
			MENU_ITEM_WITH_POINTER(CALCULATOR->functions[i]->title(true).c_str(), insert_button_function, CALCULATOR->functions[i])
			g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_fx_button_press), CALCULATOR->functions[i]);
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_fx_popup_menu), (gpointer) CALCULATOR->functions[i]);
			b = true;
		}
	}
	bool b2 = false;
	for(size_t i = 0; i < recent_functions.size(); i++) {
		if(!recent_functions[i]->isLocal() && CALCULATOR->stillHasFunction(recent_functions[i])) {
			if(!b2 && b) {MENU_SEPARATOR}
			b2 = true;
			MENU_ITEM_WITH_POINTER(recent_functions[i]->title(true).c_str(), insert_button_function_save, recent_functions[i])
		}
	}
	if(b2 || b) {MENU_SEPARATOR}
	MENU_ITEM(_("All functions"), on_menu_item_manage_functions_activate);
}


void on_mb_pi_toggled(GtkToggleButton *w, gpointer) {
	
	if(!gtk_toggle_button_get_active(w)) return;
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_pi"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	
	MENU_ITEM_WITH_POINTER(CALCULATOR->v_e->title(true).c_str(), insert_button_variable, CALCULATOR->v_e)
	Variable *v = CALCULATOR->getActiveVariable("euler");
	if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v);}
	v = CALCULATOR->getActiveVariable("golden");
	if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v);}
	MENU_SEPARATOR

	int i_added = 0;
	for(size_t i = 0; i < recent_variables.size(); i++) {
		if(!recent_variables[i]->isLocal() && CALCULATOR->stillHasVariable(recent_variables[i])) {
			MENU_ITEM_WITH_POINTER(recent_variables[i]->title(true).c_str(), insert_variable, recent_variables[i])
			i_added++;
		}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("c");
		if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("newtonian_constant");
		if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("planck");
		if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("boltzmann");
		if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v); i_added++;}
	}
	if(i_added < 5)	{
		v = CALCULATOR->getActiveVariable("avogadro");
		if(v) {MENU_ITEM_WITH_POINTER(v->title(true).c_str(), insert_button_variable, v); i_added++;}
	}
	
	MENU_SEPARATOR
	MENU_ITEM(_("All variables"), on_menu_item_manage_variables_activate);
	
}

void on_popup_menu_sto_set_activate(GtkMenuItem *w, gpointer data) {
	((KnownVariable*) data)->set(*mstruct);
}
void on_popup_menu_sto_edit_activate(GtkMenuItem *w, gpointer data) {
	edit_variable("", (Variable*) data, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_popup_menu_sto_delete_activate(GtkMenuItem *w, gpointer data) {
	// For some reason a dialog is required to close/update the menu with the deleted variable
	if(ask_question(_("Are you sure you want to delete the variable?"), GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")))) {
		delete_variable((Variable*) data);
	}
}

gulong on_popup_menu_sto_set_activate_handler = 0, on_popup_menu_sto_edit_activate_handler = 0, on_popup_menu_sto_delete_activate_handler = 0;

gboolean on_menu_sto_popup_menu(GtkWidget*, gpointer data) {
	if(b_busy) return TRUE;
	if(on_popup_menu_sto_set_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_set"), on_popup_menu_sto_set_activate_handler);
	if(on_popup_menu_sto_edit_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_edit"), on_popup_menu_sto_edit_activate_handler);
	if(on_popup_menu_sto_delete_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_sto_delete"), on_popup_menu_sto_delete_activate_handler);
	if(((Variable*) data)->isKnown() && mstruct && displayed_mstruct) on_popup_menu_sto_set_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_set"), "activate", G_CALLBACK(on_popup_menu_sto_set_activate), data);
	on_popup_menu_sto_edit_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_edit"), "activate", G_CALLBACK(on_popup_menu_sto_edit_activate), data);
	on_popup_menu_sto_delete_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_sto_delete"), "activate", G_CALLBACK(on_popup_menu_sto_delete_activate), data);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_sto")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_sto")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_menu_sto_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	/* Ignore double-clicks and triple-clicks */
	if(gdk_event_triggers_context_menu((GdkEvent *) event) && event->type == GDK_BUTTON_PRESS) {
		on_menu_sto_popup_menu(widget, data);
		return TRUE;
	}
	return FALSE;
}

void on_mb_sto_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) return;
	GtkMenu *sub = GTK_MENU(gtk_builder_get_object(main_builder, "menu_sto"));
	GtkWidget *item;
	GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
	for(GList *l = list; l != NULL; l = l->next) {
		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(list);
	bool b = false;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isLocal() && !CALCULATOR->variables[i]->isBuiltin() && CALCULATOR->variables[i]->isActive() && !CALCULATOR->variables[i]->isHidden()) {
			MENU_ITEM_WITH_POINTER(CALCULATOR->variables[i]->title(true).c_str(), insert_button_variable, CALCULATOR->variables[i])
			g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_sto_button_press), CALCULATOR->variables[i]);
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_sto_popup_menu), (gpointer) CALCULATOR->variables[i]);
			b = true;
		}
	}
	if(!b) {MENU_NO_ITEMS(_("No items found"))}
}


void on_menu_item_enable_variables_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.variables_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(evalops.parse_options.variables_enabled);
}
void on_menu_item_enable_functions_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.functions_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(evalops.parse_options.functions_enabled);
}
void on_menu_item_enable_units_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.units_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(evalops.parse_options.units_enabled);
}
void on_menu_item_enable_unknown_variables_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.unknowns_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(evalops.parse_options.unknowns_enabled);
}
void on_menu_item_calculate_variables_activate(GtkMenuItem *w, gpointer) {
	evalops.calculate_variables = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_calculation_updated();
}
void on_menu_item_allow_complex_activate(GtkMenuItem *w, gpointer) {
	evalops.allow_complex = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_calculation_updated();
}
void on_menu_item_allow_infinite_activate(GtkMenuItem *w, gpointer) {
	evalops.allow_infinite = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_calculation_updated();
}
void on_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer) {
	evalops.assume_denominators_nonzero = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_calculation_updated();
}
void on_menu_item_warn_about_denominators_assumed_nonzero_activate(GtkMenuItem *w, gpointer) {
	evalops.warn_about_denominators_assumed_nonzero = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	if(evalops.warn_about_denominators_assumed_nonzero) expression_calculation_updated();
}
void on_menu_item_algebraic_mode_simplify_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.structuring = STRUCTURING_SIMPLIFY;
	printops.allow_factorization = false;
	expression_calculation_updated();
}
void on_menu_item_algebraic_mode_factorize_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.structuring = STRUCTURING_FACTORIZE;
	printops.allow_factorization = true;
	expression_calculation_updated();
}
void on_menu_item_algebraic_mode_hybrid_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.structuring = STRUCTURING_HYBRID;
	printops.allow_factorization = false;
	expression_calculation_updated();
}
void on_menu_item_read_precision_activate(GtkMenuItem *w, gpointer) {
	 if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) evalops.parse_options.read_precision = READ_PRECISION_WHEN_DECIMALS;
	 else evalops.parse_options.read_precision = DONT_READ_PRECISION;
	 expression_format_updated(true);
}
void on_menu_item_new_unknown_activate(GtkMenuItem*, gpointer) {
	edit_unknown(_("My Variables"), NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_menu_item_new_variable_activate(GtkMenuItem*, gpointer) {
	edit_variable(_("My Variables"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_menu_item_new_matrix_activate(GtkMenuItem*, gpointer) {
	edit_matrix(_("Matrices"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), FALSE);
}
void on_menu_item_new_vector_activate(GtkMenuItem*, gpointer) {
	edit_matrix(_("Vectors"), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), TRUE);
}
void on_menu_item_new_function_activate(GtkMenuItem*, gpointer) {
	edit_function("", NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_menu_item_new_function_simple_activate(GtkMenuItem*, gpointer) {
	edit_function_simple("", NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_menu_item_new_dataset_activate(GtkMenuItem*, gpointer) {
	edit_dataset(NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}
void on_menu_item_new_unit_activate(GtkMenuItem*, gpointer) {
	edit_unit("", NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
}

void on_menu_item_rpn_mode_activate(GtkMenuItem *w, gpointer) {
	set_rpn_mode(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_rpn_syntax_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.rpn = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(false);
}
void on_menu_item_limit_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.limit_implicit_multiplication = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	printops.limit_implicit_multiplication = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(true);
	result_format_updated();
}
void on_menu_item_adaptive_parsing_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	expression_format_updated(true);
}
void on_menu_item_ignore_whitespace_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.parse_options.parsing_mode = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
	expression_format_updated(true);
}
void on_menu_item_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.parse_options.parsing_mode = PARSING_MODE_CONVENTIONAL;
	expression_format_updated(true);
}

void on_menu_item_fetch_exchange_rates_activate(GtkMenuItem*, gpointer) {
	do_timeout = false;
	fetch_exchange_rates(15);
	CALCULATOR->loadExchangeRates();
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
	do_timeout = true;
	expression_calculation_updated();
}
void on_menu_item_save_defs_activate(GtkMenuItem*, gpointer) {
	save_defs();
}
void on_menu_item_save_mode_activate(GtkMenuItem*, gpointer) {
	save_mode();
}
void on_menu_item_edit_prefs_activate(GtkMenuItem*, gpointer) {
	edit_preferences();
}
void on_menu_item_degrees_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_DEGREES;
		set_angle_button();
		expression_format_updated(true);
	}
}
void on_menu_item_radians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
		set_angle_button();
		expression_format_updated(true);
	}
}
void on_menu_item_gradians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_GRADIANS;
		set_angle_button();
		expression_format_updated(true);
	}
}
void on_menu_item_no_default_angle_unit_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		evalops.parse_options.angle_unit = ANGLE_UNIT_NONE;
		set_angle_button();
		expression_format_updated(true);
	}
}

void set_output_base_from_dialog(int base) {
	bool b = (printops.base == base);
	printops.base = base;
	switch(printops.base) {
		case BASE_BINARY: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_binary_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_binary")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_binary_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 0);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		case BASE_OCTAL: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_octal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_octal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_octal_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 1);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		case BASE_DECIMAL: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_decimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_decimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_decimal_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 2);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		case BASE_HEXADECIMAL: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 3);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		case BASE_SEXAGESIMAL: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sexagesimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 4);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		case BASE_TIME: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_time_format"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_time_format")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_time_format"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 5);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_roman")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 6);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
		default: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_custom_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_base_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_custom_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_base_activate, NULL);
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 7);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
			break;
		}
	}
	if(!b) result_format_updated();
}
void output_base_updated_from_menu() {
	if(setbase_builder) {
		switch(printops.base) {
			case BASE_BINARY: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_binary_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_binary_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			case BASE_OCTAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_octal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_octal_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			case BASE_DECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_decimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_decimal_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			case BASE_HEXADECIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_hexadecimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_hexadecimal_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			case BASE_SEXAGESIMAL: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_sexagesimal_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_sexagesimal_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			case BASE_TIME: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_time_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_time_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_roman_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_roman_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
				break;
			}
			default: {
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_radiobutton_output_other_toggled, NULL);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), TRUE);
				g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_spinbutton_output_other_value_changed, NULL);
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), printops.base);
				g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_set_base_spinbutton_output_other_value_changed, NULL);
			}
		}
	}
}


void on_menu_item_binary_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.base = BASE_BINARY;
	result_format_updated();
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 0);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
}
void on_menu_item_octal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.base = BASE_OCTAL;
	result_format_updated();
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 1);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
}
void on_menu_item_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.base = BASE_DECIMAL;
	result_format_updated();
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 2);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
}
void on_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.base = BASE_HEXADECIMAL;
	result_format_updated();
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 3);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
}
void on_menu_item_custom_base_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	GtkWidget *dialog = get_set_base_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")));
}
void on_menu_item_roman_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.base = BASE_ROMAN_NUMERALS;
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 6);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	result_format_updated();
}
void on_menu_item_sexagesimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.base = BASE_SEXAGESIMAL;
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 4);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	result_format_updated();
}
void on_menu_item_time_format_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.base = BASE_TIME;
	output_base_updated_from_menu();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_base")), 5);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_base_changed, NULL);
	result_format_updated();
}
void on_set_base_spinbutton_output_other_value_changed(GtkSpinButton *w, gpointer) {
	set_output_base_from_dialog(gtk_spin_button_get_value_as_int(w));
}
void on_set_base_radiobutton_output_binary_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_BINARY);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_octal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_OCTAL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_decimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_DECIMAL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_hexadecimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_HEXADECIMAL);	
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_sexagesimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_SEXAGESIMAL);	
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_time_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_TIME);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_roman_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(BASE_ROMAN_NUMERALS);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), FALSE);
}
void on_set_base_radiobutton_output_other_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	set_output_base_from_dialog(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other"))));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_output_other")), TRUE);
}
void on_menu_item_set_base_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = get_set_base_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
}
void on_set_base_radiobutton_input_binary_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	evalops.parse_options.base = BASE_BINARY;
	expression_format_updated(true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), FALSE);
	on_historyview_selection_changed(NULL, NULL);
}
void on_set_base_radiobutton_input_octal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	evalops.parse_options.base = BASE_OCTAL;
	expression_format_updated(true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), FALSE);
	on_historyview_selection_changed(NULL, NULL);
}
void on_set_base_radiobutton_input_decimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	evalops.parse_options.base = BASE_DECIMAL;
	expression_format_updated(true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), FALSE);
	on_historyview_selection_changed(NULL, NULL);
}
void on_set_base_radiobutton_input_hexadecimal_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	evalops.parse_options.base = BASE_HEXADECIMAL;
	expression_format_updated(true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), FALSE);
	on_historyview_selection_changed(NULL, NULL);
}
void on_set_base_radiobutton_input_other_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	evalops.parse_options.base = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")));
	expression_format_updated(true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), TRUE);
	on_historyview_selection_changed(NULL, NULL);
}
void on_set_base_spinbutton_input_other_value_changed(GtkSpinButton *w, gpointer) {
	evalops.parse_options.base = gtk_spin_button_get_value_as_int(w);
	expression_format_updated(true);
	on_historyview_selection_changed(NULL, NULL);
}
void on_set_base_radiobutton_input_roman_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
	evalops.parse_options.base = BASE_ROMAN_NUMERALS;
	expression_format_updated(true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), FALSE);
	on_historyview_selection_changed(NULL, NULL);
}
void on_menu_item_abbreviate_names_activate(GtkMenuItem *w, gpointer) {
	printops.abbreviate_names = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_all_prefixes_activate(GtkMenuItem *w, gpointer) {
	printops.use_all_prefixes = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_denominator_prefixes_activate(GtkMenuItem *w, gpointer) {
	printops.use_denominator_prefix = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_place_units_separately_activate(GtkMenuItem *w, gpointer) {
	printops.place_units_separately = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_post_conversion_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.auto_post_conversion = POST_CONVERSION_NONE;
	expression_calculation_updated();
}
void on_menu_item_post_conversion_base_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.auto_post_conversion = POST_CONVERSION_BASE;
	expression_calculation_updated();
}
void on_menu_item_post_conversion_optimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL;
	expression_calculation_updated();
}
void on_menu_item_post_conversion_optimal_si_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
	expression_calculation_updated();
}
void on_menu_item_mixed_units_conversion_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	else evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
	expression_calculation_updated();
}
void on_menu_item_factorize_activate(GtkMenuItem*, gpointer) {
	executeCommand(COMMAND_FACTORIZE);
}
void on_menu_item_simplify_activate(GtkMenuItem*, gpointer) {
	executeCommand(COMMAND_SIMPLIFY);
}
void convert_number_bases(const gchar *initial_expression) {
	changing_in_nbases_dialog = false;
	GtkWidget *dialog = get_nbases_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	switch(evalops.parse_options.base) {
		case BASE_BINARY: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_binary")), initial_expression);
			break;
		}
		case BASE_OCTAL: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_octal")), initial_expression);
			break;
		}
		case BASE_HEXADECIMAL: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal")), initial_expression);
			break;
		}
		default: {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal")), initial_expression);
		}
	}	
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
}
void on_menu_item_convert_number_bases_activate(GtkMenuItem*, gpointer) {
	if(!result_text.empty()) return convert_number_bases(result_text.c_str());
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	convert_number_bases(str.c_str());
}
void show_percentage_dialog(const gchar *initial_expression) {
	GtkWidget *dialog = get_percentage_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	on_percentage_button_clear_clicked(NULL, NULL);
	if(strlen(initial_expression) > 0 && strcmp(initial_expression, "0") != 0) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_1")), initial_expression);
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
}
void on_menu_item_show_percentage_dialog_activate(GtkMenuItem*, gpointer) {
	if(!result_text.empty()) return show_percentage_dialog(result_text.c_str());
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	show_percentage_dialog(str.c_str());
}
void on_menu_item_periodic_table_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = get_periodic_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
}
void on_menu_item_plot_functions_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = get_plot_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), get_selected_expression_text(true).c_str());
	if(!gtk_widget_get_visible(dialog)) {
		gtk_list_store_clear(tPlotFunctions_store);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_remove")), FALSE);	

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_grid")), default_plot_display_grid);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_full_border")), default_plot_full_border);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), default_plot_rows);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_color")), default_plot_color);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_mono")), !default_plot_color);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_min")), default_plot_min.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_max")), default_plot_max.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_step")), default_plot_step.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")), default_plot_variable.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_steps")), default_plot_use_sampling_rate);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_step")), !default_plot_use_sampling_rate);
		switch(default_plot_type) {
			case 1: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")), TRUE); break;}
			case 2: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")), TRUE); break;}
			default: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_function")), TRUE); break;}
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), default_plot_type == 1 || default_plot_type == 2);	
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), default_plot_type != 1 && default_plot_type != 2);	
		switch(default_plot_legend_placement) {
			case PLOT_LEGEND_NONE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_NONE); break;}
			case PLOT_LEGEND_TOP_LEFT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_TOP_LEFT); break;}
			case PLOT_LEGEND_TOP_RIGHT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_TOP_RIGHT); break;}
			case PLOT_LEGEND_BOTTOM_LEFT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_BOTTOM_LEFT); break;}
			case PLOT_LEGEND_BOTTOM_RIGHT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_BOTTOM_RIGHT); break;}
			case PLOT_LEGEND_BELOW: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_BELOW); break;}
			case PLOT_LEGEND_OUTSIDE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_OUTSIDE); break;}
		}
		switch(default_plot_smoothing) {
			case PLOT_SMOOTHING_NONE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_NONE); break;}
			case PLOT_SMOOTHING_UNIQUE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_UNIQUE); break;}
			case PLOT_SMOOTHING_CSPLINES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_CSPLINES); break;}
			case PLOT_SMOOTHING_BEZIER: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_BEZIER); break;}
			case PLOT_SMOOTHING_SBEZIER: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_SBEZIER); break;}
		}
		switch(default_plot_style) {
			case PLOT_STYLE_LINES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_LINES); break;}
			case PLOT_STYLE_POINTS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_LINES); break;}
			case PLOT_STYLE_POINTS_LINES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_LINESPOINTS); break;}
			case PLOT_STYLE_BOXES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_BOXES); break;}
			case PLOT_STYLE_HISTOGRAM: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_HISTEPS); break;}
			case PLOT_STYLE_STEPS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_STEPS); break;}
			case PLOT_STYLE_CANDLESTICKS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_CANDLESTICKS); break;}
			case PLOT_STYLE_DOTS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_DOTS); break;}
		}
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")), default_plot_sampling_rate);
		
		gtk_widget_show(dialog);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(plot_builder, "plot_notebook")), 2);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(plot_builder, "plot_notebook")), 1);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(plot_builder, "plot_notebook")), 0);
	} else {
		gtk_window_present(GTK_WINDOW(dialog));
	}
}
void on_plot_dialog_hide(GtkWidget*, gpointer) {
	default_plot_display_grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_grid")));
	default_plot_full_border = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_full_border")));
	default_plot_rows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")));
	default_plot_color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_color")));
	default_plot_min = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_min")));
	default_plot_max = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_max")));
	default_plot_step = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_step")));
	default_plot_variable = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
	default_plot_use_sampling_rate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_steps")));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")))) {
		default_plot_type = 1;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")))) {
		default_plot_type = 2;
	} else {
		default_plot_type = 0;
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")))) {
		case PLOTLEGEND_MENU_NONE: {default_plot_legend_placement = PLOT_LEGEND_NONE; break;}
		case PLOTLEGEND_MENU_TOP_LEFT: {default_plot_legend_placement = PLOT_LEGEND_TOP_LEFT; break;}
		case PLOTLEGEND_MENU_TOP_RIGHT: {default_plot_legend_placement = PLOT_LEGEND_TOP_RIGHT; break;}
		case PLOTLEGEND_MENU_BOTTOM_LEFT: {default_plot_legend_placement = PLOT_LEGEND_BOTTOM_LEFT; break;}
		case PLOTLEGEND_MENU_BOTTOM_RIGHT: {default_plot_legend_placement = PLOT_LEGEND_BOTTOM_RIGHT; break;}
		case PLOTLEGEND_MENU_BELOW: {default_plot_legend_placement = PLOT_LEGEND_BELOW; break;}
		case PLOTLEGEND_MENU_OUTSIDE: {default_plot_legend_placement = PLOT_LEGEND_OUTSIDE; break;}
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")))) {
		case SMOOTHING_MENU_NONE: {default_plot_smoothing = PLOT_SMOOTHING_NONE; break;}
		case SMOOTHING_MENU_UNIQUE: {default_plot_smoothing = PLOT_SMOOTHING_UNIQUE; break;}
		case SMOOTHING_MENU_CSPLINES: {default_plot_smoothing = PLOT_SMOOTHING_CSPLINES; break;}
		case SMOOTHING_MENU_BEZIER: {default_plot_smoothing = PLOT_SMOOTHING_BEZIER; break;}
		case SMOOTHING_MENU_SBEZIER: {default_plot_smoothing = PLOT_SMOOTHING_SBEZIER; break;}
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")))) {
		case PLOTSTYLE_MENU_LINES: {default_plot_style = PLOT_STYLE_LINES; break;}
		case PLOTSTYLE_MENU_POINTS: {default_plot_style = PLOT_STYLE_POINTS; break;}
		case PLOTSTYLE_MENU_LINESPOINTS: {default_plot_style = PLOT_STYLE_POINTS_LINES; break;}
		case PLOTSTYLE_MENU_BOXES: {default_plot_style = PLOT_STYLE_BOXES; break;}
		case PLOTSTYLE_MENU_HISTEPS: {default_plot_style = PLOT_STYLE_HISTOGRAM; break;}
		case PLOTSTYLE_MENU_STEPS: {default_plot_style = PLOT_STYLE_STEPS; break;}
		case PLOTSTYLE_MENU_CANDLESTICKS: {default_plot_style = PLOT_STYLE_CANDLESTICKS; break;}
		case PLOTSTYLE_MENU_DOTS: {default_plot_style = PLOT_STYLE_DOTS; break;}
	}
	default_plot_sampling_rate = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")));
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	while(b) {
		MathStructure *y_vector, *x_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 7, &x_vector, 8, &y_vector, -1);
		if(y_vector) delete y_vector;
		if(x_vector) delete x_vector;
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), false);
	CALCULATOR->closeGnuplot();
}
void on_popup_menu_item_abort_activate(GtkMenuItem*, gpointer) {
	if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
	else if(b_busy_result) on_abort_display(NULL, 0, NULL);
	else if(b_busy_command) on_abort_command(NULL, 0, NULL);
}
void on_popup_menu_item_clear_activate(GtkMenuItem*, gpointer) {
	clear_expression_text();
	gtk_widget_grab_focus(expressiontext);
}
void on_popup_menu_item_display_normal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.negative_exponents = false;
	printops.sort_options.minus_last = true;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_normal")), TRUE);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), printops.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), printops.sort_options.minus_last);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
}
void on_popup_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assume_nonzero_denominators")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_engineering")), TRUE);
}
void on_popup_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.negative_exponents = true;
	printops.sort_options.minus_last = false;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_scientific")), TRUE);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), printops.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), printops.sort_options.minus_last);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
}
void on_popup_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.negative_exponents = true;
	printops.sort_options.minus_last = false;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific")), TRUE);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), printops.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), printops.sort_options.minus_last);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
}
void on_popup_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.negative_exponents = false;
	printops.sort_options.minus_last = true;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_non_scientific")), TRUE);

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), printops.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), printops.sort_options.minus_last);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
}
void on_popup_menu_item_display_no_prefixes_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_no_prefixes")), TRUE);
}
void on_popup_menu_item_display_prefixes_for_selected_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_selected_units")), TRUE);
}
void on_popup_menu_item_display_prefixes_for_all_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_all_units")), TRUE);
}
void on_popup_menu_item_display_prefixes_for_currencies_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_currencies")), TRUE);
}
void on_popup_menu_item_fraction_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
}
void on_popup_menu_item_fraction_decimal_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
}
void on_popup_menu_item_fraction_combined_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
}
void on_popup_menu_item_fraction_fraction_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
}
void on_popup_menu_item_binary_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_binary")), TRUE);
}
void on_popup_menu_item_roman_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_roman")), TRUE);
}
void on_popup_menu_item_sexagesimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sexagesimal")), TRUE);
}
void on_popup_menu_item_time_format_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_time_format")), TRUE);
}
void on_popup_menu_item_octal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_octal")), TRUE);
}
void on_popup_menu_item_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_decimal")), TRUE);
}
void on_popup_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
}
void on_popup_menu_item_custom_base_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")), TRUE);
}
void on_popup_menu_item_abbreviate_names_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_abbreviate_names")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_all_prefixes_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_all_prefixes")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_denominator_prefixes_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_denominator_prefixes")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_view_matrix_activate(GtkMenuItem*, gpointer) {
	insert_matrix(mstruct, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), false, false, true);
}
void on_popup_menu_item_view_vector_activate(GtkMenuItem*, gpointer) {
	insert_matrix(mstruct, GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")), true, false, true);
}

void on_menu_item_display_normal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.min_exp = EXP_PRECISION;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	result_format_updated();
}
void on_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.min_exp = EXP_BASE_3;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 1);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	result_format_updated();
}
void on_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.min_exp = EXP_SCIENTIFIC;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 2);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	result_format_updated();
}
void on_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.min_exp = EXP_PURE;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 3);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	result_format_updated();
}
void on_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.min_exp = EXP_NONE;
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 4);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_numerical_display"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_numerical_display_changed, NULL);
	result_format_updated();
}
void on_menu_item_display_no_prefixes_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.use_unit_prefixes = false;
	printops.use_prefixes_for_all_units = false;
	printops.use_prefixes_for_currencies = false;
	result_format_updated();
}
void on_menu_item_display_prefixes_for_selected_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.use_unit_prefixes = true;
	printops.use_prefixes_for_all_units = false;
	printops.use_prefixes_for_currencies = false;
	result_format_updated();
}
void on_menu_item_display_prefixes_for_currencies_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.use_unit_prefixes = true;
	printops.use_prefixes_for_all_units = false;
	printops.use_prefixes_for_currencies = true;
	result_format_updated();
}
void on_menu_item_display_prefixes_for_all_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.use_unit_prefixes = true;
	printops.use_prefixes_for_all_units = true;
	printops.use_prefixes_for_currencies = true;
	result_format_updated();
}
void on_menu_item_indicate_infinite_series_activate(GtkMenuItem *w, gpointer) {
	printops.indicate_infinite_series = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_show_ending_zeroes_activate(GtkMenuItem *w, gpointer) {
	printops.show_ending_zeroes = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_round_halfway_to_even_activate(GtkMenuItem *w, gpointer) {
	printops.round_halfway_to_even = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_negative_exponents_activate(GtkMenuItem *w, gpointer) {
	printops.negative_exponents = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_sort_minus_last_activate(GtkMenuItem *w, gpointer) {
	printops.sort_options.minus_last = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_always_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.approximation = APPROXIMATION_EXACT;

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_approximation"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_approximation_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_approximation")), 0);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_approximation"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_approximation_changed, NULL);
	
	expression_calculation_updated();
}
void on_menu_item_try_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.approximation = APPROXIMATION_TRY_EXACT;
	
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_approximation"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_approximation_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_approximation")), 1);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_approximation"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_approximation_changed, NULL);

	expression_calculation_updated();
}
void on_menu_item_approximate_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.approximation = APPROXIMATION_APPROXIMATE;
	
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_approximation"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_approximation_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_approximation")), 2);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_approximation"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_approximation_changed, NULL);
	
	expression_calculation_updated();
}
void on_menu_item_fraction_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.number_fraction_format = FRACTION_DECIMAL;
	
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_fraction_mode")), 0);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);

	result_format_updated();
}
void on_menu_item_fraction_decimal_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.number_fraction_format = FRACTION_DECIMAL_EXACT;

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_fraction_mode")), 1);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	
	result_format_updated();
}
void on_menu_item_fraction_combined_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.number_fraction_format = FRACTION_COMBINED;

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_fraction_mode")), 3);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	
	result_format_updated();
}
void on_menu_item_fraction_fraction_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
		return;
	printops.number_fraction_format = FRACTION_FRACTIONAL;

	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_fraction_mode")), 2);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "combobox_fraction_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_combobox_fraction_mode_changed, NULL);
	
	result_format_updated();
}

void on_menu_item_save_activate(GtkMenuItem*, gpointer) {
	add_as_variable();
}
void on_menu_item_save_image_activate(GtkMenuItem*, gpointer) {
	if(display_aborted || !displayed_mstruct) return;
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to save PNG image to"), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Allowed File Types"));
	gtk_file_filter_add_mime_type(filter, "image/png");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter);
	GtkFileFilter *filter_all = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter_all, "*");
	gtk_file_filter_set_name(filter_all, _("All Files"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter_all);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(d), "qalculate.png");
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
		GdkRGBA color;
		color.red = 0.0;
		color.green = 0.0;
		color.blue = 0.0;
		color.alpha = 1.0;
		cairo_surface_t *s = draw_structure(*displayed_mstruct, printops, top_ips, NULL, 1, &color);
		if(s) {
			cairo_surface_flush(s);
			cairo_surface_write_to_png(s, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d)));
			cairo_surface_destroy(s);
		}
	}
	gtk_widget_destroy(d);
}
void on_menu_item_copy_activate(GtkMenuItem*, gpointer) {
	gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), result_text.c_str(), -1);
}
void on_menu_item_precision_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = get_precision_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_precision_dialog_spinbutton_precision_value_changed, NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision")), PRECISION);	
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_precision_dialog_spinbutton_precision_value_changed, NULL);
	gtk_widget_show(dialog);
}
void on_menu_item_decimals_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = get_decimals_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_max_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_min_toggled, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_max_value_changed, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_min_value_changed, NULL);	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min")), printops.use_min_decimals);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max")), printops.use_max_decimals);	
	gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), printops.use_min_decimals);
	gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), printops.use_max_decimals);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), printops.min_decimals);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), printops.max_decimals);	
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_max_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_min_toggled, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_max_value_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_min_value_changed, NULL);	
	gtk_widget_show(dialog);
}

gboolean on_main_window_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer) {
	//focus_keeping_selection();
	return FALSE;
}


void on_button_registerup_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	if(index == 0) {
		CALCULATOR->moveRPNRegister(1, CALCULATOR->RPNStackSize());
		gtk_tree_model_iter_nth_child(model, &iter2, NULL, CALCULATOR->RPNStackSize() - 1);
		gtk_list_store_move_after(stackstore, &iter, &iter2);
	} else {
		CALCULATOR->moveRPNRegisterUp(index + 1);
		gtk_tree_model_iter_nth_child(model, &iter2, NULL, index - 1);
		gtk_list_store_swap(stackstore, &iter, &iter2);
	}	
	if(index <= 1) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		mstruct->ref();
		setResult(NULL, true, false, false, "", 0, true);
	}
	updateRPNIndexes();
}
void on_button_registerdown_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(CALCULATOR->RPNStackSize() == 0) return;
		if(!gtk_tree_model_iter_nth_child(model, &iter, NULL, CALCULATOR->RPNStackSize() - 1)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	if(index + 1 == (int) CALCULATOR->RPNStackSize()) {
		CALCULATOR->moveRPNRegister(CALCULATOR->RPNStackSize(), 1);
		gtk_tree_model_get_iter_first(model, &iter2);
		gtk_list_store_move_before(stackstore, &iter, &iter2);
	} else {
		CALCULATOR->moveRPNRegisterDown(index + 1);
		gtk_tree_model_iter_nth_child(model, &iter2, NULL, index + 1);
		gtk_list_store_swap(stackstore, &iter, &iter2);
	}			
	if(index == 0 || index == (int) CALCULATOR->RPNStackSize() - 1) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		mstruct->ref();
		setResult(NULL, true, false, false, "", 0, true);
	}
	updateRPNIndexes();
}
void on_button_registerswap_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	if(index == 0) {		
		if(!gtk_tree_model_iter_nth_child(model, &iter2, NULL, 1)) return;
		CALCULATOR->moveRPNRegister(1, 2);
		gtk_list_store_swap(stackstore, &iter, &iter2);
	} else {
		CALCULATOR->moveRPNRegister(index + 1, 1);
		gtk_tree_model_get_iter_first(model, &iter2);
		gtk_list_store_move_before(stackstore, &iter, &iter2);
	}			
	mstruct->unref();
	mstruct = CALCULATOR->getRPNRegister(1);
	mstruct->ref();
	setResult(NULL, true, false, false, "", 0, true);
	updateRPNIndexes();
}
void on_button_copyregister_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *text_copy = NULL;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	CALCULATOR->RPNStackEnter(new MathStructure(*CALCULATOR->getRPNRegister(index + 1)));
	gtk_tree_model_get(model, &iter, 1, &text_copy, -1);
	RPNRegisterAdded(text_copy, 0);
	g_free(text_copy);
	mstruct->unref();
	mstruct = CALCULATOR->getRPNRegister(1);
	mstruct->ref();
	setResult(NULL, true, false, false, "", 0, true);
}
void on_button_editregister_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(stackview), path, register_column, register_renderer, TRUE);
		gtk_tree_path_free(path);
	}
}
void on_button_deleteregister_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return; 
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	CALCULATOR->deleteRPNRegister(index + 1);
	RPNRegisterRemoved(index);
	if(CALCULATOR->RPNStackSize() == 0) {
		clearresult();
		mstruct->clear();
	} else if(index == 0) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		mstruct->ref();
		setResult(NULL, true, false, false, "", 0, true);
	}
}
void on_button_clearstack_clicked(GtkButton*, gpointer) {
	CALCULATOR->clearRPNStack();
	gtk_list_store_clear(stackstore);
	clearresult();
	mstruct->clear();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_clearstack")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy")), FALSE);
}
void on_stackview_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_editregister")), TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_editregister")), FALSE);
	}
}
void on_stackview_item_edited(GtkCellRendererText*, gchar *path, gchar *new_text, gpointer) {
	int index = s2i(path);
	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index);
	gtk_list_store_set(stackstore, &iter, 1, new_text, -1);
	execute_expression(true, false, OPERATION_ADD, NULL, true, index);
	b_editing_stack = false;
}
void on_stackview_item_editing_started(GtkCellRenderer*, GtkCellEditable*, gchar*, gpointer) {
	b_editing_stack = true;
}
void on_stackview_item_editing_canceled(GtkCellRenderer*, gpointer) {
	b_editing_stack = false;
}

void on_unit_edit_entry_relation_changed(GtkEditable *w, gpointer) {
	string str = gtk_entry_get_text(GTK_ENTRY(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_box_reversed")), str.find("\\x") != string::npos);
}

void correct_name_entry(GtkEditable *editable, ExpressionItemType etype, gpointer function) {
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	if(str.empty()) return;
	remove_blank_ends(str);
	bool b = false;
	if(!str.empty()) {
		switch(etype) {
			case TYPE_FUNCTION: {
				b = CALCULATOR->functionNameIsValid(str);
				if(!b) str = CALCULATOR->convertToValidFunctionName(str);
				break;
			}
			case TYPE_UNIT: {
				b = CALCULATOR->unitNameIsValid(str);
				if(!b) str = CALCULATOR->convertToValidUnitName(str);
				break;
			}
			case TYPE_VARIABLE: {
				b = CALCULATOR->variableNameIsValid(str);
				if(!b) str = CALCULATOR->convertToValidVariableName(str);
				break;
			}
		}
	}
	if(!b) {
		g_signal_handlers_block_matched((gpointer) editable, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, function, NULL);
		gtk_entry_set_text(GTK_ENTRY(editable), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) editable, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, function, NULL);
	}
}

/*
	check if entered unit name is valid, if not modify
*/
void on_unit_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_UNIT, (gpointer) on_unit_edit_entry_name_changed);
}
/*
	selected unit type in edit/new unit dialog has changed
*/
void on_unit_edit_combobox_class_changed(GtkComboBox *om, gpointer) {

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_relation_title")), gtk_combo_box_get_active(om) != UNIT_CLASS_BASE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_base")), gtk_combo_box_get_active(om) != UNIT_CLASS_BASE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), gtk_combo_box_get_active(om) != UNIT_CLASS_BASE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_use_prefixes")), gtk_combo_box_get_active(om) != UNIT_CLASS_COMPOSITE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_exp")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_relation")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_exact")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_reversed")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_frame_mix")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT && gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp"))) == 1);
	if(gtk_combo_box_get_active(om) != UNIT_CLASS_ALIAS_UNIT) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		on_unit_edit_checkbutton_mix_toggled(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), NULL);		
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), 1);
	}
}

void on_unit_edit_checkbutton_mix_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_priority")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_min")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), gtk_toggle_button_get_active(w));
}

void on_unit_edit_spinbutton_exp_value_changed(GtkSpinButton *w, gpointer) {
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class"))) != UNIT_CLASS_ALIAS_UNIT) return;
	if(gtk_spin_button_get_value_as_int(w) != 1) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_frame_mix")), FALSE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_frame_mix")), TRUE);
	}
}

/*
	selected unit system in edit/new unit dialog has changed
*/
void on_unit_edit_combo_system_changed(GtkComboBox *om, gpointer) {
	string str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(om));
	if(str == "SI" || str == "CGS") {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_use_prefixes")), TRUE);
	}
}

/*
	"New" button clicked in unit manager -- open new unit dialog
*/
void on_units_button_new_clicked(GtkButton*, gpointer) {
	if(selected_unit_category.empty() || selected_unit_category[0] != '/') {
		edit_unit("", NULL, GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
	} else {
		//fill in category field with selected category
		edit_unit(selected_unit_category.substr(1, selected_unit_category.length() - 1).c_str(), NULL, GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
	}
}

/*
	"Edit" button clicked in unit manager -- open edit unit dialog for selected unit
*/
void on_units_button_edit_clicked(GtkButton*, gpointer) {
	Unit *u = get_selected_unit();
	if(u) {
		edit_unit("", u, GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
	}
}

/*
	"Insert" button clicked in unit manager -- insert selected unit in expression entry
*/
void on_units_button_insert_clicked(GtkButton*, gpointer) {
	Unit *u = get_selected_unit();
	if(u) {
		gchar *gstr;
		gstr = g_strdup(u->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext).c_str());
		insert_text(gstr);
		g_free(gstr);
	}
}

/*
	"Convert" button clicked in unit manager -- convert result to selected unit
*/
void on_units_button_convert_to_clicked(GtkButton*, gpointer) {
	if(b_busy) return;
	Unit *u = get_selected_unit();
	if(u) {
		executeCommand(COMMAND_CONVERT_UNIT, true, "", u);
		focus_keeping_selection();
	}
}

/*
	deletion of unit requested
*/
void on_units_button_delete_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	Unit *u = get_selected_unit();
	if(u && u->isLocal()) {
		if(u->isUsedByOtherUnits()) {
			//do not delete units that are used by other units
			show_message(_("Cannot delete unit as it is needed by other units."), GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
			return;
		}
		for(size_t i = 0; i < recent_units.size(); i++) {
			if(recent_units[i] == u) {
				recent_units.erase(recent_units.begin() + i);
				gtk_widget_destroy(recent_unit_items[i]);
				recent_unit_items.erase(recent_unit_items.begin() + i);
				break;
			}
		}
		//ensure that all references to the unit is removed in Calculator
		u->destroy();
		//update menus and trees
		if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &model, &iter)) {
			//reselect selected unit category
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			string str = selected_unit_category;
			update_umenus();
			if(str == selected_unit_category) gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), path);
			gtk_tree_path_free(path);
		} else {
			update_umenus();
		}
	}
}

/*
	"New" button clicked in variable manager -- open new variable dialog
*/
void on_variables_button_new_clicked(GtkButton*, gpointer) {
	if(selected_variable_category.empty() || selected_variable_category[0] != '/') {
		edit_variable("", NULL, NULL, GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
	} else {
		//fill in category field with selected category
		edit_variable(selected_variable_category.substr(1, selected_variable_category.length() - 1).c_str(), NULL, NULL, GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
	}
}

/*
	"Edit" button clicked in variable manager -- open edit dialog for selected variable
*/
void on_variables_button_edit_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v) {
		if(!CALCULATOR->stillHasVariable(v)) {
			show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
			update_vmenu();
			return;
		}
		edit_variable("", v, NULL, GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
	}
}

/*
	"Insert" button clicked in variable manager -- insert variable name in expression entry
*/
void on_variables_button_insert_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v) {
		if(!CALCULATOR->stillHasVariable(v)) {
			show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
			update_vmenu();
			return;
		}
		gchar *gstr = g_strdup(v->preferredInputName(printops.abbreviate_names, true, false, false, &can_display_unicode_string_function, (void*) expressiontext).name.c_str());
		insert_text(gstr);
		g_free(gstr);
	}
}

/*
	"Delete" button clicked in variable manager -- deletion of selected variable requested
*/
void on_variables_button_delete_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	Variable *v = get_selected_variable();
	if(v && !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
		update_vmenu();
		return;
	}
	if(v && v->isLocal()) {
		for(size_t i = 0; i < recent_variables.size(); i++) {
			if(recent_variables[i] == v) {
				recent_variables.erase(recent_variables.begin() + i);
				gtk_widget_destroy(recent_variable_items[i]);
				recent_variable_items.erase(recent_variable_items.begin() + i);
				break;
			}
		}
		//ensure that all references are removed in Calculator
		v->destroy();

		//update menus and trees
		if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &model, &iter)) {
			//reselect selected variable category
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			string str = selected_variable_category;
			update_vmenu();
			if(str == selected_variable_category) gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), path);
			gtk_tree_path_free(path);
		} else {
			update_vmenu();
		}
	}
}

void on_variables_button_export_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v && !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
		update_vmenu();
		return;
	}
	if(v && v->isKnown()) {
		export_csv_file((KnownVariable*) v, GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
	}
}

/*
	"Close" button clicked in variable manager -- hide
*/
void on_variables_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
}

/*
	"New" button clicked in function manager -- open new function dialog
*/
void on_functions_button_new_clicked(GtkButton*, gpointer) {
	if(selected_function_category.empty() || selected_function_category[0] != '/') {
		edit_function("", NULL, GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
	} else {
		//fill in category field with selected category
		edit_function(selected_function_category.substr(1, selected_function_category.length() - 1).c_str(), NULL, GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
	}
}

/*
	"Edit" button clicked in function manager -- open edit function dialog for selected function
*/
void on_functions_button_edit_clicked(GtkButton*, gpointer) {
	MathFunction *f = get_selected_function();
	if(f) {
		edit_function("", f, GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
	}
}

/*
	"Insert" button clicked in function manager -- open dialog for insertion of function in expression entry
*/
void on_functions_button_insert_clicked(GtkButton*, gpointer) {
	insert_function(get_selected_function(), GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
}

/*
	"Apply" button clicked in function manager -- apply function to current result
*/
void on_functions_button_apply_clicked(GtkButton*, gpointer) {
	apply_function(get_selected_function(), GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
}

/*
	"Delete" button clicked in function manager -- deletion of selected function requested
*/
void on_functions_button_delete_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	MathFunction *f = get_selected_function();
	if(f && f->isLocal()) {
		for(size_t i = 0; i < recent_functions.size(); i++) {
			if(recent_functions[i] == f) {
				recent_functions.erase(recent_functions.begin() + i);
				gtk_widget_destroy(recent_function_items[i]);
				recent_function_items.erase(recent_function_items.begin() + i);
				break;
			}
		}
		//ensure removal of all references in Calculator
		f->destroy();
		//update menus and trees
		if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &model, &iter)) {
			//reselected selected function category
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			string str = selected_function_category;
			update_fmenu();
			if(str == selected_function_category) {
				gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), path);
			}
			gtk_tree_path_free(path);
		} else {
			update_fmenu();
		}
	}
}

/*
	"Close" button clicked in function manager -- hide
*/
void on_functions_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
}

void on_datasets_button_newset_clicked(GtkButton*, gpointer) {	
	edit_dataset(NULL, GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_editset_clicked(GtkButton*, gpointer) {
	edit_dataset(selected_dataset, GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_delset_clicked(GtkButton*, gpointer) {
	if(selected_dataset && selected_dataset->isLocal()) {
		for(size_t i = 0; i < recent_functions.size(); i++) {
			if(recent_functions[i] == selected_dataset) {
				recent_functions.erase(recent_functions.begin() + i);
				gtk_widget_destroy(recent_function_items[i]);
				recent_function_items.erase(recent_function_items.begin() + i);
				break;
			}
		}
		selected_dataset->destroy();
		selected_dataobject = NULL;		
		update_datasets_tree();
		on_tDatasets_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets)), NULL);
	}
}
void on_datasets_button_newobject_clicked(GtkButton*, gpointer) {
	edit_dataobject(selected_dataset, NULL, GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_editobject_clicked(GtkButton*, gpointer) {
	edit_dataobject(selected_dataset, selected_dataobject, GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_delobject_clicked(GtkButton*, gpointer) {
	if(selected_dataset && selected_dataobject) {
		selected_dataset->delObject(selected_dataobject);
		selected_dataobject = NULL;
		update_dataobjects();
	}
}
void on_datasets_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}


/*
	check if entered function name is valid, if not modify
*/
void on_function_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_FUNCTION, (gpointer) on_function_edit_entry_name_changed);
}
/*
	check if entered variable name is valid, if not modify
*/
void on_variable_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_VARIABLE, (gpointer) on_variable_edit_entry_name_changed);
}

void on_tMatrixEdit_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer model) {
	GtkTreeIter iter;
	gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(model), &iter, path_string);
	gtk_list_store_set(GTK_LIST_STORE (model), &iter, i_column, new_text, -1);
}
gboolean on_tMatrixEdit_editable_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer renderer) {
	switch(event->keyval) {
		case GDK_KEY_Up: {}
		case GDK_KEY_Down: {}
		case GDK_KEY_Tab: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_Return: {
			gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(w));
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
			if(path) {
				if(column) {		
					for(size_t i = 0; i < matrix_edit_columns.size(); i++) {
						if(matrix_edit_columns[i] == column) {
							if(event->keyval == GDK_KEY_Tab) {
								i++;
								if(i >= matrix_edit_columns.size()) {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_first();
									}
									i = 0;
								}
							} else {
								if(event->keyval == GDK_KEY_Up) {
									if(!gtk_tree_path_prev(path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_from_indices(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrixEdit_store), NULL) - 1, -1);
									}
								} else {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
										gtk_tree_path_free(path);
										if(event->keyval != GDK_KEY_Up) return TRUE;
										path = gtk_tree_path_new_first();
									}
								}
							}
							gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], FALSE, 0.0, 0.0);
							while(gtk_events_pending()) gtk_main_iteration();
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], TRUE);
							on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
						}
					}
				}
				gtk_tree_path_free(path);
			}
			return TRUE;
		}
	}
	return FALSE;
}
void on_tMatrixEdit_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data) {
	g_signal_connect(G_OBJECT(editable), "key-press-event", G_CALLBACK(on_tMatrixEdit_editable_key_press_event), renderer);
}
gboolean on_tMatrixEdit_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	switch(event->keyval) {
		case GDK_KEY_Return: {break;}
		case GDK_KEY_Tab: {
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
			if(path) {
				if(column) {
					for(size_t i = 0; i < matrix_edit_columns.size(); i++) {
						if(matrix_edit_columns[i] == column) {
							i++;
							if(i < matrix_edit_columns.size()) {
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], FALSE, 0.0, 0.0);
							} else {
								gtk_tree_path_next(path);
								GtkTreeIter iter;
								if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
									gtk_tree_path_free(path);
									path = gtk_tree_path_new_first();
								}
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], FALSE, 0.0, 0.0);
							}
							gtk_tree_path_free(path);
							on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
							return TRUE;
						}
					}
				}
				gtk_tree_path_free(path);
			}
			break;
		}
		default: {
			if(event->length == 0) return FALSE;
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
			if(path) {
				if(column) {
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, column, TRUE);
					while(gtk_events_pending()) gtk_main_iteration();
					gboolean return_val = FALSE;
					g_signal_emit_by_name((gpointer) gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog"), "key_press_event", event, &return_val);
					gtk_tree_path_free(path);
					return TRUE;
				}
				gtk_tree_path_free(path);
			}
		}
	}
	return FALSE;
}
gboolean on_tMatrixEdit_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(event->button != 1) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tMatrixEdit), (gint) event->x, (gint) event->y, &path, &column, NULL, NULL) && path && column) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, column, TRUE);
		gtk_tree_path_free(path);
		return TRUE;
	}
	if(path) gtk_tree_path_free(path);
	return FALSE;
}
GtkTreeIter matrix_edit_prev_iter;
gint matrix_edit_prev_column;
bool block_matrix_edit_update_cursor = false;
gboolean on_tMatrixEdit_cursor_changed(GtkTreeView*, gpointer) {
	if(block_matrix_edit_update_cursor) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
	bool b = false;
	if(path) {
		if(column) {
			if(gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
				gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "column"));
				matrix_edit_prev_iter = iter;
				matrix_edit_prev_column = i_column;
				gchar *pos_str;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")))) {
					pos_str = g_strdup_printf("(%i, %i)", i_column + 1, gtk_tree_path_get_indices(path)[0] + 1);
				} else {
					pos_str = g_strdup_printf("%i", (int) (i_column + 1 + matrix_edit_columns.size() * gtk_tree_path_get_indices(path)[0]));
				}
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_position")), pos_str);
				g_free(pos_str);
				b = true;
			}
		}
		gtk_tree_path_free(path);
	}
	if(!b) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_position")), _("none"));
	return FALSE;
}

void on_matrix_edit_spinbutton_columns_value_changed(GtkSpinButton *w, gpointer) {
	gint c = matrix_edit_columns.size();
	gint new_c = gtk_spin_button_get_value_as_int(w);
	if(new_c < c) {
		for(gint index_c = new_c; index_c < c; index_c++) {
			gtk_tree_view_remove_column(GTK_TREE_VIEW(tMatrixEdit), matrix_edit_columns[index_c]);
		}
		matrix_edit_columns.resize(new_c);
	} else {
		GtkTreeIter iter;
		for(gint index_c = c; index_c < new_c; index_c++) {
			GtkCellRenderer *matrix_edit_renderer = gtk_cell_renderer_text_new();
			g_object_set(G_OBJECT(matrix_edit_renderer), "editable", TRUE, NULL);
			g_object_set(G_OBJECT(matrix_edit_renderer), "xalign", 1.0, NULL);
			g_object_set_data(G_OBJECT(matrix_edit_renderer), "column", GINT_TO_POINTER(index_c));
			g_signal_connect(G_OBJECT(matrix_edit_renderer), "edited", G_CALLBACK(on_tMatrixEdit_edited), GTK_TREE_MODEL(tMatrixEdit_store));
			g_signal_connect(G_OBJECT(matrix_edit_renderer), "editing-started", G_CALLBACK(on_tMatrixEdit_editing_started), GTK_TREE_MODEL(tMatrixEdit_store));
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(i2s(index_c).c_str(), matrix_edit_renderer, "text", index_c, NULL);
			g_object_set_data(G_OBJECT(column), "column", GINT_TO_POINTER(index_c));
			g_object_set_data(G_OBJECT(column), "renderer", (gpointer) matrix_edit_renderer);
			gtk_tree_view_column_set_min_width(column, 50);
			gtk_tree_view_column_set_alignment(column, 0.5);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tMatrixEdit), column);
			gtk_tree_view_column_set_expand(column, TRUE);
			matrix_edit_columns.push_back(column);
		}
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrixEdit_store), &iter)) return;
		bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")));
		while(true) {
			for(gint index_c = c; index_c < new_c; index_c++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "", -1);
			}
			if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter)) break;
		}
	}
}
void on_matrix_edit_spinbutton_rows_value_changed(GtkSpinButton *w, gpointer) {
	gint new_r = gtk_spin_button_get_value_as_int(w);
	gint r = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrixEdit_store), NULL);
	gint c = matrix_edit_columns.size();
	bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")));
	GtkTreeIter iter;
	if(r < new_r) {
		while(r < new_r) {
			gtk_list_store_append(GTK_LIST_STORE(tMatrixEdit_store), &iter);
			for(gint i = 0; i < c; i++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, i, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, i, "", -1);
			}
			r++;
		}
	} else if(new_r < r) {
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(tMatrixEdit_store), &iter, NULL, new_r);
		while(gtk_list_store_iter_is_valid(GTK_LIST_STORE(tMatrixEdit_store), &iter)) {
			gtk_list_store_remove(GTK_LIST_STORE(tMatrixEdit_store), &iter);
		}
	}
}

void on_tMatrix_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer model) {
	GtkTreeIter iter;
	gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path_string);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, i_column, new_text, -1);
}
gboolean on_tMatrix_editable_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer renderer) {
	switch(event->keyval) {
		case GDK_KEY_Up: {}
		case GDK_KEY_Down: {}
		case GDK_KEY_Tab: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_Return: {
			gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(w));
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
			if(path) {
				if(column) {		
					for(size_t i = 0; i < matrix_columns.size(); i++) {
						if(matrix_columns[i] == column) {
							if(event->keyval == GDK_KEY_Tab) {
								i++;
								if(i >= matrix_columns.size()) {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_first();
									}
									i = 0;
								}
							} else {
								if(event->keyval == GDK_KEY_Up) {
									if(!gtk_tree_path_prev(path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_from_indices(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrix_store), NULL) - 1, -1);
									}
								} else {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
										gtk_tree_path_free(path);
										if(event->keyval != GDK_KEY_Up) return TRUE;
										path = gtk_tree_path_new_first();
									}
								}
							}
							gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], FALSE, 0.0, 0.0);
							while(gtk_events_pending()) gtk_main_iteration();
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], TRUE);
							on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
						}
					}
				}
				gtk_tree_path_free(path);
			}
			return TRUE;
		}
	}
	return FALSE;
}
void on_tMatrix_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data) {
	g_signal_connect(G_OBJECT(editable), "key-press-event", G_CALLBACK(on_tMatrix_editable_key_press_event), renderer);
}
gboolean on_tMatrix_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	switch(event->keyval) {
		case GDK_KEY_Return: {break;}
		case GDK_KEY_Tab: {
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
			if(path) {
				if(column) {
					for(size_t i = 0; i < matrix_columns.size(); i++) {
						if(matrix_columns[i] == column) {
							i++;
							if(i < matrix_columns.size()) {
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], FALSE, 0.0, 0.0);
							} else {
								gtk_tree_path_next(path);
								GtkTreeIter iter;
								if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
									gtk_tree_path_free(path);
									path = gtk_tree_path_new_first();
								}
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], FALSE, 0.0, 0.0);
							}
							gtk_tree_path_free(path);
							on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
							return TRUE;
						}
					}
				}
				gtk_tree_path_free(path);
			}
			break;
		}
		default: {
			if(event->length == 0) return FALSE;
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
			if(path) {
				if(column) {
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, column, TRUE);
					while(gtk_events_pending()) gtk_main_iteration();
					gboolean return_val = FALSE;
					g_signal_emit_by_name((gpointer) gtk_builder_get_object(matrix_builder, "matrix_dialog"), "key_press_event", event, &return_val);
					gtk_tree_path_free(path);
					return TRUE;
				}
				gtk_tree_path_free(path);
			}
		}
	}
	return FALSE;
}
gboolean on_tMatrix_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(event->button != 1) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tMatrix), (gint) event->x, (gint) event->y, &path, &column, NULL, NULL) && path && column) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, column, TRUE);
		gtk_tree_path_free(path);
		return TRUE;
	}
	if(path) gtk_tree_path_free(path);
	return FALSE;
}
GtkTreeIter matrix_prev_iter;
gint matrix_prev_column;
bool block_matrix_update_cursor = false;
gboolean on_tMatrix_cursor_changed(GtkTreeView*, gpointer) {
	if(block_matrix_update_cursor) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
	bool b = false;
	if(path) {
		if(column) {
			if(gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
				gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "column"));
				matrix_prev_iter = iter;
				matrix_prev_column = i_column;
				gchar *pos_str;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")))) {
					pos_str = g_strdup_printf("(%i, %i)", i_column + 1, gtk_tree_path_get_indices(path)[0] + 1);
				} else {
					pos_str = g_strdup_printf("%i", (int) (i_column + 1 + matrix_columns.size() * gtk_tree_path_get_indices(path)[0]));
				}
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_position")), pos_str);
				g_free(pos_str);
				b = true;
			}
		}
		gtk_tree_path_free(path);
	}
	if(!b) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_position")), _("none"));
	return FALSE;
}

void on_matrix_spinbutton_columns_value_changed(GtkSpinButton *w, gpointer) {
	gint c = matrix_columns.size();
	gint new_c = gtk_spin_button_get_value_as_int(w);
	if(new_c < c) {
		for(gint index_c = new_c; index_c < c; index_c++) {
			gtk_tree_view_remove_column(GTK_TREE_VIEW(tMatrix), matrix_columns[index_c]);
		}
		matrix_columns.resize(new_c);
	} else {
		GtkTreeIter iter;
		for(gint index_c = c; index_c < new_c; index_c++) {
			GtkCellRenderer *matrix_renderer = gtk_cell_renderer_text_new();
			g_object_set(G_OBJECT(matrix_renderer), "editable", TRUE, NULL);
			g_object_set(G_OBJECT(matrix_renderer), "xalign", 1.0, NULL);
			g_object_set_data(G_OBJECT(matrix_renderer), "column", GINT_TO_POINTER(index_c));
			g_signal_connect(G_OBJECT(matrix_renderer), "edited", G_CALLBACK(on_tMatrix_edited), GTK_TREE_MODEL(tMatrix_store));
			g_signal_connect(G_OBJECT(matrix_renderer), "editing-started", G_CALLBACK(on_tMatrix_editing_started), GTK_TREE_MODEL(tMatrix_store));
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(i2s(index_c).c_str(), matrix_renderer, "text", index_c, NULL);
			g_object_set_data (G_OBJECT(column), "column", GINT_TO_POINTER(index_c));
			gtk_tree_view_column_set_min_width(column, 50);
			gtk_tree_view_column_set_alignment(column, 0.5);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tMatrix), column);
			gtk_tree_view_column_set_expand(column, TRUE);
			matrix_columns.push_back(column);
		}
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrix_store), &iter)) return;
		bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")));
		while(true) {
			for(gint index_c = c; index_c < new_c; index_c++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "", -1);
			}
			if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter)) break;
		}
	}
}
void on_matrix_spinbutton_rows_value_changed(GtkSpinButton *w, gpointer) {
	gint new_r = gtk_spin_button_get_value_as_int(w);
	gint r = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrix_store), NULL);
	gint c = matrix_columns.size();
	bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")));
	GtkTreeIter iter;
	if(r < new_r) {
		while(r < new_r) {
			gtk_list_store_append(GTK_LIST_STORE(tMatrix_store), &iter);
			for(gint i = 0; i < c; i++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, i, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, i, "", -1);
			}
			r++;
		}
	} else if(new_r < r) {
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(tMatrix_store), &iter, NULL, new_r);
		while(gtk_list_store_iter_is_valid(GTK_LIST_STORE(tMatrix_store), &iter)) {
			gtk_list_store_remove(GTK_LIST_STORE(tMatrix_store), &iter);
		}
	}
}

bool updating_percentage_entries = false;
void update_percentage_entries();
vector<int> percentage_entries_changes;
void on_percentage_button_calculate_clicked(GtkWidget*, gpointer) {
	update_percentage_entries();
}
void on_percentage_button_clear_clicked(GtkWidget*, gpointer) {
	percentage_entries_changes.clear();
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_1")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_2")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_3")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_4")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_5")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_6")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_7")), "");
}
void percentage_entry_changed(int entry_id, GtkEntry *w) {
	for(size_t i = 0; i < percentage_entries_changes.size(); i++) {
		if(percentage_entries_changes[i] == entry_id) {
			percentage_entries_changes.erase(percentage_entries_changes.begin() + i);
			break;
		}
	}
	if(gtk_entry_get_text_length(w) == 0) return;
	percentage_entries_changes.push_back(entry_id);
}
void on_percentage_entry_1_changed(GtkEditable *w, gpointer) {percentage_entry_changed(1, GTK_ENTRY(w));}
void on_percentage_entry_2_changed(GtkEditable *w, gpointer) {percentage_entry_changed(2, GTK_ENTRY(w));}
void on_percentage_entry_3_changed(GtkEditable *w, gpointer) {percentage_entry_changed(4, GTK_ENTRY(w));}
void on_percentage_entry_4_changed(GtkEditable *w, gpointer) {percentage_entry_changed(8, GTK_ENTRY(w));}
void on_percentage_entry_5_changed(GtkEditable *w, gpointer) {percentage_entry_changed(16, GTK_ENTRY(w));}
void on_percentage_entry_6_changed(GtkEditable *w, gpointer) {percentage_entry_changed(32, GTK_ENTRY(w));}
void on_percentage_entry_7_changed(GtkEditable *w, gpointer) {percentage_entry_changed(64, GTK_ENTRY(w));}
void on_percentage_entry_1_activate(GtkEditable *w, gpointer) {percentage_entry_changed(1, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_2_activate(GtkEditable *w, gpointer) {percentage_entry_changed(2, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_3_activate(GtkEditable *w, gpointer) {percentage_entry_changed(4, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_4_activate(GtkEditable *w, gpointer) {percentage_entry_changed(8, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_5_activate(GtkEditable *w, gpointer) {percentage_entry_changed(16, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_6_activate(GtkEditable *w, gpointer) {percentage_entry_changed(32, GTK_ENTRY(w)); update_percentage_entries();}
void on_percentage_entry_7_activate(GtkEditable *w, gpointer) {percentage_entry_changed(64, GTK_ENTRY(w)); update_percentage_entries();}
void update_percentage_entries() {
	if(updating_percentage_entries) return;
	if(percentage_entries_changes.size() < 2) return;
	int variant = percentage_entries_changes[percentage_entries_changes.size() - 1];
	int variant2 = percentage_entries_changes[percentage_entries_changes.size() - 2];
	if(variant > 4) {
		for(int i = percentage_entries_changes.size() - 3; i >= 0 && variant2 > 4; i--) {
			variant2 = percentage_entries_changes[(size_t) i];
		}
		if(variant2 > 4) return;
	}
	variant += variant2;
	updating_percentage_entries = true;
	GtkWidget *w1 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_1"));
	GtkWidget *w2 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_2"));
	GtkWidget *w3 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_3"));
	GtkWidget *w4 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_4"));
	GtkWidget *w5 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_5"));
	GtkWidget *w6 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_6"));
	GtkWidget *w7 = GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_entry_7"));
	g_signal_handlers_block_matched((gpointer) w1, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_1_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w2, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_2_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w3, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_3_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w4, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_4_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w5, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_5_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w6, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_6_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w7, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_7_changed, NULL);
	MathStructure m1, m2, m3, m4, m5, m6, m7, m1_pre, m2_pre;
	string str1, str2;
	switch(variant) {
		case 3: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w2)); break;}
		case 5: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w3)); break;}
		case 9: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w4)); break;}
		case 17: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w5)); break;}
		case 33: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w6)); break;}
		case 65: {str1 = gtk_entry_get_text(GTK_ENTRY(w1)); str2 = gtk_entry_get_text(GTK_ENTRY(w7)); break;}
		case 6: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w3)); break;}
		case 10: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w4)); break;}
		case 18: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w5)); break;}
		case 34: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w6)); break;}
		case 66: {str1 = gtk_entry_get_text(GTK_ENTRY(w2)); str2 = gtk_entry_get_text(GTK_ENTRY(w7)); break;}
		case 12: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w4)); break;}
		case 20: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w5)); break;}
		case 36: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w6)); break;}
		case 68: {str1 = gtk_entry_get_text(GTK_ENTRY(w3)); str2 = gtk_entry_get_text(GTK_ENTRY(w7)); break;}
		default: {variant = 0;}
	}
	do_timeout = false;
	if(variant != 0) {
		m1_pre.set(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str1)));
		m2_pre.set(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str2)));
	}
	switch(variant) {
		case 3: {m1 = m1_pre; m2 = m2_pre; break;}
		case 5: {m1 = m1_pre; m2 = m2_pre; m2 += m1; break;}
		case 9: {m1 = m1_pre; m2 = m2_pre; m2 /= 100; m2 += 1; m2 *= m1; break;}
		case 17: {m1 = m1_pre; m2_pre /= 100; m2_pre += 1; m2 = m1; m2 /= m2_pre; break;}
		case 33: {m1 = m1_pre; m2 = m2_pre; m2 /= 100; m2 *= m1; break;}
		case 65: {m1 = m1_pre; m2_pre /= 100; m2 = m1; m2 /= m2_pre; break;}
		case 6: {m2 = m1_pre; m1 = m1_pre; m1 -= m2_pre; break;}
		case 10: {m2 = m1_pre; m2_pre /= 100; m2_pre += 1; m1 = m2; m1 /= m2_pre; break;}
		case 18: {m2 = m1_pre; m2_pre /= 100; m2_pre += 1; m1 = m2; m1 *= m2_pre; break;}
		case 34: {m2 = m1_pre; m2_pre /= 100; m1 = m2; m1 /= m2_pre; break;}
		case 66: {m2 = m1_pre; m2_pre /= 100; m1 = m2; m1 *= m2_pre; break;}
		case 12: {m1 = m1_pre; m2_pre /= 100; m1 /= m2_pre; m2 = m1; m2 += m1_pre; break;}
		case 20: {m1_pre.negate(); m2 = m1_pre; m2_pre /= 100; m2 /= m2_pre; m1 = m2; m1 += m1_pre; break;}
		case 36: {m1 = m1_pre; m2_pre /= 100; m2_pre -= 1; m1 /= m2_pre; m2 = m1; m2 += m1_pre; break;}
		case 68: {m1_pre.negate(); m2 = m1_pre; m2_pre /= 100; m2_pre -= 1; m2 /= m2_pre; m1 = m2; m1 += m1_pre; break;}
		default: {variant = 0;}
	}
	if(variant != 0) {
		m3 = m2; m3 -= m1;
		m6 = m2; m6 /= m1;
		m7 = m1; m7 /= m2;
		m4 = m6; m4 -= 1;
		m5 = m7; m5 -= 1;
		m4 *= 100; m5 *= 100; m6 *= 100; m7 *= 100;
		EvaluationOptions eo;
		eo.parse_options = evalops.parse_options;
		eo.parse_options.rpn = false;
		eo.parse_options.base = 10;
		eo.assume_denominators_nonzero = true;
		eo.warn_about_denominators_assumed_nonzero = false;
		CALCULATOR->calculate(&m1, 500, eo);
		CALCULATOR->calculate(&m2, 500, eo);
		CALCULATOR->calculate(&m3, 500, eo);
		CALCULATOR->calculate(&m4, 500, eo);
		CALCULATOR->calculate(&m5, 500, eo);
		CALCULATOR->calculate(&m6, 500, eo);
		CALCULATOR->calculate(&m7, 500, eo);
		PrintOptions po = printops;
		po.base = 10;
		po.number_fraction_format = FRACTION_DECIMAL;
		gtk_entry_set_text(GTK_ENTRY(w1), m1.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m1, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w2), m2.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m2, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w3), m3.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m3, 200, po).c_str());
		po.max_decimals = 2;
		po.use_max_decimals = true;
		gtk_entry_set_text(GTK_ENTRY(w4), m4.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m4, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w5), m5.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m5, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w6), m6.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m6, 200, po).c_str());
		gtk_entry_set_text(GTK_ENTRY(w7), m7.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(m7, 200, po).c_str());
	}
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_dialog")));
	do_timeout = true;
	g_signal_handlers_unblock_matched((gpointer) w1, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_1_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w2, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_2_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w3, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_3_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w4, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_4_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w5, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_5_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w6, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_6_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w7, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_percentage_entry_7_changed, NULL);
	updating_percentage_entries = false;
}

void update_nbases_entries(const MathStructure &value, int base) {
	GtkWidget *w_dec, *w_bin, *w_oct, *w_hex;
	w_dec = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal"));
	w_bin = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_binary"));
	w_oct = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_octal"));
	w_hex = GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal"));
	g_signal_handlers_block_matched((gpointer) w_dec, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_decimal_changed, NULL);			
	g_signal_handlers_block_matched((gpointer) w_bin, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_binary_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_oct, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_octal_changed, NULL);
	g_signal_handlers_block_matched((gpointer) w_hex, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_hexadecimal_changed, NULL);
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL;
	if(base != 10) {po.base = 10; gtk_entry_set_text(GTK_ENTRY(w_dec), value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po).c_str());}
	if(base != 2) {po.base = 2; gtk_entry_set_text(GTK_ENTRY(w_bin), value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po).c_str());}	
	if(base != 8) {po.base = 8; gtk_entry_set_text(GTK_ENTRY(w_oct), value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po).c_str());}	
	if(base != 16) {po.base = 16; gtk_entry_set_text(GTK_ENTRY(w_hex), value.isAborted() ? CALCULATOR->timedOutString().c_str() : CALCULATOR->print(value, 200, po).c_str());}
	g_signal_handlers_unblock_matched((gpointer) w_dec, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_decimal_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_bin, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_binary_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_oct, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_octal_changed, NULL);
	g_signal_handlers_unblock_matched((gpointer) w_hex, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_nbases_entry_hexadecimal_changed, NULL);
}
void on_nbases_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));
}
void on_nbases_entry_decimal_changed(GtkEditable *editable, gpointer) {	
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(is_in(OPERATORS EXP, str[str.length() - 1])) return;
	changing_in_nbases_dialog = true;	
	EvaluationOptions eo;
	eo.parse_options.angle_unit = evalops.parse_options.angle_unit;
	MathStructure value;
	do_timeout = false;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), evalops.parse_options), 1500, eo);
	update_nbases_entries(value, 10);
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));
	do_timeout = true;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_binary_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;
	if(is_in(OPERATORS, str[str.length() - 1])) return;
	EvaluationOptions eo;
	eo.parse_options.base = BASE_BINARY;
	eo.parse_options.angle_unit = evalops.parse_options.angle_unit;
	changing_in_nbases_dialog = true;	
	MathStructure value;
	do_timeout = false;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), evalops.parse_options), 1500, eo);
	update_nbases_entries(value, 2);
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));
	do_timeout = true;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_octal_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;	
	if(is_in(OPERATORS, str[str.length() - 1])) return;
	EvaluationOptions eo;
	eo.parse_options.base = BASE_OCTAL;
	eo.parse_options.angle_unit = evalops.parse_options.angle_unit;
	changing_in_nbases_dialog = true;
	MathStructure value;
	do_timeout = false;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), evalops.parse_options), 1500, eo);
	update_nbases_entries(value, 8);
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));
	do_timeout = true;
	changing_in_nbases_dialog = false;
}
void on_nbases_entry_hexadecimal_changed(GtkEditable *editable, gpointer) {
	if(changing_in_nbases_dialog) return;
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	remove_blank_ends(str);
	if(str.empty()) return;	
	if(is_in(OPERATORS, str[str.length() - 1])) return;
	EvaluationOptions eo;
	eo.parse_options.base = BASE_HEXADECIMAL;
	eo.parse_options.angle_unit = evalops.parse_options.angle_unit;
	changing_in_nbases_dialog = true;	
	MathStructure value;
	do_timeout = false;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(editable)), evalops.parse_options), 1500, eo);
	update_nbases_entries(value, 16);
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog")));
	do_timeout = true;
	changing_in_nbases_dialog = false;
}

void on_button_functions_clicked(GtkButton*, gpointer) {
	manage_functions();
}
void on_button_variables_clicked(GtkButton*, gpointer) {
	manage_variables();
}
void on_button_units_clicked(GtkButton*, gpointer) {
	manage_units();
}
void on_button_bases_clicked(GtkButton*, gpointer) {
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	convert_number_bases(str.c_str());
}
void on_button_convert_clicked(GtkButton*, gpointer user_data) {
	on_menu_item_convert_to_unit_expression_activate(NULL, user_data);
}


gboolean on_about_activate_link(GtkAboutDialog*, gchar *uri, gpointer) {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL)
	return TRUE;
#else
	return FALSE;
#endif
}

void on_menu_item_about_activate(GtkMenuItem*, gpointer) {
	const gchar *authors[] = {"Hanna Knutsson", NULL};
	GtkWidget *dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Powerful and easy to use calculator"));
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2003–2007, 2008, 2016-2017 Hanna Knutsson");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Qalculate! (GTK+)");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://qalculate.github.io/");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mainwindow));
	g_signal_connect(G_OBJECT(dialog), "activate-link", G_CALLBACK(on_about_activate_link), NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void on_menu_item_help_activate(GtkMenuItem*, gpointer) {
	show_help("index.html", gtk_builder_get_object(main_builder, "main_window"));
}

/*
	precision has changed in precision dialog
*/
void on_precision_dialog_spinbutton_precision_value_changed(GtkSpinButton *w, gpointer) {
	CALCULATOR->setPrecision(gtk_spin_button_get_value_as_int(w));
}
void on_precision_dialog_button_recalculate_clicked(GtkButton*, gpointer) {
	CALCULATOR->setPrecision(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision"))));
	execute_expression(true, false, OPERATION_ADD, NULL, rpn_mode);
}


void on_decimals_dialog_spinbutton_max_value_changed(GtkSpinButton *w, gpointer) {
	printops.max_decimals = gtk_spin_button_get_value_as_int(w);
	result_format_updated();
}
void on_decimals_dialog_spinbutton_min_value_changed(GtkSpinButton *w, gpointer) {
	printops.min_decimals = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
	result_format_updated();
}
void on_decimals_dialog_checkbutton_max_toggled(GtkToggleButton *w, gpointer) {
	printops.use_max_decimals = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), printops.use_max_decimals);
	result_format_updated();
}
void on_decimals_dialog_checkbutton_min_toggled(GtkToggleButton *w, gpointer) {
	printops.use_min_decimals = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), printops.use_min_decimals);
	result_format_updated();
}

void on_unknown_edit_checkbutton_custom_assumptions_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_type")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_hbox_sign")), gtk_toggle_button_get_active(w));
}
void on_unknown_edit_combobox_type_changed(GtkComboBox *om, gpointer) {	
	if((AssumptionType) gtk_combo_box_get_active(om) + 2 <= ASSUMPTION_TYPE_COMPLEX && (AssumptionSign) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"))) != ASSUMPTION_SIGN_NONZERO) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_sign_changed, NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), ASSUMPTION_SIGN_UNKNOWN);	
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_sign_changed, NULL);
	}
}
void on_unknown_edit_combobox_sign_changed(GtkComboBox *om, gpointer) {	
	if((AssumptionSign) gtk_combo_box_get_active(om) != ASSUMPTION_SIGN_NONZERO && (AssumptionSign) gtk_combo_box_get_active(om) != ASSUMPTION_SIGN_UNKNOWN && (AssumptionType) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"))) <= ASSUMPTION_TYPE_COMPLEX - 2) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_type_changed, NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), ASSUMPTION_TYPE_REAL - 2);	
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_type_changed, NULL);
	}
}

gboolean on_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(gtk_widget_has_focus(expressiontext) || b_editing_stack) return FALSE;
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_unit")))) return FALSE;
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_category"))) || gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_unit")))) {
		if(!(event->keyval >= GDK_KEY_KP_Multiply && event->keyval <= GDK_KEY_KP_9) && !(event->keyval >= GDK_KEY_parenleft && event->keyval <= GDK_KEY_A)) return FALSE;
	}
	if(event->keyval > GDK_KEY_Hyper_R || event->keyval < GDK_KEY_Shift_L) {
		GtkWidget *w = gtk_window_get_focus(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")));
		if(gtk_bindings_activate_event(G_OBJECT(o), event)) return TRUE;
		if(w && gtk_bindings_activate_event(G_OBJECT(w), event)) return TRUE;
		focus_keeping_selection();
	}
	return FALSE;
}

gboolean on_expressiontext_focus_out_event(GtkWidget*, GdkEvent*, gpointer) {
	gtk_widget_hide(completion_window);
	return FALSE;
}
gboolean on_expressiontext_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	if(b_busy) {
		if(event->keyval == GDK_KEY_Escape) {
			if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
			else if(b_busy_result) on_abort_display(NULL, 0, NULL);
			else if(b_busy_command) on_abort_command(NULL, 0, NULL);
		}
		return TRUE;
	}
	switch(event->keyval) {
		case GDK_KEY_Escape: {
			if(gtk_widget_get_visible(completion_window)) {
				gtk_widget_hide(completion_window);
				return TRUE;
			}
			break;
		}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_Return: {
			if(gtk_widget_get_visible(completion_window)) {
				GtkTreeIter iter;
				if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view)), NULL, &iter)) {
					GtkTreePath *path = gtk_tree_model_get_path(completion_filter, &iter);
					on_completion_match_selected(GTK_TREE_VIEW(completion_view), path, NULL, NULL);
					gtk_tree_path_free(path);
					return TRUE;
				}
			}
			execute_expression();
			return TRUE;
		}
		case GDK_KEY_asciicircum: {}
		case GDK_KEY_dead_circumflex: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_RAISE);
				return TRUE;
			}
			if(!evalops.parse_options.rpn) {
				wrap_expression_selection();
			}
			overwrite_expression_selection("^");
			return TRUE;
		}
		case GDK_KEY_KP_Divide: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_DIVIDE);
				return TRUE;
			}
			if(!evalops.parse_options.rpn) {
				wrap_expression_selection();
			}
			break;
		}
		case GDK_KEY_KP_Multiply: {
			if(event->state & GDK_CONTROL_MASK) {
				if(rpn_mode && rpn_keys) {
					calculateRPN(OPERATION_RAISE);
					return TRUE;
				}
				if(!evalops.parse_options.rpn) {
					wrap_expression_selection();
				}
				overwrite_expression_selection("^");
				return TRUE;
			}
			if(rpn_mode) {
				calculateRPN(OPERATION_MULTIPLY);
				return TRUE;
			}
			if(!evalops.parse_options.rpn) {
				wrap_expression_selection();
			}
			break;
		}
		case GDK_KEY_KP_Add: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_ADD);
				return TRUE;
			}
			if(!evalops.parse_options.rpn) {
				wrap_expression_selection();
			}
			break;
		}
		case GDK_KEY_KP_Subtract: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_SUBTRACT);
				return TRUE;
			}
			if(!evalops.parse_options.rpn) {
				wrap_expression_selection();
			}
			break;
		}
		case GDK_KEY_asterisk: {
			if(event->state & GDK_CONTROL_MASK) {
				if(rpn_mode && rpn_keys) {
					calculateRPN(OPERATION_RAISE);
					return TRUE;
				}
				if(!evalops.parse_options.rpn) {
					wrap_expression_selection();
				}
				overwrite_expression_selection("^");
				return TRUE;
			}
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_MULTIPLY);
				return TRUE;
			}
		}
		case GDK_KEY_slash: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_DIVIDE);
				return TRUE;
			}
		}
		case GDK_KEY_plus: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_ADD);
				return TRUE;
			}
		}
		case GDK_KEY_minus: {
			if(rpn_mode && rpn_keys) {
				calculateRPN(OPERATION_SUBTRACT);
				return TRUE;
			}
			if(!evalops.parse_options.rpn) {
				wrap_expression_selection();
			}
			break;
		}
		case GDK_KEY_E: {
			if(event->state & GDK_CONTROL_MASK) {
				if(rpn_mode && rpn_keys) {
					calculateRPN(OPERATION_EXP10);
					return TRUE;
				}
				if(!evalops.parse_options.rpn) {
					wrap_expression_selection();
				}
				if(printops.lower_case_e) overwrite_expression_selection("e");
				else overwrite_expression_selection("E");
				return TRUE;
			}
			break;
		}
		case GDK_KEY_End: {
			GtkTextIter iend;
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			if(event->state & GDK_SHIFT_MASK) {
				GtkTextIter iselstart, iselend, ipos;
				GtkTextMark *mark = gtk_text_buffer_get_insert(expressionbuffer);
				if(mark) gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mark);
				if(!gtk_text_buffer_get_selection_bounds(expressionbuffer, &iselstart, &iselend)) gtk_text_buffer_select_range(expressionbuffer, &ipos, &iend);
				else if(gtk_text_iter_equal(&iselstart, &ipos)) gtk_text_buffer_select_range(expressionbuffer, &iselend, &iend);
				else gtk_text_buffer_select_range(expressionbuffer, &iselstart, &iend);
			} else {
				gtk_text_buffer_place_cursor(expressionbuffer, &iend);
			}
			return TRUE;
		}
		case GDK_KEY_Home: {
			GtkTextIter istart;
			gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
			if(event->state & GDK_SHIFT_MASK) {
				GtkTextIter iselstart, iselend, ipos;
				GtkTextMark *mark = gtk_text_buffer_get_insert(expressionbuffer);
				if(mark) gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mark);
				if(!gtk_text_buffer_get_selection_bounds(expressionbuffer, &iselstart, &iselend)) gtk_text_buffer_select_range(expressionbuffer, &istart, &ipos);
				else if(gtk_text_iter_equal(&iselend, &ipos)) gtk_text_buffer_select_range(expressionbuffer, &istart, &iselstart);
				else gtk_text_buffer_select_range(expressionbuffer, &istart, &iselend);
			} else {
				gtk_text_buffer_place_cursor(expressionbuffer, &istart);
			}
			return TRUE;
		}
		case GDK_KEY_Up: {
			if(gtk_widget_get_visible(completion_window)) {
				GtkTreeIter iter;
				GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view));
				bool b = false;
				if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
					if(gtk_tree_model_iter_previous(completion_filter, &iter)) b = true;
					else gtk_tree_selection_unselect_all(selection);
				} else {
					gint rows = gtk_tree_model_iter_n_children(completion_filter, NULL);
					if(rows > 0) {
						GtkTreePath *path = gtk_tree_path_new_from_indices(rows - 1, -1);
						gtk_tree_model_get_iter(completion_filter, &iter, path);
						gtk_tree_path_free(path);
						b = true;
					}
				}
				if(b) {
					gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(completion_view), FALSE);
					completion_hover_blocked = true;
					GtkTreePath *path = gtk_tree_model_get_path(completion_filter, &iter);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(completion_view), path, NULL, FALSE, 0.0, 0.0);
					gtk_tree_selection_select_iter(selection, &iter);
					gtk_tree_path_free(path);
				}
				return TRUE;
			}
		}
		case GDK_KEY_KP_Page_Up: {}
		case GDK_KEY_Page_Up: {
			if(expression_history_index + 1 < (int) expression_history.size()) {
				expression_history_index++;
				dont_change_index = true;
				block_completion();
				set_expression_text(expression_history[expression_history_index].c_str());
				unblock_completion();
				dont_change_index = false;
			}
			return TRUE;
		}
		case GDK_KEY_Down: {
			if(gtk_widget_get_visible(completion_window)) {
				GtkTreeIter iter;
				GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(completion_view));
				bool b = false;
				if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
					if(gtk_tree_model_iter_next(completion_filter, &iter)) b = true;
					else gtk_tree_selection_unselect_all(selection);
				} else {
					if(gtk_tree_model_get_iter_first(completion_filter, &iter)) b = true;
				}
				if(b) {
					gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(completion_view), FALSE);
					completion_hover_blocked = true;
					GtkTreePath *path = gtk_tree_model_get_path(completion_filter, &iter);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(completion_view), path, NULL, FALSE, 0.0, 0.0);
					gtk_tree_selection_select_iter(selection, &iter);
					gtk_tree_path_free(path);
				}
				return TRUE;
			}
		}
		case GDK_KEY_KP_Page_Down: {}
		case GDK_KEY_Page_Down: {
			if(expression_history_index > -1) {
				expression_history_index--;
				dont_change_index = true;
				block_completion();
				if(expression_history_index < 0) {
					clear_expression_text();
				} else {
					set_expression_text(expression_history[expression_history_index].c_str());
				}
				unblock_completion();
				dont_change_index = false;
			}
			return TRUE;
		}
		case GDK_KEY_KP_Separator: {
			overwrite_expression_selection(CALCULATOR->getDecimalPoint().c_str());
			return TRUE;
		}
	}
	if(event->state & GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_Z: {
				expression_redo();
				return true;
			}
			case GDK_KEY_z: {
				expression_undo();
				return true;
			}
		}
	}
	switch(event->keyval) {
		case GDK_KEY_KP_Multiply: {}
		case GDK_KEY_asterisk: {
			overwrite_expression_selection(expression_times_sign());
			return TRUE;
		}
		case GDK_KEY_KP_Divide: {}
		case GDK_KEY_slash: {
			overwrite_expression_selection(expression_divide_sign());
			return TRUE;
		}
		case GDK_KEY_KP_Subtract: {}
		case GDK_KEY_minus: {
			overwrite_expression_selection(expression_sub_sign());
			return TRUE;
		}
		case GDK_KEY_KP_Add: {}
		case GDK_KEY_plus: {
			overwrite_expression_selection(expression_add_sign());
			return TRUE;
		}
		case GDK_KEY_braceleft: {}
		case GDK_KEY_braceright: {
			return TRUE;
		}
	}	
	return FALSE;
}
gboolean on_resultview_draw(GtkWidget *widget, cairo_t *cr, gpointer) {
	if(exit_in_progress) return TRUE;
	gint scalefactor = gtk_widget_get_scale_factor(widget);
	gtk_render_background(gtk_widget_get_style_context(widget), cr, 0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));	
	if(surface_result) {
		gint w = 0, h = 0;		
		if(!first_draw_of_result) {
			if(b_busy) return TRUE;
			if(display_aborted || (!displayed_mstruct && result_too_long)) {
				PangoLayout *layout = gtk_widget_create_pango_layout(widget, NULL);
				pango_layout_set_markup(layout, display_aborted ? _("result processing was aborted") : _("result is too long\nsee history"), -1);
				pango_layout_get_pixel_size(layout, &w, &h);
				cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(s, scalefactor, scalefactor);
				cairo_t *cr2 = cairo_create(s);
				GdkRGBA rgba;
				gtk_style_context_get_color(gtk_widget_get_style_context(widget), gtk_widget_get_state_flags(widget), &rgba);
				gdk_cairo_set_source_rgba(cr2, &rgba);
				pango_cairo_show_layout(cr2, layout);
				cairo_destroy(cr2);
				cairo_surface_destroy(surface_result);
				surface_result = s;
				tmp_surface = s;
			} else if(displayed_mstruct) {
				if(displayed_mstruct->isAborted()) {
					PangoLayout *layout = gtk_widget_create_pango_layout(widget, NULL);
					pango_layout_set_markup(layout, _("calculation was aborted"), -1);
					gint w = 0, h = 0;
					pango_layout_get_pixel_size(layout, &w, &h);
					tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(tmp_surface, scalefactor, scalefactor);
					cairo_t *cr2 = cairo_create(tmp_surface);
					GdkRGBA rgba;
					gtk_style_context_get_color(gtk_widget_get_style_context(widget), gtk_widget_get_state_flags(widget), &rgba);
					gdk_cairo_set_source_rgba(cr2, &rgba);
					pango_cairo_show_layout(cr2, layout);
					cairo_destroy(cr2);
					g_object_unref(layout);
				} else {
					tmp_surface = draw_structure(*displayed_mstruct, printops, top_ips, NULL, scale_n);
				}
				surface_result = tmp_surface;
			}
		}
		w = cairo_image_surface_get_width(surface_result) / scalefactor;
		h = cairo_image_surface_get_height(surface_result) / scalefactor;
		gint sbw, sbh;
		gtk_widget_get_preferred_width(gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result"))), NULL, &sbw);
		gtk_widget_get_preferred_height(gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result"))), NULL, &sbh);
		gint rh = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")));
		gint rw = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")));
		if(first_draw_of_result || result_font_updated) {
			while(displayed_mstruct && !display_aborted && scale_n < 3 && h > (w > rw - sbw ? rh - sbh : rh)) {
				int scroll_diff = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result"))) - gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")));
				double scale_div = (double) h / (gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport"))) + scroll_diff);
				if(scale_div > 1.44) {
					scale_n = 3;
				} else if(scale_n < 2 && scale_div > 1.2) {
					scale_n = 2;
				} else {
					scale_n++;
				}
				cairo_surface_destroy(surface_result);
				surface_result = draw_structure(*displayed_mstruct, printops, top_ips, NULL, scale_n);
				w = cairo_image_surface_get_width(surface_result) / scalefactor;
				h = cairo_image_surface_get_height(surface_result) / scalefactor;
			}
			result_font_updated = FALSE;
		}
		gtk_widget_set_size_request(widget, w, h);
		if(h > sbh) rw -= sbw;
		if(rw >= w) {
			cairo_set_source_surface(cr, surface_result, rw >= w ? rw - w : rw - w - (rw - w) / 2, h < rh ? (rh - h) / 2 : 0);
		} else {
			if(h + ((rh - h) / 2) < rh - sbh) cairo_set_source_surface(cr, surface_result, 0, (rh - h) / 2);
			else cairo_set_source_surface(cr, surface_result, 0, (h > rh - sbh) ? 0 : (rh - h - sbh) / 2);
		}
		cairo_paint(cr);
		first_draw_of_result = FALSE;
	} else if(showing_first_time_message) {
		PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview")), NULL);
		GdkRGBA rgba;
		pango_layout_set_markup(layout, _("<span font=\"10\">Type a mathematical expression above, e.g. \"5 + 2 / 3\",\nand press the enter key.</span>"), -1);
		gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))), gtk_widget_get_state_flags(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))), &rgba);
		cairo_move_to(cr, 6, 0);
		gdk_cairo_set_source_rgba(cr, &rgba);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);
	}
	return TRUE;
}
void on_matrix_edit_radiobutton_matrix_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_elements")), _("Elements"));
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_elements")), _("Elements (in horizontal order)"));
	}
	on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
}
void on_matrix_edit_radiobutton_vector_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_elements")), _("Elements"));
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_elements")), _("Elements (in horizontal order)"));
	}
	on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
}
void on_matrix_radiobutton_matrix_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements"));
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements (in horizontal order)"));
	}
	on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
}
void on_matrix_radiobutton_vector_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements"));
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements (in horizontal order)"));
	}
	on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
}

void on_csv_import_radiobutton_matrix_toggled(GtkToggleButton*, gpointer) {
}
void on_csv_import_radiobutton_vectors_toggled(GtkToggleButton*, gpointer) {
}
void on_csv_import_combobox_delimiter_changed(GtkComboBox *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_delimiter_other")), gtk_combo_box_get_active(w) == DELIMITER_OTHER);
}
void on_csv_import_button_file_clicked(GtkButton*, gpointer) {
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to import"), GTK_WINDOW(gtk_builder_get_object(csvimport_builder, "csv_import_dialog")), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	string filestr = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));
	remove_blank_ends(filestr);
	if(!filestr.empty()) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
		const gchar *file_str = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")), file_str);
		string name_str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
		remove_blank_ends(name_str);
		if(name_str.empty()) {
			name_str = file_str;
			size_t i = name_str.find_last_of("/");
			if(i != string::npos) name_str = name_str.substr(i + 1, name_str.length() - i);
			i = name_str.find_last_of(".");
			if(i != string::npos) name_str = name_str.substr(0, i);
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")), name_str.c_str());
		}
	}
	gtk_widget_destroy(d);
}

void on_csv_export_combobox_delimiter_changed(GtkComboBox *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_delimiter_other")), gtk_combo_box_get_active(w) == DELIMITER_OTHER);
}
void on_csv_export_button_file_clicked(GtkButton*, gpointer) {
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to export to"), GTK_WINDOW(gtk_builder_get_object(csvexport_builder, "csv_export_dialog")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	string filestr = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
	remove_blank_ends(filestr);
	if(!filestr.empty()) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d)));
	}
	gtk_widget_destroy(d);
}
void on_csv_export_radiobutton_current_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), !gtk_toggle_button_get_active(w));
}
void on_csv_export_radiobutton_matrix_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), gtk_toggle_button_get_active(w));
}

void on_type_label_date_clicked(GtkButton *w, gpointer user_data) {
	GtkWidget *d = gtk_dialog_new_with_buttons(_("Select date"), GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW)), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_OK, _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);
	GtkWidget *date_w = gtk_calendar_new();
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), date_w);
	gtk_widget_show_all(d);
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_OK) {
		guint year = 0, month = 0, day = 0;
		gtk_calendar_get_date(GTK_CALENDAR(date_w), &year, &month, &day);
		gchar *gstr = g_strdup_printf("%i-%02i-%02i", year, month + 1, day);
		gtk_entry_set_text(GTK_ENTRY(user_data), gstr);
		g_free(gstr);
	}
	gtk_widget_destroy(d);
}
void on_type_label_file_clicked(GtkButton*, gpointer user_data) {
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to import"), GTK_WINDOW(gtk_builder_get_object(csvimport_builder, "csv_import_dialog")), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	string filestr = gtk_entry_get_text(GTK_ENTRY(user_data));
	remove_blank_ends(filestr);
	if(!filestr.empty()) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
		gtk_entry_set_text(GTK_ENTRY(user_data), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d)));
	}
	gtk_widget_destroy(d);
}

void on_functions_button_deactivate_clicked(GtkButton*, gpointer) {
	MathFunction *f = get_selected_function();
	if(f) {
		f->setActive(!f->isActive());
		update_fmenu();
	}
}
void on_variables_button_deactivate_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v) {
		v->setActive(!v->isActive());
		update_vmenu();
	}
}
void on_units_button_deactivate_clicked(GtkButton*, gpointer) {
	Unit *u = get_selected_unit();
	if(u) {
		u->setActive(!u->isActive());
		update_umenus();
	}
}

void on_function_edit_button_subfunctions_clicked(GtkButton*, gpointer) {
	gtk_window_set_transient_for(GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog_subfunctions")), GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
	gtk_dialog_run(GTK_DIALOG(gtk_builder_get_object(functionedit_builder, "function_edit_dialog_subfunctions")));
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_dialog_subfunctions")));
}
void on_function_edit_button_add_subfunction_clicked(GtkButton*, gpointer) {
	GtkTreeIter iter;	
	gtk_list_store_append(tSubfunctions_store, &iter);
	string str = "\\";
	last_subfunction_index++;
	str += i2s(last_subfunction_index);
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate")))) {
		gtk_list_store_set(tSubfunctions_store, &iter, 0, str.c_str(), 1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression"))), 2, _("Yes"), 3, last_subfunction_index, 4, TRUE, -1);
	} else {
		gtk_list_store_set(tSubfunctions_store, &iter, 0, str.c_str(), 1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression"))), 2, _("No"), 3, last_subfunction_index, 4, FALSE, -1);
	}
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression")), "");
}
void on_function_edit_button_modify_subfunction_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tSubfunctions)), &model, &iter)) {
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate")))) {
			gtk_list_store_set(tSubfunctions_store, &iter, 1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression"))), 2, _("Yes"), 4, TRUE, -1);
		} else {
			gtk_list_store_set(tSubfunctions_store, &iter, 1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_subexpression"))), 2, _("No"), 4, FALSE, -1);
		}
	}
}
void on_function_edit_button_remove_subfunction_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tSubfunctions)), &model, &iter)) {
		GtkTreeIter iter2 = iter;
		while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tSubfunctions_store), &iter2)) {
			guint index;
			gtk_tree_model_get(GTK_TREE_MODEL(tSubfunctions_store), &iter2, 3, &index, -1);
			index--;
			string str = "\\";
			str += i2s(index);
			gtk_list_store_set(tSubfunctions_store, &iter2, 0, str.c_str(), 3, index, -1);
		}
		gtk_list_store_remove(tSubfunctions_store, &iter);
		last_subfunction_index--;
	}
}
void on_function_edit_entry_subexpression_activate(GtkEntry*, gpointer) {
	if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_subfunction")))) {
		on_function_edit_button_add_subfunction_clicked(GTK_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_subfunction")), NULL);
	} else if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")))) {
		on_function_edit_button_modify_subfunction_clicked(GTK_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), NULL);
	}
}
void on_function_edit_button_add_argument_clicked(GtkButton*, gpointer) {
	GtkTreeIter iter;	
	gtk_list_store_append(tFunctionArguments_store, &iter);
	Argument *arg;
	if(edited_function && edited_function->isBuiltin()) {
		arg = new Argument();
	} else {
		int menu_index = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(functionedit_builder, "function_edit_combobox_argument_type")));
		switch(menu_index) {
			case MENU_ARGUMENT_TYPE_TEXT: {arg = new TextArgument(); break;}
			case MENU_ARGUMENT_TYPE_SYMBOLIC: {arg = new SymbolicArgument(); break;}
			case MENU_ARGUMENT_TYPE_DATE: {arg = new DateArgument(); break;}
			case MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER: {arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE); break;}
			case MENU_ARGUMENT_TYPE_POSITIVE_INTEGER: {arg = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE); break;}
			case MENU_ARGUMENT_TYPE_NONZERO_INTEGER: {arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO); break;}
			case MENU_ARGUMENT_TYPE_INTEGER: {arg = new IntegerArgument(); break;}
			case MENU_ARGUMENT_TYPE_NONNEGATIVE: {arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE); break;}
			case MENU_ARGUMENT_TYPE_POSITIVE: {arg = new NumberArgument("", ARGUMENT_MIN_MAX_POSITIVE); break;}
			case MENU_ARGUMENT_TYPE_NONZERO: {arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO); break;}
			case MENU_ARGUMENT_TYPE_NUMBER: {arg = new NumberArgument(); break;}
			case MENU_ARGUMENT_TYPE_VECTOR: {arg = new VectorArgument(); break;}
			case MENU_ARGUMENT_TYPE_MATRIX: {arg = new MatrixArgument(); break;}
			case MENU_ARGUMENT_TYPE_EXPRESSION_ITEM: {arg = new ExpressionItemArgument(); break;}
			case MENU_ARGUMENT_TYPE_FUNCTION: {arg = new FunctionArgument(); break;}
			case MENU_ARGUMENT_TYPE_UNIT: {arg = new UnitArgument(); break;}
			case MENU_ARGUMENT_TYPE_VARIABLE: {arg = new VariableArgument(); break;}
			case MENU_ARGUMENT_TYPE_FILE: {arg = new FileArgument(); break;}
			case MENU_ARGUMENT_TYPE_BOOLEAN: {arg = new BooleanArgument(); break;}	
			case MENU_ARGUMENT_TYPE_ANGLE: {arg = new AngleArgument(); break;}	
			case MENU_ARGUMENT_TYPE_DATA_OBJECT: {arg = new DataObjectArgument(NULL, ""); break;}
			case MENU_ARGUMENT_TYPE_DATA_PROPERTY: {arg = new DataPropertyArgument(NULL, ""); break;}
			default: {arg = new Argument();}
		}
	}
	arg->setName(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name"))));		
	gtk_list_store_set(tFunctionArguments_store, &iter, 0, arg->name().c_str(), 1, arg->printlong().c_str(), 2, arg, -1);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name")), "");
}
void on_function_edit_button_remove_argument_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		if(selected_argument) {
			delete selected_argument;
			selected_argument = NULL;
		}
		gtk_list_store_remove(tFunctionArguments_store, &iter);
	}
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name")), "");
}
void on_function_edit_button_modify_argument_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {	
		int argtype = ARGUMENT_TYPE_FREE;
		if(edited_function && edited_function->isBuiltin()) {
			if(!selected_argument) {
				selected_argument = new Argument();
			}
		} else {
			int menu_index = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(functionedit_builder, "function_edit_combobox_argument_type")));
			switch(menu_index) {
				case MENU_ARGUMENT_TYPE_TEXT: {argtype = ARGUMENT_TYPE_TEXT; break;}
				case MENU_ARGUMENT_TYPE_SYMBOLIC: {argtype = ARGUMENT_TYPE_SYMBOLIC; break;}
				case MENU_ARGUMENT_TYPE_DATE: {argtype = ARGUMENT_TYPE_DATE; break;}
				case MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER: {}
				case MENU_ARGUMENT_TYPE_POSITIVE_INTEGER: {}
				case MENU_ARGUMENT_TYPE_NONZERO_INTEGER: {}
				case MENU_ARGUMENT_TYPE_INTEGER: {argtype = ARGUMENT_TYPE_INTEGER; break;}
				case MENU_ARGUMENT_TYPE_NONNEGATIVE: {}
				case MENU_ARGUMENT_TYPE_POSITIVE: {}
				case MENU_ARGUMENT_TYPE_NONZERO: {}
				case MENU_ARGUMENT_TYPE_NUMBER: {argtype = ARGUMENT_TYPE_NUMBER; break;}
				case MENU_ARGUMENT_TYPE_VECTOR: {argtype = ARGUMENT_TYPE_VECTOR; break;}
				case MENU_ARGUMENT_TYPE_MATRIX: {argtype = ARGUMENT_TYPE_MATRIX; break;}
				case MENU_ARGUMENT_TYPE_EXPRESSION_ITEM: {argtype = ARGUMENT_TYPE_EXPRESSION_ITEM; break;}
				case MENU_ARGUMENT_TYPE_FUNCTION: {argtype = ARGUMENT_TYPE_FUNCTION; break;}
				case MENU_ARGUMENT_TYPE_UNIT: {argtype = ARGUMENT_TYPE_UNIT; break;}
				case MENU_ARGUMENT_TYPE_VARIABLE: {argtype = ARGUMENT_TYPE_VARIABLE; break;}
				case MENU_ARGUMENT_TYPE_FILE: {argtype = ARGUMENT_TYPE_FILE; break;}
				case MENU_ARGUMENT_TYPE_BOOLEAN: {argtype = ARGUMENT_TYPE_BOOLEAN; break;}	
				case MENU_ARGUMENT_TYPE_ANGLE: {argtype = ARGUMENT_TYPE_ANGLE; break;}	
				case MENU_ARGUMENT_TYPE_DATA_OBJECT: {argtype = ARGUMENT_TYPE_DATA_OBJECT; break;}
				case MENU_ARGUMENT_TYPE_DATA_PROPERTY: {argtype = ARGUMENT_TYPE_DATA_PROPERTY; break;}	
			}			
			
			if(!selected_argument || argtype != selected_argument->type() || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER || menu_index == MENU_ARGUMENT_TYPE_POSITIVE_INTEGER || menu_index == MENU_ARGUMENT_TYPE_NONZERO_INTEGER || menu_index == MENU_ARGUMENT_TYPE_NONZERO || menu_index == MENU_ARGUMENT_TYPE_POSITIVE || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE) {
				if(selected_argument) {
					delete selected_argument;
				}
				switch(menu_index) {
					case MENU_ARGUMENT_TYPE_TEXT: {selected_argument = new TextArgument(); break;}
					case MENU_ARGUMENT_TYPE_SYMBOLIC: {selected_argument = new SymbolicArgument(); break;}
					case MENU_ARGUMENT_TYPE_DATE: {selected_argument = new DateArgument(); break;}
					case MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER: {selected_argument = new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE); break;}
					case MENU_ARGUMENT_TYPE_POSITIVE_INTEGER: {selected_argument = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE); break;}
					case MENU_ARGUMENT_TYPE_NONZERO_INTEGER: {selected_argument = new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO); break;}
					case MENU_ARGUMENT_TYPE_INTEGER: {selected_argument = new IntegerArgument(); break;}
					case MENU_ARGUMENT_TYPE_NONNEGATIVE: {selected_argument = new NumberArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE); break;}
					case MENU_ARGUMENT_TYPE_POSITIVE: {selected_argument = new NumberArgument("", ARGUMENT_MIN_MAX_POSITIVE); break;}
					case MENU_ARGUMENT_TYPE_NONZERO: {selected_argument = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO); break;}
					case MENU_ARGUMENT_TYPE_NUMBER: {selected_argument = new NumberArgument(); break;}
					case MENU_ARGUMENT_TYPE_VECTOR: {selected_argument = new VectorArgument(); break;}
					case MENU_ARGUMENT_TYPE_MATRIX: {selected_argument = new MatrixArgument(); break;}
					case MENU_ARGUMENT_TYPE_EXPRESSION_ITEM: {selected_argument = new ExpressionItemArgument(); break;}
					case MENU_ARGUMENT_TYPE_FUNCTION: {selected_argument = new FunctionArgument(); break;}
					case MENU_ARGUMENT_TYPE_UNIT: {selected_argument = new UnitArgument(); break;}
					case MENU_ARGUMENT_TYPE_VARIABLE: {selected_argument = new VariableArgument(); break;}
					case MENU_ARGUMENT_TYPE_FILE: {selected_argument = new FileArgument(); break;}
					case MENU_ARGUMENT_TYPE_BOOLEAN: {selected_argument = new BooleanArgument(); break;}	
					case MENU_ARGUMENT_TYPE_ANGLE: {selected_argument = new AngleArgument(); break;}
					case MENU_ARGUMENT_TYPE_DATA_OBJECT: {selected_argument = new DataObjectArgument(NULL, ""); break;}
					case MENU_ARGUMENT_TYPE_DATA_PROPERTY: {selected_argument = new DataPropertyArgument(NULL, ""); break;}
					default: {selected_argument = new Argument();}
				}			
			}
			
		}
		selected_argument->setName(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_argument_name"))));
		gtk_list_store_set(tFunctionArguments_store, &iter, 0, selected_argument->name().c_str(), 1, selected_argument->printlong().c_str(), 2, (gpointer) selected_argument, -1);
	}
}
void on_function_edit_entry_argument_name_activate(GtkEntry*, gpointer) {
	if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_argument")))) {
		on_function_edit_button_add_argument_clicked(GTK_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_argument")), NULL);
	} else if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")))) {
		on_function_edit_button_modify_argument_clicked(GTK_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), NULL);
	}
}
void on_function_edit_button_rules_clicked(GtkButton*, gpointer) {
	edit_argument(get_selected_argument());
}
void on_argument_rules_checkbutton_enable_min_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), gtk_toggle_button_get_active(w));
}
void on_argument_rules_checkbutton_enable_max_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), gtk_toggle_button_get_active(w));
}
void on_argument_rules_checkbutton_enable_condition_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")), gtk_toggle_button_get_active(w));
}
#define SET_NAMES_LE(x,y,z)	GtkTreeIter iter;\
				string str;\
				if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {\
					gchar *gstr;\
					gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, -1);\
					if(strlen(gstr) > 0) {\
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(x, y)), gstr);\
					}\
					g_free(gstr);\
					if(gtk_tree_model_iter_next(GTK_TREE_MODEL(tNames_store), &iter)) {\
						str += "+ ";\
						while(true) {\
							gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, -1);\
							str += gstr;\
							g_free(gstr);\
							if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tNames_store), &iter)) break;\
							str += ", ";\
						}\
					}\
				}\
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(x, z)), str.c_str());
				
void on_variable_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(get_edited_variable(), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog")));
	SET_NAMES_LE(variableedit_builder, "variable_edit_entry_name", "variable_edit_label_names")
}
void on_unknown_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(get_edited_unknown(), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog")));
	SET_NAMES_LE(unknownedit_builder, "unknown_edit_entry_name", "unknown_edit_label_names")
}
void on_matrix_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(get_edited_matrix(), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog")));
	SET_NAMES_LE(matrixedit_builder, "matrix_edit_entry_name", "matrix_edit_label_names")
}
void on_function_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(get_edited_function(), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
	SET_NAMES_LE(functionedit_builder, "function_edit_entry_name", "function_edit_label_names")
}
void on_unit_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(get_edited_unit(), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog")));
	SET_NAMES_LE(unitedit_builder, "unit_edit_entry_name", "unit_edit_label_names")
}

void on_names_edit_checkbutton_abbreviation_toggled(GtkToggleButton *w, gpointer) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive")), gtk_toggle_button_get_active(w));
}
void on_names_edit_button_add_clicked(GtkButton*, gpointer) {
	if(strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")))) == 0) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")));
		show_message(_("Empty name field."), GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog")));
		return;
	}
	bool name_taken = false;
	if(editing_variable && CALCULATOR->variableNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_variable())) name_taken = true;
	else if(editing_unknown && CALCULATOR->variableNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_unknown())) name_taken = true;
	else if(editing_matrix && CALCULATOR->variableNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_matrix())) name_taken = true;
	else if(editing_unit && CALCULATOR->unitNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_unit())) name_taken = true;
	else if(editing_function && CALCULATOR->functionNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_function())) name_taken = true;
	else if(editing_dataset && CALCULATOR->functionNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_dataset())) name_taken = true;
	if(name_taken) {
		if(!ask_question(_("A conflicting object with the same name exists. If you proceed and save changes, the conflicting object will be overwritten or deactivated.\nDo you want to proceed?"), GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog")))) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")));
			return;
		}
	}
	GtkTreeIter iter;
	gtk_list_store_append(tNames_store, &iter);
	if(editing_dataproperty) gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), NAMES_ABBREVIATION_STRING_COLUMN, "-", NAMES_PLURAL_STRING_COLUMN, "-", NAMES_REFERENCE_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference")))), NAMES_ABBREVIATION_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation"))), NAMES_PLURAL_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural"))), NAMES_UNICODE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode"))), NAMES_REFERENCE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference"))), NAMES_SUFFIX_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix"))), NAMES_AVOID_INPUT_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input"))), NAMES_CASE_SENSITIVE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive"))), -1);
	else gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), NAMES_ABBREVIATION_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation")))), NAMES_PLURAL_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural")))), NAMES_REFERENCE_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference")))), NAMES_ABBREVIATION_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation"))), NAMES_PLURAL_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural"))), NAMES_UNICODE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode"))), NAMES_REFERENCE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference"))), NAMES_SUFFIX_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix"))), NAMES_AVOID_INPUT_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input"))), NAMES_CASE_SENSITIVE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive"))), -1);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")), "");
}
void on_names_edit_button_modify_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		char *gstr;
		gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, -1);
		if(strcmp(gstr, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")))) != 0) {
			bool name_taken = false;
			if(editing_variable && CALCULATOR->variableNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_variable())) name_taken = true;
			else if(editing_unknown && CALCULATOR->variableNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_unknown())) name_taken = true;
			else if(editing_matrix && CALCULATOR->variableNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_matrix())) name_taken = true;
			else if(editing_unit && CALCULATOR->unitNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_unit())) name_taken = true;
			else if(editing_function && CALCULATOR->functionNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_function())) name_taken = true;
			else if(editing_dataset && CALCULATOR->functionNameTaken(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), get_edited_dataset())) name_taken = true;
			if(name_taken) {
				if(!ask_question(_("A conflicting object with the same name exists. If you proceed and save changes, the conflicting object will be overwritten or deactivated.\nDo you want to proceed?"), GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog")))) {
					g_free(gstr);
					return;
				}
			}
		}
		g_free(gstr);
		if(editing_dataproperty) gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), NAMES_ABBREVIATION_STRING_COLUMN, "-", NAMES_PLURAL_STRING_COLUMN, "-", NAMES_REFERENCE_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference")))), NAMES_ABBREVIATION_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation"))), NAMES_PLURAL_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural"))), NAMES_UNICODE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode"))), NAMES_REFERENCE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference"))), NAMES_SUFFIX_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix"))), NAMES_AVOID_INPUT_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input"))), NAMES_CASE_SENSITIVE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive"))), -1);
		else gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name"))), NAMES_ABBREVIATION_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation")))), NAMES_PLURAL_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural")))), NAMES_REFERENCE_STRING_COLUMN, b2yn(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference")))), NAMES_ABBREVIATION_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_abbreviation"))), NAMES_PLURAL_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_plural"))), NAMES_UNICODE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_unicode"))), NAMES_REFERENCE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_reference"))), NAMES_SUFFIX_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_suffix"))), NAMES_AVOID_INPUT_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_avoid_input"))), NAMES_CASE_SENSITIVE_COLUMN, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_checkbutton_case_sensitive"))), -1);
	}
}
void on_names_edit_button_remove_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		gtk_list_store_remove(tNames_store, &iter);
	}
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(namesedit_builder, "names_edit_entry_name")), "");
}
void on_names_edit_entry_name_activate(GtkEntry*, gpointer) {
	if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_add")))) {
		on_names_edit_button_add_clicked(GTK_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_button_add")), NULL);
	} else if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")))) {
		on_names_edit_button_modify_clicked(GTK_BUTTON(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), NULL);
	}
}
void on_names_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	int etype = -1;
	if(editing_unit) etype = TYPE_UNIT;
	else if(editing_function || editing_dataset) etype = TYPE_FUNCTION;
	else if(!editing_dataproperty) etype = TYPE_VARIABLE;
	if(etype >= 0) correct_name_entry(editable, (ExpressionItemType) etype, (gpointer) on_names_edit_entry_name_changed);
}


bool generate_plot(PlotParameters &pp, vector<MathStructure> &y_vectors, vector<MathStructure> &x_vectors, vector<PlotDataParameters*> &pdps) {
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	if(!b) {
		return false;
	}	
	while(b) {
		int count = 1;
		gchar *gstr1, *gstr2;
		gint type = 0, style = 0, smoothing = 0, axis = 1, rows = 0;
		MathStructure *y_vector, *x_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 0, &gstr1, 1, &gstr2, 2, &style, 3, &smoothing, 4, &type, 5, &axis, 6, &rows, 7, &x_vector, 8, &y_vector, -1);
		if(type == 1) {
			if(y_vector->isMatrix()) {
				count = 0;
				if(rows) {
					for(size_t i = 1; i <= y_vector->rows(); i++) {
						y_vectors.push_back(m_undefined);
						y_vector->rowToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						count++;
					}
				} else {
					for(size_t i = 1; i <= y_vector->columns(); i++) {
						y_vectors.push_back(m_undefined);
						y_vector->columnToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						count++;
					}
				}
			} else if(y_vector->isVector()) {
				y_vectors.push_back(*y_vector);
				x_vectors.push_back(m_undefined);
			} else {
				y_vectors.push_back(*y_vector);
				y_vectors[y_vectors.size() - 1].transform(STRUCT_VECTOR);
				x_vectors.push_back(m_undefined);
			}
		} else if(type == 2) {
			if(y_vector->isMatrix()) {
				count = 0;
				if(rows) {
					for(size_t i = 1; i <= y_vector->rows(); i += 2) {
						y_vectors.push_back(m_undefined);
						y_vector->rowToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						y_vector->rowToVector(i + 1, x_vectors[x_vectors.size() - 1]);
						count++;
					}
				} else {
					for(size_t i = 1; i <= y_vector->columns(); i += 2) {
						y_vectors.push_back(m_undefined);
						y_vector->columnToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						y_vector->columnToVector(i + 1, x_vectors[x_vectors.size() - 1]);
						count++;
					}
				}
			} else if(y_vector->isVector()) {
				y_vectors.push_back(*y_vector);
				x_vectors.push_back(m_undefined);
			} else {
				y_vectors.push_back(*y_vector);
				y_vectors[y_vectors.size() - 1].transform(STRUCT_VECTOR);
				x_vectors.push_back(m_undefined);
			}
		} else {
			y_vectors.push_back(*y_vector);
			x_vectors.push_back(*x_vector);
		}
		for(int i = 0; i < count; i++) {
			PlotDataParameters *pdp = new PlotDataParameters();
			pdp->title = gstr1;
			if(count > 1) {
				pdp->title += " :";
				pdp->title += i2s(i + 1);
			}
			remove_blank_ends(pdp->title);
			if(pdp->title.empty()) {
				pdp->title = gstr2;
			}
			switch(smoothing) {
				case SMOOTHING_MENU_NONE: {pdp->smoothing = PLOT_SMOOTHING_NONE; break;}
				case SMOOTHING_MENU_UNIQUE: {pdp->smoothing = PLOT_SMOOTHING_UNIQUE; break;}
				case SMOOTHING_MENU_CSPLINES: {pdp->smoothing = PLOT_SMOOTHING_CSPLINES; break;}
				case SMOOTHING_MENU_BEZIER: {pdp->smoothing = PLOT_SMOOTHING_BEZIER; break;}
				case SMOOTHING_MENU_SBEZIER: {pdp->smoothing = PLOT_SMOOTHING_SBEZIER; break;}
			}
			switch(style) {
				case PLOTSTYLE_MENU_LINES: {pdp->style = PLOT_STYLE_LINES; break;}
				case PLOTSTYLE_MENU_POINTS: {pdp->style = PLOT_STYLE_POINTS; break;}
				case PLOTSTYLE_MENU_LINESPOINTS: {pdp->style = PLOT_STYLE_POINTS_LINES; break;}
				case PLOTSTYLE_MENU_DOTS: {pdp->style = PLOT_STYLE_DOTS; break;}
				case PLOTSTYLE_MENU_BOXES: {pdp->style = PLOT_STYLE_BOXES; break;}
				case PLOTSTYLE_MENU_HISTEPS: {pdp->style = PLOT_STYLE_HISTOGRAM; break;}
				case PLOTSTYLE_MENU_STEPS: {pdp->style = PLOT_STYLE_STEPS; break;}
				case PLOTSTYLE_MENU_CANDLESTICKS: {pdp->style = PLOT_STYLE_CANDLESTICKS; break;}
			}
			pdp->yaxis2 = (axis == 2);
			pdps.push_back(pdp);
		}
		g_free(gstr1);
		g_free(gstr2);
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")))) {
		case PLOTLEGEND_MENU_NONE: {pp.legend_placement = PLOT_LEGEND_NONE; break;}
		case PLOTLEGEND_MENU_TOP_LEFT: {pp.legend_placement = PLOT_LEGEND_TOP_LEFT; break;}
		case PLOTLEGEND_MENU_TOP_RIGHT: {pp.legend_placement = PLOT_LEGEND_TOP_RIGHT; break;}
		case PLOTLEGEND_MENU_BOTTOM_LEFT: {pp.legend_placement = PLOT_LEGEND_BOTTOM_LEFT; break;}
		case PLOTLEGEND_MENU_BOTTOM_RIGHT: {pp.legend_placement = PLOT_LEGEND_BOTTOM_RIGHT; break;}
		case PLOTLEGEND_MENU_BELOW: {pp.legend_placement = PLOT_LEGEND_BELOW; break;}
		case PLOTLEGEND_MENU_OUTSIDE: {pp.legend_placement = PLOT_LEGEND_OUTSIDE; break;}
	}
	pp.title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_plottitle")));
	pp.x_label = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_xlabel")));
	pp.y_label = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_ylabel")));
	pp.grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_grid")));
	pp.x_log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_xlog")));
	pp.y_log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_ylog")));
	pp.x_log_base = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_xlog_base")));
	pp.y_log_base = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_ylog_base")));
	pp.color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_color")));
	pp.show_all_borders = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_full_border")));
	return true;
}
void on_plot_button_help_clicked(GtkButton, gpointer) {
	show_help("qalculate-plotting.html", gtk_builder_get_object(plot_builder, "plot_dialog")); 
}
void on_plot_button_save_clicked(GtkButton*, gpointer) {
	GtkWidget *d;
	d = gtk_file_chooser_dialog_new(_("Select file to export"), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Allowed File Types"));
	gtk_file_filter_add_mime_type(filter, "image/x-xfig");
	gtk_file_filter_add_mime_type(filter, "image/svg");
	gtk_file_filter_add_mime_type(filter, "text/x-tex");
	gtk_file_filter_add_mime_type(filter, "application/pdf");
	gtk_file_filter_add_mime_type(filter, "application/postscript");
	gtk_file_filter_add_mime_type(filter, "image/x-eps");
	gtk_file_filter_add_mime_type(filter, "image/png");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter);
	GtkFileFilter *filter_all = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter_all, "*");
	gtk_file_filter_set_name(filter_all, _("All Files"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter_all);
	string title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_plottitle")));
	if(title.empty()) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(d), "plot.png");
	} else {
		title += ".png";
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(d), title.c_str());
	}
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
		vector<MathStructure> y_vectors;
		vector<MathStructure> x_vectors;
		vector<PlotDataParameters*> pdps;
		PlotParameters pp;
		if(generate_plot(pp, y_vectors, x_vectors, pdps)) {
			pp.filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
			pp.filetype = PLOT_FILETYPE_AUTO;
			do_timeout = false;
			CALCULATOR->plotVectors(&pp, y_vectors, x_vectors, pdps);
			display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
			do_timeout = true;
			for(size_t i = 0; i < pdps.size(); i++) {
				if(pdps[i]) delete pdps[i];
			}
		}
	}
	gtk_widget_destroy(d);
}
void update_plot() {
	vector<MathStructure> y_vectors;
	vector<MathStructure> x_vectors;
	vector<PlotDataParameters*> pdps;
	PlotParameters pp;
	if(!generate_plot(pp, y_vectors, x_vectors, pdps)) {
		CALCULATOR->closeGnuplot();
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), false);
		return;
	}
	do_timeout = false;
	CALCULATOR->plotVectors(&pp, y_vectors, x_vectors, pdps);
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
	do_timeout = true;
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), true);
	for(size_t i = 0; i < pdps.size(); i++) {
		if(pdps[i]) delete pdps[i];
	}
}

void generate_plot_series(MathStructure **x_vector, MathStructure **y_vector, int type, string str, string str_x) {
	EvaluationOptions eo;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	eo.parse_options = evalops.parse_options;
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	do_timeout = false;
	if(type == 1 || type == 2) {
		*y_vector = new MathStructure();
		if(!CALCULATOR->calculate(*y_vector, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 5000, eo)) {
			GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("It took too long to generate the plot data."));
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
		}
		*x_vector = NULL;
	} else {
		*x_vector = new MathStructure();
		(*x_vector)->clearVector();
		MathStructure min;
		if(!CALCULATOR->calculate(&min, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_min"))), evalops.parse_options), 1000, eo)) {
			GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("It took too long to generate the plot data."));
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
			display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
			do_timeout = true;
			return;
		}
		MathStructure max;
		if(!CALCULATOR->calculate(&max, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_max"))), evalops.parse_options), 1000, eo)) {
			GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("It took too long to generate the plot data."));
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
			display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
			do_timeout = true;
			return;
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_step")))) {
			*y_vector = new MathStructure(CALCULATOR->expressionToPlotVector(str, min, max, CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_step"))), evalops.parse_options), eo), *x_vector, str_x, evalops.parse_options, 5000));
		} else {
			*y_vector = new MathStructure(CALCULATOR->expressionToPlotVector(str, min, max, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps"))), *x_vector, str_x, evalops.parse_options, 5000));
		}
	}
	display_errors(NULL, GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
	do_timeout = true;
}
void on_plot_button_add_clicked(GtkButton*, gpointer) {
	string expression = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
	if(expression.find_first_not_of(SPACES) == string::npos) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
		show_message(_("Empty expression."), GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
		return;
	}
	gint type = 0, axis = 1, rows = 0;
	string title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")))) {
		type = 1;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")))) {
		type = 2;
	}
	string str_x = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
	remove_blank_ends(str_x);
	if(str_x.empty() && type == 0) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
		show_message(_("Empty x variable."), GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
		return;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis2")))) {
		axis = 2;
	}
	if((type == 1 || type == 2) && title.empty()) {
		Variable *v = CALCULATOR->getActiveVariable(expression);
		if(v) {
			title = v->title(false);
		}
	}
	MathStructure *x_vector, *y_vector;
	generate_plot_series(&x_vector, &y_vector, type, expression, str_x);
	rows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")));
	GtkTreeIter iter;	
	gtk_list_store_append(tPlotFunctions_store, &iter);
	gtk_list_store_set(tPlotFunctions_store, &iter, 0, title.c_str(), 1, expression.c_str(), 2, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style"))), 3, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing"))), 4, type, 5, axis, 6, rows, 7, x_vector, 8, y_vector, 9, str_x.c_str(), -1);
	gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions)), &iter);
	update_plot();
}
void on_plot_button_modify_clicked(GtkButton*, gpointer) {

	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {	
		string expression = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
		if(expression.find_first_not_of(SPACES) == string::npos) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
			show_message(_("Empty expression."), GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
			return;
		}
		gint type = 0, axis = 1, rows = 0;
		string title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")))) {
			type = 1;
		} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")))) {
			type = 2;
		}
		string str_x = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
		remove_blank_ends(str_x);
		if(str_x.empty() && type == 0) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
			show_message(_("Empty x variable."), GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
			return;
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis2")))) {
			axis = 2;
		}
		if((type == 1 || type == 2) && title.empty()) {
			Variable *v = CALCULATOR->getActiveVariable(expression);
			if(v) {
				title = v->title(false);
			}
		}
		MathStructure *x_vector, *y_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 7, &x_vector, 8, &y_vector, -1);
		if(x_vector) delete x_vector;
		if(y_vector) delete y_vector;
		x_vector = NULL;
		y_vector = NULL;
		generate_plot_series(&x_vector, &y_vector, type, expression, str_x);
		rows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")));
		gtk_list_store_set(tPlotFunctions_store, &iter, 0, title.c_str(), 1, expression.c_str(), 2, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style"))), 3, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing"))), 4, type, 5, axis, 6, rows, 7, x_vector, 8, y_vector, 9, str_x.c_str(), -1);
		update_plot();
	}
}
void on_plot_button_remove_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		MathStructure *x_vector, *y_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 7, &x_vector, 8, &y_vector, -1);
		if(x_vector) delete x_vector;
		if(y_vector) delete y_vector;
		gtk_list_store_remove(tPlotFunctions_store, &iter);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")), "");	
		update_plot();
	}
}

void on_plot_checkbutton_xlog_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_xlog_base")), gtk_toggle_button_get_active(w));
}
void on_plot_checkbutton_ylog_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_ylog_base")), gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_step_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_step")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")), !gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_steps_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_step")), !gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")), gtk_toggle_button_get_active(w));
}
void on_plot_entry_expression_activate(GtkEntry*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		on_plot_button_modify_clicked(GTK_BUTTON(gtk_builder_get_object(plot_builder, "plot_button_modify")), NULL);
	} else {
		on_plot_button_add_clicked(GTK_BUTTON(gtk_builder_get_object(plot_builder, "plot_button_add")), NULL);
	}
}

void on_plot_radiobutton_function_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), !gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_vector_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), !gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_paired_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), !gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), gtk_toggle_button_get_active(w));
}
void on_plot_button_range_apply_clicked(GtkButton*, gpointer) {
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	while(b) {
		gchar *gstr2, *gstr3;
		gint type = 0;
		MathStructure *y_vector, *x_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 1, &gstr2, 4, &type, 7, &x_vector, 8, &y_vector, 9, &gstr3, -1);
		if(y_vector) delete y_vector;
		if(x_vector) delete x_vector;
		x_vector = NULL;
		y_vector = NULL;
		generate_plot_series(&x_vector, &y_vector, type, gstr2, gstr3);
		g_free(gstr2);
		g_free(gstr3);
		gtk_list_store_set(tPlotFunctions_store, &iter, 7, x_vector, 8, y_vector, -1);
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	}
	update_plot();
}
void on_plot_button_appearance_apply_clicked(GtkButton*, gpointer) {
	update_plot();
}

void convert_from_convert_entry_unit() {
	do_timeout = false;
	string ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), evalops.parse_options);
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {
		remove_blank_ends(ceu_str);
		if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-') {
			ceu_str = "?" + ceu_str;
		}
	}
	bool b_puup = printops.use_unit_prefixes;
	printops.use_unit_prefixes = true;
	executeCommand(COMMAND_CONVERT_STRING, true, ceu_str);
	printops.use_unit_prefixes = b_puup;
	do_timeout = true;
}
void on_convert_button_set_missing_prefixes_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))) != 0) {
		convert_from_convert_entry_unit();
	}
}
void on_convert_button_convert_clicked(GtkButton*, gpointer) {
	convert_from_convert_entry_unit();
	focus_keeping_selection();
}
void on_convert_entry_unit_activate(GtkEntry*, gpointer) {
	convert_from_convert_entry_unit();
	focus_keeping_selection();
}


vector<GtkWidget*> ewindows;
vector<DataObject*> eobjects;

void on_element_button_function_clicked(GtkButton *w, gpointer user_data) {
	DataProperty *dp = (DataProperty*) user_data;
	DataSet *ds = NULL;
	DataObject *o = NULL;
	GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(w));
	for(size_t i = 0; i < ewindows.size(); i++) {
		if(ewindows[i] == win) {
			o = eobjects[i];
			break;
		}
	}
	if(dp) ds = dp->parentSet();
	if(ds && o) {
		string str = ds->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name;
		str += "(";
		str += o->getProperty(ds->getPrimaryKeyProperty());
		str += CALCULATOR->getComma();
		str += " ";
		str += dp->getName();
		str += ")";
		insert_text(str.c_str());
	}
}
void on_element_button_close_clicked(GtkButton *w, gpointer user_data) {
	GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(w));
	for(size_t i = 0; i < ewindows.size(); i++) {
		if(ewindows[i] == win) {
			ewindows.erase(ewindows.begin() + i);
			eobjects.erase(eobjects.begin() + i);
			break;
		}
	}
	gtk_widget_destroy((GtkWidget*) user_data);
}
void on_element_button_clicked(GtkButton*, gpointer user_data) {
	DataObject *e = (DataObject*) user_data;
	if(e) {
		DataSet *ds = e->parentSet();
		if(!ds) return;
		GtkWidget *dialog = gtk_dialog_new();
		ewindows.push_back(dialog);
		eobjects.push_back(e);
		GtkWidget *close_button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Close"), GTK_RESPONSE_CLOSE);
		g_signal_connect((gpointer) close_button, "clicked", G_CALLBACK(on_element_button_close_clicked), (gpointer) dialog);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(periodictable_builder, "periodic_dialog")));
		gtk_window_set_title(GTK_WINDOW(dialog), "Element Data");
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);
		
		GtkWidget *vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, TRUE, 0);
		
		DataProperty *p_number = ds->getProperty("number");
		DataProperty *p_symbol = ds->getProperty("symbol");
		DataProperty *p_class = ds->getProperty("class");
		DataProperty *p_name = ds->getProperty("name");

		GtkWidget *label;
		label = gtk_label_new(NULL);
		string str = "<span size=\"large\">"; str += e->getProperty(p_number); str += "</span>";
		gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_END); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 0);
		label = gtk_label_new(NULL);
		str = "<span size=\"xx-large\">"; str += e->getProperty(p_symbol); str += "</span>";
		gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 0);
		label = gtk_label_new(NULL);
		str = "<span size=\"x-large\">"; str += e->getProperty(p_name); str += "</span>  "; 
		gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 0);

		GtkWidget *button;
		GtkWidget *ptable = gtk_grid_new();
		gtk_grid_set_column_spacing(GTK_GRID(ptable), 6);
		gtk_box_pack_start(GTK_BOX(vbox), ptable, FALSE, TRUE, 0);
		int rows = 0;

		int group = s2i(e->getProperty(p_class));
		if(group > 0) {
			rows++;
			label = gtk_label_new(NULL);
			str = "<span weight=\"bold\">"; str += _("Classification"); str += ":"; str += "</span>";
			gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), FALSE);
			gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);
			label = gtk_label_new(NULL);
			switch(group) {
				case ALKALI_METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Alkali Metal")); break;}
				case ALKALI_EARTH_METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Alkaline-Earth Metal")); break;}
				case LANTHANIDES: {gtk_label_set_markup(GTK_LABEL(label), _("Lanthanide")); break;}
				case ACTINIDES: {gtk_label_set_markup(GTK_LABEL(label), _("Actinide")); break;}
				case TRANSITION_METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Transition Metal")); break;}
				case METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Metal")); break;}
				case METALLOIDS: {gtk_label_set_markup(GTK_LABEL(label), _("Metalloid")); break;}
				case NONMETALS: {gtk_label_set_markup(GTK_LABEL(label), _("Polyatomic Non-Metal")); break;}
				case HALOGENS: {gtk_label_set_markup(GTK_LABEL(label), _("Diatomic Non-Metal")); break;}
				case NOBLE_GASES: {gtk_label_set_markup(GTK_LABEL(label), _("Noble Gas")); break;}
				case TRANSACTINIDES: {gtk_label_set_markup(GTK_LABEL(label), _("Unknown chemical properties")); break;}
				default: {gtk_label_set_markup(GTK_LABEL(label), _("Unknown")); break;}
			}
			gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
			gtk_widget_set_margin_end(label, 10);
			gtk_grid_attach(GTK_GRID(ptable), label, 1, rows - 1, 1, 1);
		}
		
		DataPropertyIter it;
		DataProperty *dp = ds->getFirstProperty(&it);
		string sval;
		while(dp) {
			if(!dp->isHidden() && dp != p_number && dp != p_class && dp != p_symbol && dp != p_name) {
				sval = e->getPropertyDisplayString(dp);
				if(!sval.empty()) {
					rows++;
					label = gtk_label_new(NULL);
					str = "<span weight=\"bold\">"; str += dp->title(); str += ":"; str += "</span>";
					gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), FALSE);
					gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);
					label = gtk_label_new(NULL);
					gtk_label_set_markup(GTK_LABEL(label), sval.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
					gtk_grid_attach(GTK_GRID(ptable), label, 1, rows - 1, 1, 1);
					button = gtk_button_new();
					gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("edit-paste", GTK_ICON_SIZE_BUTTON));
					gtk_grid_attach(GTK_GRID(ptable), button, 2, rows - 1, 1, 1);
					g_signal_connect((gpointer) button, "clicked", G_CALLBACK(on_element_button_function_clicked), (gpointer) dp);
				}
			}
			dp = ds->getNextProperty(&it);
		}
		
		gtk_widget_show_all(dialog);
		
	}
}

void on_dataset_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_FUNCTION,  (gpointer) on_dataset_edit_entry_name_changed);
}
void on_dataset_edit_button_new_property_clicked(GtkButton*, gpointer) {
	DataProperty *dp = new DataProperty(edited_dataset);
	dp->setUserModified(true);
	if(edit_dataproperty(dp)) {
		tmp_props.push_back(dp);
		tmp_props_orig.push_back(NULL);
		update_dataset_property_list(edited_dataset);
	} else {
		delete dp;
	}
}
void on_dataset_edit_button_edit_property_clicked(GtkButton*, gpointer) {
	if(selected_dataproperty) {
		if(edit_dataproperty(selected_dataproperty)) {
			update_dataset_property_list(edited_dataset);
		}
	}
}
void on_dataset_edit_button_del_property_clicked(GtkButton*, gpointer) {
	if(edited_dataset && selected_dataproperty && selected_dataproperty->isUserModified()) {
		for(size_t i = 0; i < tmp_props.size(); i++) {
			if(tmp_props[i] == selected_dataproperty) {
				if(tmp_props_orig[i]) {
					tmp_props[i] = NULL;
				} else {
					tmp_props.erase(tmp_props.begin() + i);
					tmp_props_orig.erase(tmp_props_orig.begin() + i);
				}
				break;
			}
		}
		update_dataset_property_list(edited_dataset);
	}
}
void on_dataset_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(get_edited_dataset(), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog")));
	SET_NAMES_LE(datasetedit_builder, "dataset_edit_entry_name", "dataset_edit_label_names")
}

void on_dataproperty_edit_button_names_clicked(GtkButton*, gpointer) {
	edit_names(NULL, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name"))), GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_dialog")), TRUE, get_edited_dataproperty());
	SET_NAMES_LE(datasetedit_builder, "dataproperty_edit_entry_name", "dataproperty_edit_label_names")
}

void on_menu_item_set_unknowns_activate(GtkMenuItem*, gpointer) {
	if(expression_has_changed && !rpn_mode) execute_expression(true);
	MathStructure unknowns;
	mstruct->findAllUnknowns(unknowns);
	if(unknowns.size() == 0) {
		show_message(_("No unknowns in result."), GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));
		return;
	}
	unknowns.setType(STRUCT_ADDITION);
	unknowns.sort();
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Set Unknowns"), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, _("_Apply"), GTK_RESPONSE_APPLY, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);
	GtkWidget *label;
	vector<GtkWidget*> entry;
	entry.resize(unknowns.size(), NULL);
	GtkWidget *ptable = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(ptable), 6);
	gtk_grid_set_row_spacing(GTK_GRID(ptable), 6);
	gtk_box_pack_start(GTK_BOX(vbox), ptable, FALSE, TRUE, 0);
	int rows = 0;
	for(size_t i = 0; i < unknowns.size(); i++) {
		rows++;
		label = gtk_label_new(unknowns[i].print().c_str());
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);
		entry[i] = gtk_entry_new();
		gtk_widget_set_hexpand(entry[i], TRUE);
		gtk_grid_attach(GTK_GRID(ptable), entry[i], 1, rows - 1, 1, 1);
	}
	MathStructure msave(*mstruct);
	string result_save = result_text;
	gtk_widget_show_all(dialog);
	bool b_changed = false;
	vector<string> unknown_text;
	unknown_text.resize(unknowns.size());
	while(true) {
		gint response = gtk_dialog_run(GTK_DIALOG(dialog));
		bool b1 = false, b2 = false;
		if(response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_APPLY) {
			string str, result_mod = "";
			do_timeout = false;
			for(size_t i = 0; i < unknowns.size(); i++) {
				str = gtk_entry_get_text(GTK_ENTRY(entry[i]));
				remove_blank_ends(str);
				if(((b1 || !b_changed) && !str.empty()) || (b_changed && unknown_text[i] != str)) {
					if(!result_mod.empty()) {
						result_mod += CALCULATOR->getComma();
						result_mod += " ";
					} else {
						b1 = true;
						mstruct->set(msave);
						for(size_t i2 = 0; i2 < i; i2++) {
							if(!unknown_text[i2].empty()) {
								mstruct->replace(unknowns[i2], CALCULATOR->parse(CALCULATOR->unlocalizeExpression(unknown_text[i2], evalops.parse_options), evalops.parse_options));
								b2 = true;
							}
						}
					}
					result_mod += unknowns[i].print().c_str();
					result_mod += "=";
					if(str.empty()) {
						result_mod += "?";
					} else {
						result_mod += str;
						mstruct->replace(unknowns[i], CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options));
						b2 = true;
					}
					unknown_text[i] = str;
				}
			}
			if(response == GTK_RESPONSE_ACCEPT) {
				gtk_widget_destroy(dialog);
			}
			if(b2) {
				b_changed = true;
				if(response != GTK_RESPONSE_ACCEPT) {
					gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
					gtk_widget_set_sensitive(GTK_WIDGET(dialog), FALSE);
				}
				executeCommand(COMMAND_TRANSFORM, true, result_mod);
			} else if(b1) {
				b_changed = false;
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
				setResult(NULL, true, false, false, result_mod);
			}
			do_timeout = true;
			if(response == GTK_RESPONSE_ACCEPT) {
				break;
			}
			gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(dialog), TRUE);
		} else {
			if(b_changed && response == GTK_RESPONSE_REJECT) {
				string result_mod = "";
				mstruct->set(msave);
				for(size_t i = 0; i < unknowns.size(); i++) {
					if(!unknown_text[i].empty()) {
						if(!result_mod.empty()) {
							result_mod += CALCULATOR->getComma();
							result_mod += " ";
						}
						result_mod += unknowns[i].print().c_str();
						result_mod += "=";
						result_mod += "?";
					}
				}
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
				setResult(NULL, true, false, false, result_mod);
			}
			gtk_widget_destroy(dialog);
			break;
		}
	}
}



#ifdef __cplusplus
}
#endif

