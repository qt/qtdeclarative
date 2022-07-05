# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if(NOT ANDROID) # temporarily disabled until QTBUG-100991 is fixed
add_subdirectory(qquickfiledialogimpl)
add_subdirectory(qquickfolderdialogimpl)
add_subdirectory(qquickfontdialogimpl)
add_subdirectory(qquickcolordialogimpl)
endif()
add_subdirectory(qquickmessagedialogimpl)
