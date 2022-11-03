#!/usr/bin/env python3
import os
import os.path
import platform
import shutil
import subprocess


def rm(f):
    if os.path.isdir(f):
        print('Removing ' + f)
        if platform.system() == 'Windows':
            subprocess.run('rmdir /q /s "%s"' % f, shell=True, check=True)
        else:
            shutil.rmtree(f)
    elif os.path.isfile(f):
        print('Removing ' + f)
        os.unlink(f)


os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__))))

rm('bin')
rm('build')
rm('compile_commands.json')
