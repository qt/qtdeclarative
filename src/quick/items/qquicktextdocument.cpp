/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktextdocument.h"

#include "qquicktextedit_p.h"
#include "qquicktextedit_p_p.h"
#include "qquicktext_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQuickTextDocument
    \since 5.1
    \brief The QQuickTextDocument class provides access to the QTextDocument of QQuickTextEdit
    \inmodule QtQuick

    This class provides access to the QTextDocument of QQuickTextEdit elements.
    This is provided to allow usage of the \l{Rich Text Processing} functionalities of Qt.
    You are not allowed to modify the document, but it can be used to output content, for example with \l{QTextDocumentWriter}),
    or provide additional formatting, for example with \l{QSyntaxHighlighter}.

    The class has to be used from C++ directly, using the property of the \l TextEdit.

    Warning: The QTextDocument provided is used internally by \l {Qt Quick} elements to provide text manipulation primitives.
    You are not allowed to perform any modification of the internal state of the QTextDocument. If you do, the element
    in question may stop functioning or crash.
*/

class QQuickTextDocumentPrivate : public QObjectPrivate
{
public:
    QPointer<QTextDocument> document;
};

QQuickTextDocument::QQuickTextDocument(QQuickItem *parent)
    : QObject(*(new QQuickTextDocumentPrivate), parent)
{
    Q_D(QQuickTextDocument);
    Q_ASSERT(parent);
    Q_ASSERT(qobject_cast<QQuickTextEdit*>(parent));
    d->document = QPointer<QTextDocument>(qobject_cast<QQuickTextEdit*>(parent)->d_func()->document);
}

QTextDocument* QQuickTextDocument::textDocument() const
{
    Q_D(const QQuickTextDocument);
    return d->document.data();
}

QT_END_NAMESPACE
