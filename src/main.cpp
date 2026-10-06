#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include "SDR_Audio.h"
#include "button.h"
#include "si5351.h"
#include "main.h"
#include "Process_DSP.h"
#include "Display.h"
#include "decode_ft8.h"
#include "options.h"
#include "traffic_manager.h"
#include "gen_ft8.h"
#include "ADIF.h"
#include "autoseq_engine.h"
#include "user_io.h"
#include "constants.h"
// Used for skipping the TX slot
int was_txing = 0;
bool clr_pressed = false;
bool free_text = false;
bool tx_pressed = false;
int log_display_flag;   


static char autoseq_txbuf[MAX_MSG_LEN];
static char autoseq_state_str[MAX_LINE_LEN];
static bool worked_qsos_in_display = false;


char Station_Call[11];         // six character call sign + /0
char Station_Locator[7];       // up to six character locator  + /0
char Short_Station_Locator[5]; // four character locator  + /0

uint32_t current_time, start_time, ft8_time;
uint32_t dsp_current_time, dsp_old_time, dsp_start_time, dsp_flag_time;
int ft8_flag;
int FT_8_counter;
int ft8_marker;
int decode_flag;
int WF_counter;
int xmit_flag;
int ft8_xmit_counter;
int DSP_Flag;
int master_decoded;

uint16_t cursor_freq;
uint16_t cursor_line;
int Tune_On;

int QSO_xmit;
int slot_state = 0;
int target_slot;


void update_synchronization()
{
  uint32_t current_time = millis();
  ;
  ft8_time = current_time - start_time;

  // Update slot and reset RX
  int current_slot = ft8_time / 15000 % 2;
  if (current_slot != slot_state)
  {
    // toggle the slot state
    slot_state ^= 1;
    if (was_txing)
    {
      autoseq_tick();
    }
    
    was_txing = 0;

    ft8_flag = 1;
    FT_8_counter = 0;
    ft8_marker = 1;
    WF_counter = 0;
    tx_display_update();
    show_battery_state();
    display_time(300,1050);

  }

  // Check if TX is intended
  if (QSO_xmit && target_slot == slot_state && FT_8_counter < 29)
  {
    setup_to_transmit_on_next_DSP_Flag(); // TODO: move to main.c
    QSO_xmit = 0;
    was_txing = 1;
    // Partial TX, set the TX counter based on current ft8_time
    ft8_xmit_counter = (ft8_time % 15000) / 160; // 160ms per symbol

    /*
    // Log the TX
    if (strindex(autoseq_txbuf, "CQ") < 0)
    if ((memcmp(autoseq_txbuf, "CQ ", 3) == 0) || (memcmp(autoseq_txbuf, "CQ\0", 3) == 0))
    {
      strcpy(current_message, autoseq_txbuf);
      update_message_log_display(1);
    }
    */

    tx_display_update();

    //display_worked_qsos();
  }
}


void setup() {

    Serial.begin(115200);

    auto cfg = M5.config();
    cfg.output_power = true; 
    M5.begin(cfg);
    M5.Power.setExtOutput(true, m5::ext_port_mask_t::ext_PA);
    
    init_RxSW();
    init_eeprom();
    Options_Initialize();
    start_Si5351();
    start_freq =14074;
    set_startup_freq();
    init_display();
    init_DSP();
    init_i2s_duplex();
    init_codec_driver();
    start_time = millis();
    open_stationData_file();
    init_user_io();
    init_toggle_states();
    display_station_data(0,1050);
    autoseq_init(Station_Call, Short_Station_Locator);
    Init_Log_File();
    attenuation = 1;
    FT8_Sync();

}

void loop() {
    const int offset_index = 8;

   if (!decode_flag)process_audio_stream();
   
  if (DSP_Flag)

  {
    if (xmit_flag)
    {
   
      if (!Tune_On)
      {
       
        if (ft8_xmit_counter >= offset_index && ft8_xmit_counter < 79 + offset_index)
        {
          set_FT8_Tone(tones[ft8_xmit_counter - offset_index]);
        }
        ft8_xmit_counter++;
        if (ft8_xmit_counter == 80 + offset_index)
        {
          xmit_flag = 0;
          terminate_transmit_armed();
          
        }
      }
    }

    DSP_Flag = 0;

   }
  
  
     if (decode_flag && !Tune_On && !xmit_flag) // start of servicing FT_Decode
    {

    master_decoded = ft8_decode();

    display_messages(new_decoded, master_decoded);

    for (int i = 0; i < master_decoded; ++i)
    {
      if (strindex(new_decoded[i].call_to, Station_Call) >= 0)
      {
        char received_message[22];
        sprintf(received_message, "%s %s %s", new_decoded[i].call_to, new_decoded[i].call_from, new_decoded[i].locator);
        strcpy(current_message, received_message);
        //update_message_log_display(0);
      }
    }

    
    if (!was_txing)
    {
      for (int i = 0; i < master_decoded; i++)
      {
        
        
        // TX is (potentially) necessary
        if (autoseq_on_decode(&new_decoded[i]))
        {
          // Fetch TX msg
          if (autoseq_get_next_tx(autoseq_txbuf))
          {
            queue_custom_text(autoseq_txbuf);
            QSO_xmit = 1;
            tx_display_update();
            break;
          }
        }
          
      }

      if (!QSO_xmit)
      { // Check if QSO_xmit
        // Check if retry is necessary
        
        if (autoseq_get_next_tx(autoseq_txbuf))
        
        {
          queue_custom_text(autoseq_txbuf);
          QSO_xmit = 1;
        }
        else if (Beacon_On)
        {
          target_slot = slot_state ^ 1; // toggle the slot
          autoseq_start_cq();
          autoseq_get_next_tx(autoseq_txbuf);
          queue_custom_text(autoseq_txbuf);
          QSO_xmit = 1;
          tx_display_update();
        }
        else if (Auto_QSO)
        {
          // Auto_QSO_Start
          if (Valid_CQ_Candidate)
          {
            process_selected_Station(master_decoded, max_sync_score_index);
            autoseq_on_touch(&new_decoded[max_sync_score_index]);
            autoseq_get_next_tx(autoseq_txbuf);
            queue_custom_text(autoseq_txbuf);
            QSO_xmit = 1;
            tx_display_update();
            store_CQ_Call();
          }
          
        } // Auto_QSO_End

      } // Check if QSO_xmit End
    }
    
    decode_flag = 0;

  } 

    M5.update(); 
    process_user_io();


  if (clr_pressed)
  {
    terminate_QSO();
    QSO_xmit = 0;
    was_txing = 0;
    autoseq_init(Station_Call, Short_Station_Locator);
    autoseq_txbuf[0] = '\0';
    tx_display_update();
    clr_pressed = false;
  }

  /*
  if (tx_pressed)
  {
    worked_qsos_in_display = display_worked_qsos();
    tx_pressed = false;
    tx_display_update();
  }
  */

  if (!Tune_On && log_display_flag == 1)
  {
    display_logged_messages();
    log_display_flag = 0;
  }

  if (!Tune_On && FT8_Touch_Flag && FT_8_TouchIndex < master_decoded)
  {
    process_selected_Station(master_decoded, FT_8_TouchIndex);
    autoseq_on_touch(&new_decoded[FT_8_TouchIndex]);
    autoseq_get_next_tx(autoseq_txbuf);
    queue_custom_text(autoseq_txbuf);
    QSO_xmit = 1;
    FT8_Touch_Flag = 0;
    tx_display_update();
    
  }

  update_synchronization();

}


void writeCodecRegister(uint16_t reg, uint16_t val) {
    // 1. Separate the first byte as the initial I2C register/index parameter
    uint8_t first_byte = (uint8_t)(reg >> 8); 

    // 2. Set up a properly formatted 3-byte array for the remaining bytes
    uint8_t payload[3];
    payload[0] = (uint8_t)(reg & 0xFF);  // Low byte of register address
    payload[1] = (uint8_t)(val >> 8);   // High byte of data value
    payload[2] = (uint8_t)(val & 0xFF);  // Low byte of data value

    // 3. Transmit using the correct, valid M5 Unified 5-parameter method:
    // Signature: writeRegister(device_addr, reg_byte, data_ptr, data_len, frequency)
    // 0x0A = SGTL5000 Address
    // first_byte = High byte of register passed as the first data line item
    // payload = Pointer to our remaining 3 bytes
    // 3 = Send exactly 3 bytes from our local payload array
    // 100000U = Explicit 100kHz standard speed argument required by M5's API
    M5.In_I2C.writeRegister(0x0A, first_byte, payload, 3, 100000U);
}



// M5Unified Version:
void m5_si5351_write(uint8_t addr, uint8_t reg, uint8_t data) {
    // Arguments: device_address, register_address, data_byte
    M5.In_I2C.writeRegister8(addr, reg, data, 100000U);
}


// M5Unified Version:
void m5_si5351_write_bulk(uint8_t addr, uint8_t start_reg, const uint8_t *data, size_t len) {
    // Arguments: device_address, register_address, data_buffer, buffer_length
    M5.In_I2C.writeRegister(addr, start_reg, data, len, 100000U);
}


// M5Unified Version:
uint8_t m5_si5351_read(uint8_t addr, uint8_t reg) {
    uint8_t data = 0;
    // Arguments: device_address, register_address, target_buffer, read_length
    M5.In_I2C.readRegister(addr, reg, &data, 1, 100000U);
    return data;
}

// Helper function for updating TX region display
void tx_display_update(void)
{

 if (Tune_On || worked_qsos_in_display)
  {
    return;
  }
  if (xmit_flag)
  {
    display_txing_message(autoseq_txbuf);
  }
  else
  {
    display_queued_message(autoseq_txbuf);
  }

  autoseq_get_qso_state(autoseq_state_str);
  display_qso_state(autoseq_state_str);

}