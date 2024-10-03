/*
 * SK6812.h
 *
 * Created: 15/05/2020 12:33:41
 *
 */ 

#ifndef __SK6812__
#define __SK6812__

typedef struct rgb_colour
{
	unsigned char red, green, blue;
} rgb_colour;

void __attribute__((noinline)) led_strip_write(volatile rgb_colour * colors, unsigned int count)
{
	cli();
	while(count--)
	{
		// Send a color to the LED strip.
		// The assembly below also increments the 'colors' pointer,
		// it will be pointing to the next color at the end of this loop.
		asm volatile(
		"ld __tmp_reg__, %a0+\n"
		"ld __tmp_reg__, %a0\n"
		"rcall send_led_strip_byte%=\n"  // Send red component.
		"ld __tmp_reg__, -%a0\n"
		"rcall send_led_strip_byte%=\n"  // Send green component.
		"ld __tmp_reg__, %a0+\n"
		"ld __tmp_reg__, %a0+\n"
		"ld __tmp_reg__, %a0+\n"
		"rcall send_led_strip_byte%=\n"  // Send blue component.
		"rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

		// send_led_strip_byte subroutine:  Sends a byte to the LED strip.
		"send_led_strip_byte%=:\n"
		"rcall send_led_strip_bit%=\n"  // Send MSB (bit 7).
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"
		"rcall send_led_strip_bit%=\n"  // Send LSB (bit 0).
		"ret\n"

		// send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
		// high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
		// but this function always takes the same time (2 us).
		"send_led_strip_bit%=:\n"
		"sbi 0x05, 5\n"                           // Drive the line high.
		"rol __tmp_reg__\n"                       // Rotate left through carry.
		"nop\n" "nop\n"
		"brcs .+2\n" "cbi 0x05, 5\n"              // If the bit to send is 0, drive the line low now.
		"nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
		"brcc .+2\n" "cbi 0x05, 5\n"              // If the bit to send is 1, drive the line low now.

		"ret\n"
		
		"led_strip_asm_end%=: "
		: "=b" (colors)
		: "0" (colors)                            // %a0 points to the next color to display
		);

		//sei(); asm volatile("nop\n"); cli();
	}
	sei();
	// _delay_us(80);   Send the reset signal.
}

#endif /* SK6812_H_ */
