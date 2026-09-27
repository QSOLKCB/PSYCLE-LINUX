TARGET = phase6c-sampler-ps1-pitch-render
TEMPLATE = app

include(../build-systems/qmake/common.pri)
include($$TOP_SRC_DIR/psycle-core/qmake/psycle-core.pri)

CONFIG -= qt uic lex yacc
CONFIG += console

SOURCES += $$REPO_ROOT/tests/phase6c_sampler_ps1_pitch_render.cpp
INCLUDEPATH += $$PROBE_BUILD_DIR
DESTDIR = $$PROBE_BUILD_DIR
OBJECTS_DIR = $$PROBE_BUILD_DIR/objects
MOC_DIR = $$PROBE_BUILD_DIR/moc
RCC_DIR = $$PROBE_BUILD_DIR/rcc
UI_DIR = $$PROBE_BUILD_DIR/ui
