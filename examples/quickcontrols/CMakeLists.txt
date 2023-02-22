# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(attachedstyleproperties)
qt_internal_add_example(gallery)
if (TARGET Qt::Sql)
    add_subdirectory(chattutorial)
endif()
if (QT_FEATURE_quicktemplates2_calendar AND TARGET Qt::Sql)
    qt_internal_add_example(eventcalendar)
endif()
qt_internal_add_example(texteditor)
qt_internal_add_example(contactlist)
qt_internal_add_example(wearable)
qt_internal_add_example(imagine/automotive)
qt_internal_add_example(imagine/musicplayer)
if(TARGET Qt::Widgets)
    qt_internal_add_example(flatstyle)
endif()
