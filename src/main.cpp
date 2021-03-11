#include "qmess.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/theme/default.qss");
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();

    QTranslator Translator;
    Translator.load("QMess_ua.qm");
    a.installTranslator(&Translator);

    Qmess w;
    w.show();

    return a.exec();
}
