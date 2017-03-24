#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"
#include "conf_spi_example.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Chip select. */
#define SPI_CHIP_SEL 0
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)

/* Clock specifications. */
#define SPI_CLK_POLARITY 0   //Polarity
#define SPI_CLK_PHASE 0      //Phase

/* Delay before SPCK. */
#define SPI_DLYBS 0x40

/* Delay between consecutive transfers. */
#define SPI_DLYBCT 0x10

/* SPI slave states for this example. */
#define SLAVE_STATE_IDLE           0
#define SLAVE_STATE_TEST           1
#define SLAVE_STATE_DATA           2
#define SLAVE_STATE_STATUS_ENTRY   3
#define SLAVE_STATE_STATUS         4
#define SLAVE_STATE_END            5

/* SPI example commands for this example. */
/* slave test state, begin to return RC_RDY. */
#define CMD_TEST     0x10101010

/* Slave data state, begin to return last data block. */
#define CMD_DATA     0x29380000

/* Slave status state, begin to return RC_RDY + RC_STATUS. */
#define CMD_STATUS   0x68390384

/* Slave idle state, begin to return RC_SYN. */
#define CMD_END      0x68390484

/* General return value. */
#define RC_SYN       0x55AA55AA

/* Ready status. */
#define RC_RDY       0x12345678

/* Slave data mask. */
#define CMD_DATA_MSK 0xFFFF0000

/* Slave data block mask. */
#define DATA_BLOCK_MSK 0x0000FFFF

/* Number of commands logged in status. */
#define NB_STATUS_CMD   20

/* Number of SPI clock configurations. */
#define NUM_SPCK_CONFIGURATIONS 4

/* SPI Communicate buffer size. */
#define COMM_BUFFER_SIZE   64

/* UART baudrate. */
#define UART_BAUDRATE      115200

/* Data block number. */
#define MAX_DATA_BLOCK_NUMBER  4

/* Max retry times. */
#define MAX_RETRY    4

#define STRING_EOL    "\r"
#define STRING_HEADER "--Spi Example --\r\n" \
		"-- "BOARD_NAME" --\r\n" \
		"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

#define STANDARD_FIFO_ADDRESS 0x3F
#define DIRECT_FIFO_ADDRESS 0x3E
#define SPI_BURST 0x40
#define SPI_WRITE 0x00
#define SPI_READ 0x80
#define CHIP_RDY 0x80

/* Status block. */
struct status_block_t {
	/** Number of data blocks. */
	uint32_t ul_total_block_number;
	/** Number of SPI commands (including data blocks). */
	uint32_t ul_total_command_number;
	/** Command list. */
	uint32_t ul_cmd_list[NB_STATUS_CMD];
};

/* SPI clock setting (Hz). */
uint32_t gs_ul_spi_clock = 500000;

/* Current SPI return code. */
uint32_t gs_ul_spi_cmd = RC_SYN;

/* Current SPI state. */
uint32_t gs_ul_spi_state = 0;

/* 64 bytes data buffer for SPI transfer and receive. */
uint8_t gs_uc_spi_buffer[COMM_BUFFER_SIZE];

/* Pointer to transfer buffer. */
uint8_t *gs_puc_transfer_buffer;

/* Transfer buffer index. */
uint32_t gs_ul_transfer_index;

/* Transfer buffer length. */
uint32_t gs_ul_transfer_length;

/* SPI Status. */
struct status_block_t gs_spi_status;

uint32_t gs_ul_test_block_number;

/* SPI clock configuration. */
const uint32_t gs_ul_clock_configurations[] =
		{ 500000, 1000000, 2000000, 5000000 };
		
const char *morse[36] = { "10111", "111010101", "11101011101", "1110101",
			     "1", "101011101", "111011101", "1010101",
			     "101", "1011101110111", "111010111", "101110101",
			     "1110111", "11101", "11101110111", "10111011101",
			     "1110111010111", "1011101", "10101", "111",
			     "1010111", "101010111", "101110111", "11101010111",
			     "1110101110111",  "11101110101", "1110111011101110111", "10111011101110111", 
			     "101011101110111", "1010101110111", "10101010111", "101010101",
			     "11101010101", "1110111010101", "111011101110101", "11101110111011101" };

const char *space = "000";
const char* test = "ABH29";

char *generate_morse_code(const char *data);
void delay();
void spi_master_initialize();
void spi_master_transfer(void *, uint32_t);
void spi_master_go();

int main() {
	sysclk_init();
	board_init();
	spi_master_go();
	return 0;
}

void spi_master_go() {
	uint32_t data;
	//uint32_t i;
	spi_master_initialize();
	char *morse_code = generate_morse_code(test);
	while(1) {
		data = (SPI_WRITE | SPI_BURST) | STANDARD_FIFO_ADDRESS;
		spi_master_transfer(&data, sizeof(data));
		data &= CHIP_RDY;
		if(data) {
			delay();
		}
		else {
			break;
		}
	}
	spi_master_transfer(morse_code, strlen(morse_code));
	spi_disable_clock(SPI_MASTER_BASE);
	delay();
	spi_enable_clock(SPI_MASTER_BASE);
	delay();
	while(1) {
		data = 0x35;
		spi_master_transfer(&data, sizeof(data));
		if(data & 0x20) 
			break;
	}
	
	/*data = 0x7F; //Start writing in Standard FIFO mode i. e. to the TX FIFO
	while(1) {
		spi_master_transfer(&data, sizeof(data));
		if(!(data & 0x80)) {
			for(i = 0; i < 1000000; i++);	//Wait for the chip to get ready.
		}
		else {
			break;
		}
	}
	data = 0xAA;	//Data to be stored.
	spi_master_transfer(&data, sizeof(data));
	spi_disable_clock(SPI_MASTER_BASE); //Stop burst transfer.
	for(i = 0; i < 1000000; i++);
	spi_enable_clock(SPI_MASTER_BASE);
	for(i = 0; i < 1000000; i++);
	data = 0x36;	//Enter into IDLE state.
	while(1) {
		spi_master_transfer(&data, sizeof(data));
		if(!(data & 0x80)) {
			for(i = 0; i < 1000000; i++);
		}
		else {
			break;
		}
	}
	data = 0x35;	//Start TX mode.
	spi_master_transfer(&data, sizeof(data));*/
}

void spi_master_initialize() {
	/* Configure an SPI peripheral. */
	spi_enable_clock(SPI_MASTER_BASE);
	spi_disable(SPI_MASTER_BASE);
	spi_reset(SPI_MASTER_BASE);
	spi_set_lastxfer(SPI_MASTER_BASE);
	spi_set_master_mode(SPI_MASTER_BASE);
	spi_disable_mode_fault_detect(SPI_MASTER_BASE);
	spi_set_peripheral_chip_select_value(SPI_MASTER_BASE, SPI_CHIP_PCS);
	spi_set_clock_polarity(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	spi_set_clock_phase(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_PHASE);
	spi_set_bits_per_transfer(SPI_MASTER_BASE, SPI_CHIP_SEL,
			SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL, (sysclk_get_peripheral_hz() / gs_ul_spi_clock));
	spi_set_transfer_delay(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_DLYBS, SPI_DLYBCT);
	spi_enable(SPI_MASTER_BASE);
}

char *generate_morse_code(const char *data) {
	uint32_t len = strlen(data);
	char *morse_code = (char *)malloc(sizeof(char) * 128);
	morse_code[1] = '\0';
	uint32_t i;
	if(len > 5) {
		free(morse_code);
		return NULL;
	}
	for(i = 0; i < len; i++) {
		if(isdigit(data[i])) 
			strcat(morse_code, morse[(int)data[i] - 48 + 26]);
		else 
			strcat(morse_code, morse[(int)data[i] - 65]);
		if(i < (len - 1))
			strcat(morse_code, space);
	} 
	return morse_code;
}

void spi_master_transfer(void *p_buf, uint32_t size) {
	uint32_t i;
	uint8_t uc_pcs;
	static uint16_t data;

	uint8_t *p_buffer;

	p_buffer = p_buf;

	for (i = 0; i < size; i++) {
		spi_write(SPI_MASTER_BASE, p_buffer[i], 0, 0);
		/* Wait transfer done. */
		while ((spi_read_status(SPI_MASTER_BASE) & SPI_SR_RDRF) == 0);
		spi_read(SPI_MASTER_BASE, &data, &uc_pcs);
		p_buffer[i] = data;
	}
}
	
void delay() {
	int i;
	for(i = 0; i < 10000; i++);
}
