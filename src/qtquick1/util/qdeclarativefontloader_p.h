/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEFONTLOADER_H
#define QDECLARATIVEFONTLOADER_H

#include <QtDeclarative/qdeclarative.h>

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDeclarative1FontLoaderPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1FontLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1FontLoader)
    Q_ENUMS(Status)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status { Null = 0, Ready, Loading, Error };

    QDeclarative1FontLoader(QObject *parent = 0);
    ~QDeclarative1FontLoader();

    QUrl source() const;
    void setSource(const QUrl &url);

    QString name() const;
    void setName(const QString &name);

    Status status() const;

private Q_SLOTS:
    void updateFontInfo(const QString&, QDeclarative1FontLoader::Status);

Q_SIGNALS:
    void sourceChanged();
    void nameChanged();
    void statusChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1FontLoader)

QT_END_HEADER

#endif // QDECLARATIVEFONTLOADER_H

