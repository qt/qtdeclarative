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

#include "qquickanimatedimage_p.h"
#include "qquickanimatedimage_p_p.h"

#ifndef QT_NO_MOVIE

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlengine.h>
#include <QtGui/qmovie.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>

QT_BEGIN_NAMESPACE
/*!
    \qmlclass AnimatedImage QQuickAnimatedImage
    \inqmlmodule QtQuick 2
    \inherits Image
    \brief Plays animations stored as a series of images
    \ingroup qtquick-images

    The AnimatedImage element extends the features of the \l Image element, providing
    a way to play animations stored as images containing a series of frames,
    such as those stored in GIF files.

    Information about the current frame and total length of the animation can be
    obtained using the \l currentFrame and \l frameCount properties. You can
    start, pause and stop the animation by changing the values of the \l playing
    and \l paused properties.

    The full list of supported formats can be determined with QMovie::supportedFormats().

    \section1 Example Usage

    \beginfloatleft
    \image animatedimageitem.gif
    \endfloat

    The following QML shows how to display an animated image and obtain information
    about its state, such as the current frame and total number of frames.
    The result is an animated image with a simple progress indicator underneath it.

    \b Note: Unlike images, animated images are not cached or shared internally.

    \clearfloat
    \snippet qml/animatedimage.qml document

    \sa BorderImage, Image
*/

/*!
    \qmlproperty url QtQuick2::AnimatedImage::source

    This property holds the URL that refers to the source image.

    AnimatedImage can handle any image format supported by Qt, loaded from any
    URL scheme supported by Qt.

    \sa QQuickImageProvider
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedImage::asynchronous

    Specifies that images on the local filesystem should be loaded
    asynchronously in a separate thread.  The default value is
    false, causing the user interface thread to block while the
    image is loaded.  Setting \a asynchronous to true is useful where
    maintaining a responsive user interface is more desirable
    than having images immediately visible.

    Note that this property is only valid for images read from the
    local filesystem.  Images loaded via a network resource (e.g. HTTP)
    are always loaded asynchronously.
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedImage::mirror

    This property holds whether the image should be horizontally inverted
    (effectively displaying a mirrored image).

    The default value is false.
*/

QQuickAnimatedImage::QQuickAnimatedImage(QQuickItem *parent)
    : QQuickImage(*(new QQuickAnimatedImagePrivate), parent)
{
}

QQuickAnimatedImage::~QQuickAnimatedImage()
{
    Q_D(QQuickAnimatedImage);
    delete d->_movie;
}

/*!
  \qmlproperty bool QtQuick2::AnimatedImage::paused
  This property holds whether the animated image is paused.

  By default, this property is false. Set it to true when you want to pause
  the animation.
*/

bool QQuickAnimatedImage::isPaused() const
{
    Q_D(const QQuickAnimatedImage);
    if (!d->_movie)
        return false;
    return d->_movie->state()==QMovie::Paused;
}

void QQuickAnimatedImage::setPaused(bool pause)
{
    Q_D(QQuickAnimatedImage);
    if (pause == d->paused)
        return;
    d->paused = pause;
    if (!d->_movie)
        return;
    d->_movie->setPaused(pause);
}

/*!
  \qmlproperty bool QtQuick2::AnimatedImage::playing
  This property holds whether the animated image is playing.

  By default, this property is true, meaning that the animation
  will start playing immediately.
*/

bool QQuickAnimatedImage::isPlaying() const
{
    Q_D(const QQuickAnimatedImage);
    if (!d->_movie)
        return false;
    return d->_movie->state()!=QMovie::NotRunning;
}

void QQuickAnimatedImage::setPlaying(bool play)
{
    Q_D(QQuickAnimatedImage);
    if (play == d->playing)
        return;
    d->playing = play;
    if (!d->_movie)
        return;
    if (play)
        d->_movie->start();
    else
        d->_movie->stop();
}

/*!
  \qmlproperty int QtQuick2::AnimatedImage::currentFrame
  \qmlproperty int QtQuick2::AnimatedImage::frameCount

  currentFrame is the frame that is currently visible. By monitoring this property
  for changes, you can animate other items at the same time as the image.

  frameCount is the number of frames in the animation. For some animation formats,
  frameCount is unknown and has a value of zero.
*/
int QQuickAnimatedImage::currentFrame() const
{
    Q_D(const QQuickAnimatedImage);
    if (!d->_movie)
        return d->preset_currentframe;
    return d->_movie->currentFrameNumber();
}

void QQuickAnimatedImage::setCurrentFrame(int frame)
{
    Q_D(QQuickAnimatedImage);
    if (!d->_movie) {
        d->preset_currentframe = frame;
        return;
    }
    d->_movie->jumpToFrame(frame);
}

int QQuickAnimatedImage::frameCount() const
{
    Q_D(const QQuickAnimatedImage);
    if (!d->_movie)
        return 0;
    return d->_movie->frameCount();
}

void QQuickAnimatedImage::setSource(const QUrl &url)
{
    Q_D(QQuickAnimatedImage);
    if (url == d->url)
        return;

    delete d->_movie;
    d->_movie = 0;

    if (d->reply) {
        d->reply->deleteLater();
        d->reply = 0;
    }

    d->url = url;
    emit sourceChanged(d->url);

    if (isComponentComplete())
        load();
}

void QQuickAnimatedImage::load()
{
    Q_D(QQuickAnimatedImage);

    QQuickImageBase::Status oldStatus = d->status;
    qreal oldProgress = d->progress;

    if (d->url.isEmpty()) {
        delete d->_movie;
        d->setImage(QImage());
        d->progress = 0;
        d->status = Null;
        if (d->status != oldStatus)
            emit statusChanged(d->status);
        if (d->progress != oldProgress)
            emit progressChanged(d->progress);
    } else {
        QString lf = QQmlFile::urlToLocalFileOrQrc(d->url);
        if (!lf.isEmpty()) {
            //### should be unified with movieRequestFinished
            d->_movie = new QMovie(lf);
            if (!d->_movie->isValid()){
                qmlInfo(this) << "Error Reading Animated Image File " << d->url.toString();
                delete d->_movie;
                d->_movie = 0;
                d->status = Error;
                if (d->status != oldStatus)
                    emit statusChanged(d->status);
                return;
            }
            connect(d->_movie, SIGNAL(stateChanged(QMovie::MovieState)),
                    this, SLOT(playingStatusChanged()));
            connect(d->_movie, SIGNAL(frameChanged(int)),
                    this, SLOT(movieUpdate()));
            d->_movie->setCacheMode(QMovie::CacheAll);
            if (d->playing)
                d->_movie->start();
            else
                d->_movie->jumpToFrame(0);
            if (d->paused)
                d->_movie->setPaused(true);
            d->setImage(d->_movie->currentPixmap().toImage());
            d->status = Ready;
            d->progress = 1.0;
            if (d->status != oldStatus)
                emit statusChanged(d->status);
            if (d->progress != oldProgress)
                emit progressChanged(d->progress);
            return;
        }

        d->status = Loading;
        d->progress = 0;
        emit statusChanged(d->status);
        emit progressChanged(d->progress);
        QNetworkRequest req(d->url);
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
        d->reply = qmlEngine(this)->networkAccessManager()->get(req);
        QObject::connect(d->reply, SIGNAL(finished()),
                         this, SLOT(movieRequestFinished()));
        QObject::connect(d->reply, SIGNAL(downloadProgress(qint64,qint64)),
                         this, SLOT(requestProgress(qint64,qint64)));
    }
}

#define ANIMATEDIMAGE_MAXIMUM_REDIRECT_RECURSION 16

void QQuickAnimatedImage::movieRequestFinished()
{
    Q_D(QQuickAnimatedImage);

    d->redirectCount++;
    if (d->redirectCount < ANIMATEDIMAGE_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = d->reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = d->reply->url().resolved(redirect.toUrl());
            d->reply->deleteLater();
            d->reply = 0;
            setSource(url);
            return;
        }
    }
    d->redirectCount=0;

    d->_movie = new QMovie(d->reply);
    if (!d->_movie->isValid()){
#ifndef QT_NO_DEBUG_STREAM
        qmlInfo(this) << "Error Reading Animated Image File " << d->url;
#endif
        delete d->_movie;
        d->_movie = 0;
        d->status = Error;
        emit statusChanged(d->status);
        return;
    }
    connect(d->_movie, SIGNAL(stateChanged(QMovie::MovieState)),
            this, SLOT(playingStatusChanged()));
    connect(d->_movie, SIGNAL(frameChanged(int)),
            this, SLOT(movieUpdate()));
    d->_movie->setCacheMode(QMovie::CacheAll);
    if (d->playing)
        d->_movie->start();
    if (d->paused || !d->playing) {
        d->_movie->jumpToFrame(d->preset_currentframe);
        d->preset_currentframe = 0;
    }
    if (d->paused)
        d->_movie->setPaused(true);
    d->setImage(d->_movie->currentPixmap().toImage());
    d->status = Ready;
    emit statusChanged(d->status);
}

void QQuickAnimatedImage::movieUpdate()
{
    Q_D(QQuickAnimatedImage);
    d->setImage(d->_movie->currentPixmap().toImage());
    emit frameChanged();
}

void QQuickAnimatedImage::playingStatusChanged()
{
    Q_D(QQuickAnimatedImage);
    if ((d->_movie->state() != QMovie::NotRunning) != d->playing) {
        d->playing = (d->_movie->state() != QMovie::NotRunning);
        emit playingChanged();
    }
    if ((d->_movie->state() == QMovie::Paused) != d->paused) {
        d->playing = (d->_movie->state() == QMovie::Paused);
        emit pausedChanged();
    }
}

void QQuickAnimatedImage::componentComplete()
{
    Q_D(QQuickAnimatedImage);
    QQuickItem::componentComplete(); // NOT QQuickImage
    if (d->url.isValid())
        load();
    if (!d->reply) {
        setCurrentFrame(d->preset_currentframe);
        d->preset_currentframe = 0;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_MOVIE
