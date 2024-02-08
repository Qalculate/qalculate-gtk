/*
    Qalculate (gnome shell search provider)

    Copyright (C) 2020  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include <unistd.h>
#include <sys/stat.h>

#include "support.h"

#include <libqalculate/qalculate.h>

#include "gnome-search-provider2.h"

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
				namespace Sgi = std;		// GCC 3.0
#			else
				namespace Sgi = ::__gnu_cxx;	// GCC 3.1 and later
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

typedef struct _QalculateSearchProvider QalculateSearchProvider;
typedef GObjectClass QalculateSearchProviderClass;

struct _QalculateSearchProvider {
	GObject parent_instance;
	ShellSearchProvider2 *skeleton;
};

G_DEFINE_TYPE(QalculateSearchProvider, qalculate_search_provider, G_TYPE_OBJECT);

static void qalculate_search_provider_class_init(QalculateSearchProviderClass *o) {}
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

static void qalculate_search_provider_activate_result(ShellSearchProvider2 *object, GDBusMethodInvocation *invocation, gchar *result, gchar **terms, guint timestamp, gpointer user_data) {
	gchar *joined_terms = g_strjoinv(" ", terms);
	if(strcmp(result, "copy-to-clipboard") == 0) {
		unordered_map<string, string>::const_iterator it = expressions.find(joined_terms);
		if(it != expressions.end()) {
			it = results.find(it->second);
			if(it != expressions.end()) {
				size_t i = string::npos;
				if(search_po.use_unicode_signs) {
					i = it->second.find(SIGN_ALMOST_EQUAL " ");
					if(i == 0) i = strlen(SIGN_ALMOST_EQUAL) - 1;
					else i = string::npos;
				} else {
					i = it->second.find(_("approx."));
					if(i == 0) i = strlen(_("approx.")) - 1;
					else i = string::npos;
				}
				if(i == string::npos) i = it->second.find("= ");
				if(i != string::npos) i += 2;
				gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), i == string::npos ? it->second.c_str() : it->second.substr(i).c_str(), -1);
			}
		}
	} else {
		string str = "qalculate-gtk \"";
		str += joined_terms;
		str += "\"";
		g_spawn_command_line_async(str.c_str(), NULL);
	}
	g_free(joined_terms);
	g_dbus_method_invocation_return_value(invocation, NULL);
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
		if(results.find(it->second) != results.end() && !it->second.empty()) g_variant_builder_add(&builder, "s", "copy-to-clipboard");
	} else {
		bool b_valid = false;
		expressions[expression] = "";
		if(!b_valid) b_valid = (expression.find_first_of(OPERATORS NUMBERS PARENTHESISS) != string::npos);
		if(!b_valid) b_valid = CALCULATOR->hasToExpression(expression, false, search_eo);
		if(!b_valid) {
			string str = expression;
			CALCULATOR->parseSigns(str);
			b_valid = (str.find_first_of(OPERATORS NUMBERS PARENTHESISS) != string::npos);
			if(!b_valid) {
				size_t i = str.find_first_of(SPACES);
				MathStructure m;
				if(i != string::npos) {
					str = str.substr(0, i);
					b_valid = (str == "factor" || equalsIgnoreCase(str, "factorize") || equalsIgnoreCase(str, _("factorize")) || equalsIgnoreCase(str, "expand") || equalsIgnoreCase(str, _("expand")));
				}
				if(!b_valid) {
					CALCULATOR->parse(&m, str, search_eo.parse_options);
					if(!has_error() && (m.isUnit() || m.isFunction() || (m.isVariable() && (i != string::npos || m.variable()->isKnown())))) b_valid = true;
				}
			}
		}
		if(!b_valid) return;
		if(CALCULATOR->busy()) CALCULATOR->abort();
		bool result_is_comparison = false;
		string parsed;
		string str = CALCULATOR->unlocalizeExpression(expression, search_eo.parse_options);
		int max_length = 100 - unicode_length(str);
		if(max_length < 50) max_length = 50;
		string result = CALCULATOR->calculateAndPrint(str, 100, search_eo, search_po, AUTOMATIC_FRACTION_AUTO, AUTOMATIC_APPROXIMATION_AUTO, &parsed, max_length, &result_is_comparison);
		search_po.number_fraction_format = FRACTION_DECIMAL;
		if(has_error() || result.empty() || parsed.find(CALCULATOR->abortedMessage()) != string::npos || parsed.find(CALCULATOR->timedOutString()) != string::npos) {
			return;
		}
		if(result.find(CALCULATOR->abortedMessage()) != string::npos || result.find(CALCULATOR->timedOutString()) != string::npos) {
			if(parsed.empty()) return;
			result = "";
		}
		expressions[expression] = parsed;
		g_variant_builder_add(&builder, "s", expression.c_str());
		if(parsed.empty() || result == parsed) {
			results[result] = "";
		} else {
			if(!result.empty()) {
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
				g_variant_builder_add(&builder, "s", "copy-to-clipboard");
			}
			results[parsed] = result;
		}
	}
}
static gboolean qalculate_search_provider_get_initial_result_set(ShellSearchProvider2 *object, GDBusMethodInvocation *invocation, gchar **terms, gpointer user_data) {
	gchar *joined_terms = g_strjoinv(" ", terms);
	GVariantBuilder builder;
	handle_terms(joined_terms, builder);
	g_free(joined_terms);
	g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
	return TRUE;
}
static gboolean qalculate_search_provider_get_result_metas(ShellSearchProvider2 *object, GDBusMethodInvocation *invocation, gchar **eqs, gpointer user_data) {
	gint idx;
	GVariantBuilder metas;
	g_variant_builder_init(&metas, G_VARIANT_TYPE ("aa{sv}"));
	for(idx = 0; eqs[idx] != NULL; idx++) {
		if(strcmp(eqs[idx], "copy-to-clipboard") != 0) {
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
		} else {
			GVariantBuilder meta;
			g_variant_builder_init(&meta, G_VARIANT_TYPE("a{sv}"));
			g_variant_builder_add(&meta, "{sv}", "id", g_variant_new_string("copy-to-clipboard"));
			g_variant_builder_add(&meta, "{sv}", "name", g_variant_new_string(_("Copy")));
			g_variant_builder_add(&meta, "{sv}", "description", g_variant_new_string(_("Copy result to clipboard")));
			g_variant_builder_add_value(&metas, g_variant_builder_end(&meta));
		}
	}
	g_dbus_method_invocation_return_value(invocation, g_variant_new("(aa{sv})", &metas));
	return TRUE;
}
static gboolean qalculate_search_provider_get_subsearch_result_set(ShellSearchProvider2 *object, GDBusMethodInvocation *invocation, gchar **arg_previous_results, gchar **terms, gpointer user_data) {
	gchar *joined_terms = g_strjoinv(" ", terms);
	GVariantBuilder builder;
	handle_terms(joined_terms, builder);
	g_free(joined_terms);
	g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
	return TRUE;
}
static gboolean qalculate_search_provider_launch_search(ShellSearchProvider2 *object, GDBusMethodInvocation *invocation, gchar **terms, guint timestamp, gpointer user_data) {
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

void qalculate_search_provider_dbus_unexport(QalculateSearchProvider *self, GDBusConnection *connection, const gchar *object_path) {
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

static void qalculate_search_application_init(QalculateSearchApplication *self) {}

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
	search_po.exp_display = EXP_BASE10;
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

	bool simplified_percentage = true;

	CALCULATOR->useIntervalArithmetic(true);
	CALCULATOR->useBinaryPrefixes(0);
	CALCULATOR->setPrecision(10);

	FILE *file = NULL;
	gchar *gstr_file = g_build_filename(getLocalDir().c_str(), "qalculate-gtk.cfg", NULL);
	file = fopen(gstr_file, "r");

	int version_numbers[] = {4, 9, 1};

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
					simplified_percentage = v;
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
					if(!v) search_po.exp_display = EXP_BASE10;
					else if(search_po.exp_display != EXP_LOWERCASE_E) search_po.exp_display = EXP_UPPERCASE_E;
				} else if(svar == "exp_display") {
					if(v >= EXP_UPPERCASE_E && v <= EXP_BASE10) search_po.exp_display = (ExpDisplay) v;
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
					search_po.indicate_infinite_series = v;
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
	if(simplified_percentage) search_eo.parse_options.parsing_mode = (ParsingMode) (search_eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
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

