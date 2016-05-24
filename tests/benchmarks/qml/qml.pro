TEMPLATE = subdirs

SUBDIRS += \
           binding \
           compilation \
           javascript \
           holistic \
           qqmlcomponent \
           qqmlimage \
           qqmlmetaproperty \
           librarymetrics_performance \
#            script \ ### FIXME: doesn't build
           js

qtHaveModule(opengl): SUBDIRS += painting qquickwindow
qtHaveModule(widgets): SUBDIRS += creation

include(../trusted-benchmarks.pri)
