TEMPLATE = subdirs

SUBDIRS += \
    qtqml \
    folderlistmodel \
    localstorage \
    models \
    settings \
    statemachine

qtHaveModule(quick) {
    SUBDIRS += \
        qtquick2 \
        particles \
        window \
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel


QMLTYPEFILE = builtins.qmltypes

# install rule
builtins.files = $$QMLTYPEFILE
builtins.path = $$[QT_INSTALL_QML]
INSTALLS += builtins

# copy to build directory
!force_independent:if(!debug_and_release|!build_all|CONFIG(release, debug|release)) {
    defineReplace(qmlModStripSrcDir) {
        return($$relative_path($$1, $$_PRO_FILE_PWD_))
    }

    qmltypes2build.input = QMLTYPEFILE
    qmltypes2build.output = $$[QT_INSTALL_QML]/${QMAKE_FUNC_FILE_IN_qmlModStripSrcDir}
    !contains(TEMPLATE, vc.*): qmltypes2build.variable_out = PRE_TARGETDEPS
    qmltypes2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    qmltypes2build.name = COPY ${QMAKE_FILE_IN}
    qmltypes2build.CONFIG = no_link no_clean

    QMAKE_EXTRA_COMPILERS += qmltypes2build
}

# qmltypes target
!cross_compile:if(build_pass|!debug_and_release) {
    qtPrepareTool(QMLPLUGINDUMP, qmlplugindump)

    qmltypes.commands = $$QMLPLUGINDUMP -builtins > $$PWD/$$QMLTYPEFILE
    QMAKE_EXTRA_TARGETS += qmltypes
}
