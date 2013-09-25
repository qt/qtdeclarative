option(host_build)

force_bootstrap {
    QT = bootstrap-private
} else {
    QT = core
}

QT += qmldevtools-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

CONFIG -= app_bundle
SOURCES += main.cpp

load(qt_tool)
