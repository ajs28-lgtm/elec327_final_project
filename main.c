#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// HARDWARE DEFINITIONS & PIN MAPPING
// ============================================================================

// LEDs (PA5 to PA12)
#define LED1 (1u << 5)
#define LED2 (1u << 6)
#define LED3 (1u << 7)
#define LED4 (1u << 8)
#define LED5 (1u << 9)
#define LED6 (1u << 10)
#define LED7 (1u << 11)
#define LED8 (1u << 12)
#define LED_ALL (LED1 | LED2 | LED3 | LED4 | LED5 | LED6 | LED7 | LED8)

// Buzzer (PA22)
#define BUZZER (1u << 22)

// I2C and IRQ Pins
#define SDA_PIN (1u << 0)  // PA0
#define SCL_PIN (1u << 1)  // PA1
#define IRQ_PIN (1u << 13) // PA13

// Buttons are wired to GND
// They are active-low and need internal pull-ups
#define BUTTON_OCTAVE (1u << 26) // PA26 / BUTTON1
#define BUTTON_RECORD (1u << 25) // PA25 / BUTTON2
#define CONTROL_PINS (BUTTON_OCTAVE | BUTTON_RECORD)

// ADC potentiometer input
#define POT_ADC_MAX 4095u
#define VOLUME_MIN_PERCENT 5u
#define VOLUME_MAX_PERCENT 100u
// Potentiometer is used for buzzer volume control

// Recording settings
#define MAX_RECORDING_NOTES 64
#define RECORDED_NOTE_DURATION_MS 110
#define PLAYBACK_GAP_MS 8

// Recording button phases
#define RECORD_PHASE_READY 0
#define RECORD_PHASE_RECORDING 1
#define RECORD_PHASE_READY_TO_PLAY 2

// MPR121 Definitions
#define MPR121_ADDR 0x5A
#define MPR_TOUCH_STATUS_L 0x00
#define MPR_TOUCH_STATUS_H 0x01
#define MPR_ECR 0x5E
#define MPR_SOFTRESET 0x80

// ============================================================================
// DELAY FUNCTIONS
// ============================================================================

// Simple cycle delay
// At 32MHz, 32000 cycles is roughly 1ms
void delay_cycles(uint32_t cycles) {
    while (cycles--) {
        __asm("nop");
    }
}

void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_cycles(32000);
    }
}

// ============================================================================
// GPIO INITIALIZATION
// ============================================================================

void GPIO_Init(void) {
    // Unlock and enable power to Port A
    GPIOA->GPRCM.RSTCTL = GPIO_RSTCTL_KEY_UNLOCK_W |
                          GPIO_RSTCTL_RESETSTKYCLR_CLR |
                          GPIO_RSTCTL_RESETASSERT_ASSERT;

    GPIOA->GPRCM.PWREN = GPIO_PWREN_KEY_UNLOCK_W |
                         GPIO_PWREN_ENABLE_ENABLE;

    delay_cycles(16); // Power startup delay

    // Configure PINCM for LEDs (Output)
    // 0x81 = PC (bit 7) + GPIO function (bit 0)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM10] = 0x81; // PA5 LED1
    IOMUX->SECCFG.PINCM[IOMUX_PINCM11] = 0x81; // PA6 LED2
    IOMUX->SECCFG.PINCM[IOMUX_PINCM14] = 0x81; // PA7 LED3
    IOMUX->SECCFG.PINCM[IOMUX_PINCM19] = 0x81; // PA8 LED4
    IOMUX->SECCFG.PINCM[IOMUX_PINCM20] = 0x81; // PA9 LED5
    IOMUX->SECCFG.PINCM[IOMUX_PINCM21] = 0x81; // PA10 LED6
    IOMUX->SECCFG.PINCM[IOMUX_PINCM22] = 0x81; // PA11 LED7
    IOMUX->SECCFG.PINCM[IOMUX_PINCM34] = 0x81; // PA12 LED8

    // Configure PINCM for Buzzer (Output)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM47] = 0x81; // PA22

    // Configure PINCM for MPR121 IRQ (Input)
    // CRITICAL FIX: Must set INENA (bit 18) to enable the input buffer!
    // 0x40081 = INENA (bit 18) + PC (bit 7) + GPIO function (bit 0)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM35] = 0x40081; // PA13

    // Configure PINCM for Soft I2C (Open-Drain style)
    // Also need INENA (bit 18) so we can read SDA/SCL state
    IOMUX->SECCFG.PINCM[IOMUX_PINCM1] = 0x40081; // PA0 SDA
    IOMUX->SECCFG.PINCM[IOMUX_PINCM2] = 0x40081; // PA1 SCL

    // Configure user buttons as GPIO inputs with pull-ups
    // 0x60081 = pull-up + input enable + peripheral connected + GPIO function
    // PA26 is PINCM59 and PA25 is PINCM55 on MSPM0G3507
    IOMUX->SECCFG.PINCM[IOMUX_PINCM59] = 0x60081; // PA26 / BUTTON1 / octave
    IOMUX->SECCFG.PINCM[IOMUX_PINCM55] = 0x60081; // PA25 / BUTTON2 / record

    // Configure PA27/POT for analog use
    // For analog functions, PINCM.PF and PINCM.PC should be 0, 
    // so do not connect it as a digital GPIO
    IOMUX->SECCFG.PINCM[IOMUX_PINCM60] = 0x00000000; // PA27 / A0_0

    // Set output values to 0 initially
    GPIOA->DOUTCLR31_0 = LED_ALL | BUZZER | SDA_PIN | SCL_PIN;

    // Enable output drivers for LEDs and Buzzer
    GPIOA->DOESET31_0 = LED_ALL | BUZZER;

    // Disable output drivers for Inputs and Open-Drain I2C
    GPIOA->DOECLR31_0 = IRQ_PIN | SDA_PIN | SCL_PIN | CONTROL_PINS;
}

// ============================================================================
// SOFTWARE I2C IMPLEMENTATION
// ============================================================================

static void i2c_delay(void) {
    delay_cycles(120); // ~133 kHz at 32MHz
}

static void SDA_Release(void) {
    GPIOA->DOECLR31_0 = SDA_PIN;
}

static void SDA_Low(void) {
    GPIOA->DOUTCLR31_0 = SDA_PIN;
    GPIOA->DOESET31_0 = SDA_PIN;
}

static void SCL_Release(void) {
    GPIOA->DOECLR31_0 = SCL_PIN;
}

static void SCL_Low(void) {
    GPIOA->DOUTCLR31_0 = SCL_PIN;
    GPIOA->DOESET31_0 = SCL_PIN;
}

static uint8_t SDA_Read(void) {
    return (GPIOA->DIN31_0 & SDA_PIN) ? 1 : 0;
}

static void I2C_Start(void) {
    SDA_Release();
    SCL_Release();
    i2c_delay();
    SDA_Low();
    i2c_delay();
    SCL_Low();
    i2c_delay();
}

static void I2C_Stop(void) {
    SDA_Low();
    i2c_delay();
    SCL_Release();
    i2c_delay();
    SDA_Release();
    i2c_delay();
}

static uint8_t I2C_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) {
            SDA_Release();
        } else {
            SDA_Low();
        }
        i2c_delay();
        SCL_Release();
        i2c_delay();
        SCL_Low();
        data <<= 1;
    }

    SDA_Release();
    i2c_delay();
    SCL_Release();
    i2c_delay();

    uint8_t ack = !SDA_Read();

    SCL_Low();
    i2c_delay();

    return ack;
}

static uint8_t I2C_ReadByte(uint8_t ack) {
    uint8_t data = 0;
    SDA_Release();

    for (int i = 0; i < 8; i++) {
        data <<= 1;
        SCL_Release();
        i2c_delay();
        if (SDA_Read()) {
            data |= 1;
        }
        SCL_Low();
        i2c_delay();
    }

    if (ack) {
        SDA_Low();
    } else {
        SDA_Release();
    }

    i2c_delay();
    SCL_Release();
    i2c_delay();
    SCL_Low();
    SDA_Release();

    return data;
}

uint8_t SoftI2C_WriteReg(uint8_t dev, uint8_t reg, uint8_t val) {
    I2C_Start();
    if (!I2C_WriteByte((dev << 1) | 0)) return 0; // NACK
    if (!I2C_WriteByte(reg)) return 0;
    if (!I2C_WriteByte(val)) return 0;
    I2C_Stop();
    return 1; // Success
}

uint8_t SoftI2C_ReadReg(uint8_t dev, uint8_t reg, uint8_t *val) {
    I2C_Start();
    if (!I2C_WriteByte((dev << 1) | 0)) return 0;
    if (!I2C_WriteByte(reg)) return 0;

    I2C_Start(); // Repeated start
    if (!I2C_WriteByte((dev << 1) | 1)) return 0;
    *val = I2C_ReadByte(0); // NACK on last byte
    I2C_Stop();
    return 1;
}

// ============================================================================
// MPR121 SENSOR FUNCTIONS
// ============================================================================

uint8_t MPR121_Init(void) {
    // Soft reset
    if (!SoftI2C_WriteReg(MPR121_ADDR, MPR_SOFTRESET, 0x63)) return 0;
    delay_ms(5);

    // Stop mode before writing config registers
    if (!SoftI2C_WriteReg(MPR121_ADDR, MPR_ECR, 0x00)) return 0;

    // Baseline/filter configuration
    SoftI2C_WriteReg(MPR121_ADDR, 0x2B, 0x01);
    SoftI2C_WriteReg(MPR121_ADDR, 0x2C, 0x01);
    SoftI2C_WriteReg(MPR121_ADDR, 0x2D, 0x00);
    SoftI2C_WriteReg(MPR121_ADDR, 0x2E, 0x00);
    SoftI2C_WriteReg(MPR121_ADDR, 0x2F, 0x01);
    SoftI2C_WriteReg(MPR121_ADDR, 0x30, 0x01);
    SoftI2C_WriteReg(MPR121_ADDR, 0x31, 0xFF);
    SoftI2C_WriteReg(MPR121_ADDR, 0x32, 0x02);
    SoftI2C_WriteReg(MPR121_ADDR, 0x33, 0x01);
    SoftI2C_WriteReg(MPR121_ADDR, 0x34, 0x01);
    SoftI2C_WriteReg(MPR121_ADDR, 0x35, 0x00);

    // Touch/release thresholds for ELE0-ELE11
    for (uint8_t i = 0; i < 12; i++) {
        SoftI2C_WriteReg(MPR121_ADDR, 0x41 + 2*i, 12); // touch threshold
        SoftI2C_WriteReg(MPR121_ADDR, 0x42 + 2*i, 6);  // release threshold
    }

    // Debounce
    SoftI2C_WriteReg(MPR121_ADDR, 0x5B, 0x11);

    // Global CDC/CDT filter settings
    SoftI2C_WriteReg(MPR121_ADDR, 0x5C, 0x10);
    SoftI2C_WriteReg(MPR121_ADDR, 0x5D, 0x24);

    // Enable ELE0-ELE11.
    // 0x8F = baseline tracking loaded from first samples + all 12 electrodes enabled
    if (!SoftI2C_WriteReg(MPR121_ADDR, MPR_ECR, 0x8F)) return 0;

    delay_ms(100);
    return 1; // Success
}

uint8_t MPR121_ReadTouchStatus(uint16_t *status) {
    uint8_t low = 0, high = 0;
    if (!SoftI2C_ReadReg(MPR121_ADDR, MPR_TOUCH_STATUS_L, &low)) return 0;
    if (!SoftI2C_ReadReg(MPR121_ADDR, MPR_TOUCH_STATUS_H, &high)) return 0;

    *status = ((uint16_t)(high & 0x0F) << 8) | low;
    return 1;
}

// ============================================================================
// AUDIO & LED CONTROL
// ============================================================================

// Frequencies for B Major Scale (B4 to B5)
// Half-period cycles = 32,000,000 / (2 * Frequency)
const uint32_t note_cycles[8] = {
    32394, // Key 1: B4  (493.88 Hz)
    28853, // Key 2: C#5 (554.37 Hz)
    25714, // Key 3: D#5 (622.25 Hz)
    24268, // Key 4: E5  (659.25 Hz)
    21622, // Key 5: F#5 (739.99 Hz)
    19264, // Key 6: G#5 (830.61 Hz)
    17160, // Key 7: A#5 (932.33 Hz)
    16197  // Key 8: B5  (987.77 Hz)
};

const uint32_t led_pins[8] = {
    LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8
};

uint8_t ReadVolumePercent(void);

void PlayTone(uint32_t half_period_cycles, uint32_t duration_ms) {
    uint32_t total_cycles = (duration_ms * 32000);
    uint32_t elapsed = 0;

    uint32_t period_cycles = half_period_cycles * 2;
    uint8_t volume_percent = ReadVolumePercent();

    // Volume is controlled by changing the buzzer duty cycle.
    // 100% volume keeps the original 50% duty cycle.
    // Lower volume keeps the same pitch but makes the HIGH time shorter.
    uint32_t high_cycles = (half_period_cycles * volume_percent) / 100;
    uint32_t low_cycles = period_cycles - high_cycles;

    if (high_cycles < 100) {
        high_cycles = 100;
        low_cycles = period_cycles - high_cycles;
    }

    while (elapsed < total_cycles) {
        GPIOA->DOUTSET31_0 = BUZZER;
        delay_cycles(high_cycles);
        GPIOA->DOUTCLR31_0 = BUZZER;
        delay_cycles(low_cycles);
        elapsed += period_cycles;
    }
}

// ============================================================================
// ADC POTENTIOMETER / VOLUME CONTROL
// ============================================================================

void ADC_Init(void) {
    DL_ADC12_enablePower(ADC0);
    delay_cycles(16);
    DL_ADC12_reset(ADC0);
    delay_cycles(16);

    DL_ADC12_ClockConfig adcClockConfig = {
        .clockSel = DL_ADC12_CLOCK_SYSOSC,
        .divideRatio = DL_ADC12_CLOCK_DIVIDE_8,
        .freqRange = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32
    };

    DL_ADC12_setClockConfig(ADC0, &adcClockConfig);
    DL_ADC12_initSingleSample(ADC0,
                              DL_ADC12_REPEAT_MODE_DISABLED,
                              DL_ADC12_SAMPLING_SOURCE_AUTO,
                              DL_ADC12_TRIG_SRC_SOFTWARE,
                              DL_ADC12_SAMP_CONV_RES_12_BIT,
                              DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_setSampleTime0(ADC0, 32);
    DL_ADC12_configConversionMem(ADC0,
                                 DL_ADC12_MEM_IDX_0,
                                 DL_ADC12_INPUT_CHAN_0, // PA27 / A0_0
                                 DL_ADC12_REFERENCE_VOLTAGE_VDDA,
                                 DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
                                 DL_ADC12_AVERAGING_MODE_DISABLED,
                                 DL_ADC12_BURN_OUT_SOURCE_DISABLED,
                                 DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
                                 DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_enableConversions(ADC0);
}

uint16_t ADC_ReadPotRaw(void) {
    DL_ADC12_startConversion(ADC0);
    while (DL_ADC12_isConversionStarted(ADC0)) {
        // Wait for single conversion to finish
    }

    uint16_t value = DL_ADC12_getMemResult(ADC0, DL_ADC12_MEM_IDX_0);

    // Re-enable conversions for the next software-triggered sample
    DL_ADC12_enableConversions(ADC0);
    return value;
}

uint8_t ReadVolumePercent(void) {
    uint16_t raw = ADC_ReadPotRaw();

    uint32_t volume = VOLUME_MIN_PERCENT +
        (((uint32_t) raw * (VOLUME_MAX_PERCENT - VOLUME_MIN_PERCENT)) / POT_ADC_MAX);

    if (volume > VOLUME_MAX_PERCENT) {
        volume = VOLUME_MAX_PERCENT;
    }

    return (uint8_t) volume;
}

// ============================================================================
// BUTTONS, OCTAVE, AND RECORDING STATE
// ============================================================================

typedef struct {
    uint8_t key_index;    // 0 to 7
    int8_t octave_offset; // -1, 0, or +1
} RecordedNote;

static RecordedNote recording[MAX_RECORDING_NOTES];
static uint8_t recording_count = 0;
static uint8_t recording_enabled = 0;
static uint8_t playback_requested = 0;
static uint8_t record_phase = RECORD_PHASE_READY;
static int8_t octave_offset = 0;
static uint8_t prev_octave_pressed = 0;
static uint8_t prev_record_pressed = 0;

static uint8_t ButtonPressed(uint32_t button_pin) {
    return (GPIOA->DIN31_0 & button_pin) ? 0 : 1; // active-low
}

static void FlashAllLEDs(uint8_t times, uint32_t delay_time_ms) {
    for (uint8_t i = 0; i < times; i++) {
        GPIOA->DOUTSET31_0 = LED_ALL;
        delay_ms(delay_time_ms);
        GPIOA->DOUTCLR31_0 = LED_ALL;
        delay_ms(delay_time_ms);
    }
}

static void HandleButtons(void) {
    uint8_t octave_pressed = ButtonPressed(BUTTON_OCTAVE);
    uint8_t record_pressed = ButtonPressed(BUTTON_RECORD);

    // BUTTON1: cycle octave through 0, +1, -1, then back to 0
    if (octave_pressed && !prev_octave_pressed) {
        if (octave_offset == 0) {
            octave_offset = 1;
        } else if (octave_offset == 1) {
            octave_offset = -1;
        } else {
            octave_offset = 0;
        }
        FlashAllLEDs((uint8_t)(octave_offset + 2), 50);
        delay_ms(40); // debounce
    }

    // BUTTON2:
    // Phase 1: start a fresh recording and clear the old recording
    // Phase 2: stop recording without playing back immediately
    // Phase 3: play the saved recording
    // Phase 4: next press starts a new recording again
    if (record_pressed && !prev_record_pressed) {
        if (record_phase == RECORD_PHASE_READY) {
            playback_requested = 0;
            recording_enabled = 1;
            recording_count = 0;
            record_phase = RECORD_PHASE_RECORDING;
            FlashAllLEDs(2, 30); // recording started
        } else if (record_phase == RECORD_PHASE_RECORDING) {
            recording_enabled = 0;
            record_phase = RECORD_PHASE_READY_TO_PLAY;
            FlashAllLEDs(3, 30); // recording stopped
        } else if (record_phase == RECORD_PHASE_READY_TO_PLAY) {
            playback_requested = 1;
            record_phase = RECORD_PHASE_READY;
            FlashAllLEDs(1, 30); // playback requested
        }
        delay_ms(50); // debounce
    }

    prev_octave_pressed = octave_pressed;
    prev_record_pressed = record_pressed;
}

static uint32_t ApplyOctave(uint32_t base_half_period_cycles, int8_t octave) {
    if (octave > 0) {
        return base_half_period_cycles >> 1; // one octave up = 2x frequency
    } else if (octave < 0) {
        return base_half_period_cycles << 1; // one octave down = 1/2 frequency
    }
    return base_half_period_cycles;
}

static void RecordKey(uint8_t key_index) {
    if (recording_enabled && recording_count < MAX_RECORDING_NOTES) {
        recording[recording_count].key_index = key_index;
        recording[recording_count].octave_offset = octave_offset;
        recording_count++;
    }
}

void PlayRecordedSong(void);

// ============================================================================
// MAIN APPLICATION
// ============================================================================

int main(void) {
    GPIO_Init();

    // Diagnostic: Startup LED Chase
    for (int i = 0; i < 8; i++) {
        GPIOA->DOUTSET31_0 = led_pins[i];
        delay_ms(100);
        GPIOA->DOUTCLR31_0 = led_pins[i];
    }

    // Initialize MPR121
    if (!MPR121_Init()) {
        // Diagnostic: Init Failed
        // Blink LED1 and LED8 alternately
        while (1) {
            GPIOA->DOUTSET31_0 = LED1;
            GPIOA->DOUTCLR31_0 = LED8;
            delay_ms(250);
            GPIOA->DOUTCLR31_0 = LED1;
            GPIOA->DOUTSET31_0 = LED8;
            delay_ms(250);
        }
    }

    // Diagnostic: Init Success
    // Blink all LEDs twice
    for (int i = 0; i < 2; i++) {
        GPIOA->DOUTSET31_0 = LED_ALL;
        delay_ms(200);
        GPIOA->DOUTCLR31_0 = LED_ALL;
        delay_ms(200);
    }

    ADC_Init();

    uint16_t touch_status = 0;
    uint16_t previous_touch_status = 0;

    while (1) {
        HandleButtons();
        
        if (playback_requested) {
            playback_requested = 0;
            PlayRecordedSong();
        }

        // We will read the touch status CONTINUOUSLY instead of relying solely on the IRQ pin
        // This eliminates the IRQ pin as a potential point of failure
        // If the I2C read succeeds, we process the touch status
        
        if (MPR121_ReadTouchStatus(&touch_status)) {
            
            // Clear all LEDs first
            GPIOA->DOUTCLR31_0 = LED_ALL;

            // Check ELE4 to ELE11 (Bits 4 to 11)
            uint8_t any_key_touched = 0;
            
            for (int i = 0; i < 8; i++) {
                uint16_t key_mask = (1u << (i + 4));

                if (touch_status & key_mask) {
                    // Key is touched!
                    any_key_touched = 1;
                    
                    // Only record the key once when it first becomes touched.
                    if (!(previous_touch_status & key_mask)) {
                        RecordKey((uint8_t)i);
                    }

                    // 1. Turn on corresponding LED
                    GPIOA->DOUTSET31_0 = led_pins[i];
                    
                    // 2. Play corresponding note for a short duration
                    PlayTone(ApplyOctave(note_cycles[i], octave_offset), 100); 
                }
            }
            
            previous_touch_status = touch_status;

            // If no keys are touched, we just wait a bit before polling again
            if (!any_key_touched) {
                delay_ms(20);
            }
            
        } else {
            // Diagnostic: I2C Read Failed during loop
            // Blink LED1 and LED2 rapidly to indicate read error
            GPIOA->DOUTCLR31_0 = LED_ALL;
            GPIOA->DOUTSET31_0 = LED1 | LED2;
            delay_ms(100);
            GPIOA->DOUTCLR31_0 = LED1 | LED2;
            delay_ms(100);
        }
    }
}


void PlayRecordedSong(void) {
    GPIOA->DOUTCLR31_0 = LED_ALL;

    for (uint8_t i = 0; i < recording_count; i++) {
        uint8_t key_index = recording[i].key_index;
        int8_t recorded_octave = recording[i].octave_offset;

        uint32_t playback_duration = RECORDED_NOTE_DURATION_MS;
        uint32_t playback_gap = PLAYBACK_GAP_MS;

        if (key_index < 8) {
            GPIOA->DOUTSET31_0 = led_pins[key_index];

            PlayTone(ApplyOctave(note_cycles[key_index], recorded_octave),
                     playback_duration);

            GPIOA->DOUTCLR31_0 = led_pins[key_index];
            delay_ms(playback_gap);
        }
    }
}

