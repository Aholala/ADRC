#include "new_Cap.hpp"

#if defined(USE_JLINK_RTT)
#include "bsp_jlink_rtt.hpp"
#endif

uint8_t Cap_is_connect = 0;
uint32_t cap_count = 0;
static bool cap_low_voltage_latched = false;
static volatile bool cap_low_voltage_log_pending = false;
static volatile bool cap_low_voltage_recover_log_pending = false;
static bool cap_disconnect_latched = false;
static volatile bool cap_disconnect_recover_log_pending = false;

#if defined(USE_JLINK_RTT) && defined(ECF_RTT_LOG_ENABLE_NEW_CAP) && (ECF_RTT_LOG_ENABLE_NEW_CAP)
#define NEW_CAP_LOG_I(fmt, ...) LOG_RTT_I("[NEW_CAP][COMM] " fmt __VA_OPT__(,) __VA_ARGS__)
#define NEW_CAP_LOG_W(fmt, ...) LOG_RTT_W("[NEW_CAP][SENSOR] " fmt __VA_OPT__(,) __VA_ARGS__)
#define NEW_CAP_LOG_E(fmt, ...) LOG_RTT_E("[NEW_CAP][COMM] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define NEW_CAP_LOG_I(fmt, ...) ((void)0)
#define NEW_CAP_LOG_W(fmt, ...) ((void)0)
#define NEW_CAP_LOG_E(fmt, ...) ((void)0)
#endif

#ifdef STM32H723xx
static void Deal_Cap_data(BSP_n::Fdcan_c *register_instance)
#elif STM32F405xx
static void Deal_Cap_data(BSP_n::Can_c *register_instance)
#endif
{

    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_safe_task->Online();
    
    uint8_t *rxbuff;
    rxbuff = register_instance->rx_buff_;
    int16_t Vbat,Vcap,Cbat,Ccap;
    Vbat = (rxbuff[0] << 8 | rxbuff[1]) ;
    Vcap = (rxbuff[2] << 8 | rxbuff[3]) ;
    Cbat = (rxbuff[4] << 8 | rxbuff[5]) ;
    Ccap =-(rxbuff[6] << 8 | rxbuff[7]) ;

    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vbat = (float)Vbat/100.0f;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vcap = (float)Vcap/100.0f;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Cbat = (float)Cbat/100.0f;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Ccap = (float)Ccap/100.0f;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Pbat = Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Cbat * Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vbat;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Pcap = Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Ccap * Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vcap;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Chassis_Power = Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Pbat + Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Pcap;

    float dt = Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_dwt->GetDeltaT(&cap_count) ;
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Chassis_energy += Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Chassis_Power*dt;

    const bool low_voltage_now =
        (Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vbat < ECF_NEW_CAP_LOW_VBAT_WARN) ||
        (Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vcap < ECF_NEW_CAP_LOW_VCAP_WARN);
    // Avoid heavy printf/RTT operations in FDCAN IRQ callback context.
    if (low_voltage_now && !cap_low_voltage_latched)
    {
        cap_low_voltage_log_pending = true;
    }
    if ((!low_voltage_now) && cap_low_voltage_latched)
    {
        cap_low_voltage_recover_log_pending = true;
    }
    cap_low_voltage_latched = low_voltage_now;
}

void Cap_Ctrl_n::Cap_Ctrl_c::init()
{
    #ifdef STM32H723xx
    can_com = new BSP_n::Fdcan_c(CAP_FDCAN, CAP_TX_ID, CAP_RX_ID, FDCAN_STANDARD_ID, nullptr);
    #elif STM32F405xx
    can_com = new BSP_n::Can_c(CAP_CAN, CAP_TX_ID, CAP_RX_ID, CAN_ID_STD, nullptr);
    #endif
    cap_dwt = BSP_n::DWT_c::Get_DwtInstance(); 
    can_com->SetRxCallBack(Deal_Cap_data);
    cap_safe_task = new SafeTask_c("Cap_safe_task", 1000,
                                   []()
                                   {
                                       if (Cap_is_connect != 0)
                                       {
                                           NEW_CAP_LOG_E("disconnected timeout=%llums", 1000ull);
                                       }
                                       Cap_is_connect = 0;
                                       cap_disconnect_latched = true;
                                       cap_low_voltage_latched = false;
                                       cap_low_voltage_log_pending = false;
                                       cap_low_voltage_recover_log_pending = false;
                                       cap_disconnect_recover_log_pending = false;
                                   },
                                   []()
                                   {
                                       if (cap_disconnect_latched)
                                       {
                                           cap_disconnect_recover_log_pending = true;
                                           cap_disconnect_latched = false;
                                       }
                                       Cap_is_connect = 1;
                                   });
    cap_data.Chassis_energy = 0;
}

void Cap_Ctrl_n::Cap_Ctrl_c::PollLog()
{
    if (!cap_low_voltage_log_pending &&
        !cap_low_voltage_recover_log_pending &&
        !cap_disconnect_recover_log_pending)
    {
        return;
    }
    if (cap_low_voltage_log_pending)
    {
        cap_low_voltage_log_pending = false;
        NEW_CAP_LOG_W("low_voltage Vbat=%.2fV(<%.2fV), Vcap=%.2fV(<%.2fV)",
                      cap_data.Vbat, ECF_NEW_CAP_LOW_VBAT_WARN,
                      cap_data.Vcap, ECF_NEW_CAP_LOW_VCAP_WARN);
    }
    if (cap_low_voltage_recover_log_pending)
    {
        cap_low_voltage_recover_log_pending = false;
        NEW_CAP_LOG_I("low_voltage_cleared Vbat=%.2fV Vcap=%.2fV",
                      cap_data.Vbat, cap_data.Vcap);
    }
    if (cap_disconnect_recover_log_pending)
    {
        cap_disconnect_recover_log_pending = false;
        NEW_CAP_LOG_I("reconnected");
    }
}

uint8_t get_cap_static() { return Cap_is_connect;}

Cap_Ctrl_n::Cap_Ctrl_c Cap_Ctrl_n::Cap_Ctrl_c::cap_instance = Cap_Ctrl_c();
