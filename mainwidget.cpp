#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QFile>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QButtonGroup>
#include <QLineEdit>
#include <QToolButton>
#include <QMetaEnum>
#include <QButtonGroup>
#include <QRect>
#include <QDesktopWidget>
#include <QProcess>
#include <QStyle>
#include <QMenuBar>
#include <QDesktopServices>
#include <QUrl>

#include "core.h"
#include "logfile.h"

#include "widgets/custombutton.h"

#include "devices/device_iks30a.h"
#include "devices/device_t2000.h"

namespace  {

//#ifdef Q_OS_WIN32
//#include "Windows.h"

//HWND FindTopWindow(DWORD pid)
//{
//    std::pair<HWND, DWORD> params = { 0, pid };

//    // Enumerate the windows using a lambda to process each window
//    BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
//    {
//        auto pParams = (std::pair<HWND, DWORD>*)(lParam);

//        DWORD processId;
//        if (GetWindowThreadProcessId(hwnd, &processId) && processId == pParams->second)
//        {
//            // Stop enumerating
//            SetLastError(-1);
//            pParams->first = hwnd;
//            return FALSE;
//        }

//        // Continue enumerating
//        return TRUE;
//    }, (LPARAM)&params);

//    if (!bResult && GetLastError() == -1 && params.first)
//    {
//        return params.first;
//    }

//    return nullptr;
//}

//void SetExternalWindowPos(HWND wnd, const QRect& geometry)
//{
//    auto ok = SetWindowPos(wnd, (HWND)-1, geometry.left(), geometry.top(), geometry.width(), geometry.height(), SWP_SHOWWINDOW);
//    if (!ok) {
//        Logger.Log("SetExternalWindowPos failed");
//        return;
//    }
//    UpdateWindow(wnd);
//}

//void MoveAuxWindow(qint64 pid, const QRect& geometry)
//{
//    auto wnd = FindTopWindow((DWORD)pid);
//    if (wnd == nullptr) {
//        Logger.Log("FindTopWindow failed");
//        return;
//    }
//    SetExternalWindowPos(wnd, geometry);
//}

//#endif


void UpdateStyle(QWidget* w)
{
    w->style()->unpolish(w); // this is a special hack
    w->style()->polish(w);   // for updating stylesheet
}

}

Device *createDevice(const QString &name)
{
    QVariantMap params;
    Device *device { nullptr };

    if (name == "Измерение омического сопротивления ИКС-30А") {
        device = new Device_IKS30A(name);
    }
    if (name == "Тангенс прямая") {
        device = new Device_T2000(name);
        params["mode"] = Device_T2000::Direct;
    }
    if (name == "Тангенс инверсная") {
        device = new Device_T2000(name);
        params["mode"] = Device_T2000::Inverse;
    }

    if (device != nullptr) {
        device->setParams(params);
    }

    return device;
}


#include <QDebug>

class MainWidgetPrivate
{
    friend MainWidget;
    Q_DECLARE_PUBLIC(MainWidget)

    MainWidgetPrivate(QSharedPointer<Core> core, MainWidget* ownerPtr)
        : q_ptr(ownerPtr)
        , ui(new Ui::MainWidget)
        , m_core(core)
    {
    }

    MainWidget * const                  q_ptr;
    QScopedPointer<Ui::MainWidget>      ui;
    QSharedPointer<Core>                m_core;

    QMap<Core::Blocks, QToolButton*>    m_indications;
    QSet<Core::Blocks>                  m_enabled_blocks;
    bool                                m_start_is_available = false;
    bool                                m_voltage_is_on = false;

    qreal                               m_scheme_scale = 1.0;
    QProcess*                           m_aux_process = nullptr;
    int                                 m_last_probation = -1;

    QString                             m_currentTest;
    QString                             m_currentSchema;

    void configUI()
    {
//        m_indications[Core::Blocks::VOLTAGE] = ui->toolButton_1;
//        m_indications[Core::Blocks::CURRENT] = ui->toolButton_2;
//        m_indications[Core::Blocks::DOORS] = ui->toolButton_3;
//        m_indications[Core::Blocks::EMERGENCY_BUTTON] = ui->toolButton_4;
//        m_indications[Core::Blocks::GND] = ui->toolButton_5;
//        m_indications[Core::Blocks::WORK_GND] = ui->toolButton_6;

        //ui->voltageLabel->hide();

        QFile styleFile(":/main/stylesheet.css");
        styleFile.open(QFile::ReadOnly);
        QString stylesheet = styleFile.readAll();
        styleFile.close();

        Q_Q(MainWidget);
        q->setStyleSheet(stylesheet);

        QMenuBar* menuBar = new QMenuBar();
        menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QMenu *testsMenu = new QMenu(QObject::tr("Испытания"));
        ui->verticalLayoutMenuBar->addWidget(menuBar);

        const auto tests = m_core->getProbationData();
        int row = 0, column = 0;
        int maxColumn = getGridColumnCount(tests.count());
        for (const auto &test : tests) {
            const auto type = test.value<ProbationType>();
            if (type.title.size() == 0) {
                continue;
            }

            auto *button = new CustomButton(q_ptr);
            button->setText(type.title);
            button->setIcon(QPixmap(QString{ ":/ui/style/scheme/%1.svg" }.arg(type.icon)),
                            QPixmap(QString{ ":/ui/style/scheme/%1_hover.svg" }.arg(type.icon)));
            ui->gridLayoutTests->addWidget(button, row, column++);
            QObject::connect(button, &CustomButton::clicked, [this, button] {
                m_currentTest = button->text();
                updateSchemaViewPage();
                ui->stackedWidgetTests->setCurrentIndex(1);
            });

            if (column == maxColumn) {
                row++;
                column = 0;
            }

            QMenu *subMenu = new QMenu(type.title);
            testsMenu->addMenu(subMenu);
            QObject::connect(testsMenu, &QMenu::triggered, [this] (QAction *action) {
                m_currentSchema = action->text();
                setDeviceWidget();
                ui->stackedWidgetTests->setCurrentIndex(2);
            });

            for (const auto &schema : type.schemes) {
                const auto schemaType = schema.value<ProbationScheme>();
                if (schemaType.title.size() == 0) {
                    continue;
                }

                QAction *action = new QAction(schemaType.title, subMenu);
                subMenu->addAction(action);
            }
        }

        menuBar->addMenu(testsMenu);
        QMenu *libraryMenu = new QMenu(QObject::tr("База знаний"));
        menuBar->addMenu(libraryMenu);

        QMenu *protocolMenu = new QMenu(QObject::tr("Протокол"));
        menuBar->addMenu(protocolMenu);
        QObject::connect(protocolMenu, &QMenu::aboutToShow, [this] {
            if (!QDesktopServices::openUrl(QUrl(m_core->generalSetting("excel_file").toString()))) {
               QMessageBox::warning(q_ptr,
                                    QObject::tr("Ошибка открытия файла"),
                                    "Отсутствует файл с данными протокола");
            }
        });

        QMenu *helpMenu = new QMenu(QObject::tr("Справка"));
        menuBar->addMenu(helpMenu);
        QObject::connect(helpMenu, &QMenu::aboutToShow, [this] {
            QStringList infoStrings = {
                QString("%1\n%2").arg(QObject::tr("Название лаборатории:")).arg(QApplication::applicationName()),
                QString("%1\n%2").arg(QObject::tr("Серийный номер:")).arg(m_core->generalSetting("serial_number").toInt()),
                QString("%1\n%2").arg(QObject::tr("Дата изготовления:")).arg(BUILDDATE),
                QString("%1\n%2").arg(QObject::tr("Версия ПО контроллера:")).arg(m_core->generalSetting("controller_version").toInt()),
            };
            QMessageBox::information(q_ptr, QObject::tr("О лаборатории"), infoStrings.join("\n\n"));
        });

        QObject::connect(ui->stackedWidgetTests, &QStackedWidget::currentChanged, [this] (int index) {
            ui->wFooter->setVisible(index < 2);
        });

//        ui->dateLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss | dd.MM.yyyy"));
//        QTimer* dateTimer = new QTimer(q);
//        dateTimer->setInterval(1000);
//        QObject::connect(dateTimer, &QTimer::timeout, [this]{
//            ui->dateLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss | dd.MM.yyyy"));
//        });
//        dateTimer->start();

        QObject::connect(ui->closeButton, &QAbstractButton::clicked, [this, q]{
            if (m_aux_process) {
                m_aux_process->kill();
                m_aux_process->waitForFinished(3000);
            }

            q->close();
            emit q->exitTriggered();
        });

        QObject::connect(ui->minimizeButton, &QAbstractButton::clicked, [this]{
            q_ptr->setWindowState(Qt::WindowMinimized);
        });

        QObject::connect(ui->maximizeButton, &QAbstractButton::clicked, [this]{
            q_ptr->setWindowState(Qt::WindowMaximized);
        });

        QObject::connect(ui->toolButtonPrevPage, &QToolButton::clicked, [this]{
            ui->stackedWidgetTests->setCurrentIndex(0);
        });

//        QObject::connect(ui->aboutToolButton, &QAbstractButton::clicked, [this]{
//            QStringList infoStrings = {
//                QString("%1\n%2").arg(QObject::tr("Название лаборатории:")).arg(QApplication::applicationName()),
//                QString("%1\n%2").arg(QObject::tr("Серийный номер:")).arg(m_core->generalSetting("serial_number").toInt()),
//                QString("%1\n%2").arg(QObject::tr("Дата изготовления:")).arg(BUILDDATE),
//                QString("%1\n%2").arg(QObject::tr("Версия ПО контролера:")).arg(m_core->generalSetting("controller_version").toInt()),
//            };
//            QMessageBox::information(q_ptr, QObject::tr("О лаборатории"), infoStrings.join("\n\n"));
//        });

//        QObject::connect(ui->languageToolButton, &QAbstractButton::clicked, [q]{
//            QMessageBox::information(q, "TODO", "Not implemented yet");
//        });

        QObject::connect(m_core.data(), &Core::showErrorDialog, q, [q](const QString& message){
            QMessageBox::critical(q, "ERROR", message);
        }, Qt::QueuedConnection);

//        QObject::connect(ui->startPushButton, &QAbstractButton::clicked, [this]{
//            ui->startPushButton->setEnabled(false);
//            m_core->start();
//        });

        //QObject::connect(ui->stopPushButton, &QAbstractButton::clicked, m_core.data(), &Core::stop);

        QObject::connect(m_core.data(), &Core::switchIndication, q, &MainWidget::setIndication, Qt::QueuedConnection);

//        QObject::connect(ui->typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
//            if (!askAboutClosing()) {
//                QSignalBlocker blocker(ui->typeComboBox);
//                ui->typeComboBox->setCurrentIndex(m_last_probation);
//                return;
//            }
//            m_last_probation = index;

//            ui->schemeComboBox->clear();
//            ui->schemeComboBox->addItem(QObject::tr("------- Выберите схему испытания -------"));

//            if (index == 0)
//            {
//                ui->schemeComboBox->setCurrentIndex(0);
//                launchAuxProgram("");
//                return;
//            }

//            ProbationType ptype = ui->typeComboBox->itemData(index).value<ProbationType>();
//            for (auto it = ptype.schemes.begin(); it != ptype.schemes.end(); ++it) {
//                ProbationScheme scheme = (*it).value<ProbationScheme>();
//                ui->schemeComboBox->addItem(scheme.title, QVariant::fromValue(scheme));
//            }
//            setupBlocksIndicationMask(ptype.modbus_inputs);
//            m_core->setupProbation(ptype.modbus_outputs);
//            launchAuxProgram(ptype.aux_path);

//            if (ui->schemeComboBox->count() > 1)
//                ui->schemeComboBox->setCurrentIndex(1);
//        });

//        QObject::connect(ui->schemeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
//            setControlsEnabled(index != 0);
//            if (index == 0)
//            {
//                clearSchemeWidgets();
//                return;
//            }
//            auto schemeData = ui->schemeComboBox->itemData(index).value<ProbationScheme>();
//            ui->schemePixmapLabel->setPixmap(schemeData.scheme);
//            ui->hintTextEdit->setText(schemeData.hint);
//            setupBlocksIndication();
//        });

//        QObject::connect(m_core.data(), &Core::voltageSwitched, q, [this](bool enabled){
//            m_voltage_is_on = enabled;
//            Logger.Log(QString("Voltage was switched: %1").arg(m_voltage_is_on ? "ON" : "OFF"));
//            ui->voltageLabel->setVisible(m_voltage_is_on);
//            if (m_voltage_is_on)
//                switchStartIndication(false);
//            else
//                switchStartIndication(m_start_is_available);
//            ui->typeComboBox->setEnabled(!m_voltage_is_on);
//            ui->schemeComboBox->setEnabled(!m_voltage_is_on);
//        }, Qt::QueuedConnection);

//        QObject::connect(ui->plusToolButton, &QAbstractButton::pressed, [this]{
//            m_scheme_scale *= 1.1;
//            updateSchemePixmap();
//        });

//        QObject::connect(ui->minusToolButton, &QAbstractButton::pressed, [this]{
//            m_scheme_scale /= 1.1;
//            updateSchemePixmap();
//        });

        loadDataFromCore();
        setControlsEnabled(false);
    }

    int getGridColumnCount(int total)
    {
        int result = total / 2;
        if (result == 1) {
            return total;
        }

        return result;
    }

    void clearSchemaViewPage()
    {
        QLayoutItem *item;
        while ((item = ui->gridLayoutSchema->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    void clearDeviceWidget()
    {
        QLayoutItem *item;
        while ((item = ui->gridLayout_7->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    void setDeviceWidget()
    {
        clearDeviceWidget();

        auto widget = createDevice(m_currentSchema);
        if (widget == nullptr) {
            return;
        }

        ui->gridLayout_7->addWidget(widget);
    }

    void updateSchemaViewPage()
    {
        clearSchemaViewPage();

        const auto probationData = m_core->getProbationData();
        const auto test = probationData.value(m_currentTest);
        if (!test.isValid()) {
            return;
        }

        const auto testType = test.value<ProbationType>();
        int row = 0, column = 0;
        int maxColumn = getGridColumnCount(testType.schemes.count());
        for (const auto &schema : testType.schemes) {
            const auto schemaType = schema.value<ProbationScheme>();
            if (schemaType.title.size() == 0) {
                continue;
            }

            auto *button = new CustomButton(q_ptr);
            button->setText(schemaType.title);
            button->setIcon(QPixmap(QString{ ":/ui/style/scheme/%1.svg" }.arg("sch1")),
                            QPixmap(QString{ ":/ui/style/scheme/%1_hover.svg" }.arg("sch1")));
            ui->gridLayoutSchema->addWidget(button, row, column++);
            QObject::connect(button, &CustomButton::clicked, [this, button] {
                m_currentSchema = button->text();
                setDeviceWidget();
                ui->stackedWidgetTests->setCurrentIndex(2);
            });

            if (column == maxColumn) {
                row++;
                column = 0;
            }
        }
    }

    void launchAuxProgram(const QString &path)
    {
        if (m_aux_process != nullptr) {
            m_aux_process->kill();
            m_aux_process->disconnect();
            m_aux_process->deleteLater();
        }

        if (path.isEmpty())
            return;

        Q_Q(MainWidget);
        m_aux_process = new QProcess(q);
        m_aux_process->setProgram(path);
        m_aux_process->start(QIODevice::ReadOnly);
        QObject::connect(m_aux_process, QOverload<int>::of(&QProcess::finished), [this](int){
            m_aux_process->deleteLater();
            m_aux_process = nullptr;
        });

//#ifdef Q_OS_WIN
//        if (m_core->generalSetting("use_wnd_resize").toBool()) {
//            QTimer::singleShot(m_core->generalSetting("wnd_launch_timeout").toInt(), [this]{
//                if (!m_aux_process)
//                    return;
//                QRect geom = q_ptr->geometry();
//                geom.moveLeft(geom.width());
//                MoveAuxWindow(m_aux_process->processId(), geom);
//            });
//        }
//#endif
    }

//    void updateSchemaViewPage()
//    {
//        ui->stackedWidgetTests->setCurrentIndex(1);
//    }

    void updateSchemePixmap()
    {
//        QVariant userData = ui->schemeComboBox->currentData();
//        if (!userData.isNull()) {
//            auto pic = userData.value<ProbationScheme>();
//            ui->schemePixmapLabel->setPixmap(pic.scheme.scaled(pic.scheme.size() * m_scheme_scale, Qt::KeepAspectRatio, Qt::SmoothTransformation));
//        }
    }

    void clearSchemeWidgets()
    {
//        ui->hintTextEdit->clear();
//        ui->schemePixmapLabel->clear();
    }

    void setControlsEnabled(bool enabled)
    {
        for(auto *btn : m_indications.values()) {
            btn->setEnabled(enabled);
            btn->setProperty("passed", "");
            UpdateStyle(btn);
        }
        //ui->startPushButton->setEnabled(enabled);
    }

    void loadDataFromCore()
    {
        QVariantMap probationData = m_core->getProbationData();
//        ui->typeComboBox->clear();
//        ui->typeComboBox->addItem(QObject::tr("-------  Выберите тип испытания -------"));

//        for (auto it = probationData.begin(); it != probationData.end(); ++it)
//            ui->typeComboBox->addItem(it.key(), it.value());
    }

    void setupBlocksIndicationMask(const QBitArray& modbusInputs)
    {
        m_enabled_blocks.clear();
        for(int i = Core::Blocks::GND; i != Core::Blocks::BLOCKS_COUNT; ++i)
        {
            if (modbusInputs.testBit(i)){
                m_enabled_blocks << Core::Blocks(i);
            }
        }
    }

    void setupBlocksIndication()
    {
        for (auto it = m_indications.begin(); it != m_indications.end(); ++it) {
            it.value()->setEnabled(m_enabled_blocks.contains(it.key()));
            UpdateStyle(it.value());
        }
    }

    void switchStartIndication(bool enable)
    {
        //ui->startPushButton->setEnabled(enable);
        m_core->switchGreenLED(enable);
    }

    bool askAboutClosing()
    {
        if (m_aux_process != nullptr)
            return QMessageBox::Yes == QMessageBox::question(q_ptr
                                                             , QObject::tr("Подтвердите действие")
                                                             , QObject::tr("Внешняя программа для данного режима испытаний будет закрыта. Продолжить?"));
        return true;
    }
};

// ------------------------------------------------------

MainWidget::MainWidget(const QSharedPointer<Core> &programm_core, QWidget *parent)
    : QWidget(parent)
    , d_ptr(new MainWidgetPrivate(programm_core, this))
{
    Q_D(MainWidget);
    d->ui->setupUi(this);
    d->configUI();
}

MainWidget::~MainWidget()
{ }

void MainWidget::setIndication(const QBitArray& bits)
{
    Q_D(MainWidget);
    auto metaEnum = QMetaEnum::fromType<Core::Blocks>();
    bool activateStart = true;
    for(int i = 0; i < bits.size(); ++i)
    {
        if (Core::BelongsToBlocks(i)
                && d->m_enabled_blocks.contains(Core::Blocks(i)) && d->m_indications.contains(Core::Blocks(i)))
        {
            QToolButton * btn = d->m_indications[Core::Blocks(i)];
            btn->setProperty("passed", int(bits[i]));
            UpdateStyle(btn);
            activateStart = activateStart & bits[i];
        }
        if (bits[i])
            Logger.Log(QString("Activated blocking: %1").arg(metaEnum.valueToKey(i)));
    }
    if (!activateStart) {
        Logger.Log("Blockings are valid no more, stop triggered");
        d->m_core->stop();
    } else {
        Logger.Log("All blockings are fine");
    }
    d->m_start_is_available = activateStart;
//    if (!d->m_start_is_available) {
//        d->switchStartIndication(false);
//    } else if (d->ui->typeComboBox->currentIndex() > 0) {
//        d->switchStartIndication(!d->m_voltage_is_on && d->m_start_is_available);
//    }
}

void MainWidget::showEvent(QShowEvent *event)
{
    resize(QApplication::desktop()->availableGeometry().width(),
           QApplication::desktop()->availableGeometry().height());
    move(0, 0);
    QWidget::showEvent(event);
}
