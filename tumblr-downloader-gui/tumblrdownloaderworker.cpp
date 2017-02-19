#include "tumblrdownloaderworker.h"

#include <QProcess>
#include <iostream>
#include <QDir>

TumblrDownloaderWorker::TumblrDownloaderWorker(QObject *parent) : QObject(parent)
{
    process = new QProcess(this);
}

TumblrDownloaderWorker::~TumblrDownloaderWorker()
{
    delete process;
}

void TumblrDownloaderWorker::run()
{
    this->running = true;
    QDir dir;
    process->setWorkingDirectory(dir.absolutePath());
#ifdef Q_WS_WIN
    process->start("./tumblr-downloader.exe");
#else
    process->start("./tumblr-downloader");
#endif
    process->waitForReadyRead();

    char buf[1024];
    buf[0] = 0;
    do {
        process->readLine(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            emit emitImageURL(QString::fromLocal8Bit(buf).simplified());
        }
        buf[0] = 0;
    } while (!process->waitForFinished(100) && running);

    process->kill();
    emit receiveTumblrImageURLFinished();
}
