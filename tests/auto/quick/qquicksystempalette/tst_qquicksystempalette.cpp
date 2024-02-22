// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QDebug>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquicksystempalette_p.h>
#include <qpalette.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquicksystempalette : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicksystempalette();

private slots:
    void activePalette();
    void inactivePalette();
    void disabledPalette();
#ifndef QT_NO_WIDGETS
    void paletteChanged();
#endif

private:
    QQmlEngine engine;
};

tst_qquicksystempalette::tst_qquicksystempalette() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_qquicksystempalette::activePalette()
{
    QQmlComponent component(&engine, testFileUrl("systemPalette.qml"));
    QQuickSystemPalette *object = qobject_cast<QQuickSystemPalette*>(component.create());

    QVERIFY(object != nullptr);

    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Active);
    QCOMPARE(palette.window().color(), object->window());
    QCOMPARE(palette.windowText().color(), object->windowText());
    QCOMPARE(palette.base().color(), object->base());
    QCOMPARE(palette.text().color(), object->text());
    QCOMPARE(palette.alternateBase().color(), object->alternateBase());
    QCOMPARE(palette.button().color(), object->button());
    QCOMPARE(palette.buttonText().color(), object->buttonText());
    QCOMPARE(palette.light().color(), object->light());
    QCOMPARE(palette.midlight().color(), object->midlight());
    QCOMPARE(palette.dark().color(), object->dark());
    QCOMPARE(palette.mid().color(), object->mid());
    QCOMPARE(palette.shadow().color(), object->shadow());
    QCOMPARE(palette.highlight().color(), object->highlight());
    QCOMPARE(palette.highlightedText().color(), object->highlightedText());
    QCOMPARE(palette.placeholderText().color(), object->placeholderText());
    QCOMPARE(palette.accent().color(), object->accent());

    delete object;
}

void tst_qquicksystempalette::inactivePalette()
{
    QQmlComponent component(&engine, testFileUrl("systemPaletteInactive.qml"));
    QQuickSystemPalette *object = qobject_cast<QQuickSystemPalette*>(component.create());

    QVERIFY(object != nullptr);
    QCOMPARE(object->colorGroup(), QQuickSystemPalette::Inactive);

    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Inactive);
    QCOMPARE(palette.window().color(), object->window());
    QCOMPARE(palette.windowText().color(), object->windowText());
    QCOMPARE(palette.base().color(), object->base());
    QCOMPARE(palette.text().color(), object->text());
    QCOMPARE(palette.alternateBase().color(), object->alternateBase());
    QCOMPARE(palette.button().color(), object->button());
    QCOMPARE(palette.buttonText().color(), object->buttonText());
    QCOMPARE(palette.light().color(), object->light());
    QCOMPARE(palette.midlight().color(), object->midlight());
    QCOMPARE(palette.dark().color(), object->dark());
    QCOMPARE(palette.mid().color(), object->mid());
    QCOMPARE(palette.shadow().color(), object->shadow());
    QCOMPARE(palette.highlight().color(), object->highlight());
    QCOMPARE(palette.highlightedText().color(), object->highlightedText());
    QCOMPARE(palette.placeholderText().color(), object->placeholderText());
    QCOMPARE(palette.accent().color(), object->accent());

    delete object;
}

void tst_qquicksystempalette::disabledPalette()
{
    QQmlComponent component(&engine, testFileUrl("systemPaletteDisabled.qml"));
    QQuickSystemPalette *object = qobject_cast<QQuickSystemPalette*>(component.create());

    QVERIFY(object != nullptr);
    QCOMPARE(object->colorGroup(), QQuickSystemPalette::Disabled);

    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Disabled);
    QCOMPARE(palette.window().color(), object->window());
    QCOMPARE(palette.windowText().color(), object->windowText());
    QCOMPARE(palette.base().color(), object->base());
    QCOMPARE(palette.text().color(), object->text());
    QCOMPARE(palette.alternateBase().color(), object->alternateBase());
    QCOMPARE(palette.button().color(), object->button());
    QCOMPARE(palette.buttonText().color(), object->buttonText());
    QCOMPARE(palette.light().color(), object->light());
    QCOMPARE(palette.midlight().color(), object->midlight());
    QCOMPARE(palette.dark().color(), object->dark());
    QCOMPARE(palette.mid().color(), object->mid());
    QCOMPARE(palette.shadow().color(), object->shadow());
    QCOMPARE(palette.highlight().color(), object->highlight());
    QCOMPARE(palette.highlightedText().color(), object->highlightedText());
    QCOMPARE(palette.placeholderText().color(), object->placeholderText());
    QCOMPARE(palette.accent().color(), object->accent());

    delete object;
}

#ifndef QT_NO_WIDGETS
void tst_qquicksystempalette::paletteChanged()
{
    QQmlComponent component(&engine, testFileUrl("systemPalette.qml"));
    QQuickSystemPalette *object = qobject_cast<QQuickSystemPalette*>(component.create());

    QVERIFY(object != nullptr);

    QPalette p;
    p.setCurrentColorGroup(QPalette::Active);
    p.setColor(QPalette::Active, QPalette::Text, QColor("red"));
    p.setColor(QPalette::Active, QPalette::ButtonText, QColor("green"));
    p.setColor(QPalette::Active, QPalette::WindowText, QColor("blue"));

    qApp->setPalette(p);

    object->setColorGroup(QQuickSystemPalette::Active);
    QTRY_COMPARE(QColor("red"), object->text());
    QTRY_COMPARE(QColor("green"), object->buttonText());
    QTRY_COMPARE(QColor("blue"), object->windowText());

    delete object;
}
#endif

QTEST_MAIN(tst_qquicksystempalette)

#include "tst_qquicksystempalette.moc"
