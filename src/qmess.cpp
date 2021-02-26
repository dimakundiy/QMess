#include "qmess.h"
#include "ui_qmess.h"

Qmess::Qmess(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Qmess)
{
    ui->setupUi(this);
}

Qmess::~Qmess()
{
    delete ui;
}

