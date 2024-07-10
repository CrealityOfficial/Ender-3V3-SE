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

#include "../gcode.h"
#include "../../module/printcounter.h"
#include "../../lcd/dwin/e3v2/dwin.h"
#include "../../feature/powerloss.h"
#include "../queue.h"

/**
 * M79: cloud print statistics
 */
void GcodeSuite::M79()
{
  if(parser.seenval('S'))
  {
    const int16_t cloudvalue = parser.value_celsius();
    switch (cloudvalue)
    {
      case 0:
        // 0:cloud connect
        #if ENABLED(DWIN_CREALITY_LCD)
          if(recovery.info.sd_printing_flag == false)
          {
            // rtscheck.RTS_SndData(1, WIFI_CONNECTED_DISPLAY_ICON_VP);
          }
        #endif
        break;

      case 1:
        // 1:cloud print satrt
        #if ENABLED(DWIN_CREALITY_LCD)
          if(recovery.info.sd_printing_flag == false)
          {
            // rock_20210831 Solve the problem of remaining time not being zero.
            _remain_time=0;
            // Show the remaining time
            Draw_Print_ProgressRemain();
            Cloud_Progress_Bar=0;
            print_job_timer.start();
            // Cloud printing start flag bit
            HMI_flag.cloud_printing_flag=true;
            // Enable automatic compensation function rock_2021.10.29
            process_subcommands_now_P(PSTR("M420 S1 Z10"));
          }
        #endif
        break;

      case 2:
        // 2:cloud print pause
        #if ENABLED(DWIN_CREALITY_LCD)
          if(!recovery.info.sd_printing_flag && !HMI_flag.filement_resume_flag)
          {
            // Update_Time_Value = 0;
            print_job_timer.pause();
            queue.inject_P(PSTR("M25"));
            if(HMI_flag.cloud_printing_flag && (checkkey = PrintProcess))
            {
              // ICONS continue to print
              ICON_Continue();
            }
          }
        #endif
        break;

      case 3:
        // 3:cloud print resume
        #if ENABLED(DWIN_CREALITY_LCD)
          if(recovery.info.sd_printing_flag == false)
          {
            // Update_Time_Value = 0;
            if(false == HMI_flag.filement_resume_flag)
            {
              print_job_timer.start();
              queue.inject_P(PSTR("M24"));
              if(HMI_flag.cloud_printing_flag)
              {
                // Pause screen
                ICON_Pause();
              }
            }
            // // The APP can also exit the power-off and resume interface
            // if(!HMI_flag.filement_resume_flag)
            // {
            //   // resume
            //   HMI_flag.cutting_line_flag = false;
            //   if(HMI_flag.cloud_printing_flag)
            //   {
            //     gcode.process_subcommands_now_P(PSTR("M24"));
            //     Goto_PrintProcess();
            //     // Pause screen
            //     ICON_Pause();
            //   }
            // }
            // else queue.clear();
          }
          else
          {
            // Cloud control SD print pause recovery
          }
        #endif
        break;

      case 4:
        // 4:cloud print stop
        #if ENABLED(DWIN_CREALITY_LCD)
          if(recovery.info.sd_printing_flag == false)
          {
            print_job_timer.stop();
            // rock_20210831 Solve the problem of remaining time not being clear.
            _remain_time = 0;
            // Added a jump page to stop printing
            Draw_Print_ProgressRemain();
            // Cloud printing start flag bit
            HMI_flag.cloud_printing_flag = false;
            // queue.inject_P(PSTR("G1 F1200 X0 Y0"));
            // card.flag.abort_sd_printing = true;
            Goto_MainMenu();
            // rock_20210729
            // gcode.process_subcommands_now_P(PSTR(EVENT_GCODE_SD_ABORT));
          }
          else 
          {
            // Cloud control SD card printing stops.
            // abortFilePrintSoon();
            card.flag.abort_sd_printing = true;
            // rock_20210729
            gcode.process_subcommands_now_P(PSTR(EVENT_GCODE_SD_ABORT));
          }
        #endif
        break;

      case 5:
        // 5:cloud print finish
        #if ENABLED(DWIN_CREALITY_LCD)
          if(recovery.info.sd_printing_flag == false)
          {
            print_job_timer.stop();
            //rock_20210831 Solve the problem of remaining time not being clear.
            _remain_time = 0;
            Draw_Print_ProgressRemain();
            // Added a jump page to stop printing
            Goto_MainMenu();
            // Cloud printing start flag bit
            HMI_flag.cloud_printing_flag = false;
            // rock_20210729
            gcode.process_subcommands_now_P(PSTR(EVENT_GCODE_SD_ABORT));
          }
          else 
          {
            // Cloud control SD card printing is complete.
            // Handle end of file reached
            card.fileHasFinished();
          }
        #endif
        break;

      default:
        break;
    }
  }

  if(parser.seenval('T'))
  {
    Cloud_Progress_Bar = parser.value_celsius();
    #if ENABLED(RTS_AVAILABLE)
      rtscheck.RTS_SndData(Cloud_Progress_Bar, PRINT_PROCESS_VP);
      rtscheck.RTS_SndData(Cloud_Progress_Bar, PRINT_PROCESS_ICON_VP);
    #endif
  }

  if(parser.seenval('C'))
  {
    // millis_t print_time_update = parser.value_celsius();
    #if ENABLED(RTS_AVAILABLE)
      rtscheck.RTS_SndData(print_time_update / 3600, PRINT_TIME_HOUR_VP);
      rtscheck.RTS_SndData((print_time_update % 3600) / 60, PRINT_TIME_MIN_VP);
    #endif
  }

  if(parser.seenval('D'))
  {
    // millis_t next_remain_time_update = parser.value_celsius();
    #if ENABLED(RTS_AVAILABLE)
      rtscheck.RTS_SndData(next_remain_time_update / 3600, PRINT_REMAIN_TIME_HOUR_VP);
      rtscheck.RTS_SndData((next_remain_time_update % 3600) / 60, PRINT_REMAIN_TIME_MIN_VP);
    #endif
  }
}