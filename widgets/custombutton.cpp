#include "custombutton.h"

#include <QString>
#include <QPainter>

CustomButton::CustomButton(QWidget *parent)
    : QWidget(parent)
{

}

void CustomButton::setText(const QString &text)
{
    m_text = text;
}

QString CustomButton::text() const
{
    return m_text;
}

void CustomButton::setIcon(const QPixmap &base, const QPixmap &hover)
{
    m_baseIcon = base;
    m_hoverIcon = hover;
}

void CustomButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const auto pixmap = m_isHoverState ? m_hoverIcon : m_baseIcon;
    painter.drawPixmap(0, 0, pixmap);
    painter.save();
    QFont font;
    font.setPixelSize(14);
    painter.setFont(font);
    painter.setPen(QColor(m_isHoverState ? "#F08224" : "#999999"));
    painter.drawText(QRect{QPoint{0, pixmap.height()}, QSize(pixmap.width(), 90)}, Qt::AlignCenter | Qt::TextWordWrap, m_text);
    painter.restore();
}

void CustomButton::enterEvent(QEvent *event)
{
    m_isHoverState = true;
    update();
    QWidget::enterEvent(event);
}

void CustomButton::leaveEvent(QEvent *event)
{
    m_isHoverState = false;
    update();
    QWidget::leaveEvent(event);
}

void CustomButton::mousePressEvent(QMouseEvent *event)
{
    emit clicked();
    QWidget::mousePressEvent(event);
}

QSize CustomButton::sizeHint() const
{
    return QSize(160, 260);
}
