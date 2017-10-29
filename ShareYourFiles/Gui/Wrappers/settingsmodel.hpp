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

#ifndef SETTINGSMODEL_HPP
#define SETTINGSMODEL_HPP

#include "UserDiscovery/receptionpreferences.hpp"

#include <QMap>
#include <QObject>
#include <QPointer>

class LocalUser;
class PeersList;

///
/// \brief The SettingsModel class provides the c++ model used by the Settings
/// QML window.
///
/// In particular, the class exposes a copy of the main settings related to the
/// LocalUser instance (e.g. names, icon, preferences, ...) through ad-hoc
/// properties that can be accessed from QML code. The slots in charge of either
/// saving the changes or resetting them are also provided.
///
class SettingsModel : public QObject
{
    Q_OBJECT
public:
    /// \brief Provides access to the user's first name.
    Q_PROPERTY(QString firstName READ firstName WRITE setFirstName NOTIFY
                   firstNameChanged)
    /// \brief Provides access to the user's last name.
    Q_PROPERTY(
        QString lastName READ lastName WRITE setLastName NOTIFY lastNameChanged)
    /// \brief Provides access to whether the user's icon is set or not.
    Q_PROPERTY(bool iconSet READ iconSet WRITE setIconSet NOTIFY iconChanged)
    /// \brief Provides access to the user's icon path.
    Q_PROPERTY(
        QString iconPath READ iconPath WRITE setIconPath NOTIFY iconChanged)

    /// \brief Provides access to a value indicating whether the user is
    /// currently online or not.
    Q_PROPERTY(bool online READ online WRITE setOnline NOTIFY onlineChanged)

    /// \brief Provides access the action performed when a new sharing request
    /// is received.
    Q_PROPERTY(int action READ action WRITE setAction NOTIFY
                   receptionPreferencesChanged)
    /// \brief Provides access to the base path where the received files are
    /// saved.
    Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY
                   receptionPreferencesChanged)
    /// \brief Provides access to a value specifying if a folder with the user
    /// name is added to the path when a file is received.
    Q_PROPERTY(bool folderUser READ folderUser WRITE setFolderUser NOTIFY
                   receptionPreferencesChanged)
    /// \brief Provides access to a value specifying if a folder with the
    /// current date is added to the path when a file is received.
    Q_PROPERTY(bool folderDate READ folderDate WRITE setFolderDate NOTIFY
                   receptionPreferencesChanged)

    /// \brief Provides access to whether the user specific preferences have to
    /// be overwritten.
    Q_PROPERTY(bool overwrite READ overwrite WRITE setOverwrite NOTIFY
                   receptionPreferencesChanged)

    /// \brief Provides access to whether the settings have been modified or
    /// not.
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

    ///
    /// \brief Initializes a new instance of the model.
    /// \param localUser the object representing the local user.
    /// \param peersList the object representing the list of peers.
    /// \param parent the parent of the current object.
    ///
    explicit SettingsModel(LocalUser *localUser, PeersList *peersList,
                           QObject *parent = Q_NULLPTR);


    /// \brief Returns the user's first name.
    QString firstName() const { return m_firstName; }

    /// \brief Sets the user's first name.
    /// \param firstName the new first name to be set.
    void setFirstName(const QString &firstName);

    /// \brief Returns the user's last name.
    QString lastName() const { return m_lastName; }

    /// \brief Sets the user's last name.
    /// \param lastName the new last name to be set.
    void setLastName(const QString &lastName);

    /// \brief Returns whether the user's icon is set or not.
    bool iconSet() const { return m_iconSet; }

    /// \brief Sets whether the user's icon is set or not.
    /// \param iconSet a boolean value indicating the user's choice.
    void setIconSet(bool iconSet);

    /// \brief Returns the user's icon path.
    QString iconPath() const { return m_iconPath; }

    /// \brief Sets the user's icon path.
    /// \param iconPath the new icon path to be set
    void setIconPath(const QString &iconPath);


    /// \brief Returns a value indicating whether the user is currently online
    /// or not.
    bool online() const { return m_online; }

    /// \brief Sets whether the user wants to be online or not.
    /// \param online a boolean value indicating the user's choice.
    void setOnline(bool online);


    /// \brief Returns the action performed when a new sharing request is
    /// received.
    int action() const { return ACTIONS.keys().indexOf(m_action); }

    /// \brief Sets the action performed when a new sharing request is received.
    /// \param action the index representing the action to be performed.
    void setAction(const int action);

    /// \brief Returns the base path where the received files are saved.
    QString dataPath() const { return m_dataPath; }

    /// \brief Sets the base path where the received files are saved.
    /// \param dataPath the new base data path.
    void setDataPath(const QString &dataPath);

    /// \brief Returns a value specifying if a folder with the user name is
    /// added to the path when a file is received.
    bool folderUser() const { return m_folderUser; }

    /// \brief Sets whether a folder with the user name shall be added to the
    /// path when a file is received.
    /// \param folderUser true in affirmative case and false otherwise.
    void setFolderUser(bool folderUser);

    /// \brief Returns a value specifying if a folder with the current date is
    /// added to the path when a file is received.
    bool folderDate() const { return m_folderDate; }

    /// \brief Sets whether a folder with the current date shall be added to the
    /// path when a file is received.
    /// \param folderDate true in affirmative case and false otherwise.
    void setFolderDate(bool folderDate);

    /// \brief Returns whether the user specific preferences have to be
    /// overwritten.
    bool overwrite() const { return m_overwrite; }

    /// \brief Sets whether the user specific preferences have to be
    /// overwritten.
    /// \param overwrite true in affirmative case and false otherwise.
    void setOverwrite(bool overwrite);

    /// \brief Returns whether the settings have been modified or not.
    bool modified();

signals:
    /// \brief Signal emitted when the user's first name is changed.
    void firstNameChanged();
    /// \brief Signal emitted when the user's last name is changed.
    void lastNameChanged();

    /// \brief Signal emitted when the user's icon is changed.
    void iconChanged();

    /// \brief Signal emitted when the online value is changed.
    void onlineChanged();

    /// \brief Signal emitted when some reception preferences are changed.
    void receptionPreferencesChanged();

    /// \brief Signal emitted when some value is updated.
    void modifiedChanged();

public slots:
    ///
    /// \brief Saves the performed changes to the LocalUser instance.
    /// \return true in case of success and false otherwise.
    ///
    bool saveChanges();

    ///
    /// \brief Loads the values from the LocalUser instance.
    ///
    void resetChanges();

    /// \brief Sets the base path where the received files are saved.
    /// \param dataPath the new base data path.
    void setDataPathUrl(const QString &dataPath);

    /// \brief Returns the list of strings representing the possible actions.
    /// \return a list of string associated to ReceptionPreferences::Action.
    static QStringList actionValues() { return ACTIONS.values(); }

private:
    /// \brief The instance storing data about the local user.
    QPointer<LocalUser> m_localUser;
    /// \brief The instance storing data about the peers.
    QPointer<PeersList> m_peersList;

    QString m_firstName; ///< \brief User's first name.
    QString m_lastName;  ///< \brief User's last name.

    /// \brief A value indicating whether the user's icon is set or not.
    bool m_iconSet;
    QString m_iconPath; ///< \brief User's icon path.

    /// \brief A value indicating whether the user is currently online or not.
    bool m_online;

    /// \brief The action performed when a new sharing request is received.
    ReceptionPreferences::Action m_action;
    /// \brief The base path where the received files are saved.
    QString m_dataPath;
    /// \brief Specifies if a folder with the user name is added to the path.
    bool m_folderUser;
    /// \brief Specifies if a folder with the current date is added to the path.
    bool m_folderDate;

    /// \brief Specifies whether the user specific preferences have to be
    /// overwritten.
    bool m_overwrite;

    /// \brief Provides an association between ReceptionPreferences::Action and
    /// the corresponding textual string.
    static const QMap<ReceptionPreferences::Action, QString> ACTIONS;
};

#endif // SETTINGSMODEL_HPP
