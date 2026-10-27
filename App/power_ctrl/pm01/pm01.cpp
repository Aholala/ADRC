#include "pm01.hpp"
extern CAN_HandleTypeDef hcan1;
#define pm01can hcan1

//
// Created by Lann on 2025/3/1.
//


void pm01_connect(){Superpower_Calc::getinstance()->Pm01_is_connect = 1;}
void pm01_disconnect(){Superpower_Calc::getinstance()->Pm01_is_connect = 0;}

/*
说明：
pm01控制功率是*100倍描述
机器人缓冲能量为60J
飞坡增益：缓冲能量提升至250J
*/
float energy_buff = 0.0f; // 可作为裁判系统的实时功率

/**
  *@brif 超点电压信息接收回调函数
  *@note 在Chassis_Init中调用ECF_CAN_Rx_Callback_Register注册
  */
void SuperPower_V_responed(BSP_CAN_Part_n::CANInstance_c *register_instance)
{
	Superpower_Calc::getinstance()->pm01_safe_task->Online();
	
    uint8_t *rxbuff;
    if (register_instance->rx_id_ != 0x612)
    {
        return;// 不符合电机返回ID的数据不进行处理
    }
    uint16_t m_tmp;
    rxbuff = register_instance->rx_buff;
    m_tmp = (uint16_t)rxbuff[0] << 8 | rxbuff[1];
    Superpower_Calc::getinstance()->pm01_od.p_out = m_tmp;

    m_tmp = (uint16_t)rxbuff[2] << 8 | rxbuff[3];
    Superpower_Calc::getinstance()->pm01_od.v_out = m_tmp;

    m_tmp = (uint16_t)rxbuff[4] << 8 | rxbuff[5];
    Superpower_Calc::getinstance()->pm01_od.i_out = m_tmp;
}

void SuperPower_I_responed(BSP_CAN_Part_n::CANInstance_c* CAN_Rxmessage)
{
	// 0.01s 读取一次超电的电压、电流、功率等信息
	uint16_t m_tmp; 
	// 返回的数据如果不是0x611则不处理（读取超电—>电压、电流、功率等信息）
	if (CAN_Rxmessage->rx_id_ != 0x611)
	{
		return; // 不符合电机返回ID的数据不进行处理
	}
	m_tmp = (uint16_t)CAN_Rxmessage->rx_buff[0] << 8 | CAN_Rxmessage->rx_buff[1];	
	Superpower_Calc::getinstance()->pm01_od.p_in = m_tmp;

	m_tmp = (uint16_t)CAN_Rxmessage->rx_buff[2] << 8 | CAN_Rxmessage->rx_buff[3];	
	Superpower_Calc::getinstance()->pm01_od.v_in = m_tmp;	

	m_tmp = (uint16_t)CAN_Rxmessage->rx_buff[4] << 8 | CAN_Rxmessage->rx_buff[5];	
	Superpower_Calc::getinstance()->pm01_od.i_in = m_tmp;
	// 能量积累 0.01s一次 外加单位换算要乘以0.01 超电读出来普遍少了2.5W的功率 
	Superpower_Calc::getinstance()->true_energy += ((Superpower_Calc::getinstance()->pm01_od.p_in + 250) * 0.01f * 0.01f);
	energy_buff = (Superpower_Calc::getinstance()->pm01_od.p_in + 250) * 0.01f;
}

void Superpower_Calc::Init()
{
	can_com = new BSP_CAN_Part_n::CANInstance_c(&pm01can, 0x600, 0x612, CAN_ID_STD, nullptr);
	can_com->ECF_SetRxCallBack(SuperPower_V_responed);

	can_com_I = new BSP_CAN_Part_n::CANInstance_c(&pm01can, 0x600, 0x611, CAN_ID_STD, nullptr);
	can_com_I->ECF_SetRxCallBack(SuperPower_I_responed);

	pm01_safe_task = new Safe_task_c("pm01_safe_task",1000,pm01_connect,pm01_disconnect);
	
}

/**
 * @brief 获取底盘轮子速度指针
 * @param lf_
 * @param lb_
 * @param rf_
 * @param rb_
 */
void Superpower_Calc::Get_Speed(float *lf_, float *lb_, float *rf_, float *rb_)
{
	lf.feedback_speed = lf_;
	lb.feedback_speed = lb_;
	rf.feedback_speed = rf_;
	rb.feedback_speed = rb_;
}

/**
 * @brief 获取速度环pid输出指针
 * @param lf_
 * @param lb_
 * @param rf_
 * @param rb_
 */
void Superpower_Calc::Get_PID_Out(float *lf_, float *lb_, float *rf_, float *rb_)
{
	lf.pid_out = lf_;
	lb.pid_out = lb_;
	rf.pid_out = rf_;
	rb.pid_out = rb_;
}

/**
 * @brief 获取真实电流指针
 * @param lf_
 * @param lb_
 * @param rf_
 * @param rb_
 */
void Superpower_Calc::Get_Real_Current(int16_t *lf_, int16_t *lb_, int16_t *rf_, int16_t *rb_)
{
	lf.feedback_real_current = lf_;
	lb.feedback_real_current = lb_;
	rf.feedback_real_current = rf_;
	rb.feedback_real_current = rb_;
}

/**
  *@brif 主动读取超电输出功率
  */
void Superpower_Calc::Read_SuperPower_Vout(void)
{
    CAN_TxHeaderTypeDef CAN_Txmessage;
    uint8_t CAN1_Tx_Data[8]={0};
    uint32_t send_mail_box;
    CAN_Txmessage.StdId =0x612;
    CAN_Txmessage.IDE = CAN_ID_STD;
    CAN_Txmessage.RTR = CAN_RTR_REMOTE; //远程帧读取数据
    CAN_Txmessage.DLC   = 0x00;

    HAL_CAN_AddTxMessage(&pm01can, &CAN_Txmessage, CAN1_Tx_Data, &send_mail_box); //将一段数据通过 CAN 总线发送
}

void Superpower_Calc::Read_SuperPower_Input(void)
{
    CAN_TxHeaderTypeDef CAN_Txmessage;
    uint8_t CAN1_Tx_Data[8]={0};
    uint32_t send_mail_box;
    CAN_Txmessage.StdId =0x611;
    CAN_Txmessage.IDE = CAN_ID_STD;
    CAN_Txmessage.RTR = CAN_RTR_REMOTE; //远程帧读取数据
    CAN_Txmessage.DLC   = 0x00;

    HAL_CAN_AddTxMessage(&pm01can, &CAN_Txmessage, CAN1_Tx_Data, &send_mail_box); //将一段数据通过 CAN 总线发送
}

/**
  *@brif 计算超电剩余容量
  */
void Superpower_Calc::Calc_SuperPower_energy(void)
{
	float superPower_v;
	superPower_v = ((float) pm01_od.v_out) / 100.0f;
	pm01_od.energy = ((float)(superPower_v-12)/(float)(24-12)) * 100;//13v以下容易触发电调低压保护
}

/**
  *@brif 根据超电剩余容量返回限制输出功率
  */
uint8_t Superpower_Calc::Calc_SuperPower_Out_power_limit(void)
{
	Calc_SuperPower_energy();

	if (pm01_od.energy >= 30)   return 144;//Chassis.Robot_cmd->Power_Mode == 1?160:120;;//160w功率

	if (pm01_od.energy >= 15 && pm01_od.energy < 30)   return chassis_power_limit;//输入功率

	if (pm01_od.energy < 15 && pm01_od.energy >= 0)  return chassis_power_limit + pm01_od.energy - 15;//输入功率缩放

	return  chassis_power_limit-15;
}

void Superpower_Calc::Set_Chassis_Power_Limit(uint8_t power_limit)
{
	chassis_power_limit = power_limit;
}

/**
  @brief 真实物理模型功率限制
  @return 是否需要缩放
  */
uint8_t Superpower_Calc::True_power_limit(uint8_t referee_power_limit)
{
	chassis_power_limit = referee_power_limit;
	Max_Power = Calc_SuperPower_Out_power_limit();

	power_ctrl.rever[0] = 1;
    power_ctrl.rever[1] = 1;
    power_ctrl.rever[2] = 1;
    power_ctrl.rever[3] = 1;

    /*---统一取真实方便运算---*/


    power_ctrl.give_current[RF_] = *rf.pid_out;
    power_ctrl.give_current[RB_] = *rb.pid_out;
    power_ctrl.give_current[LB_] = *lb.pid_out;
    power_ctrl.give_current[LF_] = *lf.pid_out;

    for (int i = RF_; i <= LF_; i++)
    {
        if (power_ctrl.give_current[i] < 0)
        {
            power_ctrl.rever[i] = -1;
            power_ctrl.give_current[i] *= power_ctrl.rever[i];
        }        
    }

    /*---转子转速---*/
    //前走 L正 R负
    power_ctrl.speed[RF_] = *rf.feedback_speed * power_ctrl.rever[RF_];
    power_ctrl.speed[RB_] = *rb.feedback_speed * power_ctrl.rever[RB_];
    power_ctrl.speed[LB_] = *lb.feedback_speed * power_ctrl.rever[LB_];
    power_ctrl.speed[LF_] = *lf.feedback_speed * power_ctrl.rever[LF_];
    /*---当前计算出的力矩电流---*/
    power_ctrl.I_out[RF_] = *rf.feedback_real_current * power_ctrl.rever[RF_];
    power_ctrl.I_out[RB_] = *rb.feedback_real_current * power_ctrl.rever[RB_];
    power_ctrl.I_out[LB_] = *lb.feedback_real_current * power_ctrl.rever[LB_];
    power_ctrl.I_out[LF_] = *lf.feedback_real_current * power_ctrl.rever[LF_];
    /*---输出功率---*/
    power_ctrl.P_out[RF_] = power_ctrl.K * power_ctrl.I_out[RF_] * power_ctrl.speed[RF_];
    power_ctrl.P_out[RB_] = power_ctrl.K * power_ctrl.I_out[RB_] * power_ctrl.speed[RB_];
    power_ctrl.P_out[LB_] = power_ctrl.K * power_ctrl.I_out[LB_] * power_ctrl.speed[LB_];
    power_ctrl.P_out[LF_] = power_ctrl.K * power_ctrl.I_out[LF_] * power_ctrl.speed[LF_];
    /*---输入功率---*/
	//输入功率 = 输出功率 + 速度损耗 + 电流损耗 + 固定损耗
    power_ctrl.P_in[RF_] = power_ctrl.P_out[RF_] + power_ctrl.K_1 * power_ctrl.speed[RF_]*power_ctrl.speed[RF_] + power_ctrl.k2 * power_ctrl.I_out[RF_]*power_ctrl.I_out[RF_] + power_ctrl.P_C620;
    power_ctrl.P_in[RB_] = power_ctrl.P_out[RB_] + power_ctrl.K_1 * power_ctrl.speed[RB_]*power_ctrl.speed[RB_] + power_ctrl.k2 * power_ctrl.I_out[RB_]*power_ctrl.I_out[RB_] + power_ctrl.P_C620;
    power_ctrl.P_in[LB_] = power_ctrl.P_out[LB_] + power_ctrl.K_1 * power_ctrl.speed[LB_]*power_ctrl.speed[LB_] + power_ctrl.k2 * power_ctrl.I_out[LB_]*power_ctrl.I_out[LB_] + power_ctrl.P_C620;
    power_ctrl.P_in[LF_] = power_ctrl.P_out[LF_] + power_ctrl.K_1 * power_ctrl.speed[LF_]*power_ctrl.speed[LF_] + power_ctrl.k2 * power_ctrl.I_out[LF_]*power_ctrl.I_out[LF_] + power_ctrl.P_C620;
    /*---底盘输出功率总和---*/
    power_ctrl.Chassis_Power_All = 0;
    for(int i = RF_;i <= LF_; i++)
    {
        if ( power_ctrl.P_in[i] > 0 )  power_ctrl.Chassis_Power_All += power_ctrl.P_in[i];//忽略瞬时负功率 反充电
    }    


    if ( power_ctrl.Chassis_Power_All > Max_Power )//若总和超功率，则缩放
    {
        float divisor = Max_Power / power_ctrl.Chassis_Power_All;//缩放因子
        /*---目标输入功率等比例缩小---*/
        power_ctrl.P_in[RF_] = power_ctrl.P_in[RF_] * divisor;
        power_ctrl.P_in[RB_] = power_ctrl.P_in[RB_] * divisor;
        power_ctrl.P_in[LB_] = power_ctrl.P_in[LB_] * divisor;
        power_ctrl.P_in[LF_] = power_ctrl.P_in[LF_] * divisor;
        /*---非线性缩放计算该输入功率下的输出力矩---*/
        for (int i = RF_; i <= LF_; i++)
        {
            if ( power_ctrl.P_in[i] < 0 )    continue;
            float b = power_ctrl.K * power_ctrl.speed[i];
            float c = power_ctrl.K_1 * power_ctrl.speed[i] * power_ctrl.speed[i] - power_ctrl.P_in[i] + power_ctrl.P_C620;
            if ( power_ctrl.give_current[i] > 0 )//维持运动状态
            {
                float temp = (-b + sqrt(b * b - 4 * power_ctrl.k2 * c)) / (2 * power_ctrl.k2);
                if (temp > MOTOR_3508_CURRENT_LIMIT)    power_ctrl.I_max[i] = MOTOR_3508_CURRENT_LIMIT;
                else                                    power_ctrl.I_max[i] = temp;
            }
            else //制动状态
            {
                float temp = (-b - sqrt(b * b - 4 * power_ctrl.k2 * c)) / (2 * power_ctrl.k2);
                if (temp > MOTOR_3508_CURRENT_LIMIT)    power_ctrl.I_max[i] = MOTOR_3508_CURRENT_LIMIT;
                else                                    power_ctrl.I_max[i] = temp;
            }

			power_ctrl.I_max[i] = power_ctrl.I_max[i] * power_ctrl.rever[i];
        }
		return 1;
        /*---更改发送给电调的数据---*/
    }
	return 0;
}

float Superpower_Calc::getNextPowerBuff(float current_Power,float limit_Power,uint16_t buff_Power)
{
    float next_PowerBuff=0;
    next_PowerBuff=buff_Power-(current_Power-limit_Power)*0.1f;
    return next_PowerBuff;
}

//更新预测缓冲能量
void Superpower_Calc::Update_Buffer_Power()
{

}

void Superpower_Calc::SetInitInputPower(uint8_t power_limit)
{
	pm01_power_set((power_limit)*100,0x01);
	pm01_voltage_set(2350,0x01);
	pm01_current_set((6)*100,0x01);
	pm01_cmd_send( cmd, 0x01);
}

void Superpower_Calc::Read_and_Write_Power(uint16_t power_buffer)
{
	Read_SuperPower_Vout();
	if (power_time++ % 10 == 0)//100hz 读取超电电压
		{
			Read_SuperPower_Input();
		}
	if (power_time >= 100)//10hz 根据剩余缓冲能量设定超点输入功率
	{
		if (chassis_power_limit < 10)
			Cap_Input_Power = 120;
		else
			Cap_Input_Power = chassis_power_limit + (power_buffer - 50)/10;
		pm01_power_set((Cap_Input_Power)*100,0x00);
		//pm01_power_set((30)*100,0x00);
		pm01_cmd_send( cmd, 0x01);
		power_time = 0;
	}
}

float Superpower_Calc::Get_Cap_power()
{
	return Cap_Input_Power;
} 


/**
  * @brief          控制命令发送
  * @param[in]      new_cmd   0x00: 停机
                                                            0x01: 运行，不打开输出负载开关（只给超级电容充电）
                                                            0x02: 运行，打开输出负载开关（正常运行使用该指令）
                      save_flg: 0x00: 不保存至EEPROM  0x01: 保存至EEPROM
  * @retval         none
  */
void Superpower_Calc::pm01_cmd_send( uint16_t new_cmd, uint8_t save_flg )
{
    uint32_t send_mail_box;

    CAN_TxHeaderTypeDef  power_tx_message;
    uint8_t              power_can_send_data[8];

    power_tx_message.StdId = 0x600;
    power_tx_message.IDE   = CAN_ID_STD;
    power_tx_message.RTR   = CAN_RTR_DATA;
    power_tx_message.DLC   = 0x04;
    power_can_send_data[0] = (uint8_t)(new_cmd >> 8   );
    power_can_send_data[1] = (uint8_t)(new_cmd &  0xFF);
    power_can_send_data[2] = 0x00;
    power_can_send_data[3] = (save_flg == 0x01);

    HAL_CAN_AddTxMessage(&pm01can, &power_tx_message, power_can_send_data, &send_mail_box);

}

/**
  * @brief          设置功率
  * @param[in]      new_power：新的功率值
                    save_flg: 0x00: 不保存至EEPROM  0x01: 保存至EEPROM
  * @retval         none
  */
void Superpower_Calc::pm01_power_set( uint16_t new_power, uint8_t save_flg )
{
	uint32_t send_mail_box;

	CAN_TxHeaderTypeDef  power_tx_message;
	uint8_t              power_can_send_data[8];

	power_tx_message.StdId = 0x601;
	power_tx_message.IDE   = CAN_ID_STD;
	power_tx_message.RTR   = CAN_RTR_DATA;
	power_tx_message.DLC   = 0x04;
	power_can_send_data[0] = (uint8_t)(new_power >> 8   );
	power_can_send_data[1] = (uint8_t)(new_power &  0xFF);
	power_can_send_data[2] = 0x00;
	power_can_send_data[3] = (save_flg == 0x01);

	HAL_CAN_AddTxMessage(&pm01can, &power_tx_message, power_can_send_data, &send_mail_box);

}
/**
  * @brief          设置输出电压
  * @param[in]      new_volt：新的电压值
                    save_flg: 0x00: 不保存至EEPROM  0x01: 保存至EEPROM
  * @retval         none
  */
void Superpower_Calc::pm01_voltage_set( uint16_t new_voltage, uint8_t save_flg )
{
	uint32_t send_mail_box;

	CAN_TxHeaderTypeDef  power_tx_message;
	uint8_t              power_can_send_data[8];


	power_tx_message.StdId = 0x602;
	power_tx_message.IDE   = CAN_ID_STD;
	power_tx_message.RTR   = CAN_RTR_DATA;
	power_tx_message.DLC   = 0x04;
	power_can_send_data[0] = (uint8_t)(new_voltage >> 8   );
	power_can_send_data[1] = (uint8_t)(new_voltage &  0xFF);
	power_can_send_data[2] = 0x00;
	power_can_send_data[3] = (save_flg == 0x01);


	HAL_CAN_AddTxMessage(&pm01can, &power_tx_message, power_can_send_data, &send_mail_box);

}
/**
  * @brief          设置输出电流
  * @param[in]      new_volt：新的电压值
                    save_flg: 0x00: 不保存至EEPROM  0x01: 保存至EEPROM
  * @retval         none
  */
void Superpower_Calc::pm01_current_set( uint16_t new_current, uint8_t save_flg )
{
	uint32_t send_mail_box;

	CAN_TxHeaderTypeDef  power_tx_message;
	uint8_t              power_can_send_data[8];

	power_tx_message.StdId = 0x603;
	power_tx_message.IDE   = CAN_ID_STD;
	power_tx_message.RTR   = CAN_RTR_DATA;
	power_tx_message.DLC   = 0x04;
	power_can_send_data[0] = (uint8_t)(new_current >> 8   );
	power_can_send_data[1] = (uint8_t)(new_current &  0xFF);
	power_can_send_data[2] = 0x00;
	power_can_send_data[3] = (save_flg == 0x01);

	HAL_CAN_AddTxMessage(&pm01can, &power_tx_message, power_can_send_data, &send_mail_box);

}//610//611
/**
  * @brief          轮询访问PM01参数，请以100HZ频率调用
  * @param[in]      none
  * @retval         none
  */
void Superpower_Calc::pm01_access_poll(void)
{

	static uint8_t   m_state   = 0x00;   /* 状态    */
	static uint16_t  m_can_id  = 0x600;  /* 标识符  */
	static uint32_t  m_timeout = 0;      /* 超时    */

	static uint16_t  i = 0;

	CAN_TxHeaderTypeDef  power_tx_message;

	uint8_t              power_can_send_data[8];

	uint32_t             send_mail_box;

	switch( m_state )
	{
		case 0x00:  /* 发送数据 */

		  m_timeout = 0;   /* 清除超时定时器 */

		  m_state = 0x01;  /* 切换到等待状态 */

		  switch(i)
			{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
							pm01_access_id = m_can_id; /* 记录本次访问的标识符 */

							power_tx_message.StdId = m_can_id;       /* 发送相应的标识符 */
							power_tx_message.IDE   = CAN_ID_STD;     /* 标准帧           */
							power_tx_message.RTR   = CAN_RTR_REMOTE; /* 远程帧           */
							power_tx_message.DLC   = 0x00;

							HAL_CAN_AddTxMessage(&pm01can, &power_tx_message, power_can_send_data, &send_mail_box);
					  break;
					case 8:

							pm01_access_id = m_can_id; /* 记录本次访问的标识符 */

						  pm01_cmd_send( g_cmd_set, 0x00 );
					  break;
					case 9:

							pm01_access_id = m_can_id; /* 记录本次访问的标识符 */

						  pm01_power_set( g_power_set, 0x00 );
					  break;
					case 10:

							pm01_access_id = m_can_id; /* 记录本次访问的标识符 */

						  pm01_voltage_set( g_vout_set, 0x00 );
					  break;
					case 11:

							pm01_access_id = m_can_id; /* 记录本次访问的标识符 */

						  pm01_current_set( g_iout_set, 0x00 );
					  break;
			}

			break;
		case 0x01: /* 等待响应 */


      if( pm01_response_flg == 0x01 )
			{

				pm01_response_flg = 0x00; /* 复位应答标志位 */

				 /* 访问成功，继续访问下一个标识符 */

				i = (i+1) % 12;


				switch( i )
				{
					case 0: m_can_id = 0x600; break;
					case 1: m_can_id = 0x601; break;
					case 2: m_can_id = 0x602; break;
					case 3: m_can_id = 0x603; break;
					case 4: m_can_id = 0x610; break;
					case 5: m_can_id = 0x611; break;
					case 6: m_can_id = 0x612; break;
					case 7: m_can_id = 0x613; break;
				  case 8: m_can_id = 0x600; break; /* 写 */
					case 9: m_can_id = 0x601; break; /* 写 */
					case 10: m_can_id = 0x602; break; /* 写 */
					case 11: m_can_id = 0x603; break; /* 写 */

				}

				m_state = 0x00;  /* 启动下一轮访问 */

			}
			else
			{
				m_timeout++;
			}

		  /* 超时1s，重新访问  */
      if( m_timeout > 100 )
			{
				m_state = 0x00;
			}

			break;
		default:
			m_state = 0x00;

	}

}
/**
  * @brief          应答处理，在CAN接收中断中调用
  * @param[in]      none
  * @retval         none
  */
void Superpower_Calc::pm01_response_handle(CAN_RxHeaderTypeDef  *can_rx_header, uint8_t *can_rx_data )
{

	uint16_t m_tmp;

	pm01_response_flg = ( pm01_access_id == can_rx_header->StdId );

	if( can_rx_header->RTR == CAN_RTR_REMOTE )return;

	switch( can_rx_header->StdId )
	{
		case 0x600:

      m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

		  pm01_od.ccr = m_tmp;

			break;
		case 0x601:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.p_set = m_tmp;

			break;
		case 0x602:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.v_set = m_tmp;

			break;
		case 0x603:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.i_set = m_tmp;

			break;
		case 0x610:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.sta_code.all = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[2] << 8 | can_rx_data[3];

			pm01_od.err_code = m_tmp;

			break;
		case 0x611:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.p_in = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[2] << 8 | can_rx_data[3];

			pm01_od.v_in = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[4] << 8 | can_rx_data[5];

			pm01_od.i_in = m_tmp;

			break;
		case 0x612:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.p_out = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[2] << 8 | can_rx_data[3];

			pm01_od.v_out = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[4] << 8 | can_rx_data[5];

			pm01_od.i_out = m_tmp;

			break;
		case 0x613:

		  m_tmp = (uint16_t)can_rx_data[0] << 8 | can_rx_data[1];

			pm01_od.temp = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[2] << 8 | can_rx_data[3];

			pm01_od.total_time = m_tmp;

		  m_tmp = (uint16_t)can_rx_data[4] << 8 | can_rx_data[5];

			pm01_od.run_time = m_tmp;

			break;
	}


}

//创建单例
Superpower_Calc Superpower_Calc::superpower_instance = Superpower_Calc();
