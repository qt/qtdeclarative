// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTSELECTION_H
#define QQUICKTEXTSELECTION_H

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

#include <private/qtquickglobal_p.h>

#include <QtQuick/qquicktextdocument.h>

#include <QtQml/qqml.h>

#include <QtGui/qtextcursor.h>

QT_BEGIN_NAMESPACE

class QFont;
class QQuickTextControl;

class Q_QUICK_EXPORT QQuickTextSelection : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    // specify the selection
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged FINAL)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged FINAL)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY selectionEndChanged FINAL)

    // modify the selected text
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)

    QML_NAMED_ELEMENT(TextSelection)
    QML_ADDED_IN_VERSION(6, 11)

public:

    // keep this in sync with QTextCursor::MoveOperation
    enum MoveOperation {
        NoMove = QTextCursor::NoMove,

        Start,
        Up,
        StartOfLine,
        StartOfBlock,
        StartOfWord,
        PreviousBlock,
        PreviousCharacter,
        PreviousWord,
        Left,
        WordLeft,

        End,
        Down,
        EndOfLine,
        EndOfWord,
        EndOfBlock,
        NextBlock,
        NextCharacter,
        NextWord,
        Right,
        WordRight,

        NextCell,
        PreviousCell,
        NextRow,
        PreviousRow
    };
    Q_ENUM(MoveOperation);

    explicit QQuickTextSelection(QObject *parent = nullptr);

    void classBegin() override {}
    void componentComplete() override;

    QQuickTextDocument *document() const;
    void setDocument(QQuickTextDocument *doc);

    int selectionStart() const;
    void setSelectionStart(int start);

    int selectionEnd() const;
    void setSelectionEnd(int end);

    QString text() const;
    void setText(const QString &text);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(QColor color);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment align);

    Q_INVOKABLE bool moveSelectionStart(MoveOperation op, int n = 1);
    Q_INVOKABLE bool moveSelectionEnd(MoveOperation op, int n = 1);

    Q_INVOKABLE void duplicate();

    Q_INVOKABLE void linkTo(const QUrl &destination);

Q_SIGNALS:
    void documentChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void textChanged();
    void fontChanged();
    void colorChanged();
    void alignmentChanged();

private:
    QTextCursor cursor() const;
    void updateFromCharFormat(const QTextCharFormat &fmt);
    void updateFromBlockFormat();

private:
    QTextCursor m_cursor;
    QTextCharFormat m_charFormat;
    QTextBlockFormat m_blockFormat;
    QQuickTextDocument *m_doc = nullptr;
    QQuickTextControl *m_control = nullptr;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextSelection)

#endif // QQUICKTEXTSELECTION_H
