INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qquickstyleitem.h \
    $$PWD/qquickstyleitembutton.h \
    $$PWD/qquickstyleitemgroupbox.h \
    $$PWD/qquickstyleitemcheckbox.h \
    $$PWD/qquickstyleitemradiobutton.h \
    $$PWD/qquickstyleitemslider.h \
    $$PWD/qquickstyleitemspinbox.h \
    $$PWD/qquickstyleitemtextfield.h \
    $$PWD/qquickstyleitemframe.h \
    $$PWD/qquickstyleitemcombobox.h \
    $$PWD/qquickstyleitemscrollbar.h \
    $$PWD/qquickstyleitemprogressbar.h \
    $$PWD/qquickstyleitemdial.h \

SOURCES += \
    $$PWD/qquickstyleitem.cpp \
    $$PWD/qquickstyleitembutton.cpp \
    $$PWD/qquickstyleitemgroupbox.cpp \
    $$PWD/qquickstyleitemcheckbox.cpp \
    $$PWD/qquickstyleitemradiobutton.cpp \
    $$PWD/qquickstyleitemslider.cpp \
    $$PWD/qquickstyleitemspinbox.cpp \
    $$PWD/qquickstyleitemtextfield.cpp \
    $$PWD/qquickstyleitemframe.cpp \
    $$PWD/qquickstyleitemcombobox.cpp \
    $$PWD/qquickstyleitemscrollbar.cpp \
    $$PWD/qquickstyleitemprogressbar.cpp \
    $$PWD/qquickstyleitemdial.cpp \

macos {
    HEADERS += $$PWD/qquickstyleitemscrollviewcorner.h
    SOURCES += $$PWD/qquickstyleitemscrollviewcorner.cpp
}
