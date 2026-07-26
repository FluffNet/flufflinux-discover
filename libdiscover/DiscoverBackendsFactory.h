/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"
#include <QList>
#include <QStringList>
class AbstractResourcesBackend;

class DISCOVERCOMMON_EXPORT DiscoverBackendsFactory
{
public:
    QVector<AbstractResourcesBackend *> backend(const QString &name) const;
    QVector<AbstractResourcesBackend *> allBackends() const;
    QStringList allBackendNames() const;

private:
    QVector<AbstractResourcesBackend *> backendForFile(const QString &path, const QString &name) const;
};
