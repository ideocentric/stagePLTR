/*
 * stagePLTR — stage plots and tech riders for bands.
 * Copyright (C) 2026 Matt Comeione
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pageconfig.h"

#include <QSettings>
#include <QtTest>

class TestPageConfig : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void letterPortraitPixels();
    void a4LandscapePixels();
    void jsonRoundTrip();
    void settingsRoundTrip();
    void cleanupTestCase();
};

void TestPageConfig::initTestCase()
{
    // Use a test-only org/app so QSettings I/O never touches the real app's
    // stored preferences.
    QCoreApplication::setOrganizationName(QStringLiteral("stagePLTR-test"));
    QCoreApplication::setApplicationName(QStringLiteral("stagePLTR-unit"));
    QSettings().clear();
}

void TestPageConfig::letterPortraitPixels()
{
    const QSizeF px = PageConfig{QPageSize::Letter, QPageLayout::Portrait}.pixelSize();
    QCOMPARE(qRound(px.width()), 816);    // 8.5in * 96dpi
    QCOMPARE(qRound(px.height()), 1056);  // 11in  * 96dpi
}

void TestPageConfig::a4LandscapePixels()
{
    const QSizeF px = PageConfig{QPageSize::A4, QPageLayout::Landscape}.pixelSize();
    QCOMPARE(qRound(px.width()), 1123);   // A4 long edge in px (297mm @ 96dpi)
    QCOMPARE(qRound(px.height()), 793);   // A4 short edge in px (210mm @ 96dpi)
}

void TestPageConfig::jsonRoundTrip()
{
    const PageConfig c{QPageSize::A4, QPageLayout::Landscape};
    QVERIFY(PageConfig::fromJson(c.toJson()) == c);

    const PageConfig d{QPageSize::Letter, QPageLayout::Portrait};
    QVERIFY(PageConfig::fromJson(d.toJson()) == d);
}

void TestPageConfig::settingsRoundTrip()
{
    const PageConfig c{QPageSize::A4, QPageLayout::Landscape};
    pageconfig::save(DocumentFeature::StagePlot, c);
    QVERIFY(pageconfig::load(DocumentFeature::StagePlot) == c);
}

void TestPageConfig::cleanupTestCase()
{
    QSettings().clear();
}

QTEST_GUILESS_MAIN(TestPageConfig)
#include "test_pageconfig.moc"