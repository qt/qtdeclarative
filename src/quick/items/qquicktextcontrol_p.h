/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKTEXTCONTROL_P_H
#define QQUICKTEXTCONTROL_P_H

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

#include <QtGui/qtextdocument.h>
#include <QtGui/qtextoption.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qtextformat.h>
#include <QtCore/qrect.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextdocumentfragment.h>
#include <QtGui/qclipboard.h>
#include <QtCore/qmimedata.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QStyleSheet;
class QTextDocument;
class QQuickTextControlPrivate;
class QAbstractScrollArea;
class QEvent;
class QTimerEvent;
class QPagedPaintDevice;

class Q_AUTOTEST_EXPORT QQuickTextControl : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickTextControl)
public:
    explicit QQuickTextControl(QTextDocument *doc, QObject *parent = 0);
    virtual ~QQuickTextControl();

    QTextDocument *document() const;

    void setTextCursor(const QTextCursor &cursor);
    QTextCursor textCursor() const;

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    QString toPlainText() const;

#ifndef QT_NO_TEXTHTMLPARSER
    QString toHtml() const;
#endif

    QRectF cursorRect(const QTextCursor &cursor) const;
    QRectF cursorRect() const;
    QRectF selectionRect(const QTextCursor &cursor) const;
    QRectF selectionRect() const;

    QString anchorAt(const QPointF &pos) const;

    void setCursorWidth(int width);

    void setAcceptRichText(bool accept);

    void moveCursor(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    bool canPaste() const;

    void setCursorIsFocusIndicator(bool b);
    void setWordSelectionEnabled(bool enabled);

    virtual int hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const;
    virtual QRectF blockBoundingRect(const QTextBlock &block) const;
    QAbstractTextDocumentLayout::PaintContext getPaintContext() const;

public Q_SLOTS:
    void setPlainText(const QString &text);
    void setHtml(const QString &text);

#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy();
    void paste(QClipboard::Mode mode = QClipboard::Clipboard);
#endif

    void undo();
    void redo();

    void selectAll();

Q_SIGNALS:
    void textChanged();
    void undoAvailable(bool b);
    void redoAvailable(bool b);
    void currentCharFormatChanged(const QTextCharFormat &format);
    void copyAvailable(bool b);
    void selectionChanged();
    void cursorPositionChanged();

    // control signals
    void updateCursorRequest(const QRectF &rect = QRectF());
    void updateRequest(const QRectF &rect = QRectF());
    void documentSizeChanged(const QSizeF &);
    void cursorRectangleChanged();
    void linkActivated(const QString &link);
    void linkHovered(const QString &);

public:
    virtual void processEvent(QEvent *e, const QMatrix &matrix);
    void processEvent(QEvent *e, const QPointF &coordinateOffset = QPointF());

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    virtual QMimeData *createMimeDataFromSelection() const;
    virtual bool canInsertFromMimeData(const QMimeData *source) const;
    virtual void insertFromMimeData(const QMimeData *source);

    bool cursorOn() const;

protected:
    virtual void timerEvent(QTimerEvent *e);

    virtual bool event(QEvent *e);

private:
    Q_DISABLE_COPY(QQuickTextControl)
    Q_PRIVATE_SLOT(d_func(), void _q_updateCurrentCharFormatAndSelection())
    Q_PRIVATE_SLOT(d_func(), void _q_emitCursorPosChanged(const QTextCursor &))
    Q_PRIVATE_SLOT(d_func(), void _q_updateBlock(const QTextBlock &))
    Q_PRIVATE_SLOT(d_func(), void _q_documentLayoutChanged())
};


// also used by QLabel
class QQuickTextEditMimeData : public QMimeData
{
public:
    inline QQuickTextEditMimeData(const QTextDocumentFragment &aFragment) : fragment(aFragment) {}

    virtual QStringList formats() const;
protected:
    virtual QVariant retrieveData(const QString &mimeType, QVariant::Type type) const;
private:
    void setup() const;

    mutable QTextDocumentFragment fragment;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQuickTextControl_H
