// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMRADIOBUTTON_H
#define QQUICKSTYLEITEMRADIOBUTTON_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickradiobutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemRadioButton : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RadioButton)

public:
    QFont styleFont(QQuickItem *control) const override;

protected:
    void connectToControl() const override;
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionButton &styleOption) const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMRADIOBUTTON_H
