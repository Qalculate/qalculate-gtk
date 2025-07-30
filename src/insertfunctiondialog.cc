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
#include "mainwindow.h"
#include "stackview.h"
#include "expressionedit.h"
#include "insertfunctiondialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool keep_function_dialog_open = false;
int default_signed = -1;
int default_bits = -1;

#define USE_QUOTES(arg, f) (arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && f->id() != FUNCTION_ID_BASE && f->id() != FUNCTION_ID_BIN && f->id() != FUNCTION_ID_OCT && f->id() != FUNCTION_ID_DEC && f->id() != FUNCTION_ID_HEX)

bool read_insert_function_dialog_settings_line(string &svar, string&, int &v) {
	if(svar == "keep_function_dialog_open") {
		keep_function_dialog_open = v;
	} else if(svar == "bit_width") {
		default_bits = v;
	} else if(svar == "signed_integer") {
		default_signed = v;
	} else {
		return false;
	}
	return true;
}
void write_insert_function_dialog_settings(FILE *file) {
	fprintf(file, "keep_function_dialog_open=%i\n", keep_function_dialog_open);
	if(default_bits >= 0) fprintf(file, "bit_width=%i\n", default_bits);
	if(default_signed >= 0) fprintf(file, "signed_integer=%i\n", default_signed);
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
void on_type_label_vector_clicked(GtkEntry *w, gpointer) {
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
	insert_matrix(str.empty() ? NULL : &mstruct_v, GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW)), TRUE, false, false, w);
}
void on_type_label_matrix_clicked(GtkEntry *w, gpointer) {
	MathStructure mstruct_m;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	if(!str.empty()) {
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->parse(&mstruct_m, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops.parse_options);
		CALCULATOR->endTemporaryStopMessages();
		if(!mstruct_m.isMatrix()) str = "";
	}
	insert_matrix(str.empty() ? NULL : &mstruct_m, GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(w), GTK_TYPE_WINDOW)), FALSE, false, false, w);
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
	MathFunction *f;
};

unordered_map<MathFunction*, FunctionDialog*> function_dialogs;

void insert_function_do(MathFunction *f, FunctionDialog *fd) {
	string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_FUNCTION, true) + "(", str2;

	int argcount = fd->args;
	if((f->maxargs() < 0 || f->minargs() < f->maxargs()) && argcount > f->minargs()) {
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
	restore_expression_selection();
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
	gtk_text_buffer_set_text(expression_edit_buffer(), "", -1);
	insert_function_do(f, fd);
	execute_expression();
	if(fd->keep_open) {
		string str;
		bool b_approx = current_result_text_is_approximate() || (current_result() && current_result()->isApproximate());
		if(!b_approx) {
			str = "=";
		} else {
			if(printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) fd->w_result)) {
				str = SIGN_ALMOST_EQUAL;
			} else {
				str = "= ";
				str += _("approx.");
			}
		}
		str += " <span font-weight=\"bold\">";
		str += current_result_text();
		str += "</span>";
		gtk_label_set_markup(GTK_LABEL(fd->w_result), str.c_str());
		gtk_widget_grab_focus(fd->entry[0]);
	} else {
		gtk_widget_destroy(fd->dialog);
		delete fd;
		function_dialogs.erase(f);
	}
}
void on_insert_function_insert_rpn(GtkWidget*, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	if(!fd->keep_open) gtk_widget_hide(fd->dialog);
	if(rpn_mode) {
		calculateRPN(f);
		if(fd->add_to_menu) function_inserted(f);
	} else {
		insert_function_do(f, fd);
	}
	if(fd->keep_open) {
		gtk_widget_grab_focus(fd->entry[0]);
	} else {
		gtk_widget_destroy(fd->dialog);
		delete fd;
		function_dialogs.erase(f);
	}
}
void update_insert_function_dialogs() {
	for(unordered_map<MathFunction*, FunctionDialog*>::iterator it = function_dialogs.begin(); it != function_dialogs.end(); ++it) {
		FunctionDialog *fd = it->second;
		gtk_widget_set_sensitive(fd->b_insert, !rpn_mode || CALCULATOR->RPNStackSize() >= (fd->f->minargs() <= 0 ? 1 : (size_t) fd->f->minargs()));
		gtk_button_set_label(GTK_BUTTON(fd->b_insert), rpn_mode ? _("Apply to Stack") : _("_Insert"));
	}
}
void on_insert_function_keepopen(GtkToggleButton *w, gpointer p) {
	MathFunction *f = (MathFunction*) p;
	FunctionDialog *fd = function_dialogs[f];
	fd->keep_open = gtk_toggle_button_get_active(w);
	keep_function_dialog_open = fd->keep_open;
}
void on_insert_function_changed(GtkWidget*, gpointer p) {
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
				if(fd->keep_open || rpn_mode) on_insert_function_exec(w, p);
				else on_insert_function_insert_rpn(w, p);
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
*/
void insert_function(MathFunction *f, GtkWindow *parent, bool add_to_menu) {
	if(!f) {
		return;
	}

	//if function takes no arguments, do not display dialog and insert function directly
	if(f->args() == 0) {
		string str = f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_FUNCTION, true) + "()";
		gchar *gstr = g_strdup(str.c_str());
		function_inserted(f);
		insert_text(gstr);
		g_free(gstr);
		return;
	}

	store_expression_selection();

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
	fd->f = f;

	int args = 0;
	bool has_vector = false;
	if(f->args() > 0) {
		args = f->args();
	} else if(f->minargs() > 0) {
		args = f->minargs();
		while(!f->getDefaultValue(args + 1).empty()) args++;
		if(args == 1 || f->id() == FUNCTION_ID_PLOT) args++;
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
	gtk_window_set_transient_for(GTK_WINDOW(fd->dialog), parent);
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
	gtk_widget_set_sensitive(fd->b_insert, !rpn_mode || CALCULATOR->RPNStackSize() >= (f->minargs() <= 0 ? 1 : (size_t) f->minargs()));
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
							rpn_vector += ",";
							rpn_vector += " ";
							rpn_vector += str;
							i2++;
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
	g_signal_connect(G_OBJECT(fd->b_insert), "clicked", G_CALLBACK(on_insert_function_insert_rpn), (gpointer) f);
	g_signal_connect(G_OBJECT(fd->b_cancel), "clicked", G_CALLBACK(on_insert_function_close), (gpointer) f);
	g_signal_connect(G_OBJECT(fd->b_keepopen), "toggled", G_CALLBACK(on_insert_function_keepopen), (gpointer) f);
	g_signal_connect(G_OBJECT(fd->dialog), "delete-event", G_CALLBACK(on_insert_function_delete), (gpointer) f);

	gtk_widget_show_all(fd->dialog);

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

	if(f->minargs() > 2) return insert_function(f, main_window(), save_to_recent);

	bool b_bitrot = (f->referenceName() == "bitrot");

	const ExpressionName *ename = &f->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget());
	Argument *arg = f->getArgumentDefinition(1);
	Argument *arg2 = f->getArgumentDefinition(2);
	bool b_text = USE_QUOTES(arg, f);
	bool b_text2 = USE_QUOTES(arg2, f);
	GtkTextIter istart, iend, ipos;
	gtk_text_buffer_get_start_iter(expression_edit_buffer(), &istart);
	gtk_text_buffer_get_end_iter(expression_edit_buffer(), &iend);
	gchar *expr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
	GtkTextMark *mpos = gtk_text_buffer_get_insert(expression_edit_buffer());
	gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &ipos, mpos);
	if(!gtk_text_buffer_get_has_selection(expression_edit_buffer()) && gtk_text_iter_is_end(&ipos)) {
		if(!rpn_mode && chain_mode) {
			string str = CALCULATOR->unlocalizeExpression(expr, evalops.parse_options);
			remove_blanks(str);
			size_t par_n = 0, vec_n = 0;
			for(size_t i = 0; i < str.length(); i++) {
				if(str[i] == LEFT_PARENTHESIS_CH) par_n++;
				else if(par_n > 0 && str[i] == RIGHT_PARENTHESIS_CH) par_n--;
				else if(str[i] == LEFT_VECTOR_WRAP_CH) vec_n++;
				else if(vec_n > 0 && str[i] == RIGHT_VECTOR_WRAP_CH) vec_n--;
			}
			if(par_n <= 0 && vec_n <= 0 && !str.empty() && str[0] != '/' && !CALCULATOR->hasToExpression(str, true, evalops) && !CALCULATOR->hasWhereExpression(str, evalops) && !last_is_operator(str)) {
				GtkTextIter ibegin;
				gtk_text_buffer_get_end_iter(expression_edit_buffer(), &ibegin);
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
							str = "";
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
				gtk_text_buffer_select_range(expression_edit_buffer(), &ibegin, &iend);
			}
		} else if(last_is_number(expr)) {
			size_t par_n = 0, vec_n = 0;
			for(size_t i = 0; i < strlen(expr); i++) {
				if(expr[i] == LEFT_PARENTHESIS_CH) par_n++;
				else if(par_n > 0 && expr[i] == RIGHT_PARENTHESIS_CH) par_n--;
				else if(expr[i] == LEFT_VECTOR_WRAP_CH) vec_n++;
				else if(vec_n > 0 && expr[i] == RIGHT_VECTOR_WRAP_CH) vec_n--;
			}
			if(par_n <= 0 && vec_n <= 0) {
				// special case: the user just entered a number, then select all, so that it gets executed
				gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iend);
			}
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
		gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &istart, &iend);
		GtkWidget *dialog = gtk_dialog_new_with_buttons(f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str(), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
		GtkWidget *grid = gtk_grid_new();
		gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
		gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
		gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
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
		if(!b_bitrot) {
			PangoLayout *layout = gtk_widget_create_pango_layout(w1, "000000000000000");
			gint w = 0;
			pango_layout_get_pixel_size(layout, &w, NULL);
			g_object_unref(layout);
			gtk_widget_set_size_request(w1, w, -1);
		}
		combo_set_bits(GTK_COMBO_BOX(w1), default_bits > 0 ? default_bits : printops.binary_bits, false);
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
			gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iend);
			return;
		}
		gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iend);
		Number bits(combo_get_bits(GTK_COMBO_BOX(w1), false), 1, 0);
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
		gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &istart, &iend);
		GtkWidget *dialog = gtk_dialog_new_with_buttons(f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) main_window()).c_str(), main_window(), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);
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
			gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iend);
			return;
		}
		gtk_text_buffer_select_range(expression_edit_buffer(), &istart, &iend);
		if(evalops.parse_options.base != BASE_DECIMAL) {
			Number nr(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)), 1);
			str2 = print_with_evalops(nr);
		} else {
			str2 = gtk_entry_get_text(GTK_ENTRY(entry));
		}
		gtk_widget_destroy(dialog);
	}
	if(gtk_text_buffer_get_has_selection(expression_edit_buffer())) {
		gtk_text_buffer_get_selection_bounds(expression_edit_buffer(), &istart, &iend);
		// execute expression, if the whole expression was selected, no need for additional enter
		bool do_exec = (!str2.empty() || (f->minargs() < 2 && !b_rootlog)) && !rpn_mode && ((gtk_text_iter_is_start(&istart) && gtk_text_iter_is_end(&iend)) || (gtk_text_iter_is_start(&iend) && gtk_text_iter_is_end(&istart)));
		//set selection as argument
		gchar *gstr = gtk_text_buffer_get_text(expression_edit_buffer(), &istart, &iend, FALSE);
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
			gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, gtk_text_buffer_get_insert(expression_edit_buffer()));
			gtk_text_iter_backward_chars(&iter, (b_text2 ? 2 : 1) + unicode_length(sto));
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
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
			gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, gtk_text_buffer_get_insert(expression_edit_buffer()));
			if(index == 2) {
				gtk_text_iter_backward_chars(&iter, g_utf8_strlen(str2.c_str(), -1) + (b_text ? 4 : 3));
			} else {
				gtk_text_iter_backward_chars(&iter, b_text ? 2 : 1);
			}
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
			g_free(gstr2);
		} else {
			gchar *gstr2;
			gstr2 = g_strdup_printf(b_text ? "%s(\"\")" : "%s()", ename->formattedName(TYPE_FUNCTION, true).c_str());
			insert_text(gstr2);
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(expression_edit_buffer(), &iter, gtk_text_buffer_get_insert(expression_edit_buffer()));
			gtk_text_iter_backward_chars(&iter, b_text ? 2 : 1);
			gtk_text_buffer_place_cursor(expression_edit_buffer(), &iter);
			g_free(gstr2);
		}
	}
	g_free(expr);
	if(save_to_recent) function_inserted(f);
}

void insert_function_binary_bits_changed() {
	default_bits = -1;
}
void insert_function_twos_complement_changed(bool, bool, bool, bool hi) {
	if(hi && evalops.parse_options.twos_complement != default_signed) default_signed = -1;
}
