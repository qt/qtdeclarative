/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVETEXTDOCUMENT_H
#define QDECLARATIVETEXTDOCUMENT_H

#include <qdeclarative.h>

#include <QtCore/qobject.h>
#include <QDebug>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeTextDocumentPrivate;
class QTextLayout;
class QDeclarativeTextDocument;

class Q_AUTOTEST_EXPORT QDeclarativeTextLine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)

public:
    QDeclarativeTextLine(QDeclarativeTextDocument* doc);
    qreal x() const;
    void setX(qreal x);
    qreal y() const;
    void setY(qreal y);

private:
    QDeclarativeTextDocument* _doc;
    qreal _x;
    qreal _y;
};

class Q_AUTOTEST_EXPORT QDeclarativeTextDocument : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarativeTextDocument)

    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY lineCountChanged)

public:
    QDeclarativeTextDocument(QObject *parent = 0);
    ~QDeclarativeTextDocument();

    qreal width() const;
    void setWidth(qreal);

    QString text() const;
    void setText(const QString&);

    QTextLayout *layout();

    int lineCount() const;

    Q_INVOKABLE QObject *lineAt(int i) const;

    void doLayout();

Q_SIGNALS:
    void textLayoutChanged();
    void widthChanged();
    void textChanged();
    void lineCountChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeTextDocument)

QT_END_HEADER

#endif // QDECLARATIVETEXTDOCUMENT_H
