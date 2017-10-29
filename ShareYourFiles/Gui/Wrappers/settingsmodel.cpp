/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "settingsmodel.hpp"
#include "UserDiscovery/user.hpp"
#include "UserDiscovery/users.hpp"

#include <QDir>
#include <QImage>
#include <QUrl>

// Static variables initialization
static QMap<ReceptionPreferences::Action, QString> actionsInitializer()
{
    using Action = ReceptionPreferences::Action;
    QMap<Action, QString> actions;
    actions.insert(Action::Ask, QObject::tr("Ask"));
    actions.insert(Action::Accept, QObject::tr("Accept"));
    actions.insert(Action::Reject, QObject::tr("Reject"));
    return actions;
}
const QMap<ReceptionPreferences::Action, QString>
    SettingsModel::ACTIONS(actionsInitializer());


SettingsModel::SettingsModel(LocalUser *localUser, PeersList *peersList,
                             QObject *parent)
        : QObject(parent), m_localUser(localUser), m_peersList(peersList)
{
    connect(m_localUser, &LocalUser::modeChanged, this,
            [this](Enums::OperationalMode mode) {
                setOnline(mode == Enums::OperationalMode::Online);
            });

    resetChanges();
}

void SettingsModel::setFirstName(const QString &firstName)
{
    if (m_firstName != firstName) {
        m_firstName = firstName;
        emit firstNameChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setLastName(const QString &lastName)
{
    if (m_lastName != lastName) {
        m_lastName = lastName;
        emit lastNameChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setIconSet(bool iconSet)
{
    if (m_iconSet != iconSet) {
        m_iconSet = iconSet;
        emit iconChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setIconPath(const QString &iconPath)
{
    QString localPath = QUrl(iconPath).toLocalFile();
    if (m_iconPath != localPath) {
        m_iconPath = localPath;
        emit iconChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setOnline(bool online)
{
    if (m_online != online) {
        m_online = online;
        emit onlineChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setAction(const int action)
{
    ReceptionPreferences::Action newAction =
        *(ACTIONS.keys().constBegin() + action);
    if (m_action != newAction) {
        m_action = newAction;
        emit receptionPreferencesChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setDataPath(const QString &dataPath)
{
    if (m_dataPath != dataPath) {
        m_dataPath = dataPath;
        emit receptionPreferencesChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setDataPathUrl(const QString &dataPath)
{
    QString localPath = QUrl(dataPath).toLocalFile();
    setDataPath(localPath);
}

void SettingsModel::setFolderUser(bool folderUser)
{
    if (m_folderUser != folderUser) {
        m_folderUser = folderUser;
        emit receptionPreferencesChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setFolderDate(bool folderDate)
{
    if (m_folderDate != folderDate) {
        m_folderDate = folderDate;
        emit receptionPreferencesChanged();
        emit modifiedChanged();
    }
}

void SettingsModel::setOverwrite(bool overwrite)
{
    if (m_overwrite != overwrite) {
        m_overwrite = overwrite;
        emit receptionPreferencesChanged();
        emit modifiedChanged();
    }
}

bool SettingsModel::modified()
{
    const UserInfo &info = m_localUser->info();
    return m_firstName != info.firstName() || m_lastName != info.lastName() ||
           m_iconSet != info.icon().set() || m_iconPath != info.icon().path() ||
           m_online !=
               (m_localUser->mode() == Enums::OperationalMode::Online) ||
           m_action != info.preferences().action() ||
           m_dataPath != info.preferences().path() ||
           m_folderUser != info.preferences().folderUser() ||
           m_folderDate != info.preferences().folderDate() ||
           m_overwrite != false;
}

bool SettingsModel::saveChanges()
{
    bool success = true;

    m_localUser->setNames(m_firstName.trimmed(), m_lastName.trimmed());

    // Update the image only if changed
    const UserIcon &icon = m_localUser->info().icon();
    if (m_iconSet != icon.set() || m_iconPath != icon.path()) {
        success =
            m_localUser->setIcon((m_iconSet) ? QImage(m_iconPath) : QImage());
    }

    // Set the operational mode
    m_localUser->setMode(m_online ? Enums::OperationalMode::Online
                                  : Enums::OperationalMode::Offline);

    // Set the new preferences
    ReceptionPreferences preferences(m_action,
                                     QDir().absoluteFilePath(m_dataPath),
                                     m_folderUser, m_folderDate);
    m_localUser->setReceptionPreferences(preferences);
    if (m_overwrite) {
        m_peersList->resetReceptionPreferences();
    }

    // Reload the data from the model
    resetChanges();
    return success;
}

void SettingsModel::resetChanges()
{
    const UserInfo &info = m_localUser->info();
    m_firstName = info.firstName();
    m_lastName = info.lastName();
    m_iconSet = info.icon().set();
    m_iconPath = info.icon().path();

    m_online = m_localUser->mode() == Enums::OperationalMode::Online;

    m_action = info.preferences().action();
    m_dataPath = info.preferences().path();
    m_folderUser = info.preferences().folderUser();
    m_folderDate = info.preferences().folderDate();
    m_overwrite = false;

    emit firstNameChanged();
    emit lastNameChanged();
    emit iconChanged();
    emit onlineChanged();
    emit receptionPreferencesChanged();
    emit modifiedChanged();
}
