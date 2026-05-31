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

//Program settings:
#define FAVOURITE_COMPILER "g++"
#define OUTPUT_FILENAME "output"

//Window settings:
#define WINDOW_HEIGHT 580
#define WINDOW_WIDHT 970

QT_BEGIN_NAMESPACE

#define UI_MSG(Window_name, Window_txt) \
QMessageBox(QMessageBox::Icon::Warning, Window_name, Window_txt).exec(); \
return;

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

enum class Compile_state : short {
    PREPROCESS = 0,
    ASSEMBLY = 1,
    OBJECT = 2
};

//Command to execute by your system
struct Command_builder {
private:
    std::string compiler;
    std::string output_filename;
    std::string input_filename;
    std::string flag;
    Compile_state state;

    //output command
    std::string command;

public:
    Command_builder() = default;

    ~Command_builder() = default;

    [[nodiscard]] std::string get_output_name() {
        return output_filename;
    }

    Command_builder &with_compile_stage(Compile_state new_state) {
        state = new_state;
        return *this;
    }

    Command_builder &with_compiler(const std::string &new_compiler) {
        compiler = new_compiler;
        return *this;
    }

    Command_builder &with_input_filename(const std::string &new_input_filename) {
        input_filename = new_input_filename;
        return *this;
    }

    Command_builder &with_output_filename(const std::string &new_output_filename) {
        std::string file_ext;
        if (!new_output_filename.contains('.')) {
            switch (state) {
                case Compile_state::PREPROCESS:
                    file_ext += ".i";
                    break;
                case Compile_state::ASSEMBLY:
                    file_ext += ".asm";
                    break;
                case Compile_state::OBJECT:
                    file_ext += ".o";
                    break;
            }
        }
        output_filename = new_output_filename + file_ext;
        return *this;
    }

    Command_builder &with_flags(std::initializer_list<std::string> strings) {
        std::string result;
        switch (state) {
            [[likely]] case Compile_state::PREPROCESS:
                result += "-E ";
                break;
            case Compile_state::ASSEMBLY:
                result += "-S ";
                break;
            case Compile_state::OBJECT:
                result += "-c ";
                break;
        }
        for (const auto &str: strings) {
            result += str + " ";
        }
        flag = result;
        return *this;
    }

    [[nodiscard]] std::string build() {
        command += compiler + " ";
        command += flag + " ";
        command += input_filename + " ";
        command += "-o " + output_filename;
        return command;
    }
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

    void set_visible(bool visible_state) {
        preprocessed_win->setVisible(visible_state);
    }

    void show_win() {
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

    Compile_state compile_state;

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_O && event->modifiers().testAnyFlags(Qt::ControlModifier)) [[likely]] {
            QString filename = QFileDialog::getOpenFileName(nullptr, "Choose File");
            if (filename.endsWith(".cpp") or filename.endsWith(".hpp")) {
                if (filename.isEmpty()) {
                    UI_MSG("Error", "Filename is empty")
                }
                compile_res_win = new Results_win();
                compile_res_win->set_visible(false);
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
            compile_state = Compile_state::PREPROCESS;
            onCompile();
        } else if (event->key() == Qt::Key_S && event->modifiers().testAnyFlags(Qt::ShiftModifier)) [[likely]] {
            compile_state = Compile_state::ASSEMBLY;
            onCompile();
        } else if (event->key() == Qt::Key_C && not event->modifiers().testAnyFlags(Qt::ShiftModifier)) {
            compile_state = Compile_state::OBJECT;
            onCompile();
        }
    }
//private slots functionality
private slots:

    void onCompilationFinished(const QString &output_filename) {
        QFile file(output_filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            compile_res_win->handle_txt_edit()->setPlainText(in.readAll());
            file.close();
        } else {
            UI_MSG("Error", "Cannot open file for read: " + output_filename)
        }
    }

    void onCompilationError() {
        UI_MSG("Error", "Compilation error occurred")
    }

    void onCompile() {
        compile_res_win->set_visible(true);
        compile_res_win->handle_txt_edit()->setReadOnly(true);
        compile_res_win->handle_txt_edit()->setPlainText("Compilation in progress");
        compile_res_win->show_win();

        auto res = std::async(std::launch::async, [this]() -> void {
            try {
                auto command = Command_builder()
                        .with_compiler(FAVOURITE_COMPILER)
                        .with_compile_stage(compile_state)
                        .with_flags({"-std=c++23"})
                        .with_input_filename(input_filename.toStdString())
                        .with_output_filename(OUTPUT_FILENAME);

                auto build_command = command.build();

                std::system(build_command.c_str()); //compile given file
                emit compilationFinished(QString(command.get_output_name().c_str()));
            } catch (const std::exception &e) {
                emit compilationError(e.what());
            }
        });
        res.get();
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

    ~MainWindow() = default;

    MainWindow() {
        QObject::connect(
                this,
                &MainWindow::compilationFinished,
                this,
                &MainWindow::onCompilationFinished,
                Qt::AutoConnection
        );

        QObject::connect(
                this,
                &MainWindow::compilationError,
                this,
                &MainWindow::onCompilationError,
                Qt::AutoConnection
        );
    }
};

QT_END_NAMESPACE

#endif
