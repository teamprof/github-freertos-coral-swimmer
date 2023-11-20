/* Copyright 2023 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include "libs/base/i2c.h"

using namespace coralmicro;

// #define I2C_DEV_ADDR ((uint8_t)0x55) // device address

class I2cDevice
{
public:
  I2cDevice(I2c i2c) : config(I2cGetDefaultConfig(i2c)) {}

  bool init(void)
  {
    return I2cInitController(config);
  }

  bool read(uint8_t address, uint8_t *buffer, size_t count)
  {
    return I2cControllerRead(config, address, buffer, count);
  }

  bool write(uint8_t address, const uint8_t *buffer, size_t count)
  {
    return I2cControllerWrite(config, address, (uint8_t *)buffer, count);
  }

  bool writeRead(uint8_t address,
                 const uint8_t *wbuffer, size_t wcount,
                 uint8_t *rbuffer, size_t rcount)
  {
    if (I2cControllerWrite(config, address, (uint8_t *)wbuffer, wcount))
    {
      return I2cControllerRead(config, address, rbuffer, rcount);
    }
    return false;
  }

protected:
  I2cConfig config;
};
