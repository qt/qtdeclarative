option(host_build)

force_bootstrap {
    QT = bootstrap-private
} else {
    QT = core
}

QT += qmldevtools-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

SOURCES += main.cpp

load(qt_tool)
