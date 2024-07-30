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
#include "openhelp.h"
#include "nameseditdialog.h"
#include "variableeditdialog.h"
#include "unknowneditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *unknownedit_builder = NULL;

UnknownVariable *edited_unknown = NULL;

void on_unknown_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")))) > 0);
}
void on_unknown_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(edited_unknown, TYPE_VARIABLE, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog")))) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_entry_name_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_entry_name_changed, NULL);
	}
	on_unknown_changed();
}
void on_unknown_edit_checkbutton_custom_assumptions_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_type")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_sign")), gtk_toggle_button_get_active(w));
}
void on_unknown_edit_combobox_sign_changed(GtkComboBox *om, gpointer);
void on_unknown_edit_combobox_type_changed(GtkComboBox *om, gpointer) {
	if((gtk_combo_box_get_active(om) == 0 && (AssumptionSign) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"))) != ASSUMPTION_SIGN_NONZERO && gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"))) != ASSUMPTION_SIGN_UNKNOWN) || ((AssumptionType) gtk_combo_box_get_active(om) + 3 == ASSUMPTION_TYPE_BOOLEAN && (AssumptionSign) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"))) != ASSUMPTION_SIGN_UNKNOWN)) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_sign_changed, NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), ASSUMPTION_SIGN_UNKNOWN);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_sign_changed, NULL);
	}
}
void on_unknown_edit_combobox_sign_changed(GtkComboBox *om, gpointer) {
	if((gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"))) == 0 && (AssumptionSign) gtk_combo_box_get_active(om) != ASSUMPTION_SIGN_UNKNOWN && (AssumptionSign) gtk_combo_box_get_active(om) != ASSUMPTION_SIGN_NONZERO) || ((AssumptionType) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"))) + 3 == ASSUMPTION_TYPE_BOOLEAN && (AssumptionSign) gtk_combo_box_get_active(om) != ASSUMPTION_SIGN_UNKNOWN)) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_type_changed, NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), 1);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unknown_edit_combobox_type_changed, NULL);
	}
}

GtkWidget* get_unknown_edit_dialog(void) {

	if(!unknownedit_builder) {

		unknownedit_builder = getBuilder("unknownedit.ui");
		g_assert(unknownedit_builder != NULL);

		g_assert(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), 0);

		gtk_builder_add_callback_symbols(unknownedit_builder, "on_unknown_changed", G_CALLBACK(on_unknown_changed), "on_unknown_edit_checkbutton_custom_assumptions_toggled", G_CALLBACK(on_unknown_edit_checkbutton_custom_assumptions_toggled), "on_unknown_edit_combobox_type_changed", G_CALLBACK(on_unknown_edit_combobox_type_changed), "on_unknown_edit_combobox_sign_changed", G_CALLBACK(on_unknown_edit_combobox_sign_changed), "on_variable_edit_entry_name_changed", G_CALLBACK(on_variable_edit_entry_name_changed), "on_unknown_edit_button_names_clicked", G_CALLBACK(on_unknown_edit_button_names_clicked), NULL);
		gtk_builder_connect_signals(unknownedit_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog"));
}

void edit_unknown(const char *category, Variable *var, GtkWindow *win) {

	if(var != NULL && var->isKnown()) {
		edit_variable(category, var, NULL, win);
		return;
	}

	UnknownVariable *v = (UnknownVariable*) var;
	edited_unknown = v;
	reset_names_status();

	GtkWidget *dialog = get_unknown_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

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
		set_name_label_and_entry(v, GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), !v->isBuiltin() && v->isLocal());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), !v->isBuiltin());
		if(v->assumptions()) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_type")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_sign")), TRUE);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")),  v->assumptions()->type() < ASSUMPTION_TYPE_REAL ? 0 : v->assumptions()->type() - 3);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), v->assumptions()->sign());
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_type")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_sign")), FALSE);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), CALCULATOR->defaultAssumptions()->type() < ASSUMPTION_TYPE_REAL ? 0 : CALCULATOR->defaultAssumptions()->type() - 3);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), CALCULATOR->defaultAssumptions()->sign());
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_temporary")), v->category() == CALCULATOR->temporaryCategory());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_temporary")), !v->isBuiltin() && v->isLocal());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_button_ok")), FALSE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), TRUE);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_type")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_label_sign")), TRUE);

		//fill in default values
		string v_name;
		int i = 1;
		do {
			v_name = "v"; v_name += i2s(i);
			i++;
		} while(CALCULATOR->nameTaken(v_name));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")), v_name.c_str());
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")), CALCULATOR->defaultAssumptions()->type() < ASSUMPTION_TYPE_REAL ? 0 : CALCULATOR->defaultAssumptions()->type() - 3);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign")), CALCULATOR->defaultAssumptions()->sign());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_temporary")), category && CALCULATOR->temporaryCategory() == category);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_button_ok")), TRUE);
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
		if(str.empty() && (!names_status() || !has_name())) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_unknown_edit_dialog;
		}

		//unknown with the same name exists -- overwrite or open dialog again
		if((!v || !v->hasName(str)) && ((names_status() != 1 && !str.empty()) || !has_name()) && CALCULATOR->variableNameTaken(str, v)) {
			Variable *var = CALCULATOR->getActiveVariable(str, true);
			if((!v || v != var) && (!var || var->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_entry_name")));
				goto run_unknown_edit_dialog;
			}
		}
		if(!v) {
			//no need to create a new unknown when a unknown with the same name exists
			var = CALCULATOR->getActiveVariable(str, true);
			if(var && var->isLocal() && !var->isKnown()) v = (UnknownVariable*) var;
		}
		bool add_var = false;
		if(v) {
			//update existing unknown
			if(v->isLocal()) {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_temporary")))) {
					v->setCategory(CALCULATOR->temporaryCategory());
				} else if(v->category() == CALCULATOR->temporaryCategory()) {
					v->setCategory(CALCULATOR->getVariableById(VARIABLE_ID_X)->category());
				}
			}
		} else {
			//new unknown
			v = new UnknownVariable(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_temporary"))) ? CALCULATOR->temporaryCategory() : CALCULATOR->getVariableById(VARIABLE_ID_X)->category(), "", "", true);
			add_var = true;
		}
		if(v) {
			if(!v->isBuiltin()) {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unknownedit_builder, "unknown_edit_checkbutton_custom_assumptions")))) {
					if(!v->assumptions()) v->setAssumptions(new Assumptions());
					int type = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_type")));
					if(type == 0) type = 2;
					else type += 3;
					v->assumptions()->setType((AssumptionType) type);
					v->assumptions()->setSign((AssumptionSign) gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unknownedit_builder, "unknown_edit_combobox_sign"))));
				} else {
					v->setAssumptions(NULL);
				}
			}
			if(v->isLocal()) set_edited_names(v, str);
			if(add_var) {
				CALCULATOR->addVariable(v);
			}
			variable_edited(v);
		}
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-variables.html#qalculate-variable-creation", GTK_WIDGET(gtk_builder_get_object(unknownedit_builder, "unknown_edit_dialog")));
		goto run_unknown_edit_dialog;
	}
	edited_unknown = NULL;
	gtk_widget_hide(dialog);
}
