/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "SwitchAction.h"
#include "Painting.h"
#include <QtWidgets>

SwitchWidget::SwitchWidget(QWidget* parent):QWidget(parent),
    mAction(nullptr),
    mText ("None"),
    mToggled(false)
{
    //setFixedSize(90, 40);

}

SwitchWidget::SwitchWidget(QWidget* parent, QWidgetAction* action):QWidget(parent),
    mAction(action),
    mToggled(false)
{
    //setFixedSize(90, 40);
}

void SwitchWidget::setToggled(bool isToggled)
{
    if (isToggled != mToggled) {
        mToggled = isToggled;
        if (mAction) {
            mAction->toggle();
        }
        emit toggled(isToggled);
        update();
    }
}

void SwitchWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    setToggled(!mToggled);
}


void SwitchWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect();
    r.adjust(1, 1, -1, -1);
    
    QColor colorBorder = mToggled ? CURVE_COLOR_BORDER : CHRONOMODEL_COLOR_BORDER;
    QColor colorBack = mToggled ? CURVE_COLOR_BACK : CHRONOMODEL_COLOR_BACK;
    QColor colorText = mToggled ? CURVE_COLOR_TEXT : CHRONOMODEL_COLOR_TEXT;
    
    QPen pen = painter.pen();
    pen.setColor(colorBorder);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(colorBack);
    painter.drawRoundedRect(r, 4, 4);
    
    pen.setColor(colorText);
    painter.setPen(pen);
    
    QFont f1(font());
    f1.setPointSizeF(8);
    painter.setFont(f1);
    //QFontMetrics fm1(f1);
    QString text1 = tr("Functional Link");
    //int text1W = fm1.horizontalAdvance(text1);
    
    QFont f2(font());
    f2.setPointSizeF(14);
    f2.setWeight(QFont::Bold);
    painter.setFont(f2);
    //QFontMetrics fm2(f2);
   // QString text2 = mToggled ? "CURVE" : "PHASES";
   // int text2W = fm2.horizontalAdvance(text2);
    
    //int textW = text1W + text2W;;
    
    painter.setFont(f1);
    painter.drawText(r.adjusted(0, 5, 0, -2*r.height()/3 + 2), Qt::AlignHCenter | Qt::AlignVCenter, text1);
    
    painter.setFont(f2);
    painter.drawText(r.adjusted(0, r.height()/3, 0, 0), Qt::AlignHCenter | Qt::AlignVCenter, mText);
}


SwitchAction::SwitchAction(QObject* parent):QWidgetAction(parent)
{
    
}

QWidget* SwitchAction::createWidget(QWidget* parent)
{
    return new SwitchWidget(parent, this);
    //return QWidgetAction::createWidget(parent);
}
