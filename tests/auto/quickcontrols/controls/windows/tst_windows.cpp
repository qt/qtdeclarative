// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTest/quicktest.h>
#include <QtQuickControls2/qquickstyle.h>

int main(int argc, char *argv[])
{
    QTEST_SET_MAIN_SOURCE_PATH
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
    // We have tst_customization::override and its controlNotCustomizable check
    // to ensure that customizing native styles results in warnings,
    // rather than having to ignore specific warnings for each control we customize in tests.
    // We do have the various test_default/test_empty test functions to check
    // that a "default-constructed" control doesn't result in any warnings, but because
    // we set this, those won't be useful for ensuring that customization warnings aren't
    // issued when default-constructing controls. For that we have
    // tst_customization::noCustomizationWarningsForDefaultControls.
    qputenv("QT_QUICK_CONTROLS_IGNORE_CUSTOMIZATION_WARNINGS", "1");
    // The tests were originally written before native menus existed,
    // and some of them try to open menus, which we can't test natively.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);
    QQuickStyle::setStyle("Windows");
    return quick_test_main(argc, argv, "tst_controls::Windows", TST_CONTROLS_DATA);
}
