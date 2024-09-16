// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <private/qv4estable_p.h>

class tst_qv4estable : public QObject
{
    Q_OBJECT

private slots:
    void checkRemoveAvoidsHeapBufferOverflow();
};

// QTBUG-123999
void tst_qv4estable::checkRemoveAvoidsHeapBufferOverflow()
{
    QV4::ESTable estable;

    // Fill the ESTable with values so it is at max capacity.
    QCOMPARE_EQ(estable.m_capacity, 8U);
    for (uint i = 0; i < estable.m_capacity; ++i) {
        estable.set(QV4::Value::fromUInt32(i), QV4::Value::fromUInt32(i));
    }
    // Our |m_keys| array should now contain eight values.
    // > [v0, v1, v2, v3, v4, v5, v6, v7]
    for (uint i = 0; i < estable.m_capacity; ++i) {
        QVERIFY(estable.m_keys[i].sameValueZero(QV4::Value::fromUInt32(i)));
    }
    QCOMPARE_EQ(estable.m_capacity, 8U);
    QCOMPARE_EQ(estable.m_size, 8U);

    // Remove the first item from the set to verify that asan does not trip.
    // Relies on the CI platform propagating asan flag to all tests.
    estable.remove(QV4::Value::fromUInt32(0));
}

QTEST_MAIN(tst_qv4estable)

#include "tst_qv4estable.moc"
