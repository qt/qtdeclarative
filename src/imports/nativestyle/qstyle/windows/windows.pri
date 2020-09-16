
INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qquickwindowsstyle_p.h \
    $$PWD/qquickwindowsstyle_p_p.h \
#    $$PWD/qquickwindowsvistastyle_p.h \
#    $$PWD/qquickwindowsvistastyle_p_p.h \
    $$PWD/qquickwindowsxpstyle_p.h \
    $$PWD/qquickwindowsxpstyle_p_p.h

SOURCES += \
    $$PWD/qquickwindowsstyle.cpp \
#    $$PWD/qquickwindowsvistastyle.cpp \
    $$PWD/qquickwindowsxpstyle.cpp

QMAKE_USE_PRIVATE += user32 gdi32
LIBS_PRIVATE *= -luxtheme
