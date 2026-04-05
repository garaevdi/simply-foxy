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
config_h_conf.set_quoted('APP_NAME', meson.project_name())
config_h_conf.set_quoted('VERSION', meson.project_version())
config_h_conf.set_quoted('DATADIR', datadir)
config_h_conf.set_quoted('GETTEXT_DOMAIN', gettext_domain)
config_h = configure_file(configuration: config_h_conf, output: 'config.h')

peel_gen = find_program('peel-gen')

env = environment()
env.append('GI_GIR_PATH', meson.project_source_root() / 'gir')

peel_h = custom_target(
  'peel-codegen',
  env: env,
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
  'log.h',
  'sf-application.hpp',
  'sf-application.cpp',
  'sf-application-window.hpp',
  'sf-application-window.cpp',
  'sf-firefox-manager.hpp',
  'sf-firefox-manager.cpp',
  'sf-firefox-profile.hpp',
  'sf-firefox-profile.cpp',
  'sf-theme-manager.hpp',
  'sf-theme-manager.cpp',
]

executable(
  app_id,
  srcs,
  cpp_args: ['-fpermissive'],
  dependencies: deps,
  install: true
)
