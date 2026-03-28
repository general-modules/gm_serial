/**
 * @file      gm_serial.c
 * @brief     串口模块源文件
 * @author    huenrong (sgyhy1028@outlook.com)
 * @date      2026-02-16 16:26:32
 *
 * @copyright Copyright (c) 2026 huenrong
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <pthread.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "gm_serial.h"

// 串口对象
struct _gm_serial_t
{
    int fd;
    pthread_mutex_t mutex;
};

/**
 * @brief 设置串口标准波特率
 *
 * @param[in,out] options  : 串口属性
 * @param[in]     baud_rate: 波特率
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
static int gm_serial_set_std_baud_rate(struct termios *options, const gm_serial_baud_rate_e baud_rate)
{
    if (options == NULL)
    {
        return -1;
    }

    // 设置输入波特率
    if (cfsetispeed(options, baud_rate) != 0)
    {
        return -2;
    }

    // 设置输出波特率
    if (cfsetospeed(options, baud_rate) != 0)
    {
        return -3;
    }

    return 0;
}

/**
 * @brief 设置串口特殊波特率
 *
 * @param[in,out] options  : 串口属性
 * @param[in]     fd       : 文件描述符
 * @param[in]     baud_rate: 波特率
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
static int gm_serial_set_special_baud_rate(struct termios *options, const int fd, const int baud_rate)
{
    if (options == NULL)
    {
        return -1;
    }

    if (fd < 0)
    {
        return -2;
    }

    // 设置输入波特率为 38400
    if (cfsetispeed(options, B38400) != 0)
    {
        return -3;
    }

    // 设置输出波特率为 38400
    if (cfsetospeed(options, B38400) != 0)
    {
        return -4;
    }

    struct serial_struct serial = {0};
    if (ioctl(fd, TIOCGSERIAL, &serial) != 0)
    {
        return -5;
    }

    // 设置标志位和系数
    serial.flags = ASYNC_SPD_CUST;
    serial.custom_divisor = (serial.baud_base / baud_rate);
    if (ioctl(fd, TIOCSSERIAL, &serial) != 0)
    {
        return -5;
    }

    return 0;
}

/**
 * @brief 设置串口数据位
 *
 * @param[in,out] options : 串口属性
 * @param[in]     data_bit: 数据位
 *
 * @retval 0 : 成功
 * @retval <0: 失败
 */
static int gm_serial_set_data_bit(struct termios *options, const gm_serial_data_bit_e data_bit)
{
    if (options == NULL)
    {
        return -1;
    }

    switch (data_bit)
    {
    case E_DATA_BIT_5:
    case E_DATA_BIT_6:
    case E_DATA_BIT_7:
    case E_DATA_BIT_8:
    {
        options->c_cflag &= ~CSIZE;
        options->c_cflag |= data_bit;

        break;
    }

    default:
    {
        return -2;
    }
    }

    return 0;
}

/**
 * @brief 设置串口奇偶检验位
 *
 * @param[in,out] options   : 串口属性
 * @param[in]     parity_bit: 奇偶检验位
 *
 * @retval 0 : 成功
 * @retval <0: 失败
 */
static int gm_serial_set_parity_bit(struct termios *options, const gm_serial_parity_bit_e parity_bit)
{
    if (options == NULL)
    {
        return -1;
    }

    switch (parity_bit)
    {
    // 无校验
    case E_PARITY_BIT_N:
    {
        options->c_cflag &= ~PARENB;
        options->c_iflag &= ~INPCK;

        break;
    }

    // 奇校验
    case E_PARITY_BIT_O:
    {
        options->c_cflag |= (PARODD | PARENB);
        options->c_iflag |= INPCK;

        break;
    }

    // 偶校验
    case E_PARITY_BIT_E:
    {
        options->c_cflag |= PARENB;
        options->c_cflag &= ~PARODD;
        options->c_iflag |= INPCK;

        break;
    }

    default:
    {
        return -2;
    }
    }

    return 0;
}

/**
 * @brief 设置串口停止位
 *
 * @param[in,out] options : 串口属性
 * @param[in]     stop_bit: 停止位
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
static int gm_serial_set_stop_bit(struct termios *options, const gm_serial_stop_bit_e stop_bit)
{
    if (options == NULL)
    {
        return -1;
    }

    switch (stop_bit)
    {
    case E_STOP_BIT_1:
    {
        options->c_cflag &= ~CSTOPB;

        break;
    }

    case E_STOP_BIT_2:
    {
        options->c_cflag |= CSTOPB;

        break;
    }

    default:
    {
        return -2;
    }
    }

    return 0;
}

gm_serial_t *gm_serial_create(void)
{
    gm_serial_t *gm_serial = (gm_serial_t *)malloc(sizeof(gm_serial_t));
    if (gm_serial == NULL)
    {
        return NULL;
    }

    gm_serial->fd = -1;
    pthread_mutex_init(&gm_serial->mutex, NULL);

    return gm_serial;
}

int gm_serial_init(gm_serial_t *gm_serial, const char *serial_name, const gm_serial_baud_rate_e std_baud_rate,
                   const int special_baud_rate, const gm_serial_data_bit_e data_bit,
                   const gm_serial_parity_bit_e parity_bit, const gm_serial_stop_bit_e stop_bit)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    if (serial_name == NULL)
    {
        return -2;
    }

    pthread_mutex_lock(&gm_serial->mutex);
    if (gm_serial->fd != -1)
    {
        close(gm_serial->fd);
        gm_serial->fd = -1;
    }

    // 打开串口
    int fd = open(serial_name, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -3;
    }

    // 获取终端属性
    struct termios options = {0};
    if (tcgetattr(fd, &options) != 0)
    {
        close(fd);
        pthread_mutex_unlock(&gm_serial->mutex);

        return -4;
    }

    // 设置标准波特率
    if (std_baud_rate != E_BAUD_RATE_SPECIAL)
    {
        if (gm_serial_set_std_baud_rate(&options, std_baud_rate) != 0)
        {
            close(fd);
            pthread_mutex_unlock(&gm_serial->mutex);

            return -5;
        }
    }
    // 设置特殊波特率
    else
    {
        if (gm_serial_set_special_baud_rate(&options, fd, special_baud_rate) != 0)
        {
            close(fd);
            pthread_mutex_unlock(&gm_serial->mutex);

            return -6;
        }
    }

    // 设置数据位
    if (gm_serial_set_data_bit(&options, data_bit) != 0)
    {
        close(fd);
        pthread_mutex_unlock(&gm_serial->mutex);

        return -7;
    }

    // 设置奇偶检验位
    if (gm_serial_set_parity_bit(&options, parity_bit) != 0)
    {
        close(fd);
        pthread_mutex_unlock(&gm_serial->mutex);

        return -8;
    }

    // 设置停止位
    if (gm_serial_set_stop_bit(&options, stop_bit) != 0)
    {
        close(fd);
        pthread_mutex_unlock(&gm_serial->mutex);

        return -9;
    }

    // 一般必设置的标志
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_oflag &= ~(OPOST);
    options.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    options.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | IXOFF | IXANY);

    // 清空输入输出缓冲区
    if (tcflush(fd, TCIOFLUSH) != 0)
    {
        close(fd);
        pthread_mutex_unlock(&gm_serial->mutex);

        return -10;
    }

    // 设置最小接收字符数和超时时间
    // 当 MIN=0, TIME=0 时，如果有数据可用，则 read 最多返回所要求的字节数
    // 如果无数据可用，则 read 立即返回 0
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    // 设置终端属性
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        close(fd);
        pthread_mutex_unlock(&gm_serial->mutex);

        return -11;
    }

    gm_serial->fd = fd;
    pthread_mutex_unlock(&gm_serial->mutex);

    return 0;
}

int gm_serial_destroy(gm_serial_t *gm_serial)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&gm_serial->mutex);

    // 串口已初始化，先关闭再销毁
    if (gm_serial->fd != -1)
    {
        if (close(gm_serial->fd) != 0)
        {
            pthread_mutex_unlock(&gm_serial->mutex);

            return -2;
        }
        gm_serial->fd = -1;
    }

    pthread_mutex_unlock(&gm_serial->mutex);
    pthread_mutex_destroy(&gm_serial->mutex);
    free(gm_serial);

    return 0;
}

int gm_serial_flush_input_cache(gm_serial_t *gm_serial)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&gm_serial->mutex);
    if (gm_serial->fd < 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -2;
    }

    if (tcflush(gm_serial->fd, TCIFLUSH) != 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -3;
    }
    pthread_mutex_unlock(&gm_serial->mutex);

    return 0;
}

int gm_serial_flush_output_cache(gm_serial_t *gm_serial)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&gm_serial->mutex);
    if (gm_serial->fd < 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -2;
    }

    if (tcflush(gm_serial->fd, TCOFLUSH) != 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -3;
    }
    pthread_mutex_unlock(&gm_serial->mutex);

    return 0;
}

int gm_serial_flush_both_cache(gm_serial_t *gm_serial)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&gm_serial->mutex);
    if (gm_serial->fd < 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -2;
    }

    if (tcflush(gm_serial->fd, TCIOFLUSH) != 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -3;
    }
    pthread_mutex_unlock(&gm_serial->mutex);

    return 0;
}

ssize_t gm_serial_write_data(gm_serial_t *gm_serial, const uint8_t *send_data, const size_t send_data_len)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    if (send_data == NULL)
    {
        return -2;
    }

    if (send_data_len == 0)
    {
        return -3;
    }

    pthread_mutex_lock(&gm_serial->mutex);
    if (gm_serial->fd < 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -4;
    }

    ssize_t write_len = write(gm_serial->fd, send_data, send_data_len);
    pthread_mutex_unlock(&gm_serial->mutex);

    return write_len;
}

ssize_t gm_serial_read_data(gm_serial_t *gm_serial, uint8_t *recv_data, const size_t recv_data_len,
                            const uint32_t timeout_msec)
{
    if (gm_serial == NULL)
    {
        return -1;
    }

    if (recv_data == NULL)
    {
        return -2;
    }

    if (recv_data_len == 0)
    {
        return -3;
    }

    pthread_mutex_lock(&gm_serial->mutex);
    if (gm_serial->fd < 0)
    {
        pthread_mutex_unlock(&gm_serial->mutex);

        return -4;
    }

    memset(recv_data, 0, recv_data_len);

    // 指定fds数组中的项目数
    nfds_t nfds = 1;
    // 指定要监视的文件描述符集
    struct pollfd fds[1] = {0};
    // 已读取数据长度
    ssize_t total_data_len = 0;
    // 未读取数据长度
    size_t remain_data_len = recv_data_len;

    while (true)
    {
        // 设置需要监听的文件描述符
        memset(fds, 0, sizeof(fds));
        fds[0].fd = gm_serial->fd;
        fds[0].events = POLLIN;

        int ret = poll(fds, nfds, timeout_msec);
        // 返回负值，发生错误
        if (ret < 0)
        {
            pthread_mutex_unlock(&gm_serial->mutex);

            return -5;
        }
        // 返回 0，超时
        else if (ret == 0)
        {
            pthread_mutex_unlock(&gm_serial->mutex);

            // 如果超时后，已读取数据长度大于 0，返回实际接收数据长度
            if (total_data_len > 0)
            {
                return total_data_len;
            }

            return 0;
        }
        // 返回值大于 0，成功
        else
        {
            // 判断是否是期望的返回
            if (fds[0].revents & POLLIN)
            {
                // 读取数据
                ret = read(fds[0].fd, &recv_data[total_data_len], remain_data_len);
                if (ret < 0)
                {
                    pthread_mutex_unlock(&gm_serial->mutex);

                    return -6;
                }

                // 计算已读取数据长度
                total_data_len += ret;
                // 计算剩余需要读取长度
                remain_data_len = (recv_data_len - total_data_len);
                // 读取完毕
                if (total_data_len == recv_data_len)
                {
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&gm_serial->mutex);

    return total_data_len;
}
