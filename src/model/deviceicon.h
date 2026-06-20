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

#ifndef DEVICEICON_H
#define DEVICEICON_H

#include <QByteArray>
#include <QString>

#include <utility>

class QIcon;
class QPainter;
class QRectF;

// A device's symbol, held as raw bytes plus a format tag ("svg", "png", …) so
// it works uniformly whether it came from a bundled resource, a file the user
// chose, or bytes embedded in a saved plot. SVG is preferred (scales cleanly);
// raster formats are supported for convenience.
class DeviceIcon
{
public:
    DeviceIcon() = default;
    DeviceIcon(QByteArray data, QString format)
        : m_data(std::move(data)), m_format(std::move(format).toLower())
    {
    }

    // Read an icon from a path — a Qt resource (":/plot/x.svg") or a file on
    // disk. The format is taken from the suffix. Returns an empty icon on error.
    static DeviceIcon fromPath(const QString &path);

    bool isValid() const { return !m_data.isEmpty(); }
    bool isSvg() const { return m_format == QLatin1String("svg"); }
    const QByteArray &data() const { return m_data; }
    QString format() const { return m_format; }

    // Render into `rect` (used on the canvas / in the PDF), and produce a QIcon
    // for the device palette.
    void render(QPainter *painter, const QRectF &rect) const;
    QIcon toIcon() const;

private:
    QByteArray m_data;
    QString m_format;
};

#endif // DEVICEICON_H