/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QML preview debug service.
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

#ifndef QQMLPREVIEWPOSITION_H
#define QQMLPREVIEWPOSITION_H

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

#include <QtCore/qvector.h>
#include <QtCore/qpoint.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qsettings.h>
#include <QtCore/qrect.h>
#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

class QWindow;

class QQmlPreviewPosition
{
public:
    class ScreenData {
    public:
        bool operator==(const QQmlPreviewPosition::ScreenData &other) const;
        QString name;
        QRect rect;
    };
    class Position {
    public:
        QString screenName;
        QPoint nativePosition;
    };
    enum InitializeState {
        InitializePosition,
        PositionInitialized
    };

    QQmlPreviewPosition();
    ~QQmlPreviewPosition();


    void takePosition(QWindow *window, InitializeState state = PositionInitialized);
    void initLastSavedWindowPosition(QWindow *window);
    void loadWindowPositionSettings(const QUrl &url);

private:
    void setPosition(const QQmlPreviewPosition::Position &position, QWindow *window);
    QByteArray fromPositionToByteArray(const Position &position);
    void readLastPositionFromByteArray(const QByteArray &array);
    void saveWindowPosition();

    bool m_hasPosition = false;
    InitializeState m_initializeState = InitializePosition;
    QSettings m_settings;
    QString m_settingsKey;
    QTimer m_savePositionTimer;
    Position m_lastWindowPosition;
    QVector<QWindow *> m_positionedWindows;

    QVector<ScreenData> m_currentInitScreensData;
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWPOSITION_H
