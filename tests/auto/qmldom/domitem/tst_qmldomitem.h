// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TST_QMLDOMITEM_H
#define TST_QMLDOMITEM_H
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomastdumper_p.h>
#include <QtQmlDom/private/qqmldommock_p.h>
#include <QtQmlDom/private/qqmldomcompare_p.h>
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>
#include <QtQmlDom/private/qqmldomscriptelements_p.h>

#include <QtTest/QtTest>
#include <QtCore/QCborValue>
#include <QtCore/QDebug>
#include <QtCore/QLibraryInfo>
#include <QtCore/QFileInfo>

#include <memory>
#include <variant>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

inline DomItem wrapInt(DomItem &self, const PathEls::PathComponent &p, const int &i)
{
    return self.subDataItem(p, i);
}

class DomTestClass
{
public:
    std::shared_ptr<int> i;
};

class TestDomItem : public QObject
{
    Q_OBJECT
public:
    static ErrorGroups myErrors()
    {
        static ErrorGroups res { { NewErrorGroup("tests"), NewErrorGroup("domitem") } };
        return res;
    }

private slots:
    void initTestCase()
    {
        baseDir = QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/domitem");
        qmltypeDirs = QStringList({ baseDir, QLibraryInfo::path(QLibraryInfo::QmlImportsPath) });
        universePtr =
                std::shared_ptr<DomUniverse>(new DomUniverse(QStringLiteral(u"dummyUniverse")));
        envPtr = std::shared_ptr<DomEnvironment>(new DomEnvironment(
                QStringList(), DomEnvironment::Option::SingleThreaded, universePtr));
        env = DomItem(envPtr);
        testOwnerPtr = std::shared_ptr<MockOwner>(new MockOwner(
                Path::Root(u"env").field(u"testOwner"), 0,
                QMap<QString, MockObject> {
                        MockObject(
                                Path::Field(u"obj1"),
                                QMap<QString, MockObject> {
                                        MockObject(
                                                Path::Field(u"obj1").field(u"obj1_2"),
                                                QMap<QString, MockObject> {},
                                                QMap<QString, QCborValue> {
                                                        { QStringLiteral(u"val1"), QCborValue(3) },
                                                        { QLatin1String("val2"), QCborValue(4) } })
                                                .asStringPair() },
                                QMap<QString, QCborValue> {
                                        { QStringLiteral(u"val1"), QCborValue(1) },
                                        { QLatin1String("val2"), QCborValue(2) } })
                                .asStringPair(),
                        MockObject(
                                Path::Field(u"obj2"),
                                QMap<QString, MockObject> {
                                        MockObject(
                                                Path::Field(u"obj2").field(u"obj2_2"),
                                                QMap<QString, MockObject> {},
                                                QMap<QString, QCborValue> {
                                                        { QStringLiteral(u"val1"), QCborValue(5) },
                                                        { QLatin1String("val2"), QCborValue(6) },
                                                        { QLatin1String("valX"),
                                                          QCborValue(QStringLiteral(u"pippo")) } })
                                                .asStringPair() },
                                QMap<QString, QCborValue> {
                                        { QStringLiteral(u"val1"), QCborValue(7) },
                                        { QLatin1String("val2"), QCborValue(8) } })
                                .asStringPair() },
                QMap<QString, QCborValue> {
                        { QStringLiteral(u"val1"), QCborValue(9) },
                },
                QMap<QString, QMap<QString, MockObject>> {
                        { QStringLiteral(u"map"),
                          QMap<QString, MockObject> {
                                  MockObject(Path::Field(u"map").key(u"a"),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(10) },
                                                     { QLatin1String("val2"), QCborValue(11) } })
                                          .asStringPair(),
                                  MockObject(Path::Field(u"map").key(u"b"),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(12) },
                                                     { QLatin1String("val2"), QCborValue(13) } })
                                          .asStringPair() } } },
                QMap<QString, QMultiMap<QString, MockObject>> {
                        { QStringLiteral(u"mmap"),
                          QMultiMap<QString, MockObject> {
                                  { QStringLiteral(u"a"),
                                    MockObject(
                                            Path::Field(u"mmap").key(u"a").index(0),
                                            QMap<QString, MockObject> {},
                                            QMap<QString, QCborValue> {
                                                    { QStringLiteral(u"val1"), QCborValue(14) },
                                                    { QLatin1String("val2"), QCborValue(15) } }) },
                                  { QStringLiteral(u"a"),
                                    MockObject(Path::Field(u"mmap").key(u"a").index(1),
                                               QMap<QString, MockObject> {},
                                               QMap<QString, QCborValue> {
                                                       { QStringLiteral(u"val1"), QCborValue(16) },
                                                       { QLatin1String("val2"),
                                                         QCborValue(17) } }) } } } },
                QMap<QString, QList<MockObject>> {
                        { QStringLiteral(u"list"),
                          QList<MockObject> {
                                  MockObject(Path::Field(u"list").index(0),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(18) },
                                                     { QLatin1String("val2"), QCborValue(19) } }),
                                  MockObject(Path::Field(u"list").index(1),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(20) },
                                                     { QLatin1String("val2"),
                                                       QCborValue(21) } }) } } }));
        envPtr->setExtraOwningItem(QStringLiteral(u"testOwner"), testOwnerPtr);
        tOwner = env.field(u"testOwner");
    }

    void testList()
    {
        QList<int> l({ 1, 2, 3, 4 });
        QList<int> l2 = l;
        QList<int> l3({ 1 });
        QList<int> l4 = l3;
        QCOMPARE(&(l[1]), &(l[1]));
        QCOMPARE(&(l3[0]), &(l3[0]));
        // QCOMPARE(&(l3[0]), &(l4[0])); // shallow copy actually copies els (QVector behavior)...
        DomItem list1 = env.subListItem(List::fromQListRef<int>(Path::Field(u"list"), l, &wrapInt));
        DomItem list2 = env.subListItem(List::fromQListRef<int>(Path::Field(u"reverseList"), l,
                                                                &wrapInt, ListOptions::Reverse));
        QCOMPARE(list1.domKind(), DomKind::List);
        QCOMPARE(list1.indexes(), 4);
        QCOMPARE(list1[0].value().toInteger(), 1);
        QCOMPARE(list1[3].value().toInteger(), 4);
        QVERIFY(!list1[4]);
        QCOMPARE(list1[4].value().toInteger(-1), -1);
        QVERIFY(list1[0].value() != list2[0].value());
        QCOMPARE(list1[0].value(), list2[3].value());
        QCOMPARE(list1[3].value(), list2[0].value());
    }
    void testMap()
    {
        QMap<QString, int> map({ { QStringLiteral(u"a"), 1 }, { QStringLiteral(u"b"), 2 } });
        // QMap<QString, int> map2 = map;
        QMap<QString, int> map3({ { QStringLiteral(u"a"), 1 } });
        // QMap<QString, int> map4 = map3;
        auto it = map.find(QStringLiteral(u"a"));
        auto it2 = map.find(QStringLiteral(u"a"));
        auto it3 = map3.find(QStringLiteral(u"a"));
        auto it4 = map3.find(QStringLiteral(u"a"));
        // auto it5 = map4.find(QStringLiteral(u"a"));
        QVERIFY(it != map.end());
        QVERIFY(it2 != map.end());
        QCOMPARE(&(*it), &(*it2));
        QCOMPARE(&(*it), &(map[QStringLiteral(u"a")]));
        QCOMPARE(&(it.value()), &(it2.value()));
        // QCOMPARE(&(*it), &(map2[QStringLiteral(u"a")]));
        QCOMPARE(&(*it3), &(*it4));
        // QCOMPARE(&(*it3), &(*it5));
        DomItem map1 = env.subMapItem(Map::fromMapRef<int>(Path::Field(u"map"), map, &wrapInt));
        QCOMPARE(map1.domKind(), DomKind::Map);
        QCOMPARE(map1[u"a"].value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).value().toInteger(), 1);
        QCOMPARE(map1[u"b"].value().toInteger(), 2);
        QVERIFY(!map1[u"c"]);
    }
    void testMultiMap()
    {
        QMultiMap<QString, int> mmap({ { QStringLiteral(u"a"), 1 },
                                       { QStringLiteral(u"b"), 2 },
                                       { QStringLiteral(u"a"), 3 } });
        // QMultiMap<QString, int> mmap2 = mmap;
        QMultiMap<QString, int> mmap3({ { QStringLiteral(u"a"), 1 } });
        // QMultiMap<QString, int> mmap4 = mmap3;
        auto it = mmap.find(QStringLiteral(u"a"));
        auto it2 = mmap.find(QStringLiteral(u"a"));
        // auto it3 = mmap2.find(QStringLiteral(u"a"));
        auto it4 = mmap3.find(QStringLiteral(u"a"));
        auto it5 = mmap3.find(QStringLiteral(u"a"));
        // auto it6 = mmap4.find(QStringLiteral(u"a"));
        QVERIFY(it != mmap.end());
        QVERIFY(it2 != mmap.end());
        QCOMPARE(&(it.value()), &(it2.value()));
        QCOMPARE(&(*it), &(it2.value()));
        // QCOMPARE(&(*it), &(*it2)); // copy has different address (copies elements for int)
        // QCOMPARE(&(*it), &(*it3));
        QCOMPARE(&(*it4), &(*it5));
        // QCOMPARE(&(*it4), &(*it6));
        DomItem map1 = env.subMapItem(Map::fromMultiMapRef<int>(Path::Field(u"mmap"), mmap));
        QCOMPARE(map1[u"b"].index(0).value().toInteger(), 2);
        QVERIFY(!map1[u"b"].index(2));
        QVERIFY(!map1[u"c"]);
        QCOMPARE(map1[u"a"][0].value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).index(0).value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).index(1).value().toInteger(), 3);
        {
            QMultiMap<QString, DomTestClass> m1;
            m1.insert(QStringLiteral(u"xx"), DomTestClass { std::shared_ptr<int>(new int(4)) });
            QCOMPARE(m1.begin().value().i.use_count(), 1);
            QMultiMap<QString, DomTestClass> m2 = m1;
            m1.clear();
            auto it = m2.cbegin();
            auto end = m2.cend();
            while (it != end) {
                QCOMPARE(it.value().i.use_count(), 1);
                m1.insert(it.key(), it.value());
                QCOMPARE(it.value().i.use_count(), 2);
                ++it;
            }
            m2.insert(QStringLiteral(u"xy"), DomTestClass { std::shared_ptr<int>(new int(8)) });
            QMultiMap<QString, DomTestClass> m3 = m2;
            m3.begin().value() = DomTestClass { std::shared_ptr<int>(new int(2)) };
            auto it2 = m2.begin();
            auto it3 = m3.begin();
            QCOMPARE(*it2.value().i, 4);
            QCOMPARE(*it3.value().i, 2);
            QCOMPARE(it2.value().i.use_count(), 2);
            QCOMPARE(it3.value().i.use_count(), 1);
            m3.insert(QStringLiteral(u"xz"), DomTestClass { std::shared_ptr<int>(new int(16)) });
            it2 = m2.begin();
            it3 = m3.begin();
            QCOMPARE(*it2.value().i, 4);
            QCOMPARE(*it3.value().i, 2);
            QCOMPARE(it2.value().i.use_count(), 2);
            QCOMPARE(it3.value().i.use_count(), 1);
            ++it2;
            ++it3;
            QCOMPARE(*it2.value().i, 8);
            QCOMPARE(*it3.value().i, 8);
            QCOMPARE(it2.value().i.use_count(), 2);
            QCOMPARE(it3.value().i.use_count(), 2);
            ++it2;
            ++it3;
            QVERIFY(it2 == m2.end());
            QVERIFY(it3 != m3.end());
            QCOMPARE(*it3.value().i, 16);
            QCOMPARE(it3.value().i.use_count(), 1);
            ++it3;
            QVERIFY(it3 == m3.end());
        }
    }
    void testReference()
    {
        Path p = Path::Root(u"env");
        DomItem ref = env.subReferenceItem(PathEls::Field(u"ref"), p);
        QCOMPARE(ref.field(u"referredObjectPath").value().toString(), p.toString());
        QCOMPARE(ref.fields(),
                 QList<QString>({ QStringLiteral(u"referredObjectPath"), QStringLiteral(u"get") }));
        QCOMPARE(ref.field(u"get").internalKind(), DomType::DomEnvironment);
        // test stability (cache)
        QCOMPARE(ref.field(u"get").internalKind(), DomType::DomEnvironment);
    }
    void testRefCache()
    {
        Path refPath = env.canonicalPath().field(u"dummyRef");
        RefCacheEntry e0 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e0.cached, RefCacheEntry::Cached::None);
        bool didAdd1 = RefCacheEntry::addForPath(
                env, refPath, RefCacheEntry { RefCacheEntry::Cached::First, {} });
        QVERIFY(didAdd1);
        RefCacheEntry e1 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e1.cached, RefCacheEntry::Cached::All);
        QCOMPARE(e1.canonicalPaths.isEmpty(), true);
        bool didAdd2 = RefCacheEntry::addForPath(
                env, refPath,
                RefCacheEntry { RefCacheEntry::Cached::First, { env.canonicalPath() } },
                AddOption::Overwrite);
        QVERIFY(didAdd2);
        RefCacheEntry e2 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e2.cached, RefCacheEntry::Cached::First);
        QCOMPARE(e2.canonicalPaths.size(), 1);
        QCOMPARE(e2.canonicalPaths.first().toString(), env.canonicalPath().toString());
        bool didAdd3 = RefCacheEntry::addForPath(
                env, refPath,
                RefCacheEntry { RefCacheEntry::Cached::All,
                                { env.canonicalPath(), tOwner.canonicalPath() } },
                AddOption::Overwrite);
        QVERIFY(didAdd3);
        RefCacheEntry e3 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e3.cached, RefCacheEntry::Cached::All);
        QCOMPARE(e3.canonicalPaths.size(), 2);
        QCOMPARE(e3.canonicalPaths.first().toString(), env.canonicalPath().toString());
        QCOMPARE(e3.canonicalPaths.last().toString(), tOwner.canonicalPath().toString());
    }
    void testEnvUniverse()
    {
        QCOMPARE(env.internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.pathFromOwner(), Path());
        QCOMPARE(env.containingObject().internalKind(), DomType::Empty);
        QCOMPARE(env.container().internalKind(), DomType::Empty);
        QCOMPARE(env.canonicalPath(), Path::Root(u"env"));
        QCOMPARE(env.path(u"$env").internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.top().internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.environment().internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.owningItemPtr(), envPtr);
        QCOMPARE(env.topPtr(), envPtr);
        DomItem univ = env.universe();
        QCOMPARE(univ.internalKind(), DomType::DomUniverse);
        QCOMPARE(univ.owningItemPtr(), universePtr);
        DomItem univ2 = env.path(u".universe");
        QCOMPARE(univ2.internalKind(), DomType::DomUniverse);
        QCOMPARE(univ2.owningItemPtr(), universePtr);
        QCOMPARE(univ2.topPtr(), universePtr);
        DomItem univ3 = env.field(u"universe");
        QCOMPARE(univ3.internalKind(), DomType::DomUniverse);
    }

    void testTOwner()
    {
        QVERIFY(env.fields().contains(QLatin1String("testOwner")));
        QCOMPARE(tOwner.internalKind(), DomType::MockOwner);
        QCOMPARE(tOwner.pathFromOwner(), Path());
        DomItem map = tOwner.field(u"map");
        QCOMPARE(map[u"b"].field(u"val1").value().toInteger(), 12);
        QCOMPARE(map[u"b"].container(), map);
        QCOMPARE(map[u"b"].container()[u"b"].field(u"val1").value().toInteger(), 12);
        QCOMPARE(map[u"b"].containingObject(), tOwner);
        DomItem mmap = tOwner.field(u"mmap");
        QCOMPARE(mmap[u"a"].index(0).field(u"val1").value().toInteger(), 14);
        QCOMPARE(mmap[u"a"].container(), mmap);
        QCOMPARE(mmap[u"a"].container()[u"a"].index(0).field(u"val1").value().toInteger(), 14);
        QCOMPARE(mmap[u"a"].containingObject(), tOwner);
        QCOMPARE(mmap[u"a"].index(0).container(), mmap[u"a"]);
        QCOMPARE(mmap[u"a"].index(0).containingObject(), tOwner);
        DomItem list = tOwner.field(u"list");
        QCOMPARE(list[0].field(u"val1").value().toInteger(), 18);
        QCOMPARE(list.container(), tOwner);
        QCOMPARE(list[0].container(), list);
        QCOMPARE(list[0].containingObject(), tOwner);
        QCOMPARE(list[0].container()[0].field(u"val1").value().toInteger(), 18);
        QCOMPARE(tOwner.containingObject().internalKind(), DomType::DomEnvironment);
        QCOMPARE(tOwner.container().internalKind(), DomType::DomEnvironment);
        QCOMPARE(tOwner.fields(),
                 QStringList({ QStringLiteral(u"val1"), QStringLiteral(u"obj1"),
                               QStringLiteral(u"obj2"), QStringLiteral(u"map"),
                               QStringLiteral(u"mmap"), QStringLiteral(u"list") }));
        auto tOwner2 = env.path(u"$env.testOwner");
        QCOMPARE(tOwner2.internalKind(), DomType::MockOwner);
        auto tOwner3 = tOwner.path(u"$env.testOwner");
        QCOMPARE(tOwner3.internalKind(), DomType::MockOwner);
        QList<qint64> values;
        tOwner.visitTree(Path(), [&values](Path p, DomItem i, bool) {
            if (i.pathFromOwner() != p)
                myErrors()
                        .error(QStringLiteral(u"unexpected path %1 %2")
                                       .arg(i.pathFromOwner().toString(), p.toString()))
                        .handle(defaultErrorHandler);
            Q_ASSERT(i == i.path(i.canonicalPath()));
            if (DomItem v1 = i.path(u".val1"))
                values.append(v1.value().toInteger());
            return true;
        });
        QCOMPARE(values, QList<qint64>({ 9, 1, 3, 7, 5, 10, 12, 14, 16, 18, 20 }));
    }
    void testSubObj()
    {
        auto obj1 = tOwner.field(u"obj1");
        QCOMPARE(obj1.internalKind(), DomType::MockObject);
        auto obj1_1 = env.path(u".testOwner.obj1.obj1_2");
        QCOMPARE(obj1_1.internalKind(), DomType::MockObject);
        QCOMPARE(obj1_1.field(u"val1").value().toInteger(), 3);
    }
    void testEquality()
    {
        auto obj1 = tOwner.field(u"obj1");
        auto obj1_1 = env.path(u".testOwner.obj1.obj1_2");
        QCOMPARE(obj1_1.container(), obj1);
        QCOMPARE(obj1_1.environment(), env);
        QCOMPARE(obj1_1.owner(), tOwner);
    }
    void testLoadNoDep()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Test uncompatible with Android (QTBUG-100171)");
#endif
        auto univPtr = std::shared_ptr<QQmlJS::Dom::DomUniverse>(
                new QQmlJS::Dom::DomUniverse(QLatin1String("univ1")));
        auto envPtr = std::shared_ptr<QQmlJS::Dom::DomEnvironment>(new QQmlJS::Dom::DomEnvironment(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                univPtr));
        QQmlJS::Dom::DomItem env(envPtr);
        QVERIFY(env);
        QString testFile1 = baseDir + QLatin1String("/test1.qml");
        DomItem tFile;
        // env.loadBuiltins();
        env.loadFile(
                FileToLoad::fromFileSystem(envPtr, testFile1),
                [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; },
                LoadOption::DefaultLoad);
        env.loadFile(FileToLoad::fromFileSystem(envPtr, baseDir), {}, LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        QVERIFY(tFile);
        tFile = tFile.field(Fields::currentItem);
        QVERIFY(tFile);
        DomItem comp1 = tFile.field(Fields::components).key(QString()).index(0);
        QVERIFY(comp1);
        DomItem obj1 = comp1.field(Fields::objects).index(0);
        QVERIFY(obj1);

        tFile.visitTree(Path(), [&tFile](Path p, DomItem i, bool) {
            if (!(i == i.path(i.canonicalPath()))) {
                DomItem i2 = i.path(i.canonicalPath());
                qDebug() << p << i.canonicalPath() << i.internalKindStr() << i2.internalKindStr()
                         << i.id() << i2.id() << i.pathFromOwner() << i2.pathFromOwner();
            }
            Q_ASSERT(i == i.path(i.canonicalPath()));
            Q_ASSERT(!i || i == tFile.path(i.canonicalPath()));
            Q_ASSERT(!i || i.containingObject());

            return true;
        });

        {
            DomItem width = obj1.field(Fields::bindings).key(QLatin1String("width")).index(0);
            QCOMPARE(width.field(Fields::value).qmlObject(), obj1);
            DomItem w = obj1.bindings().key(QLatin1String("width"));
            QVERIFY(w.indexes() > 0);
            QCOMPARE(w.indexes(), 1);
            QVERIFY(w.index(0).as<Binding>());
            QVERIFY(w.index(0).as<Binding>()->scriptExpressionValue());
            QCOMPARE(w.index(0).as<Binding>()->scriptExpressionValue()->code(), u"640");
            PropertyInfo mPInfo;
            mPInfo.bindings = { width };
            mPInfo.propertyDefs.append(width);
            DomItem wrappedPInfo = obj1.wrapField(Fields::propertyInfos, mPInfo);
            QVERIFY(wrappedPInfo);
            const SimpleObjectWrapBase *wrappedPInfoPtr =
                    static_cast<const SimpleObjectWrapBase *>(wrappedPInfo.base());
            QVERIFY(wrappedPInfoPtr);
            const PropertyInfo *p1 =
                    reinterpret_cast<const PropertyInfo *>(wrappedPInfoPtr->m_value.data());
            PropertyInfo p2 = wrappedPInfoPtr->m_value.value<PropertyInfo>();
            QCOMPARE(mPInfo.bindings.size(), 1);
            QCOMPARE(mPInfo.propertyDefs.size(), 1);
            QCOMPARE(mPInfo.bindings.first().toString(), mPInfo.bindings.first().toString());
            QCOMPARE(mPInfo.propertyDefs.first().toString(),
                     mPInfo.propertyDefs.first().toString());

            QCOMPARE(p2.bindings.size(), 1);
            QCOMPARE(p2.propertyDefs.size(), 1);
            QCOMPARE(p2.bindings.first().toString(), mPInfo.bindings.first().toString());
            QCOMPARE(p2.propertyDefs.first().toString(), mPInfo.propertyDefs.first().toString());
            QCOMPARE(p1->bindings.size(), 1);
            QCOMPARE(p1->propertyDefs.size(), 1);
            QCOMPARE(p1->bindings.first().toString(), mPInfo.bindings.first().toString());
            QCOMPARE(p1->propertyDefs.first().toString(), mPInfo.propertyDefs.first().toString());
        }
        QString bPath = QFileInfo(baseDir).canonicalPath();
        if (!bPath.isEmpty()) {
            Path p = tFile.canonicalPath();
            Q_ASSERT(env.path(p));
            env.ownerAs<DomEnvironment>()->removePath(bPath);
            Q_ASSERT(!env.path(p));
        }
    }

    void testLoadDep()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Test uncompatible with Android (QTBUG-100171)");
#endif
        auto univPtr = std::shared_ptr<QQmlJS::Dom::DomUniverse>(
                new QQmlJS::Dom::DomUniverse(QLatin1String("univ1")));
        auto envPtr = std::shared_ptr<QQmlJS::Dom::DomEnvironment>(new QQmlJS::Dom::DomEnvironment(
                qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, univPtr));
        QQmlJS::Dom::DomItem env(envPtr);
        QVERIFY(env);
        QString testFile1 = baseDir + QLatin1String("/test1.qml");
        DomItem tFile;
        env.loadBuiltins();
        env.loadFile(
                FileToLoad::fromFileSystem(envPtr, testFile1),
                [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; },
                LoadOption::DefaultLoad);
        env.loadFile(FileToLoad::fromFileSystem(envPtr, baseDir), {}, LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        QVERIFY(tFile);
        tFile = tFile.field(Fields::currentItem);
        QVERIFY(tFile);
        DomItem comp1 = tFile.field(Fields::components).key(QString()).index(0);
        QVERIFY(comp1);
        DomItem obj1 = comp1.field(Fields::objects).index(0);
        QVERIFY(obj1);

        {
            using namespace Qt::StringLiterals;

            QList<DomItem> rect =
                    obj1.lookup(u"Rectangle"_s, LookupType::Type, LookupOption::Normal);
            QList<DomItem> rect2 =
                    obj1.lookup(u"Rectangle"_s, LookupType::Symbol, LookupOption::Normal);
            QList<DomItem> rectAs =
                    obj1.lookup(u"QQ.Rectangle"_s, LookupType::Symbol, LookupOption::Normal);

            QVERIFY(rect.size() == 1);
            QVERIFY(rect2.size() == 1);
            QVERIFY(rectAs.size() == 1);
            QCOMPARE(rect.first().internalKind(), DomType::Export);
            QCOMPARE(rect.first(), rect2.first());
            QCOMPARE(rect.first(), rectAs.first());
            DomItem rect3 = rect.first().proceedToScope();
            QCOMPARE(rect3.internalKind(), DomType::QmlObject);
            QList<DomItem> rects;
            obj1.resolve(
                    Path::Current(PathCurrent::Lookup).field(Fields::type).key(u"Rectangle"_s),
                    [&rects](Path, DomItem &el) {
                        rects.append(el);
                        return true;
                    },
                    {});
            QVERIFY(rects.size() == 1);
            for (DomItem &el : rects) {
                QCOMPARE(rect.first(), el);
            }
        }
        {
            QString fPath = tFile.canonicalFilePath();
            QString fPath2 = fPath.mid(0, fPath.lastIndexOf(u'/')) % u"/MySingleton.qml";
            Path p2 = Paths::qmlFileObjectPath(fPath2);
            DomItem f2 = env.path(p2);
            QVERIFY2(f2, "Directory dependencies did not load MySingleton.qml");
        }
    }

    void testImports()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Test uncompatible with Android (QTBUG-100171)");
#endif
        using namespace Qt::StringLiterals;

        QString testFile1 = baseDir + QLatin1String("/TestImports.qml");
        DomItem env = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), testFile1),
                [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        QVERIFY(tFile);
        QList<QmlUri> importedModules;
        for (auto &import : tFile.field(Fields::imports).values()) {
            if (const Import *importPtr = import.as<Import>()) {
                if (!importPtr->implicit)
                    importedModules.append(importPtr->uri);
            }
        }
        QCOMPARE(importedModules.at(0).moduleUri(), u"QtQuick"_s);
        QCOMPARE(importedModules.at(0).directoryString(), u""_s);
        QCOMPARE(importedModules.at(1).directoryString(), u"../.."_s);
        QCOMPARE(importedModules.at(1).localPath(), u"../.."_s);
        QCOMPARE(importedModules.at(1).absoluteLocalPath(), QString());
        QCOMPARE(importedModules.at(1).absoluteLocalPath(u"/bla/bla"_s), u"/bla/bla/../..");
        QCOMPARE(importedModules.at(2).directoryString(), u"../dommerging"_s);
        QCOMPARE(importedModules.at(2).localPath(), u"../dommerging"_s);
        QCOMPARE(importedModules.at(2).absoluteLocalPath(), QString());
        QCOMPARE(importedModules.at(2).absoluteLocalPath(u"/bla/bla"_s),
                 u"/bla/bla/../dommerging");
        QCOMPARE(importedModules.at(3).directoryString(), u"C:/some/path"_s);
        QCOMPARE(importedModules.at(3).localPath(), u"C:/some/path"_s);
        QCOMPARE(importedModules.at(4).directoryString(), u"http://bla.com/"_s);
        QCOMPARE(importedModules.at(4).directoryUrl().toString(), u"http://bla.com/"_s);
        QCOMPARE(importedModules.at(5).absoluteLocalPath(), u"/absolute/path"_s);
        QVERIFY(QmlUri::fromDirectoryString("QtQuick") != importedModules.at(0));
        QCOMPARE(QmlUri::fromUriString("QtQuick"), importedModules.at(0));
    }
    void testDeepCopy()
    {
        QString testFile = baseDir + QLatin1String("/test1.qml");

        DomItem env = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile; // place where to store the loaded file
        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), testFile),
                [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();
        DomItem f = tFile.fileObject();
        QString dump1;
        f.dump([&dump1](QStringView v) { dump1.append(v); });
        MutableDomItem copy = f.makeCopy();
        QString dump2;
        copy.item().dump([&dump2](QStringView v) { dump2.append(v); });
        QString diff = lineDiff(dump1, dump2, 2);
        if (!diff.isEmpty())
            qDebug().nospace().noquote() << diff;
        QCOMPARE(dump1, dump2);
        QStringList diffs = domCompareStrList(f, copy, FieldFilter::compareFilter());
        if (!diffs.isEmpty())
            qDebug() << "testDeepCopy.diffs:" << diffs;
        QVERIFY(diffs.isEmpty());
        DomItem univFile = env.universe().path(f.canonicalPath());
        MutableDomItem univFileCopy = univFile.makeCopy();
        QStringList univFileDiffs =
                domCompareStrList(univFile, univFileCopy, FieldFilter::compareFilter());
        if (!univFileDiffs.isEmpty())
            qDebug() << "testDeepCopy.univFileDiffs:" << univFileDiffs;
        QVERIFY(univFileDiffs.isEmpty());
        QString bPath = QFileInfo(baseDir).canonicalFilePath();
        if (!bPath.isEmpty()) {
            Path p = f.canonicalPath();
            Q_ASSERT(env.path(p));
            env.ownerAs<DomEnvironment>()->removePath(bPath);
            Q_ASSERT(!env.path(p));
        }
    }

    void testInMemory()
    {
        DomItem res = DomItem::fromCode("MyItem{}");
        DomItem obj = res.qmlObject(GoTo::MostLikely);
        QCOMPARE(obj.name(), u"MyItem");
    }

    static void checkAliases(DomItem &qmlObj)
    {
        using namespace Qt::StringLiterals;

        if (const QmlObject *qmlObjPtr = qmlObj.as<QmlObject>()) {
            auto pDefs = qmlObjPtr->propertyDefs();
            auto i = pDefs.constBegin();
            while (i != pDefs.constEnd()) {
                if (i.value().isAlias()) {
                    QString propName = i.key();
                    DomItem value = qmlObj.bindings().key(propName).index(0).field(Fields::value);
                    LocallyResolvedAlias rAlias =
                            qmlObjPtr->resolveAlias(qmlObj, value.ownerAs<ScriptExpression>());
                    if (propName.startsWith(u"a")) {
                        QCOMPARE(rAlias.baseObject.internalKind(), DomType::QmlObject);
                        switch (propName.last(1).at(0).unicode()) {
                        case u'i':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedProperty);
                            QCOMPARE(rAlias.typeName, u"int"_s);
                            QVERIFY(rAlias.accessedPath.isEmpty());
                            QCOMPARE(rAlias.localPropertyDef.internalKind(),
                                     DomType::PropertyDefinition);
                            break;
                        case u'r':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedProperty);
                            QCOMPARE(rAlias.typeName, u"real"_s);
                            QVERIFY(rAlias.accessedPath.isEmpty());
                            QCOMPARE(rAlias.localPropertyDef.internalKind(),
                                     DomType::PropertyDefinition);
                            break;
                        case u'I':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                            QCOMPARE(rAlias.typeName, u"Item"_s);
                            QCOMPARE(rAlias.accessedPath, QStringList { u"objectName"_s });
                            QVERIFY(!rAlias.localPropertyDef);
                            break;
                        case u'q':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                            QCOMPARE(rAlias.typeName, u"QtObject"_s);
                            QCOMPARE(rAlias.accessedPath, QStringList { u"objectName"_s });
                            QVERIFY(!rAlias.localPropertyDef);
                            break;
                        case u'Q':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                            QCOMPARE(rAlias.typeName, u"QtObject"_s);
                            QCOMPARE(rAlias.accessedPath, QStringList { u"objectName"_s });
                            QVERIFY(rAlias.localPropertyDef);
                            break;
                        default:
                            Q_ASSERT(false);
                        }
                    } else if (propName.startsWith(u"loop")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::Loop);
                    } else if (propName.startsWith(u"tooDeep")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::TooDeep);
                    } else if (propName.startsWith(u"invalid")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::Invalid);
                    } else if (propName.startsWith(u"objRef")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                    } else {
                        Q_ASSERT(false);
                    }
                }
                ++i;
            }
            for (DomItem obj : qmlObj.children().values()) {
                if (obj.as<QmlObject>())
                    checkAliases(obj);
            }
        }
    }

    void testAliasResolve_data()
    {
        QTest::addColumn<QString>("inFile");

        QTest::newRow("aliasProperties") << QStringLiteral(u"aliasProperties.qml");
        QTest::newRow("invalidAliasProperties") << QStringLiteral(u"invalidAliasProperties.qml");
    }
    void testAliasResolve()
    {
        using namespace Qt::StringLiterals;

        QFETCH(QString, inFile);
        QString testFile1 = baseDir + u"/"_s + inFile;
        DomItem env = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), testFile1),
                [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        DomItem rootObj = tFile.qmlObject(GoTo::MostLikely);
        checkAliases(rootObj);
    }

    void inlineComponents()
    {
        using namespace Qt::StringLiterals;

        QString testFile = baseDir + u"/inlineComponents.qml"_s;

        DomItem env = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), testFile),
                [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        auto rootQmlObject = tFile.rootQmlObject(GoTo::MostLikely);

        // check if the lookup can find the inline components correctly, to see if the
        // visitScopeChain also visit them.
        auto ic3 = rootQmlObject.lookup("IC3", LookupType::Type, LookupOption::Normal,
                                        [](const ErrorMessage &) {});

        QCOMPARE(ic3.size(), 1);
        QCOMPARE(ic3.front().name(), "inlineComponents.IC3");

        auto ic1 = rootQmlObject.lookup("IC1", LookupType::Type, LookupOption::Normal,
                                        [](const ErrorMessage &) {});

        QCOMPARE(ic1.size(), 1);
        QCOMPARE(ic1.front().name(), "inlineComponents.IC1");
    }

    void inlineObject()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/inlineObject.qml"_s;

        DomItem env = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), testFile),
                [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        auto rootQmlObject = tFile.rootQmlObject(GoTo::MostLikely);

        // check that the inline objects have their prototypes set.

        {
            auto prototypes = rootQmlObject.propertyInfos()
                                      .key(u"myItem"_s)
                                      .field(Fields::bindings)
                                      .index(0)
                                      .field(Fields::value)
                                      .field(Fields::prototypes);
            QVERIFY(prototypes.internalKind() != DomType::Empty);
            QCOMPARE(prototypes.indexes(), 1);
            QCOMPARE(prototypes.index(0)
                             .field(Fields::referredObjectPath)
                             .as<ConstantData>()
                             ->value()
                             .toString(),
                     u"@lookup.type[\"Item\"]"_s);
        }

        {
            auto prototypes2 = rootQmlObject.propertyInfos()
                                       .key(u"myItem2"_s)
                                       .field(Fields::bindings)
                                       .index(0)
                                       .field(Fields::value)
                                       .field(Fields::prototypes);
            QVERIFY(prototypes2.internalKind() != DomType::Empty);
            QCOMPARE(prototypes2.indexes(), 1);
            QCOMPARE(prototypes2.index(0)
                             .field(Fields::referredObjectPath)
                             .as<ConstantData>()
                             ->value()
                             .toString(),
                     u"@lookup.type[\"IC\"]"_s);
        }
    }

    void scopesInDom()
    {
        QString fileName = baseDir + u"/checkScopes.qml"_s;

        const QStringList importPaths = {
            QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
        };

        DomItem tFile;

        DomItem env = DomEnvironment::create(
                importPaths,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), fileName,
                                           WithSemanticAnalysis),
                [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        auto root = tFile.rootQmlObject(GoTo::MostLikely);

        {
            auto rootQmlObject = root.as<QmlObject>();
            QVERIFY(rootQmlObject);
            auto rootScope = rootQmlObject->semanticScope();
            QVERIFY(rootScope);
            QVERIFY(rootScope->hasOwnProperty("myInt"));
            QVERIFY(rootScope->hasOwnProperty("myInt2"));
            QVERIFY(rootScope->hasOwnPropertyBindings("myInt"));
            QVERIFY(rootScope->hasOwnPropertyBindings("myInt2"));
        }
    }

    void domConstructionTime_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<DomCreationOptions>("withScope");

        DomCreationOptions withScope = DomCreationOption::WithSemanticAnalysis;
        DomCreationOptions noScope = DomCreationOption::None;
        DomCreationOptions withScopeAndScriptExpressions;
        withScopeAndScriptExpressions.setFlag(DomCreationOption::WithSemanticAnalysis);
        withScopeAndScriptExpressions.setFlag(DomCreationOption::WithScriptExpressions);

        QTest::addRow("tiger.qml") << baseDir + u"/longQmlFile.qml"_s << noScope;
        QTest::addRow("tiger.qml-with-scope") << baseDir + u"/longQmlFile.qml"_s << withScope;
        QTest::addRow("tiger.qml-with-scope-and-scriptexpressions")
                << baseDir + u"/longQmlFile.qml"_s << withScopeAndScriptExpressions;

        QTest::addRow("deeplyNested.qml") << baseDir + u"/deeplyNested.qml"_s << noScope;
        QTest::addRow("deeplyNested.qml-with-scope")
                << baseDir + u"/deeplyNested.qml"_s << withScope;
        QTest::addRow("deeplyNested.qml-with-scope-and-scriptexpressions")
                << baseDir + u"/deeplyNested.qml"_s << withScopeAndScriptExpressions;
    }

    void domConstructionTime()
    {
        QFETCH(QString, fileName);
        QFETCH(DomCreationOptions, withScope);

        const QStringList importPaths = {
            QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
        };

        DomItem tFile;
        QBENCHMARK {
            DomItem env = DomEnvironment::create(
                    importPaths,
                    QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                            | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

            env.loadFile(
                    FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), fileName, withScope),
                    [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                    LoadOption::DefaultLoad);
            env.loadPendingDependencies();
        }
    }

    void propertyBindings()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/propertyBindings.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        // check the binding to a and b
        DomItem a = rootQmlObject.path(".bindings[\"a\"][0].value.scriptElement.value");
        QCOMPARE(a.value().toDouble(), 42);

        DomItem b = rootQmlObject.path(".bindings[\"b\"][0].value.scriptElement.identifier");
        QCOMPARE(b.value().toString(), "a");
    }

    void variableDeclarations()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/variableDeclarations.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement");

        // This block should have a semantic scope that defines sum and helloWorld
        auto blockSemanticScope = block.semanticScope();
        QVERIFY(blockSemanticScope);
        QVERIFY(*blockSemanticScope);
        QVERIFY(blockSemanticScope.value()->JSIdentifier(u"sum"_s));
        QVERIFY(blockSemanticScope.value()->JSIdentifier(u"helloWorld"_s));

        DomItem statements = block.field(Fields::statements);
        QCOMPARE(statements.indexes(), 2);

        // let sum = 0, helloWorld = "hello"
        DomItem variableDeclaration = statements.index(0).field(Fields::declarations);
        QCOMPARE(variableDeclaration.indexes(), 2);
        DomItem sumInitialization = variableDeclaration.index(0);
        QCOMPARE(sumInitialization.field(Fields::identifier).value().toString(), "sum");
        QCOMPARE(sumInitialization.field(Fields::initializer)
                         .field(Fields::value)
                         .value()
                         .toDouble(),
                 0);

        DomItem helloWorldInitialization = variableDeclaration.index(1);
        QCOMPARE(helloWorldInitialization.field(Fields::identifier).value().toString(),
                 "helloWorld");
        QCOMPARE(helloWorldInitialization.field(Fields::initializer)
                         .field(Fields::value)
                         .value()
                         .toString(),
                 "hello");
    }

    void ifStatements()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/ifStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"conditional\"][0].body.scriptElement");
        DomItem statements = block.field(Fields::statements);
        QCOMPARE(statements.indexes(), 5);

        // let i = 5
        DomItem iDeclaration = statements.index(0);
        QCOMPARE(iDeclaration.internalKind(), DomType::ScriptVariableDeclaration);

        {
            // if (i)
            //     i = 42
            DomItem conditional = statements.index(1);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(condition.field(Fields::identifier).value().toString(), u"i"_s);

            DomItem consequence = conditional.field(Fields::consequence);
            QCOMPARE(consequence.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(consequence.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(consequence.field(Fields::right).field(Fields::value).value().toDouble(), 42);

            QCOMPARE(conditional.field(Fields::alternative).internalKind(), DomType::Empty);
        }
        {
            // if (i == 55)
            //     i = 32
            // else
            //     i = i - 1
            DomItem conditional = statements.index(2);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 55);

            DomItem consequence = conditional.field(Fields::consequence);
            QCOMPARE(consequence.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(consequence.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(consequence.field(Fields::right).field(Fields::value).value().toDouble(), 32);

            DomItem alternative = conditional.field(Fields::alternative);
            QCOMPARE(alternative.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(alternative.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(alternative.field(Fields::right).internalKind(),
                     DomType::ScriptBinaryExpression);
        }
        {
            // if (i == 42) {
            //     i = 111
            // }
            DomItem conditional = statements.index(3);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 42);

            DomItem consequence = conditional.field(Fields::consequence);
            auto blockSemanticScope = consequence.semanticScope();
            QVERIFY(blockSemanticScope);
            QVERIFY(*blockSemanticScope);
            QCOMPARE(consequence.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(consequence.field(Fields::statements).indexes(), 1);
            DomItem consequence1 = consequence.field(Fields::statements).index(0);
            QCOMPARE(consequence1.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(consequence1.field(Fields::right).field(Fields::value).value().toDouble(),
                     111);

            QCOMPARE(conditional.field(Fields::alternative).internalKind(), DomType::Empty);
        }
        {
            // if (i == 746) {
            //     i = 123
            // } else {
            //     i = 456
            // }

            DomItem conditional = statements.index(4);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 746);

            {
                DomItem consequence = conditional.field(Fields::consequence);
                auto blockSemanticScope = consequence.semanticScope();
                QVERIFY(blockSemanticScope);
                QVERIFY(*blockSemanticScope);
                QCOMPARE(consequence.internalKind(), DomType::ScriptBlockStatement);
                QCOMPARE(consequence.field(Fields::statements).indexes(), 1);
                DomItem consequence1 = consequence.field(Fields::statements).index(0);
                QCOMPARE(consequence1.field(Fields::left)
                                 .field(Fields::identifier)
                                 .value()
                                 .toString(),
                         u"i"_s);
                QCOMPARE(consequence1.field(Fields::right).field(Fields::value).value().toDouble(),
                         123);
            }

            {
                DomItem alternative = conditional.field(Fields::alternative);
                auto blockSemanticScope = alternative.semanticScope();
                QVERIFY(blockSemanticScope);
                QVERIFY(*blockSemanticScope);
                QCOMPARE(alternative.internalKind(), DomType::ScriptBlockStatement);
                QCOMPARE(alternative.field(Fields::statements).indexes(), 1);
                DomItem alternative1 = alternative.field(Fields::statements).index(0);
                QCOMPARE(alternative1.field(Fields::left)
                                 .field(Fields::identifier)
                                 .value()
                                 .toString(),
                         u"i"_s);
                QCOMPARE(alternative1.field(Fields::right).field(Fields::value).value().toDouble(),
                         456);
            }
        }
    }

    void returnStatement()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/returnStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"returningFunction\"][0].body.scriptElement");
        QCOMPARE(block.internalKind(), DomType::ScriptBlockStatement);
        QCOMPARE(block.field(Fields::statements).indexes(), 1);
        DomItem conditional = block.field(Fields::statements).index(0);
        DomItem consequence = conditional.field(Fields::consequence);
        QCOMPARE(consequence.internalKind(), DomType::ScriptReturnStatement);
        {
            DomItem returnValue = consequence.field(Fields::expression);
            QCOMPARE(returnValue.internalKind(), DomType::ScriptLiteral);
            QCOMPARE(returnValue.field(Fields::value).value().toDouble(), 123);
        }
        DomItem alternative = conditional.field(Fields::alternative);
        QCOMPARE(alternative.internalKind(), DomType::ScriptReturnStatement);
        {
            DomItem returnValue = alternative.field(Fields::expression);
            QCOMPARE(returnValue.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(returnValue.field(Fields::left).field(Fields::value).value().toDouble(), 1);
            QCOMPARE(returnValue.field(Fields::right).field(Fields::value).value().toDouble(), 2);
        }
    }

    void forStatements()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/forStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement");
        DomItem statements = block.field(Fields::statements);
        DomItem forLoop = statements.index(1);
        {
            // for ( >> let i = 0 << ; i < 100; i = i + 1) {
            DomItem declarationList =
                    forLoop.field(Fields::declarations).field(Fields::declarations);
            QCOMPARE(declarationList.internalKind(), DomType::List);

            QCOMPARE(declarationList.indexes(), 1);
            DomItem declaration = declarationList.index(0);

            QCOMPARE(declaration.internalKind(), DomType::ScriptVariableDeclarationEntry);
            QCOMPARE(declaration.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);

            QCOMPARE(declaration.field(Fields::identifier).value().toString(), "i");
            QCOMPARE(declaration.field(Fields::initializer).field(Fields::value).value().toDouble(),
                     0);
        }
        {
            // for ( let i = 0; >> i < 100 <<; i = i + 1) {
            DomItem condition = forLoop.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);

            QCOMPARE(condition.field(Fields::left).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(condition.field(Fields::left).field(Fields::identifier).value().toString(),
                     "i");

            QCOMPARE(condition.field(Fields::right).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 100);
        }
        {
            // for ( let i = 0; i < 100; >> i = i + 1 << ) {
            DomItem expression = forLoop.field(Fields::expression);
            QCOMPARE(expression.internalKind(), DomType::ScriptBinaryExpression);
            DomItem left = expression.field(Fields::left);
            QCOMPARE(left.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(left.field(Fields::identifier).value().toString(), "i");

            // for ( let i = 0; i < 100; i = >> i + 1 << ) {
            DomItem right = expression.field(Fields::right);
            QCOMPARE(right.internalKind(), DomType::ScriptBinaryExpression);
            DomItem left2 = right.field(Fields::left);
            QCOMPARE(left2.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(left2.field(Fields::identifier).value().toString(), "i");

            DomItem right2 = right.field(Fields::right);
            QCOMPARE(right2.internalKind(), DomType::ScriptLiteral);
            QCOMPARE(right2.field(Fields::value).value().toDouble(), 1);
        }
        {
            // test the body of the for-loop
            DomItem body = forLoop.field(Fields::body);
            auto blockSemanticScope = body.semanticScope();
            QVERIFY(blockSemanticScope);
            QVERIFY(*blockSemanticScope);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
            DomItem statementList = body.field(Fields::statements);
            QCOMPARE(statementList.indexes(), 2);
            {
                //  >> sum = sum + 1 <<
                DomItem binaryExpression = statementList.index(0);
                QCOMPARE(binaryExpression.internalKind(), DomType::ScriptBinaryExpression);

                DomItem left = binaryExpression.field(Fields::left);
                QCOMPARE(left.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(left.field(Fields::identifier).value().toString(), "sum");

                //  sum = >> sum + 1 <<
                DomItem right = binaryExpression.field(Fields::right);
                QCOMPARE(right.internalKind(), DomType::ScriptBinaryExpression);

                DomItem left2 = right.field(Fields::left);
                QCOMPARE(left2.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(left2.field(Fields::identifier).value().toString(), "sum");

                DomItem right2 = right.field(Fields::right);
                QCOMPARE(right2.internalKind(), DomType::ScriptLiteral);
                QCOMPARE(right2.field(Fields::value).value().toDouble(), 1);
            }

            {
                //  >> for (;;) <<
                //         i = 42
                DomItem innerForLoop = statementList.index(1);
                QCOMPARE(innerForLoop.internalKind(), DomType::ScriptForStatement);
                QCOMPARE(innerForLoop.field(Fields::declarations).indexes(), 0);
                QVERIFY(!innerForLoop.field(Fields::initializer));
                QVERIFY(!innerForLoop.field(Fields::condition));
                QVERIFY(!innerForLoop.field(Fields::expression));
                QVERIFY(innerForLoop.field(Fields::body));

                //  for (;;)
                //     >> i = 42 <<
                DomItem expression = innerForLoop.field(Fields::body);
                QCOMPARE(expression.internalKind(), DomType::ScriptBinaryExpression);

                DomItem left = expression.field(Fields::left);
                QCOMPARE(left.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(left.field(Fields::identifier).value().toString(), "i");

                DomItem right = expression.field(Fields::right);
                QCOMPARE(right.internalKind(), DomType::ScriptLiteral);
                QCOMPARE(right.field(Fields::value).value().toDouble(), 42);
            }
        }
    }

private:
    static DomItem rootQmlObjectFromFile(const QString &path, QStringList &qmltypeDirs)
    {
        DomItem env = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        DomCreationOptions options;
        options.setFlag(DomCreationOption::WithScriptExpressions);
        options.setFlag(DomCreationOption::WithSemanticAnalysis);

        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), path, options),
                [&tFile](Path, DomItem &, DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        return tFile.rootQmlObject(GoTo::MostLikely);
    }

private:
    QString baseDir;
    QStringList qmltypeDirs;
    std::shared_ptr<DomUniverse> universePtr;
    std::shared_ptr<DomEnvironment> envPtr;
    DomItem env;
    std::shared_ptr<MockOwner> testOwnerPtr;
    DomItem tOwner;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
