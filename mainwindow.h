#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QLabel>
#include <QJsonObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent* event);

public slots:
    void selectConfigFile();
    void selectDogcomFile();
    void onReadyReadStdOut();
    void onReadyStdError();
    void onSerciceExited(int, QProcess::ExitStatus);
    void onServiceStarted();
    void restartService();
    void onSettingChanged();
    void onExitAppAction();
    void iconActivated(QSystemTrayIcon::ActivationReason);

private:
    bool validParams();
    void parseSetting();
    void recoverSetting();
    void initTray();
    void initTrayIcon();

private:
    Ui::MainWindow *ui;

    QProcess* workProcess = NULL;
    QJsonObject setting;

    // 系统托盘
    QSystemTrayIcon* trayIcon;
    QMenu* trayMenu;

    QAction *trayShowMainAction;
    QAction *trayExitAppAction;
};


#endif // MAINWINDOW_H
