#ifndef EventDialog_H
#define EventDialog_H

#include <QDialog>
#include "Event.h"

class QLabel;
class LineEdit;
class QComboBox;
class ColorPicker;


class EventDialog: public QDialog
{
    Q_OBJECT
public:
    EventDialog(QWidget* parent, const QString& title, Qt::WindowFlags flags = 0);
    ~EventDialog();
    
    QString getName() const;
    QColor getColor() const;
    
public:
    LineEdit* mNameEdit;
    ColorPicker* mColorPicker;
};

#endif
