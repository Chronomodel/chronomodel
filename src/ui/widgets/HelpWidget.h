#ifndef HelpWidget_H
#define HelpWidget_H

#include <QWidget>


class HelpWidget: public QWidget
{
    Q_OBJECT
public:
    HelpWidget(QWidget* parent = 0);
    HelpWidget(const QString& text, QWidget* parent = 0);
    
    ~HelpWidget();
    
    void setText(const QString& text);
    
    int heightForWidth(int w) const;
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    QString mText;
    QFont mFont;
};

#endif
