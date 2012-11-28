
QT += quick

SOURCES += \
    simplematerial.cpp

OTHER_FILES += \
    main.qml

sources.files = $$SOURCES $$OTHER_FILES simplematerial.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/scenegraph/simplematerial
target.path = $$sources.path
INSTALLS += sources target
