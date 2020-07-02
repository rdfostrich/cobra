#!/bin/bash
# Run ostrich
if [ "$1" == "--debug" ]; then
    shift
    /opt/cobra/run-debug.sh $@
else
    /opt/cobra/build/cobra-evaluate $@
fi
