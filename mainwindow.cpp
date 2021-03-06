#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_connectdialog.h"

#include <QTimer>
#include <QtWidgets/QGridLayout>
#include <QtCharts/QLogValueAxis>
#include <QAreaSeries>

#define DEFAULT_HISTORY (25000)
#define DEFAULT_DAC_LSB (1000.0e-9 / 65536)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_numSamples(0),
    m_avarRunning(false)
{
    ui->setupUi(this);

    m_cnct = new GpsdoConnection(this);
    connect(m_cnct, SIGNAL(dataReady(QString)), this, SLOT(newData(QString)));
    connect(m_cnct, SIGNAL(connected()), this, SLOT(connectionEstablished()));
    connect(m_cnct, SIGNAL(socketError(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    m_cnct->connectTo("192.168.2.136");

    // TIC chart
    m_chart = new HistoryChart();
    m_chart->addSeries(new QLineSeries);
    m_chart->addSeries(new QLineSeries);
    m_chart->series()[0]->setOpacity(0.2);
    m_chart->legend()->hide();
    m_chart->setTitle("TIC");
    m_chart->setHistoryLength(DEFAULT_HISTORY);
    m_chart->axisX()->setLabelFormat("%d");
    QChartView *chartView = ui->ticView;
    chartView->setChart(m_chart);
    chartView->setUpdatesEnabled(true);

    // DAC chart
    m_dacChart = new HistoryChart();
    m_dacChart->addSeries(new QLineSeries);
    m_dacChart->addSeries(new QLineSeries);
    m_dacChart->addSeries(new QLineSeries);
    m_dacChart->legend()->hide();
    m_dacChart->setTitle("DAC");
    m_dacChart->axisY()->setLabelFormat("%.1f");
    m_dacChart->axisX()->setLabelFormat("%d");
    m_dacChart->setHistoryLength(DEFAULT_HISTORY);
    chartView = ui->dacView;
    chartView->setChart(m_dacChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setUpdatesEnabled(true);

    // Temp chart
    m_TempChart = new HistoryChart();
    m_TempChart->addSeries(new QLineSeries);
    m_TempChart->legend()->hide();
    m_TempChart->setTitle("Temperature");
    m_TempChart->axisX()->setLabelFormat("%d");
    m_TempChart->setHistoryLength(DEFAULT_HISTORY);
    chartView = ui->tempView;
    chartView->setChart(m_TempChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setUpdatesEnabled(true);

    /* attach the adev computation thread */
    m_xdev = new xdev();
    m_xdev->setOverlapMode(true);
    m_xdev->setFrequencySeries(true);
    m_xdev->moveToThread(&avarThread);
    connect(this, &MainWindow::computeAdev, m_xdev, &xdev::recompute);
    connect(m_xdev, &xdev::done, this, &MainWindow::adevResultAvailable);
    avarThread.start();

    /* adev chart */
    m_adevChart = new LogLogChart();
    QLineSeries *series = new QLineSeries;
    m_adevChart->addSeries(series);

    QPen pen;
    pen.setStyle(Qt::DashDotLine);

    QAreaSeries *aseries = new QAreaSeries(new QLineSeries, new QLineSeries);
    aseries->setPen(pen);
    aseries->setOpacity(0.2);
    m_adevChart->addSeries(aseries);

    m_adevChart->legend()->hide();
    m_adevChart->setTitle("TIC MDEV");

    chartView = ui->adevView;
    chartView->setChart(m_adevChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setUpdatesEnabled(true);

    m_skyView = new QPolarChart();
    m_skyView->setTitle("GNSS Constellations");

    QValueAxis *angularAxis = new QValueAxis();
    angularAxis->setTickCount(9); // First and last ticks are co-located on 0/360 angle.
    angularAxis->setLabelFormat("%d");
    angularAxis->setRange(0, 360);
    angularAxis->setReverse(true);
    m_skyView->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);

    QValueAxis *radialAxis = new QValueAxis();
    radialAxis->setTickCount(9);
    radialAxis->setLabelFormat("%d");
    radialAxis->setLabelsVisible(false);
    radialAxis->setRange(0, 90);
    radialAxis->setReverse(true);
    m_skyView->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);

    m_gpsSats = new QScatterSeries;
    m_skyView->addSeries(m_gpsSats);
    m_gpsSats->attachAxis(angularAxis);
    m_gpsSats->attachAxis(radialAxis);
    m_gpsSats->setName("GPS");

    m_galileoSats = new QScatterSeries;
    m_skyView->addSeries(m_galileoSats);
    m_galileoSats->attachAxis(angularAxis);
    m_galileoSats->attachAxis(radialAxis);
    m_galileoSats->setName("Galileo");

    m_unusedSats = new QScatterSeries;
    m_skyView->addSeries(m_unusedSats);
    m_unusedSats->attachAxis(angularAxis);
    m_unusedSats->attachAxis(radialAxis);
    m_unusedSats->setName("Unused");

    chartView = ui->gnssView;
    chartView->setChart(m_skyView);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setUpdatesEnabled(true);

    connect(&m_satUpdateTimer, &QTimer::timeout, this, &MainWindow::requestSatInfo);

    setWindowTitle("GPSDO Monitor");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
    avarThread.quit();
    avarThread.wait();
    delete ui;
}

void MainWindow::connectionEstablished()
{
    m_cnct->write("cl 1\n");
    m_satUpdateTimer.start(30000);
    requestSatInfo();
}

void MainWindow::reconnect()
{
    m_cnct->connectTo("192.168.2.136");
}

void MainWindow::socketError(QAbstractSocket::SocketError se)
{
    Q_UNUSED(se);

    QTimer::singleShot(500, this, &MainWindow::reconnect);
    m_satUpdateTimer.stop();
}

void MainWindow::newData(QString data)
{
    double val;

    // skip lines starting with '#'
    if (data.startsWith("#"))
        return;

    if (data.startsWith("SV")) {
        displaySatInfo(data);
        return;
    }

    QStringList values = data.trimmed().split(",", QString::SkipEmptyParts);

    // skip warmup and initial phase search
    if (values[8].toInt() > 3)
        return;

    QVector<double> v;
    v.append(values[0].toDouble());
    v.append(values[4].toDouble());
    m_chart->addValue(v);
#if 0
    m_timeSeries.append(v[1] * 1.0e-9);
#endif
    v.clear();

    v.append(values[2].toDouble());
    v.append(values[3].toDouble());
    v.append(values[5].toDouble());
    m_dacChart->addValue(v);

#if 1
    // feed adev with scaled dac values
    m_timeSeries.append(v[2] * DEFAULT_DAC_LSB);
#endif

    if (m_timeSeries.size() > DEFAULT_HISTORY)
        m_timeSeries.removeFirst();

    if (!m_avarRunning) {
        m_avarRunning = true;
        QTimer::singleShot(5000, this, &MainWindow::updateAdev);
    }

    val = values[6].toDouble();
    m_TempChart->addValue(val);

}

void MainWindow::updateAdev()
{
    m_xdev->setSeries(m_timeSeries);
    emit computeAdev();
}

void MainWindow::adevResultAvailable()
{
    QVector<QPointF> adevPoints;
    QVector<QPointF> aerrUpperPoints;
    QVector<QPointF> aerrLowerPoints;
    const QVector<xdev::tausigma> &adevVals = m_xdev->adevVals();
    QVector<xdev::tausigma>::const_iterator it = adevVals.begin();

    while (it != adevVals.end()) {
        adevPoints.append(QPointF(it->tau, it->sigma));
        aerrUpperPoints.append(QPointF(it->tau, it->sigma + it->err));
        aerrLowerPoints.append(QPointF(it->tau, it->sigma - it->err));
        ++it;
    }

    m_adevChart->setData(adevPoints);
    m_adevChart->setData(aerrUpperPoints, aerrLowerPoints);
    m_avarRunning = false;
}

void MainWindow::requestSatInfo()
{
    m_cnct->write("gsat\n");
    m_updateRequested = true;
}

void MainWindow::displaySatInfo(QString data)
{
    QRegularExpression re("^SV: (?<gnss>\\d):\\s*(?<svid>\\d+), azim:\\s+(?<azim>-*\\d+), elev:\\s+(?<elev>-*\\d+),.*flags:\\s+(?<flags>-*([0-9,a-f,A-F])+)");
    QRegularExpressionMatch match = re.match(data);

    if (match.hasMatch()) {
        if (m_updateRequested) {
            m_gpsSats->clear();
            m_galileoSats->clear();
            m_unusedSats->clear();
            m_updateRequested = false;
        }

        int gnssid = match.captured("gnss").toInt();
        //int svid = match.captured("svid").toInt();
        int azim = match.captured("azim").toInt();
        int elev = 90 - match.captured("elev").toInt();
        QString flags_str = match.captured("flags");
        int flags = flags_str.toInt(Q_NULLPTR, 16);

        if ((flags & 0xf) != 0xf) {
            m_unusedSats->append(azim, elev);
        } else {
            if (gnssid == 0)
                m_gpsSats->append(azim, elev);
            if (gnssid == 2)
                m_galileoSats->append(azim, elev);
        }
    }
}

void MainWindow::actionConnect()
{
    Ui::Dialog *connectDialog = new Ui::Dialog();
    connectDialog->setupUi((QDialog*)connectDialog);
}
