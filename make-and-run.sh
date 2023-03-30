#!/bin/bash
#
# plugin will be installed in:
#   ~/.local/share/eog/plugins/libsdprompt-viewer.so
#   ~/.local/share/eog/plugins/sdprompt-viewer.plugin
#

test_images_path="$HOME/Extra/Test"

meson build && ninja -C build install
status=$?

if [ $status -eq 0 ]; then
  if [ -e "$test_images_path" ]; then
    EOG_DEBUG_PLUGINS='true'  eog "$test_images_path" & disown
  else
    EOG_DEBUG_PLUGINS='true'  eog &disown
  fi
fi

echo "Press any key to exit..."
read -n 1 -s
echo "Exiting..."

#sleep 20
#exit $status
