/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qtest.h>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QtQml/qqml.h>
#include <QtQml/qqmlprivate.h>
#include <QtQml/qqmlproperty.h>
#include <QDebug>
#include <private/qquickstate_p.h>
#include "../../shared/util.h"

class tst_qqmllistreference : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmllistreference() {}

private:
    void modeData();

private slots:
    void initTestCase();
    void qmllistreference();
    void qmllistreference_invalid();
    void isValid();
    void object();
    void listElementType();
    void canAppend();
    void canAt();
    void canClear();
    void canCount();
    void canReplace();
    void canRemoveLast();
    void isReadable();
    void isManipulable();
    void append();
    void at();
    void clear_data() { modeData(); }
    void clear();
    void count();
    void replace_data() { modeData(); }
    void replace();
    void removeLast_data() { modeData(); }
    void removeLast();
    void copy();
    void qmlmetaproperty();
    void engineTypes();
    void variantToList();
    void listProperty();
};

class TestType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<TestType> data READ dataProperty)
    Q_PROPERTY(int intProperty READ intProperty)

public:
    enum Mode {
        SyntheticClear,
        SyntheticReplace,
        SyntheticClearAndReplace,
        SyntheticRemoveLast,
        SyntheticRemoveLastAndReplace,
        AutomaticReference,
        AutomaticPointer
    };

    static void append(QQmlListProperty<TestType> *p, TestType *v) {
        reinterpret_cast<QList<TestType *> *>(p->data)->append(v);
    }
    static int count(QQmlListProperty<TestType> *p) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->count();
    }
    static TestType *at(QQmlListProperty<TestType> *p, int idx) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->at(idx);
    }
    static void clear(QQmlListProperty<TestType> *p) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->clear();
    }
    static void replace(QQmlListProperty<TestType> *p, int idx, TestType *v) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->replace(idx, v);
    }
    static void removeLast(QQmlListProperty<TestType> *p) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->removeLast();
    }

    TestType(Mode mode = AutomaticReference)
    {
        switch (mode) {
        case SyntheticClear:
            property = QQmlListProperty<TestType>(this, &data, append, count, at, nullptr,
                                                  replace, removeLast);
            break;
        case SyntheticReplace:
            property = QQmlListProperty<TestType>(this, &data, append, count, at, clear,
                                                  nullptr, removeLast);
            break;
        case SyntheticClearAndReplace:
            property = QQmlListProperty<TestType>(this, &data, append, count, at, nullptr,
                                                  nullptr, removeLast);
            break;
        case SyntheticRemoveLast:
            property = QQmlListProperty<TestType>(this, &data, append, count, at, clear,
                                                  replace, nullptr);
            break;
        case SyntheticRemoveLastAndReplace:
            property = QQmlListProperty<TestType>(this, &data, append, count, at, clear,
                                                  nullptr, nullptr);
            break;
        case AutomaticReference:
            property = QQmlListProperty<TestType>(this, data);
            break;
        case AutomaticPointer:
            property = QQmlListProperty<TestType>(this, &data);
            break;
        }
    }

    QQmlListProperty<TestType> dataProperty() { return property; }
    int intProperty() const { return 10; }

    QList<TestType *> data;
    QQmlListProperty<TestType> property;
};

Q_DECLARE_METATYPE(TestType::Mode)

void tst_qqmllistreference::modeData()
{
    QTest::addColumn<TestType::Mode>("mode");
    QTest::addRow("AutomaticReference") << TestType::AutomaticReference;
    QTest::addRow("AutomaticPointer") << TestType::AutomaticPointer;
    QTest::addRow("SyntheticClear") << TestType::SyntheticClear;
    QTest::addRow("SyntheticReplace") << TestType::SyntheticReplace;
    QTest::addRow("SyntheticClearAndReplace") << TestType::SyntheticClearAndReplace;
    QTest::addRow("SyntheticRemoveLast") << TestType::SyntheticRemoveLast;
    QTest::addRow("SyntheticRemoveLastAndReplace") << TestType::SyntheticRemoveLastAndReplace;
}

void tst_qqmllistreference::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterAnonymousType<TestType>("Test", 1);
}

void tst_qqmllistreference::qmllistreference()
{
    TestType tt;

    QQmlListReference r(&tt, "data");
    QVERIFY(r.isValid());
    QCOMPARE(r.count(), 0);

    tt.data.append(&tt);
    QCOMPARE(r.count(), 1);
}

void tst_qqmllistreference::qmllistreference_invalid()
{
    TestType tt;

    // Invalid
    {
    QQmlListReference r;
    QVERIFY(!r.isValid());
    QVERIFY(!r.object());
    QVERIFY(!r.listElementType());
    QVERIFY(!r.canAt());
    QVERIFY(!r.canClear());
    QVERIFY(!r.canCount());
    QVERIFY(!r.append(nullptr));
    QVERIFY(!r.at(10));
    QVERIFY(!r.clear());
    QCOMPARE(r.count(), 0);
    QVERIFY(!r.isReadable());
    QVERIFY(!r.isManipulable());
    }

    // Non-property
    {
    QQmlListReference r(&tt, "blah");
    QVERIFY(!r.isValid());
    QVERIFY(!r.object());
    QVERIFY(!r.listElementType());
    QVERIFY(!r.canAt());
    QVERIFY(!r.canClear());
    QVERIFY(!r.canCount());
    QVERIFY(!r.append(nullptr));
    QVERIFY(!r.at(10));
    QVERIFY(!r.clear());
    QCOMPARE(r.count(), 0);
    QVERIFY(!r.isReadable());
    QVERIFY(!r.isManipulable());
    }

    // Non-list property
    {
    QQmlListReference r(&tt, "intProperty");
    QVERIFY(!r.isValid());
    QVERIFY(!r.object());
    QVERIFY(!r.listElementType());
    QVERIFY(!r.canAt());
    QVERIFY(!r.canClear());
    QVERIFY(!r.canCount());
    QVERIFY(!r.append(nullptr));
    QVERIFY(!r.at(10));
    QVERIFY(!r.clear());
    QCOMPARE(r.count(), 0);
    QVERIFY(!r.isReadable());
    QVERIFY(!r.isManipulable());
    }
}

void tst_qqmllistreference::isValid()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.isValid());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.isValid());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.isValid());
    delete tt;
    QVERIFY(!ref.isValid());
    }
}

void tst_qqmllistreference::object()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.object());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.object());
    }

    {
    QQmlListReference ref(tt, "data");
    QCOMPARE(ref.object(), tt);
    delete tt;
    QVERIFY(!ref.object());
    }
}

void tst_qqmllistreference::listElementType()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.listElementType());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.listElementType());
    }

    {
    QQmlListReference ref(tt, "data");
    QCOMPARE(ref.listElementType(), &TestType::staticMetaObject);
    delete tt;
    QVERIFY(!ref.listElementType());
    }
}

void tst_qqmllistreference::canAppend()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.canAppend());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.canAppend());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.canAppend());
    delete tt;
    QVERIFY(!ref.canAppend());
    }

    {
    TestType tt;
    tt.property.append = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.canAppend());
    }
}

void tst_qqmllistreference::canAt()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.canAt());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.canAt());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.canAt());
    delete tt;
    QVERIFY(!ref.canAt());
    }

    {
    TestType tt;
    tt.property.at = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.canAt());
    }
}

void tst_qqmllistreference::canClear()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.canClear());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.canClear());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.canClear());
    delete tt;
    QVERIFY(!ref.canClear());
    }

    {
    TestType tt;
    tt.property.clear = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.canClear());
    }
}

void tst_qqmllistreference::canCount()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.canCount());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.canCount());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.canCount());
    delete tt;
    QVERIFY(!ref.canCount());
    }

    {
    TestType tt;
    tt.property.count = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.canCount());
    }
}

void tst_qqmllistreference::canReplace()
{
    QScopedPointer<TestType> tt(new TestType);

    {
        QQmlListReference ref;
        QVERIFY(!ref.canReplace());
    }

    {
        QQmlListReference ref(tt.data(), "blah");
        QVERIFY(!ref.canReplace());
    }

    {
        QQmlListReference ref(tt.data(), "data");
        QVERIFY(ref.canReplace());
        tt.reset();
        QVERIFY(!ref.canReplace());
    }

    {
        TestType tt;
        tt.property.replace = nullptr;
        QQmlListReference ref(&tt, "data");
        QVERIFY(!ref.canReplace());
    }
}

void tst_qqmllistreference::canRemoveLast()
{
    QScopedPointer<TestType> tt(new TestType);

    {
        QQmlListReference ref;
        QVERIFY(!ref.canRemoveLast());
    }

    {
        QQmlListReference ref(tt.data(), "blah");
        QVERIFY(!ref.canRemoveLast());
    }

    {
        QQmlListReference ref(tt.data(), "data");
        QVERIFY(ref.canRemoveLast());
        tt.reset();
        QVERIFY(!ref.canRemoveLast());
    }

    {
        TestType tt;
        tt.property.removeLast = nullptr;
        QQmlListReference ref(&tt, "data");
        QVERIFY(!ref.canRemoveLast());
    }
}

void tst_qqmllistreference::isReadable()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.isReadable());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.isReadable());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.isReadable());
    delete tt;
    QVERIFY(!ref.isReadable());
    }

    {
    TestType tt;
    tt.property.count = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.isReadable());
    }
}

void tst_qqmllistreference::isManipulable()
{
    TestType *tt = new TestType;

    {
    QQmlListReference ref;
    QVERIFY(!ref.isManipulable());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.isManipulable());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.isManipulable());
    delete tt;
    QVERIFY(!ref.isManipulable());
    }

    {
    TestType tt;
    tt.property.count = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.isManipulable());
    }
}

void tst_qqmllistreference::append()
{
    TestType *tt = new TestType;
    QObject object;

    {
    QQmlListReference ref;
    QVERIFY(!ref.append(tt));
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.append(tt));
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.append(tt));
    QCOMPARE(tt->data.count(), 1);
    QCOMPARE(tt->data.at(0), tt);
    QVERIFY(!ref.append(&object));
    QCOMPARE(tt->data.count(), 1);
    QCOMPARE(tt->data.at(0), tt);
    QVERIFY(ref.append(nullptr));
    QCOMPARE(tt->data.count(), 2);
    QCOMPARE(tt->data.at(0), tt);
    QVERIFY(!tt->data.at(1));
    delete tt;
    QVERIFY(!ref.append(nullptr));
    }

    {
    TestType tt;
    tt.property.append = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.append(&tt));
    }
}

void tst_qqmllistreference::at()
{
    TestType *tt = new TestType;
    tt->data.append(tt);
    tt->data.append(0);
    tt->data.append(tt);

    {
    QQmlListReference ref;
    QVERIFY(!ref.at(0));
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.at(0));
    }

    {
    QQmlListReference ref(tt, "data");
    QCOMPARE(ref.at(0), tt);
    QVERIFY(!ref.at(1));
    QCOMPARE(ref.at(2), tt);
    delete tt;
    QVERIFY(!ref.at(0));
    }

    {
    TestType tt;
    tt.data.append(&tt);
    tt.property.at = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.at(0));
    }
}

void tst_qqmllistreference::clear()
{
    QFETCH(TestType::Mode, mode);
    TestType *tt = new TestType(mode);
    tt->data.append(tt);
    tt->data.append(0);
    tt->data.append(tt);

    {
    QQmlListReference ref;
    QVERIFY(!ref.clear());
    }

    {
    QQmlListReference ref(tt, "blah");
    QVERIFY(!ref.clear());
    }

    {
    QQmlListReference ref(tt, "data");
    QVERIFY(ref.clear());
    QCOMPARE(tt->data.count(), 0);
    delete tt;
    QVERIFY(!ref.clear());
    }

    {
    TestType tt;
    tt.property.clear = nullptr;
    QQmlListReference ref(&tt, "data");
    QVERIFY(!ref.clear());
    }
}

void tst_qqmllistreference::count()
{
    TestType *tt = new TestType;
    tt->data.append(tt);
    tt->data.append(0);
    tt->data.append(tt);

    {
    QQmlListReference ref;
    QCOMPARE(ref.count(), 0);
    }

    {
    QQmlListReference ref(tt, "blah");
    QCOMPARE(ref.count(), 0);
    }

    {
    QQmlListReference ref(tt, "data");
    QCOMPARE(ref.count(), 3);
    tt->data.removeAt(1);
    QCOMPARE(ref.count(), 2);
    delete tt;
    QCOMPARE(ref.count(), 0);
    }

    {
    TestType tt;
    tt.data.append(&tt);
    tt.property.count = nullptr;
    QQmlListReference ref(&tt, "data");
    QCOMPARE(ref.count(), 0);
    }
}

void tst_qqmllistreference::replace()
{
    QFETCH(TestType::Mode, mode);
    QScopedPointer<TestType> tt(new TestType(mode));
    tt->data.append(tt.get());
    tt->data.append(nullptr);
    tt->data.append(tt.get());

    {
        QQmlListReference ref(tt.get(), "data");
        QVERIFY(ref.replace(1, tt.get()));
        QCOMPARE(ref.at(1), tt.get());
        QVERIFY(ref.replace(2, nullptr));
        QCOMPARE(ref.at(2), nullptr);
        QCOMPARE(ref.count(), 3);
        tt.reset();
        QVERIFY(!ref.replace(0, tt.get()));
    }

    {
        TestType tt;
        tt.data.append(&tt);
        tt.property.replace = nullptr;
        QQmlListReference ref(&tt, "data");
        QVERIFY(!ref.replace(0, nullptr));
    }
}

void tst_qqmllistreference::removeLast()
{
    QFETCH(TestType::Mode, mode);
    QScopedPointer<TestType> tt(new TestType(mode));
    tt->data.append(tt.get());
    tt->data.append(nullptr);
    tt->data.append(tt.get());

    {
        QQmlListReference ref;
        QVERIFY(!ref.removeLast());
    }

    {
        QQmlListReference ref(tt.get(), "blah");
        QVERIFY(!ref.removeLast());
    }

    {
        QQmlListReference ref(tt.get(), "data");
        QCOMPARE(tt->data.count(), 3);
        QVERIFY(ref.removeLast());
        QCOMPARE(tt->data.count(), 2);
        tt.reset();
        QVERIFY(!ref.removeLast());
    }

    {
        TestType tt;
        tt.property.removeLast = nullptr;
        QQmlListReference ref(&tt, "data");
        ref.append(&tt);
        QVERIFY(!ref.removeLast());
    }
}

void tst_qqmllistreference::copy()
{
    TestType tt;
    tt.data.append(&tt);
    tt.data.append(0);
    tt.data.append(&tt);

    QQmlListReference *r1 = new QQmlListReference(&tt, "data");
    QCOMPARE(r1->count(), 3);

    QQmlListReference r2(*r1);
    QQmlListReference r3;
    r3 = *r1;

    QCOMPARE(r2.count(), 3);
    QCOMPARE(r3.count(), 3);

    delete r1;

    QCOMPARE(r2.count(), 3);
    QCOMPARE(r3.count(), 3);

    tt.data.removeAt(2);

    QCOMPARE(r2.count(), 2);
    QCOMPARE(r3.count(), 2);
}

void tst_qqmllistreference::qmlmetaproperty()
{
    TestType tt;
    tt.data.append(&tt);
    tt.data.append(0);
    tt.data.append(&tt);

    QQmlProperty prop(&tt, QLatin1String("data"));
    QVariant v = prop.read();
    QCOMPARE(v.userType(), qMetaTypeId<QQmlListReference>());
    QQmlListReference ref = qvariant_cast<QQmlListReference>(v);
    QCOMPARE(ref.count(), 3);
    QCOMPARE(ref.listElementType(), &TestType::staticMetaObject);
}

void tst_qqmllistreference::engineTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("engineTypes.qml"));

    QObject *o = component.create();
    QVERIFY(o);

    QQmlProperty p1(o, QLatin1String("myList"));
    QCOMPARE(p1.propertyTypeCategory(), QQmlProperty::List);

    QQmlProperty p2(o, QLatin1String("myList"), engine.rootContext());
    QCOMPARE(p2.propertyTypeCategory(), QQmlProperty::List);
    QVariant v = p2.read();
    QCOMPARE(v.userType(), qMetaTypeId<QQmlListReference>());
    QQmlListReference ref = qvariant_cast<QQmlListReference>(v);
    QCOMPARE(ref.count(), 2);
    QVERIFY(ref.listElementType());
    QVERIFY(ref.listElementType() != &QObject::staticMetaObject);

    delete o;
}

void tst_qqmllistreference::variantToList()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("variantToList.qml"));

    QObject *o = component.create();
    QVERIFY(o);

    QCOMPARE(o->property("value").userType(), qMetaTypeId<QQmlListReference>());
    QCOMPARE(o->property("test").toInt(), 1);

    delete o;
}

void tst_qqmllistreference::listProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("propertyList.qml"));

    QScopedPointer<QObject> object( component.create() );
    QVERIFY(object != nullptr);

    QCOMPARE( object->property("state").toString(), QStringLiteral("MyState2") );
    QQmlListReference list( object.data(), "states");
    QCOMPARE( list.count(), 2 );

    QQuickState* state1 = dynamic_cast<QQuickState*>( list.at( 0 ) );
    QVERIFY(state1 != nullptr);
    QCOMPARE( state1->name(), QStringLiteral("MyState1") );
    QQuickState* state2 = dynamic_cast<QQuickState*>( list.at( 1 ) );
    QVERIFY(state2 != nullptr);

    QCOMPARE( state2->name(), QStringLiteral("MyState2") );
}


QTEST_MAIN(tst_qqmllistreference)

#include "tst_qqmllistreference.moc"
