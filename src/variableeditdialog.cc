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
#include "openhelp.h"
#include "nameseditdialog.h"
#include "matrixeditdialog.h"
#include "unknowneditdialog.h"
#include "variableeditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *variableedit_builder = NULL;

KnownVariable *edited_variable = NULL;

bool variable_value_changed = false;

void on_variables_edit_textview_value_changed(GtkTextBuffer*, gpointer) {
	variable_value_changed = true;
}
void on_variable_edit_checkbutton_temporary_toggled(GtkToggleButton *w, gpointer) {
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category")))), gtk_toggle_button_get_active(w) ? CALCULATOR->temporaryCategory().c_str() : "");
}
void on_variable_edit_combo_category_changed(GtkComboBox *w, gpointer) {
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_temporary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_checkbutton_temporary_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_temporary")), CALCULATOR->temporaryCategory() == gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w)));
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_temporary"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_checkbutton_temporary_toggled, NULL);
}
void on_variable_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_VARIABLE, (gpointer) on_variable_edit_entry_name_changed);
	name_entry_changed();
}
void on_variable_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")))) > 0);
}
void on_variable_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(edited_variable, TYPE_VARIABLE, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog")))) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_entry_name_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_entry_name_changed, NULL);
	}
	on_variable_changed();
}
gboolean on_variable_edit_textview_value_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer) {
	if(textview_in_quotes(GTK_TEXT_VIEW(w))) return FALSE;
	const gchar *key = key_press_get_symbol(event);
	if(!key) return FALSE;
	if(strlen(key) > 0) {
		GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
		if(gtk_text_view_get_overwrite(GTK_TEXT_VIEW(w)) && !gtk_text_buffer_get_has_selection(expression_buffer)) {
			GtkTextIter ipos;
			gtk_text_buffer_get_iter_at_mark(expression_buffer, &ipos, gtk_text_buffer_get_insert(expression_buffer));
			if(!gtk_text_iter_is_end(&ipos)) {
				GtkTextIter ipos2 = ipos;
				gtk_text_iter_forward_char(&ipos2);
				gtk_text_buffer_delete(expression_buffer, &ipos, &ipos2);
			}
		} else {
			gtk_text_buffer_delete_selection(expression_buffer, FALSE, TRUE);
		}
		gtk_text_buffer_insert_at_cursor(expression_buffer, key, -1);
		return TRUE;
	}
	return FALSE;
}

GtkWidget* get_variable_edit_dialog(void) {

	if(!variableedit_builder) {

		variableedit_builder = getBuilder("variableedit.ui");
		g_assert(variableedit_builder != NULL);

		g_assert(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog") != NULL);

		g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_value"))), "changed", G_CALLBACK(on_variables_edit_textview_value_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_description"))), "changed", G_CALLBACK(on_variable_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_value"))), "changed", G_CALLBACK(on_variable_changed), NULL);

		gtk_builder_add_callback_symbols(variableedit_builder, "on_variable_changed", G_CALLBACK(on_variable_changed), "on_variable_edit_entry_name_changed", G_CALLBACK(on_variable_edit_entry_name_changed), "on_variable_edit_button_names_clicked", G_CALLBACK(on_variable_edit_button_names_clicked), "on_variable_edit_textview_value_key_press_event", G_CALLBACK(on_variable_edit_textview_value_key_press_event), "on_variable_edit_checkbutton_temporary_toggled", G_CALLBACK(on_variable_edit_checkbutton_temporary_toggled), "on_variable_edit_combo_category_changed", G_CALLBACK(on_variable_edit_combo_category_changed), NULL);
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

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog"));
}

/*
	display edit/new variable dialog
	creates new variable if v == NULL, mstruct_ is forced value, win is parent window
*/
void edit_variable(const char *category, Variable *var, MathStructure *mstruct_, GtkWindow *win) {

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
	reset_names_status();
	GtkWidget *dialog = get_variable_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

	GtkTextBuffer *value_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_value")));
	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_description")));

	if(v) {
		if(v->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Variable"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Variable (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Variable"));
	}

	//gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(variableedit_builder, "variable_edit_label_names")), "");

	gint w;
	gtk_window_get_size(GTK_WINDOW(dialog), &w, NULL);
	gtk_window_resize(GTK_WINDOW(dialog), w, 1);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(variableedit_builder, "variable_edit_tabs")), 0);
	gtk_text_buffer_set_text(description_buffer, "", -1);
	gtk_text_buffer_set_text(value_buffer, "", -1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_hidden")), false);

	if(v) {
		//fill in original parameters
		set_name_label_and_entry(v, GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
		string value_str;
		if(mstruct_) {
			value_str = get_value_string(*mstruct_, 1);
		} else if(v->isExpression()) {
			value_str = localize_expression(v->expression());
			bool is_relative = false;
			if((!v->uncertainty(&is_relative).empty() || !v->unit().empty()) && !is_relative && v->expression().find_first_not_of(NUMBER_ELEMENTS) != string::npos) {
				value_str.insert(0, 1, '(');
				value_str += ')';
			}
			if(!v->uncertainty(&is_relative).empty()) {
				if(is_relative) {
					value_str.insert(0, "(");
					value_str.insert(0, CALCULATOR->f_uncertainty->referenceName());
					value_str += CALCULATOR->getComma();
					value_str += " ";
					value_str += localize_expression(v->uncertainty());
					value_str += CALCULATOR->getComma();
					value_str += " 1)";
				} else {
					value_str += SIGN_PLUSMINUS;
					value_str += localize_expression(v->uncertainty());
				}
			} else if(v->isApproximate() && value_str.find(CALCULATOR->getFunctionById(FUNCTION_ID_INTERVAL)->referenceName()) == string::npos && value_str.find("±") == string::npos && value_str.find("+/-") == string::npos) {
				value_str = get_value_string(v->get(), 2);
			}
			if(!v->unit().empty() && v->unit() != "auto") {
				value_str += " ";
				value_str += localize_expression(v->unit(), true);
			}
		} else {
			value_str = get_value_string(v->get(), 2);
		}
		gtk_text_buffer_set_text(value_buffer, value_str.c_str(), -1);
		gtk_text_buffer_set_text(description_buffer, v->description().c_str(), -1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_hidden")), v->isHidden());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_value")), !v->isBuiltin());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_temporary")), v->category() == CALCULATOR->temporaryCategory());
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category")))), v->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_desc")), v->title(false).c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_button_ok")), FALSE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_value")), TRUE);

		//fill in default values
		string v_name;
		int i = 1;
		do {
			v_name = "v"; v_name += i2s(i);
			i++;
		} while(CALCULATOR->nameTaken(v_name));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")), v_name.c_str());
		//gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(variableedit_builder, "variable_edit_label_names")), "");
		if(mstruct_) gtk_text_buffer_set_text(value_buffer, get_value_string(*mstruct_, 1).c_str(), -1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_temporary")), !category || CALCULATOR->temporaryCategory() == category);
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category")))), !category ? CALCULATOR->temporaryCategory().c_str() : category);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_desc")), "");
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_button_ok")), TRUE);
	}

	variable_value_changed = false;

	CALCULATOR->endTemporaryStopMessages();

	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));

run_variable_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
		GtkTextIter v_iter_s, v_iter_e;
		gtk_text_buffer_get_start_iter(value_buffer, &v_iter_s);
		gtk_text_buffer_get_end_iter(value_buffer, &v_iter_e);
		gchar *gstr = gtk_text_buffer_get_text(value_buffer, &v_iter_s, &v_iter_e, FALSE);
		string str2 = unlocalize_expression(gstr);
		g_free(gstr);
		remove_blank_ends(str);
		remove_blank_ends(str2);
		if(str.empty() && (!names_status() || !has_name())) {
			//no name -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(variableedit_builder, "variable_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
			show_message(_("Empty name field."), GTK_WINDOW(dialog));
			goto run_variable_edit_dialog;
		}
		if(str2.empty()) {
			//no value -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(variableedit_builder, "variable_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_textview_value")));
			show_message(_("Empty value field."), GTK_WINDOW(dialog));
			goto run_variable_edit_dialog;
		}
		//variable with the same name exists -- overwrite or open dialog again
		if((!v || !v->hasName(str)) && ((names_status() != 1 && !str.empty()) || !has_name()) && CALCULATOR->variableNameTaken(str, v)) {
			Variable *var = CALCULATOR->getActiveVariable(str, true);
			if((!v || v != var) && (!var || var->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A unit or variable with the same name already exists.\nDo you want to overwrite it?"), GTK_WINDOW(dialog))) {
				gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(variableedit_builder, "variable_edit_tabs")), 0);
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_name")));
				goto run_variable_edit_dialog;
			}
		}
		if(!v) {
			//no need to create a new variable when a variable with the same name exists
			var = CALCULATOR->getActiveVariable(str, true);
			if(var && var->isLocal() && var->isKnown()) v = (KnownVariable*) var;
		}
		bool add_var = false;
		if(v) {
			//update existing variable
			v->setLocal(true);
			if(!v->isBuiltin()) {
				v->setApproximate(false); v->setUncertainty(""); v->setUnit("");
				if(mstruct_ && !variable_value_changed) {
					v->set(*mstruct_);
				} else {
					v->set(str2);
				}
			}
		} else {
			//new variable
			if(mstruct_ && !variable_value_changed) {
				//forced value
				v = new KnownVariable("", "", *mstruct_, "", true);
			} else {
				v = new KnownVariable("", "", str2, "", true);
			}
			add_var = true;
		}
		if(v) {
			v->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(variableedit_builder, "variable_edit_checkbutton_hidden"))));
			v->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(variableedit_builder, "variable_edit_combo_category"))));
			v->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(variableedit_builder, "variable_edit_entry_desc"))));
			set_edited_names(v, str);
			GtkTextIter d_iter_s, d_iter_e;
			gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
			gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
			gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
			v->setDescription(gstr_descr);
			g_free(gstr_descr);
			if(add_var) {
				CALCULATOR->addVariable(v);
			}
			variable_edited(v);
		}
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-variables.html#qalculate-variable-creation", GTK_WINDOW(gtk_builder_get_object(variableedit_builder, "variable_edit_dialog")));
		goto run_variable_edit_dialog;
	}
	edited_variable = NULL;
	gtk_widget_hide(dialog);
}
