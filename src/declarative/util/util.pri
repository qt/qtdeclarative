INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/qdeclarativeapplication.cpp \
    $$PWD/qdeclarativeutilmodule.cpp\
    $$PWD/qdeclarativeconnections.cpp \
    $$PWD/qdeclarativepackage.cpp \
    $$PWD/qdeclarativeanimation.cpp \
    $$PWD/qdeclarativesystempalette.cpp \
    $$PWD/qdeclarativespringanimation.cpp \
    $$PWD/qdeclarativesmoothedanimation.cpp \
    $$PWD/qdeclarativestate.cpp\
    $$PWD/qdeclarativetransitionmanager.cpp \
    $$PWD/qdeclarativestateoperations.cpp \
    $$PWD/qdeclarativepropertychanges.cpp \
    $$PWD/qdeclarativestategroup.cpp \
    $$PWD/qdeclarativetransition.cpp \
    $$PWD/qdeclarativelistmodel.cpp\
    $$PWD/qdeclarativelistaccessor.cpp \
    $$PWD/qdeclarativeopenmetaobject.cpp \
    $$PWD/qdeclarativetimeline.cpp \
    $$PWD/qdeclarativetimer.cpp \
    $$PWD/qdeclarativebind.cpp \
    $$PWD/qdeclarativepropertymap.cpp \
    $$PWD/qdeclarativepixmapcache.cpp \
    $$PWD/qdeclarativebehavior.cpp \
    $$PWD/qdeclarativefontloader.cpp \
    $$PWD/qdeclarativestyledtext.cpp \
    $$PWD/qdeclarativelistmodelworkeragent.cpp \
    $$PWD/qdeclarativepath.cpp \
    $$PWD/qdeclarativechangeset.cpp \
    $$PWD/qdeclarativelistcompositor.cpp \
    $$PWD/qlistmodelinterface.cpp \
    $$PWD/qdeclarativepathinterpolator.cpp \
    $$PWD/qdeclarativesvgparser.cpp

HEADERS += \
    $$PWD/qdeclarativeapplication_p.h \
    $$PWD/qdeclarativeutilmodule_p.h\
    $$PWD/qdeclarativeconnections_p.h \
    $$PWD/qdeclarativepackage_p.h \
    $$PWD/qdeclarativeanimation_p.h \
    $$PWD/qdeclarativeanimation_p_p.h \
    $$PWD/qdeclarativesystempalette_p.h \
    $$PWD/qdeclarativespringanimation_p.h \
    $$PWD/qdeclarativesmoothedanimation_p.h \
    $$PWD/qdeclarativesmoothedanimation_p_p.h \
    $$PWD/qdeclarativestate_p.h\
    $$PWD/qdeclarativestateoperations_p.h \
    $$PWD/qdeclarativepropertychanges_p.h \
    $$PWD/qdeclarativestate_p_p.h\
    $$PWD/qdeclarativetransitionmanager_p_p.h \
    $$PWD/qdeclarativestategroup_p.h \
    $$PWD/qdeclarativetransition_p.h \
    $$PWD/qdeclarativelistmodel_p.h\
    $$PWD/qdeclarativelistmodel_p_p.h\
    $$PWD/qdeclarativelistaccessor_p.h \
    $$PWD/qdeclarativeopenmetaobject_p.h \
    $$PWD/qdeclarativetimeline_p_p.h \
    $$PWD/qdeclarativetimer_p.h \
    $$PWD/qdeclarativebind_p.h \
    $$PWD/qdeclarativepropertymap.h \
    $$PWD/qdeclarativepixmapcache_p.h \
    $$PWD/qdeclarativebehavior_p.h \
    $$PWD/qdeclarativefontloader_p.h \
    $$PWD/qdeclarativestyledtext_p.h \
    $$PWD/qdeclarativelistmodelworkeragent_p.h \
    $$PWD/qdeclarativepath_p.h \
    $$PWD/qdeclarativepath_p_p.h \
    $$PWD/qdeclarativechangeset_p.h \
    $$PWD/qdeclarativelistcompositor_p.h \
    $$PWD/qlistmodelinterface_p.h \
    $$PWD/qdeclarativepathinterpolator_p.h \
    $$PWD/qdeclarativesvgparser_p.h

contains(QT_CONFIG, xmlpatterns) {
    QT+=xmlpatterns
    SOURCES += $$PWD/qdeclarativexmllistmodel.cpp
    HEADERS += $$PWD/qdeclarativexmllistmodel_p.h
} else {
    DEFINES += QT_NO_XMLPATTERNS
}
