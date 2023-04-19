/**
 * @file    resources.h
 * @brief   Defines constants for accessing resources, such as UI and CSS.
 * @author  Martin Rizzo | <martinrizzo@gmail.com>
 * @date    Mar 26, 2023
 * @repo    https://github.com/martin-rizzo/SDPromptViewer
 * @license MIT
 *//*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                      Stable Diffusion Prompt Viewer
      A plugin for "Eye of GNOME" that displays SD embedded prompts.
  
     Copyright (c) 2023 Martin Rizzo
  
     Permission is hereby granted, free of charge, to any person obtaining
     a copy of this software and associated documentation files (the
     "Software"), to deal in the Software without restriction, including
     without limitation the rights to use, copy, modify, merge, publish,
     distribute, sublicense, and/or sell copies of the Software, and to
     permit persons to whom the Software is furnished to do so, subject to
     the following conditions:
  
     The above copyright notice and this permission notice shall be
     included in all copies or substantial portions of the Software.
  
     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
     TORT OR OTHERWISE, ARISING FROM,OUT OF OR IN CONNECTION WITH THE
     SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
*/

/* FILE: resources/org.gnome.eog.plugins.sdprompt-viewer.gschema.xml */
#define SDPROMPT_VIEWER_GSCHEMA_ID    "org.gnome.eog.plugins.sdprompt-viewer"
#define SDPROMPT_VIEWER_GSCHEMA_PATH "/org/gnome/eog/plugins/sdprompt-viewer/"
#define     SETTINGS_SHOW_UNKNOWN_PARAMS    "show-unknown-params"
#define     SETTINGS_FORCE_MINIMUM_WIDTH    "force-minimum-width"
#define     SETTINGS_MINIMUM_WIDTH          "minimum-width"
#define     SETTINGS_FORCE_VISIBILITY       "force-visibility"
#define     SETTINGS_VISUAL_STYLE           "visual-style"

/* FILE: resources.xml */
#define RES_PREFIX   "/dev/martin-rizzo/sdprompt-viewer"
#define RES_PREFERENCES_UI RES_PREFIX"/sdprompt-viewer-preferences.ui"
#define RES_PLUGIN_UI      RES_PREFIX"/sdprompt-viewer-plugin.ui"

#define THEMES_RES_DIR RES_PREFIX"/themes"
