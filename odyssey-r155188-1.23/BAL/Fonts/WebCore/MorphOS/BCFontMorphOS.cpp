/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "Font.h"

#include "GlyphBuffer.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "Pattern.h"
#include "Logging.h"
#include "SimpleFontData.h"
#include "cairo.h"

namespace WebCore {

#define IS_HIGH_SURROGATE(u)  ((UChar)(u) >= (UChar)0xd800 && (UChar)(u) <= (UChar)0xdbff)
#define IS_LOW_SURROGATE(u)  ((UChar)(u) >= (UChar)0xdc00 && (UChar)(u) <= (UChar)0xdfff)

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    BalNotImplemented();
}


float Font::floatWidthForComplexText(const TextRun& run) const
{
    BalNotImplemented();
    return 0.0;
}

int Font::offsetForPositionForComplexText(const TextRun& run, int x, bool includePartialGlyphs) const
{
    BalNotImplemented();
    return 0;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const IntPoint& point, int h, int from, int to) const
{
    BalNotImplemented();
    return FloatRect();
}

void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer,
                      int from, int numGlyphs, const FloatPoint& point) const
{
    cairo_t* cr = context->platformContext();
    cairo_save(cr);

    font->setFont(cr);

    GlyphBufferGlyph* glyphs = (GlyphBufferGlyph*)glyphBuffer.glyphs(from);

    float offset = point.x();
    for (int i = 0; i < numGlyphs; i++) {
        glyphs[i].x = offset;
        glyphs[i].y = point.y();
        offset += glyphBuffer.advanceAt(from + i);
    }

    Color fillColor = context->fillColor();

    // Text shadow, inspired by FontMac
    IntSize shadowSize;
    int shadowBlur = 0;
    Color shadowColor;
    bool hasShadow = context->textDrawingMode() == cTextFill &&
        context->getShadow(shadowSize, shadowBlur, shadowColor);

    // TODO: Blur support
    if (hasShadow) {
        // Disable graphics context shadows (not yet implemented) and paint them manually
        context->clearShadow();
        Color shadowFillColor(shadowColor.red(), shadowColor.green(), shadowColor.blue(), shadowColor.alpha() * fillColor.alpha() / 255);
        cairo_save(cr);

        float red, green, blue, alpha;
        shadowFillColor.getRGBA(red, green, blue, alpha);
        cairo_set_source_rgba(cr, red, green, blue, alpha);

        cairo_translate(cr, shadowSize.width(), shadowSize.height());
        cairo_show_glyphs(cr, glyphs, numGlyphs);

        cairo_restore(cr);
    }

    if (context->textDrawingMode() & cTextFill) {
        if (context->fillGradient()) {
            cairo_set_source(cr, context->fillGradient()->platformGradient());
            if (context->getAlpha() < 1.0f) {
                cairo_push_group(cr);
                cairo_paint_with_alpha(cr, context->getAlpha());
                cairo_pop_group_to_source(cr);
            }
        } else if (context->fillPattern()) {
            TransformationMatrix affine;
            cairo_set_source(cr, context->fillPattern()->createPlatformPattern(affine));
            if (context->getAlpha() < 1.0f) {
                cairo_push_group(cr);
                cairo_paint_with_alpha(cr, context->getAlpha());
                cairo_pop_group_to_source(cr);
            }
        } else {
            float red, green, blue, alpha;
            fillColor.getRGBA(red, green, blue, alpha);
            cairo_set_source_rgba(cr, red, green, blue, alpha * context->getAlpha());
        }
        cairo_show_glyphs(cr, glyphs, numGlyphs);
    }

    if (context->textDrawingMode() & cTextStroke) {
        if (context->strokeGradient()) {
            cairo_set_source(cr, context->strokeGradient()->platformGradient());
            if (context->getAlpha() < 1.0f) {
                cairo_push_group(cr);
                cairo_paint_with_alpha(cr, context->getAlpha());
                cairo_pop_group_to_source(cr);
            }
        } else if (context->strokePattern()) {
            TransformationMatrix affine;
            cairo_set_source(cr, context->strokePattern()->createPlatformPattern(affine));
            if (context->getAlpha() < 1.0f) {
                cairo_push_group(cr);
                cairo_paint_with_alpha(cr, context->getAlpha());
                cairo_pop_group_to_source(cr);
            }
        } else {
            Color strokeColor = context->strokeColor();
            float red, green, blue, alpha;
            strokeColor.getRGBA(red, green, blue, alpha);
            cairo_set_source_rgba(cr, red, green, blue, alpha * context->getAlpha());
        }
        cairo_glyph_path(cr, glyphs, numGlyphs);
        cairo_set_line_width(cr, context->strokeThickness());
        cairo_stroke(cr);
    }

    // Re-enable the platform shadow we disabled earlier
    if (hasShadow)
        context->setShadow(shadowSize, shadowBlur, shadowColor);

    cairo_restore(cr);
}

}
