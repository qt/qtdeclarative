option(host_build)
TARGET = tst_compile
force_bootstrap {
    QT = bootstrap-private
    !build_pass: CONFIG += release
} else {
    QT = core
    !build_pass:contains(QT_CONFIG, debug_and_release): CONFIG += release
}
QT += qmldevtools-private
macx:CONFIG -= app_bundle

SOURCES += tst_compile.cpp

load(qt_common)
