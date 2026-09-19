# Project-authored harness; build from the historical player/qmake directory so
# common.pri resolves its preserved source root exactly as in the player build.
TARGET = phase6c-serialization-probe
TEMPLATE = app
include(../../build-systems/qmake/common.pri)
include($$TOP_SRC_DIR/psycle-core/qmake/psycle-core.pri)
CONFIG -= qt uic lex yacc
SOURCES += $$REPO_ROOT/tests/phase6c_serialization.cpp
DESTDIR = $$PROBE_BUILD_DIR
OBJECTS_DIR = $$PROBE_BUILD_DIR/objects
