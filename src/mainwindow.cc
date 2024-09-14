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
#include <sys/stat.h>
#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <time.h>
#include <limits>
#include <fstream>
#include <sstream>

#include "support.h"
#include "mainwindow.h"
#include "settings.h"
#include "util.h"
#include <stack>
#include <deque>

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::iterator;
using std::list;
using std::deque;
using std::stack;

int block_error_timeout = 0;

KnownVariable *vans[5], *v_memory;
extern string selected_function_category;
extern MathFunction *selected_function;
extern string selected_variable_category;
extern Variable *selected_variable;
extern string selected_unit_category;
extern Unit *selected_unit;
extern DataSet *selected_dataset;
bool save_mode_on_exit;
bool save_defs_on_exit;
bool save_history_separately = false;
int gtk_theme = -1;
bool use_custom_app_font;
bool save_custom_app_font = false;
string custom_app_font;
bool hyp_is_on, inv_is_on;
bool show_keypad, show_history, show_stack, show_convert, persistent_keypad, minimal_mode;
bool copy_ascii = false;
bool copy_ascii_without_units = false;
bool caret_as_xor = false;
int close_with_esc = -1;
extern bool first_time;
extern int allow_multiple_instances;
int b_decimal_comma;
bool first_error;
bool display_expression_status;
MathStructure *mstruct = NULL, *matrix_mstruct = NULL, *parsed_mstruct = NULL, *parsed_tostruct = NULL;
MathStructure mbak_convert;
string result_text, parsed_text;
bool result_text_approximate = false;
string result_text_long;
vector<vector<GtkWidget*> > insert_element_entries;
bool b_busy = false, b_busy_command = false, b_busy_result = false, b_busy_expression = false;
int block_result_update = 0, block_expression_execution = 0;
extern int visible_keypad;
extern int programming_inbase, programming_outbase;
bool title_modified = false;
int simplified_percentage = -1;
int version_numbers[3];

extern bool cursor_has_moved;

int enable_tooltips = 1;
bool toe_changed = false;

int previous_precision = 0;

string custom_angle_unit;

string command_convert_units_string;
Unit *command_convert_unit;

bool remember_position = false, always_on_top = false, aot_changed = false;

gint minimal_width;

string text_color;

bool stop_timeouts = false;

PrintOptions printops;
EvaluationOptions evalops;
bool dot_question_asked = false, implicit_question_asked = false;

bool rpn_mode, rpn_keys;
bool adaptive_interval_display;

bool tc_set = false, sinc_set = false;

bool use_systray_icon = false, hide_on_startup = false;

Thread *view_thread, *command_thread;
bool exit_in_progress = false, command_aborted = false;

bool text_color_set;

extern QalculateDateTime last_version_check_date;
string last_found_version;

bool automatic_fraction = false;
int default_fraction_fraction = -1;
bool scientific_negexp = true;
bool scientific_notminuslast = true;
bool scientific_noprefix = true;
int auto_prefix = 0;

bool ignore_locale = false;
string custom_lang;

bool auto_calculate = false;
bool result_autocalculated = false;
gint autocalc_history_timeout_id = 0;
int autocalc_history_delay = 2000;
bool chain_mode = false;

bool parsed_in_result = false;

int to_fraction = 0;
long int to_fixed_fraction = 0;
char to_prefix = 0;
int to_base = 0;
bool to_duo_syms = false;
int to_caf = -1;
unsigned int to_bits = 0;
Number to_nbase;

bool do_imaginary_j = false;
bool complex_angle_form = false;

bool default_shortcuts;

extern bool check_version;

class ViewThread : public Thread {
protected:
	virtual void run();
};

class CommandThread : public Thread {
protected:
	virtual void run();
};

enum {
	TITLE_APP,
	TITLE_RESULT,
	TITLE_APP_RESULT,
	TITLE_MODE,
	TITLE_APP_MODE
};

#include "exchangerates.h"
#include "floatingpointdialog.h"
#include "calendarconversiondialog.h"
#include "percentagecalculationdialog.h"
#include "numberbasesdialog.h"
#include "periodictabledialog.h"
#include "plotdialog.h"
#include "functionsdialog.h"
#include "insertfunctiondialog.h"
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
#include "openhelp.h"
#include "conversionview.h"
#include "expressionedit.h"
#include "expressionstatus.h"
#include "expressioncompletion.h"
#include "historyview.h"
#include "stackview.h"
#include "resultview.h"
#include "keypad.h"
#include "menubar.h"
#include "modes.h"

int title_type = TITLE_APP;

GtkBuilder *main_builder = NULL;

GtkWindow *mainwindow;

GtkWidget *tabs, *expander_keypad, *expander_history, *expander_stack, *expander_convert;

GtkAccelGroup *accel_group;

GtkCssProvider *topframe_provider = NULL, *app_provider = NULL, *app_provider_theme = NULL, *color_provider = NULL;

string themestr;

gint win_height = -1, win_width = -1, win_x = 0, win_y = 0, win_monitor = 0, history_height = 0;
bool win_monitor_primary = false;
gint hidden_x = -1, hidden_y = -1, hidden_monitor = 1;
bool hidden_monitor_primary = false;

unordered_map<string, cairo_surface_t*> flag_surfaces;
int flagheight;

DECLARE_BUILTIN_FUNCTION(SetTitleFunction, 0)

SetTitleFunction::SetTitleFunction() : MathFunction("settitle", 1, 1, CALCULATOR->f_warning->category(), _("Set Window Title")) {
	setArgumentDefinition(1, new TextArgument());
}
int SetTitleFunction::calculate(MathStructure&, const MathStructure &vargs, const EvaluationOptions&) {
	gtk_window_set_title(main_window(), vargs[0].symbol().c_str());
	title_modified = true;
	return 1;
}

MathStructure *current_result() {return mstruct;}
void replace_current_result(MathStructure *m) {
	mstruct->unref();
	mstruct = m;
	mstruct->ref();
}
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

string copy_text;

void end_cb(GtkClipboard*, gpointer) {}
void get_cb(GtkClipboard*, GtkSelectionData* sd, guint info, gpointer) {
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
					string str2 = (i2 != string::npos ? str.substr(i2 + 1, str.length() - (i2 + 1)) : str);
					gsub(DOT, "", str2);
					gsub(COMMA, "", str2);
					CALCULATOR->parse(&m, str2, po);
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

void replace_result_cis_gtk(string &resstr) {
	if(can_display_unicode_string_function_exact("∠", (void*) history_view_widget())) gsub(" cis ", "∠", resstr);
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
	block_error_timeout++;
}
void unblock_error() {
	block_error_timeout--;
}
bool error_blocked() {
	return block_error_timeout > 0;
}
bool result_is_autocalculated() {
	return result_autocalculated;
}

void clearresult() {
	if(!current_parsed_expression_is_displayed_in_result() || rpn_mode) minimal_mode_show_resultview(false);
	if(!parsed_in_result) result_autocalculated = false;
	result_view_clear();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
	if(gtk_revealer_get_child_revealed(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")))) {
		gtk_info_bar_response(GTK_INFO_BAR(gtk_builder_get_object(main_builder, "message_bar")), GTK_RESPONSE_CLOSE);
	}
	update_expression_icons();
	if(visible_keypad & PROGRAMMING_KEYPAD) clear_result_bases();
}
void clear_parsed_in_result() {
	result_view_clear_parsed();
	if(result_view_empty()) minimal_mode_show_resultview(false);
}
void show_parsed_in_result(MathStructure &mparse, const PrintOptions &po) {
	draw_parsed(mparse, po);
	minimal_mode_show_resultview();
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
	if(type == 8) str += current_parsed_expression_text();
	else if(type <= 3 || type > 5) str += result_text;
	set_clipboard(str, ascii, type <= 3 || type > 5, type <= 3 || type == 8, copy_without_units);
}

bool result_text_empty() {
	return result_text.empty() && !autocalc_history_timeout_id;
}
const string &current_result_text() {
	return result_text;
}
bool current_result_text_is_approximate() {
	return result_text_approximate;
}
string get_result_text() {
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		do_autocalc_history_timeout(NULL);
	}
	return unhtmlize(result_text);
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
bool is_memory_variable(Variable *v) {
	return v == v_memory;
}

bool expression_display_errors(GtkWindow *win, int type, bool do_exrate_sources, string &str, int mtype_highest) {
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
			GtkWidget *edialog = gtk_message_dialog_new(win, GTK_DIALOG_DESTROY_WITH_PARENT, mtype_highest == MESSAGE_ERROR ? GTK_MESSAGE_ERROR : (mtype_highest == MESSAGE_WARNING ? GTK_MESSAGE_WARNING : GTK_MESSAGE_INFO), GTK_BUTTONS_CLOSE, "%s", str.c_str());
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
bool display_errors(GtkWindow *win, int type, bool add_to_history) {
	int mtype_highest = MESSAGE_INFORMATION;
	string str = history_display_errors(add_to_history, win, type, NULL, time(NULL), &mtype_highest);
	return expression_display_errors(win, type, add_to_history, str, mtype_highest);

}

gboolean on_display_errors_timeout(gpointer) {
	if(stop_timeouts) return FALSE;
	if(error_blocked()) return TRUE;
	if(CALCULATOR->checkSaveFunctionCalled()) {
		update_vmenu(false);
		update_fmenu(false);
		update_umenus();
	}
	display_errors();
	return TRUE;
}

#ifdef AUTO_UPDATE
void auto_update(string new_version, string url) {
	char selfpath[1000];
	ssize_t n = readlink("/proc/self/exe", selfpath, 999);
	if(n < 0 || n >= 999) {
		GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Path of executable not found."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}
	selfpath[n] = '\0';
	gchar *selfdir = g_path_get_dirname(selfpath);
	FILE *pipe = popen("curl --version 1>/dev/null", "w");
	if(!pipe) {
		GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("curl not found."));
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
		GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Failed to run update script.\n%s"), error_str);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(error_str);
		g_error_free(error);
		return;
	}
	qalculate_quit();
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
		GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, ret < 0 ? GTK_MESSAGE_ERROR : GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, ret < 0 ? _("Failed to check for updates.") : _("No updates found."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(ret < 0) return;
	}
	if(ret > 0 && (!do_not_show_again || new_version != last_found_version)) {
		last_found_version = new_version;
#ifdef AUTO_UPDATE
		GtkWidget *dialog = gtk_dialog_new_with_buttons(NULL, main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
#else
#	ifdef _WIN32
		GtkWidget *dialog = NULL;
		if(url.empty()) dialog = gtk_dialog_new_with_buttons(NULL, main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Close"), GTK_RESPONSE_REJECT, NULL);
		else dialog = gtk_dialog_new_with_buttons(NULL, main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Download"), GTK_RESPONSE_ACCEPT, _("_Close"), GTK_RESPONSE_REJECT, NULL);
#	else
		GtkWidget *dialog = gtk_dialog_new_with_buttons(NULL, main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Close"), GTK_RESPONSE_REJECT, NULL);
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Temperature Calculation Mode"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Sinc Function"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Interpretation of dots"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Parsing Mode"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Percentage Interpretation"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
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
bool autocalculation_stopped_at_operator() {
	return auto_calc_stopped_at_operator;
}

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
bool contains_fraction_gtk(const MathStructure &m) {
	if(m.isNumber()) return !m.number().isInteger();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_fraction_gtk(m[i])) return true;
	}
	return false;
}

string prev_autocalc_str;
MathStructure mauto;

void do_auto_calc(int recalculate = 1, std::string str = std::string()) {
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
			GtkTextMark *mark = gtk_text_buffer_get_insert(expression_edit_buffer());
			if(mark) {
				GtkTextIter ipos;
				gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mark);
				bool b_to = CALCULATOR->hasToExpression(str, false, evalops) || CALCULATOR->hasWhereExpression(str, evalops);
				if(gtk_text_iter_is_end(&ipos)) {
					if(last_is_operator(str, evalops.parse_options.base == 10) && (evalops.parse_options.base != BASE_ROMAN_NUMERALS || str[str.length() - 1] != '|' || str.find('|') == str.length() - 1)) {
						size_t n = 1;
						while(n < str.length() && (char) str[str.length() - n] < 0 && (unsigned char) str[str.length() - n] < 0xC0) n++;
						if((b_to && n == 1 && (str[str.length() - 1] != ' ' || str[str.length() - 1] != '/')) || n == str.length() || (display_expression_status && !b_to && parsed_mstruct->equals(current_parsed_expression(), true, true)) || ((!display_expression_status || b_to) && str.length() - n == prev_autocalc_str.length() && str.substr(0, str.length() - n) == prev_autocalc_str)) {
							auto_calc_stopped_at_operator = true;
							return;
						}
					}
				} else if(!b_to && display_expression_status) {
					GtkTextIter iter = ipos;
					gtk_text_iter_forward_char(&iter);
					gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &ipos, &iter, FALSE);
					string c2 = gstr;
					g_free(gstr);
					string c1;
					if(!gtk_text_iter_is_start(&ipos)) {
						iter = ipos;
						gtk_text_iter_backward_char(&iter);
						gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &iter, &ipos, FALSE);
						c1 = gstr;
						g_free(gstr);
					}
					if((c2.length() == 1 && is_in("*/^|&<>=)]", c2[0]) && (c2[0] != '|' || evalops.parse_options.base != BASE_ROMAN_NUMERALS)) || (c2.length() > 1 && (c2 == "∧" || c2 == "∨" || c2 == "⊻" || c2 == expression_times_sign() || c2 == expression_divide_sign() || c2 == SIGN_NOT_EQUAL || c2 == SIGN_GREATER_OR_EQUAL || c2 == SIGN_LESS_OR_EQUAL))) {
						if(c1.empty() || (c1.length() == 1 && is_in(OPERATORS LEFT_PARENTHESIS, c1[0]) && c1[0] != '!' && (c1[0] != '|' || (evalops.parse_options.base != BASE_ROMAN_NUMERALS && c1 != "|")) && (c1[0] != '&' || c2 != "&") && (c1[0] != '/' || (c2 != "/" && c2 != expression_divide_sign())) && (c1[0] != '*' || (c2 != "*" && c2 != expression_times_sign())) && ((c1[0] != '>' && c1[0] != '<') || (c2 != "=" && c2 != c1)) && ((c2 != ">" && c2 == "<") || (c1[0] != '=' && c1 != c2))) || (c1.length() > 1 && (c1 == "∧" || c1 == "∨" || c1 == "⊻" || c1 == SIGN_NOT_EQUAL || c1 == SIGN_GREATER_OR_EQUAL || c1 == SIGN_LESS_OR_EQUAL || (c1 == expression_times_sign() && c2 != "*" && c2 != expression_times_sign()) || (c1 == expression_divide_sign() && c2 != "/" && c2 != expression_divide_sign()) || c1 == expression_add_sign() || c1 == expression_sub_sign()))) {
							if(parsed_mstruct->equals(current_parsed_expression(), true, true)) {
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
		if(origstr && str_conv.empty() && conversionview_continuous_conversion() && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
			string ceu_str = current_conversion_expression();
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
			po.can_display_unicode_string_arg = (void*) parse_status_widget();
			po.allow_non_usable = true;
			result_text = displayed_mstruct_pre->print(printops, true);
			result_text_approximate = *po.is_approximate;
			fix_history_string_new2(result_text);
			gsub("&nbsp;", " ", result_text);
			result_text_long = "";
			if(CALCULATOR->aborted()) {
				CALCULATOR->endTemporaryStopMessages();
				result_text = "";
				result_autocalculated = false;
			} else {
				CALCULATOR->endTemporaryStopMessages(true);
				if(complex_angle_form) replace_result_cis_gtk(result_text);
				result_autocalculated = true;
				if(!display_errors(NULL, 1)) update_expression_icons(EXPRESSION_CLEAR);
				if(visible_keypad & PROGRAMMING_KEYPAD) {
					set_result_bases(*displayed_mstruct_pre);
					update_result_bases();
				}
			}
			displayed_mstruct_pre->unref();
			display_parse_status();
		} else {
			bool b = draw_result(displayed_mstruct_pre);
			CALCULATOR->endTemporaryStopMessages(b);
			if(b) {
				result_autocalculated = true;
				minimal_mode_show_resultview();
				if(autocalc_history_timeout_id == 0 || ((printops.base == BASE_BINARY || (printops.base <= BASE_FP16 && printops.base >= BASE_FP80)) && current_displayed_result()->isInteger()) || title_type == TITLE_RESULT || title_type == TITLE_APP_RESULT) {
					PrintOptions po = printops;
					po.base_display = BASE_DISPLAY_SUFFIX;
					po.can_display_unicode_string_arg = (void*) main_window();
					po.allow_non_usable = true;
					result_text = current_displayed_result()->print(po, true);
					gsub("&nbsp;", " ", result_text);
					if(complex_angle_form) replace_result_cis_gtk(result_text);
				} else {
					result_text = "";
				}
				result_text_long = "";
				gtk_widget_set_tooltip_text(result_view_widget(), "");
				if(!display_errors(NULL, 1)) update_expression_icons(EXPRESSION_CLEAR);
				if(visible_keypad & PROGRAMMING_KEYPAD) {
					set_result_bases(*current_displayed_result());
					update_result_bases();
				}
			} else {
				result_text = "";
			}
			if(title_type == TITLE_RESULT || title_type == TITLE_APP_RESULT) update_window_title(unhtmlize(result_text).c_str(), true);
		}
		CALCULATOR->stopControl();
	} else if(parsed_in_result) {
		result_text = "";
		result_autocalculated = false;
		display_parse_status();
	} else {
		auto_calculate = false;
		clearresult();
		result_text = "";
		auto_calculate = true;
		if(title_type == TITLE_RESULT || title_type == TITLE_APP_RESULT) update_window_title("", true);
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

void toggle_binary_pos(int pos) {
	size_t index = result_text.find("<");
	string new_binary = result_text;
	if(index != string::npos) new_binary = new_binary.substr(0, index);
	index = new_binary.length();
	int n = 0;
	for(; index > 0; index--) {
		if(result_text[index - 1] == '0' || result_text[index - 1] == '1') {
			if(n == pos) break;
			n++;
		} else if(result_text[index - 1] != ' ') {
			index = 0;
			break;
		}
	}
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
	if(!rpn_mode && chain_mode && !current_parsed_function() && evalops.parse_options.base != BASE_UNICODE && (evalops.parse_options.base != BASE_CUSTOM || (CALCULATOR->customInputBase() <= 62 && CALCULATOR->customInputBase() >= -62))) {
		GtkTextIter iend, istart;
		gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iend, gtk_text_buffer_get_insert(expression_edit_buffer()));
		if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
			GtkTextMark *mstart = gtk_text_buffer_get_selection_bound(expression_edit_buffer());
			if(mstart) {
				gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &istart, mstart);
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
			gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
			gtk_text_buffer_insert(expression_edit_buffer(), &istart, "(", -1);
			gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
			gtk_text_buffer_insert(expression_edit_buffer(), &iend, ")", -1);
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iend);
			unblock_undo();
		} else if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
			gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iend);
		}
		insert_text(op);
		rpn_mode = false;
		return true;
	}
	return false;
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

void update_tooltips_enabled() {
	set_tooltips_enabled(GTK_WIDGET(main_window()), enable_tooltips);
	set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), enable_tooltips == 1);
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


void ViewThread::run() {

	while(true) {
		void *x = NULL;
		if(!read(&x) || !x) break;
		MathStructure m(*((MathStructure*) x));
		bool b_stack = false;
		if(!read(&b_stack)) break;
		if(!read(&x)) break;
		MathStructure *mm = (MathStructure*) x;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		printops.can_display_unicode_string_arg = (void*) history_view_widget();
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
			po.can_display_unicode_string_arg = (void*) parse_status_widget();
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

		if(!b_stack && (m.isAborted() || unhtmlize(result_text).length() > 900)) {
			*printops.is_approximate = false;
			draw_result_failure(m, !m.isAborted());
		} else if(!b_stack) {
			draw_result_temp(m);
		}
		result_autocalculated = false;
		printops.use_unit_prefixes = b_puup;
		b_busy = false;
		CALCULATOR->stopControl();
	}
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
		draw_result_pre();
	}

	printops.prefix = prefix;

#define SET_RESULT_RETURN {b_busy = false; b_busy_result = false; draw_result_abort(); unblock_error(); return;}

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
		if(stack_index == 0) draw_result_waiting();
		g_application_mark_busy(g_application_get_default());
		update_expression_icons(stack_index == 0 ? (!minimal_mode ? RESULT_SPINNER : EXPRESSION_SPINNER) : EXPRESSION_STOP);
		if(!minimal_mode) start_result_spinner();
		else start_expression_spinner();
		if(update_window_title(_("Processing…"))) title_set = true;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(keypad_widget(), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(history_view_widget()), FALSE);
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
		draw_result_check();
	}

	if(was_busy) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(keypad_widget(), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(history_view_widget()), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(!update_parse && stack_index == 0) hide_expression_spinner();
		if(title_set && stack_index != 0) update_window_title();
		if(!minimal_mode) stop_result_spinner();
		else stop_expression_spinner();
		g_application_unmark_busy(g_application_get_default());
	}

	if(stack_index == 0) {
		if(visible_keypad & PROGRAMMING_KEYPAD) update_result_bases();
		if(draw_result_finalize()) minimal_mode_show_resultview();
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
		error_icon = display_errors(supress_dialog ? NULL : main_window(), supress_dialog ? 2 : 0);
	} else {
		bool b_approx = result_text_approximate || mstruct->isApproximate();
		string error_str;
		int mtype_highest = MESSAGE_INFORMATION;
		add_result_to_history(update_history, update_parse, register_moved, b_rpn_operation, result_text, b_approx, parsed_text, parsed_approx, transformation, supress_dialog ? NULL : main_window(), &error_str, &mtype_highest, &implicit_warning);
		error_icon = expression_display_errors(supress_dialog ? NULL : main_window(), 1, true, error_str, mtype_highest);
		if(update_history && result_text.length() < 1000) {
			string str;
			if(!b_approx) {
				str = "=";
			} else {
				if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) main_window())) {
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
			gtk_widget_set_tooltip_text(result_view_widget(), enable_tooltips && str.length() < 1000 ? str.c_str() : "");
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

	if(!register_moved && stack_index == 0 && mstruct->isMatrix() && matrix_mstruct->isMatrix() && matrix_mstruct->columns() < 200 && result_did_not_fit(false)) {
		focus_expression();
		if(update_history && update_parse && force) {
			expression_select_all();
		}
		if(!supress_dialog) insert_matrix(matrix_mstruct, main_window(), false, true, true);
	}

	if(!error_icon && (update_parse || stack_index != 0)) update_expression_icons(rpn_mode ? 0 : EXPRESSION_CLEAR);

	if(implicit_warning) ask_implicit();

	unblock_error();

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
				if(conversionview_continuous_conversion() && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
					ceu_str = current_conversion_expression();
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
		} else if(!current_displayed_result() && !force) {
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

	draw_result_backup();

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
		gtk_widget_set_sensitive(keypad_widget(), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(history_view_widget()), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		update_expression_icons(!minimal_mode ? RESULT_SPINNER : EXPRESSION_SPINNER);
		if(!minimal_mode) {
			draw_result_clear();
		}
		if(!minimal_mode) start_result_spinner();
		else start_expression_spinner();
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
		gtk_widget_set_sensitive(keypad_widget(), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(history_view_widget()), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(title_set) update_window_title();
		hide_expression_spinner();
		if(!minimal_mode) stop_result_spinner();
		else stop_expression_spinner();
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
		}
	}

	draw_result_restore();

	unblock_error();

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

void result_format_updated() {
	if(result_blocked()) return;
	update_message_print_options();
	if(result_autocalculated) print_auto_calc();
	else setResult(NULL, true, false, false);
	update_status_text();
	set_expression_output_updated(true);
	display_parse_status();
}

bool contains_prefix2(const MathStructure &m) {
	if(m.isUnit() && (m.prefix() || m.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_prefix2(m[i])) return true;
	}
	return false;
}
void result_prefix_changed(Prefix *prefix) {
	if((!expression_modified() || rpn_mode) && !current_displayed_result()) {
		return;
	}
	to_prefix = 0;
	bool b_use_unit_prefixes = printops.use_unit_prefixes;
	bool b_use_prefixes_for_all_units = printops.use_prefixes_for_all_units;
	if(contains_prefix2(*mstruct)) {
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

bool calculator_busy() {
	return b_busy;
}
void set_busy(bool b) {
	b_busy = b;
}

void abort_calculation() {
	if(b_busy_expression || b_busy_result || b_busy_command) CALCULATOR->abort();
	if(b_busy_command) {
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
			set_input_base(v, false, false);
		} else {
			set_output_base(v);
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
	} else if(equalsIgnoreCase(svar, "two's complement") || svar == "twos") {
		int v = s2b(svalue);
		if(v >= 0) set_twos_complement(v);
	} else if(equalsIgnoreCase(svar, "hexadecimal two's") || svar == "hextwos"){
		int v = s2b(svalue);
		if(v >= 0) set_twos_complement(-1, v);
	} else if(equalsIgnoreCase(svar, "two's complement input") || svar == "twosin") {
		int v = s2b(svalue);
		if(v >= 0) set_twos_complement(-1, -1, v);
	} else if(equalsIgnoreCase(svar, "hexadecimal two's input") || svar == "hextwosin") {
		int v = s2b(svalue);
		if(v >= 0) set_twos_complement(-1, -1, -1, v);
	} else if(equalsIgnoreCase(svar, "binary bits") || svar == "bits") {
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
			set_binary_bits(v);
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
			set_angle_unit((AngleUnit) v);
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
		set_exchange_rates_frequency(v);
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
		else if(!display && (empty_value || svalue == "sci" || equalsIgnoreCase(svalue, "scientific"))) v = EXP_SCIENTIFIC;
		else if(!display && (svalue == "eng" || equalsIgnoreCase(svalue, "engineering"))) v = EXP_BASE_3;
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
			set_min_exp(v, false);
		} else {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		}
	} else if(equalsIgnoreCase(svar, "precision") || svar == "prec") {
		int v = 0;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			set_precision(v);
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
					if(!display_errors(main_window(), 3, true)) update_expression_icons(EXPRESSION_CLEAR);
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
				show_help("index.html", main_window());
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
						manage_units(main_window(), str2.c_str(), true);
					} else if(list_type == 'f') {
						manage_functions(main_window(), str2.c_str());
					} else if(list_type == 'v') {
						manage_variables(main_window(), str2.c_str());
					} else if(list_type == 'u') {
						manage_units(main_window(), str2.c_str());
					} else if(list_type == 'p') {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					}
				}
				if(list_type == 0) {
					ExpressionItem *item = CALCULATOR->getActiveExpressionItem(str);
					if(item) {
						if(item->type() == TYPE_UNIT) {
							manage_units(main_window(), str.c_str());
						} else if(item->type() == TYPE_FUNCTION) {
							manage_functions(main_window(), str.c_str());
						} else if(item->type() == TYPE_VARIABLE) {
							manage_variables(main_window(), str.c_str());
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
				qalculate_quit();
				return;
			} else {
				CALCULATOR->error(true, "Unknown command: %s.", str.c_str(), NULL);
			}
			expression_select_all();
			set_history_activated();
			if(!display_errors(main_window(), 3, true)) update_expression_icons(EXPRESSION_CLEAR);
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
					convert_number_bases(main_window(), current_result());
					return;
				}
				do_bases = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, _("calendars"))) {
				if(from_str.empty()) {
					b_busy = false;
					b_busy_expression = false;
					restore_previous_expression();
					show_calendarconversion_dialog(main_window(), mstruct && mstruct->isDateTime() ? mstruct->datetime() : NULL);
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

	if(do_ceu && str_conv.empty() && conversionview_continuous_conversion() && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
		string ceu_str = current_conversion_expression();
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
		if(stack_index == 0) draw_result_destroy();
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), FALSE);
		gtk_widget_set_sensitive(keypad_widget(), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(history_view_widget()), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), FALSE);
		update_expression_icons(stack_index == 0 ? (!minimal_mode ? RESULT_SPINNER : EXPRESSION_SPINNER) : EXPRESSION_STOP);
		if(!minimal_mode) start_result_spinner();
		else start_expression_spinner();
		g_application_mark_busy(g_application_get_default());
		was_busy = true;
	}
	while(CALCULATOR->busy()) {
		while(gtk_events_pending()) gtk_main_iteration();
		sleep_ms(100);
	}

	if(was_busy) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), TRUE);
		gtk_widget_set_sensitive(keypad_widget(), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(history_view_widget()), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "historyactions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "rpntab")), TRUE);
		if(title_set) update_window_title();
		if(!minimal_mode) stop_result_spinner();
		else stop_expression_spinner();
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

	if(do_bases) convert_number_bases(main_window(), current_result());
	if(do_calendars) show_calendarconversion_dialog(main_window(), mstruct && mstruct->isDateTime() ? mstruct->datetime() : NULL);
	
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
	gtk_widget_hide(result_view_widget());
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
	gtk_widget_show(result_view_widget());
	set_expression_modified(true, false, false);
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

void show_parsed(bool b) {
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		do_autocalc_history_timeout(NULL);
	}
	display_parsed_instead_of_result(b);
}
void set_parsed_in_result(bool b) {
	if(b == parsed_in_result) return;
	if(b) {
		parsed_in_result = true;
	} else {
		parsed_in_result = false;
		clear_parsed_in_result();
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
		if(auto_calculate && result_autocalculated) result_text = "";
		clearresult();
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
		prev_autocalc_str = "";
		if(auto_calculate) {
			result_autocalculated = false;
			do_auto_calc(2);
		}
	}
	update_menu_calculator_mode();
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

/*
	save definitions to ~/.conf/qalculate/qalculate.cfg
	the hard work is done in the Calculator class
*/
bool save_defs(bool allow_cancel) {
	if(!CALCULATOR->saveDefinitions()) {
		GtkWidget *edialog = gtk_message_dialog_new(main_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, _("Couldn't write definitions"));
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

void load_mode(const mode_struct *mode) {
	block_result();
	block_calculation();
	block_status();
	if(mode->keypad == 1) {
		programming_inbase = 0;
		programming_outbase = 0;
	}
	CALCULATOR->setCustomOutputBase(mode->custom_output_base);
	CALCULATOR->setCustomInputBase(mode->custom_input_base);
	CALCULATOR->setConciseUncertaintyInputEnabled(mode->concise_uncertainty_input);
	CALCULATOR->setFixedDenominator(mode->fixed_denominator);
	update_window_title();
	visible_keypad = mode->keypad;
	int dff = default_fraction_fraction;
	set_mode_items(mode, false);
	default_fraction_fraction = dff;
	update_keypad_state();
	previous_precision = 0;
	update_setbase();
	update_decimals();
	update_precision();
	implicit_question_asked = mode->implicit_question_asked;
	evalops.approximation = mode->eo.approximation;
	unblock_result();
	unblock_calculation();
	unblock_status();
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	update_message_print_options();
	update_status_text();
	auto_calculate = mode->autocalc;
	chain_mode = mode->chain_mode;
	complex_angle_form = mode->complex_angle_form;
	set_rpn_mode(mode->rpn_mode);
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

void update_minimal_width() {
	gint w;
	gtk_window_get_size(main_window(), &w, NULL);
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
		gtk_window_get_size(main_window(), &w, NULL);
		win_width = w;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")));
		if(expression_is_empty() || !current_displayed_result()) {
			clearresult();
		}
		gtk_window_resize(main_window(), minimal_width > 0 ? minimal_width : win_width, 1);
		gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), TRUE);
		gtk_widget_set_vexpand(result_view_widget(), FALSE);
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
		gtk_widget_set_vexpand(result_view_widget(), !gtk_widget_get_visible(keypad_widget()) && !gtk_widget_get_visible(tabs));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		set_status_bottom_border_visible(true);
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		if(history_height > 0 && (gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) || gtk_expander_get_expanded(GTK_EXPANDER(expander_stack)))) {
			gdk_threads_add_timeout(500, do_minimal_mode_timeout, NULL);
		}
		gint h = 1;
		if(gtk_widget_is_visible(tabs) || gtk_widget_is_visible(keypad_widget())) {
			gtk_window_get_size(main_window(), NULL, &h);
		}
		gtk_window_resize(main_window(), win_width < 0 ? 1 : win_width, h);
	}
	set_expression_size_request();
}

/*
	load preferences from ~/.conf/qalculate/qalculate-gtk.cfg
*/
void load_preferences() {

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

	save_initial_modes();

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
	minimal_width = 500;
	history_height = 0;
	save_history_separately = false;
	save_mode_on_exit = true;
	save_defs_on_exit = true;
	hyp_is_on = false;
	inv_is_on = false;
	use_custom_app_font = false;
	custom_app_font = "";
	text_color = "#FFFFFF";
	text_color_set = false;
	show_keypad = true;
	show_history = false;
	show_stack = true;
	show_convert = false;
	persistent_keypad = false;
	minimal_mode = false;
	load_global_defs = true;
	display_expression_status = true;
	parsed_in_result = false;
	first_time = false;
	first_error = true;
	gtk_theme = -1;
	custom_lang = "";

	CALCULATOR->setPrecision(10);
	previous_precision = 0;

	default_shortcuts = true;
	keyboard_shortcuts.clear();

	last_version_check_date.setToCurrentDate();

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

	version_numbers[0] = 5;
	version_numbers[1] = 2;
	version_numbers[2] = 0;

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
					} else if(svar == "error_info_shown") {
						first_error = !v;
					} else if(svar == "save_mode_on_exit") {
						save_mode_on_exit = v;
					} else if(svar == "save_definitions_on_exit") {
						save_defs_on_exit = v;
					} else if(svar == "save_history_separately") {
						save_history_separately = v;
					} else if(svar == "language") {
						custom_lang = svalue;
					} else if(svar == "ignore_locale") {
						ignore_locale = v;
					} else if(svar == "window_title_mode") {
						if(v >= 0 && v <= 4) title_type = v;
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
					} else if(svar == "display_expression_status") {
						display_expression_status = v;
					} else if(svar == "parsed_expression_in_resultview") {
						parsed_in_result = v;
					} else if(svar == "calculate_as_you_type_history_delay") {
						autocalc_history_delay = v;
					} else if(svar == "programming_bits" || svar == "binary_bits") {
						evalops.parse_options.binary_bits = v;
						printops.binary_bits = v;
					} else if(svar == "previous_precision") {
						previous_precision = v;
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
					} else if(svar == "local_currency_conversion") {
						evalops.local_currency_conversion = v;
					} else if(svar == "use_binary_prefixes") {
						CALCULATOR->useBinaryPrefixes(v);
					} else if(svar == "digit_grouping") {
						if(v >= DIGIT_GROUPING_NONE && v <= DIGIT_GROUPING_LOCALE) {
							printops.digit_grouping = (DigitGrouping) v;
						}
					} else if(svar == "rpn_keys") {
						rpn_keys = v;
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
					} else if(svar == "use_custom_application_font") {
						use_custom_app_font = v;
					} else if(svar == "custom_application_font") {
						custom_app_font = svalue;
						save_custom_app_font = true;
					} else if(svar == "text_color") {
						text_color = svalue;
						text_color_set = true;
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
					} else if(read_mode_line(mode_index, svar, svalue, v)) {
					} else if(read_exchange_rates_settings_line(svar, svalue, v)) {
					} else if(read_menubar_settings_line(svar, svalue, v)) {
					} else if(read_keypad_settings_line(svar, svalue, v)) {
					} else if(read_expression_edit_settings_line(svar, svalue, v)) {
					} else if(read_expression_status_settings_line(svar, svalue, v)) {
					} else if(read_history_settings_line(svar, svalue, v)) {
					} else if(read_datasets_dialog_settings_line(svar, svalue, v)) {
					} else if(read_conversion_view_settings_line(svar, svalue, v)) {
					} else if(read_result_view_settings_line(svar, svalue, v)) {
					} else if(read_number_bases_dialog_settings_line(svar, svalue, v)) {
					} else if(read_insert_function_dialog_settings_line(svar, svalue, v)) {
					} else if(read_plot_settings_line(svar, svalue, v)) {
					} else if(read_functions_dialog_settings_line(svar, svalue, v)) {
					} else if(read_units_dialog_settings_line(svar, svalue, v)) {
					} else if(read_variables_dialog_settings_line(svar, svalue, v)) {
					} else if(read_datasets_dialog_settings_line(svar, svalue, v)) {
					} else if(read_help_settings_line(svar, svalue, v)) {
					} else if(read_expression_history_line(svar, svalue)) {
					} else if(read_history_line(svar, svalue)) {
					}
				} else if(stmp.length() > 2 && stmp[0] == '[' && stmp[stmp.length() - 1] == ']') {
					stmp = stmp.substr(1, stmp.length() - 2);
					remove_blank_ends(stmp);
					if(stmp == "Mode") {
						mode_index = 1;
					} else if(stmp.length() > 5 && stmp.substr(0, 4) == "Mode") {
						mode_index = initialize_mode_as(stmp.substr(5, stmp.length() - 5));
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
	update_displayed_printops();
	g_free(gstr_file);
	show_history = show_history && (persistent_keypad || !show_keypad);
	show_convert = show_convert && !show_history && (persistent_keypad || !show_keypad);
	save_default_mode(custom_angle_unit.c_str());

}
void definitions_loaded() {
	remove_old_my_variables_category();

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
}
void add_recent_items() {
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
}

bool save_history(bool allow_cancel) {
	if(!save_history_separately) return true;
	FILE *file = NULL;
	string homedir = getLocalDir();
	recursiveMakeDir(homedir);
	gchar *gstr2 = g_build_filename(homedir.c_str(), "qalculate-gtk.history", NULL);
	file = fopen(gstr2, "w+");
	if(file == NULL) {
		GtkWidget *edialog = gtk_message_dialog_new(main_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, _("Couldn't write history to\n%s"), gstr2);
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

	write_expression_history(file);
	write_history(file);

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
		GtkWidget *edialog = gtk_message_dialog_new(main_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, _("Couldn't write preferences to\n%s"), gstr2);
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
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", "5.0.1");
	fprintf(file, "allow_multiple_instances=%i\n", allow_multiple_instances);
	if(title_type != TITLE_APP) fprintf(file, "window_title_mode=%i\n", title_type);
	if(minimal_width > 0 && minimal_mode) {
		fprintf(file, "width=%i\n", win_width);
	} else {
		gtk_window_get_size(main_window(), &w, &h);
		fprintf(file, "width=%i\n", w);
	}
	//fprintf(file, "height=%i\n", h);
	if(remember_position) {
		if(hidden_x >= 0 && !gtk_widget_is_visible(GTK_WIDGET(main_window()))) {
			fprintf(file, "monitor=%i\n", hidden_monitor);
			fprintf(file, "monitor_primary=%i\n", hidden_monitor_primary);
			fprintf(file, "x=%i\n", hidden_x);
			fprintf(file, "y=%i\n", hidden_y);
		} else {
			gtk_window_get_position(main_window(), &win_x, &win_y);
			GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(main_window()));
			win_monitor = 1;
			win_monitor_primary = false;
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
			GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(GTK_WIDGET(main_window())));
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
			GdkScreen *screen = gtk_window_get_screen(main_window());
			if(screen) {
				int i = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(main_window())));
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
	fprintf(file, "error_info_shown=%i\n", !first_error);
	fprintf(file, "save_mode_on_exit=%i\n", save_mode_on_exit);
	fprintf(file, "save_definitions_on_exit=%i\n", save_defs_on_exit);
	fprintf(file, "save_history_separately=%i\n", save_history_separately);
	write_exchange_rates_settings(file);
	write_history_settings(file);
	write_expression_edit_settings(file);
	write_expression_status_settings(file);
	write_keypad_settings(file);
	write_menubar_settings(file);
	write_result_view_settings(file);
	write_conversion_view_settings(file);
	write_datasets_dialog_settings(file);
	write_number_bases_dialog_settings(file);
	write_insert_function_dialog_settings(file);
	write_datasets_dialog_settings(file);
	write_functions_dialog_settings(file);
	write_units_dialog_settings(file);
	write_variables_dialog_settings(file);
	write_help_settings(file);
	if(!custom_lang.empty()) fprintf(file, "language=%s\n", custom_lang.c_str());
	fprintf(file, "ignore_locale=%i\n", ignore_locale);
	fprintf(file, "load_global_definitions=%i\n", load_global_defs);
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
	fprintf(file, "rpn_keys=%i\n", rpn_keys);
	fprintf(file, "display_expression_status=%i\n", display_expression_status);
	fprintf(file, "parsed_expression_in_resultview=%i\n", parsed_in_result);
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
	if(evalops.parse_options.binary_bits > 0) fprintf(file, "binary_bits=%i\n", evalops.parse_options.binary_bits);
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
	fprintf(file, "use_custom_application_font=%i\n", use_custom_app_font);
	if(use_custom_app_font || save_custom_app_font) fprintf(file, "custom_application_font=%s\n", custom_app_font.c_str());
	if(text_color_set) fprintf(file, "text_color=%s\n", text_color.c_str());
	fprintf(file, "multiplication_sign=%i\n", printops.multiplication_sign);
	fprintf(file, "division_sign=%i\n", printops.division_sign);
	if(automatic_fraction) fprintf(file, "automatic_number_fraction_format=%i\n", automatic_fraction);
	if(default_fraction_fraction >= 0) fprintf(file, "default_number_fraction_fraction=%i\n", default_fraction_fraction);
	if(auto_prefix > 0) fprintf(file, "automatic_unit_prefixes=%i\n", auto_prefix);
	if(!scientific_noprefix) fprintf(file, "scientific_mode_unit_prefixes=%i\n", true);
	if(!scientific_notminuslast) fprintf(file, "scientific_mode_sort_minus_last=%i\n", true);
	if(!scientific_negexp) fprintf(file, "scientific_mode_negative_exponents=%i\n", false);
	if(tc_set) fprintf(file, "temperature_calculation=%i\n", CALCULATOR->getTemperatureCalculationMode());
	if(sinc_set) fprintf(file, "sinc_function=%i\n", CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->getDefaultValue(2) == "pi" ? 1 : 0);
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
	if(!save_history_separately) {
		write_expression_history(file);
		write_history(file);
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
	if(mode) save_default_mode();
	for(size_t i = 1 ;; i++) {
		mode_struct *mode = get_mode(i);
		if(!mode) break;
		if(i == 1) {
			fprintf(file, "\n[Mode]\n");
		} else {
			fprintf(file, "\n[Mode %s]\n", mode->name.c_str());
		}
		write_mode(file, i);
	}
	fprintf(file, "\n[Plotting]\n");
	write_plot_settings(file);

	fclose(file);

	return true;

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

void set_autocalculate(bool b) {
	if(auto_calculate == b) return;
	auto_calculate = b;
	if(auto_calculate && !rpn_mode) {
		clear_parsed_expression();
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
	update_menu_calculator_mode();
}
void update_exchange_rates() {
	stop_autocalculate_history_timeout();
	block_error();
	fetch_exchange_rates(15);
	CALCULATOR->loadExchangeRates();
	display_errors(main_window());
	unblock_error();
	while(gtk_events_pending()) gtk_main_iteration();
	expression_calculation_updated();
}

void import_definitions_file() {
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select definitions file"), main_window(), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Import"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select definitions file"), main_window(), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Import"), GTK_RESPONSE_ACCEPT, NULL);
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
			GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not copy %s to %s."), from_file, buildPath(homedir, str).c_str());
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}
#else
		std::ifstream source(from_file);
		if(source.fail()) {
			source.close();
			GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not read %s."), from_file);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		} else {
			std::ofstream dest(buildPath(homedir, str).c_str());
			if(dest.fail()) {
				source.close();
				dest.close();
				GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not copy file to %s."), homedir.c_str());
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
	if(base == BASE_CUSTOM && open_dialog) {
		open_setbase(main_window(), true, true);
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

void open_plot() {
	string str, str2;
	if(evalops.parse_options.base == 10) {
		str = get_selected_expression_text();
		CALCULATOR->separateToExpression(str, str2, evalops, true);
		remove_blank_ends(str);
	}
	show_plot_dialog(GTK_WINDOW(main_window()), str.c_str());
}

void restore_automatic_fraction() {
	if(automatic_fraction && printops.number_fraction_format == FRACTION_DECIMAL_EXACT) {
		if(!rpn_mode) block_result();
		set_fraction_format(FRACTION_DECIMAL);
		automatic_fraction = false;
		if(!rpn_mode) unblock_result();
	}
}
void set_precision(int v, int recalc) {
	CALCULATOR->setPrecision(v);
	if(recalc > 0) execute_expression(true, false, OPERATION_ADD, NULL, rpn_mode);
	else if(recalc < 0) {update_precision(); expression_calculation_updated();}
	previous_precision = 0;
}
void set_approximation(ApproximationMode approx) {
	evalops.approximation = approx;
	update_keypad_exact();
	if(approx == APPROXIMATION_EXACT) {
		if(printops.number_fraction_format == FRACTION_DECIMAL) {
			if(!rpn_mode) block_result();
			set_fraction_format(FRACTION_DECIMAL_EXACT);
			automatic_fraction = true;
			if(!rpn_mode) unblock_result();
		}
	} else {
		restore_automatic_fraction();
	}
	update_status_approximation();
	update_menu_approximation();

	expression_calculation_updated();
}
void set_min_exp(int min_exp, bool extended) {
	if(extended) {
		block_result();
		if(default_fraction_fraction < 0) {
			if(min_exp == EXP_PRECISION || min_exp == EXP_NONE) {
				if(printops.number_fraction_format == FRACTION_FRACTIONAL) {
					set_fraction_format(FRACTION_COMBINED);
				}
			} else {
				if(printops.number_fraction_format == FRACTION_COMBINED) {
					set_fraction_format(FRACTION_FRACTIONAL);
				}
			}
			default_fraction_fraction = -1;
		}
		bool sne_bak = scientific_negexp, snml_bak = scientific_notminuslast, snp_bak = scientific_noprefix;
		if(min_exp == EXP_PRECISION || min_exp == EXP_NONE) {
			printops.negative_exponents = false;
			printops.sort_options.minus_last = true;
			int ap_bak = auto_prefix;
			if(auto_prefix == 1) set_prefix_mode(PREFIX_MODE_SELECTED_UNITS);
			else if(auto_prefix == 2) set_prefix_mode(PREFIX_MODE_CURRENCIES);
			else if(auto_prefix == 3) set_prefix_mode(PREFIX_MODE_ALL_UNITS);
			auto_prefix = ap_bak;
		} else {
			if(min_exp != EXP_BASE_3) {
				if(scientific_negexp) printops.negative_exponents = true;
				if(scientific_notminuslast) printops.sort_options.minus_last = false;
			}
			if(printops.use_unit_prefixes && scientific_noprefix) {
				if(printops.use_prefixes_for_all_units) auto_prefix = 3;
				else if(printops.use_prefixes_for_currencies) auto_prefix = 2;
				else auto_prefix = 1;
				int ap_bak = auto_prefix;
				set_prefix_mode(PREFIX_MODE_NO_PREFIXES);
				auto_prefix = ap_bak;
			}
		}
		scientific_negexp = sne_bak; scientific_notminuslast = snml_bak; scientific_noprefix = snp_bak;
		unblock_result();
	}
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
void set_twos_complement(int bo, int ho, int bi, int hi, bool recalculate) {
	if(bo >= 0) {
		if(printops.twos_complement == bo) bo = -1;
		else printops.twos_complement = bo;
	}
	if(ho >= 0) {
		if(printops.hexadecimal_twos_complement == ho) ho = -1;
		else printops.hexadecimal_twos_complement = ho;
	}
	if(bi >= 0) {
		if(evalops.parse_options.twos_complement == bi) bi = -1;
		else evalops.parse_options.twos_complement = bi;
	}
	if(hi >= 0) {
		if(evalops.parse_options.hexadecimal_twos_complement == hi) hi = -1;
		else evalops.parse_options.hexadecimal_twos_complement = hi;
	}
	if(bi >= 0 || hi >= 0) expression_format_updated(recalculate);
	else if(bo >= 0 || ho >= 0) result_format_updated();
	update_keypad_programming_base();
	preferences_update_twos_complement();
	insert_function_twos_complement_changed(bo >= 0, ho >= 0, bi >= 0, hi >= 0);
}
void set_binary_bits(unsigned int i, bool recalculate) {
	if(i == printops.binary_bits && i == evalops.parse_options.binary_bits) return;
	printops.binary_bits = i;
	evalops.parse_options.binary_bits = printops.binary_bits;
	if(evalops.parse_options.twos_complement || evalops.parse_options.hexadecimal_twos_complement) expression_format_updated(recalculate);
	else result_format_updated();
	update_keypad_programming_base();
	preferences_update_twos_complement();
	insert_function_binary_bits_changed();
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
void toggle_fraction_format(bool b) {
	if(b) {
		if(default_fraction_fraction >= 0) {
			if(default_fraction_fraction == FRACTION_FRACTIONAL) set_fraction_format(FRACTION_FRACTIONAL);
			else set_fraction_format(FRACTION_COMBINED);
		} else {
			if(printops.min_exp != EXP_NONE && printops.min_exp != EXP_PRECISION) set_fraction_format(FRACTION_FRACTIONAL);
			else set_fraction_format(FRACTION_COMBINED);
			default_fraction_fraction = -1;
		}

	} else {
		if(evalops.approximation == APPROXIMATION_EXACT) {
			set_fraction_format(FRACTION_DECIMAL_EXACT);
			automatic_fraction = true;
		} else {
			set_fraction_format(FRACTION_DECIMAL);
		}
	}
}
void set_fixed_fraction(long int v, bool combined) {
	CALCULATOR->setFixedDenominator(v);
	if(combined) set_fraction_format(FRACTION_COMBINED_FIXED_DENOMINATOR);
	else set_fraction_format(FRACTION_FRACTIONAL_FIXED_DENOMINATOR);
}
void set_angle_unit(AngleUnit au) {
	evalops.parse_options.angle_unit = au;
	expression_format_updated(true);
	update_keypad_angle();
	update_status_angle();
	update_menu_angle();
}
void set_custom_angle_unit(Unit *u) {
	evalops.parse_options.angle_unit = ANGLE_UNIT_CUSTOM;
	CALCULATOR->setCustomAngleUnit(u);
	expression_format_updated(true);
	update_keypad_angle();
	update_status_angle();
	update_menu_angle();
}
#ifdef _WIN32
gboolean on_about_activate_link(GtkAboutDialog*, gchar *uri, gpointer) {
	ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
	return TRUE;
}
#else
gboolean on_about_activate_link(GtkAboutDialog*, gchar*, gpointer) {
	return FALSE;
}
#endif

void show_about() {
	const gchar *authors[] = {"Hanna Knutsson <hanna.knutsson@protonmail.com>", NULL};
	GtkWidget *dialog = gtk_about_dialog_new();
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("translator-credits"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Powerful and easy to use calculator"));
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2003–2007, 2008, 2016–2024 Hanna Knutsson");
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), "qalculate");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Qalculate! (GTK)");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://qalculate.github.io/");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), main_window());
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
	gtk_show_uri_on_window(main_window(), "https://github.com/Qalculate/qalculate-gtk/issues", gtk_get_current_event_time(), &error);
#	else
	gtk_show_uri(NULL, "https://github.com/Qalculate/qalculate-gtk/issues", gtk_get_current_event_time(), &error);
#	endif
	if(error) {
		gchar *error_str = g_locale_to_utf8(error->message, -1, NULL, NULL, NULL);
		GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Failed to open %s.\n%s"), "https://github.com/Qalculate/qalculate-gtk/issues", error_str);
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
			insert_function(CALCULATOR->getActiveFunction(value), main_window());
			return true;
		}
		case SHORTCUT_TYPE_VARIABLE: {
			insert_variable(CALCULATOR->getActiveVariable(value));
			return true;
		}
		case SHORTCUT_TYPE_UNIT: {
			Unit *u = CALCULATOR->getActiveUnit(value);
			if(!u) {
				CALCULATOR->clearMessages();
				CompositeUnit cu("", "", "", value);
				if(CALCULATOR->message()) {
					display_errors();
				} else {
					PrintOptions po = printops;
					po.is_approximate = NULL;
					po.can_display_unicode_string_arg = (void*) expression_edit_widget();
					string str = cu.print(po, false, TAG_TYPE_HTML, true);
					insert_text(str.c_str());
				}
			} else if(u && CALCULATOR->stillHasUnit(u)) {
				if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					PrintOptions po = printops;
					po.is_approximate = NULL;
					po.can_display_unicode_string_arg = (void*) expression_edit_widget();
					string str = ((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true);
					insert_text(str.c_str());
				} else {
					insert_text(u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_UNIT, true).c_str());
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
			if(!load_mode(value)) show_message(_("Mode not found."));
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
			set_approximation(evalops.approximation == APPROXIMATION_EXACT ? APPROXIMATION_TRY_EXACT : APPROXIMATION_EXACT);
			return true;
		}
		case SHORTCUT_TYPE_DEGREES: {
			set_angle_unit(ANGLE_UNIT_DEGREES);
			return true;
		}
		case SHORTCUT_TYPE_RADIANS: {
			set_angle_unit(ANGLE_UNIT_RADIANS);
			return true;
		}
		case SHORTCUT_TYPE_GRADIANS: {
			set_angle_unit(ANGLE_UNIT_GRADIANS);
			return true;
		}
		case SHORTCUT_TYPE_FRACTIONS: {
			if(printops.number_fraction_format >= FRACTION_FRACTIONAL) set_fraction_format(FRACTION_DECIMAL);
			else set_fraction_format(FRACTION_FRACTIONAL);
			return true;
		}
		case SHORTCUT_TYPE_MIXED_FRACTIONS: {
			if(printops.number_fraction_format == FRACTION_COMBINED) set_fraction_format(FRACTION_DECIMAL);
			else set_fraction_format(FRACTION_COMBINED);
			return true;
		}
		case SHORTCUT_TYPE_SCIENTIFIC_NOTATION: {
			if(printops.min_exp == EXP_SCIENTIFIC) set_min_exp(EXP_PRECISION, true);
			else set_min_exp(EXP_SCIENTIFIC, true);
			return true;
		}
		case SHORTCUT_TYPE_SIMPLE_NOTATION: {
			if(printops.min_exp == EXP_NONE) set_min_exp(EXP_PRECISION, true);
			else set_min_exp(EXP_NONE, true);
			return true;
		}
		case SHORTCUT_TYPE_RPN_MODE: {
			set_rpn_mode(!rpn_mode);
			return true;
		}
		case SHORTCUT_TYPE_AUTOCALC: {
			set_autocalculate(!auto_calculate);
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
			manage_variables(main_window());
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {
			manage_functions(main_window());
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_UNITS: {
			manage_units(main_window());
			return true;
		}
		case SHORTCUT_TYPE_MANAGE_DATA_SETS: {
			manage_datasets(main_window());
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
			edit_variable(NULL, NULL, NULL, main_window());
			return true;
		}
		case SHORTCUT_TYPE_NEW_FUNCTION: {
			edit_function("", NULL, main_window());
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
			show_periodic_table(main_window());
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
			show_help("index.html", main_window());
			return true;
		}
		case SHORTCUT_TYPE_QUIT: {
			qalculate_quit();
			return true;
		}
		case SHORTCUT_TYPE_CHAIN_MODE: {
			chain_mode = !chain_mode;
			update_menu_calculator_mode();
			return true;
		}
		case SHORTCUT_TYPE_ALWAYS_ON_TOP: {
			always_on_top = !always_on_top;
			aot_changed = true;
			gtk_window_set_keep_above(main_window(), always_on_top);
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
				set_precision(previous_precision);
			} else {
				int prec = CALCULATOR->getPrecision();
				set_precision(v);
				previous_precision = prec;
			}
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
void apply_function(MathFunction *f) {
	if(b_busy) return;
	if(rpn_mode) {
		calculateRPN(f);
		return;
	}
	string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_buffer()).formattedName(TYPE_FUNCTION, true);
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
	if(current_parsed_result() && current_parsed_result()->containsFunction(f)) expression_format_updated(false);
	update_fmenu();
	function_inserted(f);
}
void dataset_edited(DataSet *ds) {
	if(!ds) return;
	selected_dataset = ds;
	update_fmenu();
	if(current_parsed_result() && current_parsed_result()->containsFunction(ds)) expression_format_updated(false);
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
	if(current_parsed_result() && current_parsed_result()->contains(v)) expression_format_updated(false);
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
	if(current_parsed_result() && current_parsed_result()->contains(u)) expression_format_updated(false);
	update_umenus();
	unit_inserted(u);
}
void unit_inserted(Unit *object) {
	if(!object) return;
	add_recent_unit(object);
	update_mb_units_menu();
}
void insert_answer_variable(size_t index) {
	if(index > 5) return;
	insert_text(vans[index]->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_VARIABLE, true).c_str());
}
void insert_variable(Variable *v, bool add_to_recent) {
	if(!v || !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."));
		update_vmenu();
		return;
	}
	insert_text(v->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_VARIABLE, true).c_str());
	if(add_to_recent) variable_inserted(v);
}

void insert_unit(Unit *u, bool add_to_recent) {
	if(!u || !CALCULATOR->stillHasUnit(u)) return;
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		PrintOptions po = printops;
		po.is_approximate = NULL;
		po.can_display_unicode_string_arg = (void*) expression_edit_widget();
		insert_text(((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true).c_str());
	} else {
		insert_text(u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_UNIT, true).c_str());
	}
	if(add_to_recent) unit_inserted(u);
}
void variable_removed(Variable *v) {
	remove_from_recent_variables(v);
	if(current_parsed_result() && current_parsed_result()->contains(v)) expression_format_updated(false);
	update_vmenu();
}
void unit_removed(Unit *u) {
	remove_from_recent_units(u);
	if(current_parsed_result() && current_parsed_result()->contains(u)) expression_format_updated(false);
	update_umenus();
}
void function_removed(MathFunction *f) {
	remove_from_recent_functions(f);
	if(current_parsed_result() && current_parsed_result()->containsFunction(f)) expression_format_updated(false);
	update_fmenu();
}

void new_function(GtkMenuItem*, gpointer) {
	edit_function("", NULL, main_window());
}
void new_unit(GtkMenuItem*, gpointer) {
	edit_unit("", NULL, main_window());
}
void insert_matrix(const MathStructure *initial_value, GtkWindow *win, gboolean create_vector, bool is_text_struct, bool is_result, GtkEntry *entry) {
	if(!entry) expression_save_selection();
	string matrixstr = get_matrix(initial_value, win, create_vector, is_text_struct, is_result);
	if(matrixstr.empty()) return;
	if(entry) {
		gtk_entry_set_text(entry, matrixstr.c_str());
	} else {
		expression_restore_selection();
		insert_text(matrixstr.c_str());
	}
}
void add_as_variable() {
	edit_variable(CALCULATOR->temporaryCategory().c_str(), NULL, mstruct, main_window());
}
void new_unknown(GtkMenuItem*, gpointer) {
	edit_unknown(NULL, NULL, main_window());
}
void new_variable(GtkMenuItem*, gpointer) {
	edit_variable(NULL, NULL, NULL, main_window());
}
void new_matrix(GtkMenuItem*, gpointer) {
	edit_matrix(NULL, NULL, NULL, main_window(), FALSE);
}
void new_vector(GtkMenuItem*, gpointer) {
	edit_matrix(NULL, NULL, NULL, main_window(), TRUE);
}

void set_unknowns() {
	if(expression_modified() && !expression_is_empty() && !rpn_mode) execute_expression(true);
	MathStructure unknowns;
	mstruct->findAllUnknowns(unknowns);
	if(unknowns.size() == 0) {
		show_message(_("No unknowns in result."));
		return;
	}
	unknowns.setType(STRUCT_ADDITION);
	unknowns.sort();

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Set Unknowns"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_REJECT, _("_Apply"), GTK_RESPONSE_APPLY, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
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
	if(current_result() && !result_text_empty()) {
		if(current_result()->isNumber() && !current_result()->number().hasImaginaryPart() && !result_did_not_fit()) return convert_number_bases(main_window(), current_result());
		return convert_number_bases(main_window());
	}
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	convert_number_bases(main_window(), str.c_str(), evalops.parse_options.base);
}
void open_convert_floatingpoint() {
	if(current_result() && !result_text_empty()) {
		if(current_result()->isNumber() && !current_result()->number().hasImaginaryPart() && !result_did_not_fit()) return convert_floatingpoint(current_result(), main_window());
		return convert_floatingpoint("", 0, main_window());
	}
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	convert_floatingpoint(str.c_str(), evalops.parse_options.base, main_window());
}
void open_percentage_tool() {
	if(current_result() && !result_text_empty() && !result_did_not_fit() && current_displayed_printops().base == BASE_DECIMAL) return show_percentage_dialog(main_window(), get_result_text().c_str());
	else if(evalops.parse_options.base != 10) return show_percentage_dialog(main_window());
	string str = get_selected_expression_text(true), str2;
	CALCULATOR->separateToExpression(str, str2, evalops, true);
	remove_blank_ends(str);
	show_percentage_dialog(main_window(), str.c_str());
}
void open_calendarconversion() {
	show_calendarconversion_dialog(main_window(), current_result() && current_result()->isDateTime() ? current_result()->datetime() : NULL);
}
void show_unit_conversion() {
	gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
	focus_conversion_entry();
}

bool use_keypad_buttons_for_history() {
	return persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_history)) && gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget()))) > 0;
}
bool keypad_is_visible() {return gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad)) && !minimal_mode;}

void minimal_mode_show_resultview(bool b) {
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
		gtk_window_get_size(main_window(), &w, &h);
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		set_status_bottom_border_visible(false);
		h -= 1;
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		gtk_window_resize(main_window(), w, h);
	}
}

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

void mainwindow_cursor_moved() {
	if(autocalc_history_timeout_id) {
		g_source_remove(autocalc_history_timeout_id);
		autocalc_history_timeout_id = 0;
		if(autocalc_history_delay >= 0 && !parsed_in_result) autocalc_history_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, autocalc_history_delay, do_autocalc_history_timeout, NULL, NULL);
	}
	if(auto_calc_stopped_at_operator) do_auto_calc();
}

void update_accels(int type) {
	update_result_accels(type);
	update_history_accels(type);
	update_stack_accels(type);
	update_keypad_accels(type);
	update_menu_accels(type);
}

gboolean on_event(GtkWidget*, GdkEvent *e, gpointer) {
	if(e->type == GDK_EXPOSE || e->type == GDK_PROPERTY_NOTIFY || e->type == GDK_CONFIGURE || e->type == GDK_FOCUS_CHANGE || e->type == GDK_VISIBILITY_NOTIFY) {
		return FALSE;
	}
	return TRUE;
}
gboolean on_configure_event(GtkWidget*, GdkEventConfigure*, gpointer) {
	if(minimal_mode) {
		if(minimal_window_resized_timeout_id) g_source_remove(minimal_window_resized_timeout_id);
		minimal_window_resized_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000, minimal_window_resized_timeout, NULL, NULL);
	}
	return FALSE;
}

bool disable_history_arrow_keys = false;
gboolean on_key_release_event(GtkWidget*, GdkEventKey*, gpointer) {
	disable_history_arrow_keys = false;
	return FALSE;
}
bool block_input = false;
gboolean on_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(block_input && (event->keyval == GDK_KEY_q || event->keyval == GDK_KEY_Q) && !(event->state & GDK_CONTROL_MASK)) {block_input = false; return TRUE;}
	if(gtk_widget_has_focus(expression_edit_widget()) || editing_stack() || editing_history()) return FALSE;
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
	if(gtk_widget_has_focus(history_view_widget())) {
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
	if(gtk_widget_has_focus(history_view_widget()) && event->keyval == GDK_KEY_F2) return FALSE;
	if(event->keyval > GDK_KEY_Hyper_R || event->keyval < GDK_KEY_Shift_L) {
		GtkWidget *w = gtk_window_get_focus(main_window());
		if(w && gtk_bindings_activate_event(G_OBJECT(w), event)) return TRUE;
		if(gtk_bindings_activate_event(G_OBJECT(o), event)) return TRUE;
		focus_keeping_selection();
	}
	return FALSE;
}
void on_message_bar_response(GtkInfoBar*, gint response_id, gpointer) {
	if(response_id == GTK_RESPONSE_CLOSE) {
		gint w, h, dur;
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "message_label")), "");
		gtk_window_get_size(main_window(), &w, &h);
		h -= gtk_widget_get_allocated_height(GTK_WIDGET(gtk_builder_get_object(main_builder, "message_bar")));
		dur = gtk_revealer_get_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")));
		gtk_revealer_set_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), 0);
		gtk_revealer_set_reveal_child(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), FALSE);
		gtk_window_resize(main_window(), w, h);
		gtk_revealer_set_transition_duration(GTK_REVEALER(gtk_builder_get_object(main_builder, "message_revealer")), dur);
	}
}

gboolean on_main_window_close(GtkWidget *w, GdkEvent*, gpointer) {
	if(has_systray_icon()) {
		save_preferences(save_mode_on_exit);
		save_history();
		if(save_defs_on_exit) save_defs();
		gtk_window_get_position(GTK_WINDOW(w), &hidden_x, &hidden_y);
		hidden_monitor = 1;
		hidden_monitor_primary = false;
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(main_window()));
		int n = gdk_display_get_n_monitors(display);
		GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(GTK_WIDGET(main_window())));
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
		GdkScreen *screen = gtk_window_get_screen(main_window());
		if(screen) {
			int i = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(main_window())));
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
		qalculate_quit();
	}
	return TRUE;
}
void restore_window(GtkWindow *win) {
	if(!win) win = main_window();
	if(hidden_x >= 0) {
		gtk_widget_show(GTK_WIDGET(win));
		GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(win));
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
			gtk_window_get_size(win, &w, &h);
			if(hidden_x + w > area.width) hidden_x = area.width - w;
			if(hidden_y + h > area.height) hidden_y = area.height - h;
			gtk_window_move(win, hidden_x + area.x, hidden_y + area.y);
		} else {
			gtk_window_move(win, hidden_x, hidden_y);
		}
		hidden_x = -1;
	}
#ifdef _WIN32
	gtk_window_present_with_time(win, GDK_CURRENT_TIME);
#endif
	focus_expression();
	gtk_window_present_with_time(win, GDK_CURRENT_TIME);
}

gboolean on_main_window_button_press_event(GtkWidget*, GdkEventButton*, gpointer) {
	hide_completion();
	return FALSE;
}
gboolean on_gcalc_exit(GtkWidget*, GdkEvent*, gpointer) {
	return qalculate_quit();
}

void show_tabs(bool do_show) {
	if(do_show == gtk_widget_get_visible(tabs)) return;
	gint w, h;
	gtk_window_get_size(main_window(), &w, &h);
	if(!persistent_keypad && gtk_widget_get_visible(keypad_widget())) h -= gtk_widget_get_allocated_height(keypad_widget()) + 9;
	if(do_show) {
		gtk_widget_show(tabs);
		gint a_h = gtk_widget_get_allocated_height(tabs);
		if(a_h > 10) h += a_h + 9;
		else h += history_height + 9;
		if(!persistent_keypad) gtk_widget_hide(keypad_widget());
		gtk_window_resize(main_window(), w, h);
	} else {
		h -= gtk_widget_get_allocated_height(tabs) + 9;
		gtk_widget_hide(tabs);
		set_result_size_request();
		set_expression_size_request();
		gtk_window_resize(main_window(), w, h);
	}
	gtk_widget_set_vexpand(result_view_widget(), !gtk_widget_get_visible(tabs) && !gtk_widget_get_visible(keypad_widget()));
	gtk_widget_set_vexpand(keypad_widget(), !persistent_keypad || !gtk_widget_get_visible(tabs));
}
void show_keypad_widget(bool do_show) {
	if(do_show == gtk_widget_get_visible(keypad_widget())) return;
	gint w, h;
	gtk_window_get_size(main_window(), &w, &h);
	if(!persistent_keypad && gtk_widget_get_visible(tabs)) h -= gtk_widget_get_allocated_height(tabs) + 9;
	if(persistent_keypad && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert))) {
		if(do_show) h += 6;
		else h -= 6;
	}
	if(do_show) {
		gtk_widget_show(keypad_widget());
		gint a_h = gtk_widget_get_allocated_height(keypad_widget());
		if(a_h > 10) h += a_h + 9;
		else h += 9;
		if(!persistent_keypad) gtk_widget_hide(tabs);
		gtk_window_resize(main_window(), w, h);
	} else {
		h -= gtk_widget_get_allocated_height(keypad_widget()) + 9;
		gtk_widget_hide(keypad_widget());
		set_result_size_request();
		set_expression_size_request();
		gtk_window_resize(main_window(), w, h);
	}
	gtk_widget_set_vexpand(result_view_widget(), !gtk_widget_get_visible(tabs) && !gtk_widget_get_visible(keypad_widget()));
	gtk_widget_set_vexpand(keypad_widget(), !persistent_keypad || !gtk_widget_get_visible(tabs));
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
void update_persistent_keypad(bool showhide_buttons) {
	if(!persistent_keypad && gtk_widget_is_visible(tabs)) showhide_buttons = true;
	gtk_widget_set_vexpand(keypad_widget(), !persistent_keypad || !gtk_widget_get_visible(tabs));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl")), !persistent_keypad || (rpn_mode && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))));
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnr")), !persistent_keypad || (rpn_mode && gtk_expander_get_expanded(GTK_EXPANDER(expander_stack))));
	if(showhide_buttons && (persistent_keypad || gtk_widget_is_visible(tabs))) {
		show_keypad = false;
		g_signal_handlers_block_matched((gpointer) expander_keypad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_expander_keypad_expanded, NULL);
		gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), persistent_keypad);
		g_signal_handlers_unblock_matched((gpointer) expander_keypad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_expander_keypad_expanded, NULL);
		if(persistent_keypad) gtk_widget_show(keypad_widget());
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
	if(persistent_keypad) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(history_view_widget())));
}
void on_expander_history_expanded(GObject *o, GParamSpec*, gpointer) {
	if(gtk_expander_get_expanded(GTK_EXPANDER(o))) {
		bool history_was_realized = gtk_widget_get_realized(history_view_widget());
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
void on_expander_convert_activate(GtkExpander*, gpointer) {
	focus_conversion_entry();
}

void update_app_font(bool initial = false) {
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
			gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(main_window())), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
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
void set_app_font(const char *str) {
	if(!str) {
		use_custom_app_font = false;
	} else {
		use_custom_app_font = true;
		if(custom_app_font != str) {
			save_custom_app_font = true;
			custom_app_font = str;
		}
	}
	update_app_font(false);
}
const char *app_font(bool return_default) {
	if(!return_default && !use_custom_app_font) return NULL;
	return custom_app_font.c_str();
}
void keypad_font_modified() {
	update_keypad_button_text();
	update_stack_button_text();
	while(gtk_events_pending()) gtk_main_iteration();
	gint winh, winw;
	gtk_window_get_size(main_window(), &winw, &winh);
	if(minimal_mode) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
	}
	while(gtk_events_pending()) gtk_main_iteration();
	bool b_buttons = gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad));
	if(!b_buttons) gtk_widget_show(keypad_widget());
	while(gtk_events_pending()) gtk_main_iteration();
	for(size_t i = 0; i < 5 && (!b_buttons || minimal_mode); i++) {
		sleep_ms(10);
		while(gtk_events_pending()) gtk_main_iteration();
	}
	GtkRequisition req;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")), &req, NULL);
	gtk_window_resize(main_window(), req.width + 24, 1);
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
		gtk_window_get_size(main_window(), &win_width, NULL);
		if(!minimal_mode) winw = win_width;
		if(!b_buttons) gtk_widget_hide(keypad_widget());
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_window_resize(main_window(), winw, winh);
	}
}
bool update_window_title(const char *str, bool is_result) {
	if(title_modified || !main_builder) return false;
	switch(title_type) {
		case TITLE_MODE: {
			if(is_result) return false;
			if(str && !current_mode_name().empty()) gtk_window_set_title(main_window(), (current_mode_name() + string(": ") + str).c_str());
			else if(!current_mode_name().empty()) gtk_window_set_title(main_window(), current_mode_name().c_str());
			else if(str) gtk_window_set_title(main_window(), (string("Qalculate! ") + str).c_str());
			else gtk_window_set_title(main_window(), _("Qalculate!"));
			break;
		}
		case TITLE_APP_MODE: {
			if(is_result || (!current_mode_name().empty() && str)) return false;
			if(!current_mode_name().empty()) gtk_window_set_title(main_window(), (string("Qalculate! ") + current_mode_name()).c_str());
			else if(str) gtk_window_set_title(main_window(), (string("Qalculate! ") + str).c_str());
			else gtk_window_set_title(main_window(), _("Qalculate!"));
			break;
		}
		case TITLE_RESULT: {
			if(!str) return false;
			if(str) gtk_window_set_title(main_window(), str);
			break;
		}
		case TITLE_APP_RESULT: {
			if(str) gtk_window_set_title(main_window(), strlen(str) == 0 ? "Qalculate!" : (string("Qalculate! (") + string(str) + ")").c_str());
			break;
		}
		default: {
			if(is_result) return false;
			if(str) gtk_window_set_title(main_window(), (string("Qalculate! ") + str).c_str());
			else gtk_window_set_title(main_window(), "Qalculate!");
		}
	}
	return true;
}
void set_custom_window_title(const char *str) {
	if(str) {
		gtk_window_set_title(main_window(), str);
		title_modified = true;
	} else {
		update_window_title();
		title_modified = false;
	}
}

bool border_tested = false;

#ifdef _WIN32
#	include <gdk/gdkwin32.h>
#	define WIN_TRAY_ICON_ID 1000
#	define WIN_TRAY_ICON_MESSAGE WM_APP + WIN_TRAY_ICON_ID
static NOTIFYICONDATA nid;
static HWND hwnd = NULL;

INT_PTR CALLBACK tray_window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WIN_TRAY_ICON_MESSAGE && (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)) {
		restore_window();
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
		hwnd = CreateWindow(wname, "", 0, 0, 0, 0, 0, (HWND) gdk_win32_window_get_handle(gtk_widget_get_window(GTK_WIDGET(main_window()))), NULL, GetModuleHandle(NULL), 0);
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

void set_system_tray_icon_enabled(bool b) {
	use_systray_icon = b;
#ifdef _WIN32
	if(b) create_systray_icon();
	else destroy_systray_icon();
#endif
}
bool system_tray_icon_enabled() {return use_systray_icon;}

void test_border() {
#ifndef _WIN32
	if(border_tested) return;
	GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(main_window()));
	GdkRectangle rect;
	gdk_window_get_frame_extents(window, &rect);
	gint window_border = (rect.width - gtk_widget_get_allocated_width(GTK_WIDGET(main_window()))) / 2;
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

GdkRGBA c_gray;

void update_colors(bool initial) {

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	GdkRGBA bg_color;
	gtk_style_context_get_background_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &bg_color);
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
		gtk_style_context_get_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &c);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
		if(gdk_rgba_equal(&c, &bg_color)) {
			gtk_style_context_get_color(gtk_widget_get_style_context(parse_status_widget()), GTK_STATE_FLAG_NORMAL, &c);
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

		update_status_colors(initial);

	}

}

GtkWindow *main_window() {
	if(!mainwindow) mainwindow = GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window"));
	return mainwindow;
}

void initialize_variables_and_functions() {
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
	v_memory = new KnownVariable(CALCULATOR->temporaryCategory(), "", m_zero, _("Memory"), false, true);
	ExpressionName ename;
	ename.name = "MR";
	ename.case_sensitive = true;
	ename.abbreviation = true;
	v_memory->addName(ename);
	ename.name = "MRC";
	v_memory->addName(ename);
	CALCULATOR->addVariable(v_memory);
	CALCULATOR->addFunction(new SetTitleFunction());
	initialize_history_functions();
}

/*
	save preferences, mode and definitions and then quit
*/
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
		view_thread->write(NULL);
	}
	CALCULATOR->terminateThreads();
	g_application_quit(g_application_get_default());
	return TRUE;
}

void create_main_window() {

	mstruct = new MathStructure();
	parsed_mstruct = new MathStructure();
	parsed_tostruct = new MathStructure();
	parsed_tostruct->setUndefined();
	matrix_mstruct = new MathStructure();
	mbak_convert.setUndefined();

	main_builder = getBuilder("main.ui");
	g_assert(main_builder != NULL);

	/* make sure we get a valid main window */
	g_assert(gtk_builder_get_object(main_builder, "main_window") != NULL);

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(main_window(), accel_group);

	if(win_width > 0) gtk_window_set_default_size(main_window(), win_width, win_height > 0 ? win_height : -1);

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14
	gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_swap")), "object-flip-vertical-symbolic", GTK_ICON_SIZE_BUTTON);
#endif

	char **flags_r = g_resources_enumerate_children("/qalculate-gtk/flags", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	if(flags_r) {
		PangoFontDescription *font_desc;
		gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(main_window())), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
		PangoFontset *fontset = pango_context_load_fontset(gtk_widget_get_pango_context(GTK_WIDGET(main_window())), font_desc, pango_context_get_language(gtk_widget_get_pango_context(GTK_WIDGET(main_window()))));
		PangoFontMetrics *metrics = pango_fontset_get_metrics(fontset);
		flagheight = (pango_font_metrics_get_ascent(metrics) + pango_font_metrics_get_descent(metrics)) / PANGO_SCALE;
		pango_font_metrics_unref(metrics);
		g_object_unref(fontset);
		pango_font_description_free(font_desc);
		gint scalefactor = gtk_widget_get_scale_factor(GTK_WIDGET(main_window()));
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

	tabs = GTK_WIDGET(gtk_builder_get_object(main_builder, "tabs"));

	topframe_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "topframe"))), GTK_STYLE_PROVIDER(topframe_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	string topframe_css = "* {background-color: ";

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 16)) {
		GdkRGBA bg_color;
		gtk_style_context_get_background_color(gtk_widget_get_style_context(expression_edit_widget()), GTK_STATE_FLAG_NORMAL, &bg_color);
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
	gtk_widget_set_margin_bottom(keypad_widget(), 3);

	update_app_font(true);
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
		gtk_widget_hide(keypad_widget());
	} else if(show_keypad && !persistent_keypad) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), TRUE);
		gtk_widget_hide(tabs);
		gtk_widget_show(keypad_widget());
	} else if(show_history) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_history), TRUE);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 0);
		gtk_widget_show(tabs);
		gtk_widget_hide(keypad_widget());
	} else if(show_convert) {
		gtk_expander_set_expanded(GTK_EXPANDER(expander_convert), TRUE);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 2);
		gtk_widget_show(tabs);
		gtk_widget_hide(keypad_widget());
	} else {
		gtk_widget_hide(tabs);
		gtk_widget_hide(keypad_widget());
		gtk_widget_set_vexpand(result_view_widget(), TRUE);
	}
	if(persistent_keypad) {
		if(show_keypad) {
			gtk_expander_set_expanded(GTK_EXPANDER(expander_keypad), TRUE);
			gtk_widget_show(keypad_widget());
			gtk_widget_set_vexpand(result_view_widget(), FALSE);
		}
		gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_keypad_lock")), "changes-prevent-symbolic", GTK_ICON_SIZE_BUTTON);
		if(show_convert) gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert")), 6);
	}
	GtkRequisition req;
	gtk_widget_get_preferred_size(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_keypad")), &req, NULL);
	if(req.height < 20) gtk_image_set_pixel_size(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_keypad_lock")), req.height * 0.8);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "popup_menu_item_persistent_keypad")), persistent_keypad);
	gtk_widget_set_vexpand(keypad_widget(), !persistent_keypad || !gtk_widget_get_visible(tabs));

	if(minimal_mode) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar")));
		set_status_bottom_border_visible(false);
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), TRUE);
		gtk_widget_set_vexpand(result_view_widget(), FALSE);
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
	create_result_view();
	create_expression_edit();
	create_expression_status();
	create_menubar();

	update_colors(true);

	if(!rpn_mode) gtk_widget_hide(expander_stack);

	gtk_builder_add_callback_symbols(main_builder, "on_main_window_button_press_event", G_CALLBACK(on_main_window_button_press_event), "on_main_window_close", G_CALLBACK(on_main_window_close), "on_message_bar_response", G_CALLBACK(on_message_bar_response), "on_expander_keypad_button_press_event", G_CALLBACK(on_expander_keypad_button_press_event), "on_expander_keypad_expanded", G_CALLBACK(on_expander_keypad_expanded), "on_expander_history_expanded", G_CALLBACK(on_expander_history_expanded), "on_expander_convert_activate", G_CALLBACK(on_expander_convert_activate), "on_expander_convert_expanded", G_CALLBACK(on_expander_convert_expanded), "on_expander_stack_expanded", G_CALLBACK(on_expander_stack_expanded), "on_configure_event", G_CALLBACK(on_configure_event), "on_key_press_event", G_CALLBACK(on_key_press_event), "on_key_release_event", G_CALLBACK(on_key_release_event), "on_image_keypad_lock_button_press_event", G_CALLBACK(on_image_keypad_lock_button_press_event), "on_image_keypad_lock_button_release_event", G_CALLBACK(on_image_keypad_lock_button_release_event), "on_popup_menu_item_persistent_keypad_toggled", G_CALLBACK(on_popup_menu_item_persistent_keypad_toggled), NULL);

	gtk_builder_connect_signals(main_builder, NULL);

	if(win_height <= 0) gtk_window_get_size(main_window(), NULL, &win_height);
	if(minimal_mode && minimal_width > 0) gtk_window_resize(main_window(), minimal_width, win_height);
	else if(win_width > 0) gtk_window_resize(main_window(), win_width, win_height);

	if(remember_position) {
		GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(main_window()));
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
			gtk_window_get_size(main_window(), &w, &h);
			if(win_x + w > area.width) win_x = area.width - w;
			if(win_y + h > area.height) win_y = area.height - h;
			gtk_window_move(main_window(), win_x + area.x, win_y + area.y);
		} else {
			gtk_window_move(main_window(), win_x, win_y);
		}
	}
	if(always_on_top) gtk_window_set_keep_above(main_window(), always_on_top);

	gtk_widget_show(GTK_WIDGET(main_window()));

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
		gtk_widget_hide(GTK_WIDGET(main_window()));
	}

	view_thread = new ViewThread;
	view_thread->start();
	command_thread = new CommandThread;

}
