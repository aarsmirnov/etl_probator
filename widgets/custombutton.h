#pragma once

#include <QWidget>
#include <QPixmap>

class CustomButton : public QWidget
{
    Q_OBJECT

public:
    explicit CustomButton(QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const;

    void setIcon(const QPixmap &base, const QPixmap &hover = QPixmap());

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;

signals:
    void clicked();

private:
    QString     m_text;
    QPixmap     m_baseIcon;
    QPixmap     m_hoverIcon;
    bool        m_isHoverState { false };
};

