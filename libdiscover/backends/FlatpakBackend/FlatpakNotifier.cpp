/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakNotifier.h"
#include "libdiscover_backend_flatpak_debug.h"
#include <UpdateConfig.h>
#include <UpdateState.h>

#include <glib.h>

#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrentRun>

using namespace std::chrono_literals;

static void installationChanged(GFileMonitor *monitor, GFile *child, GFile *other_file, GFileMonitorEvent event_type, gpointer data)
{
    Q_UNUSED(monitor);
    Q_UNUSED(child);
    Q_UNUSED(other_file);
    Q_UNUSED(event_type);

    auto notifier = static_cast<FlatpakNotifier *>(data);
    if (!notifier)
        return;

    for (const auto &installation : notifier->m_installations) {
        if (installation->m_monitor == monitor) {
            notifier->recheckSystemUpdateNeeded();
            break;
        }
    }
}

FlatpakNotifier::FlatpakNotifier(QObject *parent)
    : BackendNotifierModule(parent)
    , m_cancellable(g_cancellable_new())
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) installations = flatpak_get_system_installations(m_cancellable, &error);
    if (error) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to call flatpak_get_system_installations:" << error->message;
        g_clear_error(&error);
    }
    for (uint i = 0; installations && i < installations->len; i++) {
        auto installation = FLATPAK_INSTALLATION(g_ptr_array_index(installations, i));
        m_installations << std::make_shared<Installation>(this, installation);
    }

    g_autoptr(FlatpakInstallation) user = flatpak_installation_new_user(m_cancellable, &error);
    if (user) {
        m_installations << std::make_shared<Installation>(this, user);
    } else if (error) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to initialize the user Flatpak installation:" << error->message;
    }
}

FlatpakNotifier::Installation::Installation(FlatpakNotifier *notifier, FlatpakInstallation *installation)
    : m_notifier(notifier)
    , m_installation(installation)
{
    g_object_ref(installation);
}

FlatpakNotifier::Installation::~Installation()
{
    if (m_monitor)
        g_object_unref(m_monitor);
    if (m_installation)
        g_object_unref(m_installation);
}

FlatpakNotifier::~FlatpakNotifier()
{
    g_object_unref(m_cancellable);
}

void FlatpakNotifier::recheckSystemUpdateNeeded()
{
    const UpdateConfig::Settings settings = UpdateConfig::read();
    const int interval = settings.intervalSeconds;
    if (!settings.automaticUpdates) {
        return;
    }
    const UpdateState::State state = UpdateState::read();
    const QDateTime next = state.nextScheduledUpdate.isValid()
        ? state.nextScheduledUpdate
        : (state.lastSuccess.isValid() ? state.lastSuccess.addSecs(interval) : QDateTime());
    if (next.isValid() && next > QDateTime::currentDateTimeUtc()) {
        return;
    }

    setupFlatpakInstallations();
    loadRemoteUpdates();
}

void FlatpakNotifier::loadRemoteUpdates()
{
    if (m_checkInProgress) {
        m_recheckPending = true;
        return;
    }

    struct CheckResult {
        QList<bool> installationUpdates;
        int configuredSources = 0;
        int reachableSources = 0;
    };

    m_checkInProgress = true;
    auto fw = new QFutureWatcher<CheckResult>(this);
    const auto installations = m_installations;
    connect(fw, &QFutureWatcher<CheckResult>::finished, this, [this, fw]() {
        const CheckResult result = fw->result();
        const bool previouslyHadUpdates = hasUpdates();
        for (qsizetype i = 0; i < m_installations.size() && i < result.installationUpdates.size(); ++i) {
            m_installations[i]->m_hasUpdates = result.installationUpdates[i];
        }
        m_checkInProgress = false;
        fw->deleteLater();
        if (previouslyHadUpdates != hasUpdates()) {
            Q_EMIT foundUpdates();
        }
        Q_EMIT checkCompleted(result.configuredSources > 0, result.reachableSources > 0);
        if (m_recheckPending) {
            m_recheckPending = false;
            loadRemoteUpdates();
        }
    });
    fw->setFuture(QtConcurrent::run([installations]() -> CheckResult {
        CheckResult result;
        result.installationUpdates.reserve(installations.size());
        for (const auto &installation : installations) {
            bool hasUpdates = false;
            g_autoptr(GCancellable) cancellable = g_cancellable_new();
            g_autoptr(GError) installedError = nullptr;
            g_autoptr(GPtrArray) installedRefs =
                flatpak_installation_list_installed_refs(installation->m_installation, cancellable, &installedError);
            if (!installedRefs) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG)
                    << "Failed to list installed Flatpak refs:" << (installedError ? installedError->message : "unknown error");
            }
            g_autoptr(GError) remotesError = nullptr;
            g_autoptr(GPtrArray) remotes = flatpak_installation_list_remotes(installation->m_installation, cancellable, &remotesError);
            for (uint i = 0; remotes && i < remotes->len; ++i) {
                auto remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));
                if (flatpak_remote_get_disabled(remote)) {
                    continue;
                }
                result.configuredSources++;
                g_autoptr(GError) remoteError = nullptr;
                const char *name = flatpak_remote_get_name(remote);
                if (flatpak_installation_update_remote_sync(installation->m_installation, name, cancellable, &remoteError)) {
                    result.reachableSources++;
                    g_autoptr(GError) refsError = nullptr;
                    g_autoptr(GPtrArray) remoteRefs =
                        flatpak_installation_list_remote_refs_sync(installation->m_installation, name, cancellable, &refsError);
                    if (!remoteRefs) {
                        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG)
                            << "Unable to read Flatpak source" << name << ":" << (refsError ? refsError->message : "unknown error");
                        continue;
                    }
                    for (uint installedIndex = 0; installedRefs && !hasUpdates && installedIndex < installedRefs->len; ++installedIndex) {
                        auto installed = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedRefs, installedIndex));
                        if (g_strcmp0(flatpak_installed_ref_get_origin(installed), name) != 0) {
                            continue;
                        }
                        const QString refName = QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(installed)));
                        if (refName.endsWith(QLatin1String(".Locale")) || refName.endsWith(QLatin1String(".Debug"))) {
                            continue;
                        }
                        for (uint remoteIndex = 0; !hasUpdates && remoteIndex < remoteRefs->len; ++remoteIndex) {
                            auto available = FLATPAK_REMOTE_REF(g_ptr_array_index(remoteRefs, remoteIndex));
                            if (flatpak_ref_get_kind(FLATPAK_REF(installed)) == flatpak_ref_get_kind(FLATPAK_REF(available))
                                && g_strcmp0(flatpak_ref_get_name(FLATPAK_REF(installed)), flatpak_ref_get_name(FLATPAK_REF(available))) == 0
                                && g_strcmp0(flatpak_ref_get_arch(FLATPAK_REF(installed)), flatpak_ref_get_arch(FLATPAK_REF(available))) == 0
                                && g_strcmp0(flatpak_ref_get_branch(FLATPAK_REF(installed)), flatpak_ref_get_branch(FLATPAK_REF(available))) == 0
                                && g_strcmp0(flatpak_ref_get_commit(FLATPAK_REF(installed)), flatpak_ref_get_commit(FLATPAK_REF(available))) != 0) {
                                hasUpdates = true;
                            }
                        }
                    }
                } else {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG)
                        << "Unable to reach Flatpak source" << name << ":" << (remoteError ? remoteError->message : "unknown error");
                }
            }

            result.installationUpdates.append(hasUpdates);
        }
        return result;
    }));
}

bool FlatpakNotifier::hasUpdates()
{
    return std::ranges::any_of(m_installations, [](const auto &installation) {
        return installation->m_hasUpdates;
    });
}

bool FlatpakNotifier::Installation::ensureInitialized(GCancellable *cancellable)
{
    if (!m_monitor) {
        g_autoptr(GError) error = nullptr;
        m_monitor = flatpak_installation_create_monitor(m_installation, cancellable, &error);
        if (m_monitor) {
            g_signal_connect(m_monitor, "changed", G_CALLBACK(installationChanged), m_notifier);
        } else {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG)
                << "Failed to setup flatpak installation: " << (error ? error->message : "unknown Flatpak error");
        }
    }
    return m_installation && m_monitor;
}

void FlatpakNotifier::setupFlatpakInstallations()
{
    for (auto &installation : m_installations) {
        installation->ensureInitialized(m_cancellable);
    }
}

#include "moc_FlatpakNotifier.cpp"
