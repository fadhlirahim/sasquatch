/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA 02111-1307, USA.
**
****************************************************************************/

import QtQuick 2.0

Item {
    id: root
    property alias sourceItem: effectsource.sourceItem
    property real intensity: 1
    property bool waving: true
    anchors.top: sourceItem.bottom
    width: sourceItem.width
    height: sourceItem.height

    ShaderEffect {
        anchors.fill: parent
        property variant source: effectsource
        property real f: 0
        property real f2: 0
        property alias intensity: root.intensity
        smooth: true

        ShaderEffectSource {
            id: effectsource
            hideSource: false
            smooth: true
        }

        fragmentShader:
            "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform lowp float qt_Opacity;
            uniform highp float f;
            uniform highp float f2;
            uniform highp float intensity;

            void main() {
                const highp float twopi = 3.141592653589 * 2.0;

                highp float distanceFactorToPhase = pow(qt_TexCoord0.y + 0.5, 8.0) * 5.0;
                highp float ofx = sin(f * twopi + distanceFactorToPhase) / 100.0;
                highp float ofy = sin(f2 * twopi + distanceFactorToPhase * qt_TexCoord0.x) / 60.0;

                highp float intensityDampingFactor = (qt_TexCoord0.x + 0.1) * (qt_TexCoord0.y + 0.2);
                highp float distanceFactor = (1.0 - qt_TexCoord0.y) * 4.0 * intensity * intensityDampingFactor;

                ofx *= distanceFactor;
                ofy *= distanceFactor;

                highp float x = qt_TexCoord0.x + ofx;
                highp float y = 1.0 - qt_TexCoord0.y + ofy;

                highp float fake = (sin((ofy + ofx) * twopi) + 0.5) * 0.05 * (1.2 - qt_TexCoord0.y) * intensity * intensityDampingFactor;

                highp vec4 pix =
                    texture2D(source, vec2(x, y)) * 0.6 +
                    texture2D(source, vec2(x-fake, y)) * 0.15 +
                    texture2D(source, vec2(x, y-fake)) * 0.15 +
                    texture2D(source, vec2(x+fake, y)) * 0.15 +
                    texture2D(source, vec2(x, y+fake)) * 0.15;

                highp float darken = 0.6 - (ofx - ofy) / 2.0;
                pix.b *= 1.2 * darken;
                pix.r *= 0.9 * darken;
                pix.g *= darken;

                gl_FragColor = qt_Opacity * vec4(pix.r, pix.g, pix.b, 1.0);
            }
            "

        NumberAnimation on f {
            running: root.waving
            loops: Animation.Infinite
            from: 0
            to: 1
            duration: 2410
        }
        NumberAnimation on f2 {
            running: root.waving
            loops: Animation.Infinite
            from: 0
            to: 1
            duration: 1754
        }
    }
}
