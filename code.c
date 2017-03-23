#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"
#include "conf_spi_example.h"

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

void spi_master_go() {
	uint32_t data;
	uint32_t i;
	spi_master_initialize();
	data = 0x7F; //Start writing in Standard FIFO mode i. e. to the TX FIFO
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
	spi_master_transfer(&data, sizeof(data));
}	

int main() {
	sysclk_init();
	board_init();
	spi_master_go();
	return 0;
}
