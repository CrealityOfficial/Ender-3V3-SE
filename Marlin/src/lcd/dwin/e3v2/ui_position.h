
#ifndef UI_POSITION_H
#define UI_POSITION_H

#if ENABLED(DWIN_CREALITY_480_LCD) //

#elif ENABLED(DWIN_CREALITY_320_LCD)//3.2寸屏幕
//主界面
    #define LOGO_LITTLE_X  72  //小LOGO坐标
    #define LOGO_LITTLE_Y  36
//自动调平界面
//编辑调平页面
    #define WORD_TITLE_X 29 
    #define WORD_TITLE_Y 1
    #define OUTLINE_LEFT_X 12//40
    #define OUTLINE_LEFT_Y 30//72
    #define OUTLINE_RIGHT_X OUTLINE_LEFT_X+220//OUTLINE_LEFT_X+200//OUTLINE_LEFT_X+160
    #define OUTLINE_RIGHT_Y OUTLINE_LEFT_Y+220//OUTLINE_LEFT_Y+160
    //button——position
    #define BUTTON_W 82
    #define BUTTON_H 32
    #define BUTTON_EDIT_X OUTLINE_LEFT_X
    #define BUTTON_EDIT_Y OUTLINE_RIGHT_Y+20//OUTLINE_RIGHT_Y+27
    #define BUTTON_OK_X  OUTLINE_RIGHT_X-BUTTON_W
    #define BUTTON_OK_Y  BUTTON_EDIT_Y//
    //数据坐标
    #define X_Axis_Interval  50//54   //x轴上间隔距离像素
    #define Y_Axis_Interval  52//35   //y轴上间隔距离像素
    #define Rect_LU_X_POS    OUTLINE_LEFT_X+10//32   //第一个左上x坐标
    // #define Rect_LU_Y_POS    OUTLINE_LEFT_Y+10//157-4  //第一个左上y坐标
    #define Rect_LU_Y_POS    (OUTLINE_LEFT_Y+20)+3*Y_Axis_Interval//157-4  //第一个左上y坐标

    #define Rect_RD_X_POS    Rect_LU_X_POS+45//X_Axis_Interval//X_Axis_Interval//78   //第一个右下x坐标
    // #define Rect_RD_Y_POS    Rect_LU_Y_POS+20//Y_Axis_Interval//Y_Axis_Interval//177-4  //第一个右下y坐标
    #define Rect_RD_Y_POS    (Rect_LU_Y_POS+30)//Y_Axis_Interval//Y_Axis_Interval//177-4  //第一个右下y坐标
	
#define TITLE_X 29
#define TITLE_Y  1
//  TITLE_X, TITLE_Y);
//启动页面
#define CREALITY_LOGO_X   20
#define CREALITY_LOGO_Y   96
//主界面
    #define LOGO_LITTLE_X  72  //小LOGO坐标
    #define LOGO_LITTLE_Y  36

    #define ICON_PRINT_X 12
    #define ICON_PRINT_Y 51
    #define ICON_W 102-1
    #define ICON_H 115-1

    #define ICON_PREPARE_X 126
    #define ICON_PREPARE_Y 51
    #define ICON_CONTROL_X 12
    #define ICON_CONTROL_Y 178
    #define ICON_LEVEL_X 126
    #define ICON_LEVEL_Y 178

    #define WORD_PRINT_X 12+1
    #define WORD_PRINT_Y 120
    #define WORD_PREPARE_X 126+1
    #define WORD_PREPARE_Y 120
    #define WORD_CONTROL_X 12+1
    #define WORD_CONTROL_Y 247
    #define WORD_LEVEL_X 126+1
    #define WORD_LEVEL_Y 247

    //打印确认页面
    #define ICON_Defaut_Image_X 72
    #define ICON_Defaut_Image_Y 25

    //控制-》信息页面
    #define ICON_SIZE_X  20  // 
    #define ICON_SIZE_Y  75  //39
    // #define ICON_SIZE_X  20  // 
    // #define ICON_SIZE_Y  79  //39
    //语言选择页面
    #define WORD_LAGUAGE_X 20


    //打印中界面
    #define FIEL_NAME_X   12 
     #define FIEL_NAME_Y  24+11 
    #define ICON_PRINT_TIME_X 117
    #define ICON_PRINT_TIME_Y 77
    #define WORD_PRINT_TIME_X 141 
    #define WORD_PRINT_TIME_Y 61 
    #define NUM_PRINT_TIME_X  WORD_PRINT_TIME_X
    #define NUM_PRINT_TIME_Y  92

    #define ICON_RAMAIN_TIME_X ICON_PRINT_TIME_X
    #define ICON_RAMAIN_TIME_Y ICON_PRINT_TIME_Y+37+24
    #define WORD_RAMAIN_TIME_X WORD_PRINT_TIME_X
    #define WORD_RAMAIN_TIME_Y WORD_PRINT_TIME_Y+31+30
    #define NUM_RAMAIN_TIME_X  NUM_PRINT_TIME_X
    #define NUM_RAMAIN_TIME_Y  NUM_PRINT_TIME_Y+40+21

    //划线1,2,3
    #define LINE_X_START  12
    #define LINE_Y_START  59            
    #define LINE_X_END    12+216
    #define LINE_Y_END    LINE_Y_START+1
    #define LINE2_SPACE    122
    #define LINE3_SPACE    LINE2_SPACE+108
    // //划线3.4
    // #define LINE3_X_START  94
    // #define LINE3_Y_START  94            
    // #define LINE3_X_END    LINE3_X_START+134
    // #define LINE3_Y_END    LINE3_Y_START+1
    // #define LINE3_SPACE    27

    

    #define ICON_PERCENT_X   12
    #define ICON_PERCENT_Y   75
     //打印参数图标坐标
    #define ICON_NOZZ_X  6//ICON_PERCENT_X  //喷嘴图标
    #define ICON_NOZZ_Y  268.5
    #define ICON_BED_X   ICON_NOZZ_X  //热床图标
    #define ICON_BED_Y   ICON_NOZZ_Y+6+20
    #define ICON_SPEED_X 99//ICON_NOZZ_X+64+20  //打印速率
    #define ICON_SPEED_Y  ICON_NOZZ_Y
    #define ICON_STEP_E_X ICON_SPEED_X//ICON_BED_X+64+20   //E轴挤出比
    #define ICON_STEP_E_Y ICON_BED_Y
    #define ICON_ZOFFSET_X 171//ICON_STEP_E_X+64+20//z轴补偿值
    #define ICON_ZOFFSET_Y ICON_STEP_E_Y
    #define ICON_FAN_X     171//ICON_SPEED_X+64+20//风扇占空比
    #define ICON_FAN_Y     ICON_SPEED_Y
//打印参数数据坐标
    #define ICON_TO_NUM_OFFSET 4
    #define NUM_NOZZ_X     ICON_NOZZ_X+18  //(32,388)
    #define NUM_NOZZ_Y     ICON_NOZZ_Y+ICON_TO_NUM_OFFSET
    #define NUM_BED_X      ICON_BED_X+18   //(32,388)
    #define NUM_BED_Y      ICON_BED_Y+ICON_TO_NUM_OFFSET
    #define NUM_SPEED_X    ICON_SPEED_X+20
    #define NUM_SPEED_Y    ICON_SPEED_Y+ICON_TO_NUM_OFFSET
    #define NUM_STEP_E_X   ICON_STEP_E_X+20
    #define NUM_STEP_E_Y   ICON_STEP_E_Y+ICON_TO_NUM_OFFSET
    #define NUM_ZOFFSET_X  ICON_ZOFFSET_X+18
    #define NUM_ZOFFSET_Y  ICON_ZOFFSET_Y+ICON_TO_NUM_OFFSET
    #define NUM_FAN_X      ICON_FAN_X+18
    #define NUM_FAN_Y      ICON_FAN_Y+ICON_TO_NUM_OFFSET
    
    // #define NUM_PRECENT_X  ICON_PERCENT_X+21
    // #define NUM_PRECENT_Y  ICON_PERCENT_Y+26
    // #define PRECENT_X  NUM_PRECENT_X+20
    // #define PRECENT_Y  NUM_PRECENT_Y

    //图标坐标
    #define RECT_SET_X1  12
    #define RECT_SET_Y1  191
    #define RECT_SET_X2  RECT_SET_X1+68
    #define RECT_SET_Y2  RECT_SET_Y1+64
    #define RECT_OFFSET_X   6+68

    #define ICON_SET_X  RECT_SET_X1//RECT_SET_X1+19  
    #define ICON_SET_Y  RECT_SET_Y1//RECT_SET_Y1+5
    #define WORD_SET_X  RECT_SET_X1  
    #define WORD_SET_Y  RECT_SET_Y1+34

    #define ICON_PAUSE_X  ICON_SET_X+RECT_OFFSET_X  
    #define ICON_PAUSE_Y  ICON_SET_Y
    #define WORD_PAUSE_X  ICON_PAUSE_X //WORD_SET_X+RECT_OFFSET_X   
    #define WORD_PAUSE_Y  WORD_SET_Y

    #define ICON_STOP_X  ICON_PAUSE_X+RECT_OFFSET_X  
    #define ICON_STOP_Y  ICON_PAUSE_Y
    #define WORD_STOP_X  ICON_STOP_X //WORD_PAUSE_X+RECT_OFFSET_X  
    #define WORD_STOP_Y  WORD_PAUSE_Y

  

//一键对高界面坐标
    #define LINE_X  111
    #define LINE_Y  117
    #define LINE_Y_SPACE 81+18

    #define WORD_NOZZ_HOT_X 24
    #define WORD_NOZZ_HOT_Y 72
    #define WORD_Y_SPACE    54+45

    #define ICON_NOZZ_HOT_X 102
    #define ICON_NOZZ_HOT_Y 36
    #define ICON_Y_SPACE    63+36
//调平失败UI坐标
#define ICON_LEVELING_ERR_X  90
#define ICON_LEVELING_ERR_Y  79
#define WORD_LEVELING_ERR_X  16
#define WORD_LEVELING_ERR_Y  148

//打印完成清除底部范围
#define CLEAR_50_X    0
#define CLEAR_50_Y    ICON_SET_Y-1
#define OK_BUTTON_X   72  //打印完成按钮位置
#define OK_BUTTON_Y   264

//开机引导，语言选择
#define FIRST_X  20
#define FIRST_Y  29
#define LINE_START_X 12
#define LINE_START_Y 39+24
#define LINE_END_X   LINE_START_X+215
#define LINE_END_Y   LINE_START_Y+1
#define WORD_INTERVAL  40
#define LINE_INTERVAL  40
#define REC_X1   0
#define REC_Y1   25
#define REC_X2   10
#define REC_Y2   REC_Y1+37
#define REC_INTERVAL 40

//异常界面坐标
#define ICON_X 90
#define ICON_Y 53
#define WORD_X 16 
#define WORD_Y ICON_X+9+60
#define REC_60_X1 16
#define REC_60_Y1 41
#define REC_60_X2 REC_X1+208
#define REC_60_Y2 REC_Y1+210

//PID设置界面
#define WORD_AUTO_X 12
#define WORD_AUTO_Y  45+24
#define WORD_TEMP_X 84
#define WORD_TEMP_Y 101+24
#define ICON_AUTO_X 0
#define ICON_AUTO_Y 101+24
#define SHOW_CURE_X1 34
#define SHOW_CURE_Y1 167 //ICON_AUTO_Y-28  
#define SHOW_CURE_X2 SHOW_CURE_X1+194//SHOW_CURE_X1+191
#define SHOW_CURE_Y2 236//SHOW_CURE_Y1+71

// 曲线数据相关
#define CURVE_POINT_NUM       40
#define Curve_Color_Bed       0xFF6B17 // 浅红色
#define Curve_Color_Nozzle    0xFF0E42 // 红色
#define Curve_Psition_Start_X   SHOW_CURE_X1//34     // 0x0000 //左上
#define Curve_Psition_Start_Y   SHOW_CURE_Y1//144    // 0x0000
#define Curve_Psition_End_X     SHOW_CURE_X2//225    // 0x0000 //右下
#define Curve_Psition_End_Y     SHOW_CURE_Y2//221-6    // 0x0000 //右下
#define Curve_DISTANCE_BED_Y    70
#define Curve_DISTANCE_NOZZLE_Y 70
#define Curve_Line_Width        0x01
#define Curve_Step_Size_X       5      // 0x05
#define Curve_Bed_Size_Y       Curve_DISTANCE_BED_Y*256/150  // 90 //Y轴数据像素间隔   y数据比例
#define Curve_Nozzle_Size_Y    Curve_DISTANCE_NOZZLE_Y*256/300
#define Curve_Zero_Y           Curve_Psition_End_Y+2                                 // 0x0556  Y轴0点坐标

//图片预览界面图标位置

#if ENABLED(USER_LEVEL_CHECK)  //使用调平校准功能
    #define WORD_TIME_X  12  
    #define WORD_TIME_Y  122
    #define WORD_LENTH_X WORD_TIME_X
    #define WORD_LENTH_Y WORD_TIME_Y+21
    #define WORD_HIGH_X  WORD_TIME_X
    #define WORD_HIGH_Y  WORD_LENTH_Y+21
    #define WORD_PRINT_CHECK_X WORD_TIME_X
    #define WORD_PRINT_CHECK_Y WORD_HIGH_Y+21+14
    
    #define ICON_ON_OFF_X  WORD_PRINT_CHECK_X+160+20
    #define ICON_ON_OFF_Y  WORD_PRINT_CHECK_Y

    #define BUTTON_X  12
    #define BUTTON_Y  230
    #define BUTTON_OFFSET_X BUTTON_X+82+52-10 
#else
    #define WORD_TIME_X  12  
    #define WORD_TIME_Y  134
    #define WORD_LENTH_X WORD_TIME_X
    #define WORD_LENTH_Y WORD_TIME_Y+21+4
    #define WORD_HIGH_X  WORD_TIME_X
    #define WORD_HIGH_Y  WORD_LENTH_Y+21+4

    #define BUTTON_X  12
    #define BUTTON_Y  225
    #define BUTTON_OFFSET_X BUTTON_X+82+52-10 
#endif
#define DATA_OFFSET_X  103+60
#define DATA_OFFSET_Y  4


//开机引导弹窗
#define POPUP_BG_X_LU  16
#define POPUP_BG_Y_LU  67
#define POPUP_BG_X_RD  224
#define POPUP_BG_Y_RD  277
//清洁提示
#define WORD_HINT_CLEAR_X POPUP_BG_X_LU
#define WORD_HINT_CLEAR_Y 17+POPUP_BG_Y_LU
//失败后清洁提示
#define WORD_HIGH_CLEAR_X POPUP_BG_X_LU
#define WORD_HIGH_CLEAR_Y 81+POPUP_BG_Y_LU
#define ICON_HIGH_ERR_X   70+POPUP_BG_X_LU
#define ICON_HIGH_ERR_Y   12+POPUP_BG_Y_LU
//调平失败
#define ICON_LEVEL_ERR_X  42+POPUP_BG_X_LU
#define ICON_LEVEL_ERR_Y  11+POPUP_BG_Y_LU
#define WORD_SCAN_QR_X    POPUP_BG_X_LU
#define WORD_SCAN_QR_Y    146+POPUP_BG_Y_LU

#define BUTTON_BOOT_X  POPUP_BG_X_LU+63
#define BUTTON_BOOT_Y  POPUP_BG_Y_LU+164
#define BUTTON_BOOT_LEVEL_X  79
#define BUTTON_BOOT_LEVEL_Y  260


//G29各个点的坐标
#define G29_X_MIN 3
#define G29_Y_MIN 3
#define G29_X_MAX 200.75
#define G29_Y_MAX 212
#define G29_X_INTERVAL ((G29_X_MAX-G29_X_MIN)/(GRID_MAX_POINTS_X-1))
#define G29_Y_INTERVAL ((G29_Y_MAX-G29_Y_MIN)/(GRID_MAX_POINTS_Y-1))


#endif 

#endif   //E3S1_LASER_H