// license:BSD-3-Clause
// copyright-holders:David Haywood
#ifndef MAME_MACHINE_68340DMA_H
#define MAME_MACHINE_68340DMA_H

#pragma once

class m68340_cpu_device;

class mc68340_dma_module_device : public device_t
{
	friend class m68340_cpu_device;

public:
	mc68340_dma_module_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

	uint16_t read(offs_t offset, uint16_t mem_mask = ~0);
	void write(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint8_t irq_level(uint8_t id) const { return m_irqout[id] ? (m_intl[id]) : 0; }
	// uint8_t irq_level(uint8_t id) const { return m_intl[id]; }
	uint8_t irq_vector(uint8_t id) const { return m_intv[id]; }
	uint8_t arbitrate(uint8_t id, uint8_t level) const { return (irq_level(id) == level) ? m_iarb : 0; }
	uint16_t dma_register_read(uint8_t id, offs_t offset, uint16_t mem_mask);
	void dma_register_write(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask);

	void module_reset();


protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	m68340_cpu_device *m_cpu;

	devcb_write_line::array<2> m_dack_out_cb;

	devcb_write8::array<2> m_done_out_cb;

	devcb_read8::array<2> m_done_in_cb;

	int m_dack[2];
	int m_dreq[2];
	int m_irqout[2];
	uint8_t m_irq[2];
	uint8_t m_done[2];
	uint8_t m_bes[2];
	uint8_t m_bed[2];
	uint8_t m_conf[2];
	uint8_t m_brkp[2];

	uint8_t m_csr[2];
	uint8_t m_dapi[2];
	uint8_t m_dmadfc[2];
	uint8_t m_dmasfc[2];
	uint8_t m_dsize[2];
	uint8_t m_eco[2];
	uint8_t m_fcr[2];
	uint8_t m_frz0[2];
	uint8_t m_frz1[2];
	uint8_t m_iarb;
	uint8_t m_imb[2];
	uint8_t m_imr[2];
	uint8_t m_intb[2];
	uint8_t m_inte[2];
	uint8_t m_intl[2];
	uint8_t m_intn[2];
	uint8_t m_intv[2];
	uint8_t m_ism[2];
	uint8_t m_maid[2];
	uint8_t m_req[2];
	uint8_t m_s_d[2];
	uint8_t m_sapi[2];
	uint8_t m_se[2];
	uint8_t m_ssize[2];
	uint8_t m_stp;
	uint8_t m_sd[2];
	uint8_t m_str[2];
	uint8_t m_supv[2];

	uint16_t m_ccr[2];
	uint16_t m_mcr[2];

	uint32_t m_btc[2];
	uint32_t m_dar[2];
	uint32_t m_sar[2];

	uint8_t m_dhr[2][4];

	uint16_t m_bb[2];

	uint16_t m_btcle[2];
	uint16_t m_btche[2];

	uint16_t m_darle[2];
	uint16_t m_darhe[2];

	uint16_t m_sarle[2];
	uint16_t m_sarhe[2];

	void btc_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask);
	void ccr_w(uint8_t id, uint16_t data, uint16_t mem_mask);
	void csr_w(uint8_t id, uint8_t data, uint16_t mem_mask);
	void dar_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask);
	void dreq_w(uint8_t id, uint8_t state);

	void do_dma(uint8_t id);

	void update_irq(uint8_t id);
	void update_csr(uint8_t id);

	void fcr_w(uint8_t id, uint8_t data, uint16_t mem_mask);
	void intr_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask);
	void mcr_w(uint8_t id, uint16_t data, uint16_t mem_mask);
	void sar_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask);

	uint16_t intr_r(uint8_t id, offs_t offset);

	static uint8_t sizemode[];

	emu_timer *m_dma_timer[2];

	TIMER_CALLBACK_MEMBER( dma_finished );

};

DECLARE_DEVICE_TYPE(MC68340_DMA_MODULE, mc68340_dma_module_device)

#endif // MAME_MACHINE_68340DMA_H
