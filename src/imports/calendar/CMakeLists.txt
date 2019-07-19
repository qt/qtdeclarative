# Generated from calendar.pro.

#####################################################################
## qtlabscalendarplugin Plugin:
#####################################################################

add_qml_module(qtlabscalendarplugin
    CPP_PLUGIN
    URI "Qt.labs.calendar"
    VERSION "1.0"
    RESOURCE_PREFIX "/qt-project.org/imports"
    SOURCES
        qquickcalendar.cpp qquickcalendar_p.h
        qquickcalendarmodel.cpp qquickcalendarmodel_p.h
        qquickdayofweekmodel.cpp qquickdayofweekmodel_p.h
        qquickdayofweekrow.cpp qquickdayofweekrow_p.h
        qquickmonthgrid.cpp qquickmonthgrid_p.h
        qquickmonthmodel.cpp qquickmonthmodel_p.h
        qquickweeknumbercolumn.cpp qquickweeknumbercolumn_p.h
        qquickweeknumbermodel.cpp qquickweeknumbermodel_p.h
        qtlabscalendarplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

set(qml_files
    DayOfWeekRow.qml
    MonthGrid.qml
    WeekNumberColumn.qml
)

add_qt_resource(qtlabscalendarplugin qmake_Qt_labs_calendar
    PREFIX
        "/qt-project.org/imports/Qt/labs/calendar"
    FILES
        ${qml_files}
)

qt_install_qml_files(qtlabscalendarplugin
    FILES ${qml_files}
)


#### Keys ignored in scope 1:.:.:calendar.pro:<TRUE>:
# CONFIG = "no_cxx_module"
# IMPORT_VERSION = "1.0"
# OTHER_FILES = "qmldir"
# _LOADED = "qml_plugin"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:calendar.pro:NOT static:
# CONFIG = "qmlcache"
add_qt_docs(
    doc/qtlabscalendar.qdocconf
)

