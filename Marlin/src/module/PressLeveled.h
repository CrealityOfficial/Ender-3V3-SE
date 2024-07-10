/**
  * @file       文件名（PressLeveled.h/PressLeveled.c）
  * @brief      使用应变片进行喷头擦拭、高度测量、数据校验、Z轴补偿相关接口
  * @details  
  * @attention  使用者移植过程中，需保证所有的宏定义正确且运行正常（当前宏定义基于Marlin 2.0.8.3）。
  * @author     王玉龙107051
  * @date       2022-06-20
  * @version    文件当前的版本号（V1.0.0）。
  * @copyright 
  */
#ifndef __PRESS_LEVELED_H__
#define __PRESS_LEVELED_H__

#include "../inc/MarlinConfig.h"
#include "probe.h"
#include "motion.h"
#include "planner.h"
#include "stepper.h"
#include "../gcode/parser.h"
#include "temperature.h"
#include "../HAL/shared/Delay.h"
//#include "../lcd/dwin/lcd_rts.h"

#if ENABLED(USE_AUTOZ_TOOL) //一键对高第一种方式
//设置GPIO工作模式
#define GPIO_SET_MODE(pin, isOut)       {if((isOut)>0) SET_OUTPUT((pin)); else SET_INPUT_PULLUP((pin));}
//设置GPIO输出值
#define GPIO_SET_VAL(pin, val)          WRITE((pin), (val))
//获取GPIO输入值
#define GPIO_GET_VAL(pin)               READ((pin))

//禁止全局中断，HX711读取数据时需使用
#define DIASBLE_ALL_ISR()               DISABLE_ISRS()
//恢复全司中断
#define ENABLE_ALL_ISR()                ENABLE_ISRS()

//热床的X方向尺寸，单位mm
#define BED_SIZE_X_MM                   X_BED_SIZE
//热床的Y方向尺寸，单位mm
#define BED_SIZE_Y_MM                   Y_BED_SIZE

#define BED_SIZE_Z_MM                   Z_MAX_POS
//自动调平X方向上的点数量
#define BED_MESH_X_PT                   GRID_MAX_POINTS_X
//自动调平Y方向上的点数量
#define BED_MESH_Y_PT                   GRID_MAX_POINTS_Y
//自动调平，边沿上的调平检测点距离热床边沿的距离
#define BED_PROBE_EDGE                  PROBING_MARGIN

//通过串口打印调试信息
#define SERIAL_PRINT_MSG(msg)           {char* str = msg; while (*str != '\0') MYSERIAL1.write(*str++);} 
//获取系统时间戳
#define GET_TICK_MS()                   millis()
//延时us（微秒）
#define TIME_DELAY_US(dUs)              DELAY_US(dUs)
//Marlin的idle()主循环
#define MARLIN_CORE_IDLE()              idle()

//更新看门狗
#define REFRESH_WATCHDOG()              {HAL_watchdog_refresh();}
//堵塞执行一条GCODE指令
#define RUN_AND_WAIT_GCODE_CMD(gcode, isWait)   planner.synchronize();queue.inject_P(PSTR((gcode))); queue.advance(); if(isWait) {planner.synchronize();}

//设置热床温度
#define SET_BED_TEMP(temp)              thermalManager.setTargetBed(temp)
//获取热床当前温度
#define GET_BED_TEMP()                  thermalManager.temp_bed.celsius
//获取热床目标温度
#define GET_BED_TAR_TEMP()              thermalManager.temp_bed.target

//设置喷头温度
#define SET_HOTEND_TEMP(temp, index)    thermalManager.setTargetHotend((temp), (index))
//获取喷头当前温度
#define GET_HOTEND_TEMP(index)          thermalManager.temp_hotend[(index)].celsius
//获取喷头目标温度
#define GET_HOTEND_TAR_TEMP(index)      thermalManager.temp_hotend[(index)].target

//设定模型散热风扇状态0~255
#define SET_FAN_SPD(sta)                thermalManager.fan_speed[0] = (sta)

//XYZE电机，每毫米需要给定的步数
#define STEPS_PRE_UNIT                  DEFAULT_AXIS_STEPS_PER_UNIT     //#define DEFAULT_AXIS_STEPS_PER_UNIT   { 80, 80, 400, 424.9 }
//Z轴电机前进一步
#define AXIS_Z_STEP_PLUS(dUs)           {Z_STEP_WRITE(!INVERT_Z_STEP_PIN);TIME_DELAY_US((dUs));Z_STEP_WRITE(INVERT_Z_STEP_PIN);TIME_DELAY_US((dUs));}
//Z轴电机方向读取
#define AXIS_Z_DIR_GET()                Z_DIR_READ()
//Z轴电机方向设置
#define AXIS_Z_DIR_SET(dir)             Z_DIR_WRITE(dir)
//Z轴电机方向设置,向Z+方向运动
#define AXIS_Z_DIR_ADD()                Z_DIR_WRITE(0)
//Z轴电机方向设置，向Z-方向运动
#define AXIS_Z_DIR_DIV()                Z_DIR_WRITE(1)
//Z轴电机使能
#define AXIS_Z_ENABLE()                 ENABLE_AXIS_Z()

//检测电机是否正在运行，返回true为正在运行，返回false为运行完成
#define AXIS_XYZE_STATUS()              (planner.has_blocks_queued() || planner.cleaning_buffer_counter)

//设定Z-OFFSET并保存EEPROM
#define SET_Z_OFFSET(zOffset)           {last_zoffset = probe.offset.z = zOffset; RTSUpdate();}
//设定当前Z轴坐标为home点(0点)
#define SET_NOW_POS_Z_IS_HOME()         {set_axis_is_at_home(Z_AXIS);sync_plan_position();}
//获取当前喷头的xyz的坐标
#define GET_CURRENT_POS()               current_position
//检测GCODE指令是否包含给定参数
#define GET_PARSER_SEEN(c)              parser.seen(c)
//获取GCODE指令中给定参数的值int型
#define GET_PARSER_INT_VAL()            parser.value_int()
//获取GCODE指令中给定参数的值float
#define GET_PARSER_FLOAT_VAL()          parser.value_float()

//堵塞并执行XY轴运动到给定位置
#define DO_BLOCKING_MOVE_TO_XY(x_mm, y_mm, spd_mm_s)    do_blocking_move_to_xy((x_mm), (y_mm), (spd_mm_s))
//堵塞并执行Z轴运动到给定位置
#define DO_BLOCKING_MOVE_TO_Z(z_mm, spd_mm_s)           do_blocking_move_to_z((z_mm), (spd_mm_s))

/***end***/

/***以下宏定义，为系统配置参数，可根据实际需求进行修改，一般保持默认即可***/
#define SHOW_MSG      1                 //!< 测量过程中，是否打印调试信息
#define BASE_COUNT    40                //!< 测量过程中，在正常使用HX711的数据前，先读取一定次数据的数据以保证HX711工作稳定
#define MIN_HOLD      1000              //!< 触发检测的最小阈值，防止误触发
#define MAX_HOLD      20000             //!< 触发检测的最大阈值，防止过触发
#define RC_CUTE_FRQ   1                 //!< RC滤波器的截止频率0.1~10，值越小=触发越灵敏；值越大=触发越迟钝。
#define PA_FIL_LEN_MM 20                //!< 挤出喷头擦拭过程中，耗材的长度，
#define PA_FIL_DIS_MM 30                //!< 挤出喷头擦拭过程中，擦拭的距离，
#define PA_CLR_DIS_MM 20                //!< 不挤出喷头擦拭过程中，擦拭的距离，越大擦拭成功率越高，但要小于热床Y_SIZE/2
#define BED_MAX_ERR   5                 //!< 产品所允许的，热床平面高度值的绝对值。如取值为5，则表明热床中心点Z高度为0，产品热床其它任意点的高度Z相对于中心点的差的绝对值最大不超过5mm
#define MAX_REPROBE_CNT   ((BED_MESH_X_PT * BED_MESH_X_PT) / 2) //!< 如果调平点的总重复测量次数超过此值，则可认为喷头上粘有耗材，要进行喷头擦拭后再次测量
#define DEFORMATION_SIZE    0.00        // The size of the deformation when the sensor is triggered, in mm, is related to the sensitivity of the sensor itself
                                        // The sensitivity should be significantly modified or a different strain gauge should be replaced
/***end***/

/***以下宏定义，为系统调用，保持默认***/
#define IS_ZERO(val)      (fabs(val) < 0.00001 ? true : false)  //!< 浮点型变量是否为0的判断
#define PRINTF(str, ...)  {char strMsg[128] = {0}; sprintf(strMsg, str, __VA_ARGS__); SERIAL_PRINT_MSG(strMsg);} //替换sprintf和printf的接口
/***end***/

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

// RC高通滤波器，用于滤除如温度变化、导线拉扯等低频干扰。
class RCLFilter
{
private:
  double vi;
  double viPrev;
  double voPrev;
  double vo;

public:
  double cutoffFrqHz;
  double acqFrqHz;

  void init(double cutFrqHz, double acqFrqHz, double baseVal);
  double filter(double newVal);
};

//使用压力传感器探测单点高度类
#define PI_COUNT 32
class ProbeAcq
{
  public:
    xyz_float_t basePos_mm;     //即开始测量此点前，喷头需要达到的准备位置坐标
    float baseSpdXY_mm_s;       //喷头在x和y平面移动的准备速度
    float baseSpdZ_mm_s;        //喷头在z平面移动的准备速度
    float minZ_mm;              //向Z轴运动到的停止位置，即到达此位置时还未检测到压力变化，则强制停止。
    bool  isUpAfter;            //测量完成后，是否复位Z轴到准备位置
    bool  isShowMsg;

    float step_mm;              //每移动此距离，读取一次压力传感器
    int   minHold;              //最小阈值，触发条件的最后一个点需要大于此值
    int   maxHold;              //最大阈值，触发条件的最后一个点大于此值则强制触发

    float outVal_mm;            //输出，触发时对应的轴坐标
    int   outIndex;             //输出，触发时对应的序列索引,如果值不位于(PI_COUNT*2/3, PI_COUNT-1)区间，则测量错误，需要重新测量。

    HX711 hx711;                //指向CS123x或HX711的采集函数
    RCLFilter rcFilter;         //高通滤波器
    ProbeAcq* probePointByStep();   //根据此类的参数配置测试此点
    xyz_long_t readBase();      //获取干净的数据
    bool checHx711();           //检测压力传感器是否工作正常
  private:
    double valP[PI_COUNT];      //压力保存序列的压力值
    double posZ[PI_COUNT];      //压力保存序列的对应坐标值

    void calMinZ();             //根据压力保存序列计算测量结果
    bool checkTrigger(int fitVal, int unfitDifVal); //检测压力保存序列中的数据是否满足触发条件
};

/* 1、调试指令相关内容 */
void doG212ExCmd(void);

/* 2、G28 HOME点获取相关*/
void doG28ZProbe(bool isPrecision);

/* 3、G29 多点测量前的准备相关*/
void initBedMesh(void);
bool clearNozzle(float minHotendTemp, float maxHotendTemp, float bedTemp, int doExFil_mm);
bool probeReady(void);

/* 4、G29 单点测量相关*/
float doG29Probe(float xPos, float yPos);
bool setBedMesh(int xIndex, int yIndex, float xPos_mm, float yPos_mm, float zPos_mm);

/* 5、G29 测量完成后续操作*/
void checkAndReProbe(void);
void updataBedMesh(void);
void printBedMesh(void);

/* 6、打印时Z轴补偿值获取相关*/
float getMeshZ(float xPos, float yPos);
#endif
#endif
