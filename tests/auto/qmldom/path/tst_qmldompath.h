// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLDOMPATH_H
#define TST_QMLDOMPATH_H
#include <QtQmlDom/private/qqmldompath_p.h>
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

        Path p11 = Path::fromField(u"test");
        QString s = QLatin1String("test");
        Path p2 = Path::fromField(s);
        Path p3 = Path::fromField(QLatin1String("test"));
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
        auto c8_1=PathComponent(Root(PathRoot::Env));
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
        auto p2 = p1.withField(u"aa");
        QCOMPARE(p2[2].headKind(), Kind::Field);
        auto p2b = p1.withComponent(PathEls::Field(u"aa"));
        QCOMPARE(p2b.length(), 3);
        QCOMPARE(p2b[2].headKind(), Kind::Field);
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
        auto rString = u"$env.@obj.aa[4][\"bla\"].";
        QCOMPARE(p6.toString(), rString);
        auto p8 = p6.dropTail();
        QCOMPARE(p8.length(), 5);
        auto p9 = Path::fromString(rString);
        QCOMPARE(p9.length(), 6);
        auto p9Str = p9.toString();
        QCOMPARE(p9Str, rString);
        QCOMPARE(p9, p6);
        auto p10 = p6.dropFront();
        QCOMPARE(p10.length(), 5);
        auto p10Str = p10.toString();
        auto r2Str = u"@obj.aa[4][\"bla\"].";
        QCOMPARE(p10Str, r2Str);
        auto p11 = Path::fromString(r2Str);
        auto p11Str = p11.toString();
        QCOMPARE(p11Str, r2Str);
        QCOMPARE(p10, p11);
    }

    void testPathSplit()
    {
        const QList<Path> paths({Path(),
            Path::fromRoot(PathRoot::Env).withField(u"pippo").withKey(u"pluto").withIndex(4),
            Path::fromRoot(PathRoot::Env).withField(u"pippo").withKey(u"pluto"),
            Path::fromRoot(PathRoot::Env).withField(u"pippo"),
            Path::fromRoot(PathRoot::Env).withField(u"pippo").withField(u"pp"),
            Path::fromRoot(PathRoot::Env),
            Path::fromField(u"pippo").withIndex(4),
            Path::fromField(u"pippo").withKey(u"pluto").withIndex(4),
            Path::fromField(u"pippo").withKey(u"pluto"),
            Path::fromField(u"pippo"),
            Path::fromField(u"pippo").withField(u"pp"),
            Path::fromIndex(4),
            Path::fromKey(u"zz")
            });
        for (const Path &p : paths) {
            Source s = p.split();
            QCOMPARE(p, s.pathToSource.withPath(s.pathFromSource));
            if (!s.pathFromSource)
                QVERIFY(!s.pathToSource);
        }
        QCOMPARE(paths.at(1).split().pathToSource, Path::fromRoot(PathRoot::Env));
        QCOMPARE(paths.at(2).split().pathToSource, Path::fromRoot(PathRoot::Env));
        QCOMPARE(paths.at(3).split().pathToSource, Path::fromRoot(PathRoot::Env));
        QCOMPARE(paths.at(4).split().pathToSource, Path::fromRoot(PathRoot::Env).withField(u"pippo"));
        QVERIFY(!paths.at(5).split().pathToSource);
        QVERIFY(!paths.at(6).split().pathToSource);
        QVERIFY(!paths.at(7).split().pathToSource);
        QVERIFY(!paths.at(8).split().pathToSource);
        QVERIFY(!paths.at(9).split().pathToSource);
        QCOMPARE(paths.at(10).split().pathToSource, Path::fromField(u"pippo"));
        QVERIFY(!paths.at(11).split().pathToSource);
        QVERIFY(!paths.at(12).split().pathToSource);
    }

    void withPath_data()
    {
        QTest::addColumn<Path>("lhs");
        QTest::addColumn<Path>("rhs");
        QTest::addColumn<Path>("expected");

        const Path mainPath = Path::fromField("a").withField("b").withField("c").withField("d");
        const Path slicedMainPath = mainPath.mid(1, 2);
        const Path suffixPath = Path::fromField("append")
                                        .withField(QLatin1String("me"))
                                        .withField(QString(u"to"))
                                        .withField("path");
        const Path slicedSuffixPath = suffixPath.mid(1, 2);

        QTest::addRow("normal") << mainPath << suffixPath
                                << Path::fromField(u"a")
                                           .withField(u"b")
                                           .withField(u"c")
                                           .withField(u"d")
                                           .withField(u"append")
                                           .withField(u"me")
                                           .withField(u"to")
                                           .withField(u"path");

        QTest::addRow("appendToSliced") << slicedMainPath << suffixPath
                                        << Path::fromField(u"b")
                                                   .withField(u"c")
                                                   .withField(u"append")
                                                   .withField(u"me")
                                                   .withField(u"to")
                                                   .withField(u"path");

        QTest::addRow("appendSlicedToSliced")
                << slicedMainPath << slicedSuffixPath
                << Path::fromField(u"b").withField(u"c").withField(u"me").withField(u"to");

        QTest::addRow("appendTwice")
                << slicedMainPath << slicedSuffixPath.withPath(slicedSuffixPath)
                << Path::fromField(u"b")
                           .withField(u"c")
                           .withField(u"me")
                           .withField(u"to")
                           .withField(u"me")
                           .withField(u"to");
    }

    void withPath()
    {
        QFETCH(Path, lhs);
        QFETCH(Path, rhs);
        QFETCH(Path, expected);

        Path p = lhs.withPath(rhs);
        QCOMPARE(p, expected);

        // sanity check: make sure that the newly appended components do have QStringViews pointing
        // to the components own strData
        for (int i = 0, end = rhs.length(); i < end; ++i) {
            QCOMPARE(std::get<Field>(p.m_data->components[i].m_data).fieldName,
                     p.m_data->strData[end - 1 - i]);
            QCOMPARE(std::get<Field>(p.m_data->components[i].m_data).fieldName.constData(),
                     p.m_data->strData[end - 1 - i].constData());
        }
    }
};

} // namespace PathEls
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
