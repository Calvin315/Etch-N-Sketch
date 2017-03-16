/*
* 120B Final Project.c
* Author : Calvin K
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "io.c"
#include "bit.h"
#include "NOK_LCD.c"
#include "NOK_LCD_SCREENS.c"

const unsigned short MAX_RES = (84 * 48);

//Saved Data Var with Default Vals
uint8_t EEMEM Contrast_EE = 50;
uint16_t EEMEM LR_ADC_MAX_EE = 1023;
uint16_t EEMEM UD_ADC_MAX_EE = 986;

//Uses EE PROM if 1
unsigned char TESTING_EE = 1;
unsigned char TESTING_DEBUGGER = 0;

//Functions
void adc_init();
unsigned short ADC_Scaler(unsigned short ADC_Max, unsigned short ADC_Val, unsigned char Divider);
uint16_t adc_read(uint8_t ch);
void LCDdefinechar(const uint8_t *pc, uint8_t char_code);
unsigned char JoystickPos(unsigned char LR, unsigned char UD);

//Controls and Inputs
unsigned short LeftRightRAW, UpDownRAW, LR_ADC_MAX, UD_ADC_MAX;
uint8_t Contrast;

unsigned char LeftRight, UpDown;
unsigned char Button1, Button2, Button3, Button4;

//Temp Vars
unsigned char Menu_SelectNum = 1, Menu_TempSelectCursor = 16;
unsigned char Settings_SelectNum = 0;
unsigned char DrawMode = 1;

unsigned char MenuOn = 1, SettingsOn = 0, DrawingOn = 0;
unsigned char Xpos = 42, Ypos = 24;

char Menu_DEBUG_CANVAS[] = "CANVAS MODE", Menu_DEBUG_SETTINGS[] = "SET MODE";
char Setting_DEBUG[] = "PASSED";

enum MenuStates{Menu_Start, Menu_Sel, Menu_DrawingMode, Menu_SettingsMode} MenuState;
int Tick_Menu(int Menu_State)
{
    char Menu_MenuSelLCD[32] = "Canvas Mode   1 Setting Mode  2 ";

    //Actions
    switch(Menu_State)
    {
        case Menu_Start:
        //Display Screen
        if(MenuOn)
        {
            LcdImage(StartImg);
            LcdUpdate();
            LCD_DisplayString(1,Menu_MenuSelLCD);
            Menu_TempSelectCursor = 16;
            Menu_SelectNum = 1;
        }
        break;

        case Menu_Sel:
        if(Button1)
        {
            Menu_TempSelectCursor = 16;
            Menu_SelectNum = 1;
        }
        else if (Button2)
        {
            Menu_TempSelectCursor = 32;
            Menu_SelectNum = 2;
        }

        LCD_Cursor(Menu_TempSelectCursor);
        break;

        case Menu_DrawingMode:
        DrawingOn = 1;
        MenuOn = 0;
        LCD_ClearScreen();
        break;

        case Menu_SettingsMode:
        SettingsOn = 1;
        MenuOn = 0;
        LCD_ClearScreen();
        break;

        default:
        break;
    }

    //Transitions
    switch(Menu_State)
    {
        case Menu_Start:
        if (MenuOn)
        {
            Menu_State = Menu_Sel;
        }
        else
        {
            Menu_State = Menu_Start;
        }
        break;

        case Menu_Sel:
        if ( (Menu_SelectNum == 1) && (Button3))
        {
            Menu_State = Menu_DrawingMode;
        }
        else if ( (Menu_SelectNum == 2) && (Button3))
        {
            Menu_State = Menu_SettingsMode;
        }
        break;

        case Menu_DrawingMode:
        Menu_State = Menu_Start;
        break;

        case Menu_SettingsMode:
        Menu_State = Menu_Start;
        break;

        default:
        Menu_State = Menu_Start;
        break;
    }

    return Menu_State;
};

enum SettingStates{Setting_Start, Setting_Sel, Setting_ContrastSET, Setting_UDSET, Setting_RLSET, Setting_RetSet, Setting_Saved} SettingState;
int Tick_Settings(int Setting_State)
{
    char Setting_Contrast[4] = "";
    char Setting_Contrast_Dis[32] = "Saved: ";

    char Setting_NewContrast[4] = "";
    char Setting_NewContrast_Dis[32] = " New: ";

    unsigned char Contrast_Read = 0;

    //Actions
    switch(Setting_State)
    {
        case Setting_Start:
        if(SettingsOn)
        {
            LCD_DisplayString(1,"Use Joystick to Select Mode");
        }
        break;

        case Setting_Sel:

        if (JoystickPos(LeftRight, UpDown) == 0)
        {
            LCD_DisplayString(1, "To go back pressB4");
            Settings_SelectNum = 1;
        }
        else if (JoystickPos(LeftRight, UpDown) == 7)
        {
            LCD_DisplayString(1, "Set Contrast    Press B4");
            Settings_SelectNum = 2;
        }
        else if (JoystickPos(LeftRight, UpDown) == 5)
        {
            LCD_DisplayString(1, "Set UD ADC MAX  Press B4");
            Settings_SelectNum = 3;
        }
        else if (JoystickPos(LeftRight, UpDown) == 4)
        {
            LCD_DisplayString(1, "Set LR ADC MAX  Press B4");
            Settings_SelectNum = 4;
        }
        break;

        case Setting_ContrastSET:
        LCD_ClearScreen();
        Contrast_Read = eeprom_read_byte(&Contrast_EE);
        
        itoa(Contrast_Read, Setting_Contrast, 10);
        strcat(Setting_Contrast_Dis, Setting_Contrast);

        strcat(Setting_Contrast_Dis, " ");

        if(Button1 && (Contrast < 100))
        {
            Contrast += 10;
        }

        if(Button2 && (Contrast > 10))
        {
            Contrast -= 10;
        }
        LcdContrast(Contrast);

        itoa(Contrast, Setting_NewContrast, 10);
        strcat(Setting_Contrast_Dis, Setting_NewContrast_Dis);
        strcat(Setting_Contrast_Dis, Setting_NewContrast);

        LCD_DisplayString(1, Setting_Contrast_Dis);
        
        if(Button3)
        {
            eeprom_update_byte((uint8_t*) &Contrast_EE, Contrast);
        }

        break;

        case Setting_UDSET:
        break;

        case Setting_RLSET:
        break;
        
        case Setting_Saved:
        LCD_ClearScreen();
        LCD_DisplayString(1,"Saved Press any key to return");
        break;

        case Setting_RetSet:
        SettingsOn = 0;
        MenuOn = 1;
        break;

        default:
        break;
    }

    //Transitions
    switch(Setting_State)
    {
        case Setting_Start:
        if(SettingsOn)
        {
            if ( (JoystickPos(LeftRight, UpDown) != 0) || (Button1 || Button2 || Button3 || Button4))
            {
                Setting_State = Setting_Sel;
            }
            else
            {
                Setting_State = Setting_Start;
            }
        }
        break;

        case Setting_Sel:
        if ( (Settings_SelectNum == 1) && Button4)
        {
            Setting_State = Setting_RetSet;
        }
        else if ( (Settings_SelectNum == 2) && Button4)
        {
            Setting_State = Setting_ContrastSET;
        }
        else if ( (Settings_SelectNum == 3) && Button4)
        {
            Setting_State = Setting_RLSET;
        }
        else if ( (Settings_SelectNum == 4) && Button4)
        {
            Setting_State = Setting_UDSET;
        }
        break;

        case Setting_ContrastSET:
        if (Button3)
        {
            Setting_State = Setting_Saved;
        }
        break;

        case Setting_UDSET:
        break;

        case Setting_RLSET:
        break;

        case Setting_RetSet:
        Setting_State = Setting_Start;
        break;

        case Setting_Saved:
        if (Button1 || Button2 || Button3 || Button4)
        {
            Setting_State = Setting_RetSet;
        }
        break;

        default:
        Setting_State = Setting_Start;
        break;
    }

    return Setting_State;
};

enum DrawingStates{Draw_Start, Draw_Cursor, Draw_Eraser, Draw_Line, Draw_EraseLCD, Draw_Ret} DrawingState;
int Tick_Draw(int Drawing_State)
{
    char XposChar[3];
    char YposChar[3];
    char DebugDraw[32] = "";

    //Actions
    switch(Drawing_State)
    {
        case Draw_Start:
        if(DrawingOn)
        {
            LCD_DisplayString(1, "B1:   B2:       B3:   B4: Exit");
            Custom_LCD_Draw(4,1);
            Custom_LCD_Draw(11,2);
            Custom_LCD_Draw(21,3);
            LCD_Cursor(33);

            LcdImage(Blank);
            LcdUpdate();
        }
        break;

        case Draw_Cursor:
        switch(JoystickPos(LeftRight,UpDown))
        {
            case 0:
            break;

            case 1:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Xpos -= 1;
                Ypos += 1;
            }
            break;

            case 2:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Ypos += 1;
            }
            break;

            case 3:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Xpos += 1;
                Ypos += 1;
            }
            break;

            case 4:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Xpos -= 1;
            }
            break;

            case 5:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Xpos += 1;
            }
            break;

            case 6:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Xpos -= 1;
                Ypos -= 1;
            }
            break;

            case 7:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Ypos -= 1;
            }
            break;

            case 8:
            if ( (Xpos < 84  && Xpos > 0 ) && (Ypos < 48 && Ypos > 1) )
            {
                Xpos += 1;
                Ypos -= 1;
            }
            break;
        }

        if(TESTING_DEBUGGER)
        {
            itoa(Xpos, XposChar ,10);
            itoa(Ypos, YposChar ,10);
            strcat(DebugDraw, "X: ");
            strcat(DebugDraw, XposChar);
            strcat(DebugDraw, " Y: ");
            strcat(DebugDraw, YposChar);
            LCD_DisplayString(1,DebugDraw);
        }
        
        LcdPixel(Xpos,Ypos, DrawMode);
        LcdUpdate();
        break;

        case Draw_Eraser:
        DrawMode = 0;
        break;

        case Draw_Line:
        DrawMode = 1;
        break;

        case Draw_EraseLCD:
        LcdClear();
        break;

        case Draw_Ret:
        LcdClear();
        LcdUpdate();
        DrawingOn = 0;
        MenuOn = 1;
        break;

        default:
        break;
    }

    //Transitions
    switch(Drawing_State)
    {
        case Draw_Start:
        if (DrawingOn)
        {
            Drawing_State = Draw_Cursor;
        }
        break;

        case Draw_Cursor:
        if (Button1)
        {
            Drawing_State = Draw_Line;
        }
        else if (Button2)
        {
            Drawing_State = Draw_Eraser;
        }
        else if (Button3)
        {
            Drawing_State = Draw_EraseLCD;
        }
        else if (Button4)
        {
            Drawing_State = Draw_Ret;
        }
        break;

        case Draw_Eraser:
        Drawing_State = Draw_Cursor;
        break;

        case Draw_Line:
        Drawing_State = Draw_Cursor;
        break;

        case Draw_EraseLCD:
        Drawing_State = Draw_Cursor;
        break;

        case Draw_Ret:
        Drawing_State = Draw_Start;
        break;

        default:
        Drawing_State = Draw_Start;
        break;
    }

    return Drawing_State;
};

int main(void)
{
    //PORT A
    DDRA = 0x00; //Configures Port -- 00 for Input, FF for Output
    PORTA = 0xFF; //Initializes the Value in the Port

    //PORT B
    DDRB = 0xFF; //Configures Port -- 00 for Input, FF for Output
    PORTB = 0x00; //Initializes the Value in the Port

    //PORT C
    DDRC = 0xFF; //Configures Port -- 00 for Input, FF for Output
    PORTC = 0x00; //Initializes the Value in the Port

    //PORT D
    DDRD = 0xFF; //Configures Port -- 00 for Input, FF for Output
    PORTD = 0x00; //Initializes the Value in the Port

    //"Period"
    unsigned short Count = 0;
    
    //Debugging Info
    char LRchar[4], UDchar[4];
    char LRRAWchar[6], UDRAWchar[6];

    //Initializing the 16x2 Custom Chars
    LCDdefinechar(Pencil, 1);
    LCDdefinechar(Eraser, 2);
    LCDdefinechar(ClearAll16x2, 3);

    //Decides which to use (Debug Purposes) Dont need to reprogram EEPROM
    if(TESTING_EE)
    {
        //Reads Contrast from EEPROM
        Contrast = eeprom_read_byte(&Contrast_EE);
        LR_ADC_MAX = eeprom_read_word(&LR_ADC_MAX_EE);
        UD_ADC_MAX = eeprom_read_word(&UD_ADC_MAX_EE);
    }
    else
    {
        //Hard Coded
        Contrast = 50;
        UD_ADC_MAX = 1023;
        LR_ADC_MAX = 986;
    }

    adc_init();
    LCD_init();
    LcdInit();

    LcdContrast(50);
    LcdUpdate();

    unsigned char CurrentMenuState = Menu_Start;
    unsigned char CurrentSettingState = Setting_Start;
    unsigned char CurrentDrawingState = Draw_Start;
    
    LcdImage(StartImg);
    LcdUpdate();

    while (1)
    {
        Button1 = ~PINA & 4;
        Button2 = ~PINA & 8;
        Button3 = ~PINA & 16;
        Button4 = ~PINA & 32;

        LeftRightRAW = adc_read(0);
        LeftRight = ADC_Scaler(LR_ADC_MAX, LeftRightRAW, 3);

        //UpDown is inverted physically for my project (UD is LR for schematics, due to board orientation)
        UpDownRAW =  UD_ADC_MAX * (1.0 - ((double)adc_read(1)/(double)UD_ADC_MAX));
        //Inverting can give neg values?
        if (UpDownRAW < 0)
        {
            UpDownRAW = 1;
        }
        UpDown = ADC_Scaler(UD_ADC_MAX, UpDownRAW, 3);

        //if (Count == 1000)
        //{
        //Count = 0;
        //
        //if(MenuOn)
        //{
        //CurrentMenuState = Tick_Menu(CurrentMenuState);
        //}
        //else if (SettingsOn)
        //{
        //CurrentSettingState = Tick_Settings(CurrentSettingState);
        //}
        //else if (DrawingOn)
        //{
        //CurrentDrawingState = Tick_Draw(CurrentDrawingState);
        //}
        //
        //
        //}

        char Buffer[32] = "LR:";

        itoa(LeftRight,LRchar,10);
        itoa(UpDown,UDchar,10);

        itoa(LeftRightRAW,LRRAWchar,10);
        itoa(UpDownRAW,UDRAWchar,10);

        strcat(Buffer,LRchar);
        strcat(Buffer, " ");
        strcat(Buffer,LRRAWchar);
        strcat(Buffer," UD:");
        strcat(Buffer,UDchar);
        strcat(Buffer, " ");
        strcat(Buffer,UDRAWchar);

        if(Count == 500)
        {

            LcdContrast(Contrast);
            LcdImage(StartImg);
            LcdUpdate();

            LCD_DisplayString(1,Buffer);

            Count = 0;
        }

        Count++;
    }
}

void adc_init()
{
    // AREF = AVcc
    ADMUX = (1<<REFS0);

    // ADC Enable and prescaler of 128
    // 16000000/128 = 125000
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(uint8_t ch)
{
    // select the corresponding channel 0~7
    // ANDing with ⷿ will always keep the value
    // of ᣨ⠢etween 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing

    // start single convertion
    // write Ɀ to ADSC
    ADCSRA |= (1<<ADSC);

    // wait for conversion to complete
    // ADSC becomes ⰿ again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));

    return (ADC);
}

void LCDdefinechar(const uint8_t *pc, uint8_t char_code)
{
    uint8_t a, pcc;
    uint16_t i;
    a=(char_code<<3)|0x40;
    for (i=0; i<8; i++)
    {
        pcc=pgm_read_byte(&pc[i]);
        LCD_WriteCommand(a++);
        LCD_WriteData(pcc);
    }
}

unsigned short ADC_Scaler(unsigned short ADC_Max, unsigned short ADC_Val, unsigned char Divider)
{
    unsigned short Div = ADC_Max / Divider;
    unsigned char Position;

    for(unsigned char index = 1; index <= Divider; index++)
    {
        //Sets boundaries
        if((ADC_Val <= index*Div) && (ADC_Val >= (index-1)*Div) )
        {
            Position = index;
        }
    }
    return Position;
}

/*
6 | 7 | 8
4 | 0 | 5
1 | 2 | 3
*/
unsigned char JoystickPos(unsigned char LR, unsigned char UD)
{
    if ((LR == 1) && (UD == 1))
    {
        return 1;
    }

    if ((LR == 2) && (UD == 1))
    {
        return 2;
    }

    if ((LR == 3) && (UD == 1))
    {
        return 3;
    }

    if ((LR == 1) && (UD == 2))
    {
        return 4;
    }

    if ((LR == 2) && (UD == 2))
    {
        return 0;
    }

    if ((LR == 3) && (UD == 2))
    {
        return 5;
    }

    if ((LR == 1) && (UD == 3))
    {
        return 6;
    }

    if ((LR == 2) && (UD == 3))
    {
        return 7;
    }

    if ((LR == 3) && (UD == 3))
    {
        return 8;
    }
}

/*
//MAIN WHILE
char Buffer[32] = "LR:";

itoa(LeftRight,LRchar,10);
itoa(UpDown,UDchar,10);

strcat(Buffer,LRchar);
strcat(Buffer," UD:");
strcat(Buffer,UDchar);

if(Count == 500)
{
if ((JoystickPos(LeftRight, UpDown) == 2) && (Contrast < 100))
{
Contrast += 10;
}

if ((JoystickPos(LeftRight, UpDown) == 5) && (Contrast > 10))
{
Contrast -= 10;
}

LcdContrast(Contrast);
LcdImage(StartImg);
LcdUpdate();

char ContrastChar[3], randomChar[3];
itoa(Contrast,ContrastChar,10);

strcat(Buffer," CT:");
strcat(Buffer,ContrastChar);
strcat(Buffer," RND:");
strcat(Buffer,randomChar);
LCD_DisplayString(1,Buffer);


LCDdefinechar(Pencil, 1);
Custom_LCD_Draw(23,1);
LCDdefinechar(Eraser, 2);
Custom_LCD_Draw(24,2);
LCDdefinechar(ClearAll16x2, 3);
Custom_LCD_Draw(25,3);

Count = 0;
}
*/