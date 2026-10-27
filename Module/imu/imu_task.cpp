#include "Alg_PID.hpp"
#include "BMI088driver.hpp"
#include <cstdint>
#include "main.h"
#include "ecf_configList.h"

extern "C"
{
    #include "imu_task.h"
    #include "FreeRTOS.h"
    #include "task.h"
    #include "spi.h"
    #include "tim.h"
    #include "cmsis_os.h"
    #include "usart.h"
    #include <cstdio>
}

const INS_t* INS = nullptr;
BMI088Heat_c *imu = nullptr;
/*
 * @brief 返回陀螺仪类
 */
BMI088Heat_c *getImuPtr()
{
    return imu;
}

void IMU_Task(void const * argument)
{
    imu_Init();

    TickType_t xLastWakeTime;
    const TickType_t xDelay1ms = pdMS_TO_TICKS( 1 );
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        taskENTER_CRITICAL()   ;
        BMI088_c::BMI_UpData();
        taskEXIT_CRITICAL() ;
        vTaskDelayUntil( &xLastWakeTime, xDelay1ms );
    }
}

void IMU_Heat_Task(void const * argument)
{

    imu->timInit();
    imu->preHeat();
    while(1)
    {
        imu->heatLoop();
        vTaskDelay(10);
    }
}

void imu_Init()
{
    alg_n::PidInitConfig_t pid_config =
            {
                    .Kp = Heater_Kp,
                    .Ki = Heater_Ki,
                    .Kd = Heater_Kd,
                    .mode = Output_Limit | Integral_Limit | OutputFilter ,
                    .max_out = Heater_maxout,
                    .max_Ierror = Heater_maxIerror,
                    .out_filter_num = 0.1f
            };
    IMU_Heat_Config_s_t _configP =
            {
                    IMU_HEATER_TIM,
                    IMU_HEATER_TIM_CHANNEL,
                    Max_temp,
                    Init_temp,
                    Target_temp,
                    Preheat_Timeout,
                    Heater_maxout,
                    Init_Heater_Duty,
                    TEMP_UPDATE_TIMEOUT_MS,
                    &pid_config,
                    Heater_forward
            };
    imu = new BMI088Heat_c(&_configP);
    //配置imu接口
    imu->physicalInit(BMI088_SPI, isCalibrate, CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, CS2_GYRO_GPIO_Port, CS2_GYRO_Pin);
    
    //加热相关，判断是否拥有加热器
    #if isHeaterEnabled
    //创建加热任务
    osThreadId imuHeatTask;
    osThreadDef(imuHeatTask, IMU_Heat_Task, osPriorityNormal, 0, 128);
    imuHeatTask = osThreadCreate(osThread(imuHeatTask), NULL);
    //等待加热完成
    while(imu->getHeatingStatus() != IMU_Heat_OK)
    {
        IMU_Heat_Status_enum_t status = imu->getHeatingStatus();
        if (status == IMU_Heat_PreHeatFail || status == IMU_Heat_OverTemp || status == IMU_Heat_TempNotUpdate)
        {
            break;
        }
        osDelay(10);
    }
    
    #endif
    imu->CalibrateMpuOffset();//标定零飘，初始化卡尔曼
    imu->init_flag = 1;
    INS = imu->Get_INS_Data_Point();
}

