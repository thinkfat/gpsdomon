#include "xdev.h"
#include <math.h>
#include <float.h>

#define ADEV_MIN_SAMPLES 4
//#define DO_MDEV 1
#define DO_ADEV 1

xdev::xdev(QObject *parent)
    : QObject(parent),
    m_rate(1),
    m_overlap(false),
    m_frequency(true),
    m_capLowConf(false)
{
}

xdev::~xdev()
{

}

void xdev::setOverlapMode(bool m)
{
    m_overlap = m;
}

void xdev::setFrequencySeries(bool m)
{
    m_frequency = m;
}

void xdev::setLowConfidenceCap(bool m)
{
    m_capLowConf = m;
}

void xdev::setSeries(QVector<double> &data)
{
    if  (m_frequency)
        m_data = cumsum(data);
    else
        m_data = data;
    m_avals.clear();
}

void xdev::addSample(double sample)
{
    m_data.append(sample);
}

void xdev::resetSamples()
{
    m_data.clear();
    m_avals.clear();
}

void xdev::recompute()
{
    // not enough samples to start
    if (m_data.size() <= ADEV_MIN_SAMPLES) {
        emit done();
        return;
    }

    valerr adev;
    long tau = 1;
    for (int n = 0; n < m_data.size(); ++n) {
        adev = calc_xdev(tau);
        if (adev.val <= 0.0)
            break;
        m_avals.append(xdev::tausigma{(double)tau, adev.val, adev.err});
        tau = next_tau(tau, 29);
    }
    emit done();
}

const QVector<xdev::tausigma>& xdev::adevVals()
{
    return m_avals;
}

#ifdef DO_MDEV
xdev::valerr xdev::calc_xdev (long tau)
{
    long i, n;
    double sum, v;

    sum = v = n = 0;
    for (i = 0; (i + 2*tau) < m_data.size() && i < tau; i += 1) {
        v += m_data[i + 2*tau] - 2 * m_data[i + tau] + m_data[i];
    }
    sum += v * v;
    n += 1;

    for (i = 0; (i + 3*tau) < m_data.size(); i += 1) {
        v += m_data[i + 3*tau] - 3*m_data[i + 2*tau] + 3*m_data[i + tau] - m_data[i];
        sum += v * v;
        n += 1;
    }
    sum /= 2.0 * (double)tau * (double)tau;

    if (m_capLowConf == true) {
        /* don't output values with low confidence */
        if ((double)n / (double)tau < 2.0)
            return valerr{0.0, 0.0};
    }

    valerr result;
    result.val = sqrt(sum / n) / tau;
    result.err = result.val / abs(sqrt((double)n / (double)tau));

    return result;
}
#endif

#ifdef DO_ADEV
xdev::valerr xdev::calc_xdev(long tau)
{
    long i, n, stride;
    double sum, v;

    stride = m_overlap ? 1 : tau;
    sum = n = 0;
    for (i = 0; (i + 2*tau) < m_data.size(); i += stride) {
        v = m_data[i + 2*tau] - 2 * m_data[i + tau] + m_data[i];
        sum += v * v;
        n += 1;
    }
    sum /= 2.0;

    if (n < ADEV_MIN_SAMPLES)
        return valerr{0.0, 0.0};

    if (m_capLowConf == true) {
        /* don't output values with low confidence */
        if ((double)n / (double)tau < 2.0)
            return valerr{0.0, 0.0};
    }

    valerr result;
    result.val = sqrt(sum / (double)n) / (double)tau;
    result.err =result.val / abs(sqrt((double)n / (double)tau));
    return result;
}
#endif

//
// Get next tau to calculate. Some tools calculate Allan deviation
// for only one point per decade; others two or three. Below are
// several unique alternatives that produce cleaner-looking plots.
//
long xdev::next_tau(long tau, int bins)
{
    long pow10, n;

    switch (bins) {

    case 0 :    // all tau (not practical)

        return tau + 1;

    case 1 :    // one per decade

        return tau * 10;

    case 2 :    // one per octave

        return tau * 2;

    case 3 :    // 3 dB
    case 4 :    // 1-2-4 decade
    case 5 :    // 1-2-5 decade
    case 10 :   // ten per decade

        pow10 = 1;
        while (tau >= 10) {
            pow10 *= 10;
            tau /= 10;
        }
        if (bins == 3) {
            return ((tau == 3) ? 10 : 3) * pow10;
        }
        if (bins == 4 && tau == 4) {
            return 10 * pow10;
        }
        if (bins == 5 && tau == 2) {
            return 5 * pow10;
        }
        if (bins == 10) {
            return (tau + 1) * pow10;
        }
        return tau * 2 * pow10;

    case 29 :    // 29 nice round numbers per decade

        pow10 = 1;
        while (tau > 100) {
            pow10 *= 10;
            tau /= 10;
        }
        if (tau < 22) {
            return (tau + 1) * pow10;

        } else if (tau < 40) {
            return (tau + 2) * pow10;

        } else if (tau < 60) {
            return (tau + 5) * pow10;

        } else {
            return (tau + 10) * pow10;
        }

    default :   // logarithmically evenly spaced divisions

        n = (long) (log10(tau) * (double)bins + 0.5) + 1;
        n = (long) pow(10.0, (double)n / (double)bins);
        return (n > tau) ? n : tau + 1;
    }

}

QVector<double> xdev::cumsum(const QVector<double>& data)
{
    QVector<double>::const_iterator it = data.begin();
    QVector<double> result;
    double sum = 0.0;

    while (it != data.end()) {
        sum += *it;
        result.append(sum / m_rate);
        ++it;
    }

    return result;
}
