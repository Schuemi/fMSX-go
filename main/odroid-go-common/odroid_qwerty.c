#include "odroid_qwerty.h"
#include "driver/i2c.h"
#include <string.h>

// i2c
#define I2C_QWERTY_MASTER_NUM I2C_NUM_1        
#define I2C_QWERTY_MASTER_SDA_IO 15
#define I2C_QWERTY_MASTER_SCL_IO 12
#define I2C_QWERTY_MASTER_FREQ_HZ 100000
#define I2C_QWERTY_MASTER_RX_BUF_DISABLE  0
#define I2C_QWERTY_MASTER_TX_BUF_DISABLE 0
#define I2C_QWERTY_ADDRESS 0x34



// interrupt
#define I2C_QWERTY_MASTER_INT  4
#define I2C_QWERTY_MASTER_INT_SEL  (1ULL<<I2C_QWERTY_MASTER_INT)
#define I2C_QWERTY_MASTER_FLAG_DEFAULT 0




#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

#define INT_STAT_CAD_INT 0x10
#define INT_STAT_OVR_FLOW_INT 0x08
#define INT_STAT_K_LCK_INT 0x04
#define INT_STAT_GPI_INT 0x02
#define INT_STAT_K_INT 0x01


/* TCA8418 hardware limits */
#define TCA8418_MAX_ROWS 8
#define TCA8418_MAX_COLS 10

/* TCA8418 register offsets */
#define REG_CFG 0x01
#define REG_INT_STAT 0x02
#define REG_KEY_LCK_EC 0x03
#define REG_KEY_EVENT_A 0x04
#define REG_KEY_EVENT_B 0x05
#define REG_KEY_EVENT_C 0x06
#define REG_KEY_EVENT_D 0x07
#define REG_KEY_EVENT_E 0x08
#define REG_KEY_EVENT_F 0x09
#define REG_KEY_EVENT_G 0x0A
#define REG_KEY_EVENT_H 0x0B
#define REG_KEY_EVENT_I 0x0C
#define REG_KEY_EVENT_J 0x0D
#define REG_KP_LCK_TIMER 0x0E
#define REG_UNLOCK1 0x0F
#define REG_UNLOCK2 0x10
#define REG_GPIO_INT_STAT1 0x11
#define REG_GPIO_INT_STAT2 0x12
#define REG_GPIO_INT_STAT3 0x13
#define REG_GPIO_DAT_STAT1 0x14
#define REG_GPIO_DAT_STAT2 0x15
#define REG_GPIO_DAT_STAT3 0x16
#define REG_GPIO_DAT_OUT1 0x17
#define REG_GPIO_DAT_OUT2 0x18
#define REG_GPIO_DAT_OUT3 0x19
#define REG_GPIO_INT_EN1 0x1A
#define REG_GPIO_INT_EN2 0x1B
#define REG_GPIO_INT_EN3 0x1C
#define REG_KP_GPIO1 0x1D
#define REG_KP_GPIO2 0x1E
#define REG_KP_GPIO3 0x1F
#define REG_GPI_EM1 0x20
#define REG_GPI_EM2 0x21
#define REG_GPI_EM3 0x22
#define REG_GPIO_DIR1 0x23
#define REG_GPIO_DIR2 0x24
#define REG_GPIO_DIR3 0x25
#define REG_GPIO_INT_LVL1 0x26
#define REG_GPIO_INT_LVL2 0x27
#define REG_GPIO_INT_LVL3 0x28
#define REG_DEBOUNCE_DIS1 0x29
#define REG_DEBOUNCE_DIS2 0x2A
#define REG_DEBOUNCE_DIS3 0x2B
#define REG_GPIO_PULL1 0x2C
#define REG_GPIO_PULL2 0x2D
#define REG_GPIO_PULL3 0x2E

/* TCA8418 bit definitions */
#define CFG_AI 0x80
#define CFG_GPI_E_CFG 0x40
#define CFG_OVR_FLOW_M 0x20
#define CFG_INT_CFG 0x10
#define CFG_OVR_FLOW_IEN 0x08
#define CFG_K_LCK_IEN 0x04
#define CFG_GPI_IEN 0x02
#define CFG_KE_IEN 0x01



/* TCA8418 register masks */
#define KEY_LCK_EC_KEC 0x7
#define KEY_EVENT_CODE 0x7f
#define KEY_EVENT_VALUE 0x80

/* TCA8418 Rows and Columns */
#define ROW0 0x01
#define ROW1 0x02
#define ROW2 0x04
#define ROW3 0x08
#define ROW4 0x10
#define ROW5 0x20
#define ROW6 0x40
#define ROW7 0x80

#define COL0 0x0001
#define COL1 0x0002
#define COL2 0x0004
#define COL3 0x0008
#define COL4 0x0010
#define COL5 0x0020
#define COL6 0x0040
#define COL7 0x0080
#define COL8 0x0100
#define COL9 0x0200

static odroid_qwerty_state pressedKeys;
/* private funktions*/
static esp_err_t i2c_qwerty_master_write_byte(uint8_t data, uint8_t reg);
static esp_err_t i2c_qwerty_master_read_byte(uint8_t *data, uint8_t reg);
static bool i2c_qwerty_configureKeys(uint8_t rows, uint16_t cols, uint8_t config);
static void odroid_qwerty_pressKey(uint8_t key);
static void odroid_qwerty_releaseKey(uint8_t key);
static uint8_t i2c_qwerty_getInterruptStatus(void);
static bool i2c_qwerty_resetInterruptStatus(void);
static uint8_t i2c_qwerty_getKeyEventCount(void);
static uint8_t i2c_qwerty_getKeyEvent(uint8_t event);

static bool keyboardFound = false;
static bool keyInt = false;
static bool keyOverflow = false;
static bool AaOn = false;
static bool FnOn = false;
static bool StOn = false;
static void IRAM_ATTR odroid_qwerty_gpio_isr_handler(void* arg)
{
    //uint32_t gpio_num = (uint32_t) arg;
    keyInt = true;
}

void odroid_qwerty_pressKey(uint8_t key)
{
    int k = 0;
    while(pressedKeys.values[k] != ODROID_QWERTY_NONE && pressedKeys.values[k] != key && k < ODROID_QWERTY_MAX_KEYS) k++;
    if (k == ODROID_QWERTY_MAX_KEYS) return;
    pressedKeys.values[k] = key;
}
void odroid_qwerty_releaseKey(uint8_t key)
{
    int k = 0;
    while(pressedKeys.values[k] != key && k < ODROID_QWERTY_MAX_KEYS) k++;
    if (k == ODROID_QWERTY_MAX_KEYS) return;
    pressedKeys.values[k] = ODROID_QWERTY_NONE;
}

bool odroid_qwerty_read(odroid_qwerty_state* keys)
{
    if (! keyboardFound) return false;
    bool changed = false;
    if (keyInt) {
        uint8_t key = i2c_qwerty_getKeyEvent(0);
        
        if (keyOverflow) {
            // a keyboard overflow occurred. Reset all.
            memset(pressedKeys.values, ODROID_QWERTY_NONE, ODROID_QWERTY_MAX_KEYS);
            keyOverflow = 0;
        } else {
            if(key & 0x80) odroid_qwerty_pressKey(key&0x7F); else odroid_qwerty_releaseKey(key&0x7F);
            changed = true;
            memcpy(keys->values, pressedKeys.values, ODROID_QWERTY_MAX_KEYS);
            
        }
        keyInt = 0;
        i2c_qwerty_resetInterruptStatus();
        
    }
    
    return changed;
}
void odroid_qwerty_led_Fn( bool on) {
    char byte = 0;
    if (FnOn) byte = 2;
    if (on) byte += 1;
    i2c_qwerty_master_write_byte(byte, REG_GPIO_DAT_OUT3);
    AaOn = on;
    
}
void odroid_qwerty_led_Aa( bool on) {
    char byte = 0;
    if (AaOn) byte = 1;
    if (on) byte += 2;
    i2c_qwerty_master_write_byte(byte, REG_GPIO_DAT_OUT3);
    FnOn = on;
}
void odroid_qwerty_led_St( bool on) {
    if (on)
        i2c_qwerty_master_write_byte(0x80, REG_GPIO_DAT_OUT1);
    else
        i2c_qwerty_master_write_byte(0x00, REG_GPIO_DAT_OUT1);
    
    StOn = on;
}
bool odroid_qwerty_init()
{
    int i2c_master_port = I2C_QWERTY_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_QWERTY_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_QWERTY_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_QWERTY_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_QWERTY_MASTER_RX_BUF_DISABLE,
                       I2C_QWERTY_MASTER_TX_BUF_DISABLE, 0);
    bool ack = i2c_qwerty_configureKeys(ROW0 | ROW1 | ROW2 | ROW3 | ROW4 | ROW5 | ROW6,
                                    COL0 | COL1 | COL2 | COL3 | COL4 | COL5 | COL6 | COL7, 
                                    CFG_KE_IEN | CFG_OVR_FLOW_IEN | CFG_INT_CFG | CFG_OVR_FLOW_M);
    
    memset(pressedKeys.values, ODROID_QWERTY_NONE, ODROID_QWERTY_MAX_KEYS);
    
    if (ack) {
        
        i2c_qwerty_master_write_byte(0x80, REG_GPIO_DIR1); //- ROW7 GPIO data direction OUTPUT
        i2c_qwerty_master_write_byte(0x3, REG_GPIO_DIR3); //- COL8, COL9 GPIO data direction OUTPUT
        odroid_qwerty_led_Aa(false);
        odroid_qwerty_led_St(false);
        odroid_qwerty_led_Fn(false);
        
        
    
        // init iterrupt
        gpio_config_t io_conf;
        //interrupt of rising edge
        io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
        //bit mask of the pins
        io_conf.pin_bit_mask = I2C_QWERTY_MASTER_INT_SEL;
        //set as input mode    
        io_conf.mode = GPIO_MODE_INPUT;
        //disable pull-down mode
        io_conf.pull_down_en = 0;
        //enable pull-up mode
        io_conf.pull_up_en = 1;
        gpio_config(&io_conf);

        //install gpio isr service
        esp_err_t err = gpio_install_isr_service(I2C_QWERTY_MASTER_FLAG_DEFAULT);
        
        //hook isr handler for specific gpio pin
        err = gpio_isr_handler_add(I2C_QWERTY_MASTER_INT, odroid_qwerty_gpio_isr_handler, (void*) I2C_QWERTY_MASTER_INT);
        
        keyboardFound = true;
    
    } else {
        i2c_driver_delete(i2c_master_port);
    }
    return ack;
    
}
bool odroid_qwerty_alive() {
    return keyboardFound;
}
void odroid_qwerty_terminate() {
    if (keyboardFound) {
        gpio_isr_handler_remove(I2C_QWERTY_MASTER_INT);
        gpio_uninstall_isr_service();
        i2c_driver_delete(I2C_QWERTY_MASTER_NUM);
        keyboardFound = false;
    }
}
uint8_t i2c_qwerty_getKeyEventCount(void) {
  uint8_t count;
  i2c_qwerty_master_read_byte(&count, REG_KEY_LCK_EC);
  return(count & 0x0F);
}
uint8_t i2c_qwerty_getKeyEvent(uint8_t event) {
  uint8_t reg;
  uint8_t keycode = 0;
  
  if (event > 9)
    return 0x0;
  
   i2c_qwerty_master_read_byte(&reg, REG_INT_STAT);
   if(reg & INT_STAT_K_INT)
      i2c_qwerty_master_read_byte(&keycode, (REG_KEY_EVENT_A+event));
   else
      keycode = 0x0;
   
   if(reg & INT_STAT_OVR_FLOW_INT) keyOverflow = true;
  return(keycode);
}

bool i2c_qwerty_resetInterruptStatus(void)
{
    if (i2c_qwerty_master_write_byte(0x0f, REG_INT_STAT) != ESP_OK) return false;
    return true;
}
bool i2c_qwerty_configureKeys(uint8_t rows, uint16_t cols, uint8_t config)
{
  //Pins all default to GPIO. pinMode(x, KEYPAD); may be used for individual pins.
  if (i2c_qwerty_master_write_byte(rows, REG_KP_GPIO1) != ESP_OK) return false;
  uint8_t col_tmp;
  col_tmp = (uint8_t)(0xff & cols);
  if (i2c_qwerty_master_write_byte(col_tmp, REG_KP_GPIO2) != ESP_OK) return false;
  col_tmp = (uint8_t)(0x03 & (cols>>8));
  if (i2c_qwerty_master_write_byte(col_tmp, REG_KP_GPIO3) != ESP_OK) return false;
  config |= CFG_AI;
  if (i2c_qwerty_master_write_byte(config, REG_CFG) != ESP_OK) return false;
  return true;
}
uint8_t i2c_qwerty_getInterruptStatus(void) {
  uint8_t status;
  i2c_qwerty_master_read_byte(&status, REG_INT_STAT);
  return(status & 0x0F);
}
static esp_err_t i2c_qwerty_master_read_byte(uint8_t *data, uint8_t reg)
{
    esp_err_t ret;
    i2c_cmd_handle_t cmd;
    //Set active register
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_QWERTY_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN); 
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_QWERTY_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    //read register
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( I2C_QWERTY_ADDRESS << 1 ) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_QWERTY_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
    
    
}
static esp_err_t i2c_qwerty_master_write_byte(uint8_t data, uint8_t reg)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret;
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( I2C_QWERTY_ADDRESS << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_QWERTY_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
bool odroid_qwerty_is_key_pressed(odroid_qwerty_state* keys, uint8_t key)
{
    int k = 0;
    while(keys->values[k] != key && k < ODROID_QWERTY_MAX_KEYS) k++;
    if (k == ODROID_QWERTY_MAX_KEYS) return false;
    return true;
}
char odroid_qwerty_key_to_ascii(uint8_t key, bool shift) {
    if (shift) switch (key) {
        case ODROID_QWERTY_TILDE: return '~';
        case ODROID_QWERTY_1: return '!';
        case ODROID_QWERTY_2: return '@';
        case ODROID_QWERTY_3: return '#';
        case ODROID_QWERTY_4: return '$';
        case ODROID_QWERTY_5: return '%';
        case ODROID_QWERTY_6: return '^';
        case ODROID_QWERTY_7: return '&';
        case ODROID_QWERTY_8: return '*';
        case ODROID_QWERTY_9: return '(';
        case ODROID_QWERTY_0: return ')';
        case ODROID_QWERTY_ESC: return 0x1b;
        case ODROID_QWERTY_Q: return 'Q';
        case ODROID_QWERTY_W: return 'W';
        case ODROID_QWERTY_E: return 'E';
        case ODROID_QWERTY_R: return 'R';
        case ODROID_QWERTY_T: return 'T';
        case ODROID_QWERTY_Y: return 'Y';
        case ODROID_QWERTY_U: return 'U';
        case ODROID_QWERTY_I: return 'I';
        case ODROID_QWERTY_O: return 'O';
        case ODROID_QWERTY_P: return 'P';
//      case ODROID_QWERTY_CTRL: return 0x00;
        case ODROID_QWERTY_A: return 'A';
        case ODROID_QWERTY_S: return 'S';
        case ODROID_QWERTY_D: return 'D';
        case ODROID_QWERTY_F: return 'F';
        case ODROID_QWERTY_G: return 'G';
        case ODROID_QWERTY_H: return 'H';
        case ODROID_QWERTY_J: return 'J';
        case ODROID_QWERTY_K: return 'K';
        case ODROID_QWERTY_L: return 'L';
        case ODROID_QWERTY_BS: return 0x08;
//      case ODROID_QWERTY_ALT: return 0x00;
        case ODROID_QWERTY_Z: return 'Z';
        case ODROID_QWERTY_X: return 'X';
        case ODROID_QWERTY_C: return 'C';
        case ODROID_QWERTY_V: return 'V';
        case ODROID_QWERTY_B: return 'B';
        case ODROID_QWERTY_N: return 'N';
        case ODROID_QWERTY_M: return 'M';
        case ODROID_QWERTY_BACKSLASH: return 0x5C;
        case ODROID_QWERTY_ENTER: return '\n';

//        case ODROID_QWERTY_SHIFT: return 0x00;
        case ODROID_QWERTY_SEMICOLON: return ':';
        case ODROID_QWERTY_QUOT: return '"';
        case ODROID_QWERTY_DASH: return '_';
        case ODROID_QWERTY_EQUAL: return '+';
        case ODROID_QWERTY_SPACE: return ' ';
        case ODROID_QWERTY_COMMA: return '<';
        case ODROID_QWERTY_POINT: return '>';
        case ODROID_QWERTY_SLASH: return '?';
        case ODROID_QWERTY_BRACKET_OPEN: return '{';
        case ODROID_QWERTY_BRACKET_CLOSE: return '}';
        
        default: return 0x00;
    } 
    
    if (! shift) switch (key) {
        case ODROID_QWERTY_TILDE: return 0x7e;
        case ODROID_QWERTY_1: return '1';
        case ODROID_QWERTY_2: return '2';
        case ODROID_QWERTY_3: return '3';
        case ODROID_QWERTY_4: return '4';
        case ODROID_QWERTY_5: return '5';
        case ODROID_QWERTY_6: return '6';
        case ODROID_QWERTY_7: return '7';
        case ODROID_QWERTY_8: return '8';
        case ODROID_QWERTY_9: return '9';
        case ODROID_QWERTY_0: return '0';
        case ODROID_QWERTY_ESC: return 0x1b;
        case ODROID_QWERTY_Q: return 'q';
        case ODROID_QWERTY_W: return 'w';
        case ODROID_QWERTY_E: return 'e';
        case ODROID_QWERTY_R: return 'r';
        case ODROID_QWERTY_T: return 't';
        case ODROID_QWERTY_Y: return 'y';
        case ODROID_QWERTY_U: return 'u';
        case ODROID_QWERTY_I: return 'i';
        case ODROID_QWERTY_O: return 'o';
        case ODROID_QWERTY_P: return 'p';
//      case ODROID_QWERTY_CTRL: return 0x00;
        case ODROID_QWERTY_A: return 'a';
        case ODROID_QWERTY_S: return 's';
        case ODROID_QWERTY_D: return 'd';
        case ODROID_QWERTY_F: return 'f';
        case ODROID_QWERTY_G: return 'g';
        case ODROID_QWERTY_H: return 'h';
        case ODROID_QWERTY_J: return 'j';
        case ODROID_QWERTY_K: return 'k';
        case ODROID_QWERTY_L: return 'l';
        case ODROID_QWERTY_BS: return 0x08;
//      case ODROID_QWERTY_ALT: return 0x00;
        case ODROID_QWERTY_Z: return 'z';
        case ODROID_QWERTY_X: return 'x';
        case ODROID_QWERTY_C: return 'c';
        case ODROID_QWERTY_V: return 'v';
        case ODROID_QWERTY_B: return 'b';
        case ODROID_QWERTY_N: return 'n';
        case ODROID_QWERTY_M: return 'm';
        case ODROID_QWERTY_BACKSLASH: return 0x5C;
        case ODROID_QWERTY_ENTER: return '\n';

//        case ODROID_QWERTY_SHIFT: return 0x00;
        case ODROID_QWERTY_SEMICOLON: return ';';
        case ODROID_QWERTY_QUOT: return '\'';
        case ODROID_QWERTY_DASH: return '-';
        case ODROID_QWERTY_EQUAL: return '=';
        case ODROID_QWERTY_SPACE: return ' ';
        case ODROID_QWERTY_COMMA: return ',';
        case ODROID_QWERTY_POINT: return '.';
        case ODROID_QWERTY_SLASH: return '/';
        case ODROID_QWERTY_BRACKET_OPEN: return '[';
        case ODROID_QWERTY_BRACKET_CLOSE: return ']';
        
        default: return 0x00;
    }
    
    
}


