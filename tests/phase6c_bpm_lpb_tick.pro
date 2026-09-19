# Project-authored observation harness. Build from the historical player/qmake
# directory so common.pri resolves the frozen Phase 6B source tree unchanged.
TARGET = phase6c-bpm-lpb-tick-probe
TEMPLATE = app
include(../../build-systems/qmake/common.pri)
include($$TOP_SRC_DIR/psycle-core/qmake/psycle-core.pri)
CONFIG -= qt uic lex yacc
SOURCES += $$REPO_ROOT/tests/phase6c_bpm_lpb_tick.cpp
DESTDIR = $$PROBE_BUILD_DIR
OBJECTS_DIR = $$PROBE_BUILD_DIR/objects
