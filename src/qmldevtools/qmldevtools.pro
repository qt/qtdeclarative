option(host_build)
TARGET     = QtQmlDevTools
QT         = core-private
CONFIG    += minimal_syncqt internal_module generated_privates

MODULE_INCNAME = QtQml
INCLUDEPATH += $$OUT_PWD/../qml

# 2415: variable "xx" of static storage duration was declared but never referenced
# unused variable 'xx' [-Werror,-Wunused-const-variable]
intel_icc: WERROR += -ww2415
clang:if(greaterThan(QT_CLANG_MAJOR_VERSION, 3)|greaterThan(QT_CLANG_MINOR_VERSION, 3)): \
    WERROR += -Wno-error=unused-const-variable

include(../qml/common/common.pri)
include(../qml/parser/parser.pri)
include(../qml/compiler/compiler.pri)
include(../qml/qmldirparser/qmldirparser.pri)

load(qt_module)
