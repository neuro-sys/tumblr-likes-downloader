#ifndef TUMBLRDOWNLOADERWORKER_H
#define TUMBLRDOWNLOADERWORKER_H

#include <QObject>

class TumblrDownloaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit TumblrDownloaderWorker(QObject *parent = 0);
    bool running;

signals:
    void emitImageURL(const QString &);
    void receiveTumblrImageURLFinished();

public slots:
    void run();
};

#endif // TUMBLRDOWNLOADERWORKER_H
