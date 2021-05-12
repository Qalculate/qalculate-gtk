icon_dirs = [
  '16x16',
  '22x22',
  '24x24',
  '32x32',
  '48x48',
  '64x64',
  '128x128',
  '256x256',
]

hicolor_icondir = get_option('datadir') / 'icons' / 'hicolor'

foreach i:icon_dirs
    install_data(
        i / 'qalculate.png',
        install_dir: hicolor_icondir / i / 'apps',
    )
endforeach

install_data(
    'scalable' / 'qalculate.svg',
    install_dir: hicolor_icondir / 'scalable' / 'apps',
)

