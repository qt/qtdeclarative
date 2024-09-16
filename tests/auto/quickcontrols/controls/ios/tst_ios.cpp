// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTest/quicktest.h>
#include <QtQuickControls2/qquickstyle.h>

int main(int argc, char *argv[])
{
    QTEST_SET_MAIN_SOURCE_PATH
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
    // The tests were originally written before native menus existed,
    // and some of them try to open menus, which we can't test natively.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);
    QQuickStyle::setStyle("iOS");
    return quick_test_main(argc, argv, "tst_controls::iOS", TST_CONTROLS_DATA);
}

