.586
.model flat, stdcall
.code
MyProc1 proc x: DWORD, y: DWORD
	xor eax,eax
	mov eax,x
	mov ecx,y
	ror ecx,1
	shld eax,ecx,2
	jnc ET1
	mul y
	ret
ET1:mul x
	neg y
	ret
MyProc1 endp


TestProc proc x: DWORD, y: DWORD
	mov eax, x
	mov ebx, [eax]
	inc ebx
	mov [eax], ebx
	ret
TestProc endp
end