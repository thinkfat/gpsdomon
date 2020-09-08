#ifndef LOGLOGCHART_H
#define LOGLOGCHART_H

#include <QtCharts/QChart>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE

class LogLogChart : public QChart
{
    Q_OBJECT

public:
    explicit LogLogChart(QGraphicsItem *parent = Q_NULLPTR,
                Qt::WindowFlags wFlags = Qt::WindowFlags());
    virtual ~LogLogChart();

    void addSeries(QAbstractSeries*);
    void setData(QVector<QPointF>);
    void setData(QVector<QPointF>, QVector<QPointF>);
    QLogValueAxis *axisX();
    QLogValueAxis *axisY();

private:

    void updateAxisRange(const QVector<QPointF>&);

    QLogValueAxis *m_axisX;
    QLogValueAxis *m_axisY;
    QVector<QAbstractSeries*> m_series;

    qreal m_minX, m_minY;
    qreal m_maxX, m_maxY;
};

#endif // LOGLOGCHART_H
