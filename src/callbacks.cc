/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#ifdef USE_WEBKITGTK
#	include <webkit2/webkit2.h>
#endif
#include <sys/stat.h>
#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <time.h>
#include <limits>
#include <fstream>
#include <sstream>

#include "support.h"
#include "callbacks.h"
#include "interface.h"
#include "settings.h"
#include "util.h"
#include <stack>
#include <deque>

#define VERSION_BEFORE(i1, i2, i3) (version_numbers[0] < i1 || (version_numbers[0] == i1 && (version_numbers[1] < i2 || (version_numbers[1] == i2 && version_numbers[2] < i3))))
#define VERSION_AFTER(i1, i2, i3) (version_numbers[0] > i1 || (version_numbers[0] == i1 && (version_numbers[1] > i2 || (version_numbers[1] == i2 && version_numbers[2] > i3))))

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::iterator;
using std::list;
using std::ifstream;
using std::ofstream;
using std::deque;
using std::stack;

int block_error_timeout = 0;

extern GtkBuilder *main_builder;

extern GtkWidget *mainwindow;

extern GtkWidget *tabs, *expander_keypad, *expander_history, *expander_stack, *expander_convert;
extern GtkEntryCompletion *completion;
extern GtkWidget *completion_view, *completion_window, *completion_scrolled;

extern GtkCssProvider *resultview_provider, *statuslabel_l_provider, *statuslabel_r_provider, *app_provider, *statusframe_provider, *color_provider;

extern GtkWidget *expressiontext, *statuslabel_l, *statuslabel_r, *keypad;
extern GtkTextBuffer *expressionbuffer;
extern KnownVariable *vans[5], *v_memory;
extern GtkAccelGroup *accel_group;
extern string selected_function_category;
extern MathFunction *selected_function;
extern string selected_variable_category;
extern Variable *selected_variable;
extern string selected_unit_category;
extern Unit *selected_unit;
extern DataSet *selected_dataset;
bool save_mode_on_exit;
bool save_defs_on_exit;
bool clear_history_on_exit = false;
int max_history_lines = 300;
bool save_history_separately = false;
extern int history_expression_type;
int gtk_theme = -1;
bool use_custom_result_font, use_custom_expression_font, use_custom_status_font, use_custom_keypad_font, use_custom_app_font, use_custom_history_font;
bool save_custom_result_font = false, save_custom_expression_font = false, save_custom_status_font = false, save_custom_keypad_font = false, save_custom_app_font = false, save_custom_history_font = false;
string custom_result_font, custom_expression_font, custom_status_font, custom_keypad_font, custom_app_font, custom_history_font;
int scale_n = 0;
bool hyp_is_on, inv_is_on;
bool show_keypad, show_history, show_stack, show_convert, persistent_keypad, minimal_mode;
extern bool continuous_conversion, set_missing_prefixes;
extern bool show_bases_keypad;
bool copy_ascii = false;
bool copy_ascii_without_units = false;
bool caret_as_xor = false;
int close_with_esc = -1;
extern bool load_global_defs, fetch_exchange_rates_at_startup, first_time, showing_first_time_message;
extern int allow_multiple_instances;
int b_decimal_comma;
int auto_update_exchange_rates;
bool first_error;
bool display_expression_status;
extern MathStructure *mstruct, *matrix_mstruct, *parsed_mstruct, *parsed_tostruct, *displayed_mstruct;
MathStructure *displayed_parsed_mstruct = NULL;
MathStructure mbak_convert;
extern string result_text, parsed_text;
bool result_text_approximate = false;
string result_text_long;
extern GtkWidget *resultview;
extern GtkWidget *historyview;
extern cairo_surface_t *surface_result;
cairo_surface_t *surface_parsed = NULL;
vector<vector<GtkWidget*> > insert_element_entries;
bool b_busy, b_busy_command, b_busy_result, b_busy_expression, b_busy_fetch;
cairo_surface_t *tmp_surface;
int block_result_update = 0, block_expression_execution = 0, block_display_parse = 0;
string parsed_expression, parsed_expression_tooltip;
bool parsed_had_errors = false, parsed_had_warnings = false;
extern int visible_keypad, previous_keypad;
extern int programming_inbase, programming_outbase;
bool title_modified = false;
string current_mode;
extern int horizontal_button_padding, vertical_button_padding;
int simplified_percentage = -1;
int version_numbers[3];

extern bool cursor_has_moved;

bool expression_has_changed2 = false;

int enable_tooltips = 1;
bool toe_changed = false;

string prev_output_base, prev_input_base;

int previous_precision = 0;

string custom_angle_unit;

string command_convert_units_string;
Unit *command_convert_unit;

extern GtkAccelGroup *accel_group;

extern gint win_height, win_width, win_x, win_y, win_monitor, history_height, variables_width, variables_height, variables_hposition, variables_vposition, units_width, units_height, units_hposition, units_vposition, functions_width, functions_height, functions_hposition, functions_vposition, datasets_width, datasets_height, datasets_hposition, datasets_vposition1, datasets_vposition2, hidden_x, hidden_y, hidden_monitor;
extern bool win_monitor_primary, hidden_monitor_primary;
bool remember_position = false, always_on_top = false, aot_changed = false;

gint minimal_width;

extern vector<string> expression_history;
extern string current_history_expression;
extern int expression_history_index;
extern bool dont_change_index;

bool result_font_updated = false;
bool first_draw_of_result = true;

extern PlotLegendPlacement default_plot_legend_placement;
extern bool default_plot_display_grid;
extern bool default_plot_full_border;
extern std::string default_plot_min;
extern std::string default_plot_max;
extern std::string default_plot_step;
extern int default_plot_sampling_rate;
extern int default_plot_linewidth;
extern int default_plot_complex;
extern bool default_plot_use_sampling_rate;
extern bool default_plot_rows;
extern int default_plot_type;
extern PlotStyle default_plot_style;
extern PlotSmoothing default_plot_smoothing;
extern std::string default_plot_variable;
extern bool default_plot_color;
extern int max_plot_time;

string status_error_color, status_warning_color, text_color;

bool stop_timeouts = false;
size_t current_function_index = 0;
MathFunction *current_function = NULL;

PrintOptions printops, parsed_printops, displayed_printops;
bool displayed_caf = false;
EvaluationOptions evalops;
bool dot_question_asked = false, implicit_question_asked = false;

bool rpn_mode, rpn_keys;
bool adaptive_interval_display;

bool tc_set = false, sinc_set = false;

bool use_systray_icon = false, hide_on_startup = false;

extern Thread *view_thread, *command_thread;
bool exit_in_progress = false, command_aborted = false, display_aborted = false, result_too_long = false, result_display_overflow = false;

vector<mode_struct> modes;

extern deque<string> inhistory;
extern deque<bool> inhistory_protected;
extern deque<int> inhistory_type;
extern deque<int> inhistory_value;
extern deque<time_t> inhistory_time;
extern int unformatted_history;

extern int initial_inhistory_index;

extern int expression_lines;

unordered_map<void*, string> date_map;
unordered_map<void*, bool> date_approx_map;
unordered_map<void*, string> number_map;
unordered_map<void*, string> number_base_map;
unordered_map<void*, bool> number_approx_map;
unordered_map<void*, string> number_exp_map;
unordered_map<void*, bool> number_exp_minus_map;

bool status_error_color_set;
bool status_warning_color_set;
bool text_color_set;

extern QalculateDateTime last_version_check_date;
string last_found_version;

bool keep_function_dialog_open = false;

bool automatic_fraction = false;
int default_fraction_fraction = -1;
bool scientific_negexp = true;
bool scientific_notminuslast = true;
bool scientific_noprefix = true;
int auto_prefix = 0;
bool fraction_fixed_combined = true;

bool ignore_locale = false;
string custom_lang;

int default_signed = -1;
int default_bits = -1;

extern vector<string> history_bookmarks;
extern unordered_map<string, size_t> history_bookmark_titles;

extern bool versatile_exact;

bool auto_calculate = false;
bool result_autocalculated = false;
gint autocalc_history_timeout_id = 0;
int autocalc_history_delay = 2000;
bool chain_mode = false;

bool parsed_in_result = false, show_parsed_instead_of_result = false;

int to_fraction = 0;
long int to_fixed_fraction = 0;
char to_prefix = 0;
int to_base = 0;
bool to_duo_syms = false;
int to_caf = -1;
unsigned int to_bits = 0;
Number to_nbase;

extern bool do_imaginary_j;
bool complex_angle_form = false;

extern Unit *latest_button_unit, *latest_button_currency;
extern string latest_button_unit_pre, latest_button_currency_pre;

bool default_shortcuts;

extern bool check_version;

PangoLayout *status_layout = NULL;

vector<PangoRectangle> binary_rect;
vector<int> binary_pos;
int binary_x_diff = 0;
int binary_y_diff = 0;

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z))
#define EQUALS_IGNORECASE_AND_LOCAL_NR(x,y,z,a)	(equalsIgnoreCase(x, y a) || (x.length() == strlen(z) + strlen(a) && equalsIgnoreCase(x.substr(0, x.length() - strlen(a)), z) && equalsIgnoreCase(x.substr(x.length() - strlen(a)), a)))

#define TEXT_TAGS			"<span size=\"xx-large\">"
#define TEXT_TAGS_END			"</span>"
#define TEXT_TAGS_SMALL			"<span size=\"large\">"
#define TEXT_TAGS_SMALL_END		"</span>"
#define TEXT_TAGS_XSMALL		"<span size=\"medium\">"
#define TEXT_TAGS_XSMALL_END		"</span>"

#define TTB(str)			if(scaledown <= 0) {str += "<span size=\"xx-large\">";} else if(scaledown == 1) {str += "<span size=\"x-large\">";} else if(scaledown == 2) {str += "<span size=\"large\">";} else {str += "<span size=\"medium\">";}
#define TTB_SMALL(str)			if(scaledown <= 0) {str += "<span size=\"large\">";} else if(scaledown == 1) {str += "<span size=\"medium\">";} else if(scaledown == 2) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";}
#define TTB_XSMALL(str)			if(scaledown <= 0) {str += "<span size=\"medium\">";} else if(scaledown == 1) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";}
#define TTB_XXSMALL(str)		if(scaledown <= 0) {str += "<span size=\"x-small\">";} else if(scaledown == 1) {str += "<span size=\"xx-small\">";} else {str += "<span size=\"xx-small\">";}
#define TTBP(str)			if(ips.power_depth > 1) {TTB_XSMALL(str);} else if(ips.power_depth > 0) {TTB_SMALL(str);} else {TTB(str);}
#define TTBP_SMALL(str)			if(ips.power_depth > 0) {TTB_XSMALL(str);} else {TTB_SMALL(str);}
#define TTE(str)			str += "</span>";
#define TT(str, x)			{if(scaledown <= 0) {str += "<span size=\"xx-large\">";} else if(scaledown == 1) {str += "<span size=\"x-large\">";} else if(scaledown == 2) {str += "<span size=\"large\">";} else {str += "<span size=\"medium\">";} str += x; str += "</span>";}
#define TT_SMALL(str, x)		{if(scaledown <= 0) {str += "<span size=\"large\">";} else if(scaledown == 1) {str += "<span size=\"medium\">";} else if(scaledown == 2) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";} str += x; str += "</span>";}
#define TT_XSMALL(str, x)		{if(scaledown <= 0) {str += "<span size=\"medium\">";} else if(scaledown == 1) {str += "<span size=\"small\">";} else {str += "<span size=\"x-small\">";} str += x; str += "</span>";}
#define TTP(str, x)			if(ips.power_depth > 1) {TT_XSMALL(str, x);} else if(ips.power_depth > 0) {TT_SMALL(str, x);} else {TT(str, x);}
#define TTP_SMALL(str, x)		if(ips.power_depth > 0) {TT_XSMALL(str, x);} else {TT_SMALL(str, x);}

#define PANGO_TT(layout, x)		if(scaledown <= 0) {pango_layout_set_markup(layout, "<span size=\"xx-large\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"x-large\">" x "</span>", -1);} else if(scaledown == 2) {pango_layout_set_markup(layout, "<span size=\"large\">" x "</span>", -1);} else {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);}
#define PANGO_TT_SMALL(layout, x)	if(scaledown <= 0) {pango_layout_set_markup(layout, "<span size=\"large\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);} else {pango_layout_set_markup(layout, "<span size=\"x-small\">" x "</span>", -1);}
#define PANGO_TT_XSMALL(layout, x)	if(scaledown <= 0) {pango_layout_set_markup(layout, "<span size=\"medium\">" x "</span>", -1);} else if(scaledown == 1) {pango_layout_set_markup(layout, "<span size=\"small\">" x "</span>", -1);} else {pango_layout_set_markup(layout, "<span size=\"x-small\">" x "</span>", -1);}
#define PANGO_TTP(layout, x)		if(ips.power_depth > 1) {PANGO_TT_XSMALL(layout, x);} else if(ips.power_depth > 0) {PANGO_TT_SMALL(layout, x);} else {PANGO_TT(layout, x);}
#define PANGO_TTP_SMALL(layout, x)	if(ips.power_depth > 0) {PANGO_TT_XSMALL(layout, x);} else {PANGO_TT_SMALL(layout, x);}

#define CALCULATE_SPACE_W		gint space_w, space_h; PangoLayout *layout_space = gtk_widget_create_pango_layout(resultview, NULL); PANGO_TTP(layout_space, " "); pango_layout_get_pixel_size(layout_space, &space_w, &space_h); g_object_unref(layout_space);

#define USE_QUOTES(arg, f) (arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && f->id() != FUNCTION_ID_BASE && f->id() != FUNCTION_ID_BIN && f->id() != FUNCTION_ID_OCT && f->id() != FUNCTION_ID_DEC && f->id() != FUNCTION_ID_HEX)

#define RESULT_SCALE_FACTOR gtk_widget_get_scale_factor(expressiontext)

bool fix_supsub_status = false, fix_supsub_result = false, fix_supsub_history = false, fix_supsub_completion = false;

enum {
	TITLE_APP,
	TITLE_RESULT,
	TITLE_APP_RESULT,
	TITLE_MODE,
	TITLE_APP_MODE
};

#include "floatingpointdialog.h"
#include "calendarconversiondialog.h"
#include "percentagecalculationdialog.h"
#include "numberbasesdialog.h"
#include "periodictabledialog.h"
#include "plotdialog.h"
#include "functionsdialog.h"
#include "unitsdialog.h"
#include "variablesdialog.h"
#include "datasetsdialog.h"
#include "precisiondialog.h"
#include "decimalsdialog.h"
#include "shortcutsdialog.h"
#include "buttonseditdialog.h"
#include "setbasedialog.h"
#include "preferencesdialog.h"
#include "matrixdialog.h"
#include "importcsvdialog.h"
#include "exportcsvdialog.h"
#include "nameseditdialog.h"
#include "uniteditdialog.h"
#include "variableeditdialog.h"
#include "matrixeditdialog.h"
#include "unknowneditdialog.h"
#include "functioneditdialog.h"
#include "dataseteditdialog.h"
#include "conversionview.h"
#include "expressionedit.h"
#include "expressioncompletion.h"
#include "historyview.h"
#include "stackview.h"
#include "keypad.h"
#include "menubar.h"

int title_type = TITLE_APP;

void string_strdown(const string &str, string &strnew) {
	char *cstr = utf8_strdown(str.c_str());
	if(cstr) {
		strnew = cstr;
		free(cstr);
	} else {
		strnew = str;
	}
}

SetTitleFunction::SetTitleFunction() : MathFunction("settitle", 1, 1, CALCULATOR->f_warning->category(), _("Set Window Title")) {
	setArgumentDefinition(1, new TextArgument());
}
int SetTitleFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	gtk_window_set_title(GTK_WINDOW(mainwindow), vargs[0].symbol().c_str());
	title_modified = true;
	return 1;
}

MathStructure *current_result() {return mstruct;}
void replace_current_result(MathStructure *m) {
	mstruct->unref();
	mstruct = m;
	mstruct->ref();
}
MathStructure *current_displayed_result() {return displayed_mstruct;}
MathStructure *current_parsed_result() {return parsed_mstruct;}

int has_information_unit_gtk(const MathStructure &m, bool top = true) {
	if(m.isUnit_exp()) {
		if(m.isUnit()) {
			if(m.unit()->baseUnit()->referenceName() == "bit") return 1;
		} else {
			if(m[0].unit()->baseUnit()->referenceName() == "bit") {
				if(m[1].isInteger() && m[1].number().isPositive()) return 1;
				return 2;
			}
		}
		return 0;
	}
	for(size_t i = 0; i < m.size(); i++) {
		int ret = has_information_unit_gtk(m[i], false);
		if(ret > 0) {
			if(ret == 1 && top && m.isMultiplication() && m[0].isNumber() && m[0].number().isFraction()) return 2;
			return ret;
		}
	}
	return 0;
}

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

string copy_text;

void end_cb(GtkClipboard*, gpointer) {}
void get_cb(GtkClipboard* cb, GtkSelectionData* sd, guint info, gpointer) {
	if(info == 1) gtk_selection_data_set(sd, gtk_selection_data_get_target(sd), 8, reinterpret_cast<const guchar*>(copy_text.c_str()), copy_text.length());
	else if(info == 3) gtk_selection_data_set_text(sd, unformat(unhtmlize(copy_text, true)).c_str(), -1);
	else gtk_selection_data_set_text(sd, unhtmlize(copy_text).c_str(), -1);
}

void set_clipboard(string str, int ascii, bool html, bool is_result, int copy_without_units) {
	if(ascii > 0 || (ascii < 0 && copy_ascii)) {
		str = unformat(unhtmlize(str, true));
		if(copy_without_units > 0 || (copy_without_units < 0 && copy_ascii_without_units && is_result)) {
			size_t i2 = string::npos;
			if(!is_result) i2 = str.rfind("=");
			size_t i = str.rfind(" ");
			if(i != string::npos && (i2 == string::npos || i < i2)) {
				MathStructure m;
				ParseOptions po;
				po.preserve_format = true;
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&m, str.substr(i + 1, str.length() - (i + 1)), po);
				if(is_unit_multiexp(m)) {
					CALCULATOR->parse(&m, i2 != string::npos ? str.substr(i2 + 1, str.length() - (i2 + 1)) : str, po);
					if(m.isMultiplication() || m.isDivision()) {
						str = str.substr(0, i);
					}
				}
				CALCULATOR->endTemporaryStopMessages();
			}
		}
		gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), str.c_str(), -1);
	} else {
#ifdef _WIN32
		OpenClipboard(0);
		EmptyClipboard();
		string copy_str = "Version:1.0\nStartHTML:0000000101\nEndHTML:";
		for(size_t i = i2s(139 + str.length()).length(); i < 10; i++) copy_str += '0';
		copy_str += i2s(139 + str.length());
		copy_str += "\nStartFragment:0000000121\nEndFragment:";
		for(size_t i = i2s(121 + str.length()).length(); i < 10; i++) copy_str += '0';
		copy_str += i2s(121 + str.length());
		copy_str += "\n\n<!--StartFragment-->";
		copy_str += str;
		copy_str += "<!--EndFragment-->";
		HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, copy_str.length() + 1);
		memcpy(GlobalLock(hMem), copy_str.c_str(), copy_str.length() + 1);
		GlobalUnlock(hMem);
		SetClipboardData(RegisterClipboardFormat("HTML Format"), hMem);
		copy_str = unhtmlize(str, true);
		::std::wstring wstr;
		int l = MultiByteToWideChar(CP_UTF8, 0, copy_str.c_str(), (int) copy_str.length(), NULL, 0);
		if(l > 0) {
			wstr.resize(l + 10);
			l = MultiByteToWideChar(CP_UTF8, 0, copy_str.c_str(), (int) copy_str.length(), &wstr[0], (int) wstr.size());
		}
		if(l > 0) {
			hMem = GlobalAlloc(GMEM_DDESHARE, sizeof(WCHAR) * (wcslen(wstr.data()) + 1));
			WCHAR* pchData = (WCHAR*) GlobalLock(hMem);
			wcscpy(pchData, wstr.data());
			GlobalUnlock(hMem);
			SetClipboardData(CF_UNICODETEXT, hMem);
		}
		copy_str = unformat(copy_str);
		hMem =  GlobalAlloc(GMEM_MOVEABLE, copy_str.length() + 1);
		memcpy(GlobalLock(hMem), copy_str.c_str(), copy_str.length() + 1);
		GlobalUnlock(hMem);
		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
#else
		copy_text = str;
		if(html) {
			GtkTargetEntry targets[] = {{(gchar*) "text/html", 0, 1}, {(gchar*) "UTF8_STRING", 0, (guint) 2}, {(gchar*) "STRING", 0, 3}};
			gtk_clipboard_set_with_data(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), targets, 3, &get_cb, &end_cb, NULL);
		} else {
			GtkTargetEntry targets[] = {{(gchar*) "UTF8_STRING", 0, (guint) 2}, {(gchar*) "STRING", 0, 3}};
			gtk_clipboard_set_with_data(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), targets, 2, &get_cb, &end_cb, NULL);
		}
#endif
	}
}

gint help_width = -1, help_height = -1;
gdouble help_zoom = -1.0;

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
		ifstream ifile(get_doc_uri("index.html", false).c_str());
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
						ifstream ifile_i(get_doc_uri(sfile, false).c_str());
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
	if(help_width != -1 || event->width != 800 || event->height != 600) {
		help_width = event->width;
		help_height = event->height;
	}
	return FALSE;
}
gboolean on_help_key_press_event(GtkWidget *d, GdkEventKey *event, gpointer w) {
	GtkWidget *entry_find = help_find_entries[GTK_WIDGET(w)];
	switch(event->keyval) {
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
			if(event->state & GDK_CONTROL_MASK || event->state & GDK_MOD1_MASK) {
				webkit_web_view_go_back(WEBKIT_WEB_VIEW(w));
				return TRUE;
			}
			break;
		}
		case GDK_KEY_Right: {
			if(event->state & GDK_CONTROL_MASK || event->state & GDK_MOD1_MASK) {
				webkit_web_view_go_forward(WEBKIT_WEB_VIEW(w));
				return TRUE;
			}
			break;
		}
		case GDK_KEY_KP_Add: {}
		case GDK_KEY_plus: {
			if(event->state & GDK_CONTROL_MASK || event->state & GDK_MOD1_MASK) {
				help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) + 0.1;
				webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
				return TRUE;
			}
			break;
		}
		case GDK_KEY_KP_Subtract: {}
		case GDK_KEY_minus: {
			if((event->state & GDK_CONTROL_MASK || event->state & GDK_MOD1_MASK) && webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) > 0.1) {
				help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) - 0.1;
				webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
				return TRUE;
			}
			break;
		}
		case GDK_KEY_Home: {
			if(event->state & GDK_CONTROL_MASK || event->state & GDK_MOD1_MASK) {
				webkit_web_view_load_uri(WEBKIT_WEB_VIEW(w), get_doc_uri("index.html").c_str());
				return TRUE;
			}
			break;
		}
		case GDK_KEY_f: {
			if(event->state & GDK_CONTROL_MASK) {
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
}
void on_help_button_zoomout_clicked(GtkButton*, gpointer w) {
	if(webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) > 0.1) {
		help_zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(w)) - 0.1;
		webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(w), help_zoom);
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
gboolean on_help_decide_policy(WebKitWebView *w, WebKitPolicyDecision *d, WebKitPolicyDecisionType t, gpointer window) {
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

void show_help(const char *file, GtkWidget *parent) {
#ifdef _WIN32
	if(ShellExecuteA(NULL, "open", get_doc_uri("index.html").c_str(), NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE) 32) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not display help for Qalculate!."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
#elif USE_WEBKITGTK
	GtkWidget *dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_window_set_title(GTK_WINDOW(dialog), "Qalculate! Manual");
	if(parent) {
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
		gtk_window_set_modal(GTK_WINDOW(dialog), gtk_window_get_modal(GTK_WINDOW(parent)));
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
	GtkWidget *button_zoomout = gtk_button_new_from_icon_name("zoom-out-symbolic", GTK_ICON_SIZE_BUTTON);
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
	if(help_zoom > 0.0) webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webView), help_zoom);
	PangoFontDescription *font_desc;
	gtk_style_context_get(gtk_widget_get_style_context(mainwindow), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
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
	gtk_show_uri_on_window(GTK_WINDOW(parent), get_doc_uri(file).c_str(), gtk_get_current_event_time(), &error);
#	else
	gtk_show_uri(NULL, get_doc_uri(file).c_str(), gtk_get_current_event_time(), &error);
#	endif
	if(error) {
		gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not display help for Qalculate!.\n%s"), error_str);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(error_str);
		g_error_free(error);
	}
#endif
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

bool block_input = false;
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

void replace_result_cis_gtk(string &resstr) {
	if(can_display_unicode_string_function_exact("∠", (void*) historyview)) gsub(" cis ", "∠", resstr);
}

void block_calculation() {
	block_expression_execution++;
}
void unblock_calculation() {
	block_expression_execution--;
}
bool calculation_blocked() {
	return block_expression_execution > 0;
}
void block_result() {
	block_result_update++;
}
void unblock_result() {
	block_result_update--;
}
bool result_blocked() {
	return block_result_update > 0;
}
void block_error() {
	block_error_timeout--;
}
void unblock_error() {
	block_error_timeout++;
}
bool result_is_autocalculated() {
	return result_autocalculated;
}

void minimal_mode_show_resultview(bool b = true) {
	if(!minimal_mode) return;
	if(b && !gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")))) {
		gint h = -1;
		gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), NULL, &h);
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), -1, gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled"))));
		//gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusseparator1")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), -1, h);
	} else if(!b && gtk_widget_is_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")))) {
		gint w, h;
		gtk_window_get_size(GTK_WINDOW(mainwindow), &w, &h);
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		set_status_bottom_border_visible(false);
		h -= 1;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		gtk_window_resize(GTK_WINDOW(mainwindow), w, h);
	}
}

void stop_autocalculate_history_timeout() {
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		autocalc_history_timeout_id = 0;
	}
}

gboolean do_autocalc_history_timeout(gpointer);
void copy_result(int ascii, int type) {
	int copy_without_units = -1;
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		do_autocalc_history_timeout(NULL);
	}
	if(type < 0 || type > 8) type = 0;
	if(ascii < 0 && type > 0) {
		if(type == 1 || type == 4 || type == 6) ascii = 0;
		else ascii = 1;
		if(type == 3) copy_without_units = 1;
		else copy_without_units = 0;
	}
	string str;
	if(type > 3 && type < 8) {
		if(expression_modified()) {
			if(!result_text.empty()) str = last_history_expression();
		} else {
			str = get_expression_text();
		}
	}
	if(!str.empty()) {
		if(ascii > 0 || (!result_text_approximate && (!mstruct || !mstruct->isApproximate()))) str += " = ";
		else str += " " SIGN_ALMOST_EQUAL " ";
		fix_history_string2(str);
	}
	if(type == 8) str += parsed_expression;
	else if(type <= 3 || type > 5) str += result_text;
	set_clipboard(str, ascii, type <= 3 || type > 5, type <= 3 || type == 8, copy_without_units);
}

bool result_text_empty() {
	return result_text.empty() && !autocalc_history_timeout_id;
}
string get_result_text() {
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		do_autocalc_history_timeout(NULL);
	}
	return unhtmlize(result_text);
}

string sdot_s, saltdot_s, sdiv_s, sslash_s, stimes_s, sminus_s;
string sdot_o, saltdot_o, sdiv_o, sslash_o, stimes_o, sminus_o;

const char *sub_sign() {
	if(!printops.use_unicode_signs) return "-";
	return sminus_o.c_str();
}
const char *times_sign(bool unit_expression) {
	if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_DOT) return sdot_o.c_str();
	else if(printops.use_unicode_signs && (printops.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT || (unit_expression && printops.multiplication_sign == MULTIPLICATION_SIGN_X))) return saltdot_o.c_str();
	else if(printops.use_unicode_signs && printops.multiplication_sign == MULTIPLICATION_SIGN_X) return stimes_o.c_str();
	return "*";
}
const char *divide_sign() {
	if(!printops.use_unicode_signs) return "/";
	if(printops.division_sign == DIVISION_SIGN_DIVISION) return sdiv_o.c_str();
	return sslash_o.c_str();
}

void set_status_operator_symbols() {
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
	if(status_layout) {
		g_object_unref(status_layout);
		status_layout = NULL;
	}
}
void set_app_operator_symbols() {
	if(can_display_unicode_string_function_exact(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit"))) sminus_o = SIGN_MINUS;
	else sminus_o = "-";
	if(can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit"))) sdiv_o = SIGN_DIVISION;
	else sdiv_o = "/";
	sslash_o = "/";
	if(can_display_unicode_string_function(SIGN_MULTIDOT, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit"))) sdot_o = SIGN_MULTIDOT;
	else sdot_o = "*";
	if(can_display_unicode_string_function(SIGN_MIDDLEDOT, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit"))) saltdot_o = SIGN_MIDDLEDOT;
	else saltdot_o = "*";
	if(can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit"))) stimes_o = SIGN_MULTIPLICATION;
	else stimes_o = "*";
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

void result_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	fix_supsub_result = test_supsub(resultview);
	set_result_size_request();
	result_font_updated = true;
	result_display_updated();
}
void status_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	fix_supsub_status = test_supsub(statuslabel_l);
	set_status_size_request();
	set_status_operator_symbols();
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

unordered_map<void*, unordered_map<const char*, int> > coverage_map;

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

double par_width = 6.0;

void set_result_size_request() {
	MathStructure mtest;
	MathStructure m1("Ü");
	MathStructure mden("y"); mden ^= m1;
	mtest = m1; mtest ^= m1; mtest.transform(STRUCT_DIVISION, mden);
	mtest.transform(CALCULATOR->f_sqrt);
	mtest.transform(CALCULATOR->f_abs);
	PrintOptions po;
	po.can_display_unicode_string_function = &can_display_unicode_string_function;
	po.can_display_unicode_string_arg = (void*) resultview;
	cairo_surface_t *tmp_surface2 = draw_structure(mtest, po, false, top_ips, NULL, 3);
	if(tmp_surface2) {
		cairo_surface_flush(tmp_surface2);
		gint h = cairo_image_surface_get_height(tmp_surface2) / RESULT_SCALE_FACTOR;
		gint sbh = 0;
		gtk_widget_get_preferred_height(gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result"))), NULL, &sbh);
		h += sbh;
		h += 3;
		cairo_surface_destroy(tmp_surface2);
		mtest.set(9);
		mtest.transform(STRUCT_DIVISION, 9);
		tmp_surface2 = draw_structure(mtest, po);
		if(tmp_surface2) {
			cairo_surface_flush(tmp_surface2);
			gint h2 = cairo_image_surface_get_height(tmp_surface2) / RESULT_SCALE_FACTOR + 3;
			if(h2 > h) h = h2;
			cairo_surface_destroy(tmp_surface2);
		}
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), -1, h);
	}
	PangoLayout *layout_test = gtk_widget_create_pango_layout(resultview, "x");
	gint h;
	pango_layout_get_pixel_size(layout_test, NULL, &h);
	par_width = h / 2.2;
	g_object_unref(layout_test);
}

void get_image_blank_height(cairo_surface_t *surface, int *y1, int *y2);
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

bool use_supsub(GtkWidget *w_supsub) {
	return (w_supsub == statuslabel_l && fix_supsub_status) || (w_supsub == resultview && fix_supsub_result) || (w_supsub == historyview && fix_supsub_history) || (w_supsub == completion_view && fix_supsub_completion) || (w_supsub != statuslabel_l && w_supsub != resultview && w_supsub != historyview && w_supsub != completion_view && test_supsub(w_supsub));
}

void set_status_size_request() {
	PangoLayout *layout_test = gtk_widget_create_pango_layout(statuslabel_l, NULL);
	FIX_SUPSUB_PRE(statuslabel_l)
	string str = "Ä<sub>x</sub>y<sup>2</sup>";
	FIX_SUPSUB(str)
	pango_layout_set_markup(layout_test, str.c_str(), -1);
	gint h;
	pango_layout_get_pixel_size(layout_test, NULL, &h);
	g_object_unref(layout_test);
	gtk_widget_set_size_request(statuslabel_l, -1, h);
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

tree_struct function_cats, unit_cats, variable_cats;
string volume_cat;
vector<string> alt_volcats;
vector<void*> ia_units, ia_variables, ia_functions;
vector<Unit*> user_units;
vector<Variable*> user_variables;
vector<MathFunction*> user_functions;
vector<string> recent_functions_pre;
vector<string> recent_variables_pre;
vector<string> recent_units_pre;
extern vector<MathFunction*> recent_functions;
extern vector<Variable*> recent_variables;
extern vector<Unit*> recent_units;

bool is_answer_variable(Variable *v) {
	return v == vans[0] || v == vans[1] || v == vans[2] || v == vans[3] || v == vans[4];
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

gboolean do_notification_timeout(gpointer) {
	gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "overlayrevealer")), FALSE);
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "overlayrevealer")));
	return FALSE;
}
void show_notification(string text) {
	text.insert(0, "<big>");
	text += "</big>";
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "overlaylabel")), text.c_str());
	gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "overlayrevealer")));
	gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "overlayrevealer")), TRUE);
	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000, do_notification_timeout, NULL, NULL);
}

enum {
	STATUS_TEXT_NONE,
	STATUS_TEXT_FUNCTION,
	STATUS_TEXT_ERROR,
	STATUS_TEXT_PARSED,
	STATUS_TEXT_AUTOCALC,
	STATUS_TEXT_OTHER
};
int status_text_source = STATUS_TEXT_NONE;

#define STATUS_SPACE	if(b) str += "  "; else b = true;

void set_status_text(string text, bool break_begin = false, bool had_errors = false, bool had_warnings = false, string tooltip_text = "") {

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
	if(text.empty()) status_text_source = STATUS_TEXT_NONE;

	if(break_begin) gtk_label_set_ellipsize(GTK_LABEL(statuslabel_l), PANGO_ELLIPSIZE_START);
	else gtk_label_set_ellipsize(GTK_LABEL(statuslabel_l), PANGO_ELLIPSIZE_END);

	gtk_label_set_markup(GTK_LABEL(statuslabel_l), str.c_str());
	gint w = 0;
	if(str.length() > 500) {
		w = -1;
	} else if(str.length() > 20) {
		if(!status_layout) status_layout = gtk_widget_create_pango_layout(statuslabel_l, "");
		pango_layout_set_markup(status_layout, str.c_str(), -1);
		pango_layout_get_pixel_size(status_layout, &w, NULL);
	}
	if(((auto_calculate && !rpn_mode) || !had_errors || tooltip_text.empty()) && (w < 0 || w > gtk_widget_get_allocated_width(statuslabel_l))) gtk_widget_set_tooltip_markup(statuslabel_l, text.c_str());
	else gtk_widget_set_tooltip_text(statuslabel_l, tooltip_text.c_str());
}
void clear_status_text() {
	set_status_text("");
}

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
	if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
		STATUS_SPACE
		str += _("RPN");
	}
	if(evalops.parse_options.parsing_mode == PARSING_MODE_CHAIN) {
		STATUS_SPACE
		// Chain mode
		str += _("CHN");
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
		case 12: {
			STATUS_SPACE
			str += _("DUO");
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
		case BASE_BIJECTIVE_26: {
			STATUS_SPACE
			str += "B26";
			break;
		}
		case BASE_BINARY_DECIMAL: {
			STATUS_SPACE
			str += "BCD";
			break;
		}
		case BASE_CUSTOM: {
			STATUS_SPACE
			str += CALCULATOR->customInputBase().print(CALCULATOR->messagePrintOptions());
			break;
		}
		case BASE_GOLDEN_RATIO: {
			STATUS_SPACE
			str += "φ";
			break;
		}
		case BASE_SUPER_GOLDEN_RATIO: {
			STATUS_SPACE
			str += "ψ";
			break;
		}
		case BASE_PI: {
			STATUS_SPACE
			str += "π";
			break;
		}
		case BASE_E: {
			STATUS_SPACE
			str += "e";
			break;
		}
		case BASE_SQRT2: {
			STATUS_SPACE
			str += "√2";
			break;
		}
		case BASE_UNICODE: {
			STATUS_SPACE
			str += "UNICODE";
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

bool check_exchange_rates(GtkWidget *win, bool set_result) {
	int i = CALCULATOR->exchangeRatesUsed();
	if(i == 0) return false;
	if(auto_update_exchange_rates == 0 && win != NULL) return false;
	if(CALCULATOR->checkExchangeRatesDate(auto_update_exchange_rates > 0 ? auto_update_exchange_rates : 7, false, auto_update_exchange_rates == 0, i)) return false;
	if(auto_update_exchange_rates == 0) return false;
	bool b = false;
	if(auto_update_exchange_rates < 0) {
		int days = (int) floor(difftime(time(NULL), CALCULATOR->getExchangeRatesTime(i)) / 86400);
		GtkWidget *edialog = gtk_message_dialog_new(win == NULL ? GTK_WINDOW(mainwindow) : GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you wish to update the exchange rates now?"));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(edialog), _n("It has been %s day since the exchange rates last were updated.", "It has been %s days since the exchange rates last were updated.", days), i2s(days).c_str());
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
		if(auto_update_exchange_rates <= 0) i = -1;
		if(!b && set_result) setResult(NULL, false, false, false, "", 0, false);
		fetch_exchange_rates(b ? 15 : 8, i);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}

bool expression_display_errors(GtkWidget *win, int type, bool do_exrate_sources, string &str, int mtype_highest) {
	if(str.empty() && do_exrate_sources && type == 1) {
		CALCULATOR->setExchangeRatesUsed(-100);
		int i = CALCULATOR->exchangeRatesUsed();
		CALCULATOR->setExchangeRatesUsed(-100);
		if(i > 0) {
			int n = 0;
			if(i & 0b0001) {str += "\n"; str += CALCULATOR->getExchangeRatesUrl(1); n++;}
			if(i & 0b0010) {str += "\n"; str += CALCULATOR->getExchangeRatesUrl(2); n++;}
			if(i & 0b0100) {str += "\n"; str += CALCULATOR->getExchangeRatesUrl(3); n++;}
			if(i & 0b1000) {str += "\n"; str += CALCULATOR->getExchangeRatesUrl(4); n++;}
			if(n > 0) {
				str.insert(0, _n("Exchange rate source:", "Exchange rate sources:", n));
				str += "\n(";
				gchar *gstr = g_strdup_printf(_n("updated %s", "updated %s", n), QalculateDateTime(CALCULATOR->getExchangeRatesTime(CALCULATOR->exchangeRatesUsed())).toISOString().c_str());
				str += gstr;
				g_free(gstr);
				str += ")";
			}
		}
	}
	if(!str.empty()) {
		if(type == 1 || type == 3) {
			gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")), str.c_str());
			if(mtype_highest == MESSAGE_ERROR) {
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_tooltip_icon")), "dialog-error", GTK_ICON_SIZE_BUTTON);
			} else if(mtype_highest == MESSAGE_WARNING) {
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_tooltip_icon")), "dialog-warning", GTK_ICON_SIZE_BUTTON);
			} else {
				gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "message_tooltip_icon")), "dialog-information", GTK_ICON_SIZE_BUTTON);
			}
			update_expression_icons(EXPRESSION_INFO);
			if(first_error && ((auto_calculate && !rpn_mode) || minimal_mode)) first_error = false;
			if(first_error && !minimal_mode) {
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "message_label")), _("When errors, warnings and other information are generated during calculation, the icon in the upper right corner of the expression entry changes to reflect this. If you hold the pointer over or click the icon, the message will be shown."));
				gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_icon")));
				gtk_info_bar_set_message_type(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_MESSAGE_INFO);
				gtk_info_bar_set_show_close_button(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), TRUE);
				gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), TRUE);
				first_error = false;
			}
			return true;
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
			gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), TRUE);
		} else if(mtype_highest != MESSAGE_INFORMATION) {
			GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(win),GTK_DIALOG_DESTROY_WITH_PARENT, mtype_highest == MESSAGE_ERROR ? GTK_MESSAGE_ERROR : (mtype_highest == MESSAGE_WARNING ? GTK_MESSAGE_WARNING : GTK_MESSAGE_INFO), GTK_BUTTONS_CLOSE, "%s", str.c_str());
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
			gtk_dialog_run(GTK_DIALOG(edialog));
			gtk_widget_destroy(edialog);
		}
	}
	return false;
}

/*
	display errors generated under calculation
*/
bool display_errors(GtkWidget *win, int type, bool add_to_history) {
	int mtype_highest = MESSAGE_INFORMATION;
	string str = history_display_errors(add_to_history, win, type, NULL, time(NULL), &mtype_highest);
	return expression_display_errors(win, type, add_to_history, str, mtype_highest);

}

gboolean on_display_errors_timeout(gpointer) {
	if(stop_timeouts) return FALSE;
	if(block_error_timeout > 0) return TRUE;
	if(CALCULATOR->checkSaveFunctionCalled()) {
		update_vmenu(false);
		update_fmenu(false);
		update_umenus();
	}
	display_errors();
	return TRUE;
}

gboolean on_activate_link(GtkLabel*, gchar *uri, gpointer) {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
	return TRUE;
#else
	return FALSE;
#endif
}

#ifdef AUTO_UPDATE
void auto_update(string new_version, string url) {
	char selfpath[1000];
	ssize_t n = readlink("/proc/self/exe", selfpath, 999);
	if(n < 0 || n >= 999) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Path of executable not found."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}
	selfpath[n] = '\0';
	gchar *selfdir = g_path_get_dirname(selfpath);
	FILE *pipe = popen("curl --version 1>/dev/null", "w");
	if(!pipe) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("curl not found."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}
	pclose(pipe);
	if(url.empty()) {
		url = "https://github.com/Qalculate/qalculate-gtk/releases/download/v${new_version}/qalculate-";
		url += new_version;
		url += "-x86_64.tar.xz";
	}
	string tmpdir = getLocalTmpDir();
	recursiveMakeDir(tmpdir);
	string script = "#!/bin/sh\n\n";
	script += "echo \"Updating Qalculate!...\";\n";
	script += "sleep 1;\n";
	script += "new_version="; script += new_version; script += ";\n";
	script += "url=\""; script += url; script += "\";\n";
	script += "filename=${url##*/};";
	script += "if cd \""; script += tmpdir; script += "\"; then\n";
	script += "\tif curl -L -o ${filename} ${url}; then\n";
	script += "\t\techo \"Extracting files...\";\n";
	script += "\t\tif tar -xJf ${filename}; then\n";
	script += "\t\t\tcd  qalculate-${new_version};\n";
	script += "\t\t\tif cp -f qalculate \""; script += selfpath; script += "\"; then\n";
	script += "\t\t\t\tcp -f qalc \""; script += selfdir; script += "/\";\n";
	script += "\t\t\t\tcd ..;\n\t\t\trm -r qalculate-${new_version};\n\t\t\trm ${filename};\n";
	script += "\t\t\t\texit 0;\n";
	script += "\t\t\tfi\n";
	script += "\t\t\tcd ..;\n\t\trm -r qalculate-${new_version};\n";
	script += "\t\tfi\n";
	script += "\t\trm ${filename};\n";
	script += "\tfi\n";
	script += "fi\n";
	script += "echo \"Update failed\";\n";
	script += "echo \"Press Enter to continue\";\n";
	script += "read _;\n";
	script += "exit 1\n";
	g_free(selfdir);
	std::ofstream ofs;
	string scriptpath = tmpdir; scriptpath += "/update.sh";
	ofs.open(scriptpath.c_str(), std::ofstream::out | std::ofstream::trunc);
	ofs << script;
	ofs.close();
	chmod(scriptpath.c_str(), S_IRWXU);
	string termcom = "#!/bin/sh\n\n";
	termcom += "if [ $(command -v gnome-terminal) ]; then\n";
	termcom += "\tif gnome-terminal --wait --version; then\n\t\tdetected_term=\"gnome-terminal --wait -- \";\n";
	termcom += "\telse\n\t\tdetected_term=\"gnome-terminal --disable-factory -- \";\n\tfi\n";
	termcom += "elif [ $(command -v xfce4-terminal) ]; then\n\tdetected_term=\"xfce4-terminal --disable-server -e \";\n";
	termcom += "else\n";
	termcom += "\tfor t in x-terminal-emulator konsole alacritty qterminal xterm urxvt rxvt kitty sakura terminology termite tilix; do\n\t\tif [ $(command -v $t) ]; then\n\t\t\tdetected_term=\"$t -e \";\n\t\t\tbreak\n\t\tfi\n\tdone\nfi\n";
	termcom += "$detected_term "; termcom += scriptpath; termcom += ";\n";
	termcom += "exec "; termcom += selfpath; termcom += "\n";
	std::ofstream ofs2;
	string scriptpath2 = tmpdir; scriptpath2 += "/terminal.sh";
	ofs2.open(scriptpath2.c_str(), std::ofstream::out | std::ofstream::trunc);
	ofs2 << termcom;
	ofs2.close();
	chmod(scriptpath2.c_str(), S_IRWXU);
	GError *error = NULL;
	g_spawn_command_line_async(scriptpath2.c_str(), &error);
	if(error) {
		gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Failed to run update script.\n%s"), error_str);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(error_str);
		g_error_free(error);
		return;
	}
	on_gcalc_exit(NULL, NULL, NULL);
}
#endif

void check_for_new_version(bool do_not_show_again) {
	string new_version, url;
#ifdef _WIN32
	int ret = checkAvailableVersion("windows", VERSION, &new_version, &url, do_not_show_again ? 5 : 10);
#else
#	ifdef AUTO_UPDATE
	int ret = checkAvailableVersion("qalculate-gtk", VERSION, &new_version, &url, do_not_show_again ? 5 : 10);
#	else
	int ret = checkAvailableVersion("qalculate-gtk", VERSION, &new_version, do_not_show_again ? 5 : 10);
#	endif
#endif
	if(!do_not_show_again && ret <= 0) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, ret < 0 ? GTK_MESSAGE_ERROR : GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, ret < 0 ? _("Failed to check for updates.") : _("No updates found."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(ret < 0) return;
	}
	if(ret > 0 && (!do_not_show_again || new_version != last_found_version)) {
		last_found_version = new_version;
#ifdef AUTO_UPDATE
		GtkWidget *dialog = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
#else
#	ifdef _WIN32
		GtkWidget *dialog = NULL;
		if(url.empty()) dialog = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Close"), GTK_RESPONSE_REJECT, NULL);
		else dialog = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Download"), GTK_RESPONSE_ACCEPT, _("_Close"), GTK_RESPONSE_REJECT, NULL);
#	else
		GtkWidget *dialog = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Close"), GTK_RESPONSE_REJECT, NULL);
#	endif
#endif
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
		GtkWidget *label = gtk_label_new(NULL);
#ifdef AUTO_UPDATE
		gchar *gstr = g_strdup_printf(_("A new version of %s is available at %s.\n\nDo you wish to update to version %s?"), "Qalculate!", "<a href=\"https://qalculate.github.io/downloads.html\">qalculate.github.io</a>", new_version.c_str());
#else
		gchar *gstr = g_strdup_printf(_("A new version of %s is available.\n\nYou can get version %s at %s."), "Qalculate!", new_version.c_str(), "<a href=\"https://qalculate.github.io/downloads.html\">qalculate.github.io</a>");
#endif
		gtk_label_set_markup(GTK_LABEL(label), gstr);
		g_free(gstr);
		gtk_container_add(GTK_CONTAINER(hbox), label);
		g_signal_connect(G_OBJECT(label), "activate-link", G_CALLBACK(on_activate_link), NULL);
		gtk_widget_show_all(dialog);
#ifdef AUTO_UPDATE
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			auto_update(new_version, url);
		}
#else
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT && !url.empty()) {
#	ifdef _WIN32
			ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#	endif
		}
#endif
		gtk_widget_destroy(dialog);
	}
	last_version_check_date.setToCurrentDate();
}

gboolean on_check_version_idle(gpointer) {
	check_for_new_version(true);
	return FALSE;
}

bool display_function_hint(MathFunction *f, int arg_index = 1) {
	if(!f) return false;
	int iargs = f->maxargs();
	Argument *arg;
	Argument default_arg;
	string str, str2, str3;
	const ExpressionName *ename = &f->preferredName(false, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) statuslabel_l);
	bool last_is_vctr = f->getArgumentDefinition(iargs) && f->getArgumentDefinition(iargs)->type() == ARGUMENT_TYPE_VECTOR;
	if(arg_index > iargs && iargs >= 0 && !last_is_vctr) {
		if(iargs == 1 && f->getArgumentDefinition(1) && f->getArgumentDefinition(1)->handlesVector()) {
			return false;
		}
		gchar *gstr = g_strdup_printf(_("Too many arguments for %s()."), ename->formattedName(TYPE_FUNCTION, true, true).c_str());
		set_status_text(gstr, false, false, true);
		g_free(gstr);
		status_text_source = STATUS_TEXT_FUNCTION;
		return true;
	}
	str += ename->formattedName(TYPE_FUNCTION, true, true);
	if(iargs < 0) {
		iargs = f->minargs() + 1;
		if((int) f->lastArgumentDefinitionIndex() > iargs) iargs = (int) f->lastArgumentDefinitionIndex();
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
				if(i2 > 1 || f->maxargs() != 1) {
					str2 += " ";
					str2 += i2s(i2);
				}
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
				if(i_reduced != 2 && printops.use_unicode_signs) {
					gsub(">=", SIGN_GREATER_OR_EQUAL, str3);
					gsub("<=", SIGN_LESS_OR_EQUAL, str3);
					gsub("!=", SIGN_NOT_EQUAL, str3);
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
						str = ename->formattedName(TYPE_FUNCTION, true, true);
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
	status_text_source = STATUS_TEXT_FUNCTION;
	return true;
}

void replace_interval_with_function(MathStructure &m);

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

bool is_time(const MathStructure &m) {
	bool b = false;
	if(m.isUnit() && m.unit()->baseUnit()->referenceName() == "s") {
		b = true;
	} else if(m.isMultiplication() && m.size() == 2 && m[0].isNumber() && m[1].isUnit() && m[1].unit()->baseUnit()->referenceName() == "s") {
		b = true;
	} else if(m.isAddition() && m.size() > 0) {
		b = true;
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isUnit() && m[i].unit()->baseUnit()->referenceName() == "s") {}
			else if(m[i].isMultiplication() && m[i].size() == 2 && m[i][0].isNumber() && m[i][1].isUnit() && m[i][1].unit()->baseUnit()->referenceName() == "s") {}
			else {b = false; break;}
		}
	}
	return b;
}

void add_to_expression_history(string str);

bool contains_temperature_unit_gtk(const MathStructure &m) {
	if(m.isUnit()) {
		return m.unit() == CALCULATOR->getUnitById(UNIT_ID_CELSIUS) || m.unit() == CALCULATOR->getUnitById(UNIT_ID_FAHRENHEIT);
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		return contains_temperature_unit_gtk(((KnownVariable*) m.variable())->get());
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_temperature_unit_gtk(m[i])) return true;
	}
	return false;
}
bool test_ask_tc(MathStructure &m) {
	if(tc_set || !contains_temperature_unit_gtk(m)) return false;
	MathStructure *mp = &m;
	if(m.isMultiplication() && m.size() == 2 && m[0].isMinusOne()) mp = &m[1];
	else if(m.isNegate()) mp = &m[0];
	if(mp->isUnit_exp()) return false;
	if(mp->isMultiplication() && mp->size() > 0 && mp->last().isUnit_exp()) {
		bool b = false;
		for(size_t i = 0; i < mp->size() - 1; i++) {
			if(contains_temperature_unit_gtk((*mp)[i])) {b = true; break;}
		}
		if(!b) return false;
	}
	return true;
}
bool ask_tc() {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Temperature Calculation Mode"), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("The expression is ambiguous.\nPlease select temperature calculation mode\n(the mode can later be changed in preferences)."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_abs = gtk_radio_button_new_with_label(NULL, _("Absolute"));
	gtk_widget_set_valign(w_abs, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_abs, 0, 1, 1, 1);
	label = gtk_label_new("<i>1 °C + 1 °C ≈ 274 K + 274 K ≈ 548 K\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 K\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_rel = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_abs), _("Relative"));
	gtk_widget_set_valign(w_rel, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_rel, 0, 2, 1, 1);
	label = gtk_label_new("<i>1 °C + 1 °C = 2 °C\n1 °C + 5 °F = 1 °C + 5 °R ≈ 4 °C ≈ 277 K\n2 °C − 1 °C = 1 °C\n1 °C − 5 °F = 1 °C - 5 °R ≈ −2 °C\n1 °C + 1 K = 2 °C</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	GtkWidget *w_hybrid = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_abs), _("Hybrid"));
	gtk_widget_set_valign(w_hybrid, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_hybrid, 0, 3, 1, 1);
	label = gtk_label_new("<i>1 °C + 1 °C ≈ 2 °C\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 °C\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
	switch(CALCULATOR->getTemperatureCalculationMode()) {
		case TEMPERATURE_CALCULATION_ABSOLUTE: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_abs), TRUE); break;}
		case TEMPERATURE_CALCULATION_RELATIVE: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_rel), TRUE); break;}
		default: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_hybrid), TRUE); break;}
	}
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	TemperatureCalculationMode tc_mode = TEMPERATURE_CALCULATION_HYBRID;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_abs))) tc_mode = TEMPERATURE_CALCULATION_ABSOLUTE;
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_rel))) tc_mode = TEMPERATURE_CALCULATION_RELATIVE;
	gtk_widget_destroy(dialog);
	tc_set = true;
	if(tc_mode != CALCULATOR->getTemperatureCalculationMode()) {
		CALCULATOR->setTemperatureCalculationMode(tc_mode);
		preferences_update_temperature_calculation();
		return true;
	}
	return false;
}
bool test_ask_sinc(MathStructure &m) {
	return !sinc_set && m.containsFunctionId(FUNCTION_ID_SINC);
}
bool ask_sinc() {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Sinc Function"), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("Please select desired variant of the sinc function."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_1 = gtk_radio_button_new_with_label(NULL, _("Unnormalized"));
	gtk_widget_set_valign(w_1, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_1, 0, 1, 1, 1);
	label = gtk_label_new("<i>sinc(x) = sinc(x)/x</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_pi = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_1), _("Normalized"));
	gtk_widget_set_valign(w_pi, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_pi, 0, 2, 1, 1);
	label = gtk_label_new("<i>sinc(x) = sinc(πx)/(πx)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_1), TRUE);
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	bool b_pi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_pi));
	gtk_widget_destroy(dialog);
	sinc_set = true;
	if(b_pi) {
		CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "pi");
		return true;
	}
	return false;
}
bool test_ask_dot(const string &str) {
	if(dot_question_asked || CALCULATOR->getDecimalPoint() == DOT) return false;
	size_t i = 0;
	while(true) {
		i = str.find(DOT, i);
		if(i == string::npos) return false;
		i = str.find_first_not_of(SPACES, i + 1);
		if(i == string::npos) return false;
		if(is_in(NUMBERS, str[i])) return true;
	}
	return false;
}

bool ask_dot() {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Interpretation of dots"), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("Please select interpretation of dots (\".\")\n(this can later be changed in preferences)."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_bothdeci = gtk_radio_button_new_with_label(NULL, _("Both dot and comma as decimal separators"));
	gtk_widget_set_valign(w_bothdeci, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_bothdeci, 0, 1, 1, 1);
	label = gtk_label_new("<i>(1.2 = 1,2)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_ignoredot = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_bothdeci), _("Dot as thousands separator"));
	gtk_widget_set_valign(w_ignoredot, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_ignoredot, 0, 2, 1, 1);
	label = gtk_label_new("<i>(1.000.000 = 1000000)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	GtkWidget *w_dotdeci = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_bothdeci), _("Only dot as decimal separator"));
	gtk_widget_set_valign(w_dotdeci, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_dotdeci, 0, 3, 1, 1);
	label = gtk_label_new("<i>(1.2 + root(16, 4) = 3.2)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
	if(evalops.parse_options.dot_as_separator) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_ignoredot), TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_bothdeci), TRUE);
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	dot_question_asked = true;
	bool das = evalops.parse_options.dot_as_separator;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_dotdeci))) {
		evalops.parse_options.dot_as_separator = false;
		evalops.parse_options.comma_as_separator = false;
		b_decimal_comma = false;
		CALCULATOR->useDecimalPoint(false);
		das = !evalops.parse_options.dot_as_separator;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_ignoredot))) {
		evalops.parse_options.dot_as_separator = true;
	} else {
		evalops.parse_options.dot_as_separator = false;
	}
	preferences_update_dot();
	gtk_widget_destroy(dialog);
	return das != evalops.parse_options.dot_as_separator;
}

bool ask_implicit() {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Parsing Mode"), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("The expression is ambiguous.\nPlease select interpretation of expressions with implicit multiplication\n(this can later be changed in preferences)."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_implicitfirst = gtk_radio_button_new_with_label(NULL, _("Implicit multiplication first"));
	if(evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_implicitfirst), TRUE);
	gtk_widget_set_valign(w_implicitfirst, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_implicitfirst, 0, 1, 1, 1);
	label = gtk_label_new("<i>1/2x = 1/(2x)</i>\n<i>5 m/2 s = (5 m)/(2 s)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_conventional = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_implicitfirst), _("Conventional"));
	if(evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_conventional), TRUE);
	gtk_widget_set_valign(w_conventional, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_conventional, 0, 2, 1, 1);
	label = gtk_label_new("<i>1/2x = (1/2)x</i>\n<i>5 m/2 s = (5 m/2)s</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	GtkWidget *w_adaptive = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_implicitfirst), _("Adaptive"));
	if(evalops.parse_options.parsing_mode == PARSING_MODE_ADAPTIVE) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_adaptive), TRUE);
	gtk_widget_set_valign(w_adaptive, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_adaptive, 0, 3, 1, 1);
	label = gtk_label_new("<i>1/2x = 1/(2x); 1/2 x = (1/2)x</i>\n<i>5 m/2 s = (5 m)/(2 s)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	implicit_question_asked = true;
	ParsingMode pm_bak = evalops.parse_options.parsing_mode;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_implicitfirst))) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_conventional))) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
	} else {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
	}
	gtk_widget_destroy(dialog);
	return pm_bak != evalops.parse_options.parsing_mode;
}

bool test_ask_percent() {
	return simplified_percentage < 0 && CALCULATOR->simplifiedPercentageUsed();
}
bool ask_percent() {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Percentage Interpretation"), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("Please select interpretation of percentage addition."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_1 = gtk_radio_button_new_with_label(NULL, _("Add percentage of original value"));
	gtk_widget_set_valign(w_1, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_1, 0, 1, 1, 1);
	string s_eg = "<i>100 + 10% = 100 "; s_eg += times_sign(); s_eg += " 110% = 110</i>)";
	label = gtk_label_new(s_eg.c_str());
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_0 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_1), _("Add percentage multiplied by 1/100"));
	gtk_widget_set_valign(w_0, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_0, 0, 2, 1, 1);
	s_eg = "<i>100 + 10% = 100 + (10 "; s_eg += times_sign(); s_eg += " 0.01) = 100.1</i>)";
	label = gtk_label_new(CALCULATOR->localizeExpression(s_eg, evalops.parse_options).c_str());
	label = gtk_label_new("<i>100 + 10% = 100 + (10 * 0.01) = 100.1</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_1), TRUE);
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_0))) simplified_percentage = 0;
	else simplified_percentage = 1;
	gtk_widget_destroy(dialog);
	return simplified_percentage == 0;
}

vector<CalculatorMessage> autocalc_messages;
gboolean do_autocalc_history_timeout(gpointer) {
	autocalc_history_timeout_id = 0;
	if(stop_timeouts || !result_autocalculated || rpn_mode) return FALSE;
	if((test_ask_tc(*parsed_mstruct) && ask_tc()) || (test_ask_dot(result_text) && ask_dot()) || ((test_ask_sinc(*parsed_mstruct) || test_ask_sinc(*mstruct)) && ask_sinc()) || (test_ask_percent() && ask_percent()) || check_exchange_rates(NULL, true)) {
		execute_expression(true, false, OPERATION_ADD, NULL, false, 0, "", "", false);
		return FALSE;
	}
	CALCULATOR->addMessages(&autocalc_messages);
	result_text = get_expression_text();
	add_to_expression_history(result_text);
	string to_str = CALCULATOR->parseComments(result_text, evalops.parse_options);
	if(!to_str.empty()) {
		if(result_text.empty()) return FALSE;
		else CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
	}
	set_expression_modified(false, false, false);
	setResult(NULL, true, true, true, "", 0, false, true);
	update_conversion_view_selection(mstruct);
	result_autocalculated = false;
	return FALSE;
}

void add_autocalculated_result_to_history() {
	if(expression_modified() && result_is_autocalculated() && !parsed_in_result && (autocalc_history_delay < 0 || autocalc_history_timeout_id)) {
		if(autocalc_history_timeout_id) g_source_remove(autocalc_history_timeout_id);
		do_autocalc_history_timeout(NULL);
	}
}

bool auto_calc_stopped_at_operator = false;

bool test_parsed_comparison_gtk(const MathStructure &m) {
	if(m.isComparison()) return true;
	if((m.isLogicalOr() || m.isLogicalAnd()) && m.size() > 0) {
		for(size_t i = 0; i < m.size(); i++) {
			if(!test_parsed_comparison_gtk(m[i])) return false;
		}
		return true;
	}
	return false;
}
bool contains_plot_or_save(const string &str) {
	if(expression_contains_save_function(CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options, false)) return true;
	if(CALCULATOR->f_plot) {
		for(size_t i = 1; i <= CALCULATOR->f_plot->countNames(); i++) {
			if(str.find(CALCULATOR->f_plot->getName(i).name) != string::npos) return true;
		}
	}
	return false;
}

long int get_fixed_denominator_gtk2(const string &str, int &to_fraction, char sgn, bool qalc_command) {
	long int fden = 0;
	if(!qalc_command && (equalsIgnoreCase(str, "fraction") || equalsIgnoreCase(str, _("fraction")))) {
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
		if(sgn == '-' || (fden > 0 && !qalc_command && sgn != '+' && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))))) to_fraction = 2;
		else if(fden > 0 && sgn == 0) to_fraction = -1;
		else to_fraction = 1;
	}
	return fden;
}
long int get_fixed_denominator_gtk(const string &str, int &to_fraction, bool qalc_command = false) {
	size_t n = 0;
	if(str[0] == '-' || str[0] == '+') n = 1;
	if(n > 0) return get_fixed_denominator_gtk2(str.substr(n, str.length() - n), to_fraction, str[0], qalc_command);
	return get_fixed_denominator_gtk2(str, to_fraction, 0, qalc_command);
}

bool contains_fraction_gtk(const MathStructure &m) {
	if(m.isNumber()) return !m.number().isInteger();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_fraction_gtk(m[i])) return true;
	}
	return false;
}

string prev_autocalc_str;
MathStructure current_status_struct, mauto;

void do_auto_calc(int recalculate, string str) {
	if(result_blocked() || calculation_blocked()) return;

	bool do_factors = false, do_pfe = false, do_expand = false;

	ComplexNumberForm cnf_bak = evalops.complex_number_form;
	ComplexNumberForm cnf = evalops.complex_number_form;
	bool delay_complex = false;
	bool caf_bak = complex_angle_form;
	bool b_units_saved = evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = evalops.mixed_units_conversion;
	Number save_nbase;
	bool custom_base_set = false;
	int save_base = printops.base;
	unsigned int save_bits = printops.binary_bits;
	bool save_pre = printops.use_unit_prefixes;
	bool save_cur = printops.use_prefixes_for_currencies;
	bool save_allu = printops.use_prefixes_for_all_units;
	bool save_all = printops.use_all_prefixes;
	bool save_den = printops.use_denominator_prefix;
	int save_bin = CALCULATOR->usesBinaryPrefixes();
	long int save_fden = CALCULATOR->fixedDenominator();
	NumberFractionFormat save_format = printops.number_fraction_format;
	bool save_restrict_fraction_length = printops.restrict_fraction_length;
	bool do_to = false;

	if(recalculate) {
		if(!mbak_convert.isUndefined()) mbak_convert.setUndefined();
		auto_calc_stopped_at_operator = false;
		stop_autocalculate_history_timeout();
		bool origstr = str.empty();
		if(origstr) str = get_expression_text();
		if(origstr) CALCULATOR->parseComments(str, evalops.parse_options);
		if(str.empty() || (origstr && (str == "MC" || str == "MS" || str == "M+" || str == "M-" || str == "M−" || contains_plot_or_save(str)))) {
			result_autocalculated = false;
			result_text = "";
			if(parsed_in_result) display_parse_status();
			else clearresult();
			return;
		}
		if(origstr && str.length() > 1 && str[0] == '/') {
			size_t i = str.find_first_not_of(SPACES, 1);
			if(i != string::npos && (signed char) str[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, str[i])) {
				result_autocalculated = false;
				result_text = "";
				if(parsed_in_result) display_parse_status();
				else clearresult();
				return;
			}
		}
		if(recalculate == 2 && gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "expression_button_stack"))) != GTK_WIDGET(gtk_builder_get_object(main_builder, "message_tooltip_icon")) && evalops.parse_options.base != BASE_UNICODE && (evalops.parse_options.base != BASE_CUSTOM || (CALCULATOR->customInputBase() <= 62 && CALCULATOR->customInputBase() >= -62))) {
			GtkTextMark *mark = gtk_text_buffer_get_insert(expressionbuffer);
			if(mark) {
				GtkTextIter ipos;
				gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mark);
				bool b_to = CALCULATOR->hasToExpression(str, false, evalops) || CALCULATOR->hasWhereExpression(str, evalops);
				if(gtk_text_iter_is_end(&ipos)) {
					if(last_is_operator(str, evalops.parse_options.base == 10) && (evalops.parse_options.base != BASE_ROMAN_NUMERALS || str[str.length() - 1] != '|' || str.find('|') == str.length() - 1)) {
						size_t n = 1;
						while(n < str.length() && (char) str[str.length() - n] < 0 && (unsigned char) str[str.length() - n] < 0xC0) n++;
						if((b_to && n == 1 && (str[str.length() - 1] != ' ' || str[str.length() - 1] != '/')) || n == str.length() || (display_expression_status && !b_to && parsed_mstruct->equals(current_status_struct, true, true)) || ((!display_expression_status || b_to) && str.length() - n == prev_autocalc_str.length() && str.substr(0, str.length() - n) == prev_autocalc_str)) {
							auto_calc_stopped_at_operator = true;
							return;
						}
					}
				} else if(!b_to && display_expression_status) {
					GtkTextIter iter = ipos;
					gtk_text_iter_forward_char(&iter);
					gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &ipos, &iter, FALSE);
					string c2 = gstr;
					g_free(gstr);
					string c1;
					if(!gtk_text_iter_is_start(&ipos)) {
						iter = ipos;
						gtk_text_iter_backward_char(&iter);
						gstr = gtk_text_buffer_get_text(expressionbuffer, &iter, &ipos, FALSE);
						c1 = gstr;
						g_free(gstr);
					}
					if((c2.length() == 1 && is_in("*/^|&<>=)]", c2[0]) && (c2[0] != '|' || evalops.parse_options.base != BASE_ROMAN_NUMERALS)) || (c2.length() > 1 && (c2 == "∧" || c2 == "∨" || c2 == "⊻" || c2 == expression_times_sign() || c2 == expression_divide_sign() || c2 == SIGN_NOT_EQUAL || c2 == SIGN_GREATER_OR_EQUAL || c2 == SIGN_LESS_OR_EQUAL))) {
						if(c1.empty() || (c1.length() == 1 && is_in(OPERATORS LEFT_PARENTHESIS, c1[0]) && c1[0] != '!' && (c1[0] != '|' || (evalops.parse_options.base != BASE_ROMAN_NUMERALS && c1 != "|")) && (c1[0] != '&' || c2 != "&") && (c1[0] != '/' || (c2 != "/" && c2 != expression_divide_sign())) && (c1[0] != '*' || (c2 != "*" && c2 != expression_times_sign())) && ((c1[0] != '>' && c1[0] != '<') || (c2 != "=" && c2 != c1)) && ((c2 != ">" && c2 == "<") || (c1[0] != '=' && c1 != c2))) || (c1.length() > 1 && (c1 == "∧" || c1 == "∨" || c1 == "⊻" || c1 == SIGN_NOT_EQUAL || c1 == SIGN_GREATER_OR_EQUAL || c1 == SIGN_LESS_OR_EQUAL || (c1 == expression_times_sign() && c2 != "*" && c2 != expression_times_sign()) || (c1 == expression_divide_sign() && c2 != "/" && c2 != expression_divide_sign()) || c1 == expression_add_sign() || c1 == expression_sub_sign()))) {
							if(parsed_mstruct->equals(current_status_struct, true, true)) {
								auto_calc_stopped_at_operator = true;
								if(parsed_in_result) {
									result_text = "";
									display_parse_status();
								}
								return;
							}
						}
					}
				}
			}
		}
		prev_autocalc_str = str;
		if(origstr) {
			to_caf = -1; to_fraction = 0; to_fixed_fraction = 0; to_prefix = 0; to_base = 0; to_bits = 0; to_nbase.clear();
		}
		string from_str = str, to_str, str_conv;
		bool had_to_expression = false;
		bool last_is_space = !from_str.empty() && is_in(SPACES, from_str[from_str.length() - 1]);
		if(origstr && CALCULATOR->separateToExpression(from_str, to_str, evalops, true, parsed_in_result)) {
			had_to_expression = true;
			if(from_str.empty()) {
				evalops.complex_number_form = cnf_bak;
				evalops.auto_post_conversion = save_auto_post_conversion;
				evalops.parse_options.units_enabled = b_units_saved;
				evalops.mixed_units_conversion = save_mixed_units_conversion;
				if(parsed_in_result) {
					mauto.setAborted();
					result_text = "";
					result_autocalculated = false;
					display_parse_status();
				} else {
					clearresult();
				}
				return;
			}
			remove_duplicate_blanks(to_str);
			string str_left;
			string to_str1, to_str2;
			while(true) {
				if(last_is_space) to_str += " ";
				CALCULATOR->separateToExpression(to_str, str_left, evalops, true, false);
				remove_blank_ends(to_str);
				size_t ispace = to_str.find_first_of(SPACES);
				if(ispace != string::npos) {
					to_str1 = to_str.substr(0, ispace);
					remove_blank_ends(to_str1);
					to_str2 = to_str.substr(ispace + 1);
					remove_blank_ends(to_str2);
				}
				if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, _("hexadecimal"))) {
					to_base = BASE_HEXADECIMAL;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, _("octal"))) {
					to_base = BASE_OCTAL;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "dec") || equalsIgnoreCase(to_str, "decimal") || equalsIgnoreCase(to_str, _("decimal"))) {
					to_base = BASE_DECIMAL;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "duo") || equalsIgnoreCase(to_str, "duodecimal") || equalsIgnoreCase(to_str, _("duodecimal"))) {
					to_base = BASE_DUODECIMAL;
					to_duo_syms = false;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "doz") || equalsIgnoreCase(to_str, "dozenal")) {
					to_base = BASE_DUODECIMAL;
					to_duo_syms = true;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, _("binary"))) {
					to_base = BASE_BINARY;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
					to_base = BASE_ROMAN_NUMERALS;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, _("bijective"))) {
					to_base = BASE_BIJECTIVE_26;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "bcd")) {
					to_base = BASE_BINARY_DECIMAL;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, _("sexagesimal"))) {
					to_base = BASE_SEXAGESIMAL;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "2")) {
					to_base = BASE_SEXAGESIMAL_2;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "3")) {
					to_base = BASE_SEXAGESIMAL_3;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "latitude") || equalsIgnoreCase(to_str, _("latitude"))) {
					to_base = BASE_LATITUDE;
					do_to = true;
				} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", _("latitude"), "2")) {
					to_base = BASE_LATITUDE_2;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "longitude") || equalsIgnoreCase(to_str, _("longitude"))) {
					to_base = BASE_LONGITUDE;
					do_to = true;
				} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", _("longitude"), "2")) {
					to_base = BASE_LONGITUDE_2;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
					to_base = BASE_FP32;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
					to_base = BASE_FP64;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
					to_base = BASE_FP16;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "fp80")) {
					to_base = BASE_FP80;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
					to_base = BASE_FP128;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
					to_base = BASE_TIME;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "Unicode")) {
					to_base = BASE_UNICODE;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
					printops.time_zone = TIME_ZONE_UTC;
					do_to = true;
				} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
					to_base = BASE_BINARY;
					int bits = s2i(to_str.substr(3));
					if(bits >= 0) {
						if(bits > 4096) to_bits = 4096;
						else to_bits = bits;
					}
					do_to = true;
				} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
					to_base = BASE_HEXADECIMAL;
					int bits = s2i(to_str.substr(3));
					if(bits >= 0) {
						if(bits > 4096) to_bits = 4096;
						else to_bits = bits;
					}
					do_to = true;
				} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
					to_str = to_str.substr(3);
					remove_blanks(to_str);
					bool b_minus = false;
					if(to_str[0] == '+') {
						to_str.erase(0, 1);
					} else if(to_str[0] == '-') {
						b_minus = true;
						to_str.erase(0, 1);
					} else if(to_str.find(SIGN_MINUS) == 0) {
						b_minus = true;
						to_str.erase(0, strlen(SIGN_MINUS));
					}
					unsigned int tzh = 0, tzm = 0;
					int itz = 0;
					if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
						itz = tzh * 60 + tzm;
						if(b_minus) itz = -itz;
					}
					printops.time_zone = TIME_ZONE_CUSTOM;
					printops.custom_time_zone = itz;
					do_to = true;
				} else if(to_str == "CET") {
					printops.time_zone = TIME_ZONE_CUSTOM;
					printops.custom_time_zone = 60;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, _("bases"))) {
					str = from_str;
				} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, _("calendars"))) {
					str = from_str;
				} else if(equalsIgnoreCase(to_str, "rectangular") || equalsIgnoreCase(to_str, "cartesian") || equalsIgnoreCase(to_str, _("rectangular")) || equalsIgnoreCase(to_str, _("cartesian"))) {
					to_caf = 0;
					cnf = COMPLEX_NUMBER_FORM_RECTANGULAR;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "exponential") || equalsIgnoreCase(to_str, _("exponential"))) {
					to_caf = 0;
					cnf = COMPLEX_NUMBER_FORM_EXPONENTIAL;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "polar") || equalsIgnoreCase(to_str, _("polar"))) {
					to_caf = 0;
					cnf = COMPLEX_NUMBER_FORM_POLAR;
					do_to = true;
				} else if(to_str == "cis") {
					to_caf = 0;
					cnf = COMPLEX_NUMBER_FORM_CIS;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "angle") || equalsIgnoreCase(to_str, _("angle")) || equalsIgnoreCase(to_str, "phasor") || equalsIgnoreCase(to_str, _("phasor"))) {
					to_caf = 1;
					cnf = COMPLEX_NUMBER_FORM_CIS;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, _("optimal"))) {
					evalops.parse_options.units_enabled = true;
					evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
					str_conv = "";
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "prefix") || equalsIgnoreCase(to_str, _("prefix"))) {
					evalops.parse_options.units_enabled = true;
					to_prefix = 1;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, _("base"))) {
					evalops.parse_options.units_enabled = true;
					evalops.auto_post_conversion = POST_CONVERSION_BASE;
					str_conv = "";
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, _("mixed"))) {
					evalops.parse_options.units_enabled = true;
					evalops.auto_post_conversion = POST_CONVERSION_NONE;
					evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, _("factors")) || equalsIgnoreCase(to_str, "factor")) {
					do_factors = true;
					str = from_str;
				} else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction"))) {
					do_pfe = true;
					str = from_str;
				} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, _("base"))) {
					base_from_string(to_str2, to_base, to_nbase);
					to_duo_syms = false;
					do_to = true;
				} else if(equalsIgnoreCase(to_str, "decimals") || equalsIgnoreCase(to_str, _("decimals"))) {
					to_fixed_fraction = 0;
					to_fraction = 3;
					do_to = true;
				} else {
					do_to = true;
					long int fden = get_fixed_denominator_gtk(unlocalize_expression(to_str), to_fraction);
					if(fden != 0) {
						if(fden < 0) to_fixed_fraction = 0;
						else to_fixed_fraction = fden;
					} else {
						if(to_str[0] == '?') {
							to_prefix = 1;
						} else if(to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
							to_prefix = to_str[0];
						}
						Unit *u = CALCULATOR->getActiveUnit(to_str);
						if(delay_complex != (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS) && u && u->baseUnit() == CALCULATOR->getRadUnit() && u->baseExponent() == 1) delay_complex = !delay_complex;
						if(!str_conv.empty()) str_conv += " to ";
						str_conv += to_str;
					}
				}
				if(str_left.empty()) break;
				to_str = str_left;
			}
			if(do_to) {
				str = from_str;
				if(!str_conv.empty()) {
					str += " to ";
					str += str_conv;
				}
			}
		}
		if(!delay_complex || (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS)) {
			evalops.complex_number_form = cnf;
			delay_complex = false;
		} else {
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		}
		if(origstr) {
			size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
			if(i != string::npos) {
				to_str = str.substr(0, i);
				if(to_str == "factor" || equalsIgnoreCase(to_str, "factorize") || equalsIgnoreCase(to_str, _("factorize"))) {
					str = str.substr(i + 1);
					do_factors = true;
				} else if(equalsIgnoreCase(to_str, "expand") || equalsIgnoreCase(to_str, _("expand"))) {
					str = str.substr(i + 1);
					do_expand = true;
				}
			}
		}
		if(origstr && str_conv.empty() && continuous_conversion && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
			ParseOptions pa = evalops.parse_options; pa.base = 10;
			string ceu_str = CALCULATOR->unlocalizeExpression(current_conversion_expression(), pa);
			remove_blank_ends(ceu_str);
			if(set_missing_prefixes && !ceu_str.empty()) {
				if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
					ceu_str = "?" + ceu_str;
				}
			}
			if(ceu_str.empty()) {
				parsed_tostruct->setUndefined();
			} else {
				if(ceu_str[0] == '?') {
					to_prefix = 1;
				} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
					to_prefix = ceu_str[0];
				}
				parsed_tostruct->set(ceu_str);
			}
		} else {
			parsed_tostruct->setUndefined();
		}

		block_error();
		
		CALCULATOR->resetExchangeRatesUsed();

		CALCULATOR->beginTemporaryStopMessages();
		if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
		CALCULATOR->setSimplifiedPercentageUsed(false);
		if(!CALCULATOR->calculate(&mauto, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 100, evalops, parsed_mstruct, parsed_tostruct)) {
			mauto.setAborted();
		} else if(do_factors || do_pfe || do_expand) {
			CALCULATOR->startControl(100);
			if(do_factors) {
				if((mauto.isNumber() || mauto.isVector()) && to_fraction == 0 && to_fixed_fraction == 0) to_fraction = 2;
				if(!mauto.integerFactorize()) {
					mauto.structure(STRUCTURING_FACTORIZE, evalops, true);
				}
			} else if(do_pfe) {
				mauto.expandPartialFractions(evalops);
			} else if(do_expand) {
				mauto.expand(evalops);
			}
			if(CALCULATOR->aborted()) mauto.setAborted();
			CALCULATOR->stopControl();
		// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
		} else if((!parsed_tostruct || parsed_tostruct->isUndefined()) && origstr && !had_to_expression && (evalops.approximation == APPROXIMATION_EXACT || evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || evalops.auto_post_conversion == POST_CONVERSION_NONE) && parsed_mstruct && ((parsed_mstruct->isMultiplication() && parsed_mstruct->size() == 2 && (*parsed_mstruct)[0].isNumber() && (*parsed_mstruct)[1].isUnit_exp() && parsed_mstruct->equals(mauto)) || (parsed_mstruct->isNegate() && (*parsed_mstruct)[0].isMultiplication() && (*parsed_mstruct)[0].size() == 2 && (*parsed_mstruct)[0][0].isNumber() && (*parsed_mstruct)[0][1].isUnit_exp() && mauto.isMultiplication() && mauto.size() == 2 && mauto[1] == (*parsed_mstruct)[0][1] && mauto[0].isNumber() && (*parsed_mstruct)[0][0].number() == -mauto[0].number()) || (parsed_mstruct->isUnit_exp() && parsed_mstruct->equals(mauto)))) {
			Unit *u = NULL;
			MathStructure *munit = NULL;
			if(mauto.isMultiplication()) munit = &mauto[1];
			else munit = &mauto;
			if(munit->isUnit()) u = munit->unit();
			else u = (*munit)[0].unit();
			if(u && u->isCurrency()) {
				if(evalops.local_currency_conversion && CALCULATOR->getLocalCurrency() && u != CALCULATOR->getLocalCurrency()) {
					ApproximationMode abak = evalops.approximation;
					if(evalops.approximation == APPROXIMATION_EXACT) evalops.approximation = APPROXIMATION_TRY_EXACT;
					mauto.set(CALCULATOR->convertToOptimalUnit(mauto, evalops, true));
					evalops.approximation = abak;
				}
			} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
				MathStructure mbak(mauto);
				if(evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || evalops.auto_post_conversion == POST_CONVERSION_NONE) {
					if(munit->isUnit() && u->referenceName() == "oF") {
						u = CALCULATOR->getActiveUnit("oC");
						if(u) mauto.set(CALCULATOR->convert(mauto, u, evalops, true, false, false));
					} else if(munit->isUnit() && u->referenceName() == "oC") {
						u = CALCULATOR->getActiveUnit("oF");
						if(u) mauto.set(CALCULATOR->convert(mauto, u, evalops, true, false, false));
					} else {
						mauto.set(CALCULATOR->convertToOptimalUnit(mauto, evalops, true));
					}
				}
				if(evalops.approximation == APPROXIMATION_EXACT && ((evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL && evalops.auto_post_conversion != POST_CONVERSION_NONE) || mauto.equals(mbak))) {
					evalops.approximation = APPROXIMATION_TRY_EXACT;
					if(evalops.auto_post_conversion == POST_CONVERSION_BASE) mauto.set(CALCULATOR->convertToBaseUnits(mauto, evalops));
					else mauto.set(CALCULATOR->convertToOptimalUnit(mauto, evalops, true));
					evalops.approximation = APPROXIMATION_EXACT;
				}
			}
		}
		if(delay_complex) {
			CALCULATOR->startControl(100);
			evalops.complex_number_form = cnf;
			if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mstruct->complexToCisForm(evalops);
			else if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mstruct->complexToPolarForm(evalops);
			CALCULATOR->stopControl();
		}
		if(!parsed_tostruct->isUndefined() && origstr && str_conv.empty() && !mauto.containsType(STRUCT_UNIT, true)) parsed_tostruct->setUndefined();
		if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);
		CALCULATOR->endTemporaryStopMessages(!mauto.isAborted(), &autocalc_messages);
		if(!mauto.isAborted()) {
			mstruct->set(mauto);
			if(autocalc_history_delay >= 0 && auto_calculate && !parsed_in_result) {
				autocalc_history_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, autocalc_history_delay, do_autocalc_history_timeout, NULL, NULL);
			}
		} else {
			result_autocalculated = false;
			if(parsed_in_result) display_parse_status();
		}
	} else {
		block_error();
	}
	if(!recalculate || !mauto.isAborted()) {

		CALCULATOR->beginTemporaryStopMessages();

		CALCULATOR->startControl(100);

		if(to_base != 0 || to_fraction > 0 || to_fixed_fraction >= 2 || to_prefix != 0 || (to_caf >= 0 && to_caf != complex_angle_form)) {
			if(to_base != 0 && (to_base != printops.base || to_bits != printops.binary_bits || (to_base == BASE_CUSTOM && to_nbase != CALCULATOR->customOutputBase()) || (to_base == BASE_DUODECIMAL && to_duo_syms != printops.duodecimal_symbols))) {
				printops.base = to_base;
				printops.duodecimal_symbols = to_duo_syms;
				printops.binary_bits = to_bits;
				if(to_base == BASE_CUSTOM) {
					custom_base_set = true;
					save_nbase = CALCULATOR->customOutputBase();
					CALCULATOR->setCustomOutputBase(to_nbase);
				}
				do_to = true;
			}
			if(to_fixed_fraction >= 2) {
				if(to_fraction == 2 || (to_fraction < 0 && !contains_fraction_gtk(mauto))) printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
				else printops.number_fraction_format = FRACTION_COMBINED_FIXED_DENOMINATOR;
				CALCULATOR->setFixedDenominator(to_fixed_fraction);
				do_to = true;
			} else if(to_fraction > 0 && (printops.restrict_fraction_length || (to_fraction != 2 && printops.number_fraction_format != FRACTION_COMBINED) || (to_fraction == 2 && printops.number_fraction_format != FRACTION_FRACTIONAL) || (to_fraction == 3 && printops.number_fraction_format != FRACTION_DECIMAL))) {
				printops.restrict_fraction_length = false;
				if(to_fraction == 3) printops.number_fraction_format = FRACTION_DECIMAL;
				else if(to_fraction == 2) printops.number_fraction_format = FRACTION_FRACTIONAL;
				else printops.number_fraction_format = FRACTION_COMBINED;
				do_to = true;
			}
			if(to_caf >= 0 && to_caf != complex_angle_form) {
				complex_angle_form = to_caf;
				do_to = true;
			}
			if(to_prefix != 0) {
				bool new_pre = printops.use_unit_prefixes;
				bool new_cur = printops.use_prefixes_for_currencies;
				bool new_allu = printops.use_prefixes_for_all_units;
				bool new_all = printops.use_all_prefixes;
				bool new_den = printops.use_denominator_prefix;
				int new_bin = CALCULATOR->usesBinaryPrefixes();
				new_pre = true;
				if(to_prefix == 'b') {
					int i = has_information_unit_gtk(*mstruct);
					new_bin = (i > 0 ? 1 : 2);
					if(i == 1) {
						new_den = false;
					} else if(i > 1) {
						new_den = true;
					} else {
						new_cur = true;
						new_allu = true;
					}
				} else {
					new_cur = true;
					new_allu = true;
					if(to_prefix == 'a') new_all = true;
					else if(to_prefix == 'd') new_bin = 0;
				}
				if(printops.use_unit_prefixes != new_pre || printops.use_prefixes_for_currencies != new_cur || printops.use_prefixes_for_all_units != new_allu || printops.use_all_prefixes != new_all || printops.use_denominator_prefix != new_den || CALCULATOR->usesBinaryPrefixes() != new_bin) {
					printops.use_unit_prefixes = new_pre;
					printops.use_all_prefixes = new_all;
					printops.use_prefixes_for_currencies = new_cur;
					printops.use_prefixes_for_all_units = new_allu;
					printops.use_denominator_prefix = new_den;
					CALCULATOR->useBinaryPrefixes(new_bin);
					do_to = true;
				}
			}
		}

		MathStructure *displayed_mstruct_pre = new MathStructure();
		displayed_mstruct_pre->set(parsed_in_result ? mauto : *mstruct);
		if(printops.interval_display == INTERVAL_DISPLAY_INTERVAL) replace_interval_with_function(*displayed_mstruct_pre);

		printops.allow_non_usable = true;
		printops.can_display_unicode_string_arg = (void*) resultview;

		date_map.clear();
		date_approx_map.clear();
		number_map.clear();
		number_base_map.clear();
		number_exp_map.clear();
		number_exp_minus_map.clear();
		number_approx_map.clear();

		// convert time units to hours when using time format
		if(printops.base == BASE_TIME && is_time(*displayed_mstruct_pre)) {
			Unit *u = CALCULATOR->getActiveUnit("h");
			if(u) {
				displayed_mstruct_pre->divide(u);
				displayed_mstruct_pre->eval(evalops);
			}
		}

		if(printops.spell_out_logical_operators && parsed_mstruct && test_parsed_comparison_gtk(*parsed_mstruct)) {
			if(displayed_mstruct_pre->isZero()) {
				Variable *v = CALCULATOR->getActiveVariable("false");
				if(v) displayed_mstruct_pre->set(v);
			} else if(displayed_mstruct_pre->isOne()) {
				Variable *v = CALCULATOR->getActiveVariable("true");
				if(v) displayed_mstruct_pre->set(v);
			}
		}

		displayed_mstruct_pre->removeDefaultAngleUnit(evalops);
		displayed_mstruct_pre->format(printops);
		displayed_mstruct_pre->removeDefaultAngleUnit(evalops);
		if(parsed_in_result) {
			PrintOptions po = printops;
			po.base_display = BASE_DISPLAY_SUFFIX;
			result_text = displayed_mstruct_pre->print(printops, true);
			result_text_approximate = *po.is_approximate;
			fix_history_string_new2(result_text);
			gsub("&nbsp;", " ", result_text);
			printops.can_display_unicode_string_arg = NULL;
			printops.allow_non_usable = false;
			result_text_long = "";
			if(CALCULATOR->aborted()) {
				CALCULATOR->endTemporaryStopMessages();
				result_text = "";
				result_autocalculated = false;
				displayed_mstruct_pre->unref();
			} else {
				CALCULATOR->endTemporaryStopMessages(true);
				if(complex_angle_form) replace_result_cis_gtk(result_text);
				if(visible_keypad & PROGRAMMING_KEYPAD) {
					set_result_bases(*displayed_mstruct_pre);
					update_result_bases();
				}
				result_autocalculated = true;
			}
			display_parse_status();
		} else {
			tmp_surface = draw_structure(*displayed_mstruct_pre, printops, complex_angle_form, top_ips, NULL, 0);
			printops.can_display_unicode_string_arg = NULL;
			printops.allow_non_usable = false;
			if(tmp_surface && CALCULATOR->aborted()) {
				CALCULATOR->endTemporaryStopMessages();
				cairo_surface_destroy(tmp_surface);
				tmp_surface = NULL;
				clearresult();
				displayed_mstruct_pre->unref();
				result_autocalculated = true;
			} else if(tmp_surface) {
				CALCULATOR->endTemporaryStopMessages(true);
				scale_n = 0;
				showing_first_time_message = false;
				if(surface_result) cairo_surface_destroy(surface_result);
				if(displayed_mstruct) displayed_mstruct->unref();
				displayed_mstruct = displayed_mstruct_pre;
				displayed_printops = printops;
				displayed_printops.allow_non_usable = true;
				displayed_caf = complex_angle_form;
				result_autocalculated = true;
				display_aborted = false;
				surface_result = tmp_surface;
				first_draw_of_result = true;
				minimal_mode_show_resultview();
				gtk_widget_queue_draw(resultview);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), true);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), true);
				if(autocalc_history_timeout_id == 0 || ((printops.base == BASE_BINARY || (printops.base <= BASE_FP16 && printops.base >= BASE_FP80)) && displayed_mstruct->isInteger())) {
					PrintOptions po = printops;
					po.base_display = BASE_DISPLAY_SUFFIX;
					result_text = displayed_mstruct->print(po, true);
					gsub("&nbsp;", " ", result_text);
					if(complex_angle_form) replace_result_cis_gtk(result_text);
				} else {
					result_text = "";
				}
				result_text_long = "";
				gtk_widget_set_tooltip_text(resultview, "");
				if(!display_errors(NULL, 1)) update_expression_icons(EXPRESSION_CLEAR);
				if(visible_keypad & PROGRAMMING_KEYPAD) {
					set_result_bases(*displayed_mstruct);
					update_result_bases();
				}
			} else {
				displayed_mstruct_pre->unref();
				CALCULATOR->endTemporaryStopMessages();
				clearresult();
			}
		}
		CALCULATOR->stopControl();
	} else if(parsed_in_result) {
		result_text = "";
		result_autocalculated = false;
		display_parse_status();
	} else {
		auto_calculate = false;
		clearresult();
		auto_calculate = true;
	}

	if(do_to) {
		printops.base = save_base;
		printops.binary_bits = save_bits;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_nbase);
		printops.use_unit_prefixes = save_pre;
		printops.use_all_prefixes = save_all;
		printops.use_prefixes_for_currencies = save_cur;
		printops.use_prefixes_for_all_units = save_allu;
		printops.use_denominator_prefix = save_den;
		CALCULATOR->useBinaryPrefixes(save_bin);
		CALCULATOR->setFixedDenominator(save_fden);
		printops.number_fraction_format = save_format;
		printops.restrict_fraction_length = save_restrict_fraction_length;
		complex_angle_form = caf_bak;
		evalops.complex_number_form = cnf_bak;
		evalops.auto_post_conversion = save_auto_post_conversion;
		evalops.parse_options.units_enabled = b_units_saved;
		evalops.mixed_units_conversion = save_mixed_units_conversion;
		printops.time_zone = TIME_ZONE_LOCAL;
	}
	
	unblock_error();
}
void print_auto_calc() {
	do_auto_calc(false);
}

void autocalc_result_bases() {
	string str = CALCULATOR->unlocalizeExpression(get_expression_text(), evalops.parse_options);
	CALCULATOR->parseSigns(str);
	remove_blank_ends(str);
	if(str.empty()) return;
	for(size_t i = 0; i < str.length(); i++) {
		if((str[i] < '0' || str[i] > '9') && (str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && str[i] != ' ' && (i > 0 || str[i] != '-')) return;
	}
	CALCULATOR->beginTemporaryStopMessages();
	ParseOptions pa = evalops.parse_options;
	pa.preserve_format = true;
	MathStructure m;
	CALCULATOR->parse(&m, str, pa);
	if(!CALCULATOR->endTemporaryStopMessages() && (m.isInteger() || (m.isNegate() && m[0].isInteger()))) {
		if(m.isNegate()) {m.setToChild(1); m.number().negate();}
		set_result_bases(m);
		update_result_bases();
	}
}

void handle_expression_modified(bool autocalc) {
	show_parsed_instead_of_result = false;
	if(!parsed_in_result || rpn_mode) display_parse_status();
	if(autocalc && !rpn_mode && auto_calculate && !parsed_in_result) do_auto_calc(2);
	if(result_text.empty() && !autocalc_history_timeout_id && (!parsed_in_result || rpn_mode) && (!chain_mode || auto_calculate)) return;
	if(!dont_change_index) expression_history_index = -1;
	if((!autocalc || !auto_calculate || parsed_in_result) && !rpn_mode) {
		clearresult();
	}
	if(parsed_in_result && !rpn_mode) {
		display_parse_status();
		if(autocalc && auto_calculate) do_auto_calc(2);
	}
	if(autocalc && !rpn_mode && !auto_calculate && (visible_keypad & PROGRAMMING_KEYPAD)) autocalc_result_bases();
}

bool do_chain_mode(const gchar *op) {
	if(!rpn_mode && chain_mode && !current_function && evalops.parse_options.base != BASE_UNICODE && (evalops.parse_options.base != BASE_CUSTOM || (CALCULATOR->customInputBase() <= 62 && CALCULATOR->customInputBase() >= -62))) {
		GtkTextIter iend, istart;
		gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iend, gtk_text_buffer_get_insert(expressionbuffer));
		if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
			GtkTextMark *mstart = gtk_text_buffer_get_selection_bound(expressionbuffer);
			if(mstart) {
				gtk_text_buffer_get_iter_at_mark(expressionbuffer, &istart, mstart);
				if((!gtk_text_iter_is_start(&istart) || !gtk_text_iter_is_end(&iend)) && (!gtk_text_iter_is_end(&istart) || !gtk_text_iter_is_start(&iend))) return false;
			}
		} else {
			if(!gtk_text_iter_is_end(&iend)) return false;
		}
		string str = get_expression_text();
		remove_blanks(str);
		if(str.empty() || str[0] == '/' || CALCULATOR->hasToExpression(str, true, evalops) || CALCULATOR->hasWhereExpression(str, evalops) || last_is_operator(str)) return false;
		size_t par_n = 0, vec_n = 0;
		for(size_t i = 0; i < str.length(); i++) {
			if(str[i] == LEFT_PARENTHESIS_CH) par_n++;
			else if(par_n > 0 && str[i] == RIGHT_PARENTHESIS_CH) par_n--;
			else if(str[i] == LEFT_VECTOR_WRAP_CH) vec_n++;
			else if(vec_n > 0 && str[i] == RIGHT_VECTOR_WRAP_CH) vec_n--;
		}
		if(par_n > 0 || vec_n > 0) return false;
		if(!auto_calculate) do_auto_calc();
		rpn_mode = true;
		if(get_expression_text().find_first_not_of(NUMBER_ELEMENTS SPACE) != string::npos && (!parsed_mstruct || ((!parsed_mstruct->isMultiplication() || op != expression_times_sign()) && (!parsed_mstruct->isAddition() || (op != expression_add_sign() && op != expression_sub_sign())) && (!parsed_mstruct->isBitwiseOr() || strcmp(op, BITWISE_OR) != 0) && (!parsed_mstruct->isBitwiseAnd() || strcmp(op, BITWISE_AND) != 0)))) {
			block_undo();
			gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
			gtk_text_buffer_insert(expressionbuffer, &istart, "(", -1);
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			gtk_text_buffer_insert(expressionbuffer, &iend, ")", -1);
			gtk_text_buffer_place_cursor(expressionbuffer, &iend);
			unblock_undo();
		} else if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			gtk_text_buffer_place_cursor(expressionbuffer, &iend);
		}
		insert_text(op);
		rpn_mode = false;
		return true;
	}
	return false;
}

MathStructure *current_from_struct = NULL;
vector<Unit*> current_from_units;

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
		for(size_t i = 0; i < m.size();) {
			if(m[i].isFunction() || m[i].containsType(STRUCT_UNIT, true) <= 0) {
				m.delChild(i + 1);
			} else {
				remove_non_units(m[i]);
				i++;
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
		return;
	}
	if(top) {
		if(mparse && !m.containsType(STRUCT_UNIT, true) && (mparse->containsFunctionId(FUNCTION_ID_ASIN) || mparse->containsFunctionId(FUNCTION_ID_ACOS) || mparse->containsFunctionId(FUNCTION_ID_ATAN))) {
			v.push_back(CALCULATOR->getRadUnit());
			return;
		}
		MathStructure m2(m);
		remove_non_units(m2);
		CALCULATOR->beginTemporaryStopMessages();
		m2 = CALCULATOR->convertToOptimalUnit(m2, evalops, evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL_SI);
		CALCULATOR->endTemporaryStopMessages();
		find_matching_units(m2, mparse, v, false);
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			if(!m.isFunction() || !m.function()->getArgumentDefinition(i + 1) || m.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
				find_matching_units(m[i], mparse, v, false);
			}
		}
	}
}

#define CLEAR_PARSED_IN_RESULT \
	if(surface_parsed) {\
		cairo_surface_destroy(surface_parsed);\
		surface_parsed = NULL;\
		if(!surface_result) {\
			gtk_widget_queue_draw(resultview);\
			minimal_mode_show_resultview(false);\
		}\
	}

MathStructure mwhere;
vector<MathStructure> displayed_parsed_to;

void set_expression_output_updated(bool b) {
	expression_has_changed2 = b;
}
bool expression_output_updated() {
	return expression_has_changed2;
}

void display_parse_status() {
	current_function = NULL;
	if(auto_calculate && !rpn_mode && expression_output_updated()) current_status_struct.setAborted();
	if(!display_expression_status) return;
	if(block_display_parse) return;
	GtkTextIter istart, iend, ipos;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	string text = gtext, str_f;
	g_free(gtext);
	bool double_tag = false;
	string to_str = CALCULATOR->parseComments(text, evalops.parse_options, &double_tag);
	if(!to_str.empty() && text.empty() && double_tag) {
		text = CALCULATOR->f_message->referenceName();
		text += "(";
		if(to_str.find("\"") == string::npos) {text += "\""; text += to_str; text += "\"";}
		else if(to_str.find("\'") == string::npos) {text += "\'"; text += to_str; text += "\'";}
		else text += to_str;
		text += ")";
	}
	if(text.empty()) {
		set_status_text("", true, false, false);
		parsed_expression = "";
		parsed_expression_tooltip = "";
		set_expression_output_updated(false);
		CLEAR_PARSED_IN_RESULT
		return;
	}
	if(text[0] == '/' && text.length() > 1) {
		size_t i = text.find_first_not_of(SPACES, 1);
		if(i != string::npos && (signed char) text[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, text[i])) {
			set_status_text("qalc command", true, false, false);
			status_text_source = STATUS_TEXT_OTHER;
			CLEAR_PARSED_IN_RESULT
			return;
		}
	} else if(text == "MC") {
		set_status_text(_("MC (memory clear)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		CLEAR_PARSED_IN_RESULT
		return;
	} else if(text == "MS") {
		set_status_text(_("MS (memory store)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		CLEAR_PARSED_IN_RESULT
		return;
	} else if(text == "M+") {
		set_status_text(_("M+ (memory plus)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		CLEAR_PARSED_IN_RESULT
		return;
	} else if(text == "M-" || text == "M−") {
		set_status_text(_("M− (memory minus)"), true, false, false);
		status_text_source = STATUS_TEXT_OTHER;
		CLEAR_PARSED_IN_RESULT
		return;
	}
	gsub(ID_WRAP_LEFT, LEFT_PARENTHESIS, text);
	gsub(ID_WRAP_RIGHT, RIGHT_PARENTHESIS, text);
	remove_duplicate_blanks(text);
	size_t i = text.find_first_of(SPACES LEFT_PARENTHESIS);
	if(i != string::npos) {
		str_f = text.substr(0, i);
		if(str_f == "factor" || equalsIgnoreCase(str_f, "factorize") || equalsIgnoreCase(str_f, _("factorize"))) {
			text = text.substr(i + 1);
			str_f = _("factorize");
		} else if(equalsIgnoreCase(str_f, "expand") || equalsIgnoreCase(str_f, _("expand"))) {
			text = text.substr(i + 1);
			str_f = _("expand");
		} else {
			str_f = "";
		}
	}
	GtkTextMark *mark = gtk_text_buffer_get_insert(expressionbuffer);
	if(mark) gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mark);
	else ipos = iend;
	MathStructure mparse, mfunc;
	bool full_parsed = false;
	string str_e, str_u, str_w;
	bool had_errors = false, had_warnings = false;
	if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	evalops.parse_options.preserve_format = true;
	on_display_errors_timeout(NULL);
	block_error();
	if(!gtk_text_iter_is_start(&ipos)) {
		evalops.parse_options.unended_function = &mfunc;
		if(!gtk_text_iter_is_end(&ipos)) {
			if(current_from_struct) {current_from_struct->unref(); current_from_struct = NULL; current_from_units.clear();}
			gtext = gtk_text_buffer_get_text(expressionbuffer, &istart, &ipos, FALSE);
			str_e = CALCULATOR->unlocalizeExpression(gtext, evalops.parse_options);
			bool b = CALCULATOR->separateToExpression(str_e, str_u, evalops, false, !auto_calculate || rpn_mode || parsed_in_result);
			b = CALCULATOR->separateWhereExpression(str_e, str_w, evalops) || b;
			if(!b) {
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
				CALCULATOR->endTemporaryStopMessages();
			}
			g_free(gtext);
		} else {
			str_e = CALCULATOR->unlocalizeExpression(text, evalops.parse_options);
			transform_expression_for_equals_save(str_e, evalops.parse_options);
			bool b = CALCULATOR->separateToExpression(str_e, str_u, evalops, false, !auto_calculate || rpn_mode || parsed_in_result);
			b = CALCULATOR->separateWhereExpression(str_e, str_w, evalops) || b;
			if(!b) {
				CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
				if(current_from_struct) {current_from_struct->unref(); current_from_struct = NULL; current_from_units.clear();}
				full_parsed = true;
			}
		}
		evalops.parse_options.unended_function = NULL;
	}
	bool b_func = false;
	if(mfunc.isFunction()) {
		current_function = mfunc.function();
		if(mfunc.countChildren() == 0) {
			current_function_index = 1;
			b_func = display_function_hint(mfunc.function(), 1);
		} else {
			current_function_index = mfunc.countChildren();
			b_func = display_function_hint(mfunc.function(), mfunc.countChildren());
		}
	}
	if(expression_output_updated()) {
		bool last_is_space = false;
		parsed_expression_tooltip = "";
		if(!full_parsed) {
			str_e = CALCULATOR->unlocalizeExpression(text, evalops.parse_options);
			transform_expression_for_equals_save(str_e, evalops.parse_options);
			last_is_space = is_in(SPACES, str_e[str_e.length() - 1]);
			bool b_to = CALCULATOR->separateToExpression(str_e, str_u, evalops, true, !auto_calculate || rpn_mode || parsed_in_result);
			CALCULATOR->separateWhereExpression(str_e, str_w, evalops);
			if(!str_e.empty()) CALCULATOR->parse(&mparse, str_e, evalops.parse_options);
			if(b_to && !str_e.empty()) {
				if(!current_from_struct && !mparse.containsFunction(CALCULATOR->f_save) && (!CALCULATOR->f_plot || !mparse.containsFunction(CALCULATOR->f_plot))) {
					current_from_struct = new MathStructure;
					EvaluationOptions eo = evalops;
					eo.structuring = STRUCTURING_NONE;
					eo.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
					eo.auto_post_conversion = POST_CONVERSION_NONE;
					eo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					eo.expand = -2;
					if(!CALCULATOR->calculate(current_from_struct, str_w.empty() ? str_e : str_e + "/." + str_w, 20, eo)) current_from_struct->setAborted();
					find_matching_units(*current_from_struct, &mparse, current_from_units);
				}
			} else if(current_from_struct) {
				current_from_struct->unref();
				current_from_struct = NULL;
				current_from_units.clear();
			}
		}
		if(auto_calculate && !rpn_mode) current_status_struct = mparse;
		PrintOptions po;
		po.preserve_format = true;
		po.show_ending_zeroes = evalops.parse_options.read_precision != DONT_READ_PRECISION && !CALCULATOR->usesIntervalArithmetic() && evalops.parse_options.base > BASE_CUSTOM;
		po.exp_display = printops.exp_display;
		po.lower_case_numbers = printops.lower_case_numbers;
		po.base_display = printops.base_display;
		po.twos_complement = printops.twos_complement;
		po.hexadecimal_twos_complement = printops.hexadecimal_twos_complement;
		po.round_halfway_to_even = printops.round_halfway_to_even;
		po.base = evalops.parse_options.base;
		Number nr_base;
		if(po.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
			nr_base = CALCULATOR->customOutputBase();
			CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
		} else if(po.base == BASE_CUSTOM || (po.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26 && po.base != BASE_BINARY_DECIMAL)) {
			po.base = 10;
			po.min_exp = 6;
			po.use_max_decimals = true;
			po.max_decimals = 5;
			po.preserve_format = false;
		}
		po.abbreviate_names = false;
		po.hide_underscore_spaces = true;
		po.use_unicode_signs = printops.use_unicode_signs;
		po.digit_grouping = printops.digit_grouping;
		po.multiplication_sign = printops.multiplication_sign;
		po.division_sign = printops.division_sign;
		po.short_multiplication = false;
		po.excessive_parenthesis = true;
		po.improve_division_multipliers = false;
		po.can_display_unicode_string_function = &can_display_unicode_string_function;
		po.can_display_unicode_string_arg = (void*) statuslabel_l;
		po.spell_out_logical_operators = printops.spell_out_logical_operators;
		po.restrict_to_parent_precision = false;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		size_t parse_l = 0;
		mwhere.clear();
		if(!str_w.empty()) {
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->parseExpressionAndWhere(&mparse, &mwhere, str_e, str_w, evalops.parse_options);
			mparse.format(po);
			parsed_expression = mparse.print(po, true, false, TAG_TYPE_HTML);
			parse_l = parsed_expression.length();
			parsed_expression += CALCULATOR->localWhereString();
			mwhere.format(po);
			parsed_expression += mwhere.print(po, true, false, TAG_TYPE_HTML);
			CALCULATOR->endTemporaryStopMessages();
		} else if(str_e.empty()) {
			parsed_expression = "";
		} else {
			CALCULATOR->beginTemporaryStopMessages();
			mparse.format(po);
			parsed_expression = mparse.print(po, true, false, TAG_TYPE_HTML);
			parse_l = parsed_expression.length();
			CALCULATOR->endTemporaryStopMessages();
		}
		displayed_parsed_to.clear();
		if(!str_u.empty()) {
			string str_u2;
			bool had_to_conv = false;
			MathStructure *mparse2 = NULL;
			while(true) {
				bool unit_struct = false;
				if(last_is_space) str_u += " ";
				CALCULATOR->separateToExpression(str_u, str_u2, evalops, true, false);
				remove_blank_ends(str_u);
				if(parsed_expression.empty()) {
					parsed_expression += CALCULATOR->localToString(false);
					parsed_expression += " ";
				} else {
					parsed_expression += CALCULATOR->localToString();
				}
				size_t to_begin = parsed_expression.length();
				remove_duplicate_blanks(str_u);
				string to_str1, to_str2;
				size_t ispace = str_u.find_first_of(SPACES);
				if(ispace != string::npos) {
					to_str1 = str_u.substr(0, ispace);
					remove_blank_ends(to_str1);
					to_str2 = str_u.substr(ispace + 1);
					remove_blank_ends(to_str2);
				}
				if(equalsIgnoreCase(str_u, "hex") || equalsIgnoreCase(str_u, "hexadecimal") || equalsIgnoreCase(str_u, _("hexadecimal"))) {
					parsed_expression += _("hexadecimal number");
				} else if(equalsIgnoreCase(str_u, "oct") || equalsIgnoreCase(str_u, "octal") || equalsIgnoreCase(str_u, _("octal"))) {
					parsed_expression += _("octal number");
				} else if(equalsIgnoreCase(str_u, "dec") || equalsIgnoreCase(str_u, "decimal") || equalsIgnoreCase(str_u, _("decimal"))) {
					parsed_expression += _("decimal number");
				} else if(equalsIgnoreCase(str_u, "duo") || equalsIgnoreCase(str_u, "duodecimal") || equalsIgnoreCase(str_u, _("duodecimal"))) {
					parsed_expression += _("duodecimal number");
				} else if(equalsIgnoreCase(str_u, "doz") || equalsIgnoreCase(str_u, "dozenal")) {
					parsed_expression += _("duodecimal number");
				} else if(equalsIgnoreCase(str_u, "bin") || equalsIgnoreCase(str_u, "binary") || equalsIgnoreCase(str_u, _("binary"))) {
					parsed_expression += _("binary number");
				} else if(equalsIgnoreCase(str_u, "roman") || equalsIgnoreCase(str_u, _("roman"))) {
					parsed_expression += _("roman numerals");
				} else if(equalsIgnoreCase(str_u, "bijective") || equalsIgnoreCase(str_u, _("bijective"))) {
					parsed_expression += _("bijective base-26");
				} else if(equalsIgnoreCase(str_u, "bcd")) {
					parsed_expression += _("binary-coded decimal");
				} else if(equalsIgnoreCase(str_u, "sexa") || equalsIgnoreCase(str_u, "sexa2") || equalsIgnoreCase(str_u, "sexa3") || equalsIgnoreCase(str_u, "sexagesimal") || equalsIgnoreCase(str_u, _("sexagesimal")) || EQUALS_IGNORECASE_AND_LOCAL_NR(str_u, "sexagesimal", _("sexagesimal"), "2") || EQUALS_IGNORECASE_AND_LOCAL_NR(str_u, "sexagesimal", _("sexagesimal"), "3")) {
					parsed_expression += _("sexagesimal number");
				} else if(equalsIgnoreCase(str_u, "latitude") || equalsIgnoreCase(str_u, _("latitude")) || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", _("latitude"), "2")) {
					parsed_expression += _("latitude");
				} else if(equalsIgnoreCase(str_u, "longitude") || equalsIgnoreCase(str_u, _("longitude")) || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", _("longitude"), "2")) {
					parsed_expression += _("longitude");
				} else if(equalsIgnoreCase(str_u, "fp32") || equalsIgnoreCase(str_u, "binary32") || equalsIgnoreCase(str_u, "float")) {
					parsed_expression += _("32-bit floating point");
				} else if(equalsIgnoreCase(str_u, "fp64") || equalsIgnoreCase(str_u, "binary64") || equalsIgnoreCase(str_u, "double")) {
					parsed_expression += _("64-bit floating point");
				} else if(equalsIgnoreCase(str_u, "fp16") || equalsIgnoreCase(str_u, "binary16")) {
					parsed_expression += _("16-bit floating point");
				} else if(equalsIgnoreCase(str_u, "fp80")) {
					parsed_expression += _("80-bit (x86) floating point");
				} else if(equalsIgnoreCase(str_u, "fp128") || equalsIgnoreCase(str_u, "binary128")) {
					parsed_expression += _("128-bit floating point");
				} else if(equalsIgnoreCase(str_u, "time") || equalsIgnoreCase(str_u, _("time"))) {
					parsed_expression += _("time format");
				} else if(equalsIgnoreCase(str_u, "unicode")) {
					parsed_expression += _("Unicode");
				} else if(equalsIgnoreCase(str_u, "bases") || equalsIgnoreCase(str_u, _("bases"))) {
					parsed_expression += _("number bases");
				} else if(equalsIgnoreCase(str_u, "calendars") || equalsIgnoreCase(str_u, _("calendars"))) {
					parsed_expression += _("calendars");
				} else if(equalsIgnoreCase(str_u, "optimal") || equalsIgnoreCase(str_u, _("optimal"))) {
					parsed_expression += _("optimal unit");
				} else if(equalsIgnoreCase(str_u, "prefix") || equalsIgnoreCase(str_u, _("prefix")) || str_u == "?" || (str_u.length() == 2 && str_u[1] == '?' && (str_u[0] == 'b' || str_u[0] == 'a' || str_u[0] == 'd'))) {
					parsed_expression += _("optimal prefix");
				} else if(equalsIgnoreCase(str_u, "base") || equalsIgnoreCase(str_u, _("base"))) {
					parsed_expression += _("base units");
				} else if(equalsIgnoreCase(str_u, "mixed") || equalsIgnoreCase(str_u, _("mixed"))) {
					parsed_expression += _("mixed units");
				} else if(equalsIgnoreCase(str_u, "factors") || equalsIgnoreCase(str_u, _("factors")) || equalsIgnoreCase(str_u, "factor")) {
					parsed_expression += _("factors");
				} else if(equalsIgnoreCase(str_u, "partial fraction") || equalsIgnoreCase(str_u, _("partial fraction"))) {
					parsed_expression += _("expanded partial fractions");
				} else if(equalsIgnoreCase(str_u, "rectangular") || equalsIgnoreCase(str_u, "cartesian") || equalsIgnoreCase(str_u, _("rectangular")) || equalsIgnoreCase(str_u, _("cartesian"))) {
					parsed_expression += _("complex rectangular form");
				} else if(equalsIgnoreCase(str_u, "exponential") || equalsIgnoreCase(str_u, _("exponential"))) {
					parsed_expression += _("complex exponential form");
				} else if(equalsIgnoreCase(str_u, "polar") || equalsIgnoreCase(str_u, _("polar"))) {
					parsed_expression += _("complex polar form");
				} else if(str_u == "cis") {
					parsed_expression += _("complex cis form");
				} else if(equalsIgnoreCase(str_u, "angle") || equalsIgnoreCase(str_u, _("angle"))) {
					parsed_expression += _("complex angle notation");
				} else if(equalsIgnoreCase(str_u, "phasor") || equalsIgnoreCase(str_u, _("phasor"))) {
					parsed_expression += _("complex phasor notation");
				} else if(equalsIgnoreCase(str_u, "utc") || equalsIgnoreCase(str_u, "gmt")) {
					parsed_expression += _("UTC time zone");
				} else if(str_u.length() > 3 && (equalsIgnoreCase(str_u.substr(0, 3), "utc") || equalsIgnoreCase(str_u.substr(0, 3), "gmt"))) {
					str_u = str_u.substr(3);
					parsed_expression += "UTC";
					remove_blanks(str_u);
					bool b_minus = false;
					if(str_u[0] == '+') {
						str_u.erase(0, 1);
					} else if(str_u[0] == '-') {
						b_minus = true;
						str_u.erase(0, 1);
					} else if(str_u.find(SIGN_MINUS) == 0) {
						b_minus = true;
						str_u.erase(0, strlen(SIGN_MINUS));
					}
					unsigned int tzh = 0, tzm = 0;
					int itz = 0;
					if(!str_u.empty() && sscanf(str_u.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
						itz = tzh * 60 + tzm;
					} else {
						had_errors = true;
					}
					if(itz > 0) {
						if(b_minus) parsed_expression += '-';
						else parsed_expression += '+';
						if(itz < 60) {
							parsed_expression += "00";
						} else {
							if(itz < 60 * 10) parsed_expression += '0';
							parsed_expression += i2s(itz / 60);
						}
						if(itz % 60 > 0) {
							parsed_expression += ":";
							if(itz % 60 < 10) parsed_expression += '0';
							parsed_expression += i2s(itz % 60);
						}
					}
				} else if(str_u.length() > 3 && equalsIgnoreCase(str_u.substr(0, 3), "bin") && is_in(NUMBERS, str_u[3])) {
					unsigned int bits = s2i(str_u.substr(3));
					if(bits > 4096) bits = 4096;
					parsed_expression += i2s(bits);
					parsed_expression += "-bit ";
					parsed_expression += _("binary number");
				} else if(str_u.length() > 3 && equalsIgnoreCase(str_u.substr(0, 3), "hex") && is_in(NUMBERS, str_u[3])) {
					unsigned int bits = s2i(str_u.substr(3));
					if(bits > 4096) bits = 4096;
					parsed_expression += i2s(bits);
					parsed_expression += "-bit ";
					parsed_expression += _("hexadecimal number");
				} else if(str_u == "CET") {
					parsed_expression += "UTC";
					parsed_expression += "+01";
				} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, _("base"))) {
					gchar *gstr = g_strdup_printf(_("number base %s"), to_str2.c_str());
					parsed_expression += gstr;
					g_free(gstr);

				} else if(equalsIgnoreCase(str_u, "decimals") || equalsIgnoreCase(str_u, _("decimals"))) {
					parsed_expression += _("decimal fraction");
				} else {
					int tofr = 0;
					long int fden = get_fixed_denominator_gtk(unlocalize_expression(str_u), tofr);
					if(fden > 0) {
						parsed_expression += _("fraction");
						parsed_expression += " (";
						parsed_expression += "1/";
						parsed_expression += i2s(fden);
						parsed_expression += ")";
					} else if(fden < 0) {
						parsed_expression += _("fraction");
					} else {
						if(str_u[0] == '0' || str_u[0] == '?' || str_u[0] == '+' || str_u[0] == '-') {
							str_u = str_u.substr(1, str_u.length() - 1);
							remove_blank_ends(str_u);
						} else if(str_u.length() > 1 && str_u[1] == '?' && (str_u[0] == 'b' || str_u[0] == 'a' || str_u[0] == 'd')) {
							str_u = str_u.substr(2, str_u.length() - 2);
							remove_blank_ends(str_u);
						}
						MathStructure mparse_to;
						Unit *u = CALCULATOR->getActiveUnit(str_u);
						if(!u) u = CALCULATOR->getCompositeUnit(str_u);
						Variable *v = NULL;
						if(!u) v = CALCULATOR->getActiveVariable(str_u);
						if(v && !v->isKnown()) v = NULL;
						Prefix *p = NULL;
						if(!u && !v && CALCULATOR->unitNameIsValid(str_u)) p = CALCULATOR->getPrefix(str_u);
						if(u) {
							mparse_to = u;
							if(!had_to_conv && !str_e.empty()) {
								CALCULATOR->beginTemporaryStopMessages();
								MathStructure to_struct = get_units_for_parsed_expression(&mparse, u, evalops, current_from_struct && !current_from_struct->isAborted() ? current_from_struct : NULL);
								if(!to_struct.isZero()) {
									mparse2 = new MathStructure();
									CALCULATOR->parse(mparse2, str_e, evalops.parse_options);
									po.preserve_format = false;
									to_struct.format(po);
									po.preserve_format = true;
									if(to_struct.isMultiplication() && to_struct.size() >= 2) {
										if(to_struct[0].isOne()) to_struct.delChild(1, true);
										else if(to_struct[1].isOne()) to_struct.delChild(2, true);
									}
									mparse2->multiply(to_struct);
								}
								CALCULATOR->endTemporaryStopMessages();
							}
						} else if(v) {
							mparse_to = v;
						} else if(!p) {
							CALCULATOR->beginTemporaryStopMessages();
							CompositeUnit cu("", evalops.parse_options.limit_implicit_multiplication ? "01" : "00", "", str_u);
							int i_warn = 0, i_error = CALCULATOR->endTemporaryStopMessages(NULL, &i_warn);
							if(!had_to_conv && cu.countUnits() > 0 && !str_e.empty()) {
								CALCULATOR->beginTemporaryStopMessages();
								MathStructure to_struct = get_units_for_parsed_expression(&mparse, &cu, evalops, current_from_struct && !current_from_struct->isAborted() ? current_from_struct : NULL);
								if(!to_struct.isZero()) {
									mparse2 = new MathStructure();
									CALCULATOR->parse(mparse2, str_e, evalops.parse_options);
									po.preserve_format = false;
									to_struct.format(po);
									po.preserve_format = true;
									if(to_struct.isMultiplication() && to_struct.size() >= 2) {
										if(to_struct[0].isOne()) to_struct.delChild(1, true);
										else if(to_struct[1].isOne()) to_struct.delChild(2, true);
									}
									mparse2->multiply(to_struct);
								}
								CALCULATOR->endTemporaryStopMessages();
							}
							if(i_error) {
								ParseOptions pa = evalops.parse_options;
								pa.units_enabled = true;
								CALCULATOR->parse(&mparse_to, str_u, pa);
							} else {
								if(i_warn > 0) had_warnings = true;
								mparse_to = cu.generateMathStructure(true);
							}
							mparse_to.format(po);
						}
						if(p) {
							parsed_expression += p->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, false, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).formattedName(-1, true, TAG_TYPE_HTML, 0, true, po.hide_underscore_spaces);
						} else {
							CALCULATOR->beginTemporaryStopMessages();
							parsed_expression += mparse_to.print(po, true, false, TAG_TYPE_HTML);
							CALCULATOR->endTemporaryStopMessages();
							if(parsed_in_result || show_parsed_instead_of_result) displayed_parsed_to.push_back(mparse_to);
							unit_struct = true;
						}
						had_to_conv = true;
					}
					if((parsed_in_result || show_parsed_instead_of_result) && !unit_struct && to_begin < parsed_expression.length()) {
						displayed_parsed_to.push_back(MathStructure(string("to expression:") + parsed_expression.substr(to_begin), true));
					}
				}
				if(str_u2.empty()) break;
				str_u = str_u2;
			}
			if(mparse2) {
				mparse2->format(po);
				parsed_expression.replace(0, parse_l, mparse2->print(po, true, false, TAG_TYPE_HTML));
				if(parsed_in_result || show_parsed_instead_of_result) mparse.set_nocopy(*mparse2);
				mparse2->unref();
			}
		}
		if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
		size_t message_n = 0;
		if(result_autocalculated && ((parsed_in_result && !rpn_mode && !result_text.empty()) || show_parsed_instead_of_result)) {
			CALCULATOR->clearMessages();
			for(size_t i = 0; i < autocalc_messages.size(); i++) {
				MessageType mtype = autocalc_messages[i].type();
				if((mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) && (!implicit_question_asked || autocalc_messages[i].category() != MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION)) {
					if(mtype == MESSAGE_ERROR) had_errors = true;
					else had_warnings = true;
					if(message_n > 0) {
						if(message_n == 1) parsed_expression_tooltip = "• " + parsed_expression_tooltip;
						parsed_expression_tooltip += "\n• ";
					}
					parsed_expression_tooltip += autocalc_messages[i].message();
					message_n++;
				}
			}
		} else {
			while(CALCULATOR->message()) {
				MessageType mtype = CALCULATOR->message()->type();
				if((mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) && (!implicit_question_asked || CALCULATOR->message()->category() != MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION)) {
					if(mtype == MESSAGE_ERROR) had_errors = true;
					else had_warnings = true;
					if(message_n > 0) {
						if(message_n == 1) parsed_expression_tooltip = "• " + parsed_expression_tooltip;
						parsed_expression_tooltip += "\n• ";
					}
					parsed_expression_tooltip += CALCULATOR->message()->message();
					message_n++;
				}
				CALCULATOR->nextMessage();
			}
		}
		unblock_error();
		parsed_had_errors = had_errors; parsed_had_warnings = had_warnings;
		if(!str_f.empty()) {str_f += " "; parsed_expression.insert(0, str_f);}
		fix_history_string_new2(parsed_expression);
		gsub("&nbsp;", " ", parsed_expression);
		FIX_SUPSUB_PRE(statuslabel_l)
		FIX_SUPSUB(parsed_expression)
		if(show_parsed_instead_of_result || (parsed_in_result && !surface_result && !rpn_mode)) {
			tmp_surface = draw_structure(parse_l == 0 ? m_undefined : mparse, po, complex_angle_form, top_ips, NULL, 3, NULL, NULL, NULL, -1, true, &mwhere, &displayed_parsed_to);
			showing_first_time_message = false;
			if(surface_parsed) cairo_surface_destroy(surface_parsed);
			surface_parsed = tmp_surface;
			first_draw_of_result = true;
			parsed_printops = po;
			if(!displayed_parsed_mstruct) displayed_parsed_mstruct = new MathStructure();
			if(parse_l == 0) displayed_parsed_mstruct->setUndefined();
			else displayed_parsed_mstruct->set_nocopy(mparse);
			gtk_widget_queue_draw(resultview);
			minimal_mode_show_resultview();
			if((result_autocalculated || show_parsed_instead_of_result) && !result_text.empty()) {
				string equalsstr;
				if(result_text_approximate) {
					if(printops.use_unicode_signs && (!printops.can_display_unicode_string_function || (*printops.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, (void*) statuslabel_l))) {
						equalsstr = SIGN_ALMOST_EQUAL " ";
					} else {
						equalsstr = "= ";
						equalsstr += _("approx.");
					}
				} else {
					equalsstr = "= ";
				}
				set_status_text(equalsstr + result_text, false, parsed_had_errors, parsed_had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_AUTOCALC;
			} else if((!auto_calc_stopped_at_operator || !result_autocalculated) && message_n >= 1 && !b_func && !last_is_operator(str_e)) {
				size_t i = parsed_expression_tooltip.rfind("\n• ");
				if(i == string::npos) set_status_text(parsed_expression_tooltip, false, had_errors, had_warnings);
				else set_status_text(parsed_expression_tooltip.substr(i + strlen("\n• ")), false, had_errors, had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_ERROR;
			} else if(!b_func) {
				set_status_text("");
			}
		} else {
			if(!b_func) {
				set_status_text(parsed_expression.c_str(), true, had_errors, had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_PARSED;
			}
		}
		set_expression_output_updated(false);
	} else if(!b_func) {
		CALCULATOR->clearMessages();
		unblock_error();
		if(show_parsed_instead_of_result || (parsed_in_result && !surface_result && !rpn_mode)) {
			if(!result_text.empty() && (result_autocalculated || show_parsed_instead_of_result)) {
				string equalsstr;
				if(result_text_approximate) {
					if(printops.use_unicode_signs && (!printops.can_display_unicode_string_function || (*printops.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, (void*) statuslabel_l))) {
						equalsstr = SIGN_ALMOST_EQUAL " ";
					} else {
						equalsstr = "= ";
						equalsstr += _("approx.");
					}
				} else {
					equalsstr = "= ";
				}
				set_status_text(equalsstr + result_text, false, parsed_had_errors, parsed_had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_AUTOCALC;
			} else if((!auto_calc_stopped_at_operator || !result_autocalculated) && !parsed_expression_tooltip.empty()) {
				size_t i = parsed_expression_tooltip.rfind("\n• ");
				if(i == string::npos) set_status_text(parsed_expression_tooltip, false, parsed_had_errors, parsed_had_warnings);
				else set_status_text(parsed_expression_tooltip.substr(i + strlen("\n• ")), false, parsed_had_errors, parsed_had_warnings, parsed_expression_tooltip);
				status_text_source = STATUS_TEXT_ERROR;
			} else {
				set_status_text("");
			}
		} else {
			set_status_text(parsed_expression.c_str(), true, parsed_had_errors, parsed_had_warnings, parsed_expression_tooltip);
			status_text_source = STATUS_TEXT_PARSED;
		}
	}
	if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	evalops.parse_options.preserve_format = false;
}

void mainwindow_cursor_moved() {
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		autocalc_history_timeout_id = 0;
		if(autocalc_history_delay >= 0 && !parsed_in_result) autocalc_history_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, autocalc_history_delay, do_autocalc_history_timeout, NULL, NULL);
	}
	if(auto_calc_stopped_at_operator) do_auto_calc();
}

void generate_units_tree_struct() {
	size_t cat_i, cat_i_prev;
	bool b;
	string str, cat, cat_sub;
	Unit *u = NULL;
	unit_cats.items.clear();
	unit_cats.objects.clear();
	unit_cats.parent = NULL;
	alt_volcats.clear();
	volume_cat = "";
	u = CALCULATOR->getActiveUnit("L");
	if(u) volume_cat = u->category();
	ia_units.clear();
	user_units.clear();
	list<tree_struct>::iterator it;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(!CALCULATOR->units[i]->isActive()) {
			b = false;
			for(size_t i3 = 0; i3 < ia_units.size(); i3++) {
				u = (Unit*) ia_units[i3];
				if(string_is_less(CALCULATOR->units[i]->title(true, printops.use_unicode_signs), u->title(true, printops.use_unicode_signs))) {
					b = true;
					ia_units.insert(ia_units.begin() + i3, (void*) CALCULATOR->units[i]);
					break;
				}
			}
			if(!b) ia_units.push_back((void*) CALCULATOR->units[i]);
		} else {
			if(CALCULATOR->units[i]->isLocal() && !CALCULATOR->units[i]->isBuiltin()) {
				b = false;
				for(size_t i3 = 0; i3 < user_units.size(); i3++) {
					u = user_units[i3];
					if(string_is_less(CALCULATOR->units[i]->title(true, printops.use_unicode_signs), u->title(true, printops.use_unicode_signs))) {
						b = true;
						user_units.insert(user_units.begin() + i3, CALCULATOR->units[i]);
						break;
					}
				}
				if(!b) user_units.push_back(CALCULATOR->units[i]);
			}
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
						if(item->item == volume_cat && CALCULATOR->units[i]->baseUnit()->referenceName() == "m" && CALCULATOR->units[i]->baseExponent() == 3) {
							alt_volcats.push_back(CALCULATOR->units[i]->category());
						}
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
				if(string_is_less(CALCULATOR->units[i]->title(true, printops.use_unicode_signs), u->title(true, printops.use_unicode_signs))) {
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

void remove_old_my_variables_category() {
	if(VERSION_AFTER(4, 7, 0)) return;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isLocal() && (CALCULATOR->variables[i]->category() == "My Variables" || CALCULATOR->variables[i]->category() == _("My Variables"))) {
			CALCULATOR->variables[i]->setCategory("");
		}
	}
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
	user_variables.clear();
	list<tree_struct>::iterator it;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->isActive()) {
			//deactivated variable
			b = false;
			for(size_t i3 = 0; i3 < ia_variables.size(); i3++) {
				v = (Variable*) ia_variables[i3];
				if(string_is_less(CALCULATOR->variables[i]->title(true, printops.use_unicode_signs, &can_display_unicode_string_function), v->title(true, printops.use_unicode_signs))) {
					b = true;
					ia_variables.insert(ia_variables.begin() + i3, (void*) CALCULATOR->variables[i]);
					break;
				}
			}
			if(!b) ia_variables.push_back((void*) CALCULATOR->variables[i]);
		} else {
			if(CALCULATOR->variables[i]->isLocal() && !CALCULATOR->variables[i]->isBuiltin()) {
				b = false;
				for(size_t i3 = 0; i3 < user_variables.size(); i3++) {
					v = user_variables[i3];
					if(string_is_less(CALCULATOR->variables[i]->title(true, printops.use_unicode_signs), v->title(true, printops.use_unicode_signs))) {
						b = true;
						user_variables.insert(user_variables.begin() + i3, CALCULATOR->variables[i]);
						break;
					}
				}
				if(!b) user_variables.push_back(CALCULATOR->variables[i]);
			}
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
				if(string_is_less(CALCULATOR->variables[i]->title(true, printops.use_unicode_signs), v->title(true, printops.use_unicode_signs))) {
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
	user_functions.clear();
	list<tree_struct>::iterator it;

	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(!CALCULATOR->functions[i]->isActive()) {
			//deactivated function
			b = false;
			for(size_t i3 = 0; i3 < ia_functions.size(); i3++) {
				f = (MathFunction*) ia_functions[i3];
				if(string_is_less(CALCULATOR->functions[i]->title(true, printops.use_unicode_signs), f->title(true, printops.use_unicode_signs))) {
					b = true;
					ia_functions.insert(ia_functions.begin() + i3, (void*) CALCULATOR->functions[i]);
					break;
				}
			}
			if(!b) ia_functions.push_back((void*) CALCULATOR->functions[i]);
		} else {
			if(CALCULATOR->functions[i]->isLocal() && !CALCULATOR->functions[i]->isBuiltin()) {
				b = false;
				for(size_t i3 = 0; i3 < user_functions.size(); i3++) {
					f = user_functions[i3];
					if(string_is_less(CALCULATOR->functions[i]->title(true, printops.use_unicode_signs), f->title(true, printops.use_unicode_signs))) {
						b = true;
						user_functions.insert(user_functions.begin() + i3, CALCULATOR->functions[i]);
						break;
					}
				}
				if(!b) user_functions.push_back(CALCULATOR->functions[i]);
			}
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
				if(string_is_less(CALCULATOR->functions[i]->title(true, printops.use_unicode_signs), f->title(true, printops.use_unicode_signs))) {
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
			return f->title(true, printops.use_unicode_signs);
		}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
			MathFunction *f = CALCULATOR->getActiveFunction(value);
			return f->title(true, printops.use_unicode_signs);
		}
		case SHORTCUT_TYPE_VARIABLE: {
			Variable *v = CALCULATOR->getActiveVariable(value);
			return v->title(true, printops.use_unicode_signs);
		}
		case SHORTCUT_TYPE_UNIT: {
			Unit *u = CALCULATOR->getActiveUnit(value);
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

void update_tooltips_enabled() {
	set_tooltips_enabled(mainwindow, enable_tooltips);
	set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), enable_tooltips == 1);
}

void update_result_accels(int type) {
	bool b = false;
	for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
		if(it->second.type.size() != 1 || (type >= 0 && it->second.type[0] != type)) continue;
		b = true;
		switch(it->second.type[0]) {
			case SHORTCUT_TYPE_COPY_RESULT: {
				int v = s2i(it->second.value[0]);
				if(v > 0 && v <= 7) break;
				if(!copy_ascii) {
					gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_copy")))), it->second.key, (GdkModifierType) it->second.modifier);
					if(type >= 0) {
						gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_copy_ascii")))), 0, (GdkModifierType) 0);
					}
				} else {
					gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_copy_ascii")))), it->second.key, (GdkModifierType) it->second.modifier);
					if(type >= 0) {
						gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_copy")))), 0, (GdkModifierType) 0);
					}
				}
				break;
			}
		}
		if(type >= 0) break;
	}
	if(!b) {
		switch(type) {
			case SHORTCUT_TYPE_COPY_RESULT: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_copy")))), 0, (GdkModifierType) 0);
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "popup_menu_item_copy_ascii")))), 0, (GdkModifierType) 0);
				break;
			}
		}
	}
}
void update_accels(int type) {
	update_result_accels(type);
	update_history_accels(type);
	update_stack_accels(type);
	update_keypad_accels(type);
	update_menu_accels(type);
}

/*
	recreate unit menus and update unit manager (when units have changed)
*/
void update_umenus(bool update_compl) {
	generate_units_tree_struct();
	create_umenu();
	recreate_recent_units();
	create_umenu2();
	add_custom_angles_to_menus();
	update_units_tree();
	update_unit_selector_tree();
	if(update_compl) update_completion();
}

/*
	recreate variables menu and update variable manager (when variables have changed)
*/
void update_vmenu(bool update_compl) {
	if(variable_cats.items.empty() && variable_cats.objects.empty()) return;
	generate_variables_tree_struct();
	create_vmenu();
	recreate_recent_variables();
	update_variables_tree();
	if(update_compl) update_completion();
	update_mb_sto_menu();
}

/*
	recreate functions menu and update function manager (when functions have changed)
*/
void update_fmenu(bool update_compl) {
	if(function_cats.items.empty() && function_cats.objects.empty()) return;
	generate_functions_tree_struct();
	create_fmenu();
	recreate_recent_functions();
	if(update_compl) update_completion();
	update_functions_tree();
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
		if(po.number_fraction_format == FRACTION_DECIMAL) po.number_fraction_format = FRACTION_DECIMAL_EXACT;
	}
	string str = CALCULATOR->print(mstruct_, 100, po);
	return str;
}

void draw_background(cairo_t *cr, gint w, gint h) {
/*	GdkRGBA rgba;
	gtk_style_context_get_background_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview);, &rgba);
	gdk_cairo_set_source_rgba(cr, &rgba);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);*/
}

#define PAR_SPACE 1
#define PAR_WIDTH (scaledown + ips.power_depth > 1 ? par_width / 1.7 : (scaledown + ips.power_depth > 0 ? par_width / 1.4 : par_width)) + (PAR_SPACE * 2)

cairo_surface_t *get_left_parenthesis(gint arc_w, gint arc_h, int, GdkRGBA *color) {
	gint scalefactor = RESULT_SCALE_FACTOR;
	cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, arc_w * scalefactor, arc_h * scalefactor);
	cairo_surface_set_device_scale(s, scalefactor, scalefactor);
	cairo_t *cr = cairo_create(s);
	gdk_cairo_set_source_rgba(cr, color);
	cairo_save(cr);
	double hscale = 2;
	double radius = arc_w - PAR_SPACE * 2;
	if(radius * 2 * hscale > arc_h - 4) hscale = (arc_h - 4) / (radius * 2.0);
	cairo_scale(cr, 1, hscale);
	cairo_arc(cr, radius + PAR_SPACE, (arc_h - 2) / hscale - radius, radius, 1.8708, 3.14159);
	cairo_arc(cr, radius + PAR_SPACE, radius + 2, radius, 3.14159, 4.41239);
	cairo_restore(cr);
	cairo_set_line_width(cr, arc_w > 7 ? 2 : 1);
	cairo_stroke(cr);
	cairo_destroy(cr);
	return s;
}
cairo_surface_t *get_right_parenthesis(gint arc_w, gint arc_h, int, GdkRGBA *color) {
	gint scalefactor = RESULT_SCALE_FACTOR;
	cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, arc_w * scalefactor, arc_h * scalefactor);
	cairo_surface_set_device_scale(s, scalefactor, scalefactor);
	cairo_t *cr = cairo_create(s);
	gdk_cairo_set_source_rgba(cr, color);
	cairo_save(cr);
	double hscale = 2;
	double radius = arc_w - PAR_SPACE * 2;
	if(radius * 2 * hscale > arc_h - 4) hscale = (arc_h - 4) / (radius * 2.0);
	cairo_scale(cr, 1, hscale);
	cairo_arc(cr, PAR_SPACE, radius + 2, radius, -1.2708, 0);
	cairo_arc(cr, PAR_SPACE, (arc_h - 2) / hscale - radius, radius, 0, 1.2708);
	cairo_restore(cr);
	cairo_set_line_width(cr, arc_w > 7 ? 2 : 1);
	cairo_stroke(cr);
	cairo_destroy(cr);
	return s;
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

#define SHOW_WITH_ROOT_SIGN(x) (x.isFunction() && ((x.function() == CALCULATOR->f_sqrt && x.size() == 1) || (x.function() == CALCULATOR->f_cbrt && x.size() == 1) || (x.function() == CALCULATOR->f_root && x.size() == 2 && x[1].isNumber() && x[1].number().isInteger() && x[1].number().isPositive() && x[1].number().isLessThan(10))))

cairo_surface_t *draw_structure(MathStructure &m, PrintOptions po, bool caf, InternalPrintStruct ips, gint *point_central, int scaledown, GdkRGBA *color, gint *x_offset, gint *w_offset, gint max_width, bool for_result_widget, MathStructure *where_struct, vector<MathStructure> *to_structs) {

	if(for_result_widget && ips.depth == 0) {
		binary_rect.clear();
		binary_pos.clear();
	}

	if(CALCULATOR->aborted()) return NULL;

	if(BASE_IS_SEXAGESIMAL(po.base) && m.isMultiplication() && m.size() == 2 && m[0].isNumber() && m[1].isUnit() && m[1].unit() == CALCULATOR->getDegUnit() && !po.preserve_format) {
		return draw_structure(m[0], po, caf, ips, point_central, scaledown, color, x_offset, w_offset, max_width, for_result_widget);
	}

	gint scalefactor = RESULT_SCALE_FACTOR;

	if(ips.depth == 0 && po.is_approximate) *po.is_approximate = false;

	cairo_surface_t *surface = NULL;

	GdkRGBA rgba;
	if(!color) {
		gtk_style_context_get_color(gtk_widget_get_style_context(resultview), GTK_STATE_FLAG_NORMAL, &rgba);
		color = &rgba;
	}
	gint w, h;
	gint central_point = 0;
	gint offset_x = 0;
	gint offset_w = 0;

	InternalPrintStruct ips_n = ips;
	if(m.isApproximate()) ips_n.parent_approximate = true;
	if(m.precision() > 0 && (ips_n.parent_precision < 1 || m.precision() < ips_n.parent_precision)) ips_n.parent_precision = m.precision();

	if(where_struct && where_struct->isZero()) where_struct = NULL;
	if(to_structs && to_structs->empty()) to_structs = NULL;
	if(to_structs || where_struct) {
		ips_n.depth++;

		vector<cairo_surface_t*> surface_terms;
		vector<PangoLayout*> surface_termsl;
		vector<gint> hpt, wpt, cpt, xpt;
		gint where_w = 0, where_h = 0, to_w = 0, to_h = 0, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0, xtmp = 0, wotmp = 0;
		CALCULATE_SPACE_W

		PangoLayout *layout_to = NULL;
		if(to_structs) {
			layout_to = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			TTB(str);
			str += CALCULATOR->localToString(false);
			TTE(str);
			pango_layout_set_markup(layout_to, str.c_str(), -1);
			pango_layout_get_pixel_size(layout_to, &to_w, &to_h);
		}
		PangoLayout *layout_where = NULL;
		if(where_struct) {
			layout_where = gtk_widget_create_pango_layout(resultview, NULL);
			string str;
			TTB(str);
			str += CALCULATOR->localWhereString();
			TTE(str);
			pango_layout_set_markup(layout_where, str.c_str(), -1);
			pango_layout_get_pixel_size(layout_where, &where_w, &where_h);
		}

		for(size_t i = 0; ; i++) {
			if(i == 0 && m.isUndefined() && !where_struct) continue;
			if(i == 1 && !where_struct) continue;
			if(i > 1 && (!to_structs || i - 2 >= to_structs->size())) break;
			hetmp = 0;
			surface_terms.push_back(draw_structure(i == 0 ? m : (i == 1 ? *where_struct : (*to_structs)[i - 2]), po, caf, ips_n, &hetmp, scaledown, color, &xtmp, &wotmp));
			if(CALCULATOR->aborted()) {
				for(size_t i = 0; i < surface_terms.size(); i++) {
					if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
				}
				return NULL;
			}
			if(surface_terms.size() == 1) {
				offset_x = xtmp;
				xtmp = 0;
			} else if((i == 1 && !to_structs) || (i > 1 && i - 2 == to_structs->size() - 1)) {
				offset_w = wotmp;
				wotmp = 0;
			}
			wtmp = cairo_image_surface_get_width(surface_terms[surface_terms.size() - 1]) / scalefactor;
			htmp = cairo_image_surface_get_height(surface_terms[surface_terms.size() - 1]) / scalefactor;
			hpt.push_back(htmp);
			cpt.push_back(hetmp);
			wpt.push_back(wtmp);
			xpt.push_back(xtmp);
			w -= xtmp;
			w += wtmp;
			if(i == 0) {
				w += space_w;
			} else if(i == 1) {
				w += where_w + space_w;
				if(to_structs) w += space_w;
			} else {
				w += space_w + to_w;
				if(i - 2 < to_structs->size() - 1) w += space_w;
			}
			if(htmp - hetmp > uh) {
				uh = htmp - hetmp;
			}
			if(hetmp > dh) {
				dh = hetmp;
			}
		}


		if(to_h / 2 > dh) dh = to_h / 2;
		if(to_h / 2 + to_h % 2 > uh) uh = to_h / 2 + to_h % 2;
		if(where_h / 2 > dh) dh = where_h / 2;
		if(where_h / 2 + where_h % 2 > uh) uh = where_h / 2 + where_h % 2;

		central_point = dh;
		h = dh + uh;
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cairo_t *cr = cairo_create(surface);
		w = 0;
		for(size_t i = 0; i < surface_terms.size(); i++) {
			if(!CALCULATOR->aborted()) {
				gdk_cairo_set_source_rgba(cr, color);
				if(i > 0) w += space_w;
				if(i == 1 && where_struct) {
					cairo_move_to(cr, w, uh - where_h / 2 - where_h % 2);
					pango_cairo_show_layout(cr, layout_where);
					w += where_w;
					w += space_w;
				} else if(i > 0 || (!where_struct && m.isUndefined())) {
					cairo_move_to(cr, w, uh - to_h / 2 - to_h % 2);
					pango_cairo_show_layout(cr, layout_to);
					w += to_w;
					w += space_w;
				}
				w -= xpt[i];
				cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
				cairo_paint(cr);
				w += wpt[i];
			}
			cairo_surface_destroy(surface_terms[i]);
		}
		if(layout_to) g_object_unref(layout_to);
		if(layout_where) g_object_unref(layout_where);
		cairo_destroy(cr);
	} else if(caf && m.isMultiplication() && m.size() == 2 && m[1].isFunction() && m[1].size() == 1 && m[1].function()->referenceName() == "cis") {
		// angle/phasor notation: x+y*i=a*cis(b)=a∠b
		ips_n.depth++;

		vector<cairo_surface_t*> surface_terms;

		vector<gint> hpt;
		vector<gint> wpt;
		vector<gint> cpt;
		gint sign_w, sign_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0, space_w = 0;

		PangoLayout *layout_sign = NULL;

		if(can_display_unicode_string_function_exact("∠", (void*) resultview)) {
			layout_sign = gtk_widget_create_pango_layout(resultview, NULL);
			PANGO_TTP(layout_sign, "∠");
			pango_layout_get_pixel_size(layout_sign, &sign_w, &sign_h);
			w = sign_w;
			uh = sign_h / 2 + sign_h % 2;
			dh = sign_h / 2;
		}
		for(size_t i = 0; i < 2; i++) {
			hetmp = 0;
			ips_n.wrap = false;
			surface_terms.push_back(draw_structure(i == 0 ? m[0] : m[1][0], po, caf, ips_n, &hetmp, scaledown, color));
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

		central_point = dh;
		h = dh + uh;

		if(!layout_sign) {
			space_w = 5;
			sign_h = (h * 6) / 10;
			sign_w = sign_h;
			w += sign_w;
		}

		w += space_w * 2;

		double divider = 1.0;
		if(ips.power_depth >= 1) divider = 1.5;
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cairo_t *cr = cairo_create(surface);
		w = 0;
		for(size_t i = 0; i < surface_terms.size(); i++) {
			if(!CALCULATOR->aborted()) {
				gdk_cairo_set_source_rgba(cr, color);
				if(i > 0) {
					w += space_w;
					if(layout_sign) {
						cairo_move_to(cr, w, uh - sign_h / 2 - sign_h % 2);
						pango_cairo_show_layout(cr, layout_sign);
					} else {
						cairo_move_to(cr, w, h - 2 / divider - (h - sign_h) / 2);
						cairo_line_to(cr, w + (sign_w * 3) / 4, (h - sign_h) / 2);
						cairo_move_to(cr, w, h - 2 / divider - (h - sign_h) / 2);
						cairo_line_to(cr, w + sign_w, h - 2 / divider - (h - sign_h) / 2);
						cairo_set_line_width(cr, 2 / divider);
						cairo_stroke(cr);
					}
					w += sign_w;
					w += space_w;
				}
				cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
				cairo_paint(cr);
				w += wpt[i];
			}
			cairo_surface_destroy(surface_terms[i]);
		}
		if(layout_sign) g_object_unref(layout_sign);
		cairo_destroy(cr);
	} else {
		switch(m.type()) {
			case STRUCT_NUMBER: {
				unordered_map<void*, string>::iterator it = number_map.find((void*) &m.number());
				string str;
				string exp = "";
				bool exp_minus = false;
				bool base10 = (po.base == BASE_DECIMAL);
				bool base_without_exp = (po.base != BASE_DECIMAL && !BASE_IS_SEXAGESIMAL(po.base) && po.base != BASE_TIME && ((po.base < BASE_CUSTOM && po.base != BASE_BIJECTIVE_26) || (po.base == BASE_CUSTOM && (!CALCULATOR->customOutputBase().isInteger() || CALCULATOR->customOutputBase() > 62 || CALCULATOR->customOutputBase() < 2))));
				string value_str;
				int base = po.base;
				if(base <= BASE_FP16 && base >= BASE_FP80) base = BASE_BINARY;
				if(it != number_map.end()) {
					value_str += it->second;
					if(number_approx_map.find((void*) &m.number()) != number_approx_map.end()) {
						if(po.is_approximate && !(*po.is_approximate) && number_approx_map[(void*) &m.number()]) *po.is_approximate = true;
					}
					if(number_exp_map.find((void*) &m.number()) != number_exp_map.end()) {
						exp = number_exp_map[(void*) &m.number()];
						exp_minus = number_exp_minus_map[(void*) &m.number()];
					}
					if(printops.exp_display != EXP_POWER_OF_10 && base_without_exp) {
						if(!exp.empty()) base10 = true;
						exp = "";
						exp_minus = false;
					}
				} else {
					if(po.number_fraction_format == FRACTION_FRACTIONAL_FIXED_DENOMINATOR || po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR) {
						po.show_ending_zeroes = false;
						po.number_fraction_format = FRACTION_FRACTIONAL;
					} else if(po.number_fraction_format == FRACTION_PERCENT || po.number_fraction_format == FRACTION_PERMILLE || po.number_fraction_format == FRACTION_PERMYRIAD) {
						po.number_fraction_format = FRACTION_DECIMAL;
					}
					bool was_approx = (po.is_approximate && *po.is_approximate);
					if(po.is_approximate) *po.is_approximate = false;
					if(printops.exp_display == EXP_POWER_OF_10 || base_without_exp || (po.base != BASE_DECIMAL && !BASE_IS_SEXAGESIMAL(po.base) && po.base != BASE_TIME)) {
						ips_n.exp = &exp;
						ips_n.exp_minus = &exp_minus;
						if(printops.exp_display != EXP_POWER_OF_10 && base_without_exp) {
							m.number().print(po, ips_n);
							base10 = !exp.empty();
							exp = "";
							exp_minus = false;
							ips_n.exp = NULL;
							ips_n.exp_minus = NULL;
						}
					} else {
						ips_n.exp = NULL;
						ips_n.exp_minus = NULL;
					}
					value_str = m.number().print(po, ips_n);
					gsub(NNBSP, THIN_SPACE, value_str);
					if(po.base == BASE_HEXADECIMAL && po.base_display == BASE_DISPLAY_NORMAL) {
						gsub("0x", "", value_str);
						size_t l = value_str.find(po.decimalpoint());
						if(l == string::npos) l = value_str.length();
						size_t i_after_minus = 0;
						if(m.number().isNegative()) {
							if(l > 1 && value_str[0] == '-') i_after_minus = 1;
							else if(value_str.find("−") == 0) i_after_minus = strlen("−");
						}
						for(int i = (int) l - 2; i > (int) i_after_minus; i -= 2) {
							value_str.insert(i, 1, ' ');
						}
						if(po.binary_bits == 0 && value_str.length() > i_after_minus + 1 && value_str[i_after_minus] == ' ') value_str.insert(i_after_minus + 1, 1, '0');
					} else if(po.base == BASE_OCTAL && po.base_display == BASE_DISPLAY_NORMAL) {
						if(value_str.length() > 1 && value_str[0] == '0' && is_in(NUMBERS, value_str[1])) value_str.erase(0, 1);
					}
					number_map[(void*) &m.number()] = value_str;
					number_exp_map[(void*) &m.number()] = exp;
					number_exp_minus_map[(void*) &m.number()] = exp_minus;
					if(!exp.empty() && (base_without_exp || (po.base != BASE_CUSTOM && (po.base < 2 || po.base > 36)) || (po.base == BASE_CUSTOM && (!CALCULATOR->customOutputBase().isInteger() || CALCULATOR->customOutputBase() > 62 || CALCULATOR->customOutputBase() < 2)))) base10 = true;
					if(!base10 && (BASE_IS_SEXAGESIMAL(po.base) || po.base == BASE_TIME) && value_str.find(po.decimalpoint()) && (value_str.find("+/-") != string::npos || value_str.find(SIGN_PLUSMINUS) != string::npos) && ((po.base == BASE_TIME && value_str.find(":") == string::npos) || (po.base != BASE_TIME && value_str.find("″") == string::npos && value_str.find("′") == string::npos && value_str.find(SIGN_DEGREE) == string::npos && value_str.find_first_of("\'\"o") == string::npos))) base10 = true;
					if(po.base != BASE_DECIMAL && (base10 || (!BASE_IS_SEXAGESIMAL(po.base) && po.base != BASE_TIME))) {
						bool twos = (((po.base == BASE_BINARY && po.twos_complement) || (po.base == BASE_HEXADECIMAL && po.hexadecimal_twos_complement)) && m.number().isNegative() && value_str.find(SIGN_MINUS) == string::npos && value_str.find("-") == string::npos);
						if(base10) {
							number_base_map[(void*) &m.number()]  = "10";
						} else if((twos || po.base_display != BASE_DISPLAY_ALTERNATIVE || (base != BASE_HEXADECIMAL && base != BASE_BINARY && base != BASE_OCTAL)) && (base > 0 || base <= BASE_CUSTOM) && base <= 36) {
							switch(base) {
								case BASE_GOLDEN_RATIO: {number_base_map[(void*) &m.number()]  = "<i>φ</i>"; break;}
								case BASE_SUPER_GOLDEN_RATIO: {number_base_map[(void*) &m.number()]  = "<i>ψ</i>"; break;}
								case BASE_PI: {number_base_map[(void*) &m.number()]  = "<i>π</i>"; break;}
								case BASE_E: {number_base_map[(void*) &m.number()]  = "<i>e</i>"; break;}
								case BASE_SQRT2: {number_base_map[(void*) &m.number()]  = "√2"; break;}
								case BASE_UNICODE: {number_base_map[(void*) &m.number()]  = "Unicode"; break;}
								case BASE_BIJECTIVE_26: {number_base_map[(void*) &m.number()]  = "b26"; break;}
								case BASE_BINARY_DECIMAL: {number_base_map[(void*) &m.number()]  = "BCD"; break;}
								case BASE_CUSTOM: {number_base_map[(void*) &m.number()]  = CALCULATOR->customOutputBase().print(CALCULATOR->messagePrintOptions()); break;}
								default: {number_base_map[(void*) &m.number()]  = i2s(base);}
							}
							if(twos) number_base_map[(void*) &m.number()] += '-';
						}
					} else {
						number_base_map[(void*) &m.number()] = "";
					}
					if(po.is_approximate) {
						number_approx_map[(void*) &m.number()] = po.is_approximate && *po.is_approximate;
					} else {
						number_approx_map[(void*) &m.number()] = FALSE;
					}
					if(po.is_approximate && was_approx) *po.is_approximate = true;
				}
				if(!exp.empty()) {
					if(value_str == "1") {
						MathStructure mnr((po.base == BASE_DECIMAL || po.base < 2 || po.base > 36) ? 10 : po.base, 1, 0);
						mnr.raise(m_one);
						if(po.base == BASE_DECIMAL || po.base < 2 || po.base > 36) number_map[(void*) &mnr[0].number()] = "10";
						number_base_map[(void*) &mnr[0].number()] = number_base_map[(void*) &m.number()];
						if(exp_minus) {
							mnr[1].transform(STRUCT_NEGATE);
							number_map[(void*) &mnr[1][0].number()] = exp;
							number_base_map[(void*) &mnr[1][0].number()] = "";
						} else {
							number_map[(void*) &mnr[1].number()] = exp;
							number_base_map[(void*) &mnr[1].number()] = "";
						}
						surface = draw_structure(mnr, po, caf, ips, point_central, scaledown, color, x_offset, w_offset, max_width);
						if(exp_minus) {number_map.erase(&mnr[1][0].number()); number_base_map.erase(&mnr[1][0].number());}
						else {number_map.erase(&mnr[1].number()); number_base_map.erase(&mnr[1].number());}
						number_base_map.erase(&mnr[0].number());
						number_map.erase(&mnr[0].number());
						return surface;
					} else {
						MathStructure mnr(m_one);
						mnr.multiply(Number((po.base == BASE_DECIMAL || po.base < 2 || po.base > 36) ? 10 : po.base, 1, 0));
						number_map[(void*) &mnr[0].number()] = value_str;
						if(base_without_exp) number_base_map[(void*) &mnr[0].number()] = "10";
						number_base_map[(void*) &mnr[0].number()] = number_base_map[(void*) &m.number()];
						number_approx_map[(void*) &mnr[0].number()] = number_approx_map[(void*) &m.number()];
						mnr[1].raise(m_one);
						if(po.base == BASE_DECIMAL || po.base < 2 || po.base > 36) number_map[(void*) &mnr[1][0].number()] = "10";
						number_base_map[(void*) &mnr[1][0].number()] = number_base_map[(void*) &m.number()];
						if(exp_minus) {
							mnr[1][1].transform(STRUCT_NEGATE);
							number_map[(void*) &mnr[1][1][0].number()] = exp;
							number_base_map[(void*) &mnr[1][1][0].number()] = "";
						} else {
							number_map[(void*) &mnr[1][1].number()] = exp;
							number_base_map[(void*) &mnr[1][1].number()] = "";
						}
						surface = draw_structure(mnr, po, caf, ips, point_central, scaledown, color, x_offset, w_offset, max_width);
						if(exp_minus) {number_map.erase(&mnr[1][1][0].number()); number_base_map.erase(&mnr[1][1][0].number());}
						else {number_map.erase(&mnr[1][1].number()); number_base_map.erase(&mnr[1][1].number());}
						number_map.erase(&mnr[1][0].number());
						number_map.erase(&mnr[0].number());
						number_base_map.erase(&mnr[1][0].number());
						number_base_map.erase(&mnr[0].number());
						number_approx_map.erase(&mnr[0].number());
						return surface;
					}
				}
				if(exp.empty() && printops.exp_display != EXP_LOWERCASE_E && ((printops.exp_display != EXP_POWER_OF_10 && po.base == BASE_DECIMAL) || BASE_IS_SEXAGESIMAL(po.base) || po.base == BASE_TIME)) {
					size_t i = 0;
					while(true) {
						i = value_str.find("E", i + 1);
						if(i == string::npos || i == value_str.length() - 1) break;
						if(value_str[i - 1] >= '0' && value_str[i - 1] <= '9' && value_str[i + 1] >= '0' && value_str[i + 1] <= '9') {
							string estr;
							TTP_SMALL(estr, "E");
							value_str.replace(i, 1, estr);
							i += estr.length();
						}
					}
					if(printops.exp_display == EXP_POWER_OF_10 && ips.power_depth == 0) {
						i = value_str.find("10^");
						if(i != string::npos) {
							i += 2;
							size_t i2 = value_str.find(")", i);
							if(i2 != string::npos) {
								value_str.insert(i2, "</sup>");
								value_str.replace(i, 1, "<sup>");
								FIX_SUP_RESULT(value_str);
							}
						}
					}
				}
				string value_str_bak, str_bak;
				vector<gint> pos_x;
				vector<PangoLayout*> pos_nr;
				gint pos_h = 0, pos_y = 0;
				gint wle = 0;
				if(max_width > 0) {
					PangoLayout *layout_equals = gtk_widget_create_pango_layout(resultview, NULL);
					if((po.is_approximate && *po.is_approximate) || m.isApproximate()) {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) {
							PANGO_TT(layout_equals, SIGN_ALMOST_EQUAL);
						} else {
							string str;
							TTB(str);
							str += "= ";
							str += _("approx.");
							TTE(str);
							pango_layout_set_markup(layout_equals, str.c_str(), -1);
						}
					} else {
						PANGO_TT(layout_equals, "=");
					}
					CALCULATE_SPACE_W
					PangoRectangle rect, lrect;
					pango_layout_get_pixel_extents(layout_equals, &rect, &lrect);
					wle = lrect.width - offset_x + space_w;
					if(rect.x < 0) wle -= rect.x;
					g_object_unref(layout_equals);
				}
				PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
				bool multiline = false;
				for(int try_i = 0; try_i <= 1; try_i++) {
					if(try_i == 1) {
						value_str_bak = value_str;
						size_t i = string::npos, l = 0;
						if(base == BASE_BINARY || (base == BASE_DECIMAL && po.digit_grouping != DIGIT_GROUPING_NONE)) {
							i = value_str.find(" ", value_str.length() / 2);
							l = 1;
							if(i == string::npos && base == BASE_DECIMAL) {
								if(po.digit_grouping != DIGIT_GROUPING_LOCALE) {
									l = strlen(THIN_SPACE);
									i = value_str.find(THIN_SPACE, value_str.length() / 2 - 1);
								} else if(!CALCULATOR->local_digit_group_separator.empty()) {
									l = CALCULATOR->local_digit_group_separator.length();
									i = value_str.find(CALCULATOR->local_digit_group_separator, value_str.length() / 2 - (l == 3 ? 1 : 0));
								}
							}
						}
						if(i == string::npos && base != BASE_BINARY) {
							l = 0;
							i = value_str.length() / 2 + 2;
							if(base == BASE_DECIMAL && (po.digit_grouping == DIGIT_GROUPING_STANDARD || (po.digit_grouping == DIGIT_GROUPING_LOCALE && CALCULATOR->local_digit_group_separator != " "))) {
								size_t i2 = 0;
								while(true) {
									i2 = value_str.find(po.digit_grouping == DIGIT_GROUPING_LOCALE ? CALCULATOR->local_digit_group_separator : THIN_SPACE, i2 + 1);
									if(i2 == string::npos || i2 == value_str.length() - 1) break;
									i++;
								}
								if(i >= value_str.length()) i = string::npos;
							}
							while((signed char) value_str[i] < 0) {
								i++;
								if(i >= value_str.length()) {i = string::npos; break;}
							}
						}
						if(i == string::npos) {
							break;
						} else {
							if(l == 0) value_str.insert(i, 1, '\n');
							else if(l == 1) value_str[i] = '\n';
							else {value_str.erase(i, l - 1); value_str[i] = '\n';}
							if(base == BASE_DECIMAL) pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
							multiline = true;
						}
					}
					TTBP(str)
					str += value_str;
					if(!number_base_map[(void*) &m.number()].empty()) {
						if(!multiline) {
							string str2 = str;
							TTE(str2)
							pango_layout_set_markup(layout, str2.c_str(), -1);
							pango_layout_get_pixel_size(layout, NULL, &central_point);
						}
						TTBP_SMALL(str)
						str += "<sub>";
						str += number_base_map[(void*) &m.number()];
						str += "</sub>";
						FIX_SUB_RESULT(str)
						TTE(str)
					}
					TTE(str)
					pango_layout_set_markup(layout, str.c_str(), -1);
					if(max_width > 0 && exp.empty() && ((base >= 2 && base <= 36 && base != BASE_DUODECIMAL) || (base == BASE_CUSTOM && CALCULATOR->customOutputBase().isInteger() && CALCULATOR->customOutputBase() <= 62 && CALCULATOR->customOutputBase() >= -62))) {
						pango_layout_get_pixel_size(layout, &w, NULL);
						if(w + wle > max_width) {
							if(try_i == 1) {
								str = str_bak;
								pango_layout_set_markup(layout, str.c_str(), -1);
								multiline = false;
							} else {
								str_bak = str;
								str = "";
							}
						} else {
							break;
						}
					} else {
						break;
					}
				}

				if(ips.depth == 0 && base == BASE_BINARY && value_str.find(po.decimalpoint()) == string::npos && value_str.find_first_not_of("10 \n") == string::npos) {
					PangoLayoutIter *iter = pango_layout_get_iter(layout);
					PangoRectangle crect;
					string str2;
					size_t n_begin = (value_str.length() + 1) % 10;
					for(size_t i = 0; i == 0 || pango_layout_iter_next_char(iter); i++) {
						int bin_pos = ((value_str.length() - n_begin) - (value_str.length() - n_begin) / 5) - ((i - n_begin) - (i - n_begin) / 5) - 1;
						if(bin_pos < 0) break;
						pango_layout_iter_get_char_extents(iter, &crect);
						pango_extents_to_pixels(&crect, NULL);
						if(i % 10 == n_begin && value_str.length() > 20 && bin_pos > 0) {
							PangoLayout *layout_pos = gtk_widget_create_pango_layout(resultview, NULL);
							str2 = "";
							TTB_XXSMALL(str2);
							str2 += i2s(bin_pos + 1);
							TTE(str2);
							pango_layout_set_markup(layout_pos, str2.c_str(), -1);
							pos_nr.push_back(layout_pos);
							if(bin_pos < 10) {
								pango_layout_get_pixel_size(layout_pos, &w, &pos_h);
								pos_x.push_back(crect.x + (crect.width - w) / 2);
							} else {
								pos_x.push_back(crect.x);
							}
						}
						if(for_result_widget && value_str[i] != ' ') {
							binary_rect.push_back(crect);
							binary_pos.push_back(bin_pos);
						}
					}
					pango_layout_iter_free(iter);
				}
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout, &rect, &lrect);
				w = lrect.width;
				h = lrect.height;
				if(rect.x < 0) {
					w -= rect.x;
					if(rect.width > w) {
						offset_w = rect.width - w;
						w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > w) {
						offset_w = rect.width + rect.x - w;
						w = rect.width + rect.x;
					}
				}
				if(multiline) {
					pango_layout_line_get_pixel_extents(pango_layout_get_line(layout, 0), NULL, &lrect);
					central_point = h - (lrect.height / 2 + lrect.height % 2);
					pos_y = h;
					if(!binary_rect.empty()) {
						central_point += pos_h;
						h += pos_h * 2;
						pos_y += pos_h;
						for(size_t i = 0; i < binary_rect.size(); i++) binary_rect[i].y += pos_h;
					}
				} else if(central_point != 0) {
					pos_y = central_point;
					if(pos_h + pos_y > h) h = pos_h + pos_y;
					central_point = h - (central_point / 2 + central_point % 2);
				} else {
					central_point = h / 2;
					pos_y = h;
					h += pos_h;
				}
				if(rect.y < 0) {
					h -= rect.y;
					pos_y -= rect.y;
				}
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, offset_x, (multiline && !binary_rect.empty() ? pos_h : 0) + (rect.y < 0 ? -rect.y : 0));
				pango_cairo_show_layout(cr, layout);
				if(!pos_nr.empty()) {
					GdkRGBA *color2 = gdk_rgba_copy(color);
					color2->alpha = color2->alpha * 0.7;
					gdk_cairo_set_source_rgba(cr, color2);
					for(size_t i = 0; i < pos_nr.size(); i++) {
						cairo_move_to(cr, pos_x[i], (multiline && i < (pos_nr.size() + 1) / 2) ? 0 : pos_y);
						pango_cairo_show_layout(cr, pos_nr[i]);
						g_object_unref(pos_nr[i]);
					}
					gdk_rgba_free(color2);
				}
				g_object_unref(layout);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_ABORTED: {}
			case STRUCT_SYMBOLIC: {
				PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				if(m.symbol().length() >= 14 && m.symbol().find("to expression:") == 0) {
					TTBP(str)
					str += m.symbol().substr(14);
					TTE(str)
				} else {
					str = "<i>";
					TTBP(str)
					str += m.symbol();
					TTE(str)
					str += "</i>";
				}
				pango_layout_set_markup(layout, str.c_str(), -1);
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout, &rect, &lrect);
				w = lrect.width;
				h = lrect.height;
				if(rect.x < 0) {
					w -= rect.x;
					if(rect.width > w) {
						offset_w = rect.width - w;
						w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > w) {
						offset_w = rect.width + rect.x - w;
						w = rect.width + rect.x;
					}
				}
				central_point = h / 2;
				if(rect.y < 0) h -= rect.y;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, offset_x, rect.y < 0 ? -rect.y : 0);
				pango_cairo_show_layout(cr, layout);
				g_object_unref(layout);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_DATETIME: {
				PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				TTBP(str)
				unordered_map<void*, string>::iterator it = date_map.find((void*) m.datetime());
				if(it != date_map.end()) {
					str += it->second;
					if(date_approx_map.find((void*) m.datetime()) != date_approx_map.end()) {
						if(po.is_approximate && !(*po.is_approximate) && date_approx_map[(void*) m.datetime()]) *po.is_approximate = true;
					}
				} else {
					bool was_approx = (po.is_approximate && *po.is_approximate);
					if(po.is_approximate) *po.is_approximate = false;
					string value_str = m.datetime()->print(po);
					date_map[(void*) m.datetime()] = value_str;
					date_approx_map[(void*) m.datetime()] = po.is_approximate && *po.is_approximate;
					if(po.is_approximate && was_approx) *po.is_approximate = true;
					str += value_str;
				}
				TTE(str)
				pango_layout_set_markup(layout, str.c_str(), -1);
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout, &rect, &lrect);
				w = lrect.width;
				h = lrect.height;
				if(rect.x < 0) {
					w -= rect.x;
					if(rect.width > w) {
						offset_w = rect.width - w;
						w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > w) {
						offset_w = rect.width + rect.x - w;
						w = rect.width + rect.x;
					}
				}
				central_point = h / 2;
				if(rect.y < 0) h -= rect.y;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, offset_x, rect.y < 0 ? -rect.y : 0);
				pango_cairo_show_layout(cr, layout);
				g_object_unref(layout);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_ADDITION: {
				ips_n.depth++;

				vector<cairo_surface_t*> surface_terms;
				vector<gint> hpt, wpt, cpt, xpt;
				gint plus_w, plus_h, minus_w, minus_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0, xtmp = 0, wotmp = 0;

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
						surface_terms.push_back(draw_structure(m[i][0], po, caf, ips_n, &hetmp, scaledown, color, &xtmp, &wotmp));
					} else {
						ips_n.wrap = m[i].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
						surface_terms.push_back(draw_structure(m[i], po, caf, ips_n, &hetmp, scaledown, color, &xtmp, &wotmp));
					}
					if(CALCULATOR->aborted()) {
						for(size_t i = 0; i < surface_terms.size(); i++) {
							if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
						}
						g_object_unref(layout_minus);
						g_object_unref(layout_plus);
						return NULL;
					}
					if(i == 0) {
						offset_x = xtmp;
						xtmp = 0;
					} else if(i == m.size() - 1) {
						offset_w = wotmp;
						wotmp = 0;
					}
					wtmp = cairo_image_surface_get_width(surface_terms[i]) / scalefactor;
					htmp = cairo_image_surface_get_height(surface_terms[i]) / scalefactor;
					hpt.push_back(htmp);
					cpt.push_back(hetmp);
					wpt.push_back(wtmp);
					xpt.push_back(xtmp);
					w -= xtmp;
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
				cairo_t *cr = cairo_create(surface);
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
						w -= xpt[i];
						cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
						cairo_paint(cr);
						w += wpt[i];
					}
					cairo_surface_destroy(surface_terms[i]);
				}
				g_object_unref(layout_minus);
				g_object_unref(layout_plus);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_NEGATE: {
				ips_n.depth++;

				gint minus_w, minus_h, uh, dh, h, w, ctmp, htmp, wtmp, hpa, cpa, xtmp;

				PangoLayout *layout_minus = gtk_widget_create_pango_layout(resultview, NULL);

				if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) {
					PANGO_TTP(layout_minus, SIGN_MINUS);
				} else {
					PANGO_TTP(layout_minus, "-");
				}
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout_minus, &rect, &lrect);
				minus_w = lrect.width;
				minus_h = lrect.height;
				if(rect.x < 0) {
					minus_w -= rect.x;
					offset_x = -rect.x;
				}

				w = minus_w + 1;
				uh = minus_h / 2 + minus_h % 2;
				dh = minus_h / 2;

				ips_n.wrap = m[0].isPower() ? m[0][0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0) : m[0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				cairo_surface_t *surface_arg = draw_structure(m[0], po, caf, ips_n, &ctmp, scaledown, color, &xtmp, &offset_w, ips.depth == 0 && max_width > 0 ? max_width - minus_w : -1);
				if(!surface_arg) {
					g_object_unref(layout_minus);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_arg) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_arg) / scalefactor;
				hpa = htmp;
				cpa = ctmp;
				w += wtmp - xtmp;
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
				cairo_t *cr = cairo_create(surface);

				w = offset_x;
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, w, uh - minus_h / 2 - minus_h % 2);
				pango_cairo_show_layout(cr, layout_minus);
				w += minus_w + 1 - xtmp;
				cairo_set_source_surface(cr, surface_arg, w, uh - (hpa - cpa));
				cairo_paint(cr);
				cairo_surface_destroy(surface_arg);

				g_object_unref(layout_minus);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_MULTIPLICATION: {

				ips_n.depth++;

				vector<cairo_surface_t*> surface_terms;
				vector<gint> hpt, wpt, cpt, xpt, wopt;
				gint mul_w = 0, mul_h = 0, altmul_w = 0, altmul_h = 0, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0, xtmp = 0, wotmp = 0;

				bool b_cis = (!caf && m.size() == 2 && (m[0].isNumber() || (m[0].isNegate() && m[0][0].isNumber())) && m[1].isFunction() && m[1].size() == 1 && m[1].function()->referenceName() == "cis" && (((m[1][0].isNumber() || (m[1][0].isNegate() && m[1][0][0].isNumber())) || (m[1][0].isMultiplication() && m[1][0].size() == 2 && (m[1][0][0].isNumber() || (m[1][0].isNegate() && m[1][0][0][0].isNumber())) && m[1][0][1].isUnit())) || (m[1][0].isNegate() && m[1][0][0].isMultiplication() && m[1][0][0].size() == 2 && m[1][0][0][0].isNumber() && m[1][0][0][1].isUnit())));

				CALCULATE_SPACE_W
				PangoLayout *layout_mul = NULL, *layout_altmul = NULL; 
				
				bool par_prev = false;
				vector<int> nm;
				for(size_t i = 0; i < m.size(); i++) {
					hetmp = 0;
					ips_n.wrap = b_cis ? (i == 1 && ((m[1][0].isMultiplication() && m[1][0][1].neededMultiplicationSign(po, ips_n, m[1][0], 2, false, false, false, false) != MULTIPLICATION_SIGN_NONE) || (m[1][0].isNegate() && m[1][0][0].isMultiplication() && m[1][0][0][1].neededMultiplicationSign(po, ips_n, m[1][0][0], 2, false, false, false, false) != MULTIPLICATION_SIGN_NONE))) : m[i].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure((b_cis && i == 1) ? m[i][0] : m[i], po, caf, ips_n, &hetmp, scaledown, color, &xtmp, &wotmp));
					if(CALCULATOR->aborted()) {
						for(size_t i = 0; i < surface_terms.size(); i++) {
							if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
						}
						g_object_unref(layout_mul);
						return NULL;
					}
					wtmp = cairo_image_surface_get_width(surface_terms[i]) / scalefactor;
					if(i == 0) {
						offset_x = xtmp;
						xtmp = 0;
					} else if(i == m.size() - 1) {
						offset_w = wotmp;
						wotmp = 0;
					}
					htmp = cairo_image_surface_get_height(surface_terms[i]) / scalefactor;
					hpt.push_back(htmp);
					cpt.push_back(hetmp);
					wpt.push_back(wtmp);
					xpt.push_back(xtmp);
					wopt.push_back(wotmp);
					w -= wotmp;
					w -= xtmp;
					w += wtmp;
					if(i > 0) {
						if(b_cis || !po.short_multiplication) {
							nm.push_back(MULTIPLICATION_SIGN_OPERATOR);
						} else {
							nm.push_back(m[i].neededMultiplicationSign(po, ips_n, m, i + 1, ips_n.wrap || (m[i].isPower() && m[i][0].needsParenthesis(po, ips_n, m[i], 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0)), par_prev, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0));
							if(nm[i] == MULTIPLICATION_SIGN_NONE && m[i].isPower() && m[i][0].isUnit() && po.use_unicode_signs && po.abbreviate_names && m[i][0].unit() == CALCULATOR->getDegUnit()) {
								PrintOptions po2 = po;
								po2.use_unicode_signs = false;
								nm[i] = m[i].neededMultiplicationSign(po2, ips_n, m, i + 1, ips_n.wrap || (m[i].isPower() && m[i][0].needsParenthesis(po, ips_n, m[i], 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0)), par_prev, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
							}
						}
						if(nm[i] != MULTIPLICATION_SIGN_NONE) {
							w += wopt[i - 1];
							wopt[i - 1] = 0;
						}
						switch(nm[i]) {
							case MULTIPLICATION_SIGN_SPACE: {
								w += space_w;
								break;
							}
							case MULTIPLICATION_SIGN_OPERATOR_SHORT: {}
							case MULTIPLICATION_SIGN_OPERATOR: {
								if(!b_cis && po.place_units_separately && po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_X || po.multiplication_sign == MULTIPLICATION_SIGN_ASTERISK) && m[i].isUnit_exp() && m[i - 1].isUnit_exp() && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) {
									if(!layout_altmul) {
										string str;
										TTP_SMALL(str, SIGN_MIDDLEDOT);
										layout_altmul = gtk_widget_create_pango_layout(resultview, NULL);
										pango_layout_set_markup(layout_altmul, str.c_str(), -1);
										pango_layout_get_pixel_size(layout_altmul, &altmul_w, &altmul_h);
									}
									w += altmul_w + (space_w / 2) * 2;
									if(altmul_h / 2 > dh) {
										dh = altmul_h / 2;
									}
									if(altmul_h / 2 + altmul_h % 2 > uh) {
										uh = altmul_h / 2 + altmul_h % 2;
									}
									break;
								}
								if(!layout_mul) {
									string str;
									if(b_cis) {
										TTP(str, "cis");
									} else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) {
										TTP_SMALL(str, SIGN_MULTIDOT);
									} else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) {
										TTP_SMALL(str, SIGN_MIDDLEDOT);
									} else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) {
										TTP_SMALL(str, SIGN_MULTIPLICATION);
									} else {
										TTP(str, "*");
									}
									layout_mul = gtk_widget_create_pango_layout(resultview, NULL);
									pango_layout_set_markup(layout_mul, str.c_str(), -1);
									pango_layout_get_pixel_size(layout_mul, &mul_w, &mul_h);
								}
								if(nm[i] == MULTIPLICATION_SIGN_OPERATOR_SHORT && m[i].isUnit_exp() && m[i - 1].isUnit_exp()) w += mul_w + (space_w / 2) * 2;
								else if(nm[i] == MULTIPLICATION_SIGN_OPERATOR_SHORT) w += mul_w;
								else w += mul_w + space_w * 2;
								if(mul_h / 2 > dh) {
									dh = mul_h / 2;
								}
								if(mul_h / 2 + mul_h % 2 > uh) {
									uh = mul_h / 2 + mul_h % 2;
								}
								break;
							}
							default: {
								if(par_prev || (m[i - 1].size() && m[i - 1].type() != STRUCT_POWER)) {
									w += xtmp;
									xpt[i] = 0;
									w += wopt[i - 1];
									wopt[i - 1] = 0;
								}
								w++;
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
					if(par_prev && i > 0 && nm[i] != MULTIPLICATION_SIGN_NONE) {
						wpt[i - 1] -= ips.power_depth > 0 ? 2 : 3;
						w -= ips.power_depth > 0 ? 2 : 3;
					}
				}
				cairo_surface_t *flag_s = NULL;
				gint flag_width = 0;
				size_t flag_i = 0;
				if(m.size() == 2 && ((m[0].isUnit() && m[0].unit()->isCurrency() && m[1].isNumber()) || (m[1].isUnit() && m[1].unit()->isCurrency() && m[0].isNumber()))) {
					size_t i_unit = 0;
					if(m[1].isUnit()) {
						i_unit = 1;
						flag_i = 1;
					} else if(nm[1] == MULTIPLICATION_SIGN_NONE) {
						flag_i = 1;
					}
					string imagefile = "/qalculate-gtk/flags/"; imagefile += m[i_unit].unit()->referenceName(); imagefile += ".png";
					h = hpt[flag_i];
					GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource_at_scale(imagefile.c_str(), -1, h / 2.5 * scalefactor, TRUE, NULL);
					if(pixbuf) {
						flag_s = gdk_cairo_surface_create_from_pixbuf(pixbuf, scalefactor, NULL);
						flag_width = cairo_image_surface_get_width(flag_s);
						w += flag_width + 2;
						g_object_unref(pixbuf);
					}
				}
				central_point = dh;
				h = dh + uh;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				w = 0;
				for(size_t i = 0; i < surface_terms.size(); i++) {
					if(!CALCULATOR->aborted()) {
						gdk_cairo_set_source_rgba(cr, color);
						if(i > 0) {
							switch(nm[i]) {
								case MULTIPLICATION_SIGN_SPACE: {
									w += space_w;
									break;
								}
								case MULTIPLICATION_SIGN_OPERATOR: {}
								case MULTIPLICATION_SIGN_OPERATOR_SHORT: {
									if(layout_altmul && m[i].isUnit_exp() && m[i - 1].isUnit_exp()) {
										w += space_w / 2;
										cairo_move_to(cr, w, uh - altmul_h / 2 - altmul_h % 2);
										pango_cairo_show_layout(cr, layout_altmul);
										w += altmul_w;
										w += space_w / 2;
									} else {
										if(nm[i] == MULTIPLICATION_SIGN_OPERATOR_SHORT && m[i].isUnit_exp() && m[i - 1].isUnit_exp()) w += space_w / 2;
										else if(nm[i] == MULTIPLICATION_SIGN_OPERATOR) w += space_w;
										cairo_move_to(cr, w, uh - mul_h / 2 - mul_h % 2);
										pango_cairo_show_layout(cr, layout_mul);
										w += mul_w;
										if(nm[i] == MULTIPLICATION_SIGN_OPERATOR_SHORT && m[i].isUnit_exp() && m[i - 1].isUnit_exp()) w += space_w / 2;
										else if(nm[i] == MULTIPLICATION_SIGN_OPERATOR) w += space_w;
									}
									break;
								}
								default: {w++;}
							}
						}
						w -= xpt[i];
						cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
						cairo_paint(cr);
						w += wpt[i];
						w -= wopt[i];
						if(flag_s && i == 0 && flag_i == 0) {
							gdk_cairo_set_source_rgba(cr, color);
							cairo_set_source_surface(cr, flag_s, w + 2, uh - (hpt[i] - cpt[i]) + hpt[i] / 8);
							cairo_paint(cr);
							cairo_surface_destroy(flag_s);
							flag_s = NULL;
							w += flag_width + 2;
						}
					}
					cairo_surface_destroy(surface_terms[i]);
				}
				if(flag_s) {
					if(!CALCULATOR->aborted()) {
						gdk_cairo_set_source_rgba(cr, color);
						cairo_set_source_surface(cr, flag_s, w + 2, uh - (hpt.back() - cpt.back()) + hpt.back() / 8);
						cairo_paint(cr);
					}
					cairo_surface_destroy(flag_s);
				}
				if(layout_mul) g_object_unref(layout_mul);
				if(layout_altmul) g_object_unref(layout_altmul);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_INVERSE: {}
			case STRUCT_DIVISION: {

				ips_n.depth++;
				ips_n.division_depth++;

				gint den_uh, den_w, den_dh, num_w, num_dh, num_uh, dh = 0, uh = 0, w = 0, h = 0, xtmp1, xtmp2, wotmp1, wotmp2;

				bool flat = ips.division_depth > 0 || ips.power_depth > 0;
				bool b_units = false;
				if(po.place_units_separately) {
					b_units = true;
					size_t i = 0;
					if(m.isDivision()) {
						i = 1;
					}
					if(m[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < m[i].size(); i2++) {
							if(!m[i][i2].isUnit_exp()) {
								b_units = false;
								break;
							}
						}
					} else if(!m[i].isUnit_exp()) {
						b_units = false;
					}
					if(b_units) {
						ips_n.division_depth--;
						flat = true;
					}
				}

				cairo_surface_t *num_surface = NULL, *den_surface = NULL;
				if(m.type() == STRUCT_DIVISION) {
					ips_n.wrap = (!m[0].isDivision() || !flat || ips.division_depth > 0 || ips.power_depth > 0) && !b_units && m[0].needsParenthesis(po, ips_n, m, 1, flat, ips.power_depth > 0);
					num_surface = draw_structure(m[0], po, caf, ips_n, &num_dh, scaledown, color, &xtmp1, &wotmp1);
				} else {
					MathStructure onestruct(1, 1);
					ips_n.wrap = false;
					num_surface = draw_structure(onestruct, po, caf, ips_n, &num_dh, scaledown, color, &xtmp1, &wotmp1);
				}
				if(!num_surface) {
					return NULL;
				}
				num_w = cairo_image_surface_get_width(num_surface) / scalefactor;
				h = cairo_image_surface_get_height(num_surface) / scalefactor;
				num_uh = h - num_dh;
				if(m.type() == STRUCT_DIVISION) {
					ips_n.wrap = m[1].needsParenthesis(po, ips_n, m, 2, flat, ips.power_depth > 0);
					den_surface = draw_structure(m[1], po, caf, ips_n, &den_dh, scaledown, color, &xtmp2, &wotmp2);
				} else {
					ips_n.wrap = m[0].needsParenthesis(po, ips_n, m, 2, flat, ips.power_depth > 0);
					den_surface = draw_structure(m[0], po, caf, ips_n, &den_dh, scaledown, color, &xtmp2, &wotmp2);
				}
				if(flat && !ips_n.wrap && m[m.type() == STRUCT_DIVISION ? 1 : 0].isNumber()) {
					unordered_map<void*, string>::iterator it = number_exp_map.find((void*) &m[m.type() == STRUCT_DIVISION ? 1 : 0].number());
					if(it != number_exp_map.end() && !it->second.empty()) ips_n.wrap = true;
				}
				if(!den_surface) {
					cairo_surface_destroy(num_surface);
					return NULL;
				}
				den_w = cairo_image_surface_get_width(den_surface) / scalefactor;
				h = cairo_image_surface_get_height(den_surface) / scalefactor;
				den_uh = h - den_dh;
				h = 0;
				if(flat) {
					offset_x = xtmp1;
					offset_w = wotmp2;
					gint div_w, div_h, space_w = 0;
					PangoLayout *layout_div = gtk_widget_create_pango_layout(resultview, NULL);
					if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
						PANGO_TTP(layout_div, SIGN_DIVISION);
					} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
						PANGO_TTP(layout_div, SIGN_DIVISION_SLASH);
						PangoRectangle rect;
						pango_layout_get_pixel_extents(layout_div, &rect, NULL);
						if(rect.x < 0) space_w = -rect.x;
					} else {
						PANGO_TTP(layout_div, "/");
					}
					pango_layout_get_pixel_size(layout_div, &div_w, &div_h);
					w = num_w + den_w - xtmp2 + space_w * 2 + div_w;
					dh = num_dh; uh = num_uh;
					if(den_dh > dh) dh = den_dh;
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
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);
					w = 0;
					cairo_set_source_surface(cr, num_surface, w, uh - num_uh);
					cairo_paint(cr);
					w += num_w;
					w += space_w;
					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, w, uh - div_h / 2 - div_h % 2);
					pango_cairo_show_layout(cr, layout_div);
					w += div_w;
					w += space_w;
					w -= xtmp2;
					cairo_set_source_surface(cr, den_surface, w, uh - den_uh);
					cairo_paint(cr);
					g_object_unref(layout_div);
					cairo_destroy(cr);
				} else {
					num_w = num_w - xtmp1 - wotmp1;
					den_w = den_w - xtmp2 - wotmp2;
					int y1n;
					get_image_blank_height(num_surface, &y1n, NULL);
					y1n /= scalefactor;
					num_uh -= y1n;
					int y2d;
					get_image_blank_height(den_surface, NULL, &y2d);
					y2d = ::ceil((y2d + 1) / scalefactor);
					den_dh -= (den_dh + den_uh - y2d);
					gint wfr;
					dh = den_dh + den_uh + 3;
					uh = num_dh + num_uh + 3;
					wfr = den_w;
					if(num_w > wfr) wfr = num_w;
					w = wfr;
					h = uh + dh;
					central_point = dh;
					gint w_extra = ips.depth > 0 ? 4 : 1;
					gint num_pos = (wfr - num_w) / 2;
					gint den_pos = (wfr - den_w) / 2;
					if(num_pos - xtmp1 < 0) offset_x = -(num_pos - xtmp1);
					if(den_pos - xtmp2 < -offset_x) offset_x = -(den_pos - xtmp2);
					if(num_pos + num_w + wotmp1 > w) offset_w = (num_pos + num_w + wotmp1) - w;
					if((den_pos + den_w + wotmp2) - w > offset_w) offset_w = (den_pos + den_w + wotmp2) - w;
					w += offset_x + offset_w;
					wfr = w;
					if(num_pos - (wotmp1 + xtmp1) > den_pos) num_pos = (wfr - num_w) / 2;
					else num_pos += offset_x;
					if(den_pos - (wotmp2 + xtmp2) > num_pos) den_pos = (wfr - den_w) / 2;
					else den_pos += offset_x;
					wfr += 2; w += 2; num_pos++; den_pos++;
					w += w_extra * 2;
					surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);
					w = w_extra;
					cairo_set_source_surface(cr, num_surface, w + num_pos - xtmp1, -y1n);
					cairo_paint(cr);
					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, w, uh);
					cairo_line_to(cr, w + wfr, uh);
					cairo_set_line_width(cr, 2);
					cairo_stroke(cr);
					cairo_set_source_surface(cr, den_surface, w + den_pos - xtmp2, uh + 3);
					cairo_paint(cr);
					offset_x = 0;
					offset_w = 0;
					cairo_destroy(cr);
				}
				if(num_surface) cairo_surface_destroy(num_surface);
				if(den_surface) cairo_surface_destroy(den_surface);
				break;
			}
			case STRUCT_POWER: {

				ips_n.depth++;

				gint base_w, base_h, exp_w, exp_h, w = 0, h = 0, ctmp = 0;
				CALCULATE_SPACE_W
				ips_n.wrap = SHOW_WITH_ROOT_SIGN(m[0]) || m[0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0, false);
				if(!ips_n.wrap && m[0].isNumber()) {
					unordered_map<void*, string>::iterator it = number_exp_map.find((void*) &m[0].number());
					if(it != number_exp_map.end() && !it->second.empty()) ips_n.wrap = true;
				}
				cairo_surface_t *surface_base = NULL;
				if(m[0].isUnit() && po.use_unicode_signs && po.abbreviate_names && m[0].unit() == CALCULATOR->getDegUnit()) {
					PrintOptions po2 = po;
					po2.use_unicode_signs = false;
					surface_base = draw_structure(m[0], po2, caf, ips_n, &central_point, scaledown, color, &offset_x);
				} else {
					surface_base = draw_structure(m[0], po, caf, ips_n, &central_point, scaledown, color, &offset_x);
				}
				if(!surface_base) {
					return NULL;
				}
				base_w = cairo_image_surface_get_width(surface_base) / scalefactor;
				base_h = cairo_image_surface_get_height(surface_base) / scalefactor;

				ips_n.power_depth++;
				ips_n.wrap = false;
				PrintOptions po2 = po;
				po2.show_ending_zeroes = false;
				cairo_surface_t *surface_exp = draw_structure(m[1], po2, caf, ips_n, &ctmp, scaledown, color);
				if(!surface_exp) {
					cairo_surface_destroy(surface_base);
					return NULL;
				}
				exp_w = cairo_image_surface_get_width(surface_exp) / scalefactor;
				exp_h = cairo_image_surface_get_height(surface_exp) / scalefactor;
				h = base_h;
				w = base_w;
				if(exp_h <= h) {
					h += exp_h / 5;
				} else {
					h += exp_h - base_h / 1.5;
				}
				w += exp_w;

				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
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
				cairo_destroy(cr);

				break;
			}
			case STRUCT_LOGICAL_AND: {
				if(m.size() == 2 && m[0].isComparison() && m[1].isComparison() && m[0].comparisonType() != COMPARISON_EQUALS && m[0].comparisonType() != COMPARISON_NOT_EQUALS && m[1].comparisonType() != COMPARISON_EQUALS && m[1].comparisonType() != COMPARISON_NOT_EQUALS && m[0][0] == m[1][0]) {
					ips_n.depth++;

					vector<cairo_surface_t*> surface_terms;
					vector<gint> hpt, wpt, cpt, xpt;
					gint sign_w, sign_h, sign2_w, sign2_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0, xtmp = 0;
					CALCULATE_SPACE_W

					hetmp = 0;
					ips_n.wrap = m[0][1].needsParenthesis(po, ips_n, m[0], 2, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure(m[0][1], po, caf, ips_n, &hetmp, scaledown, color, &offset_x, NULL));
					if(CALCULATOR->aborted()) {
						cairo_surface_destroy(surface_terms[0]);
						return NULL;
					}
					wtmp = cairo_image_surface_get_width(surface_terms[0]) / scalefactor;
					htmp = cairo_image_surface_get_height(surface_terms[0]) / scalefactor;
					hpt.push_back(htmp);
					cpt.push_back(hetmp);
					wpt.push_back(wtmp);
					xpt.push_back(0);
					w += wtmp;
					if(htmp - hetmp > uh) {
						uh = htmp - hetmp;
					}
					if(hetmp > dh) {
						dh = hetmp;
					}
					hetmp = 0;
					ips_n.wrap = m[0][0].needsParenthesis(po, ips_n, m[0], 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure(m[0][0], po, caf, ips_n, &hetmp, scaledown, color, &xtmp, NULL));
					if(CALCULATOR->aborted()) {
						cairo_surface_destroy(surface_terms[0]);
						cairo_surface_destroy(surface_terms[1]);
						return NULL;
					}
					wtmp = cairo_image_surface_get_width(surface_terms[1]) / scalefactor;
					htmp = cairo_image_surface_get_height(surface_terms[1]) / scalefactor;
					hpt.push_back(htmp);
					cpt.push_back(hetmp);
					wpt.push_back(wtmp);
					xpt.push_back(xtmp);
					w -= xtmp;
					w += wtmp;
					if(htmp - hetmp > uh) {
						uh = htmp - hetmp;
					}
					if(hetmp > dh) {
						dh = hetmp;
					}
					hetmp = 0;
					ips_n.wrap = m[1][1].needsParenthesis(po, ips_n, m[1], 2, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure(m[1][1], po, caf, ips_n, &hetmp, scaledown, color, &xtmp, &offset_w));
					if(CALCULATOR->aborted()) {
						cairo_surface_destroy(surface_terms[0]);
						cairo_surface_destroy(surface_terms[1]);
						cairo_surface_destroy(surface_terms[2]);
						return NULL;
					}
					wtmp = cairo_image_surface_get_width(surface_terms[2]) / scalefactor;
					htmp = cairo_image_surface_get_height(surface_terms[2]) / scalefactor;
					hpt.push_back(htmp);
					cpt.push_back(hetmp);
					wpt.push_back(wtmp);
					xpt.push_back(xtmp);
					w -= xtmp;
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
					cairo_t *cr = cairo_create(surface);
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
						w -= xpt[i];
						cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
						cairo_paint(cr);
						w += wpt[i];
						cairo_surface_destroy(surface_terms[i]);
					}
					g_object_unref(layout_sign);
					g_object_unref(layout_sign2);
					cairo_destroy(cr);
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
				vector<gint> hpt, wpt, cpt, xpt;
				gint sign_w, sign_h, wtmp, htmp, hetmp = 0, w = 0, h = 0, dh = 0, uh = 0, xtmp = 0, wotmp = 0;
				CALCULATE_SPACE_W

				for(size_t i = 0; i < m.size(); i++) {
					hetmp = 0;
					ips_n.wrap = m[i].needsParenthesis(po, ips_n, m, i + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					surface_terms.push_back(draw_structure(m[i], po, caf, ips_n, &hetmp, scaledown, color, &xtmp, &wotmp));
					if(CALCULATOR->aborted()) {
						for(size_t i = 0; i < surface_terms.size(); i++) {
							if(surface_terms[i]) cairo_surface_destroy(surface_terms[i]);
						}
						return NULL;
					}
					if(i == 0) {
						offset_x = xtmp;
						xtmp = 0;
					} else if(i == m.size() - 1) {
						offset_w = wotmp;
						wotmp = 0;
					}
					wtmp = cairo_image_surface_get_width(surface_terms[i]) / scalefactor;
					htmp = cairo_image_surface_get_height(surface_terms[i]) / scalefactor;
					hpt.push_back(htmp);
					cpt.push_back(hetmp);
					wpt.push_back(wtmp);
					xpt.push_back(xtmp);
					w -= xtmp;
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
							if((ips.depth == 0 || (po.interval_display != INTERVAL_DISPLAY_INTERVAL && m.containsInterval())) && po.use_unicode_signs && ((po.is_approximate && *po.is_approximate) || m.isApproximate()) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) {
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
					str += "xor";
				} else if(m.type() == STRUCT_BITWISE_AND) {
					str += "&amp;";
				} else if(m.type() == STRUCT_BITWISE_OR) {
					str += "|";
				} else if(m.type() == STRUCT_BITWISE_XOR) {
					str += "xor";
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
				cairo_t *cr = cairo_create(surface);
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
						w -= xpt[i];
						cairo_set_source_surface(cr, surface_terms[i], w, uh - (hpt[i] - cpt[i]));
						cairo_paint(cr);
						w += wpt[i];
					}
					cairo_surface_destroy(surface_terms[i]);
				}
				g_object_unref(layout_sign);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_LOGICAL_NOT: {}
			case STRUCT_BITWISE_NOT: {

				ips_n.depth++;

				gint not_w, not_h, uh, dh, h, w, ctmp, htmp, wtmp, hpa, cpa, xtmp;
				//gint wpa;

				PangoLayout *layout_not = gtk_widget_create_pango_layout(resultview, NULL);

				if(m.type() == STRUCT_LOGICAL_NOT) {
					PANGO_TTP(layout_not, "!");
				} else {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("¬", po.can_display_unicode_string_arg))) {
						PANGO_TTP(layout_not, "¬");
					} else {
						PANGO_TTP(layout_not, "~");
					}
				}
				pango_layout_get_pixel_size(layout_not, &not_w, &not_h);

				w = not_w + 1;
				uh = not_h / 2 + not_h % 2;
				dh = not_h / 2;

				ips_n.wrap = m[0].needsParenthesis(po, ips_n, m, 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
				cairo_surface_t *surface_arg = draw_structure(m[0], po, caf, ips_n, &ctmp, scaledown, color, &xtmp, &offset_w);
				if(!surface_arg) {
					g_object_unref(layout_not);
					return NULL;
				}
				wtmp = cairo_image_surface_get_width(surface_arg) / scalefactor;
				htmp = cairo_image_surface_get_height(surface_arg) / scalefactor;
				hpa = htmp;
				cpa = ctmp;
				//wpa = wtmp;
				w -= xtmp;
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
				cairo_t *cr = cairo_create(surface);

				w = 0;
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, w, uh - not_h / 2 - not_h % 2);
				pango_cairo_show_layout(cr, layout_not);
				w += not_w + 1 - xtmp;
				cairo_set_source_surface(cr, surface_arg, w, uh - (hpa - cpa));
				cairo_paint(cr);
				cairo_surface_destroy(surface_arg);

				g_object_unref(layout_not);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_VECTOR: {

				ips_n.depth++;

				bool b_matrix = m.isMatrix();
				if(m.size() == 0 || (b_matrix && m[0].size() == 0)) {
					PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
					string str;
					TTBP(str)
					str += "[ ]";
					TTE(str)
					pango_layout_set_markup(layout, str.c_str(), -1);
					pango_layout_get_pixel_size(layout, &w, &h);
					w += 1;
					central_point = h / 2;
					surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, 1, 0);
					pango_cairo_show_layout(cr, layout);
					g_object_unref(layout);
					cairo_destroy(cr);
					break;
				}
				bool flat_matrix = po.preserve_format && b_matrix && m.rows() > 3;
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
				element_w.resize(b_matrix ? m.size() : 1);
				element_h.resize(b_matrix ? m.size() : 1);
				element_c.resize(b_matrix ? m.size() : 1);
				surface_elements.resize(b_matrix ? m.size() : 1);
				PangoLayout *layout_comma = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				gint comma_w = 0, comma_h = 0;
				TTP(str, ";")
				pango_layout_set_markup(layout_comma, str.c_str(), -1);
				pango_layout_get_pixel_size(layout_comma, &comma_w, &comma_h);
				for(size_t index_r = 0; index_r < m.size(); index_r++) {
					for(size_t index_c = 0; index_c < (b_matrix ? m[index_r].size() : m.size()); index_c++) {
						ctmp = 0;
						if(b_matrix) ips_n.wrap = m[index_r][index_c].needsParenthesis(po, ips_n, m, index_r + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
						else ips_n.wrap = m[index_c].needsParenthesis(po, ips_n, m, index_r + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
						surface_elements[index_r].push_back(draw_structure(b_matrix ? m[index_r][index_c] : m[index_c], po, caf, ips_n, &ctmp, scaledown, color));
						if(CALCULATOR->aborted()) {
							break;
						}
						wtmp = cairo_image_surface_get_width(surface_elements[index_r][index_c]) / scalefactor;
						htmp = cairo_image_surface_get_height(surface_elements[index_r][index_c]) / scalefactor;
						element_w[index_r].push_back(wtmp);
						element_h[index_r].push_back(htmp);
						element_c[index_r].push_back(ctmp);
						if(flat_matrix) {
							w += wtmp;
							if(index_c != 0) w += space_w * 2;
						} else if(index_r == 0) {
							col_w.push_back(wtmp);
						} else if(wtmp > col_w[index_c]) {
							col_w[index_c] = wtmp;
						}
						if(index_c == 0 && (!flat_matrix || index_r == 0)) {
							row_uh.push_back(htmp - ctmp);
							row_dh.push_back(ctmp);
						} else {
							if(ctmp > row_dh[flat_matrix ? 0 : index_r]) {
								row_dh[flat_matrix ? 0 : index_r] = ctmp;
							}
							if(htmp - ctmp > row_uh[flat_matrix ? 0 : index_r]) {
								row_uh[flat_matrix ? 0 : index_r] = htmp - ctmp;
							}
						}
					}
					if(CALCULATOR->aborted()) {
						break;
					}
					if(!flat_matrix || index_r == m.size() - 1) {
						row_h.push_back(row_uh[flat_matrix ? 0 : index_r] + row_dh[flat_matrix ? 0 : index_r]);
						h += row_h[flat_matrix ? 0 : index_r];
					} else if(flat_matrix) {
						w += space_w;
						w += comma_w;
					}
					if(!flat_matrix && index_r != 0) {
						h += 4;
					}
					if(!b_matrix) break;
				}
				h += 4;
				for(size_t i = 0; !flat_matrix && i < col_w.size(); i++) {
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
				cairo_t *cr = cairo_create(surface);
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
						if(index_r == 0 || !flat_matrix) {
							w = wll + 1;
						} else {
							cairo_move_to(cr, w, central_point - comma_h / 2 - comma_h % 2);
							pango_cairo_show_layout(cr, layout_comma);
							w += space_w;
							w += comma_w;
						}
					}
					for(size_t index_c = 0; index_c < surface_elements[index_r].size(); index_c++) {
						if(!CALCULATOR->aborted()) {
							cairo_set_source_surface(cr, surface_elements[index_r][index_c], flat_matrix ? w : w + (col_w[index_c] - element_w[index_r][index_c]), h + row_uh[flat_matrix ? 0 : index_r] - (element_h[index_r][index_c] - element_c[index_r][index_c]));
							cairo_paint(cr);
							if(flat_matrix) w += element_w[index_r][index_c];
							else w += col_w[index_c];
							if(index_c != (b_matrix ? m[index_r].size() - 1 : m.size() - 1)) {
								w += space_w * 2;
							}
						}
						if(surface_elements[index_r][index_c]) {
							cairo_surface_destroy(surface_elements[index_r][index_c]);
						}
					}
					if(!CALCULATOR->aborted() && !flat_matrix) {
						h += row_h[index_r];
						h += 4;
					}
				}
				if(flat_matrix) h += row_h[0];
				else h -= 4;
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
				cairo_destroy(cr);
				break;
			}
			case STRUCT_UNIT: {

				string str, str2;
				TTBP(str);

				const ExpressionName *ename = &m.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, m.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);

				if(m.prefix()) {
					str += m.prefix()->preferredDisplayName(ename->abbreviation, po.use_unicode_signs, m.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).formattedName(-1, false, true, false, true, true);
				}
				str += ename->formattedName(TYPE_UNIT, true, true, false, true, true);
				FIX_SUB_RESULT(str)
				size_t i = 0;
				while(true) {
					i = str.find("<sub>", i);
					if(i == string::npos) break;
					string str_s;
					TTBP_SMALL(str_s);
					str.insert(i, str_s);
					i = str.find("</sub>", i);
					if(i == string::npos) break;
					str.insert(i + 6, TEXT_TAGS_END);
				}
				TTE(str);
				PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
				pango_layout_set_markup(layout, str.c_str(), -1);
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout, &rect, &lrect);
				w = lrect.width;
				h = lrect.height;
				if(rect.x < 0) {
					w -= rect.x;
					if(rect.width > w) {
						offset_w = rect.width - w;
						w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > w) {
						offset_w = rect.width + rect.x - w;
						w = rect.width + rect.x;
					}
				}
				central_point = h / 2;
				if(rect.y < 0) h -= rect.y;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, offset_x, rect.y < 0 ? -rect.y : 0);
				pango_cairo_show_layout(cr, layout);
				g_object_unref(layout);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_VARIABLE: {

				PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
				string str;

				const ExpressionName *ename = &m.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);

				bool cursive = m.variable() != CALCULATOR->v_i && ename->name != "%" && ename->name != "‰" && ename->name != "‱" && m.variable()->referenceName() != "true" && m.variable()->referenceName() != "false";
				if(cursive) str = "<i>";
				TTBP(str);
				str += ename->formattedName(TYPE_VARIABLE, true, true, false, true, true);
				FIX_SUB_RESULT(str)
				size_t i = 0;
				while(true) {
					i = str.find("<sub>", i);
					if(i == string::npos) break;
					string str_s;
					TTBP_SMALL(str_s);
					str.insert(i, str_s);
					i = str.find("</sub>", i);
					if(i == string::npos) break;
					str.insert(i + 6, TEXT_TAGS_END);
				}
				TTE(str);
				if(cursive) str += "</i>";

				pango_layout_set_markup(layout, str.c_str(), -1);
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout, &rect, &lrect);
				w = lrect.width;
				h = lrect.height;
				if(rect.x < 0) {
					w -= rect.x;
					if(rect.width > w) {
						offset_w = rect.width - w;
						w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > w) {
						offset_w = rect.width + rect.x - w;
						w = rect.width + rect.x;
					}
				}
				if(m.variable() == CALCULATOR->v_i) w += 1;
				central_point = h / 2;
				if(rect.y < 0) h -= rect.y;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, offset_x, rect.y < 0 ? -rect.y : 0);
				pango_cairo_show_layout(cr, layout);
				g_object_unref(layout);
				cairo_destroy(cr);
				break;
			}
			case STRUCT_FUNCTION: {

				if(m.function() == CALCULATOR->f_uncertainty && m.size() == 3 && m[2].isZero()) {
					ips_n.depth++;
					gint unc_uh, unc_w, unc_dh, mid_w, mid_dh, mid_uh, dh = 0, uh = 0, w = 0, h = 0;
					cairo_surface_t *mid_surface = NULL, *unc_surface = NULL;
					ips_n.wrap = !m[0].isNumber();
					PrintOptions po2 = po;
					po2.show_ending_zeroes = false;
					po2.number_fraction_format = FRACTION_DECIMAL;
					mid_surface = draw_structure(m[0], po2, caf, ips_n, &mid_dh, scaledown, color, &offset_x, NULL);
					if(!mid_surface) {
						return NULL;
					}
					mid_w = cairo_image_surface_get_width(mid_surface) / scalefactor;
					h = cairo_image_surface_get_height(mid_surface) / scalefactor;
					mid_uh = h - mid_dh;
					ips_n.wrap = !m[1].isNumber();
					unc_surface = draw_structure(m[1], po2, caf, ips_n, &unc_dh, scaledown, color, NULL, &offset_w);
					unc_w = cairo_image_surface_get_width(unc_surface) / scalefactor;
					h = cairo_image_surface_get_height(unc_surface) / scalefactor;
					unc_uh = h - unc_dh;
					h = 0;
					gint pm_w, pm_h;
					PangoLayout *layout_pm = gtk_widget_create_pango_layout(resultview, NULL);
					PANGO_TTP(layout_pm, SIGN_PLUSMINUS);
					pango_layout_get_pixel_size(layout_pm, &pm_w, &pm_h);
					w = mid_w + unc_w + pm_w;
					dh = mid_dh; uh = mid_uh;
					if(unc_dh > dh) h = unc_dh;
					if(unc_uh > uh) uh = unc_uh;
					if(pm_h / 2 > dh) {
						dh = pm_h / 2;
					}
					if(pm_h / 2 + pm_h % 2 > uh) {
						uh = pm_h / 2 + pm_h % 2;
					}
					h = uh + dh;
					central_point = dh;
					surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);
					w = 0;
					cairo_set_source_surface(cr, mid_surface, w, uh - mid_uh);
					cairo_paint(cr);
					w += mid_w;
					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, w, uh - pm_h / 2 - pm_h % 2);
					pango_cairo_show_layout(cr, layout_pm);
					w += pm_w;
					cairo_set_source_surface(cr, unc_surface, w, uh - unc_uh);
					cairo_paint(cr);
					g_object_unref(layout_pm);
					cairo_surface_destroy(mid_surface);
					cairo_surface_destroy(unc_surface);
					cairo_destroy(cr);
					break;
				} else if(SHOW_WITH_ROOT_SIGN(m)) {

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
					PangoRectangle rect;
					pango_layout_get_pixel_extents(layout_root, &rect, NULL);
					root_h = rect.y + rect.height;

					ips_n.wrap = false;
					cairo_surface_t *surface_arg = draw_structure(m[0], po, caf, ips_n, &ctmp, scaledown, color);
					if(!surface_arg) return NULL;

					arg_w = cairo_image_surface_get_width(surface_arg) / scalefactor;
					arg_h = cairo_image_surface_get_height(surface_arg) / scalefactor;

					int y;
					get_image_blank_height(surface_arg, &y, NULL);
					y /= scalefactor;
					y -= 6;
					arg_h -= y;

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
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);

					cairo_move_to(cr, 0, h / 2.0 + h / 15.0);
					cairo_line_to(cr, sign_w / 6.0, h / 2.0);
					cairo_line_to(cr, sign_w / 2.2, h - extra_space / divider);
					cairo_line_to(cr, sign_w,  extra_space / divider);
					cairo_line_to(cr, w,  extra_space / divider);
					cairo_set_line_width(cr, 2 / divider);
					cairo_stroke(cr);

					if(i_root != 2) {
						cairo_move_to(cr, (sign_w - root_w) / 3.0, (h / 2.0) - root_h - extra_space / (divider * 2) - 1);
						cairo_surface_set_device_scale(surface, scalefactor / divider, scalefactor / divider);
						pango_cairo_show_layout(cr, layout_root);
						cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					}

					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, 0, 0);
					cairo_set_source_surface(cr, surface_arg, sign_w + 1, h - arg_h - extra_space / divider - y);
					cairo_paint(cr);

					cairo_surface_destroy(surface_arg);
					g_object_unref(layout_root);
					cairo_destroy(cr);

					break;

				} else if((m.function() == CALCULATOR->f_abs || m.function() == CALCULATOR->f_floor || m.function() == CALCULATOR->f_ceil) && m.size() == 1) {

					ips_n.depth++;
					gint arg_w, arg_h, h, w, ctmp;

					ips_n.wrap = false;
					cairo_surface_t *surface_arg = draw_structure(m[0], po, caf, ips_n, &ctmp, scaledown, color);
					if(!surface_arg) return NULL;

					arg_w = cairo_image_surface_get_width(surface_arg) / scalefactor;
					arg_h = cairo_image_surface_get_height(surface_arg) / scalefactor;

					double divider = 1.0;
					if(ips.power_depth >= 1) divider = 1.5;

					gint extra_space = m.function() == CALCULATOR->f_abs ? 5 : 3;
					gint bracket_length = (m.function() == CALCULATOR->f_abs ? 0 : 7);
					
					int y;
					get_image_blank_height(surface_arg, &y, NULL);
					y /= scalefactor;
					y -= 6; if(y < 0) y = 0;
					arg_h -= y;

					gint line_space = extra_space / divider;
					central_point = ctmp + line_space;
					h = arg_h + line_space * 2;
					w = arg_w + (m.function() != CALCULATOR->f_abs && extra_space > 2 ? 4 : extra_space * 2) + extra_space * 2 / divider + bracket_length * 2;
					double linewidth = 2 / divider;

					surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);

					cairo_move_to(cr, (extra_space / divider), line_space);
					cairo_line_to(cr, (extra_space / divider), h - line_space);
					cairo_move_to(cr, w - (extra_space / divider), line_space);
					cairo_line_to(cr, w - (extra_space / divider), h - line_space);
					if(m.function() == CALCULATOR->f_floor) {
						cairo_move_to(cr, extra_space / divider, h - line_space - linewidth / 2);
						cairo_line_to(cr, extra_space / divider + bracket_length, h - line_space - linewidth / 2);
						cairo_move_to(cr, w - (extra_space / divider) - bracket_length, h - line_space - linewidth / 2);
						cairo_line_to(cr, w - (extra_space / divider), h - line_space - linewidth / 2);
					} else if(m.function() == CALCULATOR->f_ceil) {
						cairo_move_to(cr, extra_space / divider, line_space + linewidth / 2);
						cairo_line_to(cr, extra_space / divider + bracket_length, line_space + linewidth / 2);
						cairo_move_to(cr, w - (extra_space / divider) - bracket_length, line_space + linewidth / 2);
						cairo_line_to(cr, w - (extra_space / divider), line_space + linewidth / 2);
					}
					cairo_set_line_width(cr, linewidth);
					cairo_stroke(cr);

					gdk_cairo_set_source_rgba(cr, color);
					cairo_move_to(cr, 0, 0);
					cairo_set_source_surface(cr, surface_arg, (w - arg_w) / 2.0, line_space - y);
					cairo_paint(cr);

					cairo_surface_destroy(surface_arg);
					cairo_destroy(cr);

					break;
				} else if(m.function() == CALCULATOR->f_diff && (m.size() == 3 || (m.size() == 4 && m[3].isUndefined())) && (m[1].isVariable() || m[1].isSymbolic()) && m[2].isInteger()) {

					MathStructure mdx("d");
					if(!m[2].isOne()) mdx ^= m[2];
					string s = "d";
					if(m[1].isSymbolic()) s += m[1].symbol();
					else s += m[1].variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).formattedName(TYPE_VARIABLE, true, true, false, true, true);
					mdx.transform(STRUCT_DIVISION, s);
					if(!m[2].isOne()) mdx[1] ^= m[2];

					ips_n.depth++;

					gint hpt1, hpt2;
					gint wpt1, wpt2;
					gint cpt1, cpt2;
					gint w = 0, h = 0, dh = 0, uh = 0;

					CALCULATE_SPACE_W

					ips_n.wrap = false;
					cairo_surface_t *surface_term1 = draw_structure(mdx, po, caf, ips_n, &cpt1, scaledown, color);
					wpt1 = cairo_image_surface_get_width(surface_term1) / scalefactor;
					hpt1 = cairo_image_surface_get_height(surface_term1) / scalefactor;
					ips_n.wrap = true;
					cairo_surface_t *surface_term2 = draw_structure(m[0], po, caf, ips_n, &cpt2, scaledown, color);
					wpt2 = cairo_image_surface_get_width(surface_term2) / scalefactor;
					hpt2 = cairo_image_surface_get_height(surface_term2) / scalefactor;
					w = wpt1 + wpt2 + space_w;
					if(hpt1 - cpt1 > hpt2 - cpt2) uh = hpt1 - cpt1;
					else uh = hpt2 - cpt2;
					if(cpt1 > cpt2) dh = cpt1;
					else dh = cpt2;
					central_point = dh;
					h = dh + uh;
					surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
					cairo_t *cr = cairo_create(surface);
					gdk_cairo_set_source_rgba(cr, color);
					cairo_set_source_surface(cr, surface_term1, 0, uh - (hpt1 - cpt1));
					cairo_paint(cr);
					gdk_cairo_set_source_rgba(cr, color);
					cairo_set_source_surface(cr, surface_term2, wpt1 + space_w, uh - (hpt2 - cpt2));
					cairo_paint(cr);
					cairo_surface_destroy(surface_term1);
					cairo_surface_destroy(surface_term2);
					cairo_destroy(cr);

					break;
				}

				ips_n.depth++;

				gint comma_w, comma_h, function_w, function_h, uh, dh, h, w, ctmp, htmp, wtmp, arc_w, arc_h, xtmp;
				vector<cairo_surface_t*> surface_args;
				vector<gint> hpa, cpa, wpa, xpa;

				CALCULATE_SPACE_W
				PangoLayout *layout_comma = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				TTP(str, po.comma())
				pango_layout_set_markup(layout_comma, str.c_str(), -1);
				pango_layout_get_pixel_size(layout_comma, &comma_w, &comma_h);
				PangoLayout *layout_function = gtk_widget_create_pango_layout(resultview, NULL);

				str = "";
				TTBP(str);

				size_t argcount = m.size();

				if(m.function() == CALCULATOR->f_signum && argcount > 1) {
					argcount = 1;
				} else if(m.function() == CALCULATOR->f_integrate && argcount > 3) {
					if(m[1].isUndefined() && m[2].isUndefined()) argcount = 1;
					else argcount = 3;
				} else if((m.function()->maxargs() < 0 || m.function()->minargs() < m.function()->maxargs()) && m.size() > (size_t) m.function()->minargs()) {
					while(true) {
						string defstr = m.function()->getDefaultValue(argcount);
						if(defstr.empty() && m.function()->maxargs() < 0) break;
						Argument *arg = m.function()->getArgumentDefinition(argcount);
						remove_blank_ends(defstr);
						if(defstr.empty()) break;
						if(m[argcount - 1].isUndefined() && defstr == "undefined") {
							argcount--;
						} else if(argcount > 1 && arg && arg->type() == ARGUMENT_TYPE_SYMBOLIC && ((argcount > 1 && defstr == "undefined" && m[argcount - 1] == m[0].find_x_var()) || (defstr == "\"\"" && m[argcount - 1] == ""))) {
							argcount--;
						} else if(m[argcount - 1].isVariable() && (!arg || (arg->type() != ARGUMENT_TYPE_TEXT && !arg->suggestsQuotes())) && defstr == m[argcount - 1].variable()->referenceName()) {
							argcount--;
						} else if(m[argcount - 1].isInteger() && (!arg || (arg->type() != ARGUMENT_TYPE_TEXT && !arg->suggestsQuotes())) && defstr.find_first_not_of(NUMBERS, defstr[0] == '-' && defstr.length() > 1 ? 1 : 0) == string::npos && m[argcount - 1].number() == s2i(defstr)) {
							argcount--;
						} else if(defstr[0] == '-' && m[argcount - 1].isNegate() && m[argcount - 1][0].isInteger() && (!arg || (arg->type() != ARGUMENT_TYPE_TEXT && !arg->suggestsQuotes())) && defstr.find_first_not_of(NUMBERS, 1) == string::npos && m[argcount - 1][0].number() == -s2i(defstr)) {
							argcount--;
						} else if(defstr[0] == '-' && m[argcount - 1].isMultiplication() && m[argcount - 1].size() == 2 && (m[argcount - 1][0].isMinusOne() || (m[argcount - 1][0].isNegate() && m[argcount - 1][0][0].isOne())) && m[argcount - 1][1].isInteger() && (!arg || (arg->type() != ARGUMENT_TYPE_TEXT && !arg->suggestsQuotes())) && defstr.find_first_not_of(NUMBERS, 1) == string::npos && m[argcount - 1][1].number() == -s2i(defstr)) {
							argcount--;
						} else if(m[argcount - 1].isSymbolic() && arg && arg->type() == ARGUMENT_TYPE_TEXT && (m[argcount - 1].symbol() == defstr || (defstr == "\"\"" && m[argcount - 1].symbol().empty()))) {
							argcount--;
						} else {
							break;
						}
						if(argcount == 0 || argcount == (size_t) m.function()->minargs()) break;
					}
				}

				const ExpressionName *ename = &m.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
				str += ename->formattedName(TYPE_FUNCTION, true, true, false, true, true);
				FIX_SUB_RESULT(str)
				size_t i = 0;
				bool b_sub = false;
				while(true) {
					i = str.find("<sub>", i);
					if(i == string::npos) break;
					b_sub = true;
					string str_s;
					TTBP_SMALL(str_s);
					str.insert(i, str_s);
					i = str.find("</sub>", i);
					if(i == string::npos) break;
					str.insert(i + 6, TEXT_TAGS_END);
				}
				if(!b_sub && (m.function() == CALCULATOR->f_lambert_w || m.function() == CALCULATOR->f_logn) && m.size() == 2 && ((m[1].size() == 0 && (!m[1].isNumber() || (m[1].number().isInteger() && m[1].number() < 100 && m[1].number() > -100))) || (m[1].isNegate() && m[1][0].size() == 0 && (!m[1][0].isNumber() || (m[1][0].number().isInteger() && m[1][0].number() < 100 && m[1][0].number() > -100))))) {
					argcount = 1;
					str += "<sub>";
					str += m[1].print(po);
					str += "</sub>";
				}

				TTE(str);

				pango_layout_set_markup(layout_function, str.c_str(), -1);
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout_function, &rect, &lrect);
				function_w = lrect.width;
				function_h = lrect.height;
				if(rect.x < 0) {
					function_w -= rect.x;
					if(rect.width > function_w) {
						function_w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > function_w) {
						function_w = rect.width + rect.x;
					}
				}
				w = function_w + 1;
				uh = function_h / 2 + function_h % 2;
				dh = function_h / 2;
				if(rect.y < 0) {
					uh = -rect.y;
					function_h -= rect.y;
				}

				for(size_t index = 0; index < argcount; index++) {

					ips_n.wrap = m[index].needsParenthesis(po, ips_n, m, index + 1, ips.division_depth > 0 || ips.power_depth > 0, ips.power_depth > 0);
					if(m.function() == CALCULATOR->f_interval) {
						PrintOptions po2 = po;
						po2.show_ending_zeroes = false;
						if(m[index].isNumber()) {
							if(index == 0) po2.interval_display = INTERVAL_DISPLAY_LOWER;
							else if(index == 1) po2.interval_display = INTERVAL_DISPLAY_UPPER;
						}
						surface_args.push_back(draw_structure(m[index], po2, caf, ips_n, &ctmp, scaledown, color, &xtmp));
					} else {
						surface_args.push_back(draw_structure(m[index], po, caf, ips_n, &ctmp, scaledown, color, &xtmp));
					}
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
					if(index == 0) xtmp = 0;
					hpa.push_back(htmp);
					cpa.push_back(ctmp);
					wpa.push_back(wtmp);
					xpa.push_back(xtmp);
					if(index > 0) {
						w += comma_w;
						w += space_w;
					}
					w -= xtmp;
					w += wtmp;
					if(ctmp > dh) {
						dh = ctmp;
					}
					if(htmp - ctmp > uh) {
						uh = htmp - ctmp;
					}
				}

				if(dh > uh) uh = dh;
				else if(uh > dh) dh = uh;
				h = uh + dh;
				central_point = dh;
				arc_h = h;
				arc_w = PAR_WIDTH;
				w += arc_w * 2;
				w += ips.power_depth > 0 ? 3 : 4;

				int x1 = 0, x2 = 0;
				if(surface_args.size() == 1) {
					get_image_blank_width(surface_args[0], &x1, &x2);
					x1 /= scalefactor;
					x1++;
					x2 = ::ceil(x2 / scalefactor);
					w -= wpa[0];
					wpa[0] = x2 - x1;
					w += wpa[0];
				} else if(surface_args.size() > 1) {
					get_image_blank_width(surface_args[0], &x1, NULL);
					x1 /= scalefactor;
					x1++;
					w -= x1;
					wpa[0] -= x1;
					int i_last = surface_args.size() - 1;
					get_image_blank_width(surface_args[i_last], NULL, &x2);
					x2 = ::ceil(x2 / scalefactor);
					w -= wpa[i_last] - x2;
					wpa[i_last] -= wpa[i_last] - x2;
				}

				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);

				w = 0;
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, w, uh - function_h / 2 - function_h % 2);
				pango_cairo_show_layout(cr, layout_function);
				w += function_w;
				w += ips.power_depth > 0 ? 2 : 3;
				cairo_set_source_surface(cr, get_left_parenthesis(arc_w, arc_h, scaledown, color), w, (h - arc_h) / 2);
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
						w -= xpa[index];
						cairo_set_source_surface(cr, surface_args[index], index == 0 ? w - x1 : w, uh - (hpa[index] - cpa[index]));
						cairo_paint(cr);
						w += wpa[index];
					}
					cairo_surface_destroy(surface_args[index]);
				}
				cairo_set_source_surface(cr, get_right_parenthesis(arc_w, arc_h, scaledown, color), w, (h - arc_h) / 2);
				cairo_paint(cr);

				g_object_unref(layout_comma);
				g_object_unref(layout_function);
				cairo_destroy(cr);

				break;
			}
			case STRUCT_UNDEFINED: {
				PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
				string str;
				TTP(str, _("undefined"));
				pango_layout_set_markup(layout, str.c_str(), -1);
				PangoRectangle rect, lrect;
				pango_layout_get_pixel_extents(layout, &rect, &lrect);
				w = lrect.width;
				h = lrect.height;
				if(rect.x < 0) {
					w -= rect.x;
					if(rect.width > w) {
						offset_w = rect.width - w;
						w = rect.width;
					}
					offset_x = -rect.x;
				} else {
					if(rect.width + rect.x > w) {
						offset_w = rect.width + rect.x - w;
						w = rect.width + rect.x;
					}
				}
				central_point = h / 2;
				if(rect.y < 0) h -= rect.y;
				surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
				cairo_t *cr = cairo_create(surface);
				gdk_cairo_set_source_rgba(cr, color);
				cairo_move_to(cr, offset_x, rect.y < 0 ? -rect.y : 0);
				pango_cairo_show_layout(cr, layout);
				g_object_unref(layout);
				cairo_destroy(cr);
				break;
			}
			default: {}
		}
	}
	if(ips.wrap && surface) {
		gint w, h, base_h, base_w;
		offset_w = 0; offset_x = 0;
		base_w = cairo_image_surface_get_width(surface) / scalefactor;
		base_h = cairo_image_surface_get_height(surface) / scalefactor;
		int x1 = 0, x2 = 0;
		get_image_blank_width(surface, &x1, &x2);
		x1 /= scalefactor;
		x1++;
		x2 = ::ceil(x2 / scalefactor);
		base_w = x2 - x1;
		h = base_h;
		w = base_w;
		gint base_dh = central_point;
		if(h > central_point * 2) central_point = h - central_point;
		gint arc_base_h = central_point * 2;
		if(h < arc_base_h) h = arc_base_h;
		gint arc_base_w = PAR_WIDTH;
		w += arc_base_w * 2;
		w += ips.power_depth > 0 ? 2 : 3;
		cairo_surface_t *surface_old = surface;
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cairo_t *cr = cairo_create(surface);
		gdk_cairo_set_source_rgba(cr, color);
		w = ips.power_depth > 0 ? 2 : 3;
		cairo_set_source_surface(cr, get_left_parenthesis(arc_base_w, arc_base_h, scaledown, color), w, (h - arc_base_h) / 2);
		cairo_paint(cr);
		w += arc_base_w;
		cairo_set_source_surface(cr, surface_old, w - x1, central_point - (base_h - base_dh));
		cairo_paint(cr);
		cairo_surface_destroy(surface_old);
		w += base_w;
		cairo_set_source_surface(cr, get_right_parenthesis(arc_base_w, arc_base_h, scaledown, color), w, (h - arc_base_h) / 2);
		cairo_paint(cr);
		cairo_destroy(cr);
	}
	if(ips.depth == 0 && !po.preserve_format && !(m.isComparison() && (!((po.is_approximate && *po.is_approximate) || m.isApproximate()) || (m.comparisonType() == COMPARISON_EQUALS && po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))))) && surface) {
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
				TTB(str);
				str += "= ";
				str += _("approx.");
				TTE(str);
				pango_layout_set_markup(layout_equals, str.c_str(), -1);
			}
		} else {
			PANGO_TT(layout_equals, "=");
		}
		CALCULATE_SPACE_W
		PangoRectangle rect, lrect;
		pango_layout_get_pixel_extents(layout_equals, &rect, &lrect);
		wle = lrect.width - offset_x;
		offset_x = 0;
		if(rect.x < 0) {
			wle -= rect.x;
			offset_x = -rect.x;
		}
		hle = lrect.height;
		w_new = w + wle + space_w;
		h_new = h;
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w_new * scalefactor, h_new * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
		cairo_t *cr = cairo_create(surface);
		gdk_cairo_set_source_rgba(cr, color);
		cairo_move_to(cr, offset_x, h - central_point - hle / 2 - hle % 2);
		pango_cairo_show_layout(cr, layout_equals);
		for(size_t i = 0; i < binary_rect.size(); i++) {
			binary_rect[i].x = binary_rect[i].x + wle + space_w;
		}
		cairo_set_source_surface(cr, surface_old, wle + space_w, 0);
		cairo_paint(cr);
		cairo_surface_destroy(surface_old);
		g_object_unref(layout_equals);
		cairo_destroy(cr);
	}
	if(!surface) {
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1 * scalefactor, 1 * scalefactor);
		cairo_surface_set_device_scale(surface, scalefactor, scalefactor);
	}
	if(point_central) *point_central = central_point;
	if(x_offset) *x_offset = offset_x;
	if(w_offset) *w_offset = offset_w;
	return surface;
}

void set_status_bottom_border_visible(bool b) {
	gchar *gstr = gtk_css_provider_to_string(statusframe_provider);
	string status_css = gstr;
	g_free(gstr);
	if(b) {
		gsub("border-bottom-width: 0;", "", status_css);
	} else {
		gsub("}", "border-bottom-width: 0;}", status_css);
	}
	gtk_css_provider_load_from_data(statusframe_provider, status_css.c_str(), -1, NULL);
}

void clearresult() {
	if(!parsed_in_result || !surface_parsed || rpn_mode) minimal_mode_show_resultview(false);
	show_parsed_instead_of_result = false;
	showing_first_time_message = false;
	if(displayed_mstruct) {
		displayed_mstruct->unref();
		displayed_mstruct = NULL;
		if(!surface_result && !surface_parsed) gtk_widget_queue_draw(resultview);
	}
	if(!parsed_in_result) result_autocalculated = false;
	result_too_long = false;
	display_aborted = false;
	result_display_overflow = false;
	date_map.clear();
	number_map.clear();
	number_base_map.clear();
	number_exp_map.clear();
	number_exp_minus_map.clear();
	number_approx_map.clear();
	if(gtk_revealer_get_child_revealed(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")))) {
		gtk_info_bar_response(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_RESPONSE_CLOSE);
	}
	update_expression_icons();
	if(surface_parsed) {
		cairo_surface_destroy(surface_parsed);
		surface_parsed = NULL;
		if(!surface_result) gtk_widget_queue_draw(resultview);
	}
	if(surface_result) {
		cairo_surface_destroy(surface_result);
		surface_result = NULL;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
		gtk_widget_queue_draw(resultview);
	}
	if(visible_keypad & PROGRAMMING_KEYPAD) clear_result_bases();
	gtk_widget_set_tooltip_text(resultview, "");
}

void on_abort_display(GtkDialog*, gint, gpointer) {
	CALCULATOR->abort();
}

void replace_interval_with_function(MathStructure &m) {
	if(m.isNumber() && m.number().isInterval()) {
		m.transform(STRUCT_FUNCTION);
		m.setFunction(CALCULATOR->f_interval);
		m.addChild(m[0]);
	} else {
		for(size_t i = 0; i < m.size(); i++) replace_interval_with_function(m[i]);
	}
}

void ViewThread::run() {

	while(true) {
		int scale_tmp = 0;
		if(!read(&scale_tmp)) break;
		void *x = NULL;
		if(!read(&x) || !x) break;
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
			if(!read(&po.is_approximate)) break;
			void *x_to = NULL;
			if(!read(&x_to)) break;
			po.show_ending_zeroes = evalops.parse_options.read_precision != DONT_READ_PRECISION && CALCULATOR->usesIntervalArithmetic() && evalops.parse_options.base > BASE_CUSTOM;
			po.exp_display = printops.exp_display;
			po.lower_case_numbers = printops.lower_case_numbers;
			po.base_display = printops.base_display;
			po.round_halfway_to_even = printops.round_halfway_to_even;
			po.twos_complement = printops.twos_complement;
			po.hexadecimal_twos_complement = printops.hexadecimal_twos_complement;
			po.base = evalops.parse_options.base;
			po.preserve_format = (x_to != NULL);
			Number nr_base;
			if(po.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
				nr_base = CALCULATOR->customOutputBase();
				CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
			} else if(po.base == BASE_CUSTOM || (po.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26 && po.base != BASE_BINARY_DECIMAL)) {
				po.base = 10;
				po.min_exp = 6;
				po.use_max_decimals = true;
				po.max_decimals = 5;
				po.preserve_format = false;
			}
			po.abbreviate_names = false;
			po.use_unicode_signs = printops.use_unicode_signs;
			po.digit_grouping = printops.digit_grouping;
			po.multiplication_sign = printops.multiplication_sign;
			po.division_sign = printops.division_sign;
			po.short_multiplication = false;
			po.excessive_parenthesis = true;
			po.improve_division_multipliers = false;
			po.can_display_unicode_string_function = &can_display_unicode_string_function;
			po.can_display_unicode_string_arg = (void*) statuslabel_l;
			po.spell_out_logical_operators = printops.spell_out_logical_operators;
			po.restrict_to_parent_precision = false;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			MathStructure mp(*((MathStructure*) x));
			mp.format(po);
			parsed_text = mp.print(po, true);
			if(x_to && !((MathStructure*) x_to)->isUndefined()) {
				mp.set(*((MathStructure*) x_to));
				parsed_text += CALCULATOR->localToString();
				mp.format(po);
				parsed_text += mp.print(po, true);
				printops.use_unit_prefixes = true;
			}
			gsub("&nbsp;", " ", parsed_text);
			if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
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

		// convert time units to hours when using time format
		if(printops.base == BASE_TIME && is_time(m)) {
			Unit *u = CALCULATOR->getActiveUnit("h");
			if(u) {
				m.divide(u);
				m.eval(evalops);
			}
		}

		if(printops.spell_out_logical_operators && x && test_parsed_comparison_gtk(*((MathStructure*) x))) {
			if(m.isZero()) {
				Variable *v = CALCULATOR->getActiveVariable("false");
				if(v) m.set(v);
			} else if(m.isOne()) {
				Variable *v = CALCULATOR->getActiveVariable("true");
				if(v) m.set(v);
			}
		}

		m.removeDefaultAngleUnit(evalops);
		m.format(printops);
		m.removeDefaultAngleUnit(evalops);
		gint64 time1 = g_get_monotonic_time();
		PrintOptions po = printops;
		po.base_display = BASE_DISPLAY_SUFFIX;
		result_text = m.print(po, true);
		gsub("&nbsp;", " ", result_text);
		if(complex_angle_form) replace_result_cis_gtk(result_text);
		result_text_approximate = *printops.is_approximate;

		if(!b_stack && visible_keypad & PROGRAMMING_KEYPAD) {
			set_result_bases(m);
		}

		if(!b_stack && g_get_monotonic_time() - time1 < 200000) {
			PrintOptions printops_long = printops;
			printops_long.abbreviate_names = false;
			printops_long.short_multiplication = false;
			printops_long.excessive_parenthesis = true;
			printops_long.is_approximate = NULL;
			if(printops_long.use_unicode_signs) printops_long.use_unicode_signs = UNICODE_SIGNS_ONLY_UNIT_EXPONENTS;
			result_text_long = m.print(printops_long);
			if(complex_angle_form) replace_result_cis_gtk(result_text_long);
		} else if(!b_stack) {
			result_text_long = "";
		}
		printops.can_display_unicode_string_arg = NULL;

		result_too_long = false;
		result_display_overflow = false;
		if(!b_stack && unhtmlize(result_text).length() > 900) {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			result_too_long = true;
			pango_layout_set_markup(layout, _("result is too long\nsee history"), -1);
			gint w = 0, h = 0;
			pango_layout_get_pixel_size(layout, &w, &h);
			PangoRectangle rect;
			pango_layout_get_pixel_extents(layout, &rect, NULL);
			if(rect.x < 0) {w -= rect.x; if(rect.width > w) w = rect.width;}
			else if(w < rect.x + rect.width) w = rect.x + rect.width;
			gint scalefactor = RESULT_SCALE_FACTOR;
			tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_surface_set_device_scale(tmp_surface, scalefactor, scalefactor);
			cairo_t *cr = cairo_create(tmp_surface);
			GdkRGBA rgba;
			gtk_style_context_get_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview), &rgba);
			gdk_cairo_set_source_rgba(cr, &rgba);
			if(rect.x < 0) cairo_move_to(cr, -rect.x, 0);
			pango_cairo_show_layout(cr, layout);
			cairo_destroy(cr);
			g_object_unref(layout);
			*printops.is_approximate = false;
			if(displayed_mstruct) displayed_mstruct->unref();
			displayed_mstruct = new MathStructure(m);
		} else if(!b_stack && m.isAborted()) {
			PangoLayout *layout = gtk_widget_create_pango_layout(resultview, NULL);
			pango_layout_set_markup(layout, _("calculation was aborted"), -1);
			gint w = 0, h = 0;
			pango_layout_get_pixel_size(layout, &w, &h);
			PangoRectangle rect;
			pango_layout_get_pixel_extents(layout, &rect, NULL);
			if(rect.x < 0) {w -= rect.x; if(rect.width > w) w = rect.width;}
			else if(w < rect.x + rect.width) w = rect.x + rect.width;
			gint scalefactor = RESULT_SCALE_FACTOR;
			tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
			cairo_t *cr = cairo_create(tmp_surface);
			GdkRGBA rgba;
			gtk_style_context_get_color(gtk_widget_get_style_context(resultview), gtk_widget_get_state_flags(resultview), &rgba);
			gdk_cairo_set_source_rgba(cr, &rgba);
			if(rect.x < 0) cairo_move_to(cr, -rect.x, 0);
			pango_cairo_show_layout(cr, layout);
			cairo_destroy(cr);
			g_object_unref(layout);
			*printops.is_approximate = false;
			if(displayed_mstruct) displayed_mstruct->unref();
			displayed_mstruct = new MathStructure(m);
			displayed_printops = printops;
			displayed_printops.allow_non_usable = true;
			displayed_caf = complex_angle_form;
		} else if(!b_stack) {
			if(!CALCULATOR->aborted()) {
				printops.allow_non_usable = true;
				printops.can_display_unicode_string_arg = (void*) resultview;

				MathStructure *displayed_mstruct_pre = new MathStructure(m);
				if(printops.interval_display == INTERVAL_DISPLAY_INTERVAL) replace_interval_with_function(*displayed_mstruct_pre);
				tmp_surface = draw_structure(*displayed_mstruct_pre, printops, complex_angle_form, top_ips, NULL, scale_tmp);
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
			} else {
				displayed_printops = printops;
				displayed_printops.allow_non_usable = true;
				displayed_caf = complex_angle_form;
			}
		}
		result_autocalculated = false;
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

bool update_window_title(const char *str, bool is_result) {
	if(title_modified || !main_builder) return false;
	switch(title_type) {
		case TITLE_MODE: {
			if(is_result) return false;
			if(str && !current_mode.empty()) gtk_window_set_title(GTK_WINDOW(mainwindow), (current_mode + string(": ") + str).c_str());
			else if(!current_mode.empty()) gtk_window_set_title(GTK_WINDOW(mainwindow), current_mode.c_str());
			else if(str) gtk_window_set_title(GTK_WINDOW(mainwindow), (string("Qalculate! ") + str).c_str());
			else gtk_window_set_title(GTK_WINDOW(mainwindow), _("Qalculate!"));
			break;
		}
		case TITLE_APP_MODE: {
			if(is_result || (!current_mode.empty() && str)) return false;
			if(!current_mode.empty()) gtk_window_set_title(GTK_WINDOW(mainwindow), (string("Qalculate! ") + current_mode).c_str());
			else if(str) gtk_window_set_title(GTK_WINDOW(mainwindow), (string("Qalculate! ") + str).c_str());
			else gtk_window_set_title(GTK_WINDOW(mainwindow), _("Qalculate!"));
			break;
		}
		case TITLE_RESULT: {
			if(!str) return false;
			if(str) gtk_window_set_title(GTK_WINDOW(mainwindow), str);
			break;
		}
		case TITLE_APP_RESULT: {
			if(str) gtk_window_set_title(GTK_WINDOW(mainwindow), (string("Qalculate! (") + string(str) + ")").c_str());
			break;
		}
		default: {
			if(is_result) return false;
			if(str) gtk_window_set_title(GTK_WINDOW(mainwindow), (string("Qalculate! ") + str).c_str());
			else gtk_window_set_title(GTK_WINDOW(mainwindow), "Qalculate!");
		}
	}
	return true;
}

int intervals_are_relative(MathStructure &m) {
	int ret = -1;
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_UNCERTAINTY && m.size() == 3) {
		if(m[2].isOne() && m[1].isMultiplication() && m[1].size() > 1 && m[1].last().isVariable() && (m[1].last().variable() == CALCULATOR->getVariableById(VARIABLE_ID_PERCENT) || m[1].last().variable() == CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE) || m[1].last().variable() == CALCULATOR->getVariableById(VARIABLE_ID_PERMYRIAD))) {
			ret = 1;
		} else {
			return 0;
		}
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_INTERVAL) return 0;
	for(size_t i = 0; i < m.size(); i++) {
		int ret_i = intervals_are_relative(m[i]);
		if(ret_i == 0) return 0;
		else if(ret_i > 0) ret = ret_i;
	}
	return ret;
}

/*
	set result in result widget and add to history widget
*/
void setResult(Prefix *prefix, bool update_history, bool update_parse, bool force, string transformation, size_t stack_index, bool register_moved, bool supress_dialog) {

	if(result_blocked() || exit_in_progress) return;

	if(expression_modified() && (!rpn_mode || CALCULATOR->RPNStackSize() == 0)) {
		if(!force) return;
		execute_expression();
		if(!prefix) return;
	}

	if(rpn_mode && CALCULATOR->RPNStackSize() == 0) return;

	if(history_new_expression_count() == 0 && !register_moved && !update_parse && update_history) {
		update_history = false;
	}

	if(b_busy || b_busy_result || b_busy_expression || b_busy_command) return;

	stop_autocalculate_history_timeout();

	if(!rpn_mode) stack_index = 0;

	if(stack_index != 0) {
		update_history = true;
		update_parse = false;
	}
	if(register_moved) {
		update_history = true;
		update_parse = false;
	}

	bool error_icon = false;

	if(update_parse && parsed_mstruct && parsed_mstruct->isFunction() && (parsed_mstruct->function() == CALCULATOR->f_error || parsed_mstruct->function() == CALCULATOR->f_warning || parsed_mstruct->function() == CALCULATOR->f_message)) {
		string error_str;
		int mtype_highest = MESSAGE_INFORMATION;
		add_message_to_history(&error_str, &mtype_highest);
		block_expression_icon_update();
		clearresult();
		unblock_expression_icon_update();
		clear_expression_text();
		expression_display_errors(NULL, 1, true, error_str, mtype_highest);
		return;
	}

	block_error();
	b_busy = true;
	b_busy_result = true;
	display_aborted = false;

	if(!view_thread->running && !view_thread->start()) {
		b_busy = false;
		b_busy_result = false;
		unblock_error();
		return;
	}

	bool b_rpn_operation = false;
	if(update_history) {
		if(register_moved) {
			result_text = _("RPN Register Moved");
		} else if(result_text == _("RPN Operation")) {
			b_rpn_operation = true;
		}
	}

	bool first_expression = false;
	if(!add_result_to_history_pre(update_parse, update_history, register_moved, b_rpn_operation, &first_expression, result_text, transformation)) {
		b_busy = false;
		b_busy_result = false;
		unblock_error();
		return;
	}
	if(update_parse && adaptive_interval_display) {
		string expression_str = get_expression_text();
		if((parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_uncertainty)) || expression_str.find("+/-") != string::npos || expression_str.find("+/" SIGN_MINUS) != string::npos || expression_str.find("±") != string::npos) {
			if(parsed_mstruct && intervals_are_relative(*parsed_mstruct) > 0) printops.interval_display = INTERVAL_DISPLAY_RELATIVE;
			else printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		} else if(parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_interval)) printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
		else printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	}
	if(update_history) result_text = "?";

	if(update_parse) {
		parsed_text = "aborted";
	}

	if(stack_index == 0) {
		block_expression_icon_update();
		clearresult();
		unblock_expression_icon_update();
	}

	scale_n = 0;

	gint w = 0, h = 0;
	bool parsed_approx = false;
	bool title_set = false, was_busy = false;

	Number save_nbase;
	bool custom_base_set = false;
	int save_base = printops.base;
	bool caf_bak = complex_angle_form;
	unsigned int save_bits = printops.binary_bits;
	bool save_pre = printops.use_unit_prefixes;
	bool save_cur = printops.use_prefixes_for_currencies;
	bool save_allu = printops.use_prefixes_for_all_units;
	bool save_all = printops.use_all_prefixes;
	bool save_den = printops.use_denominator_prefix;
	int save_bin = CALCULATOR->usesBinaryPrefixes();
	long int save_fden = CALCULATOR->fixedDenominator();
	NumberFractionFormat save_format = printops.number_fraction_format;
	bool save_restrict_fraction_length = printops.restrict_fraction_length;
	bool do_to = false;
	bool result_cleared = false;

	if(stack_index == 0) {
		if(to_base != 0 || to_fraction > 0 || to_fixed_fraction >= 2 || to_prefix != 0 || (to_caf >= 0 && to_caf != complex_angle_form)) {
			if(to_base != 0 && (to_base != printops.base || to_bits != printops.binary_bits || (to_base == BASE_CUSTOM && to_nbase != CALCULATOR->customOutputBase()) || (to_base == BASE_DUODECIMAL && to_duo_syms != printops.duodecimal_symbols))) {
				printops.base = to_base;
				printops.duodecimal_symbols = to_duo_syms;
				printops.binary_bits = to_bits;
				if(to_base == BASE_CUSTOM) {
					custom_base_set = true;
					save_nbase = CALCULATOR->customOutputBase();
					CALCULATOR->setCustomOutputBase(to_nbase);
				}
				do_to = true;
			}
			if(to_fixed_fraction >= 2) {
				if(to_fraction == 2 || (to_fraction < 0 && !contains_fraction_gtk(*mstruct))) printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
				else printops.number_fraction_format = FRACTION_COMBINED_FIXED_DENOMINATOR;
				CALCULATOR->setFixedDenominator(to_fixed_fraction);
				do_to = true;
			} else if(to_fraction > 0 && (printops.restrict_fraction_length || (to_fraction != 2 && printops.number_fraction_format != FRACTION_COMBINED) || (to_fraction == 2 && printops.number_fraction_format != FRACTION_FRACTIONAL) || (to_fraction == 3 && printops.number_fraction_format != FRACTION_DECIMAL))) {
				printops.restrict_fraction_length = false;
				if(to_fraction == 3) printops.number_fraction_format = FRACTION_DECIMAL;
				else if(to_fraction == 2) printops.number_fraction_format = FRACTION_FRACTIONAL;
				else printops.number_fraction_format = FRACTION_COMBINED;
				do_to = true;
			}
			if(to_caf >= 0 && to_caf != complex_angle_form) {
				complex_angle_form = to_caf;
				do_to = true;
			}
			if(to_prefix != 0 && !prefix) {
				bool new_pre = printops.use_unit_prefixes;
				bool new_cur = printops.use_prefixes_for_currencies;
				bool new_allu = printops.use_prefixes_for_all_units;
				bool new_all = printops.use_all_prefixes;
				bool new_den = printops.use_denominator_prefix;
				int new_bin = CALCULATOR->usesBinaryPrefixes();
				new_pre = true;
				if(to_prefix == 'b') {
					int i = has_information_unit_gtk(*mstruct);
					new_bin = (i > 0 ? 1 : 2);
					if(i == 1) {
						new_den = false;
					} else if(i > 1) {
						new_den = true;
					} else {
						new_cur = true;
						new_allu = true;
					}
				} else {
					new_cur = true;
					new_allu = true;
					if(to_prefix == 'a') new_all = true;
					else if(to_prefix == 'd') new_bin = 0;
				}
				if(printops.use_unit_prefixes != new_pre || printops.use_prefixes_for_currencies != new_cur || printops.use_prefixes_for_all_units != new_allu || printops.use_all_prefixes != new_all || printops.use_denominator_prefix != new_den || CALCULATOR->usesBinaryPrefixes() != new_bin) {
					printops.use_unit_prefixes = new_pre;
					printops.use_all_prefixes = new_all;
					printops.use_prefixes_for_currencies = new_cur;
					printops.use_prefixes_for_all_units = new_allu;
					printops.use_denominator_prefix = new_den;
					CALCULATOR->useBinaryPrefixes(new_bin);
					do_to = true;
				}
			}
		}
		if(surface_result) {
			cairo_surface_destroy(surface_result);
			surface_result = NULL;
			result_cleared = true;
		}
		date_map.clear();
		number_map.clear();
		number_base_map.clear();
		number_exp_map.clear();
		number_exp_minus_map.clear();
		number_approx_map.clear();
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);
	}

	printops.prefix = prefix;
	tmp_surface = NULL;

#define SET_RESULT_RETURN {b_busy = false; b_busy_result = false; unblock_error(); return;}

	if(!view_thread->write(scale_n)) SET_RESULT_RETURN
	if(stack_index == 0) {
		if(!view_thread->write((void *) mstruct)) SET_RESULT_RETURN
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void *) mreg)) SET_RESULT_RETURN
	}
	bool b_stack = stack_index != 0;
	if(!view_thread->write(b_stack)) SET_RESULT_RETURN
	if(b_stack) {
		if(!view_thread->write(NULL)) SET_RESULT_RETURN
	} else {
		matrix_mstruct->clear();
		if(!view_thread->write((void *) matrix_mstruct)) SET_RESULT_RETURN
	}
	if(update_parse) {
		if(!view_thread->write((void *) parsed_mstruct)) SET_RESULT_RETURN
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write((void *) parsed_approx_p)) SET_RESULT_RETURN
		if(!view_thread->write((void *) (b_rpn_operation ? NULL : parsed_tostruct))) SET_RESULT_RETURN
	} else {
		if(!view_thread->write(NULL)) SET_RESULT_RETURN
	}

	int i = 0;
	while(b_busy && view_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(b_busy && view_thread->running) {
		if(result_cleared) gtk_widget_queue_draw(resultview);
		g_application_mark_busy(g_application_get_default());
		update_expression_icons(stack_index == 0 ? (!minimal_mode ? RESULT_SPINNER : EXPRESSION_SPINNER) : EXPRESSION_STOP);
		if(minimal_mode) gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		else gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		if(update_window_title(_("Processing…"))) title_set = true;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		was_busy = true;
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
			gint scalefactor = RESULT_SCALE_FACTOR;
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

	if(was_busy) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(!update_parse && stack_index == 0) hide_expression_spinner();
		if(title_set && stack_index != 0) update_window_title();
		if(minimal_mode) gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		else gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_unmark_busy(g_application_get_default());
	}

	if(stack_index == 0) {
		if(visible_keypad & PROGRAMMING_KEYPAD) update_result_bases();
		surface_result = NULL;
		if(tmp_surface) {
			showing_first_time_message = false;
			first_draw_of_result = true;
			surface_result = tmp_surface;
			minimal_mode_show_resultview();
			gtk_widget_queue_draw(resultview);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), displayed_mstruct && !result_too_long && !display_aborted);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), displayed_mstruct && !result_too_long && !display_aborted);
		}
		if(!update_window_title(unhtmlize(result_text).c_str(), true) && title_set) update_window_title();
	}
	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	} else if(first_expression) {
		update_parse = true;
	}
	bool implicit_warning = false;
	if(stack_index != 0) {
		if(result_text.length() > 500000) {
			if(mstruct->isMatrix()) {
				result_text = "matrix ("; result_text += i2s(mstruct->rows()); result_text += SIGN_MULTIPLICATION; result_text += i2s(mstruct->columns()); result_text += ")";
			} else {
				result_text = fix_history_string(ellipsize_result(unhtmlize(result_text), 5000));
			}
		}
		RPNRegisterChanged(unhtmlize(result_text), stack_index);
		error_icon = display_errors(supress_dialog ? NULL : mainwindow, supress_dialog ? 2 : 0);
	} else {
		bool b_approx = result_text_approximate || mstruct->isApproximate();
		string error_str;
		int mtype_highest = MESSAGE_INFORMATION;
		add_result_to_history(update_history, update_parse, register_moved, b_rpn_operation, result_text, b_approx, parsed_text, parsed_approx, transformation, supress_dialog ? NULL : mainwindow, &error_str, &mtype_highest, &implicit_warning);
		error_icon = expression_display_errors(supress_dialog ? NULL : mainwindow, 1, true, error_str, mtype_highest);
		if(update_history && result_text.length() < 1000) {
			string str;
			if(!b_approx) {
				str = "=";
			} else {
				if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) mainwindow)) {
					str = SIGN_ALMOST_EQUAL;
				} else {
					str = "= ";
					str += _("approx.");
				}
			}
			str += " ";
			if(result_text_long.empty()) {
				str += unhtmlize(result_text);
			} else {
				str += result_text_long;
			}
			gtk_widget_set_tooltip_text(resultview, enable_tooltips && str.length() < 1000 ? str.c_str() : "");
		}
		if(update_history && rpn_mode && !register_moved) {
			RPNRegisterChanged(unhtmlize(result_text), stack_index);
		}
	}
	if(do_to) {
		complex_angle_form = caf_bak;
		printops.base = save_base;
		printops.binary_bits = save_bits;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_nbase);
		printops.use_unit_prefixes = save_pre;
		printops.use_all_prefixes = save_all;
		printops.use_prefixes_for_currencies = save_cur;
		printops.use_prefixes_for_all_units = save_allu;
		printops.use_denominator_prefix = save_den;
		CALCULATOR->useBinaryPrefixes(save_bin);
		CALCULATOR->setFixedDenominator(save_fden);
		printops.number_fraction_format = save_format;
		printops.restrict_fraction_length = save_restrict_fraction_length;
	}
	printops.prefix = NULL;
	b_busy = false;
	b_busy_result = false;

	while(gtk_events_pending()) gtk_main_iteration();

	if(!register_moved && stack_index == 0 && mstruct->isMatrix() && matrix_mstruct->isMatrix() && matrix_mstruct->columns() < 200 && (result_too_long || result_display_overflow)) {
		focus_expression();
		if(update_history && update_parse && force) {
			expression_select_all();
		}
		if(!supress_dialog) insert_matrix(matrix_mstruct, mainwindow, false, true, true);
	}

	if(!error_icon && (update_parse || stack_index != 0)) update_expression_icons(rpn_mode ? 0 : EXPRESSION_CLEAR);

	if(implicit_warning) ask_implicit();

	unblock_error();

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
		if(!read(&x) || !x) break;
		CALCULATOR->startControl();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->structure(STRUCTURING_FACTORIZE, evalops, true);
				}
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				((MathStructure*) x)->expandPartialFractions(evalops);
				break;
			}
			case COMMAND_EXPAND: {
				((MathStructure*) x)->expand(evalops);
				break;
			}
			case COMMAND_TRANSFORM: {
				string ceu_str;
				if(continuous_conversion && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
					ParseOptions pa = evalops.parse_options; pa.base = 10;
					ceu_str = CALCULATOR->unlocalizeExpression(current_conversion_expression(), pa);
					remove_blank_ends(ceu_str);
					if(set_missing_prefixes && !ceu_str.empty()) {
						if(ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
							ceu_str = "?" + ceu_str;
						}
					}
					if(!ceu_str.empty() && ceu_str[0] == '?') {
						to_prefix = 1;
					} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
						to_prefix = ceu_str[0];
					}
				}
				((MathStructure*) x)->set(CALCULATOR->calculate(*((MathStructure*) x), evalops, ceu_str));
				break;
			}
			case COMMAND_CONVERT_STRING: {
				MathStructure pm_tmp(*parsed_mstruct);
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_units_string, evalops, NULL, true, &pm_tmp));
				break;
			}
			case COMMAND_CONVERT_UNIT: {
				MathStructure pm_tmp(*parsed_mstruct);
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_unit, evalops, false, true, true, &pm_tmp));
				break;
			}
			case COMMAND_CONVERT_OPTIMAL: {
				((MathStructure*) x)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x), evalops, true));
				break;
			}
			case COMMAND_CONVERT_BASE: {
				((MathStructure*) x)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x), evalops));
				break;
			}
			case COMMAND_CALCULATE: {
				EvaluationOptions eo2 = evalops;
				eo2.calculate_functions = false;
				eo2.sync_units = false;
				((MathStructure*) x)->calculatesub(eo2, eo2, true);
				break;
			}
			case COMMAND_EVAL: {
				((MathStructure*) x)->eval(evalops);
				break;
			}
		}
		b_busy = false;
		CALCULATOR->stopControl();
	}
}

void executeCommand(int command_type, bool show_result, bool force, string ceu_str, Unit *u, int run) {

	if(exit_in_progress) return;

	if(run == 1) {

		if(expression_modified() && !rpn_mode && command_type != COMMAND_TRANSFORM) {
			if(get_expression_text().find_first_not_of(SPACES) == string::npos) return;
			execute_expression();
		} else if(!displayed_mstruct && !force) {
			return;
		}

		if(b_busy || b_busy_result || b_busy_expression || b_busy_command) return;

		stop_autocalculate_history_timeout();

		if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING) {
			if(mbak_convert.isUndefined()) mbak_convert.set(*mstruct);
			else mstruct->set(mbak_convert);
		} else {
			if(!mbak_convert.isUndefined()) mbak_convert.setUndefined();
		}

		block_error();
		b_busy = true;
		b_busy_command = true;
		command_aborted = false;

		if(command_type >= COMMAND_CONVERT_UNIT) {
			CALCULATOR->resetExchangeRatesUsed();
			command_convert_units_string = ceu_str;
			command_convert_unit = u;
		}
		if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING || command_type == COMMAND_CONVERT_BASE || command_type == COMMAND_CONVERT_OPTIMAL) {
			to_prefix = 0;
		}
	}

	bool title_set = false, was_busy = false;

	int i = 0;

	MathStructure *mfactor = new MathStructure(*mstruct);
	MathStructure parsebak(*parsed_mstruct);

	rerun_command:

	if((!command_thread->running && !command_thread->start()) || !command_thread->write(command_type) || !command_thread->write((void *) mfactor)) {unblock_error(); b_busy = false; b_busy_command = false; return;}

	while(b_busy && command_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	cairo_surface_t *surface_result_bak = surface_result;

	if(b_busy && command_thread->running) {
		string progress_str;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				progress_str = _("Factorizing…");
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				progress_str = _("Expanding partial fractions…");
				break;
			}
			case COMMAND_EXPAND: {
				progress_str = _("Expanding…");
				break;
			}
			case COMMAND_EVAL: {}
			case COMMAND_TRANSFORM: {
				progress_str = _("Calculating…");
				break;
			}
			default: {
				progress_str = _("Converting…");
				break;
			}
		}
		if(update_window_title(progress_str.c_str())) title_set = true;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		update_expression_icons(!minimal_mode ? RESULT_SPINNER : EXPRESSION_SPINNER);
		if(!minimal_mode && surface_result) {
			surface_result = NULL;
			gtk_widget_queue_draw(resultview);
		}
		if(!minimal_mode) gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		else gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_mark_busy(g_application_get_default());
		was_busy = true;
	}
	while(b_busy && command_thread->running) {
		while(gtk_events_pending()) gtk_main_iteration();
		sleep_ms(100);
	}
	if(!command_thread->running) command_aborted = true;

	if(!command_aborted && run == 1 && command_type >= COMMAND_CONVERT_UNIT && check_exchange_rates(NULL, show_result)) {
		b_busy = true;
		mfactor->set(*mstruct);
		parsebak.set(*parsed_mstruct);
		run = 2;
		goto rerun_command;
	}

	b_busy = false;
	b_busy_command = false;

	if(was_busy) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(title_set) update_window_title();
		hide_expression_spinner();
		if(!minimal_mode) gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		else gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_unmark_busy(g_application_get_default());
	}

	if(command_type == COMMAND_CONVERT_STRING && !ceu_str.empty()) {
		if(ceu_str[0] == '?') {
			to_prefix = 1;
		} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
			to_prefix = ceu_str[0];
		}
	}

	if(!command_aborted) {
		mstruct->set(*mfactor);
		mfactor->unref();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				printops.allow_factorization = true;
				break;
			}
			case COMMAND_EXPAND: {
				printops.allow_factorization = false;
				break;
			}
			default: {
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
			}
		}
		if(show_result) {
			setResult(NULL, true, !parsed_mstruct->equals(parsebak, true, true), true, command_type == COMMAND_TRANSFORM ? ceu_str : "");
			surface_result_bak = NULL;
		}
	}

	if(!surface_result && surface_result_bak) {
		surface_result = surface_result_bak;
		gtk_widget_queue_draw(resultview);
	}

	unblock_error();

}

void fetch_exchange_rates(int timeout, int n) {
	bool b_busy_bak = b_busy;
	block_error();
	b_busy = true;
	FetchExchangeRatesThread fetch_thread;
	if(fetch_thread.start() && fetch_thread.write(timeout) && fetch_thread.write(n)) {
		int i = 0;
		while(fetch_thread.running && i < 50) {
			while(gtk_events_pending()) gtk_main_iteration();
			sleep_ms(10);
			i++;
		}
		if(fetch_thread.running) {
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL), GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, _("Fetching exchange rates."));
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
			gtk_widget_show(dialog);
			while(fetch_thread.running) {
				while(gtk_events_pending()) gtk_main_iteration();
				sleep_ms(10);
			}
			gtk_widget_destroy(dialog);
		}
	}
	b_busy = b_busy_bak;
	unblock_error();
}

void FetchExchangeRatesThread::run() {
	int timeout = 15;
	int n = -1;
	if(!read(&timeout)) return;
	if(!read(&n)) return;
	CALCULATOR->fetchExchangeRates(timeout, n);
}

void update_message_print_options() {
	PrintOptions message_printoptions = printops;
	message_printoptions.is_approximate = NULL;
	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.show_ending_zeroes = false;
	message_printoptions.base = 10;
	if(printops.min_exp < -10 || printops.min_exp > 10 || ((printops.min_exp == EXP_PRECISION || printops.min_exp == EXP_NONE) && PRECISION > 10)) message_printoptions.min_exp = 10;
	else if(printops.min_exp == EXP_NONE) message_printoptions.min_exp = EXP_PRECISION;
	if(PRECISION > 10) {
		message_printoptions.use_max_decimals = true;
		message_printoptions.max_decimals = 10;
	}
	CALCULATOR->setMessagePrintOptions(message_printoptions);
}

void result_display_updated() {
	if(result_blocked()) return;
	displayed_printops.use_unicode_signs = printops.use_unicode_signs;
	displayed_printops.spell_out_logical_operators = printops.spell_out_logical_operators;
	displayed_printops.multiplication_sign = printops.multiplication_sign;
	displayed_printops.division_sign = printops.division_sign;
	date_map.clear();
	number_map.clear();
	number_base_map.clear();
	number_exp_map.clear();
	number_exp_minus_map.clear();
	number_approx_map.clear();
	gtk_widget_queue_draw(resultview);
	update_message_print_options();
	update_status_text();
	set_expression_output_updated(true);
	display_parse_status();
}
void result_format_updated() {
	if(result_blocked()) return;
	update_message_print_options();
	if(result_autocalculated) print_auto_calc();
	else setResult(NULL, true, false, false);
	update_status_text();
	set_expression_output_updated(true);
	display_parse_status();
}
void result_action_executed() {
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	setResult(NULL, true, false, true);
}
bool contains_prefix(const MathStructure &m) {
	if(m.isUnit() && (m.prefix() || m.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_prefix(m[i])) return true;
	}
	return false;
}
void result_prefix_changed(Prefix *prefix) {
	if((!expression_modified() || rpn_mode) && !displayed_mstruct) {
		return;
	}
	to_prefix = 0;
	bool b_use_unit_prefixes = printops.use_unit_prefixes;
	bool b_use_prefixes_for_all_units = printops.use_prefixes_for_all_units;
	if(contains_prefix(*mstruct)) {
		mstruct->unformat(evalops);
		executeCommand(COMMAND_CALCULATE, false);
	}
	if(!prefix) {
		//mstruct->unformat(evalops);
		printops.use_unit_prefixes = true;
		printops.use_prefixes_for_all_units = true;
	}
	if(result_autocalculated) {
		printops.prefix = prefix;
		print_auto_calc();
		printops.prefix = NULL;
	} else {
		setResult(prefix, true, false, true);
	}
	printops.use_unit_prefixes = b_use_unit_prefixes;
	printops.use_prefixes_for_all_units = b_use_prefixes_for_all_units;

}
void expression_calculation_updated() {
	set_expression_output_updated(true);
	display_parse_status();
	update_message_print_options();
	if(!rpn_mode) {
		if(parsed_mstruct) {
			for(size_t i = 0; i < 5; i++) {
				if(parsed_mstruct->contains(vans[i])) return;
			}
		}
		if(auto_calculate) do_auto_calc();
		else if(expression_modified() && (visible_keypad & PROGRAMMING_KEYPAD)) autocalc_result_bases();
		else execute_expression(false);
	}
	update_status_text();
}
void expression_format_updated(bool recalculate) {
	set_expression_output_updated(true);
	if(rpn_mode) recalculate = false;
	if(!parsed_in_result || rpn_mode) display_parse_status();
	update_message_print_options();
	if(!expression_modified() && !recalculate && !rpn_mode && !auto_calculate) {
		clearresult();
	} else if(!rpn_mode && parsed_mstruct) {
		for(size_t i = 0; i < 5; i++) {
			if(parsed_mstruct->contains(vans[i])) clearresult();
		}
	}
	if(!rpn_mode) {
		if(auto_calculate) do_auto_calc();
		else if((!recalculate || expression_modified()) && (visible_keypad & PROGRAMMING_KEYPAD)) autocalc_result_bases();
		else if(recalculate) execute_expression(false);
		if(!recalculate && !rpn_mode && parsed_in_result) display_parse_status();
	}
	update_status_text();
}

void abort_calculation() {
	if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
	else if(b_busy_result) on_abort_display(NULL, 0, NULL);
	else if(b_busy_command) on_abort_command(NULL, 0, NULL);
}
void on_abort_calculation(GtkDialog*, gint, gpointer) {
	CALCULATOR->abort();
}

int s2b(const string &str) {
	if(str.empty()) return -1;
	if(equalsIgnoreCase(str, "yes")) return 1;
	if(equalsIgnoreCase(str, "no")) return 0;
	if(equalsIgnoreCase(str, "true")) return 1;
	if(equalsIgnoreCase(str, "false")) return 0;
	if(equalsIgnoreCase(str, "on")) return 1;
	if(equalsIgnoreCase(str, "off")) return 0;
	if(str.find_first_not_of(SPACES NUMBERS) != string::npos) return -1;
	int i = s2i(str);
	if(i > 0) return 1;
	return 0;
}

#define SET_BOOL_MENU(x)	{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, x)), v);}
#define SET_BOOL_D(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; result_display_updated();}}
#define SET_BOOL_F(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; result_format_updated();}}
#define SET_BOOL_PREF(x)	{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else {preferences_dialog_set(x, v);}}
#define SET_BOOL_E(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expression_calculation_updated();}}
#define SET_BOOL_PF(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expression_format_updated(false);}}
#define SET_BOOL(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v;}}

void set_assumption(const string &str, AssumptionType &at, AssumptionSign &as, bool last_of_two = false) {
	if(equalsIgnoreCase(str, "none") || str == "0") {
		as = ASSUMPTION_SIGN_UNKNOWN;
		at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "unknown")) {
		if(!last_of_two) as = ASSUMPTION_SIGN_UNKNOWN;
		else at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "real")) {
		at = ASSUMPTION_TYPE_REAL;
	} else if(equalsIgnoreCase(str, "number") || equalsIgnoreCase(str, "complex") || str == "num" || str == "cplx") {
		at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "rational") || str == "rat") {
		at = ASSUMPTION_TYPE_RATIONAL;
	} else if(equalsIgnoreCase(str, "integer") || str == "int") {
		at = ASSUMPTION_TYPE_INTEGER;
	} else if(equalsIgnoreCase(str, "boolean") || str == "bool") {
		at = ASSUMPTION_TYPE_BOOLEAN;
	} else if(equalsIgnoreCase(str, "non-zero") || str == "nz") {
		as = ASSUMPTION_SIGN_NONZERO;
	} else if(equalsIgnoreCase(str, "positive") || str == "pos") {
		as = ASSUMPTION_SIGN_POSITIVE;
	} else if(equalsIgnoreCase(str, "non-negative") || str == "nneg") {
		as = ASSUMPTION_SIGN_NONNEGATIVE;
	} else if(equalsIgnoreCase(str, "negative") || str == "neg") {
		as = ASSUMPTION_SIGN_NEGATIVE;
	} else if(equalsIgnoreCase(str, "non-positive") || str == "npos") {
		as = ASSUMPTION_SIGN_NONPOSITIVE;
	} else {
		CALCULATOR->error(true, "Unrecognized assumption: %s.", str.c_str(), NULL);
	}
}

void set_option(string str) {
	remove_blank_ends(str);
	gsub(SIGN_MINUS, "-", str);
	string svalue, svar;
	bool empty_value = false;
	size_t i_underscore = str.find("_");
	size_t index;
	if(i_underscore != string::npos) {
		index = str.find_first_of(SPACES);
		if(index != string::npos && i_underscore > index) i_underscore = string::npos;
	}
	if(i_underscore == string::npos) index = str.find_last_of(SPACES);
	if(index != string::npos) {
		svar = str.substr(0, index);
		remove_blank_ends(svar);
		svalue = str.substr(index + 1);
		remove_blank_ends(svalue);
	} else {
		svar = str;
	}
	if(i_underscore != string::npos) gsub("_", " ", svar);
	if(svalue.empty()) {
		empty_value = true;
		svalue = "1";
	}

	set_option_place:
	if(equalsIgnoreCase(svar, "base") || equalsIgnoreCase(svar, "input base") || svar == "inbase" || equalsIgnoreCase(svar, "output base") || svar == "outbase") {
		int v = 0;
		bool b_in = equalsIgnoreCase(svar, "input base") || svar == "inbase";
		bool b_out = equalsIgnoreCase(svar, "output base") || svar == "outbase";
		if(equalsIgnoreCase(svalue, "roman")) v = BASE_ROMAN_NUMERALS;
		else if(equalsIgnoreCase(svalue, "bijective") || svalue == "b26" || svalue == "B26") v = BASE_BIJECTIVE_26;
		else if(equalsIgnoreCase(svalue, "bcd")) v = BASE_BINARY_DECIMAL;
		else if(equalsIgnoreCase(svalue, "fp32") || equalsIgnoreCase(svalue, "binary32") || equalsIgnoreCase(svalue, "float")) {if(b_in) v = 0; else v = BASE_FP32;}
		else if(equalsIgnoreCase(svalue, "fp64") || equalsIgnoreCase(svalue, "binary64") || equalsIgnoreCase(svalue, "double")) {if(b_in) v = 0; else v = BASE_FP64;}
		else if(equalsIgnoreCase(svalue, "fp16") || equalsIgnoreCase(svalue, "binary16")) {if(b_in) v = 0; else v = BASE_FP16;}
		else if(equalsIgnoreCase(svalue, "fp80")) {if(b_in) v = 0; else v = BASE_FP80;}
		else if(equalsIgnoreCase(svalue, "fp128") || equalsIgnoreCase(svalue, "binary128")) {if(b_in) v = 0; else v = BASE_FP128;}
		else if(equalsIgnoreCase(svalue, "time")) {if(b_in) v = 0; else v = BASE_TIME;}
		else if(equalsIgnoreCase(svalue, "hex") || equalsIgnoreCase(svalue, "hexadecimal")) v = BASE_HEXADECIMAL;
		else if(equalsIgnoreCase(svalue, "golden") || equalsIgnoreCase(svalue, "golden ratio") || svalue == "φ") v = BASE_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "supergolden") || equalsIgnoreCase(svalue, "supergolden ratio") || svalue == "ψ") v = BASE_SUPER_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "pi") || svalue == "π") v = BASE_PI;
		else if(svalue == "e") v = BASE_E;
		else if(svalue == "sqrt(2)" || svalue == "sqrt 2" || svalue == "sqrt2" || svalue == "√2") v = BASE_SQRT2;
		else if(equalsIgnoreCase(svalue, "unicode")) v = BASE_UNICODE;
		else if(equalsIgnoreCase(svalue, "duo") || equalsIgnoreCase(svalue, "duodecimal")) v = 12;
		else if(equalsIgnoreCase(svalue, "bin") || equalsIgnoreCase(svalue, "binary")) v = BASE_BINARY;
		else if(equalsIgnoreCase(svalue, "oct") || equalsIgnoreCase(svalue, "octal")) v = BASE_OCTAL;
		else if(equalsIgnoreCase(svalue, "dec") || equalsIgnoreCase(svalue, "decimal")) v = BASE_DECIMAL;
		else if(equalsIgnoreCase(svalue, "sexa") || equalsIgnoreCase(svalue, "sexagesimal")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL;}
		else if(equalsIgnoreCase(svalue, "sexa2") || equalsIgnoreCase(svalue, "sexagesimal2")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_2;}
		else if(equalsIgnoreCase(svalue, "sexa3") || equalsIgnoreCase(svalue, "sexagesimal3")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_3;}
		else if(equalsIgnoreCase(svalue, "latitude")) {if(b_in) v = 0; else v = BASE_LATITUDE;}
		else if(equalsIgnoreCase(svalue, "latitude2")) {if(b_in) v = 0; else v = BASE_LATITUDE_2;}
		else if(equalsIgnoreCase(svalue, "longitude")) {if(b_in) v = 0; else v = BASE_LONGITUDE;}
		else if(equalsIgnoreCase(svalue, "longitude2")) {if(b_in) v = 0; else v = BASE_LONGITUDE_2;}
		else if(!b_in && !b_out && (index = svalue.find_first_of(SPACES)) != string::npos) {
			str = svalue;
			svalue = str.substr(index + 1, str.length() - (index + 1));
			remove_blank_ends(svalue);
			svar += " ";
			str = str.substr(0, index);
			remove_blank_ends(str);
			svar += str;
			gsub("_", " ", svar);
			if(equalsIgnoreCase(svar, "base display")) {
				goto set_option_place;
			}
			set_option(string("inbase ") + svalue);
			set_option(string("outbase ") + str);
			return;
		} else if(!empty_value) {
			MathStructure m;
			EvaluationOptions eo = evalops;
			eo.parse_options.base = 10;
			eo.approximation = APPROXIMATION_TRY_EXACT;
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(svalue, eo.parse_options), 500, eo);
			if(CALCULATOR->endTemporaryStopMessages()) {
				v = 0;
			} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
				v = m.number().intValue();
			} else if(m.isNumber() && (b_in || ((!m.number().isNegative() || m.number().isInteger()) && (m.number() > 1 || m.number() < -1)))) {
				v = BASE_CUSTOM;
				if(b_in) CALCULATOR->setCustomInputBase(m.number());
				else CALCULATOR->setCustomOutputBase(m.number());
			}
		}
		if(v == 0) {
			CALCULATOR->error(true, "Illegal base: %s.", svalue.c_str(), NULL);
		} else if(b_in) {
			if(v == BASE_CUSTOM || v != evalops.parse_options.base) {
				evalops.parse_options.base = v;
				update_setbase();
				update_keypad_programming_base();
				expression_format_updated(false);
				history_input_base_changed();
			}
		} else {
			if(v == BASE_CUSTOM || v != printops.base) {
				printops.base = v;
				to_base = 0;
				to_bits = 0;
				update_menu_base();
				update_setbase();
				update_keypad_programming_base();
				result_format_updated();
			}
		}
	} else if(equalsIgnoreCase(svar, "assumptions") || svar == "ass" || svar == "asm") {
		size_t i = svalue.find_first_of(SPACES);
		AssumptionType at = CALCULATOR->defaultAssumptions()->type();
		AssumptionSign as = CALCULATOR->defaultAssumptions()->sign();
		if(i != string::npos) {
			set_assumption(svalue.substr(0, i), at, as, false);
			set_assumption(svalue.substr(i + 1, svalue.length() - (i + 1)), at, as, true);
		} else {
			set_assumption(svalue, at, as, false);
		}
		set_assumptions_items(at, as);
	} else if(equalsIgnoreCase(svar, "all prefixes") || svar == "allpref") SET_BOOL_MENU("menu_item_all_prefixes")
	else if(equalsIgnoreCase(svar, "complex numbers") || svar == "cplx") SET_BOOL_MENU("menu_item_allow_complex")
	else if(equalsIgnoreCase(svar, "excessive parentheses") || svar == "expar") SET_BOOL_D(printops.excessive_parenthesis)
	else if(equalsIgnoreCase(svar, "functions") || svar == "func") SET_BOOL_MENU("menu_item_enable_functions")
	else if(equalsIgnoreCase(svar, "infinite numbers") || svar == "inf") SET_BOOL_MENU("menu_item_allow_infinite")
	else if(equalsIgnoreCase(svar, "show negative exponents") || svar == "negexp") SET_BOOL_MENU("menu_item_negative_exponents")
	else if(equalsIgnoreCase(svar, "minus last") || svar == "minlast") SET_BOOL_MENU("menu_item_sort_minus_last")
	else if(equalsIgnoreCase(svar, "assume nonzero denominators") || svar == "nzd") SET_BOOL_MENU("menu_item_assume_nonzero_denominators")
	else if(equalsIgnoreCase(svar, "warn nonzero denominators") || svar == "warnnzd") SET_BOOL_MENU("menu_item_warn_about_denominators_assumed_nonzero")
	else if(equalsIgnoreCase(svar, "prefixes") || svar == "pref") SET_BOOL_MENU("menu_item_prefixes_for_selected_units")
	else if(equalsIgnoreCase(svar, "binary prefixes") || svar == "binpref") SET_BOOL_PREF("preferences_checkbutton_binary_prefixes")
	else if(equalsIgnoreCase(svar, "denominator prefixes") || svar == "denpref") SET_BOOL_MENU("menu_item_denominator_prefixes")
	else if(equalsIgnoreCase(svar, "place units separately") || svar == "unitsep") SET_BOOL_MENU("menu_item_place_units_separately")
	else if(equalsIgnoreCase(svar, "calculate variables") || svar == "calcvar") SET_BOOL_MENU("menu_item_calculate_variables")
	else if(equalsIgnoreCase(svar, "calculate functions") || svar == "calcfunc") SET_BOOL_E(evalops.calculate_functions)
	else if(equalsIgnoreCase(svar, "sync units") || svar == "sync") SET_BOOL_E(evalops.sync_units)
	else if(equalsIgnoreCase(svar, "temperature calculation") || svar == "temp")  {
		int v = -1;
		if(equalsIgnoreCase(svalue, "relative")) v = TEMPERATURE_CALCULATION_RELATIVE;
		else if(equalsIgnoreCase(svalue, "hybrid")) v = TEMPERATURE_CALCULATION_HYBRID;
		else if(equalsIgnoreCase(svalue, "absolute")) v = TEMPERATURE_CALCULATION_ABSOLUTE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v != CALCULATOR->getTemperatureCalculationMode()) {
			CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) v);
			preferences_update_temperature_calculation();
			tc_set = true;
			expression_calculation_updated();
		}
	} else if(svar == "sinc")  {
		int v = -1;
		if(equalsIgnoreCase(svalue, "unnormalized")) v = 0;
		else if(equalsIgnoreCase(svalue, "normalized")) v = 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == 0) CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "");
			else CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "pi");
			sinc_set = true;
			expression_calculation_updated();
		}
	} else if(equalsIgnoreCase(svar, "round to even") || svar == "rndeven") {
		bool b = printops.round_halfway_to_even;
		SET_BOOL(b)
		if(b != (printops.rounding == ROUNDING_HALF_TO_EVEN)) {
			if(b) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_to_even")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rounding_half_away_from_zero")), TRUE);
		}
	} else if(equalsIgnoreCase(svar, "rounding")) {
		int v = -1;
		if(equalsIgnoreCase(svalue, "even") || equalsIgnoreCase(svalue, "round to even") || equalsIgnoreCase(svalue, "half to even")) v = ROUNDING_HALF_TO_EVEN;
		else if(equalsIgnoreCase(svalue, "standard") || equalsIgnoreCase(svalue, "half away from zero")) v = ROUNDING_HALF_AWAY_FROM_ZERO;
		else if(equalsIgnoreCase(svalue, "truncate") || equalsIgnoreCase(svalue, "toward zero")) v = ROUNDING_TOWARD_ZERO;
		else if(equalsIgnoreCase(svalue, "half to odd")) v = ROUNDING_HALF_TO_ODD;
		else if(equalsIgnoreCase(svalue, "half toward zero")) v = ROUNDING_HALF_TOWARD_ZERO;
		else if(equalsIgnoreCase(svalue, "half random")) v = ROUNDING_HALF_RANDOM;
		else if(equalsIgnoreCase(svalue, "half up")) v = ROUNDING_HALF_UP;
		else if(equalsIgnoreCase(svalue, "half down")) v = ROUNDING_HALF_DOWN;
		else if(equalsIgnoreCase(svalue, "up")) v = ROUNDING_UP;
		else if(equalsIgnoreCase(svalue, "down")) v = ROUNDING_DOWN;
		else if(equalsIgnoreCase(svalue, "away from zero")) v = ROUNDING_AWAY_FROM_ZERO;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
			if(v == 2) v = ROUNDING_TOWARD_ZERO;
			else if(v > 2 && v <= ROUNDING_TOWARD_ZERO) v--;
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v != printops.rounding) {
			switch(v) {
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
		}
	} else if(equalsIgnoreCase(svar, "rpn syntax") || svar == "rpnsyn") {
		bool b = (evalops.parse_options.parsing_mode == PARSING_MODE_RPN);
		SET_BOOL(b)
		if(b != (evalops.parse_options.parsing_mode == PARSING_MODE_RPN)) {
			if(b) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
		}
	} else if(equalsIgnoreCase(svar, "rpn") && svalue.find(" ") == string::npos) SET_BOOL_MENU("menu_item_rpn_mode")
	else if(equalsIgnoreCase(svar, "simplified percentage") || svar == "percent") SET_BOOL_MENU("menu_item_simplified_percentage")
	else if(equalsIgnoreCase(svar, "short multiplication") || svar == "shortmul") SET_BOOL_D(printops.short_multiplication)
	else if(equalsIgnoreCase(svar, "lowercase e") || svar == "lowe") {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			block_result();
			preferences_dialog_set("preferences_checkbutton_e_notation", TRUE);
			unblock_result();
			preferences_dialog_set("preferences_checkbutton_lower_case_e", v);
		}
	} else if(equalsIgnoreCase(svar, "lowercase numbers") || svar == "lownum") SET_BOOL_PREF("preferences_checkbutton_lower_case_numbers")
	else if(equalsIgnoreCase(svar, "duodecimal symbols") || svar == "duosyms") SET_BOOL_PREF("preferences_checkbutton_duodecimal_symbols")
	else if(equalsIgnoreCase(svar, "imaginary j") || svar == "imgj") SET_BOOL_PREF("preferences_checkbutton_imaginary_j")
	else if(equalsIgnoreCase(svar, "base display") || svar == "basedisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = BASE_DISPLAY_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "normal")) v = BASE_DISPLAY_NORMAL;
		else if(equalsIgnoreCase(svalue, "alternative")) v = BASE_DISPLAY_ALTERNATIVE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			preferences_dialog_set("preferences_checkbutton_alternative_base_prefixes", v == BASE_DISPLAY_ALTERNATIVE);
		}
	} else if(equalsIgnoreCase(svar, "two's complement") || svar == "twos") SET_BOOL_PREF("preferences_checkbutton_twos_complement")
	else if(equalsIgnoreCase(svar, "hexadecimal two's") || svar == "hextwos") SET_BOOL_PREF("preferences_checkbutton_hexadecimal_twos_complement")
	else if(equalsIgnoreCase(svar, "two's complement input") || svar == "twosin") SET_BOOL_PREF("preferences_checkbutton_twos_complement_input")
	else if(equalsIgnoreCase(svar, "hexadecimal two's input") || svar == "hextwosin") SET_BOOL_PREF("preferences_checkbutton_hexadecimal_twos_complement_input")
	else if(equalsIgnoreCase(svar, "binary bits") || svar == "bits") {
		int v = -1;
		if(empty_value) {
			v = 0;
		} else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v < 0) v = 0;
		}
		if(v < 0 || v == 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			printops.binary_bits = v;
			evalops.parse_options.binary_bits = v;
			default_bits = -1;
			update_keypad_programming_base();
			preferences_update_twos_complement();
			if(evalops.parse_options.twos_complement || evalops.parse_options.hexadecimal_twos_complement) expression_format_updated(true);
			else result_format_updated();
		}
	} else if(equalsIgnoreCase(svar, "digit grouping") || svar =="group") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DIGIT_GROUPING_NONE;
		else if(equalsIgnoreCase(svalue, "none")) v = DIGIT_GROUPING_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "standard") || equalsIgnoreCase(svalue, "on")) v = DIGIT_GROUPING_STANDARD;
		else if(equalsIgnoreCase(svalue, "locale")) v = DIGIT_GROUPING_LOCALE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < DIGIT_GROUPING_NONE || v > DIGIT_GROUPING_LOCALE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == DIGIT_GROUPING_NONE) preferences_dialog_set("preferences_radiobutton_digit_grouping_none", TRUE);
			else if(v == DIGIT_GROUPING_STANDARD) preferences_dialog_set("preferences_radiobutton_digit_grouping_standard", TRUE);
			else if(v == DIGIT_GROUPING_LOCALE) preferences_dialog_set("preferences_radiobutton_digit_grouping_locale", TRUE);
		}
	} else if(equalsIgnoreCase(svar, "spell out logical") || svar == "spellout") SET_BOOL_PREF("preferences_checkbutton_spell_out_logical_operators")
	else if((equalsIgnoreCase(svar, "ignore dot") || svar == "nodot") && CALCULATOR->getDecimalPoint() != DOT) SET_BOOL_PREF("preferences_checkbutton_dot_as_separator")
	else if((equalsIgnoreCase(svar, "ignore comma") || svar == "nocomma") && CALCULATOR->getDecimalPoint() != COMMA) SET_BOOL_PREF("preferences_checkbutton_comma_as_separator")
	else if(equalsIgnoreCase(svar, "decimal comma")) {
		int v = -2;
		if(equalsIgnoreCase(svalue, "off")) v = 0;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = 1;
		else if(equalsIgnoreCase(svalue, "locale")) v = -1;
		else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < -1 || v > 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v >= 0) preferences_dialog_set("preferences_checkbutton_decimal_comma", v);
			else b_decimal_comma = v;
		}
	} else if(equalsIgnoreCase(svar, "limit implicit multiplication") || svar == "limimpl") SET_BOOL_MENU("menu_item_limit_implicit_multiplication")
	else if(equalsIgnoreCase(svar, "spacious") || svar == "space") SET_BOOL_D(printops.spacious)
	else if(equalsIgnoreCase(svar, "unicode") || svar == "uni") SET_BOOL_PREF("preferences_checkbutton_unicode_signs")
	else if(equalsIgnoreCase(svar, "units") || svar == "unit") SET_BOOL_MENU("menu_item_enable_units")
	else if(equalsIgnoreCase(svar, "unknowns") || svar == "unknown") SET_BOOL_MENU("menu_item_enable_unknown_variables")
	else if(equalsIgnoreCase(svar, "variables") || svar == "var") SET_BOOL_MENU("menu_item_enable_variables")
	else if(equalsIgnoreCase(svar, "abbreviations") || svar == "abbr" || svar == "abbrev") SET_BOOL_MENU("menu_item_abbreviate_names")
	else if(equalsIgnoreCase(svar, "show ending zeroes") || svar == "zeroes") SET_BOOL_MENU("menu_item_show_ending_zeroes")
	else if(equalsIgnoreCase(svar, "repeating decimals") || svar == "repdeci") SET_BOOL_MENU("menu_item_indicate_infinite_series")
	else if(equalsIgnoreCase(svar, "angle unit") || svar == "angle") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rad") || equalsIgnoreCase(svalue, "radians")) v = ANGLE_UNIT_RADIANS;
		else if(equalsIgnoreCase(svalue, "deg") || equalsIgnoreCase(svalue, "degrees")) v = ANGLE_UNIT_DEGREES;
		else if(equalsIgnoreCase(svalue, "gra") || equalsIgnoreCase(svalue, "gradians")) v = ANGLE_UNIT_GRADIANS;
		else if(equalsIgnoreCase(svalue, "none")) v = ANGLE_UNIT_NONE;
		else if(equalsIgnoreCase(svalue, "custom")) v = ANGLE_UNIT_CUSTOM;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		} else {
			Unit *u = CALCULATOR->getActiveUnit(svalue);
			if(u && u->baseUnit() == CALCULATOR->getRadUnit() && u->baseExponent() == 1 && u->isActive() && u->isRegistered() && !u->isHidden()) {
				if(u == CALCULATOR->getRadUnit()) v = ANGLE_UNIT_RADIANS;
				else if(u == CALCULATOR->getGraUnit()) v = ANGLE_UNIT_GRADIANS;
				else if(u == CALCULATOR->getDegUnit()) v = ANGLE_UNIT_DEGREES;
				else {v = ANGLE_UNIT_CUSTOM; CALCULATOR->setCustomAngleUnit(u);}
			}
		}
		if(v < 0 || v > 4) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()) {
			CALCULATOR->error(true, "Please specify a custom angle unit as argument (e.g. set angle arcsec).", NULL);
		} else {
			menu_set_angle_unit(v);
		}
	} else if(equalsIgnoreCase(svar, "caret as xor") || equalsIgnoreCase(svar, "xor^")) SET_BOOL_PREF("preferences_checkbutton_caret_as_xor")
	else if(equalsIgnoreCase(svar, "concise uncertainty") || equalsIgnoreCase(svar, "concise")) SET_BOOL_MENU("menu_item_concise_uncertainty_input")
	else if(equalsIgnoreCase(svar, "parsing mode") || svar == "parse" || svar == "syntax") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = PARSING_MODE_ADAPTIVE;
		else if(equalsIgnoreCase(svalue, "implicit first")) v = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
		else if(equalsIgnoreCase(svalue, "conventional")) v = PARSING_MODE_CONVENTIONAL;
		else if(equalsIgnoreCase(svalue, "chain")) v = PARSING_MODE_CHAIN;
		else if(equalsIgnoreCase(svalue, "rpn")) v = PARSING_MODE_RPN;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < PARSING_MODE_ADAPTIVE || v > PARSING_MODE_RPN) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == PARSING_MODE_ADAPTIVE) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
			else if(v == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
			else if(v == PARSING_MODE_CONVENTIONAL) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
			else if(v == PARSING_MODE_CHAIN) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), TRUE);
			else if(v == PARSING_MODE_RPN) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
		}
	} else if(equalsIgnoreCase(svar, "update exchange rates") || svar == "upxrates") {
		int v = -2;
		if(equalsIgnoreCase(svalue, "never")) {
			v = 0;
		} else if(equalsIgnoreCase(svalue, "ask")) {
			v = -1;
		} else {
			v = s2i(svalue);
		}
		if(v < -1) v = -1;
		auto_update_exchange_rates = v;
		preferences_update_exchange_rates();
	} else if(equalsIgnoreCase(svar, "multiplication sign") || svar == "mulsign") {
		int v = -1;
		if(svalue == SIGN_MULTIDOT || svalue == ".") v = MULTIPLICATION_SIGN_DOT;
		else if(svalue == SIGN_MIDDLEDOT) v = MULTIPLICATION_SIGN_ALTDOT;
		else if(svalue == SIGN_MULTIPLICATION || svalue == "x") v = MULTIPLICATION_SIGN_X;
		else if(svalue == "*") v = MULTIPLICATION_SIGN_ASTERISK;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < MULTIPLICATION_SIGN_ASTERISK || v > MULTIPLICATION_SIGN_ALTDOT) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			switch(v) {
				case MULTIPLICATION_SIGN_DOT: {
					preferences_dialog_set("preferences_radiobutton_dot", TRUE);
					break;
				}
				case MULTIPLICATION_SIGN_ALTDOT: {
					preferences_dialog_set("preferences_radiobutton_altdot", TRUE);
					break;
				}
				case MULTIPLICATION_SIGN_X: {
					preferences_dialog_set("preferences_radiobutton_ex", TRUE);
					break;
				}
				default: {
					preferences_dialog_set("preferences_radiobutton_asterisk", TRUE);
					break;
				}
			}
		}
	} else if(equalsIgnoreCase(svar, "division sign") || svar == "divsign") {
		int v = -1;
		if(svalue == SIGN_DIVISION_SLASH) v = DIVISION_SIGN_DIVISION_SLASH;
		else if(svalue == SIGN_DIVISION) v = DIVISION_SIGN_DIVISION;
		else if(svalue == "/") v = DIVISION_SIGN_SLASH;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			switch(v) {
				case DIVISION_SIGN_DIVISION_SLASH: {
					preferences_dialog_set("preferences_radiobutton_division_slash", TRUE);
					break;
				}
				case DIVISION_SIGN_DIVISION: {
					preferences_dialog_set("preferences_radiobutton_division", TRUE);
					break;
				}
				default: {
					preferences_dialog_set("preferences_radiobutton_slash", TRUE);
					break;
				}
			}
		}
	} else if(equalsIgnoreCase(svar, "approximation") || svar == "appr" || svar == "approx") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "exact")) v = APPROXIMATION_EXACT;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(equalsIgnoreCase(svalue, "dual")) v = APPROXIMATION_APPROXIMATE + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "try exact") || svalue == "try") v = APPROXIMATION_TRY_EXACT;
		else if(equalsIgnoreCase(svalue, "approximate") || svalue == "approx") v = APPROXIMATION_APPROXIMATE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v > APPROXIMATION_APPROXIMATE + 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v < APPROXIMATION_EXACT || v > APPROXIMATION_APPROXIMATE) {
			CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
		} else {
			set_approximation((ApproximationMode) v);
		}
	} else if(equalsIgnoreCase(svar, "interval calculation") || svar == "ic" || equalsIgnoreCase(svar, "uncertainty propagation") || svar == "up") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "variance formula") || equalsIgnoreCase(svalue, "variance")) v = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		else if(equalsIgnoreCase(svalue, "interval arithmetic") || svalue == "iv") v = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < INTERVAL_CALCULATION_NONE || v > INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			switch(v) {
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
		}
	} else if(equalsIgnoreCase(svar, "autoconversion") || svar == "conv") {
		int v = -1;
		MixedUnitsConversion muc = MIXED_UNITS_CONVERSION_DEFAULT;
		if(equalsIgnoreCase(svalue, "none")) {v = POST_CONVERSION_NONE;  muc = MIXED_UNITS_CONVERSION_NONE;}
		else if(equalsIgnoreCase(svalue, "best")) v = POST_CONVERSION_OPTIMAL_SI;
		else if(equalsIgnoreCase(svalue, "optimalsi") || svalue == "si") v = POST_CONVERSION_OPTIMAL_SI;
		else if(empty_value || equalsIgnoreCase(svalue, "optimal")) v = POST_CONVERSION_OPTIMAL;
		else if(equalsIgnoreCase(svalue, "base")) v = POST_CONVERSION_BASE;
		else if(equalsIgnoreCase(svalue, "mixed")) v = POST_CONVERSION_OPTIMAL + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
			if(v == 1) v = 3;
			else if(v == 3) v = 1;
		}
		if(v == POST_CONVERSION_OPTIMAL + 1) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_DEFAULT;
		} else if(v == 0) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_NONE;
		}
		if(v < 0 || v > POST_CONVERSION_OPTIMAL) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			switch(v) {
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
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_mixed_units_conversion")), muc != MIXED_UNITS_CONVERSION_NONE);
		}
	} else if(equalsIgnoreCase(svar, "currency conversion") || svar == "curconv") SET_BOOL_PREF("preferences_checkbutton_local_currency_conversion")
	else if(equalsIgnoreCase(svar, "algebra mode") || svar == "alg") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = STRUCTURING_NONE;
		else if(equalsIgnoreCase(svalue, "simplify") || equalsIgnoreCase(svalue, "expand")) v = STRUCTURING_SIMPLIFY;
		else if(equalsIgnoreCase(svalue, "factorize") || svalue == "factor") v = STRUCTURING_FACTORIZE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > STRUCTURING_FACTORIZE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == STRUCTURING_FACTORIZE) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_factorize")), TRUE);
			else if(v == STRUCTURING_SIMPLIFY) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_simplify")), TRUE);
			else  {
				evalops.structuring = (StructuringMode) v;
				printops.allow_factorization = false;
				expression_calculation_updated();
			}
		}
	} else if(equalsIgnoreCase(svar, "exact")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), v > 0);
		}
	} else if(equalsIgnoreCase(svar, "ignore locale")) SET_BOOL_PREF("preferences_checkbutton_ignore_locale")
	else if(equalsIgnoreCase(svar, "save mode")) SET_BOOL_PREF("preferences_checkbutton_mode")
	else if(equalsIgnoreCase(svar, "save definitions") || svar == "save defs") SET_BOOL_PREF("preferences_checkbutton_save_defs")
	else if(equalsIgnoreCase(svar, "scientific notation") || svar == "exp mode" || svar == "exp" || equalsIgnoreCase(svar, "exp display") || svar == "edisp") {
		int v = -1;
		bool display = (svar == "exp display" || svar == "edisp");
		bool valid = true;
		if(!display && equalsIgnoreCase(svalue, "off")) v = EXP_NONE;
		else if(!display && equalsIgnoreCase(svalue, "auto")) v = EXP_PRECISION;
		else if(!display && equalsIgnoreCase(svalue, "pure")) v = EXP_PURE;
		else if(!display && (empty_value || equalsIgnoreCase(svalue, "scientific"))) v = EXP_SCIENTIFIC;
		else if(!display && equalsIgnoreCase(svalue, "engineering")) v = EXP_BASE_3;
		else if(svalue == "E" || (display && empty_value && printops.exp_display == EXP_POWER_OF_10)) {v = EXP_UPPERCASE_E; display = true;}
		else if(svalue == "e") {v = EXP_LOWERCASE_E; display = true;}
		//scientific notation
		else if((display && svalue == "10") || (display && empty_value && printops.exp_display != EXP_POWER_OF_10) || svalue == "pow" || svalue == "pow10" || equalsIgnoreCase(svalue, "power") || equalsIgnoreCase(svalue, "power of 10")) {
			v = EXP_POWER_OF_10;
			display = true;
		} else if(svalue.find_first_not_of(SPACES NUMBERS MINUS) == string::npos) {
			v = s2i(svalue);
			if(display) v++;
		} else {
			valid = false;
		}
		if(display && valid && (v >= EXP_UPPERCASE_E && v <= EXP_POWER_OF_10)) {
			switch(v) {
				case EXP_LOWERCASE_E: {
					block_result();
					preferences_dialog_set("preferences_checkbutton_e_notation", TRUE);
					unblock_result();
					preferences_dialog_set("preferences_checkbutton_lower_case_e", TRUE);
					break;
				}
				case EXP_UPPERCASE_E: {
					block_result();
					preferences_dialog_set("preferences_checkbutton_e_notation", TRUE);
					unblock_result();
					preferences_dialog_set("preferences_checkbutton_lower_case_e", FALSE);
					break;
				}
				case EXP_POWER_OF_10: {
					preferences_dialog_set("preferences_checkbutton_e_notation", FALSE);
					break;
				}
			}
		} else if(!display && valid) {
			set_min_exp(v);
		} else {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		}
	} else if(equalsIgnoreCase(svar, "precision") || svar == "prec") {
		int v = 0;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			CALCULATOR->setPrecision(v);
			previous_precision = 0;
			expression_calculation_updated();
			update_precision();
		}
	} else if(equalsIgnoreCase(svar, "interval display") || svar == "ivdisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = 0;
		else if(equalsIgnoreCase(svalue, "significant")) v = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS + 1;
		else if(equalsIgnoreCase(svalue, "interval")) v = INTERVAL_DISPLAY_INTERVAL + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "plusminus")) v = INTERVAL_DISPLAY_PLUSMINUS + 1;
		else if(equalsIgnoreCase(svalue, "midpoint")) v = INTERVAL_DISPLAY_MIDPOINT + 1;
		else if(equalsIgnoreCase(svalue, "upper")) v = INTERVAL_DISPLAY_UPPER + 1;
		else if(equalsIgnoreCase(svalue, "lower")) v = INTERVAL_DISPLAY_LOWER + 1;
		else if(equalsIgnoreCase(svalue, "concise")) v = INTERVAL_DISPLAY_CONCISE + 1;
		else if(equalsIgnoreCase(svalue, "relative")) v = INTERVAL_DISPLAY_RELATIVE + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v == 0) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_adaptive")), TRUE);
		} else {
			v--;
			if(v < INTERVAL_DISPLAY_SIGNIFICANT_DIGITS || v > INTERVAL_DISPLAY_RELATIVE) {
				CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
			} else {
				switch(v) {
					case INTERVAL_DISPLAY_INTERVAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_interval")), TRUE); break;}
					case INTERVAL_DISPLAY_PLUSMINUS: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_plusminus")), TRUE); break;}
					case INTERVAL_DISPLAY_CONCISE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_concise")), TRUE); break;}
					case INTERVAL_DISPLAY_RELATIVE: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_relative")), TRUE); break;}
					case INTERVAL_DISPLAY_MIDPOINT: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_midpoint")), TRUE); break;}
					case INTERVAL_DISPLAY_LOWER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_lower")), TRUE); break;}
					case INTERVAL_DISPLAY_UPPER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_upper")), TRUE); break;}
				}
			}
		}
	} else if(equalsIgnoreCase(svar, "interval arithmetic") || svar == "ia" || svar == "interval") SET_BOOL_MENU("menu_item_interval_arithmetic")
	else if(equalsIgnoreCase(svar, "variable units") || svar == "varunits") SET_BOOL_MENU("menu_item_enable_variable_units")
	else if(equalsIgnoreCase(svar, "color")) CALCULATOR->error(true, "Unsupported option: %s.", svar.c_str(), NULL);
	else if(equalsIgnoreCase(svar, "max decimals") || svar == "maxdeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v >= 0) printops.max_decimals = v;
		printops.use_max_decimals = v >= 0;
		result_format_updated();
		update_decimals();
	} else if(equalsIgnoreCase(svar, "min decimals") || svar == "mindeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v >= 0) printops.min_decimals = v;
		printops.use_min_decimals = v >= 0;
		result_format_updated();
		update_decimals();
	} else if(equalsIgnoreCase(svar, "fractions") || svar == "fr") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = FRACTION_DECIMAL;
		else if(equalsIgnoreCase(svalue, "exact")) v = FRACTION_DECIMAL_EXACT;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = FRACTION_FRACTIONAL;
		else if(equalsIgnoreCase(svalue, "combined") || equalsIgnoreCase(svalue, "mixed")) v = FRACTION_COMBINED;
		else if(equalsIgnoreCase(svalue, "long")) v = FRACTION_COMBINED_FIXED_DENOMINATOR + 1;
		else if(equalsIgnoreCase(svalue, "dual")) v = FRACTION_COMBINED_FIXED_DENOMINATOR + 2;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
			if(v == FRACTION_COMBINED + 1) v = FRACTION_COMBINED_FIXED_DENOMINATOR + 1;
			else if(v == FRACTION_COMBINED + 2) v = FRACTION_COMBINED_FIXED_DENOMINATOR + 2;
			else if(v == FRACTION_COMBINED_FIXED_DENOMINATOR + 1) v = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
			else if(v == FRACTION_COMBINED_FIXED_DENOMINATOR + 2) v = FRACTION_COMBINED_FIXED_DENOMINATOR;
		} else {
			int tofr = 0;
			long int fden = get_fixed_denominator_gtk(unlocalize_expression(svalue), tofr, true);
			if(fden != 0) {
				if(tofr == 1) v = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
				else v = FRACTION_COMBINED_FIXED_DENOMINATOR;
				if(fden > 0) CALCULATOR->setFixedDenominator(fden);
			}
		}
		if(v > FRACTION_COMBINED_FIXED_DENOMINATOR + 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v < 0 || v > FRACTION_COMBINED_FIXED_DENOMINATOR + 1) {
			CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
		} else {
			int dff = default_fraction_fraction;
			set_fraction_format(v);
			default_fraction_fraction = dff;
		}
	} else if(equalsIgnoreCase(svar, "complex form") || svar == "cplxform") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rectangular") || equalsIgnoreCase(svalue, "cartesian") || svalue == "rect") v = COMPLEX_NUMBER_FORM_RECTANGULAR;
		else if(equalsIgnoreCase(svalue, "exponential") || svalue == "exp") v = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		else if(equalsIgnoreCase(svalue, "polar")) v = COMPLEX_NUMBER_FORM_POLAR;
		else if(equalsIgnoreCase(svalue, "angle") || equalsIgnoreCase(svalue, "phasor")) v = COMPLEX_NUMBER_FORM_CIS + 1;
		else if(svar == "cis") v = COMPLEX_NUMBER_FORM_CIS;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 4) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			switch(v) {
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
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
					break;
				}
				default: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_angle")), TRUE);
				}
			}
		}
	} else if(equalsIgnoreCase(svar, "read precision") || svar == "readprec") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DONT_READ_PRECISION;
		else if(equalsIgnoreCase(svalue, "always")) v = ALWAYS_READ_PRECISION;
		else if(empty_value || equalsIgnoreCase(svalue, "when decimals") || equalsIgnoreCase(svalue, "on")) v = READ_PRECISION_WHEN_DECIMALS;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == ALWAYS_READ_PRECISION) {
				evalops.parse_options.read_precision = (ReadPrecisionMode) v;
				expression_format_updated(true);
			} else {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), v != DONT_READ_PRECISION);
			}
		}
	} else {
		if(i_underscore == string::npos) {
			if(index != string::npos) {
				if((index = svar.find_last_of(SPACES)) != string::npos) {
					svar = svar.substr(0, index);
					remove_blank_ends(svar);
					str = str.substr(index + 1);
					remove_blank_ends(str);
					svalue = str;
					gsub("_", " ", svar);
					goto set_option_place;
				}
			}
			if(!empty_value && !svalue.empty()) {
				svar += " ";
				svar += svalue;
				svalue = "1";
				empty_value = true;
				goto set_option_place;
			}
		}
		CALCULATOR->error(true, "Unrecognized option: %s.", svar.c_str(), NULL);
	}
}

/*
	calculate entered expression and display result
*/
void execute_expression(bool force, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, string execute_str, string str, bool check_exrates) {

	if(calculation_blocked() || exit_in_progress) return;

	string saved_execute_str = execute_str;

	if(b_busy || b_busy_result || b_busy_expression || b_busy_command) return;

	stop_completion_timeout();
	stop_autocalculate_history_timeout();

	b_busy = true;
	b_busy_expression = true;

	bool do_factors = false, do_pfe = false, do_expand = false, do_ceu = execute_str.empty(), do_bases = false, do_calendars = false;
	if(do_stack && !rpn_mode) do_stack = false;
	if(do_stack && do_mathoperation && f && stack_index == 0) do_stack = false;
	if(!do_stack) stack_index = 0;

	if(!mbak_convert.isUndefined() && stack_index == 0) mbak_convert.setUndefined();

	if(execute_str.empty()) {
		to_fraction = 0; to_fixed_fraction = 0; to_prefix = 0; to_base = 0; to_bits = 0; to_nbase.clear(); to_caf = -1;
	}

	if(str.empty() && !do_mathoperation) {
		if(do_stack) {
			str = get_register_text(stack_index + 1);
		} else {
			str = get_expression_text();
			if(!force && (expression_modified() || str.find_first_not_of(SPACES) == string::npos)) {
				b_busy = false;
				b_busy_expression = false;
				return;
			}
			set_expression_modified(false, false, false);
			if(!do_mathoperation && !str.empty()) add_to_expression_history(str);
			if(test_ask_dot(str)) ask_dot();
		}
	}
	block_error();

	string to_str, str_conv;

	if(execute_str.empty()) {
		bool double_tag = false;
		to_str = CALCULATOR->parseComments(str, evalops.parse_options, &double_tag);
		if(!to_str.empty()) {
			if(str.empty()) {
				if(!double_tag && !history_activated()) {
					clear_expression_text();
					CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
					if(!display_errors(mainwindow, 3, true)) update_expression_icons(EXPRESSION_CLEAR);
					unblock_error();
					b_busy = false;
					b_busy_expression = false;
					return;
				}
				execute_str = CALCULATOR->f_message->referenceName();
				execute_str += "(";
				if(to_str.find("\"") == string::npos) {execute_str += "\""; execute_str += to_str; execute_str += "\"";}
				else if(to_str.find("\'") == string::npos) {execute_str += "\'"; execute_str += to_str; execute_str += "\'";}
				else execute_str += to_str;
				execute_str += ")";
			} else {
				CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
			}
		}
		// qalc command
		bool b_command = false;
		if(str[0] == '/' && str.length() > 1) {
			size_t i = str.find_first_not_of(SPACES, 1);
			if(i != string::npos && (signed char) str[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, str[i])) {
				b_command = true;
			}
		}
		if(b_command) {
			str.erase(0, 1);
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			string scom;
			if(ispace == string::npos) {
				scom = "";
			} else {
				scom = str.substr(1, ispace);
			}
			if(equalsIgnoreCase(scom, "convert") || equalsIgnoreCase(scom, "to")) {
				str = string("to") + str.substr(ispace, slen - ispace);
				b_command = false;
			} else if((str.length() > 2 && str[0] == '-' && str[1] == '>') || (str.length() > 3 && str[0] == '\xe2' && ((str[1] == '\x86' && str[2] == '\x92') || (str[1] == '\x9e' && (unsigned char) str[2] >= 148 && (unsigned char) str[3] <= 191)))) {
				b_command = false;
			} else if(str == "M+" || str == "M-" || str == "M−" || str == "MS" || str == "MC") {
				b_command = false;
			}
		}
		if(b_command) {
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			string scom;
			if(ispace == string::npos) {
				scom = "";
			} else {
				scom = str.substr(0, ispace);
			}
			b_busy = false;
			b_busy_expression = false;
			if(equalsIgnoreCase(scom, "set")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				str = str.substr(ispace + 1, slen - (ispace + 1));
				set_option(str);
			} else if(equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(equalsIgnoreCase(str, "mode")) {save_mode(); clear_expression_text();}
				else if(equalsIgnoreCase(str, "definitions")) {save_defs(); clear_expression_text();}
				else {
					string name = str, cat, title;
					if(str[0] == '\"') {
						size_t i = str.find('\"', 1);
						if(i != string::npos) {
							name = str.substr(1, i - 1);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
					} else {
						size_t i = str.find_first_of(SPACES, 1);
						if(i != string::npos) {
							name = str.substr(0, i);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
						bool catset = false;
						if(str.empty()) {
							cat = CALCULATOR->temporaryCategory();
						} else {
							if(str[0] == '\"') {
								size_t i = str.find('\"', 1);
								if(i != string::npos) {
									cat = str.substr(1, i - 1);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							} else {
								size_t i = str.find_first_of(SPACES, 1);
								if(i != string::npos) {
									cat = str.substr(0, i);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							}
							catset = true;
						}
						bool b = true;
						if(!CALCULATOR->variableNameIsValid(name)) {
							CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
							b = false;
						}
						Variable *v = NULL;
						if(b) v = CALCULATOR->getActiveVariable(name, true);
						if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
							CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
							b = false;
						}
						if(b) {
							if(v && v->isLocal() && v->isKnown()) {
								if(catset) v->setCategory(cat);
								if(!title.empty()) v->setTitle(title);
								((KnownVariable*) v)->set(*mstruct);
								if(v->countNames() == 0) {
									ExpressionName ename(name);
									ename.reference = true;
									v->setName(ename, 1);
								} else {
									v->setName(name, 1);
								}
							} else {
								CALCULATOR->addVariable(new KnownVariable(cat, name, *mstruct, title));
							}
							update_vmenu();
							clear_expression_text();
						}
					}
				}
			} else if(equalsIgnoreCase(scom, "variable")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->variableNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				Variable *v = NULL;
				if(b) v = CALCULATOR->getActiveVariable(name, true);
				if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
					CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(v && v->isLocal() && v->isKnown()) {
						((KnownVariable*) v)->set(expr);
						if(v->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							v->setName(ename, 1);
						} else {
							v->setName(name, 1);
						}
					} else {
						CALCULATOR->addVariable(new KnownVariable("", name, expr));
					}
					update_vmenu();
					clear_expression_text();
				}
			} else if(equalsIgnoreCase(scom, "function")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->functionNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				MathFunction *f = CALCULATOR->getActiveFunction(name, true);
				if(b && ((!f && CALCULATOR->functionNameTaken(name)) || (f && (!f->isLocal() || f->subtype() != SUBTYPE_USER_FUNCTION)))) {
					CALCULATOR->error(true, "A function with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(expr.find("\\") == string::npos) {
						gsub("x", "\\x", expr);
						gsub("y", "\\y", expr);
						gsub("z", "\\z", expr);
					}
					if(f && f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) {
						((UserFunction*) f)->setFormula(expr);
						if(f->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							f->setName(ename, 1);
						} else {
							f->setName(name, 1);
						}
					} else {
						CALCULATOR->addFunction(new UserFunction("", name, expr));
					}
					update_fmenu();
					clear_expression_text();
				}
			} else if(equalsIgnoreCase(scom, "keep")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				Variable *v = CALCULATOR->getActiveVariable(str);
				bool b = v && v->isLocal();
				if(b && v->category() == CALCULATOR->temporaryCategory()) {
					v->setCategory("");
					update_fmenu();
					clear_expression_text();
				} else {
					if(str.length() > 2 && str[str.length() - 2] == '(' && str[str.length() - 1] == ')') str = str.substr(0, str.length() - 2);
					MathFunction *f = CALCULATOR->getActiveFunction(str);
					if(f && f->isLocal()) {
						if(f->category() == CALCULATOR->temporaryCategory()) {
							f->setCategory("");
							update_fmenu();
							clear_expression_text();
						}
					} else if(!b) {
						CALCULATOR->error(true, "No user-defined variable or function with the specified name (%s) exist.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(scom, "delete")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				Variable *v = CALCULATOR->getActiveVariable(str);
				if(v && v->isLocal()) {
					v->destroy();
					update_vmenu();
					clear_expression_text();
				} else {
					if(str.length() > 2 && str[str.length() - 2] == '(' && str[str.length() - 1] == ')') str = str.substr(0, str.length() - 2);
					MathFunction *f = CALCULATOR->getActiveFunction(str);
					if(f && f->isLocal()) {
						f->destroy();
						update_fmenu();
						clear_expression_text();
					} else {
						CALCULATOR->error(true, "No user-defined variable or function with the specified name (%s) exist.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(scom, "base")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				set_option(str);
			} else if(equalsIgnoreCase(scom, "assume")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				string str2 = "assumptions ";
				set_option(str2 + str.substr(ispace + 1, slen - (ispace + 1)));
			} else if(equalsIgnoreCase(scom, "rpn")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(equalsIgnoreCase(str, "syntax")) {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), FALSE);
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
				} else if(equalsIgnoreCase(str, "stack")) {
					if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
					}
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), TRUE);
				} else {
					int v = s2b(str);
					if(v < 0) {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					} else if(v) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), TRUE);
					} else {
						if(evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
							gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
						}
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), FALSE);
					}
				}
			} else if(equalsIgnoreCase(str, "exrates")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				update_exchange_rates();
			} else if(equalsIgnoreCase(str, "stack")) {
				gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), TRUE);
			} else if(equalsIgnoreCase(str, "swap")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					stack_view_swap();
				}
			} else if(equalsIgnoreCase(scom, "swap")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					int index1 = 0, index2 = 0;
					str = str.substr(ispace + 1, slen - (ispace + 1));
					string str2 = "";
					remove_blank_ends(str);
					ispace = str.find_first_of(SPACES);
					if(ispace != string::npos) {
						str2 = str.substr(ispace + 1, str.length() - (ispace + 1));
						str = str.substr(0, ispace);
						remove_blank_ends(str2);
						remove_blank_ends(str);
					}
					index1 = s2i(str);
					if(str2.empty()) index2 = 1;
					else index2 = s2i(str2);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index2 < 0) index2 = (int) CALCULATOR->RPNStackSize() + 1 + index2;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize() || (!str2.empty() && (index2 <= 0 || index2 > (int) CALCULATOR->RPNStackSize()))) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else if(index2 != 1 && index1 != 1) {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					} else if(index1 != index2) {
						if(index1 == 1) index1 = index2;
						stack_view_swap(index1);
					}
				}
			} else if(equalsIgnoreCase(scom, "move")) {
				CALCULATOR->error(true, "Unsupported command: %s.", scom.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					stack_view_rotate(false);
				}
			} else if(equalsIgnoreCase(scom, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					if(equalsIgnoreCase(str, "up")) {
						stack_view_rotate(true);
					} else if(equalsIgnoreCase(str, "down")) {
						stack_view_rotate(false);
					} else {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(str, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					stack_view_copy();
				}
			} else if(equalsIgnoreCase(scom, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						stack_view_copy(index1);
					}
				}
			} else if(equalsIgnoreCase(str, "clear stack")) {
				if(CALCULATOR->RPNStackSize() > 0) stack_view_clear();
			} else if(equalsIgnoreCase(str, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					stack_view_pop();
				}
			} else if(equalsIgnoreCase(scom, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						stack_view_pop(index1);
					}
				}
			} else if(equalsIgnoreCase(str, "factor")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				executeCommand(COMMAND_FACTORIZE, true, true);
			} else if(equalsIgnoreCase(str, "partial fraction")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS, true, true);
			} else if(equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				executeCommand(COMMAND_EXPAND, true, true);
			} else if(equalsIgnoreCase(str, "exact")) {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), TRUE);
			} else if(equalsIgnoreCase(str, "approximate") || str == "approx") {
				restore_previous_expression();
				set_expression_modified(false, false, false);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
			} else if(equalsIgnoreCase(str, "mode")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "help") || str == "?") {
				show_help("index.html", mainwindow);
			} else if(equalsIgnoreCase(str, "list")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) {
				str = str.substr(ispace + 1);
				remove_blank_ends(str);
				char list_type = 0;
				if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find")) {
					size_t i = str.find_first_of(SPACES);
					string str1, str2;
					if(i == string::npos) {
						str1 = str;
					} else {
						str1 = str.substr(0, i);
						str2 = str.substr(i + 1);
						remove_blank_ends(str2);
					}
					if(equalsIgnoreCase(str1, "currencies")) list_type = 'c';
					else if(equalsIgnoreCase(str1, "functions")) list_type = 'f';
					else if(equalsIgnoreCase(str1, "variables")) list_type = 'v';
					else if(equalsIgnoreCase(str1, "units")) list_type = 'u';
					else if(equalsIgnoreCase(str1, "prefixes")) list_type = 'p';
					if(list_type == 'c') {
						manage_units(GTK_WINDOW(mainwindow), str2.c_str(), true);
					} else if(list_type == 'f') {
						manage_functions(GTK_WINDOW(mainwindow), str2.c_str());
					} else if(list_type == 'v') {
						manage_variables(GTK_WINDOW(mainwindow), str2.c_str());
					} else if(list_type == 'u') {
						manage_units(GTK_WINDOW(mainwindow), str2.c_str());
					} else if(list_type == 'p') {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					}
				}
				if(list_type == 0) {
					ExpressionItem *item = CALCULATOR->getActiveExpressionItem(str);
					if(item) {
						if(item->type() == TYPE_UNIT) {
							manage_units(GTK_WINDOW(mainwindow), str.c_str());
						} else if(item->type() == TYPE_FUNCTION) {
							manage_functions(GTK_WINDOW(mainwindow), str.c_str());
						} else if(item->type() == TYPE_VARIABLE) {
							manage_variables(GTK_WINDOW(mainwindow), str.c_str());
						}
						clear_expression_text();
					} else {
						CALCULATOR->error(true, "No function, variable, or unit with the specified name (%s) was found.", str.c_str(), NULL);
					}
				} else {
					clear_expression_text();
				}
			} else if(equalsIgnoreCase(str, "clear history")) {
				history_clear();
				clear_expression_history();
			} else if(equalsIgnoreCase(str, "clear")) {
				clear_expression_text();
				focus_keeping_selection();
			} else if(equalsIgnoreCase(str, "quit") || equalsIgnoreCase(str, "exit")) {
				on_gcalc_exit(NULL, NULL, NULL);
				return;
			} else {
				CALCULATOR->error(true, "Unknown command: %s.", str.c_str(), NULL);
			}
			expression_select_all();
			set_history_activated();
			if(!display_errors(mainwindow, 3, true)) update_expression_icons(EXPRESSION_CLEAR);
			unblock_error();
			return;
		}
	}

	if(execute_str.empty()) {
		if(str == "MC") {
			b_busy = false;
			b_busy_expression = false;
			restore_previous_expression();
			set_expression_modified(false, false, false);
			memory_clear();
			setResult(NULL, false, false);
			return;
		} else if(str == "MS") {
			b_busy = false;
			b_busy_expression = false;
			restore_previous_expression();
			set_expression_modified(false, false, false);
			memory_store();
			setResult(NULL, false, false);
			return;
		} else if(str == "M+") {
			b_busy = false;
			b_busy_expression = false;
			restore_previous_expression();
			set_expression_modified(false, false, false);
			memory_add();
			setResult(NULL, false, false);
			return;
		} else if(str == "M-" || str == "M−") {
			b_busy = false;
			b_busy_expression = false;
			restore_previous_expression();
			set_expression_modified(false, false, false);
			memory_subtract();
			setResult(NULL, false, false);
			return;
		}
	}

	ComplexNumberForm cnf_bak = evalops.complex_number_form;
	ComplexNumberForm cnf = evalops.complex_number_form;
	bool delay_complex = false;
	bool b_units_saved = evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = evalops.mixed_units_conversion;

	bool had_to_expression = false;
	string from_str = str;
	bool last_is_space = !from_str.empty() && is_in(SPACES, from_str[from_str.length() - 1]);
	if(execute_str.empty() && CALCULATOR->separateToExpression(from_str, to_str, evalops, true, !do_stack && (!auto_calculate || rpn_mode || parsed_in_result))) {
		remove_duplicate_blanks(to_str);
		had_to_expression = true;
		string str_left;
		string to_str1, to_str2;
		bool do_to = false;
		while(true) {
			if(!from_str.empty()) {
				if(last_is_space) to_str += " ";
				CALCULATOR->separateToExpression(to_str, str_left, evalops, true, false);
				remove_blank_ends(to_str);
			}
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, _("hexadecimal"))) {
				to_base = BASE_HEXADECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, _("octal"))) {
				to_base = BASE_OCTAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "dec") || equalsIgnoreCase(to_str, "decimal") || equalsIgnoreCase(to_str, _("decimal"))) {
				to_base = BASE_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "duo") || equalsIgnoreCase(to_str, "duodecimal") || equalsIgnoreCase(to_str, _("duodecimal"))) {
				to_base = BASE_DUODECIMAL;
				to_duo_syms = false;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "doz") || equalsIgnoreCase(to_str, "dozenal")) {
				to_base = BASE_DUODECIMAL;
				to_duo_syms = true;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, _("binary"))) {
				to_base = BASE_BINARY;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
				to_base = BASE_ROMAN_NUMERALS;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, _("bijective"))) {
				to_base = BASE_BIJECTIVE_26;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bcd")) {
				to_base = BASE_BINARY_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, _("sexagesimal"))) {
				to_base = BASE_SEXAGESIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "2")) {
				to_base = BASE_SEXAGESIMAL_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "3")) {
				to_base = BASE_SEXAGESIMAL_3;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "latitude") || equalsIgnoreCase(to_str, _("latitude"))) {
				to_base = BASE_LATITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", _("latitude"), "2")) {
				to_base = BASE_LATITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "longitude") || equalsIgnoreCase(to_str, _("longitude"))) {
				to_base = BASE_LONGITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", _("longitude"), "2")) {
				to_base = BASE_LONGITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
				to_base = BASE_FP32;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
				to_base = BASE_FP64;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
				to_base = BASE_FP16;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp80")) {
				to_base = BASE_FP80;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
				to_base = BASE_FP128;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
				to_base = BASE_TIME;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "Unicode")) {
				to_base = BASE_UNICODE;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				printops.time_zone = TIME_ZONE_UTC;
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					setResult(NULL, true, false, false); restore_previous_expression();
					printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_BINARY;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_HEXADECIMAL;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
				to_str = to_str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					CALCULATOR->error(true, _("Time zone parsing failed."), NULL);
				}
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = itz;
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					setResult(NULL, true, false, false); restore_previous_expression();
					printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str == "CET") {
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = 60;
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					setResult(NULL, true, false, false); restore_previous_expression();
					printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, _("bases"))) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					restore_previous_expression();
					convert_number_bases(GTK_WINDOW(mainwindow), unhtmlize(result_text).c_str(), evalops.parse_options.base);
					return;
				}
				do_bases = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, _("calendars"))) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					restore_previous_expression();
					on_popup_menu_item_calendarconversion_activate(NULL, NULL);
					return;
				}
				do_calendars = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "rectangular") || equalsIgnoreCase(to_str, "cartesian") || equalsIgnoreCase(to_str, _("rectangular")) || equalsIgnoreCase(to_str, _("cartesian"))) {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_EVAL, true, true);
					restore_previous_expression();
					evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_RECTANGULAR;
			} else if(equalsIgnoreCase(to_str, "exponential") || equalsIgnoreCase(to_str, _("exponential"))) {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_EVAL, true, true);
					restore_previous_expression();
					evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_EXPONENTIAL;
			} else if(equalsIgnoreCase(to_str, "polar") || equalsIgnoreCase(to_str, _("polar"))) {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_EVAL, true, true);
					restore_previous_expression();
					evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_POLAR;
			} else if(to_str == "cis") {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_EVAL, true, true);
					restore_previous_expression();
					evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(equalsIgnoreCase(to_str, "phasor") || equalsIgnoreCase(to_str, _("phasor")) || equalsIgnoreCase(to_str, "angle") || equalsIgnoreCase(to_str, _("angle"))) {
				to_caf = 1;
				do_to = true;
				if(from_str.empty()) {
					evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_EVAL, true, true);
					restore_previous_expression();
					evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, _("optimal"))) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_CONVERT_OPTIMAL, true, true);
					restore_previous_expression();
					return;
				}
				evalops.parse_options.units_enabled = true;
				evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "prefix") || equalsIgnoreCase(to_str, _("prefix"))) {
				evalops.parse_options.units_enabled = true;
				to_prefix = 1;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, _("base"))) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_CONVERT_BASE, true, true);
					restore_previous_expression();
					return;
				}
				evalops.parse_options.units_enabled = true;
				evalops.auto_post_conversion = POST_CONVERSION_BASE;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, _("mixed"))) {
				evalops.parse_options.units_enabled = true;
				evalops.auto_post_conversion = POST_CONVERSION_NONE;
				evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					if(!get_previous_expression().empty()) execute_expression(force, do_mathoperation, op, f, do_stack, stack_index, get_previous_expression());
					restore_previous_expression();
					evalops.auto_post_conversion = save_auto_post_conversion;
					evalops.mixed_units_conversion = save_mixed_units_conversion;
					evalops.parse_options.units_enabled = b_units_saved;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, _("factors")) || equalsIgnoreCase(to_str, "factor")) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_FACTORIZE, true, true);
					restore_previous_expression();
					return;
				}
				do_factors = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction"))) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS, true, true);
					restore_previous_expression();
					return;
				}
				do_pfe = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, _("base"))) {
				base_from_string(to_str2, to_base, to_nbase);
				to_duo_syms = false;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "decimals") || equalsIgnoreCase(to_str, _("decimals"))) {
				to_fixed_fraction = 0;
				to_fraction = 3;
				do_to = true;
			} else {
				do_to = true;
				long int fden = get_fixed_denominator_gtk(unlocalize_expression(to_str), to_fraction);
				if(fden != 0) {
					if(fden < 0) to_fixed_fraction = 0;
					else to_fixed_fraction = fden;
				} else if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					executeCommand(COMMAND_CONVERT_STRING, true, true, CALCULATOR->unlocalizeExpression(to_str, evalops.parse_options));
					restore_previous_expression();
					return;
				} else {
					if(to_str[0] == '?') {
						to_prefix = 1;
					} else if(to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
						to_prefix = to_str[0];

					}
					Unit *u = CALCULATOR->getActiveUnit(to_str);
					if(delay_complex != (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS) && u && u->baseUnit() == CALCULATOR->getRadUnit() && u->baseExponent() == 1) delay_complex = !delay_complex;
					if(!str_conv.empty()) str_conv += " to ";
					str_conv += to_str;
				}
			}
			if(str_left.empty()) break;
			to_str = str_left;
		}
		if(do_to) {
			if(from_str.empty()) {
				b_busy = false;
				b_busy_expression = false;
				setResult(NULL, true, false, false);
				restore_previous_expression();
				return;
			} else {
				execute_str = from_str;
				if(!str_conv.empty()) {
					execute_str += " to ";
					execute_str += str_conv;
				}
			}
		}
	}
	if(!delay_complex || (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS)) {
		evalops.complex_number_form = cnf;
		delay_complex = false;
	} else {
		evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	}
	if(execute_str.empty()) {
		size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
		if(i != string::npos) {
			to_str = str.substr(0, i);
			if(to_str == "factor" || equalsIgnoreCase(to_str, "factorize") || equalsIgnoreCase(to_str, _("factorize"))) {
				execute_str = str.substr(i + 1);
				do_factors = true;
			} else if(equalsIgnoreCase(to_str, "expand") || equalsIgnoreCase(to_str, _("expand"))) {
				execute_str = str.substr(i + 1);
				do_expand = true;
			}
		}
	}

	size_t stack_size = 0;

	if(do_ceu && str_conv.empty() && continuous_conversion && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
		ParseOptions pa = evalops.parse_options; pa.base = 10;
		string ceu_str = CALCULATOR->unlocalizeExpression(current_conversion_expression(), pa);
		remove_blank_ends(ceu_str);
		if(set_missing_prefixes && !ceu_str.empty()) {
			if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
				ceu_str = "?" + ceu_str;
			}
		}
		if(ceu_str.empty()) {
			parsed_tostruct->setUndefined();
		} else {
			if(ceu_str[0] == '?') {
				to_prefix = 1;
			} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
				to_prefix = ceu_str[0];
			}
			parsed_tostruct->set(ceu_str);
		}
	} else {
		parsed_tostruct->setUndefined();
	}
	CALCULATOR->resetExchangeRatesUsed();
	if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	CALCULATOR->setSimplifiedPercentageUsed(false);
	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation && f) {
			CALCULATOR->getRPNRegister(stack_index + 1)->transform(f);
			parsed_mstruct->set(*CALCULATOR->getRPNRegister(stack_index + 1));
			CALCULATOR->calculateRPNRegister(stack_index + 1, 0, evalops);
		} else {
			CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, evalops.parse_options), 0, evalops, parsed_mstruct, parsed_tostruct);
		}
	} else if(rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			update_lastx();
			if(f) CALCULATOR->calculateRPN(f, 0, evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, evalops, parsed_mstruct);
		} else {
			string str2 = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, evalops.parse_options);
			transform_expression_for_equals_save(str2, evalops.parse_options);
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
					case '\\': {
						MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
						if(fdiv) {
							CALCULATOR->calculateRPN(fdiv, 0, evalops, parsed_mstruct);
							break;
						}
					}
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
				} else if(str2 == "//") {
					MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
					if(fdiv) {
						CALCULATOR->calculateRPN(fdiv, 0, evalops, parsed_mstruct);
						do_mathoperation = true;
					}
				}
			} else if(str2.length() == 3) {
				if(str2 == "⊻") {
					CALCULATOR->calculateRPN(OPERATION_BITWISE_XOR, 0, evalops, parsed_mstruct);
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
			if(do_mathoperation) update_lastx();
		}
	} else {
		string str2 = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, evalops.parse_options);
		transform_expression_for_equals_save(str2, evalops.parse_options);
		CALCULATOR->calculate(mstruct, str2, 0, evalops, parsed_mstruct, parsed_tostruct);
		result_autocalculated = false;
	}

	bool title_set = false, was_busy = false;

	int i = 0;
	while(CALCULATOR->busy() && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(CALCULATOR->busy()) {
		if(update_window_title(_("Calculating…"))) title_set = true;
		if(stack_index == 0 && surface_result) {
			cairo_surface_destroy(surface_result);
			surface_result = NULL;
			gtk_widget_queue_draw(resultview);
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		update_expression_icons(stack_index == 0 ? (!minimal_mode ? RESULT_SPINNER : EXPRESSION_SPINNER) : EXPRESSION_STOP);
		if(!minimal_mode) gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		else gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_mark_busy(g_application_get_default());
		was_busy = true;
	}
	while(CALCULATOR->busy()) {
		while(gtk_events_pending()) gtk_main_iteration();
		sleep_ms(100);
	}

	if(was_busy) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyview")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(title_set) update_window_title();
		if(!minimal_mode) gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
		else gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "expressionspinner")));
		g_application_unmark_busy(g_application_get_default());
	}

	b_busy = false;
	b_busy_expression = false;

	if(delay_complex) {
		evalops.complex_number_form = cnf;
		CALCULATOR->startControl(100);
		if(!rpn_mode) {
			if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mstruct->complexToCisForm(evalops);
			else if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mstruct->complexToPolarForm(evalops);
		} else if(!do_stack) {
			MathStructure *mreg = CALCULATOR->getRPNRegister(do_stack ? stack_index + 1 : 1);
			if(mreg) {
				if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mreg->complexToCisForm(evalops);
				else if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mreg->complexToPolarForm(evalops);
			}
		}
		CALCULATOR->stopControl();
	}

	if(rpn_mode && stack_index == 0) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
	}

	if(do_stack && stack_index > 0) {
	} else if(rpn_mode && do_mathoperation) {
		result_text = _("RPN Operation");
	} else {
		result_text = str;
	}
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	if(rpn_mode && stack_index == 0) {
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
		mstruct->set(CALCULATOR->convert(*mstruct, parsed_tostruct->symbol(), evalops, NULL, false, parsed_mstruct));
	}

	// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
	if(!rpn_mode && (!parsed_tostruct || parsed_tostruct->isUndefined()) && execute_str.empty() && !had_to_expression && (evalops.approximation == APPROXIMATION_EXACT || evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || evalops.auto_post_conversion == POST_CONVERSION_NONE) && parsed_mstruct && mstruct && ((parsed_mstruct->isMultiplication() && parsed_mstruct->size() == 2 && (*parsed_mstruct)[0].isNumber() && (*parsed_mstruct)[1].isUnit_exp() && parsed_mstruct->equals(*mstruct)) || (parsed_mstruct->isNegate() && (*parsed_mstruct)[0].isMultiplication() && (*parsed_mstruct)[0].size() == 2 && (*parsed_mstruct)[0][0].isNumber() && (*parsed_mstruct)[0][1].isUnit_exp() && mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1] == (*parsed_mstruct)[0][1] && (*mstruct)[0].isNumber() && (*parsed_mstruct)[0][0].number() == -(*mstruct)[0].number()) || (parsed_mstruct->isUnit_exp() && parsed_mstruct->equals(*mstruct)))) {
		Unit *u = NULL;
		MathStructure *munit = NULL;
		if(mstruct->isMultiplication()) munit = &(*mstruct)[1];
		else munit = mstruct;
		if(munit->isUnit()) u = munit->unit();
		else u = (*munit)[0].unit();
		if(u && u->isCurrency()) {
			if(evalops.local_currency_conversion && CALCULATOR->getLocalCurrency() && u != CALCULATOR->getLocalCurrency()) {
				ApproximationMode abak = evalops.approximation;
				if(evalops.approximation == APPROXIMATION_EXACT) evalops.approximation = APPROXIMATION_TRY_EXACT;
				mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, evalops, true));
				evalops.approximation = abak;
			}
		} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
			MathStructure mbak(*mstruct);
			if(evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || evalops.auto_post_conversion == POST_CONVERSION_NONE) {
				if(munit->isUnit() && u->referenceName() == "oF") {
					u = CALCULATOR->getActiveUnit("oC");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, evalops, true, false, false));
				} else if(munit->isUnit() && u->referenceName() == "oC") {
					u = CALCULATOR->getActiveUnit("oF");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, evalops, true, false, false));
				} else {
					mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, evalops, true));
				}
			}
			if(evalops.approximation == APPROXIMATION_EXACT && ((evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL && evalops.auto_post_conversion != POST_CONVERSION_NONE) || mstruct->equals(mbak))) {
				evalops.approximation = APPROXIMATION_TRY_EXACT;
				if(evalops.auto_post_conversion == POST_CONVERSION_BASE) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, evalops));
				else mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, evalops, true));
				evalops.approximation = APPROXIMATION_EXACT;
			}
		}
	}

	if(!do_mathoperation && ((test_ask_tc(*parsed_mstruct) && ask_tc()) || ((test_ask_sinc(*parsed_mstruct) || test_ask_sinc(*mstruct)) && ask_sinc()) || (test_ask_percent() && ask_percent()) || (check_exrates && check_exchange_rates(NULL, stack_index == 0 && !do_bases && !do_calendars && !do_pfe && !do_factors && !do_expand)))) {
		execute_expression(force, do_mathoperation, op, f, rpn_mode, stack_index, saved_execute_str, str, false);
		evalops.complex_number_form = cnf_bak;
		evalops.auto_post_conversion = save_auto_post_conversion;
		evalops.parse_options.units_enabled = b_units_saved;
		evalops.mixed_units_conversion = save_mixed_units_conversion;
		if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);
		return;
	}

	//update "ans" variables
	if(stack_index == 0) {
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

	if(do_factors || do_pfe || do_expand) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			if(do_factors && (mstruct->isNumber() || mstruct->isVector()) && to_fraction == 0 && to_fixed_fraction == 0) to_fraction = 2;
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false, true);
			mstruct = save_mstruct;
		} else {
			if(do_factors && (mstruct->isNumber() || mstruct->isVector()) && to_fraction == 0 && to_fixed_fraction == 0) to_fraction = 2;
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false, true);
		}
	}

	if(!do_stack) set_previous_expression(execute_str.empty() ? str : execute_str);
	if(!parsed_tostruct->isUndefined() && do_ceu && str_conv.empty() && !mstruct->containsType(STRUCT_UNIT, true)) parsed_tostruct->setUndefined();
	setResult(NULL, true, stack_index == 0, true, "", stack_index);

	if(do_bases) convert_number_bases(GTK_WINDOW(mainwindow), execute_str.c_str(), evalops.parse_options.base);
	if(do_calendars) on_popup_menu_item_calendarconversion_activate(NULL, NULL);
	
	evalops.complex_number_form = cnf_bak;
	evalops.auto_post_conversion = save_auto_post_conversion;
	evalops.parse_options.units_enabled = b_units_saved;
	evalops.mixed_units_conversion = save_mixed_units_conversion;
	if(!simplified_percentage) evalops.parse_options.parsing_mode = (ParsingMode) (evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);

	if(stack_index == 0) {
		update_conversion_view_selection(mstruct);
		focus_expression();
		expression_select_all();
		cursor_has_moved = false;
	}
	unblock_error();

}

void execute_from_file(string command_file) {
	FILE *cfile = fopen(command_file.c_str(), "r");
	if(!cfile) {
		printf(_("Failed to open %s.\n%s"), command_file.c_str(), "");
		return;
	}
	char buffer[10000];
	string str, scom;
	size_t ispace;
	bool rpn_save = rpn_mode;
	bool autocalc_save = auto_calculate;
	auto_calculate = false;
	rpn_mode = false;
	set_previous_expression("");
	if(!undo_blocked() && !expression_is_empty()) add_expression_to_undo();
	gtk_widget_hide(resultview);
	block_undo();
	block_expression_history();
	block_completion();
	while(fgets(buffer, 10000, cfile)) {
		str = buffer;
		remove_blank_ends(str);
		ispace = str.find_first_of(SPACES);
		if(ispace == string::npos) scom = "";
		else scom = str.substr(0, ispace);
		if(equalsIgnoreCase(str, "exrates") || equalsIgnoreCase(str, "stack") || equalsIgnoreCase(str, "swap") || equalsIgnoreCase(str, "rotate") || equalsIgnoreCase(str, "copy") || equalsIgnoreCase(str, "clear stack") || equalsIgnoreCase(str, "exact") || equalsIgnoreCase(str, "approximate") || equalsIgnoreCase(str, "approx") || equalsIgnoreCase(str, "factor") || equalsIgnoreCase(str, "partial fraction") || equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand") || equalsIgnoreCase(str, "mode") || equalsIgnoreCase(str, "help") || equalsIgnoreCase(str, "?") || equalsIgnoreCase(str, "list") || equalsIgnoreCase(str, "exit") || equalsIgnoreCase(str, "quit") || equalsIgnoreCase(str, "clear") || equalsIgnoreCase(str, "clear history") || equalsIgnoreCase(scom, "variable") || equalsIgnoreCase(scom, "function") || equalsIgnoreCase(scom, "set") || equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store") || equalsIgnoreCase(scom, "swap") || equalsIgnoreCase(scom, "delete") || equalsIgnoreCase(scom, "keep") || equalsIgnoreCase(scom, "assume") || equalsIgnoreCase(scom, "base") || equalsIgnoreCase(scom, "rpn") || equalsIgnoreCase(scom, "move") || equalsIgnoreCase(scom, "rotate") || equalsIgnoreCase(scom, "copy") || equalsIgnoreCase(scom, "pop") || equalsIgnoreCase(scom, "convert") || (equalsIgnoreCase(scom, "to") && scom != "to") || equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) str.insert(0, 1, '/');
		if(!str.empty()) execute_expression(true, false, OPERATION_ADD, NULL, false, 0, "", str.c_str(), false);
	}
	clear_expression_text();
	clearresult();
	gtk_widget_show(resultview);
	set_expression_modified(true, false, false);
	if(displayed_mstruct) {
		displayed_mstruct->unref();
		displayed_mstruct = NULL;
	}
	if(parsed_mstruct) parsed_mstruct->clear();
	if(parsed_tostruct) parsed_tostruct->setUndefined();
	if(matrix_mstruct) matrix_mstruct->clear();
	unblock_completion();
	unblock_undo();
	block_expression_history();
	rpn_mode = rpn_save;
	auto_calculate = autocalc_save;
	set_previous_expression("");
	if(mstruct) {
		if(rpn_mode) {
			mstruct->unref();
			mstruct = CALCULATOR->getRPNRegister(1);
			if(!mstruct) mstruct = new MathStructure();
			else mstruct->ref();
		} else {
			mstruct->clear();
		}
	}
	fclose(cfile);
}

bool use_keypad_buttons_for_history() {
	return persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) && gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(historyview))) > 0;
}
bool keypad_is_visible() {return gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad)) && !minimal_mode;}

extern void on_menu_item_chain_mode_activate(GtkMenuItem*, gpointer user_data);
extern void on_menu_item_autocalc_activate(GtkMenuItem*, gpointer user_data);
int scale_n_bak = 0;
void show_parsed(bool b) {
	show_parsed_instead_of_result = b;
	first_draw_of_result = false;
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		do_autocalc_history_timeout(NULL);
	}
	if(show_parsed_instead_of_result) {
		scale_n_bak = scale_n;
		scale_n = 3;
		if(!parsed_in_result) set_expression_output_updated(true);
		display_parse_status();
		if(!parsed_in_result) set_expression_output_updated(false);
	} else {
		scale_n = scale_n_bak;
		display_parse_status();
	}
	gtk_widget_queue_draw(resultview);
}
void set_parsed_in_result(bool b) {
	if(b == parsed_in_result) return;
	if(b) {
		parsed_in_result = true;
	} else {
		parsed_in_result = false;
		CLEAR_PARSED_IN_RESULT
	}
	if(parsed_in_result) {
		if(autocalc_history_timeout_id) {
			g_source_remove(autocalc_history_timeout_id);
			autocalc_history_timeout_id = 0;
		}
		if(expression_modified() || result_autocalculated) {
			clearresult();
			set_expression_output_updated(true);
		} else {
			parsed_in_result = false;
			show_parsed(true);
			parsed_in_result = true;
			return;
		}
	} else if(result_autocalculated) {
		result_autocalculated = false;
		do_auto_calc(2);
	} else if(show_parsed_instead_of_result) {
		show_parsed(false);
	}
	display_parse_status();
	preferences_update_expression_status();
}
void set_rpn_mode(bool b) {
	if(b == rpn_mode) return;
	rpn_mode = b;
	update_expression_icons();
	if(rpn_mode) {
		gtk_widget_show(expander_stack);
		show_history = gtk_expander_get_expanded(GTK_EXPANDER(expander_history));
		show_keypad = !persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad));
		show_convert = gtk_expander_get_expanded(GTK_EXPANDER(expander_convert));
		if(show_stack) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), TRUE);
		}
		set_expression_modified(true, false, false);
		set_expression_output_updated(true);
		expression_history_index = -1;
		if(auto_calculate && result_autocalculated) result_text = "";
		clearresult();
		menu_rpn_mode_changed();
	} else {
		gtk_widget_hide(expander_stack);
		show_stack = gtk_expander_get_expanded(GTK_EXPANDER(expander_stack));
		if(show_stack) {
			if(show_history) gtk_expander_set_expanded(GTK_EXPANDER(expander_history), TRUE);
			else if(show_keypad && !persistent_keypad) gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), TRUE);
			else if(show_convert) gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
			else gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), FALSE);
		}
		CALCULATOR->clearRPNStack();
		RPNStackCleared();
		clearresult();
		menu_rpn_mode_changed();
		prev_autocalc_str = "";
		if(auto_calculate) {
			result_autocalculated = false;
			do_auto_calc(2);
		}
	}
	keypad_rpn_mode_changed();
	preferences_rpn_mode_changed();
	if(enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_equals")), FALSE);
}

void calculateRPN(int op) {
	if(expression_modified()) {
		if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
			execute_expression(true);
		}
	}
	execute_expression(true, true, (MathOperation) op, NULL);
}
void calculateRPN(MathFunction *f) {
	if(expression_modified()) {
		if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
			execute_expression(true);
		}
	}
	execute_expression(true, true, OPERATION_ADD, f);
}


void function_edited(MathFunction *f) {
	if(!f) return;
	if(!f->isActive()) {
		selected_function_category = _("Inactive");
	} else if(f->isLocal()) {
		selected_function_category = _("User functions");
	} else if(f->category().empty()) {
		selected_function_category = _("Uncategorized");
	} else {
		selected_function_category = "/";
		selected_function_category += f->category();
	}
	//select the new function
	selected_function = f;
	update_fmenu();
	function_inserted(f);
}
void dataset_edited(DataSet *ds) {
	if(!ds) return;
	selected_dataset = ds;
	update_fmenu();
	function_inserted(ds);
	update_datasets_tree();
}
void function_inserted(MathFunction *object) {
	if(!object) return;
	add_recent_function(object);
	update_mb_fx_menu();
}
void variable_edited(Variable *v) {
	if(!v) return;
	selected_variable = v;
	if(!v->isActive()) {
		selected_variable_category = _("Inactive");
	} else if(v->isLocal()) {
		selected_variable_category = _("User variables");
	} else if(v->category().empty()) {
		selected_variable_category = _("Uncategorized");
	} else {
		selected_variable_category = "/";
		selected_variable_category += v->category();
	}
	update_vmenu();
	variable_inserted(v);
}
void variable_inserted(Variable *object) {
	if(!object || object == CALCULATOR->v_x || object == CALCULATOR->v_y || object == CALCULATOR->v_z) {
		return;
	}
	add_recent_variable(object);
	update_mb_pi_menu();
}
void unit_edited(Unit *u) {
	if(!u) return;
	selected_unit = u;
	if(!u->isActive()) {
		selected_unit_category = _("Inactive");
	} else if(u->isLocal()) {
		selected_unit_category = _("User units");
	} else if(u->category().empty()) {
		selected_unit_category = _("Uncategorized");
	} else {
		selected_unit_category = "/";
		selected_unit_category += u->category();
	}
	update_umenus();
	unit_inserted(u);
}
void unit_inserted(Unit *object) {
	if(!object) return;
	add_recent_unit(object);
	update_mb_units_menu();
}

void apply_function(MathFunction *f) {
	if(b_busy) return;
	if(rpn_mode) {
		calculateRPN(f);
		return;
	}
	string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionbuffer).formattedName(TYPE_FUNCTION, true);
	if(f->args() == 0) {
		str += "()";
	} else {
		str += "(";
		str += get_expression_text();
		str += ")";
	}
	block_undo();
	clear_expression_text();
	unblock_undo();
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

struct FunctionDialog {
	GtkWidget *dialog;
	GtkWidget *b_cancel, *b_exec, *b_insert, *b_keepopen, *w_result;
	vector<GtkWidget*> label;
	vector<GtkWidget*> entry;
	vector<GtkWidget*> type_label;
	vector<GtkWidget*> boolean_buttons;
	vector<int> boolean_index;
	GtkListStore *properties_store;
	bool add_to_menu, keep_open, rpn;
	int args;
};

unordered_map<MathFunction*, FunctionDialog*> function_dialogs;

void insert_function_do(MathFunction *f, FunctionDialog *fd) {
	string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_FUNCTION, true) + "(", str2;

	int argcount = fd->args;
	if(f->maxargs() > 0 && f->minargs() < f->maxargs() && argcount > f->minargs()) {
		while(true) {
			string defstr = localize_expression(f->getDefaultValue(argcount));
			remove_blank_ends(defstr);
			if(f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_BOOLEAN) {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fd->boolean_buttons[fd->boolean_index[argcount - 1]]))) {
					str2 = "1";
				} else {
					str2 = "0";
				}
			} else if(evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_INTEGER) {
				Number nr(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fd->entry[argcount - 1])), 1);
				str2 = print_with_evalops(nr);
			} else if(fd->properties_store && f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
				GtkTreeIter iter;
				DataProperty *dp = NULL;
				if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(fd->entry[argcount - 1]), &iter)) {
					gtk_tree_model_get(GTK_TREE_MODEL(fd->properties_store), &iter, 1, &dp, -1);
				}
				if(dp) {
					str2 = dp->getName();
				} else {
					str2 = "info";
				}
			} else {
				str2 = gtk_entry_get_text(GTK_ENTRY(fd->entry[argcount - 1]));
				remove_blank_ends(str2);
			}
			if(!str2.empty() && USE_QUOTES(f->getArgumentDefinition(argcount), f) && (unicode_length(str2) <= 2 || str2.find_first_of("\"\'") == string::npos)) {
				if(str2.find("\"") != string::npos) {
					str2.insert(0, "\'");
					str2 += "\'";
				} else {
					str2.insert(0, "\"");
					str2 += "\"";
				}
			}
			if(str2.empty() || str2 == defstr) argcount--;
			else break;
			if(argcount == 0 || argcount == f->minargs()) break;
		}
	}

	int i_vector = f->maxargs() > 0 ? f->maxargs() : argcount;
	for(int i = 0; i < argcount; i++) {
		if(f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fd->boolean_buttons[fd->boolean_index[i]]))) {
				str2 = "1";
			} else {
				str2 = "0";
			}
		} else if((i != (f->maxargs() > 0 ? f->maxargs() : argcount) - 1 || i_vector == i - 1) && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_VECTOR) {
			i_vector = i;
			str2 = gtk_entry_get_text(GTK_ENTRY(fd->entry[i]));
			remove_blank_ends(str2);
			if(str2.find_first_of(PARENTHESISS VECTOR_WRAPS) == string::npos && str2.find_first_of(CALCULATOR->getComma() == COMMA ? COMMAS : CALCULATOR->getComma()) != string::npos) {
				str2.insert(0, 1, '[');
				str2 += ']';
			}
		} else if(evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_INTEGER) {
			Number nr(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fd->entry[i])), 1);
			str2 = print_with_evalops(nr);
		} else if(fd->properties_store && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
			GtkTreeIter iter;
			DataProperty *dp = NULL;
			if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(fd->entry[i]), &iter)) {
				gtk_tree_model_get(GTK_TREE_MODEL(fd->properties_store), &iter, 1, &dp, -1);
			}
			if(dp) {
				str2 = dp->getName();
			} else {
				str2 = "info";
			}
		} else {
			str2 = gtk_entry_get_text(GTK_ENTRY(fd->entry[i]));
			remove_blank_ends(str2);
		}
		if((i < f->minargs() || !str2.empty()) && USE_QUOTES(f->getArgumentDefinition(i + 1), f) && (unicode_length(str2) <= 2 || str2.find_first_of("\"\'") == string::npos)) {
			if(str2.find("\"") != string::npos) {
				str2.insert(0, "\'");
				str2 += "\'";
			} else {
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
	insert_text(str.c_str());
	if(fd->add_to_menu) function_inserted(f);
}

gboolean on_insert_function_delete(GtkWidget*, GdkEvent*, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	gtk_widget_destroy(fd->dialog);
	delete fd;
	function_dialogs.erase(f);
	return true;
}
void on_insert_function_close(GtkWidget*, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	gtk_widget_destroy(fd->dialog);
	delete fd;
	function_dialogs.erase(f);
}
void on_insert_function_exec(GtkWidget*, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	if(!fd->keep_open) gtk_widget_hide(fd->dialog);
	gtk_text_buffer_set_text(expressionbuffer, "", -1);
	insert_function_do(f, fd);
	execute_expression();
	if(fd->keep_open) {
		string str;
		bool b_approx = result_text_approximate || (mstruct && mstruct->isApproximate());
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
		str += " <span font-weight=\"bold\">";
		str += result_text;
		str += "</span>";
		gtk_label_set_markup(GTK_LABEL(fd->w_result), str.c_str());
		gtk_widget_grab_focus(fd->entry[0]);
	} else {
		gtk_widget_destroy(fd->dialog);
		delete fd;
		function_dialogs.erase(f);
	}
}
void on_insert_function_insert(GtkWidget*, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	if(!fd->keep_open) gtk_widget_hide(fd->dialog);
	insert_function_do(f, fd);
	if(fd->keep_open) {
		gtk_widget_grab_focus(fd->entry[0]);
	} else {
		gtk_widget_destroy(fd->dialog);
		delete fd;
		function_dialogs.erase(f);
	}
}
void on_insert_function_rpn(GtkWidget *w, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	if(!fd->keep_open) gtk_widget_hide(fd->dialog);
	calculateRPN(f);
	if(fd->add_to_menu) function_inserted(f);
	if(fd->keep_open) {
		gtk_widget_grab_focus(fd->entry[0]);
	} else {
		gtk_widget_destroy(fd->dialog);
		delete fd;
		function_dialogs.erase(f);
	}
}
void on_insert_function_keepopen(GtkToggleButton *w, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	fd->keep_open = gtk_toggle_button_get_active(w);
	keep_function_dialog_open = fd->keep_open;
}
void on_insert_function_changed(GtkWidget *w, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	gtk_label_set_text(GTK_LABEL(fd->w_result), "");
}
void on_insert_function_entry_activated(GtkWidget *w, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	for(int i = 0; i < fd->args; i++) {
		if(fd->entry[i] == w) {
			if(i == fd->args - 1) {
				if(fd->rpn) on_insert_function_rpn(w, p);
				else if(fd->keep_open || rpn_mode) on_insert_function_exec(w, p);
				else on_insert_function_insert(w, p);
			} else {
				if(f->getArgumentDefinition(i + 2) && f->getArgumentDefinition(i + 2)->type() == ARGUMENT_TYPE_BOOLEAN) {
					gtk_widget_grab_focus(fd->boolean_buttons[fd->boolean_index[i + 1]]);
				} else {
					gtk_widget_grab_focus(fd->entry[i + 1]);
				}
			}
			break;
		}
	}

}

/*
	insert function
	pops up an argument entry dialog and inserts function into expression entry
	parent is parent window
*/
void insert_function(MathFunction *f, GtkWidget *parent, bool add_to_menu) {
	if(!f) {
		return;
	}

	//if function takes no arguments, do not display dialog and insert function directly
	if(f->args() == 0) {
		string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_FUNCTION, true) + "()";
		gchar *gstr = g_strdup(str.c_str());
		function_inserted(f);
		insert_text(gstr);
		g_free(gstr);
		return;
	}

	GtkTextIter istart, iend;
	gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);

	if(function_dialogs.find(f) != function_dialogs.end()) {
		FunctionDialog *fd = function_dialogs[f];
		if(fd->args > 0) {
			Argument *arg = f->getArgumentDefinition(1);
			if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
			} else if(fd->properties_store && arg && arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
			} else {
				g_signal_handlers_block_matched((gpointer) fd->entry[0], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
				//insert selection in expression entry into the first argument entry
				string str = get_selected_expression_text(true), str2;
				CALCULATOR->separateToExpression(str, str2, evalops, true);
				remove_blank_ends(str);
				gtk_entry_set_text(GTK_ENTRY(fd->entry[0]), str.c_str());
				if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
					gtk_spin_button_update(GTK_SPIN_BUTTON(fd->entry[0]));
				}
				g_signal_handlers_unblock_matched((gpointer) fd->entry[0], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
			}
			gtk_widget_grab_focus(fd->entry[0]);
		}
		gtk_window_present_with_time(GTK_WINDOW(fd->dialog), GDK_CURRENT_TIME);
		return;
	}

	FunctionDialog *fd = new FunctionDialog;

	function_dialogs[f] = fd;

	int args = 0;
	bool has_vector = false;
	if(f->args() > 0) {
		args = f->args();
	} else if(f->minargs() > 0) {
		args = f->minargs();
		while(!f->getDefaultValue(args + 1).empty()) args++;
		args++;
	} else {
		args = 1;
		has_vector = true;
	}
	fd->args = args;

	fd->rpn = rpn_mode && expression_is_empty() && CALCULATOR->RPNStackSize() >= (f->minargs() <= 0 ? 1 : (size_t) f->minargs());
	fd->add_to_menu = add_to_menu;

	fd->dialog = gtk_dialog_new();
	string f_title = f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) fd->dialog);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(fd->dialog), always_on_top);
	gtk_window_set_title(GTK_WINDOW(fd->dialog), f_title.c_str());
	gtk_window_set_transient_for(GTK_WINDOW(fd->dialog), GTK_WINDOW(parent));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(fd->dialog), TRUE);

	fd->b_keepopen = gtk_check_button_new_with_label(_("Keep open"));
	gtk_dialog_add_action_widget(GTK_DIALOG(fd->dialog), fd->b_keepopen, GTK_RESPONSE_NONE);
	fd->keep_open = keep_function_dialog_open;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fd->b_keepopen), fd->keep_open);

	fd->b_cancel = gtk_button_new_with_mnemonic(_("_Close"));
	gtk_dialog_add_action_widget(GTK_DIALOG(fd->dialog), fd->b_cancel, GTK_RESPONSE_REJECT);

	// RPN Enter (calculate and add to stack)
	fd->b_exec = gtk_button_new_with_mnemonic(rpn_mode ? _("Enter") : _("C_alculate"));
	gtk_dialog_add_action_widget(GTK_DIALOG(fd->dialog), fd->b_exec, GTK_RESPONSE_APPLY);

	fd->b_insert = gtk_button_new_with_mnemonic(rpn_mode ? _("Apply to Stack") : _("_Insert"));
	if(rpn_mode && CALCULATOR->RPNStackSize() < (f->minargs() <= 0 ? 1 : (size_t) f->minargs())) gtk_widget_set_sensitive(fd->b_insert, FALSE);
	gtk_dialog_add_action_widget(GTK_DIALOG(fd->dialog), fd->b_insert, GTK_RESPONSE_ACCEPT);

	gtk_container_set_border_width(GTK_CONTAINER(fd->dialog), 6);
	gtk_window_set_resizable(GTK_WINDOW(fd->dialog), FALSE);
	GtkWidget *vbox_pre = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_pre), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(fd->dialog))), vbox_pre);
	f_title.insert(0, "<b>");
	f_title += "</b>";
	GtkWidget *title_label = gtk_label_new(f_title.c_str());
	gtk_label_set_use_markup(GTK_LABEL(title_label), TRUE);
	gtk_widget_set_halign(title_label, GTK_ALIGN_START);

	gtk_container_add(GTK_CONTAINER(vbox_pre), title_label);

	GtkWidget *table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 6);
	gtk_grid_set_column_spacing(GTK_GRID(table), 12);
	gtk_grid_set_row_homogeneous(GTK_GRID(table), FALSE);
	gtk_container_add(GTK_CONTAINER(vbox_pre), table);
	gtk_widget_set_hexpand(table, TRUE);
	fd->label.resize(args, NULL);
	fd->entry.resize(args, NULL);
	fd->type_label.resize(args, NULL);
	fd->boolean_index.resize(args, 0);

	fd->w_result = gtk_label_new("");
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(fd->w_result, 6);
#else
	gtk_widget_set_margin_right(fd->w_result, 6);
#endif
	gtk_widget_set_margin_bottom(fd->w_result, 6);
	gtk_label_set_max_width_chars(GTK_LABEL(fd->w_result), 20);
	gtk_label_set_ellipsize(GTK_LABEL(fd->w_result), PANGO_ELLIPSIZE_MIDDLE);
	gtk_widget_set_hexpand(fd->w_result, TRUE);
	gtk_label_set_selectable(GTK_LABEL(fd->w_result), TRUE);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_label_set_xalign(GTK_LABEL(fd->w_result), 1.0);
#else
	gtk_misc_set_alignment(GTK_MISC(fd->w_result), 1.0, 0.5);
#endif

	int bindex = 0;
	int r = 0;
	string argstr, typestr, defstr;
	string freetype = Argument().printlong();
	Argument *arg;
	//create argument entries
	fd->properties_store = NULL;
	for(int i = 0; i < args; i++) {
		arg = f->getArgumentDefinition(i + 1);
		if(!arg || arg->name().empty()) {
			if(args == 1) {
				argstr = _("Value");
			} else {
				argstr = _("Argument");
				if(i > 0 || f->maxargs() != 1) {
					argstr += " ";
					argstr += i2s(i + 1);
				}
			}
		} else {
			argstr = arg->name();
		}
		typestr = "";
		defstr = localize_expression(f->getDefaultValue(i + 1));
		if(arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && defstr.length() >= 2 && defstr[0] == '\"' && defstr[defstr.length() - 1] == '\"') {
			defstr = defstr.substr(1, defstr.length() - 2);
		}
		fd->label[i] = gtk_label_new(argstr.c_str());
		gtk_widget_set_halign(fd->label[i], GTK_ALIGN_END);
		gtk_widget_set_hexpand(fd->label[i], FALSE);
		GtkWidget *combo = NULL;
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
					fd->entry[i] = gtk_spin_button_new_with_range(min, max, 1);
					gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(fd->entry[i]), evalops.parse_options.base != BASE_DECIMAL);
					gtk_entry_set_alignment(GTK_ENTRY(fd->entry[i]), 1.0);
					g_signal_connect(G_OBJECT(fd->entry[i]), "input", G_CALLBACK(on_function_int_input), NULL);
					g_signal_connect(G_OBJECT(fd->entry[i]), "key-press-event", G_CALLBACK(on_math_entry_key_press_event), NULL);
					if(!arg->zeroForbidden() && min <= 0 && max >= 0) {
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(fd->entry[i]), 0);
					} else {
						if(max < 0) {
							gtk_spin_button_set_value(GTK_SPIN_BUTTON(fd->entry[i]), max);
						} else if(min <= 1) {
							gtk_spin_button_set_value(GTK_SPIN_BUTTON(fd->entry[i]), 1);
						} else {
							gtk_spin_button_set_value(GTK_SPIN_BUTTON(fd->entry[i]), min);
						}
					}
					g_signal_connect(G_OBJECT(fd->entry[i]), "changed", G_CALLBACK(on_insert_function_changed), (gpointer) f);
					g_signal_connect(G_OBJECT(fd->entry[i]), "activate", G_CALLBACK(on_insert_function_entry_activated), (gpointer) f);
					break;
				}
				case ARGUMENT_TYPE_BOOLEAN: {
					fd->boolean_index[i] = bindex;
					bindex += 2;
					fd->entry[i] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
					gtk_box_set_homogeneous(GTK_BOX(fd->entry[i]), TRUE);
					gtk_widget_set_halign(fd->entry[i], GTK_ALIGN_START);
					fd->boolean_buttons.push_back(gtk_radio_button_new_with_label(NULL, _("True")));
					gtk_box_pack_start(GTK_BOX(fd->entry[i]), fd->boolean_buttons[fd->boolean_buttons.size() - 1], TRUE, TRUE, 0);
					fd->boolean_buttons.push_back(gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fd->boolean_buttons[fd->boolean_buttons.size() - 1]), _("False")));
					gtk_box_pack_end(GTK_BOX(fd->entry[i]), fd->boolean_buttons[fd->boolean_buttons.size() - 1], TRUE, TRUE, 0);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fd->boolean_buttons[fd->boolean_buttons.size() - 1]), TRUE);
					g_signal_connect(G_OBJECT(fd->boolean_buttons[fd->boolean_buttons.size() - 1]), "toggled", G_CALLBACK(on_insert_function_changed), (gpointer) f);
					g_signal_connect(G_OBJECT(fd->boolean_buttons[fd->boolean_buttons.size() - 2]), "toggled", G_CALLBACK(on_insert_function_changed), (gpointer) f);
					break;
				}
				case ARGUMENT_TYPE_DATA_PROPERTY: {
					if(f->subtype() == SUBTYPE_DATA_SET) {
						fd->properties_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
						gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fd->properties_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
						gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fd->properties_store), 0, GTK_SORT_ASCENDING);
						fd->entry[i] = gtk_combo_box_new_with_model(GTK_TREE_MODEL(fd->properties_store));
						GtkCellRenderer *cell = gtk_cell_renderer_text_new();
						gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fd->entry[i]), cell, TRUE);
						gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(fd->entry[i]), cell, "text", 0);
						DataPropertyIter it;
						DataSet *ds = (DataSet*) f;
						DataProperty *dp = ds->getFirstProperty(&it);
						GtkTreeIter iter;
						bool active_set = false;
						if(fd->rpn && (size_t) i < CALCULATOR->RPNStackSize()) {
							defstr = get_register_text(i + 1);
						}
						while(dp) {
							if(!dp->isHidden()) {
								gtk_list_store_append(fd->properties_store, &iter);
								if(!active_set && defstr == dp->getName()) {
									gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fd->entry[i]), &iter);
									active_set = true;
								}
								gtk_list_store_set(fd->properties_store, &iter, 0, dp->title().c_str(), 1, (gpointer) dp, -1);
							}
							dp = ds->getNextProperty(&it);
						}
						gtk_list_store_append(fd->properties_store, &iter);
						if(!active_set) {
							gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fd->entry[i]), &iter);
						}
						gtk_list_store_set(fd->properties_store, &iter, 0, _("Info"), 1, (gpointer) NULL, -1);
						g_signal_connect(G_OBJECT(fd->entry[i]), "changed", G_CALLBACK(on_insert_function_changed), (gpointer) f);
						break;
					}
				}
				default: {
					typestr = arg->printlong();
					if(typestr == freetype) typestr = "";
					if(arg->type() == ARGUMENT_TYPE_DATA_OBJECT && f->subtype() == SUBTYPE_DATA_SET && ((DataSet*) f)->getPrimaryKeyProperty()) {
						combo = gtk_combo_box_text_new_with_entry();
						DataObjectIter it;
						DataSet *ds = (DataSet*) f;
						DataObject *obj = ds->getFirstObject(&it);
						DataProperty *dp = ds->getProperty("name");
						if(!dp || !dp->isKey()) dp = ds->getPrimaryKeyProperty();
						while(obj) {
							gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), obj->getPropertyInputString(dp).c_str());
							obj = ds->getNextObject(&it);
						}
						fd->entry[i] = gtk_bin_get_child(GTK_BIN(combo));
						gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), "");
					} else if(i == 1 && f == CALCULATOR->f_ascii && arg->type() == ARGUMENT_TYPE_TEXT) {
						combo = gtk_combo_box_text_new_with_entry();
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "UTF-8");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "UTF-16");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "UTF-32");
						fd->entry[i] = gtk_bin_get_child(GTK_BIN(combo));
					} else if(i == 3 && f == CALCULATOR->f_date && arg->type() == ARGUMENT_TYPE_TEXT) {
						combo = gtk_combo_box_text_new_with_entry();
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "chinese");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "coptic");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "egyptian");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "ethiopian");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "gregorian");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "hebrew");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "indian");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "islamic");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "julian");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "milankovic");
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "persian");
						fd->entry[i] = gtk_bin_get_child(GTK_BIN(combo));
					} else {
						fd->entry[i] = gtk_entry_new();
					}
					if(i >= f->minargs() && !has_vector) {
						gtk_entry_set_placeholder_text(GTK_ENTRY(fd->entry[i]), _("optional"));
					}
					gtk_entry_set_alignment(GTK_ENTRY(fd->entry[i]), 1.0);
					if(!USE_QUOTES(arg, f)) g_signal_connect(G_OBJECT(fd->entry[i]), "key-press-event", G_CALLBACK(on_math_entry_key_press_event), NULL);
					g_signal_connect(G_OBJECT(fd->entry[i]), "changed", G_CALLBACK(on_insert_function_changed), (gpointer) f);
					g_signal_connect(G_OBJECT(fd->entry[i]), "activate", G_CALLBACK(on_insert_function_entry_activated), (gpointer) f);
				}
			}
		} else {
			fd->entry[i] = gtk_entry_new();
			if(i >= f->minargs() && !has_vector) {
				gtk_entry_set_placeholder_text(GTK_ENTRY(fd->entry[i]), _("optional"));
			}
			gtk_entry_set_alignment(GTK_ENTRY(fd->entry[i]), 1.0);
			g_signal_connect(G_OBJECT(fd->entry[i]), "key-press-event", G_CALLBACK(on_math_entry_key_press_event), NULL);
			g_signal_connect(G_OBJECT(fd->entry[i]), "changed", G_CALLBACK(on_insert_function_changed), (gpointer) f);
			g_signal_connect(G_OBJECT(fd->entry[i]), "activate", G_CALLBACK(on_insert_function_entry_activated), (gpointer) f);
		}
		gtk_widget_set_hexpand(fd->entry[i], TRUE);
		if(arg && arg->type() == ARGUMENT_TYPE_DATE) {
			if(defstr == "now") defstr = CALCULATOR->v_now->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) fd->entry[i]).formattedName(TYPE_VARIABLE, true);
			else if(defstr == "today") defstr = CALCULATOR->v_today->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) fd->entry[i]).formattedName(TYPE_VARIABLE, true);
			gtk_entry_set_icon_from_icon_name(GTK_ENTRY(fd->entry[i]), GTK_ENTRY_ICON_SECONDARY, "document-edit-symbolic");
			g_signal_connect(G_OBJECT(fd->entry[i]), "icon_press", G_CALLBACK(on_type_label_date_clicked), NULL);
		} else if(arg && arg->type() == ARGUMENT_TYPE_FILE) {
			gtk_entry_set_icon_from_icon_name(GTK_ENTRY(fd->entry[i]), GTK_ENTRY_ICON_SECONDARY, "document-open-symbolic");
			g_signal_connect(G_OBJECT(fd->entry[i]), "icon_press", G_CALLBACK(on_type_label_file_clicked), NULL);
		} else if(arg && (arg->type() == ARGUMENT_TYPE_VECTOR || arg->type() == ARGUMENT_TYPE_MATRIX)) {
			gtk_entry_set_icon_from_icon_name(GTK_ENTRY(fd->entry[i]), GTK_ENTRY_ICON_SECONDARY, "document-edit-symbolic");
			g_signal_connect(G_OBJECT(fd->entry[i]), "icon_press", G_CALLBACK(arg->type() == ARGUMENT_TYPE_VECTOR ? on_type_label_vector_clicked : on_type_label_matrix_clicked), NULL);
		} else if(!typestr.empty()) {
			if(printops.use_unicode_signs) {
				gsub(">=", SIGN_GREATER_OR_EQUAL, typestr);
				gsub("<=", SIGN_LESS_OR_EQUAL, typestr);
				gsub("!=", SIGN_NOT_EQUAL, typestr);
			}
			gsub("&", "&amp;", typestr);
			gsub(">", "&gt;", typestr);
			gsub("<", "&lt;", typestr);
			typestr.insert(0, "<i><small>"); typestr += "</small></i>";
			fd->type_label[i] = gtk_label_new(typestr.c_str());
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
			gtk_widget_set_margin_end(fd->type_label[i], 6);
#else
			gtk_widget_set_margin_right(fd->type_label[i], 6);
#endif
			gtk_label_set_use_markup(GTK_LABEL(fd->type_label[i]), TRUE);
			gtk_label_set_line_wrap(GTK_LABEL(fd->type_label[i]), TRUE);
			gtk_widget_set_halign(fd->type_label[i], GTK_ALIGN_END);
			gtk_widget_set_valign(fd->type_label[i], GTK_ALIGN_START);
		} else {
			fd->type_label[i] = NULL;
		}
		if(fd->rpn && (size_t) i < CALCULATOR->RPNStackSize()) {
			string str = get_register_text(i + 1);
			if(!str.empty()) {
				if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
					if(str == "1") {
						g_signal_handlers_block_matched((gpointer) fd->boolean_buttons[fd->boolean_buttons.size() - 2], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fd->boolean_buttons[fd->boolean_buttons.size() - 2]), TRUE);
						g_signal_handlers_unblock_matched((gpointer) fd->boolean_buttons[fd->boolean_buttons.size() - 2], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
					}
				} else if(fd->properties_store && arg && arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
				} else {
					g_signal_handlers_block_matched((gpointer) fd->entry[i], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
					if(i == 0 && args == 1 && (has_vector || arg->type() == ARGUMENT_TYPE_VECTOR)) {
						string rpn_vector = str;
						int i2 = i + 1;
						while(true) {
							str = get_register_text(i2 + 1);
							if(str.empty()) break;
							rpn_vector += CALCULATOR->getComma();
							rpn_vector += " ";
							rpn_vector += str;

						}
						gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), rpn_vector.c_str());
					} else {
						gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), str.c_str());
						if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
							gtk_spin_button_update(GTK_SPIN_BUTTON(fd->entry[i]));
						}
					}
					g_signal_handlers_unblock_matched((gpointer) fd->entry[i], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
				}
			}
		} else if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(defstr == "1") {
				g_signal_handlers_block_matched((gpointer) fd->boolean_buttons[fd->boolean_buttons.size() - 2], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fd->boolean_buttons[fd->boolean_buttons.size() - 2]), TRUE);
				g_signal_handlers_unblock_matched((gpointer) fd->boolean_buttons[fd->boolean_buttons.size() - 2], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
			}
		} else if(fd->properties_store && arg && arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
		} else {
			g_signal_handlers_block_matched((gpointer) fd->entry[i], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
			if(!defstr.empty() && (i < f->minargs() || has_vector || (defstr != "undefined" && defstr != "\"\""))) {
				gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), defstr.c_str());
				if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
					gtk_spin_button_update(GTK_SPIN_BUTTON(fd->entry[i]));
				}
			}
			//insert selection in expression entry into the first argument entry
			if(i == 0) {
				string seltext = get_selected_expression_text(true), str2;
				CALCULATOR->separateToExpression(seltext, str2, evalops, true);
				remove_blank_ends(seltext);
				if(!seltext.empty()) {
					gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), seltext.c_str());
					if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
						gtk_spin_button_update(GTK_SPIN_BUTTON(fd->entry[i]));
					}
				}
			}
			g_signal_handlers_unblock_matched((gpointer) fd->entry[i], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
		}
		gtk_grid_attach(GTK_GRID(table), fd->label[i], 0, r, 1, 1);
		if(combo) gtk_grid_attach(GTK_GRID(table), combo, 1, r, 1, 1);
		else gtk_grid_attach(GTK_GRID(table), fd->entry[i], 1, r, 1, 1);
		r++;
		if(fd->type_label[i]) {
			gtk_widget_set_hexpand(fd->type_label[i], FALSE);
			gtk_grid_attach(GTK_GRID(table), fd->type_label[i], 1, r, 1, 1);
			r++;
		}
	}

	//display function description
	if(!f->description().empty() || !f->example(true).empty()) {
		GtkWidget *descr_frame = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(vbox_pre), descr_frame);
		gtk_container_add(GTK_CONTAINER(vbox_pre), fd->w_result);
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
		if(printops.use_unicode_signs) {
			gsub(">=", SIGN_GREATER_OR_EQUAL, str);
			gsub("<=", SIGN_LESS_OR_EQUAL, str);
			gsub("!=", SIGN_NOT_EQUAL, str);
			gsub("...", "…", str);
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
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(descr), 6);
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
	} else {
		gtk_widget_set_margin_top(fd->w_result, 6);
		gtk_grid_attach(GTK_GRID(table), fd->w_result, 0, r, 2, 1);
	}

	g_signal_connect(G_OBJECT(fd->b_exec), "clicked", G_CALLBACK(on_insert_function_exec), (gpointer) f);
	if(fd->rpn) g_signal_connect(G_OBJECT(fd->b_insert), "clicked", G_CALLBACK(on_insert_function_rpn), (gpointer) f);
	else g_signal_connect(G_OBJECT(fd->b_insert), "clicked", G_CALLBACK(on_insert_function_insert), (gpointer) f);
	g_signal_connect(G_OBJECT(fd->b_cancel), "clicked", G_CALLBACK(on_insert_function_close), (gpointer) f);
	g_signal_connect(G_OBJECT(fd->b_keepopen), "toggled", G_CALLBACK(on_insert_function_keepopen), (gpointer) f);
	g_signal_connect(G_OBJECT(fd->dialog), "delete-event", G_CALLBACK(on_insert_function_delete), (gpointer) f);

	gtk_widget_show_all(fd->dialog);

	block_undo();
	gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
	unblock_undo();

}

void insert_variable(Variable *v, bool add_to_recent) {
	if(!v || !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), mainwindow);
		update_vmenu();
		return;
	}
	insert_text(v->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_VARIABLE, true).c_str());
	if(add_to_recent) variable_inserted(v);
}

void insert_unit(Unit *u, bool add_to_recent) {
	if(!u || !CALCULATOR->stillHasUnit(u)) return;
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		PrintOptions po = printops;
		po.is_approximate = NULL;
		po.can_display_unicode_string_arg = (void*) expressiontext;
		insert_text(((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true).c_str());
	} else {
		insert_text(u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_UNIT, true).c_str());
	}
	if(add_to_recent) unit_inserted(u);
}

/*
	"New function" menu item selected
*/
void new_function(GtkMenuItem*, gpointer)
{
	edit_function("", NULL, GTK_WINDOW(mainwindow));
}
/*
	"New unit" menu item selected
*/
void new_unit(GtkMenuItem*, gpointer)
{
	edit_unit("", NULL, GTK_WINDOW(mainwindow));
}

void convert_result_to_unit(Unit *u) {
	executeCommand(COMMAND_CONVERT_UNIT, true, false, "", u);
	focus_keeping_selection();
}
void convert_result_to_unit_expression(string str) {
	block_error();
	ParseOptions pa = evalops.parse_options; pa.base = 10;
	string ceu_str = CALCULATOR->unlocalizeExpression(str, pa);
	bool b_puup = printops.use_unit_prefixes;
	to_prefix = 0;
	printops.use_unit_prefixes = true;
	executeCommand(COMMAND_CONVERT_STRING, true, false, ceu_str);
	printops.use_unit_prefixes = b_puup;
	unblock_error();
}

void convert_to_unit_noprefix(GtkMenuItem*, gpointer user_data) {
	GtkWidget *edialog;
	ExpressionItem *u = (ExpressionItem*) user_data;
	if(!u) {
		edialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Unit does not exist"));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
	}
	string ceu_str = u->name();
	//result is stored in MathStructure *mstruct
	executeCommand(COMMAND_CONVERT_STRING, true, false, ceu_str);
	focus_keeping_selection();
}

void variable_removed(Variable *v) {
	remove_from_recent_variables(v);
	update_vmenu();
}

void unit_removed(Unit *u) {
	remove_from_recent_units(u);
	update_umenus();
}
void function_removed(MathFunction *f) {
	remove_from_recent_functions(f);
	update_fmenu();
}

void insert_matrix(const MathStructure *initial_value, GtkWidget *win, gboolean create_vector, bool is_text_struct, bool is_result, GtkEntry *entry) {
	if(!entry) expression_save_selection();
	string matrixstr = get_matrix(initial_value, GTK_WINDOW(win), create_vector, is_text_struct, is_result);
	if(matrixstr.empty()) return;
	if(entry) {
		gtk_entry_set_text(entry, matrixstr.c_str());
	} else {
		expression_restore_selection();
		insert_text(matrixstr.c_str());
	}
}

/*
	add a new variable (from menu) with the value of result
*/
void add_as_variable()
{
	edit_variable(CALCULATOR->temporaryCategory().c_str(), NULL, mstruct, GTK_WINDOW(mainwindow));
}

void new_unknown(GtkMenuItem*, gpointer)
{
	edit_unknown(NULL, NULL, GTK_WINDOW(mainwindow));
}

/*
	add a new variable (from menu)
*/
void new_variable(GtkMenuItem*, gpointer)
{
	edit_variable(NULL, NULL, NULL, GTK_WINDOW(mainwindow));
}

/*
	add a new matrix (from menu)
*/
void new_matrix(GtkMenuItem*, gpointer)
{
	edit_matrix(NULL, NULL, NULL, GTK_WINDOW(mainwindow), FALSE);
}
/*
	add a new vector (from menu)
*/
void new_vector(GtkMenuItem*, gpointer)
{
	edit_matrix(NULL, NULL, NULL, GTK_WINDOW(mainwindow), TRUE);
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
void insert_button_function(MathFunction *f, bool save_to_recent, bool apply_to_stack) {
	if(!f) return;
	if(!CALCULATOR->stillHasFunction(f)) return;
	bool b_rootlog = (f == CALCULATOR->f_logn || f == CALCULATOR->f_root) && f->args() > 1;
	if(rpn_mode && apply_to_stack && ((b_rootlog && CALCULATOR->RPNStackSize() >= 2) || (!b_rootlog && (f->minargs() <= 1 || (int) CALCULATOR->RPNStackSize() >= f->minargs())))) {
		calculateRPN(f);
		return;
	}

	if(f->minargs() > 2) return insert_function(f, mainwindow, save_to_recent);

	bool b_bitrot = (f->referenceName() == "bitrot");

	const ExpressionName *ename = &f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext);
	Argument *arg = f->getArgumentDefinition(1);
	Argument *arg2 = f->getArgumentDefinition(2);
	bool b_text = USE_QUOTES(arg, f);
	bool b_text2 = USE_QUOTES(arg2, f);
	GtkTextIter istart, iend, ipos;
	gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
	gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
	gchar *expr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expressionbuffer);
	gtk_text_buffer_get_iter_at_mark(expressionbuffer, &ipos, mpos);
	if(!gtk_text_buffer_get_has_selection(expressionbuffer) && gtk_text_iter_is_end(&ipos)) {
		if(!rpn_mode && chain_mode) {
			string str;
			GtkTextIter ibegin;
			gtk_text_buffer_get_end_iter(expressionbuffer, &ibegin);
			gchar *p = expr + strlen(expr), *prev_p = p;
			int nr_of_p = 0;
			bool prev_plusminus = false;
			while(p != expr) {
				p = g_utf8_prev_char(p);
				if(p[0] == LEFT_PARENTHESIS_CH) {
					if(nr_of_p == 0) {
						if(!prev_plusminus) {gtk_text_iter_backward_char(&ibegin);}
						break;
					}
					nr_of_p--;
				} else if(p[0] == RIGHT_PARENTHESIS_CH) {
					if(nr_of_p == 0 && prev_p != expr + strlen(expr)) {
						if(prev_plusminus) {gtk_text_iter_forward_char(&ibegin);}
						break;
					}
					nr_of_p++;
				} else if(nr_of_p == 0) {
					if((signed char) p[0] < 0) {
						for(size_t i = 0; p + i < prev_p; i++) str += p[i];
						CALCULATOR->parseSigns(str);
						if(!str.empty() && (signed char) str[0] > 0) {
							if(is_in("+-", str[0])) {
								prev_plusminus = true;
							} else if(is_in("*/&|=><^", str[0])) {
								break;
							} else if(prev_plusminus) {
								gtk_text_iter_forward_char(&ibegin);
								break;
							}
						}
					} else if(is_in("+-", p[0])) {
						prev_plusminus = true;
					} else if(is_in("*/&|=><^", p[0])) {
						break;
					} else if(prev_plusminus) {
						gtk_text_iter_forward_char(&ibegin);
						break;
					}
				}
				gtk_text_iter_backward_char(&ibegin);
				prev_p = p;
			}
			gtk_text_buffer_select_range(expressionbuffer, &ibegin, &iend);
		} else if(last_is_number(expr)) {
			// special case: the user just entered a number, then select all, so that it gets executed
			gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
		}
	}
	string str2;
	int index = 2;
	if(b_bitrot || f == CALCULATOR->f_bitcmp) {
		Argument *arg3 = f->getArgumentDefinition(3);
		Argument *arg4 = NULL;
		if(b_bitrot) {
			arg4 = arg2;
			arg2 = arg3;
			arg3 = f->getArgumentDefinition(4);
		}
		if(!arg2 || !arg3 || (b_bitrot && !arg4)) return;
		gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);
		GtkWidget *dialog = gtk_dialog_new_with_buttons(f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) mainwindow).c_str(), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
		GtkWidget *grid = gtk_grid_new();
		gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
		gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
		gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
		GtkWidget *w3 = NULL;
		if(b_bitrot) {
			GtkWidget *label2 = gtk_label_new(arg4->name().c_str());
			gtk_widget_set_halign(label2, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(grid), label2, 0, 0, 1, 1);
			glong min = LONG_MIN, max = LONG_MAX;
			if(arg4->type() == ARGUMENT_TYPE_INTEGER) {
				IntegerArgument *iarg = (IntegerArgument*) arg4;
				if(iarg->min()) {
					min = iarg->min()->lintValue();
				}
				if(iarg->max()) {
					max = iarg->max()->lintValue();
				}
			}
			w3 = gtk_spin_button_new_with_range(min, max, 1);
			gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w3), evalops.parse_options.base != BASE_DECIMAL);
			gtk_entry_set_alignment(GTK_ENTRY(w3), 1.0);
			g_signal_connect(G_OBJECT(w3), "input", G_CALLBACK(on_function_int_input), NULL);
			g_signal_connect(G_OBJECT(w3), "key-press-event", G_CALLBACK(on_math_entry_key_press_event), NULL);
			if(!f->getDefaultValue(2).empty()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(w3), s2i(f->getDefaultValue(index)));
			} else if(!arg4->zeroForbidden() && min <= 0 && max >= 0) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(w3), 0);
			} else {
				if(max < 0) {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(w3), max);
				} else if(min <= 1) {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(w3), 1);
				} else {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(w3), min);
				}
			}
			gtk_grid_attach(GTK_GRID(grid), w3, 1, 0, 1, 1);
		}
		GtkWidget *label = gtk_label_new(arg2->name().c_str());
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(grid), label, 0, b_bitrot ? 1 : 0, 1, 1);
		GtkWidget *w1 = gtk_combo_box_text_new();
		gtk_widget_set_hexpand(w1, TRUE);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "8");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "16");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "32");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "64");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "128");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "256");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "512");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w1), "1024");
		switch(default_bits) {
			case 8: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 0); break;}
			case 16: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 1); break;}
			case 32: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 2); break;}
			case 64: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 3); break;}
			case 128: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 4); break;}
			case 256: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 5); break;}
			case 512: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 6); break;}
			case 1024: {gtk_combo_box_set_active(GTK_COMBO_BOX(w1), 7); break;}
			default: {
				gint i = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_bits")));
				if(i <= 0) i = 2;
				else i--;
				gtk_combo_box_set_active(GTK_COMBO_BOX(w1), i);
				break;

			}
		}
		gtk_grid_attach(GTK_GRID(grid), w1, 1, b_bitrot ? 1 : 0, 1, 1);
		GtkWidget *w2 = gtk_check_button_new_with_label(arg3->name().c_str());
		if(default_signed > 0 || (default_signed < 0 && (evalops.parse_options.twos_complement || (b_bitrot && printops.twos_complement)))) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w2), TRUE);
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w2), FALSE);
		}
		gtk_widget_set_halign(w2, GTK_ALIGN_END);
		gtk_widget_set_hexpand(w2, TRUE);
		gtk_grid_attach(GTK_GRID(grid), w2, 0, b_bitrot ? 2 : 1, 2, 1);
		gtk_widget_show_all(dialog);
		if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
			g_free(expr);
			gtk_widget_destroy(dialog);
			gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
			return;
		}
		gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
		Number bits;
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(w1))) {
			case 0: {bits = 8; break;}
			case 1: {bits = 16; break;}
			case 3: {bits = 64; break;}
			case 4: {bits = 128; break;}
			case 5: {bits = 256; break;}
			case 6: {bits = 512; break;}
			case 7: {bits = 1024; break;}
			default: {bits = 32; break;}
		}
		if(b_bitrot) {
			if(evalops.parse_options.base != BASE_DECIMAL) {
				Number nr(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w3)), 1);
				str2 += print_with_evalops(nr);
			} else {
				str2 += gtk_entry_get_text(GTK_ENTRY(w3));
			}
			str2 += CALCULATOR->getComma();
			str2 += " ";
		}
		str2 += print_with_evalops(bits);
		str2 += CALCULATOR->getComma();
		str2 += " ";
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w2))) str2 += "1";
		else str2 += "0";
		default_bits = bits.intValue();
		default_signed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w2));
		gtk_widget_destroy(dialog);
	} else if((f->minargs() > 1 || b_rootlog) && ((arg2 && (b_rootlog || arg2->type() == ARGUMENT_TYPE_INTEGER)) xor (arg && arg->type() == ARGUMENT_TYPE_INTEGER))) {
		if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
			arg2 = arg;
			index = 1;
		}
		gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);
		GtkWidget *dialog = gtk_dialog_new_with_buttons(f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) mainwindow).c_str(), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
		GtkWidget *label = gtk_label_new(arg2->name().c_str());
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		glong min = LONG_MIN, max = LONG_MAX;
		if(arg2->type() == ARGUMENT_TYPE_INTEGER) {
			IntegerArgument *iarg = (IntegerArgument*) arg2;
			if(iarg->min()) {
				min = iarg->min()->lintValue();
			}
			if(iarg->max()) {
				max = iarg->max()->lintValue();
			}
		}
		GtkWidget *entry;
		if(evalops.parse_options.base == BASE_DECIMAL && f == CALCULATOR->f_logn) {
			entry = gtk_entry_new();
			if(f->getDefaultValue(index).empty()) gtk_entry_set_text(GTK_ENTRY(entry), "e");
			else gtk_entry_set_text(GTK_ENTRY(entry), f->getDefaultValue(index).c_str());
			gtk_widget_grab_focus(entry);
		} else {
			entry = gtk_spin_button_new_with_range(min, max, 1);
			gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), evalops.parse_options.base != BASE_DECIMAL);
			g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(on_math_entry_key_press_event), NULL);
			g_signal_connect(GTK_SPIN_BUTTON(entry), "input", G_CALLBACK(on_function_int_input), NULL);
			if(b_rootlog) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), 2);
			} else if(!f->getDefaultValue(index).empty()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), s2i(f->getDefaultValue(index)));
			} else if(!arg2->zeroForbidden() && min <= 0 && max >= 0) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), 0);
			} else {
				if(max < 0) {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), max);
				} else if(min <= 1) {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), 1);
				} else {
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), min);
				}
			}
		}
		gtk_entry_set_alignment(GTK_ENTRY(entry), 1.0);
		gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
		gtk_widget_show_all(dialog);
		if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
			g_free(expr);
			gtk_widget_destroy(dialog);
			gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
			return;
		}
		gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
		if(evalops.parse_options.base != BASE_DECIMAL) {
			Number nr(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)), 1);
			str2 = print_with_evalops(nr);
		} else {
			str2 = gtk_entry_get_text(GTK_ENTRY(entry));
		}
		gtk_widget_destroy(dialog);
	}
	if(gtk_text_buffer_get_has_selection(expressionbuffer)) {
		gtk_text_buffer_get_selection_bounds(expressionbuffer, &istart, &iend);
		// execute expression, if the whole expression was selected, no need for additional enter
		bool do_exec = (!str2.empty() || (f->minargs() < 2 && !b_rootlog)) && !rpn_mode && ((gtk_text_iter_is_start(&istart) && gtk_text_iter_is_end(&iend)) || (gtk_text_iter_is_start(&iend) && gtk_text_iter_is_end(&istart)));
		//set selection as argument
		gchar *gstr = gtk_text_buffer_get_text(expressionbuffer, &istart, &iend, FALSE);
		string str = gstr;
		remove_blank_ends(str);
		string sto;
		bool b_to = false;
		if(((gtk_text_iter_is_start(&istart) && gtk_text_iter_is_end(&iend)) || (gtk_text_iter_is_start(&iend) && gtk_text_iter_is_end(&istart)))) {
			CALCULATOR->separateToExpression(str, sto, evalops, true, true);
			if(!sto.empty()) b_to = true;
			CALCULATOR->separateWhereExpression(str, sto, evalops);
			if(!sto.empty()) b_to = true;
		}
		gchar *gstr2;
		if(b_text && str.length() > 2 && str.find_first_of("\"\'") != string::npos) b_text = false;
		if(b_text2 && str2.length() > 2 && str2.find_first_of("\"\'") != string::npos) b_text2 = false;
		if(f->minargs() > 1 || !str2.empty()) {
			if(b_text2) {
				if(index == 1) gstr2 = g_strdup_printf(b_text ? "%s(\"%s\"%s \"%s\")" : "%s(%s%s \"%s\")", ename->formattedName(TYPE_FUNCTION, true).c_str(), str2.c_str(), CALCULATOR->getComma().c_str(), gstr);
				else gstr2 = g_strdup_printf(b_text ? "%s(\"%s\"%s \"%s\")" : "%s(%s%s \"%s\")", ename->formattedName(TYPE_FUNCTION, true).c_str(), str.c_str(), CALCULATOR->getComma().c_str(), str2.c_str());
			} else {
				if(index == 1) gstr2 = g_strdup_printf(b_text ? "%s(\"%s\"%s %s)" : "%s(%s%s %s)", ename->formattedName(TYPE_FUNCTION, true).c_str(), str2.c_str(), CALCULATOR->getComma().c_str(), gstr);
				else gstr2 = g_strdup_printf(b_text ? "%s(\"%s\"%s %s)" : "%s(%s%s %s)", ename->formattedName(TYPE_FUNCTION, true).c_str(), str.c_str(), CALCULATOR->getComma().c_str(), str2.c_str());
			}
		} else {
			gstr2 = g_strdup_printf(b_text ? "%s(\"%s\")" : "%s(%s)", f->referenceName() == "neg" ? expression_sub_sign() : ename->formattedName(TYPE_FUNCTION, true).c_str(), str.c_str());
		}
		if(b_to) {
			string sexpr = gstr;
			sto = sexpr.substr(str.length());
			insert_text((string(gstr2) + sto).c_str());
		} else {
			insert_text(gstr2);
		}
		if(str2.empty() && (f->minargs() > 1 || b_rootlog || last_is_operator(str))) {
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iter, gtk_text_buffer_get_insert(expressionbuffer));
			gtk_text_iter_backward_chars(&iter, (b_text2 ? 2 : 1) + unicode_length(sto));
			gtk_text_buffer_place_cursor(expressionbuffer, &iter);
			do_exec = false;
		}
		if(do_exec) execute_expression();
		g_free(gstr);
		g_free(gstr2);
	} else {
		if(f->minargs() > 1 || b_rootlog || !str2.empty()) {
			if(b_text && str2.length() > 2 && str2.find_first_of("\"\'") != string::npos) b_text = false;
			gchar *gstr2;
			if(index == 1) gstr2 = g_strdup_printf(b_text ? "%s(\"%s\"%s )" : "%s(%s%s )", ename->formattedName(TYPE_FUNCTION, true).c_str(), str2.c_str(), CALCULATOR->getComma().c_str());
			else gstr2 = g_strdup_printf(b_text ? "%s(\"\"%s %s)" : "%s(%s %s)", ename->formattedName(TYPE_FUNCTION, true).c_str(), CALCULATOR->getComma().c_str(), str2.c_str());
			insert_text(gstr2);
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iter, gtk_text_buffer_get_insert(expressionbuffer));
			if(index == 2) {
				gtk_text_iter_backward_chars(&iter, g_utf8_strlen(str2.c_str(), -1) + (b_text ? 4 : 3));
			} else {
				gtk_text_iter_backward_chars(&iter, b_text ? 2 : 1);
			}
			gtk_text_buffer_place_cursor(expressionbuffer, &iter);
			g_free(gstr2);
		} else {
			gchar *gstr2;
			gstr2 = g_strdup_printf(b_text ? "%s(\"\")" : "%s()", ename->formattedName(TYPE_FUNCTION, true).c_str());
			insert_text(gstr2);
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(expressionbuffer, &iter, gtk_text_buffer_get_insert(expressionbuffer));
			gtk_text_iter_backward_chars(&iter, b_text ? 2 : 1);
			gtk_text_buffer_place_cursor(expressionbuffer, &iter);
			g_free(gstr2);
		}
	}
	g_free(expr);
	if(save_to_recent) function_inserted(f);
}

void fix_deactivate_label_width(GtkWidget *w) {
	gint w1, w2;
	string str = _("Deacti_vate");
	size_t i = str.find("_"); if(i != string::npos) str.erase(i, 1);
	PangoLayout *layout_test = gtk_widget_create_pango_layout(w, str.c_str());
	pango_layout_get_pixel_size(layout_test, &w1, NULL);
	str = _("Acti_vate");
	i = str.find("_"); if(i != string::npos) str.erase(i, 1);
	pango_layout_set_text(layout_test, str.c_str(), -1);
	pango_layout_get_pixel_size(layout_test, &w2, NULL);
	g_object_unref(layout_test);
	g_object_set(w, "width-request", w2 > w1 ? w2 : w1, NULL);
}

/*
	save definitions to ~/.conf/qalculate/qalculate.cfg
	the hard work is done in the Calculator class
*/
bool save_defs(bool allow_cancel) {
	if(!CALCULATOR->saveDefinitions()) {
		GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, _("Couldn't write definitions"));
		if(allow_cancel) {
			gtk_dialog_add_buttons(GTK_DIALOG(edialog), _("Ignore"), GTK_RESPONSE_CLOSE, _("Cancel"), GTK_RESPONSE_CANCEL, _("Retry"), GTK_RESPONSE_APPLY, NULL);
		} else {
			gtk_dialog_add_buttons(GTK_DIALOG(edialog), _("Ignore"), GTK_RESPONSE_CLOSE, _("Retry"), GTK_RESPONSE_APPLY, NULL);
		}
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		int ret = gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
		if(ret == GTK_RESPONSE_CANCEL) return false;
		if(ret == GTK_RESPONSE_APPLY) return save_defs(allow_cancel);
	}
	return true;
}

/*
	save mode to file
*/
void save_mode() {
	save_preferences(true);
	save_history();
}

/*
	remember current mode
*/
void set_saved_mode() {
	modes[1].precision = CALCULATOR->getPrecision();
	modes[1].interval = CALCULATOR->usesIntervalArithmetic();
	modes[1].concise_uncertainty_input = CALCULATOR->conciseUncertaintyInputEnabled();
	modes[1].fixed_denominator = CALCULATOR->fixedDenominator();
	modes[1].adaptive_interval_display = adaptive_interval_display;
	modes[1].variable_units_enabled = CALCULATOR->variableUnitsEnabled();
	modes[1].po = printops;
	modes[1].po.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	modes[1].eo = evalops;
	modes[1].at = CALCULATOR->defaultAssumptions()->type();
	modes[1].as = CALCULATOR->defaultAssumptions()->sign();
	modes[1].rpn_mode = rpn_mode;
	modes[1].autocalc = auto_calculate;
	modes[1].chain_mode = chain_mode;
	modes[1].keypad = visible_keypad;
	modes[1].custom_output_base = CALCULATOR->customOutputBase();
	modes[1].custom_input_base = CALCULATOR->customInputBase();
	modes[1].complex_angle_form = complex_angle_form;
	modes[1].implicit_question_asked = implicit_question_asked;
	modes[1].simplified_percentage = simplified_percentage;
	modes[1].custom_angle_unit = custom_angle_unit;
}

size_t save_mode_as(string name, bool *new_mode) {
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
	modes[index].interval = CALCULATOR->usesIntervalArithmetic();
	modes[index].adaptive_interval_display = adaptive_interval_display;
	modes[index].variable_units_enabled = CALCULATOR->variableUnitsEnabled();
	modes[index].at = CALCULATOR->defaultAssumptions()->type();
	modes[index].as = CALCULATOR->defaultAssumptions()->sign();
	modes[index].concise_uncertainty_input = CALCULATOR->conciseUncertaintyInputEnabled();
	modes[index].fixed_denominator = CALCULATOR->fixedDenominator();
	modes[index].name = name;
	modes[index].rpn_mode = rpn_mode;
	modes[index].autocalc = auto_calculate;
	modes[index].chain_mode = chain_mode;
	modes[index].keypad = visible_keypad;
	modes[index].custom_output_base = CALCULATOR->customOutputBase();
	modes[index].custom_input_base = CALCULATOR->customInputBase();
	modes[index].complex_angle_form = complex_angle_form;
	modes[index].implicit_question_asked = implicit_question_asked;
	modes[index].simplified_percentage = simplified_percentage;
	modes[index].custom_angle_unit = "";
	if(CALCULATOR->customAngleUnit()) modes[index].custom_angle_unit = CALCULATOR->customAngleUnit()->referenceName();
	return index;
}

void load_mode(const mode_struct &mode) {
	block_result();
	block_calculation();
	block_display_parse++;
	if(mode.keypad == 1) {
		programming_inbase = 0;
		programming_outbase = 0;
	}
	if(mode.name == _("Preset") || mode.name == _("Default")) current_mode = "";
	else current_mode = mode.name;
	update_window_title();
	CALCULATOR->setCustomOutputBase(mode.custom_output_base);
	CALCULATOR->setCustomInputBase(mode.custom_input_base);
	CALCULATOR->setConciseUncertaintyInputEnabled(mode.concise_uncertainty_input);
	CALCULATOR->setFixedDenominator(mode.fixed_denominator);
	custom_angle_unit = mode.custom_angle_unit;
	set_mode_items(mode.po, mode.eo, mode.at, mode.as, mode.rpn_mode, mode.precision, mode.interval, mode.variable_units_enabled, mode.adaptive_interval_display, mode.autocalc, mode.chain_mode, mode.complex_angle_form, mode.simplified_percentage, false);
	previous_precision = 0;
	update_setbase();
	update_decimals();
	update_precision();
	visible_keypad = mode.keypad;
	update_keypad_state();
	implicit_question_asked = mode.implicit_question_asked;
	evalops.approximation = mode.eo.approximation;
	unblock_result();
	unblock_calculation();
	block_display_parse--;
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	update_message_print_options();
	update_status_text();
	auto_calculate = mode.autocalc;
	chain_mode = mode.chain_mode;
	complex_angle_form = mode.complex_angle_form;
	set_rpn_mode(mode.rpn_mode);
	string str = get_expression_text();
	set_expression_output_updated(true);
	display_parse_status();
	if(auto_calculate && !rpn_mode) {
		do_auto_calc();
	} else if(rpn_mode || expression_modified() || str.find_first_not_of(SPACES) == string::npos) {
		setResult(NULL, true, false, false);
	} else {
		execute_expression(false);
	}
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

void show_tabs(bool do_show) {
	if(do_show == gtk_widget_get_visible(tabs)) return;
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &w, &h);
	if(!persistent_keypad && gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")))) h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))) + 9;
	if(do_show) {
		gtk_widget_show(tabs);
		gint a_h = gtk_widget_get_allocated_height(tabs);
		if(a_h > 10) h += a_h + 9;
		else h += history_height + 9;
		if(!persistent_keypad) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		gtk_window_resize(GTK_WINDOW(mainwindow), w, h);
	} else {
		h -= gtk_widget_get_allocated_height(tabs) + 9;
		gtk_widget_hide(tabs);
		set_result_size_request();
		set_expression_size_request();
		gtk_window_resize(GTK_WINDOW(mainwindow), w, h);
	}
	gtk_widget_set_vexpand(resultview, !gtk_widget_get_visible(tabs) && !gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))));
	gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), !persistent_keypad || !gtk_widget_get_visible(tabs));
}
void show_keypad_widget(bool do_show) {
	if(do_show == gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")))) return;
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &w, &h);
	if(!persistent_keypad && gtk_widget_get_visible(tabs)) h -= gtk_widget_get_allocated_height(tabs) + 9;
	if(persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
		if(do_show) h += 6;
		else h -= 6;
	}
	if(do_show) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		gint a_h = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		if(a_h > 10) h += a_h + 9;
		else h += 9;
		if(!persistent_keypad) gtk_widget_hide(tabs);
		gtk_window_resize(GTK_WINDOW(mainwindow), w, h);
	} else {
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))) + 9;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		set_result_size_request();
		set_expression_size_request();
		gtk_window_resize(GTK_WINDOW(mainwindow), w, h);
	}
	gtk_widget_set_vexpand(resultview, !gtk_widget_get_visible(tabs) && !gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))));
	gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), !persistent_keypad || !gtk_widget_get_visible(tabs));
}

void update_persistent_keypad(bool showhide_buttons) {
	if(!persistent_keypad && gtk_widget_is_visible(tabs)) showhide_buttons = true;
	gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")), !persistent_keypad || !gtk_widget_get_visible(tabs));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl")), !persistent_keypad || (rpn_mode && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnr")), !persistent_keypad || (rpn_mode && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))));
	if(showhide_buttons && (persistent_keypad || gtk_widget_is_visible(tabs))) {
		show_keypad = false;
		g_signal_handlers_block_matched((gpointer) expander_keypad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_expander_keypad_expanded, NULL);
		gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), persistent_keypad);
		g_signal_handlers_unblock_matched((gpointer) expander_keypad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_expander_keypad_expanded, NULL);
		if(persistent_keypad) gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		else show_keypad_widget(false);
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_hi")), !persistent_keypad);
	preferences_update_persistent_keypad();
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_persistent_keypad"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_persistent_keypad_toggled, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_persistent_keypad")), persistent_keypad);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_persistent_keypad"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_persistent_keypad_toggled, NULL);
	GtkRequisition req;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_keypad")), &req, NULL);
	gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_keypad_lock")), persistent_keypad ? "changes-prevent-symbolic" : "changes-allow-symbolic", GTK_ICON_SIZE_BUTTON);
	if(req.height < 20) gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_keypad_lock")), req.height * 0.8);
	if(showhide_buttons) gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) ? 6 : 0);
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(historyview)));
}
void on_expander_keypad_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		show_keypad_widget(true);
		if(!persistent_keypad) {
			if(gtk_expander_get_expanded(GTK_EXPANDER(expander_history))) {
				gtk_expander_set_expanded(GTK_EXPANDER(expander_history), FALSE);
			} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) {
				gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), FALSE);
			} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
				gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), FALSE);
			}
		}
	} else {
		show_keypad_widget(false);
	}
	if(persistent_keypad) gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), gtk_expander_get_expanded(GTK_EXPANDER(o)) ? 6 : 0);
}
void on_expander_history_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		bool history_was_realized = gtk_widget_get_realized(historyview);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 0);
		show_tabs(true);
		while(!history_was_realized && gtk_events_pending()) gtk_main_iteration();
		if(!history_was_realized) history_scroll_on_realized();
		if(!persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad))) {
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
		if(!persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_history))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_history), FALSE);
		} else if(gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), FALSE);
		}
	} else if(!gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) && !gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
		show_tabs(false);
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl")), !persistent_keypad || gtk_expander_get_expanded(GTK_EXPANDER(o)));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnr")), !persistent_keypad || gtk_expander_get_expanded(GTK_EXPANDER(o)));
}
void on_expander_convert_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 2);
		show_tabs(true);
		if(!persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad))) {
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

void update_minimal_width() {
	gint w;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &w, NULL);
	if(w != win_width) minimal_width = w;
}

gint minimal_window_resized_timeout_id = 0;
gboolean minimal_window_resized_timeout(gpointer) {
	minimal_window_resized_timeout_id = 0;
	if(minimal_mode) update_minimal_width();
	return FALSE;
}
gboolean do_minimal_mode_timeout(gpointer) {
	gtk_widget_set_size_request(tabs, -1, -1);
	return FALSE;
}
void set_minimal_mode(bool b) {
	minimal_mode = b;
	if(minimal_mode) {
		if(gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) {
			gint h = gtk_widget_get_allocated_height(tabs);
			if(h > 10) history_height = h;
		}
		gint w = 0;
		gtk_window_get_size(GTK_WINDOW(mainwindow), &w, NULL);
		win_width = w;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")));
		if(expression_is_empty() || !displayed_mstruct) {
			clearresult();
		}
		gtk_window_resize(GTK_WINDOW(mainwindow), minimal_width > 0 ? minimal_width : win_width, 1);
		gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), TRUE);
		gtk_widget_set_vexpand(resultview, FALSE);
	} else {
		if(minimal_window_resized_timeout_id) {
			g_source_remove(minimal_window_resized_timeout_id);
			minimal_window_resized_timeout_id = 0;
			update_minimal_width();
		}
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")));
		if(history_height > 0 && (gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_stack)))) {
			gtk_widget_set_size_request(tabs, -1, history_height);
		}
		gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), FALSE);
		gtk_widget_set_vexpand(resultview, !gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"))) && !gtk_widget_get_visible(tabs));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		set_status_bottom_border_visible(true);
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		if(history_height > 0 && (gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_stack)))) {
			gdk_threads_add_timeout(500, do_minimal_mode_timeout, NULL);
		}
		gint h = 1;
		if(gtk_widget_is_visible(tabs) || gtk_widget_is_visible(keypad)) {
			gtk_window_get_size(GTK_WINDOW(mainwindow), NULL, &h);
		}
		gtk_window_resize(GTK_WINDOW(mainwindow), win_width < 0 ? 1 : win_width, h);
	}
	set_expression_size_request();
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
	default_plot_sampling_rate = 1001;
	default_plot_linewidth = 2;
	default_plot_rows = false;
	default_plot_type = 0;
	default_plot_style = PLOT_STYLE_LINES;
	default_plot_smoothing = PLOT_SMOOTHING_NONE;
	default_plot_variable = "x";
	default_plot_color = true;
	default_plot_use_sampling_rate = true;
	default_plot_complex = -1;
	max_plot_time = 5;

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
	printops.show_ending_zeroes = true;
	printops.round_halfway_to_even = false;
	printops.rounding = ROUNDING_HALF_AWAY_FROM_ZERO;
	printops.number_fraction_format = FRACTION_DECIMAL;
	printops.restrict_fraction_length = false;
	printops.abbreviate_names = true;
	printops.use_unicode_signs = true;
	printops.digit_grouping = DIGIT_GROUPING_STANDARD;
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
	printops.duodecimal_symbols = false;
	printops.exp_display = EXP_POWER_OF_10;
	printops.lower_case_e = false;
	printops.base_display = BASE_DISPLAY_NORMAL;
	printops.twos_complement = true;
	printops.hexadecimal_twos_complement = false;
	printops.limit_implicit_multiplication = false;
	printops.can_display_unicode_string_function = &can_display_unicode_string_function;
	printops.allow_factorization = false;
	printops.spell_out_logical_operators = true;
	printops.exp_to_root = true;
	printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;

	evalops.approximation = APPROXIMATION_TRY_EXACT;
	evalops.sync_units = true;
	evalops.structuring = STRUCTURING_SIMPLIFY;
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
	implicit_question_asked = false;
	evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	custom_angle_unit = "";
	evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	dot_question_asked = false;
	evalops.parse_options.comma_as_separator = false;
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	complex_angle_form = false;
	evalops.local_currency_conversion = true;
	evalops.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	b_decimal_comma = -1;
	simplified_percentage = -1;

	use_systray_icon = false;
	hide_on_startup = false;

#ifdef _WIN32
	check_version = true;
#else
	check_version = false;
#endif

	title_type = TITLE_APP;

	auto_calculate = false;
	chain_mode = false;
	autocalc_history_delay = 2000;

	default_signed = -1;
	default_bits = -1;

	programming_inbase = 0;
	programming_outbase = 0;

	visible_keypad = 0;

	caret_as_xor = false;

	close_with_esc = -1;

	ignore_locale = false;

	automatic_fraction = false;
	default_fraction_fraction = -1;
	scientific_noprefix = true;
	scientific_notminuslast = true;
	scientific_negexp = true;
	auto_prefix = 0;
	fraction_fixed_combined = true;

	keep_function_dialog_open = false;

	copy_ascii = false;
	copy_ascii_without_units = false;

	adaptive_interval_display = true;

	CALCULATOR->useIntervalArithmetic(true);

	CALCULATOR->setConciseUncertaintyInputEnabled(false);

	CALCULATOR->setTemperatureCalculationMode(TEMPERATURE_CALCULATION_HYBRID);
	tc_set = false;
	sinc_set = false;

	CALCULATOR->useBinaryPrefixes(0);

	rpn_mode = false;
	rpn_keys = true;

	enable_tooltips = 1;
	toe_changed = false;

	save_mode_as(_("Preset"));
	save_mode_as(_("Default"));
	size_t mode_index = 1;

	win_x = 0;
	win_y = 0;
	win_monitor = 0;
	win_monitor_primary = false;
	remember_position = false;
	always_on_top = false;
	aot_changed = false;
	win_width = -1;
	win_height = -1;
	variables_width = -1;
	variables_height = -1;
	variables_hposition = -1;
	variables_vposition = -1;
	units_width = -1;
	units_height = -1;
	units_hposition = -1;
	units_vposition = -1;
	functions_width = -1;
	functions_height = -1;
	functions_hposition = -1;
	functions_vposition = -1;
	datasets_width = -1;
	datasets_height = -1;
	datasets_hposition = -1;
	datasets_vposition1 = -1;
	datasets_vposition2 = -1;
	help_width = -1;
	help_height = -1;
	help_zoom = -1.0;
#ifdef _WIN32
	horizontal_button_padding = 6;
#else
	horizontal_button_padding = -1;
#endif
	vertical_button_padding = -1;
	minimal_width = 500;
	history_height = 0;
	save_mode_on_exit = true;
	save_defs_on_exit = true;
	clear_history_on_exit = false;
	max_history_lines = 300;
	save_history_separately = false;
	history_expression_type = 2;
	hyp_is_on = false;
	inv_is_on = false;
	use_custom_result_font = false;
	use_custom_expression_font = false;
	use_custom_status_font = false;
	use_custom_keypad_font = false;
	use_custom_history_font = false;
	use_custom_app_font = false;
	custom_result_font = "";
	custom_expression_font = "";
	custom_status_font = "";
	custom_keypad_font = "";
	custom_history_font = "";
	custom_app_font = "";
	status_error_color = "#FF0000";
	status_warning_color = "#0000FF";
	status_error_color_set = false;
	status_warning_color_set = false;
	text_color = "#FFFFFF";
	text_color_set = false;
	show_keypad = true;
	show_history = false;
	show_stack = true;
	show_convert = false;
	persistent_keypad = false;
	minimal_mode = false;
	show_bases_keypad = true;
	continuous_conversion = true;
	set_missing_prefixes = false;
	load_global_defs = true;
	fetch_exchange_rates_at_startup = false;
	auto_update_exchange_rates = -1;
	display_expression_status = true;
	parsed_in_result = false;
	enable_completion = true;
	enable_completion2 = true;
	completion_min = 1;
	completion_min2 = 1;
	completion_delay = 0;
	first_time = false;
	first_error = true;
	expression_history.clear();
	expression_history_index = -1;
	expression_lines = -1;
	gtk_theme = -1;
	custom_lang = "";

	CALCULATOR->setPrecision(10);
	previous_precision = 0;

	time_t history_time = 0;

	default_shortcuts = true;
	keyboard_shortcuts.clear();

	custom_buttons.resize(49);
	for(size_t i = 0; i < 49; i++) {
		custom_buttons[i].type[0] = -1;
		custom_buttons[i].type[1] = -1;
		custom_buttons[i].type[2] = -1;
		custom_buttons[i].value[0] = "";
		custom_buttons[i].value[1] = "";
		custom_buttons[i].value[2] = "";
		custom_buttons[i].text = "";
	}

	last_version_check_date.setToCurrentDate();

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
		if(!file) g_free(gstr_oldfile);
#endif
	}

	size_t bookmark_index = 0;

	version_numbers[0] = 5;
	version_numbers[1] = 2;
	version_numbers[2] = 0;

	bool old_history_format = false;
	unformatted_history = 0;

	if(file) {
		char line[1000000L];
		string stmp, svalue, svar;
		size_t i;
		int v;
		for(int file_i = 1; file_i <= 2; file_i++) {
			while(true) {
				if(fgets(line, 1000000L, file) == NULL) break;
				stmp = line;
				remove_blank_ends(stmp);
				if((i = stmp.find_first_of("=")) != string::npos) {
					svar = stmp.substr(0, i);
					remove_blank_ends(svar);
					svalue = stmp.substr(i + 1);
					remove_blank_ends(svalue);
					v = s2i(svalue);
					if(svar == "version") {
						parse_qalculate_version(svalue, version_numbers);
						old_history_format = (version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 4)));
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 22) || (version_numbers[0] == 3 && version_numbers[1] == 22 && version_numbers[2] < 1)) unformatted_history = 1;
					} else if(svar == "allow_multiple_instances") {
						if(v == 0 && version_numbers[0] < 3) v = -1;
						allow_multiple_instances = v;
					} else if(svar == "width") {
						win_width = v;
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 15)) win_width -= 6;
					/*} else if(svar == "height") {
						win_height = v;*/
					} else if(svar == "always_on_top") {
						always_on_top = v;
					} else if(svar == "enable_tooltips") {
						enable_tooltips = v;
						if(enable_tooltips < 0) enable_tooltips = 1;
						else if(enable_tooltips > 2) enable_tooltips = 2;
					} else if(svar == "monitor") {
						if(win_monitor > 0) win_monitor = v;
					} else if(svar == "monitor_primary") {
						win_monitor_primary = v;
					} else if(svar == "x") {
						win_x = v;
						remember_position = true;
					} else if(svar == "y") {
						win_y = v;
						remember_position = true;
#ifdef _WIN32
					} else if(svar == "use_system_tray_icon") {
						use_systray_icon = v;
#endif
					} else if(svar == "hide_on_startup") {
						hide_on_startup = v;
					} else if(svar == "variables_width") {
						variables_width = v;
					} else if(svar == "variables_height") {
						variables_height = v;
					} else if(svar == "variables_panel_position") {
						variables_hposition = v;
					} else if(svar == "variables_vpanel_position") {
						variables_vposition = v;
					} else if(svar == "variables_hpanel_position") {
						variables_hposition = v;
					} else if(svar == "units_width") {
						units_width = v;
					} else if(svar == "units_height") {
						units_height = v;
					} else if(svar == "units_panel_position") {
						units_hposition = v;
					} else if(svar == "units_hpanel_position") {
						units_hposition = v;
					} else if(svar == "units_vpanel_position") {
						units_vposition = v;
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
					} else if(svar == "help_width") {
						help_width = v;
					} else if(svar == "help_height") {
						help_height = v;
					} else if(svar == "help_zoom") {
						help_zoom = strtod(svalue.c_str(), NULL);
					} else if(svar == "keep_function_dialog_open") {
						keep_function_dialog_open = v;
					} else if(svar == "error_info_shown") {
						first_error = !v;
					} else if(svar == "save_mode_on_exit") {
						save_mode_on_exit = v;
					} else if(svar == "save_definitions_on_exit") {
						save_defs_on_exit = v;
					} else if(svar == "clear_history_on_exit") {
						clear_history_on_exit = v;
					} else if(svar == "max_history_lines") {
						max_history_lines = v;
					} else if(svar == "save_history_separately") {
						save_history_separately = v;
					} else if(svar == "history_expression_type") {
						history_expression_type = v;
					} else if(svar == "language") {
						custom_lang = svalue;
					} else if(svar == "ignore_locale") {
						ignore_locale = v;
					} else if(svar == "window_title_mode") {
						if(v >= 0 && v <= 4) title_type = v;
					} else if(svar == "fetch_exchange_rates_at_startup") {
						if(auto_update_exchange_rates < 0 && v) auto_update_exchange_rates = 1;
					} else if(svar == "auto_update_exchange_rates") {
						auto_update_exchange_rates = v;
					} else if(svar == "check_version") {
						check_version = v;
					} else if(svar == "last_version_check") {
						last_version_check_date.set(svalue);
					} else if(svar == "last_found_version") {
						last_found_version = svalue;
					} else if(svar == "show_keypad") {
						show_keypad = v;
					} else if(svar == "show_history") {
						show_history = v;
					} else if(svar == "history_height") {
						history_height = v;
					} else if(svar == "minimal_width") {
						if(v != 0 || version_numbers[0] > 3 || (version_numbers[0] == 3 && version_numbers[1] >= 15)) minimal_width = v;
					} else if(svar == "show_stack") {
						show_stack = v;
					} else if(svar == "show_convert") {
						show_convert = v;
					} else if(svar == "persistent_keypad") {
						persistent_keypad = v;
					} else if(svar == "minimal_mode") {
						minimal_mode = v;
					} else if(svar == "show_bases_keypad") {
						show_bases_keypad = v;
					} else if(svar == "continuous_conversion") {
						continuous_conversion = v;
					} else if(svar == "set_missing_prefixes") {
						set_missing_prefixes = v;
					} else if(svar == "expression_lines") {
						expression_lines = v;
					} else if(svar == "display_expression_status") {
						display_expression_status = v;
					} else if(svar == "parsed_expression_in_resultview") {
						parsed_in_result = v;
					} else if(svar == "enable_completion") {
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
					} else if(svar == "calculate_as_you_type_history_delay") {
						autocalc_history_delay = v;
					} else if(svar == "programming_outbase") {
						programming_outbase = v;
					} else if(svar == "programming_inbase") {
						programming_inbase = v;
					} else if(svar == "programming_bits" || svar == "binary_bits") {
						evalops.parse_options.binary_bits = v;
						printops.binary_bits = v;
					} else if(svar == "general_exact") {
						versatile_exact = v;
					} else if(svar == "bit_width") {
						default_bits = v;
					} else if(svar == "signed_integer") {
						default_signed = v;
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
						if(v == 8 && (version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] <= 12))) v = 10;
						if(mode_index == 1) CALCULATOR->setPrecision(v);
						else modes[mode_index].precision = v;
					} else if(svar == "previous_precision") {
						previous_precision = v;
					} else if(svar == "min_exp") {
						if(mode_index == 1) printops.min_exp = v;
						else modes[mode_index].po.min_exp = v;
					} else if(svar == "interval_arithmetic") {
						if(version_numbers[0] >= 3) {
							if(mode_index == 1) CALCULATOR->useIntervalArithmetic(v);
							else modes[mode_index].interval = v;
						} else {
							modes[mode_index].interval = true;
						}
					} else if(svar == "interval_display") {
						if(v == 0) {
							if(mode_index == 1) {printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; adaptive_interval_display = true;}
							else {modes[mode_index].po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; modes[mode_index].adaptive_interval_display = true;}
						} else {
							v--;
							if(v >= INTERVAL_DISPLAY_SIGNIFICANT_DIGITS && v <= INTERVAL_DISPLAY_RELATIVE) {
								if(mode_index == 1) {printops.interval_display = (IntervalDisplay) v; adaptive_interval_display = false;}
								else {modes[mode_index].po.interval_display = (IntervalDisplay) v; modes[mode_index].adaptive_interval_display = false;}
							}
						}
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
						if(mode_index == 1) printops.restrict_fraction_length = (printops.number_fraction_format >= FRACTION_FRACTIONAL);
						else modes[mode_index].po.restrict_fraction_length = (modes[mode_index].po.number_fraction_format >= FRACTION_FRACTIONAL);
					} else if(svar == "number_fraction_format") {
						if(v >= FRACTION_DECIMAL && v <= FRACTION_PERMYRIAD) {
							if(mode_index == 1) printops.number_fraction_format = (NumberFractionFormat) v;
							else modes[mode_index].po.number_fraction_format = (NumberFractionFormat) v;
						}
						if(mode_index == 1) printops.restrict_fraction_length = (printops.number_fraction_format == FRACTION_FRACTIONAL || printops.number_fraction_format == FRACTION_COMBINED);
						else modes[mode_index].po.restrict_fraction_length = (modes[mode_index].po.number_fraction_format == FRACTION_FRACTIONAL || modes[mode_index].po.number_fraction_format == FRACTION_COMBINED);
					} else if(svar == "number_fraction_denominator") {
						if(mode_index == 1) CALCULATOR->setFixedDenominator(v);
						else modes[mode_index].fixed_denominator = v;
					} else if(svar == "automatic_number_fraction_format") {
						automatic_fraction = v;
					} else if(svar == "default_number_fraction_fraction") {
						if(v >= FRACTION_FRACTIONAL && v <= FRACTION_COMBINED) default_fraction_fraction = (NumberFractionFormat) v;
					} else if(svar == "automatic_unit_prefixes") {
						auto_prefix = v;
					} else if(svar == "scientific_mode_unit_prefixes") {
						scientific_noprefix = !v;
					} else if(svar == "scientific_mode_sort_minus_last") {
						scientific_notminuslast = !v;
					} else if(svar == "scientific_mode_negative_exponents") {
						scientific_negexp = v;
					} else if(svar == "fraction_fixed_combined") {
						fraction_fixed_combined = v;
					} else if(svar == "complex_number_form") {
						if(v == COMPLEX_NUMBER_FORM_CIS + 1) {
							if(mode_index == 1) {
								evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
								complex_angle_form = true;
							} else {
								modes[mode_index].eo.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
								modes[mode_index].complex_angle_form = true;
							}
						} else if(v >= COMPLEX_NUMBER_FORM_RECTANGULAR && v <= COMPLEX_NUMBER_FORM_CIS) {
							if(mode_index == 1) {
								evalops.complex_number_form = (ComplexNumberForm) v;
								complex_angle_form = false;
							} else {
								modes[mode_index].eo.complex_number_form = (ComplexNumberForm) v;
								modes[mode_index].complex_angle_form = false;
							}
						}
					} else if(svar == "number_base") {
						if(mode_index == 1) printops.base = v;
						else modes[mode_index].po.base = v;
					} else if(svar == "custom_number_base") {
						CALCULATOR->beginTemporaryStopMessages();
						MathStructure m;
						CALCULATOR->calculate(&m, svalue, 500);
						CALCULATOR->endTemporaryStopMessages();
						if(mode_index == 1) CALCULATOR->setCustomOutputBase(m.number());
						else modes[mode_index].custom_output_base = m.number();
					} else if(svar == "number_base_expression") {
						if(mode_index == 1) evalops.parse_options.base = v;
						else modes[mode_index].eo.parse_options.base = v;
					} else if(svar == "custom_number_base_expression") {
						CALCULATOR->beginTemporaryStopMessages();
						MathStructure m;
						CALCULATOR->calculate(&m, svalue, 500);
						CALCULATOR->endTemporaryStopMessages();
						if(mode_index == 1) CALCULATOR->setCustomInputBase(m.number());
						else modes[mode_index].custom_input_base = m.number();
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
						if(v >= STRUCTURING_NONE && v <= STRUCTURING_FACTORIZE) {
							if((v == STRUCTURING_NONE) && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 12))) {
								v = STRUCTURING_SIMPLIFY;
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
						if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_CUSTOM) {
							if(mode_index == 1) evalops.parse_options.angle_unit = (AngleUnit) v;
							else modes[mode_index].eo.parse_options.angle_unit = (AngleUnit) v;
						}
					} else if(svar == "custom_angle_unit") {
						if(mode_index == 1) custom_angle_unit = svalue;
						else modes[mode_index].custom_angle_unit = svalue;
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
					} else if(svar == "variable_units_enabled") {
						if(mode_index == 1) CALCULATOR->setVariableUnitsEnabled(v);
						else modes[mode_index].variable_units_enabled = v;
					} else if(svar == "calculate_functions") {
						if(mode_index == 1) evalops.calculate_functions = v;
						else modes[mode_index].eo.calculate_functions = v;
					} else if(svar == "sync_units") {
						if(mode_index == 1) evalops.sync_units = v;
						else modes[mode_index].eo.sync_units = v;
					} else if(svar == "temperature_calculation") {
						CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) v);
						tc_set = true;
					} else if(svar == "sinc_function") {
						if(v == 1) {
							CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "pi");
							sinc_set = true;
						} else if(v == 0) {
							sinc_set = true;
						}
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
					} else if(svar == "local_currency_conversion") {
						evalops.local_currency_conversion = v;
					} else if(svar == "use_binary_prefixes") {
						CALCULATOR->useBinaryPrefixes(v);
					} else if(svar == "indicate_infinite_series") {
						if(mode_index == 1) printops.indicate_infinite_series = v;
						else modes[mode_index].po.indicate_infinite_series = v;
					} else if(svar == "show_ending_zeroes") {
						if(version_numbers[0] > 2 || (version_numbers[0] == 2 && version_numbers[1] >= 9)) {
							if(mode_index == 1) printops.show_ending_zeroes = v;
							else modes[mode_index].po.show_ending_zeroes = v;
						}
					} else if(svar == "digit_grouping") {
						if(v >= DIGIT_GROUPING_NONE && v <= DIGIT_GROUPING_LOCALE) {
							printops.digit_grouping = (DigitGrouping) v;
						}
					} else if(svar == "round_halfway_to_even") {//obsolete
						if(v) {
							if(mode_index == 1) printops.rounding = ROUNDING_HALF_TO_EVEN;
							else modes[mode_index].po.rounding = ROUNDING_HALF_TO_EVEN;
						}
					} else if(svar == "rounding_mode") {
						if(v >= ROUNDING_HALF_AWAY_FROM_ZERO && v <= ROUNDING_DOWN) {
							if(!VERSION_AFTER(4, 9, 0) && v == 2) v = ROUNDING_TOWARD_ZERO;
							if(mode_index == 1) printops.rounding = (RoundingMode) v;
							else modes[mode_index].po.rounding = (RoundingMode) v;
						}
					} else if(svar == "always_exact") {//obsolete
						if(mode_index == 1) {
							evalops.approximation = APPROXIMATION_EXACT;
						} else {
							modes[mode_index].eo.approximation = APPROXIMATION_EXACT;
							modes[mode_index].interval = false;
						}
					} else if(svar == "approximation") {
						if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
							if(mode_index == 1) {
								evalops.approximation = (ApproximationMode) v;
							} else {
								modes[mode_index].eo.approximation = (ApproximationMode) v;
							}
						}
					} else if(svar == "interval_calculation") {
						if(v >= INTERVAL_CALCULATION_NONE && v <= INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
							if(mode_index == 1) evalops.interval_calculation = (IntervalCalculation) v;
							else modes[mode_index].eo.interval_calculation = (IntervalCalculation) v;
						}
					} else if(svar == "calculate_as_you_type") {
						if(mode_index == 1) auto_calculate = v;
						else modes[mode_index].autocalc = v;
					} else if(svar == "chain_mode") {
						if(mode_index == 1) chain_mode = v;
						else modes[mode_index].chain_mode = v;
					} else if(svar == "in_rpn_mode") {
						if(mode_index == 1) rpn_mode = v;
						else modes[mode_index].rpn_mode = v;
					} else if(svar == "rpn_keys") {
						rpn_keys = v;
					} else if(svar == "rpn_syntax") {
						if(v) {
							if(mode_index == 1) evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
							else modes[mode_index].eo.parse_options.parsing_mode = PARSING_MODE_RPN;
						}
					} else if(svar == "limit_implicit_multiplication") {
						if(mode_index == 1) {
							evalops.parse_options.limit_implicit_multiplication = v;
							printops.limit_implicit_multiplication = v;
						} else {
							modes[mode_index].eo.parse_options.limit_implicit_multiplication = v;
							modes[mode_index].po.limit_implicit_multiplication = v;
						}
					} else if(svar == "parsing_mode") {
						if((evalops.parse_options.parsing_mode != PARSING_MODE_RPN || version_numbers[0] > 3 || (version_numbers[0] == 3 && version_numbers[1] > 15)) && v >= PARSING_MODE_ADAPTIVE && v <= PARSING_MODE_RPN) {
							if(mode_index == 1) {
								evalops.parse_options.parsing_mode = (ParsingMode) v;
								if(evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) implicit_question_asked = true;
							} else {
								modes[mode_index].eo.parse_options.parsing_mode = (ParsingMode) v;
								if(modes[mode_index].eo.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || modes[mode_index].eo.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) implicit_question_asked = true;
							}
						}
					} else if(svar == "simplified_percentage") {
						if(v > 0 && !VERSION_AFTER(5, 0, 0)) v = -1;
						if(mode_index == 1) simplified_percentage = v;
						else modes[mode_index].simplified_percentage = v;
					} else if(svar == "implicit_question_asked") {
						if(mode_index == 1) implicit_question_asked = v;
						else modes[mode_index].implicit_question_asked = v;
					} else if(svar == "default_assumption_type") {
						if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_BOOLEAN) {
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
					} else if(svar == "visible_keypad") {
						if(mode_index == 1) visible_keypad = v;
						else modes[mode_index].keypad = v;
					} else if(svar == "use_unicode_signs" && (version_numbers[0] > 0 || version_numbers[1] > 7 || (version_numbers[1] == 7 && version_numbers[2] > 0))) {
						printops.use_unicode_signs = v;
					} else if(svar == "lower_case_numbers") {
						printops.lower_case_numbers = v;
					} else if(svar == "duodecimal_symbols") {
						printops.duodecimal_symbols = v;
					} else if(svar == "lower_case_e") {
						if(v) printops.exp_display = EXP_LOWERCASE_E;
					} else if(svar == "e_notation") {
						if(!v) printops.exp_display = EXP_POWER_OF_10;
						else if(printops.exp_display != EXP_LOWERCASE_E) printops.exp_display = EXP_UPPERCASE_E;
					} else if(svar == "exp_display") {
						if(v >= EXP_UPPERCASE_E && v <= EXP_POWER_OF_10) printops.exp_display = (ExpDisplay) v;
					} else if(svar == "imaginary_j") {
						do_imaginary_j = v;
					} else if(svar == "base_display") {
						if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_ALTERNATIVE) printops.base_display = (BaseDisplay) v;
					} else if(svar == "twos_complement") {
						printops.twos_complement = v;
					} else if(svar == "hexadecimal_twos_complement") {
						printops.hexadecimal_twos_complement = v;
					} else if(svar == "twos_complement_input") {
						evalops.parse_options.twos_complement = v;
					} else if(svar == "hexadecimal_twos_complement_input") {
						evalops.parse_options.hexadecimal_twos_complement = v;
					} else if(svar == "spell_out_logical_operators") {
						printops.spell_out_logical_operators = v;
					} else if(svar == "close_with_esc") {
						close_with_esc = v;
					} else if(svar == "caret_as_xor") {
						caret_as_xor = v;
					} else if(svar == "concise_uncertainty_input") {
						if(mode_index == 1) CALCULATOR->setConciseUncertaintyInputEnabled(v);
						else modes[mode_index].concise_uncertainty_input = v;
					} else if(svar == "copy_separator") {//obsolete
						copy_ascii = !v;
					} else if(svar == "copy_ascii") {
						copy_ascii = v;
					} else if(svar == "copy_ascii_without_units") {
						copy_ascii_without_units = v;
					} else if(svar == "decimal_comma") {
						b_decimal_comma = v;
						if(v == 0) CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
						else if(v > 0) CALCULATOR->useDecimalComma();
					} else if(svar == "dot_as_separator") {
						if(v < 0 || (CALCULATOR->default_dot_as_separator == v && (version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 18) || (version_numbers[0] == 3 && version_numbers[1] == 18 && version_numbers[2] < 1)))) {
							evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
							dot_question_asked = false;
						} else {
							evalops.parse_options.dot_as_separator = v;
							dot_question_asked = true;
						}
					} else if(svar == "comma_as_separator") {
						evalops.parse_options.comma_as_separator = v;
						if(CALCULATOR->getDecimalPoint() != COMMA) {
							CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
						}
					} else if(svar == "use_dark_theme") {
						if(v > 0) gtk_theme = 1;
					} else if(svar == "gtk_theme") {
						gtk_theme = v;
					} else if(svar == "horizontal_button_padding") {
						horizontal_button_padding = v;
					} else if(svar == "vertical_button_padding") {
						vertical_button_padding = v;
					} else if(svar == "use_custom_result_font") {
						use_custom_result_font = v;
					} else if(svar == "use_custom_expression_font") {
						use_custom_expression_font = v;
					} else if(svar == "use_custom_status_font") {
						use_custom_status_font = v;
					} else if(svar == "use_custom_keypad_font") {
						use_custom_keypad_font = v;
					} else if(svar == "use_custom_history_font") {
						use_custom_history_font = v;
					} else if(svar == "use_custom_application_font") {
						use_custom_app_font = v;
					} else if(svar == "custom_result_font") {
						custom_result_font = svalue;
						save_custom_result_font = true;
					} else if(svar == "custom_expression_font") {
						custom_expression_font = svalue;
						save_custom_expression_font = true;
					} else if(svar == "custom_status_font") {
						custom_status_font = svalue;
						save_custom_status_font = true;
					} else if(svar == "custom_keypad_font") {
						custom_keypad_font = svalue;
						save_custom_keypad_font = true;
					} else if(svar == "custom_history_font") {
						custom_history_font = svalue;
						save_custom_history_font = true;
					} else if(svar == "custom_application_font") {
						custom_app_font = svalue;
						save_custom_app_font = true;
					} else if(svar == "status_error_color") {
						status_error_color = svalue;
						status_error_color_set = true;
					} else if(svar == "text_color") {
						text_color = svalue;
						text_color_set = true;
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
						if(v >= PLOT_STYLE_LINES && v <= PLOT_STYLE_POLAR) default_plot_style = (PlotStyle) v;
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
					} else if(svar == "plot_complex") {
						default_plot_complex = v;
					} else if(svar == "plot_variable") {
						default_plot_variable = svalue;
					} else if(svar == "plot_rows") {
						default_plot_rows = v;
					} else if(svar == "plot_type") {
						default_plot_type = v;
					} else if(svar == "plot_color") {
						if(version_numbers[0] > 2 || (version_numbers[0] == 2 && (version_numbers[1] > 2 || (version_numbers[1] == 2 && version_numbers[2] > 1)))) {
							default_plot_color = v;
						}
					} else if(svar == "plot_linewidth") {
						default_plot_linewidth = v;
					} else if(svar == "max_plot_time") {
						max_plot_time = v;
					} else if(svar == "custom_button_label") {
						unsigned int index = 0;
						char str[20];
						int n = sscanf(svalue.c_str(), "%u:%19[^\n]", &index, str);
						if(n >= 2 && index < custom_buttons.size()) {
							custom_buttons[index].text = str;
							remove_blank_ends(custom_buttons[index].text);
						}
					} else if(svar == "custom_button") {
						unsigned int index = 0;
						unsigned int bi = 0;
						char str[20];
						int type = -1;
						int n = sscanf(svalue.c_str(), "%u:%u:%i:%19[^\n]", &index, &bi, &type, str);
						if(n >= 3 && index < custom_buttons.size()) {
							if(bi <= 2) {
								string value;
								if(n >= 4) {
									value = str;
									if(type != SHORTCUT_TYPE_TEXT) remove_blank_ends(value);
								}
								custom_buttons[index].type[bi] = type;
								custom_buttons[index].value[bi] = value;
							}
						}
					} else if(svar == "keyboard_shortcut") {
						default_shortcuts = false;
						int type = -1;
						guint key, modifier;
						int n = sscanf(svalue.c_str(), "%u:%u:%i:%999999[^\n]", &key, &modifier, &type, line);
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 9) || (version_numbers[0] == 3 && version_numbers[1] == 9 && version_numbers[2] < 1)) {
							if(type >= SHORTCUT_TYPE_DEGREES) type += 3;
						}
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 9) || (version_numbers[0] == 3 && version_numbers[1] == 9 && version_numbers[2] < 2)) {
							if(type >= SHORTCUT_TYPE_HISTORY_SEARCH) type++;
						}
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 9)) {
							if(type >= SHORTCUT_TYPE_MINIMAL) type++;
						}
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && (version_numbers[1] < 13 || (version_numbers[1] == 13 && version_numbers[2] == 0)))) {
							if(type >= SHORTCUT_TYPE_MEMORY_CLEAR) type += 5;
						}
						if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 8)) {
							if(type >= SHORTCUT_TYPE_FLOATING_POINT) type++;
						}
						if(n >= 3 && type >= SHORTCUT_TYPE_FUNCTION && type <= LAST_SHORTCUT_TYPE) {
							string value;
							if(n == 4) {
								value = line;
								if(type != SHORTCUT_TYPE_TEXT) remove_blank_ends(value);
							}
							unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find((guint64) key + (guint64) G_MAXUINT32 * (guint64) modifier);
							if(it != keyboard_shortcuts.end()) {
								it->second.type.push_back(type);
								it->second.value.push_back(value);
							} else {
								keyboard_shortcut ks;
								ks.type.push_back(type);
								ks.value.push_back(value);
								ks.key = key;
								ks.modifier = modifier;
								keyboard_shortcuts[(guint64) key + (guint64) G_MAXUINT32 * (guint64) modifier] = ks;
							}
						}
					} else if(svar == "expression_history") {
						expression_history.push_back(svalue);
					} else if(svar == "history") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_OLD);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_old") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_OLD);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_expression") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_EXPRESSION);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_expression*") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_EXPRESSION);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(true);
						inhistory_value.push_front(0);
					} else if(svar == "history_transformation") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_TRANSFORMATION);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_result") {
						if(VERSION_BEFORE(4, 1, 1) && svalue.length() > 20 && svalue.substr(svalue.length() - 4, 4) == " …" && (unsigned char) svalue[svalue.length() - 5] >= 0xC0) svalue.erase(svalue.length() - 5, 1);
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_RESULT);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_result_approximate") {
						if(VERSION_BEFORE(4, 1, 1) && svalue.length() > 20 && svalue.substr(svalue.length() - 4, 4) == " …" && (unsigned char) svalue[svalue.length() - 5] >= 0xC0) svalue.erase(svalue.length() - 5, 1);
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_RESULT_APPROXIMATE);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_parse") {
						inhistory.push_front(svalue);
						if(old_history_format) inhistory_type.push_front(QALCULATE_HISTORY_PARSE_WITHEQUALS);
						else inhistory_type.push_front(QALCULATE_HISTORY_PARSE);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_parse_withequals") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_PARSE_WITHEQUALS);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_parse_approximate") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_PARSE_APPROXIMATE);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_register_moved") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_REGISTER_MOVED);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_rpn_operation") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_RPN_OPERATION);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_register_moved*") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_REGISTER_MOVED);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(true);
						inhistory_value.push_front(0);
					} else if(svar == "history_rpn_operation*") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_RPN_OPERATION);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(true);
						inhistory_value.push_front(0);
					} else if(svar == "history_warning") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_WARNING);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_message") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_MESSAGE);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_error") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_ERROR);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
					} else if(svar == "history_bookmark") {
						inhistory.push_front(svalue);
						inhistory_type.push_front(QALCULATE_HISTORY_BOOKMARK);
						inhistory_time.push_front(history_time);
						inhistory_protected.push_front(false);
						inhistory_value.push_front(0);
						bool b = false;
						bookmark_index = 0;
						for(vector<string>::iterator it = history_bookmarks.begin(); it != history_bookmarks.end(); ++it) {
							if(string_is_less(svalue, *it)) {
								history_bookmarks.insert(it, svalue);
								b = true;
								break;
							}
							bookmark_index++;
						}
						if(!b) history_bookmarks.push_back(svalue);
					} else if(svar == "history_continued") {
						if(inhistory.size() > 0) {
							inhistory[0] += "\n";
							inhistory[0] += svalue;
							if(inhistory_type[0] == QALCULATE_HISTORY_BOOKMARK) {
								history_bookmarks[bookmark_index] += "\n";
								history_bookmarks[bookmark_index] += svalue;
							}
						}
					} else if(svar == "history_time") {
						history_time = (time_t) strtoll(svalue.c_str(), NULL, 10);
					}
				} else if(stmp.length() > 2 && stmp[0] == '[' && stmp[stmp.length() - 1] == ']') {
					stmp = stmp.substr(1, stmp.length() - 2);
					remove_blank_ends(stmp);
					if(stmp == "Mode") {
						mode_index = 1;
					} else if(stmp.length() > 5 && stmp.substr(0, 4) == "Mode") {
						mode_index = save_mode_as(stmp.substr(5, stmp.length() - 5));
						modes[mode_index].implicit_question_asked = false;
					}
				}
			}
			fclose(file);
			if(file_i == 1) {
				if(gstr_oldfile) {
					recursiveMakeDir(getLocalDir());
					move_file(gstr_oldfile, gstr_file);
					g_free(gstr_oldfile);
				}
				if(!save_history_separately) break;
				gchar *gstr_file2 = g_build_filename(getLocalDir().c_str(), "qalculate-gtk.history", NULL);
				file = fopen(gstr_file2, "r");
				g_free(gstr_file2);
				if(!file) break;
			}
		}
	} else {
		first_time = true;
	}
	if(default_shortcuts) {
		keyboard_shortcut ks;
		ks.type.push_back(0);
		ks.value.push_back("");
#define ADD_SHORTCUT(k, m, t, v) ks.key = k; ks.modifier = m; ks.type[0] = t; ks.value[0] = v; keyboard_shortcuts[(guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier] = ks;
		ADD_SHORTCUT(GDK_KEY_b, GDK_CONTROL_MASK, SHORTCUT_TYPE_NUMBER_BASES, "")
		ADD_SHORTCUT(GDK_KEY_q, GDK_CONTROL_MASK, SHORTCUT_TYPE_QUIT, "")
		ADD_SHORTCUT(GDK_KEY_F1, 0, SHORTCUT_TYPE_HELP, "")
		ADD_SHORTCUT(GDK_KEY_c, GDK_CONTROL_MASK | GDK_MOD1_MASK, SHORTCUT_TYPE_COPY_RESULT, "")
		ADD_SHORTCUT(GDK_KEY_s, GDK_CONTROL_MASK, SHORTCUT_TYPE_STORE, "")
		ADD_SHORTCUT(GDK_KEY_m, GDK_CONTROL_MASK, SHORTCUT_TYPE_MANAGE_VARIABLES, "")
		ADD_SHORTCUT(GDK_KEY_f, GDK_CONTROL_MASK, SHORTCUT_TYPE_MANAGE_FUNCTIONS, "")
		ADD_SHORTCUT(GDK_KEY_u, GDK_CONTROL_MASK, SHORTCUT_TYPE_MANAGE_UNITS, "")
		ADD_SHORTCUT(GDK_KEY_k, GDK_CONTROL_MASK, SHORTCUT_TYPE_KEYPAD, "")
		ADD_SHORTCUT(GDK_KEY_k, GDK_MOD1_MASK, SHORTCUT_TYPE_KEYPAD, "")
		ADD_SHORTCUT(GDK_KEY_h, GDK_CONTROL_MASK, SHORTCUT_TYPE_HISTORY, "")
		ADD_SHORTCUT(GDK_KEY_h, GDK_MOD1_MASK, SHORTCUT_TYPE_HISTORY, "")
		ADD_SHORTCUT(GDK_KEY_space, GDK_CONTROL_MASK, SHORTCUT_TYPE_MINIMAL, "")
		ADD_SHORTCUT(GDK_KEY_o, GDK_CONTROL_MASK, SHORTCUT_TYPE_CONVERSION, "")
		ADD_SHORTCUT(GDK_KEY_o, GDK_MOD1_MASK, SHORTCUT_TYPE_CONVERSION, "")
		ADD_SHORTCUT(GDK_KEY_t, GDK_CONTROL_MASK, SHORTCUT_TYPE_CONVERT_ENTRY, "")
		ADD_SHORTCUT(GDK_KEY_p, GDK_CONTROL_MASK, SHORTCUT_TYPE_PROGRAMMING, "")
		ADD_SHORTCUT(GDK_KEY_r, GDK_CONTROL_MASK, SHORTCUT_TYPE_RPN_MODE, "")
		ADD_SHORTCUT(GDK_KEY_parenright, GDK_CONTROL_MASK | GDK_SHIFT_MASK, SHORTCUT_TYPE_SMART_PARENTHESES, "")
		ADD_SHORTCUT(GDK_KEY_parenleft, GDK_CONTROL_MASK | GDK_SHIFT_MASK, SHORTCUT_TYPE_SMART_PARENTHESES, "")
		ADD_SHORTCUT(GDK_KEY_Up, GDK_CONTROL_MASK, SHORTCUT_TYPE_RPN_UP, "")
		ADD_SHORTCUT(GDK_KEY_Down, GDK_CONTROL_MASK, SHORTCUT_TYPE_RPN_DOWN, "")
		ADD_SHORTCUT(GDK_KEY_Right, GDK_CONTROL_MASK, SHORTCUT_TYPE_RPN_SWAP, "")
		ADD_SHORTCUT(GDK_KEY_Left, GDK_CONTROL_MASK, SHORTCUT_TYPE_RPN_LASTX, "")
		ADD_SHORTCUT(GDK_KEY_c, GDK_CONTROL_MASK | GDK_SHIFT_MASK, SHORTCUT_TYPE_RPN_COPY, "")
		ADD_SHORTCUT(GDK_KEY_Delete, GDK_CONTROL_MASK, SHORTCUT_TYPE_RPN_DELETE, "")
		ADD_SHORTCUT(GDK_KEY_Delete, GDK_CONTROL_MASK | GDK_SHIFT_MASK, SHORTCUT_TYPE_RPN_CLEAR, "")
		ADD_SHORTCUT(GDK_KEY_Tab, 0, SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION, "")
	} else if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 19)) {
		keyboard_shortcut ks;
		ks.key = GDK_KEY_Tab; ks.modifier = 0; ks.type.push_back(SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION); ks.value.push_back("");
		if(keyboard_shortcuts.find((guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier) == keyboard_shortcuts.end()) {
			keyboard_shortcuts[(guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier] = ks;
		}
		if(version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 9)) {
			ks.key = GDK_KEY_space; ks.modifier = GDK_CONTROL_MASK; ks.type[0] = SHORTCUT_TYPE_MINIMAL; ks.value[0] = "";
			if(keyboard_shortcuts.find((guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier) == keyboard_shortcuts.end()) {
				keyboard_shortcuts[(guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier] = ks;
			}
		}
	}
	if(show_keypad && !(visible_keypad & HIDE_RIGHT_KEYPAD) && !(visible_keypad & HIDE_LEFT_KEYPAD) && (version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 15))) win_width = -1;
#ifdef _WIN32
	else if(!(visible_keypad & HIDE_RIGHT_KEYPAD) && !(visible_keypad & HIDE_LEFT_KEYPAD) && (version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] < 19))) win_width -= 84;
#endif
	if(!VERSION_AFTER(5, 0, 0) && !(visible_keypad & PROGRAMMING_KEYPAD)) {
		evalops.parse_options.twos_complement = false;
		evalops.parse_options.hexadecimal_twos_complement = false;
		printops.binary_bits = 0;
		evalops.parse_options.binary_bits = 0;
	}
	update_message_print_options();
	displayed_printops = printops;
	displayed_printops.allow_non_usable = true;
	displayed_caf = complex_angle_form;
	initial_inhistory_index = inhistory.size() - 1;
	g_free(gstr_file);
	show_history = show_history && (persistent_keypad || !show_keypad);
	show_convert = show_convert && !show_history && (persistent_keypad || !show_keypad);
	set_saved_mode();

}

void save_history_to_file(FILE *file) {
	if(!clear_history_on_exit) {
		for(size_t i = 0; i < expression_history.size(); i++) {
			gsub("\n", " ", expression_history[i]);
			fprintf(file, "expression_history=%s\n", expression_history[i].c_str());
		}
	}

	int lines = max_history_lines;
	bool end_after_result = false, end_before_expression = false;
	bool is_protected = false;
	bool is_bookmarked = false;
	bool doend = false;
	size_t hi = inhistory.size();
	time_t history_time_prev = 0;
	while(!doend && hi > 0) {
		hi--;
		if(inhistory_time[hi] != history_time_prev) {
			history_time_prev = inhistory_time[hi];
			fprintf(file, "history_time=%lli\n", (long long int) history_time_prev);
		}
		switch(inhistory_type[hi]) {
			case QALCULATE_HISTORY_EXPRESSION: {
				if(end_before_expression) {
					doend = true;
				} else {
					if(inhistory_protected[hi]) fprintf(file, "history_expression*=");
					else if(!clear_history_on_exit || is_bookmarked) fprintf(file, "history_expression=");
					is_protected = inhistory_protected[hi] || is_bookmarked;
					is_bookmarked = false;
				}
				break;
			}
			case QALCULATE_HISTORY_TRANSFORMATION: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_transformation=");
				break;
			}
			case QALCULATE_HISTORY_RESULT: {
				if(end_after_result) doend = true;
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_result=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_RESULT_APPROXIMATE: {
				if(end_after_result) doend = true;
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_result_approximate=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_PARSE: {}
			case QALCULATE_HISTORY_PARSE_WITHEQUALS: {}
			case QALCULATE_HISTORY_PARSE_APPROXIMATE: {
				if(clear_history_on_exit && !is_protected) break;
				if(inhistory_type[hi] == QALCULATE_HISTORY_PARSE) fprintf(file, "history_parse=");
				else if(inhistory_type[hi] == QALCULATE_HISTORY_PARSE_WITHEQUALS) fprintf(file, "history_parse_withequals=");
				else fprintf(file, "history_parse_approximate=");
				lines--;
				if(lines < 0) {
					if(hi + 1 < inhistory_protected.size() && inhistory_protected[hi + 1]) {
						end_before_expression = true;
					} else if(hi + 2 < inhistory_type.size() && inhistory_type[hi + 2] == QALCULATE_HISTORY_BOOKMARK) {
						end_before_expression = true;
					}
					if(!end_before_expression) end_after_result = true;
				}
				break;
			}
			case QALCULATE_HISTORY_REGISTER_MOVED: {
				if(end_before_expression) {
					doend = true;
				} else {
					if(inhistory_protected[hi]) fprintf(file, "history_register_moved*=");
					else if(!clear_history_on_exit || is_bookmarked) fprintf(file, "history_register_moved=");
					is_protected = inhistory_protected[hi] || is_bookmarked;
					is_bookmarked = false;
				}
				break;
			}
			case QALCULATE_HISTORY_RPN_OPERATION: {
				if(end_before_expression) {
					doend = true;
				} else {
					if(inhistory_protected[hi]) fprintf(file, "history_rpn_operation*=");
					else if(!clear_history_on_exit || is_bookmarked) fprintf(file, "history_rpn_operation=");
					is_protected = inhistory_protected[hi] || is_bookmarked;
					is_bookmarked = false;
				}
				break;
			}
			case QALCULATE_HISTORY_WARNING: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_warning=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_MESSAGE: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_message=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_ERROR: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_error=");
				lines--;
				break;
			}
			case QALCULATE_HISTORY_BOOKMARK: {
				if(end_before_expression && hi > 0 && (inhistory_type[hi - 1] == QALCULATE_HISTORY_EXPRESSION || inhistory_type[hi - 1] == QALCULATE_HISTORY_REGISTER_MOVED || inhistory_type[hi - 1] == QALCULATE_HISTORY_RPN_OPERATION)) {
					doend = true;
				} else {
					fprintf(file, "history_bookmark=");
					is_bookmarked = true;
					is_protected = true;
					lines--;
					break;
				}
			}
			case QALCULATE_HISTORY_OLD: {
				if(clear_history_on_exit && !is_protected) break;
				fprintf(file, "history_old=");
				lines--;
				if(lines < 0) doend = true;
				break;
			}
		}
		if(doend && end_before_expression) break;
		if(!clear_history_on_exit || is_protected) {
			size_t i3 = inhistory[hi].find('\n');
			if(i3 == string::npos) {
				if(!is_protected && inhistory[hi].length() > 5000) {
					int index = 50;
					while(index > 0 && (signed char) inhistory[hi][index] < 0 && (unsigned char) inhistory[hi][index + 1] < 0xC0) index--;
					fprintf(file, "%s …\n", fix_history_string(unhtmlize(inhistory[hi].substr(0, index + 1))).c_str());
				} else {
					fprintf(file, "%s\n", inhistory[hi].c_str());
					if(inhistory[hi].length() > 300) {
						if(inhistory[hi].length() > 9000) lines -= 30;
						else lines -= inhistory[hi].length() / 300;
					}
				}
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
	}
	while(hi >= 0 && inhistory.size() > 0) {
		if(inhistory_protected[hi] || (inhistory_type[hi] == QALCULATE_HISTORY_BOOKMARK && hi != 0 && inhistory_type[hi - 1] != QALCULATE_HISTORY_OLD)) {
			bool b_first = true;
			if(inhistory_time[hi] != history_time_prev) {
				history_time_prev = inhistory_time[hi];
				fprintf(file, "history_time=%lli\n", (long long int) history_time_prev);
			}
			while(hi >= 0) {
				bool do_end = false;
				switch(inhistory_type[hi]) {
					case QALCULATE_HISTORY_EXPRESSION: {
						if(!b_first) {
							do_end = true;
						} else {
							if(inhistory_protected[hi]) fprintf(file, "history_expression*=");
							else fprintf(file, "history_expression=");
							b_first = false;
						}
						break;
					}
					case QALCULATE_HISTORY_TRANSFORMATION: {
						fprintf(file, "history_transformation=");
						break;
					}
					case QALCULATE_HISTORY_RESULT: {
						fprintf(file, "history_result=");
						break;
					}
					case QALCULATE_HISTORY_RESULT_APPROXIMATE: {
						fprintf(file, "history_result_approximate=");
						break;
					}
					case QALCULATE_HISTORY_PARSE: {
						fprintf(file, "history_parse=");
						break;
					}
					case QALCULATE_HISTORY_PARSE_WITHEQUALS: {
						fprintf(file, "history_parse_withequals=");
						break;
					}
					case QALCULATE_HISTORY_PARSE_APPROXIMATE: {
						fprintf(file, "history_parse_approximate=");
						break;
					}
					case QALCULATE_HISTORY_REGISTER_MOVED: {
						if(!b_first) {
							do_end = true;
						} else {
							if(inhistory_protected[hi]) fprintf(file, "history_register_moved*=");
							else fprintf(file, "history_register_moved=");
							b_first = false;
						}
						break;
					}
					case QALCULATE_HISTORY_RPN_OPERATION: {
						if(!b_first) {
							do_end = true;
						} else {
							if(inhistory_protected[hi]) fprintf(file, "history_rpn_operation*=");
							else fprintf(file, "history_rpn_operation=");
							b_first = false;
						}
						break;
					}
					case QALCULATE_HISTORY_WARNING: {
						fprintf(file, "history_warning=");
						break;
					}
					case QALCULATE_HISTORY_MESSAGE: {
						fprintf(file, "history_message=");
						break;
					}
					case QALCULATE_HISTORY_ERROR: {
						fprintf(file, "history_error=");
						break;
					}
					case QALCULATE_HISTORY_BOOKMARK: {
						if(!b_first) {
							do_end = true;
							break;
						}
						fprintf(file, "history_bookmark=");
						break;
					}
					case QALCULATE_HISTORY_OLD: {
						do_end = true;
						break;
					}
				}
				if(do_end) {
					hi++;
					break;
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
						i3 = i2 + 1;
						i2 = inhistory[hi].find('\n', i3);
					}
					fprintf(file, "history_continued=%s\n", inhistory[hi].substr(i3, inhistory[hi].length() - i3).c_str());
				}
				if(hi == 0) break;
				hi--;
			}
			if(hi > inhistory_type.size()) break;
		}
		if(hi == 0) break;
		hi--;
	}
}

bool save_history(bool allow_cancel) {
	if(!save_history_separately) return true;
	FILE *file = NULL;
	string homedir = getLocalDir();
	recursiveMakeDir(homedir);
	gchar *gstr2 = g_build_filename(homedir.c_str(), "qalculate-gtk.history", NULL);
	file = fopen(gstr2, "w+");
	if(file == NULL) {
		GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, _("Couldn't write history to\n%s"), gstr2);
		if(allow_cancel) {
			gtk_dialog_add_buttons(GTK_DIALOG(edialog), _("Ignore"), GTK_RESPONSE_CLOSE, _("Cancel"), GTK_RESPONSE_CANCEL, _("Retry"), GTK_RESPONSE_APPLY, NULL);
		} else {
			gtk_dialog_add_buttons(GTK_DIALOG(edialog), _("Ignore"), GTK_RESPONSE_CLOSE, _("Retry"), GTK_RESPONSE_APPLY, NULL);
		}
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		int ret = gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
		g_free(gstr2);
		if(ret == GTK_RESPONSE_CANCEL) return false;
		if(ret == GTK_RESPONSE_APPLY) return save_history(allow_cancel);
		return true;
	}
	g_free(gstr2);

	save_history_to_file(file);

	fclose(file);

	return true;

}

/*
	save preferences to ~/.config/qalculate/qalculate-gtk.cfg
	set mode to true to save current calculator mode
*/

bool save_preferences(bool mode, bool allow_cancel) {

	FILE *file = NULL;
	string homedir = getLocalDir();
	recursiveMakeDir(homedir);
	gchar *gstr2 = g_build_filename(homedir.c_str(), "qalculate-gtk.cfg", NULL);
	file = fopen(gstr2, "w+");
	if(file == NULL) {
#ifndef _WIN32
		GStatBuf stat;
		if(g_lstat(gstr2, &stat) == 0 && S_ISLNK(stat.st_mode)) {
			g_free(gstr2);
			return true;
		}
#endif
		GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, _("Couldn't write preferences to\n%s"), gstr2);
		if(allow_cancel) {
			gtk_dialog_add_buttons(GTK_DIALOG(edialog), _("Ignore"), GTK_RESPONSE_CLOSE, _("Cancel"), GTK_RESPONSE_CANCEL, _("Retry"), GTK_RESPONSE_APPLY, NULL);
		} else {
			gtk_dialog_add_buttons(GTK_DIALOG(edialog), _("Ignore"), GTK_RESPONSE_CLOSE, _("Retry"), GTK_RESPONSE_APPLY, NULL);
		}
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		int ret = gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
		g_free(gstr2);
		if(ret == GTK_RESPONSE_CANCEL) return false;
		if(ret == GTK_RESPONSE_APPLY) return save_preferences(mode, allow_cancel);
		return true;
	}
	g_free(gstr2);
	gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), FALSE);
	gint w, h;
	update_variables_settings();
	update_units_settings();
	update_functions_settings();
	update_datasets_settings();
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", "5.0.1");
	fprintf(file, "allow_multiple_instances=%i\n", allow_multiple_instances);
	if(title_type != TITLE_APP) fprintf(file, "window_title_mode=%i\n", title_type);
	if(minimal_width > 0 && minimal_mode) {
		fprintf(file, "width=%i\n", win_width);
	} else {
		gtk_window_get_size(GTK_WINDOW(mainwindow), &w, &h);
		fprintf(file, "width=%i\n", w);
	}
	//fprintf(file, "height=%i\n", h);
	if(remember_position) {
		if(hidden_x >= 0 && !gtk_widget_is_visible(mainwindow)) {
			fprintf(file, "monitor=%i\n", hidden_monitor);
			fprintf(file, "monitor_primary=%i\n", hidden_monitor_primary);
			fprintf(file, "x=%i\n", hidden_x);
			fprintf(file, "y=%i\n", hidden_y);
		} else {
			gtk_window_get_position(GTK_WINDOW(mainwindow), &win_x, &win_y);
			GdkDisplay *display = gtk_widget_get_display(mainwindow);
			win_monitor = 1;
			win_monitor_primary = false;
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
			GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(mainwindow));
			if(monitor) {
				int n = gdk_display_get_n_monitors(display);
				if(n > 1) {
					for(int i = 0; i < n; i++) {
						if(monitor == gdk_display_get_monitor(display, i)) {
							win_monitor = i + 1;
							break;
						}
					}
				}
				GdkRectangle area;
				gdk_monitor_get_workarea(monitor, &area);
				win_x -= area.x;
				win_y -= area.y;
				win_monitor_primary = gdk_monitor_is_primary(monitor);
			}
#else
			GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(mainwindow));
			if(screen) {
				int i = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(mainwindow));
				if(i >= 0) {
					win_monitor_primary = (i == gdk_screen_get_primary_monitor(screen));
					GdkRectangle area;
					gdk_screen_get_monitor_workarea(screen, i, &area);
					win_monitor = i + 1;
					win_x -= area.x;
					win_y -= area.y;
				}
			}
#endif
			fprintf(file, "monitor=%i\n", win_monitor);
			fprintf(file, "monitor_primary=%i\n", win_monitor_primary);
			fprintf(file, "x=%i\n", win_x);
			fprintf(file, "y=%i\n", win_y);
		}
	}
	fprintf(file, "always_on_top=%i\n", always_on_top);
	fprintf(file, "enable_tooltips=%i\n", enable_tooltips);
#ifdef _WIN32
	fprintf(file, "use_system_tray_icon=%i\n", use_systray_icon);
	fprintf(file, "hide_on_startup=%i\n", hide_on_startup);
#else
	if(hide_on_startup) fprintf(file, "hide_on_startup=%i\n", hide_on_startup);
#endif
	if(variables_height > -1) fprintf(file, "variables_height=%i\n", variables_height);
	if(variables_width > -1) fprintf(file, "variables_width=%i\n", variables_width);
	if(variables_height > -1) fprintf(file, "variables_height=%i\n", variables_height);
	if(variables_hposition > -1) fprintf(file, "variables_hpanel_position=%i\n", variables_hposition);
	if(variables_vposition > -1) fprintf(file, "variables_vpanel_position=%i\n", variables_vposition);
	if(units_width > -1) fprintf(file, "units_width=%i\n", units_width);
	if(units_height > -1) fprintf(file, "units_height=%i\n", units_height);
	if(units_hposition > -1) fprintf(file, "units_hpanel_position=%i\n", units_hposition);
	if(units_vposition > -1) fprintf(file, "units_vpanel_position=%i\n", units_vposition);
	if(functions_width > -1) fprintf(file, "functions_width=%i\n", functions_width);
	if(functions_height > -1) fprintf(file, "functions_height=%i\n", functions_height);
	if(functions_hposition > -1) fprintf(file, "functions_hpanel_position=%i\n", functions_hposition);
	if(functions_vposition > -1) fprintf(file, "functions_vpanel_position=%i\n", functions_vposition);
	if(datasets_width > -1) fprintf(file, "datasets_width=%i\n", datasets_width);
	if(datasets_height > -1) fprintf(file, "datasets_height=%i\n", datasets_height);
	if(datasets_hposition > -1) fprintf(file, "datasets_hpanel_position=%i\n", datasets_hposition);
	if(datasets_vposition1 > -1) fprintf(file, "datasets_vpanel1_position=%i\n", datasets_vposition1);
	if(datasets_vposition2 > -1) fprintf(file, "datasets_vpanel2_position=%i\n", datasets_vposition2);
	if(help_width != -1) fprintf(file, "help_width=%i\n", help_width);
	if(help_height != -1) fprintf(file, "help_height=%i\n", help_height);
	if(help_zoom >= 0.0) fprintf(file, "help_zoom=%f\n", help_zoom);
	fprintf(file, "keep_function_dialog_open=%i\n", keep_function_dialog_open);
	fprintf(file, "error_info_shown=%i\n", !first_error);
	fprintf(file, "save_mode_on_exit=%i\n", save_mode_on_exit);
	fprintf(file, "save_definitions_on_exit=%i\n", save_defs_on_exit);
	fprintf(file, "clear_history_on_exit=%i\n", clear_history_on_exit);
	if(max_history_lines != 300) fprintf(file, "max_history_lines=%i\n", max_history_lines);
	fprintf(file, "save_history_separately=%i\n", save_history_separately);
	fprintf(file, "history_expression_type=%i\n", history_expression_type);
	if(!custom_lang.empty()) fprintf(file, "language=%s\n", custom_lang.c_str());
	fprintf(file, "ignore_locale=%i\n", ignore_locale);
	fprintf(file, "load_global_definitions=%i\n", load_global_defs);
	//fprintf(file, "fetch_exchange_rates_at_startup=%i\n", fetch_exchange_rates_at_startup);
	fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
	fprintf(file, "local_currency_conversion=%i\n", evalops.local_currency_conversion);
	fprintf(file, "use_binary_prefixes=%i\n", CALCULATOR->usesBinaryPrefixes());
	fprintf(file, "check_version=%i\n", check_version);
	if(check_version) {
		fprintf(file, "last_version_check=%s\n", last_version_check_date.toISOString().c_str());
		if(!last_found_version.empty()) fprintf(file, "last_found_version=%s\n", last_found_version.c_str());
	}
	fprintf(file, "show_keypad=%i\n", (rpn_mode && show_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) || gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad)));
	fprintf(file, "show_history=%i\n", (rpn_mode && show_history && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) || gtk_expander_get_expanded(GTK_EXPANDER(expander_history)));
	h = gtk_widget_get_allocated_height(tabs);
	fprintf(file, "history_height=%i\n", h > 10 ? h : history_height);
	if(minimal_window_resized_timeout_id) {
		g_source_remove(minimal_window_resized_timeout_id);
		minimal_window_resized_timeout_id = 0;
		update_minimal_width();
	}
	if(minimal_width > 0) fprintf(file, "minimal_width=%i\n", minimal_width);
	fprintf(file, "show_stack=%i\n", rpn_mode ? gtk_expander_get_expanded(GTK_EXPANDER(expander_stack)) : show_stack);
	fprintf(file, "show_convert=%i\n", (rpn_mode && show_convert && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))) || gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)));
	fprintf(file, "persistent_keypad=%i\n", persistent_keypad);
	fprintf(file, "minimal_mode=%i\n", minimal_mode);
	fprintf(file, "show_bases_keypad=%i\n", show_bases_keypad);
	fprintf(file, "continuous_conversion=%i\n", continuous_conversion);
	fprintf(file, "set_missing_prefixes=%i\n", set_missing_prefixes);
	fprintf(file, "rpn_keys=%i\n", rpn_keys);
	if(expression_lines > 0) fprintf(file, "expression_lines=%i\n", expression_lines);
	fprintf(file, "display_expression_status=%i\n", display_expression_status);
	fprintf(file, "parsed_expression_in_resultview=%i\n", parsed_in_result);
	fprintf(file, "enable_completion=%i\n", enable_completion);
	fprintf(file, "enable_completion2=%i\n", enable_completion2);
	fprintf(file, "completion_min=%i\n", completion_min);
	fprintf(file, "completion_min2=%i\n", completion_min2);
	fprintf(file, "completion_delay=%i\n", completion_delay);
	fprintf(file, "calculate_as_you_type_history_delay=%i\n", autocalc_history_delay);
	fprintf(file, "use_unicode_signs=%i\n", printops.use_unicode_signs);
	fprintf(file, "lower_case_numbers=%i\n", printops.lower_case_numbers);
	fprintf(file, "duodecimal_symbols=%i\n", printops.duodecimal_symbols);
	fprintf(file, "exp_display=%i\n", printops.exp_display);
	fprintf(file, "imaginary_j=%i\n", CALCULATOR->v_i->hasName("j") > 0);
	fprintf(file, "base_display=%i\n", printops.base_display);
	if(printops.binary_bits != 0) fprintf(file, "binary_bits=%i\n", printops.binary_bits);
	fprintf(file, "twos_complement=%i\n", printops.twos_complement);
	fprintf(file, "hexadecimal_twos_complement=%i\n", printops.hexadecimal_twos_complement);
	fprintf(file, "twos_complement_input=%i\n", evalops.parse_options.twos_complement);
	fprintf(file, "hexadecimal_twos_complement_input=%i\n", evalops.parse_options.hexadecimal_twos_complement);
	if(~visible_keypad & PROGRAMMING_KEYPAD && programming_outbase != 0 && programming_inbase != 0) {
		fprintf(file, "programming_outbase=%i\n", programming_outbase);
		fprintf(file, "programming_inbase=%i\n", programming_inbase);
	}
	if(evalops.parse_options.binary_bits > 0) fprintf(file, "binary_bits=%i\n", evalops.parse_options.binary_bits);
	if(visible_keypad & PROGRAMMING_KEYPAD && versatile_exact) {
		fprintf(file, "general_exact=%i\n", versatile_exact);
	}
	if(default_bits >= 0) fprintf(file, "bit_width=%i\n", default_bits);
	if(default_signed >= 0) fprintf(file, "signed_integer=%i\n", default_signed);
	fprintf(file, "spell_out_logical_operators=%i\n", printops.spell_out_logical_operators);
	fprintf(file, "caret_as_xor=%i\n", caret_as_xor);
	fprintf(file, "close_with_esc=%i\n", close_with_esc);
	fprintf(file, "digit_grouping=%i\n", printops.digit_grouping);
	fprintf(file, "copy_ascii=%i\n", copy_ascii);
	fprintf(file, "copy_ascii_without_units=%i\n", copy_ascii_without_units);
	fprintf(file, "decimal_comma=%i\n", b_decimal_comma);
	fprintf(file, "dot_as_separator=%i\n", dot_question_asked ? evalops.parse_options.dot_as_separator : -1);
	fprintf(file, "comma_as_separator=%i\n", evalops.parse_options.comma_as_separator);
	if(previous_precision > 0) fprintf(file, "previous_precision=%i\n", previous_precision);
	if(gtk_theme >= 0) fprintf(file, "gtk_theme=%i\n", gtk_theme);
	fprintf(file, "vertical_button_padding=%i\n", vertical_button_padding);
	fprintf(file, "horizontal_button_padding=%i\n", horizontal_button_padding);
	fprintf(file, "use_custom_result_font=%i\n", use_custom_result_font);
	fprintf(file, "use_custom_expression_font=%i\n", use_custom_expression_font);
	fprintf(file, "use_custom_status_font=%i\n", use_custom_status_font);
	fprintf(file, "use_custom_keypad_font=%i\n", use_custom_keypad_font);
	fprintf(file, "use_custom_history_font=%i\n", use_custom_history_font);
	fprintf(file, "use_custom_application_font=%i\n", use_custom_app_font);
	if(use_custom_result_font || save_custom_result_font) fprintf(file, "custom_result_font=%s\n", custom_result_font.c_str());
	if(use_custom_expression_font || save_custom_expression_font) fprintf(file, "custom_expression_font=%s\n", custom_expression_font.c_str());
	if(use_custom_status_font || save_custom_status_font) fprintf(file, "custom_status_font=%s\n", custom_status_font.c_str());
	if(use_custom_keypad_font || save_custom_keypad_font) fprintf(file, "custom_keypad_font=%s\n", custom_keypad_font.c_str());
	if(use_custom_history_font || save_custom_history_font) fprintf(file, "custom_history_font=%s\n", custom_history_font.c_str());
	if(use_custom_app_font || save_custom_app_font) fprintf(file, "custom_application_font=%s\n", custom_app_font.c_str());
	if(status_error_color_set) fprintf(file, "status_error_color=%s\n", status_error_color.c_str());
	if(status_warning_color_set) fprintf(file, "status_warning_color=%s\n", status_warning_color.c_str());
	if(text_color_set) fprintf(file, "text_color=%s\n", text_color.c_str());
	fprintf(file, "multiplication_sign=%i\n", printops.multiplication_sign);
	fprintf(file, "division_sign=%i\n", printops.division_sign);
	if(automatic_fraction) fprintf(file, "automatic_number_fraction_format=%i\n", automatic_fraction);
	if(default_fraction_fraction >= 0) fprintf(file, "default_number_fraction_fraction=%i\n", default_fraction_fraction);
	if(auto_prefix > 0) fprintf(file, "automatic_unit_prefixes=%i\n", auto_prefix);
	if(!scientific_noprefix) fprintf(file, "scientific_mode_unit_prefixes=%i\n", true);
	if(!scientific_notminuslast) fprintf(file, "scientific_mode_sort_minus_last=%i\n", true);
	if(!scientific_negexp) fprintf(file, "scientific_mode_negative_exponents=%i\n", false);
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")))) fprintf(file, "fraction_fixed_combined=%i\n", false);
	if(tc_set) fprintf(file, "temperature_calculation=%i\n", CALCULATOR->getTemperatureCalculationMode());
	if(sinc_set) fprintf(file, "sinc_function=%i\n", CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->getDefaultValue(2) == "pi" ? 1 : 0);
	for(unsigned int i = 0; i < custom_buttons.size(); i++) {
		if(!custom_buttons[i].text.empty()) fprintf(file, "custom_button_label=%u:%s\n", i, custom_buttons[i].text.c_str());
		for(unsigned int bi = 0; bi <= 2; bi++) {
			if(custom_buttons[i].type[bi] != -1) {
				if(custom_buttons[i].value[bi].empty()) fprintf(file, "custom_button=%u:%u:%i\n", i, bi, custom_buttons[i].type[bi]);
				else fprintf(file, "custom_button=%u:%u:%i:%s\n", i, bi, custom_buttons[i].type[bi], custom_buttons[i].value[bi].c_str());
			}
		}
	}
	if(!default_shortcuts) {
		std::vector<guint64> ksv;
		ksv.reserve(keyboard_shortcuts.size());
		for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
			if(ksv.empty() || it->first > ksv.back()) {
				ksv.push_back(it->first);
			} else {
				for(vector<guint64>::iterator it2 = ksv.begin(); it2 != ksv.end(); ++it2) {
					if(it->first <= *it2) {ksv.insert(it2, it->first); break;}
				}
			}
		}
		for(size_t i = 0; i < ksv.size(); i++) {
			unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find(ksv[i]);
			for(size_t i2 = 0; i2 < it->second.type.size(); i2++) {
				if(it->second.value[i2].empty()) fprintf(file, "keyboard_shortcut=%u:%u:%i\n", it->second.key, it->second.modifier, it->second.type[i2]);
				else fprintf(file, "keyboard_shortcut=%u:%u:%i:%s\n", it->second.key, it->second.modifier, it->second.type[i2], it->second.value[i2].c_str());
			}
		}
	}
	if(!save_history_separately) save_history_to_file(file);
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
	if(CALCULATOR->customAngleUnit()) custom_angle_unit = CALCULATOR->customAngleUnit()->referenceName();
	else custom_angle_unit = "";
	if(mode) set_saved_mode();
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
		fprintf(file, "interval_arithmetic=%i\n", modes[i].interval);
		fprintf(file, "interval_display=%i\n", modes[i].adaptive_interval_display ? 0 : modes[i].po.interval_display + 1);
		fprintf(file, "min_exp=%i\n", modes[i].po.min_exp);
		fprintf(file, "negative_exponents=%i\n", modes[i].po.negative_exponents);
		fprintf(file, "sort_minus_last=%i\n", modes[i].po.sort_options.minus_last);
		fprintf(file, "number_fraction_format=%i\n", modes[i].po.number_fraction_format);
		if(modes[i].po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR || modes[i].po.number_fraction_format == FRACTION_FRACTIONAL_FIXED_DENOMINATOR) fprintf(file, "number_fraction_denominator=%li\n", modes[i].fixed_denominator);
		fprintf(file, "complex_number_form=%i\n", (modes[i].complex_angle_form && modes[i].eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) ? modes[i].eo.complex_number_form + 1 : modes[i].eo.complex_number_form);
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
		if(!modes[i].custom_output_base.isZero()) fprintf(file, "custom_number_base=%s\n", modes[i].custom_output_base.print(CALCULATOR->save_printoptions).c_str());
		fprintf(file, "number_base_expression=%i\n", modes[i].eo.parse_options.base);
		if(!modes[i].custom_input_base.isZero()) fprintf(file, "custom_number_base_expression=%s\n", modes[i].custom_input_base.print(CALCULATOR->save_printoptions).c_str());
		fprintf(file, "read_precision=%i\n", modes[i].eo.parse_options.read_precision);
		fprintf(file, "assume_denominators_nonzero=%i\n", modes[i].eo.assume_denominators_nonzero);
		fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", modes[i].eo.warn_about_denominators_assumed_nonzero);
		fprintf(file, "structuring=%i\n", modes[i].eo.structuring);
		fprintf(file, "angle_unit=%i\n", modes[i].eo.parse_options.angle_unit);
		if(modes[i].eo.parse_options.angle_unit == ANGLE_UNIT_CUSTOM) fprintf(file, "custom_angle_unit=%s\n", modes[i].custom_angle_unit.c_str());
		fprintf(file, "functions_enabled=%i\n", modes[i].eo.parse_options.functions_enabled);
		fprintf(file, "variables_enabled=%i\n", modes[i].eo.parse_options.variables_enabled);
		fprintf(file, "calculate_functions=%i\n", modes[i].eo.calculate_functions);
		fprintf(file, "calculate_variables=%i\n", modes[i].eo.calculate_variables);
		fprintf(file, "variable_units_enabled=%i\n", modes[i].variable_units_enabled);
		fprintf(file, "sync_units=%i\n", modes[i].eo.sync_units);
		fprintf(file, "unknownvariables_enabled=%i\n", modes[i].eo.parse_options.unknowns_enabled);
		fprintf(file, "units_enabled=%i\n", modes[i].eo.parse_options.units_enabled);
		fprintf(file, "allow_complex=%i\n", modes[i].eo.allow_complex);
		fprintf(file, "allow_infinite=%i\n", modes[i].eo.allow_infinite);
		fprintf(file, "indicate_infinite_series=%i\n", modes[i].po.indicate_infinite_series);
		fprintf(file, "show_ending_zeroes=%i\n", modes[i].po.show_ending_zeroes);
		fprintf(file, "rounding_mode=%i\n", modes[i].po.rounding);
		fprintf(file, "approximation=%i\n", modes[i].eo.approximation);
		fprintf(file, "interval_calculation=%i\n", modes[i].eo.interval_calculation);
		fprintf(file, "concise_uncertainty_input=%i\n", modes[i].concise_uncertainty_input);
		fprintf(file, "calculate_as_you_type=%i\n", modes[i].autocalc);
		fprintf(file, "in_rpn_mode=%i\n", modes[i].rpn_mode);
		fprintf(file, "chain_mode=%i\n", modes[i].chain_mode);
		fprintf(file, "limit_implicit_multiplication=%i\n", modes[i].eo.parse_options.limit_implicit_multiplication);
		fprintf(file, "parsing_mode=%i\n", modes[i].eo.parse_options.parsing_mode);
		fprintf(file, "simplified_percentage=%i\n", modes[i].simplified_percentage);
		if(modes[i].implicit_question_asked) fprintf(file, "implicit_question_asked=%i\n", modes[i].implicit_question_asked);
		fprintf(file, "spacious=%i\n", modes[i].po.spacious);
		fprintf(file, "excessive_parenthesis=%i\n", modes[i].po.excessive_parenthesis);
		fprintf(file, "visible_keypad=%i\n", modes[i].keypad);
		fprintf(file, "short_multiplication=%i\n", modes[i].po.short_multiplication);
		fprintf(file, "default_assumption_type=%i\n", modes[i].at);
		if(modes[i].at != ASSUMPTION_TYPE_BOOLEAN) fprintf(file, "default_assumption_sign=%i\n", modes[i].as);
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
	if(default_plot_complex >= 0) fprintf(file, "plot_complex=%i\n", default_plot_complex);
	fprintf(file, "plot_variable=%s\n", default_plot_variable.c_str());
	fprintf(file, "plot_rows=%i\n", default_plot_rows);
	fprintf(file, "plot_type=%i\n", default_plot_type);
	fprintf(file, "plot_color=%i\n", default_plot_color);
	fprintf(file, "plot_linewidth=%i\n", default_plot_linewidth);
	if(max_plot_time != 5) fprintf(file, "max_plot_time=%i\n", max_plot_time);

	fclose(file);

	return true;

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

#ifdef __cplusplus
extern "C" {
#endif

void on_message_bar_response(GtkInfoBar *w, gint response_id, gpointer) {
	if(response_id == GTK_RESPONSE_CLOSE) {
		gint w, h, dur;
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "message_label")), "");
		gtk_window_get_size(GTK_WINDOW(mainwindow), &w, &h);
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_bar")));
		dur = gtk_revealer_get_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")));
		gtk_revealer_set_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), 0);
		gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), FALSE);
		gtk_window_resize(GTK_WINDOW(mainwindow), w, h);
		gtk_revealer_set_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), dur);
	}
}

gboolean on_main_window_close(GtkWidget *w, GdkEvent *event, gpointer user_data) {
	if(has_systray_icon()) {
		save_preferences(save_mode_on_exit);
		save_history();
		if(save_defs_on_exit) save_defs();
		gtk_window_get_position(GTK_WINDOW(w), &hidden_x, &hidden_y);
		hidden_monitor = 1;
		hidden_monitor_primary = false;
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		GdkDisplay *display = gtk_widget_get_display(mainwindow);
		int n = gdk_display_get_n_monitors(display);
		GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(mainwindow));
		if(monitor) {
			if(n > 1) {
				for(int i = 0; i < n; i++) {
					if(monitor == gdk_display_get_monitor(display, i)) {
						hidden_monitor = i + 1;
						break;
					}
				}
			}
			GdkRectangle area;
			gdk_monitor_get_workarea(monitor, &area);
			hidden_x -= area.x;
			hidden_y -= area.y;
			hidden_monitor_primary = gdk_monitor_is_primary(monitor);
		}
#else
		GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(mainwindow));
		if(screen) {
			int i = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(mainwindow));
			if(i >= 0) {
				hidden_monitor_primary = (i == gdk_screen_get_primary_monitor(screen));
				GdkRectangle area;
				gdk_screen_get_monitor_workarea(screen, i, &area);
				hidden_monitor = i + 1;
				hidden_x -= area.x;
				hidden_y -= area.y;
			}
		}
#endif
		gtk_widget_hide(w);
		if(!b_busy) {
			if(expression_is_empty()) clearresult();
			else clear_expression_text();
		}
	} else {
		on_gcalc_exit(w, event, user_data);
	}
	return TRUE;
}

gboolean on_status_left_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(event->type == GDK_BUTTON_PRESS && event->button == 3 && !b_busy) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_parsed_in_result_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result")), parsed_in_result && !rpn_mode);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_parsed_in_result_activate, NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_parsed_in_result")), display_expression_status && !rpn_mode);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_expression_status"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_expression_status_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_expression_status")), display_expression_status);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_expression_status"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_expression_status_activate, NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_copy_status")), status_text_source == STATUS_TEXT_AUTOCALC || status_text_source == STATUS_TEXT_PARSED);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_copy_ascii_status")), status_text_source == STATUS_TEXT_AUTOCALC || status_text_source == STATUS_TEXT_PARSED);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_left")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_left")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_status_right_button_release_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(event->type == GDK_BUTTON_RELEASE && event->button == 1) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_status_right_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS && event->button != 1) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_status_right")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_image_keypad_lock_button_release_event(GtkWidget*, GdkEvent*, gpointer) {
	persistent_keypad = !persistent_keypad;
	update_persistent_keypad(false);
	return TRUE;
}
void on_popup_menu_item_persistent_keypad_toggled(GtkCheckMenuItem *w, gpointer) {
	persistent_keypad = gtk_check_menu_item_get_active(w);
	update_persistent_keypad(true);
}
gboolean on_image_keypad_lock_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS && event->button != 1) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_expander_keypad")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_expander_keypad")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_expander_keypad_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_expander_keypad")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_expander_keypad")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}

void on_menu_item_show_parsed_activate(GtkMenuItem *w, gpointer) {
	show_parsed(true);
}
void on_menu_item_show_result_activate(GtkMenuItem *w, gpointer) {
	show_parsed(false);
}

void on_menu_item_parsed_in_result_activate(GtkMenuItem *w, gpointer) {
	set_parsed_in_result(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_expression_status_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)) == display_expression_status) return;
	display_expression_status = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	if(display_expression_status) {
		display_parse_status();
		preferences_update_expression_status();
	} else {
		set_parsed_in_result(false);
		clear_status_text();
	}
}

void update_app_font(bool initial) {
	if(use_custom_app_font) {
		if(!app_provider) {
			app_provider = gtk_css_provider_new();
			gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(app_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}
		gchar *gstr = font_name_to_css(custom_app_font.c_str());
		gtk_css_provider_load_from_data(app_provider, gstr, -1, NULL);
		g_free(gstr);
	} else if(initial) {
		if(custom_app_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(mainwindow), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_app_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	} else if(app_provider) {
		gtk_css_provider_load_from_data(app_provider, "", -1, NULL);
	}
	if(!initial) {
		while(gtk_events_pending()) gtk_main_iteration();
		variables_font_updated();
		units_font_updated();
		functions_font_updated();
		expression_font_modified();
		status_font_modified();
		result_font_modified();
		keypad_font_modified();
		history_font_modified();
		completion_font_modified();
	}
}
void update_result_font(bool initial) {
	gint h_old = 0, h_new = 0;
	if(!initial) gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_old);
	if(use_custom_result_font) {
		gchar *gstr = font_name_to_css(custom_result_font.c_str());
		gtk_css_provider_load_from_data(resultview_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(resultview_provider, "* {font-size: larger;}", -1, NULL);
		if(initial && custom_result_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(resultview), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_result_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(initial) {
		fix_supsub_result = test_supsub(resultview);
	} else {
		result_font_modified();
		gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_new);
		gint winh, winw;
		gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
		winh += (h_new - h_old);
		gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
	}
}
void update_status_font(bool initial) {
	gint h_old = 0;
	if(!initial) h_old = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
	if(use_custom_status_font) {
		gchar *gstr = font_name_to_css(custom_status_font.c_str());
		gtk_css_provider_load_from_data(statuslabel_l_provider, gstr, -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(statuslabel_l_provider, "* {font-size: 90%;}", -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, "* {font-size: 90%;}", -1, NULL);
		if(initial && custom_status_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(statuslabel_l), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_status_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(initial) {
		fix_supsub_status = test_supsub(statuslabel_l);
	} else {
		status_font_modified();
		while(gtk_events_pending()) gtk_main_iteration();
		gint h_new = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")));
		gint winh, winw;
		gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
		winh += (h_new - h_old);
		gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
	}
}
void keypad_font_modified() {
	update_keypad_button_text();
	update_stack_button_text();
	while(gtk_events_pending()) gtk_main_iteration();
	gint winh, winw;
	gtk_window_get_size(GTK_WINDOW(mainwindow), &winw, &winh);
	if(minimal_mode) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
	}
	while(gtk_events_pending()) gtk_main_iteration();
	bool b_buttons = gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad));
	if(!b_buttons) gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
	while(gtk_events_pending()) gtk_main_iteration();
	for(size_t i = 0; i < 5 && (!b_buttons || minimal_mode); i++) {
		sleep_ms(10);
		while(gtk_events_pending()) gtk_main_iteration();
	}
	GtkRequisition req;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), &req, NULL);
	gtk_window_resize(GTK_WINDOW(mainwindow), req.width + 24, 1);
	if(!b_buttons || minimal_mode) {
		while(gtk_events_pending()) gtk_main_iteration();
		for(size_t i = 0; i < 5; i++) {
			sleep_ms(10);
			while(gtk_events_pending()) gtk_main_iteration();
		}
		if(minimal_mode) {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
			if(winw < req.width + 24) winw = req.width + 24;
		}
		gtk_window_get_size(GTK_WINDOW(mainwindow), &win_width, NULL);
		if(!minimal_mode) winw = win_width;
		if(!b_buttons) gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons")));
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_window_resize(GTK_WINDOW(mainwindow), winw, winh);
	}
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

bool test_can_approximate(const MathStructure &m, bool top = true) {
	if((m.isVariable() && m.variable()->isKnown()) || (m.isNumber() && !top)) return true;
	if(m.isUnit_exp()) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(test_can_approximate(m[i], false)) return true;
	}
	return false;
}

bool has_prefix(const MathStructure &m) {
	if(m.isUnit() && (m.prefix() && m.prefix() != CALCULATOR->decimal_null_prefix && m.prefix() != CALCULATOR->binary_null_prefix)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(has_prefix(m[i])) return true;
	}
	return false;
}

void update_resultview_popup() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_octal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_decimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_duodecimal_activate, NULL);
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
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_mixed_units_conversion"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_mixed_units_conversion_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_abbreviate_names_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_denominator_prefixes_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_denominator_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_all_prefixes_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_exact_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_combined_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_fraction_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_exact_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_assume_nonzero_denominators_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_rectangular"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_rectangular_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_exponential"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_exponential_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_polar"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_polar_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_angle"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_angle_activate, NULL);

	bool b_parsed = (parsed_in_result && !displayed_mstruct && displayed_parsed_mstruct);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), displayed_mstruct || displayed_parsed_mstruct);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_copy")), displayed_mstruct || b_parsed);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_copy_ascii")), displayed_mstruct || b_parsed);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "result_popup_menu_item_show_parsed")), !show_parsed_instead_of_result && displayed_mstruct);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "result_popup_menu_item_show_result")), show_parsed_instead_of_result && displayed_mstruct);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_show_parsed")), displayed_mstruct != NULL);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result")), b_parsed);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_parsed_in_result")), b_parsed);
	if(b_parsed) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_parsed_in_result_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result")), TRUE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_parsed_in_result_activate, NULL);
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "result_popup_menu_item_meta_modes")), !b_parsed);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_result_popup_modes")), !b_parsed);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save")), !b_parsed);

	bool b_unit = !b_parsed && displayed_mstruct && contains_convertible_unit(*displayed_mstruct);
	bool b_date = !b_parsed && displayed_mstruct && displayed_mstruct->isDateTime();
	bool b_complex = !b_parsed && displayed_mstruct && mstruct && (contains_imaginary_number(*mstruct) || mstruct->containsFunctionId(FUNCTION_ID_CIS));
	bool b_rational = !b_parsed && displayed_mstruct && mstruct && contains_rational_number(*mstruct);
	bool b_object = !b_parsed && displayed_mstruct && (displayed_mstruct->containsType(STRUCT_UNIT) || displayed_mstruct->containsType(STRUCT_FUNCTION) || displayed_mstruct->containsType(STRUCT_VARIABLE));

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names")), b_object);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_abbreviate_names")), b_object);

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
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_convert_to")), FALSE);
	if(!b_parsed && displayed_mstruct && ((displayed_mstruct->isMultiplication() && displayed_mstruct->size() == 2 && (*displayed_mstruct)[1].isUnit() && (*displayed_mstruct)[0].isNumber() && (*displayed_mstruct)[1].unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) (*displayed_mstruct)[1].unit())->mixWithBase()) || (displayed_mstruct->isAddition() && displayed_mstruct->size() > 0 && (*displayed_mstruct)[0].isMultiplication() && (*displayed_mstruct)[0].size() == 2 && (*displayed_mstruct)[0][1].isUnit() && (*displayed_mstruct)[0][0].isNumber() && (*displayed_mstruct)[0][1].unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) (*displayed_mstruct)[0][1].unit())->mixWithBase()))) {
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_mixed_units_conversion")), TRUE);
	} else {
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_mixed_units_conversion")), FALSE);
	}
	if(b_unit) {
		GtkWidget *sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_convert_to"));
		GtkWidget *item;
		if(expression_modified() && !rpn_mode && (!auto_calculate || parsed_in_result)) execute_expression(true);
		GList *list = gtk_container_get_children(GTK_CONTAINER(sub));
		for(GList *l = list; l != NULL; l = l->next) {
			gtk_widget_destroy(GTK_WIDGET(l->data));
		}
		g_list_free(list);
		Unit *u_result = NULL;
		if(displayed_mstruct) u_result = find_exact_matching_unit(*displayed_mstruct);
		bool b_exact = (u_result != NULL);
		if(!u_result && mstruct) u_result = CALCULATOR->findMatchingUnit(*mstruct);
		bool b_prefix = false;
		if(b_exact && u_result && u_result->subtype() != SUBTYPE_COMPOSITE_UNIT) b_prefix = has_prefix(displayed_mstruct ? *displayed_mstruct : *mstruct);
		vector<Unit*> to_us;
		if(u_result && u_result->isCurrency()) {
			Unit *u_local_currency = CALCULATOR->getLocalCurrency();
			if(latest_button_currency && (!b_exact || b_prefix || latest_button_currency != u_result) && latest_button_currency != u_local_currency) to_us.push_back(latest_button_currency);
			for(size_t i = 0; i < CALCULATOR->units.size() + 2; i++) {
				Unit * u;
				if(i == 0) u = u_local_currency;
				else if(i == 1) u = latest_button_currency;
				else u = CALCULATOR->units[i - 2];
				if(u && (!b_exact || b_prefix || u != u_result) && u->isActive() && u->isCurrency() && (i == 0 || (u != u_local_currency && u != latest_button_currency && !u->isHidden()))) {
					bool b = false;
					for(size_t i2 = 0; i2 < to_us.size(); i2++) {
						if(string_is_less(u->title(true, printops.use_unicode_signs), to_us[i2]->title(true, printops.use_unicode_signs))) {
							to_us.insert(to_us.begin() + i2, u);
							b = true;
							break;
						}
					}
					if(!b) to_us.push_back(u);
				}
			}
			for(size_t i = 0; i < to_us.size(); i++) {
				MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us[i], convert_to_unit)
			}
			vector<Unit*> to_us2;
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if(CALCULATOR->units[i]->isCurrency()) {
					Unit *u = CALCULATOR->units[i];
					if(u->isActive() && (!b_exact || b_prefix || u != u_result) && u->isHidden() && u != u_local_currency && u != latest_button_currency) {
						bool b = false;
						for(int i2 = to_us2.size() - 1; i2 >= 0; i2--) {
							if(u->title(true, printops.use_unicode_signs) > to_us2[(size_t) i2]->title(true, printops.use_unicode_signs)) {
								if((size_t) i2 == to_us2.size() - 1) to_us2.push_back(u);
								else to_us2.insert(to_us2.begin() + (size_t) i2 + 1, u);
								b = true;
								break;
							}
						}
						if(!b) to_us2.insert(to_us2.begin(), u);
					}
				}
			}
			if(to_us2.size() > 0) {
				SUBMENU_ITEM(_("more"), sub);
				for(size_t i = 0; i < to_us2.size(); i++) {
					// Show further items in a submenu
					MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us2[i], convert_to_unit)
				}
			}
			gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_convert_to")), TRUE);
		} else if(u_result && !u_result->category().empty()) {
			string s_cat = u_result->category();
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if(CALCULATOR->units[i]->category() == s_cat) {
					Unit *u = CALCULATOR->units[i];
					if((!b_exact || b_prefix || u != u_result) && u->isActive() && !u->isHidden()) {
						bool b = false;
						for(size_t i2 = 0; i2 < to_us.size(); i2++) {
							if(string_is_less(u->title(true, printops.use_unicode_signs), to_us[i2]->title(true, printops.use_unicode_signs))) {
								to_us.insert(to_us.begin() + i2, u);
								b = true;
								break;
							}
						}
						if(!b) to_us.push_back(u);
					}
				}
			}
			for(size_t i = 0; i < to_us.size(); i++) {
				MENU_ITEM_WITH_OBJECT(to_us[i], convert_to_unit_noprefix)
			}
			gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_convert_to")), TRUE);
		}
	}

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_units")), b_unit);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_octal")), !b_parsed && !b_unit && !b_date && !b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_decimal")), !b_parsed && !b_unit && !b_date && !b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_duodecimal")), !b_parsed && !b_unit && !b_date && !b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_hexadecimal")), !b_parsed && !b_unit && !b_date && !b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_binary")), !b_parsed && !b_unit && !b_date && !b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_roman")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_sexagesimal")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_time_format")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_custom_base")), !b_parsed && !b_unit && !b_date && !b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_base")), !b_parsed && !b_unit && !b_date && !b_complex);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_complex_rectangular")), b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_complex_exponential")), b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_complex_polar")), b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_complex_angle")), b_complex);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_complex")), b_complex);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_normal")), !b_parsed && !b_unit && !b_date);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_engineering")), !b_parsed && !b_unit && !b_date);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_scientific")), !b_parsed && !b_unit && !b_date);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_purely_scientific")), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_display_non_scientific")), !b_parsed && !b_unit && !b_date);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_display")), !b_parsed && !b_unit && !b_date);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_fraction")), b_rational);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal")), b_rational);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal_exact")), b_rational);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_combined")), b_rational);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_fraction")), b_rational);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_calendarconversion")), b_date);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_to_utc")), b_date);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_display_date")), b_date);

	if(!b_parsed && mstruct && mstruct->containsUnknowns()) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_unknowns")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_unknowns")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	}
	if(!b_parsed && mstruct && mstruct->containsType(STRUCT_ADDITION)) {
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
		if(contains_polynomial_division(*mstruct)) gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_expand_partial_fractions")));
		else gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_expand_partial_fractions")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	} else {
		if(!b_parsed && mstruct && mstruct->isNumber() && mstruct->number().isInteger() && !mstruct->number().isZero()) {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
		} else {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
		}
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_simplify")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_expand_partial_fractions")));
	}
	if(!b_parsed && mstruct && (mstruct->isApproximate() || test_can_approximate(*mstruct))) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_exact")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_nonzero")));
		if(!mstruct->isApproximate() && mstruct->containsDivision()) gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
		else gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_exact")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_nonzero")));
	}
	if(!b_parsed && mstruct->isVector() && (mstruct->size() != 1 || !(*mstruct)[0].isVector() || (*mstruct)[0].size() > 0)) {
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
	switch(printops.base) {
		case BASE_OCTAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_octal")), TRUE);
			break;
		}
		case BASE_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_decimal")), TRUE);
			break;
		}
		case BASE_DUODECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_duodecimal")), TRUE);
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
		/*case BASE_SEXAGESIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_sexagesimal")), TRUE);
			break;
		}
		case BASE_TIME: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_time_format")), TRUE);
			break;
		}*/
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
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_mixed_units_conversion")), evalops.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names")), printops.abbreviate_names);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes")), printops.use_all_prefixes);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_denominator_prefixes")), printops.use_denominator_prefix);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_exact")), evalops.approximation == APPROXIMATION_EXACT);
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
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_fraction_other")), TRUE);
			break;
		}
	}
	switch(evalops.complex_number_form) {
		case COMPLEX_NUMBER_FORM_RECTANGULAR: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_complex_rectangular")), TRUE);
			break;
		}
		case COMPLEX_NUMBER_FORM_EXPONENTIAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_complex_exponential")), TRUE);
			break;
		}
		case COMPLEX_NUMBER_FORM_POLAR: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_complex_polar")), TRUE);
			break;
		}
		case COMPLEX_NUMBER_FORM_CIS: {
			if(complex_angle_form) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_complex_angle")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_complex_polar")), TRUE);
			break;
		}
	}
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_octal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_decimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_duodecimal_activate, NULL);
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
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_mixed_units_conversion"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_mixed_units_conversion_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_abbreviate_names_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_abbreviate_names"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_all_prefixes_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_all_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_denominator_prefixes_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_decimal_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_decimal_exact_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_combined_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_fraction_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_fraction_fraction_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_exact_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_assume_nonzero_denominators_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_rectangular"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_rectangular_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_exponential"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_exponential_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_polar"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_polar_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "popup_menu_item_complex_angle"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_complex_angle_activate, NULL);
}

gboolean on_main_window_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	hide_completion();
	return FALSE;
}
guint32 prev_result_press_time = 0;
gboolean on_resultview_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(b_busy) return FALSE;
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		update_resultview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	if(event->button == 1 && event->time > prev_result_press_time + 100 && surface_result && !show_parsed_instead_of_result && event->x >= gtk_widget_get_allocated_width(resultview) - cairo_image_surface_get_width(surface_result) - 20) {
		gint x = event->x - binary_x_diff;
		gint y = event->y - binary_y_diff;
		if(!binary_rect.empty() && x >= binary_rect[0].x) {
			for(size_t i = 0; i < binary_rect.size(); i++) {
				if(x >= binary_rect[i].x && x <= binary_rect[i].x + binary_rect[i].width && y >= binary_rect[i].y && y <= binary_rect[i].y + binary_rect[i].height) {
					size_t index = result_text.find("<");
					string new_binary = result_text;
					if(index != string::npos) new_binary = new_binary.substr(0, index);
					index = new_binary.length();
					int n = 0;
					for(; index > 0; index--) {
						if(result_text[index - 1] == '0' || result_text[index - 1] == '1') {
							if(n == binary_pos[i]) break;
							n++;
						} else if(result_text[index - 1] != ' ') {
							index = 0;
							break;
						}
					}
					prev_result_press_time = event->time;
					if(index > 0) {
						index--;
						if(new_binary[index] == '1') new_binary[index] = '0';
						else new_binary[index] = '1';
						ParseOptions po;
						po.base = BASE_BINARY;
						po.twos_complement = printops.twos_complement;
						gsub(SIGN_MINUS, "-", new_binary);
						Number nr(new_binary, po);
						set_expression_text(print_with_evalops(nr).c_str());
						if(rpn_mode || !auto_calculate || parsed_in_result) execute_expression();
					}
					return TRUE;
				}
			}
		} else {
			prev_result_press_time = event->time;
			copy_result(-1);
			// Result was copied
			show_notification(_("Copied"));
		}
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

/*
	save preferences, mode and definitions and then quit
*/
gboolean on_gcalc_exit(GtkWidget*, GdkEvent*, gpointer) {
	return qalculate_quit();
}
bool qalculate_quit() {
	exit_in_progress = true;
	stop_autocalculate_history_timeout();
	block_error();
	hide_plot_dialog();
	CALCULATOR->abort();
	if(!save_preferences(save_mode_on_exit, true)) {
		unblock_error();
		exit_in_progress = false;
		return FALSE;
	}
	if(!save_history(true)) {
		unblock_error();
		exit_in_progress = false;
		return FALSE;
	}
	if(save_defs_on_exit && !save_defs(true)) {
		unblock_error();
		exit_in_progress = false;
		return FALSE;
	}
	stop_timeouts = true;
#ifdef _WIN32
	if(use_systray_icon) destroy_systray_icon();
#endif
	history_free();
	if(command_thread->running) {
		command_thread->write((int) 0);
		command_thread->write(NULL);
	}
	if(view_thread->running) {
		view_thread->write((int) 0);
		view_thread->write(NULL);
	}
	CALCULATOR->terminateThreads();
	g_application_quit(g_application_get_default());
	return TRUE;
}

void entry_insert_text(GtkWidget *w, const gchar *text) {
	gtk_editable_delete_selection(GTK_EDITABLE(w));
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

void memory_recall() {
	bool b_exec = !rpn_mode && (!auto_calculate || parsed_in_result) && (expression_is_empty() || !expression_modified());
	insert_variable(v_memory);
	if(b_exec) execute_expression(true);
}
void memory_store() {
	if(expression_modified() && !rpn_mode && (!auto_calculate || parsed_in_result)) execute_expression(true);
	if(!mstruct) return;
	v_memory->set(*mstruct);
	if(parsed_mstruct && parsed_mstruct->contains(v_memory, true)) expression_calculation_updated();
}
void memory_add() {
	if(expression_modified() && !rpn_mode && (!auto_calculate || parsed_in_result)) execute_expression(true);
	if(!mstruct) return;
	MathStructure m = v_memory->get();
	m.calculateAdd(*mstruct, evalops);
	v_memory->set(m);
	if(parsed_mstruct && parsed_mstruct->contains(v_memory, true)) expression_calculation_updated();
}
void memory_subtract() {
	if(expression_modified() && !rpn_mode && (!auto_calculate || parsed_in_result)) execute_expression(true);
	if(!mstruct) return;
	MathStructure m = v_memory->get();
	m.calculateSubtract(*mstruct, evalops);
	v_memory->set(m);
	if(parsed_mstruct && parsed_mstruct->contains(v_memory, true)) expression_calculation_updated();
}
void memory_clear() {
	v_memory->set(m_zero);
	if(parsed_mstruct && parsed_mstruct->contains(v_memory, true)) expression_calculation_updated();
}


void on_expander_convert_activate(GtkExpander *o, gpointer) {
	focus_conversion_entry();
}

void on_menu_item_status_exact_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), TRUE);
	else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), TRUE);
}
void on_menu_item_status_read_precision_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_status_rpn_syntax_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
}
void on_menu_item_status_chain_syntax_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), TRUE);
}
void on_menu_item_status_adaptive_parsing_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
}
void on_menu_item_status_ignore_whitespace_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
}
void on_menu_item_status_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
}

void set_autocalculate(bool b) {
	if(auto_calculate == b) return;
	auto_calculate = b;
	if(auto_calculate && !rpn_mode) {
		current_status_struct.setAborted();
		prev_autocalc_str = "";
		do_auto_calc();
	} else if(!auto_calculate && result_autocalculated) {
		mauto.clear();
		result_text = "";
		if(result_autocalculated) {
			result_autocalculated = false;
			if(parsed_in_result) display_parse_status();
			else clearresult();
		}
	}
}
void update_exchange_rates() {
	stop_autocalculate_history_timeout();
	block_error();
	fetch_exchange_rates(15);
	CALCULATOR->loadExchangeRates();
	display_errors(mainwindow);
	unblock_error();
	while(gtk_events_pending()) gtk_main_iteration();
	expression_calculation_updated();
}

void import_definitions_file() {
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select definitions file"), GTK_WINDOW(mainwindow), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Import"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select definitions file"), GTK_WINDOW(mainwindow), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Import"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
#endif
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("XML Files"));
	gtk_file_filter_add_mime_type(filter, "text/xml");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter);
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#else
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#endif
		GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(d));
		char *str = g_file_get_basename(file);
		char *from_file = g_file_get_path(file);
		string homedir = buildPath(getLocalDataDir(), "definitions");
		recursiveMakeDir(homedir);
#ifdef _WIN32
		if(CopyFile(from_file, buildPath(homedir, str).c_str(), false) != 0) {
			CALCULATOR->loadDefinitions(buildPath(homedir, str).c_str(), false, true);
			update_fmenu(false);
			update_vmenu(false);
			update_umenus();
		} else {
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not copy %s to %s."), from_file, buildPath(homedir, str).c_str());
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}
#else
		ifstream source(from_file);
		if(source.fail()) {
			source.close();
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not read %s."), from_file);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		} else {
			ofstream dest(buildPath(homedir, str).c_str());
			if(dest.fail()) {
				source.close();
				dest.close();
				GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not copy file to %s."), homedir.c_str());
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			} else {
				dest << source.rdbuf();
				source.close();
				dest.close();
				CALCULATOR->loadDefinitions(buildPath(homedir, str).c_str(), false, true);
				update_fmenu(false);
				update_vmenu(false);
				update_umenus();
			}
		}
#endif
		g_free(str);
		g_free(from_file);
		g_object_unref(file);
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	g_object_unref(d);
#else
	gtk_widget_destroy(d);
#endif
}

void set_input_base(int base, bool open_dialog, bool recalculate) {
	if(base == BASE_CUSTOM) {
		open_setbase(GTK_WINDOW(mainwindow), true, true);
		return;
	}
	bool b = (evalops.parse_options.base == base && base != BASE_CUSTOM);
	evalops.parse_options.base = base;
	update_setbase();
	update_keypad_programming_base();
	history_input_base_changed();
	if(!b) expression_format_updated(recalculate);
}
void set_output_base(int base) {
	bool b = (printops.base == base && base != BASE_CUSTOM);
	to_base = 0;
	to_bits = 0;
	printops.base = base;
	update_keypad_base();
	update_menu_base();
	update_setbase();
	update_keypad_programming_base();
	if(!b) result_format_updated();
}

void on_popup_menu_item_calendarconversion_activate(GtkMenuItem *w, gpointer) {
	show_calendarconversion_dialog(GTK_WINDOW(mainwindow), mstruct && mstruct->isDateTime() ? mstruct->datetime() : NULL);
}
void on_popup_menu_item_to_utc_activate(GtkMenuItem *w, gpointer) {
	printops.time_zone = TIME_ZONE_UTC;
	result_format_updated();
	printops.time_zone = TIME_ZONE_LOCAL;
}
void open_plot() {
	string str, str2;
	if(evalops.parse_options.base == 10) {
		str = get_selected_expression_text();
		CALCULATOR->separateToExpression(str, str2, evalops, true);
		remove_blank_ends(str);
	}
	show_plot_dialog(GTK_WINDOW(GTK_WINDOW(mainwindow)), str.c_str());
}
void on_popup_menu_item_display_normal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0);
}
void on_popup_menu_item_exact_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), true);
	else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), true);
}
void on_popup_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assume_nonzero_denominators")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 1);
}
void on_popup_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 2);
}
void on_popup_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 3);
}
void on_popup_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 4);
}
void on_popup_menu_item_complex_rectangular_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_rectangular")), TRUE);
}
void on_popup_menu_item_complex_exponential_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_exponential")), TRUE);
}
void on_popup_menu_item_complex_polar_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
}
void on_popup_menu_item_complex_angle_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_angle")), TRUE);
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
void on_popup_menu_item_mixed_units_conversion_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_mixed_units_conversion")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
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
void on_popup_menu_item_duodecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_duodecimal")), TRUE);
}
void on_popup_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
}
void on_popup_menu_item_custom_base_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	gtk_menu_item_activate(GTK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")));
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
	insert_matrix(mstruct, mainwindow, false, false, true);
}
void on_popup_menu_item_view_vector_activate(GtkMenuItem*, gpointer) {
	insert_matrix(mstruct, mainwindow, true, false, true);
}

void restore_automatic_fraction() {
	if(automatic_fraction && printops.number_fraction_format == FRACTION_DECIMAL_EXACT) {
		if(!rpn_mode) block_result();
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
		automatic_fraction = false;
		if(!rpn_mode) unblock_result();
	}
}
void set_approximation(ApproximationMode approx) {
	evalops.approximation = approx;
	update_keypad_exact();
	if(approx == APPROXIMATION_EXACT) {
		if(printops.number_fraction_format == FRACTION_DECIMAL) {
			if(!rpn_mode) block_result();
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
			automatic_fraction = true;
			if(!rpn_mode) unblock_result();
		}
	} else {
		restore_automatic_fraction();
	}
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_exact_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), approx == APPROXIMATION_EXACT);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_exact_activate, NULL);
	update_menu_approximation();

	expression_calculation_updated();
}
void set_min_exp(int min_exp) {
	printops.min_exp = min_exp;
	update_keypad_numerical_display();
	update_menu_numerical_display();
	result_format_updated();
}
void set_prefix_mode(int i) {
	to_prefix = 0;
	printops.use_unit_prefixes = (i != PREFIX_MODE_NO_PREFIXES);
	printops.use_prefixes_for_all_units = (i == PREFIX_MODE_ALL_UNITS);
	printops.use_prefixes_for_currencies = (i == PREFIX_MODE_ALL_UNITS || i == PREFIX_MODE_CURRENCIES);
	if(!printops.use_unit_prefixes && printops.min_exp != EXP_NONE && printops.min_exp != EXP_PRECISION) scientific_noprefix = true;
	else if(printops.use_unit_prefixes && printops.min_exp != EXP_NONE && printops.min_exp != EXP_PRECISION) scientific_noprefix = false;
	auto_prefix = 0;
	result_format_updated();
}

void set_fraction_format(int nff) {
	to_fraction = 0;
	to_fixed_fraction = 0;
	if(nff > FRACTION_COMBINED_FIXED_DENOMINATOR) {
		nff = FRACTION_FRACTIONAL;
		printops.restrict_fraction_length = false;
	} else {
		printops.restrict_fraction_length = (nff == FRACTION_COMBINED || nff == FRACTION_FRACTIONAL);
	}
	printops.number_fraction_format = (NumberFractionFormat) nff;
	automatic_fraction = false;
	update_keypad_fraction();
	update_menu_fraction();
	result_format_updated();
}
void set_fixed_fraction(long int v, bool combined) {
	CALCULATOR->setFixedDenominator(v);
	if(combined) set_fraction_format(FRACTION_COMBINED_FIXED_DENOMINATOR);
	else set_fraction_format(FRACTION_FRACTIONAL_FIXED_DENOMINATOR);
}

void save_as_image() {
	if(display_aborted || !displayed_mstruct) return;
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select file to save PNG image to"), GTK_WINDOW(mainwindow), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Save"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to save PNG image to"), GTK_WINDOW(mainwindow), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
#endif
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
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#else
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#endif
		GdkRGBA color;
		color.red = 0.0;
		color.green = 0.0;
		color.blue = 0.0;
		color.alpha = 1.0;
		cairo_surface_t *s = NULL;
		if(((parsed_in_result && !displayed_mstruct) || show_parsed_instead_of_result) && displayed_parsed_mstruct) s = draw_structure(*displayed_parsed_mstruct, parsed_printops, complex_angle_form, top_ips, NULL, 1, &color, NULL, NULL, -1, false, &mwhere, &displayed_parsed_to);
		else s = draw_structure(*displayed_mstruct, displayed_printops, displayed_caf, top_ips, NULL, 1, &color, NULL, NULL, -1, false);
		if(s) {
			cairo_surface_flush(s);
			cairo_surface_write_to_png(s, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d)));
			cairo_surface_destroy(s);
		}
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	g_object_unref(d);
#else
	gtk_widget_destroy(d);
#endif
}

void on_popup_menu_item_copy_activate(GtkMenuItem*, gpointer) {
	copy_result(0, (((parsed_in_result && !displayed_mstruct) || show_parsed_instead_of_result) && displayed_parsed_mstruct) ? 8 : 0);
}
void on_popup_menu_item_copy_ascii_activate(GtkMenuItem*, gpointer) {
	copy_result(1, (((parsed_in_result && !displayed_mstruct) || show_parsed_instead_of_result) && displayed_parsed_mstruct) ? 8 : 0);
}
void on_menu_item_copy_status_activate(GtkMenuItem*, gpointer) {
	copy_result(0, status_text_source == STATUS_TEXT_AUTOCALC ? 0 : 8);
}
void on_menu_item_copy_ascii_status_activate(GtkMenuItem*, gpointer) {
	copy_result(1, status_text_source == STATUS_TEXT_AUTOCALC ? 0 : 8);
}

gboolean on_about_activate_link(GtkAboutDialog*, gchar *uri, gpointer) {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
	return TRUE;
#else
	return FALSE;
#endif
}
void show_about() {
	const gchar *authors[] = {"Hanna Knutsson <hanna.knutsson@protonmail.com>", NULL};
	GtkWidget *dialog = gtk_about_dialog_new();
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("translator-credits"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Powerful and easy to use calculator"));
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2003–2007, 2008, 2016–2023 Hanna Knutsson");
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), "qalculate");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Qalculate! (GTK)");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://qalculate.github.io/");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mainwindow));
	g_signal_connect(G_OBJECT(dialog), "activate-link", G_CALLBACK(on_about_activate_link), NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void report_bug() {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", "https://github.com/Qalculate/qalculate-gtk/issues", NULL, NULL, SW_SHOWNORMAL);
#else
	GError *error = NULL;
#	if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_show_uri_on_window(GTK_WINDOW(mainwindow), "https://github.com/Qalculate/qalculate-gtk/issues", gtk_get_current_event_time(), &error);
#	else
	gtk_show_uri(NULL, "https://github.com/Qalculate/qalculate-gtk/issues", gtk_get_current_event_time(), &error);
#	endif
	if(error) {
		gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Failed to open %s.\n%s"), "https://github.com/Qalculate/qalculate-gtk/issues", error_str);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(error_str);
		g_error_free(error);
	}
#endif
}

bool do_shortcut(int type, string value) {
	switch(type) {
		case SHORTCUT_TYPE_FUNCTION: {
			insert_button_function(CALCULATOR->getActiveFunction(value));
			return true;
		}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
			insert_function(CALCULATOR->getActiveFunction(value), mainwindow);
			return true;
		}
		case SHORTCUT_TYPE_VARIABLE: {
			insert_variable(CALCULATOR->getActiveVariable(value));
			return true;
		}
		case SHORTCUT_TYPE_UNIT: {
			Unit *u = CALCULATOR->getActiveUnit(value);
			if(u && CALCULATOR->stillHasUnit(u)) {
				if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					PrintOptions po = printops;
					po.is_approximate = NULL;
					po.can_display_unicode_string_arg = (void*) expressiontext;
					string str = ((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true);
					insert_text(str.c_str());
				} else {
					insert_text(u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_UNIT, true).c_str());
				}
			}
			return true;
		}
		case SHORTCUT_TYPE_TEXT: {
			insert_text(value.c_str());
			return true;
		}
		case SHORTCUT_TYPE_DATE: {
			expression_insert_date();
			return true;
		}
		case SHORTCUT_TYPE_VECTOR: {
			expression_insert_vector();
			return true;
		}
		case SHORTCUT_TYPE_MATRIX: {
			expression_insert_matrix();
			return true;
		}
		case SHORTCUT_TYPE_SMART_PARENTHESES: {
			brace_wrap();
			return true;
		}
		case SHORTCUT_TYPE_CONVERT: {
			ParseOptions pa = evalops.parse_options; pa.base = 10;
			executeCommand(COMMAND_CONVERT_STRING, true, false, CALCULATOR->unlocalizeExpression(value, pa));
			return true;
		}
		case SHORTCUT_TYPE_CONVERT_ENTRY: {
			show_unit_conversion();
			return true;
		}
		case SHORTCUT_TYPE_OPTIMAL_UNIT: {
			executeCommand(COMMAND_CONVERT_OPTIMAL);
			return true;
		}
		case SHORTCUT_TYPE_BASE_UNITS: {
			executeCommand(COMMAND_CONVERT_BASE);
			return true;
		}
		case SHORTCUT_TYPE_OPTIMAL_PREFIX: {
			result_prefix_changed(NULL);
			return true;
		}
		case SHORTCUT_TYPE_TO_NUMBER_BASE: {
			int save_base = printops.base;
			Number save_nbase = CALCULATOR->customOutputBase();
			to_base = 0;
			to_bits = 0;
			Number nbase;
			base_from_string(value, printops.base, nbase);
			CALCULATOR->setCustomOutputBase(nbase);
			result_format_updated();
			printops.base = save_base;
			CALCULATOR->setCustomOutputBase(save_nbase);
			return true;
		}
		case SHORTCUT_TYPE_FACTORIZE: {
			executeCommand(COMMAND_FACTORIZE);
			return true;
		}
		case SHORTCUT_TYPE_EXPAND: {
			executeCommand(COMMAND_EXPAND);
			return true;
		}
		case SHORTCUT_TYPE_PARTIAL_FRACTIONS: {
			executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
			return true;
		}
		case SHORTCUT_TYPE_SET_UNKNOWNS: {
			set_unknowns();
			return true;
		}
		case SHORTCUT_TYPE_RPN_UP: {
			if(!rpn_mode) return false;
			stack_view_rotate(true);
			return true;
		}
		case SHORTCUT_TYPE_RPN_DOWN: {
			if(!rpn_mode) return false;
			stack_view_rotate(false);
			return true;
		}
		case SHORTCUT_TYPE_RPN_SWAP: {
			if(!rpn_mode) return false;
			stack_view_swap();
			return true;
		}
		case SHORTCUT_TYPE_RPN_COPY: {
			if(!rpn_mode) return false;
			stack_view_copy();
			return true;
		}
		case SHORTCUT_TYPE_RPN_LASTX: {
			if(!rpn_mode) return false;
			stack_view_lastx();
			return true;
		}
		case SHORTCUT_TYPE_RPN_DELETE: {
			if(!rpn_mode) return false;
			stack_view_pop();
			return true;
		}
		case SHORTCUT_TYPE_RPN_CLEAR: {
			if(!rpn_mode) return false;
			stack_view_clear();
			return true;
		}
		case SHORTCUT_TYPE_META_MODE: {
			for(size_t i = 0; i < modes.size(); i++) {
				if(equalsIgnoreCase(modes[i].name, value)) {
					load_mode(modes[i]);
					return true;
				}
			}
			show_message(_("Mode not found."), mainwindow);
			return true;
		}
		case SHORTCUT_TYPE_INPUT_BASE: {
			Number nbase; int base;
			base_from_string(value, base, nbase, true);
			CALCULATOR->setCustomInputBase(nbase);
			set_input_base(base);
			update_setbase();
			return true;
		}
		case SHORTCUT_TYPE_OUTPUT_BASE: {
			Number nbase; int base;
			base_from_string(value, base, nbase);
			CALCULATOR->setCustomOutputBase(nbase);
			set_output_base(base);
			update_setbase();
			return true;
		}
		case SHORTCUT_TYPE_EXACT_MODE: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact"))));
			return true;
		}
		case SHORTCUT_TYPE_DEGREES: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_degrees")), TRUE);
			return true;
		}
		case SHORTCUT_TYPE_RADIANS: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_radians")), TRUE);
			return true;
		}
		case SHORTCUT_TYPE_GRADIANS: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_gradians")), TRUE);
			return true;
		}
		case SHORTCUT_TYPE_FRACTIONS: {
			if(printops.number_fraction_format >= FRACTION_FRACTIONAL) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
			return true;
		}
		case SHORTCUT_TYPE_MIXED_FRACTIONS: {
			if(printops.number_fraction_format == FRACTION_COMBINED) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
			return true;
		}
		case SHORTCUT_TYPE_SCIENTIFIC_NOTATION: {
			if(printops.min_exp == EXP_SCIENTIFIC) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0);
			else gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 2);
			return true;
		}
		case SHORTCUT_TYPE_SIMPLE_NOTATION: {
			if(printops.min_exp == EXP_NONE) gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 0);
			else gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(main_builder, "combobox_numerical_display")), 4);
			return true;
		}
		case SHORTCUT_TYPE_RPN_MODE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), !rpn_mode);
			return true;
		}
		case SHORTCUT_TYPE_AUTOCALC: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_autocalc")), !auto_calculate);
			return true;
		}
		case SHORTCUT_TYPE_PROGRAMMING: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_programmers_keypad")), ~visible_keypad & PROGRAMMING_KEYPAD);
			if(visible_keypad & PROGRAMMING_KEYPAD) gtk_expander_set_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_keypad")), true);
			return true;
		}
		case SHORTCUT_TYPE_KEYPAD: {
			//void on_expander_history_expanded(GObject *o, GParamSpec*, gpointer)
			gtk_expander_set_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_keypad")), !gtk_expander_get_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_keypad"))));
			return true;
		}
		case SHORTCUT_TYPE_HISTORY: {
			gtk_expander_set_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_history")), !gtk_expander_get_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_history"))));
			return true;
		}
		case SHORTCUT_TYPE_HISTORY_SEARCH: {
			set_minimal_mode(false);
			gtk_expander_set_expanded(GTK_EXPANDER(expander_history), TRUE);
			history_search();
			return true;
		}
		case SHORTCUT_TYPE_HISTORY_CLEAR: {
			history_clear();
			clear_expression_history();
			return true;
		}
		case SHORTCUT_TYPE_CONVERSION: {
			gtk_expander_set_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_convert")), !gtk_expander_get_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_convert"))));
			return true;
		}
		case SHORTCUT_TYPE_STACK: {
			gtk_expander_set_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_stack")), !gtk_expander_get_expanded(GTK_EXPANDER(gtk_builder_get_object(main_builder, "expander_stack"))));
			return true;
		}
		case SHORTCUT_TYPE_MINIMAL: {
			set_minimal_mode(!minimal_mode);
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_VARIABLES: {
			manage_variables(GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {
			manage_functions(GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_UNITS: {
			manage_units(GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_DATA_SETS: {
			manage_datasets(GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_STORE: {
			add_as_variable();
			return true;
		}
		case SHORTCUT_TYPE_MEMORY_CLEAR: {
			memory_clear();
			return true;
		}
		case SHORTCUT_TYPE_MEMORY_RECALL: {
			memory_recall();
			return true;
		}
		case SHORTCUT_TYPE_MEMORY_STORE: {
			memory_store();
			return true;
		}
		case SHORTCUT_TYPE_MEMORY_ADD: {
			memory_add();
			return true;
		}
		case SHORTCUT_TYPE_MEMORY_SUBTRACT: {
			memory_subtract();
			return true;
		}
		case SHORTCUT_TYPE_NEW_VARIABLE: {
			edit_variable(NULL, NULL, NULL, GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_NEW_FUNCTION: {
			edit_function("", NULL, GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_PLOT: {
			open_plot();
			return true;
		}
		case SHORTCUT_TYPE_NUMBER_BASES: {
			open_convert_number_bases();
			return true;
		}
		case SHORTCUT_TYPE_FLOATING_POINT: {
			open_convert_floatingpoint();
			return true;
		}
		case SHORTCUT_TYPE_CALENDARS: {
			open_calendarconversion();
			return true;
		}
		case SHORTCUT_TYPE_PERCENTAGE_TOOL: {
			open_percentage_tool();
			return true;
		}
		case SHORTCUT_TYPE_PERIODIC_TABLE: {
			show_periodic_table(GTK_WINDOW(mainwindow));
			return true;
		}
		case SHORTCUT_TYPE_UPDATE_EXRATES: {
			update_exchange_rates();
			return true;
		}
		case SHORTCUT_TYPE_COPY_RESULT: {
			copy_result(-1, value.empty() ? 0 : s2i(value));
			return true;
		}
		case SHORTCUT_TYPE_INSERT_RESULT: {
			if(!result_text_empty()) insert_text(get_result_text().c_str());
			return true;
		}
		case SHORTCUT_TYPE_SAVE_IMAGE: {
			save_as_image();
			return true;
		}
		case SHORTCUT_TYPE_HELP: {
			show_help("index.html", mainwindow);
			return true;
		}
		case SHORTCUT_TYPE_QUIT: {
			qalculate_quit();
			return true;
		}
		case SHORTCUT_TYPE_CHAIN_MODE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), !chain_mode);
			return true;
		}
		case SHORTCUT_TYPE_ALWAYS_ON_TOP: {
			always_on_top = !always_on_top;
			aot_changed = true;
			gtk_window_set_keep_above(GTK_WINDOW(mainwindow), always_on_top);
			preferences_update_keep_above();
			return true;
		}
		case SHORTCUT_TYPE_DO_COMPLETION: {
			toggle_completion_visible();
			return true;
		}
		case SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION: {
			return activate_first_completion();
		}
		case SHORTCUT_TYPE_PRECISION: {
			int v = s2i(value);
			if(previous_precision > 0 && CALCULATOR->getPrecision() == v) {
				v = previous_precision;
				previous_precision = 0;
			} else {
				previous_precision = CALCULATOR->getPrecision();
			}
			CALCULATOR->setPrecision(v);
			update_precision();
			expression_calculation_updated();
			return true;
		}
		case SHORTCUT_TYPE_MAX_DECIMALS: {}
		case SHORTCUT_TYPE_MIN_DECIMALS: {}
		case SHORTCUT_TYPE_MINMAX_DECIMALS: {
			int v = s2i(value);
			if((type == SHORTCUT_TYPE_MIN_DECIMALS || (printops.use_max_decimals && printops.max_decimals == v)) && (type == SHORTCUT_TYPE_MAX_DECIMALS || (printops.use_min_decimals && printops.min_decimals == v))) v = -1;
			if(type != SHORTCUT_TYPE_MAX_DECIMALS) {
				if(v >= 0) printops.min_decimals = v;
				printops.use_min_decimals = v >= 0;
			}
			if(type != SHORTCUT_TYPE_MIN_DECIMALS) {
				if(v >= 0) printops.max_decimals = v;
				printops.use_max_decimals = v >= 0;
			}
			result_format_updated();
			update_decimals();
			return true;
		}
	}
	return false;
}
bool do_keyboard_shortcut(GdkEventKey *event) {
	guint state = CLEAN_MODIFIERS(event->state);
	FIX_ALT_GR
	unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find((guint64) event->keyval + (guint64) G_MAXUINT32 * (guint64) state);
	if(it == keyboard_shortcuts.end() && event->keyval == GDK_KEY_KP_Delete) it = keyboard_shortcuts.find((guint64) GDK_KEY_Delete + (guint64) G_MAXUINT32 * (guint64) state);
	if(it != keyboard_shortcuts.end()) {
		bool b = false;
		for(size_t i = 0; i < it->second.type.size(); i++) {
			if(do_shortcut(it->second.type[i], it->second.value[i])) b = true;
		}
		return b;
	}
	return false;
}

gboolean on_configure_event(GtkWidget*, GdkEventConfigure *event, gpointer) {
	if(minimal_mode) {
		if(minimal_window_resized_timeout_id) g_source_remove(minimal_window_resized_timeout_id);
		minimal_window_resized_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000, minimal_window_resized_timeout, NULL, NULL);
	}
	return FALSE;
}

gboolean on_resultspinner_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	if(event->button != 1 || !gtk_widget_is_visible(w)) return FALSE;
	if(b_busy_command) on_abort_command(NULL, 0, NULL);
	else if(b_busy_expression) on_abort_calculation(NULL, 0, NULL);
	else if(b_busy_result) on_abort_display(NULL, 0, NULL);
	return TRUE;
}

bool disable_history_arrow_keys = false;
gboolean on_key_release_event(GtkWidget*, GdkEventKey*, gpointer) {
	disable_history_arrow_keys = false;
	return FALSE;
}
gboolean on_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(block_input && (event->keyval == GDK_KEY_q || event->keyval == GDK_KEY_Q) && !(event->state & GDK_CONTROL_MASK)) {block_input = false; return TRUE;}
	if(gtk_widget_has_focus(expressiontext) || editing_stack() || editing_history()) return FALSE;
	if(!b_busy && gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "mb_to"))) && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "mb_to"))) && (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_ISO_Enter || event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_space)) {update_mb_to_menu(); gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "mb_to")));}
	if((event->keyval == GDK_KEY_ISO_Left_Tab || event->keyval == GDK_KEY_Tab) && (CLEAN_MODIFIERS(event->state) == 0 || CLEAN_MODIFIERS(event->state) == GDK_SHIFT_MASK)) return FALSE;
	if(do_keyboard_shortcut(event)) return TRUE;
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_unit")))) {
		return FALSE;
	}
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_search")))) {
		if(event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_Page_Up || event->keyval == GDK_KEY_Page_Down || event->keyval == GDK_KEY_KP_Page_Up || event->keyval == GDK_KEY_KP_Page_Down) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_unit")));
		}
		return FALSE;
	}
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_unit")))) {
		if(!(event->keyval >= GDK_KEY_KP_Multiply && event->keyval <= GDK_KEY_KP_9) && !(event->keyval >= GDK_KEY_parenleft && event->keyval <= GDK_KEY_A)) {
			if(gdk_keyval_to_unicode(event->keyval) > 32) {
				if(!gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_search")))) {
					gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_search")));
				}
			}
			return FALSE;
		}
	}
	if(gtk_widget_has_focus(historyview)) {
		guint state = CLEAN_MODIFIERS(event->state);
		FIX_ALT_GR
		if((state == 0 && (event->keyval == GDK_KEY_F2 || event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return)) || (state == GDK_CONTROL_MASK && event->keyval == GDK_KEY_c) || (state == GDK_SHIFT_MASK && event->keyval == GDK_KEY_Delete)) {
			return FALSE;
		}
	}
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_category")))) {
		if(!(event->keyval >= GDK_KEY_KP_Multiply && event->keyval <= GDK_KEY_KP_9) && !(event->keyval >= GDK_KEY_parenleft && event->keyval <= GDK_KEY_A)) {
			return FALSE;
		}
	}
	if(gtk_widget_has_focus(historyview) && event->keyval == GDK_KEY_F2) return FALSE;
	if(event->keyval > GDK_KEY_Hyper_R || event->keyval < GDK_KEY_Shift_L) {
		GtkWidget *w = gtk_window_get_focus(GTK_WINDOW(mainwindow));
		if(w && gtk_bindings_activate_event(G_OBJECT(w), event)) return TRUE;
		if(gtk_bindings_activate_event(G_OBJECT(o), event)) return TRUE;
		focus_keeping_selection();
	}
	return FALSE;
}


gboolean on_resultview_draw(GtkWidget *widget, cairo_t *cr, gpointer) {
	if(exit_in_progress) return TRUE;
	gint scalefactor = gtk_widget_get_scale_factor(widget);
	gtk_render_background(gtk_widget_get_style_context(widget), cr, 0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
	result_display_overflow = false;
	if(surface_result || (surface_parsed && ((parsed_in_result && !rpn_mode) || show_parsed_instead_of_result))) {
		gint w = 0, h = 0;
		if(!first_draw_of_result) {
			if(b_busy) {
				if(b_busy_result) return TRUE;
			} else if((!surface_result || show_parsed_instead_of_result) && displayed_parsed_mstruct) {
				gint rw = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result"))) - 12;
				displayed_printops.can_display_unicode_string_arg = (void*) resultview;
				tmp_surface = draw_structure(*displayed_parsed_mstruct, parsed_printops, false, top_ips, NULL, 3, NULL, NULL, NULL, rw, true, &mwhere, &displayed_parsed_to);
				parsed_printops.can_display_unicode_string_arg = NULL;
				cairo_surface_destroy(surface_parsed);
				surface_parsed = tmp_surface;
			} else if(display_aborted || result_too_long) {
				PangoLayout *layout = gtk_widget_create_pango_layout(widget, NULL);
				pango_layout_set_markup(layout, display_aborted ? _("result processing was aborted") : _("result is too long\nsee history"), -1);
				pango_layout_get_pixel_size(layout, &w, &h);
				PangoRectangle rect;
				pango_layout_get_pixel_extents(layout, &rect, NULL);
				if(rect.x < 0) {w -= rect.x; if(rect.width > w) w = rect.width;}
				else if(w < rect.x + rect.width) w = rect.x + rect.width;
				cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
				cairo_surface_set_device_scale(s, scalefactor, scalefactor);
				cairo_t *cr2 = cairo_create(s);
				GdkRGBA rgba;
				gtk_style_context_get_color(gtk_widget_get_style_context(widget), gtk_widget_get_state_flags(widget), &rgba);
				gdk_cairo_set_source_rgba(cr2, &rgba);
				if(rect.x < 0) cairo_move_to(cr, -rect.x, 0);
				pango_cairo_show_layout(cr2, layout);
				cairo_destroy(cr2);
				cairo_surface_destroy(surface_result);
				surface_result = s;
				tmp_surface = s;
			} else if(surface_result && displayed_mstruct) {
				if(displayed_mstruct->isAborted()) {
					PangoLayout *layout = gtk_widget_create_pango_layout(widget, NULL);
					pango_layout_set_markup(layout, _("calculation was aborted"), -1);
					gint w = 0, h = 0;
					pango_layout_get_pixel_size(layout, &w, &h);
					PangoRectangle rect;
					pango_layout_get_pixel_extents(layout, &rect, NULL);
					if(rect.x < 0) {w -= rect.x; if(rect.width > w) w = rect.width;}
					else if(w < rect.x + rect.width) w = rect.x + rect.width;
					tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
					cairo_surface_set_device_scale(tmp_surface, scalefactor, scalefactor);
					cairo_t *cr2 = cairo_create(tmp_surface);
					GdkRGBA rgba;
					gtk_style_context_get_color(gtk_widget_get_style_context(widget), gtk_widget_get_state_flags(widget), &rgba);
					gdk_cairo_set_source_rgba(cr2, &rgba);
					if(rect.x < 0) cairo_move_to(cr, -rect.x, 0);
					pango_cairo_show_layout(cr2, layout);
					cairo_destroy(cr2);
					g_object_unref(layout);
				} else {
					gint rw = -1;
					if(scale_n == 3) rw = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result"))) - 12;
					displayed_printops.can_display_unicode_string_arg = (void*) resultview;
					tmp_surface = draw_structure(*displayed_mstruct, displayed_printops, displayed_caf, top_ips, NULL, scale_n, NULL, NULL, NULL, rw);
					displayed_printops.can_display_unicode_string_arg = NULL;
				}
				cairo_surface_destroy(surface_result);
				surface_result = tmp_surface;
			}
		}
		cairo_surface_t *surface = surface_result;
		if(show_parsed_instead_of_result && !surface_parsed) show_parsed_instead_of_result = false;
		if(!surface || show_parsed_instead_of_result) {
			surface = surface_parsed;
			scale_n = 3;
		}
		w = cairo_image_surface_get_width(surface) / scalefactor;
		h = cairo_image_surface_get_height(surface) / scalefactor;
		gint sbw, sbh;
		gtk_widget_get_preferred_width(gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result"))), NULL, &sbw);
		gtk_widget_get_preferred_height(gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result"))), NULL, &sbh);
		gint rh = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")));
		gint rw = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result"))) - 12;
		if((surface_result && !show_parsed_instead_of_result) && (first_draw_of_result || (!b_busy && result_font_updated))) {
			gint margin = 24;
			while(displayed_mstruct && !display_aborted && !result_too_long && scale_n < 3 && (w > rw || (w > rw - sbw ? h + margin / 1.5 > rh - sbh : h + margin > rh))) {
				int scroll_diff = gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result"))) - gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")));
				double scale_div = (double) h / (gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport"))) + scroll_diff);
				if(scale_div > 1.44) {
					scale_n = 3;
				} else if(scale_n < 2 && scale_div > 1.2) {
					scale_n = 2;
				} else {
					scale_n++;
				}
				cairo_surface_destroy(surface);
				displayed_printops.can_display_unicode_string_arg = (void*) resultview;
				surface_result = draw_structure(*displayed_mstruct, displayed_printops, displayed_caf, top_ips, NULL, scale_n, NULL, NULL, NULL, scale_n == 3 ? rw : -1);
				surface = surface_result;
				displayed_printops.can_display_unicode_string_arg = NULL;
				w = cairo_image_surface_get_width(surface) / scalefactor;
				h = cairo_image_surface_get_height(surface) / scalefactor;
				if(scale_n == 1) margin = 18;
				else margin = 12;
			}
			result_font_updated = false;
		}
		gtk_widget_set_size_request(widget, w, h);
		if(h > sbh) rw -= sbw;
		result_display_overflow = w > rw || h > rh;
		gint rx = 0, ry = 0;
		if(rw >= w) {
			if(surface_result && !show_parsed_instead_of_result) {
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
				// compensate for overlay scrollbars
				if(rw >= w + 5) rx = rw - w - 5;
#else
				if(rw >= w) rx = rw - w;
#endif
				else rx = rw - w - (rw - w) / 2;
			} else {
				rx = 12;
			}
			if(h < rh) ry = (rh - h) / 2;
		} else {
			if(h + ((rh - h) / 2) < rh - sbh) ry = (rh - h) / 2;
			else if(h <= rh - sbh) ry = (rh - h - sbh) / 2;
		}
		cairo_set_source_surface(cr, surface, rx, ry);
		binary_x_diff = rx;
		binary_y_diff = ry;
		cairo_paint(cr);
		if(!surface_result && result_display_overflow) {
			GtkWidget *hscroll = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result")));
			if(hscroll) {
				gtk_range_set_value(GTK_RANGE(hscroll), gtk_adjustment_get_upper(gtk_range_get_adjustment(GTK_RANGE(hscroll))));
			}
		}
		first_draw_of_result = false;
	} else if(showing_first_time_message) {
		PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview")), NULL);
		GdkRGBA rgba;
		pango_layout_set_markup(layout, (string("<span size=\"smaller\">") + string(_("Type a mathematical expression above, e.g. \"5 + 2 / 3\",\nand press the enter key.")) + "</span>").c_str(), -1);
		gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))), gtk_widget_get_state_flags(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"))), &rgba);
		cairo_move_to(cr, 6, 6);
		gdk_cairo_set_source_rgba(cr, &rgba);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);
	} else {
		gtk_widget_set_size_request(widget, -1, -1);
	}
	return TRUE;
}

void on_type_label_date_clicked(GtkEntry *w, gpointer) {
	GtkWidget *d = gtk_dialog_new_with_buttons(_("Select date"), GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW)), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
	GtkWidget *date_w = gtk_calendar_new();
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), date_w);
	gtk_widget_show_all(d);
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_OK) {
		guint year = 0, month = 0, day = 0;
		gtk_calendar_get_date(GTK_CALENDAR(date_w), &year, &month, &day);
		gchar *gstr = g_strdup_printf("%i-%02i-%02i", year, month + 1, day);
		gtk_entry_set_text(w, gstr);
		g_free(gstr);
	}
	gtk_widget_destroy(d);
}
void on_type_label_vector_clicked(GtkEntry *w, gpointer user_data) {
	MathStructure mstruct_v;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	if(!str.empty()) {
		if(str[0] != '(' && str[0] != '[') {
			str.insert(0, 1, '[');
			str += ']';
		}
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_v, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
	}
	insert_matrix(str.empty() ? NULL : &mstruct_v, gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW), TRUE, false, false, w);
}
void on_type_label_matrix_clicked(GtkEntry *w, gpointer user_data) {
	MathStructure mstruct_m;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	if(!str.empty()) {
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_m, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
		if(!mstruct_m.isMatrix()) str = "";
	}
	insert_matrix(str.empty() ? NULL : &mstruct_m, gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW), FALSE, false, false, w);
}
void on_type_label_file_clicked(GtkEntry *w, gpointer) {
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select file"), GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW)), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Open"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file"), GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW)), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
#endif
	string filestr = gtk_entry_get_text(w);
	remove_blank_ends(filestr);
	if(!filestr.empty()) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#else
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#endif
		gtk_entry_set_text(w, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d)));
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	g_object_unref(d);
#else
	gtk_widget_destroy(d);
#endif
}

void set_unknowns() {
	if(expression_modified() && !expression_is_empty() && !rpn_mode) execute_expression(true);
	MathStructure unknowns;
	mstruct->findAllUnknowns(unknowns);
	if(unknowns.size() == 0) {
		show_message(_("No unknowns in result."), mainwindow);
		return;
	}
	unknowns.setType(STRUCT_ADDITION);
	unknowns.sort();

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Set Unknowns"), GTK_WINDOW(mainwindow), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_REJECT, _("_Apply"), GTK_RESPONSE_APPLY, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
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
		g_signal_connect(G_OBJECT(entry[i]), "key-press-event", G_CALLBACK(on_math_entry_key_press_event), NULL);
		gtk_widget_set_hexpand(entry[i], TRUE);
		gtk_grid_attach(GTK_GRID(ptable), entry[i], 1, rows - 1, 1, 1);
	}
	MathStructure msave(*mstruct);
	string result_save = get_result_text();
	gtk_widget_show_all(dialog);
	bool b_changed = false;
	vector<string> unknown_text;
	unknown_text.resize(unknowns.size());
	while(true) {
		gint response = gtk_dialog_run(GTK_DIALOG(dialog));
		bool b1 = false, b2 = false;
		if(response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_APPLY) {
			string str, result_mod = "";
			block_error();
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
				executeCommand(COMMAND_TRANSFORM, true, false, result_mod);
			} else if(b1) {
				b_changed = false;
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
				setResult(NULL, true, false, false, result_mod);
			}
			unblock_error();
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
void open_convert_number_bases() {
	if(current_displayed_result() && !result_text_empty() && !result_too_long) return convert_number_bases(GTK_WINDOW(mainwindow), ((current_result()->isNumber() && !current_result()->number().hasImaginaryPart()) || current_result()->isUndefined()) ? get_result_text().c_str() : "", displayed_printops.base);
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	convert_number_bases(GTK_WINDOW(mainwindow), str.c_str(), evalops.parse_options.base);
}
void open_convert_floatingpoint() {
	if(current_displayed_result() && !result_text_empty() && !result_too_long) return convert_floatingpoint(((current_result()->isNumber() && !current_result()->number().hasImaginaryPart()) || current_result()->isUndefined()) ? get_result_text().c_str() : "", true, GTK_WINDOW(mainwindow));
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	convert_floatingpoint(str.c_str(), false, GTK_WINDOW(mainwindow));
}
void open_percentage_tool() {
	if(!result_text_empty()) return show_percentage_dialog(GTK_WINDOW(mainwindow), get_result_text().c_str());
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	show_percentage_dialog(GTK_WINDOW(mainwindow), str.c_str());
}
void open_calendarconversion() {
	show_calendarconversion_dialog(GTK_WINDOW(mainwindow), current_displayed_result() && current_result() && current_result()->isDateTime() ? current_result()->datetime() : NULL);
}
void show_unit_conversion() {
	gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
	focus_conversion_entry();
}
#ifdef __cplusplus
}
#endif
void update_status_menu(bool initial) {
	if(initial) {
		switch(evalops.approximation) {
			case APPROXIMATION_EXACT: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), TRUE);
				break;
			}
			case APPROXIMATION_TRY_EXACT: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), FALSE);
				break;
			}
			default: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), FALSE);
				break;
			}
		}
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_read_precision")), evalops.parse_options.read_precision != DONT_READ_PRECISION);
		switch(evalops.parse_options.parsing_mode) {
			case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace")), TRUE);
				break;
			}
			case PARSING_MODE_CONVENTIONAL: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication")), TRUE);
				break;
			}
			case PARSING_MODE_CHAIN: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax")), TRUE);
				break;
			}
			case PARSING_MODE_RPN: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax")), TRUE);
				break;
			}
			default: {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing")), TRUE);
				break;
			}
		}
	} else {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_exact_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_exact")), evalops.approximation == APPROXIMATION_EXACT);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_exact_activate, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_read_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_read_precision_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_read_precision")), evalops.parse_options.read_precision != DONT_READ_PRECISION);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_read_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_read_precision_activate, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_rpn_syntax_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax")), evalops.parse_options.parsing_mode == PARSING_MODE_RPN);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_rpn_syntax_activate, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_adaptive_parsing_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing")), evalops.parse_options.parsing_mode == PARSING_MODE_ADAPTIVE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_adaptive_parsing"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_adaptive_parsing_activate, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_ignore_whitespace_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace")), evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_ignore_whitespace"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_ignore_whitespace_activate, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_no_special_implicit_multiplication_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication")), evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_no_special_implicit_multiplication"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_no_special_implicit_multiplication_activate, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_chain_syntax_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax")), evalops.parse_options.parsing_mode == PARSING_MODE_CHAIN);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_status_chain_syntax"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_status_chain_syntax_activate, NULL);
	}
}
