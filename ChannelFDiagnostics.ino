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
void register_test(const char* name, uint8_t romc_write, uint8_t romc_read) {
    Serial.print(name);
    for (uint8_t i = 0; i < sizeof(random_data); i++) {
        execute_out(romc_write, random_data[i]); // Write data
        if (execute_in(romc_read) == random_data[i]) { // Read data
            Serial.print(".");
        } else {
            Serial.print("X");
        }
    }
    Serial.print("\n");
}

void set_PC0(uint8_t value, bool low_high) {
    if (low_high) {
        execute_out(0x14, value);
    } else {
        execute_out(0x17, value);
    }
}

void set_PC1(uint8_t value, bool low_high) {
    if (low_high) {
        execute_out(0x15, value);
    } else {
        execute_out(0x18, value);
    }
}

void set_DC0(uint8_t value, bool low_high) {
    if (low_high) {
        execute_out(0x16, value);
    } else {
        execute_out(0x19, value);
    }
}

uint8_t get_PC0(bool low_high) {
    if (low_high) {
        return execute_in(0x1F);
    } else {
        return execute_in(0x1E);
    }
}

uint8_t get_DC0(bool low_high) {
    if (low_high) {
        return execute_in(0x06);
    } else {
        return execute_in(0x09);
    }
}

void reset_PC0(void) {
    execute_out(0x14, 0x08); // PC0 = 0x08??
    execute_out(0x17, 0x00); // PC0 = 0x0800
}

void reset_PC1(void) {
    execute_out(0x15, 0x08); // PC1 = 0x08??
    execute_out(0x18, 0x00); // PC1 = 0x0800
}

void reset_DC0(void) {
    execute_out(0x16, 0x08); // DC0 = 0x08??
    execute_out(0x19, 0x00); // DC0 = 0x0800
}

void _assert(const char* name, uint8_t expected, uint8_t actual) {
    _assert_16(name, (uint16_t) expected, (uint16_t) actual);
}

void _assert_16(const char* name, uint16_t expected, uint16_t actual) {
    Serial.print(name);
    if (actual == expected) {
        Serial.println("  PASSED");
    } else {
        Serial.print("  FAILED: ");
        Serial.print(actual);
        Serial.print(" but expected ");
        Serial.println(expected);
    }
}

// Run multiple test cases to verify videocart functionality
void test_videocart(void) {
    uint8_t data_at_PC0;
    uint16_t DC0;
    uint16_t PC0;

  
    // ROM read test 1: ROMC 0x00, 0x08
    Serial.println("ROM read test 1:");
    execute_in(0x08);                      // PC0 = 0xFFFF
    for (uint16_t i = 0; i < 0x801; i++) { // Increment PC0 until it's 0x0800
        execute_in(0x00);
    }
    read_ROM(0x00);

  
    // ROM read test 2: ROMC 0x00, 0x14, 0x17
    Serial.println("ROM read test 2:");
    reset_PC0(); // PC0 = 0x0800
    read_ROM(0x00);


    // ROM read test 3: ROMC 0x03, 0x08
    Serial.println("ROM read test 3:");
    execute_in(0x08);                      // PC0 = 0xFFFF
    for (uint16_t i = 0; i < 0x801; i++) { // Increment PC0 until it's 0x0800
        execute_in(0x03);
    }
    read_ROM(0x03);

  
    // ROM read test 4: ROMC 0x03, 0x14, 0x17
    Serial.println("ROM read test 4:");
    reset_PC0(); // PC0 = 0x0800
    read_ROM(0x03);


    // Register read/write test: ROMC 0x06, 0x07, 0x09, 0x0B, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1E, 0x1F
    Serial.println("Register read/write test:");
    reset_PC0(); // PC0 = 0x0800
    register_test("  PC0H ", 0x14, 0x1F);
    reset_PC0(); // PC0 = 0x0800
    register_test("  PC0L ", 0x17, 0x1E);
    reset_PC1(); // PC1 = 0x0800
    register_test("  PC1H ", 0x15, 0x07);
    reset_PC1(); // PC1 = 0x0800
    register_test("  PC1L ", 0x18, 0x0B);
    reset_DC0(); // DC0 = 0x0800
    register_test("  DC0H ", 0x16, 0x06);
    reset_DC0(); // DC0 = 0x0800
    register_test("  DC0L ", 0x19, 0x09);

    
    // PC copy test: ROMC 0x04
    Serial.println("PC copy test:");
    set_PC0(0x08, HIGH);
    set_PC0(0x09, LOW);
    set_PC1(0x0A, HIGH);
    set_PC1(0x0B, LOW);
    execute_in(0x04); // PC0 = PC1

    _assert("  PC0H: ", 0x0A, get_PC0(HIGH));
    _assert("  PC0L: ", 0x0B, get_PC0(LOW));

    
    // DC Swap test: ROMC 0x1D
    Serial.println("Swap test:");
    set_DC0(0x08, HIGH);
    set_DC0(0x09, LOW);
    execute_in(0x1D);        // Swap DC0 & DC1
    set_DC0(0x0A, HIGH);
    set_DC0(0x0B, LOW);
    execute_in(0x1D);        // Swap DC0 & DC1

    _assert("  DC0H: ", 0x08, get_DC0(HIGH));
    _assert("  DC0L: ", 0x09, get_DC0(LOW));
    execute_in(0x1D);        // Swap DC0 & DC1
    _assert("  DC1H: ", 0x0A, get_DC0(HIGH));
    _assert("  DC1L: ", 0x0B, get_DC0(LOW));


    // DC0 add test: ROMC 0x0A
    set_DC0(0x08, HIGH);
    set_DC0(0xF3, LOW);
    execute_out(0x0A, 0x13); // DC0 += (signed) 0x13
    DC0 = get_DC0(LOW);
    DC0 = (DC0 & 0x00ff) | (get_DC0(HIGH) << 8);
    _assert_16("DC0 add test: ", 0x08F3 + 0x13, DC0);


    // PC data to Reg test 1: ROMC 0x0E
    reset_PC0();
    reset_DC0();
    data_at_PC0 = execute_in(0x0E);// dbus, DC0 low = [PC0]
    _assert("PC data to Reg test 1: ", get_DC0(LOW), data_at_PC0); 


    // PC data to Reg test 2: ROMC 0x0C
    reset_PC0();
    data_at_PC0 = execute_in(0x0C); // dbus, PC0 low = [PC0]
    _assert("PC data to Reg test 2: ", get_PC0(LOW), data_at_PC0);


    // DC0 indirect addressing tests: ROMC 0x02
    reset_PC0();
    reset_DC0();
    // Note: the order of argument evaluation doesn't matter here
    _assert("DC0 indirect addressing test 1: ", execute_in(0x00), execute_in(0x02)); // 0x00: dbus = [PC0]; PC0++
    _assert("DC0 indirect addressing test 2: ", 0x01, get_DC0(LOW));                 // 0x02: dbus = [DC0]; DC0++


    //SRAM test: ROMC 0x05
    set_DC0(0x28, HIGH);
    set_DC0(0x00, LOW);
    execute_out(0x05, 0x13); // [DC0] = 0x13; DC0++
    set_DC0(0x28, HIGH);
    set_DC0(0x00, LOW);
    _assert("SRAM test:", 0x13, execute_in(0x02)); // 0x02: dbus = [DC0]; DC0++


    // PC0 jump test: ROMC 0x01
    set_PC0(0x08, HIGH);
    set_PC0(0xF3, LOW);
    data_at_PC0 = execute_in(0x00);
    set_PC0(0x08, HIGH);
    set_PC0(0xF3, LOW);
    _assert("PC0 jump test 1:", data_at_PC0,  execute_in(0x01)); // 0x01: dbus = [PC0]; PC0 += [PC0]
    PC0 = get_PC0(LOW);
    PC0 = (PC0 & 0x00ff) | (get_PC0(HIGH) << 8);
    _assert_16("PC0 jump test 2:", 0x08F3 + (int8_t) data_at_PC0, PC0);
    
    Serial.println("\nThat's All Folks!");
}
