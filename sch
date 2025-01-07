#!/bin/sh
$HOME/.local/bin/osscz "$@"
/usr/bin/ssh "$@"
$HOME/.local/bin/osscz RESET-SCHEME
