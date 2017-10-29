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

#ifndef RECEPTIONPREFERENCES_HPP
#define RECEPTIONPREFERENCES_HPP

#include <QDate>
#include <QDir>
#include <QObject>

///
/// \brief The UserPreferences class stores the preferences regarding the file
/// reception from a given peer.
///
/// In particular, it represents the action that should be performed when a
/// sharing request is received from the current user: it is possible to
/// automatically accept or reject the transfer, or request the user
/// confirmation every time. Moreover, the default path where the received files
/// are to be stored is also provided.
///
/// In case the useDefault value is set to true, the global preferences will
/// be used instead of the ones stored in this instance (whose content is
/// actually undefined).
///
class ReceptionPreferences
{
    Q_GADGET

public:
    ///
    /// \brief The Action enum specifies the possible actions that can be taken
    /// when a new sharing request is received.
    ///
    enum class Action {
        Ask,    ///< \brief Ask to the user which action is to be performed.
        Accept, ///< \brief Automatically accept all transfers.
        Reject  ///< \brief Automatically reject all transfers.
    };
    Q_ENUM(Action)

    ///
    /// \brief Creates a new instance which uses the default preferences.
    ///
    explicit ReceptionPreferences() : m_useDefault(true) {}

    ///
    /// \brief Creates a new instance from the parameter and default values.
    /// \param path the base path where the received files are saved.
    ///
    explicit ReceptionPreferences(const QString &path)
            : ReceptionPreferences(Action::Ask, path, false, false)
    {
    }

    ///
    /// \brief Creates a new instance from the parameters.
    /// \param action the action performed when a new sharing request is
    /// received.
    /// \param path the base path where the received files are saved.
    /// \param folderUser true if a folder with the user name is added to the
    /// path.
    /// \param folderDate true if a folder with the current date is added to the
    /// path.
    ///
    explicit ReceptionPreferences(Action action, const QString &path,
                                  bool folderUser, bool folderDate)
            : m_useDefault(false),
              m_action(action),
              m_path(QDir::cleanPath(path)),
              m_folderUser(folderUser),
              m_folderDate(folderDate)
    {
    }

    ///
    /// \brief Generates an instance which is an exact copy of the parameter.
    /// \param other the instance to be copied.
    ///
    ReceptionPreferences(const ReceptionPreferences &other) = default;

    ///
    /// \brief Copies the data from the parameter to the current instance.
    /// \param other the instance to be copied.
    ///
    ReceptionPreferences &
    operator=(const ReceptionPreferences &other) = default;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    ~ReceptionPreferences() = default;

    /// \brief Returns a value specifying if the default preferences shall be
    /// used.
    bool useDefaults() const { return m_useDefault; }

    /// \brief Returns the action performed when a new sharing request is
    /// received.
    Action action() const { return m_action; }

    /// \brief Returns the base path where the received files are saved.
    const QString &path() const { return m_path; }

    /// \brief Returns a value specifying if a folder with the user name is
    /// added to the path.
    bool folderUser() const { return m_folderUser; }

    /// \brief Returns a value specifying if a folder with the current date is
    /// added to the path.
    bool folderDate() const { return m_folderDate; }

    ///
    /// \brief Returns the full data path according to the preferences.
    /// \param userName the name of the user.
    /// \return  the desired path.
    ///
    QString fullPath(const QString &userName)
    {
        if (m_useDefault) {
            return QString();
        }

        QString path = m_path;
        if (m_folderUser) {
            path.append("/" + userName);
        }
        if (m_folderDate) {
            path.append("/" + QDate::currentDate().toString("yyyyMMdd"));
        }
        return path;
    }

private:
    /// \brief Specifies if the default preferences shall be used.
    bool m_useDefault;

    /// \brief The action performed when a new sharing request is received.
    Action m_action;

    /// \brief The base path where the received files are saved.
    QString m_path;
    /// \brief Specifies if a folder with the user name is added to the path.
    bool m_folderUser;
    /// \brief Specifies if a folder with the current date is added to the path.
    bool m_folderDate;
};

#endif // RECEPTIONPREFERENCES_HPP
