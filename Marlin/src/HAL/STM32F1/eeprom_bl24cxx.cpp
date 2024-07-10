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
#ifdef __STM32F1__

/**
 * PersistentStore for Arduino-style EEPROM interface
 * with simple implementations supplied by Marlin.
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(IIC_BL24CXX_EEPROM)

#include "../shared/eeprom_if.h"
#include "../shared/eeprom_api.h"

#include "../../libs/BL24CXX.h"


uint8_t eeprom_bl24cxx_ram[MARLIN_EEPROM_SIZE];
static bool written;
static uint16_t change_start_addr, change_end_addr;

//
// PersistentStore
//

#ifndef MARLIN_EEPROM_SIZE
  #error "MARLIN_EEPROM_SIZE is required for IIC_BL24CXX_EEPROM."
#endif

size_t PersistentStore::capacity()    { return MARLIN_EEPROM_SIZE; }

bool PersistentStore::access_start()  {
  eeprom_init();
  written = false;

  // 同步数据
  static bool first_read = true;
  if(first_read) {
    first_read = false;
    BL24CXX::quickReadBytes(0, eeprom_bl24cxx_ram, MARLIN_EEPROM_SIZE);
    // for(int i = 0; i < MARLIN_EEPROM_SIZE; i++) {
    //   char buf[8];
    //   sprintf(buf, "%02x ", eeprom_bl24cxx_ram[i]);
    //   SERIAL_ECHOPAIR(buf);
    //   watchdog_refresh();
    // }
  }
  return true; 
}

bool PersistentStore::access_finish() { 
  if(written) {
    written = false;

    uint16_t len = change_end_addr - change_start_addr;
    BL24CXX::quickWriteBytes(change_start_addr, &eeprom_bl24cxx_ram[change_start_addr], len);
  }
  return true; 
}

bool PersistentStore::write_data(int &pos, const uint8_t *value, size_t size, uint16_t *crc) {
  while (size--) {
    uint8_t v = *value;
    uint8_t * const p = (uint8_t * const)pos;
    if(eeprom_read_byte(p) != v) {
      const unsigned eeprom_address = (unsigned)pos;
      // 记录已变数据的开始地址和结束地址
      if(!written) {
        change_start_addr = eeprom_address;
        change_end_addr = eeprom_address + 1;
      }
      else if(eeprom_address < change_start_addr) {
        change_start_addr = eeprom_address;
      }
      else if(change_end_addr < (eeprom_address + 1)) {
        change_end_addr = eeprom_address + 1;
      }

      written = true;
      eeprom_write_byte(p, v);
    }
    crc16(crc, &v, 1);
    pos++;
    value++;
  }
  return false;
}

bool PersistentStore::read_data(int &pos, uint8_t *value, size_t size, uint16_t *crc, const bool writing/*=true*/) {
  do {
    uint8_t * const p = (uint8_t * const)pos;
    uint8_t c = eeprom_read_byte(p);
    if (writing) *value = c;
    crc16(crc, &c, 1);
    pos++;
    value++;
  } while (--size);
  return false;
}

#endif // IIC_BL24CXX_EEPROM
#endif // __STM32F1__
