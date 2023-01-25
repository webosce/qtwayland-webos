// Copyright (c) 2020-2023 LG Electronics, Inc.
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

#include <QtWaylandCompositor/private/qwlclientbufferintegrationfactory_p.h>
#include <QtWaylandCompositor/private/qwlclientbufferintegrationplugin_p.h>
#include "../webos-wayland-egl-support/weboseglclientbufferintegration_p.h"

QT_BEGIN_NAMESPACE

class WebOSBufferIntegrationPlugin : public QtWayland::ClientBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QtWaylandClientBufferIntegrationFactoryInterface_iid FILE "webos-wayland-egl.json")
public:
    QtWayland::ClientBufferIntegration *create(const QString&, const QStringList&) override;
};

QtWayland::ClientBufferIntegration *WebOSBufferIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(system);
    return new WebOSEglClientBufferIntegration();
}

QT_END_NAMESPACE

#include "main.moc"
