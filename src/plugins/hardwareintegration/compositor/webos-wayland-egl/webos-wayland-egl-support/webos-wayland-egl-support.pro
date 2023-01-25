# Copyright (c) 2021-2023 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

TARGET = WebOSEglClientBuffer
MODULE = webos_egl_clientbuffer
load(qt_module)

QT = waylandcompositor waylandcompositor-private core-private gui-private

versionAtLeast(QT_VERSION, 6.0.0) {
    QT += wayland_egl_compositor_hw_integration-private
} else {
    LIBS += -lQt5WaylandEglClientBufferIntegration
}

SOURCES += \
    weboseglclientbufferintegration.cpp

HEADERS += \
    weboseglclientbufferintegration_p.h
