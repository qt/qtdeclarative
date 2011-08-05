TEMPLATE = subdirs

SUBDIRS += \
           binding \
           creation \
           javascript \
           holistic \
           pointers \
           qdeclarativecomponent \
           qdeclarativeimage \
           qdeclarativemetaproperty \
           script \
           qmltime \
           js

contains(QT_CONFIG, opengl): SUBDIRS += painting

include(../trusted-benchmarks.pri)
