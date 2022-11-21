// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQuickTest/quicktest.h>
#include <QtQuickControls2/qquickstyle.h>

int main(int argc, char *argv[])
{
    QTEST_SET_MAIN_SOURCE_PATH
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
    // See comment in tst_windows.cpp.
    qputenv("QT_QUICK_CONTROLS_IGNORE_CUSTOMIZATION_WARNINGS", "1");
    QQuickStyle::setStyle("macOS");
    return quick_test_main(argc, argv, "tst_controls::macOS", TST_CONTROLS_DATA);
}

