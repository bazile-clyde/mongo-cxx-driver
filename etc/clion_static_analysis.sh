#!/bin/bash
################################################################################
#
# This script downloads CLion and runs its static analysis tool via the CLI
#
################################################################################
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

snap install clion --classic

# create a default inspection profile
cat <<EOF > default.xml
<profile version="1.0">
  <option name="myName" value="Default" />
  <inspection_tool class="OCInconsistentNaming" enabled="true" level="WEAK WARNING" enabled_by_default="true" />
</profile>
EOF

CLION=$(find /snap/clion -name "clion.sh")
$CLION inspect ~/mongo-cxx-driver ~/default.xml ~/results -v2 -d ~/mongo-cxx-driver/src