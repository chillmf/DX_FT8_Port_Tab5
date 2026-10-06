#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include "button.h"
#include "main.h"
#include "autoseq_engine.h"
#include "Display.h"
#include "Process_DSP.h"
#include "traffic_manager.h"

#define ft8_shift 6.25

enum SwitchType {
    ONE_SHOT,   
    TOGGLE      
};

struct UIControl {
    int x; int y; int w; int h;
    const char* label;      
    SwitchType type;
    bool isActive;          
    const char* activeLabel;   
    const char* inactiveLabel; 
    void (*callback)(bool); 
};

// Forward declaration of custom user functions
void handleModeToggle(bool state);
void handleTuneToggle(bool state);
void handleRplyToggle(bool state);
void handleMsgToggle(bool state);
void handleFreqToggle(bool state);

void handleClrPulse(bool state);
void handleSyncPulse(bool state);
void handleBndLowerPulse(bool state);
void handleBndRaisePulse(bool state);
void handleFreqLowerPulse(bool state);
void handleFreqRaisePulse(bool state);

// Explicit landscape coordinates mapped to the 1280x720 panel
UIControl controls[] = {
    // X,   Y,   W,   H,   Main Label,     Type,     Init, Active,   Inactive, Callback

    {0,    1100, 200, 50,   "Mode",         TOGGLE, false,  "Becn",   "QSO",    handleModeToggle},
    {220,  1100, 200, 50,   "Tune",         TOGGLE, false,  " On ",  "Off" ,    handleTuneToggle},
    {440,  1100, 200, 50,   "Rply",         TOGGLE, false,  "Auto",   "Man",    handleRplyToggle},
    {0,    1200, 200, 50,   " Msg",         TOGGLE, false,  "POTA",  " CQ ",    handleMsgToggle},
    {220,  1200, 200, 50,   "Freq",         TOGGLE, false,  "Rcvd",  "Fixd",    handleFreqToggle},
    {0,   100, 100, 50,      "Clr",       ONE_SHOT, false,      "",      "",    handleClrPulse},
    {110,   100, 100, 50,   "Sync",       ONE_SHOT, false,      "",      "",    handleSyncPulse},
    {220,   100, 100, 50,   "Bnd-",       ONE_SHOT, false,      "",      "",    handleBndLowerPulse},
    {440,   100, 100, 50,   "Bnd+",       ONE_SHOT, false,      "",      "",    handleBndRaisePulse},
    {600,   0, 100, 50,      " F-",       ONE_SHOT, false,      "",      "",    handleFreqLowerPulse},
    {600,   100, 100, 50,    " F+",       ONE_SHOT, false,      "",      "",    handleFreqRaisePulse}
};
    void init_toggle_states(void){

        Beacon_On = 0;
        Tune_On = 0;
        Auto_QSO = 0;
        Free_Index = 0;
        QSO_Fix = 0;
       // FT8_Touch_Flag = 0;
    }





void show_state(void) {

    M5.Display.setTextColor(WHITE);

    M5.Display.setCursor(0, 700);
    M5.Display.print("Mode");
    show_variable(200,700,Beacon_On);

    M5.Display.setCursor(0, 750);
    M5.Display.print("Tune");
    show_variable(200,750,Tune_On);

    M5.Display.setCursor(0, 800);
    M5.Display.print("Auto_QSO");
    show_variable(200,800,Auto_QSO);

    M5.Display.setCursor(0, 850);
    M5.Display.print("Freq");
    show_variable(200,850,QSO_Fix);

    M5.Display.setCursor(0, 900);
    M5.Display.print("xmit_flag");
    show_variable(200,900,xmit_flag);
    
}




// ==========================================
// USER CALLBACK HARDWARE INTERFACES
// ==========================================
void handleModeToggle(bool state) {
    if(state) Beacon_On = 1;
    else
    Beacon_On = 0;
}

void handleTuneToggle(bool state) {
    if (state)  {
      Tune_On = 1;
      transmit_sequence();
      delay(10);
      xmit_flag = 1;
      tune_On_sequence();
      Arm_Tune = 1;
    }
    else
    {
      Tune_On = 0; // Turns off display of FT8 traffic
      tune_Off_sequence();
      Arm_Tune = 0;
      xmit_flag = 0;
      receive_sequence();
    }
}

void handleRplyToggle(bool state) {
    if (state)  {
       Auto_QSO = 1;
    }
    else
    {
      Auto_QSO = 0;
    }
    
}

void handleMsgToggle(bool state) {

    Free_Index = 0;
    
}
void handleFreqToggle(bool state) {
    if (state)  {
       QSO_Fix = 1;
    }
    else
    {
      QSO_Fix = 0;
    }
}

void handleClrPulse(bool state) {
    clr_pressed = true;
}
void handleSyncPulse(bool state) {
    FT8_Sync();
    FT8_Touch_Flag = 0;
}

void handleBndLowerPulse(bool state) {
     if (BandIndex > Band_Minimum)
    {
      BandIndex--;
      show_wide(320, 120, sBand_Data[BandIndex].Frequency); // 790 - 700 = 90
    };
    
}

void handleBndRaisePulse(bool state) {
    if (BandIndex < _10M)
    {
      BandIndex++;
      show_wide(320, 120, sBand_Data[BandIndex].Frequency); // 790 - 700 = 90
    }
    
}

void handleFreqLowerPulse(bool state) {
     if (cursor_line > 0)  {
      cursor_line--;
      cursor_freq = (uint16_t)((float)(cursor_line + ft8_min_bin) * ft8_shift);
      display_cursor_line = 2 * cursor_line;
      show_variable(cursor_freq_X ,cursor_freq_line , cursor_freq);
     
    }
    
}

void handleFreqRaisePulse(bool state) {

    if (cursor_line <= (ft8_buffer_size - ft8_min_bin - 2))
    {
      cursor_line++;
      cursor_freq = (uint16_t)((float)(cursor_line + ft8_min_bin) * ft8_shift);
      display_cursor_line = 2 * cursor_line;
      show_variable(cursor_freq_X , cursor_freq_line, cursor_freq);
    }
    
}

// FIXED: Now accurately counts all elements in the array
const int controlCount = sizeof(controls) / sizeof(controls[0]); 

void drawControl(UIControl &ctrl) {
    M5.Display.setTextSize(3);
    
    if (ctrl.type == TOGGLE) {
        M5.Display.drawRect(ctrl.x, ctrl.y, ctrl.w, ctrl.h, WHITE);
        
        int toggleBoxW = 100; 
        int txtAreaW = ctrl.w - toggleBoxW;
        
        M5.Display.fillRect(ctrl.x + 2, ctrl.y + 2, txtAreaW - 2, ctrl.h - 4, NAVY);
        M5.Display.setTextColor(WHITE);
        M5.Display.drawString(ctrl.label, ctrl.x + 20, ctrl.y + (ctrl.h / 2) - 12);
        
        int tX = ctrl.x + txtAreaW;
        M5.Display.fillRect(tX, ctrl.y + 2, toggleBoxW - 2, ctrl.h - 4, BLACK);
        
        if (ctrl.isActive) {
            M5.Display.fillRect(tX + 10, ctrl.y + 2, toggleBoxW - 20, ctrl.h - 6, RED);
            M5.Display.setTextColor(BLACK);
            M5.Display.drawCenterString(ctrl.activeLabel, tX + (toggleBoxW / 2), ctrl.y + (ctrl.h / 2) - 12);
        } else {
            M5.Display.fillRect(tX + 10, ctrl.y + 2, toggleBoxW - 20, ctrl.h - 6,GREEN );
            M5.Display.setTextColor(BLACK);
            M5.Display.drawCenterString(ctrl.inactiveLabel, tX + (toggleBoxW / 2), ctrl.y + (ctrl.h / 2) - 12);
        }
    } 
    else if (ctrl.type == ONE_SHOT) {
        M5.Display.drawRect(ctrl.x, ctrl.y, ctrl.w, ctrl.h, BLACK);
        M5.Display.fillRect(ctrl.x + 4, ctrl.y + 4, ctrl.w - 8, ctrl.h - 8, YELLOW);
        M5.Display.setTextColor(BLACK);
        M5.Display.drawCenterString(ctrl.label, ctrl.x + (ctrl.w / 2), ctrl.y + (ctrl.h / 2) - 12);
    }
}

void drawAllUI() {

    // This loop will now execute exactly 3 times
    for (int i = 0; i < controlCount; i++) {
        drawControl(controls[i]);
    }
}

void init_user_io(void) {

    drawAllUI();
}

void process_user_io(void) {

    if (M5.Touch.getCount() > 0) {
        auto touchDetail = M5.Touch.getDetail(0);

        if (touchDetail.wasPressed()) {
            int touchX = touchDetail.x;
            int touchY = touchDetail.y;

            for (int i = 0; i < controlCount; i++) {
                if (touchX >= controls[i].x && touchX <= (controls[i].x + controls[i].w) &&
                    touchY >= controls[i].y && touchY <= (controls[i].y + controls[i].h)) {
                    
                    if (controls[i].type == TOGGLE) {
                        controls[i].isActive = !controls[i].isActive;
                        drawControl(controls[i]);
                        controls[i].callback(controls[i].isActive);
                    } 
                    else if (controls[i].type == ONE_SHOT) {
                        M5.Display.fillRect(controls[i].x + 4, controls[i].y + 4, controls[i].w - 8, controls[i].h - 8, WHITE);
                        M5.Display.setTextColor(BLACK);
                        M5.Display.drawCenterString(controls[i].label, controls[i].x + (controls[i].w / 2), controls[i].y + (controls[i].h / 2) - 12);
                        
                        controls[i].callback(true); 

                        drawControl(controls[i]); 
                    }

                }

                FT8_Message_Touch = Xmit_message_Touch(touchX , touchY);
                FT8_Touch_Flag = FT8_Touch(touchX , touchY);
                check_WF_Touch(touchX, touchY);
            }
        
        }
       
    }

}


        void show_battery_state(void){

        int32_t volt_mv = M5.Power.getBatteryVoltage();
        int32_t curr_ma = M5.Power.getBatteryCurrent();
    
        float volt_v = volt_mv / 1000.0f; // Convert mV to V (e.g., 7.4V)

        M5.Display.setCursor(500, 1180);
        M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5.Display.printf("V: %2.2f V", volt_v);
        M5.Display.setCursor(500, 1220);
        M5.Display.printf("I: %d mA", curr_ma);
        }



