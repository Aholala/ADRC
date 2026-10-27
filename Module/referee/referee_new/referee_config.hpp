#ifndef REFEREE_CONFIG_HPP
#define REFEREE_CONFIG_HPP

#include <stdint.h>

namespace Referee_n {

/*********************************** 基础配置 ************************************/
#define REFEREE_RX_BUFFER_NUM 255U
#define VTM_RX_BUFFER_NUM 21U
#define REFEREE_DATA_LEN_MAX 256U
#define REFEREE_HEADER_LEN 5U
#define REFEREE_CMDID_LEN 2U
#define REFEREE_CRC16_LEN 2U
#define REFEREE_FRAME_TAG 0xA5

#define REFEREE_ERROR_NONE 0U
#define REFEREE_ERROR_CRC  1U
#define REFEREE_ERROR_LEN  2U
/*********************************** VT13 配置 ************************************/
#define VT13_CH_VALUE_OFFSET ((uint16_t) 1024)

#pragma pack(1) //按1字节对齐

#define DATA_GAME_STATUS_LEN          11U
#define ID_GAME_STATUS              0x0001U
// 0x0001 比赛状态
typedef struct {
    uint8_t game_type : 4;        // 赛事类型
    uint8_t game_progress : 4;    // 阶段（准备、进行、结算等）
    uint16_t stage_remain_time;   // 当前阶段剩余秒
    uint64_t sync_timestamp;      // 裁判系统同步时间戳
    uint8_t error;
} GameStatus_t;

#define DATA_GAME_RESULT_LEN          1U
#define ID_GAME_RESULT              0x0002U
// 0x0002 比赛结果
typedef struct {
    uint8_t winner;               // 0 平 1 红 2 蓝
    uint8_t error;
} GameResult_t;

#define DATA_ROBOT_HP_LEN             20U
#define ID_ROBOT_HP                 0x0003U
// 0x0003 己方机器人/建筑血量
typedef struct {
    uint16_t hero_hp;// 己方 1 号英雄机器人血量，若该机器人未上场或者被罚下，则血量为 0，下文同理
    uint16_t engineer_hp;
    uint16_t standard3_hp;
    uint16_t standard4_hp;
    int16_t  damage_difference;
    uint16_t sentry_hp;
    uint16_t outpost_hp;
    uint16_t base_hp;
    uint16_t enemy_outpost_HP; 
    uint16_t enemy_base_HP;
    uint8_t error;
} RobotHp_t;

#define DATA_EVENT_LEN                4U
#define ID_EVENT_DATA               0x0101U
// 0x0101 场地事件状态, 0：未占领/未激活 1：已占领/已激活
typedef struct {
    uint32_t supply_zone_overlap : 1;// 己方与资源区不重叠的补给区占领状态，1 为已占领
    uint32_t supply_zone_no_overlap : 1;// 己方与资源区重叠的补给区占领状态，1 为已占领
    uint32_t supply_zone : 1;// 己方补给区的占领状态，1 为已占领（仅 RMUL 适用）
    uint32_t energy_small : 2;// 己方小能量机关的激活状态，0 为未激活，1 为已激活，2 为正在激活
    uint32_t energy_big : 2;// 己方大能量机关的激活状态，0 为未激活，1 为已激活，2 为正在激活
    uint32_t center : 2;// 己方中央高地的占领状态，1 为被己方占领，2 为被对方占领
    uint32_t trapezium_highland : 2;// 己方梯形高地的占领状态，1 为已占领
    uint32_t dart_last_hit : 9;// 对方飞镖最后一次击中己方前哨站或基地的时间（0-420，开局默认为 0）
    uint32_t dart_last_hit_target : 3;// 对方飞镖最后一次击中己方前哨站或基地的具体目标，开局默认为 0，1 为击中前哨站，2 为击中基地固定目标，3 为击中基地随机固定目标，4 为击中基地随机移动目标，5 为击中基地末端移动目标
    uint32_t center_gain_point : 2;// 中心增益点的占领状态，0 为未被占领，1 为被己方占领，2 为被对方占领，3 为被双方占领。（仅 RMUL 适用）
    uint32_t fortress_gain : 2;// 己方堡垒增益点的占领状态，0 为未被占领，1 为被己方占领，2 为被对方占领，3 为被双方占领
    uint32_t outpost_gain : 2;// 己方前哨站增益点的占领状态，0 为未被占领，1 为被己方占领，2 为被对方占领
    uint32_t center_buff_state :1;// 己方基地增益点的占领状态，1 为已占领
    uint32_t reserved :2;
    uint8_t error;
} EventData_t;


#define DATA_REFEREE_WARNING_LEN      3U
#define ID_REFEREE_WARNING          0x0104U
// 0x0104 裁判警告
typedef struct {
    uint8_t level;                // 0 正常 1 黄牌 2 红牌 3 判负
    uint8_t robot_id;             // 违规机器人 ID
    uint8_t count;                // 己方最后一次受到判罚的违规机器人对应判罚等级的违规次数。（开局默认为 0。）
    uint8_t error;
} RefereeWarning_t;

#define DATA_DART_INFO_LEN            3U
#define ID_DART_INFO                0x0105U
// 0x0105 飞镖信息
typedef struct {
    uint8_t remaining_time;       // 己方飞镖发射剩余时间，单位：秒
    uint16_t latest_target : 3;   // bit 0-2：最近一次己方飞镖击中的目标，开局默认为 0，1 为击中前哨站，2 为击中基地固定目标，3 为击中基地随机固定目标，4 为击中基地随机移动目标，5 为击中基地末端移动目标
    uint16_t hit_count : 3;       // bit 3-5: 对方最近被击中的目标累计被击中计次数，开局默认为 0，至多为 4
    uint16_t selected_target : 3; // bit 6-8：飞镖此时选定的击打目标，开局默认或未选定/选定前哨站时为 0，选中基地固定目标为 1，选中基地随机固定目标为 2，选中基地随机移动目标为3，选中基地末端移动目标为 4
    uint16_t reserved : 7;// bit 9-15：保留
    uint8_t error;
} DartInfo_t;

#define DATA_ROBOT_STATUS_LEN         17U
#define ID_ROBOT_STATUS             0x0201U
// 0x0201 机器人状态
typedef struct {
    uint8_t robot_id;// 本机器人 ID
    uint8_t robot_level;// 机器人等级
    uint16_t current_hp;// 机器人当前血量
    uint16_t maximum_hp;// 机器人血量上限
    uint16_t shooter_cooling_value;// 机器人射击热量每秒冷却值
    uint16_t shooter_heat_limit;// 机器人射击热量上
    uint16_t chassis_power_limit;// 机器人底盘功率上限
    float bullet_speed_limit;// 机器人弹丸初速度上限 
    uint8_t gimbal_output : 1;// bit 0：gimbal 口输出，0 为无输出，1 为 24V 输出
    uint8_t chassis_output : 1;// bit 1：chassis 口输出，0 为无输出，1 为 24V 输出
    uint8_t shooter_output : 1;// bit 2：shooter 口输出，0 为无输出，1 为 24V 输出
    uint8_t reserved : 5;
    uint8_t error;
} RobotStatus_t;

#define DATA_POWER_HEAT_LEN           14U
#define ID_POWER_HEAT               0x0202U
// 0x0202 实时功率热量
typedef struct {
    uint16_t reserved0;// 保留位
    uint16_t reserved1;// 保留位
    float reserved2;// 保留位
    uint16_t buffer_energy;// 缓冲能量（单位：J）
    uint16_t shooter_17mm_1_barrel_heat;// 第 1 个 17mm 发射机构的射击热量
    uint16_t shooter_42mm_barrel_heat;// 42mm 发射机构的射击热量
    uint8_t error;
} PowerHeat_t;

#define DATA_ROBOT_POS_LEN            12U
#define ID_ROBOT_POSITION           0x0203U
// 0x0203 机器人位置
typedef struct {
    float x;// 本机器人位置 x 坐标，单位：m
    float y;// 本机器人位置 y 坐标，单位：m
    float angle;// 本机器人测速模块的朝向，单位：度。正北为 0 度
    uint8_t error;
} RobotPosition_t;

#define DATA_BUFF_LEN                 8U
#define ID_BUFF                     0x0204U
// 0x0204 增益状态
typedef struct {
    uint8_t recovery_buff;// 机器人回血增益（百分比，值为 10 表示每秒恢复血量上限的 10%）
    uint16_t cooling_buff;// 机器人射击热量冷却增益具体值（直接值，值为 x 表示热量冷却增加 x/s）
    uint8_t defence_buff; // 机器人防御增益（百分比，值为 50 表示 50%防御增益）
    uint8_t vulnerability_buff; // 机器人负防御增益（百分比，值为 30 表示-30%防御增益）
    uint16_t attack_buff;// 机器人攻击增益（百分比，值为 50 表示 50%攻击增益）
    uint8_t remaining_energy;// bit 0-6：机器人剩余能量值反馈，以 16 进制标识机器人剩余能量值比例，仅在机器人剩余能量小于 50%时反馈，其余默认反馈 0x80。机器人初始能量视为 100%
    uint8_t error;
} Buff_t;

#define DATA_ROBOT_HURT_LEN           1U
#define ID_ROBOT_HURT               0x0206U
// 0x0206 伤害状态
typedef struct {
    uint8_t armor_id : 4;// bit 0-3：当扣血原因为装甲模块被弹丸攻击、受撞击或离线时，该 4 bit 组成的数值为装甲模块或测速模块的 ID 编号；当其他原因导致扣血时，该数值为 0
    uint8_t hurt_type : 4;// bit 4-7：血量变化类型 0：装甲模块被弹丸攻击导致扣血 1：装甲模块或超级电容管理模块离线导致扣血 5：装甲模块受到撞击导致扣血
    uint8_t error;
} RobotHurt_t;

#define DATA_SHOOT_LEN                7U
#define ID_SHOOT_DATA               0x0207U
// 0x0207 射击数据
typedef struct {
    uint8_t bullet_type;// 弹丸类型：bit 1：17mm 弹丸bit 2：42mm 弹丸
    uint8_t shooter_id;// 发射机构 ID： 1： 17mm 发射机构 2：保留位 3：42mm 发射机构
    uint8_t launch_freq;// 弹丸射速（单位：Hz）
    float initial_speed;// 弹丸初速度（单位：m/s）
    uint8_t error;
} ShootData_t;

#define DATA_PROJECTILE_ALLOW_LEN     8U
#define ID_PROJECTILE_ALLOW         0x0208U
// 0x0208 发弹量 & 金币
typedef struct {
    uint16_t allowance_17mm;// 机器人自身拥有的 17mm 弹丸允许发弹量
    uint16_t allowance_42mm;// 42mm 弹丸允许发弹量
    uint16_t remaining_gold;// 剩余金币数量
    uint16_t fortress_allowance;// 堡垒增益点提供的储备 17mm 弹丸允许发弹量；该值与机器人是否实际占领堡垒无关
    uint8_t error;
} ProjectileAllowance_t;

#define DATA_RFID_STATUS_LEN          5U
#define ID_RFID_STATUS              0x0209U
// 0x0209 RFID 状态
typedef struct {
    uint32_t rfid_bits;// bit 位值为 1/0 的含义：是否已检测到该增益点 RFID 卡 
    // bit 0：己方基地增益点 
    // bit 1：己方中央高地增益点 
    // bit 2：对方中央高地增益点 
    // bit 3：己方梯形高地增益点 
    // bit 4：对方梯形高地增益点
    // bit 5：己方地形跨越增益点（飞坡）（靠近己方一侧飞坡前） 
    // bit 6：己方地形跨越增益点（飞坡）（靠近己方一侧飞坡后） 
    // bit 7：对方地形跨越增益点（飞坡）（靠近对方一侧飞坡前） 
    // bit 8：对方地形跨越增益点（飞坡）（靠近对方一侧飞坡后） 
    // bit 9：己方地形跨越增益点（中央高地下方） 
    // bit 10：己方地形跨越增益点（中央高地上方） 
    // bit 11：对方地形跨越增益点（中央高地下方） 
    // bit 12：对方地形跨越增益点（中央高地上方） 
    // bit 13：己方地形跨越增益点（公路下方） 
    // bit 14：己方地形跨越增益点（公路上方） 
    // bit15：对方地形跨越增益点（公路下方） 
    // bit16：对方地形跨越增益点（公路上方） 
    // bit 17：己方堡垒增益点 
    // bit 18：己方前哨站增益点 
    // bit 19：己方与资源区不重叠的补给区/RMUL 补给区 
    // bit 20：己方与资源区重叠的补给区 
    // bit 21：己方装配增益点 
    // bit 22：对方装配增益点 
    // bit 23：中心增益点（仅 RMUL 适用） 
    // bit 24：对方堡垒增益点 
    // bit 25：对方前哨站增益点 
    // bit 26：己方地形跨越增益点（隧道）（靠近己方一侧公路区下方）
    // bit 27：己方地形跨越增益点（隧道）（靠近己方一侧公路区中间）
    // bit 28：己方地形跨越增益点（隧道）（靠近己方一侧公路区上方）
    // bit 29：己方地形跨越增益点（隧道）（靠近己方梯形高地较低处）
    // bit 30：己方地形跨越增益点（隧道）（靠近己方梯形高地较中间）
    // bit 31：己方地形跨越增益点（隧道）（靠近己方梯形高地较高处)
    uint8_t rfid_bits2;
    // bit 0：对方地形跨越增益点（隧道）（靠近对方公路一侧下方）
    // bit 1：对方地形跨越增益点（隧道）（靠近对方公路一侧中间）
    // bit 2：对方地形跨越增益点（隧道）（靠近对方公路一侧上方）
    // bit 3：对方地形跨越增益点（隧道）（靠近对方梯形高地较低处）
    // bit 4：对方地形跨越增益点（隧道）（靠近对方梯形高地较中间） 
    // bit 5：对方地形跨越增益点（隧道）（靠近对方梯形高地较高处）
    uint8_t error;
} RfidStatus_t;

#define DATA_DART_CLIENT_LEN          6U
#define ID_DART_CLIENT_CMD          0x020AU
// 0x020A 飞镖客户端
typedef struct {
    uint8_t launch_state;// 当前飞镖发射站的状态： 1：关闭 2：正在开启或者关闭中 0：已经开启
    uint8_t reserved;
    uint16_t target_change_time;// 切换击打目标时的比赛剩余时间，单位：秒，无/未切换动作，默认为 0。
    uint16_t last_launch_time;// 最后一次操作手确定发射指令时的比赛剩余时间，单位：秒，初始值为 0。
    uint8_t error;
} DartClientCmd_t;

#define DATA_GROUND_ROBOT_POS_LEN     40U
#define ID_GROUND_ROBOT_POSITION    0x020BU
// 0x020B 地面机器人位置
typedef struct {
    float hero_x;// 己方英雄机器人位置 x 轴坐标，单位：m,以此类推
    float hero_y;
    float engineer_x;
    float engineer_y;
    float standard3_x;
    float standard3_y;
    float standard4_x;
    float standard4_y;
    float standard5_x;
    float standard5_y;
    uint8_t error;
} GroundRobotPosition_t;

#define DATA_RADAR_MARK_LEN           2U
#define ID_RADAR_MARK               0x020CU
// 0x020C 雷达标记进度
typedef struct {
    uint16_t mark_bits;
    // bit 0：对方 1 号英雄机器人易伤情况 
    // bit 1：对方 2 号工程机器人易伤情况 
    // bit 2：对方 3 号步兵机器人易伤情况 
    // bit 3：对方 4 号步兵机器人易伤情况 
    // bit 4：对方空中机器人特殊标识情况
    // bit 5：对方哨兵机器人易伤情况
    // bit 6：己方 1 号英雄机器人特殊标识情况 
    // bit 7：己方 2 号工程机器人特殊标识情况 
    // bit 8：己方 3 号步兵机器人特殊标识情况 
    // bit 9：己方 4 号步兵机器人特殊标识情况 
    // bit 10：己方空中机器人特殊标识情况
    // bit 11：己方哨兵机器人特殊标识情况 
    // bit 12-15：保留位
    uint8_t error;
} RadarMark_t;

#define DATA_SENTRY_INFO_LEN          14U
#define ID_SENTRY_INFO              0x020DU
// 0x020D 哨兵同步
typedef struct {
    union {
        uint32_t sentry_info;
        struct {
            uint32_t exchange_allow_bullet : 11;         // bit 0-10 除远程兑换外，哨兵机器人成功兑换的允许发弹量，开局为0，在哨兵机器人成功兑换一定允许发弹量后，该值将变为哨兵机器人成功兑换的允许发弹量值
            uint32_t remote_exchange_bullet_cnt : 4;     // bit 11-14 哨兵机器人成功远程兑换允许发弹量的次数，开局为 0，在哨兵机器人成功远程兑换允许发弹量后，该值将变为哨兵机器人成功远程兑换允许发弹量的次数
            uint32_t remote_exchange_hp_cnt : 4;         // bit 15-18 哨兵机器人成功远程兑换血量的次数，开局为 0，在哨兵机器人成功远程兑换血量后，该值将变为哨兵机器人成功远程兑换血量的次数
            uint32_t can_confirm_free_respawn : 1;       // bit 19 哨兵机器人当前是否可以确认免费复活，可以确认免费复活时值为 1，否则为 0
            uint32_t can_exchange_immediate_respawn : 1; // bit 20 哨兵机器人当前是否可以兑换立即复活，可以兑换立即复活时值为 1，否则为 0 
            uint32_t immediate_respawn_cost : 10;        // bit 21-30 哨兵机器人当前若兑换立即复活需要花费的金币数
            uint32_t reserved_31 : 1;                    // bit 31
        } sentry_info_bits;
    };
    union {
        uint16_t sentry_info_2;
        struct {
            uint16_t is_out_of_track : 1;              // bit 0 哨兵当前是否处于脱战状态，处于脱战状态时为 1，否则为 0
            uint16_t remaining_allow_bullet_17mm : 11; // bit 1-11 队伍 17mm 允许发弹量的剩余可兑换数
            uint16_t posture : 2;                      // bit 12-13 哨兵当前姿态，1 为进攻姿态，2 为防御姿态，3 为移动姿态
            uint16_t energy_mech_can_activate : 1;     // bit 14 己方能量机关是否能够进入正在激活状态，1 为当前可激活
            uint16_t posture_enhanced : 1;             // bit 15 哨兵当前姿态是否为强化姿态，1 为强化姿态，否则为 0
        } sentry_info_2_bits;
    };
    union {
        uint64_t sentry_info_3;
        struct {
            uint64_t offensive_posture_remaining_time : 8;          // bit 0-7 哨兵进攻姿态弱化前剩余可持续时长，单位：秒
            uint64_t defensive_posture_remaining_time : 8;          // bit 8-15 哨兵防御姿态弱化前剩余可持续时长，单位：秒
            uint64_t moving_posture_remaining_time : 8;             // bit 16-23 哨兵移动姿态弱化前剩余可持续时长，单位：秒
            uint64_t reserved_24_31 : 8;                             // bit 24-31 保留位
            uint64_t enhanced_offensive_posture_remaining_time : 8; // bit 32-39 哨兵强化进攻姿态剩余可持续时长，单位：秒
            uint64_t enhanced_defensive_posture_remaining_time : 8; // bit 40-47 哨兵强化防御姿态剩余可持续时长，单位：秒
            uint64_t enhanced_moving_posture_remaining_time : 8;    // bit 48-55 哨兵强化移动姿态剩余可持续时长，单位：秒
            uint64_t reserved_56_63 : 8;                             // bit 56-63 保留位
        } sentry_info_3_bits;
    };
    uint8_t error;
} SentryInfo_t;

#define DATA_RADAR_INFO_LEN           1U
#define ID_RADAR_INFO               0x020EU
// 0x020E 雷达同步
typedef struct {
    union {
        uint8_t info_bits;
        struct {
            uint8_t double_damage_chance : 2; // bit 0-1 雷达是否拥有触发双倍易伤的机会，开局为 0，数值为雷达拥有触发双倍易伤的机会，至多为 2
            uint8_t enemy_double_damage_active : 1; // bit 2 对方是否正在被触发双倍易伤 0：对方未被触发双倍易伤 1：对方正在被触发双倍易伤
            uint8_t encryption_level : 2; // bit 3-4 己方加密等级（即对方干扰波难度等级），开局为 1，最高为 3
            uint8_t can_modify_password : 1; // bit 5 当前是否可以修改密钥，1 为可修改
            uint8_t reserved_6_7 : 2; // bit 6-7 
        } info_bits_detail;
    };
    uint8_t error;
} RadarInfo_t;

#define DATA_INTERACTION_LEN          118U
#define ID_ROBOT_INTERACTION        0x0301U
// 0x0301 机器人交互统一包
typedef struct {
    uint16_t data_cmd_id;// 子内容 ID需为开放的子内容 ID
    uint16_t sender_id;// 发送者 ID需与自身 ID 匹配，ID 编号详见附录
    uint16_t receiver_id;// 仅限己方通信 需为规则允许的多机通讯接收者 若接收者为选手端，则仅可发送至发送者对应的选手端 ID 编号详见附录
    uint8_t payload[112];// x 最大为 112
    uint8_t error;
} RobotInteraction_t;

#define DATA_DIY_CONTROLLER_LEN       30U
#define ID_DIY_CONTROLLER           0x0302U
// 0x0302 自定义控制器→机器人
typedef struct {
    uint8_t data[DATA_DIY_CONTROLLER_LEN];
    uint8_t error;
} DiyController_t;
#define DATA_MAP_DOWNLOAD_LEN         12U
#define ID_MAP_DOWNLOAD             0x0303U
// 0x0303 小地图命令
typedef struct {
    float target_position_x;// 目标位置 x 轴坐标，单位 m
    float target_position_y;// 目标位置 y 轴坐标，单位 m
    // float target_position_z;
    uint8_t cmd_keyboard;   // 云台手按下的键盘按键通用键值
    uint8_t target_robot_id;// 对方机器人id
    uint16_t cmd_source;    //信息来源id
    uint8_t error;
} MapDownload_t;

#define DATA_REMOTE_CONTROL_LEN       12U
#define ID_REMOTE_CONTROL           0x0304U
// 0x0304 键鼠遥控
typedef struct
{
    struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t press_l;
        uint8_t press_r;
    } mouse;
    union
    {
        uint16_t key_code;
        struct
        {
            uint16_t W : 1;
            uint16_t S : 1;
            uint16_t A : 1;
            uint16_t D : 1;
            uint16_t SHIFT : 1;
            uint16_t CTRL : 1;
            uint16_t Q : 1;
            uint16_t E : 1;
            uint16_t R : 1;
            uint16_t F : 1;
            uint16_t G : 1;
            uint16_t Z : 1;
            uint16_t X : 1;
            uint16_t C : 1;
            uint16_t V : 1;
            uint16_t B : 1;
        } bit;
    } kb;
    uint16_t other;
    uint8_t error;
} PHOTO_ctrl_t;

#define DATA_MAP_RECEIVE_LEN          24U
#define ID_MAP_RECEIVE              0x0305U
// 0x0305 雷达推送坐标
typedef struct {
    uint16_t hero_x;
    uint16_t hero_y;
    uint16_t engineer_x;
    uint16_t engineer_y;
    uint16_t standard3_x;
    uint16_t standard3_y;
    uint16_t standard4_x;
    uint16_t standard4_y;
    uint16_t standard5_x;
    uint16_t standard5_y;
    uint16_t sentry_x;
    uint16_t sentry_y;
    uint8_t error;
} MapReceive_t;

#define DATA_CUSTOM_CLIENT_LEN        8U
#define ID_CUSTOM_CLIENT            0x0306U
// 0x0306 自定义客户端（非链路数据）
typedef struct {
    uint16_t key_value;
    uint16_t x_position;
    uint16_t y_position;
    uint16_t reserved;
    uint8_t error;
} CustomClient_t;

#define DATA_MAP_PATH_LEN             105U
#define ID_MAP_PATH                 0x0307U
// 0x0307 哨兵/半自动路径
typedef struct {
    uint8_t intention;// 1：到目标点攻击 2：到目标点防守 3：移动到目标点
    int16_t start_x;// 路径起点 x 轴坐标，单位：dm
    int16_t start_y;// 路径起点 y 轴坐标，单位：dm
    int8_t delta_x[49];// 路径点 x 轴增量数组，单位：dm
    int8_t delta_y[49];// 路径点 y 轴增量数组，单位：dm
    uint16_t sender_id;// 发送者 ID，需与自身 ID 匹配，ID 编号详见附录
    uint8_t error;
} MapPath_t;

#define DATA_CUSTOM_MESSAGE_LEN       34U
#define ID_CUSTOM_MESSAGE           0x0308U
// 0x0308 机器人→选手端自定义消息
typedef struct {
    uint16_t sender_id;// 需要校验发送者的 ID 正确性
    uint16_t receiver_id;// 需要校验接收者的 ID 正确性，仅支持发送己方选手端
    uint8_t user_data[30];// 以 utf-16 格式编码发送，支持显示中文。编码发送时请注意数据的大小端问题
    uint8_t error;
} CustomMessage_t;

#define DATA_ROBOT_CUSTOM_LEN         30U
#define ID_ROBOT_CUSTOM             0x0309U
// 0x0309 机器人→自定义控制器（自定义数据 ）
typedef struct {
    uint8_t data[DATA_ROBOT_CUSTOM_LEN];// 自定义数据
    uint8_t error;
} RobotCustomData_t;

#define DATA_CUSTOM_BYTE_BLOCK_LEN    300U
#define ID_CUSTOM_BYTE_BLOCK        0x0310U
// 0x0310 机器人→自定义数据大包
typedef struct 
{ 
    uint8_t data2[DATA_CUSTOM_BYTE_BLOCK_LEN];// 自定义数据 
    uint8_t error;
}RobotCustomData2_t;

#define ID_CUSTOM_TO_ROBOT                   0x0311U
#define DATA_CUSTOM_TO_ROBOT_LEN             30U
// 0x0311 自定义客户端→机器人自定义数据
typedef struct {
    uint8_t data[DATA_CUSTOM_TO_ROBOT_LEN];// 自定义数据
    uint8_t error;
} CustomToRobotData_t;

#define DATA_SET_CHANNEL_LEN          1U
#define ID_SET_VIDEO_CHANNEL        0x0F01U
// 0x0F01 设置图传出图信道
typedef struct {
    uint8_t channel;// 设置值 1-6：设置信道为 1~6； 反馈值0：图传启动中，无法设置信道； 反馈值1：设置信道成功； 反馈值2：设置信道有误，无法设置。
    uint8_t error;
} VideoChannelSet_t;

#define DATA_QUERY_CHANNEL_LEN        1U
#define ID_QUERY_VIDEO_CHANNEL      0x0F02U
// 0x0F02 查询图传出图信道
typedef struct {
    uint8_t current_channel;
    uint8_t error;// 数据段无需信息，仅发送该命令码即可查询图传出图信道。 反馈值 0：未设置信道； 反馈值 1~6：当前运行的信道。
} VideoChannelQuery_t;

// 数据聚合，方便仿照 referee_config.hpp 使用
typedef struct {
    uint8_t RefereeData[REFEREE_DATA_LEN_MAX];
    uint8_t RealData[REFEREE_DATA_LEN_MAX];
    int16_t DataLen;
    int16_t RealLen;
    int16_t Cmd_ID;
    uint8_t receive_flag;

    GameStatus_t Game_Status;
    GameResult_t Game_Result;
    RobotHp_t Robot_HP;
    EventData_t Event_Data;
    RefereeWarning_t Referee_Warning;
    DartInfo_t Dart_info_;
    RobotStatus_t Robot_Status;
    PowerHeat_t Power_Heat;
    RobotPosition_t Robot_Position;
    Buff_t Buff;
    RobotHurt_t Robot_Hurt;
    ShootData_t Shoot_Data;
    ProjectileAllowance_t Bullet_Num;
    RfidStatus_t RFID_Status;
    DartClientCmd_t Dart_Client;
    GroundRobotPosition_t Robot_Position_Al;
    RadarMark_t radar_mark;
    SentryInfo_t sentry;
    RadarInfo_t radar_info;
    RobotInteraction_t Interact_Data;
    DiyController_t DIY_control;
    MapDownload_t map_download_;
    PHOTO_ctrl_t PHOTO_ctrl;
    MapReceive_t map_receive_;
    CustomClient_t custom_client_;
    MapPath_t map_path_;
    CustomMessage_t custom_message_;
    RobotCustomData_t robot_custom_;
    RobotCustomData2_t custom_byte_block_;
    CustomToRobotData_t custom_to_robot_;
    VideoChannelSet_t set_channel_;
    VideoChannelQuery_t query_channel_;
} RefereeData_t;

// VT13 图传数据
typedef struct {
    uint8_t sof_1;
    uint8_t sof_2;
    uint64_t ch_0 : 11;
    uint64_t ch_1 : 11;
    uint64_t ch_2 : 11;
    uint64_t ch_3 : 11;
    uint64_t mode_sw : 2;
    uint64_t pause : 1;
    uint64_t fn_1 : 1;
    uint64_t fn_2 : 1;
    uint64_t wheel : 11;
    uint64_t trigger : 1;

    int16_t mouse_x;
    int16_t mouse_y;
    int16_t mouse_z;
    uint8_t mouse_left : 2;
    uint8_t mouse_right : 2;
    uint8_t mouse_middle : 2;
    union
    {
        uint16_t key;
        struct
        {
            uint16_t W : 1;
            uint16_t S : 1;
            uint16_t A : 1;
            uint16_t D : 1;
            uint16_t SHIFT : 1;
            uint16_t CTRL : 1;
            uint16_t Q : 1;
            uint16_t E : 1;
            uint16_t R : 1;
            uint16_t F : 1;
            uint16_t G : 1;
            uint16_t Z : 1;
            uint16_t X : 1;
            uint16_t C : 1;
            uint16_t V : 1;
            uint16_t B : 1;
        } bit;
    } kb;
    uint16_t crc16;

    uint8_t data[256];
    int16_t dataLen;
    int16_t ch[5];
} VT13data_t;

#pragma pack()

} // namespace Referee_n

#endif // REFEREE_CONFIG_HPP
