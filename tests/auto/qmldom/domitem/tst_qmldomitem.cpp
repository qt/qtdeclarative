/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

#include <QtTest/QtTest>
#include <QCborValue>
#include <QDebug>

#include <memory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

DomItem wrapInt(const DomItem &self, Path p, const int &i){
    return self.subDataPath(p, i).item;
}

class TestDomItem: public QObject
{
    Q_OBJECT
public:

private slots:
    void initTestCase() {
        universePtr = std::make_shared<DomUniverse>(QStringLiteral(u"dummyUniverse"));
        envPtr = std::make_shared<DomEnvironment>(universePtr, QStringList());
        env = DomItem(envPtr);
    }

    void testList() {
        QList<int> l({1,2,3,4});
        QList<int> l2 = l;
        QList<int> l3({1});
        QList<int> l4 = l3;
        QCOMPARE(&(l[1]), &(l[1]));
        QCOMPARE(&(l3[0]), &(l3[0]));
        // QCOMPARE(&(l3[0]), &(l4[0])); // shallow copy actually copies els (QVector behavior)...
        DomItem list1 = env.subList(
                    List::fromQListRef<int>(Path::field(u"list"), l, &wrapInt)).item;
        DomItem list2 = env.subList(
                    List::fromQListRef<int>(Path::field(u"reverseList"), l, &wrapInt, ListOptions::Reverse)).item;
        QCOMPARE(list1.domKind(), DomKind::List);
        QCOMPARE(list1.indexes(), 4);
        QCOMPARE(list1[0].value().toInteger(), 1);
        QCOMPARE(list1[3].value().toInteger(), 4);
        QVERIFY(!list1[4]);
        QCOMPARE(list1[4].value().toInteger(-1), -1);
        QVERIFY(list1[0].value() != list2[0].value());
        QCOMPARE(list1[0].value(), list2[3].value());
        QCOMPARE(list1[3].value(), list2[0].value());
        QCOMPARE(list1.container(), env);
    }
    void testMap() {
        QMap<QString, int> map({{QStringLiteral(u"a"),1},{QStringLiteral(u"b"),2}});
        //QMap<QString, int> map2 = map;
        QMap<QString, int> map3({{QStringLiteral(u"a"),1}});
        //QMap<QString, int> map4 = map3;
        auto it = map.find(QStringLiteral(u"a"));
        auto it2 = map.find(QStringLiteral(u"a"));
        auto it3 = map3.find(QStringLiteral(u"a"));
        auto it4 = map3.find(QStringLiteral(u"a"));
        //auto it5 = map4.find(QStringLiteral(u"a"));
        QVERIFY(it != map.end());
        QVERIFY(it2 != map.end());
        QCOMPARE(&(*it), &(*it2));
        QCOMPARE(&(*it), &(map[QStringLiteral(u"a")]));
        QCOMPARE(&(it.value()), &(it2.value()));
        //QCOMPARE(&(*it), &(map2[QStringLiteral(u"a")]));
        QCOMPARE(&(*it3), &(*it4));
        //QCOMPARE(&(*it3), &(*it5));
        DomItem map1 = env.subMap(
                    Map::fromMapRef<int>(
                        Path::field(u"map"), map,
                        &wrapInt)).item;
        QCOMPARE(map1.domKind(), DomKind::Map);
        QCOMPARE(map1[u"a"].value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).value().toInteger(), 1);
        QCOMPARE(map1[u"b"].value().toInteger(), 2);
        QVERIFY(!map1[u"c"]);
        QCOMPARE(map1.container(), env);
    }
    void testMultiMap() {
        QMultiMap<QString, int> mmap({{QStringLiteral(u"a"),1},{QStringLiteral(u"b"),2},{QStringLiteral(u"a"),3}});
        //QMultiMap<QString, int> mmap2 = mmap;
        QMultiMap<QString, int> mmap3({{QStringLiteral(u"a"),1}});
        //QMultiMap<QString, int> mmap4 = mmap3;
        auto it = mmap.find(QStringLiteral(u"a"));
        auto it2 = mmap.find(QStringLiteral(u"a"));
        //auto it3 = mmap2.find(QStringLiteral(u"a"));
        auto it4 = mmap3.find(QStringLiteral(u"a"));
        auto it5 = mmap3.find(QStringLiteral(u"a"));
        //auto it6 = mmap4.find(QStringLiteral(u"a"));
        QVERIFY(it != mmap.end());
        QVERIFY(it2 != mmap.end());
        QCOMPARE(&(it.value()), &(it2.value()));
        QCOMPARE(&(*it), &(it2.value()));
        //QCOMPARE(&(*it), &(*it2)); // copy has different address (copies elements for int)
        //QCOMPARE(&(*it), &(*it3));
        QCOMPARE(&(*it4), &(*it5));
        //QCOMPARE(&(*it4), &(*it6));
        DomItem map1 = env.subMap(
                    Map::fromMultiMapRef<int>(
                        Path::field(u"mmap"), mmap,
                        &wrapInt)).item;
        QCOMPARE(map1[u"b"].index(0).value().toInteger(), 2);
        QVERIFY(!map1[u"b"].index(2));
        QVERIFY(!map1[u"c"]);
        QCOMPARE(map1[u"a"][0].value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).index(0).value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).index(1).value().toInteger(), 3);
        QCOMPARE(map1.container(), env);
    }
    void testReference() {
        Path p = Path::root(u"env");
        DomItem ref = env.subReferenceField(u"ref",p).item;
        QCOMPARE(ref.field(u"referredObjectPath").value().toString(), p.toString());
        QCOMPARE(ref.fields(), QList<QString>({QStringLiteral(u"referredObjectPath"), QStringLiteral(u"get")}));
        QCOMPARE(ref.field(u"get").internalKind(), DomType::DomEnvironment);
    }
    void testEnvUniverse() {
        QCOMPARE(env.internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.pathFromOwner(), Path());
        QCOMPARE(env.containingObject().internalKind(), DomType::Empty);
        QCOMPARE(env.container().internalKind(), DomType::Empty);
        QCOMPARE(env.canonicalPath(), Path::root(u"env"));
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
private:
    std::shared_ptr<DomUniverse> universePtr;
    std::shared_ptr<DomEnvironment> envPtr;
    DomItem env;
};


} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

QTEST_MAIN(QQmlJS::Dom::TestDomItem)
#include "tst_qmldomitem.moc"
