/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DiscoverBackendsFactory.h"
#include "libdiscover_debug.h"
#include "resources/AbstractResourcesBackend.h"
#include "resources/ResourcesModel.h"
#include "utils.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QPluginLoader>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

QVector<AbstractResourcesBackend *> DiscoverBackendsFactory::backend(const QString &name) const
{
    if (QDir::isAbsolutePath(name) && QStandardPaths::isTestModeEnabled()) {
        return backendForFile(name, QFileInfo(name).fileName());
    } else {
        return backendForFile(name, name);
    }
}

QVector<AbstractResourcesBackend *> DiscoverBackendsFactory::backendForFile(const QString &libname, const QString &name) const
{
    QPluginLoader *loader = new QPluginLoader(QLatin1String("discover/") + libname, QCoreApplication::instance());

    if (const auto iid = loader->metaData().value("IID"_L1).toString(); iid != QLatin1StringView(DISCOVER_PLUGIN_IID)) {
        qCWarning(LIBDISCOVER_LOG) << "Plugin" << libname << "doesn't have the right IID" << iid << "expected" << DISCOVER_PLUGIN_IID;
        delete loader;
        return {};
    }

    // qCDebug(LIBDISCOVER_LOG) << "trying to load plugin:" << loader->fileName();
    AbstractResourcesBackendFactory *f = qobject_cast<AbstractResourcesBackendFactory *>(loader->instance());
    if (!f) {
        qCWarning(LIBDISCOVER_LOG) << "error loading" << libname << loader->errorString() << loader->metaData();
        delete loader;
        return {};
    }
    QElapsedTimer backendInitTime;
    backendInitTime.start();
    auto instances = f->newInstance(QCoreApplication::instance(), name);
    if (instances.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "Couldn't initialize the Flatpak backend:" << libname;
        delete loader;
        return instances;
    }

    if (backendInitTime.elapsed() > 20) {
        qDebug() << "Took" << backendInitTime.elapsed() << "ms to initialise" << name << instances.size();
    }

    return instances;
}

QStringList DiscoverBackendsFactory::allBackendNames() const
{
    return {QStringLiteral("flatpak-backend")};
}

QVector<AbstractResourcesBackend *> DiscoverBackendsFactory::allBackends() const
{
    QStringList names = allBackendNames();
    auto ret = kTransform<QVector<AbstractResourcesBackend *>>(names, [this](const QString &name) {
        return backend(name);
    });
    ret.removeAll(nullptr);

    if (ret.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "Didn't find any Discover backend!";
    }
    return ret;
}
