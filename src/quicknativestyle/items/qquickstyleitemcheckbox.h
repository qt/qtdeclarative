// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMCHECKBOX_H
#define QQUICKSTYLEITEMCHECKBOX_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickcheckbox_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemCheckBox : public QQuickStyleItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CheckBox)

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

#endif // QQUICKSTYLEITEMCHECKBOX_H
