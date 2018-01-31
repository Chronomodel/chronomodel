#ifndef Label_H
#define Label_H

#include <QLabel>


class Label: public QLabel
{
    Q_OBJECT
public:
    explicit Label(QWidget* parent = nullptr);
    explicit Label(const QString& text, QWidget* parent = nullptr);
    
    ~Label();
    
    void setLight();
    void setDark();
    
    void setPalette(QPalette &palette);
    void setBackground(QColor color);

    void setIsTitle(bool isTitle);

    virtual void paintEvent(QPaintEvent*);
    
private:
    void init();
    
    bool mIsTitle;
    QPalette mPalette;
};

#endif
