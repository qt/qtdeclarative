// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCLASSICPREVIEWHANDLER_H
#define QQMLCLASSICPREVIEWHANDLER_H

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

#include <QtCore/qpointer.h>
#include <QtQml/qqmlcomponent.h>

QT_BEGIN_NAMESPACE

class QQuickItem;

class QQmlClassicPreviewHandler : public QQmlPreviewHandler
{
    Q_OBJECT
public:
    explicit QQmlClassicPreviewHandler(QObject *parent = nullptr);
    ~QQmlClassicPreviewHandler() override;

    void removeEngine(QQmlEngine *engine) final;
    void connectToService(QQmlPreviewServiceImpl *service) final;

    void load(const QUrl &url) final;

private:
    void dropCU(const QUrl &url);
    void rerun();
    void clear();

    void zoom(qreal newFactor);

    void beforeSynchronizing();
    void afterSynchronizing();
    void beforeRendering();
    void frameSwapped();

    void tryCreateObject();
    void showObject(QObject *object);

    bool eventFilter(QObject *obj, QEvent *event) final;

    QScopedPointer<QQuickItem> m_dummyItem;
    QList<QPointer<QObject>> m_createdObjects;
    QScopedPointer<QQmlComponent> m_component;

    qreal m_zoomFactor = 1.0;
    bool m_supportsMultipleWindows;
    QQmlPreviewPosition m_lastPosition;
};

QT_END_NAMESPACE

#endif // QQMLCLASSICPREVIEWHANDLER_H
