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
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "support.h"
#include "settings.h"
#include "util.h"
#include "modes.h"
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
#include "preferencesdialog.h"
#include "decimalsdialog.h"
#include "shortcutsdialog.h"
#include "buttonseditdialog.h"
#include "setbasedialog.h"
#include "matrixdialog.h"
#include "importcsvdialog.h"
#include "exportcsvdialog.h"
#include "uniteditdialog.h"
#include "variableeditdialog.h"
#include "matrixeditdialog.h"
#include "unknowneditdialog.h"
#include "functioneditdialog.h"
#include "dataseteditdialog.h"
#include "expressionedit.h"
#include "expressionstatus.h"
#include "expressioncompletion.h"
#include "insertfunctiondialog.h"
#include "mainwindow.h"
#include "resultview.h"
#include "openhelp.h"
#include "keypad.h"
#include "menubar.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::stack;

extern GtkBuilder *main_builder;

unordered_map<Unit*, GtkWidget*> angle_unit_items;

GtkWidget *f_menu = NULL, *v_menu = NULL, *u_menu = NULL, *u_menu2 = NULL, *recent_menu = NULL;

vector<MathFunction*> recent_functions;
vector<Variable*> recent_variables;
vector<Unit*> recent_units;
vector<GtkWidget*> recent_function_items;
vector<GtkWidget*> recent_variable_items;
vector<GtkWidget*> recent_unit_items;

vector<GtkWidget*> mode_items;
vector<GtkWidget*> popup_result_mode_items;

bool fraction_fixed_combined = true;

bool read_menubar_settings_line(string &svar, string&, int &v) {
	if(svar == "fraction_fixed_combined") {
		fraction_fixed_combined = v;
	} else {
		return false;
	}
	return true;
}
void write_menubar_settings(FILE *file) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")))) fprintf(file, "fraction_fixed_combined=%i\n", false);
}

bool combined_fixed_fraction_set() {
	return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")));
}
void insert_unit_from_menu(GtkMenuItem*, gpointer user_data) {
	insert_unit((Unit*) user_data, true);
}
void insert_variable_from_menu(GtkMenuItem*, gpointer user_data) {
	insert_variable((Variable*) user_data, true);
}
void insert_prefix_from_menu(GtkMenuItem*, gpointer user_data) {
	insert_text(((Prefix*) user_data)->name(printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) expression_edit_widget()).c_str());
}
void insert_function_from_menu(GtkMenuItem*, gpointer user_data) {
	if(!CALCULATOR->stillHasFunction((MathFunction*) user_data)) return;
	insert_function((MathFunction*) user_data, main_window());
}
void convert_to_unit(GtkMenuItem*, gpointer user_data) {
	GtkWidget *edialog;
	Unit *u = (Unit*) user_data;
	if(!u) {
		edialog = gtk_message_dialog_new(main_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Unit does not exist"));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
	}
	convert_result_to_unit(u);
}

void on_menu_item_copy_activate(GtkMenuItem*, gpointer) {
	copy_result(0);
}
void on_menu_item_copy_ascii_activate(GtkMenuItem*, gpointer) {
	copy_result(1);
}
void on_menu_item_precision_activate(GtkMenuItem*, gpointer) {
	open_precision(main_window());
}
void on_menu_item_decimals_activate(GtkMenuItem*, gpointer) {
	open_decimals(main_window());
}
void on_menu_item_check_updates_activate(GtkMenuItem*, gpointer) {
	check_for_new_version(false);
}
void on_menu_item_about_activate(GtkMenuItem*, gpointer) {
	show_about();
}
void on_menu_item_reportbug_activate(GtkMenuItem*, gpointer) {
	report_bug();
}
void on_menu_item_help_activate(GtkMenuItem*, gpointer) {
	show_help("index.html", main_window());
}
void on_menu_item_save_activate(GtkMenuItem*, gpointer) {
	add_as_variable();
}
void on_menu_item_save_image_activate(GtkMenuItem*, gpointer) {
	save_as_image();
}
void on_menu_item_customize_buttons_activate(GtkMenuItem*, gpointer) {
	edit_buttons(main_window());
}
void on_menu_item_edit_shortcuts_activate(GtkMenuItem*, gpointer) {
	edit_shortcuts(main_window());
}
void on_menu_item_periodic_table_activate(GtkMenuItem*, gpointer) {
	show_periodic_table(main_window());
}
void on_menu_item_plot_functions_activate(GtkMenuItem*, gpointer) {
	open_plot();
}
void on_menu_item_set_unknowns_activate(GtkMenuItem*, gpointer) {
	set_unknowns();
}
void on_menu_item_fraction_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_DECIMAL);
}
void on_menu_item_fraction_decimal_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_DECIMAL_EXACT);
}
void on_menu_item_fraction_combined_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_COMBINED);
}
void on_menu_item_fraction_fraction_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_FRACTIONAL);
}
void on_menu_item_fraction_halves_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(2, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_3rds_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(3, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_4ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(4, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_5ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(5, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_6ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(6, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_8ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(8, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_10ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(10, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_12ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(12, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_16ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(16, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_32ths_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fixed_fraction(32, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"))));
}
void on_menu_item_fraction_fixed_combined_activate(GtkMenuItem *w, gpointer) {
	bool b = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	if((!b && printops.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR) || (b && printops.number_fraction_format == FRACTION_FRACTIONAL_FIXED_DENOMINATOR)) {
		set_fixed_fraction(CALCULATOR->fixedDenominator(), b);
	}
}
void on_menu_item_fraction_percent_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_PERCENT);
}
void on_menu_item_fraction_permille_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_PERMILLE);
}
void on_menu_item_fraction_permyriad_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_PERMYRIAD);
}
void update_menu_fraction() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_decimal_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_decimal_exact_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_combined_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_fraction_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_halves"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_halves_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_3rds"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_3rds_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_4ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_4ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_5ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_5ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_6ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_6ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_8ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_8ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_10ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_10ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_12ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_12ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_16ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_16ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_32ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_32ths_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_percent"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_percent_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_permille"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_permille_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_permyriad"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_permyriad_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_fixed_combined_activate, NULL);
	switch(printops.number_fraction_format) {
		case FRACTION_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
			break;
		}
		case FRACTION_DECIMAL_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
			break;
		}
		case FRACTION_COMBINED: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
			break;
		}
		case FRACTION_FRACTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
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
					if(printops.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
					} else {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
					}
					break;
				}
			}
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")), printops.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR);
			break;
		}
		case FRACTION_PERCENT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_percent")), TRUE);
			break;
		}
		case FRACTION_PERMILLE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permille")), TRUE);
			break;
		}
		case FRACTION_PERMYRIAD: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permyriad")), TRUE);
			break;
		}
	}
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_decimal_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_decimal_exact_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_combined_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_fraction"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_fraction_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_halves"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_halves_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_3rds"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_3rds_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_4ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_4ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_5ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_5ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_6ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_6ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_8ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_8ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_10ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_10ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_12ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_12ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_16ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_16ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_32ths"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_32ths_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_percent"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_percent_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_permille"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_permille_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_permyriad"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_permyriad_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_fraction_fixed_combined_activate, NULL);
}

void on_menu_item_interval_adaptive_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = true;
	printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	result_format_updated();
}
void on_menu_item_interval_significant_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	result_format_updated();
}
void on_menu_item_interval_interval_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
	result_format_updated();
}
void on_menu_item_interval_plusminus_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	result_format_updated();
}
void on_menu_item_interval_relative_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_RELATIVE;
	result_format_updated();
}
void on_menu_item_interval_concise_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_CONCISE;
	result_format_updated();
}
void on_menu_item_interval_midpoint_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_MIDPOINT;
	result_format_updated();
}
void on_menu_item_interval_lower_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_LOWER;
	result_format_updated();
}
void on_menu_item_interval_upper_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	adaptive_interval_display = false;
	printops.interval_display = INTERVAL_DISPLAY_UPPER;
	result_format_updated();
}

extern int to_caf;
void on_menu_item_complex_rectangular_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	complex_angle_form = false;
	to_caf = -1;
	expression_calculation_updated();
}
void on_menu_item_complex_exponential_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
	complex_angle_form = false;
	to_caf = -1;
	expression_calculation_updated();
}
void on_menu_item_complex_polar_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
	complex_angle_form = false;
	to_caf = -1;
	expression_calculation_updated();
}
void on_menu_item_complex_angle_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
	complex_angle_form = true;
	to_caf = -1;
	expression_calculation_updated();
}

void on_menu_item_ic_none_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.interval_calculation = INTERVAL_CALCULATION_NONE;
	expression_calculation_updated();
}
void on_menu_item_ic_variance_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	expression_calculation_updated();
}
void on_menu_item_ic_interval_arithmetic_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
	expression_calculation_updated();
}
void on_menu_item_ic_simple_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
	expression_calculation_updated();
}
void on_menu_item_interval_arithmetic_activate(GtkMenuItem *w, gpointer) {
	CALCULATOR->useIntervalArithmetic(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
	expression_calculation_updated();
}
void on_menu_item_concise_uncertainty_input_activate(GtkMenuItem *w, gpointer) {
	CALCULATOR->setConciseUncertaintyInputEnabled(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
	expression_format_updated();
}
void on_menu_item_always_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_approximation(APPROXIMATION_EXACT);
}
void on_menu_item_try_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_approximation(APPROXIMATION_TRY_EXACT);
}
void on_menu_item_approximate_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_approximation(APPROXIMATION_APPROXIMATE);
}
void update_menu_approximation() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_always_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_always_exact_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), evalops.approximation == APPROXIMATION_EXACT);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_always_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_always_exact_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_try_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_try_exact_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), evalops.approximation == APPROXIMATION_TRY_EXACT);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_try_exact"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_try_exact_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_approximate"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_approximate_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_approximate")), evalops.approximation == APPROXIMATION_APPROXIMATE);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_approximate"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_approximate_activate, NULL);
}
void on_menu_item_display_normal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_PRECISION, false);
}
void on_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_BASE_3, false);
}
void on_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_SCIENTIFIC, false);
}
void on_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_PURE, false);
}
void on_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_NONE, false);
}
void on_menu_item_display_no_prefixes_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_NO_PREFIXES);
}
void on_menu_item_display_prefixes_for_selected_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_SELECTED_UNITS);
}
void on_menu_item_display_prefixes_for_currencies_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_CURRENCIES);
}
void on_menu_item_display_prefixes_for_all_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_ALL_UNITS);
}
void on_menu_item_indicate_infinite_series_activate(GtkMenuItem *w, gpointer) {
	printops.indicate_infinite_series = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_show_ending_zeroes_activate(GtkMenuItem *w, gpointer) {
	printops.show_ending_zeroes = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_rounding_half_to_even_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_TO_EVEN;
	result_format_updated();
}
void on_menu_item_rounding_half_away_from_zero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_AWAY_FROM_ZERO;
	result_format_updated();
}
void on_menu_item_rounding_half_to_odd_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_TO_ODD;
	result_format_updated();
}
void on_menu_item_rounding_half_toward_zero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_TOWARD_ZERO;
	result_format_updated();
}
void on_menu_item_rounding_half_random_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_RANDOM;
	result_format_updated();
}
void on_menu_item_rounding_half_up_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_UP;
	result_format_updated();
}
void on_menu_item_rounding_half_down_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_HALF_DOWN;
	result_format_updated();
}
void on_menu_item_rounding_toward_zero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_TOWARD_ZERO;
	result_format_updated();
}
void on_menu_item_rounding_away_from_zero_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_AWAY_FROM_ZERO;
	result_format_updated();
}
void on_menu_item_rounding_up_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_UP;
	result_format_updated();
}
void on_menu_item_rounding_down_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	printops.rounding = ROUNDING_DOWN;
	result_format_updated();
}
extern bool scientific_negexp;
extern bool scientific_notminuslast;
void on_menu_item_negative_exponents_activate(GtkMenuItem *w, gpointer) {
	printops.negative_exponents = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	if(printops.min_exp != EXP_NONE && printops.min_exp != EXP_PRECISION) scientific_negexp = printops.negative_exponents;
	result_format_updated();
}
void on_menu_item_sort_minus_last_activate(GtkMenuItem *w, gpointer) {
	printops.sort_options.minus_last = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	if(printops.min_exp != EXP_NONE && printops.min_exp != EXP_PRECISION) scientific_notminuslast = !printops.sort_options.minus_last;
	result_format_updated();
}
void update_menu_numerical_display() {
	switch(printops.min_exp) {
		case EXP_PRECISION: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_normal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_normal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_normal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_normal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_normal_activate, NULL);
			break;
		}
		case EXP_BASE_3: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_engineering"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_engineering_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_engineering")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_engineering"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_engineering_activate, NULL);
			break;
		}
		case EXP_SCIENTIFIC: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_scientific_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_scientific")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_scientific_activate, NULL);
			break;
		}
		case EXP_PURE: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_purely_scientific_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_purely_scientific_activate, NULL);
			break;
		}
		case EXP_NONE: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_non_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_non_scientific_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_non_scientific")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_non_scientific"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_non_scientific_activate, NULL);
			break;
		}
	}
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), printops.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), printops.sort_options.minus_last);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_negative_exponents"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_negative_exponents_activate, NULL);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sort_minus_last"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_sort_minus_last_activate, NULL);

	if(!printops.use_unit_prefixes) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_no_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_no_prefixes_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_no_prefixes")), TRUE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_no_prefixes"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_no_prefixes_activate, NULL);
	} else if(printops.use_prefixes_for_all_units) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_all_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_prefixes_for_all_units_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_all_units")), TRUE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_all_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_prefixes_for_all_units_activate, NULL);
	} else if(printops.use_prefixes_for_currencies) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_currencies"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_prefixes_for_currencies_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_currencies")), TRUE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_currencies"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_prefixes_for_currencies_activate, NULL);
	} else {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_selected_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_prefixes_for_selected_units_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_selected_units")), TRUE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_selected_units"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_display_prefixes_for_selected_units_activate, NULL);
	}
}
void on_menu_item_binary_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_BINARY);
}
void on_menu_item_octal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_OCTAL);
}
void on_menu_item_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_DECIMAL);
}
void on_menu_item_duodecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(12);
}
void on_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_HEXADECIMAL);
}
void on_menu_item_custom_base_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	open_setbase(main_window(), true, false);
}
void on_menu_item_roman_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_ROMAN_NUMERALS);
}
void on_menu_item_sexagesimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_SEXAGESIMAL);
}
void on_menu_item_time_format_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_TIME);
}
void on_menu_item_set_base_activate(GtkMenuItem*, gpointer) {
	open_setbase(main_window());
}
void on_menu_item_abbreviate_names_activate(GtkMenuItem *w, gpointer) {
	printops.abbreviate_names = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
extern char to_prefix;
void on_menu_item_all_prefixes_activate(GtkMenuItem *w, gpointer) {
	to_prefix = 0;
	printops.use_all_prefixes = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	result_format_updated();
}
void on_menu_item_denominator_prefixes_activate(GtkMenuItem *w, gpointer) {
	to_prefix = 0;
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
void on_menu_item_expand_partial_fractions_activate(GtkMenuItem*, gpointer) {
	executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
}
void on_menu_item_simplify_activate(GtkMenuItem*, gpointer) {
	executeCommand(COMMAND_EXPAND);
}
void on_menu_item_convert_number_bases_activate(GtkMenuItem*, gpointer) {
	open_convert_number_bases();
}
void on_menu_item_convert_floatingpoint_activate(GtkMenuItem*, gpointer) {
	open_convert_floatingpoint();
}

void on_menu_item_show_percentage_dialog_activate(GtkMenuItem*, gpointer) {
	open_percentage_tool();
}

void on_menu_item_show_calendarconversion_dialog_activate(GtkMenuItem*, gpointer) {
	open_calendarconversion();
}

void on_menu_item_save_mode_activate(GtkMenuItem*, gpointer) {
	save_mode();
}
void on_menu_item_edit_prefs_activate(GtkMenuItem*, gpointer) {
	edit_preferences(main_window());
}
void on_menu_item_degrees_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		set_angle_unit(ANGLE_UNIT_DEGREES);
	}
}
void on_menu_item_radians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		set_angle_unit(ANGLE_UNIT_RADIANS);
	}
}
void on_menu_item_gradians_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		set_angle_unit(ANGLE_UNIT_GRADIANS);
	}
}
void on_menu_item_custom_angle_unit_activate(GtkMenuItem *w, gpointer data) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		Unit *u = (Unit*) data;
		if(CALCULATOR->hasUnit(u)) {
			set_custom_angle_unit(u);
		}
	}
}
void on_menu_item_no_default_angle_unit_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		set_angle_unit(ANGLE_UNIT_NONE);
	}
}
void on_menu_item_fetch_exchange_rates_activate(GtkMenuItem*, gpointer) {
	update_exchange_rates();
}
extern bool save_defs_on_exit;
void on_menu_item_save_defs_activate(GtkMenuItem*, gpointer) {
	save_defs();
}
void on_menu_item_import_definitions_activate(GtkMenuItem*, gpointer) {
	import_definitions_file();
}
void on_menu_item_minimal_mode_activate(GtkMenuItem*, gpointer) {
	set_minimal_mode(true);
}
void on_menu_item_quit_activate(GtkMenuItem*, gpointer) {
	qalculate_quit();
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
void on_menu_item_enable_variable_units_activate(GtkMenuItem *w, gpointer) {
	CALCULATOR->setVariableUnitsEnabled(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
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
	keypad_algebraic_mode_changed();
}
void on_menu_item_algebraic_mode_factorize_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	evalops.structuring = STRUCTURING_FACTORIZE;
	printops.allow_factorization = true;
	expression_calculation_updated();
	keypad_algebraic_mode_changed();
}
void on_menu_item_read_precision_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) evalops.parse_options.read_precision = READ_PRECISION_WHEN_DECIMALS;
	else evalops.parse_options.read_precision = DONT_READ_PRECISION;
	expression_format_updated(true);
}
void on_menu_item_new_unknown_activate(GtkMenuItem*, gpointer) {
	edit_unknown(NULL, NULL, main_window());
}
void on_menu_item_new_variable_activate(GtkMenuItem*, gpointer) {
	edit_variable(NULL, NULL, NULL, main_window());
}
void on_menu_item_new_matrix_activate(GtkMenuItem*, gpointer) {
	edit_matrix(NULL, NULL, NULL, main_window(), FALSE);
}
void on_menu_item_new_vector_activate(GtkMenuItem*, gpointer) {
	edit_matrix(NULL, NULL, NULL, main_window(), TRUE);
}
void on_menu_item_new_function_activate(GtkMenuItem*, gpointer) {
	edit_function("", NULL, main_window());
}
void on_menu_item_new_dataset_activate(GtkMenuItem*, gpointer) {
	edit_dataset(NULL, main_window());
}
void on_menu_item_new_unit_activate(GtkMenuItem*, gpointer) {
	edit_unit("", NULL, main_window());
}
void on_menu_item_autocalc_activate(GtkMenuItem *w, gpointer) {
	set_autocalculate(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_chain_mode_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)) == chain_mode) return;
	chain_mode = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
}
void on_menu_item_rpn_mode_activate(GtkMenuItem *w, gpointer) {
	set_rpn_mode(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_menu_item_rpn_syntax_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	update_status_menu();
	evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
	expression_format_updated(false);
}
void on_menu_item_limit_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	evalops.parse_options.limit_implicit_multiplication = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	printops.limit_implicit_multiplication = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(true);
	result_format_updated();
}
void on_menu_item_simplified_percentage_activate(GtkMenuItem *w, gpointer) {
	simplified_percentage = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	expression_format_updated(true);
}
void on_menu_item_adaptive_parsing_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	update_status_menu();
	evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	expression_format_updated(true);
}
void on_menu_item_ignore_whitespace_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	update_status_menu();
	evalops.parse_options.parsing_mode = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
	implicit_question_asked = true;
	expression_format_updated(true);
}
void on_menu_item_no_special_implicit_multiplication_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	update_status_menu();
	evalops.parse_options.parsing_mode = PARSING_MODE_CONVENTIONAL;
	implicit_question_asked = true;
	expression_format_updated(true);
}
void on_menu_item_chain_syntax_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	update_status_menu();
	evalops.parse_options.parsing_mode = PARSING_MODE_CHAIN;
	expression_format_updated(true);
}

void on_menu_item_manage_variables_activate(GtkMenuItem*, gpointer) {
	manage_variables(main_window());
}
void on_menu_item_manage_functions_activate(GtkMenuItem*, gpointer) {
	manage_functions(main_window());
}
void on_menu_item_manage_units_activate(GtkMenuItem*, gpointer) {
	manage_units(main_window());
}
void on_menu_item_datasets_activate(GtkMenuItem*, gpointer) {
	manage_datasets(main_window());
}

void on_menu_item_import_csv_file_activate(GtkMenuItem*, gpointer) {
	import_csv_file(main_window());
}

void on_menu_item_export_csv_file_activate(GtkMenuItem*, gpointer) {
	export_csv_file(main_window());
}
void on_menu_item_convert_to_unit_expression_activate(GtkMenuItem*, gpointer) {
	show_unit_conversion();
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

void on_menu_item_insert_date_activate(GtkMenuItem*, gpointer) {
	expression_insert_date();
}

void on_menu_item_insert_matrix_activate(GtkMenuItem*, gpointer) {
	expression_insert_matrix();
}
void on_menu_item_insert_vector_activate(GtkMenuItem*, gpointer) {
	expression_insert_vector();
}

void update_assumptions_items() {
	block_calculation();
	set_assumptions_items(CALCULATOR->defaultAssumptions()->type(), CALCULATOR->defaultAssumptions()->sign());
	unblock_calculation();
}

void on_menu_item_assumptions_integer_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_INTEGER);
	update_assumptions_items();
	expression_calculation_updated();
}
void on_menu_item_assumptions_boolean_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_BOOLEAN);
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

int mode_menu_i = 0;
void on_popup_menu_mode_update_activate(GtkMenuItem*, gpointer data) {
	save_mode_as((const char*) data, NULL, true);
	update_window_title();
	if(mode_menu_i == 1) {
		gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "mode_menu_menu")));
		gtk_menu_shell_deselect(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menubar")));
	} else if(mode_menu_i == 2) {
		gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")));
	} else if(mode_menu_i == 3) {
		gtk_menu_popdown(expression_edit_popup_menu());
	}
	focus_keeping_selection();
}
void on_popup_menu_mode_delete_activate(GtkMenuItem*, gpointer data) {
	size_t index = remove_mode((const char*) data);
	if(index == (size_t) -1) return;
	gtk_widget_destroy(mode_items[index]);
	gtk_widget_destroy(popup_result_mode_items[index]);
	mode_items.erase(mode_items.begin() + index);
	popup_result_mode_items.erase(popup_result_mode_items.begin() + index);
	if(mode_count(false) == 0) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), FALSE);
	if(mode_menu_i == 1) {
		gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "mode_menu_menu")));
		gtk_menu_shell_deselect(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menubar")));
	} else if(mode_menu_i == 2) {
		gtk_menu_popdown(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")));
	} else if(mode_menu_i == 3) {
		gtk_menu_popdown(expression_edit_popup_menu());
	}
	focus_keeping_selection();
}

gulong on_popup_menu_mode_update_activate_handler = 0, on_popup_menu_mode_delete_activate_handler = 0;
extern vector<GtkWidget*> popup_expression_mode_items;

gboolean on_menu_item_meta_mode_popup_menu(GtkWidget *w, gpointer data) {
	size_t index = mode_index((const char*) data);
	if(index == (size_t) -1) return TRUE;
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_mode_update")), index > 0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_mode_delete")), index > 1);
	if(on_popup_menu_mode_update_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_mode_update"), on_popup_menu_mode_update_activate_handler);
	if(on_popup_menu_mode_delete_activate_handler != 0) g_signal_handler_disconnect(gtk_builder_get_object(main_builder, "popup_menu_mode_delete"), on_popup_menu_mode_delete_activate_handler);
	on_popup_menu_mode_update_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_mode_update"), "activate", G_CALLBACK(on_popup_menu_mode_update_activate), data);
	on_popup_menu_mode_delete_activate_handler = g_signal_connect(gtk_builder_get_object(main_builder, "popup_menu_mode_delete"), "activate", G_CALLBACK(on_popup_menu_mode_delete_activate), data);
	mode_menu_i = 0;
	for(size_t i = 0; i < mode_items.size(); i++) {
		if(mode_items[i] == w) {mode_menu_i = 1; break;}
	}
	if(mode_menu_i == 0) {
		for(size_t i = 0; i < popup_result_mode_items.size(); i++) {
			if(popup_result_mode_items[i] == w) {mode_menu_i = 2; break;}
		}
	}
	if(mode_menu_i == 0) {
		for(size_t i = 0; i < popup_expression_mode_items.size(); i++) {
			if(popup_expression_mode_items[i] == w) {mode_menu_i = 3; break;}
		}
	}
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_mode")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_mode")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

gboolean on_menu_item_meta_mode_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	/* Ignore double-clicks and triple-clicks */
	if(gdk_event_triggers_context_menu((GdkEvent *) event) && event->type == GDK_BUTTON_PRESS) {
		on_menu_item_meta_mode_popup_menu(widget, data);
		return TRUE;
	}
	return FALSE;
}

void on_menu_item_meta_mode_activate(GtkMenuItem*, gpointer user_data) {
	const char *name = (const char*) user_data;
	load_mode(name);
}
void on_menu_item_meta_mode_save_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Save Mode"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_REJECT, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
	gtk_widget_show(hbox);
	GtkWidget *label = gtk_label_new(_("Name"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	GtkWidget *entry = gtk_combo_box_text_new_with_entry();
	for(size_t i = 2; ; i++) {
		mode_struct *mode = get_mode(i);
		if(!mode) break;
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), mode->name.c_str());
	}
	gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_widget_show(entry);
run_meta_mode_save_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT) {
		bool new_mode = true;
		string name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry));
		remove_blank_ends(name);
		if(name.empty()) {
			show_message(_("Empty name field."), GTK_WINDOW(dialog));
			goto run_meta_mode_save_dialog;
		}
		if(name == get_mode(0)->name) {
			show_message(_("Preset mode cannot be overwritten."), GTK_WINDOW(dialog));
			goto run_meta_mode_save_dialog;
		}
		size_t index = save_mode_as(name, &new_mode, true);
		update_window_title();
		if(new_mode) {
			mode_struct *mode = get_mode(index);
			GtkWidget *item = gtk_menu_item_new_with_label(mode->name.c_str());
			gtk_widget_show(item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) mode->name.c_str());
			g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) mode->name.c_str());
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) mode->name.c_str());
			gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_meta_modes")), item, (gint) index);
			mode_items.push_back(item);
			item = gtk_menu_item_new_with_label(mode->name.c_str());
			gtk_widget_show(item);
			g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) mode->name.c_str());
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) mode->name.c_str());
			g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) mode->name.c_str());
			gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_result_popup_meta_modes")), item, (gint) index);
			popup_result_mode_items.push_back(item);
			if(mode_count(false) == 1) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), TRUE);
		}
	}
	gtk_widget_destroy(dialog);
}

void on_menu_item_meta_mode_delete_activate(GtkMenuItem*, gpointer) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Delete Mode"), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_REJECT, _("_Delete"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
	gtk_widget_show(hbox);
	GtkWidget *label = gtk_label_new(_("Mode"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	GtkWidget *menu = gtk_combo_box_text_new();
	for(size_t i = 2; ; i++) {
		mode_struct *mode = get_mode(i);
		if(!mode) break;
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(menu), mode->name.c_str());
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu), 0);
	gtk_box_pack_end(GTK_BOX(hbox), menu, TRUE, TRUE, 0);
	gtk_widget_show(menu);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT && gtk_combo_box_get_active(GTK_COMBO_BOX(menu)) >= 0) {
		size_t index = gtk_combo_box_get_active(GTK_COMBO_BOX(menu)) + 2;
		gtk_widget_destroy(mode_items[index]);
		gtk_widget_destroy(popup_result_mode_items[index]);
		remove_mode(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(menu)));
		mode_items.erase(mode_items.begin() + index);
		popup_result_mode_items.erase(popup_result_mode_items.begin() + index);
		if(mode_count(false) == 0) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), FALSE);
	}
	gtk_widget_destroy(dialog);
}

void update_menu_base() {
	update_keypad_base();
	switch(printops.base) {
		case 2: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_binary_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_binary")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_binary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_binary_activate, NULL);
			break;
		}
		case 8: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_octal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_octal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_octal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_octal_activate, NULL);
			break;
		}
		case 10: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_decimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_decimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_decimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_decimal_activate, NULL);
			break;
		}
		case 12: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_decimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_duodecimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_duodecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_decimal_activate, NULL);
			break;
		}
		case 16: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_hexadecimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			break;
		}
		case BASE_SEXAGESIMAL: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sexagesimal")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_sexagesimal"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			break;
		}
		case BASE_TIME: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_time_format"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_time_format")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_time_format"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_roman")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_roman"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_hexadecimal_activate, NULL);
			break;
		}
		default: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_custom_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_base_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_custom_base"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_base_activate, NULL);
		}
	}
}

/*
	generate units menu
*/
void create_umenu() {
	if(u_menu) gtk_widget_destroy(u_menu);
	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "units_menu"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	if(RUNTIME_CHECK_GTK_VERSION(3, 22)) g_signal_connect(G_OBJECT(sub), "popped-up", G_CALLBACK(hide_completion), NULL);

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
				if(!is_currencies && u->isCurrency()) is_currencies = true;
				if(u->isActive() && !u->isHidden()) {
					if(is_currencies) {MENU_ITEM_WITH_OBJECT_AND_FLAG(u, insert_unit_from_menu)}
					else {MENU_ITEM_WITH_OBJECT(u, insert_unit_from_menu)}
				}
			}
			if(is_currencies) {
				SUBMENU_ITEM_PREPEND(_("more"), sub3)
				for(size_t i = 0; i < titem->objects.size(); i++) {
					u = (Unit*) titem->objects[i];
					if(u->isActive() && u->isHidden()) {
						MENU_ITEM_WITH_OBJECT_AND_FLAG(u, insert_unit_from_menu)
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
		if(u->isActive() && !u->isHidden() && !u->isLocal()) {
			MENU_ITEM_WITH_OBJECT(u, insert_unit_from_menu)
		}
	}
	if(!user_units.empty()) {
		SUBMENU_ITEM_PREPEND(_("User units"), sub)
		for(size_t i = 0; i < user_units.size(); i++) {
			MENU_ITEM_WITH_OBJECT(user_units[i], insert_unit_from_menu);
		}
	}

	sub = sub2;
	MENU_SEPARATOR
	item = gtk_menu_item_new_with_label(_("Prefixes"));
	gtk_widget_show (item);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
	create_pmenu(item);

}

/*
	generate unit submenu in edit menu
*/
void create_umenu2() {
	if(u_menu2) gtk_widget_destroy(u_menu2);
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
				if(!is_currencies && u->isCurrency()) is_currencies = true;
				if(u->isActive() && !u->isHidden()) {
					if(is_currencies) {MENU_ITEM_WITH_OBJECT_AND_FLAG(u, convert_to_unit)}
					else {MENU_ITEM_WITH_OBJECT(u, convert_to_unit)}
				}
			}
			if(is_currencies) {
				SUBMENU_ITEM_PREPEND(_("more"), sub3)
				for(size_t i = 0; i < titem->objects.size(); i++) {
					u = (Unit*) titem->objects[i];
					if(u->isActive() && u->isHidden()) {
						MENU_ITEM_WITH_OBJECT_AND_FLAG(u, convert_to_unit)
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
		if(u->isActive() && !u->isHidden() && !u->isLocal()) {
			MENU_ITEM_WITH_OBJECT(u, convert_to_unit)
		}
	}
	if(!user_units.empty()) {
		SUBMENU_ITEM_PREPEND(_("User units"), sub)
		for(size_t i = 0; i < user_units.size(); i++) {
			MENU_ITEM_WITH_OBJECT(user_units[i], insert_unit_from_menu);
		}
	}
}

/*
	generate functions menu
*/
void create_fmenu() {
	if(f_menu) gtk_widget_destroy(f_menu);
	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "functions_menu"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	if(RUNTIME_CHECK_GTK_VERSION(3, 22)) g_signal_connect(G_OBJECT(sub), "popped-up", G_CALLBACK(hide_completion), NULL);

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
					MENU_ITEM_WITH_OBJECT(f, insert_function_from_menu)
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
		if(f->isActive() && !f->isHidden() && !f->isLocal()) {
			MENU_ITEM_WITH_OBJECT(f, insert_function_from_menu)
		}
	}
	if(!user_functions.empty()) {
		SUBMENU_ITEM_PREPEND(_("User functions"), sub)
		for(size_t i = 0; i < user_functions.size(); i++) {
			MENU_ITEM_WITH_OBJECT(user_functions[i], insert_function_from_menu);
		}
	}
}

/*
	generate variables menu
*/
void create_vmenu() {

	if(v_menu) gtk_widget_destroy(v_menu);
	GtkWidget *item;
	GtkWidget *sub, *sub2, *sub3;
	item = GTK_WIDGET(gtk_builder_get_object(main_builder, "variables_menu"));
	sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
	if(RUNTIME_CHECK_GTK_VERSION(3, 22)) g_signal_connect(G_OBJECT(sub), "popped-up", G_CALLBACK(hide_completion), NULL);

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
					MENU_ITEM_WITH_OBJECT(v, insert_variable_from_menu);
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
		if(v->isActive() && !v->isHidden() && !v->isLocal()) {
			MENU_ITEM_WITH_OBJECT(v, insert_variable_from_menu);
		}
	}

	if(!user_variables.empty()) {
		SUBMENU_ITEM_PREPEND(_("User variables"), sub)
		for(size_t i = 0; i < user_variables.size(); i++) {
			MENU_ITEM_WITH_OBJECT(user_variables[i], insert_variable_from_menu);
		}
	}

}


/*
	generate prefixes submenu in units menu
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
	string str;
	FIX_SUPSUB_PRE_W(item)
	while(p) {
		str = p->preferredDisplayName(false, true, false, false, &can_display_unicode_string_function, (void*) item).name;
		switch(p->type()) {
			case PREFIX_DECIMAL: {
				str +=" (10<sup>";
				str += i2s(((DecimalPrefix*) p)->exponent());
				str += "</sup>)";
				break;
			}
			case PREFIX_BINARY: {
				str +=" (2<sup>";
				str += i2s(((BinaryPrefix*) p)->exponent());
				str += "</sup>)";
				break;
			}
			default: {}
		}
		FIX_SUPSUB(str);
		MENU_ITEM_WITH_POINTER(str.c_str(), insert_prefix_from_menu, p)
		gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), TRUE);
		index++;
		p = CALCULATOR->getPrefix(index);
	}
	pango_font_description_free(font_desc);
}

/*
	generate prefixes submenu in edit menu
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
	FIX_SUPSUB_PRE_W(item)
	string str;
	while(p) {
		str = p->preferredDisplayName(false, true, false, false, &can_display_unicode_string_function, (void*) item).name;
		switch(p->type()) {
			case PREFIX_DECIMAL: {
				str +=" (10<sup>";
				str += i2s(((DecimalPrefix*) p)->exponent());
				str += "</sup>)";
				break;
			}
			case PREFIX_BINARY: {
				str +=" (2<sup>";
				str += i2s(((BinaryPrefix*) p)->exponent());
				str += "</sup>)";
				break;
			}
			default: {}
		}
		FIX_SUPSUB(str);
		MENU_ITEM_WITH_POINTER(str.c_str(), on_menu_item_set_prefix_activate, p)
		gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), TRUE);
		index++;
		p = CALCULATOR->getPrefix(index);
	}
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
			item = gtk_menu_item_new_with_label(recent_functions[i]->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
			recent_function_items.push_back(item);
			gtk_widget_show(item);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_function_from_menu), (gpointer) recent_functions[i]);
		}
	}
	update_mb_fx_menu();
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
			item = gtk_menu_item_new_with_label(recent_variables[i]->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
			recent_variable_items.push_back(item);
			gtk_widget_show(item);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_variable_from_menu), (gpointer) recent_variables[i]);
		}
	}
	update_mb_pi_menu();
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
			item = gtk_menu_item_new_with_label(recent_units[i]->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
			recent_unit_items.push_back(item);
			gtk_widget_show(item);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_unit_from_menu), (gpointer) recent_units[i]);
		}
	}
	update_mb_units_menu();
}

void add_recent_function(MathFunction *object) {
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
	item = gtk_menu_item_new_with_label(object->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
	recent_function_items.push_back(item);
	recent_functions.push_back(object);
	gtk_widget_show(item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_function_from_menu), (gpointer) object);
}
void add_recent_variable(Variable *object) {
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
	item = gtk_menu_item_new_with_label(object->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
	recent_variable_items.push_back(item);
	recent_variables.push_back(object);
	gtk_widget_show(item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_variable_from_menu), (gpointer) object);
}
void add_recent_unit(Unit *object) {
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
	item = gtk_menu_item_new_with_label(object->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
	recent_unit_items.push_back(item);
	recent_units.push_back(object);
	gtk_widget_show(item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(insert_unit_from_menu), (gpointer) object);
}

void remove_from_recent_functions(MathFunction *f) {
	for(size_t i = 0; i < recent_functions.size(); i++) {
		if(recent_functions[i] == f) {
			recent_functions.erase(recent_functions.begin() + i);
			gtk_widget_destroy(recent_function_items[i]);
			recent_function_items.erase(recent_function_items.begin() + i);
			break;
		}
	}
}
void remove_from_recent_variables(Variable *v) {
	for(size_t i = 0; i < recent_variables.size(); i++) {
		if(recent_variables[i] == v) {
			recent_variables.erase(recent_variables.begin() + i);
			gtk_widget_destroy(recent_variable_items[i]);
			recent_variable_items.erase(recent_variable_items.begin() + i);
			break;
		}
	}
}
void remove_from_recent_units(Unit *u) {
	for(size_t i = 0; i < recent_units.size(); i++) {
		if(recent_units[i] == u) {
			recent_units.erase(recent_units.begin() + i);
			gtk_widget_destroy(recent_unit_items[i]);
			recent_unit_items.erase(recent_unit_items.begin() + i);
			break;
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

void add_custom_angles_to_menus() {
	Unit *u_rad = CALCULATOR->getRadUnit();
	GtkWidget *item;
	GtkWidget *sub = GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_angle_unit_menu"));
	GSList *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_degrees")));
	unordered_map<Unit*, bool> angle_unit_item_exists;
	for(unordered_map<Unit*, GtkWidget*>::iterator it = angle_unit_items.begin(); it != angle_unit_items.end(); ++it) {
		angle_unit_item_exists[it->first] = false;
	}
	int n = 3;
	bool b_selected = false;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->baseUnit() == u_rad) {
			Unit *u = CALCULATOR->units[i];
			if(u != u_rad && !u->isHidden() && u->isActive() && u->baseExponent() == 1 && !u->hasName("gra") && !u->hasName("deg")) {
				unordered_map<Unit*, GtkWidget*>::iterator it = angle_unit_items.find(u);
				if(it != angle_unit_items.end()) {
					item = it->second;
					gtk_menu_item_set_label(GTK_MENU_ITEM(item), u->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
					angle_unit_item_exists[u] = true;
				} else {
					item = gtk_radio_menu_item_new_with_label(group, u->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str());
					angle_unit_items[u] = item;
					g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(on_menu_item_custom_angle_unit_activate), (gpointer) u);
					gtk_menu_shell_insert(GTK_MENU_SHELL(sub), item, n);
					gtk_widget_show(item);
				}
				if(evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && CALCULATOR->customAngleUnit() == u) {
					b_selected = true;
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
				}
				n++;
			}
		}
	}
	for(unordered_map<Unit*, bool>::iterator it = angle_unit_item_exists.begin(); it != angle_unit_item_exists.end(); ++it) {
		if(!it->second) {
			item = angle_unit_items[it->first];
			g_signal_handlers_block_matched((gpointer) item, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_angle_unit_activate, NULL);
			gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(item), NULL);
			g_object_ref(G_OBJECT(item));
			gtk_container_remove(GTK_CONTAINER(sub), item);
			angle_unit_items.erase(it->first);
		}
	}
	if(!b_selected && evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_radians")), TRUE);
	}
}

void update_menu_angle() {
	switch(evalops.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_degrees_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_degrees")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_degrees"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_degrees_activate, NULL);
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_radians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_radians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_radians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_radians_activate, NULL);
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_gradians_activate, NULL);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_gradians")), TRUE);
			g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_gradians"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_gradians_activate, NULL);
			break;
		}
		case ANGLE_UNIT_CUSTOM: {
			unordered_map<Unit*, GtkWidget*>::iterator it = angle_unit_items.find(CALCULATOR->customAngleUnit());
			if(it != angle_unit_items.end()) {
				g_signal_handlers_block_matched((gpointer) it->second, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_angle_unit_activate, NULL);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(it->second), TRUE);
				g_signal_handlers_unblock_matched((gpointer) it->second, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_custom_angle_unit_activate, NULL);
			}
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_default_angle_unit")), TRUE);
			break;
		}
	}
}
void update_menu_calculator_mode() {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_autocalc"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_autocalc_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_autocalc")), auto_calculate && !rpn_mode);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_autocalc"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_autocalc_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_chain_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_chain_mode_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), chain_mode && !rpn_mode);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_chain_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_chain_mode_activate, NULL);
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_rpn_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_rpn_mode_activate, NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), rpn_mode);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "menu_item_rpn_mode"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_menu_item_rpn_mode_activate, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_autocalc")), !rpn_mode);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), !rpn_mode);
}

void set_mode_items(const mode_struct *mode, bool initial_update) {

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_autocalc")), mode->autocalc && (!initial_update || !mode->rpn_mode));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), mode->chain_mode && (!initial_update || !mode->rpn_mode));
	auto_calculate = mode->autocalc;
	chain_mode = mode->chain_mode;
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_autocalc")), !mode->rpn_mode);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_chain_mode")), !mode->rpn_mode);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), mode->rpn_mode);
	switch(mode->eo.approximation) {
		case APPROXIMATION_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), TRUE);
			break;
		}
		case APPROXIMATION_TRY_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_approximate")), TRUE);
			break;
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_arithmetic")), mode->interval);

	switch(mode->eo.interval_calculation) {
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

	switch(mode->eo.auto_post_conversion) {
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

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_mixed_units_conversion")), mode->eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE);

	switch(mode->eo.parse_options.angle_unit) {
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
			Unit *u = (initial_update || mode->custom_angle_unit.empty()) ? NULL : CALCULATOR->getActiveUnit(mode->custom_angle_unit);
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

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), mode->eo.parse_options.read_precision != DONT_READ_PRECISION);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_limit_implicit_multiplication")), mode->eo.parse_options.limit_implicit_multiplication);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_simplified_percentage")), mode->simplified_percentage);
	switch(mode->eo.parse_options.parsing_mode) {
		case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
			break;
		}
		case PARSING_MODE_CONVENTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
			break;
		}
		case PARSING_MODE_CHAIN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), TRUE);
			break;
		}
		case PARSING_MODE_RPN: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
			break;
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assume_nonzero_denominators")), mode->eo.assume_denominators_nonzero);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_warn_about_denominators_assumed_nonzero")), mode->eo.warn_about_denominators_assumed_nonzero);

	switch(mode->eo.structuring) {
		case STRUCTURING_FACTORIZE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_factorize")), TRUE);
			break;
		}
		default: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_simplify")), TRUE);
			break;
		}
	}

	switch(mode->po.base) {
		case BASE_BINARY: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_binary")), TRUE);
			break;
		}
		case BASE_OCTAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_octal")), TRUE);
			break;
		}
		case BASE_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_decimal")), TRUE);
			break;
		}
		case 12: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_duodecimal")), TRUE);
			break;
		}
		case BASE_HEXADECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_hexadecimal")), TRUE);
			break;
		}
		case BASE_ROMAN_NUMERALS: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_roman")), TRUE);
			break;
		}
		case BASE_SEXAGESIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sexagesimal")), TRUE);
			break;
		}
		case BASE_TIME: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_time_format")), TRUE);
			break;
		}
		default: {
			if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_custom_base")), TRUE);
			set_output_base(mode->po.base);
		}
	}

	switch(mode->po.min_exp) {
		case EXP_PRECISION: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_normal")), TRUE);
			break;
		}
		case EXP_BASE_3: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_engineering")), TRUE);
			break;
		}
		case EXP_SCIENTIFIC: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_scientific")), TRUE);
			break;
		}
		case EXP_PURE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific")), TRUE);
			break;
		}
		case EXP_NONE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_non_scientific")), TRUE);
			break;
		}
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_indicate_infinite_series")), mode->po.indicate_infinite_series);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_show_ending_zeroes")), mode->po.show_ending_zeroes);
	switch(mode->po.rounding) {
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

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_negative_exponents")), mode->po.negative_exponents);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_sort_minus_last")), mode->po.sort_options.minus_last);

	if(!mode->po.use_unit_prefixes) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_no_prefixes")), TRUE);
	} else if(mode->po.use_prefixes_for_all_units) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_all_units")), TRUE);
	} else if(mode->po.use_prefixes_for_currencies) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_currencies")), TRUE);
	} else {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_prefixes_for_selected_units")), TRUE);
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_all_prefixes")), mode->po.use_all_prefixes);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_denominator_prefixes")), mode->po.use_denominator_prefix);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_place_units_separately")), mode->po.place_units_separately);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_abbreviate_names")), mode->po.abbreviate_names);

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_variables")), mode->eo.parse_options.variables_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_functions")), mode->eo.parse_options.functions_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_units")), mode->eo.parse_options.units_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_unknown_variables")), mode->eo.parse_options.unknowns_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_enable_variable_units")), mode->variable_units_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_calculate_variables")), mode->eo.calculate_variables);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_allow_complex")), mode->eo.allow_complex);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_allow_infinite")), mode->eo.allow_infinite);

	if(initial_update) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")), fraction_fixed_combined);
	switch(mode->po.number_fraction_format) {
		case FRACTION_DECIMAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
			break;
		}
		case FRACTION_DECIMAL_EXACT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
			break;
		}
		case FRACTION_COMBINED: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
			break;
		}
		case FRACTION_FRACTIONAL: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
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
					if(mode->po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
						printops.number_fraction_format = FRACTION_COMBINED_FIXED_DENOMINATOR;
					} else {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
						printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
					}
					break;
				}
			}
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fixed_combined")), mode->po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR);
			break;
		}
		case FRACTION_PERCENT: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_percent")), TRUE);
			break;
		}
		case FRACTION_PERMILLE: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permille")), TRUE);
			break;
		}
		case FRACTION_PERMYRIAD: {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_permyriad")), TRUE);
			break;
		}
	}

	switch(mode->eo.complex_number_form) {
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
			if(mode->complex_angle_form) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_angle")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
			break;
		}
	}

	if(mode->adaptive_interval_display) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_adaptive")), TRUE);
	} else {
		switch(mode->po.interval_display) {
			case INTERVAL_DISPLAY_INTERVAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_interval")), TRUE); break;}
			case INTERVAL_DISPLAY_PLUSMINUS: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_plusminus")), TRUE); break;}
			case INTERVAL_DISPLAY_MIDPOINT: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_midpoint")), TRUE); break;}
			case INTERVAL_DISPLAY_SIGNIFICANT_DIGITS: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_significant")), TRUE); break;}
			default: {}
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_concise_uncertainty_input")), CALCULATOR->conciseUncertaintyInputEnabled());

	set_assumptions_items(mode->at, mode->as);

	if(!initial_update) {
		printops.max_decimals = mode->po.max_decimals;
		printops.use_max_decimals = mode->po.use_max_decimals;
		printops.max_decimals = mode->po.min_decimals;
		printops.use_min_decimals = mode->po.use_min_decimals;
		CALCULATOR->setPrecision(mode->precision);
		printops.spacious = mode->po.spacious;
		printops.short_multiplication = mode->po.short_multiplication;
		printops.excessive_parenthesis = mode->po.excessive_parenthesis;
		evalops.calculate_functions = mode->eo.calculate_functions;
		evalops.parse_options.base = mode->eo.parse_options.base;
	}
}
void update_menu_accels(int type) {
	bool b = false;
	for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
		if(it->second.type.size() != 1 || (type >= 0 && it->second.type[0] != type)) continue;
		b = true;
		switch(it->second.type[0]) {
			case SHORTCUT_TYPE_DATE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_insert_date")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_VECTOR: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_insert_vector")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_MATRIX: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_insert_matrix")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_CONVERT_ENTRY: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_to_custom_unit")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_OPTIMAL_UNIT: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_to_best_unit")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_BASE_UNITS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_to_base_units")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_FACTORIZE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_factorize")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_EXPAND: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_simplify")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_PARTIAL_FRACTIONS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_expand_partial_fractions")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_SET_UNKNOWNS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_set_unknowns")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_DEGREES: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_degrees")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_RADIANS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_radians")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_GRADIANS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_gradians")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_RPN_MODE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_AUTOCALC: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_autocalc")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_MINIMAL: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_minimal_mode")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_MANAGE_VARIABLES: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_manage_variables")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_manage_functions")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_MANAGE_UNITS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_manage_units")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_MANAGE_DATA_SETS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_datasets")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_STORE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_save")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_NEW_VARIABLE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_new_variable")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_NEW_FUNCTION: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_new_function")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_PLOT: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_plot_functions")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_NUMBER_BASES: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_number_bases")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_FLOATING_POINT: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_floatingpoint")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_CALENDARS: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_show_calendarconversion_dialog")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_PERCENTAGE_TOOL: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_show_percentage_dialog")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_PERIODIC_TABLE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_periodic_table")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_UPDATE_EXRATES: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_fetch_exchange_rates")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_COPY_RESULT: {
				int v = s2i(it->second.value[0]);
				if(v > 0 && v <= 7) break;
				if(!copy_ascii) {
					gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_copy")))), it->second.key, (GdkModifierType) it->second.modifier);
					if(type >= 0) {
						gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_copy_ascii")))), 0, (GdkModifierType) 0);
					}
				} else {
					gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_copy_ascii")))), it->second.key, (GdkModifierType) it->second.modifier);
					if(type >= 0) {
						gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_copy")))), 0, (GdkModifierType) 0);
					}
				}
				break;
			}
			case SHORTCUT_TYPE_SAVE_IMAGE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_save_image")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_HELP: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_help")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_QUIT: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_quit")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
			case SHORTCUT_TYPE_CHAIN_MODE: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_chain_mode")))), it->second.key, (GdkModifierType) it->second.modifier);
				break;
			}
		}
		if(type >= 0) break;
	}
	if(!b) {
		switch(type) {
			case SHORTCUT_TYPE_DATE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_insert_date")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_VECTOR: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_insert_vector")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_MATRIX: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_insert_matrix")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_CONVERT_ENTRY: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_to_custom_unit")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_OPTIMAL_UNIT: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_to_best_unit")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_BASE_UNITS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_to_base_units")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_FACTORIZE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_factorize")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_EXPAND: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_simplify")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_PARTIAL_FRACTIONS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_expand_partial_fractions")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_SET_UNKNOWNS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_set_unknowns")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_DEGREES: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_degrees")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_RADIANS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_radians")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_GRADIANS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_gradians")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_RPN_MODE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_AUTOCALC: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_autocalc")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_MINIMAL: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_minimal_mode")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_MANAGE_VARIABLES: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_manage_variables")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_manage_functions")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_MANAGE_UNITS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_manage_units")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_MANAGE_DATA_SETS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_datasets")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_STORE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_save")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_NEW_VARIABLE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_new_variable")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_NEW_FUNCTION: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_new_function")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_PLOT: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_plot_functions")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_NUMBER_BASES: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_number_bases")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_FLOATING_POINT: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_convert_floatingpoint")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_CALENDARS: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_show_calendarconversion_dialog")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_PERCENTAGE_TOOL: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_show_percentage_dialog")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_PERIODIC_TABLE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_periodic_table")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_UPDATE_EXRATES: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_fetch_exchange_rates")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_COPY_RESULT: {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_copy")))), 0, (GdkModifierType) 0);
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_copy_ascii")))), 0, (GdkModifierType) 0);
				break;
			}
			case SHORTCUT_TYPE_SAVE_IMAGE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_save_image")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_HELP: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_help")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_QUIT: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_quit")))), 0, (GdkModifierType) 0); break;}
			case SHORTCUT_TYPE_CHAIN_MODE: {gtk_accel_label_set_accel(GTK_ACCEL_LABEL(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(main_builder, "menu_item_chain_mode")))), 0, (GdkModifierType) 0); break;}
		}
	}
}

void create_menubar() {
	set_mode_items(get_mode(1), true);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_fetch_exchange_rates")), CALCULATOR->canFetch());
	for(size_t i = 0; ; i++) {
		mode_struct *mode = get_mode(i);
		if(!mode) break;
		GtkWidget *item = gtk_menu_item_new_with_label(mode->name.c_str());
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) mode->name.c_str());
		g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) mode->name.c_str());
		g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) mode->name.c_str());
		gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_meta_modes")), item, (gint) i);
		mode_items.push_back(item);
		item = gtk_menu_item_new_with_label(mode->name.c_str());
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_item_meta_mode_activate), (gpointer) mode->name.c_str());
		g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(on_menu_item_meta_mode_button_press), (gpointer) mode->name.c_str());
		g_signal_connect(G_OBJECT(item), "popup-menu", G_CALLBACK(on_menu_item_meta_mode_popup_menu), (gpointer) mode->name.c_str());
		gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(main_builder, "menu_result_popup_meta_modes")), item, (gint) i);
		popup_result_mode_items.push_back(item);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_meta_mode_delete")), mode_count(false) > 0);

	if(RUNTIME_CHECK_GTK_VERSION(3, 22)) {
		g_signal_connect(G_OBJECT(gtk_builder_get_object(main_builder, "file_menu_menu")), "popped-up", G_CALLBACK(hide_completion), NULL);
		g_signal_connect(G_OBJECT(gtk_builder_get_object(main_builder, "edit_menu_menu")), "popped-up", G_CALLBACK(hide_completion), NULL);
		g_signal_connect(G_OBJECT(gtk_builder_get_object(main_builder, "mode_menu_menu")), "popped-up", G_CALLBACK(hide_completion), NULL);
		g_signal_connect(G_OBJECT(gtk_builder_get_object(main_builder, "help_menu_menu")), "popped-up", G_CALLBACK(hide_completion), NULL);
	}

	gtk_builder_add_callback_symbols(main_builder, "on_menu_item_new_variable_activate", G_CALLBACK(on_menu_item_new_variable_activate), "on_menu_item_new_matrix_activate", G_CALLBACK(on_menu_item_new_matrix_activate), "on_menu_item_new_vector_activate", G_CALLBACK(on_menu_item_new_vector_activate), "on_menu_item_new_unknown_activate", G_CALLBACK(on_menu_item_new_unknown_activate), "on_menu_item_new_function_activate", G_CALLBACK(on_menu_item_new_function_activate), "on_menu_item_new_dataset_activate", G_CALLBACK(on_menu_item_new_dataset_activate), "on_menu_item_new_unit_activate", G_CALLBACK(on_menu_item_new_unit_activate), "on_menu_item_import_csv_file_activate", G_CALLBACK(on_menu_item_import_csv_file_activate), "on_menu_item_export_csv_file_activate", G_CALLBACK(on_menu_item_export_csv_file_activate), "on_menu_item_save_activate", G_CALLBACK(on_menu_item_save_activate), "on_menu_item_save_image_activate", G_CALLBACK(on_menu_item_save_image_activate), "on_menu_item_save_defs_activate", G_CALLBACK(on_menu_item_save_defs_activate), "on_menu_item_import_definitions_activate", G_CALLBACK(on_menu_item_import_definitions_activate), "on_menu_item_fetch_exchange_rates_activate", G_CALLBACK(on_menu_item_fetch_exchange_rates_activate), "on_menu_item_plot_functions_activate", G_CALLBACK(on_menu_item_plot_functions_activate), "on_menu_item_convert_number_bases_activate", G_CALLBACK(on_menu_item_convert_number_bases_activate), "on_menu_item_convert_floatingpoint_activate", G_CALLBACK(on_menu_item_convert_floatingpoint_activate), "on_menu_item_show_calendarconversion_dialog_activate", G_CALLBACK(on_menu_item_show_calendarconversion_dialog_activate), "on_menu_item_show_percentage_dialog_activate", G_CALLBACK(on_menu_item_show_percentage_dialog_activate), "on_menu_item_periodic_table_activate", G_CALLBACK(on_menu_item_periodic_table_activate), "on_menu_item_minimal_mode_activate", G_CALLBACK(on_menu_item_minimal_mode_activate), "on_menu_item_quit_activate", G_CALLBACK(on_menu_item_quit_activate), "on_menu_item_manage_variables_activate", G_CALLBACK(on_menu_item_manage_variables_activate), "on_menu_item_manage_functions_activate", G_CALLBACK(on_menu_item_manage_functions_activate), "on_menu_item_manage_units_activate", G_CALLBACK(on_menu_item_manage_units_activate), "on_menu_item_datasets_activate", G_CALLBACK(on_menu_item_datasets_activate), "on_menu_item_factorize_activate", G_CALLBACK(on_menu_item_factorize_activate), "on_menu_item_simplify_activate", G_CALLBACK(on_menu_item_simplify_activate), "on_menu_item_expand_partial_fractions_activate", G_CALLBACK(on_menu_item_expand_partial_fractions_activate), "on_menu_item_set_unknowns_activate", G_CALLBACK(on_menu_item_set_unknowns_activate), "on_menu_item_convert_to_unit_expression_activate", G_CALLBACK(on_menu_item_convert_to_unit_expression_activate), "on_menu_item_convert_to_base_units_activate", G_CALLBACK(on_menu_item_convert_to_base_units_activate), "on_menu_item_convert_to_best_unit_activate", G_CALLBACK(on_menu_item_convert_to_best_unit_activate), "on_menu_item_insert_date_activate", G_CALLBACK(on_menu_item_insert_date_activate), "on_menu_item_insert_matrix_activate", G_CALLBACK(on_menu_item_insert_matrix_activate), "on_menu_item_insert_vector_activate", G_CALLBACK(on_menu_item_insert_vector_activate), "on_menu_item_copy_activate", G_CALLBACK(on_menu_item_copy_activate), "on_menu_item_copy_ascii_activate", G_CALLBACK(on_menu_item_copy_ascii_activate), "on_menu_item_edit_shortcuts_activate", G_CALLBACK(on_menu_item_edit_shortcuts_activate), "on_menu_item_customize_buttons_activate", G_CALLBACK(on_menu_item_customize_buttons_activate), "on_menu_item_edit_prefs_activate", G_CALLBACK(on_menu_item_edit_prefs_activate), "on_menu_item_set_base_activate", G_CALLBACK(on_menu_item_set_base_activate), "on_menu_item_binary_activate", G_CALLBACK(on_menu_item_binary_activate), "on_menu_item_octal_activate", G_CALLBACK(on_menu_item_octal_activate), "on_menu_item_decimal_activate", G_CALLBACK(on_menu_item_decimal_activate), "on_menu_item_duodecimal_activate", G_CALLBACK(on_menu_item_duodecimal_activate), "on_menu_item_hexadecimal_activate", G_CALLBACK(on_menu_item_hexadecimal_activate), "on_menu_item_custom_base_activate", G_CALLBACK(on_menu_item_custom_base_activate), "on_menu_item_sexagesimal_activate", G_CALLBACK(on_menu_item_sexagesimal_activate), "on_menu_item_time_format_activate", G_CALLBACK(on_menu_item_time_format_activate), "on_menu_item_roman_activate", G_CALLBACK(on_menu_item_roman_activate), "on_menu_item_display_normal_activate", G_CALLBACK(on_menu_item_display_normal_activate), "on_menu_item_display_engineering_activate", G_CALLBACK(on_menu_item_display_engineering_activate), "on_menu_item_display_scientific_activate", G_CALLBACK(on_menu_item_display_scientific_activate), "on_menu_item_display_purely_scientific_activate", G_CALLBACK(on_menu_item_display_purely_scientific_activate), "on_menu_item_display_non_scientific_activate", G_CALLBACK(on_menu_item_display_non_scientific_activate), "on_menu_item_indicate_infinite_series_activate", G_CALLBACK(on_menu_item_indicate_infinite_series_activate), "on_menu_item_show_ending_zeroes_activate", G_CALLBACK(on_menu_item_show_ending_zeroes_activate), "on_menu_item_sort_minus_last_activate", G_CALLBACK(on_menu_item_sort_minus_last_activate), "on_menu_item_rounding_half_away_from_zero_activate", G_CALLBACK(on_menu_item_rounding_half_away_from_zero_activate), "on_menu_item_rounding_half_to_even_activate", G_CALLBACK(on_menu_item_rounding_half_to_even_activate), "on_menu_item_rounding_half_to_odd_activate", G_CALLBACK(on_menu_item_rounding_half_to_odd_activate), "on_menu_item_rounding_half_toward_zero_activate", G_CALLBACK(on_menu_item_rounding_half_toward_zero_activate),
	"on_menu_item_rounding_half_random_activate", G_CALLBACK(on_menu_item_rounding_half_random_activate), "on_menu_item_rounding_half_up_activate", G_CALLBACK(on_menu_item_rounding_half_up_activate), "on_menu_item_rounding_half_down_activate", G_CALLBACK(on_menu_item_rounding_half_down_activate), "on_menu_item_rounding_toward_zero_activate", G_CALLBACK(on_menu_item_rounding_toward_zero_activate), "on_menu_item_rounding_away_from_zero_activate", G_CALLBACK(on_menu_item_rounding_away_from_zero_activate), "on_menu_item_rounding_up_activate", G_CALLBACK(on_menu_item_rounding_up_activate), "on_menu_item_rounding_down_activate", G_CALLBACK(on_menu_item_rounding_down_activate), "on_menu_item_complex_rectangular_activate", G_CALLBACK(on_menu_item_complex_rectangular_activate), "on_menu_item_complex_exponential_activate", G_CALLBACK(on_menu_item_complex_exponential_activate), "on_menu_item_complex_polar_activate", G_CALLBACK(on_menu_item_complex_polar_activate), "on_menu_item_complex_angle_activate", G_CALLBACK(on_menu_item_complex_angle_activate), "on_menu_item_fraction_decimal_activate", G_CALLBACK(on_menu_item_fraction_decimal_activate), "on_menu_item_fraction_decimal_exact_activate", G_CALLBACK(on_menu_item_fraction_decimal_exact_activate), "on_menu_item_fraction_fraction_activate", G_CALLBACK(on_menu_item_fraction_fraction_activate), "on_menu_item_fraction_combined_activate", G_CALLBACK(on_menu_item_fraction_combined_activate), "on_menu_item_fraction_halves_activate", G_CALLBACK(on_menu_item_fraction_halves_activate), "on_menu_item_fraction_3rds_activate", G_CALLBACK(on_menu_item_fraction_3rds_activate), "on_menu_item_fraction_4ths_activate", G_CALLBACK(on_menu_item_fraction_4ths_activate), "on_menu_item_fraction_5ths_activate", G_CALLBACK(on_menu_item_fraction_5ths_activate), "on_menu_item_fraction_6ths_activate", G_CALLBACK(on_menu_item_fraction_6ths_activate), "on_menu_item_fraction_8ths_activate", G_CALLBACK(on_menu_item_fraction_8ths_activate), "on_menu_item_fraction_10ths_activate", G_CALLBACK(on_menu_item_fraction_10ths_activate), "on_menu_item_fraction_12ths_activate", G_CALLBACK(on_menu_item_fraction_12ths_activate), "on_menu_item_fraction_16ths_activate", G_CALLBACK(on_menu_item_fraction_16ths_activate), "on_menu_item_fraction_32ths_activate", G_CALLBACK(on_menu_item_fraction_32ths_activate), "on_menu_item_fraction_fixed_combined_activate", G_CALLBACK(on_menu_item_fraction_fixed_combined_activate), "on_menu_item_fraction_percent_activate", G_CALLBACK(on_menu_item_fraction_percent_activate), "on_menu_item_fraction_permille_activate", G_CALLBACK(on_menu_item_fraction_permille_activate), "on_menu_item_fraction_permyriad_activate", G_CALLBACK(on_menu_item_fraction_permyriad_activate), "on_menu_item_interval_adaptive_activate", G_CALLBACK(on_menu_item_interval_adaptive_activate), "on_menu_item_interval_significant_activate", G_CALLBACK(on_menu_item_interval_significant_activate), "on_menu_item_interval_interval_activate", G_CALLBACK(on_menu_item_interval_interval_activate), "on_menu_item_interval_plusminus_activate", G_CALLBACK(on_menu_item_interval_plusminus_activate), "on_menu_item_interval_relative_activate", G_CALLBACK(on_menu_item_interval_relative_activate), "on_menu_item_interval_concise_activate", G_CALLBACK(on_menu_item_interval_concise_activate), "on_menu_item_interval_midpoint_activate", G_CALLBACK(on_menu_item_interval_midpoint_activate), "on_menu_item_interval_lower_activate", G_CALLBACK(on_menu_item_interval_lower_activate), "on_menu_item_interval_upper_activate", G_CALLBACK(on_menu_item_interval_upper_activate), "on_menu_item_concise_uncertainty_input_activate", G_CALLBACK(on_menu_item_concise_uncertainty_input_activate), "on_menu_item_display_no_prefixes_activate", G_CALLBACK(on_menu_item_display_no_prefixes_activate), "on_menu_item_display_prefixes_for_selected_units_activate", G_CALLBACK(on_menu_item_display_prefixes_for_selected_units_activate),
	"on_menu_item_display_prefixes_for_currencies_activate", G_CALLBACK(on_menu_item_display_prefixes_for_currencies_activate), "on_menu_item_display_prefixes_for_all_units_activate", G_CALLBACK(on_menu_item_display_prefixes_for_all_units_activate), "on_menu_item_all_prefixes_activate", G_CALLBACK(on_menu_item_all_prefixes_activate), "on_menu_item_denominator_prefixes_activate", G_CALLBACK(on_menu_item_denominator_prefixes_activate), "on_menu_item_negative_exponents_activate", G_CALLBACK(on_menu_item_negative_exponents_activate), "on_menu_item_place_units_separately_activate", G_CALLBACK(on_menu_item_place_units_separately_activate), "on_menu_item_post_conversion_none_activate", G_CALLBACK(on_menu_item_post_conversion_none_activate), "on_menu_item_post_conversion_base_activate", G_CALLBACK(on_menu_item_post_conversion_base_activate), "on_menu_item_post_conversion_optimal_activate", G_CALLBACK(on_menu_item_post_conversion_optimal_activate), "on_menu_item_post_conversion_optimal_si_activate", G_CALLBACK(on_menu_item_post_conversion_optimal_si_activate), "on_menu_item_mixed_units_conversion_activate", G_CALLBACK(on_menu_item_mixed_units_conversion_activate), "on_menu_item_abbreviate_names_activate", G_CALLBACK(on_menu_item_abbreviate_names_activate), "on_menu_item_enable_variables_activate", G_CALLBACK(on_menu_item_enable_variables_activate), "on_menu_item_enable_functions_activate", G_CALLBACK(on_menu_item_enable_functions_activate), "on_menu_item_enable_units_activate", G_CALLBACK(on_menu_item_enable_units_activate), "on_menu_item_enable_unknown_variables_activate", G_CALLBACK(on_menu_item_enable_unknown_variables_activate), "on_menu_item_enable_variable_units_activate", G_CALLBACK(on_menu_item_enable_variable_units_activate), "on_menu_item_calculate_variables_activate", G_CALLBACK(on_menu_item_calculate_variables_activate), "on_menu_item_allow_complex_activate", G_CALLBACK(on_menu_item_allow_complex_activate), "on_menu_item_allow_infinite_activate", G_CALLBACK(on_menu_item_allow_infinite_activate), "on_menu_item_always_exact_activate", G_CALLBACK(on_menu_item_always_exact_activate), "on_menu_item_try_exact_activate", G_CALLBACK(on_menu_item_try_exact_activate), "on_menu_item_approximate_activate", G_CALLBACK(on_menu_item_approximate_activate), "on_menu_item_interval_arithmetic_activate", G_CALLBACK(on_menu_item_interval_arithmetic_activate), "on_menu_item_ic_none_activate", G_CALLBACK(on_menu_item_ic_none_activate), "on_menu_item_ic_variance_activate", G_CALLBACK(on_menu_item_ic_variance_activate), "on_menu_item_ic_interval_arithmetic_activate", G_CALLBACK(on_menu_item_ic_interval_arithmetic_activate), "on_menu_item_ic_simple_activate", G_CALLBACK(on_menu_item_ic_simple_activate), "on_menu_item_degrees_activate", G_CALLBACK(on_menu_item_degrees_activate), "on_menu_item_radians_activate", G_CALLBACK(on_menu_item_radians_activate), "on_menu_item_gradians_activate", G_CALLBACK(on_menu_item_gradians_activate), "on_menu_item_no_default_angle_unit_activate", G_CALLBACK(on_menu_item_no_default_angle_unit_activate), "on_menu_item_assumptions_none_activate", G_CALLBACK(on_menu_item_assumptions_none_activate), "on_menu_item_assumptions_nonmatrix_activate", G_CALLBACK(on_menu_item_assumptions_nonmatrix_activate), "on_menu_item_assumptions_number_activate", G_CALLBACK(on_menu_item_assumptions_number_activate), "on_menu_item_assumptions_complex_activate", G_CALLBACK(on_menu_item_assumptions_complex_activate), "on_menu_item_assumptions_real_activate", G_CALLBACK(on_menu_item_assumptions_real_activate), "on_menu_item_assumptions_rational_activate", G_CALLBACK(on_menu_item_assumptions_rational_activate), "on_menu_item_assumptions_integer_activate", G_CALLBACK(on_menu_item_assumptions_integer_activate), "on_menu_item_assumptions_boolean_activate", G_CALLBACK(on_menu_item_assumptions_boolean_activate), "on_menu_item_assumptions_unknown_activate", G_CALLBACK(on_menu_item_assumptions_unknown_activate), "on_menu_item_assumptions_nonzero_activate", G_CALLBACK(on_menu_item_assumptions_nonzero_activate), "on_menu_item_assumptions_positive_activate", G_CALLBACK(on_menu_item_assumptions_positive_activate), "on_menu_item_assumptions_nonnegative_activate", G_CALLBACK(on_menu_item_assumptions_nonnegative_activate), "on_menu_item_assumptions_negative_activate",
	G_CALLBACK(on_menu_item_assumptions_negative_activate), "on_menu_item_assumptions_nonpositive_activate", G_CALLBACK(on_menu_item_assumptions_nonpositive_activate), "on_menu_item_algebraic_mode_simplify_activate", G_CALLBACK(on_menu_item_algebraic_mode_simplify_activate), "on_menu_item_algebraic_mode_factorize_activate", G_CALLBACK(on_menu_item_algebraic_mode_factorize_activate), "on_menu_item_assume_nonzero_denominators_activate", G_CALLBACK(on_menu_item_assume_nonzero_denominators_activate), "on_menu_item_warn_about_denominators_assumed_nonzero_activate", G_CALLBACK(on_menu_item_warn_about_denominators_assumed_nonzero_activate), "on_menu_item_adaptive_parsing_activate", G_CALLBACK(on_menu_item_adaptive_parsing_activate), "on_menu_item_ignore_whitespace_activate", G_CALLBACK(on_menu_item_ignore_whitespace_activate), "on_menu_item_no_special_implicit_multiplication_activate", G_CALLBACK(on_menu_item_no_special_implicit_multiplication_activate), "on_menu_item_chain_syntax_activate", G_CALLBACK(on_menu_item_chain_syntax_activate), "on_menu_item_rpn_syntax_activate", G_CALLBACK(on_menu_item_rpn_syntax_activate), "on_menu_item_simplified_percentage_activate", G_CALLBACK(on_menu_item_simplified_percentage_activate), "on_menu_item_limit_implicit_multiplication_activate", G_CALLBACK(on_menu_item_limit_implicit_multiplication_activate), "on_menu_item_read_precision_activate", G_CALLBACK(on_menu_item_read_precision_activate), "on_menu_item_precision_activate", G_CALLBACK(on_menu_item_precision_activate), "on_menu_item_decimals_activate", G_CALLBACK(on_menu_item_decimals_activate), "on_menu_item_autocalc_activate", G_CALLBACK(on_menu_item_autocalc_activate), "on_menu_item_chain_mode_activate", G_CALLBACK(on_menu_item_chain_mode_activate), "on_menu_item_rpn_mode_activate", G_CALLBACK(on_menu_item_rpn_mode_activate), "on_menu_item_meta_mode_save_activate", G_CALLBACK(on_menu_item_meta_mode_save_activate), "on_menu_item_meta_mode_delete_activate", G_CALLBACK(on_menu_item_meta_mode_delete_activate), "on_menu_item_save_mode_activate", G_CALLBACK(on_menu_item_save_mode_activate), "on_menu_item_help_activate", G_CALLBACK(on_menu_item_help_activate), "on_menu_item_reportbug_activate", G_CALLBACK(on_menu_item_reportbug_activate), "on_menu_item_check_updates_activate", G_CALLBACK(on_menu_item_check_updates_activate), "on_menu_item_about_activate", G_CALLBACK(on_menu_item_about_activate), "on_menu_item_set_prefix_activate", G_CALLBACK(on_menu_item_set_prefix_activate), NULL);
}
