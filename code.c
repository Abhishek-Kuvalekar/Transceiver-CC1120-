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

void spi_master_go() {
	uint32_t data;
	uint32_t i;

int main() {
	sysclk_init();
	board_init();
	spi_master_go();
	return 0;
}
