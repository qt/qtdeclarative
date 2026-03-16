// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype StyleAnimation
    \inqmlmodule Qt.labs.StyleKit
    \inherits ParallelAnimation
    \brief A convenience type for animating style properties.

    StyleAnimation is an \l [QtQuick]{Animation} that animates style properties
    when a control changes \l {ControlStateStyle}{state}. For example, setting
    \l animateBackgroundColors to \c true will animate all color properties of
    the \l {ControlStyle::}{background}, such as \l {DelegateStyle::color}{background.color},
    \l {BorderStyle::color}{background.border.color}, \l {ImageStyle::color}{background.image.color},
    and \l {ShadowStyle::color}{background.shadow.color}. It is used inside a
    \l {ControlStyle::}{transition}:

    \snippet StyleAnimationSnippets.qml transition

    StyleAnimation is just a convenience \l [QtQuick]{Animation} that handles listing the
    affected style properties automatically. The snippet above could also be written as:

    \snippet StyleAnimationSnippets.qml custom transition

    A StyleAnimation can also be used together with other animations:

    \snippet StyleAnimationSnippets.qml mixed transition

    \labs

    \sa {ControlStyleProperties::transition}{ControlStyle.transition}, ControlStyle
*/

/*!
    \qmlproperty EasingCurve StyleAnimation::easing

    The easing curve applied to the animation.

    \sa duration, PropertyAnimation
*/

/*!
    \qmlproperty int StyleAnimation::duration

    The duration in milliseconds of the animation.

    \sa easing, PropertyAnimation
*/

/*!
    \qmlproperty bool StyleAnimation::animateColors

    When \c true, animates all color properties across all delegates. This is a
    shorthand for enabling \l animateBackgroundColors, \l animateHandleColors,
    and \l animateIndicatorColors at once.

    Note that \l ColorAnimation also animates all color properties that
    change during a transition when \l {PropertyAnimation::}{property} and
    \l {PropertyAnimation::}{properties} are both left unset. This makes it a
    an alternative to StyleAnimation when only colors need to be animated.

    \sa animateBackgroundColors, animateHandleColors, animateIndicatorColors
*/

/*!
    \qmlproperty bool StyleAnimation::animateControlGeometry

    When \c true, animates the control's geometry properties, such as
    \l {ControlStyleProperties::spacing}{spacing} and
    \l {ControlStyleProperties::padding}{padding}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateBackgroundColors

    When \c true, animates the color properties of the
    \l {ControlStyleProperties::}{background} delegate,
    including \l {BorderStyle::color}{border} and \l {ShadowStyle::color}{shadow} colors.

    \sa animateColors, animateBackgroundGeometry
*/

/*!
    \qmlproperty bool StyleAnimation::animateBackgroundGeometry

    When \c true, animates the geometry properties of the
    \l {ControlStyleProperties::}{background} delegate,
    such as \l {DelegateStyle::implicitWidth}{implicit size}
    and \l {DelegateStyle::}{margins}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateBackgroundRadii

    When \c true, animates the corner \l {DelegateStyle::radius}{radii} of the
    \l {ControlStyleProperties::}{background} delegate.
*/

/*!
    \qmlproperty bool StyleAnimation::animateBackgroundBorder

    When \c true, animates the \l {BorderStyle::width}{border width} of the
    \l {ControlStyleProperties::}{background} delegate.
*/

/*!
    \qmlproperty bool StyleAnimation::animateBackgroundShadow

    When \c true, animates the shadow properties of the
    \l {ControlStyleProperties::}{background} delegate,
    such as \l {ShadowStyle::verticalOffset}{vertical offset},
    \l {ShadowStyle::horizontalOffset}{horizontal offset},
    \l {ShadowStyle::scale}{scale}, and \l {ShadowStyle::blur}{blur}.
    The shadow \l {ShadowStyle::color}{color} is animated by \l animateHandleColors.
*/

/*!
    \qmlproperty bool StyleAnimation::animateBackgroundScaleAndRotation

    When \c true, animates the  and rotation of the
    \l {ControlStyleProperties::}{background} delegate.
*/

/*!
    \qmlproperty bool StyleAnimation::animateHandleColors

    When \c true, animates the color properties of the
    \l {ControlStyleProperties::}{handle} delegate and its \l {HandleStyle}{sub-handles},
    including \l {BorderStyle::color}{border} and \l {ShadowStyle::color}{shadow} colors.

    \sa animateColors, animateHandleGeometry
*/

/*!
    \qmlproperty bool StyleAnimation::animateHandleGeometry

    When \c true, animates the geometry properties of the
    \l {ControlStyleProperties::}{handle} delegate and its \l {HandleStyle}{sub-handles},
    such as \l {DelegateStyle::implicitWidth}{implicit size} and
    \l {DelegateStyle::}{margins}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateHandleRadii

    When \c true, animates the corner \l {DelegateStyle::radius}{radii} of the
    \l {ControlStyleProperties::}{handle} delegate and its \l {HandleStyle}{sub-handles}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateHandleBorder

    When \c true, animates the \l {BorderStyle::width}{border width} of the
    \l {ControlStyleProperties::}{handle} delegate and its \l {HandleStyle}{sub-handles}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateHandleShadow

    When \c true, animates the shadow properties of the
    \l {ControlStyleProperties::}{handle} delegate and its \l {HandleStyle}{sub-handles},
    such as \l {ShadowStyle::verticalOffset}{vertical offset},
    \l {ShadowStyle::horizontalOffset}{horizontal offset},
    \l {ShadowStyle::scale}{scale}, and \l {ShadowStyle::blur}{blur}.
    The shadow \l {ShadowStyle::color}{color} is animated by \l animateHandleColors.
*/

/*!
    \qmlproperty bool StyleAnimation::animateHandleScaleAndRotation

    When \c true, animates the \l {DelegateStyle::}{scale} and \l {DelegateStyle::}{rotation} of the
    \l {ControlStyleProperties::}{handle} delegate and its \l {HandleStyle}{sub-handles}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateIndicatorColors

    When \c true, animates the color properties of the
    \l {ControlStyleProperties::}{indicator} delegate and its \l {IndicatorStyle}{sub-indicators},
    including \l {BorderStyle::color}{border} and \l {ShadowStyle::color}{shadow} colors.

    \sa animateColors, animateIndicatorGeometry
*/

/*!
    \qmlproperty bool StyleAnimation::animateIndicatorGeometry

    When \c true, animates the geometry properties of the
    \l {ControlStyleProperties::}{indicator} delegate and its \l {IndicatorStyle}{sub-indicators},
    such as \l {DelegateStyle::implicitWidth}{implicit size} and \l {DelegateStyle::}{margins}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateIndicatorRadii

    When \c true, animates the corner \l {DelegateStyle::radius}{radii} of the
    \l {ControlStyleProperties::}{indicator} delegate and its \l {IndicatorStyle}{sub-indicators}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateIndicatorBorder

    When \c true, animates the \l {BorderStyle::width}{border width} of the
    \l {ControlStyleProperties::}{indicator} delegate and its \l {IndicatorStyle}{sub-indicators}.
*/

/*!
    \qmlproperty bool StyleAnimation::animateIndicatorShadow

    When \c true, animates the shadow properties of the
    \l {ControlStyleProperties::}{indicator} delegate and its \l {IndicatorStyle}{sub-indicators},
    such as \l {ShadowStyle::verticalOffset}{vertical offset},
    \l {ShadowStyle::horizontalOffset}{horizontal offset},
    \l {ShadowStyle::scale}{scale}, and \l {ShadowStyle::blur}{blur}.
    The shadow \l {ShadowStyle::color}{color} is animated by \l animateHandleColors.
*/

/*!
    \qmlproperty bool StyleAnimation::animateIndicatorScaleAndRotation

    When \c true, animates the scale and rotation of the
    \l {ControlStyleProperties::}{indicator} delegate and its \l {IndicatorStyle}{sub-indicators}.
*/

QT_END_NAMESPACE
