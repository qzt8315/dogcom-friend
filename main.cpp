#include "mainwindow.h"

#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>


bool isOnly() {
    QSharedMemory shareMem("DOGCOMAPP");
    if (!shareMem.create(1)) {
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!isOnly()) {
        QMessageBox::information(NULL, "信息", "已经有实例运行");
        qApp->quit();
        return 0;
    }

    MainWindow w;

    w.setWindowTitle("dogcom守护者");

    w.show();
    return a.exec();
}
