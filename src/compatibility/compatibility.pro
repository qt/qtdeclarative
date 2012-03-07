TARGET = QtDeclarative
TEMPLATE = subdirs

MODULE_PRI = ../../modules/qt_declarative.pri

QT = qml

pritarget.path = $$[QT_HOST_DATA]/mkspecs/modules
pritarget.files = $$MODULE_PRI
INSTALLS += pritarget

#load up the headers info
CONFIG += qt_install_headers
HEADERS_PRI = ../../include/QtDeclarative/headers.pri
include($$HEADERS_PRI, "", true)|clear(HEADERS_PRI)

INSTALL_HEADERS = $$SYNCQT.HEADER_FILES

flat_headers.files = $$INSTALL_HEADERS
flat_headers.path = $$[QT_INSTALL_HEADERS]/Qt
INSTALLS += flat_headers

class_headers.path = $$[QT_INSTALL_HEADERS]/$$TARGET
class_headers.files = $$SYNCQT.HEADER_CLASSES
INSTALLS += class_headers

targ_headers.files = $$INSTALL_HEADERS
targ_headers.path = $$[QT_INSTALL_HEADERS]/$$TARGET
INSTALLS += targ_headers

private_headers.files = $$SYNCQT.PRIVATE_HEADER_FILES
private_headers.path = $$[QT_INSTALL_HEADERS]/$$TARGET/$${QT.declarative.VERSION}/$$TARGET/private
INSTALLS += private_headers
