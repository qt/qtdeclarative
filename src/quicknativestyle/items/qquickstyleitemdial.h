// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMDIAL_H
#define QQUICKSTYLEITEMDIAL_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickdial_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemDial : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Dial)

public:
    QFont styleFont(QQuickItem *control) const override;

protected:
    void connectToControl() const override;
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionSlider &styleOption) const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMDIAL_H
