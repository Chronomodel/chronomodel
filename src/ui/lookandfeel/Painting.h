#ifndef Painting_H
#define Painting_H

#include <QtWidgets>

static QColor mainColorLight = QColor(146, 50, 154);
static QColor mainColorDark = QColor(54, 23, 106);

float pointSize(float size);

void drawButton(QPainter& painter, const QRectF& r, bool hover, bool isEnabled = true, const QString& text = QString(), const QIcon& icon = QIcon(), bool isFlat = false);
void drawButton2(QPainter& painter, const QRectF& r, bool hover, bool isEnabled = true, const QString& text = QString(), const QIcon& icon = QIcon(), bool isFlat = false);
void drawBox(QPainter& painter, const QRectF& r, const QString& text);
void drawRadio(QPainter& painter, const QRectF& r, const QString& text, bool toggled);
void drawCheckbox(QPainter& painter, const QRectF& r, const QString& text, Qt::CheckState state);
void drawCheckBoxBox(QPainter& painter, const QRectF& r, Qt::CheckState state, const QColor& back, const QColor& border);


#endif
