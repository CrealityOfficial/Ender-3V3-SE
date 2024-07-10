#include "lcd_rts.h"
#include "../../../inc/MarlinConfigPre.h"
#include "../../../MarlinCore.h"
#include "../../../core/serial.h"
#include "../../../sd/cardreader.h"
#include "../../../module/base64/base64.h"
#include "../ui_dacai.h"

#include "../../../MarlinCore.h"
#include "../../../core/serial.h"
#include "../../../core/macros.h"
#include "../../../gcode/queue.h"

#include "../../../module/temperature.h"
#include "../../../module/printcounter.h"
#include "../../../module/motion.h"
#include "../../../module/planner.h"
#define USER_LOGIC_DEUBG
#include "../dwin_lcd.h"
/**
 * @功能   从U盘中读取预览图数据并解码
 * @Author Creality
 * @Time   2022-04-13
 * buf          : 用于保存解码后的数据
 * picLen       : 需要的数据长度
 * resetFlag    : 重置数据标志 -- 由于Base64解码后是3的倍数（4个Base64字符解码后是4个字节数据），但是入参‘picLen’不一定是3的倍数。
 *                所以单次调用后，剩余的没有使用到的字节数据保存在“base64_out”，其长度为“deCodeBase64Cnt”
 *                当显示完第一张图片后，显示第二张图时，需要清除一下这两个数据，防止影响第二张图片的显示
 *                true -- 清除历史数据 （“base64_out”，“deCodeBase64Cnt”）
 *                false -- 不动作
 */
bool gcodePicGetDataFormBase64(char * buf, unsigned long picLen, bool resetFlag)
{
  char base64_in[4];                          // 保存base64编码的数组
  static unsigned char base64_out[3] = {'0'}; // 保存base64解码的数组
  char getBase64Cnt = 0;                      // 从U盘获取的，base64编码的数据
  static signed char deCodeBase64Cnt = 0;     // 已经解码得了数据
  unsigned long deCodePicLenCnt = 0;          // 保存已经获取的图片数据
  static char lCmdBuf[100];
  bool getPicEndFlag = false;

  // if (ENABLED(USER_LOGIC_DEUBG))
  // {
       // SERIAL_ECHOLNPAIR("\r\n gcodePicGetDataFormBase64(...), .deCodeBase64Cnt = ", deCodeBase64Cnt,
                         // "\r\n gcodePicGetDataFormBase64(...), .deCodePicLenCnt = ", deCodePicLenCnt,
                         // "\r\n gcodePicGetDataFormBase64(...), .picLen = ", picLen);
  // }

  // 清除上次记录
  if (resetFlag)
  {
    for (int i = 0; i < sizeof(base64_out); i++)
    {
      base64_out[i] = '0x00';
    }
    deCodeBase64Cnt = 0;
    return true;
  }

  if ((deCodeBase64Cnt > 0) && (deCodePicLenCnt < picLen))
  {
    if (ENABLED(USER_LOGIC_DEUBG)) 
    {
      // SERIAL_ECHO("\r\n There are parameters left last time ");
      memset(lCmdBuf, 0, sizeof(lCmdBuf));
      // sprintf(lCmdBuf, "\r\n ------------------------------deCodeBase64Cnt = %d; base64_out[3 - deCodeBase64Cnt] = %x",deCodeBase64Cnt,base64_out[3 - deCodeBase64Cnt]);
      // SERIAL_ECHO(lCmdBuf);
    }

    for (int deCode = deCodeBase64Cnt; deCode > 0; deCode--)
    {
      if (deCodePicLenCnt < picLen)
      {
        buf[deCodePicLenCnt++] = base64_out[3 - deCode];
      }
      else
      {
        break;
      }
    }
  }

  // if (ENABLED(USER_LOGIC_DEUBG))
  // {
       // SERIAL_ECHOLNPAIR("\r\n gcodePicGetDataFormBase64(...), ..deCodeBase64Cnt = ", deCodeBase64Cnt,
                         // "\r\n gcodePicGetDataFormBase64(...), ..deCodePicLenCnt = ", deCodePicLenCnt);
  // }

  while(deCodePicLenCnt < picLen)
  {
    char j, ret;
    for ( j = 0; j < 20; j++)
    {
      // 从U盘中获取一个字符
      ret = card.get();

      if (ret == ';' || ret == ' ' || ret == '\r' || ret == '\n')
      {
        continue;
      }

      base64_in[getBase64Cnt++] = ret;
      if (getBase64Cnt >= 4)
      {
        getBase64Cnt = 0;
        break;
      }
    }

    memset(base64_out, 0, sizeof(base64_out));
    deCodeBase64Cnt = base64_decode(base64_in, 4, base64_out);
    for(int i = deCodeBase64Cnt; i < 3; i++)
    {
      base64_out[i] = 0;
    }
    // 这里强制给3，因为始终是4 --> 3 字符
    deCodeBase64Cnt = 3;

    // if (ENABLED(USER_LOGIC_DEUBG))
    // {
    //   memset(lCmdBuf, 0, sizeof(lCmdBuf));
    //   sprintf(lCmdBuf, "\r\n deCodePicLenCnt = %d ;in = %s; ", deCodePicLenCnt, base64_in);
    //   SERIAL_ECHO(lCmdBuf);
    // }
    int test = deCodeBase64Cnt;
    for (int deCode = 0; deCode < test; deCode++)
    {
      if (deCodePicLenCnt < picLen)
      {
        // 特殊处理一下末尾字符，找到了FF D9后退出
        if (getPicEndFlag)
        {
          buf[deCodePicLenCnt++] = 0;
        }
        else
        {
          buf[deCodePicLenCnt++] = base64_out[deCode];
        }

        if (deCodePicLenCnt > 2 && \
           ((buf[deCodePicLenCnt-1] == 0xD9 && buf[deCodePicLenCnt-2] == 0xFF) || (buf[deCodePicLenCnt-1] == 0xd9 && buf[deCodePicLenCnt-2] == 0xff)))
        {
          getPicEndFlag = true;
          if (ENABLED(USER_LOGIC_DEUBG))
          {
            SERIAL_ECHOLNPAIR("\r\n ---------------- deCodePicLenCnt = ", deCodePicLenCnt);
          }
        }
        deCodeBase64Cnt--;
      }
      else
      {
        break;
      }
    }

    watchdog_refresh();

    // if (ENABLED(USER_LOGIC_DEUBG))
    // {
    //   memset(lCmdBuf, 0, sizeof(lCmdBuf));
    //   sprintf(lCmdBuf, "j = %d ;in = %s; out = %x %x %x; buf = %x %x %x", j, base64_in, base64_out[0], base64_out[1], base64_out[2], buf[deCodePicLenCnt-3], buf[deCodePicLenCnt-2], buf[deCodePicLenCnt-1]);
    //   SERIAL_ECHO(lCmdBuf);
    //   SERIAL_ECHO("\r\n-- ");
    // }
  }

  if (ENABLED(USER_LOGIC_DEUBG))
  {
    SERIAL_ECHOLNPAIR("\r\n gcodePicGetDataFormBase64(...), ....deCodePicLenCnt = ", deCodePicLenCnt);
  }
  return true;
}

/**
 * @功能   gcode预览图显示、隐藏
 * @Author Creality
 * @Time   2021-0-27
 * jpgAddr      地址
 * onoff        显示(onoff == true)，隐藏(onoff == false)
 * 显示地址
 */
void gcodePicDispalyOnOff(unsigned int jpgAddr, bool onoff)
{
  if (onoff)
  {
    // rock_20221013
    #if ENABLED(DWIN_CREALITY_480_LCD)
      DWIN_ICON_SHOW_SRAM(36, 35, 0);
    #elif ENABLED(DWIN_CREALITY_320_LCD)
      DWIN_ICON_SHOW_SRAM(72, 24, 0);
    #endif
  }
  else
  {

  }
}

model_information_t model_information;
static const char * gcode_information_name[] =
{
  "TIME","Filament used","Layer height","MINX",
  "MINY","MINZ","MAXX","MAXY","MAXZ"
};
//PIC_OK;
#define READ_PIC_TIME            500  //读取图片预览数据超时时间
uint8_t read_gcode_model_information(void)
{ 
  millis_t ms = millis();
  millis_t next_read_pic_ms = millis()+READ_PIC_TIME;
  int32_t  temp_value=0;
  char buf[50];
  char buf_true[50] = {0};
  unsigned char i,j,k;
  char *char_pos;
  char ret;
  unsigned char buf_state = 0;
  uint8_t loop_max=0;
  // SERIAL_ECHOLNPAIR("  millis()=: ", millis()); // rock_20210909
  // SERIAL_ECHOLNPAIR(" next_read_pic_ms=: ", next_read_pic_ms); // rock_20210909
  while(ret != ';')
  {
    temp_value= (millis()-next_read_pic_ms);
    ret = card.get();
    // SERIAL_ECHOLNPAIR(" temp_value=: ",  temp_value); // rock_20210909 
    if(temp_value>0)
    {
      return PIC_MISS_ERR;
    }
  }
  
  while((loop_max++)<5)
  {
    i = 0;
    memset(buf_true, 0, sizeof(buf_true));
    // ret=0;
    // 搜索到下一个;字符,就结束
    next_read_pic_ms = millis()+READ_PIC_TIME;
    while(ret != ';')
    {
      //temp_value= (millis()-next_read_pic_ms);
      ret = card.get();
      buf[i] = ret;
      i++;
      // 如果找了很多都没找到';',说明文件错误,直接退出
      if(i > 50)
      {
        goto gcode_information_err;
      }
      // 结束,buf[]包含结束';'
    }
    // SERIAL_ECHO_MSG("buf:",buf);
    // for(k = 0;k < 9;k++)
    for(k = 0;k < 3;k++)
    {
      // 查找关键字
      char_pos = strstr(buf, gcode_information_name[k]);
      // 已经找到相对应的数据
      if(char_pos != NULL)
      {
        // 舍去文字,直接提取数值
        char_pos+=strlen(gcode_information_name[k]);
        while (1)
        {
          // 跳过冒号
          // char_pos++;
          if(*char_pos++ == ':')break;
        }
        for(j = 0;j < 15 ;j++)
        {
          buf_true[j] = *char_pos;
          char_pos++;
          if(*char_pos == '\r')
          {
            break;
          }
        }
        // SERIAL_ECHOLNPAIR(" strlen(buf_true)=: ", strlen(buf_true)); // rock_20210909
        if(' '==buf_true[0])strncpy(buf_true, buf_true+1,strlen(buf_true));//去除字符串前面的空格
        i = 0;
        memset(buf, 0, sizeof(buf));
        // SERIAL_ECHOLN(buf_true);
        switch(k)
        {
          case 0:
            memset(model_information.pre_time, 0, sizeof(model_information.pre_time));
            strcpy(model_information.pre_time, buf_true);
            break;
          case 1:
            memset(model_information.filament, 0, sizeof(model_information.filament));           
            if(strlen(buf_true)>6)
            {
              strncpy(model_information.filament, buf_true,5);
              if('m'==buf_true[strlen(buf_true)-1])
              strncat(model_information.filament, &buf_true[strlen(buf_true)-1],1);
              else if('m'==buf_true[strlen(buf_true)-2])strncat(model_information.filament, &buf_true[strlen(buf_true)-2],1);
            }
            else strcpy(model_information.filament, buf_true);
            break;
          case 2:
            memset(model_information.height, 0, sizeof(model_information.height));
            strcpy(model_information.height, buf_true);
            strcat(model_information.height, "mm");
            buf_state = 1;
            break;
        }
        // break;
      }
    }
    ret = 0;  //清空ret
    if(buf_state)
    {
      /*
      SERIAL_ECHOLN("model_information end");
      SERIAL_ECHOLN(model_information.pre_time);
      SERIAL_ECHOLN(model_information.height);
      SERIAL_ECHOLN(model_information.MAXX);
      SERIAL_ECHOLN(model_information.MAXY);
      SERIAL_ECHOLN(model_information.MAXZ);
      SERIAL_ECHOLN(model_information.MINX);
      SERIAL_ECHOLN(model_information.MINY);
      SERIAL_ECHOLN(model_information.MINZ);
      SERIAL_ECHOLN(model_information.filament);
      */
      // Exit the loop
      return PIC_OK;
    }
    // else 
    // {
    //    SERIAL_ECHOLNPAIR(" buf_state4444=: ", buf_state); // rock_20210909
    //   return PIC_MISS_ERR;
    // }
  }
  return PIC_MISS_ERR;
  gcode_information_err:
    memset(model_information.pre_time, 0, sizeof(model_information.pre_time));
    memset(model_information.filament, 0, sizeof(model_information.filament));
    memset(model_information.height, 0, sizeof(model_information.height));
    memset(model_information.MINX, 0, sizeof(model_information.MINX));
    memset(model_information.MINY, 0, sizeof(model_information.MINY));
    memset(model_information.MINZ, 0, sizeof(model_information.MINZ));
    memset(model_information.MAXX, 0, sizeof(model_information.MAXX));
    memset(model_information.MAXY, 0, sizeof(model_information.MAXY));
    memset(model_information.MAXZ, 0, sizeof(model_information.MAXZ));
    // SERIAL_ECHOLN("model_information error");
    // SERIAL_ECHOLNPAIR(" buf_state5554=: ", buf_state); // rock_20210909
    return PIC_MISS_ERR;
}

/**
 * @功能   从gcode里面读取jpeg图片显示：1、发送到屏显示；2、让指针跳过这段图片，再去寻找下一张图片
 * @Author Creality
 * @Time   2021-12-01
 * picLenth     : 图片长度(base64编码的长度)
 * isDisplay    : 是否显示该图片
 * jpgAddr      : 显示图片的地址
 */
extern unsigned long arr_data_num;
bool gcodePicDataRead(unsigned long picLenth, char isDisplay, unsigned long jpgAddr)
{
  //          96*96-耗时-Ms  200*200-耗时-Ms
  //  * 2  :      1780        8900 
  //  * 4  :      940         4490 
  //  * 8  :      518         2010 
  //  * 12 :      435         1300 
  //  * 16 :      420         1130 
  #define PIN_BUG_LEN_DACAI   2048 
  #define PIN_BUG_LEN_DWIN    PIN_BUG_LEN_DACAI + 20  // (JPG_BYTES_PER_FRAME * 12)
  #define PIN_DATA_LEN_DWIN   PIN_BUG_LEN_DACAI + 20  // (PIN_BUG_LEN_DWIN / 2)

  static char picBuf[PIN_BUG_LEN_DWIN+1];   // 这个取 MXA(PIN_BUG_LEN_DACAI, PIN_BUG_LEN_DWIN)
  unsigned long picLen;                     // 图片长度(解码后的长度)
  unsigned long j;

  // (picLenth / 4) * 3;
  picLen = picLenth;

  gcodePicGetDataFormBase64(picBuf, 0, true);

  arr_data_num = 0;
  // 开始读取
  // 一次传送2048个数据
  for (j = 0; j < (picLen / PIN_BUG_LEN_DACAI); j++)
  {
    memset(picBuf, 0, sizeof(picBuf));
    // card.read(picBuf, PIN_BUG_LEN_DACAI);
    gcodePicGetDataFormBase64(picBuf, PIN_BUG_LEN_DACAI, false);

    // 发送图片数据到指定地址
    if (isDisplay)
    {
      uiShow.UI_SendJpegDate(picBuf, PIN_BUG_LEN_DACAI);
    }
  }
  // 剩下的不足2048字符的数据处理，根据迪文处理内容
  watchdog_refresh();
  if (picLen % PIN_BUG_LEN_DACAI != 0)
  {
    memset(picBuf, 0, sizeof(picBuf));
    gcodePicGetDataFormBase64(picBuf, (picLen - PIN_BUG_LEN_DACAI * j), false);
    // card.read(picBuf, (picLen - PIN_BUG_LEN_DACAI * j));
    // 发送图片数据到指定地址
    if (isDisplay)
    {
      uiShow.UI_SendJpegDate(picBuf, picLen - PIN_BUG_LEN_DACAI * j);
    }
  }
  if (isDisplay)
  {
    gcodePicDispalyOnOff(jpgAddr, true);
  }

  read_gcode_model_information();
  return true;
}

/**
 * @功能   gcode预览图存在判断
 * @Author Creality
 * @Time   2021-12-01
 * fileName         gcode文件名
 * jpgAddr          显示地址
 * jpgFormat        图片类型（jpg、png）
 * jpgResolution    图片大小
 */
char gcodePicExistjudge(char *fileName, unsigned int jpgAddr, unsigned char jpgFormat, unsigned char jpgResolution)
{
  #define JUDGE_PIC_BUF_CNT   20
  enum{
    MENU_PIC_FORMAT,        // 图片格式判断
    MENU_PIC_HEADER,        // 标识位判断
    MENU_PIC_RESOLITION,    // 图片分辨率判断
    MENU_PIC_LEN,           // 数据长度判断
    MENU_PIC_START_LINE,    // 起始行
    MENU_PIC_END_LINE,      // 结束行
    MENU_PIC_HIGH,          // 模型高度
    MENU_PIC_MAX
  };
  unsigned char picMenu = MENU_PIC_FORMAT;
  unsigned long picLen = 0;     // 图片数据长度
  unsigned int  i;
  // unsigned long dataLen = 0; // 包数据长度
  unsigned char picFormat = PIC_FORMAT_MAX;         // 图片格式
  unsigned char picResolution = PIC_RESOLITION_MAX; // 图片分辨率
  unsigned char ret;
  // unsigned char picGetDataCnt = 0;               // 读取数据计数
  // char picFileHeader[PIC_PRESENT_LEN] = {0xAA, 0x55, 0xA5, 0x5A};
  unsigned char buf[20] = {0};
  unsigned char bufIndex = 0;
  char lCmdBuf[20];

  // 查找图片，只从前20字节串查找，
  for (i = 0; i < JUDGE_PIC_BUF_CNT; i++)
  {
    memset(buf, 0, sizeof(buf));
    bufIndex = 0;
    int j;
    for (j = 0; j < 20; j++)
    {
      // 从U盘中获取一个字符
      ret = card.get();
      // 第一个字符是无效的，重新读取
      if((ret == ';' || ret == ' ' || ret == '\r' || ret == '\n') && bufIndex == 0)
      {
        continue;
      }
      else if ((ret == ';' || ret == ' ' || ret == '\r' || ret == '\n') && bufIndex != 0)
      {
        break;
      }
      buf[bufIndex++] = ret;
    }
    if (j == 20) break;
    memset(lCmdBuf, 0, sizeof(lCmdBuf));
    sprintf(lCmdBuf, "%s", buf);
    if (ENABLED(USER_LOGIC_DEUBG))
    {
      SERIAL_ECHO(lCmdBuf);
      SERIAL_ECHO("\r\n");
    }

    switch(picMenu)
    {
      // 图片的格式判断 jpg或png
      case MENU_PIC_FORMAT:
        picFormat = PIC_FORMAT_MAX;
        if (strcmp(lCmdBuf, FORMAT_JPG) == 0)
        {
          picFormat = PIC_FORMAT_JPG;
        }
        else if (strcmp(lCmdBuf, FORMAT_PNG) == 0)
        {
          picFormat = PIC_FORMAT_PNG;
        }
        if (ENABLED(USER_LOGIC_DEUBG))
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_FORMAT, lCmdBuf = ", lCmdBuf,
                            "\r\n picMenu = MENU_PIC_FORMAT, FORMAT_JPG = ", FORMAT_JPG,
                            "\r\n picMenu = MENU_PIC_FORMAT, picFormat = ", picFormat);
        }
        // 判断是否完成标识位判断，完成后进入下一步
        if (picFormat != PIC_FORMAT_MAX)
        {
          picMenu = MENU_PIC_HEADER;
        }
        break;

      case MENU_PIC_HEADER:
        // 开始标志判断，找到（begin）
        if (ENABLED(USER_LOGIC_DEUBG)) 
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_HEADER, lCmdBuf = ", lCmdBuf,
                            "\r\n picMenu = MENU_PIC_HEADER, PIC_HEADER = ", PIC_HEADER);
        }

        // 判断是否完成包数据长度判断，完成后进图下一步
        if (strcmp(lCmdBuf, PIC_HEADER) == 0)
        {
          picMenu = MENU_PIC_RESOLITION;
        }
        break;

      case MENU_PIC_RESOLITION:
        // 图片分辨率判断
        picResolution = PIC_RESOLITION_MAX;
        if (strcmp(lCmdBuf, RESOLITION_36_36) == 0)
        {
          picResolution = PIC_RESOLITION_36_36;
        }
        else if (strcmp(lCmdBuf, RESOLITION_48_48) == 0)
        {
          picResolution = PIC_RESOLITION_48_48;
        }
        else if (strcmp(lCmdBuf, RESOLITION_64_64) == 0)
        {
          picResolution = PIC_RESOLITION_64_64;
        }
        else if (strcmp(lCmdBuf, RESOLITION_96_96) == 0)
        {
          picResolution = PIC_RESOLITION_96_96;
        }
        else if (strcmp(lCmdBuf, RESOLITION_144_144) == 0)
        {
          picResolution = PIC_RESOLITION_144_144;
        }
        else if (strcmp(lCmdBuf, RESOLITION_200_200) == 0)
        {
          picResolution = PIC_RESOLITION_200_200;
        }
        else if (strcmp(lCmdBuf, RESOLITION_300_300) == 0)
        {
          picResolution = PIC_RESOLITION_300_300;
        }
        else if (strcmp(lCmdBuf, RESOLITION_600_600) == 0)
        {
          picResolution = PIC_RESOLITION_600_600;
        }

        if (picResolution != PIC_RESOLITION_MAX)
        {
          picMenu = MENU_PIC_LEN;
        }

        if (ENABLED(USER_LOGIC_DEUBG))
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_RESOLITION, lCmdBuf = ", lCmdBuf);
        }
        break;

      case MENU_PIC_LEN:
        // 图片长度
        picLen = atoi(lCmdBuf);
        picMenu = MENU_PIC_START_LINE;

        if (ENABLED(USER_LOGIC_DEUBG))
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_LEN, lCmdBuf = ", lCmdBuf,
                            "\r\n picMenu = MENU_PIC_LEN, picLen = ", picLen);
        }
        break;

      case MENU_PIC_START_LINE:
        // 起始行
        picMenu = MENU_PIC_END_LINE;

        if (ENABLED(USER_LOGIC_DEUBG))
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_START_LINE, lCmdBuf = ", lCmdBuf);
        }
        break;

      case MENU_PIC_END_LINE:
        // 结束行
        picMenu = MENU_PIC_HIGH;

        if (ENABLED(USER_LOGIC_DEUBG))
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_END_LINE, lCmdBuf = ", lCmdBuf);
        }
        break;

      case MENU_PIC_HIGH:
        // 高度
        picMenu = MENU_PIC_MAX;
        if (ENABLED(USER_LOGIC_DEUBG))
        {
          SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_HIGH, lCmdBuf = ", lCmdBuf);
        }
        break;

      default:
        break;
    }   // switch(picMenu)

    if (picMenu == MENU_PIC_MAX)
    {
      if (ENABLED(USER_LOGIC_DEUBG))
      {
        SERIAL_ECHOLNPAIR("\r\n picMenu = MENU_PIC_MAX, picMenu = ", picMenu);
      }
      break;
    }
  }

  if (ENABLED(USER_LOGIC_DEUBG))
  {
    // SERIAL_ECHOPAIR("\r\n gcode pic time test 1 msTest = ", (millis() - msTest));
    // msTest = millis();
  }
  // That means we found the photo,
  if (picMenu == MENU_PIC_MAX)
  {
    if (ENABLED(USER_LOGIC_DEUBG))
    {
      SERIAL_ECHOLNPAIR("\r\n ...picResolution = ", picResolution,
                        "\r\n ...picFormat = ", picFormat,
                        "\r\n ...picLen = ", picLen);
    }

    // uint32_t index = card.getIndex1();//card.getIndex();
    // card.setIndex((index - (JUDGE_PIC_BUF_LEN - i -1)));
    // if (ENABLED(USER_LOGIC_DEUBG)) 
    // {
    //     SERIAL_ECHOLNPAIR("\r\n ...old_index = ", index,
    //                     "\r\n ...new_index = ", (index - (JUDGE_PIC_BUF_LEN - i -1)));
    // }
    // 从gcode里面读出图片数据，根据选择的是不是预定格式或预定大小图片来判断是否需要发送到屏上

    // 判断是否是需要的分辨率 和 格式
    if ((picResolution == jpgResolution) && (picFormat == jpgFormat))
    {
      gcodePicDataRead(picLen, true, jpgAddr);
    }
    else
    {
      // 直接移动指针，跳过无效的图片
      // 协议规定完整一行数据：';' + ' ' + "数据" + '\n'  1+1+76+1 = 79字节
      // 最后一行为“; png end\r” 或 “; jpg end\r”,
      // uint32_t index1 = card.getIndex();
      uint32_t index1 = card.getIndex1();
      uint32_t picLen1 = 0;
      if (picLen % 3 == 0)
      {
        picLen1 = picLen / 3 * 4;
      }
      else
      {
        picLen1 = (picLen / 3 + 1) * 4;
      }
      uint32_t indexAdd = (picLen1 / 76) * 3 + picLen1 + 10;
      if ((picLen1 % 76) != 0) 
      {
        indexAdd += 3;
      }

      card.setIndex((index1 + indexAdd));
      if (ENABLED(USER_LOGIC_DEUBG)) 
      {
        SERIAL_ECHOLNPAIR("\r\n ...old_index1 = ", index1,
                          "\r\n ...indexAdd = ", indexAdd);
      }

      if (picResolution != jpgResolution)
      {
        return PIC_RESOLITION_ERR;
      }
      else
      {
        return PIC_FORMAT_ERR;
      }
    }
  }
  else
  {
    // 该gcode中没有图标
    // card.closefile();
    return PIC_MISS_ERR;
  }
  // card.closefile();
  // if (ENABLED(USER_LOGIC_DEUBG)) 
  //     SERIAL_ECHOPAIR("\r\n gcode pic time test 3 msTest = ", (millis() - msTest));
  // msTest = millis();
  return PIC_OK;
}

/**
 * @功能   gcode预览图发送到迪文
 * @Author Creality
 * @Time   2021-12-01
 * fileName     gcode文件名
 * jpgAddr      显示地址
 * jpgFormat    图片类型（jpg、png）
 * jpgResolution     图片大小
 */
/*
uint8_t gcodePicDataSendToDwin(char *fileName, unsigned int jpgAddr, unsigned char jpgFormat, unsigned char jpgResolution)
{
  char ret;
  char returyCnt = 0;
  SERIAL_ECHO("\r\n gcodePicDataSendToDwin fileName = ");
  SERIAL_ECHO(fileName);
  card.openFileRead(fileName);
  // msTest = millis();
  while (1)
  {
    
    ret = gcodePicExistjudge(fileName, jpgAddr, jpgFormat, jpgResolution);
    // 当gcode中没有pic时，直接返回
    if (ret == PIC_MISS_ERR)
    {
      card.closefile();
      return PIC_MISS_ERR;
    }
    else if ((ret == PIC_FORMAT_ERR) || (ret == PIC_RESOLITION_ERR))
    {
      // 当格式或大小错误，继续往下判断
      if (++returyCnt >= 3)
      {
        card.closefile();
        return PIC_MISS_ERR;
      }
      else
      {
        continue;
      }
    }
    else
    {
      card.closefile();
      return PIC_OK;
    }
  }
  
}
*/

/**
 * @功能   xxxxxxxxxxxxxxxxxxxxxxxx
 * @Author Creality
 * @Time   2021-12-01
 * time : 			gcode预计打印时间
 * Filament used：	材料用量
 * Layer height：	层高
 *   PIC_OK,              // 图片显示ok
  PIC_FORMAT_ERR,      // 图片格式错误
  PIC_RESOLITION_ERR,  // 图片分辨率错误
  PIC_MISS_ERR,        // gcode无图片
 */
// Parse_Only_Picture_Data(char* fileName, char * time, char * FilamentUsed, char * layerHeight)

uint8_t gcodePicDataSendToDwin(char *fileName, unsigned int jpgAddr, unsigned char jpgFormat, unsigned char jpgResolution)
{
  char ret;
  char returyCnt = 0; 

 
 
  HAL_watchdog_refresh();
  SERIAL_ECHO("\r\n gcodePicDataSendToDwin fileName = ");
  SERIAL_ECHO(fileName);
  card.openFileRead(fileName);
  // SERIAL_ECHOLNPAIR(" ret333=: ", ret); // rock_20210909
  // msTest = millis();
  // while (1)
  // {
    ret=read_gcode_model_information();
    // 当gcode中没有pic时，直接返回
    if (ret == PIC_MISS_ERR)
    {
      card.closefile();
      return PIC_MISS_ERR;
    }
    /*
    else if ((ret == PIC_FORMAT_ERR) || (ret == PIC_RESOLITION_ERR))
    {
      // 当格式或大小错误，继续往下判断
      if (++returyCnt >= 3)
      {
        card.closefile();
        return PIC_MISS_ERR;
      }
      // else
      // {
      //   continue;
      // }
    }
    */
    else
    {
      card.closefile();
      return PIC_OK;
    }
  // }
  
}
/*

char Parse_Only_Picture_Data(char* fileName, char * time, char * FilamentUsed, char * layerHeight)
{
   #define STRING_MAX_LEN      60
	  unsigned char ret;
    unsigned char strBuf[STRING_MAX_LEN] = {0};
    unsigned char bufIndex = 0;
	unsigned char buf[10] = {0};
	uint8_t retryTimes = 0;	//重复查询次数

  SERIAL_ECHO("\r\n gcodePicDataSendToDwin fileName = ");
  SERIAL_ECHO(fileName);
  card.openFileRead(fileName);

    // 读取一个字符串，以空格隔开
    #define GET_STRING_ON_GCODE()
        {
            // 读取一行，以换行符隔开
            memset(strBuf, 0, sizeof(strBuf));
            int strLenMax;
            bool strStartFg = false;
            uint8_t curBufLen = 0;
            retryTimes = 0;   // 查找次数
            do {
                for (strLenMax = 0; strLenMax < STRING_MAX_LEN; strLenMax++)
                {
                    ret = card.get();   // 从U盘中获取一个字符
                    SERIAL_ECHO_MSG("1:",&ret);
                    if (ret != ';' && strStartFg == false)  // 读到';'为一行的开始
                        continue;
                    else
                        strStartFg = true;
                    if ((ret == '\r' || ret == '\n') && bufIndex != 0) break;   // 读到换行符，退出
                    strBuf[bufIndex++] = ret;
                }
                SERIAL_ECHO_MSG("4f56ds:",strBuf);
                if (strLenMax >= STRING_MAX_LEN) {
                    SERIAL_ECHO_MSG("curren srting lenth more than STRING_MAX_LEN(60)");
                    card.closefile();
                    return false;	// 返回失败
                }
                curBufLen = sizeof(strBuf);
                if (retryTimes++ >= 5)
                {
                    SERIAL_ECHO_MSG("retryTimes more than5 times");
                    card.closefile();
                    return false;	// 返回失败
                }
            }while(curBufLen < 20);

			      // 调试打印
            SERIAL_ECHO_MSG("strBuf = ", strBuf);
            SERIAL_ECHO_MSG("curBufLen = ", curBufLen);
      }


	// 获取某一行的指定值
	// 例如，获取“;TIME:464.876” 中的 “464.876”


	// 查找TIME
	retryTimes = 0;
	do {
		GET_STRING_ON_GCODE();
    
		memset(buf, 0, sizeof(buf));
		if ( strstr((const char *)strBuf, "TIME" ) == NULL) {
			sscanf((const char *)strBuf,"%s:%s",&buf, time);

			// 调试打印信息
			SERIAL_ECHO_MSG("buf = ", buf);
            SERIAL_ECHO_MSG("time = ", time);
			break;
		}

		if (retryTimes++ >= 3) 
    {
      card.closefile();
      return false;	// 超过3次，返回失败
    }
	}while(1);


	// 查找 "Filament used"
	retryTimes = 0;
	do {
		GET_STRING_ON_GCODE();
		memset(buf, 0, sizeof(buf));
		if ( strstr((const char *)strBuf, "Filament used" ) == NULL) {
			sscanf((const char *)strBuf,"%s:%s",&buf, FilamentUsed);

			// 调试打印信息
			SERIAL_ECHO_MSG("buf = ", buf);
            SERIAL_ECHO_MSG("time = ", FilamentUsed);
			break;
		}

		if (retryTimes++ >= 3) 
    {
      card.closefile();
      return false;	// 超过3次，返回失败
    }
	}while(1);

	// 查找 "Layer height"
	retryTimes = 0;
	do {
		GET_STRING_ON_GCODE();
		memset(buf, 0, sizeof(buf));
		if ( strstr((const char *)strBuf, "Layer height" ) == NULL) {
			sscanf((const char *)strBuf,"%s:%s",&buf, layerHeight);

			// 调试打印信息
			SERIAL_ECHO_MSG("buf = ", buf);
            SERIAL_ECHO_MSG("time = ", layerHeight);
			break;
		}

		if (retryTimes++ >= 3) 
    {
      card.closefile();
      return false;	// 超过3次，返回失败
    }
	}while(1);
}
*/
////////////////////////////