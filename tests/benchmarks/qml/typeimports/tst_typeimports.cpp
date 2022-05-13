// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>

class tst_typeimports : public QObject
{
    Q_OBJECT
public:
    tst_typeimports();

private slots:
    void cpp();
    void qml();

private:
    QQmlEngine engine;
};

class TestType1 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> resources READ resources);
    Q_CLASSINFO("DefaultProperty", "resources");
public:
    TestType1(QObject *parent = nullptr) : QObject(parent) {}

    QQmlListProperty<QObject> resources() {
        return QQmlListProperty<QObject>(this, 0, resources_append, 0, 0, 0);
    }

    static void resources_append(QQmlListProperty<QObject> *p, QObject *o) {
        o->setParent(p->object);
    }
};

class TestType2 : public TestType1
{
    Q_OBJECT
public:
    TestType2(QObject *parent = nullptr) : TestType1(parent) {}
};


class TestType3 : public TestType1
{
    Q_OBJECT
public:
    TestType3(QObject *parent = nullptr) : TestType1(parent) {}
};

class TestType4 : public TestType1
{
    Q_OBJECT
public:
    TestType4(QObject *parent = nullptr) : TestType1(parent) {}
};


tst_typeimports::tst_typeimports()
{
    qmlRegisterType<TestType1>("Qt.test", 1, 0, "TestType1");
    qmlRegisterType<TestType2>("Qt.test", 1, 0, "TestType2");
    qmlRegisterType<TestType3>("Qt.test", 2, 0, "TestType3");
    qmlRegisterType<TestType4>("Qt.test", 2, 0, "TestType4");
}

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

void tst_typeimports::cpp()
{
    QBENCHMARK {
        QQmlComponent component(&engine, TEST_FILE("cpp.qml"));
        QVERIFY(component.isReady());
    }
}

void tst_typeimports::qml()
{
    //get rid of initialization effects
    { QQmlComponent component(&engine, TEST_FILE("qml.qml")); }

    QBENCHMARK {
        QQmlComponent component(&engine, TEST_FILE("qml.qml"));
        QVERIFY(component.isReady());
    }
}

QTEST_MAIN(tst_typeimports)

#include "tst_typeimports.moc"
