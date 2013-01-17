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

qtHaveModule(opengl): SUBDIRS += painting

include(../trusted-benchmarks.pri)
