// Arduino Fairchild Channel F cart dumper 1.0

constexpr uint8_t PHI = 40; // Time for a single clock cycle in microseconds. 0.5 in Channel F
constexpr uint8_t WRITE_PIN = 34;
constexpr uint8_t PHI_PIN = 38;
constexpr uint8_t DBUS_PINS[8] = {51, 49, 39, 35, 36, 32, 30, 28}; //D0 - D7
constexpr uint8_t ROMC_PINS[5] = {45, 43, 41, 37, 40}; //ROMC0 - ROMC4

void setup() {
    Serial.begin(9600);

    // Initialize cartridge pins
    digitalWrite(PHI_PIN, LOW);
    pinMode(PHI_PIN, OUTPUT);

    digitalWrite(WRITE_PIN, LOW);
    pinMode(WRITE_PIN, OUTPUT);

    for (uint8_t i = 0; i < 8; i++) {
        pinMode(DBUS_PINS[i], INPUT_PULLUP); // Default to high impedance
    }
    for (uint8_t i = 0; i < 5; i++) {
        digitalWrite(ROMC_PINS[i], LOW); // Default to ROMC 0
        pinMode(ROMC_PINS[i], OUTPUT);
    }

    test_videocart(); 
}

void loop() {}



// Sets the current ROMC command
void set_romc(uint8_t new_romc) {
    for (uint8_t i = 0; i < 5; i++) {
        digitalWrite(ROMC_PINS[i], bitRead(new_romc, i));
    }
}

// Retrieves the current value on the data bus
uint8_t read_dbus(void) {
    uint8_t dbus_data;
    for (uint8_t i = 0; i < 8; i++) {
        bitWrite(dbus_data, i, digitalRead(DBUS_PINS[i]));
    }
    return dbus_data;
}

// Execute a ROMC command that puts data on the data bus
void execute_out(uint8_t romc, uint8_t data) { // Short cycle (4 ticks)
    // After 1 tick, output the ROMC command
    clock_tick(1);
    set_romc(romc);

    // 3 ticks before the cycle ends, write to the data bus
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(DBUS_PINS[i], bitRead(data, i));
        pinMode(DBUS_PINS[i], OUTPUT);
    }
    clock_tick(2);
    
    // Toggle WRITE on the last tick
    digitalWrite(WRITE_PIN, HIGH);
    clock_tick(1);
    digitalWrite(WRITE_PIN, LOW);

    // Free the data bus
    for (uint8_t i = 0; i < 8; i++) {
        pinMode(DBUS_PINS[i], INPUT_PULLUP);
    }
}

// Execute a ROMC command that recieves data from the data bus
uint8_t execute_in(uint8_t romc) { // Short cycle (4 ticks)
    // After 1 tick, output the ROMC command
    clock_tick(1);
    set_romc(romc);
    
    clock_tick(2);
    
    // Toggle WRITE on the last tick
    digitalWrite(WRITE_PIN, HIGH);
    clock_tick(1);
    uint8_t data = read_dbus(); // Data bus will be valid when WRITE is high
    digitalWrite(WRITE_PIN, LOW);
    
    return data;
}


// Execute one or more clock cycles
void clock_tick(uint8_t repeat) {
    for (uint8_t i = 0; i < repeat; i++) {
      digitalWrite(PHI_PIN, HIGH);
      delayMicroseconds(PHI / 2);
      digitalWrite(PHI_PIN, LOW);
      delayMicroseconds(PHI / 2);
    }
}

// Output ROM data from PC to PC + 4K 
void read_ROM(uint8_t romc_fetch) {
    char dataString[4];
    char addrString[7];
    Serial.print("\n0000:");
    for (uint16_t addr = 0; addr < 0x1000; addr++) { // Assume 4K ROM
        sprintf(dataString, " %02X", execute_in(romc_fetch));
        Serial.print(dataString);
        if ((addr & 0xF) == 0xF && addr != 0x1000 - 1) {
            sprintf(addrString, "\n%04X:", addr + 1 + 0x800);
            Serial.print(addrString);
        }
    }
}

// Range is [0x08 - 0x18) to stay in 4K Videocart address space
uint8_t random_data[16] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};

// Verify the read/write function of a register
void register_test(uint8_t romc_write, uint8_t romc_read) {
    for (uint8_t i = 0; i < sizeof(random_data); i++) {
        execute_out(romc_write, random_data[i]);
        if (execute_in(romc_read) == random_data[i]) {
            Serial.print(".");
        } else {
            Serial.print("X");
        }
    }
    Serial.print("\n");
}

// Run multiple test cases to verify videocart functionality
void test_videocart(void) {
  
    
    // ROM read test 1: ROMC 0x00, 0x08
    Serial.println("ROM read test 1:");
    execute_in(0x08); // Load the data bus into both halves of PC0. Sets PC0 to 0xFFFF
    for (uint16_t i = 0; i < 0x801; i++) { // Increment PC0 until it's 0x0800
        execute_in(0x00);
    }
    read_ROM(0x00);
  
    
    // ROM read test 2: ROMC 0x00, 0x14, 0x17
    Serial.println("ROM read test 2:");
    execute_out(0x14, 0x08); // Load the data bus into the high order byte of PC0. Sets PC0 to 0x08??
    execute_out(0x17, 0x00); // Load the data bus into the low order byte of PC0. Sets PC0 to 0x0800
    read_ROM(0x00);
    
    
    // ROM read test 3: ROMC 0x03, 0x08
    Serial.println("ROM read test 3:");
    execute_in(0x08); // Load the data bus into both halves of PC0. Sets PC0 to 0xFFFF
    for (uint16_t i = 0; i < 0x801; i++) { // Increment PC0 until it's 0x0800
        execute_in(0x03);
    }
    read_ROM(0x03);

  
    // ROM read test 4: ROMC 0x03, 0x14, 0x17
    Serial.println("ROM read test 4:");
    execute_out(0x14, 0x08); // Load the data bus into the high order byte of PC0. Sets PC0 to 0x08??
    execute_out(0x17, 0x00); // Load the data bus into the low order byte of PC0. Sets PC0 to 0x0800
    read_ROM(0x03);
    
    // Register read/write test: ROMC 0x06, 0x07, 0x09, 0x0B, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1E, 0x1F
    Serial.println("Register read/write test:");
    Serial.print("  PC0H ");
    register_test(0x14, 0x1F); // 0x14  high PC0 = dbus; 0x1F  dbus = high PC0
    Serial.print("  PC0L ");
    register_test(0x17, 0x1E); // 0x17  low PC0 = dbus; 0x1E  dbus = low PC0
    Serial.print("  PC1H ");
    register_test(0x15, 0x07); // 0x15  high PC1 = dbus; 0x07  dbus = high PC1
    Serial.print("  PC1L ");
    register_test(0x18, 0x0B); // 0x18  low PC1 = dbus; 0x0B  dbus = low PC1
    Serial.print("  DC0H ");
    register_test(0x16, 0x06); // 0x16  high DC0 = dbus; 0x06  dbus = high DC0
    Serial.print("  DC0L ");
    register_test(0x19, 0x09); // 0x19  low DC0 = dbus; 0x09  dbus = low DC0
    
    // PC copy test: ROMC 0x04, 0x12, 0x0D
    Serial.println("PC copy test:");
    execute_out(0x14, 0x08); // 0x14  high PC0 = dbus
    execute_out(0x17, 0x09); // 0x17  low PC0 = dbus
    execute_out(0x15, 0x0A); // 0x15  high PC1 = dbus
    execute_out(0x18, 0x0B); // 0x18  low PC1 = dbus
    execute_in(0x04);        // 0x04  PC0 = PC1
    if (execute_in(0x1F) == 0x0A) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    if (execute_in(0x1E) == 0x0B) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    if (execute_in(0x07) == 0x0A) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    if (execute_in(0x0B) == 0x0B) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    
    // DC Swap test: ROMC 0x1D
    Serial.println("Swap test:");
    execute_out(0x16, 0x08); // 0x16  high DC0 = 0x8
    execute_out(0x19, 0x09); // 0x19  low DC0 = 0x9
    execute_in(0x1D);        // Swap DC0 & DC1
    execute_out(0x16, 0x0A); // 0x16  high DC0 = 0xA
    execute_out(0x19, 0x0B); // 0x19  low DC0 = 0xB
    execute_in(0x1D);        // Swap DC0 & DC1
    if (execute_in(0x06) == 0x08) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    if (execute_in(0x09) == 0x09) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    execute_in(0x1D);        // Swap DC0 & DC1
    if (execute_in(0x06) == 0x0A) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }
    if (execute_in(0x09) == 0x0B) {
        Serial.println("  PASSED");
    } else {
        Serial.println("  FAILED");
    }

}
