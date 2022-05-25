/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtQml/qqmlapplicationengine.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

class tst_AndroidAssets : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void loadsFromAssetsPath();
    void loadsFromAssetsUrl();

private:

    static QString pathPrefix()
    {
#ifdef Q_OS_ANDROID
        return QStringLiteral("assets:");
#else
        // Even when not running on android we can check that the copying to build dir worked.
        return QCoreApplication::applicationDirPath() + QStringLiteral("/android-build/assets");
#endif
    }

    static QString urlPrefix() {
#ifdef Q_OS_ANDROID
        return pathPrefix();
#else
        return QStringLiteral("file:") + pathPrefix();
#endif
    }
};


void tst_AndroidAssets::loadsFromAssetsPath()
{
    QQmlApplicationEngine engine;

    // load QML file from assets, by path:
    engine.load(pathPrefix() + QStringLiteral("/qml/main.qml"));
    QTRY_VERIFY(engine.rootObjects().length() == 1);
}

void tst_AndroidAssets::loadsFromAssetsUrl()
{
    QQmlApplicationEngine engine;

    // load QML file from assets, by URL:
    engine.load(QUrl(urlPrefix() + QStringLiteral("/qml/main.qml")));
    QTRY_VERIFY(engine.rootObjects().length() == 1);
}

QTEST_MAIN(tst_AndroidAssets)

#include "tst_androidassets.moc"
