// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMBUTTON_H
#define QQUICKSTYLEITEMBUTTON_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemButton : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Button)

public:
    QFont styleFont(QQuickItem *control) const override;

protected:
    void connectToControl() const override;
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    virtual void initStyleOption(QStyleOptionButton &styleOption) const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMBUTTON_H
