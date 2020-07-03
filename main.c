#pragma config OSC = INTIO67 // Oscillator Selection bits
#pragma config WDT = OFF     // Watchdog Timer Enable bit 
#pragma config PWRT = OFF    // Power-up Enable bit
#pragma config BOREN = ON    // Brown-out Reset Enable bit
#pragma config PBADEN = OFF  // Watchdog Timer Enable bit 
#pragma config LVP = OFF     // Low Voltage (single -supply) In-Circute Serial Pragramming Enable bit
#pragma config CPD = OFF     // Data EEPROM　Memory Code Protection bit (Data EEPROM code protection off)
#include <xc.h>
#define _XTAL_FREQ 500000

volatile int a=0;


void move_forward() // move forward
{
    PORTDbits.RD0 = 0;
    PORTDbits.RD1 = 1;
    PORTDbits.RD2 = 0;
    PORTDbits.RD3 = 1;   
}
void move_back() // move back
{
    LATCbits.LATC5 = 1;
    LATCbits.LATC4 = 1;
    PORTDbits.RD0 = 1;
    PORTDbits.RD1 = 0;
    PORTDbits.RD2 = 1;
    PORTDbits.RD3 = 0;
    for(int i=0; i<750; i+=250)
    {
        LATCbits.LATC5 ^= 1;
        LATCbits.LATC4 ^= 1;
        __delay_ms(250);   
    }
    
}
void move_right() // move right
{
    LATCbits.LATC4 = 1;
    PORTDbits.RD0 = 1;
    PORTDbits.RD1 = 1;
    PORTDbits.RD2 = 0;
    PORTDbits.RD3 = 1;   
    __delay_ms(500);   
}
void move_left() // move left
{
    LATCbits.LATC5 = 1;
    PORTDbits.RD0 = 0;
    PORTDbits.RD1 = 1;
    PORTDbits.RD2 = 1;
    PORTDbits.RD3 = 1;   
    __delay_ms(500);   
}
void stop() // no move
{
    PORTDbits.RD0 = 1;
    PORTDbits.RD1 = 1;
    PORTDbits.RD2 = 1;
    PORTDbits.RD3 = 1;   
}
int calculate_distance(){
    int b=0;
    TMR1H = 0;                //Sets the Initial Value of Timer
    TMR1L = 0;                //Sets the Initial Value of Timer
    
    //set the trigger
    LATBbits.LATB0 = 1;
    __delay_us(10);           //10uS Delay 
    LATBbits.LATB0 = 0;
    
    //read the echo
    while(PORTBbits.RB1==0);              //Waiting for Echo
    TMR1ON = 1;               //Timer Starts
    while(PORTBbits.RB1==1);               //Waiting for Echo goes LOW
    TMR1ON = 0;               //Timer Stops
    
    //calculate the distance in real world
    b = (TMR1L | (TMR1H<<8)); //Reads Timer Value
    b = b/3.676;              //Converts Time to Distance
    b = b + 1;                //Distance Calibration
    
    return b;
}
void motor_initial(){
    //set the PWM period by writing to PR2 register
    PR2 = 156;
    
    //set the PWM duty cycle by writing to the CCPRxL register and CCPxCON<5:4> bits
    CCPR1L = 0b00000011;
    
    //set the CCP1CON
    CCP1CON = 0x3c; //3 for ccp1con<5:4> contribute the 11 for last two bit c:set the pwm mode
    
    //make the CCP1 pin an output by clearing the appropriate TRIS bit
    TRISC = 0;
    
    //set the TMR2 prescale value,then enable timer2 by writing to T2CON
    T2CON = 0b00000111;
    TMR2 = 0;
}
void motor_move(int mode){
    // right
    if(mode == 1){ 
        CCPR1L = 5;
    }
    // middle
    else if(mode == 2){ 
        CCPR1L = 11;
    }
    // left
    else{ 
        CCPR1L = 21;
    }
}
void setting()
{
    // for interrupt
    RCONbits.IPEN = 1;
    INTCONbits.GIE = 1;
    INTCON3bits.INT2F = 0;
    INTCON3bits.INT2E = 1;
    
    // frequency of oscillator 
    OSCCONbits.IRCF = 0b011;
   
    // for US sensor
    TRISBbits.RB0 = 0; // trigger output pin 
    TRISBbits.RB1 = 1; // echo input pin
    T1CON = 0x10; // set the prescaler to 2 for US sensor
    
    // for engine
    TRISD = 0;
    PORTDbits.RD0 = 1;
    PORTDbits.RD1 = 1;
    PORTDbits.RD2 = 1;
    PORTDbits.RD3 = 1;   
    
    // for direction light
    TRISC = 0;
    LATCbits.LATC5 = 0;
    LATCbits.LATC4 = 0;
    
    // for motor
    motor_initial();
}
void __interrupt(high_priority) ISR()
{
    INTCON3bits.INT2F = 0;
    LATCbits.LATC5 = 1;
    LATCbits.LATC4 = 1;
     __delay_ms(3000);   
    setting(); // initial setting
    stop(); // stop engine
    motor_move(2); // set motor to middle
}
void main()
{     
    setting(); // initial setting
    stop(); // stop engine
    motor_move(2); // set motor to middle

    while(1)
    {
        // calculate distance forward right now
        a = calculate_distance(); 
        
        // turn off all lights
        LATCbits.LATC5 = 0;
        LATCbits.LATC4 = 0;
        // still move forward
        if(a > 15)
        {
            move_forward();
        }
        // use US sensor to decide which side will be turned
        else
        {
            int right_d, left_d;
            move_back();
            stop();
            
            motor_move(3);
            __delay_ms(1000);
            left_d = calculate_distance();
  
            motor_move(1);
            __delay_ms(1000);
            right_d = calculate_distance();        
                    
            motor_move(2);
            __delay_ms(1000);
            
            if(right_d >= left_d)
            {
                move_right();
            }
            else
            {
                move_left();
            }
        }
    }
}
