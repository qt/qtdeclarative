// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include <QtCore/qrandom.h>
#include <private/qquickstate_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmllistreference : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmllistreference() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private:
    void modeData();

private slots:
    void initTestCase() override;
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
    void compositeListProperty();
    void nullItems();
    void jsArrayMethods();
    void jsArrayMethodsWithParams_data();
    void jsArrayMethodsWithParams();
    void listIgnoresNull_data() { modeData(); }
    void listIgnoresNull();
    void consoleLogSyntheticList();
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
        AutomaticPointer,
        IgnoreNullValues,
    };

    static void append(QQmlListProperty<TestType> *p, TestType *v) {
        reinterpret_cast<QList<TestType *> *>(p->data)->append(v);
    }
    static void appendNoNullValues(QQmlListProperty<TestType> *p, TestType *v) {
        if (!v)
            return;
        reinterpret_cast<QList<TestType *> *>(p->data)->append(v);
    }
    static qsizetype count(QQmlListProperty<TestType> *p) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->size();
    }
    static TestType *at(QQmlListProperty<TestType> *p, qsizetype idx) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->at(idx);
    }
    static void clear(QQmlListProperty<TestType> *p) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->clear();
    }
    static void replace(QQmlListProperty<TestType> *p, qsizetype idx, TestType *v) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->replace(idx, v);
    }
    static void removeLast(QQmlListProperty<TestType> *p) {
        return reinterpret_cast<QList<TestType *> *>(p->data)->removeLast();
    }

    TestType(Mode mode = AutomaticPointer)
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
        case AutomaticPointer:
            property = QQmlListProperty<TestType>(this, &data);
            break;
        case IgnoreNullValues:
            property = QQmlListProperty<TestType>(this, &data, appendNoNullValues, count, at, clear,
                                                  replace, removeLast);
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
    QTest::addRow("AutomaticPointer") << TestType::AutomaticPointer;
    QTest::addRow("SyntheticClear") << TestType::SyntheticClear;
    QTest::addRow("SyntheticReplace") << TestType::SyntheticReplace;
    QTest::addRow("SyntheticClearAndReplace") << TestType::SyntheticClearAndReplace;
    QTest::addRow("SyntheticRemoveLast") << TestType::SyntheticRemoveLast;
    QTest::addRow("SyntheticRemoveLastAndReplace") << TestType::SyntheticRemoveLastAndReplace;
    QTest::addRow("IgnoreNullValues") << TestType::IgnoreNullValues;
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

    const QMetaObject *m = tt.metaObject();
    const int index = m->indexOfProperty("data");
    const QMetaProperty prop = m->property(index);
    const QVariant var = prop.read(&tt);

    QQmlListReference fromVar(var);
    QVERIFY(fromVar.isValid());
    QCOMPARE(fromVar.count(), 1);
    fromVar.append(&tt);
    QCOMPARE(tt.data.size(), 2);
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
    QCOMPARE(tt->data.size(), 1);
    QCOMPARE(tt->data.at(0), tt);
    QVERIFY(!ref.append(&object));
    QCOMPARE(tt->data.size(), 1);
    QCOMPARE(tt->data.at(0), tt);
    QVERIFY(ref.append(nullptr));
    QCOMPARE(tt->data.size(), 2);
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
    QCOMPARE(tt->data.size(), 0);
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
        QCOMPARE(tt->data.size(), 3);
        QVERIFY(ref.removeLast());
        QCOMPARE(tt->data.size(), 2);
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
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

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


    const QMetaObject *m = o->metaObject();
    const int index = m->indexOfProperty("myList");
    const QMetaProperty prop = m->property(index);
    const QVariant var = prop.read(o);

    QQmlListReference fromVar(var);
    QVERIFY(fromVar.isValid());
    QCOMPARE(fromVar.count(), 2);
    QCOMPARE(fromVar.listElementType(), ref.listElementType());

    delete o;
}

void tst_qqmllistreference::variantToList()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("variantToList.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o);

    QCOMPARE(o->property("value").userType(), qMetaTypeId<QQmlListReference>());
    QCOMPARE(o->property("test").toInt(), 1);
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

void tst_qqmllistreference::compositeListProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("compositeListProp.qml"));

    QTest::ignoreMessage(
            QtWarningMsg, QRegularExpression("Cannot append QObject_QML_[0-9]+\\(0x[0-9a-f]+\\) "
                                             "to a QML list of AListItem_QMLTYPE_[0-9]+\\*"));
    QTest::ignoreMessage(
            QtWarningMsg, QRegularExpression("Cannot append QObject_QML_[0-9]+\\(0x[0-9a-f]+\\) "
                                             "to a QML list of AListItem_QMLTYPE_[0-9]+\\*"));
    QTest::ignoreMessage(
            QtWarningMsg, QRegularExpression("Cannot insert QObject_QML_[0-9]+\\(0x[0-9a-f]+\\) "
                                             "into a QML list of AListItem_QMLTYPE_[0-9]+\\*"));
    QTest::ignoreMessage(
            QtWarningMsg, QRegularExpression("Cannot splice QObject_QML_[0-9]+\\(0x[0-9a-f]+\\) "
                                             "into a QML list of AListItem_QMLTYPE_[0-9]+\\*"));
    QTest::ignoreMessage(
            QtWarningMsg, QRegularExpression("Cannot unshift QObject_QML_[0-9]+\\(0x[0-9a-f]+\\) "
                                             "into a QML list of AListItem_QMLTYPE_[0-9]+\\*"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QQmlListReference list1(object.data(), "items");
    QCOMPARE(list1.size(), 5);
    for (qsizetype i = 0; i < 5; ++i)
        QCOMPARE(list1.at(i), nullptr);

    QQmlComponent item(&engine, testFileUrl("AListItem.qml"));
    QScopedPointer<QObject> i1(item.create());
    QScopedPointer<QObject> i2(item.create());
    QVERIFY(!i1.isNull());
    QVERIFY(!i2.isNull());

    // We know the element type now.
    QVERIFY(list1.listElementType() != nullptr);
    QVERIFY(list1.append(i1.data()));
    QVERIFY(list1.replace(0, i2.data()));
}

void tst_qqmllistreference::nullItems()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("nullItems.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QQmlListReference list(object.data(), "items");
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.at(0), nullptr);
    QCOMPARE(list.at(1), nullptr);
    QVERIFY(list.at(2) != nullptr);
}

static void listsEqual(QObject *object, const char *method)
{
    const QByteArray listPropertyPropertyName = QByteArray("listProperty") + method;
    const QByteArray jsArrayPropertyName = QByteArray("jsArray") + method;

    const QQmlListReference listPropertyProperty(object, listPropertyPropertyName.constData());
    const QQmlListReference jsArrayProperty(object, jsArrayPropertyName.constData());

    const qsizetype listPropertyCount = listPropertyProperty.count();
    QCOMPARE(listPropertyCount, jsArrayProperty.count());

    for (qsizetype i = 0; i < listPropertyCount; ++i)
        QCOMPARE(listPropertyProperty.at(i), jsArrayProperty.at(i));
}

void tst_qqmllistreference::jsArrayMethods()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("jsArrayMethods.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("listPropertyToString"), object->property("jsArrayToString"));
    QCOMPARE(object->property("listPropertyToLocaleString"), object->property("jsArrayToLocaleString"));

    QVERIFY(object->property("entriesMatch").toBool());
    QVERIFY(object->property("keysMatch").toBool());
    QVERIFY(object->property("valuesMatch").toBool());

    listsEqual(object.data(), "Concat");
    listsEqual(object.data(), "Pop");
    listsEqual(object.data(), "Push");
    listsEqual(object.data(), "Reverse");
    listsEqual(object.data(), "Shift");
    listsEqual(object.data(), "Unshift");
    listsEqual(object.data(), "Filter");
    listsEqual(object.data(), "Sort1");
    listsEqual(object.data(), "Sort2");

    QCOMPARE(object->property("listPropertyFind"), object->property("jsArrayFind"));
    QCOMPARE(object->property("listPropertyFind").value<QObject *>()->objectName(), QStringLiteral("klaus"));

    QCOMPARE(object->property("listPropertyFindIndex"), object->property("jsArrayFindIndex"));
    QCOMPARE(object->property("listPropertyFindIndex").toInt(), 1);

    QCOMPARE(object->property("listPropertyIncludes"), object->property("jsArrayIncludes"));
    QVERIFY(object->property("listPropertyIncludes").toBool());

    QCOMPARE(object->property("listPropertyJoin"), object->property("jsArrayJoin"));
    QVERIFY(object->property("listPropertyJoin").toString().contains(QStringLiteral("klaus")));

    QCOMPARE(object->property("listPropertyPopped"), object->property("jsArrayPopped"));
    QVERIFY(object->property("listPropertyPopped").value<QObject *>()->objectName().isEmpty());

    QCOMPARE(object->property("listPropertyPushed"), object->property("jsArrayPushed"));
    QCOMPARE(object->property("listPropertyPushed").toInt(), 4);

    QCOMPARE(object->property("listPropertyShifted"), object->property("jsArrayShifted"));
    QCOMPARE(object->property("listPropertyShifted").value<QObject *>()->objectName(), QStringLiteral("klaus"));

    QCOMPARE(object->property("listPropertyUnshifted"), object->property("jsArrayUnshifted"));
    QCOMPARE(object->property("listPropertyUnshifted").toInt(), 4);

    QCOMPARE(object->property("listPropertyIndexOf"), object->property("jsArrayIndexOf"));
    QCOMPARE(object->property("listPropertyIndexOf").toInt(), 1);

    QCOMPARE(object->property("listPropertyLastIndexOf"), object->property("jsArrayLastIndexOf"));
    QCOMPARE(object->property("listPropertyLastIndexOf").toInt(), 2);

    QCOMPARE(object->property("listPropertyEvery"), object->property("jsArrayEvery"));
    QVERIFY(object->property("listPropertyEvery").toBool());

    QCOMPARE(object->property("listPropertySome"), object->property("jsArrayEvery"));
    QVERIFY(object->property("listPropertySome").toBool());

    QCOMPARE(object->property("listPropertyForEach"), object->property("jsArrayForEach"));
    QCOMPARE(object->property("listPropertyForEach").toString(), QStringLiteral("-klaus-----"));

    QCOMPARE(object->property("listPropertyMap").toStringList(), object->property("jsArrayMap").toStringList());
    QCOMPARE(object->property("listPropertyReduce").toString(), object->property("jsArrayReduce").toString());

    QCOMPARE(object->property("listPropertyOwnPropertyNames").toStringList(),
             object->property("jsArrayOwnPropertyNames").toStringList());
}

void tst_qqmllistreference::jsArrayMethodsWithParams_data()
{
    QTest::addColumn<double>("i");
    QTest::addColumn<double>("j");
    QTest::addColumn<double>("k");

    const double indices[] = {
        double(std::numeric_limits<qsizetype>::min()),
        double(std::numeric_limits<qsizetype>::min()) + 1,
        double(std::numeric_limits<uint>::min()) - 1,
        double(std::numeric_limits<uint>::min()),
        double(std::numeric_limits<uint>::min()) + 1,
        double(std::numeric_limits<int>::min()),
        -10, -3, -2, -1, 0, 1, 2, 3, 10,
        double(std::numeric_limits<int>::max()),
        double(std::numeric_limits<uint>::max()) - 1,
        double(std::numeric_limits<uint>::max()),
        double(std::numeric_limits<uint>::max()) + 1,
        double(std::numeric_limits<qsizetype>::max() - 1),
        double(std::numeric_limits<qsizetype>::max()),
    };

    // We cannot test the full cross product. So, take a random sample instead.
    const qsizetype numIndices = sizeof(indices) / sizeof(double);
    qsizetype seed = QRandomGenerator::global()->generate();
    const int numSamples = 4;
    for (int i = 0; i < numSamples; ++i) {
        seed = qHash(i, seed);
        const double vi = indices[qAbs(seed) % numIndices];
        for (int j = 0; j < numSamples; ++j) {
            seed = qHash(j, seed);
            const double vj = indices[qAbs(seed) % numIndices];
            for (int k = 0; k < numSamples; ++k) {
                seed = qHash(k, seed);
                const double vk = indices[qAbs(seed) % numIndices];
                const QString tag = QLatin1String("%1/%2/%3")
                        .arg(QString::number(vi), QString::number(vj), QString::number(vk));
                QTest::newRow(qPrintable(tag)) << vi << vj << vk;

                // output all the tags so that we can find out what combination caused a test to hang.
                qDebug().noquote() << "scheduling" << tag;
            }
        }
    }
}

void tst_qqmllistreference::jsArrayMethodsWithParams()
{
    QFETCH(double, i);
    QFETCH(double, j);
    QFETCH(double, k);
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("jsArrayMethodsWithParams.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {QStringLiteral("i"), i},
            {QStringLiteral("j"), j},
            {QStringLiteral("k"), k}
    }));
    QVERIFY(!object.isNull());

    listsEqual(object.data(), "CopyWithin");
    listsEqual(object.data(), "Fill");
    listsEqual(object.data(), "Slice");
    listsEqual(object.data(), "Splice");
    listsEqual(object.data(), "Spliced");

    QCOMPARE(object->property("listPropertyIndexOf"), object->property("jsArrayIndexOf"));
    QCOMPARE(object->property("listPropertyLastIndexOf"), object->property("jsArrayLastIndexOf"));
}

/*!
    Some of our list implementations ignore attempts to append a null object.
    This should result in warnings or type errors, and not crash our wrapper
    code.
*/
void tst_qqmllistreference::listIgnoresNull()
{
    QFETCH(const TestType::Mode, mode);
    static TestType::Mode globalMode;
    globalMode = mode;
    struct TestItem : public TestType
    {
        TestItem() : TestType(globalMode) {}
    };

    const auto id = qmlRegisterType<TestItem>("Test", 1, 0, "TestItem");
    const auto unregister = qScopeGuard([id]{
        QQmlPrivate::qmlunregister(QQmlPrivate::TypeRegistration, id);
    });

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("listIgnoresNull.qml"));

    // For lists that don't append null values, creating the component shouldn't crash
    // in the onCompleted handler, but generate type errors and warnings.
    if (mode == TestType::IgnoreNullValues) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* QML TestItem: List didn't append all objects$"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* TypeError: List doesn't append null objects$"));
    }
    QScopedPointer<QObject> object( component.create() );
    QVERIFY(object != nullptr);
}

void tst_qqmllistreference::consoleLogSyntheticList()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("consoleLogSyntheticList.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QTest::ignoreMessage(
            QtDebugMsg, QRegularExpression("\\[QObject_QML_[0-9]+\\(0x[0-9a-f]+\\)\\]"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
}

QTEST_MAIN(tst_qqmllistreference)

#include "tst_qqmllistreference.moc"
