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
 * @brief  Gruenbeck plugin.
 * @author Yann Le Glaz <yann_le@web.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "GruenbeckPlugin.h"
#include "RestApi.h"
#include "AsyncHttpClient.h"
#include "FileSystem.h"

#include <ArduinoJson.h>
#include <Logging.h>
#include <JsonFile.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/* Initialize image path. */
const char* GruenbeckPlugin::IMAGE_PATH = "/images/gruenbeck.bmp";

/* Initialize plugin topic. */
const char* GruenbeckPlugin::TOPIC      = "/ipAddress";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

void GruenbeckPlugin::getTopics(JsonArray& topics) const
{
    (void)topics.add(TOPIC);
}

bool GruenbeckPlugin::getTopic(const String& topic, JsonObject& value) const
{
    bool isSuccessful = false;

    if (0U != topic.equals(TOPIC))
    {
        String  ipAddress   = getIPAddress();

        value["ipAddress"] = ipAddress;

        isSuccessful = true;
    }

    return isSuccessful;
}

bool GruenbeckPlugin::setTopic(const String& topic, const JsonObject& value)
{
    bool isSuccessful = false;

    if (0U != topic.equals(TOPIC))
    {
        String  ipAddress;

        if (false == value["set"].isNull())
        {
            ipAddress = value["set"].as<String>();
            isSuccessful = true;
        }

        if (true == isSuccessful)
        {
            setIPAddress(ipAddress);
        }
    }

    return isSuccessful;
}

void GruenbeckPlugin::active(IGfx& gfx)
{
    lock();

    gfx.fillScreen(ColorDef::BLACK);

    if (nullptr == m_iconCanvas)
    {
        m_iconCanvas = new Canvas(ICON_WIDTH, ICON_HEIGHT, 0, 0);

        if (nullptr != m_iconCanvas)
        {
            (void)m_iconCanvas->addWidget(m_bitmapWidget);

            /* Load  icon from filesystem. */
            (void)m_bitmapWidget.load(FILESYSTEM, IMAGE_PATH);

            m_iconCanvas->update(gfx);
        }
    }
    else
    {
        m_iconCanvas->update(gfx);
    }

    if (nullptr == m_textCanvas)
    {
        m_textCanvas = new Canvas(gfx.getWidth() - ICON_WIDTH, gfx.getHeight(), ICON_WIDTH, 0);

        if (nullptr != m_textCanvas)
        {
            (void)m_textCanvas->addWidget(m_textWidget);

            m_textCanvas->update(gfx);
        }
    }
    else
    {
        m_textCanvas->update(gfx);
    }

    unlock();

    return;
}

void GruenbeckPlugin::inactive()
{
    /* Nothing to do */
    return;
}

void GruenbeckPlugin::update(IGfx& gfx)
{
    lock();

    if (false != m_httpResponseReceived)
    {
        m_textWidget.setFormatStr("\\calign" + m_relevantResponsePart + "%");
        gfx.fillScreen(ColorDef::BLACK);

        if (nullptr != m_iconCanvas)
        {
            m_iconCanvas->update(gfx);
        }

        if (nullptr != m_textCanvas)
        {
            m_textCanvas->update(gfx);
        }

        m_relevantResponsePart = "";

        m_httpResponseReceived = false;
    }

    unlock();

    return;
}

void GruenbeckPlugin::start()
{
    lock();

    /* Try to load configuration. If there is no configuration available, a default configuration
     * will be created.
     */
    if (false == loadConfiguration())
    {
        if (false == saveConfiguration())
        {
            LOG_WARNING("Failed to create initial configuration file %s.", getFullPathToConfiguration().c_str());
        }
    }

    initHttpClient();
    if (false == startHttpRequest())
    {
        /* If a request fails, show a '?' */
        m_textWidget.setFormatStr("\\calign?");

        m_requestTimer.start(UPDATE_PERIOD_SHORT);
    }
    else
    {
        m_requestTimer.start(UPDATE_PERIOD);
    }

    unlock();

    return;
}

void GruenbeckPlugin::stop()
{
    String configurationFilename = getFullPathToConfiguration();

    lock();

    m_requestTimer.stop();

    if (false != FILESYSTEM.remove(configurationFilename))
    {
        LOG_INFO("File %s removed", configurationFilename.c_str());
    }

    unlock();

    return;
}

void GruenbeckPlugin::process()
{
    lock();

    if ((true == m_requestTimer.isTimerRunning()) &&
        (true == m_requestTimer.isTimeout()))
    {
        if (false == startHttpRequest())
        {
            /* If a request fails, show a '?' */
            m_textWidget.setFormatStr("\\calign?");

            m_requestTimer.start(UPDATE_PERIOD_SHORT);
        }
        else
        {
            m_requestTimer.start(UPDATE_PERIOD);
        }
    }

    unlock();

    return;
}

String GruenbeckPlugin::getIPAddress() const
{
    String ipAddress;

    lock();
    ipAddress = m_ipAddress;
    unlock();

    return ipAddress;
}

void GruenbeckPlugin::setIPAddress(const String& ipAddress)
{
    lock();
    m_ipAddress = ipAddress;
    (void)saveConfiguration();
    unlock();

    return;
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

bool GruenbeckPlugin::startHttpRequest()
{
    bool status  = false;

    if (0 < m_ipAddress.length())
    {
        String url = String("http://") + m_ipAddress + "/mux_http";

        if (true == m_client.begin(url))
        {
            m_client.addPar("id","42");
            m_client.addPar("show","D_Y_10_1~");

            if (false == m_client.POST())
            {
                LOG_WARNING("POST %s failed.", url.c_str());
            }
            else
            {
                status = true;
            }
        }
    }

    return status;
}

void GruenbeckPlugin::initHttpClient()
{
    m_client.regOnResponse([this](const HttpResponse& rsp){
        /* Structure of response-payload for requesting D_Y_10_1
         *
         * <data><code>ok</code><D_Y_10_1>XYZ</D_Y_10_1></data>
         *
         * <data><code>ok</code><D_Y_10_1>  = 31 bytes
         * XYZ                              = 3 byte (relevant data)
         * </D_Y_10_1></data>               = 18 bytes
         */

        /* Start index of relevant data */
        const uint32_t  START_INDEX_OF_RELEVANT_DATA    = 31U;

        /* Length of relevant data */
        const uint32_t  RELEVANT_DATA_LENGTH            = 3U;

        size_t          payloadSize                     = 0U;
        const char*     payload                         = reinterpret_cast<const char*>(rsp.getPayload(payloadSize));
        char            restCapacity[RELEVANT_DATA_LENGTH + 1];

        if (payloadSize >= (START_INDEX_OF_RELEVANT_DATA + RELEVANT_DATA_LENGTH))
        {
            memcpy(restCapacity, &payload[START_INDEX_OF_RELEVANT_DATA], RELEVANT_DATA_LENGTH);
            restCapacity[RELEVANT_DATA_LENGTH] = '\0';
        }
        else
        {
            restCapacity[0] = '?';
            restCapacity[1] = '\0';
        }

        lock();
        m_relevantResponsePart = restCapacity;
        m_httpResponseReceived = true;
        unlock();
    });

    m_client.regOnClosed([this]() {
        LOG_INFO("Connection closed.");

        lock();
        if (true == m_isConnectionError)
        {
            /* If a request fails, show a '?' */
            m_textWidget.setFormatStr("\\calign?");

            m_requestTimer.start(UPDATE_PERIOD_SHORT);
        }
        m_isConnectionError = false;
        unlock();
    });

    m_client.regOnError([this]() {
        LOG_WARNING("Connection error happened.");

        lock();
        m_isConnectionError = true;
        unlock();
    });
}

bool GruenbeckPlugin::saveConfiguration() const
{
    bool                status                  = true;
    JsonFile            jsonFile(FILESYSTEM);
    const size_t        JSON_DOC_SIZE           = 512U;
    DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
    String              configurationFilename   = getFullPathToConfiguration();

    jsonDoc["gruenbeckIP"] = m_ipAddress;
    
    if (false == jsonFile.save(configurationFilename, jsonDoc))
    {
        LOG_WARNING("Failed to save file %s.", configurationFilename.c_str());
        status = false;
    }
    else
    {
        LOG_INFO("File %s saved.", configurationFilename.c_str());
    }

    return status;
}

bool GruenbeckPlugin::loadConfiguration()
{
    bool                status                  = true;
    JsonFile            jsonFile(FILESYSTEM);
    const size_t        JSON_DOC_SIZE           = 512U;
    DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
    String              configurationFilename   = getFullPathToConfiguration();

    if (false == jsonFile.load(configurationFilename, jsonDoc))
    {
        LOG_WARNING("Failed to load file %s.", configurationFilename.c_str());
        status = false;
    }
    else
    {
        m_ipAddress = jsonDoc["gruenbeckIP"].as<String>();
    }

    return status;
}

void GruenbeckPlugin::lock() const
{
    if (nullptr != m_xMutex)
    {
        (void)xSemaphoreTakeRecursive(m_xMutex, portMAX_DELAY);
    }

    return;
}

void GruenbeckPlugin::unlock() const
{
    if (nullptr != m_xMutex)
    {
        (void)xSemaphoreGiveRecursive(m_xMutex);
    }

    return;
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
