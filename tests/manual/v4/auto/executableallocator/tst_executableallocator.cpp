// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QtCore>
#include <stdlib.h>
#include <assert.h>

#include <private/qv4executableallocator.h>

using namespace QQmlJS::VM;

class tst_ExecutableAllocator : public QObject
{
    Q_OBJECT
private slots:
    void singleAlloc();
    void mergeNext();
    void mergePrev();
    void multipleChunks();
};

void tst_ExecutableAllocator::singleAlloc()
{
    ExecutableAllocator allocator;
    ExecutableAllocator::Allocation *p = allocator.allocate(256);
    QCOMPARE(allocator.freeAllocationCount(), 1);
    QCOMPARE(allocator.chunkCount(), 1);
    allocator.free(p);
    QCOMPARE(allocator.freeAllocationCount(), 0);
    QCOMPARE(allocator.chunkCount(), 0);
}

void tst_ExecutableAllocator::mergeNext()
{
    ExecutableAllocator allocator;

    ExecutableAllocator::Allocation *first = allocator.allocate(10);
    QCOMPARE(allocator.freeAllocationCount(), 1);
    QCOMPARE(allocator.chunkCount(), 1);

    ExecutableAllocator::Allocation *second = allocator.allocate(10);
    QCOMPARE(allocator.freeAllocationCount(), 1);
    QCOMPARE(allocator.chunkCount(), 1);

    allocator.free(second);
    QCOMPARE(allocator.freeAllocationCount(), 1);
    QCOMPARE(allocator.chunkCount(), 1);

    allocator.free(first);
    QCOMPARE(allocator.freeAllocationCount(), 0);
    QCOMPARE(allocator.chunkCount(), 0);
}

void tst_ExecutableAllocator::mergePrev()
{
    ExecutableAllocator allocator;

    ExecutableAllocator::Allocation *first = allocator.allocate(10);
    QCOMPARE(allocator.freeAllocationCount(), 1);

    ExecutableAllocator::Allocation *second = allocator.allocate(10);
    QCOMPARE(allocator.freeAllocationCount(), 1);

    ExecutableAllocator::Allocation *third = allocator.allocate(10);
    QCOMPARE(allocator.freeAllocationCount(), 1);

    allocator.free(first);
    QCOMPARE(allocator.freeAllocationCount(), 2);

    allocator.free(second);
    QCOMPARE(allocator.freeAllocationCount(), 2);

    allocator.free(third);
    QCOMPARE(allocator.freeAllocationCount(), 0);
}

void tst_ExecutableAllocator::multipleChunks()
{
    ExecutableAllocator allocator;

    ExecutableAllocator::Allocation *first = allocator.allocate(10);
    QCOMPARE(allocator.chunkCount(), 1);

    ExecutableAllocator::Allocation *second = allocator.allocate(8 * 1024 * 1024);
    QCOMPARE(allocator.chunkCount(), 2);

    ExecutableAllocator::Allocation *third = allocator.allocate(1024);
    QCOMPARE(allocator.chunkCount(), 2);

    allocator.free(first);
    allocator.free(second);
    allocator.free(third);
    QCOMPARE(allocator.chunkCount(), 0);
    QCOMPARE(allocator.freeAllocationCount(), 0);
}

QTEST_MAIN(tst_ExecutableAllocator)
#include "tst_executableallocator.moc"
