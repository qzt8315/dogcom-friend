#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QTextCodec>
#include <QTextStream>
#include <QMessageBox>
#include <QMetaMethod>
#include <QDebug>
#include <QThread>
#include <QJsonDocument>

#define STOP "<font color=red>服务未运行</font>"
#define RUN "<font color=green>服务正在运行</font>"
#define DOGCOM "dogcom"
#define MODE "mode"
#define CONFIGFILE "file"
#define SETTINGFILE "./conf.json"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_status->setText(STOP);

    // 解析配置
    parseSetting();
    // 恢复
    recoverSetting();

    connect(ui->pushButton_Select, SIGNAL(clicked()), this, SLOT(selectConfigFile()));
    connect(ui->pushButton_SelectDogcom, SIGNAL(clicked()), this, SLOT(selectDogcomFile()));
    connect(ui->pushButton_restartService, SIGNAL(clicked()), this, SLOT(restartService()));
    connect(ui->lineEdit_configFile, SIGNAL(textChanged(const QString &)), this, SLOT(onSettingChanged()));
    connect(ui->lineEdit_dogcomPath, SIGNAL(textChanged(const QString &)), this, SLOT(onSettingChanged()));
    connect(ui->comboBox_mode, SIGNAL(currentTextChanged(const QString &)), this, SLOT(onSettingChanged()));
    ui->lineEdit_configFile->setReadOnly(true);

    initTray();

    // 检验参数
    if (validParams()) {
        hide();
        restartService();
    }
}

void MainWindow::initTray() {
    initTrayIcon();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    connect(trayShowMainAction, SIGNAL(triggered()), this, SLOT(showNormal()));
    connect(trayExitAppAction, SIGNAL(triggered()), this, SLOT(onExitAppAction()));
}

void MainWindow::initTrayIcon() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon("://icon.png"));
    trayIcon->setToolTip(QApplication::applicationName());

    trayShowMainAction = new QAction("显示主界面", this);
    trayExitAppAction = new QAction("退出", this);
    trayMenu = new QMenu();
    trayMenu->addAction(trayShowMainAction);
    trayMenu->addSeparator();
    trayMenu->addAction(trayExitAppAction);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void MainWindow::onExitAppAction() {
    QApplication::exit();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        this->show();
    }
}




void MainWindow::parseSetting() {
    QFile settingFile(SETTINGFILE);
    if (settingFile.open(QIODevice::ReadWrite)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(settingFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError) {
            setting = doc.object();
        }
        settingFile.close();
    }
}

void MainWindow::recoverSetting() {
    if (setting.contains(DOGCOM)) {
        QJsonValue val = setting.value(DOGCOM);
        if (val.isString()){
            ui->lineEdit_dogcomPath->setText(val.toString());
        }
    }
    if (setting.contains(MODE)) {
        QJsonValue val = setting.value(MODE);
        if (val.isString()){
            ui->comboBox_mode->setCurrentText(val.toString());
        }
    }
    if (setting.contains(CONFIGFILE)) {
        QJsonValue val = setting.value(CONFIGFILE);
        if (val.isString()){
            ui->lineEdit_configFile->setText(val.toString());
        }
    }
}

void MainWindow::selectConfigFile() {
    QString path = QFileDialog::getOpenFileName(this, "选择配置文件", "./", "All Files (*);;Config Files (*.conf)");
    if (path.length() != 0) {
        ui->lineEdit_configFile->setText(path);
        ui->lineEdit_configFile->setToolTip(path);
    }
}

void MainWindow::selectDogcomFile() {
    QString path = QFileDialog::getOpenFileName(this, "选择Dog", "./", "All Files (*);;EXE Files (*.exe)");
    if (path.length() != 0 && QFile::exists(path)) {
        ui->lineEdit_dogcomPath->setText(path);
        ui->lineEdit_dogcomPath->setToolTip(path);
    }
}

void MainWindow::restartService() {
    if (workProcess) {
        workProcess->kill();
        workProcess->waitForFinished();
        workProcess = NULL;
        return;
    }
    if (!validParams()) {
        QMessageBox::warning(this, "Warning", "确认配置信息是否正确");
        return;
    }
    workProcess = new QProcess(this);
    connect(workProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onSerciceExited(int, QProcess::ExitStatus)));
    connect(workProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStdOut()));

    workProcess->start(ui->lineEdit_dogcomPath->text(), QStringList() << "-m" << ui->comboBox_mode->currentText() << "-c" << ui->lineEdit_configFile->text());
    QMetaObject::invokeMethod(this, "onServiceStarted", Qt::QueuedConnection);
}

void MainWindow::onSerciceExited(int code, QProcess::ExitStatus staus) {
    Q_UNUSED(code)
    Q_UNUSED(staus)
    ui->label_status->setText(STOP);
    ui->pushButton_restartService->setText("重启服务");
}

void MainWindow::onSettingChanged() {
    setting.insert(DOGCOM, ui->lineEdit_dogcomPath->text());
    setting.insert(MODE, ui->comboBox_mode->currentText());
    setting.insert(CONFIGFILE, ui->lineEdit_configFile->text());
    QJsonDocument doc(setting);
    QFile settingFile(SETTINGFILE);
    if (settingFile.open(QIODevice::WriteOnly)) {
        settingFile.write(doc.toJson(QJsonDocument::Compact));
        settingFile.flush();
        settingFile.close();
    } else {
        QMessageBox::warning(this, "警告", "保存配置失败");
    }
}

void MainWindow::onServiceStarted() {
    ui->label_status->setText(RUN);
    ui->pushButton_restartService->setText("停止服务");
}

void MainWindow::onReadyStdError() {
    ui->textEdit_Log->setText(ui->textEdit_Log->toPlainText() + "\n" + workProcess->readAllStandardOutput());
}

bool MainWindow::validParams() {
    QString config = ui->lineEdit_configFile->text();
    QString dogcom = ui->lineEdit_dogcomPath->text();
    return ui->comboBox_mode->currentIndex()>=0 && QFile::exists(config)&&QFile::exists(dogcom);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    hide();
    event->ignore();
}

void MainWindow::onReadyReadStdOut() {
    ui->textEdit_Log->setText(ui->textEdit_Log->toPlainText() + "\n" + workProcess->readAllStandardOutput());
}

MainWindow::~MainWindow()
{
    delete ui;
}

