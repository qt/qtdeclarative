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

#ifndef QQUICKTEXTDOCUMENT_H
#define QQUICKTEXTDOCUMENT_H

#include <QtGui/QTextDocument>
#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QQuickTextDocumentPrivate;
class Q_QUICK_EXPORT QQuickTextDocument : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    QQuickTextDocument(QQuickItem *parent);
    QTextDocument *textDocument() const;

private:
    Q_DISABLE_COPY(QQuickTextDocument)
    Q_DECLARE_PRIVATE(QQuickTextDocument)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextDocument)

#endif
