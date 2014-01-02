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












start:


