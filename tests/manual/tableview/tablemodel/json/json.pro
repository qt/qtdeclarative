TEMPLATE = app
TARGET = json
QT += qml quick
SOURCES += main.cpp
RESOURCES += \
    main.qml \
    JsonData.js

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
