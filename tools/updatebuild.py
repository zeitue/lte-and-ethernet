#!/usr/bin/env python3

import sys
import os
import subprocess
import datetime

# generate the build number from the data and time
build = "#define BUILD \"" + datetime.datetime.now().strftime(
    "%Y%m%d%H%M%S") + "\"\n"

# get the git tag for use as the version number
try:
    output = str(subprocess.check_output(
        ['git', 'describe', '--always', '--tags']),
        'utf-8').rstrip()
except subprocess.CalledProcessError as e:
    output = "v0.0.0"
version = "#define VERSION \"" + output + "\"\n"

file_content = build + version

out_file = os.path.join(sys.argv[1], 'clientname/build.h')

# write the build header
with open(out_file, 'w') as build_file:
    build_file.write(file_content)
