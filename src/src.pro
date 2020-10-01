TEMPLATE = subdirs
CONFIG += ordered
include($$OUT_PWD/qml/qtqml-config.pri)
include($$OUT_PWD/quick/qtquick-config.pri)
QT_FOR_CONFIG += qml qml-private quick-private

# We need qmltyperegistrar for all type registrations, even in qml
SUBDIRS += \
    qmltyperegistrar \
    qml \
    qmlmodels

qtConfig(qml-worker-script): \
    SUBDIRS += qmlworkerscript

qtHaveModule(gui):qtConfig(qml-animation) {
    SUBDIRS += quick

    qtConfig(quick-path): \
        SUBDIRS += quickshapes

    qtConfig(testlib): \
        SUBDIRS += qmltest

    qtConfig(quick-particles): \
        SUBDIRS += particles

    qtHaveModule(widgets): SUBDIRS += quickwidgets
    qtConfig(private_tests):qtConfig(testlib) {
        src_qmltest_doc_snippets.subdir = qmltest/doc/snippets
        src_qmltest_doc_snippets.target = sub-qmltest-doc-snippets
        src_qmltest_doc_snippets.depends = src_qmltest
        SUBDIRS += src_qmltest_doc_snippets
    }
}

SUBDIRS += \
    plugins \
    imports

qtConfig(qml-devtools) {
    SUBDIRS += \
        qmldevtools \
        qmlcompiler

    qmldevtools.depends = qml
    qmlcompiler.depends = qmldevtools
}


qtConfig(qml-network) {
    QT_FOR_CONFIG += network
    qtConfig(thread):qtConfig(localserver):qtConfig(qml-debug): SUBDIRS += qmldebug
}

DISTFILES += sync.profile configure.json
