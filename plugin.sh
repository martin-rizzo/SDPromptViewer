#!/bin/bash
#
# plugin will be installed in:
#   ~/.local/share/eog/plugins/libsdprompt-viewer.so
#   ~/.local/share/eog/plugins/sdprompt-viewer.plugin
#

status=0

test_images_path="$HOME/Extra/Test"

# Define the functions to be executed for each command
build() {
  echo "Executing build command..."
  meson build && ninja -C build
  status=$?
}

install() {
  echo "Executing install command..."
  meson build && ninja -C build install
  status=$?
}

run() {
  echo "Executing run command..."
  install
  if [ $status -eq 0 ]; then
    if [ -e "$test_images_path" ]; then
      EOG_DEBUG_PLUGINS='true'  eog "$test_images_path" & disown
    else
      EOG_DEBUG_PLUGINS='true'  eog &disown
    fi
  fi
}

clean() {
  echo "Executing clean command..."
  # TODO: add clean logic here.
  status=0
}

# if no arguments are provided, show the help message
if [ $# -eq 0 ]; then
  echo "Usage: $0 [build|install|run|clean]"
  echo "  build - Build the project"
  echo "  run   - Run the project"
  echo "  install - Install the project"
  echo "  clean - Clean the project"
  exit 1
fi

# execute the appropriate function based on the first argument
case "$1" in
  build)
    build
    ;;
  run)
    run
    ;;
  clean)
    clean
    ;;
  *)
    echo "Invalid command: $1"
    echo "Usage: $0 [build|run|clean]"
    exit 1
    ;;
esac

#sleep 20
#echo "Press any key to exit..."
#read -n 1 -s


echo "Exiting..."
exit $status
