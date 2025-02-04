// license:BSD-3-Clause
// copyright-holders:David Haywood
/* 68307 */

#include "emu.h"
#include "68307.h"
#include "68307bus.h"
#include "68307sim.h"
#include "68307tmu.h"

DEFINE_DEVICE_TYPE(M68307, m68307_cpu_device, "mc68307", "MC68307")


/* 68307 SERIAL Module */
/* all ports on this are 8-bit? */

/* this is a 68681 'compatible' chip but with only a single channel implemented
  (writes to the other channel have no effects)

  for now at least we piggyback on the existing 68307 emulation rather than having
  a custom verson here, that may change later if subtle differences exist.

*/
uint8_t m68307_cpu_device::m68307_internal_serial_r(offs_t offset)
{
	if (offset&1) return m_duart->read(offset>>1);
	return 0x0000;
}

void m68307_cpu_device::m68307_internal_serial_w(offs_t offset, uint8_t data)
{
	if (offset & 1) m_duart->write(offset >> 1, data);
}



void m68307_cpu_device::internal_map(address_map &map)
{
	map(0x000000f0, 0x000000ff).rw(FUNC(m68307_cpu_device::m68307_internal_base_r), FUNC(m68307_cpu_device::m68307_internal_base_w));
}

void m68307_cpu_device::cpu_space_map(address_map &map)
{
	map(0xfffff0, 0xffffff).r(FUNC(m68307_cpu_device::int_ack)).umask16(0x00ff);
}


void m68307_cpu_device::device_add_mconfig(machine_config &config)
{
	MC68681(config, m_duart, 16000000/4); // ?? Mhz - should be specified in inline config
	m_duart->irq_cb().set(FUNC(m68307_cpu_device::m68307_duart_irq_handler));
	m_duart->a_tx_cb().set(FUNC(m68307_cpu_device::m68307_duart_txa));
	m_duart->b_tx_cb().set(FUNC(m68307_cpu_device::m68307_duart_txb));
	m_duart->inport_cb().set(FUNC(m68307_cpu_device::m68307_duart_input_r));
	m_duart->outport_cb().set(FUNC(m68307_cpu_device::m68307_duart_output_w));
}


m68307_cpu_device::m68307_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	m68000_device(mconfig, M68307, tag, owner, clock),
	m_write_irq(*this),
	m_write_a_tx(*this),
	m_write_b_tx(*this),
	m_read_inport(*this),
	m_write_outport(*this),
	m_porta_r(*this),
	m_porta_w(*this),
	m_portb_r(*this),
	m_portb_w(*this),
	m_duart(*this, "internal68681")
{
	m_m68307SIM = nullptr;
	m_m68307MBUS = nullptr;
	m_m68307TIMER = nullptr;
	m_m68307_base = 0;
	m_m68307_scrhigh = 0;
	m_m68307_scrlow = 0;
	m_m68307_currentcs = 0;
	m_ipl = 0;

	m_cpu_space_config.m_internal_map = address_map_constructor(FUNC(m68307_cpu_device::cpu_space_map), this);
	auto imap = address_map_constructor(FUNC(m68307_cpu_device::internal_map), this);
	m_program_config.m_internal_map = imap;
	m_opcodes_config.m_internal_map = imap;
	m_uprogram_config.m_internal_map = imap;
	m_uopcodes_config.m_internal_map = imap;
}






void m68307_cpu_device::device_reset()
{
	m68000_device::device_reset();

	if (m_m68307SIM) m_m68307SIM->reset();
	if (m_m68307MBUS) m_m68307MBUS->reset();
	if (m_m68307TIMER) m_m68307TIMER->reset();

	m_m68307_base = 0xbfff;
	m_m68307_scrhigh = 0x0007;
	m_m68307_scrlow = 0xf010;

	set_ipl(0);
}

void m68307_cpu_device::reset_peripherals(int state)
{
	m_duart->reset();

	if (m_m68307MBUS) m_m68307MBUS->reset();
	if (m_m68307TIMER) m_m68307TIMER->reset();
}


/* todo: is it possible to calculate the address map based on CS when they change
   and install handlers?  Going through this logic for every memory access is
   very slow */

inline int m68307_cpu_device::calc_cs(offs_t address) const
{
	m68307_sim const &sim = *m_m68307SIM;
	for (int i=0; i < 4; i++)
	{
		int const br = sim.m_br[i] & 1;
		int const amask = (sim.m_or[i] & 0x1ffc) << 11;
		int const bra = (sim.m_br[i] & 0x1ffc) << 11;
		if (br && ((address & amask) == bra)) return i + 1;
	}
	return 0;
}

void m68307_cpu_device::set_port_callbacks(
		porta_read_delegate &&porta_r,
		porta_write_delegate &&porta_w,
		portb_read_delegate &&portb_r,
		portb_write_delegate &&portb_w)
{
	m_porta_r = std::move(porta_r);
	m_porta_w = std::move(porta_w);
	m_portb_r = std::move(portb_r);
	m_portb_w = std::move(portb_w);
}





uint16_t m68307_cpu_device::get_cs(offs_t address)
{
	m_m68307_currentcs = calc_cs(address);

	return m_m68307_currentcs;
}


/* 68307 specifics - MOVE */

void m68307_cpu_device::set_ipl(int level)
{
	if (level != m_ipl)
	{
		if (m_ipl != 0)
			set_input_line(m_ipl, CLEAR_LINE);
		m_ipl = level;
		if (m_ipl != 0)
			set_input_line(m_ipl, ASSERT_LINE);
	}
}

void m68307_cpu_device::timer0_interrupt(int state)
{
	int prioritylevel = (m_m68307SIM->m_picr & 0x7000) >> 12;
	if (state && m_ipl < prioritylevel)
		set_ipl(prioritylevel);
	else if (!state && m_ipl == prioritylevel)
		set_ipl(m_m68307SIM->get_ipl(this));
}

void m68307_cpu_device::timer1_interrupt(int state)
{
	int prioritylevel = (m_m68307SIM->m_picr & 0x0700) >> 8;
	if (state && m_ipl < prioritylevel)
		set_ipl(prioritylevel);
	else if (!state && m_ipl == prioritylevel)
		set_ipl(m_m68307SIM->get_ipl(this));
}

void m68307_cpu_device::m68307_duart_irq_handler(int state)
{
	int prioritylevel = (m_m68307SIM->m_picr & 0x0070) >> 4;
	if (state && m_ipl < prioritylevel)
		set_ipl(prioritylevel);
	else if (!state && m_ipl == prioritylevel)
		set_ipl(m_m68307SIM->get_ipl(this));
}

void m68307_cpu_device::mbus_interrupt(int state)
{
	int prioritylevel = (m_m68307SIM->m_picr & 0x0007) >> 0;
	if (state && m_ipl < prioritylevel)
		set_ipl(prioritylevel);
	else if (!state && m_ipl == prioritylevel)
		set_ipl(m_m68307SIM->get_ipl(this));
}

void m68307_cpu_device::licr2_interrupt()
{
	m_m68307SIM->m_licr2 |= 0x8;

	int prioritylevel = (m_m68307SIM->m_licr2 & 0x0007) >> 0;
	if (m_ipl < prioritylevel)
		set_ipl(prioritylevel);
}

uint8_t m68307_cpu_device::int_ack(offs_t offset)
{
	uint8_t type = m_m68307SIM->get_int_type(this, offset);
	if (!machine().side_effects_disabled())
		logerror("Interrupt acknowledged: level %d, type %01X\n", offset, type);

	// UART provides its own vector
	if (type == 0x0c)
		return m_duart->get_irq_vector();
	else
		return (m_m68307SIM->m_pivr & 0xf0) | type;
}

void m68307_cpu_device::device_start()
{
	reset_cb().append(*this, FUNC(m68307_cpu_device::reset_peripherals));

	m68000_device::device_start();

	/* basic CS logic, timers, mbus, serial logic
	   set via remappable register
	*/

	m_m68307SIM    = new m68307_sim();
	m_m68307MBUS   = new m68307_mbus();
	m_m68307TIMER  = new m68307_timer();

	m_m68307TIMER->init(this);

	m_m68307SIM->reset();
	m_m68307MBUS->reset();
	m_m68307TIMER->reset();

	m_m68307_base = 0xbfff;
	m_m68307_scrhigh = 0x0007;
	m_m68307_scrlow = 0xf010;

	m_write_irq.resolve_safe();
	m_write_a_tx.resolve_safe();
	m_write_b_tx.resolve_safe();
	m_read_inport.resolve();
	m_write_outport.resolve_safe();

	m_porta_r.set(nullptr);
	m_porta_w.set(nullptr);
	m_portb_r.set(nullptr);
	m_portb_w.set(nullptr);
}



uint16_t m68307_cpu_device::m68307_internal_base_r(offs_t offset, uint16_t mem_mask)
{
	logerror("%08x m68307_internal_base_r %08x, (%04x)\n", m_ipc, offset*2,mem_mask);

	switch (offset<<1)
	{
		case 0x2: return m_m68307_base;
		case 0x4: return m_m68307_scrhigh;
		case 0x6: return m_m68307_scrlow;
	}

	logerror("(read was illegal?)\n");

	return 0x0000;
}

void m68307_cpu_device::m68307_internal_base_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	// emumem needs some kind of overlay capability...  it's probably not the only one

	logerror("%08x m68307_internal_base_w %08x, %04x (%04x)\n", m_ipc, offset*2,data,mem_mask);
	int base;
	//int mask = 0;

	switch (offset<<1)
	{
		case 0x2:
			/* remove old internal handler */
			base = (m_m68307_base & 0x0fff) << 12;
			//mask = (m_m68307_base & 0xe000) >> 13;
			//if ( m_m68307_base & 0x1000 ) mask |= 7;
			m_s_program->unmap_readwrite(base+0x000, base+0x04f);
			m_s_program->unmap_readwrite(base+0x100, base+0x11f);
			m_s_program->unmap_readwrite(base+0x120, base+0x13f);
			m_s_program->unmap_readwrite(base+0x140, base+0x149);

			/* store new base address */
			COMBINE_DATA(&m_m68307_base);

			/* install new internal handler */
			base = (m_m68307_base & 0x0fff) << 12;
			//mask = (m_m68307_base & 0xe000) >> 13;
			//if ( m_m68307_base & 0x1000 ) mask |= 7;
			m_s_program->install_readwrite_handler(base + 0x000, base + 0x04f, read16_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_sim_r)),    write16_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_sim_w)));
			m_s_program->install_readwrite_handler(base + 0x100, base + 0x11f, read8sm_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_serial_r)), write8sm_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_serial_w)), 0xffff);
			m_s_program->install_readwrite_handler(base + 0x120, base + 0x13f, read16s_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_timer_r)),  write16s_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_timer_w)));
			m_s_program->install_readwrite_handler(base + 0x140, base + 0x149, read8sm_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_mbus_r)),   write8sm_delegate(*this, FUNC(m68307_cpu_device::m68307_internal_mbus_w)), 0xffff);


			break;

		case 0x4:
			COMBINE_DATA(&m_m68307_scrhigh);
			break;

		case 0x6:
			COMBINE_DATA(&m_m68307_scrlow);
			break;

		default:
			logerror("(write was illegal?)\n");
			break;
	}
}
