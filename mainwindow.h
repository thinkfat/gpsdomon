#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gpsdoconnection.h"
#include "historychart.h"
#include "loglogchart.h"
#include "xdev.h"

#include <QMainWindow>
#include <QThread>
#include <QTimer>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QPolarChart>
#include <QtCharts/QScatterSeries>

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void actionConnect();
    void newData(QString);
    void connectionEstablished();
    void socketError(QAbstractSocket::SocketError);
    void reconnect();
    void adevResultAvailable();
    void updateAdev();
    void displaySatInfo(QString);
    void requestSatInfo();

signals:
    void computeAdev();

private:
    Ui::MainWindow *ui;

    QThread avarThread;
    QVector<double> m_timeSeries;
    unsigned int m_numSamples;

    GpsdoConnection *m_cnct;
    HistoryChart *m_chart;
    HistoryChart *m_dacChart;
    HistoryChart *m_TempChart;
    LogLogChart *m_adevChart;

    xdev *m_xdev;
    bool m_avarRunning;

    QPolarChart *m_skyView;
    QScatterSeries *m_gpsSats;
    QScatterSeries *m_galileoSats;
    QScatterSeries *m_unusedSats;
    QTimer m_satUpdateTimer;
    bool m_updateRequested;
};

#endif // MAINWINDOW_H
