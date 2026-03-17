blueprint_compiler = find_program('blueprint-compiler', required: true)
blueprints = custom_target(
  'blueprints',
  input: files(
    'ui/sf-application-window.blp',
  ),
  output: '.',
  command: [
    blueprint_compiler,
    'batch-compile',
    '@OUTPUT@',
    '@CURRENT_SOURCE_DIR@',
    '@INPUT@',
  ],
)

resources_conf = configuration_data()
resources_conf.set('APP_PATH', app_path)
resources_file = configure_file(
  input: 'sf.gresource.xml.in',
  configuration: resources_conf,
  output: '@0@.gresource.xml'.format(app_id),
)
resources = gnome.compile_resources(
  '@0@-resources'.format(meson.project_name()),
  resources_file,
  c_name: meson.project_name(),
  dependencies: blueprints,
)
