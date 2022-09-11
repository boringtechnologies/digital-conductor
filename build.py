#! /usr/bin/env python3
import multiprocessing
import os
import os.path
import platform
import subprocess
import sys

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

build_args = []

if platform.system() == 'Windows':
    build_args = [
        '/nologo',
        '/property:StopOnFirstFailure=true',
        ]
else:
    build_args = [
        '-j%s' % multiprocessing.cpu_count(),
        '--no-print-directory',
        ]

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__))))

log_inf('mkdir -p build')
os.makedirs('build', exist_ok=True)

log_inf('cd build')
os.chdir('build')

run('cmake ..')
run('cmake --build . -- %s' % ' '.join(build_args))
