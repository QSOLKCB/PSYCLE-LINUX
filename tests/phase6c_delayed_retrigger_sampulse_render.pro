# Project-authored Phase 6C fixed-frame Sampulse render probe.
TARGET = phase6c-delayed-retrigger-sampulse-render
TEMPLATE = app
include(../../build-systems/qmake/common.pri)
include($$TOP_SRC_DIR/psycle-core/qmake/psycle-core.pri)
CONFIG -= qt uic lex yacc
SOURCES += $$REPO_ROOT/tests/phase6c_delayed_retrigger_sampulse_render.cpp
INCLUDEPATH += $$PROBE_BUILD_DIR
DESTDIR = $$PROBE_BUILD_DIR
OBJECTS_DIR = $$PROBE_BUILD_DIR/objects
