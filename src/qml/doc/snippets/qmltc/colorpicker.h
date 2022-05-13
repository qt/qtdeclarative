// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>
#include <QtGui/qcolor.h>

class MyColorPicker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // Stores a value in the range [0, 1); myApp.qml type sets this with
    // Math.random()
    Q_PROPERTY(double encodedColor READ encodedColor WRITE setEncodedColor BINDABLE bindableEncodedColor)

    QProperty<double> m_encodedColor{0.5};
public:
    MyColorPicker(QObject *parent = nullptr) : QObject(parent) {}

    double encodedColor() { return m_encodedColor; }
    void setEncodedColor(double value);
    QBindable<double> bindableEncodedColor();

    // Returns a QColor "decoded" from encodedColor
    Q_INVOKABLE QColor decodeColor();
};
