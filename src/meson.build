qalculate_gtk_src = [
    'callbacks.cc',
    'interface.cc',
    'main.cc',
    flags_generated_sources,
    ui_generated_sources,
]

executable(
    meson.project_name(),
    qalculate_gtk_src,
    dependencies: qalculate_gtk_deps,
    include_directories: incs,
    install: true,
    link_args: '-rdynamic',
)

if search_provider_enabled
    search_provider_src = [
        'searchprovider.cc',
        search_provider_generated_src,
    ]

    executable(
        'qalculate-search-provider',
        search_provider_src,
        dependencies: search_provider_deps,
        include_directories: incs,
        install: true,
        link_args: '-rdynamic',
    )
endif
