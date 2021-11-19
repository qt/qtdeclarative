/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtCore/qscopedpointer.h>
#include <QtTest>
#include <QtQml>
#include <QtQuickControls2/qquickstyle.h>

#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

using namespace QQuickControlsTestUtils;
using namespace QQuickVisualTestUtils;

class tst_CreationTime : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void basic();
    void basic_data();

    void fusion();
    void fusion_data();

    void imagine();
    void imagine_data();

    void material();
    void material_data();

    void universal();
    void universal_data();

private:
    QQuickStyleHelper styleHelper;
};

void tst_CreationTime::initTestCase()
{
    styleHelper.engine.reset(new QQmlEngine);
}

void tst_CreationTime::init()
{
    styleHelper.engine->clearComponentCache();
}

static void doBenchmark(QQuickStyleHelper &styleHelper, const QUrl &url)
{
    const QString tagStr = QString::fromLatin1(QTest::currentDataTag());
    QStringList styleAndFileName = tagStr.split('/');
    QCOMPARE(styleAndFileName.size(), 2);
    QString style = styleAndFileName.first();
    style[0] = style.at(0).toUpper();
    styleHelper.updateStyle(style);

    QQmlComponent component(styleHelper.engine.data());
    component.loadUrl(url);

    QObjectList objects;
    objects.reserve(4096);
    QBENCHMARK {
        QObject *object = component.create();
        QVERIFY2(object, qPrintable(component.errorString()));
        objects += object;
    }
    qDeleteAll(objects);
}

void tst_CreationTime::basic()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::basic_data()
{
    QTest::addColumn<QUrl>("url");
    // Calendar is excluded because it's a singleton and can't be created.
    // TreeViewDelegate is excluded since it's a delegate that can only be created by TreeView.
    addTestRowForEachControl(styleHelper.engine.data(), QQC2_IMPORT_PATH, "basic", "QtQuick/Controls/Basic",
        QStringList() << "ApplicationWindow" << "Calendar" << "TreeViewDelegate");
}

void tst_CreationTime::fusion()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::fusion_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), QQC2_IMPORT_PATH, "fusion", "QtQuick/Controls/Fusion",
        QStringList() << "ApplicationWindow" << "ButtonPanel" << "CheckIndicator"
            << "RadioIndicator" << "SliderGroove" << "SliderHandle" << "SwitchIndicator");
}

void tst_CreationTime::imagine()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::imagine_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), QQC2_IMPORT_PATH, "imagine", "QtQuick/Controls/Imagine",
        QStringList() << "ApplicationWindow");
}

void tst_CreationTime::material()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::material_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), QQC2_IMPORT_PATH, "material", "QtQuick/Controls/Material",
        QStringList() << "ApplicationWindow" << "Ripple" << "SliderHandle" << "CheckIndicator" << "RadioIndicator"
            << "SwitchIndicator" << "BoxShadow" << "ElevationEffect" << "CursorDelegate");
}

void tst_CreationTime::universal()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::universal_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), QQC2_IMPORT_PATH, "universal", "QtQuick/Controls/Universal",
        QStringList() << "ApplicationWindow" << "CheckIndicator" << "RadioIndicator" << "SwitchIndicator");
}

QTEST_MAIN(tst_CreationTime)

#include "tst_creationtime.moc"
