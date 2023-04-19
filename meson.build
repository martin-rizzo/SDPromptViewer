#
# File    : meson.build
# Brief   : Build definition file for Meson build system
# Author  : Martin Rizzo | <martinrizzo@gmail.com>
# Date    : Mar 25, 2023
# Repo    : https://github.com/martin-rizzo/SDPromptViewer
# License : MIT
#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#                      Stable Diffusion Prompt Viewer
#      A plugin for "Eye of GNOME" that displays SD embedded prompts.
#   
#     Copyright (c) 2023 Martin Rizzo
#     
#     Permission is hereby granted, free of charge, to any person obtaining
#     a copy of this software and associated documentation files (the
#     "Software"), to deal in the Software without restriction, including
#     without limitation the rights to use, copy, modify, merge, publish,
#     distribute, sublicense, and/or sell copies of the Software, and to
#     permit persons to whom the Software is furnished to do so, subject to
#     the following conditions:
#     
#     The above copyright notice and this permission notice shall be
#     included in all copies or substantial portions of the Software.
#     
#     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#     TORT OR OTHERWISE, ARISING FROM,OUT OF OR IN CONNECTION WITH THE
#     SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
#
# meson build --buildtype=
#    debug         : enables full debug information and disables optimization.
#    debugoptimized: enables debug information and enables optimization.
#    release       : enables full optimization and disables debug information.
#    plain         : neither optimization nor debug information is enabled.
#    minsize       : optimizes for minimum binary size.
#
project(
  'sdprompt-viewer', 'c',
  version : '1.0',
  license : 'MIT',
  
  default_options : ['prefix=~/.local','buildtype=debugoptimized'],
  meson_version   : '>= 0.58.0',
)
gnome = import('gnome')
i18n  = import('i18n')

##----------------------------- DEPENDENCIES ------------------------------##

eog_dep        = dependency('eog', version: '>= 41.0')
libpeas_dep    = dependency('libpeas-1.0', version: '>= 1.14.1')
libpeasgtk_dep = dependency('libpeas-gtk-1.0', version: '>= 1.14.1')
gio_dep        = dependency('gio-2.0', version: '>= 2.53.4')

sdprompt_viewer_deps = [
  eog_dep,
  libpeas_dep,
  libpeasgtk_dep,
]

##-------------------------------- PATHS ----------------------------------##

my_src_root      = meson.current_source_dir()
my_po_dir        = my_src_root / 'po'
my_gschema_input = my_src_root / 'system' / 'sdprompt-viewer.gschema.xml.in'
my_appdata_input = my_src_root / 'system' / 'sdprompt-viewer.appdata.xml.in'

root_include_dir = include_directories('.')
data_dir         = get_option('prefix') / get_option('datadir')
plugin_data_dir  = data_dir / 'eog' / 'plugins'
app_stream_dir   = data_dir / 'metainfo'
glib_dir         = data_dir / 'glib-2.0'
locale_dir       = data_dir / 'locale'

# The "gio_schemasdir" variable is used to specify the directory where
# the GSettings schema file should be located. This allows the plugin
# to find and load the configuration options.
gio_schemasdir = gio_dep.get_variable(
  'schemasdir',
  pkgconfig_define: ['datadir', data_dir],
  default_value: glib_dir / 'schemas',
)

##------------------------------ C COMPILER -------------------------------##

cc             = meson.get_compiler('c')
common_flags   = ['-DHAVE_CONFIG_H']
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
    '-Wno-unused-function',
    '-Wstrict-prototypes',
  ])
endif
add_project_arguments(common_flags + compiler_flags, language: 'c')


##=========================================================================##

#subdir('po')

# Summary message
summary({
  'Prefix'                 : get_option('prefix'),
  'Compiler'               : cc.get_id(),
  'Extra compiler warnings': compiler_flags,
}, section: 'Build')

summary({
  'GNOME schemas dir' : gio_schemasdir,
  'Eog plugin dir'    : plugin_data_dir,
}, section: 'Install')


##---------------------------- FILE: config.h -----------------------------##

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())

configure_file(
  output:       'config.h',
  configuration: config_h
)

##---------------------------- FILE: lib*.so ------------------------------##

# Defines the source files for the sdprompt-viewer-plugin
sdprompt_viewer_sources = files(
  'sdprompt-viewer-plugin.c',
  'sdprompt-viewer-preferences.c',
)

# Compiles the resources and saves them in the pair .c/.h
# ( 'sdprompt-viewer-resources.c/.h' )
sdprompt_viewer_res = gnome.compile_resources(
  'sdprompt-viewer-resources',
  'resources.xml',
)
sdprompt_viewer_sources += [ sdprompt_viewer_res.get(0) ]

sdprompt_viewer_sha = shared_module(
  'sdprompt-viewer',
  sources:             sdprompt_viewer_sources,
  include_directories: root_include_dir,
  dependencies:        sdprompt_viewer_deps,
  install:             true,
  install_dir:         plugin_data_dir,
)

##---------------------------- FILE: *.plugin -----------------------------##

# FIXME: Remove `args` parameter when gettext acquires plugin support.
#        http://lists.gnu.org/archive/html/bug-gettext/2017-06/msg00001.html
# NOTE:  We need to keep the .desktop suffix on the source file to be able
#        to extract translatable strings. .plugin is not recognized.
i18n.merge_file(
  input       : 'system/sdprompt-viewer.plugin.desktop.in',
  output      : 'sdprompt-viewer.plugin',
  po_dir      : my_po_dir,
  type        : 'desktop',
  args        : ['--keyword=Name', '--keyword=Description'],
  install     : true,
  install_dir : plugin_data_dir,
)

##------------------------- FILE: *.gschema.xml ---------------------------##

configure_file(
  input         : my_gschema_input,
  output        : 'org.gnome.eog.plugins.sdprompt-viewer.gschema.xml',
  configuration : {'GETTEXT_PACKAGE': 'eog-plugin-sdprompt-viewer'},
  install       : true,
  install_dir   : gio_schemasdir,
)

# glib-compile-schemas ~/.local/share/glib-2.0/schemas/
gnome.post_install(
  glib_compile_schemas: true,
)

##------------------------- FILE: *.appdata.xml ---------------------------##

#sdprompt_viewer_appdata = i18n.merge_file(
#  input        : my_appdata_input,
#  output       : 'eog-sdprompt-viewer.appdata.xml',
#  po_dir       : my_src_root / 'po',
#  type         : 'xml',
#  install      : false,
#  #install     : true,
#  #install_dir : app_stream_dir,
#)

#appstream_util = find_program('appstream-util', required: false)
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

##---------------------------- OUTPUT FILES -------------------------------##

# so_path = sdprompt_viewer_sha.full_path()
