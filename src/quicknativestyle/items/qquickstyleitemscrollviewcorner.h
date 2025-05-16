// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQUICKSTYLEITEMSCROLLVIEWCORNER_H
#define QQUICKSTYLEITEMSCROLLVIEWCORNER_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickscrollbar_p.h>

class QQuickStyleItemScrollViewCorner : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ScrollViewCorner)

protected:
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionSlider &styleOption) const;
};

#endif // QQUICKSTYLEITEMSCROLLVIEWCORNER_H
