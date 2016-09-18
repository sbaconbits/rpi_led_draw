; Copyright (c) 2016 Steven Bacon
;
; Permission is hereby granted, free of charge, to any person obtaining a copy of
; this software and associated documentation files (the "Software"), to deal in
; the Software without restriction, including without limitation the rights to
; use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
; of the Software, and to permit persons to whom the Software is furnished to do
; so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;
; File Name: leds.S
;

; http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_reg_usage
; Call-used registers (r18-r27, r30-r31)
; Call-saved registers (r2-r17, r28-r29)
; Fixed registers (r0, r1)

;***************************
; FUNCTION
;
; - input param in r25:r24 (hi:lo)
; - X is indirect address register (r27:r26)
; - r22 all high
; - r0 all low
; - r18 bit position
; - r20 bit data
; - r21 byte position
;***************************

#include <avr/io.h>


.global asm_send_led_data
asm_send_led_data:
            clr r0
            ser r22

            mov r27, r25                    ; Copy passed param into X
            mov r26, r24                    ;

            ldi r21, 180                    ; byte loop counter
 BYTE_LOOP:
            ld  r20, X+                     ; load data from memory into register, post increment address.
            ldi r18, 8                      ; bit loop counter
 NEXT_BIT:
            sbi _SFR_IO_ADDR(PORTC), 5      ; Set bit 5, PC5. Take two cycles
            lsl r20                         ; logical shift next bit of data into carry.
            brcs OUT_ONE                    ; branch if carry set
 OUT_ZERO:
            nop
            nop
            cbi _SFR_IO_ADDR(PORTC), 5      ; Clear bit 5, PC5. Take two cycles
            nop
            nop
            nop
            nop
            nop
            nop
            nop
 OUT_ONE:
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            cbi _SFR_IO_ADDR(PORTC), 5      ; Clear bit 5, PC5. Takes two cycles
            nop
            nop
            nop
            nop
            nop
            dec r18                         ; decrement bit loop counter
            brne NEXT_BIT_DELAY
            dec r21
            brne BYTE_LOOP
            rjmp END_FUNC

 NEXT_BIT_DELAY:                            ; delay to match the BYTE_LOOP path
            nop
            nop
            rjmp NEXT_BIT

 END_FUNC:
            clr r1
            ret

