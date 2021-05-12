project(
  'qalculate-gtk',
  ['c', 'cpp'],
  version: '3.17.0',
  meson_version: '>=0.50.0',
  default_options: ['warning_level=2', 'c_std=c11', 'cpp_std=c++11'],

)

conf = configuration_data()
i18n = import('i18n')
gnome = import ('gnome')

glib_dep = dependency('glib-2.0', version: '>=2.4')
gobject_dep = dependency('gobject-2.0', version: '>=2.4')
gio_dep = dependency('gio-2.0', version: '>=2.4')
gtk_dep = dependency('gtk+-3.0', version: '>=3.10')
libxml_dep = dependency('libxml-2.0')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0')
libqalculate_dep = dependency('libqalculate', version: '>=@0@'.format(meson.project_version()))

qalculate_gtk_deps = [
    glib_dep,
    gobject_dep,
    gio_dep,
    gtk_dep,
    libxml_dep,
    gdk_pixbuf_dep,
    libqalculate_dep,
]

if get_option('webkitgtk').enabled()
    conf.set('USE_WEBKITGTK', 1)
    qalculate_gtk_deps += [
        dependency('webkit2gtk-4.0')
    ]
endif

if get_option('gnome-search').enabled()
    search_provider_deps = [
        dependency('gio-unix-2.0'),
        glib_dep,
        gtk_dep,
        libqalculate_dep,
    ]
    search_provider_enabled = true
else
    search_provider_enabled = false
endif

if get_option('local-help')
    conf.set('LOCAL_HELP', 1)
    local_help_enabled = true
else
    local_help_enabled = false
endif

gettext_package = meson.project_name()
prefix = get_option('prefix')
datadir = prefix / get_option('datadir')
dbusdir = datadir / 'dbus-1' / 'services'
docdir = datadir / 'doc' / 'qalculate-gtk'
libexecdir = prefix / get_option('libexecdir')
localedir = prefix / get_option('localedir')

conf.set_quoted(
    'GETTEXT_PACKAGE',
    gettext_package,
)
conf.set_quoted(
    'PACKAGE_LOCALE_DIR',
    localedir,
)
conf.set_quoted(
    'PACKAGE_DATA_DIR',
    datadir,
)
conf.set_quoted(
    'PACKAGE_DOC_DIR',
    docdir,
)
conf.set_quoted(
    'VERSION',
    meson.project_version(),
)
add_project_arguments(
  '-DHAVE_CONFIG_H=1',
  language: 'cpp',
)

cpp_compiler = meson.get_compiler('cpp')
if cpp_compiler.check_header('unordered_map')
    conf.set('HAVE_UNORDERED_MAP', 1)
endif

configure_file(
  output: 'config.h',
  configuration: conf,
)

# Include the config.h we just generated
incs = include_directories('.')

subdir('data')
subdir('po')
subdir('src')

if local_help_enabled
    subdir('doc')
endif

meson.add_install_script(
  'meson-post-install.sh',
  get_option('datadir'),
)
