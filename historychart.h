#ifndef FIFOCHART_H
#define FIFOCHART_H

#include <QQueue>
#include <QPointF>
#include <QVector>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE


class HistoryChart : public QChart
{
    Q_OBJECT

public:
    explicit HistoryChart(QGraphicsItem *parent = Q_NULLPTR,
                       Qt::WindowFlags wFlags = Qt::WindowFlags());
    virtual ~HistoryChart();

    void addSeries(QXYSeries*);
    void addValue(QVector<double>&);
    void addValue(double);
    void addPoint(QPointF p);
    void setHistoryLength(int);
    QValueAxis *axisX();
    QValueAxis *axisY();

private:
    void adjustAxisRanges();
    void findMinMax(int s);
    enum { UpperBound, LowerBound };
    void findMaxAndTicks(double &min, double &max, int &ticks);

    QVector<QQueue<double>> m_data;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QVector<QXYSeries*> m_series;
    int m_numSeries;
    int m_size;
    qreal m_minY;
    qreal m_maxY;
};

#endif // FIFOCHART_H
