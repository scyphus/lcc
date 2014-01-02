global  start

label1:
	add	al,1
	add	ax,2
	add	eax,3
        add	rax,4

	add	cl,1
	add	ch,2
	add	cx,3
	add	ecx,4
	add	rcx,5

	add	cx,0x7f01
	add	ecx,0x7f02
	add	rcx,0x7f03

	add	al,ah
	add	ax,cx
	add	eax,ecx
	add	rax,rcx

	add	al,[rbp]
	add	ax,[rbp]
	add	eax,[rbp]
	add	rax,[rbp]

	add	[rbp],al
	add	[rbp],ax
	add	[rbp],eax
	add	[rbp],rax

label2:
	and	al,1
	and	ax,2
	and	eax,3
        and	rax,4

	and	cl,1
	and	ch,2
	and	cx,3
	and	ecx,4
	and	rcx,5

	and	cx,0x7f01
	and	ecx,0x7f02
	and	rcx,0x7f03

	and	al,ah
	and	ax,cx
	and	eax,ecx
	and	rax,rcx

	and	al,[rbp]
	and	ax,[rbp]
	and	eax,[rbp]
	and	rax,[rbp]

	and	[rbp],al
	and	[rbp],ax
	and	[rbp],eax
	and	[rbp],rax

label3:
	bsf	ax,cx
	bsf	eax,ecx
	bsf	rax,rcx

	bsf	ax,[rbp]
	bsf	eax,[rbp]
	bsf	rax,[rbp]

label4:
	bsr	ax,cx
	bsr	eax,ecx
	bsr	rax,rcx

	bsr	ax,[rbp]
	bsr	eax,[rbp]
	bsr	rax,[rbp]

label5:
	bswap	ecx
	bswap	rcx

label6:
	bt	ax,cx
	bt	eax,ecx
	bt	rax,rcx

	bt	[rbp],ax
	bt	[rbp],eax
	bt	[rbp],rax

	bt	ax,1
	bt	eax,2
	bt	rax,3
	
	bt	word [rbp],4
	bt	dword [rbp],5
	bt	qword [rbp],6

label7:
	btc	ax,cx
	btc	eax,ecx
	btc	rax,rcx

	btc	[rbp],ax
	btc	[rbp],eax
	btc	[rbp],rax

	btc	ax,1
	btc	eax,2
	btc	rax,3
	
	btc	word [rbp],4
	btc	dword [rbp],5
	btc	qword [rbp],6

label8:
	btr	ax,cx
	btr	eax,ecx
	btr	rax,rcx

	btr	[rbp],ax
	btr	[rbp],eax
	btr	[rbp],rax

	btr	ax,1
	btr	eax,2
	btr	rax,3
	
	btr	word [rbp],4
	btr	dword [rbp],5
	btr	qword [rbp],6

label9:
	bts	ax,cx
	bts	eax,ecx
	bts	rax,rcx

	bts	[rbp],ax
	bts	[rbp],eax
	bts	[rbp],rax

	bts	ax,1
	bts	eax,2
	bts	rax,3
	
	bts	word [rbp],4
	bts	dword [rbp],5
	bts	qword [rbp],6

label10:
	cbw
	cwde
	cdqe

label11:
	clc

label12:
	cld

label13:
	clflush	byte [r8]

label14:
	cli

label15:
	cmp	al,1
	cmp	ax,2
	cmp	eax,3
        cmp	rax,4

	cmp	cl,1
	cmp	ch,2
	cmp	cx,3
	cmp	ecx,4
	cmp	rcx,5

	cmp	cx,0x7f01
	cmp	ecx,0x7f02
	cmp	rcx,0x7f03

	cmp	al,ah
	cmp	ax,cx
	cmp	eax,ecx
	cmp	rax,rcx

	cmp	al,[rbp]
	cmp	ax,[rbp]
	cmp	eax,[rbp]
	cmp	rax,[rbp]

	cmp	[rbp],al
	cmp	[rbp],ax
	cmp	[rbp],eax
	cmp	[rbp],rax

label16:
	cmpxchg	al,ah
	cmpxchg	ax,cx
	cmpxchg	eax,ecx
	cmpxchg	rax,rcx

	cmpxchg	[rbp],al
	cmpxchg	[rbp],ax
	cmpxchg	[rbp],eax
	cmpxchg	[rbp],rax

label17:
	cpuid

label18:
	crc32	eax,cl
	crc32	eax,cx
	crc32	eax,ecx
	crc32	rax,cl
	crc32	rax,rcx

	crc32	eax,byte [rbp]
	crc32	eax,word [rbp]
	crc32	eax,dword [rbp]
	crc32	rax,byte [rbp]
	crc32	rax,qword [rbp]

label19:
	cwd
	cdq
	cqo

label20:
	daa

label21:
	das





start:


