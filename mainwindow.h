#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();

private slots:
    void slot_btn_download_click();

    void slot_btn_upload_click();

    void slot_file_upload_process(const QString &local, uint16_t rate);

    void slot_file_download_process(const QString &remote, uint16_t rate);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
