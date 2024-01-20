// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTEXTDOCUMENT_H
#define QQUICKTEXTDOCUMENT_H

#include <QtGui/QTextDocument>
#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QQuickTextDocumentPrivate;
class Q_QUICK_EXPORT QQuickTextDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged REVISION(6, 7))
    Q_PROPERTY(bool modified READ isModified WRITE setModified NOTIFY modifiedChanged REVISION(6, 7))

    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickTextDocument(QQuickItem *parent);

    QUrl source() const;
    void setSource(const QUrl &url);

    bool isModified() const;
    void setModified(bool modified);

    QTextDocument *textDocument() const;
    void setTextDocument(QTextDocument *document);

    Q_REVISION(6, 7) Q_INVOKABLE void save();
    Q_REVISION(6, 7) Q_INVOKABLE void saveAs(const QUrl &url);

Q_SIGNALS:
    Q_REVISION(6,7) void textDocumentChanged();
    Q_REVISION(6, 7) void sourceChanged();
    Q_REVISION(6, 7) void modifiedChanged();

    Q_REVISION(6, 7) void error(const QString &message);

private:
    Q_DISABLE_COPY(QQuickTextDocument)
    Q_DECLARE_PRIVATE(QQuickTextDocument)
};

QT_END_NAMESPACE

#endif
