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
#include "util.h"
#include "resultview.h"
#include "expressionedit.h"
#include "drawstructure.h"
#include "unordered_map_define.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

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

#define CALCULATE_SPACE_W		gint space_w, space_h; PangoLayout *layout_space = gtk_widget_create_pango_layout(result_view_widget(), NULL); PANGO_TTP(layout_space, " "); pango_layout_get_pixel_size(layout_space, &space_w, &space_h); g_object_unref(layout_space);

#define RESULT_SCALE_FACTOR gtk_widget_get_scale_factor(expression_edit_widget())

#define FIX_SUB_RESULT(str) \
	if(fix_supsub_result) {\
		int s = scaledown;\
		if(ips.power_depth > 0) s++;\
		if(pango_version() >= 15000) {\
			if(s <= 0) gsub("<sub>", "<span size=\"80%\" baseline_shift=\"subscript\">", str);\
			else if(s == 1) gsub("<sub>", "<span size=\"60%\" baseline_shift=\"subscript\">", str);\
			else gsub("<sub>", "<span size=\"50%\" baseline_shift=\"subscript\">", str);\
		} else {\
			PangoFontDescription *font_supsub;\
			gtk_style_context_get(gtk_widget_get_style_context(result_view_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_supsub, NULL);\
			string s_sub;\
			if(s <= 0) s_sub = "<span size=\"small\" rise=\"-";\
			else if(s == 1) s_sub = "<span size=\"x-small\" rise=\"-";\
			else s_sub = "<span size=\"xx-small\" rise=\"-";\
			s_sub += i2s(pango_font_description_get_size(font_supsub) * 0.2); s_sub += "\">";\
			if(ips.power_depth > 0) s++;\
			gsub("<sub>", s_sub, str);\
			gsub("</sub>", "</span>", str);\
		}\
		gsub("</sub>", "</span>", str);\
	}

#define FIX_SUP_RESULT(str) \
	if(fix_supsub_result) {\
		int s = scaledown;\
		if(ips.power_depth > 0) s++;\
		if(pango_version() >= 15000) {\
			if(s <= 0) gsub("<sup>", "<span size=\"80%\" baseline_shift=\"superscript\">", str);\
			else if(s == 1) gsub("<sup>", "<span size=\"60%\" baseline_shift=\"superscript\">", str);\
			else gsub("<sup>", "<span size=\"50%\" baseline_shift=\"superscript\">", str);\
		} else {\
			PangoFontDescription *font_supsub;\
			gtk_style_context_get(gtk_widget_get_style_context(result_view_widget()), GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font_supsub, NULL);\
			string s_sup;\
			if(s <= 0) s_sup = "<span size=\"small\" rise=\"";\
			else if(s == 1) s_sup = "<span size=\"x-small\" rise=\"";\
			else s_sup = "<span size=\"xx-small\" rise=\"";\
			s_sup += i2s(pango_font_description_get_size(font_supsub) * 0.5); s_sup += "\">";\
			if(ips.power_depth > 0) s++;\
			gsub("<sup>", s_sup, str);\
		}\
		gsub("</sup>", "</span>", str);\
	}

#define PAR_SPACE 1
#define PAR_WIDTH (scaledown + ips.power_depth > 1 ? par_width / 1.7 : (scaledown + ips.power_depth > 0 ? par_width / 1.4 : par_width)) + (PAR_SPACE * 2)

unordered_map<void*, string> date_map;
unordered_map<void*, bool> date_approx_map;
unordered_map<void*, string> number_map;
unordered_map<void*, string> number_base_map;
unordered_map<void*, bool> number_approx_map;
unordered_map<void*, string> number_exp_map;
unordered_map<void*, bool> number_exp_minus_map;

double par_width = 6.0;
bool fix_supsub_result = false;

vector<PangoRectangle> binary_rect;
vector<int> binary_pos;

int get_binary_result_pos(int x, int y) {
	if(!binary_rect.empty() && x >= binary_rect[0].x) {
		for(size_t i = 0; i < binary_rect.size(); i++) {
			if(x >= binary_rect[i].x && x <= binary_rect[i].x + binary_rect[i].width && y >= binary_rect[i].y && y <= binary_rect[i].y + binary_rect[i].height) {
				return binary_pos[i];
			}
		}
	}
	return -1;
}

void calculate_par_width() {
	PangoLayout *layout_test = gtk_widget_create_pango_layout(result_view_widget(), "x");
	gint h;
	pango_layout_get_pixel_size(layout_test, NULL, &h);
	par_width = h / 2.2;
	g_object_unref(layout_test);
}

void draw_font_modified() {
	fix_supsub_result = test_supsub(result_view_widget());
}

void clear_draw_caches() {
	date_map.clear();
	date_approx_map.clear();
	number_map.clear();
	number_base_map.clear();
	number_exp_map.clear();
	number_exp_minus_map.clear();
	number_approx_map.clear();
}

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
		gtk_style_context_get_color(gtk_widget_get_style_context(result_view_widget()), GTK_STATE_FLAG_NORMAL, &rgba);
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
			layout_to = gtk_widget_create_pango_layout(result_view_widget(), NULL);
			string str;
			TTB(str);
			str += CALCULATOR->localToString(false);
			TTE(str);
			pango_layout_set_markup(layout_to, str.c_str(), -1);
			pango_layout_get_pixel_size(layout_to, &to_w, &to_h);
		}
		PangoLayout *layout_where = NULL;
		if(where_struct) {
			layout_where = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

		if(can_display_unicode_string_function_exact("∠", (void*) result_view_widget())) {
			layout_sign = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
					if(po.exp_display != EXP_POWER_OF_10 && base_without_exp) {
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
					if(po.exp_display == EXP_POWER_OF_10 || base_without_exp || (po.base != BASE_DECIMAL && !BASE_IS_SEXAGESIMAL(po.base) && po.base != BASE_TIME)) {
						ips_n.exp = &exp;
						ips_n.exp_minus = &exp_minus;
						if(po.exp_display != EXP_POWER_OF_10 && base_without_exp) {
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
					if(po.indicate_infinite_series == REPEATING_DECIMALS_OVERLINE && m.number().isRational() && !m.number().isInteger() && po.base != BASE_CUSTOM && po.base != BASE_UNICODE) {
						size_t i = value_str.find("¯");
						if(i != string::npos) {
							size_t i_number_end = value_str.length();
							if(po.base == BASE_DECIMAL) {
								size_t i2 = value_str.find_first_of("Ee", i);
								if(i2 != string::npos) i_number_end = i2;
							}
							value_str.insert(i_number_end, "</span>");
							value_str.erase(i, strlen("¯"));
							value_str.insert(i, "<span overline=\"single\">");
						}
					}
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
				if(exp.empty() && po.exp_display != EXP_LOWERCASE_E && ((po.exp_display != EXP_POWER_OF_10 && po.base == BASE_DECIMAL) || BASE_IS_SEXAGESIMAL(po.base) || po.base == BASE_TIME)) {
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
					if(po.exp_display == EXP_POWER_OF_10 && ips.power_depth == 0) {
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
					PangoLayout *layout_equals = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
					if(base == BASE_BINARY && pango_version() >= 13800) str += "<span font_features=\"tnum\">";
					str += value_str;
					if(base == BASE_BINARY && pango_version() >= 13800) str += "</span>";
					if(!number_base_map[(void*) &m.number()].empty()) {
						if(!multiline) {
							string str2 = str;
							TTE(str2)
							pango_layout_set_markup(layout, str2.c_str(), -1);
							pango_layout_get_pixel_size(layout, NULL, &central_point);
						}
						if(!fix_supsub_result) {TTBP_SMALL(str)}
						str += "<sub>";
						str += number_base_map[(void*) &m.number()];
						str += "</sub>";
						FIX_SUB_RESULT(str)
						if(!fix_supsub_result) {TTE(str)}
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
							PangoLayout *layout_pos = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				PangoLayout *layout_plus = gtk_widget_create_pango_layout(result_view_widget(), NULL);
				PANGO_TTP(layout_plus, "+");
				pango_layout_get_pixel_size(layout_plus, &plus_w, &plus_h);
				PangoLayout *layout_minus = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

				PangoLayout *layout_minus = gtk_widget_create_pango_layout(result_view_widget(), NULL);

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
										layout_altmul = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
									layout_mul = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
					PangoLayout *layout_div = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

					PangoLayout *layout_sign = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

					PangoLayout *layout_sign2 = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

				PangoLayout *layout_sign = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

				PangoLayout *layout_not = gtk_widget_create_pango_layout(result_view_widget(), NULL);

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
					PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				PangoLayout *layout_comma = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				TTE(str);
				PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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

				PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
				string str;

				const ExpressionName *ename = &m.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);

				bool cursive = m.variable() != CALCULATOR->v_i && ename->name != "%" && ename->name != "‰" && ename->name != "‱" && m.variable()->referenceName() != "true" && m.variable()->referenceName() != "false";
				if(cursive) str = "<i>";
				TTBP(str);
				str += ename->formattedName(TYPE_VARIABLE, true, true, false, true, true);
				FIX_SUB_RESULT(str)
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
					PangoLayout *layout_pm = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
					PangoLayout *layout_root = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
				PangoLayout *layout_comma = gtk_widget_create_pango_layout(result_view_widget(), NULL);
				string str;
				TTP(str, po.comma())
				pango_layout_set_markup(layout_comma, str.c_str(), -1);
				pango_layout_get_pixel_size(layout_comma, &comma_w, &comma_h);
				PangoLayout *layout_function = gtk_widget_create_pango_layout(result_view_widget(), NULL);

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

				if(str.find("<sub>") != string::npos) {
					FIX_SUB_RESULT(str);
				} else if((m.function() == CALCULATOR->f_lambert_w || m.function() == CALCULATOR->f_logn) && m.size() == 2 && ((m[1].size() == 0 && (!m[1].isNumber() || (m[1].number().isInteger() && m[1].number() < 100 && m[1].number() > -100))) || (m[1].isNegate() && m[1][0].size() == 0 && (!m[1][0].isNumber() || (m[1][0].number().isInteger() && m[1][0].number() < 100 && m[1][0].number() > -100))))) {
					argcount = 1;
					str += "<sub>";
					str += m[1].print(po);
					str += "</sub>";
					FIX_SUB_RESULT(str);
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
				PangoLayout *layout = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
		PangoLayout *layout_equals = gtk_widget_create_pango_layout(result_view_widget(), NULL);
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
