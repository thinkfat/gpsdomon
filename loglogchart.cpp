#include "loglogchart.h"

#include <QAreaSeries>

LogLogChart::LogLogChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(parent, wFlags),
      m_axisX(new QLogValueAxis()),
      m_axisY(new QLogValueAxis())
{
    m_axisX->setRange(1, 10);
    m_axisX->setLabelFormat("%g");
    m_axisX->setMinorTickCount(4);
    m_axisX->setBase(10.0);

    m_axisY->setRange(1, 10);
    m_axisY->setLabelFormat("%g");
    m_axisY->setMinorTickCount(4);
    m_axisY->setBase(10.0);

    addAxis(m_axisX, Qt::AlignBottom);
    addAxis(m_axisY, Qt::AlignLeft);
}

LogLogChart::~LogLogChart()
{

}

void LogLogChart::addSeries(QAbstractSeries *series)
{
    QChart::addSeries(series);
    series->attachAxis(m_axisX);
    series->attachAxis(m_axisY);
    m_series.append(series);
}

QLogValueAxis *LogLogChart::axisX()
{
    return static_cast<QLogValueAxis*>(QChart::axisX());
}

QLogValueAxis *LogLogChart::axisY()
{
    return static_cast<QLogValueAxis*>(QChart::axisY());
}

void LogLogChart::updateAxisRange(const QVector<QPointF> &data)
{
    for (QVector<QPointF>::const_iterator it = data.begin();
         it != data.end(); ++it)
    {
        if (it->y() < m_minY)
            m_minY = it->y();
        if (it->y() > m_maxY)
            m_maxY = it->y();

        if (it->x() < m_minX)
            m_minX = it->x();
        if (it->x() > m_maxX)
            m_maxX = it->x();
    }
}

void LogLogChart::setData(QVector<QPointF> upper, QVector<QPointF> lower)
{
    QAreaSeries *a = dynamic_cast<QAreaSeries*>(m_series[1]);
    if (a != nullptr) {
        a->upperSeries()->replace(upper);
        a->lowerSeries()->replace(lower);
    }

    updateAxisRange(upper);
    updateAxisRange(lower);
    m_axisX->setRange(m_minX, m_maxX * 10);
    m_axisY->setRange(m_minY / 10, m_maxY * 10);
}

void LogLogChart::setData(QVector<QPointF> data)
{
    if (data.size() == 0)
        return;

    m_minX = m_maxX = data[0].x();
    m_minY = m_maxY = data[0].y();

    QLineSeries *s = dynamic_cast<QLineSeries*>(m_series[0]);
    if (s != nullptr)
        s->replace(data);

    updateAxisRange(data);
    m_axisX->setRange(m_minX, m_maxX * 10);
    m_axisY->setRange(m_minY / 10, m_maxY * 10);
}
