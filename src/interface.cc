/*
    Qalculate (GTK+ UI)

    Copyright (C) 2003-2007, 2008, 2016-2020  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "support.h"
#include "callbacks.h"
#include "interface.h"
#include "main.h"

#include <deque>

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
	using std::unordered_map;
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

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::iterator;
using std::list;
using std::deque;

extern GtkBuilder *main_builder, *argumentrules_builder, *csvimport_builder, *csvexport_builder, *datasetedit_builder, *datasets_builder, *setbase_builder, *decimals_builder;
extern GtkBuilder *functionedit_builder, *functions_builder, *matrixedit_builder, *matrix_builder, *namesedit_builder, *nbases_builder, *plot_builder, *precision_builder;
extern GtkBuilder *shortcuts_builder, *preferences_builder, *unitedit_builder, *units_builder, *unknownedit_builder, *variableedit_builder, *variables_builder, *buttonsedit_builder;
extern GtkBuilder *periodictable_builder, *simplefunctionedit_builder, *percentage_builder, *calendarconversion_builder, *floatingpoint_builder;
extern vector<mode_struct> modes;

GtkWidget *mainwindow;

GtkWidget *tFunctionCategories;
GtkWidget *tFunctions;
GtkListStore *tFunctions_store;
GtkTreeModel *tFunctions_store_filter;
GtkTreeStore *tFunctionCategories_store;

GtkWidget *tVariableCategories;
GtkWidget *tVariables;
GtkListStore *tVariables_store;
GtkTreeModel *tVariables_store_filter;
GtkTreeStore *tVariableCategories_store;

GtkWidget *tUnitCategories;
GtkWidget *tUnits;
GtkListStore *tUnits_store;
GtkTreeModel *tUnits_store_filter, *units_convert_filter;
GtkTreeStore *tUnitCategories_store;
GtkWidget *units_convert_view, *units_convert_window, *units_convert_scrolled;
GtkCellRenderer *units_convert_flag_renderer;

GtkWidget *tUnitSelectorCategories;
GtkWidget *tUnitSelector;
GtkListStore *tUnitSelector_store;
GtkTreeModel *tUnitSelector_store_filter;
GtkTreeStore *tUnitSelectorCategories_store;

GtkWidget *tDatasets;
GtkWidget *tDataObjects;
GtkListStore *tDatasets_store;
GtkListStore *tDataObjects_store;

GtkWidget *tDataProperties;
GtkListStore *tDataProperties_store;

GtkWidget *tNames;
GtkListStore *tNames_store;

GtkWidget *tShortcuts, *tShortcutsType;
GtkListStore *tShortcuts_store, *tShortcutsType_store;

GtkWidget *tButtonsEditType, *tButtonsEdit;
GtkListStore *tButtonsEditType_store, *tButtonsEdit_store;

GtkWidget *tabs, *expander_keypad, *expander_history, *expander_stack, *expander_convert;
GtkEntryCompletion *completion;
GtkWidget *completion_view, *completion_window, *completion_scrolled;
GtkTreeModel *completion_filter, *completion_sort;
GtkListStore *completion_store;

GtkWidget *tFunctionArguments;
GtkListStore *tFunctionArguments_store;
GtkWidget *tSubfunctions;
GtkListStore *tSubfunctions_store;

GtkWidget *tPlotFunctions;
GtkListStore *tPlotFunctions_store;

GtkWidget *item_factorize, *item_simplify;

GtkWidget *tMatrixEdit, *tMatrix;
GtkListStore *tMatrixEdit_store, *tMatrix_store;
extern vector<GtkTreeViewColumn*> matrix_edit_columns, matrix_columns;

GtkCellRenderer *history_renderer, *history_index_renderer, *ans_renderer, *register_renderer;
GtkTreeViewColumn *register_column, *history_column, *history_index_column, *flag_column, *units_flag_column;

GtkWidget *expressiontext;
GtkTextBuffer *expressionbuffer;
GtkTextTag *expression_par_tag;
GtkWidget *resultview;
GtkWidget *historyview;
GtkListStore *historystore;
GtkWidget *stackview;
GtkListStore *stackstore;
GtkWidget *statuslabel_l, *statuslabel_r, *result_bases, *keypad;
GtkWidget *f_menu ,*v_menu, *u_menu, *u_menu2, *recent_menu;
GtkAccelGroup *accel_group;

gint history_scroll_width = 16;

GtkCssProvider *expression_provider, *resultview_provider, *statuslabel_l_provider, *statuslabel_r_provider, *keypad_provider, *box_rpnl_provider, *app_provider, *app_provider_theme;

extern bool show_keypad, show_history, show_stack, show_convert, continuous_conversion, set_missing_prefixes, persistent_keypad, minimal_mode;
extern bool save_mode_on_exit, save_defs_on_exit, load_global_defs, hyp_is_on, inv_is_on, fetch_exchange_rates_at_startup, clear_history_on_exit;
extern int allow_multiple_instances;
extern int title_type;
extern bool display_expression_status;
extern int expression_lines;
extern int use_dark_theme;
extern bool use_custom_result_font, use_custom_expression_font, use_custom_status_font, use_custom_keypad_font, use_custom_app_font;
extern string custom_result_font, custom_expression_font, custom_status_font, custom_keypad_font, custom_app_font;
extern string status_error_color, status_warning_color;
extern bool status_error_color_set, status_warning_color_set;
extern int auto_update_exchange_rates;
extern bool copy_separator;
extern bool ignore_locale;
extern bool caret_as_xor;
extern int visible_keypad;
extern bool auto_calculate;
extern bool complex_angle_form;
extern bool check_version;
extern int max_plot_time;
extern int default_fraction_fraction;

extern string nbases_error_color, nbases_warning_color;

extern PrintOptions printops;
extern EvaluationOptions evalops;

extern bool rpn_mode, rpn_keys, adaptive_interval_display, use_e_notation;

extern vector<GtkWidget*> mode_items;
extern vector<GtkWidget*> popup_result_mode_items;

extern deque<string> expression_undo_buffer;

gint win_height, win_width, win_x, win_y, history_height, variables_width, variables_height, variables_position, units_width, units_height, units_position, functions_width, functions_height, functions_hposition, functions_vposition, datasets_width, datasets_height, datasets_hposition, datasets_vposition1, datasets_vposition2;
extern bool remember_position;
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

extern unordered_map<string, GdkPixbuf*> flag_images;

extern unordered_map<guint64, keyboard_shortcut> keyboard_shortcuts;
extern vector<custom_button> custom_buttons;

extern string fix_history_string(const string &str);

gint compare_categories(gconstpointer a, gconstpointer b) {
	return strcasecmp((const char*) a, (const char*) b);
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
		case ASSUMPTION_TYPE_INTEGER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_integer")), TRUE); break;}
		case ASSUMPTION_TYPE_RATIONAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_rational")), TRUE); break;}
		case ASSUMPTION_TYPE_REAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_real")), TRUE); break;}
		case ASSUMPTION_TYPE_COMPLEX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_complex")), TRUE); break;}
		case ASSUMPTION_TYPE_NUMBER: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_number")), TRUE); break;}
		case ASSUMPTION_TYPE_NONMATRIX: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_nonmatrix")), TRUE); break;}
		default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assumptions_none")), TRUE);}
	}
}

void set_mode_items(const PrintOptions &po, const EvaluationOptions &eo, AssumptionType at, AssumptionSign as, bool in_rpn_mode, int precision, bool interval, bool variable_units, bool id_adaptive, int keypad, bool autocalc, bool caf, bool initial_update) {

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), evalops.parse_options.rpn);
	if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_rpn_syntax")), evalops.parse_options.rpn);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_autocalc")), autocalc && (!initial_update || !in_rpn_mode));
	auto_calculate = autocalc;
	if(initial_update) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_autocalc")), !in_rpn_mode);
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
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_default_angle_unit")), TRUE);
			break;
		}
	}
	if(initial_update) update_mb_angles(eo.parse_options.angle_unit);

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), eo.parse_options.read_precision != DONT_READ_PRECISION);
	if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_status_read_precision")), eo.parse_options.read_precision != DONT_READ_PRECISION);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication")), eo.parse_options.limit_implicit_multiplication);
	switch(eo.parse_options.parsing_mode) {
		case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
			break;
		}
		case PARSING_MODE_CONVENTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
			break;
		 }
		 default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
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
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_round_halfway_to_even")), po.round_halfway_to_even);
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
			default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_significant")), TRUE); break;}
		}
	}

	set_assumptions_items(at, as);

	if(!initial_update) {
		if(decimals_builder) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min")), po.use_min_decimals);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max")), po.use_max_decimals);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), po.min_decimals);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), po.max_decimals);
		} else {
			printops.max_decimals = po.max_decimals;
			printops.use_max_decimals = po.use_max_decimals;
			printops.max_decimals = po.min_decimals;
			printops.use_min_decimals = po.use_min_decimals;
		}
		if(precision_builder) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision")), precision);
		} else {
			CALCULATOR->setPrecision(precision);
		}
		printops.spacious = po.spacious;
		printops.short_multiplication = po.short_multiplication;
		printops.excessive_parenthesis = po.excessive_parenthesis;
		evalops.calculate_functions = eo.calculate_functions;
		if(setbase_builder) {
			switch(eo.parse_options.base) {
				case BASE_BINARY: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_binary")), TRUE);
					break;
				}
				case BASE_OCTAL: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_octal")), TRUE);
					break;
				}
				case BASE_DECIMAL: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_decimal")), TRUE);
					break;
				}
				case 12: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_duodecimal")), TRUE);
					break;
				}
				case BASE_HEXADECIMAL: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_hexadecimal")), TRUE);
					break;
				}
				case BASE_ROMAN_NUMERALS: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_roman")), TRUE);
					break;
				}
				default: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_spinbutton_input_other")), eo.parse_options.base);
				}
			}
		} else {
			evalops.parse_options.base = eo.parse_options.base;
		}
	}
	update_keypad_bases();
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

#define SUP_STRING(X) string("<span size=\"x-small\" rise=\"" + i2s((int) (pango_font_description_get_size(font_desc) / 1.5)) + "\">") + string(X) + "</span>"

void set_keypad_tooltip(const gchar *w, const char *s1, const char *s2 = NULL, const char *s3 = NULL, bool b_markup = false, bool b_longpress = true) {
	string str;
	if(s1) str += s1;
	if(s2) {
		if(s1) str += "\n\n";
		if(b_longpress) str += _("Right-click/long press: %s");
		else str += _("Right-click: %s");
		gsub("%s", s2, str);
	}
	if(s3) {
		if(s2) str += "\n";
		else if(s1) str += "\n\n";
		str += _("Middle-click: %s");
		gsub("%s", s3, str);
	}
	if(b_markup) gtk_widget_set_tooltip_markup(GTK_WIDGET(gtk_builder_get_object(main_builder, w)), str.c_str());
	else gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, w)), str.c_str());
	g_signal_connect(gtk_builder_get_object(main_builder, w), "clicked", G_CALLBACK(hide_tooltip), NULL);
}

#define SET_LABEL_AND_TOOLTIP_2(i, w1, w2, l, t1, t2) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? (custom_buttons[i].type[1] == -1 ? NULL : t2) : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()));\
	}

#define SET_LABEL_AND_TOOLTIP_2NL(i, w2, t1, t2) \
	if(index == i || index < 0) {\
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? (custom_buttons[i].type[1] == -1 ? NULL : t2) : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()));\
	}

#define SET_LABEL_AND_TOOLTIP_3(i, w1, w2, l, t1, t2, t3) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? t3 : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()));\
	}

#define SET_LABEL_AND_TOOLTIP_3NP(i, w1, w2, l, t1, t2, t3) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? t3 : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()), false, custom_buttons[i].type[0] == -1);\
	}

#define SET_LABEL_AND_TOOLTIP_3M(i, w1, w2, l, t1, t2, t3) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? t3 : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()), true);\
	}

#define SET_LABEL_AND_TOOLTIP_3NL(i, w2, t1, t2, t3) \
	if(index == i || index < 0) {\
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? t3 : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()));\
	}

#define SET_LABEL_AND_TOOLTIP_3C(i, w1, w2, l) \
	if(index == i || index < 0) {\
		if(index >= 0 || !custom_buttons[i].text.empty()) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, w1)), custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str()); \
		set_keypad_tooltip(w2, custom_buttons[i].type[0] == -1 ? NULL : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0], true) : custom_buttons[i].value[0].c_str()), custom_buttons[i].type[1] == -1 ? NULL : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1], true) : custom_buttons[i].value[1].c_str()), custom_buttons[i].type[2] == -1 ? NULL : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2], true) : custom_buttons[i].value[2].c_str()));\
	}

void update_custom_buttons(int index) {
	if(index == 0 || index < 0) {
		if(custom_buttons[0].text.empty()) gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_move")));
		else gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move")), GTK_WIDGET(gtk_builder_get_object(main_builder, "label_move")));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_move")), custom_buttons[0].text.c_str());
		set_keypad_tooltip("button_move", custom_buttons[0].type[0] == -1 ? _("Cycle through previous expression") : (custom_buttons[0].value[0].empty() ? shortcut_type_text(custom_buttons[0].type[0], true) : custom_buttons[0].value[0].c_str()), custom_buttons[0].type[1] < 0 ? NULL : (custom_buttons[0].value[1].empty() ? shortcut_type_text(custom_buttons[0].type[1], true) : custom_buttons[0].value[1].c_str()), custom_buttons[0].type[2] < 0 ? NULL : (custom_buttons[0].value[2].empty() ? shortcut_type_text(custom_buttons[0].type[2], true) : custom_buttons[0].value[2].c_str()), false, custom_buttons[0].type[0] != -1);
	}
	if(index == 1 || index < 0) {
		if(custom_buttons[1].text.empty()) gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move2")), GTK_WIDGET(gtk_builder_get_object(main_builder, "box_move2")));
		else gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(main_builder, "stack_move2")), GTK_WIDGET(gtk_builder_get_object(main_builder, "label_move2")));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_move2")), custom_buttons[1].text.c_str());
		set_keypad_tooltip("button_move2", custom_buttons[1].type[0] == -1 ? _("Move cursor left or right") : (custom_buttons[1].value[0].empty() ? shortcut_type_text(custom_buttons[1].type[0], true) : custom_buttons[1].value[0].c_str()), custom_buttons[1].type[1] == -1 ? _("Move cursor to beginning or end") : (custom_buttons[1].value[1].empty() ? shortcut_type_text(custom_buttons[1].type[1], true) : custom_buttons[1].value[1].c_str()), custom_buttons[1].type[2] == -1 ? (custom_buttons[1].type[1] == -1 ? NULL : _("Move cursor to beginning or end")) : (custom_buttons[1].value[2].empty() ? shortcut_type_text(custom_buttons[1].type[2], true) : custom_buttons[1].value[2].c_str()), false, custom_buttons[1].type[0] != -1);
	}
	SET_LABEL_AND_TOOLTIP_2(2, "label_percent", "button_percent", "%", CALCULATOR->v_percent->title(true).c_str(), CALCULATOR->v_permille->title(true).c_str())
	SET_LABEL_AND_TOOLTIP_3(3, "label_plusminus", "button_plusminus", "±", _("Uncertainty/interval"), _("Relative error"), _("Interval"))
	SET_LABEL_AND_TOOLTIP_3(4, "label_comma", "button_comma", CALCULATOR->getComma().c_str(), _("Argument separator"), _("Blank space"), _("New line"))
	SET_LABEL_AND_TOOLTIP_2(5, "label_brace_wrap", "button_brace_wrap", "(x)", _("Smart parentheses"), _("Vector brackets"))
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_brace_wrap")), custom_buttons[5].text.empty() ? "(x)" : custom_buttons[5].text.c_str());
	SET_LABEL_AND_TOOLTIP_2(6, "label_brace_open", "button_brace_open", "(", _("Left parenthesis"), _("Left vector bracket"))
	SET_LABEL_AND_TOOLTIP_2(7, "label_brace_close", "button_brace_close", ")", _("Right parenthesis"), _("Right vector bracket"))
	SET_LABEL_AND_TOOLTIP_3M(8, "label_zero", "button_zero", "0", NULL, "x<sup>0</sup>", CALCULATOR->getDegUnit()->title(true).c_str())
	SET_LABEL_AND_TOOLTIP_3M(9, "label_one", "button_one", "1", NULL, "x<sup>1</sup>", "1/x")
	SET_LABEL_AND_TOOLTIP_3M(10, "label_two", "button_two", "2", NULL, "x<sup>2</sup>", "1/2")
	SET_LABEL_AND_TOOLTIP_3M(11, "label_three", "button_three", "3", NULL, "x<sup>3</sup>", "1/3")
	SET_LABEL_AND_TOOLTIP_3M(12, "label_four", "button_four", "4", NULL, "x<sup>4</sup>", "1/4")
	SET_LABEL_AND_TOOLTIP_3M(13, "label_five", "button_five", "5", NULL, "x<sup>5</sup>", "1/5")
	SET_LABEL_AND_TOOLTIP_3M(14, "label_six", "button_six", "6", NULL, "x<sup>6</sup>", "1/6")
	SET_LABEL_AND_TOOLTIP_3M(15, "label_seven", "button_seven", "7", NULL, "x<sup>7</sup>", "1/7")
	SET_LABEL_AND_TOOLTIP_3M(16, "label_eight", "button_eight", "8", NULL, "x<sup>8</sup>", "1/8")
	SET_LABEL_AND_TOOLTIP_3M(17, "label_nine", "button_nine", "9", NULL, "x<sup>9</sup>", "1/9")
	SET_LABEL_AND_TOOLTIP_3(18, "label_dot", "button_dot", CALCULATOR->getDecimalPoint().c_str(), _("Decimal point"), _("Blank space"), _("New line"))
	if(index < 0 || index == 19) {
		MathFunction *f = CALCULATOR->getActiveFunction("exp10");
		SET_LABEL_AND_TOOLTIP_3M(19, "label_exp", "button_exp", _("EXP"), "10<sup>x</sup> (Ctrl+Shift+E)", CALCULATOR->f_exp->title(true).c_str(), (f ? f->title(true).c_str() : NULL))
	}
	if(index == 20 || (index < 0 && !custom_buttons[20].text.empty())) {
		if(custom_buttons[20].text.empty()) {
			PangoFontDescription *font_desc = NULL;
			gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_xy"))), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_xy")), (string("x") + SUP_STRING("y")).c_str());
			pango_font_description_free(font_desc);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_xy")), custom_buttons[20].text.c_str());
		}
	}
	SET_LABEL_AND_TOOLTIP_2NL(20, "button_xy", _("Raise (Ctrl+*)"), CALCULATOR->f_sqrt->title(true).c_str())
	if(index == 24 || (index < 0 && !custom_buttons[24].text.empty())) {
		if(custom_buttons[24].text.empty()) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(main_builder, "label_sub"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), SIGN_MINUS);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), MINUS);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_sub")), custom_buttons[24].text.c_str());
		}
	}
	if(index == 22 || (index < 0 && !custom_buttons[22].text.empty())) {
		if(custom_buttons[22].text.empty()) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(main_builder, "label_times"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), SIGN_MULTIPLICATION);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), MULTIPLICATION);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_times")), custom_buttons[22].text.c_str());
		}
	}
	if(index == 21 || (index < 0 && !custom_buttons[21].text.empty())) {
		if(custom_buttons[21].text.empty()) {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), SIGN_DIVISION_SLASH);
			else if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(main_builder, "label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), SIGN_DIVISION);
			else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), DIVISION);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_divide")), custom_buttons[21].text.c_str());
		}
	}
	SET_LABEL_AND_TOOLTIP_2NL(21, "button_divide", _("Divide"), "1/x")
	SET_LABEL_AND_TOOLTIP_2NL(22, "button_times", _("Multiply"), _("Bitwise Exclusive OR"))
	SET_LABEL_AND_TOOLTIP_3(23, "label_add", "button_add", "+", _("Add"), _("M+ (memory plus)"), _("Bitwise AND"))
	if(index < 0 || index == 24) {
		MathFunction *f = CALCULATOR->getActiveFunction("neg");
		SET_LABEL_AND_TOOLTIP_3NL(24, "button_sub", _("Subtract"), f ? f->title(true).c_str() : NULL, _("Bitwise OR"));
	}
	SET_LABEL_AND_TOOLTIP_2(25, "label_ac", "button_ac", _("AC"), _("Clear"), _("MC (memory clear)"))
	SET_LABEL_AND_TOOLTIP_3NP(26, "label_del", "button_del", _("DEL"), _("Delete"), _("Backspace"), _("M− (memory minus)"))
	SET_LABEL_AND_TOOLTIP_2(27, "label_ans", "button_ans", _("ANS"), _("Previous result"), _("Previous result (static)"))
	if(index == 28 || (index < 0 && !custom_buttons[28].text.empty())) {
		if(custom_buttons[28].text.empty()) {
			gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), "<big>=</big>");
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_equals")), custom_buttons[28].text.c_str());
		}
	}
	SET_LABEL_AND_TOOLTIP_3NL(28, "button_equals", _("Calculate expression"), _("MR (memory recall)"), _("MS (memory store)"))
	SET_LABEL_AND_TOOLTIP_3C(29, "label_c1", "button_c1", "C1")
	SET_LABEL_AND_TOOLTIP_3C(30, "label_c2", "button_c2", "C2")
	SET_LABEL_AND_TOOLTIP_3C(31, "label_c3", "button_c3", "C3")
	SET_LABEL_AND_TOOLTIP_3C(32, "label_c4", "button_c4", "C4")
	SET_LABEL_AND_TOOLTIP_3C(33, "label_c5", "button_c5", "C5")
	if(index >= 29) {
		bool b_show = (custom_buttons[29].type[0] >= 0 || custom_buttons[29].type[1] >= 0 || custom_buttons[29].type[2] >= 0 || custom_buttons[30].type[0] >= 0 || custom_buttons[30].type[1] >= 0 || custom_buttons[30].type[2] >= 0 || custom_buttons[31].type[0] >= 0 || custom_buttons[31].type[1] >= 0 || custom_buttons[31].type[2] >= 0 || custom_buttons[32].type[0] >= 0 || custom_buttons[32].type[1] >= 0 || custom_buttons[32].type[2] >= 0 || custom_buttons[33].type[0] >= 0 || custom_buttons[33].type[1] >= 0 || custom_buttons[33].type[2] >= 0);
		if(b_show != gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons")))) {
			if(!b_show && gtk_expander_get_expanded(GTK_EXPANDER(expander_keypad)) && !minimal_mode) {
				gint w_c = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons"))) + 6;
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons")), b_show);
				while(gtk_events_pending()) gtk_main_iteration();
				gint w, h;
				gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), &w, &h);
				gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), w - w_c, h);
			} else {
				gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons")), b_show);
			}
		}
	}
}

void set_custom_buttons() {
	if(!latest_button_unit_pre.empty()) {
		latest_button_unit = CALCULATOR->getActiveUnit(latest_button_unit_pre);
		if(!latest_button_unit) latest_button_unit = CALCULATOR->getCompositeUnit(latest_button_unit_pre);
	}
	if(latest_button_unit) {
		string si_label_str;
		if(latest_button_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			si_label_str = ((CompositeUnit*) latest_button_unit)->print(false, true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext);
		} else {

			si_label_str = latest_button_unit->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name;
		}
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_si")), si_label_str.c_str());
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_si")), latest_button_unit->title(true).c_str());
	}
	Unit *u_local_currency = CALCULATOR->getLocalCurrency();
	if(!latest_button_currency_pre.empty()) {
		latest_button_currency = CALCULATOR->getActiveUnit(latest_button_currency_pre);
	}
	if(!latest_button_currency && u_local_currency) latest_button_currency = u_local_currency;
	if(!latest_button_currency) latest_button_currency = CALCULATOR->u_euro;
	string unit_label_str;
	if(latest_button_currency->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		unit_label_str = ((CompositeUnit*) latest_button_currency)->print(false, true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expressiontext);
	} else {

		unit_label_str = latest_button_currency->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).name;
	}
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(main_builder, "label_euro")), unit_label_str.c_str());
	gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_euro")), latest_button_currency->title(true).c_str());
}

void create_button_menus() {

	GtkWidget *item, *sub;
	MathFunction *f;

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_bases"));
	MENU_ITEM(_("Bitwise Left Shift"), insert_left_shift)
	MENU_ITEM(_("Bitwise Right Shift"), insert_right_shift)
	MENU_ITEM(_("Bitwise AND"), insert_bitwise_and)
	MENU_ITEM(_("Bitwise OR"), insert_bitwise_or)
	MENU_ITEM(_("Bitwise Exclusive OR"), insert_bitwise_xor)
	MENU_ITEM(_("Bitwise NOT"), insert_bitwise_not)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_bitcmp->title(true).c_str(), insert_button_function, CALCULATOR->f_bitcmp)
	f = CALCULATOR->getActiveFunction("bitrot");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f)}
	MENU_SEPARATOR
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_base->title(true).c_str(), insert_button_function, CALCULATOR->f_base)
	MENU_SEPARATOR
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_ascii->title(true).c_str(), insert_button_function, CALCULATOR->f_ascii)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_char->title(true).c_str(), insert_button_function, CALCULATOR->f_char)

	PangoFontDescription *font_desc;

	gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_xy"))), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);

	g_signal_connect(gtk_builder_get_object(main_builder, "button_e_var"), "clicked", G_CALLBACK(insert_button_variable), (gpointer) CALCULATOR->v_e);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_e"));
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_exp->title(true).c_str(), insert_button_function, CALCULATOR->f_exp)
	f = CALCULATOR->getActiveFunction("cis");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}

	pango_font_description_free(font_desc);

	g_signal_connect(gtk_builder_get_object(main_builder, "button_sqrt"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_sqrt);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_sqrt"));
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_cbrt->title(true).c_str(), insert_button_function, CALCULATOR->f_cbrt);
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_root->title(true).c_str(), insert_button_function, CALCULATOR->f_root);
	f = CALCULATOR->getActiveFunction("sqrtpi");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}

	g_signal_connect(gtk_builder_get_object(main_builder, "button_ln"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_ln);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_ln"));
	f = CALCULATOR->getActiveFunction("log10");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("log2");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_logn->title(true).c_str(), insert_button_function, CALCULATOR->f_logn)

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_fac"));
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_factorial2->title(true).c_str(), insert_button_function, CALCULATOR->f_factorial2)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_multifactorial->title(true).c_str(), insert_button_function, CALCULATOR->f_multifactorial)
	f = CALCULATOR->getActiveFunction("hyperfactorial");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("superfactorial");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("perm");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("comb");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("derangements");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_binomial->title(true).c_str(), insert_button_function, CALCULATOR->f_binomial)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_mod"), "clicked", G_CALLBACK(insert_function_operator), (gpointer) CALCULATOR->f_mod);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_mod"));
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_rem->title(true).c_str(), insert_function_operator, CALCULATOR->f_rem)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_abs->title(true).c_str(), insert_button_function, CALCULATOR->f_abs)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_gcd->title(true).c_str(), insert_button_function, CALCULATOR->f_gcd)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_lcm->title(true).c_str(), insert_button_function, CALCULATOR->f_lcm)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_sine"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_sin);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_sin"));
	MENU_SEPARATOR_PREPEND
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_asinh->title(true).c_str(), insert_button_function, CALCULATOR->f_asinh)
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_asin->title(true).c_str(), insert_button_function, CALCULATOR->f_asin)
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_sinh->title(true).c_str(), insert_button_function, CALCULATOR->f_sinh)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_cosine"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_cos);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_cos"));
	MENU_SEPARATOR_PREPEND
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_acosh->title(true).c_str(), insert_button_function, CALCULATOR->f_acosh)
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_acos->title(true).c_str(), insert_button_function, CALCULATOR->f_acos)
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_cosh->title(true).c_str(), insert_button_function, CALCULATOR->f_cosh)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_tan"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_tan);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_tan"));
	MENU_SEPARATOR_PREPEND
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_atanh->title(true).c_str(), insert_button_function, CALCULATOR->f_atanh)
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_atan->title(true).c_str(), insert_button_function, CALCULATOR->f_atan)
	MENU_ITEM_WITH_POINTER_PREPEND(CALCULATOR->f_tanh->title(true).c_str(), insert_button_function, CALCULATOR->f_tanh)

	g_signal_connect(gtk_builder_get_object(main_builder, "button_sum"), "clicked", G_CALLBACK(insert_button_function_norpn), (gpointer) CALCULATOR->f_sum);
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_sum"));
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_product->title(true).c_str(), insert_button_function_norpn, CALCULATOR->f_product)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_for->title(true).c_str(), insert_button_function_norpn, CALCULATOR->f_for)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_if->title(true).c_str(), insert_button_function_norpn, CALCULATOR->f_if)


	g_signal_connect(gtk_builder_get_object(main_builder, "button_mean"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->getActiveFunction("mean"));
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_mean"));
	f = CALCULATOR->getActiveFunction("median");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("var");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("stdev");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("stderr");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("harmmean");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("geomean");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	MENU_SEPARATOR
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_rand->title(true).c_str(), insert_button_function, CALCULATOR->f_rand);

	g_signal_connect(gtk_builder_get_object(main_builder, "button_pi"), "clicked", G_CALLBACK(insert_button_variable), (gpointer) CALCULATOR->v_pi);

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_xequals"));
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_solve->title(true).c_str(), insert_button_function, CALCULATOR->f_solve)
	f = CALCULATOR->getActiveFunction("solve2");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("linearfunction");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_dsolve->title(true).c_str(), insert_button_function, CALCULATOR->f_dsolve)
	f = CALCULATOR->getActiveFunction("extremum");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	MENU_SEPARATOR
	MENU_ITEM(_("Set unknowns"), on_menu_item_set_unknowns_activate)

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_factorize"));
	MENU_ITEM(_("Expand"), on_menu_item_simplify_activate)
	item_simplify = item;
	MENU_ITEM(_("Factorize"), on_menu_item_factorize_activate)
	item_factorize = item;
	MENU_ITEM(_("Expand Partial Fractions"), on_menu_item_expand_partial_fractions_activate)
	MENU_SEPARATOR
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_diff->title(true).c_str(), insert_button_function, CALCULATOR->f_diff)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_integrate->title(true).c_str(), insert_button_function, CALCULATOR->f_integrate)
	gtk_widget_set_visible(item_factorize, evalops.structuring != STRUCTURING_SIMPLIFY);
	gtk_widget_set_visible(item_simplify, evalops.structuring != STRUCTURING_FACTORIZE);

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_i"));
	MENU_ITEM("∠ (Ctrl+Shift+A)", insert_angle_symbol)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_re->title(true).c_str(), insert_button_function, CALCULATOR->f_re)
	MENU_ITEM_WITH_POINTER(CALCULATOR->f_im->title(true).c_str(), insert_button_function, CALCULATOR->f_im)
	f = CALCULATOR->getActiveFunction("arg");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}
	f = CALCULATOR->getActiveFunction("conj");
	if(f) {MENU_ITEM_WITH_POINTER(f->title(true).c_str(), insert_button_function, f);}

	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_si"));
	const char *si_units[] = {"m", "g", "s", "A", "K", "N", "Pa", "J", "W", "L", "V", "ohm", "oC", "cd", "mol", "C", "Hz", "F", "S", "Wb", "T", "H", "lm", "lx", "Bq", "Gy", "Sv", "kat"};
	vector<Unit*> to_us;
	size_t si_i = 0;
	size_t i_added = 0;
	for(; si_i < 27 && i_added < 12; si_i++) {
		Unit * u = CALCULATOR->getActiveUnit(si_units[si_i]);
		if(u && !u->isHidden()) {
			bool b = false;
			for(size_t i2 = 0; i2 < to_us.size(); i2++) {
				if(string_is_less(u->title(true), to_us[i2]->title(true))) {
					to_us.insert(to_us.begin() + i2, u);
					b = true;
					break;
				}
			}
			if(!b) to_us.push_back(u);
			i_added++;
		}
	}
	for(size_t i = 0; i < to_us.size(); i++) {
		MENU_ITEM_WITH_POINTER(to_us[i]->title(true).c_str(), insert_button_unit, to_us[i])
	}

	// Show further items in a submenu
	if(si_i < 27) {SUBMENU_ITEM(_("more"), sub);}

	to_us.clear();
	for(; si_i < 27; si_i++) {
		Unit * u = CALCULATOR->getActiveUnit(si_units[si_i]);
		if(u && !u->isHidden()) {
			bool b = false;
			for(size_t i2 = 0; i2 < to_us.size(); i2++) {
				if(string_is_less(u->title(true), to_us[i2]->title(true))) {
					to_us.insert(to_us.begin() + i2, u);
					b = true;
					break;
				}
			}
			if(!b) to_us.push_back(u);
		}
	}
	for(size_t i = 0; i < to_us.size(); i++) {
		MENU_ITEM_WITH_POINTER(to_us[i]->title(true).c_str(), insert_button_unit, to_us[i])
	}

	Unit *u_local_currency = CALCULATOR->getLocalCurrency();
	sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_euro"));
	const char *currency_units[] = {"USD", "GBP", "JPY"};
	to_us.clear();
	for(size_t i = 0; i < 5; i++) {
		Unit * u;
		if(i == 0) u = CALCULATOR->u_euro;
		else if(i == 1) u = u_local_currency;
		else u = CALCULATOR->getActiveUnit(currency_units[i - 2]);
		if(u && (i == 1 || !u->isHidden())) {
			bool b = false;
			for(size_t i2 = 0; i2 < to_us.size(); i2++) {
				if(u == to_us[i2]) {
					b = true;
					break;
				}
				if(string_is_less(u->title(true), to_us[i2]->title(true))) {
					to_us.insert(to_us.begin() + i2, u);
					b = true;
					break;
				}
			}
			if(!b) to_us.push_back(u);
		}
	}
	for(size_t i = 0; i < to_us.size(); i++) {
		MENU_ITEM_WITH_POINTER_AND_FLAG(to_us[i]->title(true).c_str(), insert_button_currency, to_us[i])
	}

	i_added = to_us.size();
	vector<Unit*> to_us2;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->baseUnit() == CALCULATOR->u_euro) {
			Unit *u = CALCULATOR->units[i];
			if(u->isActive()) {
				bool b = false;
				if(u->isHidden() && u != u_local_currency) {
					for(int i2 = to_us2.size() - 1; i2 >= 0; i2--) {
						if(u->title(true) > to_us2[(size_t) i2]->title(true)) {
							if((size_t) i2 == to_us2.size() - 1) to_us2.push_back(u);
							else to_us2.insert(to_us2.begin() + (size_t) i2 + 1, u);
							b = true;
							break;
						}
					}
					if(!b) to_us2.insert(to_us2.begin(), u);
				} else {
					for(size_t i2 = 0; i2 < i_added; i2++) {
						if(u == to_us[i2]) {
							b = true;
							break;
						}
					}
					for(size_t i2 = to_us.size() - 1; !b && i2 >= i_added; i2--) {
						if(u->title(true) > to_us[i2]->title(true)) {
							if(i2 == to_us.size() - 1) to_us.push_back(u);
							else to_us.insert(to_us.begin() + i2 + 1, u);
							b = true;
						}
					}
					if(!b) to_us.insert(to_us.begin() + i_added, u);
				}
			}
		}
	}
	for(size_t i = i_added; i < to_us.size(); i++) {
		// Show further items in a submenu
		if(i == i_added) {SUBMENU_ITEM(_("more"), sub);}
		MENU_ITEM_WITH_POINTER_AND_FLAG(to_us[i]->title(true).c_str(), insert_button_currency, to_us[i])
	}
	if(to_us2.size() > 0) {SUBMENU_ITEM(_("more"), sub);}
	for(size_t i = 0; i < to_us2.size(); i++) {
		// Show further items in a submenu
		MENU_ITEM_WITH_POINTER_AND_FLAG(to_us2[i]->title(true).c_str(), insert_button_currency, to_us2[i])
	}

	set_keypad_tooltip("button_e_var", CALCULATOR->v_e->title(true).c_str());
	set_keypad_tooltip("button_pi", CALCULATOR->v_pi->title(true).c_str());
	set_keypad_tooltip("button_sine", CALCULATOR->f_sin->title(true).c_str());
	set_keypad_tooltip("button_cosine", CALCULATOR->f_cos->title(true).c_str());
	set_keypad_tooltip("button_tan", CALCULATOR->f_tan->title(true).c_str());
	f = CALCULATOR->getActiveFunction("mean");
	if(f) set_keypad_tooltip("button_mean", f->title(true).c_str());
	set_keypad_tooltip("button_sum", CALCULATOR->f_sum->title(true).c_str());
	set_keypad_tooltip("button_mod", CALCULATOR->f_mod->title(true).c_str());
	set_keypad_tooltip("button_fac", CALCULATOR->f_factorial->title(true).c_str());
	set_keypad_tooltip("button_ln", CALCULATOR->f_ln->title(true).c_str());
	set_keypad_tooltip("button_sqrt", CALCULATOR->f_sqrt->title(true).c_str());
	set_keypad_tooltip("button_i", CALCULATOR->v_i->title(true).c_str());
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(main_builder, "label_i")), (string("<i>") + CALCULATOR->v_i->preferredDisplayName(true, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "label_i")).name + "</i>").c_str());

	g_signal_connect(gtk_builder_get_object(main_builder, "button_cmp"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_bitcmp);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_int"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_int);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_frac"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_frac);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_ln2"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_ln);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_sqrt2"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_sqrt);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_abs"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_abs);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_expf"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_exp);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_stamptodate"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_stamptodate);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_code"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_ascii);
	//g_signal_connect(gtk_builder_get_object(main_builder, "button_rnd"), "clicked", G_CALLBACK(insert_button_function), (gpointer) CALCULATOR->f_rand);
	g_signal_connect(gtk_builder_get_object(main_builder, "button_mod2"), "clicked", G_CALLBACK(insert_function_operator), (gpointer) CALCULATOR->f_mod);

	f = CALCULATOR->getActiveFunction("exp2");
	set_keypad_tooltip("button_expf", CALCULATOR->f_exp->title(true).c_str(), f ? f->title(true).c_str() : NULL);
	set_keypad_tooltip("button_mod2", CALCULATOR->f_mod->title(true).c_str(), CALCULATOR->f_rem->title(true).c_str());
	set_keypad_tooltip("button_ln2", CALCULATOR->f_ln->title(true).c_str());
	set_keypad_tooltip("button_int", CALCULATOR->f_int->title(true).c_str());
	set_keypad_tooltip("button_frac", CALCULATOR->f_frac->title(true).c_str());
	set_keypad_tooltip("button_stamptodate", CALCULATOR->f_stamptodate->title(true).c_str(), CALCULATOR->f_timestamp->title(true).c_str());
	set_keypad_tooltip("button_code", CALCULATOR->f_ascii->title(true).c_str(), CALCULATOR->f_char->title(true).c_str());
	f = CALCULATOR->getActiveFunction("log2");
	MathFunction *f2 = CALCULATOR->getActiveFunction("log10");
	set_keypad_tooltip("button_log2", f ? f->title(true).c_str() : NULL, f2 ? f2->title(true).c_str() : NULL);
	if(f) g_signal_connect(gtk_builder_get_object(main_builder, "button_log2"), "clicked", G_CALLBACK(insert_button_function), (gpointer) f);
	set_keypad_tooltip("button_reciprocal", "1/x");
	f = CALCULATOR->getActiveFunction("div");
	if(f) set_keypad_tooltip("button_idiv", f->title(true).c_str());
	set_keypad_tooltip("button_sqrt2", CALCULATOR->f_sqrt->title(true).c_str(), CALCULATOR->f_cbrt->title(true).c_str());
	set_keypad_tooltip("button_abs", CALCULATOR->f_abs->title(true).c_str());
	set_keypad_tooltip("button_fac2", CALCULATOR->f_factorial->title(true).c_str());
	//set_keypad_tooltip("button_rnd", CALCULATOR->f_rand->title(true).c_str());
	set_keypad_tooltip("button_cmp", CALCULATOR->f_bitcmp->title(true).c_str());
	f = CALCULATOR->getActiveFunction("bitrot");
	if(f) {
		set_keypad_tooltip("button_rot", f->title(true).c_str());
		g_signal_connect(gtk_builder_get_object(main_builder, "button_rot"), "clicked", G_CALLBACK(insert_button_function), (gpointer) f);
	}

	set_keypad_tooltip("button_and", _("Bitwise AND"), _("Logical AND"));
	set_keypad_tooltip("button_or", _("Bitwise OR"), _("Logical OR"));
	set_keypad_tooltip("button_not", _("Bitwise NOT"), _("Logical NOT"));

	set_keypad_tooltip("button_bin", _("Binary"), _("Toggle Result Base"));
	set_keypad_tooltip("button_oct", _("Octal"), _("Toggle Result Base"));
	set_keypad_tooltip("button_dec", _("Decimal"), _("Toggle Result Base"));
	set_keypad_tooltip("button_hex", _("Hexadecimal"), _("Toggle Result Base"));

	set_keypad_tooltip("button_store2", _("Store result as a variable"), _("Open menu with stored variables"));

	if(caret_as_xor) set_keypad_tooltip("button_xor", _("Bitwise Exclusive OR"));
	else set_keypad_tooltip("button_xor", (string(_("Bitwise Exclusive OR")) + " (Ctrl+^)").c_str());

	update_mb_fx_menu();
	update_mb_sto_menu();
	update_mb_units_menu();
	update_mb_pi_menu();
	update_mb_to_menu();

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
	GtkWidget *arrow_left = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
	gtk_widget_set_size_request(GTK_WIDGET(arrow_left), 18, 18);
	gtk_widget_show(arrow_left);
	gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(main_builder, "image_hide_left_buttons")));
	gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(main_builder, "event_hide_left_buttons")), arrow_left);
	GtkWidget *arrow_right = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_OUT);
	gtk_widget_set_size_request(GTK_WIDGET(arrow_right), 18, 18);
	gtk_widget_show(arrow_right);
	gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(main_builder, "image_hide_right_buttons")));
	gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(main_builder, "event_hide_right_buttons")), arrow_right);
	gtk_grid_set_column_spacing(GTK_GRID(gtk_builder_get_object(main_builder, "grid_buttons")), 0);
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
		for(size_t i = 0; flags_r[i] != NULL; i++) {
			string flag_s = flags_r[i];
			size_t i_ext = flag_s.find(".", 1);
			if(i_ext != string::npos) {
				GdkPixbuf *flagbuf = gdk_pixbuf_new_from_resource((string("/qalculate-gtk/flags/") + flag_s).c_str(), NULL);
				if(flagbuf) flag_images[flag_s.substr(0, i_ext)] = flagbuf;
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
	result_bases = GTK_WIDGET(gtk_builder_get_object(main_builder, "label_result_bases"));
	keypad = GTK_WIDGET(gtk_builder_get_object(main_builder, "buttons"));
	tabs = GTK_WIDGET(gtk_builder_get_object(main_builder, "tabs"));

	gtk_widget_set_margin_top(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusbox")), 3);
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
	gtk_label_set_xalign(GTK_LABEL(result_bases), 1.0);
	gtk_label_set_yalign(GTK_LABEL(result_bases), 0.5);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_l), 0.5);
	gtk_label_set_yalign(GTK_LABEL(statuslabel_r), 0.5);
#else
	gtk_misc_set_alignment(GTK_MISC(statuslabel_l), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(result_bases), 1.0, 0.5);
#endif

	expression_provider = gtk_css_provider_new();
	resultview_provider = gtk_css_provider_new();
	statuslabel_l_provider = gtk_css_provider_new();
	statuslabel_r_provider = gtk_css_provider_new();
	keypad_provider = gtk_css_provider_new();
	box_rpnl_provider = gtk_css_provider_new();
	app_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(expressiontext), GTK_STYLE_PROVIDER(expression_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(resultview), GTK_STYLE_PROVIDER(resultview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_l), GTK_STYLE_PROVIDER(statuslabel_l_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(statuslabel_r), GTK_STYLE_PROVIDER(statuslabel_r_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(keypad), GTK_STYLE_PROVIDER(keypad_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl"))), GTK_STYLE_PROVIDER(box_rpnl_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(app_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	GtkCssProvider *topframe_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "topframe"))), GTK_STYLE_PROVIDER(topframe_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 16
	GdkRGBA bg_color;
	gtk_style_context_get_background_color(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, &bg_color);
	gchar *gstr = gdk_rgba_to_string(&bg_color);
	gtk_css_provider_load_from_data(topframe_provider, (string("* {background-color: ") + string(gstr) + "; border-left: 0; border-right: 0;}").c_str(), -1, NULL);
	g_free(gstr);
#else
	gtk_css_provider_load_from_data(topframe_provider, "* {background-color: @theme_base_color; border-left: 0; border-right: 0;}", -1, NULL);
#endif


#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
#	ifdef _WIN32
	app_provider_theme = gtk_css_provider_new();
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(app_provider_theme), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	if(use_dark_theme > 0) gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/Adwaita/gtk-contained-dark.css");
	else if(use_dark_theme == 0) gtk_css_provider_load_from_resource(app_provider_theme, "/org/gtk/libgtk/theme/Adwaita/gtk-contained.css");
#	endif
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_result_bases")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_result_bases")), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_label_unit")), 12);
	gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
#else
	gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_result_bases")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "label_result_bases")), 6);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_label_unit")), 12);
	gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 12);
#endif
	gtk_widget_set_margin_bottom(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_tabs")), 6);
	gtk_widget_set_margin_bottom(tabs, 6);
	gtk_widget_set_margin_bottom(keypad, 6);

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
	
	set_mode_items(printops, evalops, CALCULATOR->defaultAssumptions()->type(), CALCULATOR->defaultAssumptions()->sign(), rpn_mode, CALCULATOR->getPrecision(), CALCULATOR->usesIntervalArithmetic(), CALCULATOR->variableUnitsEnabled(), adaptive_interval_display, visible_keypad, auto_calculate, complex_angle_form, true);

	if(use_custom_app_font) {
		gchar *gstr = font_name_to_css(custom_app_font.c_str());
		gtk_css_provider_load_from_data(app_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		if(custom_app_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(mainwindow), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			custom_app_font = pango_font_description_to_string(font_desc);
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
			custom_result_font = pango_font_description_to_string(font_desc);
			pango_font_description_free(font_desc);
		}
	}
	if(use_custom_expression_font) {
		gchar *gstr = font_name_to_css(custom_expression_font.c_str(), "textview.view");
		gtk_css_provider_load_from_data(expression_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(expression_provider, "textview.view {font-size: large;}", -1, NULL);
		if(custom_expression_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			custom_expression_font = pango_font_description_to_string(font_desc);
			pango_font_description_free(font_desc);
		}
	}
	if(use_custom_status_font) {
		gchar *gstr = font_name_to_css(custom_status_font.c_str());
		gtk_css_provider_load_from_data(statuslabel_l_provider, gstr, -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(statuslabel_l_provider, "* {font-size: small;}", -1, NULL);
		gtk_css_provider_load_from_data(statuslabel_r_provider, "* {font-size: small;}", -1, NULL);
		if(custom_status_font.empty()) {
			PangoFontDescription *font_desc;
			gtk_style_context_get(gtk_widget_get_style_context(statuslabel_l), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			custom_status_font = pango_font_description_to_string(font_desc);
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
			custom_keypad_font = pango_font_description_to_string(font_desc);
			pango_font_description_free(font_desc);
		}
	}

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

	set_unicode_buttons();

	set_operator_symbols();
	GdkRGBA c;
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

	gtk_style_context_get_color(gtk_widget_get_style_context(expressiontext), GTK_STATE_FLAG_NORMAL, &c);
	if(c.green >= 0.8) {
		c.red /= 1.5;
		c.blue /= 1.5;
		c.green = 1.0;
	} else {
		if(c.green >= 0.5) c.green = 1.0;
		else c.green += 0.5;
	}
	PangoLayout *layout_par = gtk_widget_create_pango_layout(expressiontext, "()");
	gint w1 = 0, w2 = 0;
	pango_layout_get_pixel_size(layout_par, &w1, NULL);
	pango_layout_set_markup(layout_par, "<b>()</b>", -1);
	pango_layout_get_pixel_size(layout_par, &w2, NULL);
	g_object_unref(layout_par);
	if(w1 == w2) expression_par_tag = gtk_text_buffer_create_tag(expressionbuffer, "curpar", "foreground-rgba", &c, "weight", PANGO_WEIGHT_BOLD, NULL);
	else expression_par_tag = gtk_text_buffer_create_tag(expressionbuffer, "curpar", "foreground-rgba", &c, NULL);

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
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "statusseparator1")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultoverlay")));
		gtk_widget_set_vexpand(GTK_WIDGET(gtk_builder_get_object(main_builder, "expressionscrolled")), TRUE);
		gtk_widget_set_vexpand(resultview, FALSE);
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_minimal_mode")), minimal_mode);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")), continuous_conversion);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes")), set_missing_prefixes);

#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 20
#	define SET_FOCUS_ON_CLICK(x) gtk_button_set_focus_on_click(GTK_BUTTON(x), FALSE)
#else
#	define SET_FOCUS_ON_CLICK(x) gtk_widget_set_focus_on_click(GTK_WIDGET(x), FALSE)
#endif
#define CHILDREN_SET_FOCUS_ON_CLICK(x) list = gtk_container_get_children(GTK_CONTAINER(gtk_builder_get_object(main_builder, x))); \
	for(l = list; l != NULL; l = l->next) { \
		SET_FOCUS_ON_CLICK(l->data); \
	} \
	g_list_free(list);
#define CHILDREN_SET_FOCUS_ON_CLICK_2(x, y) list = gtk_container_get_children(GTK_CONTAINER(gtk_builder_get_object(main_builder, x))); \
	obj = gtk_builder_get_object(main_builder, y); \
	for(l = list; l != NULL; l = l->next) { \
		if(l->data != obj) SET_FOCUS_ON_CLICK(l->data); \
	} \
	g_list_free(list);

	GList *l, *l2;
	GList *list, *list2;
	GObject *obj;
	CHILDREN_SET_FOCUS_ON_CLICK_2("table_buttons", "grid_numbers")
	CHILDREN_SET_FOCUS_ON_CLICK("box_custom_buttons")
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

	if(themestr.substr(0, 7) == "Adwaita" || themestr.substr(0, 6) == "ooxmox" || themestr == "Breeze" || themestr == "Breeze-Dark" || themestr.substr(0, 4) == "Yaru") {

		GtkCssProvider *link_style_top = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_top, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0}", -1, NULL);
		GtkCssProvider *link_style_bot = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bot, "* {border-top-left-radius: 0; border-top-right-radius: 0}", -1, NULL);
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

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_custom_buttons")), custom_buttons[29].type[0] >= 0 || custom_buttons[29].type[1] >= 0 || custom_buttons[29].type[2] >= 0 || custom_buttons[30].type[0] >= 0 || custom_buttons[30].type[1] >= 0 || custom_buttons[30].type[2] >= 0 || custom_buttons[31].type[0] >= 0 || custom_buttons[31].type[1] >= 0 || custom_buttons[31].type[2] >= 0 || custom_buttons[32].type[0] >= 0 || custom_buttons[32].type[1] >= 0 || custom_buttons[32].type[2] >= 0 || custom_buttons[33].type[0] >= 0 || custom_buttons[33].type[1] >= 0 || custom_buttons[33].type[2] >= 0);

	if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "document-edit-symbolic")) {
		gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_edit")), "gtk-edit", GTK_ICON_SIZE_BUTTON);
#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION <= 18
		if(themestr == "Ambiance" || themestr == "Radiance") {
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

	GdkRGBA c_gray = c;
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
	history_column = gtk_tree_view_column_new_with_attributes(_("History"), history_renderer, "markup", 0, "ypad", 4, "xpad", 5, "xalign", 6, "alignment", 7, NULL);
	gtk_tree_view_column_set_expand(history_column, TRUE);
	GtkWidget *scrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "historyscrolled")));
	if(scrollbar) gtk_widget_get_preferred_width(scrollbar, NULL, &history_scroll_width);
	if(history_scroll_width == 0) history_scroll_width = 3;
	history_scroll_width += 1;
	gtk_tree_view_append_column(GTK_TREE_VIEW(historyview), history_column);
	g_signal_connect_after(gtk_builder_get_object(main_builder, "historyscrolled"), "size-allocate", G_CALLBACK(on_history_resize), NULL);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_historyview_selection_changed), NULL);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(historyview), history_row_separator_func, NULL, NULL);

	completion_view = GTK_WIDGET(gtk_builder_get_object(main_builder, "completionview"));
	gtk_style_context_add_provider(gtk_widget_get_style_context(completion_view), GTK_STYLE_PROVIDER(expression_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20

	// Fix for breeze-gtk and Ubuntu theme
	if(themestr.substr(0, 7) != "Adwaita" && themestr.substr(0, 6) != "ooxmox" && themestr != "Yaru") {
		GtkCssProvider *historyview_provider = gtk_css_provider_new();
		gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(historyview), GTK_TREE_VIEW_GRID_LINES_NONE);
		gtk_style_context_add_provider(gtk_widget_get_style_context(historyview), GTK_STYLE_PROVIDER(historyview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		GtkCssProvider *expression_provider2 = gtk_css_provider_new();
		gtk_style_context_add_provider(gtk_widget_get_style_context(completion_view), GTK_STYLE_PROVIDER(expression_provider2), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		if(themestr == "Breeze") {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px; color: #cecece;}", -1, NULL);
			gtk_css_provider_load_from_data(expression_provider2, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px; color: #cecece;}", -1, NULL);
		} else if(themestr == "Breeze-Dark") {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px; color: #313131;}", -1, NULL);
			gtk_css_provider_load_from_data(expression_provider2, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px; color: #313131;}", -1, NULL);
		} else {
			gtk_css_provider_load_from_data(historyview_provider, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
			gtk_css_provider_load_from_data(expression_provider2, "treeview.view {-GtkTreeView-horizontal-separator: 0;}\ntreeview.view.separator {min-height: 2px;}", -1, NULL);
		}
	}

#endif

	if(theme_name) g_free(theme_name);

	stackstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(stackview), GTK_TREE_MODEL(stackstore));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT(renderer), "xalign", 0.5, NULL);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Index"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(stackview), column);
	register_renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(register_renderer), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, "xalign", 1.0, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
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
	completion_store = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_INT, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_UINT, G_TYPE_INT);
	completion_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(completion_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(completion_filter), 3);
	completion_sort = gtk_tree_model_sort_new_with_model(completion_filter);
	gtk_tree_view_set_model(GTK_TREE_VIEW(completion_view), completion_sort);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(completion_view), completion_row_separator_func, NULL, NULL);

	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	renderer = gtk_cell_renderer_text_new();
	GtkCellArea *area = gtk_cell_area_box_new();
	gtk_cell_area_box_set_spacing(GTK_CELL_AREA_BOX(area), 12);
	column = gtk_tree_view_column_new_with_area(area);
	gtk_cell_area_box_pack_start(GTK_CELL_AREA_BOX(area), renderer, TRUE, TRUE, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "markup", 0, "weight", 6, NULL);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_padding(renderer, 2, 0);
	gtk_cell_area_box_pack_end(GTK_CELL_AREA_BOX(area), renderer, FALSE, TRUE, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "pixbuf", 5, NULL);
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

	tUnitSelector_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN);
	tUnitSelector_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnitSelector_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tUnitSelector_store_filter), 3);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitSelector_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitSelector_store), 0, GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitSelector), GTK_TREE_MODEL(tUnitSelector_store_filter));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_padding(renderer, 4, 0);
	flag_column = gtk_tree_view_column_new_with_attributes(_("Flag"), renderer, "pixbuf", 2, NULL);
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

	set_result_size_request();
	set_expression_size_request();
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "expression_button")), FALSE);

	if(win_height <= 0) gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), NULL, &win_height);
	if(minimal_mode && minimal_width > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), minimal_width, win_height);
	else if(win_width > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_width, win_height);

	if(remember_position) gtk_window_move(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), win_x, win_y);

	gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "main_window")));

	if(history_height > 0) gtk_widget_set_size_request(tabs, -1, -1);

	update_status_text();

}

GtkWidget* get_functions_dialog(void) {

	if(!functions_builder) {

		functions_builder =  getBuilder("functions.ui");
		g_assert(functions_builder != NULL);

		g_assert (gtk_builder_get_object(functions_builder, "functions_dialog") != NULL);

		tFunctionCategories = GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_treeview_category"));
		tFunctions	= GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_treeview_function"));

		tFunctions_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
		tFunctions_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tFunctions_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tFunctions_store_filter), 2);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tFunctions), GTK_TREE_MODEL(tFunctions_store_filter));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Function"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctions), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tFunctions_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tFunctions_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tFunctions_store), 0, GTK_SORT_ASCENDING);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tFunctions), FALSE);

		tFunctionCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tFunctionCategories), GTK_TREE_MODEL(tFunctionCategories_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionCategories), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tFunctionCategories_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tFunctionCategories_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tFunctionCategories_store), 0, GTK_SORT_ASCENDING);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functions_builder, "functions_textview_description")));
		gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);

		if(functions_width > 0 && functions_height > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")), functions_width, functions_height);
		if(functions_hposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_hpaned")), functions_hposition);
		if(functions_vposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_vpaned")), functions_vposition);

		gtk_builder_connect_signals(functions_builder, NULL);

		update_functions_tree();

	}

	return GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog"));
}

GtkWidget* get_variables_dialog(void) {
	if(!variables_builder) {

		variables_builder = getBuilder("variables.ui");
		g_assert(variables_builder != NULL);

		g_assert (gtk_builder_get_object(variables_builder, "variables_dialog") != NULL);

		tVariableCategories = GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_treeview_category"));
		tVariables = GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_treeview_variable"));

		tVariables_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
		tVariables_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tVariables_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tVariables_store_filter), 3);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tVariables), GTK_TREE_MODEL(tVariables_store_filter));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Variable"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tVariables), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Value"), renderer, "text", 1, NULL);
		//g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_NONE, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tVariables), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tVariables_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tVariables_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tVariables_store), 1, int_string_sort_func, GINT_TO_POINTER(1), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tVariables_store), 0, GTK_SORT_ASCENDING);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tVariables), FALSE);

		tVariableCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tVariableCategories), GTK_TREE_MODEL(tVariableCategories_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tVariableCategories), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tVariableCategories_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tVariableCategories_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tVariableCategories_store), 0, GTK_SORT_ASCENDING);

		if(variables_width > 0 && variables_height > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")), variables_width, variables_height);
		if(variables_position > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(variables_builder, "variables_hpaned")), variables_position);

		gtk_builder_connect_signals(variables_builder, NULL);

		update_variables_tree();

	}

	return GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog"));
}

GtkWidget* get_units_dialog(void) {

	if(!units_builder) {

		units_builder = getBuilder("units.ui");
		g_assert(units_builder != NULL);

		g_assert (gtk_builder_get_object(units_builder, "units_dialog") != NULL);

		tUnitCategories = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_treeview_category"));
		tUnits = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_treeview_unit"));

		tUnits_store = gtk_list_store_new(UNITS_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		tUnits_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnits_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tUnits_store_filter), UNITS_VISIBLE_COLUMN);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tUnits), GTK_TREE_MODEL(tUnits_store_filter));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
		gtk_cell_renderer_set_padding(renderer, 4, 0);
		units_flag_column = gtk_tree_view_column_new_with_attributes(_("Flag"), renderer, "pixbuf", UNITS_FLAG_COLUMN, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnits), units_flag_column);
		renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", UNITS_TITLE_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, UNITS_TITLE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnits), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Unit"), renderer, "text", UNITS_NAMES_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, UNITS_NAMES_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnits), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Unit"), renderer, "text", UNITS_BASE_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, UNITS_BASE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnits), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnits_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnits_store), UNITS_TITLE_COLUMN, string_sort_func, GINT_TO_POINTER(UNITS_TITLE_COLUMN), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnits_store), UNITS_NAMES_COLUMN, string_sort_func, GINT_TO_POINTER(UNITS_NAMES_COLUMN), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnits_store), UNITS_BASE_COLUMN, string_sort_func, GINT_TO_POINTER(UNITS_BASE_COLUMN), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnits_store), UNITS_TITLE_COLUMN, GTK_SORT_ASCENDING);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tUnits), FALSE);

		tUnitCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitCategories), GTK_TREE_MODEL(tUnitCategories_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitCategories), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnitCategories_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitCategories_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitCategories_store), 0, GTK_SORT_ASCENDING);

		units_convert_window = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_window"));
		units_convert_scrolled = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_scrolled"));
		units_convert_view = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_view"));
		units_convert_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnits_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(units_convert_filter), UNITS_VISIBLE_COLUMN_CONVERT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(units_convert_view), GTK_TREE_MODEL(units_convert_filter));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		units_convert_flag_renderer = gtk_cell_renderer_pixbuf_new();
		GtkCellArea *area = gtk_cell_area_box_new();
		gtk_cell_area_box_set_spacing(GTK_CELL_AREA_BOX(area), 12);
		gtk_cell_area_box_pack_start(GTK_CELL_AREA_BOX(area), units_convert_flag_renderer, FALSE, TRUE, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), units_convert_flag_renderer, "pixbuf", UNITS_FLAG_COLUMN, NULL);
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_area_box_pack_start(GTK_CELL_AREA_BOX(area), renderer, TRUE, TRUE, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "text", UNITS_TITLE_COLUMN, NULL);
		column = gtk_tree_view_column_new_with_area(area);
		gtk_tree_view_column_set_sort_column_id(column, UNITS_TITLE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(units_convert_view), column);

		if(units_width > 0 && units_height > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")), units_width, units_height);
		if(units_position > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(units_builder, "units_hpaned")), units_position);

		gtk_builder_connect_signals(units_builder, NULL);

		update_units_tree();

		gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(units_builder, "units_entry_from_val")), "1");
		gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(units_builder, "units_entry_to_val")), "1");
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")), 1.0);

	}

	return GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog"));
}

GtkWidget* get_datasets_dialog(void) {

	if(!datasets_builder) {

		datasets_builder = getBuilder("datasets.ui");
		g_assert(datasets_builder != NULL);

		g_assert (gtk_builder_get_object(datasets_builder, "datasets_dialog") != NULL);

		tDatasets = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_treeview_datasets"));
		tDataObjects = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_treeview_objects"));

		tDataObjects_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tDataObjects), GTK_TREE_MODEL(tDataObjects_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Key 1", renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataObjects), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Key 2", renderer, "text", 1, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataObjects), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Key 3", renderer, "text", 2, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataObjects), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tDataObjects_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tDataObjects_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tDataObjects_store), 0, GTK_SORT_ASCENDING);

		tDatasets_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tDatasets), GTK_TREE_MODEL(tDatasets_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Data Set"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDatasets), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tDatasets_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tDatasets_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tDatasets_store), 0, GTK_SORT_ASCENDING);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasets_builder, "datasets_textview_description")));
		gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);

		if(datasets_width > 0 && datasets_height > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(datasets_builder, "datasets_dialog")), datasets_width, datasets_height);
		if(datasets_hposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_hpaned")), datasets_hposition);
		if(datasets_vposition1 > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned1")), datasets_vposition1);
		if(datasets_vposition2 > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned2")), datasets_vposition2);
		
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox1")), 6);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox2")), 6);
#else
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox1")), 6);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox2")), 6);
#endif


		gtk_builder_connect_signals(datasets_builder, NULL);

		update_datasets_tree();

	}

	return GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog"));
}

GtkWidget* get_preferences_dialog(void) {
	if(!preferences_builder) {

		preferences_builder = getBuilder("preferences.ui");
		g_assert(preferences_builder != NULL);

		g_assert (gtk_builder_get_object(preferences_builder, "preferences_dialog") != NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_display_expression_status")), display_expression_status);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_expression_lines_spin_button")), (double) (expression_lines < 1 ? 3 : expression_lines));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_fetch_exchange_rates")), fetch_exchange_rates_at_startup);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_local_currency_conversion")), evalops.local_currency_conversion);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_save_mode")), save_mode_on_exit);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_clear_history")), clear_history_on_exit);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_allow_multiple_instances")), allow_multiple_instances > 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_ignore_locale")), ignore_locale);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_check_version")), check_version);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_remember_position")), remember_position);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_persistent_keypad")), persistent_keypad);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(preferences_builder, "preferences_combo_title")), title_type);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_unicode_signs")), printops.use_unicode_signs);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_copy_separator")), copy_separator);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_numbers")), printops.lower_case_numbers);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_e_notation")), use_e_notation);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_lower_case_e")), printops.lower_case_e);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_imaginary_j")), CALCULATOR->v_i->hasName("j") > 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_alternative_base_prefixes")), printops.base_display == BASE_DISPLAY_ALTERNATIVE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_twos_complement")), printops.twos_complement);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_hexadecimal_twos_complement")), printops.hexadecimal_twos_complement);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_spell_out_logical_operators")), printops.spell_out_logical_operators);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_caret_as_xor")), caret_as_xor);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_binary_prefixes")), CALCULATOR->usesBinaryPrefixes() > 0);
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_custom_app_font")), use_custom_app_font);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), use_custom_result_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_result_font")), custom_result_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), use_custom_expression_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_expression_font")), custom_expression_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), use_custom_status_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_status_font")), custom_status_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), use_custom_keypad_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_keypad_font")), custom_keypad_font.c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), use_custom_app_font);
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(gtk_builder_get_object(preferences_builder, "preferences_button_app_font")), custom_app_font.c_str());
		GdkRGBA c;
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
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
#	ifdef _WIN32
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dark_theme")), use_dark_theme > 0);
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dark_theme")));
#	endif
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

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_plot_time_spin_button")), (double) max_plot_time);

	return GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_dialog"));
}

GtkWidget* get_unit_edit_dialog(void) {

	if(!unitedit_builder) {

		unitedit_builder = getBuilder("unitedit.ui");
		g_assert(unitedit_builder != NULL);

		g_assert (gtk_builder_get_object(unitedit_builder, "unit_edit_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), 0);
		
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
		gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_name")), 12);
		gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_type")), 12);
		gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_alias")), 12);
		gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_mix")), 12);
#else
		gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_name")), 12);
		gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_type")), 12);
		gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_alias")), 12);
		gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_vbox_mix")), 12);
#endif


		gtk_builder_connect_signals(unitedit_builder, NULL);

	}

	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(!CALCULATOR->units[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->units[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->units[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->units[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	return GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog"));
}

GtkWidget* get_function_edit_dialog(void) {

	if(!functionedit_builder) {

		functionedit_builder = getBuilder("functionedit.ui");
		g_assert(functionedit_builder != NULL);

		g_assert (gtk_builder_get_object(functionedit_builder, "function_edit_dialog") != NULL);

		tFunctionArguments = GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_treeview_arguments"));
		tFunctionArguments_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tFunctionArguments), GTK_TREE_MODEL(tFunctionArguments_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionArguments), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionArguments), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tFunctionArguments_selection_changed), NULL);

		tSubfunctions = GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_treeview_subfunctions"));
		tSubfunctions_store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tSubfunctions), GTK_TREE_MODEL(tSubfunctions_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tSubfunctions));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Reference"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tSubfunctions), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Expression"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tSubfunctions), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Precalculate"), renderer, "text", 2, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tSubfunctions), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tSubfunctions_selection_changed), NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(functionedit_builder, "function_edit_combobox_argument_type")), 0);

		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_description"))), "changed", G_CALLBACK(on_function_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression"))), "changed", G_CALLBACK(on_function_changed), NULL);

		gtk_builder_connect_signals(functionedit_builder, NULL);

	}

	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(!CALCULATOR->functions[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->functions[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->functions[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->functions[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	return GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_dialog"));
}

GtkWidget* get_simple_function_edit_dialog(void) {

	if(!simplefunctionedit_builder) {

		simplefunctionedit_builder = getBuilder("simplefunctionedit.ui");
		g_assert(simplefunctionedit_builder != NULL);

		g_assert(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_dialog") != NULL);

		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_textview_expression"))), "changed", G_CALLBACK(on_simple_function_changed), NULL);

		gtk_builder_connect_signals(simplefunctionedit_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(simplefunctionedit_builder, "simple_function_edit_dialog"));
}

GtkWidget* get_variable_edit_dialog(void) {

	if(!variableedit_builder) {

		variableedit_builder = getBuilder("variableedit.ui");
		g_assert(variableedit_builder != NULL);

		g_assert (gtk_builder_get_object(variableedit_builder, "variable_edit_dialog") != NULL);

		gtk_builder_connect_signals(variableedit_builder, NULL);

	}
	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->variables[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->variables[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->variables[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	return GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog"));
}

GtkWidget* get_unknown_edit_dialog(void) {

	if(!unknownedit_builder) {

		unknownedit_builder = getBuilder("unknownedit.ui");
		g_assert(unknownedit_builder != NULL);

		g_assert (gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), 0);

		gtk_builder_connect_signals(unknownedit_builder, NULL);

	}

	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->variables[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->variables[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->variables[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	return GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog"));
}

GtkWidget* get_matrix_edit_dialog(void) {
	if(!matrixedit_builder) {

		matrixedit_builder = getBuilder("matrixedit.ui");
		g_assert(matrixedit_builder != NULL);

		g_assert (gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog") != NULL);

		GType types[200];
		for(gint i = 0; i < 200; i += 1) {
			types[i] = G_TYPE_STRING;
		}
		tMatrixEdit_store = gtk_list_store_newv(200, types);
		tMatrixEdit = GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_view"));
		gtk_tree_view_set_model (GTK_TREE_VIEW(tMatrixEdit), GTK_TREE_MODEL(tMatrixEdit_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tMatrixEdit));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")))), _("Matrices"));

		gtk_builder_connect_signals(matrixedit_builder, NULL);

	}

	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->variables[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->variables[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->variables[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);


	return GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog"));
}
GtkWidget* get_matrix_dialog(void) {
	if(!matrix_builder) {

		matrix_builder = getBuilder("matrix.ui");
		g_assert(matrix_builder != NULL);

		g_assert (gtk_builder_get_object(matrix_builder, "matrix_dialog") != NULL);

		GType types[10000];
		for(gint i = 0; i < 10000; i++) {
			types[i] = G_TYPE_STRING;
		}
		tMatrix_store = gtk_list_store_newv(10000, types);
		tMatrix = GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_view"));
		gtk_tree_view_set_model (GTK_TREE_VIEW(tMatrix), GTK_TREE_MODEL(tMatrix_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tMatrix));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

		gtk_builder_connect_signals(matrix_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_dialog"));

}

GtkWidget* get_dataobject_edit_dialog(void) {

	return GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_dialog"));
}

GtkWidget* get_dataset_edit_dialog(void) {

	if(!datasetedit_builder) {

		datasetedit_builder = getBuilder("datasetedit.ui");
		g_assert(datasetedit_builder != NULL);

		g_assert (gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog") != NULL);

		tDataProperties = GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_treeview_properties"));
		tDataProperties_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tDataProperties), GTK_TREE_MODEL(tDataProperties_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataProperties));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataProperties), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataProperties), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 2, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataProperties), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tDataProperties_selection_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_description"))), "changed", G_CALLBACK(on_dataset_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_copyright"))), "changed", G_CALLBACK(on_dataset_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_textview_description"))), "changed", G_CALLBACK(on_dataproperty_changed), NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 0);

		gtk_builder_connect_signals(datasetedit_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog"));
}

GtkWidget* get_dataproperty_edit_dialog(void) {
	return GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_dialog"));
}


GtkWidget* get_names_edit_dialog(void) {
	if(!namesedit_builder) {

		namesedit_builder = getBuilder("namesedit.ui");
		g_assert(namesedit_builder != NULL);

		g_assert (gtk_builder_get_object(namesedit_builder, "names_edit_dialog") != NULL);

		tNames = GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_treeview"));

		tNames_store = gtk_list_store_new(NAMES_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tNames), GTK_TREE_MODEL(tNames_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", NAMES_NAME_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_NAME_COLUMN);
		gtk_tree_view_column_set_expand(column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Abbreviation"), renderer, "text", NAMES_ABBREVIATION_STRING_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_ABBREVIATION_STRING_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Plural"), renderer, "text", NAMES_PLURAL_STRING_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_PLURAL_STRING_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Reference"), renderer, "text", NAMES_REFERENCE_STRING_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_REFERENCE_STRING_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tNames_selection_changed), NULL);

		gtk_builder_connect_signals(namesedit_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog"));
}

GtkWidget* get_csv_import_dialog(void) {

	if(!csvimport_builder) {

		csvimport_builder = getBuilder("csvimport.ui");
		g_assert(csvimport_builder != NULL);

		g_assert (gtk_builder_get_object(csvimport_builder, "csv_import_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(csvimport_builder, "csv_import_combobox_delimiter")), 0);

		gtk_builder_connect_signals(csvimport_builder, NULL);

	}
	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->variables[i]->category().c_str()) == NULL) {
				items = g_list_append(items, (gpointer) CALCULATOR->variables[i]->category().c_str());
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->variables[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(csvimport_builder, "csv_import_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	return GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_dialog"));
}

GtkWidget* get_csv_export_dialog(void) {

	if(!csvexport_builder) {

		csvexport_builder = getBuilder("csvexport.ui");
		g_assert(csvexport_builder != NULL);

		g_assert (gtk_builder_get_object(csvexport_builder, "csv_export_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(csvexport_builder, "csv_export_combobox_delimiter")), 0);

		gtk_builder_connect_signals(csvexport_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_dialog"));

}
extern string prev_output_base, prev_input_base;
GtkWidget* get_set_base_dialog(void) {
	if(!setbase_builder) {

		setbase_builder = getBuilder("setbase.ui");
		g_assert(setbase_builder != NULL);

		g_assert (gtk_builder_get_object(setbase_builder, "set_base_dialog") != NULL);

		PrintOptions po = printops;
		po.number_fraction_format = FRACTION_DECIMAL_EXACT;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		po.preserve_precision = true;
		po.base = 10;
		if(printops.base >= BASE_CUSTOM && !CALCULATOR->customOutputBase().isZero()) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), CALCULATOR->customOutputBase().print(po).c_str());
		if(evalops.parse_options.base >= BASE_CUSTOM && !CALCULATOR->customInputBase().isZero()) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), CALCULATOR->customInputBase().print(po).c_str());
		switch(evalops.parse_options.base) {
			case BASE_BINARY: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_binary")), TRUE);
				break;
			}
			case BASE_OCTAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_octal")), TRUE);
				break;
			}
			case BASE_DECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_decimal")), TRUE);
				break;
			}
			case 12: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_duodecimal")), TRUE);
				break;
			}
			case BASE_HEXADECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_hexadecimal")), TRUE);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_roman")), TRUE);
				break;
			}
			case BASE_UNICODE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "Unicode");
				break;
			}
			case BASE_E: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "e");
				break;
			}
			case BASE_PI: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "π");
				break;
			}
			case BASE_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "φ");
				break;
			}
			case BASE_SUPER_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "ψ");
				break;
			}
			case BASE_SQRT2: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), "√2");
				break;
			}
			case BASE_BIJECTIVE_26: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), _("Bijective base-26"));
				break;
			}
			case BASE_CUSTOM: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_input_other")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_input_other")), i2s(evalops.parse_options.base).c_str());
			}
		}
		switch(printops.base) {
			case BASE_BINARY: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_binary")), TRUE);
				break;
			}
			case BASE_OCTAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_octal")), TRUE);
				break;
			}
			case BASE_DECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_decimal")), TRUE);
				break;
			}
			case 12: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_duodecimal")), TRUE);
				break;
			}
			case BASE_HEXADECIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_hexadecimal")), TRUE);
				break;
			}
			case BASE_SEXAGESIMAL: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_sexagesimal")), TRUE);
				break;
			}
			case BASE_TIME: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_time")), TRUE);
				break;
			}
			case BASE_ROMAN_NUMERALS: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_roman")), TRUE);
				break;
			}
			case BASE_UNICODE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "Unicode");
				break;
			}
			case BASE_E: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "e");
				break;
			}
			case BASE_PI: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "π");
				break;
			}
			case BASE_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "φ");
				break;
			}
			case BASE_SUPER_GOLDEN_RATIO: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "ψ");
				break;
			}
			case BASE_SQRT2: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "sqrt(2)");
				break;
			}
			case BASE_BIJECTIVE_26: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), _("Bijective base-26"));
				break;
			}
			case BASE_FP16: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp16");
				break;
			}
			case BASE_FP32: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "float");
				break;
			}
			case BASE_FP64: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "double");
				break;
			}
			case BASE_FP80: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp80");
				break;
			}
			case BASE_FP128: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), "fp128");
				break;
			}
			case BASE_CUSTOM: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(setbase_builder, "set_base_radiobutton_output_other")), TRUE);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(setbase_builder, "set_base_entry_output_other")), i2s(printops.base).c_str());
			}
		}
		
		SET_FOCUS_ON_CLICK(gtk_builder_get_object(setbase_builder, "button_close"));

		gtk_builder_connect_signals(setbase_builder, NULL);

	}
	prev_output_base = ""; prev_input_base = "";
	return GTK_WIDGET(gtk_builder_get_object(setbase_builder, "set_base_dialog"));
}

GtkWidget* get_nbases_dialog(void) {
	if(!nbases_builder) {

		nbases_builder = getBuilder("nbases.ui");
		g_assert(nbases_builder != NULL);

		g_assert (gtk_builder_get_object(nbases_builder, "nbases_dialog") != NULL);

		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_binary")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_octal")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_decimal")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_hexadecimal")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_duo")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(nbases_builder, "nbases_entry_roman")), 1.0);

		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MINUS, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_sub"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_sub")), SIGN_MINUS);
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_sub")), MINUS);
		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_MULTIPLICATION, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_times"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_times")), SIGN_MULTIPLICATION);
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_times")), MULTIPLICATION);
		if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION_SLASH, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_divide")), SIGN_DIVISION_SLASH);
		else if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_DIVISION, (void*) gtk_builder_get_object(nbases_builder, "nbases_label_divide"))) gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_divide")), SIGN_DIVISION);
		else gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(nbases_builder, "nbases_label_divide")), DIVISION);

		gchar *theme_name = NULL;
		g_object_get(gtk_settings_get_default(), "gtk-theme-name", &theme_name, NULL);
		string themestr;
		if(theme_name) {
			themestr = theme_name;
			g_free(theme_name);
		}

		if(themestr.substr(0, 7) == "Adwaita" || themestr.substr(0, 6) == "ooxmox" || themestr == "Breeze" || themestr == "Breeze-Dark" || themestr == "Yaru") {
			GtkCssProvider *link_style_top = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_top, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0}", -1, NULL);
			GtkCssProvider *link_style_bot = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bot, "* {border-top-left-radius: 0; border-top-right-radius: 0}", -1, NULL);
			GtkCssProvider *link_style_tl = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_tl, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0; border-top-right-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_tr = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_tr, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0; border-top-left-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_bl = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bl, "* {border-top-left-radius: 0; border-top-right-radius: 0; border-bottom-right-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_br = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_br, "* {border-top-left-radius: 0; border-top-right-radius: 0; border-bottom-left-radius: 0;}", -1, NULL);
			GtkCssProvider *link_style_mid = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_mid, "* {border-radius: 0;}", -1, NULL);

			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_zero"))), GTK_STYLE_PROVIDER(link_style_bl), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_one"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_two"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_three"))), GTK_STYLE_PROVIDER(link_style_br), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_four"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_five"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_six"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_seven"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_eight"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_nine"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_a"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_b"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_c"))), GTK_STYLE_PROVIDER(link_style_tl), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_d"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_e"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_f"))), GTK_STYLE_PROVIDER(link_style_tr), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_and"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_or"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_xor"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_not"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_divide"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_times"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_sub"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_add"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_left_shift"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_right_shift"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_del"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_button_ac"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}

		GdkRGBA c;
		gtk_style_context_get_color(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_label_decimal"))), GTK_STATE_FLAG_NORMAL, &c);
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
		nbases_error_color = ecs;

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
		nbases_warning_color = wcs;

		gtk_builder_connect_signals(nbases_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(nbases_builder, "nbases_dialog"));
}
GtkWidget* get_percentage_dialog(void) {
	if(!percentage_builder) {

		percentage_builder = getBuilder("percentage.ui");
		g_assert(percentage_builder != NULL);

		g_assert (gtk_builder_get_object(percentage_builder, "percentage_dialog") != NULL);

		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_1")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_2")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_3")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_4")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_5")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_6")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(percentage_builder, "percentage_entry_7")), 1.0);

		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
		gtk_text_view_set_right_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
		gtk_text_view_set_top_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
		gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
#else
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(gtk_builder_get_object(percentage_builder, "percentage_description")), 12);
#endif

		gtk_builder_connect_signals(percentage_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(percentage_builder, "percentage_dialog"));
}

unordered_map<size_t, GtkWidget*> cal_year, cal_month, cal_day, cal_label;
GtkWidget *chinese_stem, *chinese_branch;

GtkWidget* get_calendarconversion_dialog(void) {
	if(!calendarconversion_builder) {

		calendarconversion_builder = getBuilder("calendarconversion.ui");
		g_assert(calendarconversion_builder != NULL);

		g_assert(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog") != NULL);

		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_1")), _("Gregorian"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_8")), _("Revised Julian (Milanković)"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_7")), _("Julian"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_3")), _("Islamic (Hijri)"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_2")), _("Hebrew"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_6")), _("Chinese"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_4")), _("Persian (Solar Hijri)"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_9")), _("Coptic"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_10")), _("Ethiopian"));
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(calendarconversion_builder, "label_5")), _("Indian (National)"));

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_1")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_2")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_3")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_4")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_5")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_6")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_7")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_8")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_9")), 12);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_10")), 12);
#else
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_1")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_2")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_3")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_4")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_5")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_6")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_7")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_8")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_9")), 12);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_10")), 12);
#endif

		cal_year[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_1"));
		cal_month[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_1"));
		cal_day[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_1"));
		cal_label[CALENDAR_GREGORIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_1"));
		cal_year[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_2"));
		cal_month[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_2"));
		cal_day[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_2"));
		cal_label[CALENDAR_HEBREW] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_2"));
		cal_year[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_3"));
		cal_month[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_3"));
		cal_day[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_3"));
		cal_label[CALENDAR_ISLAMIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_3"));
		cal_year[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_4"));
		cal_month[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_4"));
		cal_day[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_4"));
		cal_label[CALENDAR_PERSIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_4"));
		cal_year[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_5"));
		cal_month[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_5"));
		cal_day[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_5"));
		cal_label[CALENDAR_INDIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_5"));
		cal_year[CALENDAR_CHINESE] = NULL;
		chinese_stem = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "stem_6"));
		chinese_branch = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "branch_6"));
		cal_month[CALENDAR_CHINESE] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_6"));
		cal_day[CALENDAR_CHINESE] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_6"));
		cal_label[CALENDAR_CHINESE] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_6"));
		cal_year[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_7"));
		cal_month[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_7"));
		cal_day[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_7"));
		cal_label[CALENDAR_JULIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_7"));
		cal_year[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_8"));
		cal_month[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_8"));
		cal_day[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_8"));
		cal_label[CALENDAR_MILANKOVIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_8"));
		cal_year[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_9"));
		cal_month[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_9"));
		cal_day[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_9"));
		cal_label[CALENDAR_COPTIC] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_9"));
		cal_year[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "year_10"));
		cal_month[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "month_10"));
		cal_day[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "day_10"));
		cal_label[CALENDAR_ETHIOPIAN] = GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "label_10"));

		for(size_t i = 0; i < NUMBER_OF_CALENDARS; i++) {
			if(cal_day.count(i) > 0) {
				if(i == CALENDAR_CHINESE) {
					for(size_t i2 = 1; i2 <= 5; i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(chinese_stem), chineseStemName(i2 * 2).c_str());
					for(size_t i2 = 1; i2 <= 12; i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(chinese_branch), chineseBranchName(i2).c_str());
				} else {
					gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal_year[i]), G_MININT, G_MAXINT);
					gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(cal_year[i]), TRUE);
					gtk_spin_button_set_increments(GTK_SPIN_BUTTON(cal_year[i]), 1.0, 10.0);
					gtk_spin_button_set_digits(GTK_SPIN_BUTTON(cal_year[i]), 0);
					gtk_entry_set_alignment(GTK_ENTRY(cal_year[i]), 1.0);
				}
				for(size_t i2 = 1; i2 <= (size_t) numberOfMonths((CalendarSystem) i); i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cal_month[i]), monthName(i2, (CalendarSystem) i, true).c_str());
				for(size_t i2 = 1; i2 <= 31; i2++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cal_day[i]), i2s(i2).c_str());
			}
		}

		QalculateDateTime date;
		date.setToCurrentDate();
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal_year[CALENDAR_GREGORIAN]), date.year());
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_month[CALENDAR_GREGORIAN]), date.month() - 1);

		for(size_t i = 0; i < NUMBER_OF_CALENDARS; i++) {
			if(cal_day.count(i) > 0) {
				if(i == CALENDAR_CHINESE) {
					g_signal_connect(chinese_stem, "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
					g_signal_connect(chinese_branch, "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
				} else {
					g_signal_connect(cal_year[i], "value-changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
				}
				g_signal_connect(cal_month[i], "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
				g_signal_connect(cal_day[i], "changed", G_CALLBACK(calendar_changed), GINT_TO_POINTER((gint) i));
			}
		}

		gtk_builder_connect_signals(calendarconversion_builder, NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(cal_day[CALENDAR_GREGORIAN]), date.day() - 1);

	}

	return GTK_WIDGET(gtk_builder_get_object(calendarconversion_builder, "calendar_dialog"));
}

GtkWidget *create_InfoWidget(const gchar *text) {

	GtkWidget *hbox, *image, *infolabel;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_show(hbox);
	gtk_widget_set_halign(hbox, GTK_ALIGN_START);
	//gtk_widget_set_valign(hbox, GTK_ALIGN_CENTER);

	image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image);
	gtk_box_pack_start (GTK_BOX(hbox), image, FALSE, TRUE, 0);

	infolabel = gtk_label_new(text);
	gtk_widget_show(infolabel);
	gtk_box_pack_start(GTK_BOX(hbox), infolabel, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(infolabel), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(infolabel), TRUE);

	return hbox;
}

GtkWidget* get_argument_rules_dialog(void) {

	if(!argumentrules_builder) {

		argumentrules_builder = getBuilder("argumentrules.ui");
		g_assert(argumentrules_builder != NULL);

		g_assert (gtk_builder_get_object(argumentrules_builder, "argument_rules_dialog") != NULL);

		gtk_builder_connect_signals(argumentrules_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_dialog"));
}
GtkWidget* get_decimals_dialog(void) {
	if(!decimals_builder) {

		decimals_builder = getBuilder("decimals.ui");
		g_assert(decimals_builder != NULL);

		g_assert (gtk_builder_get_object(decimals_builder, "decimals_dialog") != NULL);

		gtk_builder_connect_signals(decimals_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog"));
}
GtkWidget* get_plot_dialog(void) {
	if(!plot_builder) {

		if(!CALCULATOR->canPlot()) return NULL;

		plot_builder = getBuilder("plot.ui");
		g_assert(plot_builder != NULL);

		g_assert (gtk_builder_get_object(plot_builder, "plot_dialog") != NULL);

		tPlotFunctions = GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_treeview_data"));
		tPlotFunctions_store = gtk_list_store_new(10, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tPlotFunctions), GTK_TREE_MODEL(tPlotFunctions_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tPlotFunctions), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Expression"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tPlotFunctions), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tPlotFunctions_selection_changed), NULL);

		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), false);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), 0);

		gtk_builder_connect_signals(plot_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog"));
}
GtkWidget* get_precision_dialog(void) {
	if(!precision_builder) {

		precision_builder = getBuilder("precision.ui");
		g_assert(precision_builder != NULL);

		g_assert (gtk_builder_get_object(precision_builder, "precision_dialog") != NULL);

		gtk_builder_connect_signals(precision_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(precision_builder, "precision_dialog"));
}
GtkWidget* get_periodic_dialog(void) {
	if(!periodictable_builder) {

		periodictable_builder = getBuilder("periodictable.ui");
		g_assert(periodictable_builder != NULL);

		g_assert (gtk_builder_get_object(periodictable_builder, "periodic_dialog") != NULL);

		gtk_builder_connect_signals(periodictable_builder, NULL);

		DataSet *dc = CALCULATOR->getDataSet("atom");
		if(!dc) {
			return GTK_WIDGET(gtk_builder_get_object(periodictable_builder, "periodic_dialog"));
		}

		DataObject *e;
		GtkWidget *e_button;
		GtkGrid *e_table = GTK_GRID(gtk_builder_get_object(periodictable_builder, "periodic_table"));
		string tip;
		GtkCssProvider *e_style[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		GtkCssProvider *l_style = NULL;
		DataProperty *p_xpos = dc->getProperty("x_pos");
		DataProperty *p_ypos = dc->getProperty("y_pos");
		DataProperty *p_weight = dc->getProperty("weight");
		DataProperty *p_number = dc->getProperty("number");
		DataProperty *p_symbol = dc->getProperty("symbol");
		DataProperty *p_class = dc->getProperty("class");
		DataProperty *p_name = dc->getProperty("name");
		int x_pos = 0, y_pos = 0, group = 0;
		string weight;
		l_style = gtk_css_provider_new();
		for(size_t i3 = 0; i3 < 12; i3++) {
			e_style[i3] = gtk_css_provider_new();
		}
		for(size_t i = 1; i < 120; i++) {
			e = dc->getObject(i2s(i));
			if(e) {
				x_pos = s2i(e->getProperty(p_xpos));
				y_pos = s2i(e->getProperty(p_ypos));
			}
			if(e && x_pos > 0 && x_pos <= 18 && y_pos > 0 && y_pos <= 10) {
				e_button = gtk_button_new();
				gtk_button_set_relief(GTK_BUTTON(e_button), GTK_RELIEF_HALF);
				gtk_container_add(GTK_CONTAINER(e_button), gtk_label_new(e->getProperty(p_symbol).c_str()));
				group = s2i(e->getProperty(p_class));
				if(group > 0 && group <= 11) gtk_style_context_add_provider(gtk_widget_get_style_context(e_button), GTK_STYLE_PROVIDER(e_style[group - 1]), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				else gtk_style_context_add_provider(gtk_widget_get_style_context(e_button), GTK_STYLE_PROVIDER(e_style[11]), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				if(x_pos > 2) gtk_grid_attach(e_table, e_button, x_pos + 1, y_pos, 1, 1);
				else gtk_grid_attach(e_table, e_button, x_pos, y_pos, 1, 1);
				tip = e->getProperty(p_number);
				tip += " ";
				tip += e->getProperty(p_name);
				weight = e->getPropertyDisplayString(p_weight);
				if(!weight.empty() && weight != "-") {
					tip += "\n";
					tip += weight;
				}
				gtk_widget_set_tooltip_text(e_button, tip.c_str());
				gtk_widget_show_all(e_button);
				g_signal_connect((gpointer) e_button, "clicked", G_CALLBACK(on_element_button_clicked), (gpointer) e);
			}
		}
		gtk_css_provider_load_from_data(l_style, "* {color: #000000;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[0], "* {color: #000000; background-image: none; background-color: #eeccee;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[1], "* {color: #000000; background-image: none; background-color: #ddccee;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[2], "* {color: #000000; background-image: none; background-color: #ccddff;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[3], "* {color: #000000; background-image: none; background-color: #ddeeff;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[4], "* {color: #000000; background-image: none; background-color: #cceeee;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[5], "* {color: #000000; background-image: none; background-color: #bbffbb;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[6], "* {color: #000000; background-image: none; background-color: #eeffdd;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[7], "* {color: #000000; background-image: none; background-color: #ffffaa;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[8], "* {color: #000000; background-image: none; background-color: #ffddaa;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[9], "* {color: #000000; background-image: none; background-color: #ffccdd;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[10], "* {color: #000000; background-image: none; background-color: #aaeedd;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[11], "* {color: #000000; background-image: none;}", -1, NULL);
	}

	return GTK_WIDGET(gtk_builder_get_object(periodictable_builder, "periodic_dialog"));
}

GtkWidget* get_shortcuts_dialog(void) {
	if(!shortcuts_builder) {

		shortcuts_builder = getBuilder("shortcuts.ui");
		g_assert(shortcuts_builder != NULL);

		g_assert(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog") != NULL);

		tShortcuts = GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_treeview"));

		tShortcuts_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tShortcuts), GTK_TREE_MODEL(tShortcuts_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcuts));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcuts), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Value"), renderer, "text", 1, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcuts), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Key combination"), renderer, "text", 2, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcuts), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tShortcuts_selection_changed), NULL);

		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tShortcuts_store), 0, GTK_SORT_ASCENDING);

		tShortcutsType = GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_treeview"));

		tShortcutsType_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tShortcutsType), GTK_TREE_MODEL(tShortcutsType_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcutsType));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcutsType), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tShortcutsType_selection_changed), NULL);

		for(int i = 0; i <= SHORTCUT_TYPE_QUIT; i++) {
			GtkTreeIter iter;
			gtk_list_store_append(tShortcutsType_store, &iter);
			gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(i), 1, i, -1);
			if(i == 0) gtk_tree_selection_select_iter(selection, &iter);
		}

		for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
			GtkTreeIter iter;
			gtk_list_store_insert(tShortcuts_store, &iter, 0);
			gtk_list_store_set(tShortcuts_store, &iter, 0, shortcut_type_text(it->second.type), 1, it->second.value.c_str(), 2, shortcut_to_text(it->second.key, it->second.modifier).c_str(), 3, (guint64) it->second.key + (guint64) G_MAXUINT32 * (guint64) it->second.modifier, -1);
		}

		gtk_builder_connect_signals(shortcuts_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog"));
}
GtkWidget* get_floatingpoint_dialog(void) {
	if(!floatingpoint_builder) {

		floatingpoint_builder = getBuilder("floatingpoint.ui");
		g_assert(floatingpoint_builder != NULL);

		g_assert (gtk_builder_get_object(floatingpoint_builder, "floatingpoint_dialog") != NULL);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 18
		gtk_text_view_set_top_margin(GTK_TEXT_VIEW(gtk_builder_get_object(floatingpoint_builder, "fp_textedit_bin")), 6);
		gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(gtk_builder_get_object(floatingpoint_builder, "fp_textedit_bin")), 6);
#else
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(gtk_builder_get_object(floatingpoint_builder, "fp_textedit_bin")), 6);
#endif

		gtk_builder_connect_signals(floatingpoint_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(floatingpoint_builder, "floatingpoint_dialog"));
}

#define SET_BUTTONS_EDIT_ITEM_3(l, t1, t2, t3) \
	{gtk_list_store_set(tButtonsEdit_store, &iter, 1, custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str(), 2, custom_buttons[i].type[0] == -1 ? t1 : (custom_buttons[i].value[0].empty() ? shortcut_type_text(custom_buttons[i].type[0]) : custom_buttons[i].value[0].c_str()), 3, custom_buttons[i].type[1] == -1 ? t2 : (custom_buttons[i].value[1].empty() ? shortcut_type_text(custom_buttons[i].type[1]) : custom_buttons[i].value[1].c_str()), 4, custom_buttons[i].type[2] == -1 ? t3 : (custom_buttons[i].value[2].empty() ? shortcut_type_text(custom_buttons[i].type[2]) : custom_buttons[i].value[2].c_str()), -1);\
	if(index >= 0) break;}
	
#define SET_BUTTONS_EDIT_ITEM_3B(l, t1, t2, t3) SET_BUTTONS_EDIT_ITEM_3(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(main_builder, l))), t1, t2, t3)
#define SET_BUTTONS_EDIT_ITEM_2(l, t1, t2) SET_BUTTONS_EDIT_ITEM_3(l, t1, t2, t2)
#define SET_BUTTONS_EDIT_ITEM_2B(l, t1, t2) SET_BUTTONS_EDIT_ITEM_3(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(main_builder, l))), t1, t2, t2)
#define SET_BUTTONS_EDIT_ITEM_C(l) SET_BUTTONS_EDIT_ITEM_3(l, "-", "-", "-")

void update_custom_buttons_edit(int index, bool update_label_entry) {
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tButtonsEdit_store), &iter);
	int i = 0;
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(tButtonsEdit_store), &iter, 0, &i, -1);
		if(i == 0 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("↑ ↓", _("Cycle through previous expression"), "-")
		else if(i == 1 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("← →", _("Move cursor left or right"), _("Move cursor to beginning or end"))
		else if(i == 2 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("%", CALCULATOR->v_percent->title(true).c_str(), CALCULATOR->v_permille->title(true).c_str())
		else if(i == 3 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("±", _("Uncertainty/interval"), _("Relative error"), _("Interval"))
		else if(i == 4 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_comma", _("Argument separator"), _("Blank space"), _("New line"))
		else if(i == 5 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("(x)", _("Smart parentheses"), _("Vector brackets"))
		else if(i == 6 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("(", _("Left parenthesis"), _("Left vector bracket"))
		else if(i == 7 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2(")", _("Right parenthesis"), _("Right vector bracket"))
		else if(i == 8 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("0", "0", "x^0", CALCULATOR->getDegUnit()->title(true).c_str())
		else if(i == 9 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("1", "1", "x^1", "1/x")
		else if(i == 10 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("2", "2", "x^2", "1/2")
		else if(i == 11 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("3", "3", "x^3", "1/3")
		else if(i == 12 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("4", "4", "x^4", "1/4")
		else if(i == 13 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("5", "5", "x^5", "1/5")
		else if(i == 14 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("6", "6", "x^6", "1/6")
		else if(i == 15 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("7", "7", "x^7", "1/7")
		else if(i == 16 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("8", "8", "x^8", "1/8")
		else if(i == 17 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("9", "9", "x^9", "1/9")
		else if(i == 18 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_dot", _("Decimal point"), _("Blank space"), _("New line"))
		else if(i == 19 && (index == i || index < 0)) {
			MathFunction *f = CALCULATOR->getActiveFunction("exp10");
			SET_BUTTONS_EDIT_ITEM_3B("label_exp", "10^x", CALCULATOR->f_exp->title(true).c_str(), (f ? f->title(true).c_str() : "-"));
		} else if(i == 20 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("x^y", _("Raise"), CALCULATOR->f_sqrt->title(true).c_str())
		else if(i == 21 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_divide", _("Divide"), "1/x")
		else if(i == 22 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_times", _("Multiply"), _("Bitwise Exclusive OR"))
		else if(i == 23 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_add", _("Add"), _("M+ (memory plus)"), _("Bitwise AND"))
		else if(i == 24 && (index == i || index < 0)) {
			MathFunction *f = CALCULATOR->getActiveFunction("neg");
			SET_BUTTONS_EDIT_ITEM_3B("label_sub", _("Subtract"), f ? f->title(true).c_str() : "-", _("Bitwise OR"));
		} else if(i == 25 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_ac", _("Clear"), _("MC (memory clear)"))
		else if(i == 26 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_del", _("Delete"), _("Backspace"), _("M− (memory minus)"))
		else if(i == 27 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_ans", _("Previous result"), _("Previous result (static)"))
		else if(i == 28 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("=", _("Calculate expression"), _("MR (memory recall)"), _("MS (memory store)"))
		else if(i == 29 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C1")
		else if(i == 30 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C2")
		else if(i == 31 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C3")
		else if(i == 32 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C4")
		else if(i == 33 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C5")
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tButtonsEdit_store), &iter));
	on_tButtonsEdit_update_selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit)), update_label_entry);
}
GtkWidget* get_buttons_edit_dialog(void) {
	if(!buttonsedit_builder) {

		buttonsedit_builder = getBuilder("buttonsedit.ui");
		g_assert(buttonsedit_builder != NULL);

		g_assert(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_dialog") != NULL);
		
		tButtonsEdit = GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_treeview"));

		tButtonsEdit_store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tButtonsEdit), GTK_TREE_MODEL(tButtonsEdit_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Label"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Left-click"), renderer, "text", 2, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Right-click"), renderer, "text", 3, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Middle-click"), renderer, "text", 4, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tButtonsEdit_selection_changed), NULL);

		GtkTreeIter iter;
		for(int i = 29; i < 34; i++) {
			gtk_list_store_append(tButtonsEdit_store, &iter);
			gtk_list_store_set(tButtonsEdit_store, &iter, 0, i, -1);
		}
		for(int i = 0; i < 20; i++) {
			gtk_list_store_append(tButtonsEdit_store, &iter);
			gtk_list_store_set(tButtonsEdit_store, &iter, 0, i, -1);
		}
		gtk_tree_selection_unselect_all(selection);

		update_custom_buttons_edit();
		
		tButtonsEditType = GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_type_treeview"));

		tButtonsEditType_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tButtonsEditType), GTK_TREE_MODEL(tButtonsEditType_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEditType));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEditType), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tButtonsEditType_selection_changed), NULL);

		gtk_list_store_append(tButtonsEditType_store, &iter);
		gtk_list_store_set(tButtonsEditType_store, &iter, 0, _("None"), 1, -2, -1);
		for(int i = 0; i <= SHORTCUT_TYPE_QUIT; i++) {
			gtk_list_store_append(tButtonsEditType_store, &iter);
			gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(i), 1, i, -1);
			if(i == 0) gtk_tree_selection_select_iter(selection, &iter);
		}

		gtk_builder_connect_signals(buttonsedit_builder, NULL);

	}

	return GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_dialog"));
}

