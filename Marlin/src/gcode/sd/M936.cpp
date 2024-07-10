/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
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
 *
 */
#include "../../inc/MarlinConfig.h"
#include "../../core/serial.h"
#include "../../../Configuration.h"
#include "../gcode.h"
#include "stdio.h"
#include "string.h"
#include "../parser.h"

#if ENABLED(IIC_BL24CXX_EEPROM)
  #include "../../libs/BL24CXX.h"
#endif

#include "../../lcd/dwin/dwin_lcd.h"

#define ENABLE_OTA
#if ENABLED(ENABLE_OTA)
typedef unsigned char u8;
u8* strchr_pointer;

#define OTA_FLAG_EEPROM  90

/**
 * M936: OTA update firmware.
 */
void GcodeSuite::M936()
{
  // TODO: feedrate support?
  #if ENABLED(IIC_BL24CXX_EEPROM)
    static u8 ota_updata_flag = 0X00;
  #endif
  // char temp[20];
  if (parser.seenval('V'))
  {
    const int16_t oatvalue = parser.value_celsius();
    switch (oatvalue)
    {
      case 2:
        #if ENABLED(IIC_BL24CXX_EEPROM)
          ota_updata_flag = 0X01;
          BL24CXX::write(OTA_FLAG_EEPROM, &ota_updata_flag, sizeof(ota_updata_flag));
        #endif
        delay(10);
        SERIAL_ECHOLN("M936 V2");
        // Set the OTA upgrade flag bit to 1, indicating that the NEXT power-on requires OTA upgrade.
        delay(50);
        SERIAL_ECHOLN("\r\n Motherboard upgrade \r\n");
        delay(50);
        // The MCU resets and enters the BOOTLOADER
        // NVIC_SystemReset();
        DWIN_Backlight_SetLuminance(0);
        break;

      case 3:
        #if ENABLED(IIC_BL24CXX_EEPROM)
          ota_updata_flag = 0X02;
          BL24CXX::write(OTA_FLAG_EEPROM, &ota_updata_flag, sizeof(ota_updata_flag));
        #endif
        delay(10);
        SERIAL_ECHOLN("M936 V3");
        // Set the OTA upgrade flag bit to 1, indicating that the NEXT power-on requires OTA upgrade.
        delay(50);
        SERIAL_ECHOLN("\r\n DWIN upgrade！！ \r\n");
        delay(50);
        // The MCU resets and enters the BOOTLOADER
        // NVIC_SystemReset();
        DWIN_Backlight_SetLuminance(255);
        break;

      default:
        break;
    }
  }
}

#endif // DIRECT_STEPPING
