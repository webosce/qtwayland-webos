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

#include <QDebug>
#include <qpa/qplatformintegrationplugin.h>
#include "weboseglplatformintegration.h"

QT_BEGIN_NAMESPACE

class WebOSIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "webos-wayland-egl.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration *WebOSIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(system);
    auto *integration = new WebOSEglPlatformIntegration();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 1)
    if (!integration->init()) {
#else
    if (integration->hasFailed()) {
#endif
        qCritical() << "Failed to initialize WebOSEglPlatformIntegration, exiting";
        ::exit(1);
    }

    return integration;
}

QT_END_NAMESPACE

#include "main.moc"
