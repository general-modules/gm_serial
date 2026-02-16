/**
 * @file      gm_serial.h
 * @brief     串口模块头文件
 * @author    huenrong (sgyhy1028@outlook.com)
 * @date      2026-02-16 16:26:40
 *
 * @copyright Copyright (c) 2026 huenrong
 *
 */

#ifndef __GM_SERIAL_H
#define __GM_SERIAL_H

#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define GM_SERIAL_VERSION_MAJOR 1
#define GM_SERIAL_VERSION_MINOR 0
#define GM_SERIAL_VERSION_PATCH 0

// 串口波特率
typedef enum gm_serial_baud_rate
{
    E_BAUD_RATE_0 = B0,
    E_BAUD_RATE_50 = B50,
    E_BAUD_RATE_75 = B75,
    E_BAUD_RATE_110 = B110,
    E_BAUD_RATE_134 = B134,
    E_BAUD_RATE_150 = B150,
    E_BAUD_RATE_200 = B200,
    E_BAUD_RATE_300 = B300,
    E_BAUD_RATE_600 = B600,
    E_BAUD_RATE_1200 = B1200,
    E_BAUD_RATE_1800 = B1800,
    E_BAUD_RATE_2400 = B2400,
    E_BAUD_RATE_4800 = B4800,
    E_BAUD_RATE_9600 = B9600,
    E_BAUD_RATE_19200 = B19200,
    E_BAUD_RATE_38400 = B38400,
    E_BAUD_RATE_57600 = B57600,
    E_BAUD_RATE_115200 = B115200,
    E_BAUD_RATE_230400 = B230400,
    E_BAUD_RATE_460800 = B460800,
    E_BAUD_RATE_500000 = B500000,
    E_BAUD_RATE_576000 = B576000,
    E_BAUD_RATE_921600 = B921600,
    E_BAUD_RATE_1000000 = B1000000,
    E_BAUD_RATE_1152000 = B1152000,
    E_BAUD_RATE_1500000 = B1500000,
    E_BAUD_RATE_2000000 = B2000000,
    E_BAUD_RATE_2500000 = B2500000,
    E_BAUD_RATE_3000000 = B3000000,
    E_BAUD_RATE_3500000 = B3500000,
    E_BAUD_RATE_4000000 = B4000000,
    E_BAUD_RATE_SPECIAL = 0xFFFFFFFF, // 特殊波特率
} gm_serial_baud_rate_e;

// 串口数据位
typedef enum gm_serial_data_bit
{
    E_DATA_BIT_5 = CS5,
    E_DATA_BIT_6 = CS6,
    E_DATA_BIT_7 = CS7,
    E_DATA_BIT_8 = CS8,
} gm_serial_data_bit_e;

// 串口奇偶校验位
typedef enum gm_serial_parity_bit
{
    E_PARITY_BIT_N = 0, // 无校验
    E_PARITY_BIT_O = 1, // 奇校验
    E_PARITY_BIT_E = 2, // 偶校验
} gm_serial_parity_bit_e;

// 串口停止位
typedef enum gm_serial_stop_bit
{
    E_STOP_BIT_1 = 1,
    E_STOP_BIT_2 = 2,
} gm_serial_stop_bit_e;

// 串口对象
typedef struct _gm_serial gm_serial_t;

/**
 * @brief 创建串口对象
 *
 * @return 成功: 串口对象
 * @return 失败: NULL
 */
gm_serial_t *gm_serial_create(void);

/**
 * @brief 初始化串口对象
 *
 * @note 1. 该函数支持重复调用，重复调用时会关闭并重新打开串口
 *       2. 调用该函数前必须确保没有其它线程正在使用该串口对象，否则可能导致未定义行为
 *
 * @param[in,out] gm_serial        : 串口对象
 * @param[in]     serial_name      : 串口设备名称（如：/dev/ttyUSB0）
 * @param[in]     std_baud_rate    : 标准波特率（若使用特殊波特率必须填 E_BAUD_RATE_SPECIAL）
 * @param[in]     special_baud_rate: 特殊波特率（若不使用可填任意值）
 * @param[in]     data_bit         : 数据位
 * @param[in]     parity_bit       : 奇偶校验位
 * @param[in]     stop_bit         : 停止位
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int gm_serial_init(gm_serial_t *gm_serial, const char *serial_name, const gm_serial_baud_rate_e std_baud_rate,
                   const int special_baud_rate, const gm_serial_data_bit_e data_bit,
                   const gm_serial_parity_bit_e parity_bit, const gm_serial_stop_bit_e stop_bit);

/**
 * @brief 销毁串口对象
 *
 * @note 1. 调用该函数前必须确保没有其它线程正在使用该串口对象，否则可能导致未定义行为
 *       2. 若串口已打开，本函数会先关闭串口再释放资源
 *       3. 销毁后，串口对象将不再可用
 *
 * @param[in,out] gm_serial: 串口对象
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int gm_serial_destroy(gm_serial_t *gm_serial);

/**
 * @brief 清空串口输入缓存
 *
 * @param[in,out] gm_serial: 串口对象
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int gm_serial_flush_input_cache(gm_serial_t *gm_serial);

/**
 * @brief 清空串口输出缓存
 *
 * @param[in,out] gm_serial: 串口对象
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int gm_serial_flush_output_cache(gm_serial_t *gm_serial);

/**
 * @brief 清空串口输入输出缓存
 *
 * @param[in,out] gm_serial: 串口对象
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int gm_serial_flush_both_cache(gm_serial_t *gm_serial);

/**
 * @brief 发送数据
 *
 * @param[in,out] gm_serial    : 串口对象
 * @param[in]     send_data    : 待发送数据
 * @param[in]     send_data_len: 待发送数据长度
 *
 * @return >0 : 实际发送数据长度
 * @return <=0: 失败
 */
ssize_t gm_serial_write_data(gm_serial_t *gm_serial, const uint8_t *send_data, const size_t send_data_len);

/**
 * @brief 接收数据
 *
 * @note 若在 timeout_msec 时间内持续有数据到达，则持续接收数据直到 recv_data_len 长度。
 *       也就是说 timeout_msec 是字节间超时时间，而不是总超时时间。
 *
 * @param[in,out] gm_serial    : 串口对象
 * @param[out]    recv_data    : 接收到的数据
 * @param[in]     recv_data_len: 指定接收数据长度
 * @param[in]     timeout_msec : 单次等待超时时间（单位：毫秒）
 *
 * @return >0: 实际接收数据长度
 * @return 0 : 超时未接收到数据（从调用开始到第一次接收到数据的时间超过 timeout_msec）
 * @return <0: 失败
 */
ssize_t gm_serial_read_data(gm_serial_t *gm_serial, uint8_t *recv_data, const size_t recv_data_len,
                            const uint32_t timeout_msec);

#ifdef __cplusplus
}
#endif

#endif // __GM_SERIAL_H
