# Project-authored Phase 6C delayed/retrigger observation probe.
TARGET = phase6c-delayed-retrigger-probe
TEMPLATE = app
include(../../build-systems/qmake/common.pri)
include($$TOP_SRC_DIR/psycle-core/qmake/psycle-core.pri)
CONFIG -= qt uic lex yacc
SOURCES += $$REPO_ROOT/tests/phase6c_delayed_retrigger.cpp
DESTDIR = $$PROBE_BUILD_DIR
OBJECTS_DIR = $$PROBE_BUILD_DIR/objects
