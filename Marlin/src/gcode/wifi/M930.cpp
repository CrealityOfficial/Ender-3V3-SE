/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(HAS_CREALITY_WIFI)

#include "../gcode.h"
#if ENABLED(RTS_AVAILABLE)
  #include "../../lcd/dwin/lcd_rts.h"
#elif ENABLED(DWIN_CREALITY_LCD)
  #include "../../lcd/dwin/e3v2/dwin.h"
#endif

/**
 * M930: WIFI Box Function
 */
void GcodeSuite::M930()
{
  const int16_t WIFI_F = parser.intval('F');

  if(WIFI_F)
  {
    const int16_t WIFI_S = parser.intval('S');
    if(WIFI_S == 1)
    {
      #if ENABLED(RTS_AVAILABLE)
        if(flag_counter_wifireset)
        {
          flag_counter_wifireset = false;
          rtscheck.RTS_SndData(ExchangePageBase + 33, ExchangepageAddr);
          change_page_font = 33;
        }
        rtscheck.RTS_SndData(1, WIFI_CONNECTED_DISPLAY_ICON_VP);
      #endif
    }
    else if(WIFI_S == 2)
    {
      #if ENABLED(RTS_AVAILABLE)
        rtscheck.RTS_SndData(0, ADV_SETTING_WIFI_ICON_VP);
        rtscheck.RTS_SndData(1, WIFI_CONNECTED_DISPLAY_ICON_VP);
      #endif
    }
    else if(WIFI_S == 3)
    {
      #if ENABLED(RTS_AVAILABLE)
        rtscheck.RTS_SndData(1, ADV_SETTING_WIFI_ICON_VP);
        rtscheck.RTS_SndData(0, WIFI_CONNECTED_DISPLAY_ICON_VP);
      #endif
    }
  }
}

#endif // HAS_CREALITY_WIFI
