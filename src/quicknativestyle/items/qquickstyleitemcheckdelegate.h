// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMCHECKDELEGATE_H
#define QQUICKSTYLEITEMCHECKDELEGATE_H

#include "qquickstyleitemcheckbox.h"

QT_BEGIN_NAMESPACE

class QQuickStyleItemCheckDelegate : public QQuickStyleItemCheckBox
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CheckDelegate)

protected:
    void connectToControl() const override;

private:
    void initStyleOption(QStyleOptionButton &styleOption) const override;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMCHECKDELEGATE_H
