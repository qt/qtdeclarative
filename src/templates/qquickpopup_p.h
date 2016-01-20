/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKPOPUP_P_H
#define QQUICKPOPUP_P_H

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

#include <QtCore/qobject.h>
#include <QtLabsTemplates/private/qtlabstemplatesglobal_p.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPopupPrivate;
class QQuickTransition;

class Q_LABSTEMPLATES_EXPORT QQuickPopup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(bool focus READ hasFocus WRITE setFocus NOTIFY focusChanged)
    Q_PROPERTY(bool modal READ isModal WRITE setModal NOTIFY modalChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    Q_PROPERTY(QQuickTransition *enter READ enter WRITE setEnter NOTIFY enterChanged FINAL)
    Q_PROPERTY(QQuickTransition *exit READ exit WRITE setExit NOTIFY exitChanged FINAL)

public:
    explicit QQuickPopup(QObject *parent = nullptr);

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);

    bool hasFocus() const;
    void setFocus(bool focus);

    bool isModal() const;
    void setModal(bool modal);

    bool isVisible() const;

    QQuickTransition *enter() const;
    void setEnter(QQuickTransition *transition);

    QQuickTransition *exit() const;
    void setExit(QQuickTransition *transition);

public Q_SLOTS:
    void open();
    void close();

Q_SIGNALS:
    void contentItemChanged();
    void focusChanged();
    void modalChanged();
    void visibleChanged();
    void enterChanged();
    void exitChanged();

    void pressedOutside();
    void releasedOutside();
    void clickedOutside();

    void aboutToShow();
    void aboutToHide();

protected:
    QQuickPopup(QQuickPopupPrivate &dd, QObject *parent);

private:
    Q_DISABLE_COPY(QQuickPopup)
    Q_DECLARE_PRIVATE(QQuickPopup)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPopup)

#endif // QQUICKPOPUP_P_H
