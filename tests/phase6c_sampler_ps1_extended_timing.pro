# Project-authored Phase 6C PS1 E-D3/E-C3 runtime timing probe.
TARGET = phase6c-sampler-ps1-extended-timing-probe
TEMPLATE = app
include(../../build-systems/qmake/common.pri)
include($$TOP_SRC_DIR/psycle-core/qmake/psycle-core.pri)
CONFIG -= qt uic lex yacc
SOURCES += $$REPO_ROOT/tests/phase6c_sampler_ps1_extended_timing.cpp
DESTDIR = $$PROBE_BUILD_DIR
OBJECTS_DIR = $$PROBE_BUILD_DIR/objects
