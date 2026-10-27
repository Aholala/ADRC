#ifndef __MUSIC_LIB_HPP
#define __MUSIC_LIB_HPP

#include "buzzer.hpp"

//#define MUSIC_STANDARD_MODE // 曲谱规范模式，
                            // 此模式下要求MUSIC_TABLE[]元素索引必须与Music_PlayState_e的定义一一对应，
                            // 此模式可以减少过多曲谱下Music_Task循环检索带来的性能消耗

typedef enum {
    Idle = 0x00,
    Play_Notice = 0x01,  // 注意，程序没错但是最好不要再这么做（示例：抵达软件限位但是仍想要移动
    Play_Warning = 0x02, // 警告，出现问题但是不影响功能正常运行（示例：图传断连但是DR16仍能控制
    Play_Error = 0x03,   // 错误，出现问题导致功能不能运行，但其他功能正常运行（示例：电机断电
    Play_Fatal = 0x04,   // 致命错误
    Play_WakeUp = 0x05,  // 启动音
    Play_Sleep  = 0x06,  // 关闭音
                         // 7,8保留
    Play_Song1 = 0x09,   // 播放音乐1
    // add
} Music_PlayState_e;

#ifdef MUSIC_STANDARD_MODE
typedef struct{
    uint16_t bpm;
    uint8_t times;
    uint8_t delay_beat;
    const float* song;
}Music_Table_t;
#else
typedef struct{
    Music_PlayState_e play_state;
    uint16_t bpm;
    uint8_t times;
    uint8_t delay_beat;
    const float* song;
}Music_Table_t;
#endif

/***
 * 曲谱规范：
 *    play_state：对应状态播放的曲子
 *    bpm：拍每分钟，应以最小拍为单位，节拍不能是小数
 *    times：循环播放次数
 *    delay_beat：全部播放结束后(包括循环)等待多少个拍子
 *    song：曲谱，应以 <音高, 节拍数> 为一单位谱写
 *          以小于-1大于-10的负数表示曲谱结束
 *          以小于-11的负数表示曲谱结束后将自动进入Idle状态
 * */

/* region ================================ 曲谱 ================================ */
namespace Song_n
{
    const float ERROR[]   = {N_C7, 2, N_FS7, 2, N_ST, 1, -20};
    const float WAKEUP[]  = {N_M1, 1, N_M3, 1, N_M5, 1, N_H1, 1, -2};
    const float SLEEP[]   = {N_H1, 1, N_M5, 1, N_M3, 1, N_M1, 1, -2};
    const float SUPER_MARIO[] = // 超级马里奥，原速800bpm
            {N_M3, 1, N_ST, 1, N_M3, 2, N_ST, 2, N_M3, 2, N_ST, 2, N_M1, 2, N_M3, 2, N_ST, 2, N_M5, 4,
             N_ST, 4, N_L5, 4, N_ST, 4, N_M1, 4, N_ST, 2, N_L5, 2, N_ST, 4, N_L3, 4, N_ST, 2, N_L6, 4,
             N_L7, 2, N_ST, 2, N_L7, 2, N_L6, 4, N_L5, 4, N_M3, 4, N_M5, 4, N_M6, 4, N_M4, 2, N_M5, 2,
             N_ST, 2, N_M3, 4, N_M1, 2, N_M2, 2, N_L7, 4, N_ST, 2,
             -2};
}
// endregion

// 注册曲目
#ifdef MUSIC_STANDARD_MODE
const Music_Table_t MUSIC_TABLE[] = {
        {}, // Idle
        {2000, 1, 8, Song_n::ERROR},  // Play_Notice
        {2000, 2, 8, Song_n::ERROR},  // Play_Warning
        {2000, 3, 8, Song_n::ERROR},  // Play_Error
        {2000, 64, 0, Song_n::ERROR}, // Play_Fatal
        {240, 1, 0, Song_n::WAKEUP},  // Play_WakeUp
        {240, 1, 0, Song_n::SLEEP},   // Play_Sleep
        {}, // 7、8留空
        {},
        {800, 1, 0, Song_n::SUPER_MARIO}, // Play_Song1
};
#else
const Music_Table_t MUSIC_TABLE[] = {
        {Play_Notice, 2000, 1, 8, Song_n::ERROR},
        {Play_Warning, 2000, 2, 8, Song_n::ERROR},
        {Play_Error, 2000, 3, 8, Song_n::ERROR},
        {Play_Fatal, 2000, 64, 0, Song_n::ERROR},
        {Play_WakeUp, 240, 1, 0, Song_n::WAKEUP},
        {Play_Sleep, 240, 1, 0, Song_n::SLEEP},
        {Play_Song1, 800, 1, 0, Song_n::SUPER_MARIO},
        {Idle, 0, 0, 0, nullptr} // 应放在最后，用于检索当个终止符的
};
#endif

#endif //! __MUSIC_LIB_HPP
