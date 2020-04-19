/**
 *  Tumblr Downloader is an application that authenticates to your Tumblr
 *  account, and downloads all the liked photos to your computer.
 *  Copyright (C) 2017  Firat Salgur

 *  This file is part of Tumblr Downloader

 *  Tumblr Downloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  Tumblr Downloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Tumblr Downloader.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TUMBLRDOWNLOADERWORKER_H
#define TUMBLRDOWNLOADERWORKER_H

#include <QObject>
#include <QProcess>

class TumblrDownloaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit TumblrDownloaderWorker(QObject *parent = 0);
    ~TumblrDownloaderWorker();
    bool running;
    QProcess *process;

signals:
    void emitStatus(const QString &);
    void emitFinished();

public slots:
    void run();
};

#endif // TUMBLRDOWNLOADERWORKER_H
