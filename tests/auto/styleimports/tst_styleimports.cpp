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

#include <QtCore/qregularexpression.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

#include "../shared/util.h"

class tst_StyleImports : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void initTestCase();

    void cleanup();

    void select_data();
    void select();

    void platformSelectors();

    void importStyleWithoutControls_data();
    void importStyleWithoutControls();
};

void tst_StyleImports::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void tst_StyleImports::cleanup()
{
    qmlClearTypeRegistrations();
}

void tst_StyleImports::select_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("fallback");
    QTest::addColumn<QString>("expected");

    // Action.qml exists in the "data" style and the Default style.
    QTest::newRow("control=Action,style=empty,fallback=empty") << "Action.qml" << "" << "" << "Default";
    QTest::newRow("control=Action,style=fs,fallback=empty") << "Action.qml" << "FileSystemStyle" << "" << "Default";
    QTest::newRow("control=Action,style=qrc,fallback=empty") << "Action.qml" << "ResourceStyle" << "" << "Default";
    QTest::newRow("control=Action,style=nosuch,fallback=empty") << "Action.qml" << "NoSuchStyle" << "" << "Default";
    QTest::newRow("control=Action,style=data,fallback=empty") << "Action.qml" << "data" << "" << "data";

    QTest::newRow("control=Action,style=empty,fallback=mat") << "Action.qml" << "" << "Material" << "";
    QTest::newRow("control=Action,style=fs,fallback=mat") << "Action.qml" << "FileSystemStyle" << "Material" << "Default";
    QTest::newRow("control=Action,style=qrc,fallback=mat") << "Action.qml" << "ResourceStyle" << "Material" << "Default";
    QTest::newRow("control=Action,style=nosuch,fallback=mat") << "Action.qml" << "NoSuchStyle" << "Material" << "Default";
    QTest::newRow("control=Action,style=data,fallback=mat") << "Action.qml" << "data" << "Material" << "data";

    // ScrollView.qml only exists in the Default style.
    QTest::newRow("control=ScrollView,style=empty,fallback=empty") << "ScrollView.qml" << "" << "" << "Default";
    QTest::newRow("control=ScrollView,style=fs,fallback=empty") << "ScrollView.qml" << "FileSystemStyle" << "" << "Default";
    QTest::newRow("control=ScrollView,style=qrc,fallback=empty") << "ScrollView.qml" << "ResourceStyle" << "" << "Default";
    QTest::newRow("control=ScrollView,style=nosuch,fallback=empty") << "ScrollView.qml" << "NoSuchStyle" << "" << "Default";
    QTest::newRow("control=ScrollView,style=data,fallback=empty") << "ScrollView.qml" << "data" << "" << "Default";

    QTest::newRow("control=ScrollView,style=empty,fallback=mat") << "ScrollView.qml" << "" << "Material" << "Default";
    QTest::newRow("control=ScrollView,style=fs,fallback=mat") << "ScrollView.qml" << "FileSystemStyle" << "Material" << "Default";
    QTest::newRow("control=ScrollView,style=qrc,fallback=mat") << "ScrollView.qml" << "ResourceStyle" << "Material" << "Default";
    QTest::newRow("control=ScrollView,style=nosuch,fallback=mat") << "ScrollView.qml" << "NoSuchStyle" << "Material" << "Default";
    QTest::newRow("control=ScrollView,style=data,fallback=mat") << "ScrollView.qml" << "data" << "Material" << "Default";

    // Label.qml exists in the "data", Default and Material styles.
    QTest::newRow("control=Label,style=none,fallback=none") << "Label.qml" << "" << "" << "Default";
    QTest::newRow("control=Label,style=fs,fallback=none") << "Label.qml" << "FileSystemStyle" << "" << "Default";
    QTest::newRow("control=Label,style=qrc,fallback=none") << "Label.qml" << "ResourceStyle" << "" << "Default";
    QTest::newRow("control=Label,style=nosuch,fallback=none") << "Label.qml" << "NoSuchStyle" << "" << "Default";
    QTest::newRow("control=Label,style=data,fallback=none") << "Label.qml" << "data" << "" << "data";

    QTest::newRow("control=Label,style=none,fallback=mat") << "Label.qml" << "" << "Material" << "Default";
    QTest::newRow("control=Label,style=fs,fallback=mat") << "Label.qml" << "FileSystemStyle" << "Material" << "Default";
    QTest::newRow("control=Label,style=qrc,fallback=mat") << "Label.qml" << "ResourceStyle" << "Material" << "Default";
    QTest::newRow("control=Label,style=nosuch,fallback=mat") << "Label.qml" << "NoSuchStyle" << "Material" << "Default";
    QTest::newRow("control=Label,style=data,fallback=mat") << "Label.qml" << "data" << "Material" << "data";

    // Button.qml exists in all styles including the fs and qrc styles
    QTest::newRow("control=Button,style=none,fallback=none") << "Button.qml" << "" << "" << "Default";
    QTest::newRow("control=Button,style=fs,fallback=none") << "Button.qml" << "FileSystemStyle" << "" << "FileSystemStyle";
    QTest::newRow("control=Button,style=qrc,fallback=none") << "Button.qml" << "ResourceStyle" << "" << "ResourceStyle";
    QTest::newRow("control=Button,style=nosuch,fallback=none") << "Button.qml" << "NoSuchStyle" << "" << "Default";
    QTest::newRow("control=Button,style=data,fallback=none") << "Button.qml" << "data" << "" << "data";

    QTest::newRow("control=Button,style=none,fallback=mat") << "Button.qml" << "" << "Material" << "Default";
    QTest::newRow("control=Button,style=fs,fallback=mat") << "Button.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Button,style=qrc,fallback=mat") << "Button.qml" << "ResourceStyle" << "Material" << "ResourceStyle";
    QTest::newRow("control=Button,style=nosuch,fallback=mat") << "Button.qml" << "NoSuchStyle" << "Material" << "Default";
    QTest::newRow("control=Button,style=data,fallback=mat") << "Button.qml" << "data" << "Material" << "data";
}

void tst_StyleImports::select()
{
    QFETCH(QString, file);
    QFETCH(QString, style);
    QFETCH(QString, fallback);
    QFETCH(QString, expected);

    // In Qt 5, there were several accepted forms for style names.
    // In Qt 6, the only accepted form is the base name of the style directory.
    const bool invalidStyleName = style.contains(QLatin1Char('/'));
    if (invalidStyleName)
        QTest::ignoreMessage(QtWarningMsg,
            "Style names must not contain paths; see the \"Definition of a Style\" documentation for more information");
    QQuickStyle::setStyle(style);
    QQuickStyle::setFallbackStyle(fallback);

    QQmlEngine engine;
    engine.addImportPath(directory());
    engine.addImportPath(dataDirectory());
    QQmlComponent component(&engine);
    const QString controlName = file.mid(0, file.indexOf(QLatin1Char('.')));
    component.setData(QString::fromLatin1("import QtQuick; import QtQuick.Controls; %1 { }").arg(controlName).toUtf8(), QUrl());

    const bool nonExistentStyle = style == QLatin1String("NoSuchStyle");
    if (nonExistentStyle)
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    QScopedPointer<QObject> object(component.create());
    if (nonExistentStyle) {
        QVERIFY(object.isNull());
        return;
    }

    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    // TODO: test built-in styles below too
    // We can't check for the attached style object since that API is in a plugin,
    // and it's not possible to use e.g. the baseUrl of the QQmlContext
    // nor the metaObject to test it either.

    if (!QQuickStylePrivate::builtInStyles().contains(expected)) {
        // We're expecting a custom style.
        QCOMPARE(object->objectName(), expected);
    }
}

void tst_StyleImports::platformSelectors()
{
    QQuickStyle::setStyle(QLatin1String("PlatformStyle"));

    QQmlApplicationEngine engine;
    engine.addImportPath(dataDirectory());
    engine.load(testFileUrl("platformSelectors.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window);

    QObject *button = window->property("button").value<QObject*>();
    QVERIFY(button);

#if defined(Q_OS_LINUX)
    QCOMPARE(button->objectName(), "PlatformStyle/+linux");
#elif defined(Q_OS_MACOS)
    QCOMPARE(button->objectName(), "PlatformStyle/+macos");
#elif defined(Q_OS_WIN)
    QCOMPARE(button->objectName(), "PlatformStyle/+windows");
#else
    QCOMPARE(button->objectName(), "PlatformStyle/Button.qml");
#endif
}

void tst_StyleImports::importStyleWithoutControls_data()
{
    QTest::addColumn<QString>("style");

    const auto builtInStyles = QQuickStylePrivate::builtInStyles();
    for (const auto &styleName : builtInStyles)
        QTest::addRow(qPrintable(styleName)) << styleName;
}

// Tests that warnings are printed when trying to import a specific style without first importing QtQuick.Controls.
void tst_StyleImports::importStyleWithoutControls()
{
    QFETCH(QString, style);

    QQmlApplicationEngine engine;
    const QUrl url(testFileUrl(QString::fromLatin1("import%1StyleWithoutControls.qml").arg(style)));
    bool success = false;

    // Two warnings, because Default is used as the fallback.
    if (style != QLatin1String("Default"))
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("QtQuick.Controls must be imported before importing.*" + style));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("QtQuick.Controls must be imported before importing.*Default"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     this, [url, &success](QObject *obj, const QUrl &objUrl) {
        if (url == objUrl)
            success = obj;
    }, Qt::QueuedConnection);

    engine.load(url);
    // It should load, but with warnings.
    QTRY_VERIFY(success);
}

QTEST_MAIN(tst_StyleImports)

#include "tst_styleimports.moc"
