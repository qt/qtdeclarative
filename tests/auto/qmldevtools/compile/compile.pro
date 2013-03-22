option(host_build)
TARGET = tst_compile
force_bootstrap: \
    QT = bootstrap-private
else: \
    QT = core
QT += qmldevtools-private
macx:CONFIG -= app_bundle

SOURCES += tst_compile.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
