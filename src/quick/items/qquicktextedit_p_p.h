/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKTEXTEDIT_P_P_H
#define QQUICKTEXTEDIT_P_P_H

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

#include "qquicktextedit_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE
class QTextLayout;
class QQuickTextDocumentWithImageResources;
class QQuickTextControl;
class QQuickTextEditPrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickTextEdit)

public:
    QQuickTextEditPrivate()
        : color(QRgb(0xFF000000)), selectionColor(QRgb(0xFF000080)), selectedTextColor(QRgb(0xFFFFFFFF))
        , textMargin(0.0), yoff(0), font(sourceFont), cursorComponent(0), cursor(0), document(0), control(0)
        , lastSelectionStart(0), lastSelectionEnd(0), lineCount(0)
        , hAlign(QQuickTextEdit::AlignLeft), vAlign(QQuickTextEdit::AlignTop)
        , format(QQuickTextEdit::PlainText), wrapMode(QQuickTextEdit::NoWrap)
        , mouseSelectionMode(QQuickTextEdit::SelectCharacters), inputMethodHints(Qt::ImhNone)
        , updateType(UpdatePaintNode)
        , documentDirty(true), dirty(false), richText(false), cursorVisible(false)
        , focusOnPress(true), persistentSelection(false), requireImplicitWidth(false)
        , selectByMouse(false), canPaste(false), canPasteValid(false), hAlignImplicit(true)
        , rightToLeftText(false), textCached(false)
    {
    }

    static QQuickTextEditPrivate *get(QQuickTextEdit *item) {
        return static_cast<QQuickTextEditPrivate *>(QObjectPrivate::get(item)); }

    void init();

    void updateDefaultTextOption();
    void relayoutDocument();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickTextEdit::HAlignment, bool forceAlign = false);
    void mirrorChange();
    qreal getImplicitWidth() const;

    QColor color;
    QColor selectionColor;
    QColor selectedTextColor;

    QSizeF contentSize;

    qreal textMargin;
    qreal yoff;

    QString text;
    QUrl baseUrl;
    QFont sourceFont;
    QFont font;

    QQmlComponent* cursorComponent;
    QQuickItem* cursor;
    QQuickTextDocumentWithImageResources *document;
    QQuickTextControl *control;

    int lastSelectionStart;
    int lastSelectionEnd;
    int lineCount;

    enum UpdateType {
        UpdateNone,
        UpdateOnlyPreprocess,
        UpdatePaintNode
    };

    QQuickTextEdit::HAlignment hAlign;
    QQuickTextEdit::VAlignment vAlign;
    QQuickTextEdit::TextFormat format;
    QQuickTextEdit::WrapMode wrapMode;
    QQuickTextEdit::SelectionMode mouseSelectionMode;
    Qt::InputMethodHints inputMethodHints;
    UpdateType updateType;

    bool documentDirty : 1;
    bool dirty : 1;
    bool richText : 1;
    bool cursorVisible : 1;
    bool focusOnPress : 1;
    bool persistentSelection : 1;
    bool requireImplicitWidth:1;
    bool selectByMouse:1;
    bool canPaste:1;
    bool canPasteValid:1;
    bool hAlignImplicit:1;
    bool rightToLeftText:1;
    bool textCached:1;
};

QT_END_NAMESPACE

#endif // QQUICKTEXTEDIT_P_P_H
