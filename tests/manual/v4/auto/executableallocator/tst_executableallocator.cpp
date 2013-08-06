/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
