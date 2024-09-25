#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>

namespace Ui {
class ImageViewer;
}

class ImageViewer : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();

    void setImage(const QPixmap &image);

    void showEvent(QShowEvent *event) override;

private:
    void configUi();

private:
    Ui::ImageViewer *ui;
};

#endif // IMAGEVIEWER_H
