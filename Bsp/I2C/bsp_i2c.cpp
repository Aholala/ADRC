/************************** Dongguan-University of Technology -ACE**************************
 * @file bsp_i2c.cpp
 * @author Lann 梁健蘅 (rendezook@qq.com)
 * @brief 
 * @version v0.1
 * @date 2024-09-14
 * @version v0.9
 * @date 2024-09-23
 * @version v1.0
 * @date 2024-09-28
 *******************************************************************************************
 * @verbatim v1.0   支持硬件软件I2C
 * @todo	软件iic未支持
 * 
 * @example 以OLED为例
 * 			定义一个I2C对象
 * 				创建一个HW_I2C_Config_s类型结构体
 * 				创建模块回调函数
 * 				demo:
 * 				HW_I2C_Config_s oled_init_config = {&hi2c1, 0x78, I2C_DMA_MODE};
 * 
 *				void oled_callback(Bsp_I2C_c *register_instance)
 *				{
 *				}
 *
 *				Bsp_I2C_c oled_module(&oled_init_config, oled_callback);
 * @attention 创建模块回调函数时应注意，强烈建议加上以下switch语句，因为Master和Mem模式对应不同的回调函数
 *			  强烈建议加入以下switch模板在模块回调函数里（可以用于区分中断来源）
 *			 		switch(register_instance->Callback_type_)
 *					{
 *					case I2C_Master:
 *						············
 *						break;
 *					case I2C_Mem:
 *						············
 *						break;
 *					default:
 *							while (1)
 *								; // 未知传输模式, 程序停止 
 *							break;
 *					}
 * 
************************** Dongguan-University of Technology -ACE***************************/
#include "bsp_i2c.hpp"


namespace BSP_n
{
	// 定义一个BSP_DWT_c实例指针，并调用ECF_Get_DwtInstance()函数让外部实例指针指向唯一实例的地址
	DWT_c* dwt_time_i2c = DWT_c::Get_DwtInstance();

	//初始化硬件I2C实例指针数组
	I2C_c *I2C_c::i2c_instance_[I2C_DEVICE_CNT] = {nullptr};
	//初始化I2C实例指针数组下标
	uint8_t I2C_c::idx_ = 0;
	//初始化硬件I2C实例指针数组下标
	uint8_t I2C_c::hw_idx_ = 0;

	/**
	 * @brief 硬件I2C构造函数
	 *
	 * @param I2C_Init_Config ：
	 *          I2C_HandleTypeDef *i2c_handle;       // i2c handle
	 *          uint8_t device_address;             // 设置写入数据的地址
	 *          I2C_Work_Mode_e work_mode;       // 工作模式
	 * @param hw_iic_callback
	 */
	I2C_c::I2C_c(HWI2CConfig_s *i2c_init_config ,
								void (*hw_iic_callback)(I2C_c *i2c_init_config))
								://设置i2c实例和回调函数
								device_address_(i2c_init_config->device_address),
								i2c_handle_(i2c_init_config->i2c_handle),
								work_mode_(i2c_init_config->work_mode),
								hw_iic_callback(hw_iic_callback)
	{
		//将当前实例加入指针数组中
		i2c_instance_[idx_++] = this;
		if ((hw_idx_++) >= I2C_DEVICE_CNT)
		{
			// 超过最大实例数，错误处理
			while (true)
			{

			}
		}
	}

	/**
	 * @brief 模拟I2C构造函数
	 *
	 * @param I2C_Init_Config
	 * 			@arg GPIO_TypeDef *i2c_scl_port
	 * 			@arg uint16_t i2c_scl_pin
	 * 			@arg GPIO_TypeDef *i2c_sda_port
	 * 			@arg uint16_t i2c_sda_pin
	 * 			@arg uint8_t device_address	没有可以乱写
	 * 			@example SW_I2C_Config_s oled_init_config = {GPIOC, GPIO_PIN_8, GPIOC, GPIO_PIN_9, 0x78}
	 */
	I2C_c::I2C_c(SWI2CConfig_s *I2C_Init_Config)
							://设置i2c实例
							device_address_(I2C_Init_Config->device_address),
							i2c_scl_port_(I2C_Init_Config->i2c_scl_port),
							i2c_scl_pin_(I2C_Init_Config->i2c_scl_pin),
							i2c_sda_port_(I2C_Init_Config->i2c_sda_port),
							i2c_sda_pin_(I2C_Init_Config->i2c_sda_pin)
	{
		//将当前实例加入指针数组中
		i2c_instance_[idx_++] = this;
		//I2C_SDA_H();
		//I2C_SCL_H();
	}

	void I2C_c::SW_I2C_Init(void)
	{

	}

	/**************************************************以下为硬件I2C函数********************************************************* */
	/**
	 * @brief 硬件I2C发送数据
	 *
	 * @param data 待发送的数据首地址指针
	 * @param size 发送长度
	 */
	void I2C_c::HW_I2C_Transmit(uint8_t *data, uint16_t size)
	{
		switch(this->work_mode_)
		{
		case I2C_BLOCK_MODE:
			HAL_I2C_Master_Transmit(this->i2c_handle_, this->device_address_, data, size, 100);//默认超时时间为100ms
			break;
		case I2C_IT_MODE:
			HAL_I2C_Master_Transmit_IT(this->i2c_handle_, this->device_address_, data, size);
			break;
		case I2C_DMA_MODE:
			HAL_I2C_Master_Transmit_DMA(this->i2c_handle_, this->device_address_, data, size);
			break;
		default:
			while (1)
				; // 未知传输模式, 程序停止
			break;
		}
	}

	/**
	 * @brief 硬件I2C接收数据
	 *
	 * @param data 接收数据的首地址指针
	 * @param size 接收长度
	 */
	void I2C_c::HW_I2C_Receive(uint8_t * data, uint16_t size)
	{
		// 初始化接收缓冲区地址以及接受长度, 用于中断回调函数
		this->rx_buffer = data;
		this->rx_len_ = size;

		switch(this->work_mode_)
		{
		case I2C_BLOCK_MODE:
			HAL_I2C_Master_Receive(this->i2c_handle_, this->device_address_, data, size, 100);//默认超时时间为100ms
			break;
		case I2C_IT_MODE:
			HAL_I2C_Master_Receive_IT(this->i2c_handle_, this->device_address_, data, size);
			break;
		case I2C_DMA_MODE:
			HAL_I2C_Master_Receive_DMA(this->i2c_handle_, this->device_address_, data, size);
			break;
		default:
			while (1)
				; // 未知传输模式, 程序停止
			break;
		}
	}

	/**
	 * @brief I2C读取从机寄存器(内存),只支持阻塞模式,超时默认为1ms
	 *
	 * @param mem_addr 要读取的从机内存地址,目前只支持8位地址
	 * @param data 要读取或写入的数据首地址指针
	 * @param size 要读取或写入的数据长度
	 * @param mem8bit_flag 从机内存地址是否为8位
	 */
	void I2C_c::HW_I2CAccessMem(uint16_t mem_addr, uint8_t *data, uint16_t size, I2CMemMode_e mem_mode,uint8_t mem8bit_flag)
	{
		uint16_t bit_flag = mem8bit_flag ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;
		if (mem_mode == I2C_WRITE_MEM)
		{
			switch(this->work_mode_)
			{
			case I2C_BLOCK_MODE:
				HAL_I2C_Mem_Write(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size, 100);//默认超时时间100ms
				break;
			case I2C_IT_MODE:
				HAL_I2C_Mem_Write_IT(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size);
				break;
			case I2C_DMA_MODE:
				HAL_I2C_Mem_Write_DMA(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size);
				break;
			default:
				while (1)
					; // 未知传输模式, 程序停止
				break;
			}
		}
		//读取操作未完成
		else if (mem_mode == I2C_READ_MEM)
		{
			switch(this->work_mode_)
			{
			case I2C_BLOCK_MODE:
				HAL_I2C_Mem_Read(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size, 100);//默认超时时间100ms
				break;
			case I2C_IT_MODE:
				HAL_I2C_Mem_Read_IT(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size);
				break;
			case I2C_DMA_MODE:
				HAL_I2C_Mem_Read_DMA(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size);
				break;
			default:
				while (1)
					; // 未知传输模式, 程序停止
				break;
			}
		}
		else
		{
			while (1)
				; // 未知模式, 程序停止
		}
	}

	/*************************************************以下为回调函数************************************************************* */
	/**
	 * @brief 此回调函数会被HAL库两回调函数调用
	 *
	 * @param hi2c 			I2C句柄
	 * @param Callback_type	回调函数类型
	 */
	void I2C_c::Bsp_HW_I2C_TxCallback(I2C_HandleTypeDef *hi2c, I2CCallbackMode_e Callback_type)
	{
		// 如果是当前i2c硬件发出的complete,且dev_address和之前发起接收的地址相同,同时回到函数不为空, 则调用回调函数
		for (uint8_t i = 0; i < idx_; i++)
		{
			if (hi2c == i2c_instance_[i]->i2c_handle_)
			{
				if (i2c_instance_[i]->hw_iic_callback != NULL) // 回调函数不为空
				{
					i2c_instance_[i]->hw_iic_callback(i2c_instance_[i]);
					i2c_instance_[i]->Callback_type_ = Callback_type;
				}
				return;
			}
		}
	}

	void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
	{
		I2C_c::Bsp_HW_I2C_TxCallback(hi2c, I2C_Master);
	}

	void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
	{
		I2C_c::Bsp_HW_I2C_TxCallback(hi2c, I2C_Mem);
	}


	/* todo：接收回调函数目前处于无法使用状态，可以根据发送的自行更改 */
	/**
	 * @brief IIC接收完成回调函数
	 *
	 * @param hi2c handle
	 */
	void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
	{
		//Bsp_I2C_c::Bsp_HW_I2C_Callback(hi2c);
	}

	/**
	 * @brief 内存访问回调函数,仅做形式上的封装,仍然使用HAL_I2C_MasterRxCpltCallback
	 *
	 * @param hi2c handle
	 */
	void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
	{
		//HAL_I2C_MasterRxCpltCallback(hi2c);
	}


	/**********************************以下为旧iic搬运的函数（其实可以用    HW_I2CAccessMem     代替）************************************ */
	I2CResult_t I2C_c::writeByte(uint8_t device_address, uint8_t register_address, uint8_t data)
	{
		uint8_t d[2];

		/* Format array to send */
		d[0] = register_address;
		d[1] = data;

		/* Try to transmit via I2C_class */
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1), (uint8_t *)d, 2, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
	}

	I2CResult_t I2C_c::writeByte(uint8_t device_address, uint8_t data)
	{
		uint8_t d[1];

		/* Format array to send */

		d[0] = data;

		/* Try to transmit via I2C_class */
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1), (uint8_t *)d, 2, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
	}

	I2CResult_t I2C_c::write2Bytes(uint8_t device_address, uint8_t register_address, uint16_t data)
	{
		uint8_t d[3];

		/* Format array to send */
		d[0] = register_address;
		d[1] = (uint8_t)(data >> 8);
		d[2] = (uint8_t)(data & 255);

		/* Try to transmit via I2C_class */
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1), (uint8_t *)d, 3, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
	}

	I2CResult_t I2C_c::readMultiBytes(uint8_t device_address, uint8_t register_address, uint8_t* data, uint16_t count)
	{
		//if (HAL_I2C_Master_Transmit(hi2c, (uint8_t)device_address, &register_address, 1, 1000) != HAL_OK) {
		//device_address = 0x83;
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1) , &register_address, 1, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Receive multiple byte */
		if (HAL_I2C_Master_Receive(this->i2c_handle_, (device_address << 1) + 1, data, count, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
	}

	uint8_t I2C_c::readOneByte(uint8_t device_address, uint8_t register_address)
	{
		uint8_t data[1];
		if(readMultiBytes(device_address, register_address, data, 1) == I2C_Result_Error)
		{
			return 0;
		}
		return data[0];
	}

	/************************************以下为模拟I2C *******************************************************************************/

	/**
	 * @brief SDA引脚设置输出模式
	 * @param  无
	 * @return 无
	 */
	void I2C_c::SW_I2C_Output(void)
	{
		GPIO_InitTypeDef SW_I2C_GPIO_STRUCT;
		SW_I2C_GPIO_STRUCT.Mode = GPIO_MODE_OUTPUT_PP;
		SW_I2C_GPIO_STRUCT.Pin = this->i2c_sda_pin_;
		SW_I2C_GPIO_STRUCT.Speed = GPIO_SPEED_FREQ_HIGH;

		HAL_GPIO_Init(this->i2c_sda_port_, &SW_I2C_GPIO_STRUCT);
	}

	/**
	 * @brief SDA引脚设置输入模式
	 * @param  无
	 * @return 无
	 */
	void I2C_c::SW_I2C_Input(void)
	{
		GPIO_InitTypeDef SW_I2C_GPIO_STRUCT;
		SW_I2C_GPIO_STRUCT.Mode = GPIO_MODE_INPUT;
		SW_I2C_GPIO_STRUCT.Pin = this->i2c_sda_pin_;
		SW_I2C_GPIO_STRUCT.Speed = GPIO_SPEED_FREQ_HIGH;

		HAL_GPIO_Init(this->i2c_sda_port_, &SW_I2C_GPIO_STRUCT);
	}

	/**
	 * @brief 写模拟I2C的SCL函数
	 *
	 * @param bit 1置高电平，0置低电平
	 */
	void I2C_c::SW_I2C_W_SCL(uint8_t bit)
	{
		if(bit)
		{
			HAL_GPIO_WritePin(this->i2c_scl_port_, this->i2c_scl_pin_, GPIO_PIN_SET);
		}
		else
			HAL_GPIO_WritePin(this->i2c_scl_port_, this->i2c_scl_pin_, GPIO_PIN_RESET);
	}

	/**
	 * @brief 写模拟I2C的SDA函数
	 *
	 * @param bit 1置高电平，0置低电平
	 */
	void I2C_c::SW_I2C_W_SDA(uint8_t bit)
	{
		if(bit)
		{
			HAL_GPIO_WritePin(this->i2c_sda_port_, this->i2c_sda_pin_, GPIO_PIN_SET);
		}
		else
			HAL_GPIO_WritePin(this->i2c_sda_port_, this->i2c_sda_pin_, GPIO_PIN_RESET);
	}

	/**
	 * @brief 读模拟I2C SCL的函数
	 *
	 * @return uint8_t
	 */
	uint8_t I2C_c::SW_I2C_R_SCL(void)
	{
		uint8_t BitValue;
		BitValue = HAL_GPIO_ReadPin(this->i2c_scl_port_, this->i2c_scl_pin_);
		return BitValue;
	}

	/**
	 * @brief 读模拟I2C SDA的函数
	 *
	 * @return uint8_t
	 */
	uint8_t I2C_c::SW_I2C_R_SDA(void)
	{
		uint8_t BitValue;
		BitValue = HAL_GPIO_ReadPin(this->i2c_sda_port_, this->i2c_sda_pin_);
		return BitValue;
	}

	/**
	 * @brief I2C起始信号
	 *
	 */
	void I2C_c::SW_I2C_Start(void)
	{
		I2C_SDA_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SDA_L();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_L();
	}

	/**
	 * @brief IIC停止信号
	 *
	 */
	void I2C_c::SW_I2C_Stop(void)
	{
		I2C_SDA_L();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SDA_H();
	}

	/**
	 * @brief IIC应答信号
	 * @param  无
	 * @return 无
	 */
	void I2C_c::SW_I2C_ACK(void)
	{
		I2C_SCL_L();
		I2C_SDA_L();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_L();
	}

	/**
	 * @brief IIC无应答信号
	 * @param  无
	 * @return 无
	 */
	void I2C_c::SW_I2C_NACK(void)
	{
		I2C_SCL_L();
		I2C_SDA_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
	}

	/**
	 * @brief IIC等待应答信号
	 * @param  无
	 * @return 0无应答  1有应答
	 */
	uint8_t I2C_c::SW_I2C_Wait_ACK(void)
	{
		// uint8_t wait = 0;
		// SW_I2C_Output();
		// I2C_SDA_H();
		// SW_I2C_Input();
		// I2C_SCL_H();
		// dwt_time_i2c->Delay(I2C_DELAY_TIME);
		// while (HAL_GPIO_ReadPin(this->i2c_scl_port_, this->i2c_sda_pin_))
		// {
		//     wait++;
		//     if (wait > 200)
		//     {
		//         SW_I2C_Stop();
		//         return 0;
		//     }
		// }
		// I2C_SCL_L();
		// return 1;
		I2C_SDA_L();
		I2C_SCL_H();
		I2C_SCL_L();
	}

	/**
	 * @brief I2C写数据1
	 *
	 */
	void I2C_c::SW_I2C_W_H(void)
	{
		I2C_SCL_L();
		I2C_SDA_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_L();
	}

	/**
	 * @brief I2C写数据0
	 *
	 */
	void I2C_c::SW_I2C_W_L(void)
	{
		I2C_SCL_L();
		I2C_SDA_L();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		I2C_SCL_L();
	}

	/**
	 * @brief IIC写入单个数据
	 * @param  无
	 * @return 应答信号, 0无应答 1有应答
	 * @attention 应答信号暂时还是删除，以江科大的为主
	 */
	uint8_t I2C_c::SW_I2C_Write_Byte(uint8_t Byte)
	{
		uint8_t i;
		for (i = 0; i < 8; i++)
		{
			SW_I2C_W_SDA(Byte & (0x80 >> i));
			dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
			SW_I2C_W_SCL(1);
			dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
			SW_I2C_W_SCL(0);
		}
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		SW_I2C_W_SCL(1);	//额外的一个时钟，不处理应答信号
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		SW_I2C_W_SCL(0);
		//dwt_time_i2c->Delay(I2C_DELAY_TIME);
	}

	/**
	 * @brief IIC读一个数据
	 * @param  ACK:应答 NACK:不应答
	 * @return 返回读到的数据
	 */
	uint8_t I2C_c::SW_I2C_Recv_Byte(I2CACKSTATUS_e ack_sta)
	{
		uint8_t Byte = 0, i;
		SW_I2C_Input();
		I2C_SCL_H();
		dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		for(i = 0x80; i != 0; i >>= 1)
		{
			if(HAL_GPIO_ReadPin(this->i2c_sda_port_, this->i2c_sda_pin_) == 1)
			{
				Byte |= i;
			}
			dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
			I2C_SCL_L();
			dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
			I2C_SCL_H();
			dwt_time_i2c->Delay_ms(I2C_DELAY_TIME);
		}
		if(ack_sta == ACK)
		{
			SW_I2C_ACK();
		}
		else
		{
			SW_I2C_NACK();
		}
		return Byte;
	}
}
