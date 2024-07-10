#include "PressLeveled.h"
#include "stdio.h"
#include "string.h"
#if ENABLED(USE_AUTOZ_TOOL) //一键对高第一种方式

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
#define ARY_MAX_INDEX(max, index, ary, count) for(int i = 0; i < count; i++) {CHECK_AND_RUN(max <= ary[i], {max = ary[i]; index = i;})}

#define WAIT_BED_TEMP(maxTickMs, maxErr)    { unsigned int tickMs = GET_TICK_MS();\
                                              while (((GET_TICK_MS() - tickMs) < maxTickMs) && (abs(GET_BED_TAR_TEMP() - GET_BED_TEMP()) > maxErr))\
                                                MARLIN_CORE_IDLE();}

#define WAIT_HOTEND_TEMP(maxTickMs, maxErr) { unsigned int tickMs = GET_TICK_MS();\
                                              while (((GET_TICK_MS() - tickMs) < maxTickMs) && (abs(GET_HOTEND_TAR_TEMP(0) - GET_HOTEND_TEMP(0)) > maxErr))\
                                                MARLIN_CORE_IDLE();}

#define RUN_AND_WAIT_GCODE_STR(gCode, isWait, ...)   {char cmd[128] = {0}; sprintf(cmd, gCode, __VA_ARGS__);PRINTF("\n***RUM GCODE CMD: %s***\n", cmd); RUN_AND_WAIT_GCODE_CMD(cmd, isWait);}

int         g29ReprobeCount = 0;    //有于统计重次测量次数，可用来判断调平时喷头是否粘有耗材

// 保存擦拭参数
float pl_minHotendTemp = 0;
float pl_maxHotendTemp = 200;
float pl_bedTemp = 60;
xyz_float_t g29BasePos[] = {{BED_PROBE_EDGE - 2, BED_PROBE_EDGE - 2, 0},
                            {BED_PROBE_EDGE - 2, BED_SIZE_Y_MM - BED_PROBE_EDGE + 2, 0},
                            {BED_SIZE_X_MM - BED_PROBE_EDGE + 2, BED_SIZE_Y_MM - BED_PROBE_EDGE + 2, 0},
                            {BED_SIZE_X_MM - BED_PROBE_EDGE + 2, BED_PROBE_EDGE - 2, 0}};
xyz_float_t bedMesh[BED_MESH_X_PT + 2][BED_MESH_Y_PT + 2];

/**
 * @brief 将浮点转成字符串（注意：此处不使用ftoa()或%f，是因为部分Marlin版本BUG，会导致浮点转字符失败）
 * @param f 需要转换的浮点值
 * @return char* 转换后的字符串
 * @attention 重载调用次数不要超过16
 */
char *getStr(float f)
{
  static char str[16][16];
  static int index = 0;

  memset(str[index % 16], '\0', 16);
  sprintf(str[index % 16], (f >= 0 ? "+%d.%03d" : "-%d.%03d"), (int)fabs(f),  ((int)(fabs(f) * 1000)) % 1000);

  return str[index++ % 16];
}

/**
 * @brief 检测给定pin是否已经初始化过，避免对clk重复初始化而导致时序错乱
 * @param pin 待检测的pin
 * @return true=pin已初始化过；false=pin未经过初始化。 
 */
bool HX711::ckGpioIsInited(int pin)
{
  static int pinList[32] = {0};
  FOR_LOOP_TIMES(i, 0, 32, {CHECK_AND_RETURN((pinList[i] == pin), true);CHECK_AND_RUN_AND_RETURN((pinList[i] == 0), {pinList[i] = pin;}, false); });
  return true;
}

/**
 * @brief HX711驱动初始化（80HZ采样率）
 * @param clkPin HX711对应的时钟信号
 * @param sdoPin HX711对应的数据信号
 */
void HX711::init(int clkPin, int sdoPin)
{
  this->clkPin = clkPin;
  this->sdoPin = sdoPin;
  CHECK_AND_RUN((!HX711::ckGpioIsInited(sdoPin)), GPIO_SET_MODE(sdoPin, 0));
  CHECK_AND_RUN((!HX711::ckGpioIsInited(clkPin)), {GPIO_SET_MODE(clkPin, 1); GPIO_SET_VAL(clkPin, 0); });
}

/**
 * @brief 阻塞并读取压力值，最大阻塞时间20ms
 * @param isShowMsg isShowMsg 是否打印调试信息
 * @return int 读取到的压力值
 * @attention 要达到80HZ的采样速率，要保证HX711的频率配置引脚已给对应电平
 */
int HX711::getVal(bool isShowMsg)
{
  static unsigned int lastTickMs = 0;
  int count = 0;
  unsigned int ms = GET_TICK_MS();

  GPIO_SET_VAL(clkPin, 0);

  while (GPIO_GET_VAL(sdoPin) == 1 && (GET_TICK_MS() - ms <= 20))  //采样率80Hz（12ms周期），这里最大给20ms延时
    MARLIN_CORE_IDLE();

  DIASBLE_ALL_ISR();
  for (int i = 0; i < 24; i++)
  {
    GPIO_SET_VAL(clkPin, 1);
    count = count << 1;
    GPIO_SET_VAL(clkPin, 0);
    CHECK_AND_RUN((GPIO_GET_VAL(sdoPin) == 1), (count++));
  }
  ENABLE_ALL_ISR();

  GPIO_SET_VAL(clkPin, 1);
    count |= ((count & 0x00800000) != 0 ? 0xFF000000 : 0); //24位有符号，转成32位有符号
  GPIO_SET_VAL(clkPin, 0);

  CHECK_AND_RUN(isShowMsg, {PRINTF("T=%08d, S=%08d\n", (int)(GET_TICK_MS() - lastTickMs), (int)count);lastTickMs = GET_TICK_MS(); });

  return count;
}

/**
 * @brief RC高通滤波器初始化
 * @param cutFrqHz 滤波器的截止频率
 * @param acqFrqHz 滤波器的采样频率（等同于HX711的采样频率80HZ）
 * @param baseVal 滤波器的初始值（方便滤波器快速进入稳定状态）
 */
void RCLFilter::init(double cutFrqHz, double acqFrqHz, double baseVal)
{
  this->vi = this->viPrev = baseVal;
  this->voPrev = this->vo = 0;
  this->cutoffFrqHz = cutFrqHz; // high pass filter @cutoff frequency = XX Hz
  this->acqFrqHz = acqFrqHz;      // execute XX every second  
}

/**
 * @brief 对一个值使用RC滤波器进行滤波，并返回滤波后的值
 * @param newVal 未滤波前的值
 * @return double 滤波后的植
 */
double RCLFilter::filter(double newVal)
{
  double rc, coff;
  this->vi = newVal;
  rc = 1.0f / 2.0f / PI / this->cutoffFrqHz;
  coff = rc / (rc + 1 / this->acqFrqHz);
  this->vo = (this->vi - this->viPrev + this->voPrev) * coff;
  this->voPrev = this->vo; // update
  this->viPrev = this->vi;
  return fabs(this->vo);
}

/** 
 * @brief 获取给定数量内(BASE_COUNT/2)的最大、最小、平均压力值。
 * @return x=MIN; y=AVG; z=MAX;
 */
xyz_long_t ProbeAcq::readBase()
{
  xyz_long_t xyz = {+0x00FFFFFF, 0, -0x00FFFFFF}; // min avg max
  long avg = 0, v = 0;
  FOR_LOOP_TIMES(i, 0, BASE_COUNT / 2, { v = this->hx711.getVal(false); });
  FOR_LOOP_TIMES(i, 0, BASE_COUNT / 2, {
    v = this->hx711.getVal(0);
    xyz.x = xyz.x > v ? v : xyz.x;
    xyz.z = xyz.z < v ? v : xyz.z;
    avg += v; });

  xyz.y = (int)(avg / (BASE_COUNT / 2));
  this->rcFilter.init(this->rcFilter.cutoffFrqHz, this->rcFilter.acqFrqHz, v);

  PRINTF("\n***BASE:MIN=%d, AVG=%d, MAX=%d***\n\n", (int)xyz.x, (int)xyz.y, (int)xyz.z);
  return xyz;
}

/**
 * @brief 检测HX711模块是否工作正常
 * @return true:正常,false 不正常
 * @attention 理论上，HX711在一定时间内，最大与最值压力差应大于100，且小于MIN_HOLD。
 */
bool ProbeAcq::checHx711()
{
  xyz_long_t bv = readBase();
  return (abs(bv.x - bv.z) < 100 || abs(bv.x - bv.z) > 2 * MIN_HOLD) ? false : true;
}

/** 
 * @brief 根据触发后压力队列中的压力值，计算出对应的z轴高度
 * @param 
 * @return 
 */
void ProbeAcq::calMinZ()
{
  double valMin = +0x00FFFFFF, valMax = -0x00FFFFFF, bCs[PI_COUNT] = {0}, maxE = 0;
  // 1、一阶互补滤波
  #define CAL_K1 0.5f
  FOR_LOOP_TIMES(i, 1, PI_COUNT, valP[i] = valP[i] * CAL_K1 + valP[i - 1] * (1 - CAL_K1));

  #ifdef SHOW_MSG
  // 2、打印结果
  PRINTF("%s", "\nx=[");
  FOR_LOOP_TIMES(i, 0, PI_COUNT, PRINTF((i == (PI_COUNT - 1) ? "%s]\n\n" : "%s,"), getStr(posZ[i])));
  PRINTF("%s", "y=[");
  FOR_LOOP_TIMES(i, 0, PI_COUNT, PRINTF((i == (PI_COUNT - 1) ? "%s]\n\n" : "%s,"), getStr(valP[i])));
  #endif

  // 3、对数据进行归一化，方便处理
  ARY_MIN(valMin, valP, PI_COUNT);
  ARY_MAX(valMax, valP, PI_COUNT);
  FOR_LOOP_TIMES(i, 0, PI_COUNT, {valP[i] = (valP[i] - valMin) / (valMax - valMin); bCs[i] = valP[i]; });

  // 4、计算并按给定角度翻转数据，以方便计算出最早的触发点
  double angle = atan((valP[PI_COUNT - 1] - valP[0]) / PI_COUNT);
  double sinAngle = sin(-angle), cosAngle = cos(-angle);
  FOR_LOOP_TIMES(i, 0, PI_COUNT, valP[i] = (i - 0) * sinAngle + (valP[i] - 0) * cosAngle + 0); //延原点(0,0)顺时针旋转angle度，这里可以不考虑X轴坐标值

  // 5、找出翻转后的最小值索引
  valMin = +0x00FFFFFF;
  ARY_MIN_INDEX(valMin, this->outIndex, valP, PI_COUNT);

  ARY_MAX(maxE, bCs, this->outIndex);
  this->outVal_mm = posZ[this->outIndex];
  this->outIndex = (((maxE > 0.2) || (posZ[PI_COUNT - 1] <= minZ_mm)) ? (PI_COUNT - 1) : this->outIndex); //对计算结果进行验证

  PRINTF("\n***CalZ Idx=%d, Z=%s***\n\n", this->outIndex, getStr(this->outVal_mm));
}

/** 
 * @brief 触发状态检测，用于检测喷头是否与压力传感器正常接触
 * @param fitVal RC低通滤波后的压力值，用于特征检测
 * @param unfitDifVal 未滤波但去零后的压力值，用于超压检测
 * @return true=检测到解发或中止；false=未满足触发条件。
 */
bool ProbeAcq::checkTrigger(int fitVal, int unfitDifVal)
{
  double valMin = +0x00FFFFFF, valMax = -0x00FFFFFF, valAvg = 0, valP_t[PI_COUNT] = {0}, valP_n[PI_COUNT] = {0};
  static long long lastTickMs = 0, lastUnfitDifVal = 0;

  #ifdef SHOW_MSG
  if(isShowMsg)
  {
    PRINTF("T=%08d , S=%08d , U=%08d \n", (int)(GET_TICK_MS() - lastTickMs), fitVal, unfitDifVal);
    lastTickMs = GET_TICK_MS();
  }
  #endif

  // 0、复制一份用于检测，防止对源数据造成影响
  FOR_LOOP_TIMES(i, 0, PI_COUNT, valP_t[i] = this->valP[i]);

  // 1、最高优先级，到达最小位置还未触发，则强制停止
  CHECK_AND_RETURN((this->posZ[PI_COUNT - 1] <= this->minZ_mm), true);

  // 2、最高优先级，压力大于最大压力，则无条件停止。
  CHECK_AND_RUN_AND_RETURN((abs(unfitDifVal) >= this->maxHold && abs(lastUnfitDifVal) >= this->maxHold), this->outIndex = PI_COUNT - 1, true);
  lastUnfitDifVal = unfitDifVal;

  // 3、检测点数未满足最小需求
  CHECK_AND_RETURN(IS_ZERO(this->valP[0]), false);

  // 4、剔除错误的数据
  FOR_LOOP_TIMES(i, 1, PI_COUNT - 1, CHECK_AND_RUN((abs(valP_t[i]) >= this->maxHold && abs(valP_t[i - 1]) < (this->maxHold / 2) && abs(valP_t[i + 1]) < (this->maxHold / 2)), valP_t[i] = valP_t[i - 1]));

  // 5、检测未尾3个点大小的是否有序递增
  CHECK_AND_RETURN((!((valP_t[PI_COUNT - 1] > valP_t[PI_COUNT - 2]) && (valP_t[PI_COUNT - 2] > valP_t[PI_COUNT - 3]))), false);

  // 6、检测未3个点绝对值是否大于其它任意点
  FOR_LOOP_TIMES(i, 0, PI_COUNT - 3, CHECK_AND_RETURN((valP_t[PI_COUNT - 1] < valP_t[i] || valP_t[PI_COUNT - 2] < valP_t[i] || valP_t[PI_COUNT - 3] < valP_t[i]), false));

  // 6、对数据进行归一化，方便处理
  ARY_MIN(valMin, valP_t, PI_COUNT);
  ARY_MAX(valMax, valP_t, PI_COUNT);
  ARY_AVG(valAvg, valP_t, PI_COUNT);
  FOR_LOOP_TIMES(i, 0, PI_COUNT, valP_n[i] = (float)(((double)valP_t[i] - valMin) / (valMax - valMin)));

  // 7、保证所有点针对最后一个点的斜率均大于40度,可有郊防止过于灵敏而导致的误触发。
  FOR_LOOP_TIMES(i, 0, PI_COUNT - 1, CHECK_AND_RETURN(((valP_n[PI_COUNT - 1] - valP_n[i]) / ((32 - i) * 1.0f / 32) < 0.8), false));

  // 8、限制最小值，防止误触发。
  CHECK_AND_RETURN((valP_t[PI_COUNT - 1] < this->minHold), false);
  return true;
}

/**
 * @brief 单步进法测量，并返回测量结果。
 * @return ProbeAcq* this指针
 */
ProbeAcq* ProbeAcq::probePointByStep()
{
  int unfitAvgVal = 0, unfitNowVal = 0, fitNowVal = 0, runSteped = 0;

  #ifdef SHOW_MSG
    PRINTF("\nPROBE: rdyX=%s, rdyY=%s, rdyZ=%s, spdXY_mm_s=%s, spdZ_mm_s=%s",
                          getStr(this->basePos_mm.x), getStr(this->basePos_mm.y), getStr(this->basePos_mm.z), getStr(this->baseSpdXY_mm_s), getStr(this->baseSpdZ_mm_s));
    PRINTF("len_mm=%s, baseCount=%d, minHold=%d, maxHold=%d, step_mm=%s\n\n",
                          getStr(this->minZ_mm), BASE_COUNT, this->minHold, this->maxHold, getStr(this->step_mm)); 
  #endif

  memset(this->posZ, 0, sizeof(this->posZ));
  memset(this->valP, 0, sizeof(this->valP));

  //运动到等测量点（准备点）
  RUN_AND_WAIT_GCODE_STR("G1 F%s X%s Y%s Z%s", true,  getStr(this->baseSpdXY_mm_s * 60), getStr(this->basePos_mm.x), getStr(this->basePos_mm.y), getStr(this->basePos_mm.z));

  SET_HOTEND_TEMP(0, 0);
  unfitAvgVal = readBase().y;                   //获取干净的数据

  float steps[] = STEPS_PRE_UNIT;               // { 80, 80, 400, 424.9 }

  int runStep = this->step_mm * steps[2];       //获取每步进step_mm距离，对应电机的脉冲数量
  runStep = runStep <= 0 ? 1 : runStep;

  int delayUs = (11.0f * 1000 / runStep) / 2;   //保证12ms左右，Z轴电机运动step_mm的距离
  delayUs = delayUs <= 10 ? 10 : delayUs;

  int oldDir = AXIS_Z_DIR_GET();
  AXIS_Z_ENABLE();
  AXIS_Z_DIR_DIV();
  do{
    FOR_LOOP_TIMES(i, 0, runStep, AXIS_Z_STEP_PLUS(delayUs)); //向给定方向运动给定步进距离
    runSteped += runStep;
    MARLIN_CORE_IDLE();

    unfitNowVal = this->hx711.getVal(0);            //获取压力值
    fitNowVal = this->rcFilter.filter(unfitNowVal); //对压力值进行高通滤波
    FOR_LOOP_TIMES(i, 0, PI_COUNT - 1, this->valP[i] = this->valP[i + 1]);
    FOR_LOOP_TIMES(i, 0, PI_COUNT - 1, this->posZ[i] = this->posZ[i + 1]);

    this->valP[PI_COUNT - 1] = fitNowVal;                                           //将压力值放入队列
    this->posZ[PI_COUNT - 1] = GET_CURRENT_POS().z - (float)runSteped / steps[2];   //将压力值对应的z位置放入队列

    // if((checkTrigger(fitNowVal, abs(unfitNowVal - unfitAvgVal)) == true) && (0 == READ(PROBE_ACTIVATION_SWITCH_PIN))) //触发条件检测
    if(checkTrigger(fitNowVal, abs(unfitNowVal - unfitAvgVal)) == true)
    {
      if(thermalManager.temp_hotend[0].target < pl_minHotendTemp)
      {
        SET_HOTEND_TEMP(pl_minHotendTemp, 0);
      }
      calMinZ();          //数据整理与计算
      runSteped = ((this->isUpAfter == false) ? (PI_COUNT - this->outIndex - 1) * runStep : runSteped);
      AXIS_Z_DIR_ADD();
      FOR_LOOP_TIMES(i, 0, runSteped, AXIS_Z_STEP_PLUS(delayUs / 4)); //返回到触发点或准备点
      AXIS_Z_DIR_SET(oldDir);
      break;
    }
  } while (1);
  return this;
}

/** 
 * @brief 对p1和p2两点进行线性拟合，并计算po点在此直线上的位置
 * @param p1 待拟合的第一个点
 * @param p2 待拟合的第二个点
 * @param po 待计算的点
 * @param isBaseX 是否将三维直线投影到x平面
 * @return po点在拟合后直线上的位置
 */
float getLinear2(xyz_float_t *p1, xyz_float_t *p2, xyz_float_t *po, int isBaseX)
{
  if ((IS_ZERO(p1->x - p2->x) && isBaseX > 0) || (IS_ZERO(p1->y - p2->y) && isBaseX == 0))
    return 0;
  // 线性方式 z=ax+b或z=ay+b，以下是计算a和b
  float a = (p2->z - p1->z) / (isBaseX > 0 ? (p2->x - p1->x) : (p2->y - p1->y));
  float b = p1->z - (isBaseX > 0 ? p1->x : p1->y) * a;

  po->z = a * (isBaseX > 0 ? po->x : po->y) + b;

  return po->z;
}

/**
 * @brief 在调平测试单点时，为提高效率和安全性，这里获取最佳的Z轴准备高度
 * @param x 等检测点的x坐标
 * @param y 待检测点的y坐标
 * @return float 最佳准备点坐标
 */
float getBestProbeZ(float x, float y)
{
  if(IS_ZERO(g29BasePos[0].z) && IS_ZERO(g29BasePos[1].z) && IS_ZERO(g29BasePos[2].z) && IS_ZERO(g29BasePos[3].z))
    return BED_MAX_ERR;

  xyz_float_t pLeft = {g29BasePos[0].x, y, 0}, pRight = {g29BasePos[2].x, y, 0}, pMid = {x, y, 0};
  pLeft.z = getLinear2(&g29BasePos[0], &g29BasePos[1], &pLeft, 0);
  pRight.z = getLinear2(&g29BasePos[2], &g29BasePos[3], &pRight, 0);
  pMid.z = getLinear2(&pLeft, &pRight, &pMid, 1);

  return pMid.z;
}

/**
 * @brief 打印调平数据矩阵信息，可用于热床平面3D图绘制
 */
void printBedMesh(void)
{
  //测量数据，输出坐标+测量值方式
  FOR_LOOP_TIMES(i, 0, BED_MESH_X_PT + 2, {
    PRINTF("%s", "\n");
    FOR_LOOP_TIMES(j, 0, BED_MESH_Y_PT + 2, PRINTF("(%s, %s, %s), ", getStr(bedMesh[i][j].x), getStr(bedMesh[i][j].y), getStr(bedMesh[i][j].z)););
  });
  PRINTF("%s", "\n");

  //测量数据，输出Excel表格格式，
  FOR_LOOP_TIMES(j, 1, BED_MESH_Y_PT + 1, {
    PRINTF("%s", "\n");
    FOR_LOOP_TIMES(i, 1, BED_MESH_X_PT + 1, PRINTF(((i == BED_MESH_X_PT + 1 - 1) ? "%s" : "%s\t"), getStr(bedMesh[i][j].z)))
  });
  PRINTF("%s", "\n\n");

  //测量数据，输出PYTHON格式
  FOR_LOOP_TIMES(j, 1, BED_MESH_Y_PT + 1, {
    PRINTF("%s", "[");
    FOR_LOOP_TIMES(i, 1, BED_MESH_X_PT + 1, PRINTF("%s%s", getStr(bedMesh[i][j].z), ((i == BED_MESH_X_PT + 1 - 1) ? "],\n" : ",")));
  });
  PRINTF("%s", "\n\n");

  //测量数据+扩充数据，输出PYTHON格式
  FOR_LOOP_TIMES(i, 0, BED_MESH_X_PT + 2, {
    PRINTF("%s", "[");
    FOR_LOOP_TIMES(j, 0, BED_MESH_Y_PT + 2, PRINTF("%s%s", getStr(bedMesh[i][j].z), (j == BED_MESH_Y_PT + 1 ? "],\n" : ",")));
  });
  PRINTF("%s", "\n\n");
}

/**
 * @brief 初始化调平数据矩阵
 */
void initBedMesh(void)
{
  xyz_float_t t = {0, 0, 0};
  g29ReprobeCount = 0;
  FOR_LOOP_TIMES(i, 0, BED_MESH_X_PT + 2, FOR_LOOP_TIMES(j, 0, BED_MESH_Y_PT + 2, {bedMesh[i][j]= t;}));
  // SET_Z_OFFSET(0);
  // probe.offset.z Parameters must be saved
  // probe.offset.z = 0;
}

/**
 * @brief 在热床是进行喷头擦拭操作
 * @param minTemp 喷头耗材不再渗出时的温度
 * @param maxTemp 喷头耗材可以挤出并正常打印时的温度
 * @param doExFil 擦拭过程中挤出耗材的长度，等于0表明不挤出
 * @return true:擦拭成功；false:擦拭失败
 * @attention 喷头擦拭需要较长时间，如有优化或更合适的擦拭方式，可以改写此功能
 */
bool clearNozzle(float minHotendTemp, float maxHotendTemp, float bedTemp, int doExFil_mm)
{
  xyz_float_t srcPos = {BED_PROBE_EDGE / 2, BED_PROBE_EDGE, BED_MAX_ERR + 1};
  ProbeAcq pa;
  pa.hx711.init(HX711_SCK_PIN, HX711_SDO_PIN);
  pa.rcFilter.init(RC_CUTE_FRQ, 80, 0);
  srand(millis());        //随机取热床上的一段距离用于擦拭，防止频繁使用同一处而对热床产生磨损
  srcPos.y = rand() % (BED_SIZE_Y_MM - 6 * BED_PROBE_EDGE - PA_CLR_DIS_MM) + 4 * BED_PROBE_EDGE;
  SET_FAN_SPD(0);

  pl_minHotendTemp = minHotendTemp;
  pl_maxHotendTemp = maxHotendTemp;
  pl_bedTemp = bedTemp;

  pa.minZ_mm = -5;
  pa.basePos_mm = srcPos;
  pa.basePos_mm.y = srcPos.y - PA_FIL_DIS_MM;
  pa.baseSpdXY_mm_s = 100;
  pa.baseSpdZ_mm_s = 5;
  pa.step_mm = 0.025;
  pa.isUpAfter = true;
  pa.isShowMsg = false;
  pa.minHold = MAX_HOLD / 3;
  pa.maxHold = MAX_HOLD;
 
  SET_BED_TEMP(bedTemp);            //热床加热后有形变，因此应在60度左右时测量误差最小
  SET_HOTEND_TEMP(maxHotendTemp, 0);

  DO_BLOCKING_MOVE_TO_Z(srcPos.z, pa.baseSpdZ_mm_s);
  DO_BLOCKING_MOVE_TO_XY(0, 0, pa.baseSpdXY_mm_s);

  WAIT_HOTEND_TEMP((3*60*1000), 5);

  pa.probePointByStep();
  int unFitVal = pa.readBase().y;

  if(doExFil_mm > 0)//喷头擦拭过程中，是否挤出耗材
  {
    DO_BLOCKING_MOVE_TO_XY(srcPos.x, srcPos.y - PA_FIL_DIS_MM, pa.baseSpdXY_mm_s);
    DO_BLOCKING_MOVE_TO_Z(pa.outVal_mm + 0.2, 5);
    
    RUN_AND_WAIT_GCODE_CMD("G92 E0", true);
    RUN_AND_WAIT_GCODE_STR("G1 F100 Y%s Z%s E%s", true, getStr(srcPos.y - 5), getStr(pa.outVal_mm + 0.1), getStr(doExFil_mm));
    RUN_AND_WAIT_GCODE_STR("G1 F100 Y%s Z%s E%s", true, getStr(srcPos.y), getStr(pa.outVal_mm + 0.1), getStr(doExFil_mm - 2));
  }

  DO_BLOCKING_MOVE_TO_XY(srcPos.x, srcPos.y, pa.baseSpdXY_mm_s);
  DO_BLOCKING_MOVE_TO_Z(pa.outVal_mm + 0.1, 5);

  RUN_AND_WAIT_GCODE_STR("G1 F100 Y%s", false,  getStr(srcPos.y + PA_CLR_DIS_MM));

  SET_HOTEND_TEMP(minHotendTemp, 0);
  SET_FAN_SPD(255);

  float steps[] = STEPS_PRE_UNIT;
  int runStep = 0.02 * steps[2];
  int delayUs = (12.0f * 1000 / runStep) / 2;

  AXIS_Z_ENABLE();
  while (AXIS_XYZE_STATUS()){
    int nowVal = pa.hx711.getVal(0);
    CHECK_AND_RUN_AND_ELSE((abs(nowVal - unFitVal) < pa.minHold), AXIS_Z_DIR_DIV(), AXIS_Z_DIR_ADD());
    FOR_LOOP_TIMES(i, 0, runStep, AXIS_Z_STEP_PLUS(delayUs));
    MARLIN_CORE_IDLE();
  }

  AXIS_Z_DIR_ADD();
  WAIT_HOTEND_TEMP((3 * 60 * 1000), 5);
  SET_FAN_SPD(0);
  RUN_AND_WAIT_GCODE_STR("G1 F100 Y%s Z%s", true, getStr(GET_CURRENT_POS().y + 5), getStr(GET_CURRENT_POS().z + 1));

  DO_BLOCKING_MOVE_TO_Z(GET_CURRENT_POS().z + BED_MAX_ERR + 1, 5);
  WAIT_BED_TEMP((3 * 60 * 1000), 5);

  RUN_AND_WAIT_GCODE_CMD("G28 Z C0", true);
  return true;
}

/**
 * @brief 在进行16个点的调平点高度测量前，可以先调用此函数，获取热床大致平面，可以缩短后期16个点的测量时间
 * @param rdyZ_mm 准备点测量时的准备高度，即等于 热床平面最高点高度(Z+1)mm，如产品允许热床最大倾斜偏差为正负5mm，则此值取6mm
 * @return true:准备点测量完成;false:准备点测量失败
 */
bool probeReady(void)
{
  ProbeAcq pa;
  pa.hx711.init(HX711_SCK_PIN, HX711_SDO_PIN);
  pa.rcFilter.init(RC_CUTE_FRQ, 80, 0);

  pa.minZ_mm = -10;
  pa.baseSpdXY_mm_s = 200;
  pa.baseSpdZ_mm_s = 5;
  pa.step_mm = 0.025;
  pa.isUpAfter = true;
  pa.isShowMsg = false;
  pa.minHold = MIN_HOLD;
  pa.maxHold = MAX_HOLD;

  FOR_LOOP_TIMES(i, 0, 4, g29BasePos[i].z = BED_MAX_ERR + 1);

  FOR_LOOP_TIMES(i, 0, 4, {
    float z1 = 0;
    float z2 = 0;
    pa.basePos_mm = g29BasePos[i];
    FOR_LOOP_TIMES(j, 0, 3, {
      z1 = pa.probePointByStep()->outVal_mm;
      z2 = pa.probePointByStep()->outVal_mm;
      CHECK_AND_BREAK((fabs(z1 - z2) <= 0.2));
    });
    g29BasePos[i].z = (z1 + z1) / 2;
  });

  PRINTF("***G29 Ready={%s, %s, %s, %s}***\n", getStr(g29BasePos[0].z), getStr(g29BasePos[1].z), getStr(g29BasePos[2].z), getStr(g29BasePos[3].z));
  return true;
}

/**
 * @brief 设定调平数据矩阵给定点的坐标及测量结果，在G29调平过程中，每测量完一个点，均需要更新测量结果到mesh内
 * @param xIndex 沿X轴方向的点索引
 * @param yIndex 沿X轴方向的点索引
 * @param xPos_mm 测量点的x坐标
 * @param yPos_mm 测量点的y坐标
 * @param zPos_mm 测量点测量出的热床高度z值
 * @return true:添加成功;false:添加失败
 */
bool setBedMesh(int xIndex, int yIndex, float xPos_mm, float yPos_mm, float zPos_mm)
{
  xyz_float_t t = {xPos_mm,yPos_mm,zPos_mm};
  CHECK_AND_RETURN((xIndex >= BED_MESH_X_PT || yIndex >= BED_MESH_Y_PT), false);
  bedMesh[xIndex + 1][yIndex + 1] = t;
  return true;
}

/**
 * @brief 更新并扩充调平数据矩阵测量点外的区域点
 */
void updataBedMesh(void)
{
  //1、扩展x轴左右两侧估计点的x、y坐标
  for (int xi = 1; xi < BED_MESH_X_PT + 1; xi++)
  {
    bedMesh[xi][0].x = bedMesh[xi][1].x;
    bedMesh[xi][BED_MESH_Y_PT + 1].x = bedMesh[xi][BED_MESH_Y_PT].x;

    bedMesh[xi][0].y = 2 * bedMesh[xi][1].y - bedMesh[xi][2].y;
    bedMesh[xi][BED_MESH_Y_PT + 1].y = 2 * bedMesh[xi][BED_MESH_Y_PT].y - bedMesh[xi][BED_MESH_Y_PT - 1].y;
  }
  //2、扩展y轴上下两侧估计点的x、y坐标
  for (int yj = 0; yj < BED_MESH_Y_PT + 2; yj++)
  {
    bedMesh[0][yj].x = 2 * bedMesh[1][yj].x - bedMesh[2][yj].x;
    bedMesh[BED_MESH_X_PT + 1][yj].x = 2 * bedMesh[BED_MESH_X_PT][yj].x - bedMesh[BED_MESH_X_PT - 1][yj].x;

    bedMesh[0][yj].y = bedMesh[1][yj].y;
    bedMesh[BED_MESH_X_PT + 1][yj].y = bedMesh[BED_MESH_X_PT][yj].y;
  }

  //3、拟合计算x轴左右两侧估计点的z坐标
  for (int xi = 1; xi < BED_MESH_X_PT + 1; xi++)
  {
    bedMesh[xi][0].z = getLinear2(&bedMesh[xi][2], &bedMesh[xi][1], &bedMesh[xi][0], 0);
    bedMesh[xi][BED_MESH_Y_PT + 1].z = getLinear2(&bedMesh[xi][BED_MESH_Y_PT - 1], &bedMesh[xi][BED_MESH_Y_PT], &bedMesh[xi][BED_MESH_Y_PT + 1], 0);
  }

  //4、拟合计算y轴上下两侧估计点的z坐标
  for (int yj = 1; yj < BED_MESH_Y_PT + 1; yj++)
  {
    bedMesh[0][yj].z = getLinear2(&bedMesh[2][yj], &bedMesh[1][yj], &bedMesh[0][yj], 1);
    bedMesh[BED_MESH_X_PT + 1][yj].z = getLinear2(&bedMesh[BED_MESH_X_PT - 1][yj], &bedMesh[BED_MESH_X_PT][yj], &bedMesh[BED_MESH_X_PT + 1][yj], 1);
  }

  //5、拟合计算四个角的z坐标
  bedMesh[0][0].z = getLinear2(&bedMesh[2][2], &bedMesh[1][1], &bedMesh[0][0], 0);
  bedMesh[0][BED_MESH_Y_PT + 1].z = getLinear2(&bedMesh[2][BED_MESH_Y_PT - 1], &bedMesh[1][BED_MESH_Y_PT], &bedMesh[0][BED_MESH_Y_PT + 1], 0);
  bedMesh[BED_MESH_X_PT + 1][0].z = getLinear2(&bedMesh[BED_MESH_X_PT - 1][2], &bedMesh[BED_MESH_X_PT][1], &bedMesh[BED_MESH_X_PT + 1][0], 0);
  bedMesh[BED_MESH_X_PT + 1][BED_MESH_Y_PT + 1].z = getLinear2(&bedMesh[BED_MESH_X_PT - 1][BED_MESH_Y_PT - 1], &bedMesh[BED_MESH_X_PT][BED_MESH_Y_PT], &bedMesh[BED_MESH_X_PT + 1][BED_MESH_Y_PT + 1], 0);
}

/**
 * @brief 热床平整度检查，同时对问题点进行重新测量
 */
void checkAndReProbe(void)
{
  float bzs[] = {g29BasePos[0].z, g29BasePos[1].z, g29BasePos[2].z, g29BasePos[3].z};
  float minBz = 0, maxBz = 0;

  //1、找出准备点的最大最小z值
  ARY_MIN(minBz, bzs, 4);
  ARY_MAX(maxBz, bzs, 4);
  //2、计算热床当前的最大倾角（未测量过准备点，则取产品允许热床最大高低差）
  float norm = (BED_SIZE_X_MM < BED_SIZE_Y_MM ? BED_SIZE_X_MM : BED_SIZE_Y_MM);
  float heigh = (IS_ZERO(minBz) && IS_ZERO(maxBz)) ? (2 * BED_MAX_ERR) : (maxBz - minBz);
  float maxArcTan =  (heigh < 6 ? 6 : heigh) / norm;
  maxArcTan = (maxArcTan <= (4.0f / BED_SIZE_X_MM) ? (4.0f / BED_SIZE_X_MM) : maxArcTan); 

  if(g29ReprobeCount >= MAX_REPROBE_CNT)   // 如果重复测量点过多，表明喷头可能粘有耗材，此时要进行喷头擦拭后再重新测量
  {
    maxArcTan = 0;
    clearNozzle(pl_minHotendTemp, pl_maxHotendTemp + 20, pl_bedTemp, PA_FIL_LEN_MM);
  }

  PRINTF("\n***CHECK POINTS: minZ=%s, maxZ=%s, norm=%s, heigh=%s, maxArcTan=%s***\n", getStr(minBz), getStr(maxBz), getStr(norm), getStr(heigh), getStr(maxArcTan));
  //3、循环遍历每个点，并与其周围点做比较，以难斜率是否在给定的范围内
  for (int i = 1; i < BED_MESH_X_PT + 1; i++)
  {
    for (int j = 1; j < BED_MESH_X_PT + 1; j++)
    {
      float rd[4] = {0}, xIntval = bedMesh[2][2].x - bedMesh[1][1].x, yIntval = bedMesh[2][2].y - bedMesh[1][1].y; 
      rd[0] = (i <= 1 ? (maxArcTan + 1) : (bedMesh[i - 1][j].z - bedMesh[i][j].z) / xIntval);
      rd[1] = (i >= BED_MESH_X_PT - 1 ? (maxArcTan + 1) : (bedMesh[i + 1][j].z - bedMesh[i][j].z) / xIntval);
      rd[2] = (j <= 0 ? (maxArcTan + 1) : (bedMesh[i][j - 1].z - bedMesh[i][j].z) / yIntval);
      rd[3] = (j >= BED_MESH_Y_PT - 1 ? (maxArcTan + 1) : (bedMesh[i][j + 1].z - bedMesh[i][j].z) / yIntval);
      if ((fabsf(rd[0]) < maxArcTan) || (fabsf(rd[1]) < maxArcTan) || (fabsf(rd[2]) < maxArcTan) || (fabsf(rd[3]) < maxArcTan))
        continue;
      PRINTF("\n***MUST RE PROBE: (z=%s, z0=%s, z1=%s, z2=%s, z3=%s), (rd=%s, rd0=%s, rd1=%s, rd2=%s, rd3=%s)***\n",
        getStr(bedMesh[i][j].z), getStr(bedMesh[i - 1][j].z), getStr(bedMesh[i + 1][j].z), getStr(bedMesh[i][j - 1].z), getStr(bedMesh[i][j + 1].z),
        getStr(maxArcTan), getStr(rd[0]),getStr(rd[1]),getStr(rd[2]),getStr(rd[3]));
      bedMesh[i][j].z = doG29Probe(bedMesh[i][j].x, bedMesh[i][j].y);//重新测量
    }
  }
}

/** 
 * @brief 根据测量及扩充后的数据矩阵，进行补偿值获取
 * @param xPos 待获取位置的x坐标
 * @param yPos 待获取位置的y坐标
 * @return outZ_mm 计算出的Z补偿值
 */
float getMeshZ(float xPos, float yPos)
{
  int xS = 0, yS = 0;

  while (xS <= BED_MESH_X_PT && !(xPos >= bedMesh[xS][0].x && xPos < bedMesh[xS + 1][0].x))
    xS++;

  while (yS <= BED_MESH_Y_PT && !(yPos >= bedMesh[0][yS].y && yPos < bedMesh[0][yS + 1].y))
    yS++;

  if (xS >= BED_MESH_X_PT + 1 || yS >= BED_MESH_Y_PT + 1)
    return 0;

  xyz_float_t pLeft = {xPos, bedMesh[xS][yS].y, 0}, pRight = {xPos, bedMesh[xS][yS + 1].y, 0}, pMid = {xPos, yPos, 0};

  pLeft.z = getLinear2(&bedMesh[xS][yS], &bedMesh[xS + 1][yS], &pLeft, 1);
  pRight.z = getLinear2(&bedMesh[xS][yS + 1], &bedMesh[xS + 1][yS + 1], &pRight, 1);
  pMid.z = getLinear2(&pLeft, &pRight, &pMid, 0);

  #if 0 //SHOW_MSG  //Z轴补偿值的获取比较频繁，调试完成后应该关闭此处，以免影响打印质量。
    PRINTF("\nMeshCell: Point = (%s, %s, %s)\n", getStr(pMid.x), getStr(pMid.y), getStr(pMid.z));
    /*
    memset(msg, 0, sizeof(msg));
    sprintf(msg, "pLeft=(%.2f, %.2f, %.2f)\tpRight=(%.2f, %.2f, %.2f)\n", pLeft.x, pLeft.y, pLeft.z, pRight.x, pRight.y, pRight.z);
    serialprintPGM(msg);

    memset(msg, 0, sizeof(msg));
    sprintf(msg, "(%d, %d)=(%.2f, %.2f, %.2f)\t(%d, %d)=(%.2f, %.2f, %.2f)\n", xS, yS, bedMesh[xS][yS].x, bedMesh[xS][yS].y, bedMesh[xS][yS].z, xS, yS + 1, bedMesh[xS][yS + 1].x, bedMesh[xS][yS + 1].y, bedMesh[xS][yS + 1].z);
    serialprintPGM(msg);

    memset(msg, 0, sizeof(msg));
    sprintf(msg, "(%d, %d)=(%.2f, %.2f, %.2f)\t(%d, %d)=(%.2f, %.2f, %.2f)\n", xS + 1, yS, bedMesh[xS + 1][yS].x, bedMesh[xS + 1][yS].y, bedMesh[xS + 1][yS].z, xS + 1, yS + 1, bedMesh[xS + 1][yS + 1].x, bedMesh[xS + 1][yS + 1].y, bedMesh[xS + 1][yS + 1].z);
    serialprintPGM(msg);

    serialprintPGM("\n");
    */
  #endif
  return pMid.z;
}

/**
 * @brief 使用压力传感器测量并获取给定点的热床高度
 * @param xPos 需要获取点的x坐标
 * @param yPos 需要获取点的y坐标
 * @attention 先执行probeReady()，有利于减小调平时间，调平点数越多，效果越明显
 * @return 测量完成后，输出的结果
 */
float doG29Probe(float xPos, float yPos)
{
  ProbeAcq pa;

  pa.hx711.init(HX711_SCK_PIN, HX711_SDO_PIN);
  pa.rcFilter.init(RC_CUTE_FRQ, 80, 0);

  pa.minZ_mm = -20;
  pa.basePos_mm.x = xPos + (random(0, 3) - 1);
  pa.basePos_mm.y = yPos;
  pa.basePos_mm.z = getBestProbeZ(xPos, yPos);
  pa.basePos_mm.z = (IS_ZERO(pa.basePos_mm.z) ? (BED_MAX_ERR + 1) : pa.basePos_mm.z);
  pa.baseSpdXY_mm_s = 200;
  pa.baseSpdZ_mm_s = 5;
  pa.step_mm = 0.0125;    // 测量精度
  pa.isUpAfter = true;
  pa.isShowMsg = false;
  pa.minHold = MIN_HOLD;
  pa.maxHold = MAX_HOLD;

  // CHECK_AND_RETURN((!(pa.checHx711() || pa.checHx711())), 0); // 检测模块工作是否正常

  int i = 0;
  for (i = 0; i < 5; i++)
  {
    pa.probePointByStep()->outIndex;
    CHECK_AND_BREAK(CHECK_RANGE(pa.outIndex, 3 * PI_COUNT / 4, PI_COUNT - 1));  //验证检测结果是否正确
    pa.basePos_mm.z = getBestProbeZ(xPos, yPos);                                //检测失败，则将开始测量时的Z轴高度抬升一段
    g29ReprobeCount++;
  }
  PRINTF("\n***PROBE BY PRESS: z=%s***\n", getStr(pa.outVal_mm));
  return ((i >= 3) ? 0 : pa.outVal_mm);
}

/**
 * @brief 使用压力传感器获取Z轴HOME点
 * @param isPrecision true:为精确测量（比如喷头擦拭后）；false:为粗略测量（比如喷头擦拭前）；
 */
void doG28ZProbe(bool isPrecision)
{
  ProbeAcq pa;

  pa.hx711.init(HX711_SCK_PIN, HX711_SDO_PIN);
  pa.rcFilter.init(RC_CUTE_FRQ, 80, 0);

  pa.minZ_mm = -BED_SIZE_Y_MM * 1.5;
  // pa.basePos_mm.x = BED_SIZE_X_MM / 2 + (isPrecision ? 0 : 5);   // 如果不是精确测量，为防止喷头上的耗材污染中心点，这里给于微小偏移
  // pa.basePos_mm.y = BED_SIZE_Y_MM / 2 + (isPrecision ? 0 : 5);
  pa.basePos_mm.x = AUTOZ_TOOL_X + (isPrecision ? 0 : 5);   // 如果不是精确测量，为防止喷头上的耗材污染中心点，这里给于微小偏移
  pa.basePos_mm.y = AUTOZ_TOOL_Y + (isPrecision ? 0 : 5);
  pa.basePos_mm.z = GET_CURRENT_POS().z;
  pa.baseSpdXY_mm_s = 200;
  pa.baseSpdZ_mm_s = 5;
  pa.step_mm = 0.03;
  pa.isUpAfter = false;
  pa.isShowMsg = false;
  pa.minHold = (isPrecision ? MIN_HOLD : (MAX_HOLD / 5));
  pa.maxHold = MAX_HOLD;

  //CHECK_AND_RUN((!(pa.checHx711() || pa.checHx711())), PRINTF("%s\n", "***G28 HX711 CHECK FAILE!!!***")); //检测模块工作是否正常 

  int nowVal = 0, unFitVal = pa.readBase().y;
  do{
    pa.probePointByStep();
    nowVal = pa.readBase().y;
    SET_NOW_POS_Z_IS_HOME();
    pa.basePos_mm.z = GET_CURRENT_POS().z + 3;
  } while (abs(nowVal - unFitVal) < (pa.minHold / 2));    //使用压力大小来验证喷头是否压到热床，此方法可以排除干扰影响的误触发

  if (isPrecision)                                        //如果是高精度测量
  {
    int m = 0;
    float tmp1 = 0, tmp2 = 0;
    pa.minHold = MIN_HOLD;
    pa.step_mm = 0.01;                                    //精度修改成0.01mm
    pa.isUpAfter = true;                                  //测量完后抬升Z轴
    pa.basePos_mm.z = 2;                                  //开始测量时的高度为2mm
    do{
      pa.basePos_mm.x = BED_SIZE_X_MM / 2  + (random(0, 5) - 2);//如果不是精确测量，为防止喷头上的耗材污染中心点，这里给于微小偏移
      pa.basePos_mm.y = BED_SIZE_Y_MM / 2  + (random(0, 5) - 2);
      tmp1 = pa.probePointByStep()->outVal_mm;
      tmp2 = pa.probePointByStep()->outVal_mm;
    } while (m++ < 2 && (fabsf(tmp1 - tmp2) >= 0.05));     //测量两次，并对比两者之差

    DO_BLOCKING_MOVE_TO_Z((tmp1 + tmp2) / 2, pa.baseSpdZ_mm_s);//设定HOME点
    SET_NOW_POS_Z_IS_HOME();
  }

  DO_BLOCKING_MOVE_TO_XY(Z_SAFE_HOMING_X_POINT, Z_SAFE_HOMING_Y_POINT, pa.baseSpdXY_mm_s);
  DO_BLOCKING_MOVE_TO_Z(5, 10);
}

/**
 * @brief 用于相关内容验证，常用于功能调试
 */
void doG212ExCmd(void)
{
  float cnt = GET_PARSER_SEEN('C') ? GET_PARSER_INT_VAL() : 1;
  if(GET_PARSER_SEEN('H'))
  {
    HX711 hx711;
    hx711.init(HX711_SCK_PIN, HX711_SDO_PIN);
    FOR_LOOP_TIMES(i, 0, cnt, hx711.getVal(1));
    return;
  }

  if(GET_PARSER_SEEN('T'))
  {
    ProbeAcq pa;
    pa.hx711.init(HX711_SCK_PIN, HX711_SDO_PIN);
    pa.rcFilter.init(RC_CUTE_FRQ, 80, 0);
    pa.minZ_mm = -BED_SIZE_Y_MM * 1.5;
    pa.basePos_mm.x = GET_CURRENT_POS().x;
    pa.basePos_mm.y = GET_CURRENT_POS().y;
    pa.basePos_mm.z = GET_CURRENT_POS().z;
    pa.baseSpdXY_mm_s = 100;
    pa.baseSpdZ_mm_s = 5;
    pa.step_mm = 0.0125;
    pa.isUpAfter = true;
    pa.isShowMsg = true;
    pa.minHold = MIN_HOLD;
    pa.maxHold = MAX_HOLD;
    pa.probePointByStep();
    return;
  }

  if(GET_PARSER_SEEN('M'))
  {
    printBedMesh();
  }

  if(GET_PARSER_SEEN('L'))
  {
    clearNozzle(120, 180, 60, 0);
  }


  if(GET_PARSER_SEEN('E'))
  {
    clearNozzle(120, 200, 60, PA_FIL_LEN_MM);
  }
}
#endif