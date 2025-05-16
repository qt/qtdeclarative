// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQUICKSTYLEITEMDELAYBUTTON_H
#define QQUICKSTYLEITEMDELAYBUTTON_H

#include "qquickstyleitembutton.h"

QT_BEGIN_NAMESPACE

class QQuickStyleItemDelayButton : public QQuickStyleItemButton
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DelayButton)

protected:
    void connectToControl() const override;

private:
    void initStyleOption(QStyleOptionButton &styleOption) const override;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMDELAYBUTTON_H
