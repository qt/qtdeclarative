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

#include <qdeclarativetextdocument_p.h>
#include <qdeclarativestyledtext_p.h>
#include <QApplication>
#include <QTextLayout>
#include <QList>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

QDeclarativeTextLine::QDeclarativeTextLine(QDeclarativeTextDocument* doc)
    : QObject(0)
{
    _doc = doc;
    _x = 0;
    _y = 0;
}

qreal QDeclarativeTextLine::x() const
{
    return _x;
}

void QDeclarativeTextLine::setX(qreal x)
{
    _x = x;
    _doc->doLayout();
}

qreal QDeclarativeTextLine::y() const
{
    return _y;
}

void QDeclarativeTextLine::setY(qreal y)
{
    _y = y;
    _doc->doLayout();
}

class QDeclarativeTextDocumentPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeTextDocument)

public:
    QString text;
    qreal width;
    QTextLayout layout;
    QList<QDeclarativeTextLine*> lines;
};

QDeclarativeTextDocument::QDeclarativeTextDocument(QObject *parent)
    : QObject(*(new QDeclarativeTextDocumentPrivate), parent)
{
    Q_D(QDeclarativeTextDocument);
    d->text = QString();
    d->width = 0.0;
}

QDeclarativeTextDocument::~QDeclarativeTextDocument()
{
}

qreal QDeclarativeTextDocument::width() const
{
    Q_D(const QDeclarativeTextDocument);
    return d->width;
}

void QDeclarativeTextDocument::setWidth(qreal w)
{
    Q_D(QDeclarativeTextDocument);

    if (w != d->width) {
        d->width = w;
        emit widthChanged();
    }
}

QString QDeclarativeTextDocument::text() const
{
    Q_D(const QDeclarativeTextDocument);
    return d->text;
}

void QDeclarativeTextDocument::setText(const QString& text)
{
    Q_D(QDeclarativeTextDocument);

    if (text != d->text) {
        d->text = text;
//        d->layout.setText(text);
        QDeclarativeStyledText::parse(text, d->layout);
        emit textChanged();
        doLayout();
    }
}

QObject *QDeclarativeTextDocument::lineAt(int i) const
{
    Q_D(const QDeclarativeTextDocument);
    return d->lines.at(i);
}

void QDeclarativeTextDocument::doLayout()
{
    Q_D(QDeclarativeTextDocument);
    QTextOption textOption = d->layout.textOption();
    textOption.setAlignment(Qt::AlignJustify);
    d->layout.setTextOption(textOption);

    qreal height = 0;
    d->layout.clearLayout();
    d->layout.beginLayout();

    int i = 0;
    int l = 0;

    bool firstPass = d->lines.isEmpty();

    forever {
        QTextLine line = d->layout.createLine();

        if (!line.isValid())
            break;
        if (d->width)
            line.setLineWidth(d->width);

        if (!firstPass) {
            QDeclarativeTextLine *tl = d->lines.at(l);
            line.setPosition(QPointF(line.position().x() + tl->x(), height * tl->y() / 100));
        } else {
            line.setPosition(QPointF(line.position().x(), height));
            QDeclarativeTextLine *tline = new QDeclarativeTextLine(this);
            d->lines << tline;
        }
        height += line.height();
        i += line.textLength();
        ++l;
    }
    d->layout.endLayout();
    emit textLayoutChanged();
}

int QDeclarativeTextDocument::lineCount() const
{
    Q_D(const QDeclarativeTextDocument);
    return d->lines.count();
}

QTextLayout *QDeclarativeTextDocument::layout()
{
    Q_D(QDeclarativeTextDocument);
    return &d->layout;
}

QT_END_NAMESPACE
