// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLDOMPATH_H
#define TST_QMLDOMPATH_H
#include <QtQmlDom/private/qqmldompath_p.h>
#include <QtQmlDomPathParser/private/qqmldompathparser_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>

#include <QtTest/QTest>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
namespace PathEls {

class TestPaths: public QObject {
    Q_OBJECT
public:
    void testPathInternals(const Path &p1)
    {
        QCOMPARE(p1.component(0).kind(), Kind::Root);
        QCOMPARE(p1.component(1).kind(), Kind::Current);
    }

private slots:
    void pathComponentTestInternalAlloc() {
        PathComponent c;
        QCOMPARE(c.kind(), Kind::Empty);
        PathComponent c1{PathCurrent::Obj};
        QCOMPARE(c1.kind(), Kind::Current);
        QVERIFY(c!=c1);
        QVERIFY(c<c1);
        QVERIFY(c1>c);
        PathComponent c1_1{PathCurrent::Ids};
        QCOMPARE(c1_1.kind(), Kind::Current);
        QVERIFY(c1 != c1_1);
        QVERIFY(c < c1_1);
        QCOMPARE(c1_1.name(), QLatin1String("@ids"));
        PathComponent c2 = c1;
        QCOMPARE(c2.kind(), Kind::Current);
        QCOMPARE(c2, c1);
        PathComponent c3;
        QCOMPARE(c, c3);
        QCOMPARE(c3.kind(), Kind::Empty);
        c3 = c1;
        QCOMPARE(c3.kind(), Kind::Current);
        QCOMPARE(c3, c1);
        PathComponent c4{Key(QStringLiteral(u"bla"))};
        QCOMPARE(c4.kind(), Kind::Key);
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
        auto c8_1=PathComponent(PathRoot::Env);
        QCOMPARE(c8_1.kind(), Kind::Root);
        QCOMPARE(c8_1.name(), QLatin1String("$env"));
    }

    void testPaths() {
        Path p;
        QCOMPARE(p.length(), 0);
        QCOMPARE(p.length(), 0);
        Path p0 = Path::fromRoot(PathRoot::Env);
        QCOMPARE(p0[0].headKind(), Kind::Root);
        QCOMPARE(p0.length(), 1);
        Path p1 = p0.withCurrent(PathCurrent::Obj);
        QCOMPARE(p1.length(), 2);
        testPathInternals(p1);
        QCOMPARE(p1[0].headKind(), Kind::Root);
        QCOMPARE(p1[1].headKind(), Kind::Current);
        auto p2 = p1.withKey(u"aa");
        QCOMPARE(p2[2].headKind(), Kind::Key);
        auto p2b = p1.withComponent(PathEls::Key(QStringLiteral(u"aa")));
        QCOMPARE(p2b.length(), 3);
        QCOMPARE(p2b[2].headKind(), Kind::Key);
        QCOMPARE(p2b, p2);
        auto p3a = p2.withComponent(PathEls::Index(4));
        QCOMPARE(p3a[3].headKind(), Kind::Index);
        auto p3 = p2.withIndex(4);
        QCOMPARE(p3.length(), 4);
        QCOMPARE(p3[3].headKind(), Kind::Index);
        QCOMPARE(p3, p3a);
        auto p4 = p3.withKey(u"bla");
        QCOMPARE(p4[4].headKind(), Kind::Key);
        auto p6 = p4.withEmpty();
        QCOMPARE(p6[5].headKind(), Kind::Empty);
        auto rString = u"$env.@obj[\"aa\"][4][\"bla\"].";
        QCOMPARE(p6.toString(), rString);
        auto p8 = p6.dropTail();
        QCOMPARE(p8.length(), 5);
        auto p9 = parsePath(rString);
        QCOMPARE(p9.length(), 6);
        auto p9Str = p9.toString();
        QCOMPARE(p9Str, rString);
        QCOMPARE(p9, p6);
        auto p10 = p6.dropFront();
        QCOMPARE(p10.length(), 5);
        auto p10Str = p10.toString();
        auto r2Str = u"@obj[\"aa\"][4][\"bla\"].";
        QCOMPARE(p10Str, r2Str);
        auto p11 = parsePath(r2Str);
        auto p11Str = p11.toString();
        QCOMPARE(p11Str, r2Str);
        QCOMPARE(p10, p11);
    }

    void testPathSplit()
    {
        // Path::split() fires on Field/Root/Current components; exercise it with
        // real Fields::* names rather than ad-hoc strings (which would parse to Keys).
        const QList<Path> paths({Path(),
            Path::fromRoot(PathRoot::Env).withField(Fields::children).withField(Fields::name).withIndex(4),
            Path::fromRoot(PathRoot::Env).withField(Fields::children).withField(Fields::name),
            Path::fromRoot(PathRoot::Env).withField(Fields::children),
            Path::fromRoot(PathRoot::Env).withField(Fields::children).withField(Fields::value),
            Path::fromRoot(PathRoot::Env),
            Path::fromKey(u"pippo").withIndex(4),
            Path::fromKey(u"pippo").withKey(u"pluto").withIndex(4),
            Path::fromKey(u"pippo").withKey(u"pluto"),
            Path::fromKey(u"pippo"),
            Path::fromKey(u"pippo").withField(Fields::name),
            Path::fromIndex(4),
            Path::fromKey(u"zz")
            });
        for (const Path &p : paths) {
            Source s = p.split();
            QCOMPARE(p, s.pathToSource.withPath(s.pathFromSource));
        }
        QCOMPARE(paths.at(1).split().pathToSource,
                 Path::fromRoot(PathRoot::Env).withField(Fields::children));
        QCOMPARE(paths.at(2).split().pathToSource,
                 Path::fromRoot(PathRoot::Env).withField(Fields::children));
        QCOMPARE(paths.at(3).split().pathToSource, Path::fromRoot(PathRoot::Env));
        QCOMPARE(paths.at(4).split().pathToSource,
                 Path::fromRoot(PathRoot::Env).withField(Fields::children));
        // Lone Root: pathToSource is the Root anchor itself, pathFromSource is empty.
        QCOMPARE(paths.at(5).split().pathToSource, Path::fromRoot(PathRoot::Env));
        QVERIFY(!paths.at(5).split().pathFromSource);
        QVERIFY(!paths.at(6).split().pathToSource);
        QVERIFY(!paths.at(7).split().pathToSource);
        QVERIFY(!paths.at(8).split().pathToSource);
        QVERIFY(!paths.at(9).split().pathToSource);
        QCOMPARE(paths.at(10).split().pathToSource, Path::fromKey(u"pippo"));
        QVERIFY(!paths.at(11).split().pathToSource);
        QVERIFY(!paths.at(12).split().pathToSource);
    }

    void withPath_data()
    {
        QTest::addColumn<Path>("lhs");
        QTest::addColumn<Path>("rhs");
        QTest::addColumn<Path>("expected");

        const Path mainPath = Path::fromKey(u"a").withKey(u"b").withKey(u"c").withKey(u"d");
        const Path slicedMainPath = mainPath.mid(1, 2);
        const Path suffixPath = Path::fromKey(u"append")
                                        .withKey(u"me")
                                        .withKey(u"to")
                                        .withField(Fields::path);
        const Path slicedSuffixPath = suffixPath.mid(1, 2);

        QTest::addRow("normal") << mainPath << suffixPath
                                << Path::fromKey(u"a")
                                           .withKey(u"b")
                                           .withKey(u"c")
                                           .withKey(u"d")
                                           .withKey(u"append")
                                           .withKey(u"me")
                                           .withKey(u"to")
                                           .withField(Fields::path);

        QTest::addRow("appendToSliced") << slicedMainPath << suffixPath
                                        << Path::fromKey(u"b")
                                                   .withKey(u"c")
                                                   .withKey(u"append")
                                                   .withKey(u"me")
                                                   .withKey(u"to")
                                                   .withField(Fields::path);

        QTest::addRow("appendSlicedToSliced")
                << slicedMainPath << slicedSuffixPath
                << Path::fromKey(u"b").withKey(u"c").withKey(u"me").withKey(u"to");

        QTest::addRow("appendTwice")
                << slicedMainPath << slicedSuffixPath.withPath(slicedSuffixPath)
                << Path::fromKey(u"b")
                           .withKey(u"c")
                           .withKey(u"me")
                           .withKey(u"to")
                           .withKey(u"me")
                           .withKey(u"to");
    }

    void withPath()
    {
        QFETCH(Path, lhs);
        QFETCH(Path, rhs);
        QFETCH(Path, expected);

        Path p = lhs.withPath(rhs);
        QCOMPARE(p, expected);
    }

    void compare_data()
    {
        QTest::addColumn<Path>("lhs");
        QTest::addColumn<Path>("rhs");
        QTest::addColumn<int>("expected");

        const Path base = Path::fromRoot(PathRoot::Top).withField(Fields::children).withIndex(0);
        const Path baseCopy =
                Path::fromRoot(PathRoot::Top).withField(Fields::children).withIndex(0);

        QTest::addRow("same") << base << base << 0;
        QTest::addRow("equalButNotShared") << base.withKey(u"x") << baseCopy.withKey(u"x") << 0;
        QTest::addRow("siblings") << base.withKey(u"a") << base.withKey(u"b") << -1;
        QTest::addRow("siblingsNotShared") << base.withKey(u"a") << baseCopy.withKey(u"b") << -1;
        QTest::addRow("prefix") << base << base.withKey(u"a") << -1;
        QTest::addRow("prefixNotShared") << base << baseCopy.withKey(u"a") << -1;
        QTest::addRow("sharedThenLonger")
                << base.withKey(u"x") << base.withKey(u"x").withKey(u"y") << -1;
        // Walking from the back finds the difference at "z"/"a" first, but
        // the one at the front decides.
        QTest::addRow("frontmostDifferenceDecides")
                << Path::fromKey(u"a").withKey(u"z") << Path::fromKey(u"b").withKey(u"a") << -1;
        QTest::addRow("differenceBeforeSharedPart")
                << Path::fromKey(u"a").withKey(u"m") << Path::fromKey(u"b").withKey(u"m") << -1;
        // The nodes hidden by dropFront() are not part of the path.
        QTest::addRow("dropFrontVsFresh")
                << base.withKey(u"x").dropFront() << Path::fromField(Fields::children)
                                                             .withIndex(0)
                                                             .withKey(u"x")
                << 0;
        QTest::addRow("midVsFresh")
                << base.withKey(u"x").mid(1, 2) << Path::fromField(Fields::children).withIndex(0)
                << 0;
        QTest::addRow("dropTailVsPrefix") << base.withKey(u"x").dropTail() << baseCopy << 0;
        // cmp() treats $env and $universe like $top.
        QTest::addRow("envIsTop") << Path::fromRoot(PathRoot::Env).withKey(u"x")
                                  << Path::fromRoot(PathRoot::Top).withKey(u"x") << 0;
        QTest::addRow("empty") << Path() << Path() << 0;
        QTest::addRow("emptyVsNonEmpty") << Path() << base << -1;
    }

    void compare()
    {
        QFETCH(Path, lhs);
        QFETCH(Path, rhs);
        QFETCH(int, expected);

        const auto sign = [](int i) { return (i > 0) - (i < 0); };
        QCOMPARE(sign(Path::cmp(lhs, rhs)), expected);
        QCOMPARE(sign(Path::cmp(rhs, lhs)), -expected);
        QCOMPARE(lhs == rhs, expected == 0);
    }

    void iterateFrontToBack()
    {
        const Path p = Path::fromRoot(PathRoot::Top).withKey(u"a").withIndex(1).withKey(u"b")
                               .dropFront();
        QStringList names;
        for (const Path &component : p) {
            QCOMPARE(component.length(), 1);
            names.append(component.headName());
        }
        QCOMPARE(names, QStringList({ QStringLiteral("a"), QStringLiteral("1"), QStringLiteral("b") }));
        QVERIFY(p.begin() == p.begin());
        QVERIFY(p.begin() != p.end());
        QVERIFY(Path().begin() == Path().end());
    }

    void releaseLongPath()
    {
        // Releasing a path must not recurse once per component.
        Path p = Path::fromRoot(PathRoot::Top);
        for (int i = 0; i < 60000; ++i)
            p = p.withIndex(i);
        const Path shorter = p.dropTail(30000);
        p = Path();
        QCOMPARE(shorter.length(), 30001);
        QCOMPARE(shorter.last().headIndex(), 29999);
    }
};

} // namespace PathEls
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
