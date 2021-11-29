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

#ifndef QQUICKHOVERHANDLER_H
#define QQUICKHOVERHANDLER_H

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

#include "qquickitem.h"
#include "qevent.h"
#include "qquicksinglepointhandler_p.h"
#include <QtCore/qbasictimer.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickHoverHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    QML_NAMED_ELEMENT(HoverHandler)
    QML_ADDED_IN_MINOR_VERSION(12)

public:
    explicit QQuickHoverHandler(QQuickItem *parent = nullptr);
    ~QQuickHoverHandler();

    bool isHovered() const { return m_hovered; }

Q_SIGNALS:
    void hoveredChanged();

protected:
    void componentComplete() override;
    bool wantsPointerEvent(QQuickPointerEvent *event) override;
    void handleEventPoint(QQuickEventPoint *point) override;
    void onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabTransition transition, QQuickEventPoint *point) override;

private:
    void setHovered(bool hovered);

private:
    bool m_hovered = false;
    bool m_hoveredTablet = false;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickHoverHandler)

#endif // QQUICKHOVERHANDLER_H
