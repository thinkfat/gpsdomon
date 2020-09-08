#ifndef AVAR_H
#define AVAR_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QThread>

class xdev : public QObject
{
    Q_OBJECT

public:
    struct tausigma {
        double tau;
        double sigma;
        double err;
    };
    xdev(QObject *parent = nullptr);
    virtual ~xdev();

    void setSeries(QVector<double>&);
    void addSample(double);
    void resetSamples();
    const QVector<tausigma>& adevVals();

    void setOverlapMode(bool);
    void setFrequencySeries(bool);

public slots:
    void recompute();

signals:
    void done();

private:
    struct valerr {
        double val;
        double err;
    };

    valerr calc_xdev(long tau);
    long next_tau(long tau, int bins);
    QVector<double> cumsum(const QVector<double>&);

    QVector<double> m_data;
    QVector<tausigma> m_avals;

    unsigned int m_rate;

    bool m_overlap;
    bool m_frequency;
};

#endif // AVAR_H
