/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef CONTROLSTESTUTILS_P_H
#define CONTROLSTESTUTILS_P_H

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

#include <QtQuickTestUtils/private/visualtestutils_p.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQuickApplicationWindow;
class QQuickAbstractButton;

namespace QQuickControlsTestUtils
{
    class QQuickControlsApplicationHelper : public QQuickVisualTestUtils::QQuickApplicationHelper
    {
    public:
        QQuickControlsApplicationHelper(QQmlDataTest *testCase, const QString &testFilePath,
                const QStringList &qmlImportPaths = {},
                const QVariantMap &initialProperties = {});

        QQuickApplicationWindow *appWindow = nullptr;
    };

    struct QQuickStyleHelper
    {
        [[nodiscard]] bool updateStyle(const QString &style);

        QString currentStyle;
        QScopedPointer<QQmlEngine> engine;
    };

    typedef std::function<void(const QString &/*relativePath*/, const QUrl &/*absoluteUrl*/)> ForEachCallback;

    void forEachControl(QQmlEngine *engine, const QString &qqc2ImportPath, const QString &sourcePath,
        const QString &targetPath, const QStringList &skipList, ForEachCallback callback);
    void addTestRowForEachControl(QQmlEngine *engine, const QString &qqc2ImportPath, const QString &sourcePath,
        const QString &targetPath, const QStringList &skipList = QStringList());

    [[nodiscard]] bool verifyButtonClickable(QQuickAbstractButton *button);
    [[nodiscard]] bool clickButton(QQuickAbstractButton *button);
    [[nodiscard]] bool doubleClickButton(QQuickAbstractButton *button);
}

QT_END_NAMESPACE

#endif // CONTROLSTESTUTILS_P_H
