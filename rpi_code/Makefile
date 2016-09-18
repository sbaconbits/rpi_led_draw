

#$> source ~/svn/code/rpi/kernel/config_env.sh home_mini

ifndef CROSS_COMPILE
$(error CROSS_COMPILE is not set)
endif

C_OPTS=-Wall
CC=$(CROSS_COMPILE)gcc

all: spidev_led_matrix

spidev_led_matrix: spidev_led_matrix.c
	$(CC) $(C_OPTS) -o spidev_led_matrix spidev_led_matrix.c

clean:
	rm -f spidev_led_matrix

