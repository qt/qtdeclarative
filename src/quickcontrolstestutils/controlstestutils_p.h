// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

class QQmlComponent;
class QQmlEngine;
class QQuickApplicationWindow;
class QQuickAbstractButton;

namespace QQuickControlsTestUtils
{
    class QQuickControlsApplicationHelper : public QQuickVisualTestUtils::QQuickApplicationHelper
    {
    public:
        QQuickControlsApplicationHelper(QQmlDataTest *testCase, const QString &testFilePath,
                const QVariantMap &initialProperties = {},
                const QStringList &qmlImportPaths = {});

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

    class ComponentCreator : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_MOC_INCLUDE(<QtQml/qqmlcomponent.h>)

    public:
        Q_INVOKABLE QQmlComponent *createComponent(const QByteArray &data);
    };

    class StyleInfo : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString styleName READ styleName CONSTANT FINAL)
        QML_ELEMENT
        QML_SINGLETON

    public:
        QString styleName() const;
    };
}

QT_END_NAMESPACE

#endif // CONTROLSTESTUTILS_P_H
