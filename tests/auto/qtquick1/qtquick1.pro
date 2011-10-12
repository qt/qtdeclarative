TEMPLATE = subdirs

SUBDIRS += \
           qdeclarativeview \
           qdeclarativeviewer \
           moduleqt47

contains(QT_CONFIG, private_tests) {
    SUBDIRS += \
           qdeclarativeanchors \
           qdeclarativeanimatedimage \
           qdeclarativeanimations \
           qdeclarativeapplication \
           qdeclarativebehaviors \
           qdeclarativebinding \
           qdeclarativeborderimage \
           qdeclarativeconnection \
           qdeclarativeflickable \
           qdeclarativeflipable \
           qdeclarativefocusscope \
           qdeclarativefontloader \
           qdeclarativegridview \
           qdeclarativeimage \
           qdeclarativeimageprovider \
           qdeclarativeitem \
           qdeclarativelayoutitem \
#           qdeclarativelistmodel \
           qdeclarativelistview \
           qdeclarativeloader \
           qdeclarativemousearea \
           qdeclarativeparticles \
           qdeclarativepathview \
           qdeclarativepincharea \
           qdeclarativepositioners \
           qdeclarativerepeater \
           qdeclarativesmoothedanimation \
           qdeclarativespringanimation \
           qdeclarativestates \
           qdeclarativesystempalette \
           qdeclarativetext \
           qdeclarativetextedit \
           qdeclarativetextinput \
           qdeclarativetimer \
           qdeclarativevisualdatamodel \
           qdeclarativexmllistmodel \
           examples

    # This test needs the xmlpatterns module
    !contains(QT_CONFIG,xmlpatterns): SUBDIRS -= qdeclarativexmllistmodel
}
