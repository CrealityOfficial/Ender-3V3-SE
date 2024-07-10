/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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

#include "../inc/MarlinConfig.h"

#if ENABLED(IIC_BL24CXX_EEPROM)

/**
 * PersistentStore for Arduino-style EEPROM interface
 * with simple implementations supplied by Marlin.
 */

#include "BL24CXX.h"
#ifdef __STM32F1__
  #include <libmaple/gpio.h>
#else
  #include "../HAL/shared/Delay.h"
  #define delay_us(n) DELAY_US(n)
#endif

#ifndef EEPROM_WRITE_DELAY
  #define EEPROM_WRITE_DELAY    10
#endif
#ifndef EEPROM_DEVICE_ADDRESS
  #define EEPROM_DEVICE_ADDRESS (0x50 << 1)
#endif

// IO direction setting
#ifdef __STM32F1__
  #define SDA_IN()  do{ PIN_MAP[IIC_EEPROM_SDA].gpio_device->regs->CRL &= 0X0FFFFFFF; PIN_MAP[IIC_EEPROM_SDA].gpio_device->regs->CRL |= 8 << 28; }while(0)
  #define SDA_OUT() do{ PIN_MAP[IIC_EEPROM_SDA].gpio_device->regs->CRL &= 0X0FFFFFFF; PIN_MAP[IIC_EEPROM_SDA].gpio_device->regs->CRL |= 3 << 28; }while(0)
#else
  #define SDA_IN()  SET_INPUT(IIC_EEPROM_SDA)
  #define SDA_OUT() SET_OUTPUT(IIC_EEPROM_SDA)
#endif

// IO ops
#define IIC_SCL_0()   WRITE(IIC_EEPROM_SCL, LOW)
#define IIC_SCL_1()   WRITE(IIC_EEPROM_SCL, HIGH)
#define IIC_SDA_0()   WRITE(IIC_EEPROM_SDA, LOW)
#define IIC_SDA_1()   WRITE(IIC_EEPROM_SDA, HIGH)
#define READ_SDA()    READ(IIC_EEPROM_SDA)

//
// Simple IIC interface via libmaple
//

// Initialize IIC
void IIC::init() {
  SET_OUTPUT(IIC_EEPROM_SDA);
  SET_OUTPUT(IIC_EEPROM_SCL);
  IIC_SCL_1();
  IIC_SDA_1();
}

// Generate IIC start signal
void IIC::start() {
  SDA_OUT();    // SDA line output
  IIC_SDA_1();
  IIC_SCL_1();
  delay_us(4);
  IIC_SDA_0();  // START:when CLK is high, DATA change from high to low
  delay_us(4);
  IIC_SCL_0();  // Clamp the I2C bus, ready to send or receive data
}

// Generate IIC stop signal
void IIC::stop() {
  SDA_OUT();    // SDA line output
  IIC_SCL_0();
  IIC_SDA_0();  // STOP:when CLK is high DATA change from low to high
  delay_us(4);
  IIC_SCL_1();
  IIC_SDA_1();  // Send I2C bus end signal
  delay_us(4);
}

// Wait for the response signal to arrive
// 1 = failed to receive response
// 0 = response received
uint8_t IIC::wait_ack() {
  uint8_t ucErrTime = 0;
  SDA_IN();      // SDA is set as input
  IIC_SDA_1(); delay_us(1);
  IIC_SCL_1(); delay_us(1);
  while (READ_SDA()) {
    if (++ucErrTime > 250) {
      stop();
      return 1;
    }
  }
  IIC_SCL_0(); // Clock output 0
  return 0;
}

// Generate ACK response
void IIC::ack() {
  IIC_SCL_0();
  SDA_OUT();
  IIC_SDA_0();
  delay_us(2);
  IIC_SCL_1();
  delay_us(2);
  IIC_SCL_0();
}

// No ACK response
void IIC::nAck() {
  IIC_SCL_0();
  SDA_OUT();
  IIC_SDA_1();
  delay_us(2);
  IIC_SCL_1();
  delay_us(2);
  IIC_SCL_0();
}

// Send one IIC byte
// Return whether the slave responds
// 1 = there is a response
// 0 = no response
void IIC::send_byte(uint8_t txd) {
  SDA_OUT();
  IIC_SCL_0(); // Pull down the clock to start data transmission
  LOOP_L_N(t, 8) {
    // IIC_SDA = (txd & 0x80) >> 7;
    if (txd & 0x80) IIC_SDA_1(); else IIC_SDA_0();
    txd <<= 1;
    delay_us(2);   // All three delays are necessary for TEA5767
    IIC_SCL_1();
    delay_us(2);
    IIC_SCL_0();
    delay_us(2);
  }
}

// Read 1 byte, when ack=1, send ACK, ack=0, send nACK
uint8_t IIC::read_byte(unsigned char ack_chr) {
  unsigned char receive = 0;
  SDA_IN(); // SDA is set as input
  LOOP_L_N(i, 8) {
    IIC_SCL_0();
    delay_us(2);
    IIC_SCL_1();
    receive <<= 1;
    if (READ_SDA()) receive++;
    delay_us(1);
  }
  ack_chr ? ack() : nAck(); // Send ACK / send nACK
  return receive;
}

/******************** EEPROM ********************/

// Initialize the IIC interface
void BL24CXX::init() { IIC::init(); }

// Read a byte at the specified address
// ReadAddr: the address to start reading
// Return: the byte read
uint8_t BL24CXX::readOneByte(uint16_t ReadAddr) {
  uint8_t temp = 0;
  IIC::start();
  if (EE_TYPE > BL24C16) {
    IIC::send_byte(EEPROM_DEVICE_ADDRESS);        // Send write command
    IIC::wait_ack();
    IIC::send_byte(ReadAddr >> 8);                // Send high address
    IIC::wait_ack();
  }
  else
    IIC::send_byte(EEPROM_DEVICE_ADDRESS + ((ReadAddr >> 8) << 1)); // Send device address 0xA0, write data

  IIC::wait_ack();
  IIC::send_byte(ReadAddr & 0xFF);                // Send low address
  IIC::wait_ack();
  IIC::start();
  IIC::send_byte(EEPROM_DEVICE_ADDRESS | 0x01);   // Send byte
  IIC::wait_ack();
  temp = IIC::read_byte(0);
  IIC::stop();                                    // Generate a stop condition
  return temp;
}

// Write a data at the address specified by BL24CXX
// WriteAddr: The destination address for writing data
// DataToWrite: the data to be written
void BL24CXX::writeOneByte(uint16_t WriteAddr, uint8_t DataToWrite) 
{
  IIC::start();
  if (EE_TYPE > BL24C16) {
    IIC::send_byte(EEPROM_DEVICE_ADDRESS);        // Send write command
    IIC::wait_ack();
    IIC::send_byte(WriteAddr >> 8);               // Send high address
  }
  else
  {
    IIC::send_byte(EEPROM_DEVICE_ADDRESS + ((WriteAddr >> 8) << 1)); // Send device address 0xA0, write data
  }

  IIC::wait_ack();
  IIC::send_byte(WriteAddr & 0xFF);               // Send low address
  IIC::wait_ack();
  IIC::send_byte(DataToWrite);                    // Receiving mode
  IIC::wait_ack();
  IIC::stop();                                    // Generate a stop condition
  delay(5);
}

// Start writing data of length Len at the specified address in BL24CXX
// This function is used to write 16bit or 32bit data.
// WriteAddr: the address to start writing
// DataToWrite: the first address of the data array
// Len: The length of the data to be written 2, 4
void BL24CXX::writeLenByte(uint16_t WriteAddr, uint32_t DataToWrite, uint8_t Len) {
  LOOP_L_N(t, Len)
    writeOneByte(WriteAddr + t, (DataToWrite >> (8 * t)) & 0xFF);
}

// Start reading data of length Len from the specified address in BL24CXX
// This function is used to read 16bit or 32bit data.
// ReadAddr: the address to start reading
// Return value: data
// Len: The length of the data to be read 2,4
uint32_t BL24CXX::readLenByte(uint16_t ReadAddr, uint8_t Len) {
  uint32_t temp = 0;
  LOOP_L_N(t, Len) {
    temp <<= 8;
    temp += readOneByte(ReadAddr + Len - t - 1);
  }
  return temp;
}

// Check if BL24CXX is normal
// Return 1: Detection failed
// return 0: detection is successful
#define BL24CXX_TEST_ADDRESS 0x00
#define BL24CXX_TEST_VALUE   0x55

bool BL24CXX::_check() {
  return (readOneByte(BL24CXX_TEST_ADDRESS) != BL24CXX_TEST_VALUE); // false = success!
}

bool BL24CXX::check()
{
  if (_check())
  {
    // Value was written? Good EEPROM!
    writeOneByte(BL24CXX_TEST_ADDRESS, BL24CXX_TEST_VALUE); // Write now and check.
    return _check();
  }
  return false; // success!
}

// Start reading the specified number of data at the specified address in BL24CXX
// ReadAddr: The address to start reading is 0~255 for 24c02
// pBuffer: the first address of the data array
// NumToRead: the number of data to be read
void BL24CXX::read(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead) {
  for (; NumToRead; NumToRead--)
    *pBuffer++ = readOneByte(ReadAddr++);
}

// Start writing the specified number of data at the specified address in BL24CXX
// WriteAddr: the address to start writing, 0~255 for 24c02
// pBuffer: the first address of the data array
// NumToWrite: the number of data to be written
void BL24CXX::write(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite) 
{
  for (; NumToWrite; NumToWrite--, WriteAddr++)
  {
    writeOneByte(WriteAddr, *pBuffer++);
  }
}

void BL24CXX::EEPROM_Reset(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)
{
  write(WriteAddr, 0, NumToWrite);
  delay(200);
}

//
// 参考安富莱的EEPROM驱动代码 - 该实现代码的好处是不需要通过延时去保证写时序，而是通过等待应答去保证写时序
//
#define I2C_WR 0 /* 写控制bit */
#define I2C_RD 1 /* 读控制bit */
// 页面大小(字节) 
#define EE_PAGE_SIZE 16  // 为了保证兼容，对于页大小为32和64字节的也按16字节写
uint8_t BL24CXX::quickReadBytes(uint16_t Address, uint8_t *ReadBuf, uint16_t Size)
{
  uint16_t i;
  uint8_t addr_bytes = (EE_TYPE > BL24C16) ? 2 : 1;
  /* 采用串行EEPROM随即读取指令序列，连续读取若干字节 */
  /* 第1步：发起I2C总线启动信号 */
  IIC::start();
  /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
  if (addr_bytes == 1){
    IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_WR | (((Address >> 8) & 0x07) << 1)); /* 此处是写指令 */
  }
  else {
    IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_WR);
  }
  /* 第3步：发送ACK */
  if (IIC::wait_ack() != 0){
    goto cmd_fail; /* EEPROM器件无应答 */
  }
  /* 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址 */
  if (addr_bytes == 1){
    IIC::send_byte((uint8_t)Address);
    if (IIC::wait_ack() != 0){
        goto cmd_fail; /* EEPROM器件无应答 */
    }
  }
  else{
    IIC::send_byte(Address >> 8);
    if (IIC::wait_ack() != 0){
        goto cmd_fail; /* EEPROM器件无应答 */
    }
    IIC::send_byte(Address);
    if (IIC::wait_ack() != 0){
        goto cmd_fail; /* EEPROM器件无应答 */
    }
  }
  /* 第6步：重新启动I2C总线。下面开始读取数据 */
  IIC::start();
  /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
  IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_RD); /* 此处是读指令 */
  /* 第8步：发送ACK */
  if (IIC::wait_ack() != 0){
    goto cmd_fail; /* EEPROM器件无应答 */
  }
  /* 第9步：循环读取数据 */
  for (i = 0; i < Size; i++){
     ReadBuf[i] = IIC::read_byte((i != Size - 1) ? 1 : 0); /* 读1个字节 */
    // /* 每读完1个字节后，需要发送Ack， 最后一个字节不需要Ack，发Nack */
    // if (i != Size - 1){
    //     IIC::ack(); /* 中间字节读完后，CPU产生ACK信号(驱动SDA = 0) */
    // }
    // else{
    //     IIC::nAck(); /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    // }
  }
  /* 发送I2C总线停止信号 */
  IIC::stop();
  return 1; /* 执行成功 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
  /* 发送I2C总线停止信号 */
  IIC::stop();
  return 0;
}

uint8_t BL24CXX::quickWriteBytes(uint16_t Address, uint8_t *WriteBuf, uint16_t Size)
{
  uint16_t i, m;
  uint16_t usAddr;
  uint8_t addr_bytes = (EE_TYPE > BL24C16) ? 2 : 1;
  /*
    写串行EEPROM不像读操作可以连续读取很多字节，每次写操作只能在同一个page。
    对于24xx02，page size = 8
    简单的处理方法为：按字节写操作模式，每写1个字节，都发送地址
    为了提高连续写的效率: 本函数采用page wirte操作。
  */
  usAddr = Address;
  for (i = 0; i < Size; i++){
    /* 当发送第1个字节或是页面首地址时，需要重新发起启动信号和地址 */
    if ((i == 0) || (usAddr & (EE_PAGE_SIZE - 1)) == 0){
      /*　发停止信号，启动内部写操作　*/
      IIC::stop();
      /* 通过检查器件应答的方式，判断内部写操作是否完成, 一般小于 10ms
          CLK频率为200KHz时，查询次数为30次左右
      */
      for (m = 0; m < 1000; m++){
        /* 第1步：发起I2C总线启动信号 */
        IIC::start();
        /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
        if(addr_bytes == 1){
          IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_WR | (((usAddr >> 8) & 0x07) << 1)); /* 此处是写指令 */
        }
        else {
          IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_WR);
        }
        /* 第3步：发送一个时钟，判断器件是否正确应答 */
        if (IIC::wait_ack() == 0){
            break;
        }
      }
      if (m == 1000){
        goto cmd_fail; /* EEPROM器件写超时 */
      }
      /* 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址 */
      if (addr_bytes == 1){
        IIC::send_byte((uint8_t)usAddr);
        if (IIC::wait_ack() != 0){
            goto cmd_fail; /* EEPROM器件无应答 */
        }
      }
      else{
        IIC::send_byte(usAddr >> 8);
        if (IIC::wait_ack() != 0){
            goto cmd_fail; /* EEPROM器件无应答 */
        }
        IIC::send_byte(usAddr);
        if (IIC::wait_ack() != 0){
            goto cmd_fail; /* EEPROM器件无应答 */
        }
      }
    }
    /* 第6步：开始写入数据 */
    IIC::send_byte(WriteBuf[i]);
    /* 第7步：发送ACK */
    if (IIC::wait_ack() != 0){
      goto cmd_fail; /* EEPROM器件无应答 */
    }
    usAddr++; /* 地址增1 */
  }
  /* 命令执行成功，发送I2C总线停止信号 */
  IIC::stop();
  /* 通过检查器件应答的方式，判断内部写操作是否完成, 一般小于 10ms
      CLK频率为200KHz时，查询次数为30次左右
  */
  for (m = 0; m < 1000; m++){
    /* 第1步：发起I2C总线启动信号 */
    IIC::start();
    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    if(addr_bytes == 1) {
      IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_WR | (((Address >> 8) & 0x07) << 1)); /* 此处是写指令 */
    }
    else{
      IIC::send_byte(EEPROM_DEVICE_ADDRESS | I2C_WR); /* 此处是写指令 */
    }
    /* 第3步：发送一个时钟，判断器件是否正确应答 */
    if (IIC::wait_ack() == 0){
        break;
    }
  }
  if (m == 1000){
    goto cmd_fail; /* EEPROM器件写超时 */
  }
  /* 命令执行成功，发送I2C总线停止信号 */
  IIC::stop();
  return 1;
cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
  /* 发送I2C总线停止信号 */
  IIC::stop();
  return 0;
}

#endif // IIC_BL24CXX_EEPROM
