#include "AboutDialog.h"
#include "Painting.h"
#include <QtWidgets>


AboutDialog::AboutDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(tr("About Chronomodel"));
    
    // -----------
    
    mLabel = new QLabel();
    mLabel->setTextFormat(Qt::RichText);
    mLabel->setOpenExternalLinks(true);
    mLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mLabel->setWordWrap(true);
 
    QString text = "<b>" + qApp->applicationName() + " " + qApp->applicationVersion() + "</b><br><br>";
    text += "<a href=\"http://www.chronomodel.com\">http://www.chronomodel.com</a><br><br>";
    text += "Copyright © CNRS<br>";
    
    text += "Published in 2017<br><br>";
    text += "<b>Project director</b> : Philippe LANOS<br>";
//    text += "<b>Project co-director</b> : Anne PHILIPPE<br><br>";

    text += "<b>Authors</b> : Helori LANOS, Philippe DUFRESNE<br><br>";
    
    text += "<b>Contact</b> :<br>";
    text += "<a href=\"mailto:support@chronomodel.com \">support@chronomodel.com </a><br><br>";
    
    text += "<b>License</b> :<br>";
    text += "Chronomodel is released under the <a href=\"http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html\">CeCILL License V2.1</a><br><br>";
    
    mLabel->setText(text);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mLabel);
    setLayout(layout);
    
    setMinimumWidth(600);
}

AboutDialog::~AboutDialog()
{
    
}

void AboutDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QPixmap icon(":chronomodel.png");
    int s = 200;
    int m = 50;
    p.drawPixmap(QRect(width() - m - s, m, s, s), icon, icon.rect());
    
}
