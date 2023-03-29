
project(
  'sdprompt-viewer', 'c',
  version: '1.0',
  license: 'MIT',
  default_options: ['prefix=~/.local','buildtype=debugoptimized'],
  meson_version: '>= 0.58.0'
)

gnome  = import('gnome')
i18n   = import('i18n')

# Paths
root_include_dir = include_directories('.')

src_root = meson.current_source_dir()
po_dir = src_root / 'po'

# '.local' / 'lib64'
libdir  = get_option('prefix') / get_option('libdir')
datadir = get_option('prefix') / get_option('datadir')

pkglibdir = libdir / 'eog'
pkgdatadir = datadir / 'eog'

pluginlibdir = libdir / 'eog' / 'plugins'
plugindatadir = datadir / 'eog' / 'plugins'

appstreamdir = datadir / 'metainfo'
glibdir = datadir / 'glib-2.0'
localedir = datadir / 'locale'

# La variable "gio_schemasdir" se utiliza en el archivo meson.build para
# especificar la ubicación de los archivos de esquema de GSettings de tu
# proyecto. Esto permite que tu proyecto pueda encontrar y cargar las
# opciones de configuración de la aplicación desde los archivos de esquema
# de GSettings en el sistema del usuario.
gio_dep = dependency('gio-2.0', version: '>= 2.53.4')
gio_schemasdir = gio_dep.get_variable(
  'schemasdir',
  pkgconfig_define: ['datadir', datadir],
  default_value: glibdir / 'schemas',
)

# Dependencies in common for all plugins
libpeas_dep = dependency('libpeas-1.0', version: '>= 1.14.1')
libpeasgtk_dep = dependency('libpeas-gtk-1.0', version: '>= 1.14.1')

eog_dep = dependency('eog', version: '>= 41.0')

appstream_util = find_program('appstream-util', required: false)

source_root = meson.current_source_dir()
cc = meson.get_compiler('c')
common_flags = ['-DHAVE_CONFIG_H']
compiler_flags = []
if get_option('buildtype').contains('debug')
  compiler_flags += cc.get_supported_arguments([
    '-Werror=format=2',
    '-Werror=implicit-function-declaration',
    '-Werror=init-self',
    '-Werror=missing-include-dirs',
    '-Werror=missing-prototypes',
    '-Werror=pointer-arith',
    '-Werror=return-type',
    '-Wnested-externs',
    '-Wstrict-prototypes',
  ])
endif
add_project_arguments(common_flags + compiler_flags, language: 'c')

# config.h
config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())


#subdir('po')
#subdir('plugins')

configure_file(
  output:       'config.h',
  configuration: config_h
)

# glib-compile-schemas ~/.local/share/glib-2.0/schemas/
gnome.post_install(
  glib_compile_schemas: true,
  #glib_compile_schemas: false,
)

# Summary message

summary({
  'Prefix': get_option('prefix'),
  'Compiler': cc.get_id(),
  'Extra compiler warnings': compiler_flags,
}, section: 'Build')


#--------------------------------- lib*.so --------------------------------#

# Defines the source files for the sdprompt-viewer-plugin
sdprompt_viewer_sources = files(
  'sdprompt-viewer-plugin.c',
  'sdprompt-viewer-preferences.c',
)

# Compiles the resources of the sdprompt-viewer-plugin and saves
# them in the pair .c/.h file 'sdprompt-viewer-resources.c'/.h
sdprompt_viewer_res = gnome.compile_resources(
  'sdprompt-viewer-resources',
  'resources.xml',
)
sdprompt_viewer_sources += [ sdprompt_viewer_res.get(0) ]


sdprompt_viewer_deps = [
  eog_dep,
  libpeas_dep,
  libpeasgtk_dep,
]

sdprompt_viewer_sha = shared_module(
  'sdprompt-viewer',
  sources:             sdprompt_viewer_sources,
  include_directories: root_include_dir,
  dependencies:        sdprompt_viewer_deps,
  install:             true,
  install_dir:         plugindatadir,
)

##------------------------------- *.plugin -------------------------------##

# FIXME: Remove `args` parameter when gettext acquires plugin support.
#        http://lists.gnu.org/archive/html/bug-gettext/2017-06/msg00001.html
# NOTE:  We need to keep the .desktop suffix on the source file to be able
#        to extract translatable strings. .plugin is not recognized.
i18n.merge_file(
  input:  'resources/sdprompt-viewer.plugin.desktop.in',
  output: 'sdprompt-viewer.plugin',
  po_dir: po_dir,
  type:   'desktop',
  args:   ['--keyword=Name', '--keyword=Description'],
  install: true,
  install_dir: plugindatadir,
)

##---------------------------- *.gschema.xml ------------------------------##

configure_file(
  input:  'resources/org.gnome.eog.plugins.sdprompt-viewer.gschema.xml.in',
  output: '@BASENAME@',
  configuration: {'GETTEXT_PACKAGE': 'eog-plugin-sdprompt-viewer'},
  install: true,
  install_dir: gio_schemasdir,
)

gnome.post_install(glib_compile_schemas: true)
summary('schemas dir:', get_option('prefix') / get_option('datadir') / 'glib-2.0' / 'schemas' )



##---------------------------- *.appdata.xml ------------------------------##

#sdprompt_viewer_appdata = i18n.merge_file(
#  input: 'resources/eog-sdprompt-viewer.appdata.xml.in',
#  output: '@BASENAME@',
#  po_dir: src_root / 'po',
#  type: 'xml',
#  install: false,
#  #install: true,
#  #install_dir: appstreamdir,
#)

#if appstream_util.found()
#  test(
#    'validate-eog-sdprompt-viewer.appdata.xml',
#    appstream_util,
#    args: [
#      'validate',
#      '--nonet',
#      sdprompt-viewer_appdata.full_path(),
#    ]
#  )
#endif

