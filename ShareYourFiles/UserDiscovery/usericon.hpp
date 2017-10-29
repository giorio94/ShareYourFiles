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

#ifndef USERICON_HPP
#define USERICON_HPP

#include <QSize>
#include <QString>

class QByteArray;
class QImage;

///
/// \brief The UserIcon class represents an icon chosen by a user.
///
/// Every user of Share Your Files can select an icon, that is a representative
/// image which is shared with the other peers through the SYFIT protocol. This
/// class is used to store information about those icons, and to provide the
/// main operations that can be executed on them (mainly reading and writing).
///
class UserIcon
{
public:
    ///
    /// \brief Builds a new instance with no icon associated.
    ///
    explicit UserIcon() : m_set(false) {}

    ///
    /// \brief Builds a new instance with the specified icon associated.
    /// \param confPath the base path where configuration files are stored.
    /// \param uuid the identifier of the user.
    /// \param icon the chosen image.
    ///
    explicit UserIcon(const QString &confPath, const QString &uuid,
                      const QImage &icon);

    ///
    /// \brief Builds a new instance from the data stored in a buffer.
    /// \param confPath the base path where configuration files are stored.
    /// \param uuid the identifier of the user.
    /// \param data the data representing the icon.
    /// \param hash the SHA-1 hash of the icon.
    ///
    explicit UserIcon(const QString &confPath, const QString &uuid,
                      const QByteArray &data, const QByteArray &hash);

    ///
    /// \brief Builds a new instance reading the icon from file.
    /// \param confPath the base path where configuration files are stored.
    /// \param uuid the identifier of the user.
    /// \param hash the SHA-1 hash of the icon.
    ///
    explicit UserIcon(const QString &confPath, const QString &uuid,
                      const QByteArray &hash);

    ///
    /// \brief Generates an instance which is an exact copy of the parameter.
    /// \param other the instance to be copied.
    ///
    UserIcon(const UserIcon &other) = default;

    ///
    /// \brief Copies the data from the parameter to the current instance.
    /// \param other the instance to be copied.
    ///
    UserIcon &operator=(const UserIcon &other) = default;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    ~UserIcon() = default;

    ///
    /// \brief Returns whether the icon is set or not.
    ///
    bool set() const { return m_set; }

    ///
    /// \brief Returns the SHA-1 hash of the icon (only if set is true).
    ///
    const QByteArray &hash() const { return m_hash; }

    ///
    /// \brief Returns the path indicating where the icon is stored (or a null
    /// string if the icon is not set).
    ///
    QString path() const { return (m_set) ? m_path : QString(); }

    ///
    /// \brief Reads the icon associated to this instance.
    /// \return the requested image, or a NULL one if not set or in case of
    /// error.
    ///
    QImage read() const;

    ///
    /// \brief Reads the icon associated to this instance.
    /// \return the requested image (as a byte array), or a NULL one in case of
    /// error.
    ///
    QByteArray readData() const;

    /// \brief The size of the icons (in pixel).
    static const QSize ICON_SIZE_PX;

    /// \brief The maximum size of the file storing an icon (in bytes).
    static const quint32 ICON_MAX_SIZE_BYTES;

private:
    ///
    /// \brief Reads an icon from an array of bytes.
    /// \param identifier the identifier to be printed in case of error.
    /// \param data the buffer containing the image data.
    /// \param hash the expected SHA-1 hash of the image.
    /// \return the image read, or a NULL one in case of error.
    ///
    static QImage readIcon(const QString &identifier, const QByteArray &data,
                           const QByteArray &hash);

    ///
    /// \brief Reads an icon from file.
    /// \param filePath the path where the image is stored.
    /// \param hash the expected SHA-1 hash of the image.
    /// \param buffer the optional buffer where the image (as raw bytes) is
    /// written.
    /// \return the image read, or a NULL one in case of error.
    ///
    static QImage readIcon(const QString &filePath, const QByteArray &hash,
                           QByteArray *buffer = Q_NULLPTR);

    ///
    /// \brief Writes an icon to file.
    /// \param filePath the path where the image is to be stored.
    /// \param icon the array of bytes forming the image.
    /// \return true in case of success and false of error.
    ///
    static bool saveIcon(const QString &filePath, const QByteArray &icon);

private:
    bool m_set; ///< \brief Specifies whether the icon is set or not.

    QString m_path; ///< \brief Specifies the path where the icon is stored.
    /// \brief Specifies the SHA-1 hash associated with the icon.
    QByteArray m_hash;

    /// \brief The image format chosen to save the icons.
    static const char *ICON_FORMAT;
    /// \brief The relative path (with respect to confPath), where icons are
    /// located.
    static const QString ICON_PATH;
    /// \brief The image extension.
    static const QString ICON_EXTENSION;
};

#endif // USERICON_HPP
