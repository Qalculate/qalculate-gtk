flags_generated_sources = gnome.compile_resources(
  meson.project_name() + 'flags-resources',
  'flags.gresource.xml',
  c_name: 'qalculate_gtk_flags',
  source_dir: 'flags',
)

ui_generated_sources = gnome.compile_resources(
  meson.project_name() + 'ui-resources',
  'ui.gresource.xml',
  c_name: 'qalculate_gtk_ui',
)

search_provider_generated_src = gnome.gdbus_codegen(
    'search-provider-interface',
    sources: 'org.gnome.ShellSearchProvider2.xml',
    interface_prefix : 'org.gnome.Shell.',
    namespace : 'Shell',
)

dbus_config = configuration_data()
dbus_config.set('libexecdir', libexecdir)

configure_file(
    input: 'io.github.Qalculate.SearchProvider.service.in',
    output: '@BASENAME@',
    configuration: dbus_config,
    install: true,
    install_dir: dbusdir,
)

i18n.merge_file(
  input: 'qalculate-gtk.desktop.in',
  output: '@BASENAME@',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: datadir / 'applications'
)

i18n.merge_file(
  input: 'qalculate-gtk.appdata.xml.in',
  output: '@BASENAME@',
  po_dir: '../po',
  install: true,
  install_dir: datadir / 'metainfo'
)

install_man(
    'qalculate-gtk.1',
)

if search_provider_enabled
    install_data(
        'io.github.Qalculate.search-provider.ini',
        install_dir: datadir / 'gnome-shell' / 'search-providers'
    )
endif

subdir('icons')
