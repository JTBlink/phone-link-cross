#include "debugwindow.h"
#include "ui_debugwindow.h"

DebugWindow::DebugWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DebugWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("调试窗口"));
}

DebugWindow::~DebugWindow()
{
    delete ui;
}
