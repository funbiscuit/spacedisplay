#include "inotifydialog.h"

#include <QtWidgets>

#include "utils.h"
#include <iostream>

InotifyDialog::InotifyDialog(int oldWatchLimit, int newWatchLimit)
{
    setWindowTitle("Inotify watches limit is reached");

    auto inotifyCmd = Utils::strFormat("fs.inotify.max_user_watches=%d", newWatchLimit);
    tempCmd = Utils::strFormat("sudo sysctl %s", inotifyCmd.c_str());
    permCmdArch = Utils::strFormat("echo %s | sudo tee /etc/sysctl.d/40-max-user-watches.conf"
                                   " && sudo sysctl --system", inotifyCmd.c_str());
    permCmdUbuntu = Utils::strFormat("echo %s | sudo tee -a /etc/sysctl.conf &&"
                                   " sudo sysctl -p", inotifyCmd.c_str());

    std::cout << Utils::strFormat("Current watch limit (%d) is reached, not all changes "
                                  "will be detected.\nIncrease limit to at least %d\n",
                                  oldWatchLimit, newWatchLimit);

    auto labelMsg = new QLabel();
    auto labelPermArch = new QLabel();
    auto labelPermUbuntu = new QLabel();
    labelMsg->setText(Utils::strFormat("User limit of inotify watches (%d) is reached, "
                                       "not all changes will be detected.<br/>"
                                       "Increase limit to at least %d. To increase it <b>temporary</b> execute:",
                                       oldWatchLimit, newWatchLimit).c_str());
    labelPermArch->setText("To <b>permanently</b> increase limit, if you're running <b>Arch</b>:");
    labelPermUbuntu->setText("If you are running Debian, RedHat, or another similar Linux distribution:");

    auto lineTemp = new QLineEdit();
    auto linePermArch = new QLineEdit();
    auto linePermUbuntu = new QLineEdit();
    lineTemp->setReadOnly(true);
    linePermArch->setReadOnly(true);
    linePermUbuntu->setReadOnly(true);
    lineTemp->setText(tempCmd.c_str());
    linePermArch->setText(permCmdArch.c_str());
    linePermUbuntu->setText(permCmdUbuntu.c_str());
    lineTemp->setCursorPosition(0);
    linePermArch->setCursorPosition(0);
    linePermUbuntu->setCursorPosition(0);

    auto copyTempCmd = new QPushButton("Copy");
    auto copyPermArch = new QPushButton("Copy");
    auto copyPermUbuntu = new QPushButton("Copy");
    copyTempCmd->setAutoDefault(false);
    copyPermArch->setAutoDefault(false);
    copyPermUbuntu->setAutoDefault(false);

    mainLayout = Utils::make_unique<QGridLayout>();

    mainLayout->addWidget(labelMsg, 0, 0);

    mainLayout->addWidget(lineTemp, 1, 0);
    mainLayout->addWidget(copyTempCmd, 1, 1);

    mainLayout->addWidget(labelPermArch, 2, 0);

    mainLayout->addWidget(linePermArch, 3, 0);
    mainLayout->addWidget(copyPermArch, 3, 1);

    mainLayout->addWidget(labelPermUbuntu, 4, 0);

    mainLayout->addWidget(linePermUbuntu, 5, 0);
    mainLayout->addWidget(copyPermUbuntu, 5, 1);

    setLayout(mainLayout.get());

    connect(copyTempCmd, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(tempCmd.c_str());
    });
    connect(copyPermArch, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(permCmdArch.c_str());
    });
    connect(copyPermUbuntu, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(permCmdUbuntu.c_str());
    });
}