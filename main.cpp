#include <QApplication>
#include <QWidget>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QProcess>
#include <QLabel>
#include <QTimer>
#include <QSerialPortInfo>
#include <QSet>

class AvrdudeFrontend : public QWidget {
    Q_OBJECT

public:
    AvrdudeFrontend(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("AVRDUDE Frontend");

        auto *layout = new QGridLayout(this);

        // Programmer
        layout->addWidget(new QLabel("Programmer:"), 0, 0);
        programmerBox = new QComboBox();
        programmerBox->addItems({"arduino","usbasp",  "avrisp", "usbtiny"});
        layout->addWidget(programmerBox, 0, 1);

        // MCU
        layout->addWidget(new QLabel("MCU:"), 1, 0);
        mcuBox = new QComboBox();
        mcuBox->addItems({"atmega328p", "attiny85", "atmega16"});
        layout->addWidget(mcuBox, 1, 1);

        // Port
        layout->addWidget(new QLabel("Port:"), 2, 0);
        portBox = new QComboBox();
        layout->addWidget(portBox, 2, 1);

        // Baud
        layout->addWidget(new QLabel("Baud Rate:"), 3, 0);
        baudEdit = new QLineEdit("115200");
        layout->addWidget(baudEdit, 3, 1);

        // HEX file
        layout->addWidget(new QLabel("HEX File:"), 4, 0);
        hexEdit = new QLineEdit();
        layout->addWidget(hexEdit, 4, 1);
        QPushButton *browseButton = new QPushButton("Browse");
        layout->addWidget(browseButton, 4, 2);
        connect(browseButton, &QPushButton::clicked, this, &AvrdudeFrontend::browseHex);

        // Upload
        QPushButton *uploadButton = new QPushButton("Upload");
        layout->addWidget(uploadButton, 5, 1);
        connect(uploadButton, &QPushButton::clicked, this, &AvrdudeFrontend::upload);

        // Output
        output = new QTextEdit();
        output->setReadOnly(true);
        layout->addWidget(output, 6, 0, 1, 3);

        // Timer to update ports
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AvrdudeFrontend::updatePorts);
        timer->start(2000);
        updatePorts(); // Initial fill
    }

private slots:
    void browseHex() {
        QString fileName = QFileDialog::getOpenFileName(this, "Select HEX File", "", "HEX files (*.hex)");
        if (!fileName.isEmpty())
            hexEdit->setText(fileName);
    }

    void upload() {
        QString avrdudePath = QApplication::applicationDirPath() + "/avrdude";
        QStringList args;

        args << "-c" + programmerBox->currentText();
        args << "-p" + mcuBox->currentText();

        if (!portBox->currentText().isEmpty())
            args << "-P" + portBox->currentText();

        if (!baudEdit->text().isEmpty())
            args << "-b" + baudEdit->text();

        args << "-Uflash:w:" + hexEdit->text() + ":i";

        QProcess *proc = new QProcess(this);

        output->clear(); // Clear previous logs
        output->append("Running: " + avrdudePath + " " + args.join(" "));

        connect(proc, &QProcess::readyReadStandardOutput, [this, proc]() {
            output->append(QString::fromUtf8(proc->readAllStandardOutput()));
        });

        connect(proc, &QProcess::readyReadStandardError, [this, proc]() {
            output->append(QString::fromUtf8(proc->readAllStandardError()));
        });

        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, proc](int exitCode, QProcess::ExitStatus status) {
            output->append(QString("\nProcess exited with code %1").arg(exitCode));
            proc->deleteLater();
        });

        proc->start(avrdudePath, args);
    }


    void updatePorts() {
        QStringList currentPorts;

#if defined(Q_OS_WIN)
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
            currentPorts << info.portName();  // COM3, COM4, etc.
        }
#else
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
            QString name = info.systemLocation(); // /dev/ttyUSB0 etc.
             #ifdef __APPLE__
            if (name.contains("tty.usb") || name.contains("cu.usb"))
                currentPorts << name;
#else
            if (name.contains("ttyUSB") || name.contains("ttyACM"))
                currentPorts << name;
#endif
        }
#endif

        // Compare with existing items in the port box
        QSet<QString> currentSet = QSet<QString>::fromList(currentPorts);
        QSet<QString> existingSet;
        for (int i = 0; i < portBox->count(); ++i)
            existingSet.insert(portBox->itemText(i));

        if (currentSet != existingSet) {
            portBox->clear();
            portBox->addItems(currentPorts);
        }
    }

private:
    QComboBox *programmerBox;
    QComboBox *mcuBox;
    QComboBox *portBox;
    QLineEdit *baudEdit;
    QLineEdit *hexEdit;
    QTextEdit *output;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AvrdudeFrontend w;
    w.resize(600, 400);
    w.show();
    return app.exec();
}
