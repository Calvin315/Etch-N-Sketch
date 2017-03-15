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

typedef struct {
    uint16_t CursorPos;
    unsigned char *board;
}Canvas;

//Saved Data Var with Default Vals
uint8_t EEMEM Contrast_EE = 50;
uint16_t EEMEM LR_ADC_MAX_EE = 1023;
uint16_t EEMEM UD_ADC_MAX_EE = 986;

//Uses EE PROM if 1
unsigned char TESTING_EE = 0;

//Functions
void adc_init();
unsigned short ADC_Scaler(unsigned short ADC_Max, unsigned short ADC_Val, unsigned char Divider);
uint16_t adc_read(uint8_t ch);
void LCDdefinechar(const uint8_t *pc, uint8_t char_code);



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
    /* Replace with your application code */

    unsigned short LeftRight, UpDown, LeftRightRAW, UpDownRAW, LR_ADC_MAX, UD_ADC_MAX;
    unsigned char Contrast;
    unsigned short Count = 0;

    char LRchar[4], UDchar[4];

    Canvas Current;
    Current.CursorPos = (MAX_RES/2);
    Current.board = Blank;

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
        LR_ADC_MAX = 1023;
        UD_ADC_MAX = 986;
    }

    adc_init();
    LCD_init();
    LcdInit();
    
    LCDdefinechar(Pencil, 1);
    LCDdefinechar(Eraser, 2);
    LCDdefinechar(ClearAll16x2, 3);

    srand(0);

    while (1)
    {
        char Buffer[32] = "LR:";

        LeftRightRAW = adc_read(0);
        LeftRight = ADC_Scaler(LR_ADC_MAX, LeftRightRAW, 3);

        //UpDown is inverted physically for my project (UD is LR for schematics, due to board orientation)
        UpDownRAW =  UD_ADC_MAX * (1.0 - ((double)adc_read(1)/(double)UD_ADC_MAX));
        UpDown = ADC_Scaler(UD_ADC_MAX, UpDownRAW, 3);

        itoa(LeftRight,LRchar,10);
        itoa(UpDown,UDchar,10);

        strcat(Buffer,LRchar);
        strcat(Buffer," UD:");
        strcat(Buffer,UDchar);

        if(Count == 500)
        {

            if ((LeftRight == 3) && (Contrast < 100))
            {
                Contrast += 10;
            }

            if ((UpDown == 3) && (Contrast > 10))
            {
                Contrast -= 10;
            }

            LcdContrast(Contrast);
            LcdImage(Start);
            LcdUpdate();
            
            char ContrastChar[3], randomChar[3];
            itoa(Contrast,ContrastChar,10);
            itoa((rand()%100),randomChar,10);

            strcat(Buffer," CT:");
            strcat(Buffer,ContrastChar);
            strcat(Buffer," RND:");
            strcat(Buffer,randomChar);
            LCD_DisplayString(1,Buffer);
            
            Custom_LCD_Draw(23,1);
            Custom_LCD_Draw(24,2);
            Custom_LCD_Draw(25,3);
            
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