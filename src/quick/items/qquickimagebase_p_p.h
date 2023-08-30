// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKIMAGEBASE_P_P_H
#define QQUICKIMAGEBASE_P_P_H

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

#include "qquickimplicitsizeitem_p_p.h"
#include "qquickimagebase_p.h"

#include <QtQuick/private/qquickpixmap_p.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;
class Q_QUICK_PRIVATE_EXPORT QQuickImageBasePrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickImageBase)

public:
    QQuickImageBasePrivate()
      : async(false),
        cache(true),
        mirrorHorizontally(false),
        mirrorVertically(false),
        oldAutoTransform(false)
    {
    }

    virtual bool updateDevicePixelRatio(qreal targetDevicePixelRatio);

    void setStatus(QQuickImageBase::Status value);
    void setProgress(qreal value);

    QUrl url;
    QQuickPixmap pix;
    QSize sourcesize;
    QSize oldSourceSize;
    QRectF sourceClipRect;
    QQuickImageProviderOptions providerOptions;
    QColorSpace colorSpace;

    int currentFrame = 0;
    int frameCount = 0;
    qreal progress = 0;
    qreal devicePixelRatio = 1;
    QQuickImageBase::Status status = QQuickImageBase::Null;

    bool async : 1;
    bool cache : 1;
    bool mirrorHorizontally: 1;
    bool mirrorVertically : 1;
    bool oldAutoTransform : 1;
};

QT_END_NAMESPACE

#endif // QQUICKIMAGEBASE_P_P_H
