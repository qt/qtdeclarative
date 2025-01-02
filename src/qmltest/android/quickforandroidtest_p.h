// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUICKFORANDROIDTEST_P_H
#define QUICKFORANDROIDTEST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickTest/quicktest.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

Q_QMLTEST_EXPORT int quick_for_android_test_main(int argc, char **argv, const char *name);

#define QUICK_FOR_ANDROID_TEST_MAIN(name)                      \
    int main(int argc, char **argv)                            \
    {                                                          \
        QTEST_SET_MAIN_SOURCE_PATH                             \
        return quick_for_android_test_main(argc, argv, #name); \
    }

QT_END_NAMESPACE

#endif
