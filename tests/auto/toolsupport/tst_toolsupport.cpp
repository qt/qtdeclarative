// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

// Don't do this at home. This is test code, not production.
#define protected public
#define private public

#include <private/qobject_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4string_p.h>
#include <private/qqmlrefcount_p.h>
#include <qobject.h>

#if defined(Q_CC_GNU) || defined(Q_CC_MSVC)
#define RUN_MEMBER_OFFSET_TEST 1
#else
#define RUN_MEMBER_OFFSET_TEST 0
#endif

#if RUN_MEMBER_OFFSET_TEST
template <typename T, typename K>
size_t pmm_to_offsetof(T K:: *pmm)
{
#ifdef Q_CC_MSVC
    // Even on 64 bit MSVC uses 4 byte offsets.
    quint32 ret;
#else
    size_t ret;
#endif
    Q_STATIC_ASSERT(sizeof(ret) == sizeof(pmm));
    memcpy(&ret, &pmm, sizeof(ret));
    return ret;
}
#endif

class tst_toolsupport : public QObject
{
    Q_OBJECT

private slots:
    void offsets();
    void offsets_data();
};

void tst_toolsupport::offsets()
{
    QFETCH(size_t, actual);
    QFETCH(int, expected32);
    QFETCH(int, expected64);
    size_t expect = sizeof(void *) == 4 ? expected32 : expected64;
    QCOMPARE(actual, expect);
}

void tst_toolsupport::offsets_data()
{
    QTest::addColumn<size_t>("actual");
    QTest::addColumn<int>("expected32");
    QTest::addColumn<int>("expected64");

    {
        QTestData &data = QTest::newRow("sizeof(QObjectData)")
                << sizeof(QObjectData);
        data << 44 << 80;
    }

    {
        QTestData &data = QTest::newRow("sizeof(QQmlRefCount)")
                << sizeof(QQmlRefCount);
        data << 8 << 16;
    }

#if RUN_MEMBER_OFFSET_TEST
    {
        QTestData &data
            = QTest::newRow("CompiledData::CompilationUnit::data")
            << pmm_to_offsetof(&QV4::CompiledData::CompilationUnit::data);

        data << 20 << 40;
    }

    {
        QTestData &data
            = QTest::newRow("CompiledData::CompilationUnit::runtimeStrings")
            << pmm_to_offsetof(&QV4::CompiledData::CompilationUnit::runtimeStrings);

        data << 0 << 0;
    }

    {
        QTestData &data
            = QTest::newRow("Heap::String::textStorage")
            << pmm_to_offsetof(&QV4::Heap::String::textStorage);

        data << 4 << 8;
    }

#endif // RUN_MEMBER_OFFSET_TEST
}


QTEST_APPLESS_MAIN(tst_toolsupport);

#include "tst_toolsupport.moc"

