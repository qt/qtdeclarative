// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPINNER_H
#define SPINNER_H

#include <QtQuick/QQuickItem>

class Spinner : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool spinning READ spinning WRITE setSpinning NOTIFY spinningChanged)
    QML_ELEMENT

public:
    Spinner();

    bool spinning() const { return m_spinning; }
    void setSpinning(bool spinning);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

signals:
    void spinningChanged();

private:
    bool m_spinning;
};

#endif // SQUIRCLE_H
