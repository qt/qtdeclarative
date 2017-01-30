option(host_build)

QT = qmldevtools-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

SOURCES = qmlcachegen.cpp
TARGET = qmlcachegen

BUILD_INTEGRATION = qmlcache.prf
!force_independent {
    qmake_integration.input = BUILD_INTEGRATION
    qmake_integration.output = $$[QT_HOST_DATA]/mkspecs/features/${QMAKE_FILE_BASE}.prf
    qmake_integration.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    qmake_integration.name = COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    qmake_integration.CONFIG = no_clean no_link
    !contains(TEMPLATE, vc.*): qmake_integration.variable_out = GENERATED_FILES
    QMAKE_EXTRA_COMPILERS += qmake_integration
}

qmake_integration_installs.files = $$BUILD_INTEGRATION
qmake_integration_installs.path = $$[QT_HOST_DATA]/mkspecs/features
INSTALLS += qmake_integration_installs

load(qt_tool)
