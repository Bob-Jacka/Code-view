#ifndef CODE_INSPECTOR_INTERFACE_HPP
#define CODE_INSPECTOR_INTERFACE_HPP

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
#include <cstdlib>
#include <iostream>
#include <future>

#define UI_MSG(Window_name, Window_txt) \
QMessageBox(QMessageBox::Icon::Warning, Window_name, Window_txt).exec(); \
return;                                 \

#define FAVOURITE_COMPILER "g++"
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

//Command to execute by your system
struct Command_builder {
private:
    std::string compiler;
    std::string output_filename;
    std::string input_filename;
    std::string flag;

    //output command
    std::string command;
public:
    Command_builder &with_compiler(const std::string &new_compiler) {
        compiler = new_compiler;
        return *this;
    }

    Command_builder &with_input_filename(const std::string &new_input_filename) {
        input_filename = new_input_filename;
        return *this;
    }

    Command_builder &with_output_filename(const std::string &new_output_filename) {
        output_filename = new_output_filename;
        return *this;
    }

    Command_builder &with_flag(const std::string &new_flag_val) {
        flag = new_flag_val + " -std=c++20";
        return *this;
    }

    [[nodiscard]] std::string build() {
        command += compiler;
        command += flag;
        command += input_filename;
        command += output_filename;
        return command;
    }
};

enum class Compile_state : short {
    PREPROCESS = 0,
    ASSEMBLY = 1,
    OBJECT = 2
};

struct Results_win {
private:
    QDialog *preprocessed_win;
    QTextEdit *preproc_txt_edit;
public:
    ~Results_win() {
        delete preprocessed_win;
        delete preproc_txt_edit;
    };

    Results_win() {
        preprocessed_win = new QDialog();
        preproc_txt_edit = new QTextEdit(preprocessed_win);
    };

    void create_win() {
        preprocessed_win->setFixedSize(WINDOW_WIDHT, WINDOW_HEIGHT);
        preprocessed_win->resize(WINDOW_WIDHT, WINDOW_HEIGHT);
        preproc_txt_edit->resize(WINDOW_WIDHT, WINDOW_HEIGHT);

        preprocessed_win->show();
    }

    QTextEdit *handle_txt_edit() {
        return preproc_txt_edit;
    }
};

class MainWindow : public QMainWindow {
Q_OBJECT

private:
    QString input_filename;

    Results_win *compile_res_win;

    Compile_state state;

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_O && event->modifiers().testAnyFlags(Qt::ControlModifier)) [[likely]] {
            QString filename = QFileDialog::getOpenFileName(nullptr, "Choose File");
            if (filename.endsWith(".cpp") or filename.endsWith(".hpp")) {
                if (filename.isEmpty()) {
                    UI_MSG("Error", "Filename is empty")
                }
                compile_res_win = new Results_win();
                input_filename = filename;

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
            state = Compile_state::PREPROCESS;
            onCompile();
        } else if (event->key() == Qt::Key_S && event->modifiers().testAnyFlags(Qt::ShiftModifier)) [[likely]] {
            state = Compile_state::ASSEMBLY;
            onCompile();
        } else if (event->key() == Qt::Key_C && not event->modifiers().testAnyFlags(Qt::ShiftModifier)) {
            state = Compile_state::OBJECT;
            onCompile();
        }
    }
//private slots functionality
private slots:

    void onCompilationFinished(const QString &filename) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            compile_res_win->handle_txt_edit()->setPlainText(in.readAll());
            file.close();
        } else {
            UI_MSG("Error", "Cannot open file for read")
        }
    }

    void onCompilationError(const QString &error) {
        UI_MSG("Error", "Compilation error: " + error)
    }

    void onCompile() {
        compile_res_win->handle_txt_edit()->setPlainText("Compilation in progress");

        auto res = std::async(std::launch::async, [this]() {
            try {
                QString filename = "output.i";
                QFile file(filename);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    UI_MSG("Error", "Cannot open file for write")
                }
                file.close();

                emit compilationFinished(filename);
            } catch (const std::exception &e) {
                emit compilationError(e.what());
            }
        });
    }

signals:

    void compilationFinished(const QString &);

    void compilationError(const QString &);

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
                "Press keyboard buttons:\n 1. Shift+E - preprocessed \n 2. Shift+S - assembly \n 3. 'c' - object \n\n Ctrl+O - open file",
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
