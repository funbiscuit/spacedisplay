#ifndef SPACEDISPLAY_LOGDIALOG_H
#define SPACEDISPLAY_LOGDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>

#include <memory>
#include <string>
#include <functional>

class Logger;

class LogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogDialog(std::shared_ptr<Logger> logger);
    ~LogDialog() override;

    /**
     * Sets callback that will be called when log data changes.
     * For example, when new data is available, or when data is viewed
     * @param callback (bool) - whether new data is available or not
     * @return
     */
    void setOnDataChanged(std::function<void(bool)> callback);

protected:
    void timerEvent(QTimerEvent *event) override;


private:

    std::unique_ptr<QVBoxLayout> mainLayout;

    std::shared_ptr<Logger> logger;
    std::shared_ptr<QTextEdit> textEdit;

    int timerId;
    std::function<void(bool)> onDataChanged;
};

#endif //SPACEDISPLAY_LOGDIALOG_H
