TEMPLATE = subdirs

SUBDIRS += \
           binding \
           creation \
           javascript \
           holistic \
           pointers \
           qqmlcomponent \
           qqmlimage \
           qqmlmetaproperty \
           script \
           qmltime \
           js \
           qquickwindow

contains(QT_CONFIG, opengl): SUBDIRS += painting

include(../trusted-benchmarks.pri)
