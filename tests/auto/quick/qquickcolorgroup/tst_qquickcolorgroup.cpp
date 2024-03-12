// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/QSignalSpy>

#include <QtGui/QPalette>

#include <QtQuick/private/qquickcolorgroup_p.h>

class tst_QQuickColorGroup : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void checkColorProperty();
    void checkColorProperty_data();

    void colorGroupChangedWhenColorChanged();
};

void tst_QQuickColorGroup::checkColorProperty()
{
    QFETCH(int, propertyIndex);

    auto property = QQuickColorGroup::staticMetaObject.property(propertyIndex);

    QVERIFY(property.isReadable());
    QVERIFY(property.isWritable());
    QVERIFY(property.isResettable());
    QVERIFY(property.hasNotifySignal());

    const QQuickColorGroup defaultGroup;
    QQuickColorGroup group;

    auto notifierSignature = QString::number(QSIGNAL_CODE) + property.notifySignal().methodSignature();
    QSignalSpy sp(&group, notifierSignature.toUtf8());

    QVERIFY(property.write(&group, QColor(Qt::red)));

    QCOMPARE(qvariant_cast<QColor>(property.read(&group)), QColor(Qt::red));

    QVERIFY(property.reset(&group));

    QCOMPARE(qvariant_cast<QColor>(property.read(&group)),
             qvariant_cast<QColor>(property.read(&defaultGroup)));

    constexpr int expectedNotificationsCount = 2; // One from write + one from reset
    QCOMPARE(sp.size(), expectedNotificationsCount);
}

void tst_QQuickColorGroup::checkColorProperty_data()
{
    QTest::addColumn<int>("propertyIndex");

    auto mo = QQuickColorGroup::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        auto property = mo.property(i);
        if (property.userType() == QMetaType::QColor) {
            QTest::addRow("%s", property.name()) << i;
        }
    }
}

void tst_QQuickColorGroup::colorGroupChangedWhenColorChanged()
{
    QQuickColorGroup group;
    group.setGroupTag(QPalette::Active);

    QSignalSpy sp(&group, &QQuickColorGroup::changed);

    QVERIFY(group.mid() != Qt::blue);

    group.setMid(Qt::blue);

    QCOMPARE(sp.size(), 1);
}

QTEST_MAIN(tst_QQuickColorGroup)

#include "tst_qquickcolorgroup.moc"
