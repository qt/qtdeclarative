TEMPLATE = lib
QT = core

macos:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

SOURCES = foreign.cpp
HEADERS = foreign.h

CONFIG += metatypes static
