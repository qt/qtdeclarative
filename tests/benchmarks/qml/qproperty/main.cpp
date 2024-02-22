// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QScopedPointer>
#include <qqml.h>
#include <QProperty>

#include <qtest.h>

#include "propertytester.h"

class PropertyBenchmark : public QObject
{
    Q_OBJECT
public:
    PropertyBenchmark();

private slots:
    void oldStyleProperty();
    void oldStylePropertyReadOnce();
    void oldStylePropertyDirect();
    void oldStylePropertyDirectReadOnce();

    void newStyleProperty();
    void newStylePropertyReadOnce();
    void newStylePropertyDirect();
    void newStylePropertyDirectReadOnce();

    void notifyingProperty();
    void notifyingPropertyReadOnce();
    void notifyingPropertyDirect();
    void notifyingPropertyDirectReadOnce();
private:
    QScopedPointer<QQmlEngine> m_engine;
};

PropertyBenchmark::PropertyBenchmark()
    : m_engine(new QQmlEngine)
{
    qmlRegisterType<PropertyTester>("Test", 1, 0, "Test");
}

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

void PropertyBenchmark::oldStyleProperty()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("old.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->property("yOld").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        item->setProperty("xOld", ++i);
        if (item->property("yOld").toInt() != i)
            QFAIL("boo");
    }
}

void PropertyBenchmark::oldStylePropertyDirect()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("old.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->yOld(), 0);
    int i = 0;
    QBENCHMARK {
        item->setXOld(++i);
        if (item->yOld() != i)
            QFAIL("boo");
    }
}

void PropertyBenchmark::oldStylePropertyReadOnce()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("old.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->property("yOld").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        item->setProperty("xOld", ++i);
    }
    QCOMPARE(item->property("yOld").toInt(), i);
}

void PropertyBenchmark::oldStylePropertyDirectReadOnce()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("old.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->yOld(), 0);
    int i = 0;
    QBENCHMARK {
        item->setXOld(++i);
    }

    QCOMPARE(item->yOld(), i);
}

void PropertyBenchmark::newStyleProperty()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("new.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->property("y").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        item->setProperty("x", ++i);
        if (item->property("y").toInt() != i)
            QFAIL("boo");
    }
}

void PropertyBenchmark::newStylePropertyDirect()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("new.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->y.value(), 0);
    int i = 0;
    QBENCHMARK {
        item->x = ++i;
        if (item->y.value() != i)
            QFAIL("boo");
    }
}

void PropertyBenchmark::newStylePropertyReadOnce()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("new.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->property("y").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        item->setProperty("x", ++i);
    }
    QCOMPARE(item->property("y").toInt(), i);
}

void PropertyBenchmark::newStylePropertyDirectReadOnce()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("new.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->y.value(), 0);
    int i = 0;
    QBENCHMARK {
        item->x = ++i;
    }

    QCOMPARE(item->y.value(), i);
}

void PropertyBenchmark::notifyingProperty()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("notified.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->property("yNotified").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        item->setProperty("xNotified", ++i);
        if (item->property("yNotified").toInt() != i)
            QFAIL("boo");
    }
}

void PropertyBenchmark::notifyingPropertyDirect()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("notified.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->yNotified.value(), 0);
    int i = 0;
    QBENCHMARK {
        item->xNotified.setValue(++i);
        if (item->yNotified.value() != i)
            QFAIL("boo");
    }
}

void PropertyBenchmark::notifyingPropertyReadOnce()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("notified.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->property("yNotified").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        item->setProperty("xNotified", ++i);
    }
    QCOMPARE(item->property("yNotified").toInt(), i);
}

void PropertyBenchmark::notifyingPropertyDirectReadOnce()
{
    QQmlComponent component (m_engine.data(), QUrl(TEST_FILE("notified.qml")));
    QScopedPointer item  {(PropertyTester *) component.create()};
    QVERIFY(item);
    QCOMPARE(item->yNotified.value(), 0);
    int i = 0;
    QBENCHMARK {
        item->xNotified.setValue(++i);
    }

    QCOMPARE(item->yNotified.value(), i);
}

QTEST_MAIN(PropertyBenchmark)
#include "main.moc"
