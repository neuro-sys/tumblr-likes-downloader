#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

public slots:
    void receiveTumblrImageURL(const QString &imgURL);
    void receiveTumblrImageURLFinished();

private slots:

    void on_blogNameLineEdit_textChanged(const QString &arg1);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QThread *thread;
    TumblrDownloaderWorker *tumblrDownloaderWorker;
};

#endif // MAINWINDOW_H
