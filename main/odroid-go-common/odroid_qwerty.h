
#ifndef ODROID_K_QWERTY_H
#define ODROID_K_QWERTY_H
#include <stdbool.h>
#include <stdint.h>

#define ODROID_QWERTY_MAX_KEYS 6

    // row 1
#define ODROID_QWERTY_TILDE  0x01
#define ODROID_QWERTY_1  0x02
#define ODROID_QWERTY_2  0x03
#define ODROID_QWERTY_3  0x04
#define ODROID_QWERTY_4  0x05
#define ODROID_QWERTY_5  0x06
#define ODROID_QWERTY_6  0x07
#define ODROID_QWERTY_7  0x08
#define ODROID_QWERTY_8  0x0b
#define ODROID_QWERTY_9  0x0c
#define ODROID_QWERTY_0  0x0d
    
    // row 2
#define ODROID_QWERTY_ESC  0x0e
#define ODROID_QWERTY_Q  0x0f
#define ODROID_QWERTY_W  0x10
#define ODROID_QWERTY_E  0x11
#define ODROID_QWERTY_R  0x12
#define ODROID_QWERTY_T  0x15
#define ODROID_QWERTY_Y  0x16
#define ODROID_QWERTY_U  0x17
#define ODROID_QWERTY_I  0x18
#define ODROID_QWERTY_O  0x19
#define ODROID_QWERTY_P  0x1a
    
    
    // row 3
#define ODROID_QWERTY_CTRL  0x1b
#define ODROID_QWERTY_A  0x1c
#define ODROID_QWERTY_S  0x1f
#define ODROID_QWERTY_D  0x20
#define ODROID_QWERTY_F  0x21
#define ODROID_QWERTY_G  0x22
#define ODROID_QWERTY_H  0x23
#define ODROID_QWERTY_J  0x24
#define ODROID_QWERTY_K  0x25
#define ODROID_QWERTY_L  0x26
#define ODROID_QWERTY_BS 0x29


// row 4
#define ODROID_QWERTY_ALT  0x2a
#define ODROID_QWERTY_Z  0x2b
#define ODROID_QWERTY_X  0x2c
#define ODROID_QWERTY_C  0x2d
#define ODROID_QWERTY_V  0x2e
#define ODROID_QWERTY_B  0x2f
#define ODROID_QWERTY_N  0x30
#define ODROID_QWERTY_M  0x33
#define ODROID_QWERTY_BACKSLASH  0x34
#define ODROID_QWERTY_ENTER  0x35

// row 5
#define ODROID_QWERTY_SHIFT  0x36
#define ODROID_QWERTY_SEMICOLON  0x37
#define ODROID_QWERTY_QUOT  0x38
#define ODROID_QWERTY_DASH  0x39
#define ODROID_QWERTY_EQUAL  0x3a
#define ODROID_QWERTY_SPACE  0x3d
#define ODROID_QWERTY_COMMA  0x3e
#define ODROID_QWERTY_POINT  0x3f
#define ODROID_QWERTY_SLASH  0x40
#define ODROID_QWERTY_BRACKET_OPEN  0x41
#define ODROID_QWERTY_BRACKET_CLOSE  0x42
    
#define ODROID_QWERTY_NONE 0xff


typedef struct
{
    uint8_t values[ODROID_QWERTY_MAX_KEYS];
} odroid_qwerty_state;


#ifdef __cplusplus
extern "C" {
#endif
/* Initialize the keyboard. Will Return false, if there no keyboard was found */
bool odroid_qwerty_init();

/* terminate the keyboard */
void odroid_qwerty_terminate();

/* was a keyboard initialized and found? */
bool odroid_qwerty_alive();

/* read the keys.  6 (ODROID_QWERTY_MAX_KEYS) keys can be pressed at once, but only one keypress/unpress will change every time you call this function,
 so can't miss a keypres / keyunpress. 
 */
bool odroid_qwerty_read(odroid_qwerty_state* keys);

/* get the ASCII code for the corresponding key. Upper keys with shift = true */
char odroid_qwerty_key_to_ascii(uint8_t key, bool shift);

/* is a button pressed? */
bool odroid_qwerty_is_key_pressed(odroid_qwerty_state* keys, uint8_t key);

/* turn the Aa, Fn or St led's on or off */
void odroid_qwerty_led_Aa( bool on);
void odroid_qwerty_led_Fn( bool on);
void odroid_qwerty_led_St( bool on);

#ifdef __cplusplus
}
#endif

#endif /* ODROID_QWERTY_H */

