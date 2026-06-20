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

#include "deviceicon.h"

#include <QBuffer>
#include <QImage>
#include <QtTest>

class TestDeviceIcon : public QObject
{
    Q_OBJECT

private slots:
    void svgFromPath();
    void rasterFromData();
    void missingPathIsInvalid();
};

void TestDeviceIcon::svgFromPath()
{
    const DeviceIcon icon =
        DeviceIcon::fromPath(QStringLiteral(STAGEPLT_ASSETS_DIR "/mic-straight-overhead.svg"));
    QVERIFY(icon.isValid());
    QVERIFY(icon.isSvg());
    QCOMPARE(icon.format(), QStringLiteral("svg"));
    QVERIFY(!icon.toIcon().isNull());
}

void TestDeviceIcon::rasterFromData()
{
    QImage image(8, 8, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    const DeviceIcon icon(bytes, QStringLiteral("png"));
    QVERIFY(icon.isValid());
    QVERIFY(!icon.isSvg());
    QVERIFY(!icon.toIcon().isNull());
}

void TestDeviceIcon::missingPathIsInvalid()
{
    QVERIFY(!DeviceIcon::fromPath(QStringLiteral("/no/such/icon.svg")).isValid());
}

QTEST_MAIN(TestDeviceIcon)
#include "test_deviceicon.moc"