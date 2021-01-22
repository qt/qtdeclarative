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

#ifndef QQUICKFONTLOADER_H
#define QQUICKFONTLOADER_H

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

#include <qqml.h>

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QQuickFontLoaderPrivate;
class Q_AUTOTEST_EXPORT QQuickFontLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickFontLoader)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    QML_NAMED_ELEMENT(FontLoader)

public:
    enum Status { Null = 0, Ready, Loading, Error };
    Q_ENUM(Status)

    QQuickFontLoader(QObject *parent = nullptr);
    ~QQuickFontLoader();

    QUrl source() const;
    void setSource(const QUrl &url);

    QString name() const;
    void setName(const QString &name);

    Status status() const;

private Q_SLOTS:
    void updateFontInfo(const QString&, QQuickFontLoader::Status);

Q_SIGNALS:
    void sourceChanged();
    void nameChanged();
    void statusChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFontLoader)

#endif // QQUICKFONTLOADER_H

