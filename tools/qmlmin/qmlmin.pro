option(host_build)

force_bootstrap {
    QT = bootstrap-private
} else {
    QT = core
}

QT += qmldevtools-private

SOURCES += main.cpp

load(qt_tool)
