#include "historychart.h"

#include <QList>
#include <math.h>

HistoryChart::HistoryChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(parent, wFlags),
      m_axisX(new QValueAxis()),
      m_axisY(new QValueAxis()),
      m_numSeries(0),
      m_size(0),
      m_minY(0.0),
      m_maxY(0.0)
{
    m_axisX->setTickCount(10);
    m_axisY->setTickCount(10);
    m_axisY->setRange(-10, 10);

    addAxis(m_axisX, Qt::AlignBottom);
    addAxis(m_axisY, Qt::AlignLeft);
}

HistoryChart::~HistoryChart()
{

}

void HistoryChart::addSeries(QXYSeries *series)
{
    QChart::addSeries(series);
    series->attachAxis(m_axisX);
    series->attachAxis(m_axisY);
    m_series.append(series);
    m_data.resize(++m_numSeries);
}

QValueAxis *HistoryChart::axisX()
{
    return static_cast<QValueAxis*>(QChart::axisX());
}

 QValueAxis *HistoryChart::axisY()
{
    return static_cast<QValueAxis*>(QChart::axisY());
}

void HistoryChart::setHistoryLength(int size)
{
    m_size = size;
}

void HistoryChart::findMaxAndTicks(double &min, double &max, int &ticks)
{
    double range = max - min;

    double maxIn = max;
    double minIn = min;
    double magnitude = floor(log10(range));

    if (magnitude < -1)
       magnitude = -1;

    double stride = pow(10, magnitude) / 2.0;

    int n;
    do {
        n = 0;
        max = round((maxIn-range/2.0) / stride) * stride;
        while (max <= maxIn) {
            max += stride;
            ++n;
        }

        min = round((minIn+range/2.0) / stride) * stride;
        while( min >= minIn) {
            min -= stride;
            ++n;
        }

        if (n > 8)
            stride = stride * 2;
    } while (n > 8);

    ticks = (max - min) / stride ;
}

void HistoryChart::adjustAxisRanges()
{
    ssize_t data_size = m_data[0].size();

    int ticks;
    double maxX = data_size;
    double minX = 0;

    findMaxAndTicks(minX, maxX, ticks);

    m_axisX->setRange(0, maxX);
    m_axisX->setTickCount(ticks);

    /* autorange y axis */
    double minY = m_minY; //floor(m_minY);
    double maxY = m_maxY; //ceil(m_maxY);

    findMaxAndTicks(minY, maxY, ticks);

    m_axisY->setRange(minY, maxY);
    m_axisY->setTickCount(ticks + 1);
}

void HistoryChart::findMinMax(int s)
{
    QQueue<double>& d = m_data[s];
    QList<double>::const_iterator it = d.begin();
    while (it != d.end()) {
        if (it == d.begin() && s == 0)
            m_maxY = m_minY = *it;

        if (*it < m_minY)
            m_minY = *it;
        if (*it > m_maxY)
            m_maxY = *it;
        ++it;
    }
}

void HistoryChart::addValue(QVector<double> & v)
{
    if (v.size() != m_numSeries)
        return;

    for (int series = 0; series < m_numSeries; ++series) {
        QQueue<double>& data = m_data[series];

        data.enqueue(v[series]);
        if (data.size() > m_size)
            data.dequeue();

        /* */
        int p = 0;
        QList<QPointF> points;
        for (QQueue<double>::const_iterator it = data.begin(); it != data.end(); ++it, ++p)
            points.append(QPointF(p, *it));

        /* update the series with the new data */
        m_series[series]->replace(points);

        findMinMax(series);
    }

    adjustAxisRanges();
}

void HistoryChart::addValue(double val)
{
    QQueue<double>& data = m_data[0];

    data.enqueue(val);
    if (data.size() > m_size)
        data.dequeue();

    int p = 0;
    QList<QPointF> points;
    for (QQueue<double>::const_iterator it = data.begin(); it != data.end(); ++it, ++p)
        points.append(QPointF(p, *it));

    /* update the series with the new data */
    m_series[0]->replace(points);

    findMinMax(0);
    adjustAxisRanges();
}
