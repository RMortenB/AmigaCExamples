	section .text

_Upscale:
    move.l    d2,-(sp)

    move.w	#144-1,d0
.loop1:
    move.w	#5-1,d1
.loop2:
    move.w	#128-1,d2
.loop3:
    load	(a0)+,e0
	vperm	#$01230123,e0,e0,e1
	vperm	#$45674567,e0,e0,e2
	store	e1,(a1)+
	store	e1,(a1)+
	store	e0,(a1)+
	store	e2,(a1)+
	store	e2,(a1)+

    dbra	d2,.loop3

    sub.l	#4*256,a0
    dbra	d1,.loop2

    add.l	#4*256,a0
    dbra	d0,.loop1

    move.l	(sp)+,d2

    rts

	public _Upscale

; void ExpandLine1632(uint32_t *dest __asm("a0"), uint16_t *src __asm("a1"), int len  __asm("d0"));

_ExpandLine1632:

	lsr.l	#2,d0

.loop:
	unpack1632	(a1)+,e0:e1
	store	e0,(a0)+
	store	e1,(a0)+

	subq.l	#1,d0
	bne.s	.loop

	rts

	public _ExpandLine1632
; void Copy16(uint32_t *dest __asm("a0"), uint16_t *src __asm("a1"), int len  __asm("d0"))


_Copy16:
	lsr.l	#2,d0
.loop:
	load	(a1)+,e0
	store	e0,(a0)+

	subq.l	#1,d0
	bne.s	.loop

	rts

	public _Copy16

