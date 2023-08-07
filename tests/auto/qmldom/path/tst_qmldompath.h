// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TST_QMLDOMPATH_H
#define TST_QMLDOMPATH_H
#include <QtQmlDom/private/qqmldompath_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>

#include <QtTest/QtTest>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
namespace PathEls {

class TestPaths: public QObject {
    Q_OBJECT
public:
    void testPathInternals(Path p1)
    {
        QCOMPARE(p1.component(0).kind(), Kind::Root);
        QCOMPARE(p1.component(1).kind(), Kind::Current);

        Path p11 = Path::Field(u"test");
        QString s = QLatin1String("test");
        Path p2 = Path::Field(s);
        Path p3 = Path::Field(QLatin1String("test"));
        QCOMPARE(p11, p2);
        QCOMPARE(p11, p3);
        QVERIFY(p11.m_data->strData.isEmpty());
        QCOMPARE(p2.m_data->strData.size(), 1);
        QCOMPARE(p2.m_data->strData.first(), s);
        QCOMPARE(p3.m_data->strData.size(), 1);
        QCOMPARE(p3.m_data->strData.first(), s);
    }

private slots:
    void pathComponentTestInternalAlloc() {
        PathComponent c;
        QCOMPARE(c.kind(), Kind::Empty);
        PathComponent c1{Current()};
        QCOMPARE(c1.kind(), Kind::Current);
        QVERIFY(c!=c1);
        QVERIFY(c<c1);
        QVERIFY(c1>c);
        PathComponent c1_1{Current(PathCurrent::Ids)};
        QCOMPARE(c1_1.kind(), Kind::Current);
        QVERIFY(c1 != c1_1);
        QVERIFY(c < c1_1);
        QCOMPARE(c1_1.name(), QLatin1String("@ids"));
        PathComponent c1_2{Current(u"ids")};
        QCOMPARE(c1_1, c1_2);
        PathComponent c2 = c1;
        QCOMPARE(c2.kind(), Kind::Current);
        QCOMPARE(c2, c1);
        PathComponent c3;
        QCOMPARE(c, c3);
        QCOMPARE(c3.kind(), Kind::Empty);
        c3 = c1;
        QCOMPARE(c3.kind(), Kind::Current);
        QCOMPARE(c3, c1);
        PathComponent c4{Field(u"bla")};
        QCOMPARE(c4.kind(), Kind::Field);
        QCOMPARE(c4.name(), QLatin1String("bla"));
        auto c5=PathComponent(Index(42));
        QCOMPARE(c5.kind(), Kind::Index);
        QCOMPARE(c5.index(), 42);
        auto c6 = PathComponent(Key(QStringLiteral(u"bla")));
        QCOMPARE(c6.kind(), Kind::Key);
        QCOMPARE(c6.name(), QLatin1String("bla"));
        auto c7 = PathComponent(Key(QStringLiteral(u" ugly\n \t \\string\"'bla")));
        QCOMPARE(c7.kind(), Kind::Key);
        QCOMPARE(c7.name(), QLatin1String(" ugly\n \t \\string\"'bla"));
        auto c8=PathComponent(Root(u"pippo"));
        QCOMPARE(c8.kind(), Kind::Root);
        QCOMPARE(c8.name(), QLatin1String("$pippo"));
        auto c8_1=PathComponent(Root(PathRoot::Env));
        QCOMPARE(c8_1.kind(), Kind::Root);
        QCOMPARE(c8_1.name(), QLatin1String("$env"));
        auto c8_2=PathComponent(Root(u"env"));
        QCOMPARE(c8_1, c8_2);
        auto c9=PathComponent(Current(u"ippo"));
        QCOMPARE(c9.kind(), Kind::Current);
        QCOMPARE(c9.name(), QLatin1String("@ippo"));
        auto c10=PathComponent(Any());
        QCOMPARE(c10.kind(), Kind::Any);
        QVERIFY(c9!=c10);
        auto c11=PathComponent(Filter([](DomItem){ return true; }));
        auto c12=c11;
        auto c13=PathComponent(Filter([](DomItem){ return false; }));
        auto c14=PathComponent(Filter([](DomItem){ return false; }, u"skipAll"));
        auto c15=PathComponent(Filter([](DomItem){ return true; }, u"skipAll"));
        QCOMPARE(c11.kind(), Kind::Filter);
        QCOMPARE(c11, c11);
        QVERIFY(c11 != c12); // native code assumed to be non comparable and different even if they are the same
        QVERIFY(c11 != c13);
        QVERIFY(c13 != c14);
        QVERIFY(c11 != c14);
        QCOMPARE(c14, c14);
        QCOMPARE(c14, c15); // same description (without < at the beginning) assumes same function even if different
    }

    void testPaths() {
        Path p;
        QCOMPARE(p.length(), 0);
        QCOMPARE(p.length(), 0);
        Path p0 = Path::Root();
        QCOMPARE(p0[0].headKind(), Kind::Root);
        QCOMPARE(p0.length(), 1);
        Path p1 = p0.current();
        QCOMPARE(p1.length(), 2);
        testPathInternals(p1);
        QCOMPARE(p1[0].headKind(), Kind::Root);
        QCOMPARE(p1[1].headKind(), Kind::Current);
        auto p2 = p1.field(u"aa");
        QCOMPARE(p2[2].headKind(), Kind::Field);
        auto p2b = p1.appendComponent(PathEls::Field(u"aa"));
        QCOMPARE(p2b.length(), 3);
        QCOMPARE(p2b[2].headKind(), Kind::Field);
        QCOMPARE(p2b, p2);
        auto p3a = p2.appendComponent(PathEls::Index(4));
        QCOMPARE(p3a[3].headKind(), Kind::Index);
        auto p3 = p2.index(4);
        QCOMPARE(p3.length(), 4);
        QCOMPARE(p3[3].headKind(), Kind::Index);
        QCOMPARE(p3, p3a);
        auto p4 = p3.key("bla");
        QCOMPARE(p4[4].headKind(), Kind::Key);
        auto p5 = p4.any();
        QCOMPARE(p5[5].headKind(), Kind::Any);
        auto p6 = p5.empty();
        QCOMPARE(p6[6].headKind(), Kind::Empty);
        auto rString = u"$.@.aa[4][\"bla\"][*].";
        QCOMPARE(p6.toString(), rString);
        auto p7 = p6.filter([](DomItem){ return true; }, u"true");
        auto p7Str = p7.toString();
        QCOMPARE(p7Str, u"$.@.aa[4][\"bla\"][*].[?(true)]");
        auto p8 = p7.dropTail();
        QCOMPARE(p8.length(), 7);
        QCOMPARE(p8.toString(), rString);
        QCOMPARE(p8, p6);
        auto p9 = Path::fromString(rString);
        QCOMPARE(p9.length(), 7);
        auto p9Str = p9.toString();
        QCOMPARE(p9Str, rString);
        QCOMPARE(p9, p6);
        auto p10 = p6.dropFront();
        QCOMPARE(p10.length(), 6);
        auto p10Str = p10.toString();
        auto r2Str = u"@.aa[4][\"bla\"][*].";
        QCOMPARE(p10Str, r2Str);
        auto p11 = Path::fromString(r2Str);
        auto p11Str = p11.toString();
        QCOMPARE(p11Str, r2Str);
        QCOMPARE(p10, p11);
        auto p12 = p7.mid(1,6);
        auto p12Str = p12.toString();
        QCOMPARE(p12Str, r2Str);
        QCOMPARE(p10, p12);
    }

    void testPathSplit()
    {
        const QList<Path> paths({Path(),
            Path::Root(PathRoot::Env).field(u"pippo").key(u"pluto").index(4),
            Path::Root(PathRoot::Env).field(u"pippo").key(u"pluto"),
            Path::Root(PathRoot::Env).field(u"pippo"),
            Path::Root(PathRoot::Env).field(u"pippo").field(u"pp"),
            Path::Root(PathRoot::Env),
            Path::Field(u"pippo").index(4),
            Path::Field(u"pippo").key(u"pluto").index(4),
            Path::Field(u"pippo").key(u"pluto"),
            Path::Field(u"pippo"),
            Path::Field(u"pippo").field(u"pp"),
            Path::Index(4),
            Path::Key(u"zz")
            });
        for (Path p : paths) {
            Source s = p.split();
            QCOMPARE(p, s.pathToSource.path(s.pathFromSource));
            if (!s.pathFromSource)
                QVERIFY(!s.pathToSource);
        }
        QCOMPARE(paths.at(1).split().pathToSource, Path::Root(PathRoot::Env));
        QCOMPARE(paths.at(2).split().pathToSource, Path::Root(PathRoot::Env));
        QCOMPARE(paths.at(3).split().pathToSource, Path::Root(PathRoot::Env));
        QCOMPARE(paths.at(4).split().pathToSource, Path::Root(PathRoot::Env).field(u"pippo"));
        QVERIFY(!paths.at(5).split().pathToSource);
        QVERIFY(!paths.at(6).split().pathToSource);
        QVERIFY(!paths.at(7).split().pathToSource);
        QVERIFY(!paths.at(8).split().pathToSource);
        QVERIFY(!paths.at(9).split().pathToSource);
        QCOMPARE(paths.at(10).split().pathToSource, Path::Field(u"pippo"));
        QVERIFY(!paths.at(11).split().pathToSource);
        QVERIFY(!paths.at(12).split().pathToSource);
    }
};

} // namespace PathEls
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
