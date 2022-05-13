// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMFRAME_H
#define QQUICKSTYLEITEMFRAME_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickframe_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemFrame : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Frame)

protected:
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionFrame &styleOption) const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMFRAME_H
