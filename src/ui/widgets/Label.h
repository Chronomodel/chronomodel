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
    void setAdjustText(bool ad = true);

    
private:
    void init();
    void adjustFont();
    bool mIsTitle;
    bool mAdjustText;
    QPalette mPalette;

protected:
    void resizeEvent(QResizeEvent* e);
    virtual void paintEvent(QPaintEvent*);
};

#endif
