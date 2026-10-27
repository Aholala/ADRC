#ifndef IMU_TASK_H
#define IMU_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "BMI088driver.hpp"

#ifdef __cplusplus
void imu_Init();
extern "C" {
#endif
BMI088Heat_c *getImuPtr();
void IMU_Task(void const * argument);
void IMU_Heat_Task(void const * argument);

#ifdef __cplusplus
}
#endif



#endif // IMU_TASK_H
