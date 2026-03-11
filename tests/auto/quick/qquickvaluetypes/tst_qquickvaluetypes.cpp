// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qquickvaluetypes_p.h>

#include <QtTest/qtest.h>

#include <QtGui/qcolor.h>
#include <QtGui/qfont.h>
#include <QtGui/qquaternion.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

// Exercise the value type transformation that qmlsc does in direct mode
// when converting a constructible value type wrapper (e.g. QQuickColorValueType)
// to its underlying type (e.g. QColor).
template<typename Underlying, typename ValueType, typename Arg>
Underlying valueTypeTransform(Arg &&arg)
{
    return [](auto &&arg) {
        ValueType wrapper(std::forward<decltype(arg)>(arg));
        return Underlying(reinterpret_cast<Underlying &>(wrapper));
    }(std::forward<Arg>(arg));
}

class tst_qquickvaluetypes : public QObject
{
    Q_OBJECT

private slots:
    void colorFromString();
    void colorFromColor();
    void vector2dFromVector2d();
    void vector3dFromVector3d();
    void vector4dFromVector4d();
    void quaternionFromQuaternion();
    void fontFromFont();
};

void tst_qquickvaluetypes::colorFromString()
{
    const QColor result =
            valueTypeTransform<QColor, QQuickColorValueType>(QStringLiteral("#dddddd"));
    QCOMPARE(result, QColor("#dddddd"));

    const QColor red =
            valueTypeTransform<QColor, QQuickColorValueType>(QStringLiteral("red"));
    QCOMPARE(red, QColor(Qt::red));
}

void tst_qquickvaluetypes::colorFromColor()
{
    const QColor input(64, 128, 192, 255);
    const QColor result = valueTypeTransform<QColor, QQuickColorValueType>(input);
    QCOMPARE(result, input);
}

void tst_qquickvaluetypes::vector2dFromVector2d()
{
    const QVector2D input(1.5f, 2.5f);
    const QVector2D result = valueTypeTransform<QVector2D, QQuickVector2DValueType>(input);
    QCOMPARE(result, input);
}

void tst_qquickvaluetypes::vector3dFromVector3d()
{
    const QVector3D input(1.0f, 2.0f, 3.0f);
    const QVector3D result = valueTypeTransform<QVector3D, QQuickVector3DValueType>(input);
    QCOMPARE(result, input);
}

void tst_qquickvaluetypes::vector4dFromVector4d()
{
    const QVector4D input(1.0f, 2.0f, 3.0f, 4.0f);
    const QVector4D result = valueTypeTransform<QVector4D, QQuickVector4DValueType>(input);
    QCOMPARE(result, input);
}

void tst_qquickvaluetypes::quaternionFromQuaternion()
{
    const QQuaternion input(1.0f, 0.5f, 0.25f, 0.125f);
    const QQuaternion result = valueTypeTransform<QQuaternion, QQuickQuaternionValueType>(input);
    QCOMPARE(result, input);
}

void tst_qquickvaluetypes::fontFromFont()
{
    QFont input;
    input.setFamily(QStringLiteral("Arial"));
    input.setPixelSize(16);
    input.setBold(true);
    const QFont result = valueTypeTransform<QFont, QQuickFontValueType>(input);
    QCOMPARE(result, input);
}

QTEST_MAIN(tst_qquickvaluetypes)

#include "tst_qquickvaluetypes.moc"
