#include "Interface.hpp"

QT_BEGIN_NAMESPACE

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto *win = new MainWindow();
    win->setup_Ui();
    win->show();
    return QCoreApplication::exec();
}

QT_END_NAMESPACE
