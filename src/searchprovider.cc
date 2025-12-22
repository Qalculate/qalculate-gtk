/*
    Qalculate (gnome shell search provider)

    Copyright (C) 2020, 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <sys/stat.h>

#include "support.h"

#include <libqalculate/qalculate.h>

#include "gnome-search-provider2.h"

#include "unordered_map_define.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

#define QALCULATE_TYPE_SEARCH_PROVIDER qalculate_search_provider_get_type()
#define QALCULATE_SEARCH_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), QALCULATE_TYPE_SEARCH_PROVIDER, QalculateSearchProvider))

unordered_map<string, string> expressions;
unordered_map<string, string> results;

EvaluationOptions search_eo;
PrintOptions search_po;
bool search_complex_angle_form = false;
bool search_ignore_locale = false;
bool search_adaptive_interval_display = true;
bool search_do_imaginary_j = false;
string search_default_currency;

typedef struct _QalculateSearchProvider QalculateSearchProvider;
typedef GObjectClass QalculateSearchProviderClass;

struct _QalculateSearchProvider {
	GObject parent_instance;
	ShellSearchProvider2 *skeleton;
};

G_DEFINE_TYPE(QalculateSearchProvider, qalculate_search_provider, G_TYPE_OBJECT);

static void qalculate_search_provider_class_init(QalculateSearchProviderClass*) {}
static QalculateSearchProvider *qalculate_search_provider_new(void) {
	return QALCULATE_SEARCH_PROVIDER(g_object_new(qalculate_search_provider_get_type(), NULL));
}

bool has_error() {
	while(CALCULATOR->message()) {
		if(CALCULATOR->message()->type() == MESSAGE_ERROR) {
			CALCULATOR->clearMessages();
			return true;
		}
		CALCULATOR->nextMessage();
	}
	return false;
}

static gboolean qalculate_search_provider_activate_result(ShellSearchProvider2*, GDBusMethodInvocation *invocation, gchar *result, gchar **terms, guint, gpointer) {
	if(strcmp(result, "copy-to-clipboard") != 0) {
		gchar *joined_terms = g_strjoinv(" ", terms);
		string str = "qalculate-gtk \"";
		str += joined_terms;
		str += "\"";
		g_spawn_command_line_async(str.c_str(), NULL);
		g_free(joined_terms);
	}
	g_dbus_method_invocation_return_value(invocation, NULL);
	return TRUE;
}

bool use_with_prefix(Unit *u, Prefix *prefix) {
	if(!prefix) return true;
	if(!u->useWithPrefixesByDefault()) return false;
	if((prefix->type() != PREFIX_DECIMAL || u->minPreferredPrefix() > ((DecimalPrefix*) prefix)->exponent() || u->maxPreferredPrefix() < ((DecimalPrefix*) prefix)->exponent()) && (prefix->type() != PREFIX_BINARY || u->baseUnit()->referenceName() != "bit" || u->minPreferredPrefix() > ((BinaryPrefix*) prefix)->exponent() || u->maxPreferredPrefix() < ((BinaryPrefix*) prefix)->exponent())) {
		return false;
	}
	return true;
}
bool test_autocalculatable_search(const MathStructure &m, bool top = true) {
	if(m.isFunction()) {
		if(m.size() < (size_t) m.function()->minargs() && (m.size() != 1 || m[0].representsScalar())) {
			return false;
		}
		if(m.function()->id() == FUNCTION_ID_SAVE || m.function()->id() == FUNCTION_ID_PLOT || m.function()->id() == FUNCTION_ID_EXPORT || m.function()->id() == FUNCTION_ID_LOAD || m.function()->id() == FUNCTION_ID_COMMAND) return false;
		if(m.size() > 0 && (m.function()->id() == FUNCTION_ID_FACTORIAL || m.function()->id() == FUNCTION_ID_DOUBLE_FACTORIAL || m.function()->id() == FUNCTION_ID_MULTI_FACTORIAL) && m[0].isInteger() && m[0].number().integerLength() > 17) {
			return false;
		}
		if(m.function()->id() == FUNCTION_ID_LOGN && m.size() == 2 && m[0].isUndefined() && m[1].isNumber()) return false;
		if(top && m.function()->subtype() == SUBTYPE_DATA_SET && m.size() >= 2 && m[1].isSymbolic() && equalsIgnoreCase(m[1].symbol(), "info")) return false;

	} else if(m.isUnit() && !use_with_prefix(m.unit(), m.prefix())) {
		return false;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(!test_autocalculatable_search(m[i], false)) return false;
	}
	return true;
}

bool contains_fraction_search(const MathStructure &m) {
	if(m.isNumber()) return !m.number().isInteger();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_fraction_search(m[i])) return true;
	}
	return false;
}

bool test_search_result(const MathStructure &m, bool top = true) {
	if(m.isNumber()) {
		if(m.number().isFloatingPoint() && (mpfr_get_exp(m.number().internalUpperFloat()) > 10000000L || mpfr_get_exp(m.number().internalLowerFloat()) < -10000000L)) {
			return false;
		} else if(m.number().isInteger() && ::abs(m.number().integerLength()) > 10000000L) {
			return false;
		}
	} else if(m.isFunction() && !m.containsUnknowns()) {
		return false;
	} else if(m.isPower() && ((m[0].isUnit() && !m[1].isInteger()) || m[1].containsType(STRUCT_UNIT, false, false, true) > 0)) {
		return false;
	} else if(!top && m.isVector()) {
		return false;
	} else if(m.isAddition() && m.size() >= 2) {
		if(m[m.size() - 1].isUnitCompatible(m[m.size() - 2]) == 0) {
			const MathStructure *u1 = NULL, *u2 = NULL;
			if(m[m.size() - 1].isUnit_exp()) u1 = m.getChild(m.size());
			else if(m[m.size() - 1].isMultiplication() && m[m.size() - 1].last().isUnit_exp()) u1 = m[m.size() - 1].getChild(m[m.size() - 1].size());
			if(m[m.size() - 2].isUnit()) u2 = m.getChild(m.size() - 1);
			else if(m[m.size() - 2].isMultiplication() && m[m.size() - 2].last().isUnit_exp()) u2 = m[m.size() - 2].getChild(m[m.size() - 2].size());
			if(u1 && u1->isPower()) u1 = u1->base();
			if(u2 && u2->isPower()) u2 = u2->base();
			if(!u1 || !u2 || u1->unit() == u2->unit() || u1->unit()->baseUnit() != u2->unit()->baseUnit()) return false;
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(!test_search_result(m[i], top && m.isVector())) return false;
	}
	return true;
}


void handle_terms(gchar *joined_terms, GVariantBuilder &builder) {
	string expression = joined_terms;
	g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
	remove_blank_ends(expression);
	if(expression.empty()) return;
	unordered_map<string, string>::const_iterator it = expressions.find(expression);
	if(it != expressions.end()) {
		if(it->second.empty()) return;
		g_variant_builder_add(&builder, "s", expression.c_str());
		it = results.find(it->second);
		if(it != results.end() && !it->second.empty()) g_variant_builder_add(&builder, "s", (string("copy-to-clipboard") + it->second).c_str());
	} else {
		bool b_valid = true;
		expressions[expression] = "";
		if(CALCULATOR->busy()) CALCULATOR->abort();
		string str = CALCULATOR->unlocalizeExpression(expression, search_eo.parse_options);
		string str_to, str_where;
		remove_blank_ends(str);
		if(!CALCULATOR->parseComments(str, search_eo.parse_options).empty() || str.empty()) return;
		bool b_to = CALCULATOR->separateToExpression(str, str_to, search_eo);
		CALCULATOR->separateWhereExpression(str, str_where, search_eo);
		if(str.empty()) return;
		size_t i = str.find_first_of(SPACES);
		bool do_factors = false, do_pfe = false, do_calendars = false, do_bases = false, do_expand = false;
		if(i != string::npos) {
			string str1 = str.substr(0, i);
			if(str1 == "factor" || equalsIgnoreCase(str1, "factorize") || equalsIgnoreCase(str1, _("factorize"))) {
				str = str.substr(i + 1);
				b_to = true;
				do_factors = true;
			} else if(equalsIgnoreCase(str1, "expand") || equalsIgnoreCase(str_to, _("expand"))) {
				str = str.substr(i + 1);
				b_to = true;
				do_expand = true;
			}
		}
		MathStructure m;
		CALCULATOR->startControl(100);
		if(!str_where.empty()) {
			MathStructure m_where;
			CALCULATOR->parseExpressionAndWhere(&m, &m_where, str, str_where, search_eo.parse_options);
			if(has_error() || !test_autocalculatable_search(m_where)  || (!m_where.isComparison() && !m_where.isLogicalAnd() && !m_where.isLogicalOr())) {
				b_valid = false;
			} else {
				CALCULATOR->parseSigns(str_where);
				if(is_in(OPERATORS "\\" LEFT_PARENTHESIS LEFT_VECTOR_WRAP, str_where[str_where.length() - 1]) && str_where[str_where.length() - 1] != '!') {
					b_valid = false;
				}
			}
		} else {
			CALCULATOR->parse(&m, str, search_eo.parse_options);
		}
		if(!b_valid) {CALCULATOR->stopControl(); return;}
		if(has_error() || !test_autocalculatable_search(m)) b_valid = false;
		else if(str.length() > 3 && str.find(" to", str.length() - 3) != std::string::npos) b_valid = false;
		else if(str.length() > 6 && str.find(" where", str.length() - 6) != std::string::npos) b_valid = false;
		else {
			string str2 = str;
			CALCULATOR->parseSigns(str2);
			if(is_in(OPERATORS "\\" LEFT_PARENTHESIS LEFT_VECTOR_WRAP, str2[str2.length() - 1]) && str2[str2.length() - 1] != '!') {
				b_valid = false;
			} else if(!b_to && m.isNumber() && expression.find_first_not_of(NUMBERS) == string::npos) {
				b_valid = false;
			} else if(str_where.empty() && m.isVariable() && !m.variable()->isKnown()) {
				b_valid = false;
			} else if(m.isMultiplication() && str2.find_first_of(OPERATORS PARENTHESISS) == string::npos) {
				bool first_digit = str2[0] > '0' && str2[0] <= '9';
				for(size_t i2 = 1; i2 < m.size(); i2++) {
					if((!first_digit && !m[i2].isUnit() && str_to.empty() && str_where.empty()) || m[i2].isNumber()) {
						b_valid = false;
						break;
					}
				}
			} else if(m.isFunction() && m.size() > 0 && str2.find_first_of(NOT_IN_NAMES) == string::npos) {
				b_valid = false;
			}
		}
		if(!b_valid) {CALCULATOR->stopControl(); return;}
		if(!str_where.empty()) {str += "/."; str += str_where;}
		EvaluationOptions eo = search_eo; PrintOptions po = search_po;
		po.number_fraction_format = FRACTION_DECIMAL;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		Number custom_base;
		bool complex_angle_form = false;
		int binary_prefixes = -1;
		if(!str_to.empty()) {
			bool do_factors2 = false;
			str_to = CALCULATOR->parseToExpression(str_to, eo, po, &custom_base, &binary_prefixes, &complex_angle_form, &do_factors2, &do_pfe, &do_calendars, &do_bases);
			if(!str_to.empty()) {str += "->"; str += str_to;}
			if(do_factors2) do_factors = true;
			if(do_calendars || do_bases) return;
			remove_blank_ends(str_to);
		}
		if(!str_to.empty() && str_to.find_first_not_of(NUMBER_ELEMENTS SPACE) != string::npos) {
			if(str_to[0] == '0' || str_to[0] == '?' || str_to[0] == '+' || str_to[0] == '-') {
				str_to = str_to.substr(1, str_to.length() - 1);
				remove_blank_ends(str_to);
			} else if(str_to.length() > 1 && str_to[1] == '?' && (str_to[0] == 'b' || str_to[0] == 'a' || str_to[0] == 'd')) {
				str_to = str_to.substr(2, str_to.length() - 2);
				remove_blank_ends(str_to);
			}
			MathStructure mparse_to;
			Unit *u = CALCULATOR->getActiveUnit(str_to);
			if(!u) u = CALCULATOR->getCompositeUnit(str_to);
			Variable *v = NULL;
			if(!u) v = CALCULATOR->getActiveVariable(str_to);
			if(v && !v->isKnown()) v = NULL;
			Prefix *p = NULL;
			if(!u && !v && CALCULATOR->unitNameIsValid(str_to)) p = CALCULATOR->getPrefix(str_to);
			if(!u && !v && !p) {
				CALCULATOR->beginTemporaryStopMessages();
				CompositeUnit cu("", search_eo.parse_options.limit_implicit_multiplication ? "01" : "00", "", str_to);
				if(CALCULATOR->endTemporaryStopMessages()) b_valid = false;
			}
		}
		if(!b_valid || CALCULATOR->aborted()) {CALCULATOR->stopControl(); return;}
		bool result_is_comparison = false;
		string parsed;
		int max_length = 100 - unicode_length(str);
		if(max_length < 50) max_length = 50;

		MathStructure mparse;
		m = CALCULATOR->calculate(str, eo, &mparse);
		if(CALCULATOR->aborted()) m.setAborted();
		if(m.size() > 50 || m.countTotalChildren(false) > 500 || (m.isMatrix() && m.rows() * m.columns() > 50) || !test_search_result(m)) {CALCULATOR->stopControl(); return;}

		if(!b_to && ((eo.approximation == APPROXIMATION_EXACT && eo.auto_post_conversion != POST_CONVERSION_NONE) || eo.auto_post_conversion == POST_CONVERSION_OPTIMAL)) {
			convert_unchanged_quantity_with_unit(mparse, m, eo);
		}

		MathStructure mexact;
		mexact.setUndefined();
		if(!CALCULATOR->aborted() && po.base == BASE_DECIMAL) {
			bool b = true;
			if(b) {
				calculate_dual_exact(mexact, &m, str, &mparse, eo, AUTOMATIC_APPROXIMATION_AUTO, 0, max_length);
				if(CALCULATOR->aborted()) {
					mexact.setUndefined();
					CALCULATOR->stopControl();
					CALCULATOR->startControl(20);
				}
			}
		}

		if(do_factors) {
			if((m.isNumber() || m.isVector()) && po.number_fraction_format == FRACTION_DECIMAL) {
				po.restrict_fraction_length = false;
				po.number_fraction_format = FRACTION_FRACTIONAL;
			}
			m.integerFactorize();
			mexact.integerFactorize();
		} else if(do_expand) {
			m.expand(eo, false);
		}

		if(do_pfe) {
			m.expandPartialFractions(eo);
			mexact.expandPartialFractions(eo);
		}
		if(po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR && !contains_fraction_search(m)) po.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;

		po.allow_factorization = po.allow_factorization || eo.structuring == STRUCTURING_FACTORIZE || do_factors;

		bool exact_comparison = false;
		vector<string> alt_results;
		string result;
		if(!m.isAborted()) {
			Number base_save;
			if(!custom_base.isZero()) {
				base_save = CALCULATOR->customOutputBase();
				CALCULATOR->setCustomOutputBase(custom_base);
			}
			print_dual(m, str, mparse, mexact, result, alt_results, po, eo, po.number_fraction_format == FRACTION_DECIMAL ? AUTOMATIC_FRACTION_AUTO : AUTOMATIC_FRACTION_OFF, AUTOMATIC_APPROXIMATION_AUTO, complex_angle_form, &exact_comparison, true, false, false, TAG_TYPE_HTML, max_length, b_to);
			if(!alt_results.empty()) {
				bool use_par = m.isComparison() || m.isLogicalAnd() || m.isLogicalOr();
				str = result; result = "";
				size_t equals_length = 3;
				if(!po.use_unicode_signs && (m.isApproximate() || *po.is_approximate)) equals_length += 1 + strlen(_("approx."));
				size_t l = 0;
				if(max_length > 0) l += unicode_length(str);
				for(size_t i = 0; i < alt_results.size();) {
					if(max_length > 0) l += unicode_length(alt_results[i]) + equals_length;
					if((max_length > 0 && l > (size_t) max_length) || alt_results[i] == CALCULATOR->timedOutString()) {
						alt_results.erase(alt_results.begin() + i);
					} else {
						if(i > 0) result += " = ";
						if(use_par) result += LEFT_PARENTHESIS;
						result += alt_results[i];
						if(use_par) result += RIGHT_PARENTHESIS;
						i++;
					}
				}
				if(alt_results.empty()) {
					result += str;
				} else {
					if(m.isApproximate() || *po.is_approximate) {
						if(po.use_unicode_signs) {
							result += " " SIGN_ALMOST_EQUAL " ";
						} else {
							result += " = ";
							result += _("approx.");
							result += " ";
						}
					} else {
						result += " = ";
					}
					if(use_par) result += LEFT_PARENTHESIS;
					result += str;
					if(use_par) result += RIGHT_PARENTHESIS;
				}
			} else if((m.isComparison() || m.isLogicalOr() || m.isLogicalAnd()) && (!CALCULATOR->aborted() || result != CALCULATOR->timedOutString())) {
				result_is_comparison = true;
			}
			if(!custom_base.isZero()) CALCULATOR->setCustomOutputBase(base_save);
		}
		if(CALCULATOR->aborted()) {
			CALCULATOR->stopControl();
			CALCULATOR->startControl(20);
		}
		PrintOptions po_parsed;
		po_parsed.preserve_format = true;
		po_parsed.show_ending_zeroes = false;
		po_parsed.lower_case_e = po.lower_case_e;
		po_parsed.lower_case_numbers = po.lower_case_numbers;
		po_parsed.base_display = po.base_display;
		po_parsed.twos_complement = po.twos_complement;
		po_parsed.hexadecimal_twos_complement = po.hexadecimal_twos_complement;
		po_parsed.base = eo.parse_options.base;
		Number nr_base;
		if(po_parsed.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
			nr_base = CALCULATOR->customOutputBase();
			CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
		} else if(po_parsed.base == BASE_CUSTOM || (po_parsed.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po_parsed.base != BASE_UNICODE)) {
			po_parsed.base = 10;
			po_parsed.min_exp = 6;
			po_parsed.use_max_decimals = true;
			po_parsed.max_decimals = 5;
			po_parsed.preserve_format = false;
		}
		po_parsed.abbreviate_names = false;
		po_parsed.digit_grouping = po.digit_grouping;
		po_parsed.use_unicode_signs = po.use_unicode_signs;
		po_parsed.multiplication_sign = po.multiplication_sign;
		po_parsed.division_sign = po.division_sign;
		po_parsed.short_multiplication = false;
		po_parsed.excessive_parenthesis = true;
		po_parsed.improve_division_multipliers = false;
		po_parsed.restrict_to_parent_precision = false;
		po_parsed.spell_out_logical_operators = po.spell_out_logical_operators;
		po_parsed.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		if(po_parsed.base == BASE_CUSTOM) {
			CALCULATOR->setCustomOutputBase(nr_base);
		}
		mparse.format(po_parsed);
		parsed = mparse.print(po_parsed);
		CALCULATOR->stopControl();
		if(po.is_approximate && (exact_comparison || !alt_results.empty())) *po.is_approximate = false;
		else if(po.is_approximate && m.isApproximate()) *po.is_approximate = true;
		if(has_error() || result.empty() || parsed.find(CALCULATOR->abortedMessage()) != string::npos || parsed.find(CALCULATOR->timedOutString()) != string::npos) {
			return;
		}
		if(result.find(CALCULATOR->abortedMessage()) != string::npos || result.find(CALCULATOR->timedOutString()) != string::npos) {
			if(parsed.empty()) return;
			result = "";
		}
		expressions[expression] = parsed;
		g_variant_builder_add(&builder, "s", expression.c_str());
		if(parsed.empty() || (result == parsed && !m.isNumber())) {
			results[result] = "";
		} else {
			if(!result.empty() && result != parsed) {
				if(*search_po.is_approximate) {
					if(result_is_comparison) {result.insert(0, LEFT_PARENTHESIS); result += RIGHT_PARENTHESIS;}
					if(search_po.use_unicode_signs) {
						result.insert(0, SIGN_ALMOST_EQUAL " ");
					} else {
						result.insert(0, " ");
						result.insert(0, _("approx."));
					}
				} else if(!result_is_comparison) {
					result.insert(0, "= ");
				}
				string s = "copy-to-clipboard";
				s += result;
				g_variant_builder_add(&builder, "s", s.c_str());
			}
			results[parsed] = result;
		}
	}
}
static gboolean qalculate_search_provider_get_initial_result_set(ShellSearchProvider2*, GDBusMethodInvocation *invocation, gchar **terms, gpointer) {
	gchar *joined_terms = g_strjoinv(" ", terms);
	GVariantBuilder builder;
	handle_terms(joined_terms, builder);
	g_free(joined_terms);
	g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
	return TRUE;
}
static gboolean qalculate_search_provider_get_result_metas(ShellSearchProvider2*, GDBusMethodInvocation *invocation, gchar **eqs, gpointer) {
	gint idx;
	GVariantBuilder metas;
	g_variant_builder_init(&metas, G_VARIANT_TYPE ("aa{sv}"));
	for(idx = 0; eqs[idx] != NULL; idx++) {
		string seqs = eqs[idx];
		if(seqs.find("copy-to-clipboard") != 0) {
			unordered_map<string, string>::const_iterator it = expressions.find(eqs[idx]);
			if(it != expressions.end()) {
				GVariantBuilder meta;
				g_variant_builder_init(&meta, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&meta, "{sv}", "id", g_variant_new_string(eqs[idx]));
				g_variant_builder_add(&meta, "{sv}", "name", g_variant_new_string(it->second.c_str()));
				it = results.find(it->second);
				if(it != results.end() && !it->second.empty()) {
					g_variant_builder_add(&meta, "{sv}", "description", g_variant_new_string(it->second.c_str()));
				}
				g_variant_builder_add_value(&metas, g_variant_builder_end(&meta));
			}
		} else if(seqs.length() > 17) {
			seqs = seqs.substr(17);
			size_t i = string::npos;
			if(search_po.use_unicode_signs) {
				i = seqs.find(SIGN_ALMOST_EQUAL " ");
				if(i == 0) i = strlen(SIGN_ALMOST_EQUAL) - 1;
				else i = string::npos;
			} else {
				i = seqs.find(_("approx."));
				if(i == 0) i = strlen(_("approx.")) - 1;
				else i = string::npos;
			}
			if(i == string::npos) i = seqs.find("= ");
			if(i != string::npos) i += 2;
			GVariantBuilder meta;
			g_variant_builder_init(&meta, G_VARIANT_TYPE("a{sv}"));
			g_variant_builder_add(&meta, "{sv}", "id", g_variant_new_string("copy-to-clipboard"));
			g_variant_builder_add(&meta, "{sv}", "name", g_variant_new_string(_("Copy")));
			g_variant_builder_add(&meta, "{sv}", "description", g_variant_new_string(_("Copy result to clipboard")));
			g_variant_builder_add(&meta, "{sv}", "clipboardText", g_variant_new_string(i == string::npos ? seqs.c_str() : seqs.substr(i).c_str()));
			g_variant_builder_add_value(&metas, g_variant_builder_end(&meta));
		}
	}
	g_dbus_method_invocation_return_value(invocation, g_variant_new("(aa{sv})", &metas));
	return TRUE;
}
static gboolean qalculate_search_provider_get_subsearch_result_set(ShellSearchProvider2*, GDBusMethodInvocation *invocation, gchar**, gchar **terms, gpointer) {
	gchar *joined_terms = g_strjoinv(" ", terms);
	GVariantBuilder builder;
	handle_terms(joined_terms, builder);
	g_free(joined_terms);
	g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
	return TRUE;
}
static gboolean qalculate_search_provider_launch_search(ShellSearchProvider2*, GDBusMethodInvocation *invocation, gchar **terms, guint, gpointer) {
	gchar *joined_terms = g_strjoinv(" ", terms);
	string str = "qalculate-gtk \"";
	str += joined_terms;
	str += "\"";
	g_free(joined_terms);
	g_spawn_command_line_async(str.c_str(), NULL);
	g_dbus_method_invocation_return_value(invocation, NULL);
	return TRUE;
}

gboolean qalculate_search_provider_dbus_export(QalculateSearchProvider *self, GDBusConnection *connection, const gchar *object_path, GError **error);
void qalculate_search_provider_dbus_unexport(QalculateSearchProvider *self, GDBusConnection *connection, const gchar *object_path);

static void qalculate_search_provider_init(QalculateSearchProvider *self) {
	self->skeleton = shell_search_provider2_skeleton_new ();

	g_signal_connect_swapped(self->skeleton, "handle-activate-result", G_CALLBACK(qalculate_search_provider_activate_result), self);
	g_signal_connect_swapped(self->skeleton, "handle-get-initial-result-set", G_CALLBACK(qalculate_search_provider_get_initial_result_set), self);
	g_signal_connect_swapped(self->skeleton, "handle-get-subsearch-result-set", G_CALLBACK(qalculate_search_provider_get_subsearch_result_set), self);
	g_signal_connect_swapped(self->skeleton, "handle-get-result-metas", G_CALLBACK(qalculate_search_provider_get_result_metas), self);
	g_signal_connect_swapped(self->skeleton, "handle-launch-search", G_CALLBACK(qalculate_search_provider_launch_search), self);
}

gboolean qalculate_search_provider_dbus_export(QalculateSearchProvider *self, GDBusConnection *connection, const gchar *object_path, GError **error) {
	return g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(self->skeleton), connection, object_path, error);
}

void qalculate_search_provider_dbus_unexport(QalculateSearchProvider *self, GDBusConnection *connection, const gchar*) {
	if(g_dbus_interface_skeleton_has_connection(G_DBUS_INTERFACE_SKELETON(self->skeleton), connection)) {
		g_dbus_interface_skeleton_unexport_from_connection(G_DBUS_INTERFACE_SKELETON(self->skeleton), connection);
	}
}

typedef GtkApplicationClass QalculateSearchApplicationClass;
typedef GtkApplication QalculateSearchApplication;

typedef struct {
	QalculateSearchProvider *search_provider;
} QalculateSearchApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(QalculateSearchApplication, qalculate_search_application, GTK_TYPE_APPLICATION);

#define QALCULATE_TYPE_APPLICATION qalculate_search_application_get_type()
#define QALCULATE_SEARCH_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), QALCULATE_TYPE_APPLICATION, QalculateSearchApplication))

static gboolean qalculate_search_application_dbus_register(GApplication *application, GDBusConnection *connection, const gchar *object_path, GError **error);
static void qalculate_search_application_dbus_unregister(GApplication *application, GDBusConnection *connection, const gchar *object_path);

static void qalculate_search_application_class_init(QalculateSearchApplicationClass *o) {
	GApplicationClass *application_class = G_APPLICATION_CLASS(o);
	application_class->dbus_register = qalculate_search_application_dbus_register;
	application_class->dbus_unregister = qalculate_search_application_dbus_unregister;
}

static void qalculate_search_application_init(QalculateSearchApplication*) {}

static GtkApplication *qalculate_search_application_new(void) {
	return GTK_APPLICATION(g_object_new(qalculate_search_application_get_type(), "application-id", "io.github.Qalculate.SearchProvider", "flags", G_APPLICATION_IS_SERVICE, "inactivity-timeout", 20000, NULL));
}

static gboolean qalculate_search_application_dbus_register(GApplication *application, GDBusConnection *connection, const gchar *object_path, GError **error) {
	QalculateSearchApplication *self = QALCULATE_SEARCH_APPLICATION(application);
	QalculateSearchApplicationPrivate *priv = (QalculateSearchApplicationPrivate*) qalculate_search_application_get_instance_private(self);
	priv->search_provider = qalculate_search_provider_new();
	if(!qalculate_search_provider_dbus_export(priv->search_provider, connection, object_path, error)) return FALSE;
	return TRUE;
}
static void qalculate_search_application_dbus_unregister(GApplication *application, GDBusConnection *connection, const gchar *object_path) {
	QalculateSearchApplication *self = QALCULATE_SEARCH_APPLICATION(application);
	QalculateSearchApplicationPrivate *priv = (QalculateSearchApplicationPrivate*) qalculate_search_application_get_instance_private(self);
	qalculate_search_provider_dbus_unexport(priv->search_provider, connection, object_path);
}

void load_preferences_search() {

	search_po.multiplication_sign = MULTIPLICATION_SIGN_X;
	search_po.division_sign = DIVISION_SIGN_DIVISION_SLASH;
	search_po.is_approximate = new bool(false);
	search_po.prefix = NULL;
	search_po.use_min_decimals = false;
	search_po.use_denominator_prefix = true;
	search_po.min_decimals = 0;
	search_po.use_max_decimals = false;
	search_po.max_decimals = 2;
	search_po.base = 10;
	search_po.min_exp = EXP_PRECISION;
	search_po.negative_exponents = false;
	search_po.sort_options.minus_last = true;
	search_po.indicate_infinite_series = false;
	search_po.show_ending_zeroes = true;
	search_po.round_halfway_to_even = false;
	search_po.number_fraction_format = FRACTION_DECIMAL;
	search_po.restrict_fraction_length = false;
	search_po.abbreviate_names = true;
	search_po.use_unicode_signs = true;
	search_po.digit_grouping = DIGIT_GROUPING_STANDARD;
	search_po.use_unit_prefixes = true;
	search_po.exp_display = EXP_POWER_OF_10;
	search_po.duodecimal_symbols = false;
	search_po.use_prefixes_for_currencies = false;
	search_po.use_prefixes_for_all_units = false;
	search_po.spacious = true;
	search_po.short_multiplication = true;
	search_po.place_units_separately = true;
	search_po.use_all_prefixes = false;
	search_po.excessive_parenthesis = false;
	search_po.allow_non_usable = false;
	search_po.lower_case_numbers = false;
	search_po.lower_case_e = false;
	search_po.base_display = BASE_DISPLAY_NORMAL;
	search_po.twos_complement = true;
	search_po.hexadecimal_twos_complement = false;
	search_po.limit_implicit_multiplication = false;
	search_po.can_display_unicode_string_function = NULL;
	search_po.allow_factorization = false;
	search_po.spell_out_logical_operators = true;
	search_po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;

	search_eo.approximation = APPROXIMATION_TRY_EXACT;
	search_eo.sync_units = true;
	search_eo.structuring = STRUCTURING_SIMPLIFY;
	search_eo.parse_options.unknowns_enabled = false;
	search_eo.parse_options.read_precision = DONT_READ_PRECISION;
	search_eo.parse_options.base = BASE_DECIMAL;
	search_eo.allow_complex = true;
	search_eo.allow_infinite = true;
	search_eo.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
	search_eo.assume_denominators_nonzero = true;
	search_eo.warn_about_denominators_assumed_nonzero = true;
	search_eo.parse_options.limit_implicit_multiplication = false;
	search_eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	search_eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	search_eo.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	search_eo.parse_options.comma_as_separator = false;
	search_eo.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	search_eo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	search_eo.local_currency_conversion = true;
	search_eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	search_complex_angle_form = false;
	search_ignore_locale = false;
	search_adaptive_interval_display = true;

	bool search_simplified_percentage = true;

	CALCULATOR->useIntervalArithmetic(true);
	CALCULATOR->useBinaryPrefixes(0);
	CALCULATOR->setPrecision(10);

	FILE *file = NULL;
	gchar *gstr_file = g_build_filename(getLocalDir().c_str(), "qalculate-gtk.cfg", NULL);
	file = fopen(gstr_file, "r");

	int version_numbers[] = {5, 3, 0};

	if(file) {
		char line[1000000L];
		string stmp, svalue, svar;
		size_t i;
		int v;
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
				} else if(svar == "ignore_locale") {
					search_ignore_locale = v;
				} else if(svar == "default_currency") {
					search_default_currency = svalue;
				} else if(svar == "min_deci") {
					search_po.min_decimals = v;
				} else if(svar == "use_min_deci") {
					search_po.use_min_decimals = v;
				} else if(svar == "max_deci") {
					search_po.max_decimals = v;
				} else if(svar == "use_max_deci") {
					search_po.use_max_decimals = v;
				} /*else if(svar == "precision") {
					CALCULATOR->setPrecision(v);
				}*/ else if(svar == "interval_arithmetic") {
					CALCULATOR->useIntervalArithmetic(v);
				} else if(svar == "interval_display") {
					if(v == 0) {
						search_po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; search_adaptive_interval_display = true;
					} else {
						if(v >= INTERVAL_DISPLAY_SIGNIFICANT_DIGITS && v <= INTERVAL_DISPLAY_UPPER) {
							search_po.interval_display = (IntervalDisplay) v; search_adaptive_interval_display = false;
						}
					}
				} else if(svar == "min_exp") {
					search_po.min_exp = v;
					if(search_po.min_exp == 0) search_po.min_exp = 100;
				} else if(svar == "negative_exponents") {
					search_po.negative_exponents = v;
				} else if(svar == "sort_minus_last") {
					search_po.sort_options.minus_last = v;
				} else if(svar == "spacious") {
					search_po.spacious = v;
				} else if(svar == "excessive_parenthesis") {
					search_po.excessive_parenthesis = v;
				} else if(svar == "short_multiplication") {
					search_po.short_multiplication = v;
				} else if(svar == "limit_implicit_multiplication") {
					search_eo.parse_options.limit_implicit_multiplication = v;
					search_po.limit_implicit_multiplication = v;
				} else if(svar == "parsing_mode") {
					if(v >= PARSING_MODE_ADAPTIVE && v <= PARSING_MODE_CONVENTIONAL) {
						search_eo.parse_options.parsing_mode = (ParsingMode) v;
					}
				} else if(svar == "simplified_percentage") {
					search_simplified_percentage = v;
				} else if(svar == "place_units_separately") {
					search_po.place_units_separately = v;
				} else if(svar == "variable_units_enabled") {
					CALCULATOR->setVariableUnitsEnabled(v);
				} else if(svar == "use_prefixes") {
					search_po.use_unit_prefixes = v;
				} else if(svar == "use_prefixes_for_all_units") {
					search_po.use_prefixes_for_all_units = v;
				} else if(svar == "use_prefixes_for_currencies") {
					search_po.use_prefixes_for_currencies = v;
#if QALCULATE_MAJOR_VERSION == 3 && QALCULATE_MINOR_VERSION < 15
				} else if(svar == "number_fraction_format") {
					if(v >= FRACTION_DECIMAL && v <= FRACTION_COMBINED) {
						search_po.number_fraction_format = (NumberFractionFormat) v;
						search_po.restrict_fraction_length = (v == FRACTION_FRACTIONAL);
					}
#endif
				} else if(svar == "complex_number_form") {
					if(v == COMPLEX_NUMBER_FORM_CIS + 1) {
						search_eo.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
						search_complex_angle_form = true;
					} else if(v >= COMPLEX_NUMBER_FORM_RECTANGULAR && v <= COMPLEX_NUMBER_FORM_CIS) {
						search_eo.complex_number_form = (ComplexNumberForm) v;
						search_complex_angle_form = false;
					}
				} else if(svar == "read_precision") {
					if(v >= DONT_READ_PRECISION && v <= READ_PRECISION_WHEN_DECIMALS) {
						search_eo.parse_options.read_precision = (ReadPrecisionMode) v;
					}
				} else if(svar == "assume_denominators_nonzero") {
					if(version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
						v = true;
					}
					search_eo.assume_denominators_nonzero = v;
				} else if(svar == "warn_about_denominators_assumed_nonzero") {
					search_eo.warn_about_denominators_assumed_nonzero = v;
				} else if(svar == "structuring") {
					if(v >= STRUCTURING_NONE && v <= STRUCTURING_FACTORIZE) {
						search_eo.structuring = (StructuringMode) v;
						search_po.allow_factorization = (search_eo.structuring == STRUCTURING_FACTORIZE);
					}
				} else if(svar == "angle_unit") {
					if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_GRADIANS) {
						search_eo.parse_options.angle_unit = (AngleUnit) v;
					}
				/*} else if(svar == "functions_enabled") {
					search_eo.parse_options.functions_enabled = v;
				} else if(svar == "variables_enabled") {
					search_eo.parse_options.variables_enabled = v;
				} else if(svar == "calculate_variables") {
					search_eo.calculate_variables = v;
				} else if(svar == "calculate_functions") {
					search_eo.calculate_functions = v;
				} else if(svar == "sync_units") {
					search_eo.sync_units = v;
				} else if(svar == "unknownvariables_enabled") {
					search_eo.parse_options.unknowns_enabled = v;
				} else if(svar == "units_enabled") {
					search_eo.parse_options.units_enabled = v;*/
				} else if(svar == "allow_complex") {
					search_eo.allow_complex = v;
				} else if(svar == "allow_infinite") {
					search_eo.allow_infinite = v;
				} else if(svar == "use_short_units") {
					search_po.abbreviate_names = v;
				} else if(svar == "abbreviate_names") {
					search_po.abbreviate_names = v;
				} else if(svar == "all_prefixes_enabled") {
				 	search_po.use_all_prefixes = v;
				} else if(svar == "denominator_prefix_enabled") {
					search_po.use_denominator_prefix = v;
				} else if(svar == "use_binary_prefixes") {
					CALCULATOR->useBinaryPrefixes(v);
				/*} else if(svar == "auto_post_conversion") {
					if(v >= POST_CONVERSION_NONE && v <= POST_CONVERSION_OPTIMAL) {
						search_eo.auto_post_conversion = (AutoPostConversion) v;
					}*/
				} else if(svar == "mixed_units_conversion") {
					if(v >= MIXED_UNITS_CONVERSION_NONE && v <= MIXED_UNITS_CONVERSION_FORCE_ALL) {
						search_eo.mixed_units_conversion = (MixedUnitsConversion) v;
					}
				} else if(svar == "local_currency_conversion") {
					search_eo.local_currency_conversion = v;
				} else if(svar == "use_unicode_signs") {
					search_po.use_unicode_signs = v;
				} else if(svar == "lower_case_numbers") {
					search_po.lower_case_numbers = v;
				} else if(svar == "duodecimal_symbols") {
					search_po.duodecimal_symbols = v;
				} else if(svar == "lower_case_e") {
					if(v) search_po.exp_display = EXP_LOWERCASE_E;
				} else if(svar == "e_notation") {
					if(!v) search_po.exp_display = EXP_POWER_OF_10;
					else if(search_po.exp_display != EXP_LOWERCASE_E) search_po.exp_display = EXP_UPPERCASE_E;
				} else if(svar == "exp_display") {
					if(v >= EXP_UPPERCASE_E && v <= EXP_POWER_OF_10) search_po.exp_display = (ExpDisplay) v;
				} else if(svar == "imaginary_j") {
					search_do_imaginary_j = v;
				} else if(svar == "base_display") {
					if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_ALTERNATIVE) search_po.base_display = (BaseDisplay) v;
				} else if(svar == "twos_complement") {
					search_po.twos_complement = v;
				} else if(svar == "hexadecimal_twos_complement") {
					search_po.hexadecimal_twos_complement = v;
				} else if(svar == "spell_out_logical_operators") {
					search_po.spell_out_logical_operators = v;
				} else if(svar == "decimal_comma") {
					if(v == 0) CALCULATOR->useDecimalPoint(search_eo.parse_options.comma_as_separator);
					else if(v > 0) CALCULATOR->useDecimalComma();
				} else if(svar == "dot_as_separator") {
					if(v >= 0) search_eo.parse_options.dot_as_separator = v;
				} else if(svar == "comma_as_separator") {
					search_eo.parse_options.comma_as_separator = v;
					if(CALCULATOR->getDecimalPoint() != COMMA) {
						CALCULATOR->useDecimalPoint(search_eo.parse_options.comma_as_separator);
					}
				} else if(svar == "multiplication_sign") {
					if(v >= MULTIPLICATION_SIGN_ASTERISK && v <= MULTIPLICATION_SIGN_X) search_po.multiplication_sign = (MultiplicationSign) v;
				} else if(svar == "division_sign") {
					if(v >= DIVISION_SIGN_SLASH && v <= DIVISION_SIGN_DIVISION) search_po.division_sign = (DivisionSign) v;
				} else if(svar == "indicate_infinite_series") {
					search_po.indicate_infinite_series = v < 0 ? 0 : v;
				} else if(svar == "show_ending_zeroes") {
					if(version_numbers[0] > 2 || (version_numbers[0] == 2 && version_numbers[1] >= 9)) search_po.show_ending_zeroes = v;
				} else if(svar == "digit_grouping") {
					if(v >= DIGIT_GROUPING_NONE && v <= DIGIT_GROUPING_LOCALE) {
						search_po.digit_grouping = (DigitGrouping) v;
					}
				} else if(svar == "round_halfway_to_even") {//obsolete
					if(v) search_po.rounding = ROUNDING_HALF_TO_EVEN;
				} else if(svar == "rounding_mode") {
					if(v >= ROUNDING_HALF_AWAY_FROM_ZERO && v <= ROUNDING_DOWN) {
						if(v == 2 && (version_numbers[0] < 4 || (version_numbers[0] == 4 && version_numbers[1] < 9) || (version_numbers[0] == 4 && version_numbers[1] == 9 && version_numbers[2] == 0))) v = ROUNDING_TOWARD_ZERO;
						search_po.rounding = (RoundingMode) v;
					}
				/*} else if(svar == "approximation") {
					if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
						search_eo.approximation = (ApproximationMode) v;
					}*/
				} else if(svar == "interval_calculation") {
					if(v >= INTERVAL_CALCULATION_NONE && v <= INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
						search_eo.interval_calculation = (IntervalCalculation) v;
					}
				} else if(svar == "rpn_syntax") {
					search_eo.parse_options.rpn = v;
				} else if(svar == "default_assumption_type") {
					if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_REAL) {
						CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
					}
				/*} else if(svar == "default_assumption_sign") {
					if(v >= ASSUMPTION_SIGN_UNKNOWN && v <= ASSUMPTION_SIGN_NONZERO) {
						CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
					}*/
				}
			}
		}
		fclose(file);
	}
	if(search_simplified_percentage) search_eo.parse_options.parsing_mode = (ParsingMode) (search_eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
}


int main (int argc, char *argv[]) {

	GtkApplication *app;
	gint status;

#ifdef ENABLE_NLS
	gchar *gstr_file = g_build_filename(getLocalDir().c_str(), "qalculate-gtk.cfg", NULL);
	FILE *file = fopen(gstr_file, "r");
	char line[10000];
	string stmp;
	if(file) {
		while(true) {
			if(fgets(line, 10000, file) == NULL) break;
			if(strcmp(line, "ignore_locale=1\n") == 0) {
				search_ignore_locale = true;
				break;
			} else if(strcmp(line, "ignore_locale=0\n") == 0) {
				break;
			} else if(strncmp(line, "language=", 9) == 0) {
				string search_lang = line + sizeof(char) * 9;
				remove_blank_ends(search_lang);
				if(!search_lang.empty()) {
#	ifdef _WIN32
					_putenv_s("LANGUAGE", search_lang.c_str());
#	else
					setenv("LANGUAGE", search_lang.c_str(), 1);
					if(search_lang.find(".") == string::npos && search_lang.find("_") != string::npos) search_lang += ".utf8";
					if(search_lang.find(".") != string::npos) setenv("LC_MESSAGES", search_lang.c_str(), 1);
#	endif
				}
				break;
			}
		}
		fclose(file);
	}
	if(!search_ignore_locale) {
		bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
		textdomain(GETTEXT_PACKAGE);
	}
#endif

	if(!search_ignore_locale) setlocale(LC_ALL, "");

	app = qalculate_search_application_new();

	new Calculator(search_ignore_locale);
	CALCULATOR->setExchangeRatesWarningEnabled(false);
	load_preferences_search();

	CALCULATOR->loadExchangeRates();
	if(!CALCULATOR->loadGlobalDefinitions()) g_print(_("Failed to load global definitions!\n"));
	CALCULATOR->loadLocalDefinitions();

	if(!search_default_currency.empty()) {
		Unit *u = CALCULATOR->getActiveUnit(search_default_currency);
		if(u) CALCULATOR->setLocalCurrency(u);
	}

	if(search_do_imaginary_j && CALCULATOR->v_i->hasName("j") == 0) {
		ExpressionName ename = CALCULATOR->v_i->getName(1);
		ename.name = "j";
		ename.reference = false;
		CALCULATOR->v_i->addName(ename, 1, true);
		CALCULATOR->v_i->setChanged(false);
	}

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	CALCULATOR->terminateThreads();

	return status;

}

