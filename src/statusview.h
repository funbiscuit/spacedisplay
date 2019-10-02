
#ifndef SPACEDISPLAY_STATUSBAR_WIDGET_H
#define SPACEDISPLAY_STATUSBAR_WIDGET_H

#include <QWidget>
#include <QPainter>

#include <string>
#include <memory>

struct StatusPart
{
    QColor color;
    bool isHidden=false;
    float weight=0.f;
    std::string label;
    int textMargin = 5;
};

class ColorTheme;

class StatusView : public QWidget
{
Q_OBJECT
public:
    bool isDirScanned=false;
    bool showFree=false;
    bool showUnknown=false;
    bool scanOpened=false;

    void set_sizes(bool isAtRoot, float scannedVisible, float scannedHidden, float free, float unknown);
    void set_progress(bool isReady, float progress=1.f);

    void setTheme(std::shared_ptr<ColorTheme> theme);

    QSize minimumSizeHint() const override;

protected:
    float scannedSpaceVisible=0.f;
    float scannedSpaceHidden=0.f;
    float freeSpace=0.f;
    float unknownSpace=1.f;

    bool globalView=true;
    bool scanReady=false;
    float scanProgress=-1.f;

    const int textPadding = 3;

    std::shared_ptr<ColorTheme> colorTheme;

    std::vector<StatusPart> parts;

    std::string get_text_status_progress();
    void allocate_parts(const QFontMetrics& fm, float width);

    void paintEvent(QPaintEvent *event) override;
};

#endif //SPACEDISPLAY_STATUSBAR_WIDGET_H
