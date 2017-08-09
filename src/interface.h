/*
    Qalculate (GTK+ UI)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef INTERFACE_H
#define INTERFACE_H

#define MENU_ITEM_WITH_INT(x,y,z)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), GINT_TO_POINTER (z)); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_STRING(x,y,z)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) z); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_POINTER(x,y,z)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) z); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM(x,y)				item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define CHECK_MENU_ITEM(x,y,b)			item = gtk_check_menu_item_new_with_label(x); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), b); gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define RADIO_MENU_ITEM(x,y,b)			item = gtk_radio_menu_item_new_with_label(group, x); group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item)); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), b); gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define POPUP_CHECK_MENU_ITEM_WITH_LABEL(y,w,l)	item = gtk_check_menu_item_new_with_label(l); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))); gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(w)); if(gstr) {gtk_widget_set_tooltip_text(item, gstr); g_free(gstr);} gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define POPUP_CHECK_MENU_ITEM(y,w)		item = gtk_check_menu_item_new_with_label(gtk_menu_item_get_label(GTK_MENU_ITEM(w))); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))); gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(w)); if(gstr) {gtk_widget_set_tooltip_text(item, gstr); g_free(gstr);} gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define POPUP_RADIO_MENU_ITEM(y,w)		item = gtk_radio_menu_item_new_with_label(group, gtk_menu_item_get_label(GTK_MENU_ITEM(w))); group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item)); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))); gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(w)); if(gstr) {gtk_widget_set_tooltip_text(item, gstr); g_free(gstr);} gtk_widget_show (item); g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_SET_ACCEL(a)			gtk_widget_add_accelerator(item, "activate", accel_group, a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
#define MENU_TEAROFF				item = gtk_tearoff_menu_item_new(); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_SEPARATOR				item = gtk_separator_menu_item_new(); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_SEPARATOR_PREPEND			item = gtk_separator_menu_item_new(); gtk_widget_show (item); gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
#define RADIO_MENU_ITEM_WITH_INT_1(x,w)		item = gtk_radio_menu_item_new_with_label(w, x); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item); 
#define RADIO_MENU_ITEM_WITH_INT_2(x,y,z)	g_signal_connect (G_OBJECT (x), "activate", G_CALLBACK(y), GINT_TO_POINTER (z));
#define SUBMENU_ITEM(x,y)			item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(y), item); sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);   
#define SUBMENU_ITEM_PREPEND(x,y)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); gtk_menu_shell_prepend(GTK_MENU_SHELL(y), item); sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);   
#define SUBMENU_ITEM_INSERT(x,y,i)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); gtk_menu_shell_insert(GTK_MENU_SHELL(y), item, i); sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);   

#define EXPRESSION_YPAD 3

enum {
	UNITS_TITLE_COLUMN,
	UNITS_NAMES_COLUMN,
	UNITS_BASE_COLUMN,
	UNITS_POINTER_COLUMN,
	UNITS_N_COLUMNS
};

enum {
	UNIT_CLASS_BASE_UNIT,
	UNIT_CLASS_ALIAS_UNIT,	
	UNIT_CLASS_COMPOSITE_UNIT
};

enum {
	NAMES_NAME_COLUMN,
	NAMES_ABBREVIATION_STRING_COLUMN,
	NAMES_PLURAL_STRING_COLUMN,
	NAMES_REFERENCE_STRING_COLUMN,
	NAMES_ABBREVIATION_COLUMN,
	NAMES_SUFFIX_COLUMN,
	NAMES_UNICODE_COLUMN,
	NAMES_PLURAL_COLUMN,
	NAMES_REFERENCE_COLUMN,
	NAMES_AVOID_INPUT_COLUMN,
	NAMES_CASE_SENSITIVE_COLUMN,
	NAMES_N_COLUMNS
};

enum {
	DELIMITER_COMMA,
	DELIMITER_TABULATOR,	
	DELIMITER_SEMICOLON,
	DELIMITER_SPACE,
	DELIMITER_OTHER
};

enum {
	MENU_ARGUMENT_TYPE_FREE,
	MENU_ARGUMENT_TYPE_NUMBER,	
	MENU_ARGUMENT_TYPE_INTEGER,
	MENU_ARGUMENT_TYPE_SYMBOLIC,
	MENU_ARGUMENT_TYPE_TEXT,
	MENU_ARGUMENT_TYPE_DATE,	
	MENU_ARGUMENT_TYPE_VECTOR,	
	MENU_ARGUMENT_TYPE_MATRIX,	
	MENU_ARGUMENT_TYPE_POSITIVE,
	MENU_ARGUMENT_TYPE_NONZERO,			
	MENU_ARGUMENT_TYPE_NONNEGATIVE,
	MENU_ARGUMENT_TYPE_POSITIVE_INTEGER,
	MENU_ARGUMENT_TYPE_NONZERO_INTEGER,			
	MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER,	
	MENU_ARGUMENT_TYPE_BOOLEAN,	
	MENU_ARGUMENT_TYPE_EXPRESSION_ITEM,	
	MENU_ARGUMENT_TYPE_FUNCTION,	
	MENU_ARGUMENT_TYPE_UNIT,
	MENU_ARGUMENT_TYPE_VARIABLE,
	MENU_ARGUMENT_TYPE_FILE,
	MENU_ARGUMENT_TYPE_ANGLE,
	MENU_ARGUMENT_TYPE_DATA_OBJECT,
	MENU_ARGUMENT_TYPE_DATA_PROPERTY
};

enum {
	SMOOTHING_MENU_NONE,
	SMOOTHING_MENU_UNIQUE,
	SMOOTHING_MENU_CSPLINES,
	SMOOTHING_MENU_BEZIER,
	SMOOTHING_MENU_SBEZIER
};

enum {
	PLOTSTYLE_MENU_LINES,
	PLOTSTYLE_MENU_POINTS,
	PLOTSTYLE_MENU_LINESPOINTS,
	PLOTSTYLE_MENU_BOXES,
	PLOTSTYLE_MENU_HISTEPS,
	PLOTSTYLE_MENU_STEPS,
	PLOTSTYLE_MENU_CANDLESTICKS,
	PLOTSTYLE_MENU_DOTS
};

enum {
	PLOTLEGEND_MENU_NONE,
	PLOTLEGEND_MENU_TOP_LEFT,
	PLOTLEGEND_MENU_TOP_RIGHT,
	PLOTLEGEND_MENU_BOTTOM_LEFT,
	PLOTLEGEND_MENU_BOTTOM_RIGHT,
	PLOTLEGEND_MENU_BELOW,
	PLOTLEGEND_MENU_OUTSIDE
};

void create_button_menus(void);
void create_main_window (void);
GtkWidget* get_functions_dialog (void);
GtkWidget* get_variables_dialog (void);
GtkWidget* get_units_dialog (void);
GtkWidget* get_datasets_dialog (void);
GtkWidget* get_preferences_dialog (void);
GtkWidget* get_unit_edit_dialog (void);
GtkWidget* get_function_edit_dialog (void);
GtkWidget* get_variable_edit_dialog (void);
GtkWidget* get_unknown_edit_dialog (void);
GtkWidget* get_matrix_edit_dialog (void);
GtkWidget* get_matrix_dialog (void);
GtkWidget* get_dataobject_edit_dialog (void);
GtkWidget* get_dataset_edit_dialog (void);
GtkWidget* get_dataproperty_edit_dialog (void);
GtkWidget* get_names_edit_dialog (void);
GtkWidget* get_csv_import_dialog (void);
GtkWidget* get_csv_export_dialog (void);
GtkWidget* get_set_base_dialog (void);
GtkWidget* get_nbases_dialog (void);
GtkWidget* get_argument_rules_dialog (void);
GtkWidget* get_decimals_dialog (void);
GtkWidget* get_plot_dialog (void);
GtkWidget* get_precision_dialog (void);
GtkWidget* get_unit_dialog (void);
GtkWidget* get_periodic_dialog (void);
GtkWidget* create_InfoWidget(const gchar *text);

#endif /* INTERFACE_H */
