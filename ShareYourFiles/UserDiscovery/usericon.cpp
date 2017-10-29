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

#include "usericon.hpp"

#include <Logger.h>

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLockFile>
#include <QSaveFile>

// Static variables definition
const QSize UserIcon::ICON_SIZE_PX(128, 128);
const quint32 UserIcon::ICON_MAX_SIZE_BYTES = 16 * 1024; // 16 KB

const char *UserIcon::ICON_FORMAT = "JPG";
const QString UserIcon::ICON_PATH("/icons/");
const QString UserIcon::ICON_EXTENSION(".jpg");

///
/// This method attempts to read an icon from an array of bytes: initially
/// the SHA-1 hash is computed and compared with the expected one (to be sure
/// to actually read the expected image): in case of match, the data is used
/// to construct an image instance, which is then analyzed to verify if it
/// matches the specifications. In case of error, a null image is returned
///
QImage UserIcon::readIcon(const QString &identifier, const QByteArray &data,
                          const QByteArray &hash)
{
    // Check if the data is not empty
    if (data.isEmpty()) {
        LOG_WARNING() << "UserIcon: impossible to read the requested icon -"
                      << identifier << "- no data available";
        return QImage();
    }

    // Check if the hash corresponds
    if (QCryptographicHash::hash(data, QCryptographicHash::Algorithm::Sha1) !=
        hash) {
        LOG_WARNING() << "UserIcon: impossible to read the requested icon -"
                      << identifier << "- SHA-1 hash different from expected";
        return QImage();
    }

    // Try to load the actual image
    QImage icon = QImage::fromData(data, UserIcon::ICON_FORMAT);
    if (icon.isNull() || icon.size() != UserIcon::ICON_SIZE_PX) {
        LOG_WARNING() << "UserIcon: impossible to read the requested icon -"
                      << identifier << "- not a valid format";
        return QImage();
    }

    return icon;
}

///
/// The method attempts to read an icon from file: initially the file is
/// opened (after having obtained a lock to guarantee atomicity) and the
/// bytes are read in a buffer, which is then used to try to build the image.
/// In case of success, the image is returned and the buffer is copied into
/// the associated parameter (if not NULL) while, if an error occurs at any
/// step, a null image is returned.
///
/// \see UserIcon::readIcon(const QString&, const QByteArray&, const
/// QByteArray&)
///
QImage UserIcon::readIcon(const QString &filePath, const QByteArray &hash,
                          QByteArray *buffer)
{
    // Check if icon path exists and create it otherwise
    QDir directory = QFileInfo(filePath).absoluteDir();
    if (!directory.mkpath(".")) {
        LOG_ERROR() << "UserIcon: impossible to create directory"
                    << directory.absolutePath();
        return QImage();
    }

    // Get the lock to guarantee atomic read
    QLockFile lock(filePath + ".lock");
    if (!lock.lock()) {
        LOG_WARNING() << "UserIcon: impossible to read the requested icon -"
                      << filePath << "- failed to acquire the lock";
        return QImage();
    }

    // Attempt to read the data representing the image from file.
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        LOG_WARNING() << "UserIcon: impossible to read the requested icon -"
                      << filePath << "-" << file.errorString();
        return QImage();
    }

    // Check the file size
    if (file.size() > UserIcon::ICON_MAX_SIZE_BYTES) {
        LOG_WARNING() << "UserIcon: impossible to read the requested icon -"
                      << filePath << "- file too big";
        return QImage();
    }

    // Read data from file
    QByteArray data = file.readAll();
    file.close();
    lock.unlock();

    // Try building the image
    QImage icon = UserIcon::readIcon(filePath, data, hash);

    // If buffer set and no error, copy the data in it
    if (!icon.isNull() && buffer) {
        *buffer = data;
    }

    // Return the read icon
    return icon;
}

///
/// The method attempts to write the array of bytes representing an icon
/// to file: the file is opened (after having obtained a lock to guarantee
/// atomicity) and the bytes are written to it. A boolean value is returned
/// to signal if the operation succeeded of failed.
///
bool UserIcon::saveIcon(const QString &filePath, const QByteArray &icon)
{
    LOG_INFO() << "UserIcon: saving to" << filePath;

    // Check if icon path exists and create it otherwise
    QDir directory = QFileInfo(filePath).absoluteDir();
    if (!directory.mkpath(".")) {
        LOG_ERROR() << "UserIcon: impossible to create directory"
                    << directory.absolutePath();
        return false;
    }

    // Get the lock to guarantee atomic write
    QLockFile lock(filePath + ".lock");
    if (!lock.lock()) {
        LOG_WARNING() << "UserIcon: impossible to write the requested icon -"
                      << filePath << "- failed to acquire the lock";
        return false;
    }

    // Attempt to write the data representing the image to file.
    QSaveFile file(filePath);
    if (!file.open(QSaveFile::WriteOnly)) {
        LOG_WARNING() << "UserIcon: impossible to write the requested icon -"
                      << filePath << "-" << file.errorString();
        return false;
    }

    // Write data to file
    if (file.write(icon) != icon.length()) {
        LOG_WARNING() << "UserIcon: impossible to write the requested icon -"
                      << filePath << "-" << file.errorString();
        return false;
    }

    // Commit the data
    if (!file.commit()) {
        LOG_WARNING() << "UserIcon: impossible to write the requested icon -"
                      << filePath << "-" << file.errorString();
        return false;
    }

    return true;
}

///
/// The constructor tries to build a new UserIcon instance from an image;
/// in particular the image is converted to the expected format (scaled and
/// cropped if necessary), and then it is saved to file for later retrieval.
/// In case of success, the icon fields are initialized (the SHA-1 hash is
/// computed from the image), while in case of error an instance with the
/// icon not set is built.
///
/// \see saveIcon(const QString&, const QByteArray&)
///
UserIcon::UserIcon(const QString &confPath, const QString &uuid,
                   const QImage &icon)
        : m_set(false),
          m_path(QFileInfo(confPath + UserIcon::ICON_PATH + uuid +
                           UserIcon::ICON_EXTENSION)
                     .absoluteFilePath())
{
    LOG_ASSERT_X(!icon.isNull(),
                 "UserIcon: trying to create an instance from a NULL image");

    QImage image;
    if (icon.size() != UserIcon::ICON_SIZE_PX) {
        // In case the icon does not have the right size, scale it
        image = icon.scaled(UserIcon::ICON_SIZE_PX,
                            Qt::AspectRatioMode::KeepAspectRatioByExpanding,
                            Qt::TransformationMode::SmoothTransformation);

        // In case of icon not squared
        if (image.width() != image.height()) {
            int dx = (image.width() - UserIcon::ICON_SIZE_PX.width()) / 2;
            int dy = (image.height() - UserIcon::ICON_SIZE_PX.height()) / 2;

            // Just take the central part
            image = image.copy(dx, dy, UserIcon::ICON_SIZE_PX.width(),
                               UserIcon::ICON_SIZE_PX.height());
        }

    } else {
        // Otherwise, just copy it
        image = icon;
    }

    // Write the image to the byte array
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QBuffer::WriteOnly);
    if (!image.save(&buffer, UserIcon::ICON_FORMAT)) {
        LOG_WARNING() << "UserIcon: failed converting the image";
        return;
    }
    buffer.close();

    // Save the image to file
    if (!UserIcon::saveIcon(m_path, data)) {
        return;
    }

    m_set = true;
    // Compute the hash
    m_hash =
        QCryptographicHash::hash(data, QCryptographicHash::Algorithm::Sha1);
}

///
/// The constructor tries to build a new UserIcon instance from an array of
/// bytes (e.g. the one received from the SYFIT protocol); in particular it
/// verifies if the buffer contains valid data to build an image: in this case
/// it is saved to file for later retrieval.
/// In case of success, the icon fields are initialized, while in case of error
/// an instance with the icon not set is built.
///
UserIcon::UserIcon(const QString &confPath, const QString &uuid,
                   const QByteArray &data, const QByteArray &hash)
        : m_set(false),
          m_path(QFileInfo(confPath + UserIcon::ICON_PATH + uuid +
                           UserIcon::ICON_EXTENSION)
                     .absoluteFilePath())
{
    // Try reading the icon from the buffer
    QImage icon = UserIcon::readIcon(uuid, data, hash);

    if (icon.isNull()) {
        return;
    }

    // Save the image to file
    if (!UserIcon::saveIcon(m_path, data)) {
        return;
    }

    m_set = true;
    m_hash = hash;
}

///
/// The constructor tries to build a new UserIcon instance from a previously
/// stored image; in particular it tries to read the icon through the
/// readIcon() method: in case of success, the icon fields are initialized,
/// while in case of error an instance with the icon not set is built.
///
UserIcon::UserIcon(const QString &confPath, const QString &uuid,
                   const QByteArray &hash)
        : m_set(false),
          m_path(QFileInfo(confPath + UserIcon::ICON_PATH + uuid +
                           UserIcon::ICON_EXTENSION)
                     .absoluteFilePath())
{
    // Try reading the icon
    QImage icon = UserIcon::readIcon(m_path, hash);

    // If success, set the other fields of the structure
    if (!icon.isNull()) {
        m_set = true;
        m_hash = hash;
    }
}

///
/// The method attempts to read the image from file and returns it.
///
/// \see readIcon(const QString&, const QByteArray& hash, QByteArray* buffer)
///
QImage UserIcon::read() const
{
    // If the icon is not set return a null image
    if (!m_set) {
        return QImage();
    }

    // Otherwise read the image and return it
    return readIcon(m_path, m_hash);
}

///
/// The method attempts to read the image from file and returns it in the form
/// of an array of bytes (ready to be shared through the SYFIT protocol).
///
/// \see readIcon(const QString&, const QByteArray& hash, QByteArray* buffer)
///
QByteArray UserIcon::readData() const
{
    LOG_ASSERT_X(m_set, "UserIcon: trying to read an unset icon");
    QByteArray data;
    readIcon(m_path, m_hash, &data);
    return data;
}
