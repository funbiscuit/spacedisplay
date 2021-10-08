#include "logdialog.h"

#include <QtWidgets>

#include "utils.h"
#include "logger.h"

#include <sstream>

LogDialog::LogDialog(std::shared_ptr<Logger> logger) : logger(std::move(logger)) {
    setWindowTitle("SpaceDisplay Log");
    setMinimumSize(500, 350);
    setFocusPolicy(Qt::ClickFocus);

    textEdit = std::make_shared<QTextEdit>();
    textEdit->setReadOnly(true);

    auto clearLog = new QPushButton("Clear");
    clearLog->setAutoDefault(false);

    mainLayout = Utils::make_unique<QVBoxLayout>();

    mainLayout->addWidget(textEdit.get(), 1);
    mainLayout->addWidget(clearLog, 0, Qt::AlignRight);

    setLayout(mainLayout.get());

    connect(clearLog, &QPushButton::clicked, this, [this]() {
        textEdit->clear();
        if (this->logger)
            this->logger->clear();
    });

    timerId = startTimer(100);
}

LogDialog::~LogDialog() {
    killTimer(timerId);
}

void LogDialog::timerEvent(QTimerEvent *event) {
    if (!logger)
        return;

    if (logger->hasNew()) {
        std::stringstream str;
        auto logs = logger->getHistory();
        for (auto &log : logs)
            str << log << '\n';
        textEdit->setPlainText(str.str().c_str());
        textEdit->moveCursor(QTextCursor::End);
        if (onDataChanged)
            onDataChanged(true);
    }
}

void LogDialog::setOnDataChanged(std::function<void(bool)> callback) {
    onDataChanged = std::move(callback);
}
