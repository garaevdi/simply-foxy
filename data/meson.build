subdir('icons')

desktop_conf = configuration_data()
desktop_conf.set('APP_ID', app_id)
desktop_conf.set('GETTEXT', gettext_domain)
desktop_file = i18n.merge_file(
  input: configure_file(
    input: 'sf.desktop.in.in',
    output: 'desktop.in',
    configuration: desktop_conf,
  ),
  output: app_id + '.desktop',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
  install: true,
  install_dir: datadir / 'applications',
)
desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

metainfo_conf = configuration_data()
metainfo_conf.set('APP_ID', app_id)
metainfo_conf.set('GETTEXT', gettext_domain)
metainfo_file = i18n.merge_file(
  input: configure_file(
    input: 'sf.metainfo.xml.in.in',
    output: 'sf.metainfo.xml.in',
    configuration: metainfo_conf,
  ),
  output: app_id + '.metainfo.xml',
  po_dir: meson.project_source_root() / 'po',
  install: true,
  install_dir: get_option('datadir') / 'metainfo',
)
appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test('Validate appstream file', appstreamcli, args: ['validate', metainfo_file])
endif

gschema_conf = configuration_data()
gschema_conf.set('APP_ID', app_id)
gschema_conf.set('APP_PATH', app_path + '/')
gschema_conf.set('GETTEXT', app_id)
configure_file(
  input: 'sf.gschema.xml.in',
  configuration: gschema_conf,
  output: '@0@.gschema.xml'.format(app_id),
  install: true,
  install_dir: schemadir,
)
glib_compile_schemas = find_program('glib-compile-schemas', required: true)
if glib_compile_schemas.found()
  test(
    'validate-gschema',
    glib_compile_schemas,
    args: ['--strict', '--dry-run', meson.current_build_dir()],
  )
endif

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
