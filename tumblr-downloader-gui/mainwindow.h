#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

#include "tumblrdownloaderworker.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject* object, QEvent* event);

public slots:
    void receiveTumblrImageURL(const QString &imgURL);
    void receiveTumblrImageURLFinished();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QThread *thread;
    TumblrDownloaderWorker *tumblrDownloaderWorker;
    QSettings *settings;

    void saveSettings();
    void loadSettings();
};

#endif // MAINWINDOW_H
