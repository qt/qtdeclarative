/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "controlstestutils_p.h"

#include <QtTest/qsignalspy.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>

QQuickControlsTestUtils::QQuickControlsApplicationHelper::QQuickControlsApplicationHelper(QQmlDataTest *testCase,
    const QString &testFilePath, const QStringList &qmlImportPaths, const QVariantMap &initialProperties)
    : QQuickApplicationHelper(testCase, testFilePath, qmlImportPaths, initialProperties)
{
    if (ready)
        appWindow = qobject_cast<QQuickApplicationWindow*>(cleanup.data());
}

bool QQuickControlsTestUtils::QQuickStyleHelper::updateStyle(const QString &style)
{
    // If it's not the first time a style has been set and the new style is not different, do nothing.
    if (!currentStyle.isEmpty() && style == currentStyle)
        return false;

    engine.reset();
    currentStyle = style;
    qmlClearTypeRegistrations();
    engine.reset(new QQmlEngine);
    QQuickStyle::setStyle(style);

    QQmlComponent component(engine.data());
    component.setData(QString::fromUtf8("import QtQuick\nimport QtQuick.Controls\n Control { }").toUtf8(), QUrl());

    return true;
}

void QQuickControlsTestUtils::forEachControl(QQmlEngine *engine, const QString &qqc2ImportPath,
    const QString &sourcePath, const QString &targetPath, const QStringList &skipList,
    QQuickControlsTestUtils::ForEachCallback callback)
{
    // We cannot use QQmlComponent to load QML files directly from the source tree.
    // For styles that use internal QML types (eg. material/Ripple.qml), the source
    // dir would be added as an "implicit" import path overriding the actual import
    // path (qtbase/qml/QtQuick/Controls.2/Material). => The QML engine fails to load
    // the style C++ plugin from the implicit import path (the source dir).
    //
    // Therefore we only use the source tree for finding out the set of QML files that
    // a particular style implements, and then we locate the respective QML files in
    // the engine's import path. This way we can use QQmlComponent to load each QML file
    // for benchmarking.

    const QFileInfoList entries = QDir(qqc2ImportPath + QLatin1Char('/') + sourcePath).entryInfoList(
        QStringList(QStringLiteral("*.qml")), QDir::Files);
    for (const QFileInfo &entry : entries) {
        QString name = entry.baseName();
        if (!skipList.contains(name)) {
            const auto importPathList = engine->importPathList();
            for (const QString &importPath : importPathList) {
                QString name = entry.dir().dirName() + QLatin1Char('/') + entry.fileName();
                QString filePath = importPath + QLatin1Char('/') + targetPath + QLatin1Char('/') + entry.fileName();
                if (filePath.startsWith(QLatin1Char(':')))
                    filePath.prepend(QStringLiteral("qrc"));
                if (QFile::exists(filePath)) {
                    callback(name, QUrl::fromLocalFile(filePath));
                    break;
                } else {
                    QUrl url(filePath);
                    filePath = QQmlFile::urlToLocalFileOrQrc(filePath);
                    if (!filePath.isEmpty() && QFile::exists(filePath)) {
                        callback(name, url);
                        break;
                    }
                }
            }
        }
    }
}

void QQuickControlsTestUtils::addTestRowForEachControl(QQmlEngine *engine, const QString &qqc2ImportPath,
    const QString &sourcePath, const QString &targetPath, const QStringList &skipList)
{
    forEachControl(engine, qqc2ImportPath, sourcePath, targetPath, skipList, [&](const QString &relativePath, const QUrl &absoluteUrl) {
        QTest::newRow(qPrintable(relativePath)) << absoluteUrl;
    });
}

bool QQuickControlsTestUtils::verifyButtonClickable(QQuickAbstractButton *button)
{
    if (!button->window()) {
        qWarning() << "button" << button << "doesn't have an associated window";
        return false;
    }

    if (!button->isEnabled()) {
        qWarning() << "button" << button << "is not enabled";
        return false;
    }

    if (!button->isVisible()) {
        qWarning() << "button" << button << "is not visible";
        return false;
    }

    if (button->width() <= 0.0) {
        qWarning() << "button" << button << "must have a width greater than 0";
        return false;
    }

    if (button->height() <= 0.0) {
        qWarning() << "button" << button << "must have a height greater than 0";
        return false;
    }

    return true;
}

bool QQuickControlsTestUtils::clickButton(QQuickAbstractButton *button)
{
    if (!verifyButtonClickable(button))
        return false;

    QSignalSpy spy(button, &QQuickAbstractButton::clicked);
    if (!spy.isValid()) {
        qWarning() << "button" << button << "must have a valid clicked signal";
        return false;
    }

    const QPoint buttonCenter = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
    QTest::mouseClick(button->window(), Qt::LeftButton, Qt::NoModifier, buttonCenter);
    if (spy.count() != 1) {
        qWarning() << "clicked signal of button" << button << "was not emitted after clicking";
        return false;
    }

    return true;
}

bool QQuickControlsTestUtils::doubleClickButton(QQuickAbstractButton *button)
{
    if (!verifyButtonClickable(button))
        return false;

    QSignalSpy spy(button, &QQuickAbstractButton::clicked);
    if (!spy.isValid()) {
        qWarning() << "button" << button << "must have a valid doubleClicked signal";
        return false;
    }

    const QPoint buttonCenter = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
    QTest::mouseDClick(button->window(), Qt::LeftButton, Qt::NoModifier, buttonCenter);
    if (spy.count() != 1) {
        qWarning() << "doubleClicked signal of button" << button << "was not emitted after double-clicking";
        return false;
    }

    return true;
}
