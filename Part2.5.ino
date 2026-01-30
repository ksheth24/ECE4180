
void setup() {
  Serial.begin(115200);
  Serial.print("Hello");

  asm volatile(
    "li t0, 0x60091024\n"
    "lw t1, 0(t0)\n"
    "ori t1, t1, 0x80\n"
    "sw t1, 0(t0)\n"

    "li t0, 0x60090014\n"
    "lw t1, 0(t0)\n"
    "ori t1, t1, 0x280\n"
    "sw t1, 0(t0)\n"

    "li t0, 0x60090020\n"
    "lw t1, 0(t0)\n"
    "lui t2, 0x00001\n"
    "or t1, t1, t2\n"
    "sw t1, 0(t0)\n"
  );
}

void loop() {
  asm volatile(
    "li t1, 0x6009103C\n"
    "lw t1, 0(t1)\n"
    "andi t1, t1, 0x10\n"
    "beq x0, t1, else\n"

    "li t0, 0x60091008\n"
    "li t1, 0x80\n"
    "sw t1, 0(t0)\n"
    "jal x0, skipElse\n"

    "else: \n"
    "li t0, 0x6009100C\n"
    "li t1, 0x80\n"
    "sw t1, 0(t0)\n"
    
    "skipElse:\n"
  );
}
