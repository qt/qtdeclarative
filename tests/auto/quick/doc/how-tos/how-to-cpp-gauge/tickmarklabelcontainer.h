// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TICKMARKLABELCONTAINER_H
#define TICKMARKLABELCONTAINER_H

#include <QFont>
#include <QQmlComponent>
#include <QQuickItem>

#include "tickmarkcontainer.h"

QT_BEGIN_NAMESPACE

class TickmarkLabelContainer : public TickmarkContainer
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged REQUIRED FINAL)

    QML_ELEMENT

public:
    TickmarkLabelContainer(QQuickItem *parent = nullptr);
    virtual ~TickmarkLabelContainer() = default;

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

Q_SIGNALS:
    void fontChanged();

protected:
    void updatePolish() override;

private:
    Q_DISABLE_COPY(TickmarkLabelContainer)

    void recreateDelegateItems() override;

    // Not used by us other than to detect when the labels
    // should be repositioned due to implicit size changes.
    // Assuming Label is used as the delegate, Control's font
    // propagation already ensures that the correct font is used.
    QFont mFont;
};

QT_END_NAMESPACE

#endif // TICKMARKLABELCONTAINER_H
