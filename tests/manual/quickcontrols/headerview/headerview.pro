TEMPLATE = app
TARGET = headerview
QT += qml quick quick-private quickcontrols2 quickcontrols2-private \
      quicktemplates2-private quicktemplates2
SOURCES += main.cpp
RESOURCES += main.qml
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
