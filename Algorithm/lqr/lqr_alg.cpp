/**
 * @file lqr_alg.cpp
 * @brief lqr算法层
 *
 * @version 1.1
 * @date 2025-10
 */

#include "lqr_alg.hpp"
using namespace alg_n;


/**
 * ************************* Dongguan-University of Technology -ACE**************************
 * @brief LQR计算
 * @param  void 
 * @return output             
 * ************************* Dongguan-University of Technology -ACE**************************
 */
float alg_n::Lqr_c::Calc(void)
{
	int i,j;
	for(i = 0; i < this->lqr_data_.Control_Size; i++)
	{
		this->lqr_data_.Output[i] = 0;
		for(j = 0; j < this->lqr_data_.System_State_Size; j++)
		{
			this->lqr_data_.Output[i] += this->lqr_data_.Input[j] * this->lqr_data_.k[i * this->lqr_data_.System_State_Size + j];
		}
	}
	return this->lqr_data_.Output[0];
}


/**
 * ************************* Dongguan-University of Technology -ACE**************************
 * @brief  LQR构造初始化
 * @param  system_state_size
 * @param  control_size     
 * @param  k     
 * ************************* Dongguan-University of Technology -ACE**************************
 */
Lqr_c::Lqr_c(uint8_t system_state_size, uint8_t control_size, float *k)
{
  this->lqr_data_.System_State_Size = system_state_size;
  this->lqr_data_.Control_Size = control_size;
	
  //lqr->Control_Area = control_area;
  if(system_state_size != 0)
  {
    this->lqr_data_.Input = (float *)user_malloc(sizeof(float) * system_state_size * control_size);
	memset(this->lqr_data_.Input, 0, sizeof(float) * system_state_size * control_size);
	this->lqr_data_.k = (float *)user_malloc(sizeof(float) * system_state_size * control_size);
	memset(this->lqr_data_.k, 0, sizeof(float) * system_state_size * control_size);

  }
  if(control_size != 0)
  {
    this->lqr_data_.Output = (float *)user_malloc(sizeof(float) * control_size);
	memset(this->lqr_data_.Output, 0, sizeof(float) * control_size);

  }
  
	  this->lqr_data_.k = k;
  
}

/**
 * ************************* Dongguan-University of Technology -ACE**************************
 * @brief  LQR手动初始化
 * @param  system_state_size  
 * @param  control_size     
 * @param  k    lqr参数
 * ************************* Dongguan-University of Technology -ACE**************************
 */
void Lqr_c::Init(uint8_t system_state_size, uint8_t control_size, float *k)
{
  this->lqr_data_.System_State_Size = system_state_size;
  this->lqr_data_.Control_Size = control_size;
	
  //lqr->Control_Area = control_area;
  if(system_state_size != 0)
  {
    this->lqr_data_.Input = (float *)user_malloc(sizeof(float) * system_state_size * control_size);
	memset(this->lqr_data_.Input, 0, sizeof(float) * system_state_size * control_size);
	this->lqr_data_.k = (float *)user_malloc(sizeof(float) * system_state_size * control_size);
	memset(this->lqr_data_.k, 0, sizeof(float) * system_state_size * control_size);

  }
  if(control_size != 0)
  {
    this->lqr_data_.Output = (float *)user_malloc(sizeof(float) * control_size);
	memset(this->lqr_data_.Output, 0, sizeof(float) * control_size);

  }
  
	  this->lqr_data_.k = k;
}



/**
 * ************************* Dongguan-University of Technology -ACE**************************
 * @brief  LQR数据清除
 * @param  void 
 * ************************* Dongguan-University of Technology -ACE**************************
 */
void Lqr_c::Clear(void)
{
  memset(this->lqr_data_.Input, 0, sizeof(float) * this->lqr_data_.System_State_Size * this->lqr_data_.Control_Size);
  
  memset(this->lqr_data_.Output, 0, sizeof(float) * this->lqr_data_.Control_Size);
}


/**
 * ************************* Dongguan-University of Technology -ACE**************************
 * @brief  LQR数据更新
 * @param  system_state  系统状态矩阵            
 * ************************* Dongguan-University of Technology -ACE**************************
 */
void Lqr_c::dataUpdate(float* system_state)
{
	int i = 0;

	for(; i < this->lqr_data_.System_State_Size; i++)
	{
		this->lqr_data_.Input[i] = system_state[i];
	}
}


