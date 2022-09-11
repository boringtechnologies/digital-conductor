#! /usr/bin/env python3
import os
import os.path
import sys

def generate_qrc(prefix, files):
    out = ''

    out += '<!-- THIS FILE IS AUTO-GENERATED. DO NOT EDIT! -->\n'
    out += '\n'

    out += '<RCC>\n'
    out += '  <qresource prefix="%s">\n' % prefix

    for f in files:
        out += '    <file>%s</file>\n' % f

    out += '  </qresource>\n'
    out += '</RCC>\n'

    return out

os.chdir(os.path.dirname(
    os.path.dirname(os.path.realpath(__file__))))

if len(sys.argv) < 4:
    print("usage: %s <output qrc file> <qrc prefix> <qml file>..." % sys.argv[0],
          file=sys.stderr)
    exit(1)

qrc_filename = sys.argv[1]
qrc_prefix = sys.argv[2]
qml_files = sys.argv[3:]

qrc_content = generate_qrc(qrc_prefix, qml_files)

print('Writing %s' %  os.path.relpath(qrc_filename), file=sys.stderr)
with open(qrc_filename, 'w') as fp:
    fp.write(qrc_content)
