/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#ifndef QQUICKLABSPLATFORMICON_P_H
#define QQUICKLABSPLATFORMICON_P_H

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

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QObject;

class QQuickLabsPlatformIcon
{
    Q_GADGET
    Q_PROPERTY(QUrl source READ source WRITE setSource)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(bool mask READ isMask WRITE setMask)

public:
    QUrl source() const;
    void setSource(const QUrl &source);

    QString name() const;
    void setName(const QString &name);

    bool isMask() const;
    void setMask(bool mask);

    bool operator==(const QQuickLabsPlatformIcon &other) const;
    bool operator!=(const QQuickLabsPlatformIcon &other) const;

private:
    bool m_mask = false;
    QUrl m_source;
    QString m_name;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMICON_P_H
