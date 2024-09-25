#include "imageviewer.h"
#include "ui_imageviewer.h"

ImageViewer::ImageViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageViewer)
{
    ui->setupUi(this);
    configUi();
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

void ImageViewer::setImage(const QPixmap &image)
{
    ui->lblImage->setPixmap(image);
}

void ImageViewer::showEvent(QShowEvent *event)
{
    if (ui->lblImage->pixmap()->isNull()) {
        ui->lblImage->setText(QObject::tr("Изображение не найдено"));
    }
    else {
        ui->lblImage->setText("");
    }
    QDialog::showEvent(event);
}

void ImageViewer::configUi()
{

}
