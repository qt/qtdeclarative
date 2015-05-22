/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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
    \instantiates QQuickTheme
    \inqmlmodule QtQuick.Controls
    \ingroup utilities
    \brief A theme interface.

    TODO
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::accentColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::backgroundColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::disabledColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::focusColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::frameColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::pressColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::selectedTextColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::selectionColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::shadowColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Theme::textColor
*/

/*!
    \qmlattachedproperty real QtQuickControls2::Theme::padding
*/

/*!
    \qmlattachedproperty real QtQuickControls2::Theme::roundness
*/

/*!
    \qmlattachedproperty real QtQuickControls2::Theme::spacing
*/

/*!
    \qmlattachedproperty real QtQuickControls2::Theme::disabledOpacity
*/

Q_GLOBAL_STATIC_WITH_ARGS(QQuickThemeData, globalThemeData, (QString::fromLatin1(":/qtquickcontrols/theme.json")))

static QQuickTheme *themeInstance(QQmlEngine *engine)
{
    static QHash<QQmlEngine *, QQuickTheme *> themes;
    QHash<QQmlEngine *, QQuickTheme *>::iterator it = themes.find(engine);
    if (it == themes.end())
        it = themes.insert(engine, new QQuickTheme(*globalThemeData(), engine));
    return it.value();
}

static QQuickTheme *attachedTheme(QObject *object)
{
    if (object)
        return qobject_cast<QQuickTheme*>(qmlAttachedPropertiesObject<QQuickTheme>(object, false));
    return Q_NULLPTR;
}

static QQuickTheme *findParentTheme(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (item) {
        // lookup parent items
        QQuickItem *parent = item->parentItem();
        while (parent) {
            QQuickTheme *attached = attachedTheme(parent);
            if (attached)
                return attached;
            parent = parent->parentItem();
        }

        // fallback to item's window theme
        QQuickWindow *window = item->window();
        if (window) {
            QQuickTheme *attached = attachedTheme(window);
            if (attached)
                return attached;
        }
    }

    // lookup parent window theme
    QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
    if (window) {
        QQuickWindow *parentWindow = qobject_cast<QQuickWindow *>(window->parent());
        if (parentWindow) {
            QQuickTheme *attached = attachedTheme(window);
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

static QList<QQuickTheme *> findChildThemes(QObject *object)
{
    QList<QQuickTheme *> themes;

    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
        if (window) {
            item = window->contentItem();

            foreach (QObject *child, window->children()) {
                QQuickWindow *childWindow = qobject_cast<QQuickWindow *>(child);
                if (childWindow) {
                    QQuickTheme *theme = attachedTheme(childWindow);
                    if (theme)
                        themes += theme;
                }
            }
        }
    }

    if (item) {
        foreach (QQuickItem *child, item->childItems()) {
            QQuickTheme *theme = attachedTheme(child);
            if (theme)
                themes += theme;
            else
                themes += findChildThemes(child);
        }
    }

    return themes;
}

class QQuickThemePrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTheme)

public:
    QQuickThemePrivate(const QQuickThemeData &data) : data(data),
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
        explicitTextColor(false),
        explicitPadding(false),
        explicitSpacing(false),
        explicitRoundness(false),
        explicitDisabledOpacity(false) { }

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
    void setPadding(qreal padding, Method method);
    void setRoundness(qreal roundness, Method method);
    void setSpacing(qreal spacing, Method method);
    void setDisabledOpacity(qreal opacity, Method method);

    void inherit(QQuickTheme *theme);

    const QQuickThemeData &resolve() const;

    // TODO: add QQuickItemChangeListener::itemSceneChanged()
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) Q_DECL_OVERRIDE;

    QQuickThemeData data;
    QPointer<QQuickTheme> parentTheme;
    QSet<QQuickTheme *> childThemes;

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
    bool explicitPadding;
    bool explicitSpacing;
    bool explicitRoundness;
    bool explicitDisabledOpacity;
};

void QQuickThemePrivate::setAccentColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitAccentColor || method != Inherit) {
        explicitAccentColor = method == Explicit;
        if (data.accentColor() != color) {
            data.setAccentColor(color);
            emit q->accentColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setAccentColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setBackgroundColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitBackgroundColor || method != Inherit) {
        explicitBackgroundColor = method == Explicit;
        if (data.backgroundColor() != color) {
            data.setBackgroundColor(color);
            emit q->backgroundColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setBackgroundColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setBaseColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitBaseColor || method != Inherit) {
        explicitBaseColor = method == Explicit;
        if (data.baseColor() != color) {
            data.setBaseColor(color);
            emit q->baseColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setBaseColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setDisabledColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitDisabledColor || method != Inherit) {
        explicitDisabledColor = method == Explicit;
        if (data.disabledColor() != color) {
            data.setDisabledColor(color);
            emit q->disabledColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setDisabledColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setFocusColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitFocusColor || method != Inherit) {
        explicitFocusColor = method == Explicit;
        if (data.focusColor() != color) {
            data.setFocusColor(color);
            emit q->focusColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setFocusColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setFrameColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitFrameColor || method != Inherit) {
        explicitFrameColor = method == Explicit;
        if (data.frameColor() != color) {
            data.setFrameColor(color);
            emit q->frameColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setFrameColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setPressColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitPressColor || method != Inherit) {
        explicitPressColor = method == Explicit;
        if (data.pressColor() != color) {
            data.setPressColor(color);
            emit q->pressColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setPressColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setSelectedTextColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitSelectedTextColor || method != Inherit) {
        explicitSelectedTextColor = method == Explicit;
        if (data.selectedTextColor() != color) {
            data.setSelectedTextColor(color);
            q->selectedTextColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setSelectedTextColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setSelectionColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitSelectionColor || method != Inherit) {
        explicitSelectionColor = method == Explicit;
        if (data.selectionColor() != color) {
            data.setSelectionColor(color);
            emit q->selectionColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setSelectionColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setShadowColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitShadowColor || method != Inherit) {
        explicitShadowColor = method == Explicit;
        if (data.shadowColor() != color) {
            data.setShadowColor(color);
            emit q->shadowColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setShadowColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setTextColor(const QColor &color, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitTextColor || method != Inherit) {
        explicitTextColor = method == Explicit;
        if (data.textColor() != color) {
            data.setTextColor(color);
            emit q->textColorChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setTextColor(color, Inherit);
        }
    }
}

void QQuickThemePrivate::setPadding(qreal padding, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitPadding || method != Inherit) {
        explicitPadding = method == Explicit;
        if (data.padding() != padding) {
            data.setPadding(padding);
            emit q->paddingChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setPadding(padding, Inherit);
        }
    }
}

void QQuickThemePrivate::setRoundness(qreal roundness, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitRoundness || method != Inherit) {
        explicitRoundness = method == Explicit;
        if (data.roundness() != roundness) {
            data.setRoundness(roundness);
            emit q->roundnessChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setRoundness(roundness, Inherit);
        }
    }
}

void QQuickThemePrivate::setSpacing(qreal spacing, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitSpacing || method != Inherit) {
        explicitSpacing = method == Explicit;
        if (data.spacing() != spacing) {
            data.setSpacing(spacing);
            emit q->spacingChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setSpacing(spacing, Inherit);
        }
    }
}

void QQuickThemePrivate::setDisabledOpacity(qreal opacity, Method method)
{
    Q_Q(QQuickTheme);
    if (!explicitDisabledOpacity || method != Inherit) {
        explicitDisabledOpacity = method == Explicit;
        if (data.disabledOpacity() != opacity) {
            data.setDisabledOpacity(opacity);
            emit q->disabledOpacityChanged();

            foreach (QQuickTheme *child, childThemes)
                child->d_func()->setDisabledOpacity(opacity, Inherit);
        }
    }
}

void QQuickThemePrivate::inherit(QQuickTheme *theme)
{
    setAccentColor(theme->accentColor(), Inherit);
    setBackgroundColor(theme->backgroundColor(), Inherit);
    setBaseColor(theme->baseColor(), QQuickThemePrivate::Inherit);
    setDisabledColor(theme->disabledColor(), QQuickThemePrivate::Inherit);
    setFocusColor(theme->focusColor(), Inherit);
    setFrameColor(theme->frameColor(), Inherit);
    setPressColor(theme->pressColor(), Inherit);
    setSelectedTextColor(theme->selectedTextColor(), Inherit);
    setSelectionColor(theme->selectionColor(), Inherit);
    setShadowColor(theme->shadowColor(), Inherit);
    setTextColor(theme->textColor(), Inherit);
    setPadding(theme->padding(), Inherit);
    setRoundness(theme->roundness(), Inherit);
    setSpacing(theme->spacing(), Inherit);
    setDisabledOpacity(theme->disabledOpacity(), Inherit);
}

const QQuickThemeData &QQuickThemePrivate::resolve() const
{
    Q_Q(const QQuickTheme);
    QQuickTheme *theme = findParentTheme(const_cast<QQuickTheme *>(q));
    return theme ? theme->d_func()->data : *globalThemeData();
}

void QQuickThemePrivate::itemParentChanged(QQuickItem *item, QQuickItem *)
{
    QQuickTheme *theme = attachedTheme(item);
    if (theme) {
        QQuickTheme *parent = findParentTheme(theme);
        if (parent)
            theme->setParentTheme(parent);
    }
}

QQuickTheme::QQuickTheme(const QQuickThemeData &data, QObject *parent) :
    QObject(*(new QQuickThemePrivate(data)), parent)
{
    Q_D(QQuickTheme);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(d, QQuickItemPrivate::Parent);
}

QQuickTheme::~QQuickTheme()
{
    Q_D(QQuickTheme);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Parent);

    setParentTheme(Q_NULLPTR);
}

QQuickTheme *QQuickTheme::qmlAttachedProperties(QObject *object)
{
    QQuickTheme *theme = Q_NULLPTR;
    QQuickTheme *parent = findParentTheme(object);
    if (parent) {
        theme = new QQuickTheme(parent->d_func()->data, object);
        theme->setParentTheme(parent);
    } else {
        theme = new QQuickTheme(*globalThemeData(), object);
    }

    QList<QQuickTheme *> childThemes = findChildThemes(object);
    foreach (QQuickTheme *child, childThemes)
        child->setParentTheme(theme);
    return theme;
}

QQuickTheme *QQuickTheme::parentTheme() const
{
    Q_D(const QQuickTheme);
    return d->parentTheme;
}

void QQuickTheme::setParentTheme(QQuickTheme *theme)
{
    Q_D(QQuickTheme);
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

QColor QQuickTheme::accentColor() const
{
    Q_D(const QQuickTheme);
    return d->data.accentColor();
}

void QQuickTheme::setAccentColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setAccentColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetAccentColor()
{
    Q_D(QQuickTheme);
    d->setAccentColor(d->resolve().accentColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::backgroundColor() const
{
    Q_D(const QQuickTheme);
    return d->data.backgroundColor();
}

void QQuickTheme::setBackgroundColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setBackgroundColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetBackgroundColor()
{
    Q_D(QQuickTheme);
    d->setBackgroundColor(d->resolve().backgroundColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::baseColor() const
{
    Q_D(const QQuickTheme);
    return d->data.baseColor();
}

void QQuickTheme::setBaseColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setBaseColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetBaseColor()
{
    Q_D(QQuickTheme);
    d->setBaseColor(d->resolve().baseColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::disabledColor() const
{
    Q_D(const QQuickTheme);
    return d->data.disabledColor();
}

void QQuickTheme::setDisabledColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setDisabledColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetDisabledColor()
{
    Q_D(QQuickTheme);
    d->setDisabledColor(d->resolve().disabledColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::focusColor() const
{
    Q_D(const QQuickTheme);
    return d->data.focusColor();
}

void QQuickTheme::setFocusColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setFocusColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetFocusColor()
{
    Q_D(QQuickTheme);
    d->setFocusColor(d->resolve().focusColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::frameColor() const
{
    Q_D(const QQuickTheme);
    return d->data.frameColor();
}

void QQuickTheme::setFrameColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setFrameColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetFrameColor()
{
    Q_D(QQuickTheme);
    d->setFrameColor(d->resolve().frameColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::pressColor() const
{
    Q_D(const QQuickTheme);
    return d->data.pressColor();
}

void QQuickTheme::setPressColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setPressColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetPressColor()
{
    Q_D(QQuickTheme);
    d->setPressColor(d->resolve().pressColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::selectedTextColor() const
{
    Q_D(const QQuickTheme);
    return d->data.selectedTextColor();
}

void QQuickTheme::setSelectedTextColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setSelectedTextColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetSelectedTextColor()
{
    Q_D(QQuickTheme);
    d->setSelectedTextColor(d->resolve().selectedTextColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::selectionColor() const
{
    Q_D(const QQuickTheme);
    return d->data.selectionColor();
}

void QQuickTheme::setSelectionColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setSelectionColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetSelectionColor()
{
    Q_D(QQuickTheme);
    d->setSelectionColor(d->resolve().selectionColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::shadowColor() const
{
    Q_D(const QQuickTheme);
    return d->data.shadowColor();
}

void QQuickTheme::setShadowColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setShadowColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetShadowColor()
{
    Q_D(QQuickTheme);
    d->setShadowColor(d->resolve().shadowColor(), QQuickThemePrivate::Implicit);
}

QColor QQuickTheme::textColor() const
{
    Q_D(const QQuickTheme);
    return d->data.textColor();
}

void QQuickTheme::setTextColor(const QColor &color)
{
    Q_D(QQuickTheme);
    d->setTextColor(color, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetTextColor()
{
    Q_D(QQuickTheme);
    d->setTextColor(d->resolve().textColor(), QQuickThemePrivate::Implicit);
}

qreal QQuickTheme::padding() const
{
    Q_D(const QQuickTheme);
    return d->data.padding();
}

void QQuickTheme::setPadding(qreal padding)
{
    Q_D(QQuickTheme);
    d->setPadding(padding, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetPadding()
{
    Q_D(QQuickTheme);
    d->setPadding(d->resolve().padding(), QQuickThemePrivate::Implicit);
}

qreal QQuickTheme::roundness() const
{
    Q_D(const QQuickTheme);
    return d->data.roundness();
}

void QQuickTheme::setRoundness(qreal roundness)
{
    Q_D(QQuickTheme);
    d->setRoundness(roundness, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetRoundness()
{
    Q_D(QQuickTheme);
    d->setRoundness(d->resolve().roundness(), QQuickThemePrivate::Implicit);
}

qreal QQuickTheme::spacing() const
{
    Q_D(const QQuickTheme);
    return d->data.spacing();
}

void QQuickTheme::setSpacing(qreal spacing)
{
    Q_D(QQuickTheme);
    d->setSpacing(spacing, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetSpacing()
{
    Q_D(QQuickTheme);
    d->setSpacing(d->resolve().spacing(), QQuickThemePrivate::Implicit);
}

qreal QQuickTheme::disabledOpacity() const
{
    Q_D(const QQuickTheme);
    return d->data.disabledOpacity();
}

void QQuickTheme::setDisabledOpacity(qreal opacity)
{
    Q_D(QQuickTheme);
    d->setDisabledOpacity(opacity, QQuickThemePrivate::Explicit);
}

void QQuickTheme::resetDisabledOpacity()
{
    Q_D(QQuickTheme);
    d->setDisabledOpacity(d->resolve().disabledOpacity(), QQuickThemePrivate::Implicit);
}

QT_END_NAMESPACE
