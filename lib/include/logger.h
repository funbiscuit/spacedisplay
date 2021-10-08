#ifndef SPACEDISPLAY_LOGGER_H
#define SPACEDISPLAY_LOGGER_H

#include <string>
#include <vector>
#include <mutex>
#include <atomic>

class Logger {

public:

    Logger();

    /**
     * Creates new log entry with tag (by default `LOG`) and adds it to history.
     * Sets hasNew property to true
     * @param msg
     * @param tag
     */
    void log(const std::string &msg, const std::string &tag = "LOG");

    /**
     * Returns all log entries in history and sets hasNew property to false
     * @return
     */
    std::vector<std::string> getHistory();

    /**
     * Clears log history and sets hasNew property to false
     */
    void clear();

    /**
     * Return true if any entries were added since last call to getHistory
     * @return
     */
    bool hasNew();

private:

    std::mutex logMtx;

    std::atomic<bool> hasNewLog;

    std::vector<std::string> history;

};

#endif //SPACEDISPLAY_LOGGER_H
