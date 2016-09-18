// Copyright (c) 2016 Steven Bacon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// File Name: spidev_led_matrix.c
//

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int spi_fd = 0;
static struct spi_ioc_transfer spi_transfer_buffer;

#define SPI_CMD_NULL            0
#define SPI_CMD_CLEAR           1
#define SPI_CMD_FILL            2
#define SPI_CMD_UPDATE          3
#define SPI_CMD_SETPIXEL        4
#define SPI_CMD_SETNPIXELS      5
#define SPI_CMD_SMALL_EMPTY     6 // A test command
#define SPI_RESPONSE_ACK        0x55
#define SPI_RESPONSE_NACK_HEAD  0xaa
#define SPI_RESPONSE_NACK_TAIL  0xab
#define SPI_RESPONSE_NACK_UNK   0xac

//#define DEBUG_SPI

static void pabort(const char *s)
{
    perror(s);
    abort();
}

int spi_trx(uint8_t* tx, uint8_t* rx, uint8_t len)
{
    int ret;

    spi_transfer_buffer.tx_buf = (unsigned long)tx;
    spi_transfer_buffer.rx_buf = (unsigned long)rx;
    spi_transfer_buffer.len = len;

    ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer_buffer);
    if (ret < 1)
        pabort("can't send spi message");

    // The last byte received should be the ack.
    return rx[len-1];
}

void dump_spi_buffers(uint8_t* tx, uint8_t* rx, uint8_t len)
{
    int i;
    //printf("%s\n", __func__);

    printf("tx: ");
    for(i=0; i<len; i++)
        printf("0x%02x ", tx[i]);

    printf("\n");

    printf("rx: ");
    for(i=0; i<len; i++)
        printf("0x%02x ", rx[i]);

    printf("\n");
}

int led_cmd_clear(void)
{
    int ret;
    uint8_t tx[] = {
        SPI_CMD_CLEAR,
        (SPI_CMD_CLEAR ^ 0xff),
        0,
        0};
    uint8_t rx[ARRAY_SIZE(tx)];
    memset(rx, 0xcc, ARRAY_SIZE(rx));

    ret = spi_trx(tx, rx, ARRAY_SIZE(tx));
    dump_spi_buffers(tx, rx, ARRAY_SIZE(tx));
    return ret;
}

int led_cmd_fill(uint8_t r, uint8_t g, uint8_t b)
{
    int ret;
    uint8_t tx[] = {
        SPI_CMD_FILL,
        (SPI_CMD_FILL ^ 0xff),
        r,g,b,
        0};
    uint8_t rx[ARRAY_SIZE(tx)];
    memset(rx, 0xcc, ARRAY_SIZE(rx));

    ret = spi_trx(tx, rx, ARRAY_SIZE(tx));
    dump_spi_buffers(tx, rx, ARRAY_SIZE(tx));
    return ret;
}

int led_cmd_update(void)
{
    int ret;
    uint8_t tx[] = {
        SPI_CMD_UPDATE,
        (SPI_CMD_UPDATE ^ 0xff),
        0,
        0};
    uint8_t rx[ARRAY_SIZE(tx)];
    memset(rx, 0xcc, ARRAY_SIZE(rx));

    ret = spi_trx(tx, rx, ARRAY_SIZE(tx));
    dump_spi_buffers(tx, rx, ARRAY_SIZE(tx));
    return ret;

}

int led_cmd_set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
    int ret;
    uint8_t tx[] = {
        SPI_CMD_SETPIXEL,
        (SPI_CMD_SETPIXEL ^ 0xff),
        x, y, r, g, b,
        0};
    uint8_t rx[ARRAY_SIZE(tx)];
    memset(rx, 0xcc, ARRAY_SIZE(rx));

    ret = spi_trx(tx, rx, ARRAY_SIZE(tx));
    dump_spi_buffers(tx, rx, ARRAY_SIZE(tx));
    return ret;
}

int led_cmd_small_empty(void)
{
    int ret;
    uint8_t tx[] = {
        SPI_CMD_SMALL_EMPTY,
        (SPI_CMD_SMALL_EMPTY ^ 0xff),
        0,
        0};
    uint8_t rx[ARRAY_SIZE(tx)];
    memset(rx, 0xcc, ARRAY_SIZE(rx));

    ret = spi_trx(tx, rx, ARRAY_SIZE(tx));
    dump_spi_buffers(tx, rx, ARRAY_SIZE(tx));
    return ret;
}

void spi_init(void)
{
    int ret = 0;
    char *device = "/dev/spidev0.0";
    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 40000; //LED commands. Good speed when using faster loops for fill
    uint16_t delay = 0;

    spi_fd = open(device, O_RDWR);
    if (spi_fd < 0)
    {
        fprintf(stderr, "can't open device %s \n", device);
        abort();
    }

    // spi mode
    ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
        pabort("can't set spi mode");

    ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
        pabort("can't get spi mode");

    // bits per word
    ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't set bits per word");

    ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't get bits per word");

    // max speed hz
    ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't set max speed hz");

    ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't get max speed hz");

    spi_transfer_buffer.tx_buf          = 0;
    spi_transfer_buffer.rx_buf          = 0;
    spi_transfer_buffer.len             = 0;
    spi_transfer_buffer.delay_usecs     = delay;
    spi_transfer_buffer.speed_hz        = speed;
    spi_transfer_buffer.bits_per_word   = bits;

#ifdef DEBUG_SPI
    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
#endif // DEBUG_SPI
}

void spi_fini(void)
{
    close(spi_fd);
}

void print_usage(void)
{
    printf("Usage:\n");
    printf( "    -c                     clear\n"
            "    -u                     update\n"
            "    -f 0xrr:0xgg:0xbb      fill\n"
            "    -s x:y:0xrr:0xgg:0xbb  set pixel\n"
            );
}

void parse_opts(int argc, char* argv[])
{
    int ret;
    int processing_args = 1;
    int r = 0, g = 0, b = 0, x = 0, y = 0;

    while(processing_args)
    {
        ret = getopt(argc, argv, "cuf:s:");

        if(ret == -1)
        {
            // Parsed all args returning
            break;
        }

        switch(ret)
        {
            case 'c':
                printf("clear\n");
                led_cmd_clear();
                break;
            case 'u':
                printf("update\n");
                led_cmd_update();
                break;
            case 'f':
                if(3 != sscanf(optarg,"0x%2x:0x%2x:0x%2x", &r, &g, &b))
                {
                    printf("fill failed to parse (%s)\n", optarg);
                }
                else
                {
                    printf("fill r:0x%02x g:0x%02x b:0x%02x\n", r, g, b);
                    led_cmd_fill(r, g, b);
                }
                break;
            case 's':
                if(5 != sscanf(optarg, "%d:%d:0x%2x:0x%2x:0x%2x", &x, &y, &r, &g, &b))
                {
                    printf("set pixel failed to parse (%s)\n", optarg);
                }
                else
                {
                    printf("set pixel x:0x%02x y:0x%02x r:0x%02x g:0x%02x b:0x%02x\n",
                            x, y, r, g, b);
                    led_cmd_set_pixel(x, y, r, g, b);
                }
                break;
            default:
                print_usage();
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    spi_init();
    parse_opts(argc, argv);
    spi_fini();

    return 0;
}

