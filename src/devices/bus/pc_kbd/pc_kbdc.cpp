// license:BSD-3-Clause
// copyright-holders:Wilbert Pol
/***************************************************************************

  PC Keyboard connector interface

The following basic program can be useful for identifying scancodes:
10 sc%=0:sp%=0
20 sc%=inp(96)
30 if sc%<>sp% then print hex$(sc%):sp%=sc%
40 goto 20

***************************************************************************/


#include "emu.h"
#include "pc_kbdc.h"

#define LOG_SIGNALS (1U << 1)
//#define VERBOSE (LOG_GENERAL)
#include "logmacro.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(PC_KBDC, pc_kbdc_device, "pc_kbdc", "PC keyboard connector")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  pc_kbdc_device - constructor
//-------------------------------------------------
pc_kbdc_device::pc_kbdc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, PC_KBDC, tag, owner, clock),
	device_single_card_slot_interface<device_pc_kbd_interface>(mconfig, *this),
	m_out_clock_cb(*this),
	m_out_data_cb(*this),
	m_clock_state(-1),
	m_data_state(-1), m_mb_clock_state(0), m_mb_data_state(0),
	m_kb_clock_state(1),
	m_kb_data_state(1),
	m_keyboard(nullptr)
{
}

void pc_kbdc_device::set_keyboard(device_pc_kbd_interface *keyboard)
{
	m_keyboard = keyboard;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------
void pc_kbdc_device::device_resolve_objects()
{
	device_pc_kbd_interface *const pc_kbd = get_card_device();
	if (pc_kbd)
		pc_kbd->set_pc_kbdc(this);

	m_out_clock_cb.resolve_safe();
	m_out_data_cb.resolve_safe();
}

void pc_kbdc_device::device_start()
{
	save_item(NAME(m_clock_state));
	save_item(NAME(m_data_state));

	save_item(NAME(m_mb_clock_state));
	save_item(NAME(m_mb_data_state));
	save_item(NAME(m_kb_clock_state));
	save_item(NAME(m_kb_data_state));

	m_clock_state = -1;     /* initial state of calculated clock line */
	m_data_state = -1;      /* initial state of calculated data line */

	// Initially assume both keyboard and mainboard have released their data and clock lines
	m_mb_clock_state = 1;
	m_mb_data_state = 1;
	m_kb_clock_state = 1;
	m_kb_data_state = 1;
}


void pc_kbdc_device::update_clock_state(bool fromkb)
{
	int new_clock_state = m_mb_clock_state & m_kb_clock_state;

	if (new_clock_state != m_clock_state)
	{
		// We first set our state to prevent possible endless loops
		m_clock_state = new_clock_state;
		LOGMASKED(LOG_SIGNALS, "%s Clock: %d\n", fromkb? "<-" : "->", m_clock_state);
		// Send state to keyboard interface logic on mainboard
		m_out_clock_cb(m_clock_state);

		// Send state to keyboard
		if (m_keyboard)
			m_keyboard->clock_write(m_clock_state);
	}
}


void pc_kbdc_device::update_data_state(bool fromkb)
{
	int new_data_state = m_mb_data_state & m_kb_data_state;

	if (new_data_state != m_data_state)
	{
		// We first set our state to prevent possible endless loops
		m_data_state = new_data_state;
		LOGMASKED(LOG_SIGNALS, "%s Data:  %d\n", fromkb? "<-" : "->", m_data_state);

		// Send state to keyboard interface logic on mainboard
		m_out_data_cb(m_data_state);

		// Send state to keyboard
		if (m_keyboard)
			m_keyboard->data_write(m_data_state);
	}
}


void pc_kbdc_device::clock_write_from_mb(int state)
{
	m_mb_clock_state = state;
	update_clock_state(false);
}


void pc_kbdc_device::data_write_from_mb(int state)
{
	m_mb_data_state = state;
	update_data_state(false);
}


void pc_kbdc_device::clock_write_from_kb(int state)
{
	m_kb_clock_state = state;
	update_clock_state(true);
}


void pc_kbdc_device::data_write_from_kb(int state)
{
	m_kb_data_state = state;
	update_data_state(true);
}


//**************************************************************************
//  DEVICE PC KBD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_pc_kbd_interface - constructor
//-------------------------------------------------

device_pc_kbd_interface::device_pc_kbd_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device, "pckbd")
	, m_pc_kbdc(nullptr)
	, m_pc_kbdc_tag(nullptr)
{
}


//-------------------------------------------------
//  ~device_pc_kbd_interface - destructor
//-------------------------------------------------

device_pc_kbd_interface::~device_pc_kbd_interface()
{
}


void device_pc_kbd_interface::clock_write(int state)
{
}


void device_pc_kbd_interface::data_write(int state)
{
}


void device_pc_kbd_interface::set_pc_kbdc_device()
{
	if (m_pc_kbdc)
		m_pc_kbdc->set_keyboard(this);
}
