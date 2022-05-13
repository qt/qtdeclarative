// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMTREEINDICATOR_H
#define QQUICKSTYLEITEMTREEINDICATOR_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquicktreeviewdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemTreeIndicator : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TreeIndicator)

protected:
    void connectToControl() const override;
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionViewItem &styleOption) const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMTREEINDICATOR_H
