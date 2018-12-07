/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "common.h"
#include "hardware.h"
#include "arch/at91_ccfg.h"
#include "arch/at91_matrix.h"
#include "arch/at91_rstc.h"
#include "arch/at91_pmc.h"
#include "arch/at91_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_sdramc.h"
#include "spi.h"
#include "gpio.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "sdramc.h"
#include "timer.h"
#include "watchdog.h"
#include "sam9_l9260.h"

static void initialize_dbgu(void)
{
	/* Configure DBGU pin */
	/* {"RXD", AT91C_PIN_PB(14), 0, PIO_DEFAULT, PIO_PERIPH_A}, */
	/* {"TXD", AT91C_PIN_PB(15), 0, PIO_DEFAULT, PIO_PERIPH_A}, */
	writel(((0x01 << 14) | (0x01 << 15)), AT91C_BASE_PIOB + PIO_ASR);
	writel(((0x01 << 14) | (0x01 << 15)), AT91C_BASE_PIOB + PIO_PDR);

	pmc_enable_periph_clock(AT91C_ID_PIOB);

	usart_init(BAUDRATE(MASTER_CLOCK, 115200));
}

#ifdef CONFIG_SDRAM
static void sdramc_init(void)
{
	struct sdramc_register sdramc_config;

	sdramc_config.cr = AT91C_SDRAMC_NC_9
		| AT91C_SDRAMC_NR_13 | AT91C_SDRAMC_CAS_2
		| AT91C_SDRAMC_NB_4_BANKS | AT91C_SDRAMC_DBW_32_BITS
		| AT91C_SDRAMC_TWR_2 | AT91C_SDRAMC_TRC_7
		| AT91C_SDRAMC_TRP_2 | AT91C_SDRAMC_TRCD_2
		| AT91C_SDRAMC_TRAS_5 | AT91C_SDRAMC_TXSR_8;

	sdramc_config.tr = (MASTER_CLOCK * 7) / 1000000;
	sdramc_config.mdr = AT91C_SDRAMC_MD_SDRAM;

	/* configure sdramc pins */
	writel(0xFFFF0000, AT91C_BASE_PIOC + PIO_ASR);
	writel(0xFFFF0000, AT91C_BASE_PIOC + PIO_PDR);

	pmc_enable_periph_clock(AT91C_ID_PIOC);

	/* Initialize the matrix (memory voltage = 3.3) */
	writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) | AT91C_EBI_CS1A_SDRAMC,
			AT91C_BASE_CCFG + CCFG_EBICSA);

	sdramc_initialize(&sdramc_config, AT91C_BASE_CS1);
}
#endif  /* #ifdef CONFIG_SDRAM */

#ifdef CONFIG_HW_INIT
void hw_init(void)
{
	/* Disable watchdog */
	at91_disable_wdt();

	/*
	 * At this stage the main oscillator is supposed to be enabled
	 * PCK = MCK = MOSC
	 */
	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	pmc_cfg_plla(PLLA_SETTINGS);

	/* PCK = PLLA = 2 * MCK */
	pmc_cfg_mck(MCKR_SETTINGS);

	/* Switch MCK on PLLA output */
	pmc_cfg_mck(MCKR_CSS_SETTINGS);

	/* Enable External Reset */
	writel(AT91C_RSTC_KEY_UNLOCK | AT91C_RSTC_URSTEN, AT91C_BASE_RSTC + RSTC_RMR);

	/* Initialize matrix */
	writel((readl(AT91C_BASE_MATRIX + MATRIX_SCFG3) & (~AT91C_MATRIX_SLOT_CYCLE))
			| AT91C_MATRIX_SLOT_CYCLE_(0x40),
			AT91C_BASE_MATRIX + MATRIX_SCFG3);

	/* Init timer */
	timer_init();

	/* Initialize dbgu */
	initialize_dbgu();

#ifdef CONFIG_SDRAM
	/* Initlialize sdram controller */
	sdramc_init();
#endif

}
#endif /* #ifdef CONFIG_HW_INIT */

#ifdef CONFIG_DATAFLASH
void at91_spi0_hw_init(void)
{
	/* Configure the spi0 pins */
	/* {"MISO",	AT91C_PIN_PA(0),	0, PIO_DEFAULT, PIO_PERIPH_A}
	   {"MOSI",	AT91C_PIN_PA(1),	0, PIO_DEFAULT, PIO_PERIPH_A}
	   {"SPCK",	AT91C_PIN_PA(2),	0, PIO_DEFAULT, PIO_PERIPH_A}*/
	writel(((0x01 << 0) | (0x01 << 1) | (0x01 << 2)),
					AT91C_BASE_PIOA + PIO_ASR);
	writel(((0x01 << 0) | (0x01 << 1) | (0x01 << 2)),
					AT91C_BASE_PIOA + PIO_PDR);

	/* {"NPCS",        AT91C_PIN_PC(11),     1, PIO_PULLUP, PIO_OUTPUT}, */
	writel((0x01 << 11), AT91C_BASE_PIOC + PIO_IDR);
	writel((0x01 << 11), AT91C_BASE_PIOC + PIO_PPUDR);
	writel((0x01 << 11), AT91C_BASE_PIOC + PIO_SODR);
	writel((0x01 << 11), AT91C_BASE_PIOC + PIO_OER);
	writel((0x01 << 11), AT91C_BASE_PIOC + PIO_PER);

	pmc_enable_periph_clock(AT91C_ID_PIOA);
	pmc_enable_periph_clock(AT91C_ID_PIOC);

	/* Enable the spi0 clock */
	pmc_enable_periph_clock(AT91C_ID_SPI0);
}
#endif /* #ifdef CONFIG_DATAFLASH */
