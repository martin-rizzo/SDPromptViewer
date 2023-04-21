/**
 * @file    sdprompt-viewer-preferences.h
 * @brief   Declares the "Preferences" user interface for the plugin.
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
#ifndef __SDPROMPT_VIEWER_PREFERENCES_H__
#define __SDPROMPT_VIEWER_PREFERENCES_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <eog/eog-thumb-view.h>
#include <eog/eog-window.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

#define TYPE_SDPROMPT_VIEWER_PREFERENCES sdprompt_viewer_preferences_get_type()

/*------------------- TYPE CHECKING AND CASTING MACROS --------------------*/

/**
 * Macro to cast any #GObject to the custom #SDPromptViewerPreferences object.
 * Returns: A pointer to the casted #SDPromptViewerPreferences instance.
 **/
#define SDPROMPT_VIEWER_PREFERENCES(obj) (            \
    G_TYPE_CHECK_INSTANCE_CAST(                       \
        (obj), TYPE_SDPROMPT_VIEWER_PREFERENCES, SDPromptViewerPreferences ))

/**
 * Macro to cast #GObjectClass to the custom #SDPromptViewerPreferencesClass.
 * Returns: A pointer to the casted #SDPromptViewerPreferencesClass instance.
 **/
#define SDPROMPT_VIEWER_PREFERENCES_CLASS(klass) (    \
    G_TYPE_CHECK_CLASS_CAST(                          \
        (klass), TYPE_SDPROMPT_VIEWER_PREFERENCES, SDPromptViewerPreferencesClass ))

/**
 * Macro to retrieve the class of a #SDPromptViewerPreferences instance.
 * Returns: A pointer to the #SDPromptViewerPluginClass structure of @obj.
 **/
#define SDPROMPT_VIEWER_PREFERENCES_GET_CLASS(obj) (  \
    G_TYPE_INSTANCE_GET_CLASS(                        \
        (obj),  TYPE_SDPROMPT_VIEWER_PREFERENCES, SDPromptViewerPreferencesClass))

#define IS_SDPROMPT_VIEWER_PREFERENCES(obj) (         \
    G_TYPE_CHECK_INSTANCE_TYPE( (obj), TYPE_SDPROMPT_VIEWER_PREFERENCES))

#define IS_SDPROMPT_VIEWER_PREFERENCES_CLASS(klass) ( \
    G_TYPE_CHECK_CLASS_TYPE( (klass), TYPE_SDPROMPT_VIEWER_PREFERENCES))

/*-------------------------- PREFERENCES CLASS ----------------------------*/

typedef struct _SDPromptViewerPreferencesClass SDPromptViewerPreferencesClass;
struct         _SDPromptViewerPreferencesClass
{
    PeasExtensionBaseClass parent_class;
};

/*-------------------------- PREFERENCES OBJECT ---------------------------*/

typedef struct _SDPromptViewerPreferences SDPromptViewerPreferences;
struct         _SDPromptViewerPreferences
{
    PeasExtensionBase parent_instance;
    GtkBuilder *builder;
};

/*----------------------- PREFERENCES PRIVATE DATA ------------------------*/

typedef struct _SDPromptViewerPreferencesPrivate SDPromptViewerPreferencesPrivate;

/*---------------------------- PUBLIC FUNCTIONS ---------------------------*/

GType sdprompt_viewer_preferences_get_type( void ) G_GNUC_CONST;

G_GNUC_INTERNAL
void sdprompt_viewer_preferences_register_types( PeasObjectModule *module );


G_END_DECLS
#endif /* __SDPROMPT_VIEWER_PREFERENCES_H__ */
