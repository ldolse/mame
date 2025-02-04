// license:BSD-3-Clause
// copyright-holders:smf
/**********************************************************************

    Commodore PET User Port emulation

**********************************************************************/

#include "emu.h"
#include "user.h"

// class pet_user_port_device

DEFINE_DEVICE_TYPE(PET_USER_PORT, pet_user_port_device, "pet_user_port", "PET user port")

pet_user_port_device::pet_user_port_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, PET_USER_PORT, tag, owner, clock),
	device_slot_interface(mconfig, *this),
	m_2_handler(*this),
	m_3_handler(*this),
	m_4_handler(*this),
	m_5_handler(*this),
	m_6_handler(*this),
	m_7_handler(*this),
	m_8_handler(*this),
	m_9_handler(*this),
	m_10_handler(*this),
	m_b_handler(*this),
	m_c_handler(*this),
	m_d_handler(*this),
	m_e_handler(*this),
	m_f_handler(*this),
	m_h_handler(*this),
	m_j_handler(*this),
	m_k_handler(*this),
	m_l_handler(*this),
	m_m_handler(*this),
	m_card(nullptr)
{
}

void pet_user_port_device::device_config_complete()
{
	m_card = dynamic_cast<device_pet_user_port_interface *>(get_card_device());
}

void pet_user_port_device::device_start()
{
	m_2_handler.resolve_safe();
	m_3_handler.resolve_safe();
	m_4_handler.resolve_safe();
	m_5_handler.resolve_safe();
	m_6_handler.resolve_safe();
	m_7_handler.resolve_safe();
	m_8_handler.resolve_safe();
	m_9_handler.resolve_safe();
	m_10_handler.resolve_safe();
	m_b_handler.resolve_safe();
	m_c_handler.resolve_safe();
	m_d_handler.resolve_safe();
	m_e_handler.resolve_safe();
	m_f_handler.resolve_safe();
	m_h_handler.resolve_safe();
	m_j_handler.resolve_safe();
	m_k_handler.resolve_safe();
	m_l_handler.resolve_safe();
	m_m_handler.resolve_safe();

	// pull up
	m_3_handler(1);
	m_4_handler(1);
	m_5_handler(1);
	m_6_handler(1);
	m_7_handler(1);
	m_8_handler(1);
	m_9_handler(1);
	m_b_handler(1);
	m_c_handler(1);
	m_d_handler(1);
	m_e_handler(1);
	m_f_handler(1);
	m_h_handler(1);
	m_j_handler(1);
	m_k_handler(1);
	m_l_handler(1);
	m_m_handler(1);
}

void pet_user_port_device::write_2(int state) { if (m_card != nullptr) m_card->input_2(state); }
void pet_user_port_device::write_3(int state) { if (m_card != nullptr) m_card->input_3(state); }
void pet_user_port_device::write_4(int state) { if (m_card != nullptr) m_card->input_4(state); }
void pet_user_port_device::write_5(int state) { if (m_card != nullptr) m_card->input_5(state); }
void pet_user_port_device::write_6(int state) { if (m_card != nullptr) m_card->input_6(state); }
void pet_user_port_device::write_7(int state) { if (m_card != nullptr) m_card->input_7(state); }
void pet_user_port_device::write_8(int state) { if (m_card != nullptr) m_card->input_8(state); }
void pet_user_port_device::write_9(int state) { if (m_card != nullptr) m_card->input_9(state); }
void pet_user_port_device::write_10(int state) { if (m_card != nullptr) m_card->input_10(state); }
void pet_user_port_device::write_b(int state) { if (m_card != nullptr) m_card->input_b(state); }
void pet_user_port_device::write_c(int state) { if (m_card != nullptr) m_card->input_c(state); }
void pet_user_port_device::write_d(int state) { if (m_card != nullptr) m_card->input_d(state); }
void pet_user_port_device::write_e(int state) { if (m_card != nullptr) m_card->input_e(state); }
void pet_user_port_device::write_f(int state) { if (m_card != nullptr) m_card->input_f(state); }
void pet_user_port_device::write_h(int state) { if (m_card != nullptr) m_card->input_h(state); }
void pet_user_port_device::write_j(int state) { if (m_card != nullptr) m_card->input_j(state); }
void pet_user_port_device::write_k(int state) { if (m_card != nullptr) m_card->input_k(state); }
void pet_user_port_device::write_l(int state) { if (m_card != nullptr) m_card->input_l(state); }
void pet_user_port_device::write_m(int state) { if (m_card != nullptr) m_card->input_m(state); }


// class device_pet_user_port_interface

device_pet_user_port_interface::device_pet_user_port_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device, "petuser")
{
	m_slot = dynamic_cast<pet_user_port_device *>(device.owner());
}

device_pet_user_port_interface::~device_pet_user_port_interface()
{
}


#include "diag.h"
#include "petuja.h"
#include "cb2snd.h"
#include "2joysnd.h"

void pet_user_port_cards(device_slot_interface &device)
{
	device.option_add("diag", PET_USERPORT_DIAGNOSTIC_CONNECTOR);
	device.option_add("petuja", PET_USERPORT_JOYSTICK_ADAPTER);
	device.option_add("cb2snd", PET_USERPORT_CB2_SOUND_DEVICE);
	device.option_add("2joysnd", PET_USERPORT_JOYSTICK_AND_SOUND_DEVICE);
}
