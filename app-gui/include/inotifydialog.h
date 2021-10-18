#ifndef SPACEDISPLAY_INOTIFYDIALOG_H
#define SPACEDISPLAY_INOTIFYDIALOG_H

#include <QDialog>
#include <QGridLayout>

#include <memory>
#include <string>

class Logger;

class InotifyDialog : public QDialog {
Q_OBJECT

public:
    InotifyDialog(int64_t oldWatchLimit, int64_t newWatchLimit, Logger *logger);


private:

    std::unique_ptr<QGridLayout> mainLayout;

    std::string tempCmd;
    std::string permCmdArch;
    std::string permCmdUbuntu;

};

#endif //SPACEDISPLAY_INOTIFYDIALOG_H
