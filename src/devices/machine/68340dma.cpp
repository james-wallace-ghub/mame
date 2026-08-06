// license:BSD-3-Clause
// copyright-holders:David Haywood
/* 68340 DMA module 

TODO: Implement beyond the Cobra3 mode
*/

#include "emu.h"
#include "68340.h"


#define LOG_SETUP   (1U << 1)
#define LOG_READ    (1U << 2)
#define LOG_DMA   (1U << 3)
#define LOG_INT     (1U << 4)
#define LOG_WRITE     (1U << 5)

//#define VERBOSE  (LOG_SETUP|LOG_INT|LOG_DMA|LOG_INT|LOG_WRITE|LOG_READ)

#include "logmacro.h"

#define LOGSETUP(...) LOGMASKED(LOG_SETUP, __VA_ARGS__)
#define LOGREAD(...)     LOGMASKED(LOG_READ,  __VA_ARGS__)
#define LOGDMA(...) LOGMASKED(LOG_DMA, __VA_ARGS__)
#define LOGINT(...)   LOGMASKED(LOG_INT,   __VA_ARGS__)
#define LOGWRITE(...)   LOGMASKED(LOG_WRITE,   __VA_ARGS__)

uint8_t mc68340_dma_module_device::sizemode[] =
{
	4, //longword
	1, //byte
	2, //word
	0  //unused
};


TIMER_CALLBACK_MEMBER(mc68340_dma_module_device::dma_finished)
{
	int const index = param;
	do_dma(index);
}

void mc68340_dma_module_device::ccr_w(uint8_t id, uint16_t data, uint16_t mem_mask)
{
	LOGWRITE("%08x CH %x Write to CCR with STP %x Data %08x\n", m_cpu->pcbase(), id+1, m_stp, data);
	COMBINE_DATA(&m_ccr[id]);

	if (!m_stp)
	{
		m_str[id] = (m_ccr[id] & 0x01);
		LOGWRITE("%08x CH %x Write to STR %x\n", m_cpu->pcbase(), id+1, m_str[id]);
		m_s_d[id] = (m_ccr[id] >> 1 & 0x01);
		LOGWRITE("%08x CH %x Write to S/D %x\n", m_cpu->pcbase(), id+1, m_s_d[id]);
		m_bb[id] = (256 + (m_ccr[id] >> 2 & 0x03) * 256);
		LOGWRITE("%08x CH %x Write to BB %08d\n", m_cpu->pcbase(), id+1, m_bb[id]);
		m_req[id] = (m_ccr[id] >> 4 & 0x02);
		LOGWRITE("%08x CH %x Write to REQ %x\n", m_cpu->pcbase(), id+1, m_req[id]);
		m_dsize[id] = sizemode[(m_ccr[id] >> 6 & 0x02)];
		LOGWRITE("%08x CH %x Write to DSIZE %08x\n", m_cpu->pcbase(), id+1, m_dsize[id]);
		m_ssize[id] = sizemode[(m_ccr[id] >> 8 & 0x02)];
		LOGWRITE("%08x CH %x Write to SSIZE %08x\n", m_cpu->pcbase(), id+1, m_ssize[id]);
		m_dapi[id] = (m_ccr[id] >> 10 & 0x01);
		LOGWRITE("%08x CH %x Write to DAPI %08x\n", m_cpu->pcbase(), id+1, m_dapi[id]);
		m_sapi[id] = (m_ccr[id] >> 11 & 0x01);
		LOGWRITE("%08x CH %x Write to SAPI %08x\n", m_cpu->pcbase(), id+1, m_sapi[id]);
		m_eco[id] = (m_ccr[id] >> 12 & 0x01);
		LOGWRITE("%08x CH %x Write to ECO %08x\n", m_cpu->pcbase(), id+1, m_eco[id]);
		m_inte[id] = (m_ccr[id] >> 13 & 0x01);
		LOGWRITE("%08x CH %x Write to INTE %08x\n", m_cpu->pcbase(), id+1, m_inte[id]);
		m_intn[id] = (m_ccr[id] >> 14 & 0x01);
		LOGWRITE("%08x CH %x Write to INTN %08x\n", m_cpu->pcbase(), id+1, m_intn[id]);
		m_intb[id] = (m_ccr[id] >> 15 & 0x01);
		LOGWRITE("%08x CH %x Write to INTB %08x\n", m_cpu->pcbase(), id+1, m_intb[id]);
	}	
}

void mc68340_dma_module_device::do_dma(uint8_t id)
{

	LOGWRITE("%08x CH %x DMA BTC %x \n", m_cpu->pcbase(), id, m_btc[id]);
	logerror("%08x CH %x DMA BTC %x \n", m_cpu->pcbase(), id, m_btc[id]);

	while (m_btc[id] > 0)
	{
		LOGWRITE("%08x CH %x BTC %x \n", m_cpu->pcbase(), id, m_btc[id]);
		logerror("%08x CH %x BTC %x \n", m_cpu->pcbase(), id, m_btc[id]);


		for (int i=0; i < m_ssize[id]; i++)
		{
			logerror("%08x CH %x SAR %x \n", m_cpu->pcbase(), id, m_sar[id] + i);
			m_dhr[id][i] = m_cpu->m_read8(m_sar[id] + i);
			logerror("%08x CH %x DHR %x \n", m_cpu->pcbase(), id, m_dhr[id][i]);
		}
		if (!m_sapi[id])
		{
			m_sar[id] += m_ssize[id];
		}

		m_btc[id] -= m_ssize[id];

		for (int i=0; i < m_dsize[id]; i++)
		{
			m_cpu->m_write8(m_dar[id]+i, m_dhr[id][i]);
		}

		if (!m_dapi[id])
		{
			m_dar[id] += m_dsize[id];
		}

	}
	m_done[id] = 1;
	update_irq(id);
}

void mc68340_dma_module_device::dreq_w(uint8_t id, uint8_t state)
{
	LOGWRITE("%08x CH %x DREQ Write  %08x \n", m_cpu->pcbase(), id, state);

	if (m_dreq[id] != state)
	{
		m_dreq[id] = state;
		if (!m_dreq[id])
		{
			LOGWRITE("%08x CH %x DMA \n", m_cpu->pcbase(), id);
			m_dack[id] = 0;
			m_dack_out_cb[id](0);
			do_dma(id);
		}
	}
}

void mc68340_dma_module_device::mcr_w(uint8_t id, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_mcr[id]);

	m_iarb = m_mcr[id] & 0x0f;
	LOGWRITE("%08x CH %x IARB Write  %08x \n", m_cpu->pcbase(), id, m_iarb);
	m_maid[id] = (m_mcr[id] >> 0x04) & 0x07;
	LOGWRITE("%08x CH %x MAID Write  %08x \n", m_cpu->pcbase(), id, m_maid[id]);
	m_supv[id] = m_mcr[id] & 0x80;
	LOGWRITE("%08x CH %x SUPV Write  %08x \n", m_cpu->pcbase(), id, m_supv[id]);
	m_ism[id] = (m_mcr[id] >> 0x08) & 0x07;
	LOGWRITE("%08x CH %x ISM Write  %08x \n", m_cpu->pcbase(), id, m_ism[id]);
	m_se[id] = (m_mcr[id] >> 0x0c) & 0x01;
	LOGWRITE("%08x CH %x SE Write  %08x \n", m_cpu->pcbase(), id, m_se[id]);
	m_frz0[id] = (m_mcr[id] >> 0x0d) & 0x01;
	LOGWRITE("%08x CH %x FRZ0 Write  %08x \n", m_cpu->pcbase(), id, m_frz0[id]);
	m_frz1[id] = (m_mcr[id] >> 0x0e) & 0x01;
	LOGWRITE("%08x CH %x FRZ1 Write  %08x \n", m_cpu->pcbase(), id, m_frz1[id]);
	m_stp = (m_mcr[id] >> 0x0f) & 0x01;
	LOGWRITE("%08x CH %x STP Write  %08x \n", m_cpu->pcbase(), id, m_stp);
}

void mc68340_dma_module_device::btc_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (offset == 0)
	{
		COMBINE_DATA(&m_btche[id]);
		LOGWRITE("%08x CH %x BTC HE Write  %04x \n", m_cpu->pcbase(), id, m_btche[id]);
	}
	if (offset == 2)
	{
		COMBINE_DATA(&m_btcle[id]);
		LOGWRITE("%08x CH %x BTC LE Write  %04x \n", m_cpu->pcbase(), id, m_btcle[id]);
	}

	m_btc[id] = m_btche[id] << 16 | m_btcle[id];
	LOGWRITE("%08x CH %x BTC Write  %08x \n", m_cpu->pcbase(), id, m_btc[id]);	
}

void mc68340_dma_module_device::update_irq(uint8_t id)
{
	m_irq[id] = m_done[id] | m_bes[id] | m_bed[id] | m_conf[id] | m_brkp[id];
	m_irqout[id] = 0;
	if (m_done[id] & m_intn[id])
	{
		m_irqout[id] = 1;
	}
	if (m_brkp[id] & m_intb[id])
	{
		m_irqout[id] = 1;
	}
	if  ( (m_bes[id] || m_bed[id] || m_conf[id]) && m_inte[id])
	{
		m_irqout[id] = 1;
	}
	LOGWRITE("%08x CH %x IPL  %04x \n", m_cpu->pcbase(), id, m_irqout[id]);

	m_cpu->update_ipl();
}

void mc68340_dma_module_device::update_csr(uint8_t id)
{
	m_csr[id] = (m_irq[id] << 7 |  m_done[id] << 6 | m_bes[id] << 5 | m_bed[id] << 4 | m_conf[id] << 3| m_brkp[id] << 2);
	LOGWRITE("%08x CH %x CSR  %04x \n", m_cpu->pcbase(), id, m_csr[id]);
}

void mc68340_dma_module_device::csr_w(uint8_t id, uint8_t data, uint16_t mem_mask)
{
	LOGWRITE("%08x CH %x CSR Write Data %04x \n", m_cpu->pcbase(), id, data);

	if (BIT(data,2))
	{
		m_brkp[id]=0;	
	}
	if (BIT(data,3))
	{
		m_conf[id]=0;	
	}
	if (BIT(data,4))
	{
		m_bed[id]=0;	
	}
	if (BIT(data,5))
	{
		m_bes[id]=0;	
	}
	if (BIT(data,6))
	{
		m_done[id]=1;//0	
	}

	update_irq(id);
	update_csr(id);

}

void mc68340_dma_module_device::dar_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (offset == 0)
	{
		COMBINE_DATA(&m_darhe[id]);
		LOGWRITE("%08x CH %x DARHE Write  %04x \n", m_cpu->pcbase(), id, m_darhe[id]);
	}
	if (offset == 2)
	{
		COMBINE_DATA(&m_darle[id]);
		LOGWRITE("%08x CH %x DARHE Write  %04x \n", m_cpu->pcbase(), id, m_darle[id]);
	}

	m_dar[id] = m_darhe[id] << 16 | m_darle[id];
	LOGWRITE("%08x CH %x DAR Write  %08x \n", m_cpu->pcbase(), id, m_dar[id]);
}

void mc68340_dma_module_device::sar_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (offset == 0)
	{
		COMBINE_DATA(&m_sarhe[id]);
		LOGWRITE("%08x CH %x SARHE Write  %04x \n", m_cpu->pcbase(), id, m_sarhe[id]);
	}
	if (offset == 2)
	{
		COMBINE_DATA(&m_sarle[id]);
		LOGWRITE("%08x CH %x SARLE Write  %04x \n", m_cpu->pcbase(), id, m_sarle[id]);
	}
	m_sar[id] = m_sarhe[id] << 16 | m_sarle[id];

	LOGWRITE("%08x CH %x SAR Write Offset %x %08x \n", m_cpu->pcbase(), id, offset, m_sar[id]);
}

void mc68340_dma_module_device::fcr_w(uint8_t id, uint8_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_fcr[id]);
	LOGWRITE("%08x CH %x FCR Write  %08x \n", m_cpu->pcbase(), id, m_fcr[id]);
	
	m_dmadfc[id] = data & 0x07;
	LOGWRITE("%08x CH %x DMADFC Write  %08x \n", m_cpu->pcbase(), id, m_dmadfc[id]);
	m_dmasfc[id] = (data >> 4) & 0x07;
	LOGWRITE("%08x CH %x DMASFC Write  %08x \n", m_cpu->pcbase(), id, m_dmasfc[id]);
}

uint16_t mc68340_dma_module_device::intr_r(uint8_t id, offs_t offset)
{
	if (offset == 0)
	{
		LOGREAD("%08x CH %x INTV Read Offset %x Data\n", m_cpu->pcbase(), id+1, offset, m_intv[id]);
		return m_intv[id];
	}
	else
	{
		LOGREAD("%08x CH %x INTL Read Offset %x Data\n", m_cpu->pcbase(), id+1, offset, m_intl[id]);
		return m_intl[id];
	}	
}

void mc68340_dma_module_device::intr_w(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask)
{
	LOGWRITE("%08x CH %x INTR Write Offset %x Data %08x\n", m_cpu->pcbase(), id+1, offset, data);

	if (offset == 0)
	{
		COMBINE_DATA(&m_intv[id]);
		m_intv[id] = data;
	}
	else
	{
		COMBINE_DATA(&m_intl[id]);
		m_intl[id] &= 0x07;
	}	
}

uint16_t mc68340_dma_module_device::dma_register_read(uint8_t id, offs_t offset, uint16_t mem_mask)
{
	LOGREAD("%08x CH %x Read Offset %x Mask %08x\n", m_cpu->pcbase(), id+1, offset, mem_mask);

	switch (offset * 2)
	{
		case 0x00:
			return m_mcr[id];

		case 0x04:
			return intr_r(id, offset - 0x04);

		case 0x08:
			return m_ccr[id];

		case 0x0a:
			if (ACCESSING_BITS_0_7)
			{
				LOGREAD("%08x CH %x Read FCR %x\n", m_cpu->pcbase(), id+1, m_fcr[id]);
				return m_fcr[id];
			}
			else if (ACCESSING_BITS_8_15)
			{
				LOGREAD("%08x CH %x Read CSR %x\n", m_cpu->pcbase(), id+1, m_csr[id]);
				return m_csr[id];
			}
			return 0;

		case 0x0c:
			return m_sarhe[id];

		case 0x0e:
			return m_sarle[id];

		case 0x10:
			return m_darhe[id];
		case 0x12:
			return m_darle[id];

		case 0x14:
			return m_btche[id];
		case 0x16:
			return m_btcle[id];

		default:
			return 0;
			break;
	}
	return 0;
}

uint16_t mc68340_dma_module_device::read(offs_t offset, uint16_t mem_mask)
{
	// logerror("%s m68340_internal_dma_r %08x, (%08x)\n", machine().describe_context(), offset,mem_mask);

	if (offset < 0x20)
	{
		return dma_register_read(0, offset, mem_mask);
	}
	else
	{
		return dma_register_read(1, offset - 0x20, mem_mask);
	}
}

void mc68340_dma_module_device::dma_register_write(uint8_t id, offs_t offset, uint16_t data, uint16_t mem_mask)
{
	LOGWRITE("%08x CH %x Register Write Offset %x Data %08x Mask %08x\n", m_cpu->pcbase(), id+1, offset, data, mem_mask);

	switch (offset * 2)
	{
		case 0x00:
			mcr_w(id, data, mem_mask);
			break;

		case 0x04:
			intr_w(id, offset - 0x04, data, mem_mask);
			break;

		case 0x08:
			ccr_w(id, data, mem_mask);
			break;

		case 0x0a:
			if (ACCESSING_BITS_0_7)
			{
				fcr_w(id, data >> 8, mem_mask);
			}
			if (ACCESSING_BITS_8_15)
			{
				csr_w(id, data & 0xff, mem_mask);
			}
			break;

		case 0x0c:
		case 0x0e:
			sar_w(id, ((offset * 2) - 0x0c), data, mem_mask);
			break;

		case 0x10:
		case 0x12:
			dar_w(id, ((offset * 2) - 0x10), data, mem_mask);
			break;

		case 0x14:
		case 0x16:
			btc_w(id, ((offset *2 ) - 0x14), data, mem_mask);
			break;

		default:
			//reserved
			break;
	}
}


void mc68340_dma_module_device::write(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	
	if (offset < 0x20)
	{
		dma_register_write(0, offset, data, mem_mask);
	}
	else
	{
		dma_register_write(1, offset - 0x20, data, mem_mask);
	}

	logerror("%s m68340_internal_dma_w %08x, %08x (%08x)\n", machine().describe_context(), offset*2, data,mem_mask);
}

void mc68340_dma_module_device::device_start()
{
	m_cpu = downcast<m68340_cpu_device *>(owner());
	for (int i=0; i <2; i++)
	{
		m_dma_timer[i] = timer_alloc(FUNC(mc68340_dma_module_device::dma_finished), this);
//		m_dma_timer[i]->adjust(attotime::from_hz(m_cpu->clock()), i);

	}

}

void mc68340_dma_module_device::device_reset()
{
	module_reset();
}

void mc68340_dma_module_device::module_reset()
{
	m_stp=0;
	m_iarb=0;
	for (int id=0; id<2; id++)
	{
		m_dreq[id]=1;
		m_irq[id]=0;
		m_done[id]=0;
		m_bes[id]=0;
		m_bed[id]=0;
		m_conf[id]=0;
		m_brkp[id]=0;
		m_maid[id]=0;
		m_supv[id]=0;
		m_ism[id]=0;
		m_se[id]=0;
		m_frz0[id]=0;
		m_frz1[id]=0;
		m_intv[id]=0x000f;
		m_intl[id]=0;
		m_str[id]=0;
	}
}

mc68340_dma_module_device::mc68340_dma_module_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
  : device_t(mconfig, MC68340_DMA_MODULE, tag, owner, clock)
  	, m_dack_out_cb(*this)
  	, m_done_out_cb(*this)
  	, m_done_in_cb(*this, 0)
{
}

DEFINE_DEVICE_TYPE(MC68340_DMA_MODULE, mc68340_dma_module_device, "mc68340dma", "MC68340 DMA Module")
