#ifndef HelpWidget_H
#define HelpWidget_H

#include <QWidget>

class QLabel;


class HelpWidget: public QWidget
{
    Q_OBJECT
public:
    HelpWidget(QWidget* parent = 0);
    HelpWidget(const QString& text, QWidget* parent = 0);
    void construct();
    
    ~HelpWidget();
    
    void setText(const QString& text);
    void setLink(const QString& url);
    
    int heightForWidth(int w) const;
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
private:
    QString mText;
    QLabel* mHyperLink;
    QFont mFont;
};

#endif
