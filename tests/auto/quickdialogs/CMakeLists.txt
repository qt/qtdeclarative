# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT ANDROID) # temporarily disabled until QTBUG-100991 is fixed
    if(QT_FEATURE_quick_listview AND QT_FEATURE_quick_draganddrop)
        if(QT_FEATURE_qml_labs)
            add_subdirectory(qquickfiledialogimpl)
            add_subdirectory(qquickfolderdialogimpl)
        endif()
        add_subdirectory(qquickfontdialogimpl)
    endif()
    add_subdirectory(qquickcolordialogimpl)
endif()
add_subdirectory(qquickmessagedialogimpl)
