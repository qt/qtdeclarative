/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

#include "qquicktheme_p.h"
#include "qquickthemedata_p.h"

#include <QtCore/qset.h>
#include <QtCore/qpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Theme
    \inherits QtObject
    \instantiates QQuickThemeAttached
    \inqmlmodule Qt.labs.controls
    \ingroup utilities
    \brief A theme interface.

    TODO
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::accentColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::backgroundColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::disabledColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::focusColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::frameColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::pressColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::selectedTextColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::selectionColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::shadowColor
*/

/*!
    \qmlattachedproperty color Qt.labs.controls::Theme::textColor
*/

Q_GLOBAL_STATIC_WITH_ARGS(QQuickThemeData, globalThemeData, (QString::fromLatin1(":/qtlabscontrols/theme.json")))

static QQuickThemeAttached *themeInstance(QQmlEngine *engine)
{
    QQuickThemeAttached *theme = engine->property("_q_quicktheme").value<QQuickThemeAttached *>();
    if (!theme) {
        theme = new QQuickThemeAttached(*globalThemeData(), engine);
        engine->setProperty("_q_quicktheme", QVariant::fromValue(theme));
    }
    return theme;
}

static QQuickThemeAttached *attachedTheme(QObject *object)
{
    if (object)
        return qobject_cast<QQuickThemeAttached*>(qmlAttachedPropertiesObject<QQuickThemeAttached>(object, false));
    return Q_NULLPTR;
}

static QQuickThemeAttached *findParentTheme(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (item) {
        // lookup parent items
        QQuickItem *parent = item->parentItem();
        while (parent) {
            QQuickThemeAttached *attached = attachedTheme(parent);
            if (attached)
                return attached;
            parent = parent->parentItem();
        }

        // fallback to item's window theme
        QQuickWindow *window = item->window();
        if (window) {
            QQuickThemeAttached *attached = attachedTheme(window);
            if (attached)
                return attached;
        }
    }

    // lookup parent window theme
    QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
    if (window) {
        QQuickWindow *parentWindow = qobject_cast<QQuickWindow *>(window->parent());
        if (parentWindow) {
            QQuickThemeAttached *attached = attachedTheme(window);
            if (attached)
                return attached;
        }
    }

    // fallback to global theme
    if (object) {
        QQmlEngine *engine = qmlEngine(object);
        if (engine)
            return themeInstance(engine);
    }

    return Q_NULLPTR;
}

static QList<QQuickThemeAttached *> findChildThemes(QObject *object)
{
    QList<QQuickThemeAttached *> themes;

    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
        if (window) {
            item = window->contentItem();

            foreach (QObject *child, window->children()) {
                QQuickWindow *childWindow = qobject_cast<QQuickWindow *>(child);
                if (childWindow) {
                    QQuickThemeAttached *theme = attachedTheme(childWindow);
                    if (theme)
                        themes += theme;
                }
            }
        }
    }

    if (item) {
        foreach (QQuickItem *child, item->childItems()) {
            QQuickThemeAttached *theme = attachedTheme(child);
            if (theme)
                themes += theme;
            else
                themes += findChildThemes(child);
        }
    }

    return themes;
}

class QQuickThemeAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickThemeAttached)

public:
    QQuickThemeAttachedPrivate(const QQuickThemeData &data) : data(data),
        explicitAccentColor(false),
        explicitBackgroundColor(false),
        explicitBaseColor(false),
        explicitDisabledColor(false),
        explicitFocusColor(false),
        explicitFrameColor(false),
        explicitPressColor(false),
        explicitSelectedTextColor(false),
        explicitSelectionColor(false),
        explicitShadowColor(false),
        explicitTextColor(false) { }

    enum Method { Implicit, Explicit, Inherit };

    void setAccentColor(const QColor &color, Method method);
    void setBackgroundColor(const QColor &color, Method method);
    void setBaseColor(const QColor &color, Method method);
    void setDisabledColor(const QColor &color, Method method);
    void setFocusColor(const QColor &color, Method method);
    void setFrameColor(const QColor &color, Method method);
    void setPressColor(const QColor &color, Method method);
    void setSelectedTextColor(const QColor &color, Method method);
    void setSelectionColor(const QColor &color, Method method);
    void setShadowColor(const QColor &color, Method method);
    void setTextColor(const QColor &color, Method method);

    void inherit(QQuickThemeAttached *theme);

    const QQuickThemeData &resolve() const;

    // TODO: add QQuickItemChangeListener::itemSceneChanged()
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) Q_DECL_OVERRIDE;

    QQuickThemeData data;
    QPointer<QQuickThemeAttached> parentTheme;
    QSet<QQuickThemeAttached *> childThemes;

    bool explicitAccentColor;
    bool explicitBackgroundColor;
    bool explicitBaseColor;
    bool explicitDisabledColor;
    bool explicitFocusColor;
    bool explicitFrameColor;
    bool explicitPressColor;
    bool explicitSelectedTextColor;
    bool explicitSelectionColor;
    bool explicitShadowColor;
    bool explicitTextColor;
};

void QQuickThemeAttachedPrivate::setAccentColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitAccentColor || method != Inherit) {
        explicitAccentColor = method == Explicit;
        if (data.accentColor() != color) {
            data.setAccentColor(color);
            emit q->accentColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setAccentColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setBackgroundColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitBackgroundColor || method != Inherit) {
        explicitBackgroundColor = method == Explicit;
        if (data.backgroundColor() != color) {
            data.setBackgroundColor(color);
            emit q->backgroundColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setBackgroundColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setBaseColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitBaseColor || method != Inherit) {
        explicitBaseColor = method == Explicit;
        if (data.baseColor() != color) {
            data.setBaseColor(color);
            emit q->baseColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setBaseColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setDisabledColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitDisabledColor || method != Inherit) {
        explicitDisabledColor = method == Explicit;
        if (data.disabledColor() != color) {
            data.setDisabledColor(color);
            emit q->disabledColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setDisabledColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setFocusColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitFocusColor || method != Inherit) {
        explicitFocusColor = method == Explicit;
        if (data.focusColor() != color) {
            data.setFocusColor(color);
            emit q->focusColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setFocusColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setFrameColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitFrameColor || method != Inherit) {
        explicitFrameColor = method == Explicit;
        if (data.frameColor() != color) {
            data.setFrameColor(color);
            emit q->frameColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setFrameColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setPressColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitPressColor || method != Inherit) {
        explicitPressColor = method == Explicit;
        if (data.pressColor() != color) {
            data.setPressColor(color);
            emit q->pressColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setPressColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setSelectedTextColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitSelectedTextColor || method != Inherit) {
        explicitSelectedTextColor = method == Explicit;
        if (data.selectedTextColor() != color) {
            data.setSelectedTextColor(color);
            q->selectedTextColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setSelectedTextColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setSelectionColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitSelectionColor || method != Inherit) {
        explicitSelectionColor = method == Explicit;
        if (data.selectionColor() != color) {
            data.setSelectionColor(color);
            emit q->selectionColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setSelectionColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setShadowColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitShadowColor || method != Inherit) {
        explicitShadowColor = method == Explicit;
        if (data.shadowColor() != color) {
            data.setShadowColor(color);
            emit q->shadowColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setShadowColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::setTextColor(const QColor &color, Method method)
{
    Q_Q(QQuickThemeAttached);
    if (!explicitTextColor || method != Inherit) {
        explicitTextColor = method == Explicit;
        if (data.textColor() != color) {
            data.setTextColor(color);
            emit q->textColorChanged();

            foreach (QQuickThemeAttached *child, childThemes)
                child->d_func()->setTextColor(color, Inherit);
        }
    }
}

void QQuickThemeAttachedPrivate::inherit(QQuickThemeAttached *theme)
{
    setAccentColor(theme->accentColor(), Inherit);
    setBackgroundColor(theme->backgroundColor(), Inherit);
    setBaseColor(theme->baseColor(), QQuickThemeAttachedPrivate::Inherit);
    setDisabledColor(theme->disabledColor(), QQuickThemeAttachedPrivate::Inherit);
    setFocusColor(theme->focusColor(), Inherit);
    setFrameColor(theme->frameColor(), Inherit);
    setPressColor(theme->pressColor(), Inherit);
    setSelectedTextColor(theme->selectedTextColor(), Inherit);
    setSelectionColor(theme->selectionColor(), Inherit);
    setShadowColor(theme->shadowColor(), Inherit);
    setTextColor(theme->textColor(), Inherit);
}

const QQuickThemeData &QQuickThemeAttachedPrivate::resolve() const
{
    Q_Q(const QQuickThemeAttached);
    QQuickThemeAttached *theme = findParentTheme(const_cast<QQuickThemeAttached *>(q));
    return theme ? theme->d_func()->data : *globalThemeData();
}

void QQuickThemeAttachedPrivate::itemParentChanged(QQuickItem *item, QQuickItem *)
{
    QQuickThemeAttached *theme = attachedTheme(item);
    if (theme) {
        QQuickThemeAttached *parent = findParentTheme(theme);
        if (parent)
            theme->setParentTheme(parent);
    }
}

QQuickThemeAttached::QQuickThemeAttached(const QQuickThemeData &data, QObject *parent) :
    QObject(*(new QQuickThemeAttachedPrivate(data)), parent)
{
    Q_D(QQuickThemeAttached);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(d, QQuickItemPrivate::Parent);
}

QQuickThemeAttached::~QQuickThemeAttached()
{
    Q_D(QQuickThemeAttached);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Parent);

    setParentTheme(Q_NULLPTR);
}

QQuickThemeAttached *QQuickThemeAttached::qmlAttachedProperties(QObject *object)
{
    QQuickThemeAttached *theme = Q_NULLPTR;
    QQuickThemeAttached *parent = findParentTheme(object);
    if (parent) {
        theme = new QQuickThemeAttached(parent->d_func()->data, object);
        theme->setParentTheme(parent);
    } else {
        theme = new QQuickThemeAttached(*globalThemeData(), object);
    }

    QList<QQuickThemeAttached *> childThemes = findChildThemes(object);
    foreach (QQuickThemeAttached *child, childThemes)
        child->setParentTheme(theme);
    return theme;
}

QQuickThemeAttached *QQuickThemeAttached::parentTheme() const
{
    Q_D(const QQuickThemeAttached);
    return d->parentTheme;
}

void QQuickThemeAttached::setParentTheme(QQuickThemeAttached *theme)
{
    Q_D(QQuickThemeAttached);
    if (d->parentTheme != theme) {
        if (d->parentTheme)
            d->parentTheme->d_func()->childThemes.remove(this);
        d->parentTheme = theme;
        if (theme) {
            theme->d_func()->childThemes.insert(this);
            d->inherit(theme);
        }
    }
}

QColor QQuickThemeAttached::accentColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.accentColor();
}

void QQuickThemeAttached::setAccentColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setAccentColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetAccentColor()
{
    Q_D(QQuickThemeAttached);
    d->setAccentColor(d->resolve().accentColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::backgroundColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.backgroundColor();
}

void QQuickThemeAttached::setBackgroundColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setBackgroundColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetBackgroundColor()
{
    Q_D(QQuickThemeAttached);
    d->setBackgroundColor(d->resolve().backgroundColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::baseColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.baseColor();
}

void QQuickThemeAttached::setBaseColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setBaseColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetBaseColor()
{
    Q_D(QQuickThemeAttached);
    d->setBaseColor(d->resolve().baseColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::disabledColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.disabledColor();
}

void QQuickThemeAttached::setDisabledColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setDisabledColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetDisabledColor()
{
    Q_D(QQuickThemeAttached);
    d->setDisabledColor(d->resolve().disabledColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::focusColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.focusColor();
}

void QQuickThemeAttached::setFocusColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setFocusColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetFocusColor()
{
    Q_D(QQuickThemeAttached);
    d->setFocusColor(d->resolve().focusColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::frameColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.frameColor();
}

void QQuickThemeAttached::setFrameColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setFrameColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetFrameColor()
{
    Q_D(QQuickThemeAttached);
    d->setFrameColor(d->resolve().frameColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::pressColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.pressColor();
}

void QQuickThemeAttached::setPressColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setPressColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetPressColor()
{
    Q_D(QQuickThemeAttached);
    d->setPressColor(d->resolve().pressColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::selectedTextColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.selectedTextColor();
}

void QQuickThemeAttached::setSelectedTextColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setSelectedTextColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetSelectedTextColor()
{
    Q_D(QQuickThemeAttached);
    d->setSelectedTextColor(d->resolve().selectedTextColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::selectionColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.selectionColor();
}

void QQuickThemeAttached::setSelectionColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setSelectionColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetSelectionColor()
{
    Q_D(QQuickThemeAttached);
    d->setSelectionColor(d->resolve().selectionColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::shadowColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.shadowColor();
}

void QQuickThemeAttached::setShadowColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setShadowColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetShadowColor()
{
    Q_D(QQuickThemeAttached);
    d->setShadowColor(d->resolve().shadowColor(), QQuickThemeAttachedPrivate::Implicit);
}

QColor QQuickThemeAttached::textColor() const
{
    Q_D(const QQuickThemeAttached);
    return d->data.textColor();
}

void QQuickThemeAttached::setTextColor(const QColor &color)
{
    Q_D(QQuickThemeAttached);
    d->setTextColor(color, QQuickThemeAttachedPrivate::Explicit);
}

void QQuickThemeAttached::resetTextColor()
{
    Q_D(QQuickThemeAttached);
    d->setTextColor(d->resolve().textColor(), QQuickThemeAttachedPrivate::Implicit);
}

QT_END_NAMESPACE
