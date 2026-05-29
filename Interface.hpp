#ifndef CODE_INSPECTOR_INTERFACE_HPP
#define CODE_INSPECTOR_INTERFACE_HPP

#include <QWindow>
#include <QLabel>
#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QMainWindow>
#include <QMessageBox>
#include <QKeyEvent>
#include <QTextEdit>
#include <QFileDialog>
#include <tuple>
#include <fstream>

#define UI_MSG(Window_name, Window_txt) \
QMessageBox(QMessageBox::Icon::Warning, Window_name, Window_txt).exec(); \
return;                                 \

#define WINDOW_HEIGHT 580
#define WINDOW_WIDHT 970

QT_BEGIN_NAMESPACE

/**
 * Read file line by line.
 * @param fileName name of the file.
 * @return vector with lines.
 */
static std::vector<std::string> read_file(const std::string &fileName) {
    auto lines = std::vector<std::string>();
    try {
        if (std::ifstream file(fileName); file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                lines.emplace_back(line);
            }
            file.close();
            return lines;
        }
    } catch (...) {
#ifdef LIBIO_ERROR
        throw std::runtime_error("Error reading file: " + fileName);
#else
        qDebug() << "Error reading file: " << fileName << "\n";
#endif
    }
    return {};
}

class MainWindow : public QMainWindow {
Q_OBJECT
protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_O && event->modifiers().testAnyFlags(Qt::ControlModifier)) [[likely]] {
            QString filename = QFileDialog::getOpenFileName(nullptr, "Choose File");
            if (filename.endsWith(".cpp") or filename.endsWith(".hpp")) {
                if (filename.isEmpty()) {
                    UI_MSG("Error", "Filename is empty")
                }

                tx_edit->setReadOnly(false); //unlock text edit
                QTextCursor cursor(tx_edit->textCursor());
                cursor.movePosition(QTextCursor::End);
                auto file_txt = read_file(filename.toStdString());
                for (const std::string &txt_line: file_txt) {
                    tx_edit->insertPlainText(QString::fromStdString(txt_line));
                    tx_edit->insertPlainText("\n");
                }

                tx_edit->setReadOnly(true); //lock again
                return;
            } else {
                UI_MSG("Error", "Extensions are not supported")
            }
        }
        //Page handlers:
        if (event->key() == Qt::Key_E && event->modifiers().testAnyFlags(Qt::ShiftModifier)) [[likely]] {
            open_preprocessed();
        } else if (event->key() == Qt::Key_S && event->modifiers().testAnyFlags(Qt::ShiftModifier)) [[likely]] {
            open_assembly();
        } else if (event->key() == Qt::Key_C && not event->modifiers().testAnyFlags(Qt::ShiftModifier)) {
            open_object();
        }
    }
//slots
public slots:

    void open_assembly() {
        auto *assembly_win = new QWindow();
        assembly_win->resize(WINDOW_WIDHT, WINDOW_HEIGHT);
        assembly_win->setTitle("Here is assembly code");
        assembly_win->show();
    };

    void open_preprocessed() {
        auto *preprocessed_win = new QWindow();
        preprocessed_win->resize(WINDOW_WIDHT, WINDOW_HEIGHT);
        preprocessed_win->setTitle("Here is preprocessed code");
        preprocessed_win->show();
    };

    void open_object() {
        auto *object_win = new QWindow();
        object_win->resize(WINDOW_WIDHT, WINDOW_HEIGHT);
        object_win->setTitle("Here is object code");
        object_win->show();
    };

public:
    QTextEdit *tx_edit;
    QLabel *label_1;

    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setup_Ui() {
        if (this->objectName().isEmpty()) {
            this->setObjectName("MainWindow");
        }

        this->resize(WINDOW_WIDHT, WINDOW_HEIGHT);
        this->setFixedSize(WINDOW_WIDHT, WINDOW_HEIGHT); //do not resize window

        label_1 = new QLabel(
                "Press keyboard buttons:\n 1. E - preprocessed \n 2. Shift+S - assembly \n 3. Shift+c - object",
                this);
        label_1->setGeometry(QRect(WINDOW_WIDHT - 150, 0, 150, WINDOW_HEIGHT));
        label_1->move(QPoint(WINDOW_WIDHT - 150, 0));

        tx_edit = new QTextEdit(this);
        tx_edit->resize(WINDOW_WIDHT - 160, WINDOW_HEIGHT - 5);
        tx_edit->setReadOnly(true);

        menubar = new QMenuBar(this);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 960, 25));
        this->setMenuBar(menubar);
        statusbar = new QStatusBar(this);
        statusbar->setObjectName("statusbar");
        this->setStatusBar(statusbar);

        translate_Ui(this);
        QMetaObject::connectSlotsByName(this);
    }

    void translate_Ui(QMainWindow *MainWindow) const {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Code inspector", nullptr));
    }
};

QT_END_NAMESPACE

#endif
