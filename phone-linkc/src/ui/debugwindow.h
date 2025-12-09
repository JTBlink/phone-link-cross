#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QMainWindow>

namespace Ui {
class DebugWindow;
}

class DebugWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DebugWindow(QWidget *parent = nullptr);
    ~DebugWindow();

private:
    Ui::DebugWindow *ui;
};

#endif // DEBUGWINDOW_H
