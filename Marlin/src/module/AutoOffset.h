/*
* Assignment:  用于适配CR-TOUCH自动Z-OFFSET获取模块
* Author:      王玉龙107051
* Date:        2022-12-01
* Version:     V2.0
* Description: 使用RC低通滤波+特征法触发检测+旋转法结果计算相结合，用于自动获取Z-OFFSET。
*              内容已包括喷头擦式、CR-TOUCH测量、压力传感器测量。
* Attention:   使用者移植过程中，需保证所有的宏定义正确且运行正常（当前宏定义基于Marlin 2.0.8.3）。
*/


#ifndef __AUTO_OFFSET_H__
#define __AUTO_OFFSET_H__

#include "../inc/MarlinConfig.h"
#include "probe.h"
#include "motion.h"
#include "planner.h"
#include "stepper.h"
#include "settings.h"
#include "../gcode/parser.h"
#include "temperature.h"
#include "../HAL/shared/Delay.h"
#include "../lcd/dwin/e3v2/dwin.h"
#include "../../src/feature/bedlevel/bedlevel.h"
#include "../../src/feature/babystep.h"
#include "probe.h"

#include "stdio.h"
#include "string.h"
#include "../../gcode/gcode.h"


#if ENABLED(USE_AUTOZ_TOOL_2)

#define FOR_LOOP_TIMES(lp, sVal, eVal, fun) for(int lp = sVal; lp < eVal; lp++) fun
#define CHECK_RANGE(val, min, max) ((val > min) && (val < max))
#define CHECK_AND_BREAK(bVal) if(bVal) break
#define CHECK_AND_RUN(bVal, fun) if(bVal) fun
#define CHECK_AND_RETURN(bVal, rVal) if(bVal) return rVal
#define CHECK_AND_RUN_AND_RETURN(bVal, fun, rVal) if(bVal) {fun;return rVal;}
#define CHECK_AND_RUN_AND_ELSE(bVal, fun1, fun2); if(bVal) {fun1;} else {fun2;}

#define ARY_MIN(min, ary, count) for(int i = 0; i < count; i++) {CHECK_AND_RUN(min > ary[i], min = ary[i];)}
#define ARY_MAX(max, ary, count) for(int i = 0; i < count; i++) {CHECK_AND_RUN(max < ary[i], max = ary[i];)}
#define ARY_AVG(avg, ary, count) for(int i = 0; i < count; i++) {avg += ary[i];}; avg /= count
#define ARY_MIN_INDEX(min, index, ary, count) for(int i = 0; i < count; i++) {CHECK_AND_RUN(min >= ary[i], {min = ary[i]; index = i;})}
#define ARY_SORT(ary, count) FOR_LOOP_TIMES(i, 0, count, FOR_LOOP_TIMES(j, i, count, { if(ary[i] > ary[j]){float t = ary[i]; ary[i] = ary[j]; ary[j] = t;}}));

#define PRINTF(str, ...)  {char strMsg[128] = {0}; sprintf(strMsg, str, __VA_ARGS__); SERIAL_PRINT_MSG(strMsg);} 
#define RUN_AND_WAIT_GCODE_STR(gCode, isWait, ...)   {char cmd[128] = {0}; sprintf(cmd, gCode, __VA_ARGS__);PRINTF("\n***RUM GCODE CMD: %s***\n", cmd); RUN_AND_WAIT_GCODE_CMD(cmd, isWait);}
#define WAIT_BED_TEMP(maxTickMs, maxErr)    { unsigned int tickMs = GET_TICK_MS(); while (((GET_TICK_MS() - tickMs) < maxTickMs) && (abs(GET_BED_TAR_TEMP() - GET_BED_TEMP()) > maxErr)) MARLIN_CORE_IDLE();}
#define WAIT_HOTEND_TEMP(maxTickMs, maxErr) { unsigned int tickMs = GET_TICK_MS(); while (((GET_TICK_MS() - tickMs) < maxTickMs) && (abs(GET_HOTEND_TAR_TEMP(0) - GET_HOTEND_TEMP(0)) > maxErr)) MARLIN_CORE_IDLE();}


#define MIN_HOLD    2000    //触发检测的最小阈值，防止误触发
#define MAX_HOLD    10000   //触发检测的最大阈值，防止过触发
#define RC_CUTE_FRQ 1     //RC滤波器的截止频率0.1~10，值越小=触发越灵敏；值越大=触发越迟钝。
//速度越慢，截止频率越小


/***以下宏定义，需要根据Marlin的不同版本，对应实现***/
#define HX711_SCK_PIN                   PA4
#define HX711_SDO_PIN                   PC6
//是否有打印信息
#define SHOW_MSG    0
//压力传感器安装位置（对应螺丝孔的位置）
#define PRESS_XYZ_POS                   {AUTOZ_TOOL_X, AUTOZ_TOOL_Y, 0}
//喷头的温度膨胀补偿，即26到200度时，喷头伸长的长度
#define NOZ_TEMP_OFT_MM                 0.05
#define NOZ_AUTO_OFT_MM                 0.02 //0.04
//低通滤波器系数
#define LFILTER_K1_NEW                  0.9f
//以喷头为基准0点，CR-TOUCH的安装位置
#define CRTOUCH_OFT_POS                 NOZZLE_TO_PROBE_OFFSET
//热床的X方向尺寸，单位mm
#define BED_SIZE_X_MM                   X_BED_SIZE  //220
//热床的Y方向尺寸，单位mm
#define BED_SIZE_Y_MM                   Y_BED_SIZE  //220
//对高过程中Z轴抬升的高度，单位mm
#define HIGHT_UPRAISE_Z  4//8
//擦喷嘴的坐标
#define CLEAR_NOZZL_START_X  0
#define CLEAR_NOZZL_START_Y  15
#define CLEAR_NOZZL_END_X  0
#define CLEAR_NOZZL_END_Y  60
//自动对高的次数repeat
#define ZOFFSET_REPEAT_NIN  2
#define ZOFFSET_REPEAT_NAX  5
#define ZOFFSET_COMPARE     0.05
#define ZOFFSET_VALUE_MIN   -5
#define ZOFFSET_VALUE_MAX   0
//获取当前喷头的xyz的坐标
#define GET_CURRENT_POS()               current_position
//检测GCODE指令是否包含给定参数
#define GET_PARSER_SEEN(c)              parser.seen(c)
//获取GCODE指令中给定参数的值int型
#define GET_PARSER_INT_VAL()            parser.value_int()
//获取GCODE指令中给定参数的值float
#define GET_PARSER_FLOAT_VAL()          parser.value_float()
//通过串口打印调试信息
#define SERIAL_PRINT_MSG(msg)           {char* str = msg; while (*str != '\0') MYSERIAL1.write(*str++);} 
//获取系统时间戳
#define GET_TICK_MS()                   millis()
//延时us（微秒）
#define TIME_DELAY_US(dUs)              delay_us(dUs)
//Marlin的idle()主循环
#define MARLIN_CORE_IDLE()              idle()
//更新看门狗
#define REFRESH_WATCHDOG()              {HAL_watchdog_refresh();}
//堵塞执行一条GCODE指令
#define RUN_AND_WAIT_GCODE_CMD(gcode, isWait)   planner.synchronize();queue.inject_P(PSTR((gcode))); queue.advance(); if(isWait) {planner.synchronize();}

//设置GPIO工作模式
#define GPIO_SET_MODE(pin, isOut)       {if((isOut)>0) SET_OUTPUT((pin)); else SET_INPUT_PULLUP((pin));}
//设置GPIO输出值
#define GPIO_SET_VAL(pin, val)          WRITE((pin), (val))
//获取GPIO输入值
#define GPIO_GET_VAL(pin)               READ((pin))

//设置喷头温度
#define SET_HOTEND_TEMP(temp, index)    thermalManager.setTargetHotend((temp), (index))
//获取喷头当前温度
#define GET_HOTEND_TEMP(index)          thermalManager.temp_hotend[(index)].celsius
//获取喷头目标温度
#define GET_HOTEND_TAR_TEMP(index)      thermalManager.temp_hotend[(index)].target
//设置热床温度
#define SET_BED_TEMP(temp)              thermalManager.setTargetBed(temp)
//获取热床当前温度
#define GET_BED_TEMP()                  thermalManager.temp_bed.celsius
//获取热床目标温度
#define GET_BED_TAR_TEMP()              thermalManager.temp_bed.target
//获取喷头目标温度
#define GET_NOZZLE_TAR_TEMP(index)      thermalManager.temp_hotend[(index)].target
//堵塞等待喷头温度到达目标值
#define WAIT_NOZZLE_TEMP(index)         thermalManager.wait_for_hotend((index))
//设置热床调平使能状态
#define SET_BED_LEVE_ENABLE(sta)        set_bed_leveling_enabled(sta)


//获取模型散热风扇状态
#define GET_FAN_SPD()                   thermalManager.fan_speed[0]
//设定模型散热风扇状态0~255
#define SET_FAN_SPD(sta)                thermalManager.set_fan_speed(0, (sta))

//禁止全局中断，HX711读取数据时需使用
#define DIASBLE_ALL_ISR()               DISABLE_ISRS()
//恢复全司中断
#define ENABLE_ALL_ISR()                ENABLE_ISRS()

//XYZE电机，每毫米需要给定的步数
#define STEPS_PRE_UNIT                  DEFAULT_AXIS_STEPS_PER_UNIT     //#define DEFAULT_AXIS_STEPS_PER_UNIT   { 80, 80, 400, 424.9 }
//Z轴电机前进一步
#define AXIS_Z_STEP_PLUS(dUs)           {Z_STEP_WRITE(!INVERT_Z_STEP_PIN);TIME_DELAY_US((dUs));Z_STEP_WRITE(INVERT_Z_STEP_PIN);TIME_DELAY_US((dUs));}
//Z轴高度增加的方向
#define Z_DIR_ADD                       0
//Z轴高度减小的方向
#define Z_DIR_DIV                       (!Z_DIR_ADD)
//Z轴电机方向设置，1为前进，0为后退
#define AXIS_Z_DIR_SET(sta)             Z_DIR_WRITE(sta)
//Z轴电机方向读取
#define AXIS_Z_DIR_GET()                Z_DIR_READ()
//Z轴电机使能
#define AXIS_Z_ENABLE()                 ENABLE_AXIS_Z()

//检测电机是否正在运行，返回true为正在运行，返回false为运行完成
#define AXIS_XYZE_STATUS()              (planner.has_blocks_queued() || planner.cleaning_buffer_counter)    
//设定当前Z轴坐标为home点(0点)
#define SET_NOW_POS_Z_IS_HOME()         {set_axis_is_at_home(Z_AXIS);sync_plan_position();}

//使用CR-TOUCH测量给定点的Z值
#define PROBE_PPINT_BY_TOUCH(x, y)      probe.probe_at_point(x, y, PROBE_PT_RAISE, 0, false, true)
//设定Z-OFFSET并保存EEPROM
#define SET_Z_OFFSET(zOffset, isSave)   {dwin_zoffset = last_zoffset = probe.offset.z = zOffset; HMI_ValueStruct.offset_value = zOffset * 100;DWIN_UpdateLCD(); babystep.add_mm(Z_AXIS, zOffset);}

//堵塞并执行XY轴运动到给定位置
#define DO_BLOCKING_MOVE_TO_XY(x_mm, y_mm, spd_mm_s)    do_blocking_move_to_xy((x_mm), (y_mm), (spd_mm_s))
//堵塞并执行Z轴运动到给定位置
#define DO_BLOCKING_MOVE_TO_Z(z_mm, spd_mm_s)           do_blocking_move_to_z((z_mm), (spd_mm_s))

#define DO_BLOCKING_MOVE_TO_XYZ(x_mm, y_mm, z_mm,spd_mm_s)   do{ \
    current_position.set(x_mm, y_mm, z_mm); \
    line_to_current_position(spd_mm_s); \
    planner.synchronize(); \
  }while(0)
/***end***/

//RC高通滤波器，用于滤除如温度变化、导线拉扯等低频干扰。
//毛刺滤波器，用于去除连续数据中的毛刺
//低通滤波器，用于去除连接数据中的超高频噪声
class Filters
{
  public:
    static void hFilter(double *vals, int count, double cutFrqHz, double acqFrqHz);
    static void tFilter(double *vals, int count);
    static void lFilter(double *vals, int count, double k1New);
};

//HX711芯片驱动，用于读取压力传感器的数据
class HX711
{
public:
  void init(int clkPin, int sdoPin);
  int getVal(bool isShowMsg = 0);
  static bool ckGpioIsInited(int pin);
private:
  int clkPin;
  int sdoPin;
};

#define PI_COUNT 32

class ProbeAcq
{
  public:
    float baseSpdXY_mm_s;       //喷头在x和y平面移动的准备速度
    float baseSpdZ_mm_s;        //喷头在z平面移动的准备速度
    float minZ_mm;              //向Z轴运动到的停止位置，即到达此位置时还未检测到压力变化，则强制停止。

    float step_mm;              //每移动此距离，读取一次压力传感器
    int   minHold;              //最小阈值，触发条件的最后一个点需要大于此值
    int   maxHold;              //最大阈值，触发条件的最后一个点大于此值则强制触发

    float outVal_mm;            //输出，触发时对应的轴坐标
    int   outIndex;             //输出，触发时对应的序列索引,如果值不位于(PI_COUNT*2/3, PI_COUNT-1)区间，则测量错误，需要重新测量。

    xyz_float_t basePos_mm;     //即开始测量此点前，喷头需要达到的准备位置坐标
    
    HX711 hx711;                //指向CS123x或HX711的采集函数
    ProbeAcq* probePointByStep();   //根据此类的参数配置测试此点
    xyz_long_t readBase();      //获取干净的数据   
    bool checHx711();           //检测压力传感器是否工作正常
    void shakeZAxis(int times); //振动Z轴以消除间隙应力
    static float probeTimes(int max_times, xyz_float_t rdy_pos, float step_mm, float min_dis_mm, float max_z_err, int min_hold, int max_hold);
  private:
    double valP[PI_COUNT * 2];  //压力保存序列的压力值
    double posZ[PI_COUNT * 2];  //压力保存序列的对应坐标值
    
    void calMinZ();             //根据压力保存序列计算测量结果
    bool checkTrigger();        //检测压力保存序列中的数据是否满足触发条件
};
char *getStr(float f);
void gcodeG212();
bool clearByBed(xyz_float_t rdyPos_mm, float norm, float minTemp, float maxTemp);
bool probeByPress(xyz_float_t rdyPos_mm, float* outZ);
bool probeByTouch(xyz_float_t rdyPos_mm, float* outZ);
bool getZOffset(bool nozzleClr, bool runProByPress, bool runProByTouch, float* outOffset);
#endif
#endif