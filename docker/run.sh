#!/bin/bash
set -e

exec /basepath/etlded +exec server.cfg \
+set fs_game etrun \
+set fs_basepath /basepath \
+set fs_homepath /homepath \
+map ${DEFAULT_MAP:-oasis}
