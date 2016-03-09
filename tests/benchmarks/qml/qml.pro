TEMPLATE = subdirs

SUBDIRS += \
           binding \
           compilation \
           javascript \
           holistic \
           pointers \
           qqmlcomponent \
           qqmlimage \
           qqmlmetaproperty \
#            script \ ### FIXME: doesn't build
           js

qtHaveModule(opengl): SUBDIRS += painting qquickwindow
qtHaveModule(widgets): SUBDIRS += creation

include(../trusted-benchmarks.pri)
