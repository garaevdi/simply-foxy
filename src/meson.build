deps = [
  dependency('gobject-2.0'),
  dependency('glib-2.0'),
  dependency('gtk4'),
  dependency('libadwaita-1'),
  dependency('granite-7'),
  dependency('libsoup-3.0'),
  dependency('json-glib-1.0'),
  dependency('peel')
]

config_h_conf = configuration_data()
config_h_conf.set_quoted('APP_ID', app_id)
config_h_conf.set_quoted('APP_PATH', app_path)
config_h = configure_file(configuration: config_h_conf, output: 'config.h')

peel_gen = find_program('peel-gen')

peel_h = custom_target(
  'peel-codegen',
  env: { 'GI_GIR_PATH': meson.project_source_root() / 'gir' },
  command: [
    peel_gen,
    '--recursive',
    '--out-dir', '@OUTDIR@',
    'Adw', '1',
    'Granite', '7.0',
    'Soup', '3.0',
    'Json', '1.0'
  ],
  output: 'peel',
)

srcs = [
  resources,
  config_h,
  peel_h,
  'sf-application.hpp',
  'sf-application.cpp',
  'sf-application-window.hpp',
  'sf-application-window.cpp',
]

executable(
  app_id,
  srcs,
  dependencies: deps,
  install: true
)
