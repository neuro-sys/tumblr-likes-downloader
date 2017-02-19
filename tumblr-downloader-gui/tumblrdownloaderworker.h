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
    void emitImageURL(const QString &);
    void receiveTumblrImageURLFinished();

public slots:
    void run();
};

#endif // TUMBLRDOWNLOADERWORKER_H
