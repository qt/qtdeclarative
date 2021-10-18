HEADERS += \
    $$PWD/qquicklabsplatformcolordialog_p.h \
    $$PWD/qquicklabsplatformdialog_p.h \
    $$PWD/qquicklabsplatformfiledialog_p.h \
    $$PWD/qquicklabsplatformfolderdialog_p.h \
    $$PWD/qquicklabsplatformfontdialog_p.h \
    $$PWD/qquicklabsplatformicon_p.h \
    $$PWD/qquicklabsplatformiconloader_p.h \
    $$PWD/qquicklabsplatformmenu_p.h \
    $$PWD/qquicklabsplatformmenubar_p.h \
    $$PWD/qquicklabsplatformmenuitem_p.h \
    $$PWD/qquicklabsplatformmenuitemgroup_p.h \
    $$PWD/qquicklabsplatformmenuseparator_p.h \
    $$PWD/qquicklabsplatformmessagedialog_p.h \
    $$PWD/qquicklabsplatformstandardpaths_p.h

SOURCES += \
    $$PWD/qquicklabsplatformcolordialog.cpp \
    $$PWD/qquicklabsplatformdialog.cpp \
    $$PWD/qquicklabsplatformfiledialog.cpp \
    $$PWD/qquicklabsplatformfolderdialog.cpp \
    $$PWD/qquicklabsplatformfontdialog.cpp \
    $$PWD/qquicklabsplatformicon.cpp \
    $$PWD/qquicklabsplatformiconloader.cpp \
    $$PWD/qquicklabsplatformmenu.cpp \
    $$PWD/qquicklabsplatformmenubar.cpp \
    $$PWD/qquicklabsplatformmenuitem.cpp \
    $$PWD/qquicklabsplatformmenuitemgroup.cpp \
    $$PWD/qquicklabsplatformmenuseparator.cpp \
    $$PWD/qquicklabsplatformmessagedialog.cpp \
    $$PWD/qquicklabsplatformstandardpaths.cpp


qtConfig(systemtrayicon) {
    HEADERS += \
        $$PWD/qquicklabsplatformsystemtrayicon_p.h
    SOURCES += \
        $$PWD/qquicklabsplatformsystemtrayicon.cpp
}
