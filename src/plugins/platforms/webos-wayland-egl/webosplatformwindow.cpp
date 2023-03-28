// Copyright (c) 2015-2023 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "webosplatformwindow_p.h"
#include "webosscreen_p.h"

#include <webosplatform.h>
#include <webosshell.h>
#include <webosshellsurface.h>
#include <webosshellsurface_p.h>
#include <webospresentationtime.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtWaylandClient/private/qwaylandsurface_p.h>
#endif
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include <QGuiApplication>
#include <QDebug>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/private/qabstractanimation_p.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
WebOSPlatformWindow::WebOSPlatformWindow(QWindow *window,  QWaylandDisplay *display)
    : QWaylandEglWindow(window, display)
#else
WebOSPlatformWindow::WebOSPlatformWindow(QWindow *window)
    : QWaylandEglWindow(window)
#endif
    , m_autoOrientation(true)
    , mState(Qt::WindowNoState)
    , m_position(QPointF(0, 0))
{
    setWindowStateInternal(window->windowState());

    // If a client sets WEBOS_WINDOW_NO_AUTO_ORIENTATION to 1, it is assumed that
    // the client wants to change its own geometry by itself
    if (qgetenv("WEBOS_WINDOW_NO_AUTO_ORIENTATION").toInt() == 1)
        m_autoOrientation = false;

    WebOSShellSurfacePrivate* ssp = static_cast<WebOSShellSurfacePrivate *>(shellSurface());
    if (ssp && ssp->shellSurface()) {
        // Already created
        onShellSurfaceCreated(ssp->shellSurface(), this);
    } else {
        WebOSPlatform *platform = WebOSPlatform::instance();
        if (platform && platform->shell())
            connect(platform->shell(), &WebOSShell::shellSurfaceCreated, this, &WebOSPlatformWindow::onShellSurfaceCreated);
        else
            qWarning() << "Could not connect to WebOSShell::shellSurfaceCreated," << static_cast<QObject*>(this);
    }

    WebOSScreen *screen = static_cast<WebOSScreen *>(waylandScreen());
    if (screen) {
        qInfo() << "Screen set to output" << screen->outputId() << screen->name() << screen->geometry();
        QObject::connect(screen, &WebOSScreen::outputTransformChanged, this, &WebOSPlatformWindow::onOutputTransformChanged);
        QObject::connect(screen, &WebOSScreen::devicePixelRatioChanged, this, &WebOSPlatformWindow::onDevicePixelRatioChanged);
        QObject::connect(window, &QWindow::screenChanged, this, &WebOSPlatformWindow::onScreenChanged);
    }
}

#if (QT_VERSION < QT_VERSION_CHECK(5,10,0))
bool WebOSPlatformWindow::setWindowStateInternal(Qt::WindowState state)
#else
bool WebOSPlatformWindow::setWindowStateInternal(Qt::WindowStates state)
#endif
{
    if (mState == state) {
        return false;
    }

    // As of february 2013 QWindow::setWindowState sets the new state value after
    // QPlatformWindow::setWindowState returns, so we cannot rely on QWindow::windowState
    // here. We use then this mState variable.
    mState = state;

    /* TODO: Those functions are protected, so we will do more implementation to call it
     * when it is really needed.
    if (mShellSurface) {
        switch (state) {
            case Qt::WindowFullScreen:
                mShellSurface->setFullscreen();
                break;
            case Qt::WindowMaximized:
                mShellSurface->setMaximized();
                break;
            case Qt::WindowMinimized:
                mShellSurface->setMinimized();
                break;
            default:
                mShellSurface->setNormal();
        }
    }
    */

    // Not to call it to wait state_changed from compositor
    //QWindowSystemInterface::handleWindowStateChanged(window(), mState);

    return true;
}

#if (QT_VERSION < QT_VERSION_CHECK(5,10,0))
void WebOSPlatformWindow::setWindowState(Qt::WindowState state)
#else
void WebOSPlatformWindow::setWindowState(Qt::WindowStates state)
#endif
{
    setWindowStateInternal(state);

    if (m_shellSurface)
        m_shellSurface->setState(state);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void WebOSPlatformWindow::setVisible(bool visible)
{
    if (!m_initialized && visible) {
        // Call setVisible once since QWaylandWindow::setVisible(false) was ignored.
        m_initialized = true;
        QWaylandWindow::setVisible(visible);
    } else if (visible) {
        mDisplay->flushRequests();
        setGeometry(window()->geometry());
    } else {
        sendExposeEvent(QRect());
        // In webOS, hide the window from the compositor by attaching a null buffer
        // instead of destroying its resources
        QPointer<WebOSPlatformWindow> deleteGuard(this);
        QWindowSystemInterface::flushWindowSystemEvents();
        if (!deleteGuard.isNull()) {
            // Delay hiding window if waiting for the frame callback (See WebOSPlatformWindow::doHandleFrameCallback())
            if (!mWaitingForFrameCallback) {
                attach(0, 0, 0);
                mSurface->commit();
            }
        }
    }
}

void WebOSPlatformWindow::doHandleFrameCallback()
{
    QWaylandWindow::doHandleFrameCallback();

    // In webOS, send a null buffer if the window is invisible.
    if (!window()->isVisible()) {
        attach(0, 0, 0);
        mSurface->commit();
    }
}

QRect WebOSPlatformWindow::defaultGeometry() const
{
    return QRect(0, 0, 0, 0);
}
#endif

void WebOSPlatformWindow::setGeometry(const QRect &rect)
{
    bool initialize = false;
    if (m_initialGeometry.size().isEmpty() && !rect.size().isEmpty()) {
        initialize = true;
        m_initialGeometry = rect;
    }

    if (geometry().size() != rect.size())
        emit resizeRequested(geometry().size(), rect.size());

    QWaylandEglWindow::setGeometry(rect);

    // Handle transform for initial geometry
    if (initialize) {
        WebOSScreen *screen = static_cast<WebOSScreen *>(waylandScreen());
        if (screen && WebOSScreen::compareOutputTransform(0, screen->currentTransform()))
            onOutputTransformChanged();
    }
}

qreal WebOSPlatformWindow::devicePixelRatio() const
{
    if (waylandScreen())
        return static_cast<WebOSScreen *>(waylandScreen())->devicePixelRatio();
    else
        return 1.0;
}

void WebOSPlatformWindow::onShellSurfaceCreated(WebOSShellSurface *shellSurface, QPlatformWindow *window)
{
    if (shellSurface && window == this) {
        m_shellSurface = shellSurface;
        qInfo() << "shellSurface" << m_shellSurface << "was created for" << static_cast<QObject*>(this) << window;
        QObject::connect(m_shellSurface, &WebOSShellSurface::positionChanged, [this] {
            m_position = m_shellSurface->position();
            setGeometry(QRect(m_position.x(), m_position.y(), geometry().width(), geometry().height()));
            emit positionChanged(m_position);
        });
    }
}

void WebOSPlatformWindow::deliverUpdateRequest()
{
    static bool enablePresentationTime = (qEnvironmentVariableIntValue("WEBOS_PRESENTATION_TIME") == 1);

    // To synchronize animation timers
    QUnifiedTimer *ut = QUnifiedTimer::instance(false);
    if (ut)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        ut->updateAnimationTimers();
#else
        ut->updateAnimationTimers(-1);
#endif

    if (enablePresentationTime) {
        WebOSPlatform *platform = WebOSPlatform::instance();
        WebOSPresentationTime *presentation = platform ? platform->presentation() : nullptr;
        if (presentation)
            presentation->requestFeedback(this);
    }

    QWaylandEglWindow::deliverUpdateRequest();
}

void WebOSPlatformWindow::onOutputTransformChanged()
{
    if (m_autoOrientation) {
        WebOSScreen *screen = static_cast<WebOSScreen *>(waylandScreen());
        if (!screen)
            return;
        bool isOutputPortrait = screen->geometry().height() > screen->geometry().width();
        bool isWindowPortrait = geometry().height() > geometry().width();

        if (isOutputPortrait != isWindowPortrait) {
            // Swap width and height to make window and output orientation in sync
            QRect newGeometry(geometry());
            newGeometry.setWidth(geometry().height());
            newGeometry.setHeight(geometry().width());
            qInfo() << "Update platform window geometry as per screen geometry change:" << geometry() << "->" << newGeometry;
            setGeometry(newGeometry);
        } else {
            qInfo() << "Keep the platform window geometry:" << geometry() << "screen:" << screen->geometry();
        }
    } else {
        qInfo() << "No platform window geometry change as WEBOS_WINDOW_NO_AUTO_ORIENTATION is set";
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void WebOSPlatformWindow::handleMouseLeave(QWaylandInputDevice *inputDevice)
{
    if (mWindowDecoration) {
        if (mMouseEventsInContentArea)
            QWindowSystemInterface::handleLeaveEvent(window());
    } else {
        QWindowSystemInterface::handleLeaveEvent(window());
    }
}

void WebOSPlatformWindow::restoreMouseCursor(QWaylandInputDevice *device)
{
    //When App set overrideCursor apply it first.
    QCursor *cp = QGuiApplication::overrideCursor();
    QCursor c;
    if (!cp) {
        c = window()->cursor();
        cp = &c;
    }

    //Do not use qt's setCursor here. Cause App's window cursor haven't chagned,
    //it will not affect current cursor shape, that is same shape.
    waylandScreen()->waylandCursor()->changeCursor(cp, window());
}
#endif

void WebOSPlatformWindow::onDevicePixelRatioChanged()
{
    updateSurface(false);
}

void WebOSPlatformWindow::onScreenChanged(QScreen *screen)
{
    Q_UNUSED(screen);
    if (waylandScreen())
        qInfo() << "Screen changed to output" << waylandScreen()->outputId() << waylandScreen()->name() << waylandScreen()->geometry();
    onOutputTransformChanged();
    updateSurface(false);
}
