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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "support.h"
#include "settings.h"
#include "util.h"
#include "setbasedialog.h"
#include "expressionedit.h"
#include "expressionstatus.h"
#include "mainwindow.h"
#include "drawstructure.h"
#include "resultview.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

extern GtkBuilder *main_builder;

GtkCssProvider *resultview_provider = NULL;
GtkWidget *resultview = NULL;

cairo_surface_t *surface_result = NULL;
cairo_surface_t *surface_parsed = NULL;
cairo_surface_t *tmp_surface = NULL;
MathStructure *displayed_mstruct = NULL;
MathStructure *displayed_parsed_mstruct = NULL;
PrintOptions displayed_printops, parsed_printops;
bool displayed_caf;
GdkPixbuf *pixbuf_result = NULL;
bool result_font_updated = false;
bool first_draw_of_result = true;
bool result_too_long = false;
bool result_display_overflow = false;
bool display_aborted = false;
bool show_parsed_instead_of_result = false;
bool showing_first_time_message = false;
bool b_busy_draw = false;
int scale_n = 0;
int scale_n_bak = 0;
int binary_x_diff = 0;
int binary_y_diff = 0;

#define RESULT_SCALE_FACTOR gtk_widget_get_scale_factor(expression_edit_widget())

bool use_custom_result_font = false, save_custom_result_font = false;
string custom_result_font;

GtkWidget *result_view_widget() {
	if(!resultview) resultview = GTK_WIDGET(gtk_builder_get_object(main_builder, "resultview"));
	return resultview;
}

bool read_result_view_settings_line(string &svar, string &svalue, int &v) {
	if(svar == "use_custom_result_font") {
		use_custom_result_font = v;
	} else if(svar == "custom_result_font") {
		custom_result_font = svalue;
		save_custom_result_font = true;
	} else {
		return false;
	}
	return true;
}
void write_result_view_settings(FILE *file) {
	fprintf(file, "use_custom_result_font=%i\n", use_custom_result_font);
	if(use_custom_result_font || save_custom_result_font) fprintf(file, "custom_result_font=%s\n", custom_result_font.c_str());
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

bool parsed_expression_is_displayed_instead_of_result() {
	return parsed_in_result && !surface_result && !rpn_mode;
}

void display_parsed_instead_of_result(bool b) {
	show_parsed_instead_of_result = b;
	first_draw_of_result = false;
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
	gtk_widget_queue_draw(result_view_widget());
}

bool current_parsed_expression_is_displayed_in_result() {
	return parsed_in_result && surface_parsed;
}
void result_view_clear() {
	show_parsed_instead_of_result = false;
	showing_first_time_message = false;
	if(displayed_mstruct) {
		displayed_mstruct->unref();
		displayed_mstruct = NULL;
		if(!surface_result && !surface_parsed) gtk_widget_queue_draw(result_view_widget());
	}
	result_too_long = false;
	display_aborted = false;
	result_display_overflow = false;
	clear_draw_caches();
	if(surface_parsed) {
		cairo_surface_destroy(surface_parsed);
		surface_parsed = NULL;
		if(!surface_result) gtk_widget_queue_draw(result_view_widget());
	}
	if(surface_result) {
		cairo_surface_destroy(surface_result);
		surface_result = NULL;
		gtk_widget_queue_draw(result_view_widget());
	}
	gtk_widget_set_tooltip_text(result_view_widget(), "");
}

void on_popup_menu_item_parsed_in_result_activate(GtkMenuItem *w, gpointer) {
	set_parsed_in_result(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
bool test_can_approximate(const MathStructure &m, bool top = true) {
	if((m.isVariable() && m.variable()->isKnown()) || (m.isNumber() && !top)) return true;
	if(m.isUnit_exp()) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(test_can_approximate(m[i], false)) return true;
	}
	return false;
}
void convert_to_unit_noprefix(GtkMenuItem*, gpointer user_data) {
	ExpressionItem *u = (ExpressionItem*) user_data;
	string ceu_str = u->name();
	executeCommand(COMMAND_CONVERT_STRING, true, false, ceu_str);
	focus_keeping_selection();
}
void convert_to_currency(GtkMenuItem*, gpointer user_data) {
	Unit *u = (Unit*) user_data;
	convert_result_to_unit(u);
}
bool result_view_empty() {
	return !surface_result && !surface_parsed;
}
void result_view_clear_parsed() {
	if(surface_parsed) {
		cairo_surface_destroy(surface_parsed);
		surface_parsed = NULL;
		if(!surface_result) {
			gtk_widget_queue_draw(result_view_widget());
		}
	}
}
void draw_parsed(MathStructure &mparse, const PrintOptions &po) {
	tmp_surface = draw_structure(mparse, po, complex_angle_form, top_ips, NULL, 3, NULL, NULL, NULL, -1, true, &current_parsed_where(), &current_parsed_to());
	showing_first_time_message = false;
	if(surface_parsed) cairo_surface_destroy(surface_parsed);
	surface_parsed = tmp_surface;
	first_draw_of_result = true;
	parsed_printops = po;
	if(!displayed_parsed_mstruct) displayed_parsed_mstruct = new MathStructure();
	displayed_parsed_mstruct->set_nocopy(mparse);
	gtk_widget_queue_draw(result_view_widget());
}
void draw_result_abort() {
	b_busy_draw = false;
}

extern bool exit_in_progress;
gboolean on_resultview_draw(GtkWidget *widget, cairo_t *cr, gpointer) {
	if(exit_in_progress) return TRUE;
	gint scalefactor = gtk_widget_get_scale_factor(widget);
	gtk_render_background(gtk_widget_get_style_context(widget), cr, 0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
	result_display_overflow = false;
	if(surface_result || (surface_parsed && ((parsed_in_result && !rpn_mode) || show_parsed_instead_of_result))) {
		gint w = 0, h = 0;
		if(!first_draw_of_result) {
			if(calculator_busy()) {
				if(b_busy_draw) return TRUE;
			} else if((!surface_result || show_parsed_instead_of_result) && displayed_parsed_mstruct) {
				gint rw = gtk_widget_get_allocated_width(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result"))) - 12;
				displayed_printops.can_display_unicode_string_arg = (void*) result_view_widget();
				tmp_surface = draw_structure(*displayed_parsed_mstruct, parsed_printops, false, top_ips, NULL, 3, NULL, NULL, NULL, rw, true, &current_parsed_where(), &current_parsed_to());
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
					displayed_printops.can_display_unicode_string_arg = (void*) result_view_widget();
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
		if((surface_result && !show_parsed_instead_of_result) && (first_draw_of_result || (!calculator_busy() && result_font_updated))) {
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
				displayed_printops.can_display_unicode_string_arg = (void*) result_view_widget();
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
		PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
		GdkRGBA rgba;
		pango_layout_set_markup(layout, (string("<span size=\"smaller\">") + string(_("Type a mathematical expression above, e.g. \"5 + 2 / 3\",\nand press the enter key.")) + "</span>").c_str(), -1);
		gtk_style_context_get_color(gtk_widget_get_style_context(result_view_widget()), gtk_widget_get_state_flags(result_view_widget()), &rgba);
		cairo_move_to(cr, 6, 6);
		gdk_cairo_set_source_rgba(cr, &rgba);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);
	} else {
		gtk_widget_set_size_request(widget, -1, -1);
	}
	return TRUE;
}

void start_result_spinner() {
	gtk_spinner_start(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
}
void stop_result_spinner() {
	gtk_spinner_stop(GTK_SPINNER(gtk_builder_get_object(main_builder, "resultspinner")));
}
bool result_did_not_fit(bool only_too_long) {
	return result_too_long || (!only_too_long && result_display_overflow);
}
void set_current_displayed_result(MathStructure *m) {
	if(displayed_mstruct) displayed_mstruct->unref();
	displayed_mstruct = m;
}
void update_displayed_printops() {
	displayed_printops = printops;
	displayed_printops.allow_non_usable = true;
	if(main_builder) displayed_printops.can_display_unicode_string_arg = (void*) result_view_widget();
	displayed_caf = complex_angle_form;
}
const PrintOptions &current_displayed_printops() {
	return displayed_printops;
}

void show_result_help() {
	showing_first_time_message = true;
	gtk_widget_queue_draw(result_view_widget());
}
void redraw_result() {
	gtk_widget_queue_draw(result_view_widget());
}

cairo_surface_t *surface_result_bak = NULL;
void draw_result_backup() {
	surface_result_bak = surface_result;
}
void draw_result_clear() {
	if(surface_result) {
		surface_result = NULL;
		gtk_widget_queue_draw(result_view_widget());
	}
}
void draw_result_restore() {
	if(!surface_result && surface_result_bak) {
		surface_result = surface_result_bak;
		gtk_widget_queue_draw(result_view_widget());
	}
}
void draw_result_destroy() {
	if(surface_result) {
		cairo_surface_destroy(surface_result);
		surface_result = NULL;
			gtk_widget_queue_draw(result_view_widget());
	}
}
bool result_cleared = false;
void draw_result_pre() {
	b_busy_draw = true;
	if(surface_result) {
		cairo_surface_destroy(surface_result);
		surface_result = NULL;
		result_cleared = true;
	} else {
		result_cleared = false;
	}
	scale_n = 0;
	clear_draw_caches();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);
	tmp_surface = NULL;
	result_too_long = false;
	result_display_overflow = false;
	display_aborted = false;
}
void draw_result_waiting() {
	if(result_cleared) gtk_widget_queue_draw(result_view_widget());
}
void draw_result_temp(MathStructure &m) {
	if(!CALCULATOR->aborted()) {
		printops.allow_non_usable = true;
		printops.can_display_unicode_string_arg = (void*) result_view_widget();
		MathStructure *displayed_mstruct_pre = new MathStructure(m);
		if(printops.interval_display == INTERVAL_DISPLAY_INTERVAL) replace_interval_with_function(*displayed_mstruct_pre);
		tmp_surface = draw_structure(*displayed_mstruct_pre, printops, complex_angle_form, top_ips, NULL, 0);
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
		update_displayed_printops();
	}
}
void draw_result_check() {
	display_aborted = !tmp_surface;
	if(display_aborted) {
		gint w = 0, h = 0;
		PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
		pango_layout_set_markup(layout, _("result processing was aborted"), -1);
		pango_layout_get_pixel_size(layout, &w, &h);
		gint scalefactor = RESULT_SCALE_FACTOR;
		tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w * scalefactor, h * scalefactor);
		cairo_surface_set_device_scale(tmp_surface, scalefactor, scalefactor);
		cairo_t *cr = cairo_create(tmp_surface);
		GdkRGBA rgba;
		gtk_style_context_get_color(gtk_widget_get_style_context(result_view_widget()), gtk_widget_get_state_flags(result_view_widget()), &rgba);
		gdk_cairo_set_source_rgba(cr, &rgba);
		pango_cairo_show_layout(cr, layout);
		cairo_destroy(cr);
		g_object_unref(layout);
		*printops.is_approximate = false;
	}
}
bool draw_result_finalize() {
	surface_result = NULL;
	if(tmp_surface) {
		showing_first_time_message = false;
		first_draw_of_result = true;
		surface_result = tmp_surface;
		b_busy_draw = false;
		gtk_widget_queue_draw(result_view_widget());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), displayed_mstruct && !result_too_long && !display_aborted);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), displayed_mstruct && !result_too_long && !display_aborted);
		return true;
	}
	return false;
}
bool draw_result(MathStructure *displayed_mstruct_pre) {
	clear_draw_caches();
	printops.allow_non_usable = true;
	printops.can_display_unicode_string_arg = (void*) result_view_widget();
	if(printops.interval_display == INTERVAL_DISPLAY_INTERVAL) replace_interval_with_function(*displayed_mstruct_pre);
	tmp_surface = draw_structure(*displayed_mstruct_pre, printops, complex_angle_form, top_ips, NULL, 0);
	printops.allow_non_usable = false;
	printops.can_display_unicode_string_arg = NULL;
	if(!tmp_surface || CALCULATOR->aborted()) {
		if(tmp_surface) cairo_surface_destroy(tmp_surface);
		tmp_surface = NULL;
		displayed_mstruct_pre->unref();
		clearresult();
		return false;
	} else {
		scale_n = 0;
		showing_first_time_message = false;
		if(surface_result) cairo_surface_destroy(surface_result);
		if(displayed_mstruct) displayed_mstruct->unref();
		displayed_mstruct = displayed_mstruct_pre;
		update_displayed_printops();
		display_aborted = false;
		surface_result = tmp_surface;
		first_draw_of_result = true;
		gtk_widget_queue_draw(result_view_widget());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "menu_item_save_image")), true);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), true);
	}
	return true;
}
void draw_result_failure(MathStructure &m, bool too_long) {
	PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
	if(too_long) {
		result_too_long = true;
		pango_layout_set_markup(layout, _("result is too long\nsee history"), -1);
	} else {
		pango_layout_set_markup(layout, _("calculation was aborted"), -1);
	}
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
	gtk_style_context_get_color(gtk_widget_get_style_context(result_view_widget()), gtk_widget_get_state_flags(result_view_widget()), &rgba);
	gdk_cairo_set_source_rgba(cr, &rgba);
	if(rect.x < 0) cairo_move_to(cr, -rect.x, 0);
	pango_cairo_show_layout(cr, layout);
	cairo_destroy(cr);
	g_object_unref(layout);
	if(displayed_mstruct) displayed_mstruct->unref();
	displayed_mstruct = new MathStructure(m);
	update_displayed_printops();
}
void save_as_image() {
	if(display_aborted || !displayed_mstruct) return;
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select file to save PNG image to"), main_window(), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Save"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to save PNG image to"), main_window(), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
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
		if(((parsed_in_result && !displayed_mstruct) || show_parsed_instead_of_result) && displayed_parsed_mstruct) s = draw_structure(*displayed_parsed_mstruct, parsed_printops, complex_angle_form, top_ips, NULL, 1, &color, NULL, NULL, -1, false, &current_parsed_where(), &current_parsed_to());
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
MathStructure *current_displayed_result() {
	return displayed_mstruct;
}
void result_display_updated() {
	if(result_blocked()) return;
	displayed_printops.use_unicode_signs = printops.use_unicode_signs;
	displayed_printops.spell_out_logical_operators = printops.spell_out_logical_operators;
	displayed_printops.multiplication_sign = printops.multiplication_sign;
	displayed_printops.division_sign = printops.division_sign;
	clear_draw_caches();
	gtk_widget_queue_draw(result_view_widget());
	update_message_print_options();
	update_status_text();
	set_expression_output_updated(true);
	display_parse_status();
}
void on_popup_menu_item_calendarconversion_activate(GtkMenuItem*, gpointer) {
	open_calendarconversion();
}
void on_popup_menu_item_to_utc_activate(GtkMenuItem*, gpointer) {
	printops.time_zone = TIME_ZONE_UTC;
	result_format_updated();
	printops.time_zone = TIME_ZONE_LOCAL;
}
void on_popup_menu_item_display_normal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_PRECISION, true);
}
void on_popup_menu_item_exact_activate(GtkMenuItem *w, gpointer) {
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) set_approximation(APPROXIMATION_EXACT);
	else set_approximation(APPROXIMATION_TRY_EXACT);
}
void on_popup_menu_item_assume_nonzero_denominators_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_assume_nonzero_denominators")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_display_engineering_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_BASE_3, true);
}
void on_popup_menu_item_display_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_SCIENTIFIC, true);
}
void on_popup_menu_item_display_purely_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_PURE, true);
}
void on_popup_menu_item_display_non_scientific_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_min_exp(EXP_NONE, true);
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
	set_prefix_mode(PREFIX_MODE_NO_PREFIXES);
}
void on_popup_menu_item_display_prefixes_for_selected_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_SELECTED_UNITS);
}
void on_popup_menu_item_display_prefixes_for_all_units_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_ALL_UNITS);
}
void on_popup_menu_item_display_prefixes_for_currencies_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_prefix_mode(PREFIX_MODE_CURRENCIES);
}
void on_popup_menu_item_mixed_units_conversion_activate(GtkMenuItem *w, gpointer) {
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_mixed_units_conversion")), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}
void on_popup_menu_item_fraction_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_DECIMAL);
}
void on_popup_menu_item_fraction_decimal_exact_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_DECIMAL_EXACT);
}
void on_popup_menu_item_fraction_combined_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_COMBINED);
}
void on_popup_menu_item_fraction_fraction_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_fraction_format(FRACTION_FRACTIONAL);
}
void on_popup_menu_item_binary_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_BINARY);
}
void on_popup_menu_item_roman_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_ROMAN_NUMERALS);
}
void on_popup_menu_item_sexagesimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_SEXAGESIMAL);
}
void on_popup_menu_item_time_format_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_TIME);
}
void on_popup_menu_item_octal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_OCTAL);
}
void on_popup_menu_item_decimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_DECIMAL);
}
void on_popup_menu_item_duodecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_DUODECIMAL);
}
void on_popup_menu_item_hexadecimal_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	set_output_base(BASE_HEXADECIMAL);
}
void on_popup_menu_item_custom_base_activate(GtkMenuItem *w, gpointer) {
	if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
	open_setbase(main_window(), true, false);
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
	insert_matrix(current_result(), main_window(), false, false, true);
}
void on_popup_menu_item_view_vector_activate(GtkMenuItem*, gpointer) {
	insert_matrix(current_result(), main_window(), true, false, true);
}
void on_popup_menu_item_copy_activate(GtkMenuItem*, gpointer) {
	copy_result(0, (((parsed_in_result && !displayed_mstruct) || show_parsed_instead_of_result) && displayed_parsed_mstruct) ? 8 : 0);
}
void on_popup_menu_item_copy_ascii_activate(GtkMenuItem*, gpointer) {
	copy_result(1, (((parsed_in_result && !displayed_mstruct) || show_parsed_instead_of_result) && displayed_parsed_mstruct) ? 8 : 0);
}
void on_menu_item_show_parsed_activate(GtkMenuItem*, gpointer) {
	show_parsed(true);
}
void on_menu_item_show_result_activate(GtkMenuItem*, gpointer) {
	show_parsed(false);
}

extern Unit *latest_button_currency;
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
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_parsed_in_result_activate, NULL);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result")), TRUE);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(main_builder, "result_popup_menu_item_parsed_in_result"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_popup_menu_item_parsed_in_result_activate, NULL);
	}
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "result_popup_menu_item_meta_modes")), !b_parsed);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_result_popup_modes")), !b_parsed);
	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save")), !b_parsed);

	bool b_unit = !b_parsed && displayed_mstruct && contains_convertible_unit(*displayed_mstruct);
	bool b_date = !b_parsed && displayed_mstruct && displayed_mstruct->isDateTime();
	bool b_complex = !b_parsed && displayed_mstruct && current_result() && (contains_imaginary_number(*current_result()) || current_result()->containsFunctionId(FUNCTION_ID_CIS));
	bool b_rational = !b_parsed && displayed_mstruct && current_result() && contains_rational_number(*current_result());
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
		if(!u_result && current_result()) u_result = CALCULATOR->findMatchingUnit(*current_result());
		bool b_prefix = false;
		if(b_exact && u_result && u_result->subtype() != SUBTYPE_COMPOSITE_UNIT) b_prefix = contains_prefix(displayed_mstruct ? *displayed_mstruct : *current_result());
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
				MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us[i], convert_to_currency)
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
					MENU_ITEM_WITH_OBJECT_AND_FLAG(to_us2[i], convert_to_currency)
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

	if(!b_parsed && current_result() && current_result()->containsUnknowns()) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_unknowns")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_set_unknowns")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	}
	if(!b_parsed && current_result() && current_result()->containsType(STRUCT_ADDITION)) {
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
		if(contains_polynomial_division(*current_result())) gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_expand_partial_fractions")));
		else gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_expand_partial_fractions")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
	} else {
		if(!b_parsed && current_result() && current_result()->isNumber() && current_result()->number().isInteger() && !current_result()->number().isZero()) {
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_factorize")));
		} else {
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_factorize")));
		}
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_simplify")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_expand_partial_fractions")));
	}
	if(!b_parsed && current_result() && (current_result()->isApproximate() || test_can_approximate(*current_result()))) {
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_exact")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_nonzero")));
		if(!current_result()->isApproximate() && current_result()->containsDivision()) gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
		else gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_exact")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_assume_nonzero_denominators")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(main_builder, "separator_popup_nonzero")));
	}
	if(!b_parsed && current_result()->isVector() && (current_result()->size() != 1 || !(*current_result())[0].isVector() || (*current_result())[0].size() > 0)) {
		if(current_result()->isMatrix()) {
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
gboolean on_resultspinner_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	if(event->button != 1 || !gtk_widget_is_visible(w)) return FALSE;
	abort_calculation();
	return TRUE;
}
guint32 prev_result_press_time = 0;
gboolean on_resultview_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(calculator_busy()) return FALSE;
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		update_resultview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	if(event->button == 1 && event->time > prev_result_press_time + 100 && surface_result && !show_parsed_instead_of_result && event->x >= gtk_widget_get_allocated_width(result_view_widget()) - cairo_image_surface_get_width(surface_result) - 20) {
		gint x = event->x - binary_x_diff;
		gint y = event->y - binary_y_diff;
		int binary_pos = get_binary_result_pos(x, y);
		if(binary_pos >= 0) {
			prev_result_press_time = event->time;
			toggle_binary_pos(binary_pos);
			return TRUE;
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
	if(calculator_busy()) return TRUE;
	update_resultview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_resultview")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
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
			gtk_style_context_get(gtk_widget_get_style_context(result_view_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
			char *gstr = pango_font_description_to_string(font_desc);
			custom_result_font = gstr;
			g_free(gstr);
			pango_font_description_free(font_desc);
		}
	}
	if(initial) {
		draw_font_modified();
	} else {
		result_font_modified();
		gtk_widget_get_size_request(GTK_WIDGET(gtk_builder_get_object(main_builder, "scrolled_result")), NULL, &h_new);
		gint winh, winw;
		gtk_window_get_size(main_window(), &winw, &winh);
		winh += (h_new - h_old);
		gtk_window_resize(main_window(), winw, winh);
	}
}
void set_result_font(const char *str) {
	if(!str) {
		use_custom_result_font = false;
	} else {
		use_custom_result_font = true;
		if(custom_result_font != str) {
			save_custom_result_font = true;
			custom_result_font = str;
		}
	}
	update_result_font(false);
}
const char *result_font(bool return_default) {
	if(!return_default && !use_custom_result_font) return NULL;
	return custom_result_font.c_str();
}
void result_font_modified() {
	while(gtk_events_pending()) gtk_main_iteration();
	draw_font_modified();
	set_result_size_request();
	result_font_updated = true;
	result_display_updated();
}
void set_result_size_request() {
	MathStructure mtest;
	MathStructure m1("Ü");
	MathStructure mden("y"); mden ^= m1;
	mtest = m1; mtest ^= m1; mtest.transform(STRUCT_DIVISION, mden);
	mtest.transform(CALCULATOR->f_sqrt);
	mtest.transform(CALCULATOR->f_abs);
	PrintOptions po;
	po.can_display_unicode_string_function = &can_display_unicode_string_function;
	po.can_display_unicode_string_arg = (void*) result_view_widget();
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
	calculate_par_width();
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

void create_result_view() {
	resultview_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(result_view_widget()), GTK_STYLE_PROVIDER(resultview_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	if(RUNTIME_CHECK_GTK_VERSION(3, 16)) {
		GtkCssProvider *result_color_provider = gtk_css_provider_new();
		gtk_css_provider_load_from_data(result_color_provider, "* {color: @theme_text_color;}", -1, NULL);
		gtk_style_context_add_provider(gtk_widget_get_style_context(result_view_widget()), GTK_STYLE_PROVIDER(result_color_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 1);
	}

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(gtk_builder_get_object(main_builder, "scrolled_result")), false);
#endif

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
	gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
#else
	gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
	gtk_widget_set_margin_left(GTK_WIDGET(gtk_builder_get_object(main_builder, "resultport")), 6);
#endif

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_save_image")), FALSE);

	update_result_font(true);
	set_result_size_request();

	gtk_builder_add_callback_symbols(main_builder, "on_resultview_button_press_event", G_CALLBACK(on_resultview_button_press_event), "on_resultview_popup_menu", G_CALLBACK(on_resultview_popup_menu), "on_resultview_draw", G_CALLBACK(on_resultview_draw), "on_popup_menu_item_copy_activate", G_CALLBACK(on_popup_menu_item_copy_activate), "on_popup_menu_item_copy_ascii_activate", G_CALLBACK(on_popup_menu_item_copy_ascii_activate), "on_popup_menu_item_exact_activate", G_CALLBACK(on_popup_menu_item_exact_activate), "on_popup_menu_item_assume_nonzero_denominators_activate", G_CALLBACK(on_popup_menu_item_assume_nonzero_denominators_activate), "on_popup_menu_item_display_normal_activate", G_CALLBACK(on_popup_menu_item_display_normal_activate), "on_popup_menu_item_display_engineering_activate", G_CALLBACK(on_popup_menu_item_display_engineering_activate), "on_popup_menu_item_display_scientific_activate", G_CALLBACK(on_popup_menu_item_display_scientific_activate), "on_popup_menu_item_display_purely_scientific_activate", G_CALLBACK(on_popup_menu_item_display_purely_scientific_activate), "on_popup_menu_item_display_non_scientific_activate", G_CALLBACK(on_popup_menu_item_display_non_scientific_activate), "on_popup_menu_item_complex_rectangular_activate", G_CALLBACK(on_popup_menu_item_complex_rectangular_activate), "on_popup_menu_item_complex_exponential_activate", G_CALLBACK(on_popup_menu_item_complex_exponential_activate), "on_popup_menu_item_complex_polar_activate", G_CALLBACK(on_popup_menu_item_complex_polar_activate), "on_popup_menu_item_complex_angle_activate", G_CALLBACK(on_popup_menu_item_complex_angle_activate), "on_popup_menu_item_binary_activate", G_CALLBACK(on_popup_menu_item_binary_activate), "on_popup_menu_item_octal_activate", G_CALLBACK(on_popup_menu_item_octal_activate), "on_popup_menu_item_decimal_activate", G_CALLBACK(on_popup_menu_item_decimal_activate), "on_popup_menu_item_duodecimal_activate", G_CALLBACK(on_popup_menu_item_duodecimal_activate), "on_popup_menu_item_hexadecimal_activate", G_CALLBACK(on_popup_menu_item_hexadecimal_activate), "on_popup_menu_item_sexagesimal_activate", G_CALLBACK(on_popup_menu_item_sexagesimal_activate), "on_popup_menu_item_time_format_activate", G_CALLBACK(on_popup_menu_item_time_format_activate), "on_popup_menu_item_roman_activate", G_CALLBACK(on_popup_menu_item_roman_activate), "on_popup_menu_item_custom_base_activate", G_CALLBACK(on_popup_menu_item_custom_base_activate), "on_popup_menu_item_fraction_decimal_activate", G_CALLBACK(on_popup_menu_item_fraction_decimal_activate), "on_popup_menu_item_fraction_decimal_exact_activate", G_CALLBACK(on_popup_menu_item_fraction_decimal_exact_activate), "on_popup_menu_item_fraction_fraction_activate", G_CALLBACK(on_popup_menu_item_fraction_fraction_activate), "on_popup_menu_item_fraction_combined_activate", G_CALLBACK(on_popup_menu_item_fraction_combined_activate), "on_popup_menu_item_abbreviate_names_activate", G_CALLBACK(on_popup_menu_item_abbreviate_names_activate), "on_popup_menu_item_mixed_units_conversion_activate", G_CALLBACK(on_popup_menu_item_mixed_units_conversion_activate), "on_popup_menu_item_to_utc_activate", G_CALLBACK(on_popup_menu_item_to_utc_activate), "on_popup_menu_item_calendarconversion_activate", G_CALLBACK(on_popup_menu_item_calendarconversion_activate), "on_popup_menu_item_display_no_prefixes_activate", G_CALLBACK(on_popup_menu_item_display_no_prefixes_activate), "on_popup_menu_item_display_prefixes_for_selected_units_activate", G_CALLBACK(on_popup_menu_item_display_prefixes_for_selected_units_activate), "on_popup_menu_item_display_prefixes_for_currencies_activate", G_CALLBACK(on_popup_menu_item_display_prefixes_for_currencies_activate), "on_popup_menu_item_display_prefixes_for_all_units_activate", G_CALLBACK(on_popup_menu_item_display_prefixes_for_all_units_activate), "on_popup_menu_item_all_prefixes_activate", G_CALLBACK(on_popup_menu_item_all_prefixes_activate), "on_popup_menu_item_denominator_prefixes_activate", G_CALLBACK(on_popup_menu_item_denominator_prefixes_activate), "on_popup_menu_item_view_matrix_activate", G_CALLBACK(on_popup_menu_item_view_matrix_activate), "on_popup_menu_item_view_vector_activate", G_CALLBACK(on_popup_menu_item_view_vector_activate), "on_popup_menu_item_parsed_in_result_activate", G_CALLBACK(on_popup_menu_item_parsed_in_result_activate), "on_menu_item_show_parsed_activate", G_CALLBACK(on_menu_item_show_parsed_activate), "on_menu_item_show_result_activate", G_CALLBACK(on_menu_item_show_result_activate), "on_resultspinner_button_press_event", G_CALLBACK(on_resultspinner_button_press_event), NULL);

}
