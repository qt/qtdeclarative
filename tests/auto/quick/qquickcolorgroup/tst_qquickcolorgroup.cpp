/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
**
****************************************************************************/

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
    QCOMPARE(sp.count(), expectedNotificationsCount);
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

    QCOMPARE(sp.count(), 1);
}

QTEST_MAIN(tst_QQuickColorGroup)

#include "tst_qquickcolorgroup.moc"
