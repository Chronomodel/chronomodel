#include "DarkBlueStyle.h"
#include "QtUtilities.h"
#include "Painting.h"
#include <QtWidgets>


DarkBlueStyle::DarkBlueStyle()
{
    //mStyle = QStyleEventory::create("macintosh");
    mStyle = new QCommonStyle();
}

DarkBlueStyle::~DarkBlueStyle()
{
    
}

void DarkBlueStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    p->setRenderHint(QPainter::Antialiasing);
    switch(pe)
    {
        case PE_FrameButtonBevel:
        case PE_FrameButtonTool:
        case PE_PanelButtonCommand:
        case PE_PanelButtonBevel:
        case PE_PanelButtonTool:
        case PE_IndicatorButtonDropDown:
        {
            drawButton(*p, opt->rect, opt->state & (State_Sunken | State_On), opt->state & State_Enabled);
            break;
        }
        case PE_PanelMenu:
        case PE_PanelToolBar:
        case PE_PanelMenuBar:
        {
            break;
        }
        
        case PE_IndicatorCheckBox:
        {
            p->setPen(QColor(50, 50, 50));
            p->setBrush(QColor(90, 90, 90));
            p->drawRect(opt->rect);
            
            if(opt->state & State_On)
                p->setBrush(QColor(40, 160, 206));
            else
                p->setBrush(QColor(110, 110, 110));
            
            p->setPen(Qt::NoPen);
            p->drawRect(opt->rect.adjusted(2, 2, -2, -2));
            
            break;
        }
        case PE_IndicatorRadioButton:
        {
            p->setPen(QColor(50, 50, 50));
            p->setBrush(QColor(90, 90, 90));
            p->drawEllipse(opt->rect);
            
            if(opt->state & State_On)
                p->setBrush(QColor(40, 160, 206));
            else
                p->setBrush(QColor(110, 110, 110));
            
            p->setPen(Qt::NoPen);
            p->drawEllipse(opt->rect.adjusted(2, 2, -2, -2));
            
            break;
        }
        default:
        {
            mStyle->drawPrimitive(pe, opt, p, widget);
            break;
        }
    }
}



void DarkBlueStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt, QPainter* p, const QWidget* widget) const
{
    switch(cc)
    {
        case CC_GroupBox:
        {
            if(const QStyleOptionGroupBox* groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt))
            {
                // Draw frame
                //QRect textRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, widget);
                //QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxCheckBox, widget);
                //QRect frameRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxFrame, widget);
                
                QRect r = groupBox->rect;
                int titleH = 20;
                int m = 5;
                
                p->save();
                p->setPen(QColor(50, 50, 50));
                p->setBrush(QColor(75, 75, 75));
                p->drawRect(r);
                p->setBrush(QColor(50, 50, 50));
                p->drawRect(r.adjusted(0, 0, 0, titleH - r.height()));
                
                if((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty())
                {
                    p->setPen(Qt::white);
                    p->drawText(r.adjusted(m, 0, -m, titleH - r.height()),
                                Qt::AlignLeft | Qt::AlignVCenter,
                                groupBox->text);
                }
                p->restore();
                
                /*if(groupBox->subControls & QStyle::SC_GroupBoxFrame)
                {
                    QStyleOptionFrameV2 frame;
                    frame.QStyleOption::operator=(*groupBox);
                    frame.features = groupBox->features;
                    frame.lineWidth = groupBox->lineWidth;
                    frame.midLineWidth = groupBox->midLineWidth;
                    frame.rect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxFrame, widget);
                    p->save();
                    QRegion region(groupBox->rect);
                    if (!groupBox->text.isEmpty()) {
                        bool ltr = groupBox->direction == Qt::LeftToRight;
                        QRect finalRect;
                        if (groupBox->subControls & QStyle::SC_GroupBoxCheckBox) {
                            finalRect = checkBoxRect.united(textRect);
                            finalRect.adjust(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
                        } else {
                            finalRect = textRect;
                        }
                        region -= finalRect;
                    }
                    p->setClipRegion(region);
                    proxy()->drawPrimitive(PE_FrameGroupBox, &frame, p, widget);
                    p->restore();
                }
                
                // Draw title
                if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                    QColor textColor = groupBox->textColor;
                    if (textColor.isValid())
                        p->setPen(textColor);
                    int alignment = int(groupBox->textAlignment);
                    if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, opt, widget))
                        alignment |= Qt::TextHideMnemonic;
                    
                    proxy()->drawItemText(p, textRect,  Qt::TextShowMnemonic | Qt::AlignHCenter | alignment,
                                          groupBox->palette, groupBox->state & State_Enabled, groupBox->text,
                                          textColor.isValid() ? QPalette::NoRole : QPalette::WindowText);
                    
                    if (groupBox->state & State_HasFocus) {
                        QStyleOptionFocusRect fropt;
                        fropt.QStyleOption::operator=(*groupBox);
                        fropt.rect = textRect;
                        proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
                    }
                }
                
                // Draw checkbox
                if (groupBox->subControls & SC_GroupBoxCheckBox) {
                    QStyleOptionButton box;
                    box.QStyleOption::operator=(*groupBox);
                    box.rect = checkBoxRect;
                    proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, p, widget);
                }*/
            }
            break;
        }
        default:
        {
            mStyle->drawComplexControl(cc, opt, p, widget);
            break;
        }
    }
}


void DarkBlueStyle::drawControl(ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    switch(element)
    {
        case CE_ToolBar:
        {
            // This does nothing....
            //const QStyleOptionToolBar* toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(opt);
            //proxy()->drawPrimitive(PE_PanelToolBar, opt, p, widget);
            break;
        }
        case CE_RadioButton:
        {
            const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
            QStyleOptionButton subopt = *btn;
            
            subopt.rect = subElementRect(SE_RadioButtonIndicator, btn, widget);
            proxy()->drawPrimitive(PE_IndicatorRadioButton, &subopt, p, widget);
            
            subopt.rect = subElementRect(SE_RadioButtonContents, btn, widget);
            proxy()->drawControl(CE_RadioButtonLabel, &subopt, p, widget);
            
            break;
        }
        case CE_CheckBox:
        {
            const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
            QStyleOptionButton subopt = *btn;
            
            subopt.rect = subElementRect(SE_CheckBoxIndicator, btn, widget);
            proxy()->drawPrimitive(PE_IndicatorCheckBox, &subopt, p, widget);
            
            subopt.rect = subElementRect(SE_CheckBoxContents, btn, widget);
            proxy()->drawControl(CE_CheckBoxLabel, &subopt, p, widget);
            
            break;
        }
        case CE_RadioButtonLabel:
        case CE_CheckBoxLabel:
        {
            const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
            if(!btn->text.isEmpty())
            {
                p->setPen(QColor(200, 200, 200));
                p->drawText(btn->rect, Qt::AlignLeft | Qt::AlignVCenter, btn->text);
            }
            break;
        }
        case CE_PushButton:
        {
            const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
            proxy()->drawControl(CE_PushButtonBevel, btn, p, widget);
            
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            proxy()->drawControl(CE_PushButtonLabel, &subopt, p, widget);
            
            break;
        }
        case CE_PushButtonBevel:
        {
            const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
            QRect br = btn->rect;
            
            if(btn->features & QStyleOptionButton::DefaultButton)
            {
                proxy()->drawPrimitive(PE_FrameDefaultButton, opt, p, widget);
            }

            if(!(btn->features & (QStyleOptionButton::Flat | QStyleOptionButton::CommandLinkButton))
                    || btn->state & (State_Sunken | State_On)
                    || (btn->features & QStyleOptionButton::CommandLinkButton && btn->state & State_MouseOver))
            {
                QStyleOptionButton tmpBtn = *btn;
                tmpBtn.rect = br;
                proxy()->drawPrimitive(PE_PanelButtonCommand, &tmpBtn, p, widget);
            }
            break;
        }
        default:
        {
            mStyle->drawControl(element, opt, p, widget);
            break;
        }
    }
}

QRect DarkBlueStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const
{
    switch(cc)
    {
        case CC_GroupBox:
        {
            const QStyleOptionGroupBox* groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt);
            int m = 5;
            int titleH = 20;
            
            switch(sc)
            {
                case SC_GroupBoxFrame:
                {
                    return groupBox->rect;
                }
                case SC_GroupBoxContents:
                {
                    return groupBox->rect.adjusted(m, titleH + m, -m, -m);
                }
                case SC_GroupBoxCheckBox:
                {
                    int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, opt, widget);
                    int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, opt, widget);
                    
                    QRect r = groupBox->rect;
                    r.setLeft(r.left() + m);
                    r.setTop(r.top() + (titleH - indicatorHeight)/2);
                    r.setWidth(indicatorWidth);
                    r.setHeight(indicatorHeight);
                    
                    return r;
                }
                case SC_GroupBoxLabel:
                {
                    QFontMetrics fontMetrics = groupBox->fontMetrics;
                    int w = fontMetrics.size(Qt::TextShowMnemonic, groupBox->text + QLatin1Char(' ')).width();
                    int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, opt, widget);
                    int indicatorSpace = proxy()->pixelMetric(PM_CheckBoxLabelSpacing, opt, widget) - 1;
                    bool hasCheckBox = groupBox->subControls & QStyle::SC_GroupBoxCheckBox;
                    
                    QRect r = groupBox->rect;
   
                    r.setLeft(r.left() + m);
                    r.setWidth(w);
                    r.setHeight(titleH);
                    
                    if(hasCheckBox)
                        r.setLeft(r.left() + indicatorWidth + indicatorSpace);
                    
                    return r;
                }
                default:
                    break;
            }
        }
        default:
        {
            return mStyle->subControlRect(cc, opt, sc, widget);
        }
    }
}

QRect DarkBlueStyle::subElementRect(SubElement sr, const QStyleOption* opt, const QWidget* widget) const
{
    QRect r;
    switch(sr)
    {
        case SE_CheckBoxIndicator:
        case SE_RadioButtonIndicator:
        {
            int h = proxy()->pixelMetric(PM_IndicatorHeight, opt, widget);
            r.setRect(opt->rect.x()+1, opt->rect.y() + ((opt->rect.height() - h) / 2),
                      proxy()->pixelMetric(PM_IndicatorWidth, opt, widget), h);
            break;
        }
        case SE_CheckBoxContents:
        case SE_RadioButtonContents:
        {
            QRect ir = subElementRect(SE_CheckBoxIndicator, opt, widget);
            int spacing = proxy()->pixelMetric(PM_CheckBoxLabelSpacing, opt, widget);
            r.setRect(ir.right() + spacing, opt->rect.y(), opt->rect.width() - ir.width() - spacing, opt->rect.height());
            break;
        }
        default:
        {
            return mStyle->subElementRect(sr, opt, widget);
        }
    }
    return r;
}


int DarkBlueStyle::pixelMetric(PixelMetric m, const QStyleOption* opt, const QWidget* widget) const
{
    int ret;
    switch(m)
    {
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
        {
            ret = int(dpiScaled(15.));
            break;
        }
        case PM_LayoutLeftMargin:
        case PM_LayoutTopMargin:
        case PM_LayoutRightMargin:
        case PM_LayoutBottomMargin:
        case PM_LayoutHorizontalSpacing:
        case PM_LayoutVerticalSpacing:
        case PM_DefaultTopLevelMargin:
        case PM_DefaultChildMargin:
        case PM_DefaultLayoutSpacing:
        case PM_CheckBoxLabelSpacing:
        case PM_RadioButtonLabelSpacing:
        {
            ret = int(dpiScaled(5.));
            break;
        }
            
        /*case PM_FocusFrameVMargin:
        case PM_FocusFrameHMargin:
            ret = 2;
            break;
        case PM_MenuBarVMargin:
        case PM_MenuBarHMargin:
            ret = 0;
            break;
        case PM_DialogButtonsSeparator:
            ret = int(dpiScaled(5.));
            break;
        case PM_DialogButtonsButtonWidth:
            ret = int(dpiScaled(70.));
            break;
        case PM_DialogButtonsButtonHeight:
            ret = int(dpiScaled(30.));
            break;
        case PM_CheckListControllerSize:
        case PM_CheckListButtonSize:
            ret = int(dpiScaled(16.));
            break;
        case PM_TitleBarHeight: {
            if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
                if ((tb->titleBarFlags & Qt::WindowType_Mask) == Qt::Tool) {
                    ret = qMax(widget ? widget->fontMetrics().height() : opt->fontMetrics.height(), 16);
#ifndef QT_NO_DOCKWIDGET
                } else if (qobject_cast<const QDockWidget*>(widget)) {
                    ret = qMax(widget->fontMetrics().height(), int(dpiScaled(13)));
#endif
                } else {
                    ret = qMax(widget ? widget->fontMetrics().height() : opt->fontMetrics.height(), 18);
                }
            } else {
                ret = int(dpiScaled(18.));
            }
            
            break; }
        case PM_ScrollBarSliderMin:
            ret = int(dpiScaled(9.));
            break;
            
        case PM_ButtonMargin:
            ret = int(dpiScaled(6.));
            break;
            
        case PM_DockWidgetTitleBarButtonMargin:
            ret = int(dpiScaled(2.));
            break;
            
        case PM_ButtonDefaultIndicator:
            ret = 0;
            break;
            
        case PM_MenuButtonIndicator:
            ret = int(dpiScaled(12.));
            break;
            
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
            
        case PM_DefaultFrameWidth:
            ret = 2;
            break;
            
        case PM_ComboBoxFrameWidth:
        case PM_SpinBoxFrameWidth:
        case PM_MenuPanelWidth:
        case PM_TabBarBaseOverlap:
        case PM_TabBarBaseHeight:
            ret = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
            break;
            
        case PM_MdiSubWindowFrameWidth:
            ret = int(dpiScaled(4.));
            break;
            
        case PM_MdiSubWindowMinimizedWidth:
            ret = int(dpiScaled(196.));
            break;
            
#ifndef QT_NO_SCROLLBAR
        case PM_ScrollBarExtent:
            if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
                int s = sb->orientation == Qt::Horizontal ?
                QApplication::globalStrut().height()
                : QApplication::globalStrut().width();
                ret = qMax(16, s);
            } else {
                ret = int(dpiScaled(16.));
            }
            break;
#endif
        case PM_MaximumDragDistance:
            ret = -1;
            break;
            
#ifndef QT_NO_SLIDER
        case PM_SliderThickness:
            ret = int(dpiScaled(16.));
            break;
            
        case PM_SliderTickmarkOffset:
            if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
                int space = (sl->orientation == Qt::Horizontal) ? sl->rect.height()
                : sl->rect.width();
                int thickness = proxy()->pixelMetric(PM_SliderControlThickness, sl, widget);
                int ticks = sl->tickPosition;
                
                if (ticks == QSlider::TicksBothSides)
                    ret = (space - thickness) / 2;
                else if (ticks == QSlider::TicksAbove)
                    ret = space - thickness;
                else
                    ret = 0;
            } else {
                ret = 0;
            }
            break;
            
        case PM_SliderSpaceAvailable:
            if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
                if (sl->orientation == Qt::Horizontal)
                    ret = sl->rect.width() - proxy()->pixelMetric(PM_SliderLength, sl, widget);
                else
                    ret = sl->rect.height() - proxy()->pixelMetric(PM_SliderLength, sl, widget);
            } else {
                ret = 0;
            }
            break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_DOCKWIDGET
        case PM_DockWidgetSeparatorExtent:
            ret = int(dpiScaled(6.));
            break;
            
        case PM_DockWidgetHandleExtent:
            ret = int(dpiScaled(8.));
            break;
        case PM_DockWidgetTitleMargin:
            ret = 0;
            break;
        case PM_DockWidgetFrameWidth:
            ret = 1;
            break;
#endif // QT_NO_DOCKWIDGET
            
        case PM_SpinBoxSliderHeight:
        case PM_MenuBarPanelWidth:
            ret = 2;
            break;
            
        case PM_MenuBarItemSpacing:
            ret = 0;
            break;
            
#ifndef QT_NO_TOOLBAR
        case PM_ToolBarFrameWidth:
            ret = 1;
            break;
            
        case PM_ToolBarItemMargin:
            ret = 0;
            break;
            
        case PM_ToolBarItemSpacing:
            ret = int(dpiScaled(4.));
            break;
            
        case PM_ToolBarHandleExtent:
            ret = int(dpiScaled(8.));
            break;
            
        case PM_ToolBarSeparatorExtent:
            ret = int(dpiScaled(6.));
            break;
            
        case PM_ToolBarExtensionExtent:
            ret = int(dpiScaled(12.));
            break;
#endif // QT_NO_TOOLBAR
            
#ifndef QT_NO_TABBAR
        case PM_TabBarTabOverlap:
            ret = 3;
            break;
            
        case PM_TabBarTabHSpace:
            ret = int(dpiScaled(24.));
            break;
            
        case PM_TabBarTabShiftHorizontal:
            ret = 0;
            break;
            
        case PM_TabBarTabShiftVertical:
            ret = 2;
            break;
            
        case PM_TabBarTabVSpace: {
            const QStyleOptionTab *tb = qstyleoption_cast<const QStyleOptionTab *>(opt);
            if (tb && (tb->shape == QTabBar::RoundedNorth || tb->shape == QTabBar::RoundedSouth
                       || tb->shape == QTabBar::RoundedWest || tb->shape == QTabBar::RoundedEast))
                ret = 8;
            else
                if(tb && (tb->shape == QTabBar::TriangularWest || tb->shape == QTabBar::TriangularEast))
                    ret = 3;
                else
                    ret = 2;
            break; }
#endif
            
        case PM_ProgressBarChunkWidth:
            ret = 9;
            break;
            
        case PM_ExclusiveIndicatorWidth:
            ret = int(dpiScaled(12.));
            break;
            
        case PM_ExclusiveIndicatorHeight:
            ret = int(dpiScaled(12.));
            break;
            
        case PM_MenuTearoffHeight:
            ret = int(dpiScaled(10.));
            break;
            
        case PM_MenuScrollerHeight:
            ret = int(dpiScaled(10.));
            break;
            
        case PM_MenuDesktopFrameWidth:
        case PM_MenuHMargin:
        case PM_MenuVMargin:
            ret = 0;
            break;
            
        case PM_HeaderMargin:
            ret = int(dpiScaled(4.));
            break;
        case PM_HeaderMarkSize:
            ret = int(dpiScaled(32.));
            break;
        case PM_HeaderGripMargin:
            ret = int(dpiScaled(4.));
            break;
        case PM_TabBarScrollButtonWidth:
            ret = int(dpiScaled(16.));
            break;
            
        case PM_ToolBarIconSize:
            ret = qt_guiPlatformPlugin()->platformHint(QGuiPlatformPlugin::PH_ToolBarIconSize);
            if (!ret)
                ret = int(dpiScaled(24.));
            break;
            
        case PM_TabBarIconSize:
        case PM_ListViewIconSize:
            ret = proxy()->pixelMetric(PM_SmallIconSize, opt, widget);
            break;
            
        case PM_ButtonIconSize:
        case PM_SmallIconSize:
            ret = int(dpiScaled(16.));
            break;
        case PM_IconViewIconSize:
            ret = proxy()->pixelMetric(PM_LargeIconSize, opt, widget);
            break;
            
        case PM_LargeIconSize:
            ret = int(dpiScaled(32.));
            break;
            
        case PM_ToolTipLabelFrameWidth:
            ret = 1;
            break;
        case PM_SizeGripSize:
            ret = int(dpiScaled(13.));
            break;
        case PM_MessageBoxIconSize:
#ifdef Q_WS_MAC
            if (QApplication::desktopSettingsAware()) {
                ret = 64; // No DPI scaling, it's handled elsewhere.
            } else
#endif
            {
                ret = int(dpiScaled(32.));
            }
            break;
        case PM_TextCursorWidth:
            ret = 1;
            break;
        case PM_TabBar_ScrollButtonOverlap:
            ret = 1;
            break;
        case PM_TabCloseIndicatorWidth:
        case PM_TabCloseIndicatorHeight:
            ret = int(dpiScaled(16.));
            break;
        case PM_ScrollView_ScrollBarSpacing:
            ret = 2 * proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
            break;
        case PM_SubMenuOverlap:
            ret = -proxy()->pixelMetric(QStyle::PM_MenuPanelWidth, opt, widget);
            break;*/
        default:
            return mStyle->pixelMetric(m, opt, widget);
            ret = 0;
            break;
    }
    
    return ret;
}
