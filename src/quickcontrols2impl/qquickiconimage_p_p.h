/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#ifndef QQUICKICONIMAGE_P_P_H
#define QQUICKICONIMAGE_P_P_H

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

#include <QtQuick/private/qquickimage_p_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>
#include <QtGui/private/qiconloader_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2_PRIVATE_EXPORT QQuickIconImagePrivate : public QQuickImagePrivate
{
    Q_DECLARE_PUBLIC(QQuickIconImage)

public:
    ~QQuickIconImagePrivate() override;
    void updateIcon();
    void updateFillMode();
    qreal calculateDevicePixelRatio() const;
    bool updateDevicePixelRatio(qreal targetDevicePixelRatio) override;

    QUrl source;
    QColor color = Qt::transparent;
    QThemeIconInfo icon;
    bool updatingIcon = false;
    bool isThemeIcon = false;
    bool updatingFillMode = false;
};

QT_END_NAMESPACE

#endif // QQUICKICONIMAGE_P_P_H
