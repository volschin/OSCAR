/* gPressureChart Implementation
 *
 * Copyright (c) 2020 The Oscar Team
 * Copyright (c) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the source code
 * for more details. */

#include "gPressureChart.h"

gPressureChart::gPressureChart()
    : gSummaryChart("Pressure", MT_CPAP)
{
    addCalc(CPAP_Pressure, ST_SETMAX);     // 00
    addCalc(CPAP_Pressure, ST_MID);        // 01
    addCalc(CPAP_Pressure, ST_90P);        // 02
    addCalc(CPAP_PressureMin, ST_SETMIN);  // 03
    addCalc(CPAP_PressureMax, ST_SETMAX);  // 04

    addCalc(CPAP_EPAP, ST_SETMAX);      // 05
    addCalc(CPAP_IPAP, ST_SETMAX);      // 06
    addCalc(CPAP_EPAPLo, ST_SETMAX);    // 07
    addCalc(CPAP_IPAPHi, ST_SETMAX);    // 08

    addCalc(CPAP_EPAP, ST_MID);         // 09
    addCalc(CPAP_EPAP, ST_90P);         // 10
    addCalc(CPAP_IPAP, ST_MID);         // 11
    addCalc(CPAP_IPAP, ST_90P);         // 12
}

int gPressureChart::addCalc(ChannelID code, SummaryType type) {
    QColor color = schema::channel[code].defaultColor();
    if (type == ST_90P) {
        color = brighten(color, 1.33f);
    }
    int index = gSummaryChart::addCalc(code, type, color);
    m_calcs[code][type] = index;
    return index;
}

SummaryCalcItem* gPressureChart::getCalc(ChannelID code, SummaryType type)
{
    return &calcitems[m_calcs[code][type]];
}

void gPressureChart::afterDraw(QPainter &, gGraph &graph, QRectF rect)
{
    QStringList presstr;
    SummaryCalcItem* calc;

    calc = getCalc(CPAP_Pressure);
    if (calc->cnt > 0) {
        presstr.append(QString("%1 %2/%3/%4").
                arg(STR_TR_CPAP).
                arg(calc->min,0,'f',1).
                arg(calc->mid(), 0, 'f', 1).
                arg(calc->max,0,'f',1));
    }
    if (getCalc(CPAP_PressureMin, ST_SETMIN)->cnt > 0) {
        presstr.append(QString("%1 %2/%3/%4/%5").
                arg(STR_TR_APAP).
                arg(getCalc(CPAP_PressureMin, ST_SETMIN)->min,0,'f',1).
                arg(getCalc(CPAP_Pressure, ST_MID)->mid(), 0, 'f', 1).
                arg(getCalc(CPAP_Pressure, ST_90P)->mid(),0,'f',1).
                arg(getCalc(CPAP_PressureMax, ST_SETMAX)->max, 0, 'f', 1));

    }
    calc = getCalc(CPAP_EPAP);
    if (calc->cnt > 0) {
        presstr.append(QString("%1 %2/%3/%4").
                arg(STR_TR_EPAP).
                arg(calc->min,0,'f',1).
                arg(calc->mid(), 0, 'f', 1).
                arg(calc->max, 0, 'f', 1));
    }
    calc = getCalc(CPAP_IPAP);
    if (calc->cnt > 0) {
        presstr.append(QString("%1 %2/%3/%4").
             arg(STR_TR_IPAP).
             arg(calc->min,0,'f',1).
             arg(calc->mid(), 0, 'f', 1).
             arg(calc->max, 0, 'f', 1));
    }
    calc = getCalc(CPAP_EPAPLo);
    if (calc->cnt > 0) {
        presstr.append(QString("%1 %2/%3/%4").
            arg(STR_TR_EPAPLo).
            arg(calc->min,0,'f',1).
            arg(calc->mid(), 0, 'f', 1).
            arg(calc->max, 0, 'f', 1));
    }
    calc = getCalc(CPAP_IPAPHi);
    if (calc->cnt > 0) {
        presstr.append(QString("%1 %2/%3/%4").
            arg(STR_TR_IPAPHi).
            arg(calc->min,0,'f',1).
            arg(calc->mid(), 0, 'f', 1).
            arg(calc->max, 0, 'f', 1));
    }
    QString txt = presstr.join(" ");
    graph.renderText(txt, rect.left(), rect.top()-5*graph.printScaleY(), 0);

}


void gPressureChart::populate(Day * day, int idx)
{
    float tmp;
    CPAPMode mode =  (CPAPMode)(int)qRound(day->settings_wavg(CPAP_Mode));
    QVector<SummaryChartSlice> & slices = cache[idx];
    SummaryCalcItem* calc;

    if (mode == MODE_CPAP) {
        calc = getCalc(CPAP_Pressure);
        float pr = day->settings_max(CPAP_Pressure);
        slices.append(SummaryChartSlice(calc, pr, pr, schema::channel[CPAP_Pressure].label(), calc->color));
    } else if (mode == MODE_APAP) {
        float min = day->settings_min(CPAP_PressureMin);
        float max = day->settings_max(CPAP_PressureMax);

        tmp = min;

        slices.append(SummaryChartSlice(getCalc(CPAP_PressureMin, ST_SETMIN), min, min, schema::channel[CPAP_PressureMin].label(), getCalc(CPAP_PressureMin, ST_SETMIN)->color));
        if (!day->summaryOnly()) {
            float med = day->calcMiddle(CPAP_Pressure);
            slices.append(SummaryChartSlice(getCalc(CPAP_Pressure, ST_MID), med, med - tmp, day->calcMiddleLabel(CPAP_Pressure), getCalc(CPAP_Pressure, ST_MID)->color));
            tmp += med - tmp;

            float p90 = day->calcPercentile(CPAP_Pressure);
            slices.append(SummaryChartSlice(getCalc(CPAP_Pressure, ST_90P), p90, p90 - tmp, day->calcPercentileLabel(CPAP_Pressure), getCalc(CPAP_Pressure, ST_90P)->color));
            tmp += p90 - tmp;
        }
        slices.append(SummaryChartSlice(getCalc(CPAP_PressureMax, ST_SETMAX), max, max - tmp, schema::channel[CPAP_PressureMax].label(), getCalc(CPAP_PressureMax, ST_SETMAX)->color));

    } else if (mode == MODE_BILEVEL_FIXED) {
        float epap = day->settings_max(CPAP_EPAP);
        float ipap = day->settings_max(CPAP_IPAP);

        slices.append(SummaryChartSlice(getCalc(CPAP_EPAP), epap, epap, schema::channel[CPAP_EPAP].label(), getCalc(CPAP_EPAP)->color));
        slices.append(SummaryChartSlice(getCalc(CPAP_IPAP), ipap, ipap - epap, schema::channel[CPAP_IPAP].label(), getCalc(CPAP_IPAP)->color));

    } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
        float epap = day->settings_max(CPAP_EPAPLo);
        tmp = epap;
        float ipap = day->settings_max(CPAP_IPAPHi);

        slices.append(SummaryChartSlice(getCalc(CPAP_EPAPLo), epap, epap, schema::channel[CPAP_EPAPLo].label(), getCalc(CPAP_EPAPLo)->color));
        if (!day->summaryOnly()) {

            float e50 = day->calcMiddle(CPAP_EPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_EPAP, ST_MID), e50, e50 - tmp, day->calcMiddleLabel(CPAP_EPAP), getCalc(CPAP_EPAP, ST_MID)->color));
            tmp += e50 - tmp;

            float e90 = day->calcPercentile(CPAP_EPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_EPAP, ST_90P), e90, e90 - tmp, day->calcPercentileLabel(CPAP_EPAP), getCalc(CPAP_EPAP, ST_90P)->color));
            tmp += e90 - tmp;

            float i50 = day->calcMiddle(CPAP_IPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_IPAP, ST_MID), i50, i50 - tmp, day->calcMiddleLabel(CPAP_IPAP), getCalc(CPAP_IPAP, ST_MID)->color));
            tmp += i50 - tmp;

            float i90 = day->calcPercentile(CPAP_IPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_IPAP, ST_90P), i90, i90 - tmp, day->calcPercentileLabel(CPAP_IPAP), getCalc(CPAP_IPAP, ST_90P)->color));
            tmp += i90 - tmp;
        }
        slices.append(SummaryChartSlice(getCalc(CPAP_IPAPHi), ipap, ipap - tmp, schema::channel[CPAP_IPAPHi].label(), getCalc(CPAP_IPAPHi)->color));
    } else if ((mode == MODE_BILEVEL_AUTO_VARIABLE_PS) || (mode == MODE_ASV_VARIABLE_EPAP)) {
        float epap = day->settings_max(CPAP_EPAPLo);
        tmp = epap;

        slices.append(SummaryChartSlice(getCalc(CPAP_EPAPLo), epap, epap, schema::channel[CPAP_EPAPLo].label(), getCalc(CPAP_EPAPLo)->color));
        if (!day->summaryOnly()) {
            float e50 = day->calcMiddle(CPAP_EPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_EPAP, ST_MID), e50, e50 - tmp, day->calcMiddleLabel(CPAP_EPAP), getCalc(CPAP_EPAP, ST_MID)->color));
            tmp += e50 - tmp;

            float e90 = day->calcPercentile(CPAP_EPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_EPAP, ST_90P), e90, e90 - tmp, day->calcPercentileLabel(CPAP_EPAP), getCalc(CPAP_EPAP, ST_90P)->color));
            tmp += e90 - tmp;

            float i50 = day->calcMiddle(CPAP_IPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_IPAP, ST_MID), i50, i50 - tmp, day->calcMiddleLabel(CPAP_IPAP), getCalc(CPAP_IPAP, ST_MID)->color));
            tmp += i50 - tmp;

            float i90 = day->calcPercentile(CPAP_IPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_IPAP, ST_90P), i90, i90 - tmp, day->calcPercentileLabel(CPAP_IPAP), getCalc(CPAP_IPAP, ST_90P)->color));
            tmp += i90 - tmp;
        }
        float ipap = day->settings_max(CPAP_IPAPHi);
        slices.append(SummaryChartSlice(getCalc(CPAP_IPAPHi), ipap, ipap - tmp, schema::channel[CPAP_IPAPHi].label(), getCalc(CPAP_IPAPHi)->color));
    } else if (mode == MODE_ASV) {
        float epap = day->settings_max(CPAP_EPAP);
        tmp = epap;

        slices.append(SummaryChartSlice(getCalc(CPAP_EPAP), epap, epap, schema::channel[CPAP_EPAP].label(), getCalc(CPAP_EPAP)->color));
        if (!day->summaryOnly()) {
            float i50 = day->calcMiddle(CPAP_IPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_IPAP, ST_MID), i50, i50 - tmp, day->calcMiddleLabel(CPAP_IPAP), getCalc(CPAP_IPAP, ST_MID)->color));
            tmp += i50 - tmp;

            float i90 = day->calcPercentile(CPAP_IPAP);
            slices.append(SummaryChartSlice(getCalc(CPAP_IPAP, ST_90P), i90, i90 - tmp, day->calcPercentileLabel(CPAP_IPAP), getCalc(CPAP_IPAP, ST_90P)->color));
            tmp += i90 - tmp;
        }
        float ipap = day->settings_max(CPAP_IPAPHi);
        slices.append(SummaryChartSlice(getCalc(CPAP_IPAPHi), ipap, ipap - tmp, schema::channel[CPAP_IPAPHi].label(), getCalc(CPAP_IPAPHi)->color));
    }

}
