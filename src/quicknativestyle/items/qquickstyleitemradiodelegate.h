// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMRADIODELEGATE_H
#define QQUICKSTYLEITEMRADIODELEGATE_H

#include "qquickstyleitemradiobutton.h"

QT_BEGIN_NAMESPACE

class QQuickStyleItemRadioDelegate : public QQuickStyleItemRadioButton
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RadioDelegate)

protected:
    void connectToControl() const override;

private:
    void initStyleOption(QStyleOptionButton &styleOption) const override;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMRADIODELEGATE_H
