// Copyright (c) 2018-2019 LG Electronics, Inc.
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

#include <QGuiApplication>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformnativeinterface.h>
#include <QDebug>

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include "webosforeign.h"
#include "webosforeign_p.h"

using QtWaylandClient::QWaylandWindow;

WebOSForeignPrivate::WebOSForeignPrivate(QWaylandDisplay* display,
                                         uint32_t id)
    : QtWayland::wl_webos_foreign(display->wl_registry(), id, 1)
    , q_ptr(0)
{}

WebOSForeignPrivate::~WebOSForeignPrivate()
{
    q_ptr = NULL;
}

WebOSExported* WebOSForeignPrivate::export_element(QWindow* window, WebOSForeign::ExportedType exportedType)
{
    if (!isInitialized())
        return NULL;

    QWaylandWindow* qww = static_cast<QWaylandWindow*>(window->handle());
    struct ::wl_webos_exported* wlExported = export_element(qww->object(), exportedType);
    WebOSExported* exported = new WebOSExported(window);
    WebOSExportedPrivate* exported_p = WebOSExportedPrivate::get(exported);
    exported_p->init(wlExported);
    return exported;
}

WebOSImported* WebOSForeignPrivate::import_element(const QString& windowId,
                          WebOSForeign::ExportedType exportedType)
{
    if (!isInitialized())
        return NULL;

    struct ::wl_webos_imported* wlImported =
        QtWayland::wl_webos_foreign::import_element(windowId, exportedType);
    WebOSImported* imported = new WebOSImported(windowId, exportedType);
    WebOSImportedPrivate* imported_p = WebOSImportedPrivate::get(imported);
    imported_p->init(wlImported);
    return imported;
}

WebOSForeign::WebOSForeign(QWaylandDisplay* display, uint32_t id)
    : d_ptr(new WebOSForeignPrivate(display, id))
{
    Q_D(WebOSForeign);
    d->q_ptr = this;
}

WebOSForeign::~WebOSForeign()
{
}

WebOSExported* WebOSForeign::export_element(QWindow* window, ExportedType exportedType)
{
    Q_D(WebOSForeign);
    return d->export_element(window, exportedType);
}

WebOSImported* WebOSForeign::import_element(const QString& windowId,
                                            ExportedType exportedType)
{
    Q_D(WebOSForeign);
    return d->import_element(windowId, exportedType);
}

WebOSExportedPrivate::WebOSExportedPrivate(QWindow* window)
    : q_ptr(0)
    , m_window(window)
{}

WebOSExportedPrivate::~WebOSExportedPrivate()
{
    q_ptr = NULL;
}

void WebOSExportedPrivate::setExportedWindow(const QRegion &sourceRegion, const QRegion &destinationRegion)
{
    if (!isInitialized())
        return;

    m_sourceRegion = sourceRegion;
    m_destinationRegion = destinationRegion;

    QPlatformNativeInterface *wliface = QGuiApplication::platformNativeInterface();
    wl_compositor *wlcompositor = static_cast<wl_compositor *>(wliface->nativeResourceForIntegration("compositor"));
    qreal dpr = m_window->devicePixelRatio();

    wl_region* wl_source_region = wl_compositor_create_region(wlcompositor);
    Q_FOREACH (const QRect &sourceRect, m_sourceRegion.rects()) {
        wl_region_add(wl_source_region, sourceRect.x()*dpr, sourceRect.y()*dpr, sourceRect.width()*dpr, sourceRect.height()*dpr);
    }

    wl_region* wl_destination_region = wl_compositor_create_region(wlcompositor);
    Q_FOREACH (const QRect &destinationRect, m_destinationRegion.rects()) {
        wl_region_add(wl_destination_region, destinationRect.x()*dpr, destinationRect.y()*dpr, destinationRect.width()*dpr, destinationRect.height()*dpr);
    }

    set_exported_window(wl_source_region, wl_destination_region);

    wl_region_destroy(wl_source_region);
    wl_region_destroy(wl_destination_region);
}

QString WebOSExportedPrivate::getWindowId()
{
    return m_windowId;
}

WebOSForeign::ExportedType WebOSExportedPrivate::getExportedType()
{
    return m_exportedType;
}

void WebOSExportedPrivate::webos_exported_window_id_assigned(const QString &window_id, uint32_t exported_type)
{
    qInfo() << "window_id assigned:" << window_id;
    Q_Q(WebOSExported);
    m_windowId = window_id;
    m_exportedType = static_cast<WebOSForeign::ExportedType>(exported_type);
    q->window_id_assigned(m_windowId, m_exportedType);
}

WebOSExported::WebOSExported(QWindow *window)
    : d_ptr(new WebOSExportedPrivate(window))
{
    Q_D(WebOSExported);
    d->q_ptr = this;
}

WebOSExported::~WebOSExported()
{
}

void WebOSExported::setExportedWindow(const QRegion &sourceRegion, const QRegion &destinationRegion)
{
    Q_D(WebOSExported);
    return d->setExportedWindow(sourceRegion, destinationRegion);
}

QString WebOSExported::getWindowId()
{
    Q_D(WebOSExported);
    return d->getWindowId();
}

WebOSForeign::ExportedType WebOSExported::getExportedType()
{
    Q_D(WebOSExported);
    return d->getExportedType();
}

WebOSImportedPrivate::WebOSImportedPrivate(const QString& windowId,
                          WebOSForeign::ExportedType exportedType)

    : q_ptr(0)
    , m_windowId(windowId)
    , m_exportedType(exportedType)
{}

WebOSImportedPrivate::~WebOSImportedPrivate()
{
    q_ptr = NULL;
}

void WebOSImportedPrivate::requestPunchThrough()
{
    attach_punchthrough();
}

WebOSImported::WebOSImported(const QString& windowId,
                             WebOSForeign::ExportedType exportedType)
    : d_ptr(new WebOSImportedPrivate(windowId, exportedType))
{
    Q_D(WebOSImported);
    d->q_ptr = this;
}

WebOSImported::~WebOSImported()
{
}

void WebOSImported::requestPunchThrough()
{
    Q_D(WebOSImported);
    d->requestPunchThrough();
}