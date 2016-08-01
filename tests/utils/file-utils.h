/*
 * Copyright 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 *   Xavi Garcia <xavi.garcia.mena@gmail.com>
 */

#pragma once

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QString>

namespace FileUtils
{
    struct Info
    {
        QFileInfo info;
        QByteArray checksum;
    };

    Info createDummyFile(const QDir& dir, qint64 filesize);

    bool fillTemporaryDirectory(QString const & dir, int max_files_per_test=100, int max_filesize=1024, int max_dirs=20);

    bool compareFiles(QString const & filePath1, QString const & filePath2);

    bool compareDirectories(QString const & dir1Path, QString const & dir2Path);

    QStringList getFilesRecursively(QString const & dirPath);
};