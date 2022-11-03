#! /usr/bin/env python3
import multiprocessing
import os
import os.path
import pip
import platform
import subprocess
import shutil
import sys


def import_or_install(package):
    try:
        __import__(package)
    except ImportError:
        pip.main(['install', package])


def log_inf(s):
    print('>> ' + s)


def log_err(s):
    print('error: ' + s)


def run(cmd):
    log_inf(cmd)
    try:
        proc = subprocess.run(cmd, shell=True, stderr=subprocess.STDOUT)
    except KeyboardInterrupt:
        print('Command interrupted.', file=sys.stderr)
        exit(1)
    if proc.returncode != 0:
        print('Command failed.', file=sys.stderr)
        exit(1)


cmake_args = ['-DWITH_CONAN=ON']
native_build_args = []
conan_profile = 'intel'
build_type = 'Release'
qt_platform = 'macos'
build_dir = 'build'

if platform.system() == 'Linux':
    qt_platform = 'gcc_64'
    conan_profile = 'linux'
elif platform.system() == 'Darwin':
    qt_platform = 'macos'
    conan_profile = {
        'i386': 'intel',
        'arm': 'mx'}[platform.processor()]
elif platform.system() == 'Windows':
    native_build_args.extend(['/nologo', '/property:StopOnFirstFailure=true'])
else:
    native_build_args.extend(['--no-print-directory'])

cmake_args.extend([
    f'-DQT_DIR="{os.path.expanduser("~")}/Qt/6.4.0/{qt_platform}/lib/cmake"',
    f'-DWITH_CONAN_PROFILE={os.getcwd()}/conan/profiles/{conan_profile}',
    f'-DCMAKE_BUILD_TYPE={build_type}'])

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__))))

import_or_install('conan')
import_or_install('cmakelang')

run(f"{shutil.which('conan')} install {os.getcwd()}/conanfile.py \
      --profile {os.getcwd()}/conan/profiles/intel \
      --install-folder {os.getcwd()}/build/conan   \
      --settings build_type={build_type} --build outdated")

run(f"cmake -S . -B {build_dir} {' '.join(cmake_args)}")
run(f"cmake --build {build_dir}                      \
            --parallel {multiprocessing.cpu_count()} \
            -- {' '.join(native_build_args)}")
