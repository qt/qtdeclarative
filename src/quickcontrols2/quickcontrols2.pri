HEADERS += \
    $$PWD/qquickanimatednode_p.h \
    $$PWD/qquickattachedobject_p.h \
    $$PWD/qquickcolor_p.h \
    $$PWD/qquickcolorimageprovider_p.h \
    $$PWD/qquickiconimage_p.h \
    $$PWD/qquickiconimage_p_p.h \
    $$PWD/qquickiconlabel_p.h \
    $$PWD/qquickiconlabel_p_p.h \
    $$PWD/qquickplaceholdertext_p.h \
    $$PWD/qquickproxytheme_p.h \
    $$PWD/qquickstyle.h \
    $$PWD/qquickstyle_p.h \
    $$PWD/qquickstyleplugin_p.h \
    $$PWD/qquickstyleselector_p.h \
    $$PWD/qquickstyleselector_p_p.h \
    $$PWD/qquickpaddedrectangle_p.h

SOURCES += \
    $$PWD/qquickanimatednode.cpp \
    $$PWD/qquickattachedobject.cpp \
    $$PWD/qquickcolor.cpp \
    $$PWD/qquickcolorimageprovider.cpp \
    $$PWD/qquickiconimage.cpp \
    $$PWD/qquickiconlabel.cpp \
    $$PWD/qquickplaceholdertext.cpp \
    $$PWD/qquickproxytheme.cpp \
    $$PWD/qquickstyle.cpp \
    $$PWD/qquickstyleplugin.cpp \
    $$PWD/qquickstyleselector.cpp \
    $$PWD/qquickpaddedrectangle.cpp

qtConfig(quick-listview):qtConfig(quick-pathview) {
    HEADERS += \
        $$PWD/qquicktumblerview_p.h
    SOURCES += \
        $$PWD/qquicktumblerview.cpp
}
