
/*
cachecpy.s
ARM copy function for cache

Copyright (c) 2020, chyyran
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CHYYRAN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Copies 512 bytes from aligned src to aligned dst
 */
@void cachecpy (const void *src, const void *dst);
.section .text
.align 4
.global cache_cpy
.type	cache_cpy, STT_FUNC
.arm

cache_cpy:
    push {r0-r9}    // Save registers

    mov r8, r0      // Save src in r8
    mov r9, r1      // Save dst in r9

    ldmia r8!, {r0-r7} // 8 words = 32 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 16 words = 64 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 24 words = 96 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 32 words = 128 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 40 words = 160 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 48 words = 192 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 56 words = 96 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 64 words = 256 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 72 words = 288 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 80 words = 320 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 88 words = 352 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 96 words = 384 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 104 words = 416 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 112 words = 448 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 120 words = 480 bytes
    stmia r9!, {r0-r7} 

    ldmia r8!, {r0-r7} // 128 words = 512 bytes
    stmia r9!, {r0-r7} 

    pop	{r0-r9}     // Restore registers
	  bx	lr          // Return
