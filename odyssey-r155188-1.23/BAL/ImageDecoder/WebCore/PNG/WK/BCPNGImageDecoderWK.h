/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#ifndef PNGImageDecoder_h
#define PNGImageDecoder_h

#include "ImageDecoder.h"
#if ENABLE(APNG)
#include <png.h>
#endif
#include <wtf/OwnPtr.h>

namespace WebCore {

    class PNGImageReader;

    // This class decodes the PNG image format.
    class PNGImageDecoder : public ImageDecoder {
    public:
        PNGImageDecoder(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);
        virtual ~PNGImageDecoder();

        // ImageDecoder
        virtual String filenameExtension() const { return "png"; }
#if ENABLE(APNG)
        virtual size_t frameCount() override { return m_frameCount; }
        virtual int repetitionCount() const override { return m_playCount-1; }
#endif
        virtual bool isSizeAvailable();
        virtual bool setSize(unsigned width, unsigned height);
        virtual ImageFrame* frameBufferAtIndex(size_t index);
        // CAUTION: setFailed() deletes |m_reader|.  Be careful to avoid
        // accessing deleted memory, especially when calling this from inside
        // PNGImageReader!
        virtual bool setFailed();

        // Callbacks from libpng
        void headerAvailable();
        void rowAvailable(unsigned char* rowBuffer, unsigned rowIndex, int interlacePass);
        void pngComplete();
#if ENABLE(APNG)
        void readChunks(png_unknown_chunkp);
        void frameHeader();

        void init();
        virtual void clearFrameBufferCache(size_t clearBeforeFrame);
#endif
        bool isComplete() const
        {
            //return !m_frameBufferCache.isEmpty() && (m_frameBufferCache.first().status() == ImageFrame::FrameComplete);
            if (m_frameBufferCache.isEmpty())
                return false;

            for (auto& imageFrame : m_frameBufferCache) {
                if (imageFrame.status() != ImageFrame::FrameComplete)
                    return false;
            }

            return true;
        }

        bool isCompleteAtIndex(size_t index)
        {
            return (index < m_frameBufferCache.size() && m_frameBufferCache[index].status() == ImageFrame::FrameComplete);
        }

    private:
        // Decodes the image.  If |onlySize| is true, stops decoding after
        // calculating the image size.  If decoding fails but there is no more
        // data coming, sets the "decode failure" flag.
        void decode(bool onlySize, unsigned haltAtFrame);
#if ENABLE(APNG)
        void initFrameBuffer(size_t frameIndex);
        void frameComplete();
        int processingStart(png_unknown_chunkp);
        int processingFinish();
        void fallbackNotAnimated();
#endif
        OwnPtr<PNGImageReader> m_reader;
        bool m_doNothingOnFailure;
        unsigned m_currentFrame;
#if ENABLE(APNG)
        png_structp m_png;
        png_infop m_info;
        bool m_isAnimated;
        bool m_frameInfo;
        bool m_frameIsHidden;
        bool m_hasInfo;
        int m_gamma;
        size_t m_frameCount;
        unsigned m_playCount;
        unsigned m_totalFrames;
        unsigned m_sizePLTE;
        unsigned m_sizetRNS;
        unsigned m_sequenceNumber;
        unsigned m_width;
        unsigned m_height;
        unsigned m_xOffset;
        unsigned m_yOffset;
        unsigned m_delayNumerator;
        unsigned m_delayDenominator;
        unsigned m_dispose;
        unsigned m_blend;
        png_byte m_dataIHDR[12 + 13];
        png_byte m_dataPLTE[12 + 256 * 3];
        png_byte m_datatRNS[12 + 256];
#endif

    };

} // namespace WebCore

#endif
