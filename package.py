#!/usr/bin/env python3

#
# Asp package builder.
#

import sys, os, subprocess, shutil, pathlib

# Change directory to the root of the repository.
try:
    repo_home_dir = subprocess.run \
        (['git', 'rev-parse', '--show-toplevel'],
         check = True, capture_output = True) \
         .stdout.decode().splitlines()[0]
except:
    sys.exit('Execute this script from an Asp Git repository')
if os.getcwd() != repo_home_dir:
    print('Changing to repository home')
    os.chdir(repo_home_dir)

# Ensure that the directory looks like an Asp repository.
expected_dirs = ['engine', 'compiler', 'appspec', 'info', 'util', 'standalone']
for expected_dir in expected_dirs:
    if not os.path.isdir(expected_dir):
        sys.exit('This doesn\'t look like an Asp repository')

# Prepare to perform a clean build.
build_dir = 'build-package'
if os.path.exists(build_dir):
    if os.path.isdir(build_dir):
        answer = input \
            ('Replace \'%s\' directory with fresh build? ' % build_dir)
        if len(answer) == 0 or answer.upper()[0] != 'Y':
            sys.exit('Bailing')
        shutil.rmtree(build_dir)
    else:
        sys.exit \
            ('\'%s\' exists and is not a directory; remove and try again' % \
             build_dir)

# Determine if the repository is clean.
repo_changes = subprocess.run \
    (['git', 'status', '--porcelain'],
     check = True, capture_output = True) \
     .stdout.decode()
is_repo_clean = len(repo_changes) == 0

# Create an empty directory where we can perform a clean build.
os.mkdir(build_dir)
os.chdir(build_dir)

# Make packages for the current platform.
if sys.platform.startswith('linux'):

    print('Building Linux packages')

    print('Configuring')
    subprocess.check_call \
        (['cmake', '-DBUILD_SHARED_LIBS=ON', '-DINSTALL_DEV=ON',
          '-DCMAKE_INSTALL_PREFIX=/usr',
          '-DCMAKE_BUILD_TYPE=Release', '..'])

    print('Packaging source')
    subprocess.check_call \
        (['cpack', '--config', 'CPackSourceConfig.cmake',
          '-G', 'TBZ2;TGZ'])

    print('Packaging binary tar balls')
    subprocess.check_call \
        (['cpack', '--config', 'CPackConfig.cmake',
          '-G', 'TBZ2;TGZ'])

elif sys.platform.startswith('win'):

    print('Building Windows packages')

    print('Configuring')
    subprocess.check_call \
        (['cmake', '-DBUILD_SHARED_LIBS=ON', '-DINSTALL_DEV=ON', '..'])

    print('Packaging source')
    subprocess.check_call \
        (['cpack', '--config', 'CPackSourceConfig.cmake',
          '-G', 'ZIP'])

    print('Building')
    subprocess.check_call \
        (['cmake', '--build', '.', '--config', 'Release'])

    print('Packaging installer')
    subprocess.check_call \
        (['cpack', '--config', 'CPackConfig.cmake',
          '-G', 'NSIS'])


else:
    print('Unrecognized system')

print('Done building packages')
if not is_repo_clean:
    print \
        ('WARNING: Packages were built from a dirty repository.' \
         ' DO NOT DEPLOY!!!')
