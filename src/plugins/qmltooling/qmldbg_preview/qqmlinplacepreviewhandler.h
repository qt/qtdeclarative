// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINPLACEPREVIEWHANDLER_H
#define QQMLINPLACEPREVIEWHANDLER_H

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

#include "qqmlpreviewhandler.h"

QT_BEGIN_NAMESPACE

class QQmlInPlacePreviewHandler : public QQmlPreviewHandler
{
    Q_OBJECT
public:
    explicit QQmlInPlacePreviewHandler(QObject *parent = nullptr);
    ~QQmlInPlacePreviewHandler() override;

    void connectToService(QQmlPreviewServiceImpl *service) final;
    void load(const QUrl &url) final;

Q_SIGNALS:
    void hotReloadFailure(const QString &reason);

private:
    QList<QUrl> m_droppedUrls;
};

QT_END_NAMESPACE

#endif // QQMLINPLACEPREVIEWHANDLER_H
