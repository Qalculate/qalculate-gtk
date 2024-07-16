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
#include "nameseditdialog.h"
#include "uniteditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *unitedit_builder = NULL;

Unit *edited_unit = NULL;

enum {
	UNIT_CLASS_BASE_UNIT,
	UNIT_CLASS_ALIAS_UNIT,
	UNIT_CLASS_COMPOSITE_UNIT
};

void on_unit_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")))) > 0);
}
void on_unit_edit_entry_relation_changed(GtkEditable *w, gpointer) {
	string str = gtk_entry_get_text(GTK_ENTRY(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_reversed")), str.find("\\x") != string::npos);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), str.find("\\x") != string::npos);
}

/*
	check if entered unit name is valid, if not modify
*/
void on_unit_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_UNIT, (gpointer) on_unit_edit_entry_name_changed);
	name_entry_changed();
}

void on_unit_edit_checkbutton_mix_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_priority")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_min")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), gtk_toggle_button_get_active(w));
}

/*
	selected unit type in edit/new unit dialog has changed
*/
void on_unit_edit_combobox_class_changed(GtkComboBox *om, gpointer) {

	gtk_entry_set_icon_sensitive(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), GTK_ENTRY_ICON_SECONDARY, gtk_combo_box_get_active(om) != UNIT_CLASS_COMPOSITE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_base")), gtk_combo_box_get_active(om) != UNIT_CLASS_BASE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), gtk_combo_box_get_active(om) != UNIT_CLASS_BASE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_use_prefixes")), gtk_combo_box_get_active(om) != UNIT_CLASS_COMPOSITE_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_exp")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_relation")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_reversed")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), gtk_combo_box_get_active(om) == UNIT_CLASS_ALIAS_UNIT);
	if(gtk_combo_box_get_active(om) != UNIT_CLASS_ALIAS_UNIT || gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp"))) != 1) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), 1);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), TRUE);
	}
	on_unit_edit_checkbutton_mix_toggled(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), NULL);
}

void on_unit_edit_spinbutton_exp_value_changed(GtkSpinButton *w, gpointer) {
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class"))) != UNIT_CLASS_ALIAS_UNIT) return;
	if(gtk_spin_button_get_value_as_int(w) != 1) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_priority")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_min")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), FALSE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), TRUE);
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

void on_unit_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(edited_unit, TYPE_UNIT, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog")))) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unit_edit_entry_name_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_unit_edit_entry_name_changed, NULL);
	}
	on_unit_changed();
}

GtkWidget* get_unit_edit_dialog(void) {

	if(!unitedit_builder) {

		unitedit_builder = getBuilder("unitedit.ui");
		g_assert(unitedit_builder != NULL);

		g_assert(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), 0);

		gtk_builder_add_callback_symbols(unitedit_builder, "on_unit_changed", G_CALLBACK(on_unit_changed), "on_unit_edit_combo_system_changed", G_CALLBACK(on_unit_edit_combo_system_changed), "on_unit_edit_entry_name_changed", G_CALLBACK(on_unit_edit_entry_name_changed), "on_unit_edit_button_names_clicked", G_CALLBACK(on_unit_edit_button_names_clicked), "on_unit_edit_combobox_class_changed", G_CALLBACK(on_unit_edit_combobox_class_changed), "on_unit_edit_checkbutton_mix_toggled", G_CALLBACK(on_unit_edit_checkbutton_mix_toggled), "on_unit_edit_spinbutton_exp_value_changed", G_CALLBACK(on_unit_edit_spinbutton_exp_value_changed), "on_unit_entry_key_press_event", G_CALLBACK(on_unit_entry_key_press_event), "on_unit_edit_entry_relation_changed", G_CALLBACK(on_unit_edit_entry_relation_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), NULL);
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

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog"));
}

/*
	display edit/new unit dialog
	creates new unit if u == NULL, win is parent window
*/
void edit_unit(const char *category, Unit *u, GtkWindow *win) {

	edited_unit = u;
	reset_names_status();
	GtkWidget *dialog = get_unit_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

	if(u) {
		if(u->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Unit"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Unit (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Unit"));
	}

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(unitedit_builder, "unit_edit_textview_description")));

	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category")))), category ? category : "");

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), "");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), 1);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system")))), "");
	//gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(unitedit_builder, "unit_edit_label_names")), "");
	gtk_text_buffer_set_text(description_buffer, "", -1);

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

		set_name_label_and_entry(u, GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));

		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")), !u->isBuiltin());

		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system")))), u->system().c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_system")), !u->isBuiltin());

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_hidden")), u->isHidden());

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_use_prefixes")), u->useWithPrefixesByDefault());

		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category")))), u->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc")), u->title(false).c_str());
		gtk_text_buffer_set_text(description_buffer, u->description().c_str(), -1);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), 1);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_priority")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_min")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), FALSE);

		switch(u->subtype()) {
			case SUBTYPE_ALIAS_UNIT: {
				AliasUnit *au = (AliasUnit*) u;
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), au->firstBaseUnit()->preferredInputName(printops.abbreviate_names, true, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")).formattedName(TYPE_UNIT, true).c_str());
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), au->firstBaseExponent());
				if(au->firstBaseExponent() == 1 && !u->isBuiltin()) {
					gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), TRUE);
				}
				bool is_relative = false;
				if(au->uncertainty(&is_relative).empty()) {
					gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), localize_expression(au->expression()).c_str());
				} else if(is_relative) {
					string value = CALCULATOR->f_uncertainty->referenceName();
					value += "(";
					value += au->expression();
					value += CALCULATOR->getComma();
					value += " ";
					value += localize_expression(au->uncertainty());
					value += CALCULATOR->getComma();
					value += " 1)";
					gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), localize_expression(value).c_str());
				} else {
					gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), localize_expression(au->expression() + SIGN_PLUSMINUS + au->uncertainty()).c_str());
				}
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), localize_expression(au->inverseExpression()).c_str());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_reversed")), au->hasNonlinearExpression());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), au->hasNonlinearExpression());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp")), !u->isBuiltin());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), !u->isBuiltin());
				if(au->mixWithBase() > 0) {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), TRUE);
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), au->mixWithBase());
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), au->mixWithBaseMinimum() > 1 ? au->mixWithBaseMinimum() : 1);
					on_unit_edit_checkbutton_mix_toggled(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")), NULL);
					gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_priority")), !u->isBuiltin());
					gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_mix_min")), !u->isBuiltin());
					gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority")), !u->isBuiltin());
					gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min")), !u->isBuiltin());
				}
				break;
			}
			case SUBTYPE_COMPOSITE_UNIT: {
				PrintOptions po = printops;
				po.is_approximate = NULL;
				po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base");
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), ((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true, false).c_str());
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")), !u->isBuiltin());
			}
			default: {}
		}
	} else {
		//default values
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_hidden")), false);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_label_reversed")), false);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")), false);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), UNIT_CLASS_BASE_UNIT);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")), "1");
		on_unit_edit_combobox_class_changed(GTK_COMBO_BOX(gtk_builder_get_object(unitedit_builder, "unit_edit_combobox_class")), NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_button_ok")), TRUE);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_button_ok")), FALSE);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));

	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(unitedit_builder, "unit_edit_tabs")), 0);

run_unit_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str;
		str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));
		remove_blank_ends(str);
		if(str.empty() && (!names_status() || !has_name())) {
			//no name given
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(unitedit_builder, "unit_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_unit_edit_dialog;
		}

		//unit with the same name exists -- overwrite or open the dialog again
		if((!u || !u->hasName(str)) && ((names_status() != 1 && !str.empty()) || !has_name()) && CALCULATOR->unitNameTaken(str, u)) {
			Unit *unit = CALCULATOR->getActiveUnit(str, true);
			if((!u || u != unit) && (!unit || unit->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
				gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(unitedit_builder, "unit_edit_tabs")), 0);
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_name")));
				goto run_unit_edit_dialog;
			}
		}
		bool add_unit = false;
		Unit *old_u = u;
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
						Unit *bu = CALCULATOR->getActiveUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
						if(!bu) bu = CALCULATOR->getUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
						if(!bu) bu = CALCULATOR->getCompositeUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
						if(!bu || bu == u) {
							gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(unitedit_builder, "unit_edit_tabs")), 1);
							gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")));
							show_message(_("Base unit does not exist."), dialog);
							goto run_unit_edit_dialog;
						}
						au->setBaseUnit(bu);
						au->setApproximate(false);
						au->setExpression(unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")))));
						au->setInverseExpression(au->hasNonlinearExpression() ? unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")))) : "");
						au->setExponent(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp"))));
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
						((CompositeUnit*) u)->setBaseExpression(unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")))));
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
					Unit *bu = CALCULATOR->getActiveUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
					if(!bu) bu = CALCULATOR->getUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
					if(!bu) bu = CALCULATOR->getCompositeUnit(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base"))));
					if(!bu) {
						gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(unitedit_builder, "unit_edit_tabs")), 1);
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")));
						show_message(_("Base unit does not exist."), dialog);
						goto run_unit_edit_dialog;
					}
					u = new AliasUnit(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))), "", "", "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))), bu, unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_relation")))), gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_exp"))), unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_reversed")))), true);
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_mix")))) {
						((AliasUnit*) u)->setMixWithBase(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_priority"))));
						((AliasUnit*) u)->setMixWithBaseMinimum(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_spinbutton_mix_min"))));
					}
					break;
				}
				case UNIT_CLASS_COMPOSITE_UNIT: {
					CompositeUnit *cu = new CompositeUnit(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))), "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))), unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_base")))), true);
					u = cu;
					break;
				}
				default: {
					u = new Unit(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(unitedit_builder, "unit_edit_combo_category"))), "", "", "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(unitedit_builder, "unit_edit_entry_desc"))), true);
					break;
				}
			}
			if(old_u) {
				for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
					if(CALCULATOR->units[i]->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) CALCULATOR->units[i])->firstBaseUnit() == old_u) {
						((AliasUnit*) CALCULATOR->units[i])->setBaseUnit(u);
					} else if(CALCULATOR->units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						size_t i2 = ((CompositeUnit*) CALCULATOR->units[i])->find(old_u);
						if(i2 > 0) {
							int exp = 1; Prefix *p = NULL;
							((CompositeUnit*) CALCULATOR->units[i])->get(i2, &exp, &p);
							((CompositeUnit*) CALCULATOR->units[i])->del(i2);
							((CompositeUnit*) CALCULATOR->units[i])->add(u, exp, p);
						}
					}
				}
			}
			add_unit = true;
		}
		if(u) {
			u->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(unitedit_builder, "unit_edit_checkbutton_hidden"))));
			GtkTextIter d_iter_s, d_iter_e;
			gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
			gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
			gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
			u->setDescription(gstr_descr);
			g_free(gstr_descr);
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
			unit_edited(u);
		}
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-units.html#qalculate-unit-creation", GTK_WIDGET(gtk_builder_get_object(unitedit_builder, "unit_edit_dialog")));
		goto run_unit_edit_dialog;
	}
	edited_unit = NULL;
	gtk_widget_hide(dialog);
}
