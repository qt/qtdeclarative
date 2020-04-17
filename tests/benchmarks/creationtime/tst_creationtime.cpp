/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qscopedpointer.h>
#include <QtTest>
#include <QtQml>
#include <QtQuickControls2/qquickstyle.h>

#include "../../auto/shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_CreationTime : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void defaultStyle();
    void defaultStyle_data();

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

void tst_CreationTime::defaultStyle()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::defaultStyle_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), "controls/default", "QtQuick/Controls/Default", QStringList() << "ApplicationWindow");
}

void tst_CreationTime::fusion()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::fusion_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), "controls/fusion", "QtQuick/Controls/Fusion", QStringList() << "ApplicationWindow" << "ButtonPanel" << "CheckIndicator" << "RadioIndicator" << "SliderGroove" << "SliderHandle" << "SwitchIndicator");
}

void tst_CreationTime::imagine()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::imagine_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), "controls/imagine", "QtQuick/Controls/Imagine", QStringList() << "ApplicationWindow");
}

void tst_CreationTime::material()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::material_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), "controls/material", "QtQuick/Controls/Material", QStringList() << "ApplicationWindow" << "Ripple" << "SliderHandle" << "CheckIndicator" << "RadioIndicator" << "SwitchIndicator" << "BoxShadow" << "ElevationEffect" << "CursorDelegate");
}

void tst_CreationTime::universal()
{
    QFETCH(QUrl, url);
    doBenchmark(styleHelper, url);
}

void tst_CreationTime::universal_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRowForEachControl(styleHelper.engine.data(), "controls/universal", "QtQuick/Controls/Universal", QStringList() << "ApplicationWindow" << "CheckIndicator" << "RadioIndicator" << "SwitchIndicator");
}

QTEST_MAIN(tst_CreationTime)

#include "tst_creationtime.moc"
