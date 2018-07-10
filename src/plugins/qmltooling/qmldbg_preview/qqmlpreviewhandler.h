/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QML preview debug service.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLPREVIEWHANDLER_H
#define QQMLPREVIEWHANDLER_H

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

#include "qqmlpreviewposition.h"

#include <QtCore/qobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qrect.h>
#include <QtCore/qpointer.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQuickItem;
class QQmlPreviewUrlInterceptor;
class QQuickWindow;
class QTranslator;

class QQmlPreviewHandler : public QObject
{
    Q_OBJECT
public:
    explicit QQmlPreviewHandler(QObject *parent = nullptr);
    ~QQmlPreviewHandler();

    void addEngine(QQmlEngine *engine);
    void removeEngine(QQmlEngine *engine);

    void loadUrl(const QUrl &url);
    void rerun();
    void zoom(qreal newFactor);
    void language(const QUrl &context, const QString &locale);

    void clear();

signals:
    void error(const QString &message);
    void fps(quint16 frames);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
private:
    void tryCreateObject();
    void showObject(QObject *object);
    void setCurrentWindow(QQuickWindow *window);
    void frameSwapped();
    void fpsTimerHit();
    void removeTranslators();

    QScopedPointer<QQuickItem> m_dummyItem;
    QList<QQmlEngine *> m_engines;
    QVector<QPointer<QObject>> m_createdObjects;
    QScopedPointer<QQmlComponent> m_component;
    QPointer<QQuickWindow> m_currentWindow;
    bool m_supportsMultipleWindows;
    QQmlPreviewPosition m_lastPosition;

    QTimer m_fpsTimer;
    quint16 m_frames = 0;

    QScopedPointer<QTranslator> m_qtTranslator;
    QScopedPointer<QTranslator> m_qmlTranslator;
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWHANDLER_H
