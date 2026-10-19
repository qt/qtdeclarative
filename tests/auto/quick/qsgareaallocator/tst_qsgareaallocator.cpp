// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qrandom.h>
#include <QtCore/qrect.h>
#include <QtQuick/private/qsgareaallocator_p.h>

class tst_QSGAreaAllocator : public QObject
{
    Q_OBJECT

private slots:
    void allocationsStayWithinSlice_data();
    void allocationsStayWithinSlice();
    void allocationsStayWithinSliceAfterDeallocation();

private:
    static void verifyAllocations(const QList<QRect> &allocations, const QSize &size, int sliceHeight);
};

void tst_QSGAreaAllocator::verifyAllocations(const QList<QRect> &allocations,
                                             const QSize &size,
                                             int sliceHeight)
{
    const QRect bounds(QPoint(0, 0), size);
    for (qsizetype i = 0; i < allocations.size(); ++i) {
        const QRect &rect = allocations.at(i);
        QVERIFY2(bounds.contains(rect),
                 qPrintable(QStringLiteral("Allocation %1 is out of bounds").arg(i)));
        QVERIFY2(rect.top() / sliceHeight == rect.bottom() / sliceHeight,
                 qPrintable(QStringLiteral("Allocation %1 (y=%2, height=%3) crosses slice boundary")
                            .arg(i).arg(rect.y()).arg(rect.height())));
        for (qsizetype j = 0; j < i; ++j) {
            QVERIFY2(!rect.intersects(allocations.at(j)),
                     qPrintable(QStringLiteral("Allocations %1 and %2 overlap").arg(i).arg(j)));
        }
    }
}

void tst_QSGAreaAllocator::allocationsStayWithinSlice_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("sliceHeight");
    QTest::addColumn<QList<QSize>>("requests");
    QTest::addColumn<QList<QPoint>>("expectedPositions");

    // Full-width rows: the fourth row does not fit at the bottom of the first slice
    // and has to be moved to the start of the second one.
    QTest::newRow("rows")
            << QSize(64, 192) << 64
            << QList<QSize>{ QSize(64, 20), QSize(64, 20), QSize(64, 20), QSize(64, 20) }
            << QList<QPoint>{ QPoint(0, 0), QPoint(0, 20), QPoint(0, 40), QPoint(0, 64) };

    // Allocation which exactly fills the first slice, followed by one which has
    // to start at the next slice.
    QTest::newRow("exactFit")
            << QSize(64, 128) << 64
            << QList<QSize>{ QSize(64, 64), QSize(64, 10) }
            << QList<QPoint>{ QPoint(0, 0), QPoint(0, 64) };

    // A single allocation which is taller than what remains of the first slice
    QTest::newRow("tallAfterShort")
            << QSize(64, 128) << 64
            << QList<QSize>{ QSize(64, 50), QSize(64, 30) }
            << QList<QPoint>{ QPoint(0, 0), QPoint(0, 64) };
}

void tst_QSGAreaAllocator::allocationsStayWithinSlice()
{
    QFETCH(QSize, size);
    QFETCH(int, sliceHeight);
    QFETCH(QList<QSize>, requests);
    QFETCH(QList<QPoint>, expectedPositions);

    QSGAreaAllocator allocator(size, sliceHeight);

    QList<QRect> allocations;
    for (const QSize &request : std::as_const(requests)) {
        const QRect rect = allocator.allocate(request);
        QVERIFY(rect.isValid());
        QCOMPARE(rect.size(), request);
        allocations.append(rect);
    }

    verifyAllocations(allocations, size, sliceHeight);
    if (QTest::currentTestFailed())
        return;

    for (qsizetype i = 0; i < allocations.size(); ++i)
        QCOMPARE(allocations.at(i).topLeft(), expectedPositions.at(i));
}

void tst_QSGAreaAllocator::allocationsStayWithinSliceAfterDeallocation()
{
    const QSize size(256, 256 * 4);
    const int sliceHeight = 256;

    QSGAreaAllocator allocator(size, sliceHeight);
    QRandomGenerator rng(0x51ce);

    // Fill the allocator with randomly sized rects
    QList<QRect> allocations;
    for (int i = 0; i < 1000; ++i) {
        const QSize request(rng.bounded(4, 48), rng.bounded(4, 48));
        const QRect rect = allocator.allocate(request);
        if (rect.isValid())
            allocations.append(rect);
    }
    QVERIFY(!allocations.isEmpty());
    verifyAllocations(allocations, size, sliceHeight);
    if (QTest::currentTestFailed())
        return;

    // Free every other allocation, so that free nodes are merged, and allocate again
    QList<QRect> remaining;
    for (qsizetype i = 0; i < allocations.size(); ++i) {
        if (i % 2 == 0)
            QVERIFY(allocator.deallocate(allocations.at(i)));
        else
            remaining.append(allocations.at(i));
    }

    for (int i = 0; i < 1000; ++i) {
        const QSize request(rng.bounded(4, 64), rng.bounded(4, 64));
        const QRect rect = allocator.allocate(request);
        if (rect.isValid())
            remaining.append(rect);
    }
    verifyAllocations(remaining, size, sliceHeight);
}

QTEST_APPLESS_MAIN(tst_QSGAreaAllocator)

#include "tst_qsgareaallocator.moc"
