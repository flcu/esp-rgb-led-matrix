/* MIT License
 *
 * Copyright (c) 2019 - 2021 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @brief  LED matrix
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup gfx
 *
 * @{
 */

#ifndef __LEDMATRIX_H__
#define __LEDMATRIX_H__

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <IGfx.hpp>
#include <ColorDef.hpp>
#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <Logging.h>

#include "Board.h"

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Specific LED matrix.
 */
class LedMatrix : public IGfx
{
public:

    /**
     * Get LED matrix instance.
     *
     * @return LED matrix
     */
    static LedMatrix& getInstance()
    {
        static LedMatrix instance; /* singleton idiom to force initialization in the first usage. */

        return instance;
    }

    /**
     * Initialize base driver for the LED matrix.
     *
     * @return If successful, returns true otherwise false.
     */
    bool begin()
    {
        m_tft.init();

        m_tft.fillScreen(TFT_BLACK);
        m_tft.setCursor(0, 0, 4);
        m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
        m_tft.println("TFT ready.\n");
        sleep(3U);

        LOG_INFO("LedMatrix TFT is up.");

        return true;
    }

    /**
     * Show internal framebuffer on physical LED matrix.
     */
    void show()
    {
        //m_strip.Show();
        return;
    }

    /**
     * LED matrix is ready, when the last physical pixel update is finished.
     *
     * @return If ready for another update via show(), it will return true otherwise false.
     */
    bool isReady() const
    {
        return true;
        //return m_strip.CanShow();
    }

    /**
     * Set brightness from 0 to 255.
     *
     * @param[in] brightness    Brightness value [0; 255]
     */
    void setBrightness(uint8_t brightness)
    {
        /* To protect the the electronic parts, the brigntness will be scaled down
         * according to the max. supply current.
        const uint8_t SAFE_BRIGHTNESS =
            (Board::LedMatrix::supplyCurrentMax * brightness) /
            (Board::LedMatrix::maxCurrentPerLed * Board::LedMatrix::width *Board::LedMatrix::height);

        m_strip.SetBrightness(SAFE_BRIGHTNESS);
         */
        return;
    }

    /**
     * Clear LED matrix.
     */
    void clear()
    {
        //m_strip.ClearTo(ColorDef::BLACK);
        m_tft.fillScreen(TFT_BLACK);
        return;
    }

    /**
     * Get pixel color at given position.
     *
     * @param[in] x x-coordinate
     * @param[in] y y-coordinate
     *
     * @return Color in RGB888 format.
     */
    Color getColor(int16_t x, int16_t y) const final;

private:

    /** Pixel representation of the LED matrix */
    TFT_eSPI                                                m_tft;

    /** Panel topology, used to map coordinates to the framebuffer. */
    //NeoTopology<ColumnMajorAlternatingLayout>               m_topo;

    /**
     * Construct LED matrix.
     */
    LedMatrix();

    /**
     * Destroys LED matrix.
     */
    ~LedMatrix();

    LedMatrix(const LedMatrix& matrix);
    LedMatrix& operator=(const LedMatrix& matrix);

    /**
     * Draw a single pixel in the matrix.
     *
     * @param[in] x     x-coordinate
     * @param[in] y     y-coordinate
     * @param[in] color Pixel color in RGB888 format
     */
    void drawPixel(int16_t x, int16_t y, const Color& color) final
    {
        if ((0 <= x) &&
            (Board::LedMatrix::width > x) &&
            (0 <= y) &&
            (Board::LedMatrix::height > y))
        {
            m_tft.fillRect(( y*7U ) + 39 , (TFT_HEIGHT - (x*7U) ) - 8, 6U, 6U, color);
            //m_strip.SetPixelColor(m_topo.Map(x, y), htmlColor);
        }

        return;
    }

    /**
     * Dim color to black.
     * A dim ratio of 0 means no change.
     * 
     * Note, the base colors may be destroyed, depends on the color type.
     *
     * @param[in] x     x-coordinate
     * @param[in] y     y-coordinate
     * @param[in] ratio Dim ration [0; 255]
     */
    void dimPixel(int16_t x, int16_t y, uint8_t ratio) final
    {
        if ((0 <= x) &&
            (Board::LedMatrix::width > x) &&
            (0 <= y) &&
            (Board::LedMatrix::height > y))
        {
            //RgbColor rgbColor = m_strip.GetPixelColor(m_topo.Map(x, y)).Dim(UINT8_MAX - ratio);

            //m_strip.SetPixelColor(m_topo.Map(x, y), rgbColor);
        }

        return;
    }
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* __LEDMATRIX_H__ */

/** @} */