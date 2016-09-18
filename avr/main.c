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
// File Name: main.c
//
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#define LED_ON      PORTB |=  (1 << PB0)
#define LED_OFF     PORTB &= ~(1 << PB0)
#define LED_TOGGLE  PORTB ^=  (1 << PB0)

#define RADIO_TX_HI PORTB |= (1 << PB1)
#define RADIO_TX_LO PORTB &= ~(1 << PB1)

// /usr/lib/avr/include/avr

//      Atmega328p pinout
//          ---------
//      PC6 | 1   28| PC5   -> to LED string
//  RXD PD0 | 2   27| PC4
//  TXD PD1 | 3   26| PC3
//      PD2 | 4   25| PC2
//      PD3 | 5   24| PC1
//      PD4 | 6   23| PC0
//      VCC | 7   22| GND
//      GND | 8   21| AREF
//      PB6 | 9   20| AVCC
//      PB7 |10   19| PB5   (SCK  - spi clock)
//      PD5 |11   18| PB4   (MISO - SPI master input / slave output)
//      PD6 |12   17| PB3   (MOSI - SPI master output / slave input)
//      PD7 |13   16| PB2   (SS   - SPI slave select)
//      PB0 |14   15| PB1
//          ---------

// RPi wiring
// Atmega       RPi             Wire colour
// 19 SCK  ---  23 SCLK         Grn
// 18 MISO ---  21 MISO         Yel
// 17 MOSI ---  19 MOSI         Org
// 16 SS   ---  24 CEO_N        Red
// -  GND  ---  25 GND          Brn

// TODO
// ====
// - Reset current command position after a timeout, maybe we could use the CS line for this.
// - To save space could union a bunch of structures for each command

// Commands
// ========
// Ack = 0x55
// Nack = 0xaa
// 1) Clear (Set all LED to off)
// MOSI | CMD (0x01) | ~CMD (0xfe) |     0    |
// MISO |  0         |   0         | Ack/Nack |
// 2) Fill (Set all LEDs to a given value)
// MOSI | CMD (0x02) | ~CMD (0xfd) | R | G | B |     0    |
// MISO |  0         |   0         | 0 | 0 | 0 | Ack/Nack |
// 3) Update (Update the LED string with the current values)
// MOSI | CMD (0x03) | ~CMD (0xfc) |     0    |
// MISO |  0         |   0         | Ack/Nack |
// 4) SetPixel (Set LED at x,y to a given colour)
// MOSI | CMD (0x04) | ~CMD (0xfb) | X | Y | R | G | B |     0    |
// MISO |  0         |   0         | 0 | 0 | 0 | 0 | 0 | Ack/Nack |
// 5) SetNPixels (Set N lots of Pixel Data, starting at x,y and wrapping)
//                                             |<--------->|*N
// MOSI | CMD (0x05) | ~CMD (0xfa) | N | X | Y | R | G | B |      0   |
// MISO |  0         |   0         | 0 | 0 | 0 | 0 | 0 | 0 | Ack/Nack |

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
#define SPI_RESPONSE_TIMEOUT    0x44

typedef enum {
    e_new_cmd,              // Waiting for a new command
    e_cmd_byte,             // Got the first cmd byte
    e_inv_cmd_byte,         // Got the inv cmd byte, carry on with the rest
    e_ack_request           // The transmitter is expecting an ack.
} e_cmd_state;

typedef enum{
    e_error,
    e_complete,
    e_processing
} e_cmd_ret;


#define BYTES_PER_LED   3
#define LEDS_WIDE       10
#define LEDS_HIGH       6
#define LED_COUNT       (LEDS_WIDE * LEDS_HIGH)
#define LED_DATA_SIZE   (BYTES_PER_LED * LED_COUNT)

typedef struct
{
    uint8_t g;
    uint8_t r;
    uint8_t b;
}t_pixel;

volatile char led_data[LED_DATA_SIZE];
static volatile uint8_t spi_byte_count = 0;
static volatile uint8_t last_spi_byte = 0;
static uint8_t processed_byte_count = 0;
static uint8_t cmd_byte = 0;
static uint8_t inv_cmd_byte = 0;
static uint8_t response_byte = 0;
static e_cmd_state  current_state = e_new_cmd;
static update_pending = 0;

void init(void)
{
    PORTB = 0x00;
    // PB0, PB1 output
    DDRB |= (1 << DDB0);
    DDRB |= (1 << DDB1);

    // PC5 output
    DDRC |= (1 << DDC5);

    //Clock prescale register
    CLKPR = 0x0; //clk_io = F_CPU (no scaling)
}

// 1mS timer
// 20Mhz, /1024, *20.
void timer_init(void)
{
    // Clear timer on compare match (CTC) mode.
    // 8-bit timer.
    // WGM02:0 = 010
    TCCR0A = (1 << WGM01);
    // Clock select /1024
    // CS02:0 = 101
    TCCR0B = (1 << CS02) | (1 << CS00);
    OCR0A = 20;
    // Output compare match A interrupt enable
    TIMSK0 = (1 << OCIE0A);
}

void spi_slave_init()
{
    // PB[5,3,2] input
    DDRB &= ~((1 << DDB5) | (1 << DDB3) | (1 << DDB2));
    // PB[4] output
    DDRB |= (1 << DDB4);

    // SPI control register
    // Inerrupt enabled, SPI enable, data order MSB first, CPOL=0, CPHA=0
    // clock rate has no effect when slave
    //SPCR = (1 << SPE) | (1 << SPIE);
    // Polled version (no interrupt)
    SPCR = (1 << SPE);

    // When we receive the first byte from the master the outgoing byte should be zero.
    SPDR = 0xff;
}

void set_pix_xy(uint8_t x, uint8_t y, t_pixel* colour)
{
    t_pixel* pix = (t_pixel*)led_data;

    // As the strips are connected like this:
    // |----------
    // ^---------|
    // |---------^
    // ^
    // etc

    // Odd
    if(y & 0x1)
    {
        pix[y*LEDS_WIDE + (x)] = *colour;
    }
    else
    {
        pix[y*LEDS_WIDE + (LEDS_WIDE - 1 - x)] = *colour;
    }
}

e_cmd_ret led_cmd_clear(void)
{
    uint8_t i;
    uint16_t rrgg = 0;
    uint16_t bbrr = 0;
    uint16_t ggbb = 0;
    uint16_t* ptr = (uint16_t*)led_data;

    for(i=0; i<(LED_DATA_SIZE/6); i++)
    {
        *(ptr++) = rrgg;
        *(ptr++) = bbrr;
        *(ptr++) = ggbb;
    }

    return e_complete;
}

e_cmd_ret led_cmd_small_empty(void)
{
    return e_complete;
}

e_cmd_ret led_cmd_update(void)
{
    update_pending = 1;
    return e_complete;
}

e_cmd_ret led_cmd_fill(uint8_t next_byte, uint8_t following)
{
    static t_pixel p = {0,0,0};
    t_pixel* pix = (t_pixel*)led_data;
    uint8_t i;
    uint16_t rrgg;
    uint16_t bbrr;
    uint16_t ggbb;
    uint16_t* ptr = (uint16_t*)led_data;

    switch(following)
    {
        case 0:
            p.r = next_byte;
            return e_processing;
        case 1:
            p.g = next_byte;
            return e_processing;
        case 2:
            {
                p.b = next_byte;
                //TODO check byte order
                rrgg = (p.r << 8) | p.g;
                bbrr = (p.b << 8) | p.r;
                ggbb = (p.g << 8) | p.b;
                for(i=0; i<(LED_DATA_SIZE/6); i++)
                {
                    *(ptr++) = rrgg;
                    *(ptr++) = bbrr;
                    *(ptr++) = ggbb;
                }
            }
            return e_complete;
    }
    return e_error;
}

e_cmd_ret led_cmd_set_pixel(uint8_t next_byte, uint8_t following)
{
    static uint8_t x=0, y=0;
    static t_pixel p = {0,0,0};

    switch(following)
    {
        case 0:
            x = next_byte;
            return e_processing;
        case 1:
            y = next_byte;
            return e_processing;
        case 2:
            p.r = next_byte;
            return e_processing;
        case 3:
            p.g = next_byte;
            return e_processing;
        case 4:
            p.b = next_byte;
            set_pix_xy(x, y, &p);
            return e_complete;
    }
    return e_error;
}

volatile uint8_t ms_count = 0;

ISR (TIMER0_COMPA_vect, ISR_BLOCK)
{
    ms_count++;
}

void spi_slave_command_state_machine_loop(void)
{
    uint8_t after_cmd_count = 0;
    e_cmd_ret ret;
    uint8_t last_byte_time = ms_count;

    LED_ON;
    _delay_ms(1000);
    LED_OFF;

    //Global enable interrupts
    sei();

    while(1)
    {
        // Polled SPI
        if(SPSR & (1 << SPIF))
        {
            last_spi_byte = SPDR;
            ++spi_byte_count;
        }

        // If it's been more than 2mS between bytes then we need to reset
        // the state machine.
        if(ms_count > (last_byte_time + 2))
        {
            current_state = e_new_cmd;
            response_byte = 0;
            SPDR = SPI_RESPONSE_TIMEOUT;
        }

        // if we have a new byte to process
        if(processed_byte_count != spi_byte_count)
        {
            // Reset the time since last byte received
            last_byte_time = ms_count;

            switch(current_state)
            {
                case e_new_cmd:
                    response_byte = 1;
                    after_cmd_count = 0;
                    cmd_byte = last_spi_byte;
                    current_state = e_cmd_byte;
                    break;
                case e_cmd_byte:
                    inv_cmd_byte = last_spi_byte;
                    if(inv_cmd_byte == (cmd_byte ^ 0xff))
                    {
                        response_byte = 2;
                        current_state = e_inv_cmd_byte;
                    }
                    else
                    {
                        response_byte = SPI_RESPONSE_NACK_HEAD;
                        current_state = e_new_cmd;
                    }
                    break;
                case e_inv_cmd_byte:
                    {
                        response_byte = 3;
                        switch(cmd_byte)
                        {
                            case SPI_CMD_CLEAR:
                                ret = led_cmd_clear();
                                break;
                            case SPI_CMD_FILL:
                                ret = led_cmd_fill(last_spi_byte, after_cmd_count);
                                break;
                            case SPI_CMD_UPDATE:
                                ret = led_cmd_update();
                                break;
                            case SPI_CMD_SETPIXEL:
                                ret = led_cmd_set_pixel(last_spi_byte, after_cmd_count);
                                break;
                            case SPI_CMD_SMALL_EMPTY:
                                ret = led_cmd_small_empty();
                                break;
                            //case SPI_CMD_SETNPIXELS: break;
                            default:
                                ret = e_error;
                                break;
                        }

                        response_byte = after_cmd_count;
                        after_cmd_count++;

                        if(ret == e_error)
                        {
                            current_state = e_ack_request;
                            response_byte = SPI_RESPONSE_NACK_TAIL;
                        }
                        else if(ret == e_complete)
                        {
                            current_state = e_ack_request;
                            response_byte = SPI_RESPONSE_ACK;
                        }
                        // else processing, the led_cmd_func is expending more
                        // data so carry on as we are.

                    }
                    break;
                case e_ack_request:
                    // The response byte has been setup so just progress
                    // the state machine.
                    current_state = e_new_cmd;
                    break;
                default:
                    current_state = e_new_cmd;
                    response_byte = SPI_RESPONSE_NACK_UNK;
                    break;
            }
            processed_byte_count++;
            SPDR = response_byte;

            if(update_pending)
            {
                // Clear interrupts when sending LED data.
                cli();
                asm_send_led_data(led_data);
                // re-enable interrupts
                sei();
                update_pending = 0;
            }
        }
    } // Main loop
}

int main(void)
{
    init();
    spi_slave_init();
    timer_init();
    spi_slave_command_state_machine_loop();
}

