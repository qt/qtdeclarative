/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef LINEEXTRUDER_H
#define LINEEXTRUDER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
#include "qquickparticleextruder_p.h"

class QQuickLineExtruder : public QQuickParticleExtruder
{
    Q_OBJECT
    //Default is topleft to bottom right. Flipped makes it topright to bottom left
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY mirroredChanged)
    QML_NAMED_ELEMENT(LineShape)

public:
    explicit QQuickLineExtruder(QObject *parent = 0);
    QPointF extrude(const QRectF &) override;
    bool mirrored() const
    {
        return m_mirrored;
    }

Q_SIGNALS:

    void mirroredChanged(bool arg);

public Q_SLOTS:

    void setMirrored(bool arg)
    {
        if (m_mirrored != arg) {
            m_mirrored = arg;
            Q_EMIT mirroredChanged(arg);
        }
    }
private:
    bool m_mirrored;
};

#endif // LINEEXTRUDER_H
