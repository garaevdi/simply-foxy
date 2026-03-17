project(
  'simplyfoxy',
  'cpp',
  version: '0.1.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=0.59'
)

app_id = 'io.github.garaevdi.' + meson.project_name()
app_path = '/' + app_id.replace('.', '/')


prefix = get_option('prefix')
datadir = prefix / get_option('datadir')
schemadir = datadir / 'glib-2.0' / 'schemas'

gnome = import('gnome')

subdir('data')
subdir('src')

gnome.post_install(
  glib_compile_schemas: true
)
