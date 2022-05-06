/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKPOPUPANCHORS_P_H
#define QQUICKPOPUPANCHORS_P_H

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

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPopupAnchorsPrivate;
class QQuickPopup;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickPopupAnchors : public QObject, public QQuickItemChangeListener
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *centerIn READ centerIn WRITE setCenterIn RESET resetCenterIn NOTIFY centerInChanged)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 5)

public:
    explicit QQuickPopupAnchors(QQuickPopup *popup);
    ~QQuickPopupAnchors();

    QQuickItem *centerIn() const;
    void setCenterIn(QQuickItem *item);
    void resetCenterIn();

Q_SIGNALS:
    void centerInChanged();

private:
    void itemDestroyed(QQuickItem *item) override;

    Q_DISABLE_COPY(QQuickPopupAnchors)
    Q_DECLARE_PRIVATE(QQuickPopupAnchors)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPopupAnchors)

#endif // QQUICKPOPUPANCHORS_P_H
