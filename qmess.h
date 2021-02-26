#ifndef QMESS_H
#define QMESS_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Qmess; }
QT_END_NAMESPACE

class Qmess : public QMainWindow
{
    Q_OBJECT

public:
    Qmess(QWidget *parent = nullptr);
    ~Qmess();

private:
    Ui::Qmess *ui;
};
#endif // QMESS_H
