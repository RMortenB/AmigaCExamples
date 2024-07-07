	section .text

; int StackSwap(Fiber *fiber __asm("a0"), int rVal __asm("d0"));
_StackSwap:
	movem.l	d2-d7/a2-a6,-(a7)
	fmovem	fp2-fp7,-(a7)
	move.l	a7,a1
	move.l	(a0),a7
	move.l	a1,(a0)
	fmovem	(a7)+,fp2-fp7
	movem.l	(a7)+,d2-d7/a2-a6
	rts
	public _StackSwap

; ULONG *RegisterFill(ULONG *ptr __asm("a0"));
_RegisterFill:
	movem.l	d2-d7/a2-a6,-(a0)
	fmovem	fp2-fp7,-(a0)
	move.l	a0,d0
	rts
	public _RegisterFill
