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

#include "documentinfo.h"

#include <QtTest>

class TestDocumentInfo : public QObject
{
    Q_OBJECT

private slots:
    void fullRoundTrip();
    void emptyRoundTrip();
};

void TestDocumentInfo::fullRoundTrip()
{
    DocumentInfo d;
    d.bandName = QStringLiteral("The Wandering Hours");
    d.date = QDate(2026, 6, 16);
    d.logoData = QByteArray("\x89PNG-bytes", 10);
    d.logoFormat = QStringLiteral("png");
    d.showPhantomInList = true;
    d.allowListOverflow = true;

    const DocumentInfo r = DocumentInfo::fromJson(d.toJson());
    QCOMPARE(r.bandName, d.bandName);
    QCOMPARE(r.date, d.date);
    QCOMPARE(r.logoData, d.logoData);
    QCOMPARE(r.logoFormat, d.logoFormat);
    QCOMPARE(r.showPhantomInList, true);
    QCOMPARE(r.allowListOverflow, true);
    QVERIFY(r.hasLogo());
    QVERIFY(r.hasContent());
}

void TestDocumentInfo::emptyRoundTrip()
{
    const DocumentInfo r = DocumentInfo::fromJson(DocumentInfo().toJson());
    QVERIFY(r.bandName.isEmpty());
    QVERIFY(!r.hasLogo());
    QVERIFY(!r.hasContent());
    QCOMPARE(r.showPhantomInList, false);
    QCOMPARE(r.allowListOverflow, false);
}

QTEST_APPLESS_MAIN(TestDocumentInfo)
#include "test_documentinfo.moc"