option(host_build)

force_bootstrap {
    QT = bootstrap-private
} else {
    QT = core
}

QT += qmldevtools-private

CONFIG -= app_bundle
SOURCES += main.cpp

load(qt_tool)
